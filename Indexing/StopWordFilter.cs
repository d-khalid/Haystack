using System;
using System.Collections.Generic;
using System.IO;

// To ensure meaningless words like 'the', is' are not part of parts
public class StopWordFilter
{
    private readonly HashSet<string> _stopWords;

    // Allow loading of custom stop words if needed
    public StopWordFilter(IEnumerable<string>? customWords = null)
    {
        // Ignore case
        _stopWords = new HashSet<string>(StringComparer.OrdinalIgnoreCase);

        if (customWords != null)
        {
            foreach (var word in customWords) _stopWords.Add(word);
        }
        else
        {
            LoadDefaultEnglish();
        }
    }

    public bool IsStopWord(string word) => _stopWords.Contains(word);

    private void LoadDefaultEnglish()
    {
        var common = new[] { 
            "a", "an", "and", "are", "as", "at", "be", "but", "by", "for", 
            "if", "in", "into", "is", "it", "no", "not", "of", "on", "or", 
            "such", "that", "the", "their", "then", "there", "these", 
            "they", "this", "to", "was", "will", "with" 
        };
        foreach (var w in common) _stopWords.Add(w);
    }
}