using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;

public class AutocompleteService
{
    private class TrieNode
    {
        public Dictionary<char, TrieNode> Children = new Dictionary<char, TrieNode>();
        public bool IsEndOfWord;
        public int Weight; // Used for ranking (e.g., frequency of the word)
    }

    private readonly TrieNode _root = new TrieNode();

    /// <summary>
    /// Adds a word to the trie. If it exists, increments its weight.
    /// </summary>
    public void AddWord(string word, int weight = 1)
    {
        if (string.IsNullOrWhiteSpace(word)) return;
        
        var current = _root;
        foreach (char c in word.ToLower())
        {
            if (!current.Children.ContainsKey(c))
                current.Children[c] = new TrieNode();
            current = current.Children[c];
        }
        current.IsEndOfWord = true;
        current.Weight += weight;
    }

    /// <summary>
    /// Returns the top N suggestions for a given prefix, ranked by weight.
    /// </summary>
    public List<string> Query(string prefix, int limit = 5)
    {
        var results = new List<(string Word, int Weight)>();
        var current = _root;

        // 1. Navigate to the end of the prefix
        foreach (char c in prefix.ToLower())
        {
            if (!current.Children.TryGetValue(c, out current))
                return new List<string>(); // Prefix not found
        }

        // 2. Perform DFS to find all words under this prefix
        FindWordsRecursive(current, prefix.ToLower(), results);

        // 3. Sort by weight (descending) and return top N
        return results
            .OrderByDescending(r => r.Weight)
            .Take(limit)
            .Select(r => r.Word)
            .ToList();
    }

    private void FindWordsRecursive(TrieNode node, string currentWord, List<(string, int)> results)
    {
        if (node.IsEndOfWord) results.Add((currentWord, node.Weight));

        foreach (var child in node.Children)
        {
            FindWordsRecursive(child.Value, currentWord + child.Key, results);
        }
    }

    // --- Persistence Logic ---

    /// <summary>
    /// Saves the vocabulary and weights to a flat file.
    /// </summary>
    public void ExportToFile(string filePath)
    {
        var allWords = new List<(string, int)>();
        FindWordsRecursive(_root, "", allWords);
        
        File.WriteAllLines(filePath, allWords.Select(w => $"{w.Item1}|{w.Item2}"));
    }

    /// <summary>
    /// Loads a previously saved vocabulary file.
    /// </summary>
    public void LoadFromFile(string filePath)
    {
        if (!File.Exists(filePath)) return;

        foreach (var line in File.ReadLines(filePath))
        {
            var parts = line.Split('|');
            if (parts.Length == 2 && int.TryParse(parts[1], out int weight))
                AddWord(parts[0], weight);
        }
    }
}