using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;

public class Lexicon
{
    private readonly Dictionary<string, ulong> _wordToId = new();
    private readonly List<string> _idToWord = new();
    private ulong _nextId = 1;

    public Lexicon()
    {
        // Reserve ID 0 for "Not Found" / Null
        _idToWord.Add(string.Empty);
    }
    
    public int Count => _wordToId.Count;
    
    // Generate lexicon and trie for autocomplete side by side
    public static (Lexicon lexicon, AutocompleteService trie) Generate(ISAMStorage dataIndex)
    {
        Lexicon lexicon = new Lexicon();
        AutocompleteService autocomplete = new AutocompleteService();
        int count = 0;

        dataIndex.ResetNext();

        (long Key, byte[] Data)? entry;
        while ((entry = dataIndex.Next()) != null)
        {
            CompoundKey ck = CompoundKey.Unpack((ulong)entry.Value.Key);

            if (ck.KeyType == (byte)KeyType.PostById)
            {
                Post post = Post.Deserialize(entry.Value.Data);

                // Helper to process text for both Lexicon (ID assignment) and Trie (Autocomplete)
                void ProcessText(string text)
                {
                    if (string.IsNullOrEmpty(text)) return;
                    foreach (var token in lexicon.Tokenize(text))
                    {
                        lexicon.AddWord(token);       // Assign/Get ID
                        autocomplete.AddWord(token);  // Add to Trie & increment weight
                    }
                }

                // Process content fields
                if (post.PostTypeId == 1) ProcessText(post.Title);
                ProcessText(post.CleanedBody);

                // Process Tags
                if (post.Tags != null)
                {
                    foreach (var tag in post.Tags)
                    {
                        string norm = lexicon.NormalizeToken(tag);
                        if (!string.IsNullOrEmpty(norm))
                        {
                            lexicon.AddWord(norm);
                            autocomplete.AddWord(norm, 5); // Give tags higher weight/priority
                        }
                    }
                }

                if (++count % 1000 == 0)
                    Console.Write($"\rProcessed {count} posts...");
            }
        }

        Console.WriteLine($"\nDone. Lexicon size: {lexicon.Count}. Trie populated.");
        return (lexicon, autocomplete);
    }

    public ulong AddWord(string word)
    {
        if (_wordToId.TryGetValue(word, out ulong id))
            return id;

        _idToWord.Add(word);
        _wordToId.Add(word, _nextId);

        return _nextId++;
    }

    public string GetWord(ulong id)
    {
        if (id == 0 || id >= (ulong)_idToWord.Count)
            return string.Empty;
            
        return _idToWord[(int)id];
    }

    public ulong GetWordId(string word)
    {
        return _wordToId.TryGetValue(word, out ulong id) ? id : 0;
    }

    // File Format: newline-delimited word
    public void Save(string filePath)
    {
        File.WriteAllLines(filePath, _idToWord.Skip(1));
    }

    public void Load(string filePath)
    {
        if (!File.Exists(filePath)) return;
        
        foreach (var line in File.ReadLines(filePath))
        {
            AddWord(line);
        }
    }

    public string NormalizeToken(string token)
    {
        if (string.IsNullOrWhiteSpace(token)) return string.Empty;
        
        // Remove irrelevant symbols
        char[] trimChars = ".,().\"'?:;[]&^%$ \n\r\t".ToCharArray();
        return token.Trim(trimChars).ToLowerInvariant();
    }

    public List<string> Tokenize(string text)
    {
        // Simple whitespace split initially
        var rawTokens = text.Split(' ', StringSplitOptions.RemoveEmptyEntries);
        var result = new List<string>();

        foreach (var raw in rawTokens)
        {
            string normalized = NormalizeToken(raw);
            if (!string.IsNullOrEmpty(normalized))
                result.Add(normalized);
        }
        return result;
    }
}