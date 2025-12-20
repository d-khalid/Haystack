using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;

public static class InvertedIndex
{
    private const byte MASK_TITLE = 1 << 0;
    private const byte MASK_TAG = 1 << 1;
    private const byte MASK_BODY = 1 << 2;

    public struct Posting
    {
        public uint DocId;
        public int Score;
    }

    public static void BuildAndSave(ISAMStorage forwardIndex, string outputDir, int numBarrels)
    {
        if (numBarrels < 1) numBarrels = 1;

        // Optimization: Pre-size the array. 
        // Using Dictionary<uint, List<Posting>> is fine, but we must fix the update logic.
        var shards = new Dictionary<uint, List<Posting>>[numBarrels];
        for (int i = 0; i < numBarrels; i++) shards[i] = new Dictionary<uint, List<Posting>>();

        forwardIndex.ResetNext();
        int count = 0;

        while (true)
        {
            var entry = forwardIndex.Next();
            if (entry == null) break;

            uint docId = (uint)CompoundKey.Unpack((ulong)entry.Value.Key).PrimaryId;
            string rawContent = Encoding.UTF8.GetString(entry.Value.Data);

            // Optimization: Process the document and aggregate scores locally first
            // This prevents the O(N) FindIndex search in the global list.
            var docWordScores = new Dictionary<uint, int>();

            foreach (var info in ParseWordIdsWithMasks(rawContent))
            {
                if (info.WordId == 0) continue;

                int score = CalculateScore(info.Mask);
                if (docWordScores.ContainsKey(info.WordId))
                    docWordScores[info.WordId] += score;
                else
                    docWordScores[info.WordId] = score;
            }

            // Now add the final per-doc score to the global shards
            foreach (var kvp in docWordScores)
            {
                uint wId = kvp.Key;
                int totalDocScore = kvp.Value;

                int barrelId = (int)(wId % (uint)numBarrels);
                if (!shards[barrelId].TryGetValue(wId, out var postings))
                {
                    postings = new List<Posting>();
                    shards[barrelId][wId] = postings;
                }

                // Optimization: Always Add. Since we processed docWordScores, 
                // we know we only add one entry per docId per word.
                postings.Add(new Posting { DocId = docId, Score = totalDocScore });
            }

            if (++count % 5000 == 0)
                Console.Write($"\rIndexed {count} posts...");
        }

        SaveToDisk(shards, outputDir);
    }

    private static int CalculateScore(byte mask)
    {
        int score = 1;
        if ((mask & MASK_TITLE) != 0) score += 10;
        if ((mask & MASK_TAG) != 0) score += 15;
        return score;
    }

    private static IEnumerable<(uint WordId, byte Mask)> ParseWordIdsWithMasks(string data)
    {
        if (string.IsNullOrEmpty(data)) yield break;

        // Optimization: Use Span-based parsing if memory is still tight, 
        // but removing the FindIndex is the 90% performance win.
        var segments = data.Split(' ', StringSplitOptions.RemoveEmptyEntries);
        foreach (var segment in segments)
        {
            int commaIdx = segment.IndexOf(',');
            if (commaIdx > 0)
            {
                if (uint.TryParse(segment.AsSpan(0, commaIdx), out uint wId) &&
                    byte.TryParse(segment.AsSpan(commaIdx + 1), out byte mask))
                {
                    yield return (wId, mask);
                }
            }
        }
    }

    private static void SaveToDisk(Dictionary<uint, List<Posting>>[] shards, string directory)
    {
        Directory.CreateDirectory(directory);
        for (int i = 0; i < shards.Length; i++)
        {
            if (shards[i].Count == 0) continue;
            string path = Path.Combine(directory, $"barrel_{i}.dat");

            // Optimization: Use a larger buffer for StreamWriter to speed up disk writes
            using var sw = new StreamWriter(path, false, Encoding.UTF8, 65536);

            foreach (var kvp in shards[i])
            {
                // Format: WordID|DocId:Score,DocId:Score
                var sortedPostings = kvp.Value
                    .OrderByDescending(p => p.Score)
                    .Select(p => $"{p.DocId}:{p.Score}");

                sw.WriteLine($"{kvp.Key}|{string.Join(",", sortedPostings)}");
            }
        }
    }

    public static List<Posting> SearchWithScores(string directory, int numBarrels, uint wordId)
    {
        int barrelId = (int)(wordId % (uint)numBarrels);
        string path = Path.Combine(directory, $"barrel_{barrelId}.dat");
        if (!File.Exists(path)) return new List<Posting>();

        var allPostings = new Dictionary<uint, int>();

        // We must read the whole file because the wordId might appear multiple times (fragments)
        foreach (var line in File.ReadLines(path))
        {
            int pipeIdx = line.IndexOf('|');
            if (pipeIdx == -1) continue;

            if (uint.TryParse(line.AsSpan(0, pipeIdx), out uint id) && id == wordId)
            {
                var segments = line.Substring(pipeIdx + 1).Split(',');
                foreach (var s in segments)
                {
                    var parts = s.Split(':');
                    if (parts.Length == 2)
                    {
                        uint docId = uint.Parse(parts[0]);
                        int score = int.Parse(parts[1]);
                        // Merge scores from different fragments
                        allPostings[docId] = allPostings.GetValueOrDefault(docId) + score;
                    }
                }
            }
        }

        return allPostings.Select(kvp => new Posting { DocId = kvp.Key, Score = kvp.Value }).ToList();
    }
}

// Note: SearchWithScores remains unchanged in logic to respect the format,
// though adding an index file (offset map) is highly recommended for production.
