using System;
using System.Collections.Generic;
using System.Text;

public static class ForwardIndex
{
    public static void Generate(ISAMStorage outputStore, ISAMStorage dataIndex, Lexicon lexicon)
    {
        // Use a smaller buffer to flush to disk periodically
        var batchBuffer = new List<(long Key, byte[] Data)>();
        const int BatchSize = 10000; 
        int count = 0;

        dataIndex.ResetNext();
        
        (long Key, byte[] Data)? entry;
        while ((entry = dataIndex.Next()) != null)
        {
            Post post = Post.Deserialize(entry.Value.Data);
            StringBuilder sb = new StringBuilder();

            // 1. Title (Mask 1)
            if (post.PostTypeId == 1 && !string.IsNullOrEmpty(post.Title))
            {
                foreach (var t in lexicon.Tokenize(post.Title))
                {
                    sb.Append($"{lexicon.GetWordId(t)},1 ");
                }
            }

            // 2. Body (Mask 2)
            if (!string.IsNullOrEmpty(post.CleanedBody))
            {
                foreach (var t in lexicon.Tokenize(post.CleanedBody))
                {
                    sb.Append($"{lexicon.GetWordId(t)},2 ");
                }
            }

            // 3. Tags (Mask 4)
            foreach (var t in post.Tags)
            {
                sb.Append($"{lexicon.GetWordId(t)},4 ");
            }

            // Convert to bytes and add to buffer
            byte[] encodedData = Encoding.UTF8.GetBytes(sb.ToString().TrimEnd());
            batchBuffer.Add((entry.Value.Key, encodedData));

            count++;

            // Periodically flush to the ISAM file to save RAM
            if (count % BatchSize == 0)
            {
                outputStore.InsertBatch(batchBuffer);
                batchBuffer.Clear();
                Console.Write($"\rIndexed {count} entries...");
            }
        }

        // Final flush for remaining items
        if (batchBuffer.Count > 0)
        {
            outputStore.InsertBatch(batchBuffer);
        }

        Console.WriteLine($"\nIndexing Complete. Total: {count}");
    }
}