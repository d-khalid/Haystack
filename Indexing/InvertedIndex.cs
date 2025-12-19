using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;

public static class InvertedIndex
{
    // Weights based on where the word was found
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

        // Shards: BarrelID -> [WordID -> List of Postings]
        var shards = new Dictionary<uint, List<Posting>>[numBarrels];
        for (int i = 0; i < numBarrels; i++) shards[i] = new Dictionary<uint, List<Posting>>();

        forwardIndex.ResetNext();
        int count = 0;

        while (true)
        {
            var entry = forwardIndex.Next();
            if (entry == null) break;

            // Unpack the key to get the Document ID
            uint docId = (uint)CompoundKey.Unpack((ulong)entry.Value.Key).PrimaryId;

            // CONVERSION: Convert the raw bytes back into the "ID,Mask" string format
            string rawContent = Encoding.UTF8.GetString(entry.Value.Data);

            // Now pass the string to your parser
            foreach (var info in ParseWordIdsWithMasks(rawContent))
            {
                if (info.WordId == 0) continue;

                int barrelId = (int)(info.WordId % (uint)numBarrels);
                if (!shards[barrelId].TryGetValue(info.WordId, out var postings))
                {
                    postings = new List<Posting>();
                    shards[barrelId][info.WordId] = postings;
                }

                // Logic for scoring and updating postings...
                var existing = postings.FindIndex(p => p.DocId == docId);
                int newScore = CalculateScore(info.Mask);

                if (existing != -1)
                {
                    var p = postings[existing];
                    p.Score += newScore;
                    postings[existing] = p;
                }
                else
                {
                    postings.Add(new Posting { DocId = docId, Score = newScore });
                }
            }
        }

        SaveToDisk(shards, outputDir);
    }

    private static int CalculateScore(byte mask)
    {
        int score = 1; // Base score for body text
        if ((mask & MASK_TITLE) != 0) score += 10; // High priority for Title
        if ((mask & MASK_TAG) != 0) score += 15; // Highest priority for Tags
        return score;
    }

    private static IEnumerable<(uint WordId, byte Mask)> ParseWordIdsWithMasks(string data)
    {
        if (string.IsNullOrEmpty(data)) yield break;

        var segments = data.Split(' ', StringSplitOptions.RemoveEmptyEntries);
        foreach (var segment in segments)
        {
            var parts = segment.Split(',');
            if (parts.Length == 2 && uint.TryParse(parts[0], out uint wId) && byte.TryParse(parts[1], out byte mask))
                yield return (wId, mask);
        }
    }

    private static void SaveToDisk(Dictionary<uint, List<Posting>>[] shards, string directory)
    {
        Directory.CreateDirectory(directory);
        for (int i = 0; i < shards.Length; i++)
        {
            if (shards[i].Count == 0) continue;
            string path = Path.Combine(directory, $"barrel_{i}.dat");
            using var sw = new StreamWriter(path, false, Encoding.UTF8);

            foreach (var kvp in shards[i])
            {
                // Sort by Score DESC so the most relevant docs are at the start of the list
                var sortedPostings = kvp.Value
                    .OrderByDescending(p => p.Score)
                    .Select(p => $"{p.DocId}:{p.Score}");

                sw.WriteLine($"{kvp.Key}|{string.Join(",", sortedPostings)}");
            }
        }
    }

    /// <summary>
    /// Returns a list of DocIDs for a word, already sorted by their importance.
    /// </summary>
    public static List<uint> Search(string directory, int numBarrels, uint wordId)
    {
        int barrelId = (int)(wordId % (uint)numBarrels);
        string path = Path.Combine(directory, $"barrel_{barrelId}.dat");
        if (!File.Exists(path)) return new List<uint>();

        foreach (var line in File.ReadLines(path))
        {
            var parts = line.Split('|');
            if (parts.Length == 2 && uint.TryParse(parts[0], out uint id) && id == wordId)
            {
                // Each segment is DocId:Score, we just want the DocId
                return parts[1].Split(',')
                    .Select(s => uint.Parse(s.Split(':')[0]))
                    .ToList();
            }
        }

        return new List<uint>();
    }
}