using System;
using System.Collections.Generic;
using System.Collections.Concurrent; // Required for thread-safety
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;

public class QueryEngine
{
    private readonly string _indexDir;
    private readonly Lexicon _lexicon;
    private readonly int _numBarrels;
    private readonly ISAMStorage _dataIndex;

    // 1. Thread-safe Memory Buffer (Delta Index)
    private readonly ConcurrentDictionary<uint, List<InvertedIndex.Posting>> _deltaIndex = new();

    // 2. Thread-safe Cache structures
    private readonly ConcurrentDictionary<string, List<uint>> _cache = new();
    private readonly LinkedList<string> _lruNodes = new();
    private readonly ReaderWriterLockSlim _cacheLock = new(); // Protects the LinkedList
    private const int MaxCacheEntries = 1000;

    public QueryEngine(string indexDir, Lexicon lexicon, int numBarrels, ISAMStorage dataIndex)
    {
        _indexDir = indexDir;
        _lexicon = lexicon;
        _numBarrels = numBarrels;
        _dataIndex = dataIndex;
    }

    public void AddPost(Post post)
    {
        if (post == null || post.PostId <= 0) return;

        CompoundKey ck = new CompoundKey((byte)KeyType.PostById, (ushort)SiteID.AskUbuntu, (uint)post.PostId);
        long packedKey = (long)ck.Pack();

        // Store the post using the PACKED key, not the raw PostId
        _dataIndex.Insert(packedKey, post.Serialize());

        // 2. Tokenize (Ensure your tokenizer lowercases and trims!)
        var titleTokens = _lexicon.Tokenize(post.Title ?? "");
        var bodyTokens = _lexicon.Tokenize(post.CleanedBody ?? "");

        // Use a local dictionary to aggregate scores for THIS post before hitting the global Delta Index
        // This is much faster and more thread-safe than locking the global list repeatedly
        var localDocScores = new Dictionary<uint, int>();

        void ProcessTokenList(IEnumerable<string> tokens, int weight)
        {
            foreach (var token in tokens)
            {
                if (string.IsNullOrWhiteSpace(token)) continue;

                // AddWord MUST be thread-safe internally!
                uint wordId = _lexicon.AddWord(token);
                if (wordId == 0) continue;

                if (!localDocScores.ContainsKey(wordId)) localDocScores[wordId] = 0;
                localDocScores[wordId] += weight;
            }
        }

        ProcessTokenList(titleTokens, 10);
        ProcessTokenList(bodyTokens, 1);

        // 3. Move the local scores into the global Thread-Safe Delta Index
        foreach (var kvp in localDocScores)
        {
            uint wordId = kvp.Key;
            int score = kvp.Value;

            // Get or create the posting list for this word
            var postings = _deltaIndex.GetOrAdd(wordId, _ => new List<InvertedIndex.Posting>());

            lock (postings)
            {
                // Remove existing entry for this DocId if it exists (relevant for updates)
                postings.RemoveAll(p => p.DocId == (uint)post.PostId);
                postings.Add(new InvertedIndex.Posting { DocId = (uint)post.PostId, Score = score });
            }
        }

        // 4. CRITICAL: Clear cache so the new post appears in search results immediately
        InvalidateCache();

        // Console log for your verification (as requested previously)
        Console.WriteLine(
            $" --> [ENGINE] Post {post.PostId} indexed. {localDocScores.Count} unique words added to Delta.");
    }

    public List<uint> Search(string queryText, int limit = 20)
    {
        string normalizedQuery = queryText.Trim().ToLower();
        if (string.IsNullOrWhiteSpace(normalizedQuery)) return new List<uint>();

        // 1. Cache Check (unchanged)
        if (_cache.TryGetValue(normalizedQuery, out var cachedResults))
        {
            RefreshCachePosition(normalizedQuery);
            return cachedResults.Take(limit).ToList();
        }

        var tokens = normalizedQuery.Split(' ', StringSplitOptions.RemoveEmptyEntries);
        // ConcurrentDictionary to allow parallel threads to aggregate scores safely
        var docScores = new ConcurrentDictionary<uint, double>();

        // 2. Parallel Barrel Search
        // This allows the CPU to request data from multiple disk barrels at once
        Parallel.ForEach(tokens, token =>
        {
            uint wordId = _lexicon.GetWordId(token);
            if (wordId == 0) return;

            // A. Search Disk (Merged Fragments)
            var diskPostings = InvertedIndex.SearchWithScores(_indexDir, _numBarrels, wordId);

            // B. Search Memory
            List<InvertedIndex.Posting> livePostings = null;
            if (_deltaIndex.TryGetValue(wordId, out var sharedList))
            {
                lock (sharedList) livePostings = sharedList.ToList();
            }

            // C. Scoring Logic with "Exact Match" / "Rarity" Boost
            // Rare words (short posting lists) get higher weights than common words ("the")
            double idf = Math.Log10(1000000.0 / (diskPostings.Count + (livePostings?.Count ?? 0) + 1));

            void ProcessPosting(InvertedIndex.Posting p)
            {
                // Apply IDF boost to the score
                docScores.AddOrUpdate(p.DocId, p.Score * idf, (id, old) => old + (p.Score * idf));
            }

            foreach (var p in diskPostings) ProcessPosting(p);
            if (livePostings != null)
                foreach (var p in livePostings)
                    ProcessPosting(p);
        });

        if (docScores.IsEmpty) return new List<uint>();

        // 3. Final Ranking with Exact Title Match Boost
        var finalResults = docScores
            .OrderByDescending(kvp => kvp.Value)
            .Take(100) // Take top candidates for heavy re-ranking
            .Select(hit =>
            {
                CompoundKey ck = new CompoundKey((byte)KeyType.PostById, (ushort)SiteID.AskUbuntu, hit.Key);
                var entry = _dataIndex.Get((long)ck.Pack());
                if (entry is null) return new { hit.Key, FinalScore = hit.Value };

                var post = Post.Deserialize(entry);
                double score = hit.Value;

                // HUGE BOOST for exact title match
                if (post.Title != null && post.Title.Equals(queryText, StringComparison.OrdinalIgnoreCase))
                    score += 1000;

                // Substantial boost if all query words appear in the title
                if (tokens.All(t => post.Title?.ToLower().Contains(t) == true))
                    score += 500;

                double popularityBoost = Math.Log10(post.Score + 1) * 2.0;
                return new { hit.Key, FinalScore = score + popularityBoost };
            })
            .OrderByDescending(r => r.FinalScore)
            .Select(r => r.Key)
            .ToList();

        SaveToCache(normalizedQuery, finalResults);
        return finalResults.Take(limit).ToList();
    }

    private void SaveToCache(string query, List<uint> results)
    {
        _cacheLock.EnterWriteLock();
        try
        {
            if (_cache.Count >= MaxCacheEntries)
            {
                if (_lruNodes.Last != null)
                {
                    var oldest = _lruNodes.Last.Value;
                    _cache.TryRemove(oldest, out _);
                    _lruNodes.RemoveLast();
                }
            }

            _cache[query] = results;
            _lruNodes.AddFirst(query);
        }
        finally
        {
            _cacheLock.ExitWriteLock();
        }
    }

    private void RefreshCachePosition(string query)
    {
        _cacheLock.EnterWriteLock();
        try
        {
            if (_lruNodes.Contains(query))
            {
                _lruNodes.Remove(query);
                _lruNodes.AddFirst(query);
            }
        }
        finally
        {
            _cacheLock.ExitWriteLock();
        }
    }

    public void InvalidateCache()
    {
        _cacheLock.EnterWriteLock();
        try
        {
            _cache.Clear();
            _lruNodes.Clear();
        }
        finally
        {
            _cacheLock.ExitWriteLock();
        }
    }

    public void FlushDeltaToDisk()
    {
        foreach (var group in _deltaIndex.GroupBy(kvp => kvp.Key % (uint)_numBarrels))
        {
            uint barrelId = group.Key;
            string path = Path.Combine(_indexDir, $"barrel_{barrelId}.dat");

            // Open with Append: true
            using var sw = new StreamWriter(path, append: true, Encoding.UTF8);
            foreach (var kvp in group)
            {
                // Just dump the new postings at the end of the file
                sw.WriteLine($"{kvp.Key}|{string.Join(",", kvp.Value.Select(p => $"{p.DocId}:{p.Score}"))}");
            }
        }

        _deltaIndex.Clear();
    }
}