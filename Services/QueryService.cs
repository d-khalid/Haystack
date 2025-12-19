using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;

public class QueryEngine
{
    private readonly string _indexDir;
    private readonly Lexicon _lexicon;
    private readonly int _numBarrels;
    private readonly ISAMStorage _dataIndex;

    // 1. Memory Buffer for runtime updates (WordID -> List of Postings)
    private readonly Dictionary<uint, List<InvertedIndex.Posting>> _deltaIndex = new();
    
    private readonly Dictionary<string, List<uint>> _cache = new();
    private readonly LinkedList<string> _lruNodes = new();
    private const int MaxCacheEntries = 1000;

    public QueryEngine(string indexDir, Lexicon lexicon, int numBarrels, ISAMStorage dataIndex)
    {
        _indexDir = indexDir;
        _lexicon = lexicon;
        _numBarrels = numBarrels;
        _dataIndex = dataIndex;
    }

    /// <summary>
    /// Adds a post to the index at runtime via the memory buffer.
    /// </summary>
    public void AddPost(Post post)
    {
        // Add to main ISAM data storage
        _dataIndex.Insert((uint)post.PostId, post.Serialize());

        // Tokenize and add to Delta Index
        var titleTokens = _lexicon.Tokenize(post.Title ?? "");
        var bodyTokens = _lexicon.Tokenize(post.CleanedBody ?? "");

        void BufferToken(string token, int score) {
            uint wordId = _lexicon.AddWord(token);
            if (!_deltaIndex.ContainsKey(wordId)) _deltaIndex[wordId] = new();
            
            // Check if post already exists for this word to aggregate score
            var existing = _deltaIndex[wordId].FirstOrDefault(p => p.DocId == (uint)post.PostId);
            if (existing.DocId != 0) {
                _deltaIndex[wordId].Remove(existing);
                _deltaIndex[wordId].Add(new InvertedIndex.Posting { DocId = (uint)post.PostId, Score = existing.Score + score });
            } else {
                _deltaIndex[wordId].Add(new InvertedIndex.Posting { DocId = (uint)post.PostId, Score = score });
            }
        }

        foreach (var t in titleTokens) BufferToken(t, 10);
        foreach (var t in bodyTokens) BufferToken(t, 1);

        InvalidateCache();
    }

    public List<uint> Search(string queryText, int limit = 20)
    {
        string normalizedQuery = queryText.Trim().ToLower();
        if (string.IsNullOrWhiteSpace(normalizedQuery)) return new List<uint>();

        if (_cache.TryGetValue(normalizedQuery, out var cachedResults))
        {
            RefreshCachePosition(normalizedQuery);
            Console.WriteLine($"[QueryEngine] Cache hit for '{normalizedQuery}': {cachedResults.Count} results");
            return cachedResults.Take(limit).ToList();
        }

        var tokens = normalizedQuery.Split(' ', StringSplitOptions.RemoveEmptyEntries);
        var docScores = new Dictionary<uint, double>();
        
        Console.WriteLine($"[QueryEngine] Searching for '{normalizedQuery}' with {tokens.Length} tokens");

        foreach (var token in tokens)
        {
            uint wordId = _lexicon.GetWordId(token);
            Console.WriteLine($"[QueryEngine] Token '{token}' -> WordId {wordId}");
            
            if (wordId == 0) 
            {
                Console.WriteLine($"[QueryEngine] WordId 0 (not found) for token '{token}'");
                continue;
            }

            // 2. SEARCH MERGE: Check Disk Barrels AND Memory Buffer
            var diskPostings = InvertedIndex.SearchWithScores(_indexDir, _numBarrels, wordId);
            Console.WriteLine($"[QueryEngine] Found {diskPostings.Count} disk postings for wordId {wordId}");
            
            // Add disk results
            foreach (var p in diskPostings)
            {
                if (!docScores.ContainsKey(p.DocId)) docScores[p.DocId] = 0;
                docScores[p.DocId] += p.Score;
            }

            // Add memory buffer results
            if (_deltaIndex.TryGetValue(wordId, out var livePostings))
            {
                Console.WriteLine($"[QueryEngine] Found {livePostings.Count} memory postings for wordId {wordId}");
                foreach (var p in livePostings)
                {
                    if (!docScores.ContainsKey(p.DocId)) docScores[p.DocId] = 0;
                    docScores[p.DocId] += p.Score;
                }
            }
        }
        
        Console.WriteLine($"[QueryEngine] Total unique documents found: {docScores.Count}");
        
        if (docScores.Count == 0) return new List<uint>();

        var finalResults = docScores
            .OrderByDescending(kvp => kvp.Value)
            .Take(200) 
            .Select(hit => {
                // Construct the compound key used to store this post
                CompoundKey ck = new CompoundKey(
                    (byte)KeyType.PostById,
                    (ushort)SiteID.AskUbuntu,
                    hit.Key
                );
                
                var entry = _dataIndex.Get((long)ck.Pack());
                if (entry is null) return new { hit.Key, FinalScore = hit.Value };

                var post = Post.Deserialize(entry);
                double popularityBoost = Math.Log10(post.Score + 1) * 5.0;
                return new { hit.Key, FinalScore = hit.Value + popularityBoost };
            })
            .OrderByDescending(r => r.FinalScore)
            .Select(r => r.Key)
            .ToList();

        SaveToCache(normalizedQuery, finalResults);
        return finalResults.Take(limit).ToList();
    }

    /// <summary>
    /// Flushes the memory buffer into the physical disk barrels.
    /// This is an expensive operation and should be run on a background thread.
    /// </summary>
    public void FlushDeltaToDisk()
    {
        if (_deltaIndex.Count == 0) return;

        // Group delta items by their destination barrel
        var barrelUpdates = _deltaIndex.GroupBy(kvp => kvp.Key % (uint)_numBarrels);

        foreach (var group in barrelUpdates)
        {
            uint barrelId = group.Key;
            string path = Path.Combine(_indexDir, $"barrel_{barrelId}.dat");
            
            // Read existing barrel data
            var barrelData = new Dictionary<uint, List<InvertedIndex.Posting>>();
            if (File.Exists(path))
            {
                foreach (var line in File.ReadAllLines(path))
                {
                    var parts = line.Split('|');
                    if (parts.Length != 2) continue;
                    uint wId = uint.Parse(parts[0]);
                    var postings = parts[1].Split(',').Select(s => {
                        var p = s.Split(':');
                        return new InvertedIndex.Posting { DocId = uint.Parse(p[0]), Score = int.Parse(p[1]) };
                    }).ToList();
                    barrelData[wId] = postings;
                }
            }

            // Merge Delta into Barrel Data
            foreach (var update in group)
            {
                if (!barrelData.ContainsKey(update.Key)) barrelData[update.Key] = new();
                barrelData[update.Key].AddRange(update.Value);
            }

            // Write back to disk (In-place rewrite for current format)
            using var sw = new StreamWriter(path, false, Encoding.UTF8);
            foreach (var kvp in barrelData.OrderBy(k => k.Key))
            {
                var sorted = kvp.Value.OrderByDescending(p => p.Score);
                sw.WriteLine($"{kvp.Key}|{string.Join(",", sorted.Select(p => $"{p.DocId}:{p.Score}"))}");
            }
        }

        _deltaIndex.Clear();
        InvalidateCache();
    }

    private void SaveToCache(string query, List<uint> results)
    {
        if (_cache.Count >= MaxCacheEntries)
        {
            var oldest = _lruNodes.Last.Value;
            _cache.Remove(oldest);
            _lruNodes.RemoveLast();
        }
        _cache[query] = results;
        _lruNodes.AddFirst(query);
    }

    private void RefreshCachePosition(string query)
    {
        _lruNodes.Remove(query);
        _lruNodes.AddFirst(query);
    }

    public void InvalidateCache()
    {
        _cache.Clear();
        _lruNodes.Clear();
    }
}