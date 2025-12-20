using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;

public class Lexicon
{
    private readonly Dictionary<string, uint> _wordToId;
    private readonly List<string> _idToWord;
    private uint _nextId = 1;

    // Added capacity parameter to prevent frequent resizing
    public Lexicon(int initialCapacity = 0)
    {
        _wordToId = new Dictionary<string, uint>(initialCapacity);
        _idToWord = new List<string>(initialCapacity);
        
        // Reserve ID 0 for "Not Found" / Null
        _idToWord.Add(string.Empty);
    }
    
    public int Count => _wordToId.Count;
    
    public static (Lexicon lexicon, AutocompleteService trie) Generate(ISAMStorage dataIndex)
    {
        // Pre-sizing to a reasonable estimate (e.g., 500k) to reduce "ghost" array allocations
        const int InitialEstimate = 500_000;
        Lexicon lexicon = new Lexicon(InitialEstimate);
        AutocompleteService autocomplete = new AutocompleteService();
        int count = 0;

        dataIndex.ResetNext();

        (long Key, byte[] Data)? entry;
        while ((entry = dataIndex.Next()) != null)
        {
            CompoundKey ck = CompoundKey.Unpack((uint)entry.Value.Key);

            if (ck.KeyType == (byte)KeyType.PostById)
            {
                Post post = Post.Deserialize(entry.Value.Data);

                void ProcessText(string text)
                {
                    if (string.IsNullOrEmpty(text)) return;
                    foreach (var token in lexicon.Tokenize(text))
                    {
                        // 1. Add/Get ID
                        uint id = lexicon.AddWord(token);
                        
                        // 2. Optimization: Retrieve the internal reference from the lexicon.
                        // This ensures the Trie points to the long-lived string in the List,
                        // allowing the temporary 'token' string to be garbage collected.
                        string internalRef = lexicon.GetWord(id);
                        autocomplete.AddWord(internalRef);
                    }
                }

                if (post.PostTypeId == 1) ProcessText(post.Title);
                ProcessText(post.CleanedBody);

                if (post.Tags != null)
                {
                    foreach (var tag in post.Tags)
                    {
                        string norm = lexicon.NormalizeToken(tag);
                        if (!string.IsNullOrEmpty(norm))
                        {
                            uint id = lexicon.AddWord(norm);
                            string internalRef = lexicon.GetWord(id);
                            autocomplete.AddWord(internalRef, 5); 
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

    public uint AddWord(string word)
    {
        if (_wordToId.TryGetValue(word, out uint id))
            return id;

        _idToWord.Add(word);
        _wordToId.Add(word, _nextId);

        return _nextId++;
    }

    public string GetWord(uint id)
    {
        if (id == 0 || id >= (uint)_idToWord.Count)
            return string.Empty;
            
        return _idToWord[(int)id];
    }

    public uint GetWordId(string word)
    {
        return _wordToId.TryGetValue(word, out uint id) ? id : 0;
    }

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
        char[] trimChars = ".,().\"'?:;[]&^%$ \n\r\t".ToCharArray();
        return token.Trim(trimChars).ToLowerInvariant();
    }

    public List<string> Tokenize(string text)
    {
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