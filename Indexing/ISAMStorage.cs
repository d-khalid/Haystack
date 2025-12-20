using System;
using System.IO;
using System.Collections.Generic;

public class ISAMStorage : IDisposable
{
    private readonly string _indexPath;
    private readonly string _dataPath;
    private FileStream _indexStream;
    private FileStream _dataStream;
    
    // Each index entry is: Int64 Key (8 bytes) + Int64 Offset (8 bytes) = 16 bytes
    private const int IndexEntrySize = 16;
    
    // Tracks the current "row" in the index for Next()
    private long _currentIndexPosition = 0; 

    public ISAMStorage(string baseFileName)
    {
        _indexPath = baseFileName + ".idx";
        _dataPath = baseFileName + ".dat";

        // Check that the files exist, if not make them
        string? dir = Path.GetDirectoryName(Path.GetFullPath(baseFileName));
        if (!string.IsNullOrEmpty(dir) && !Directory.Exists(dir))
        {
            Directory.CreateDirectory(dir);
            Console.WriteLine($"Created ISAM files for: {dir}");
        }

        // Open or Create files
        _indexStream = new FileStream(_indexPath, FileMode.OpenOrCreate, FileAccess.ReadWrite, FileShare.Read);
        _dataStream = new FileStream(_dataPath, FileMode.OpenOrCreate, FileAccess.ReadWrite, FileShare.Read);
    }

    /// <summary>
    /// Appends data and updates the index in sorted order.
    /// Maintains sorted index by finding the correct insertion position.
    /// </summary>
    public void Insert(long key, byte[] data)
    {
        // Lock both streams for thread-safety
        lock (_dataStream)
        {
            lock (_indexStream)
            {
                // 1. Write to Data File
                long dataOffset = _dataStream.Length;
                _dataStream.Seek(0, SeekOrigin.End);
                _dataStream.Write(BitConverter.GetBytes(data.Length), 0, 4); // Store length prefix
                _dataStream.Write(data, 0, data.Length);
                _dataStream.Flush();

                // 2. Write to Index File in sorted order
                InsertIntoSortedIndexInternal(key, dataOffset);
            }
        }
    }

    /// <summary>
    /// Inserts an entry into the index file while maintaining sorted order by key.
    /// Must be called while holding locks on both _dataStream and _indexStream.
    /// </summary>
    private void InsertIntoSortedIndexInternal(long key, long dataOffset)
    {
        // NOTE: This method assumes locks are already held by the caller
        long indexSize = _indexStream.Length;
        long entryCount = indexSize / IndexEntrySize;
        
        // Find the correct position to insert (binary search for insertion point)
        long insertPosition = 0;
        long low = 0;
        long high = entryCount - 1;

        while (low <= high)
        {
            long mid = low + (high - low) / 2;
            _indexStream.Seek(mid * IndexEntrySize, SeekOrigin.Begin);
            
            byte[] buffer = new byte[IndexEntrySize];
            _indexStream.Read(buffer, 0, IndexEntrySize);
            long midKey = BitConverter.ToInt64(buffer, 0);

            if (midKey < key)
            {
                insertPosition = mid + 1;
                low = mid + 1;
            }
            else
            {
                high = mid - 1;
            }
        }

        // Read all entries after the insertion point
        List<(long, long)> entriesAfter = new();
        long readPosition = insertPosition * IndexEntrySize;
        
        _indexStream.Seek(readPosition, SeekOrigin.Begin);
        byte[] entryBuffer = new byte[IndexEntrySize];
        while (_indexStream.Read(entryBuffer, 0, IndexEntrySize) == IndexEntrySize)
        {
            long k = BitConverter.ToInt64(entryBuffer, 0);
            long offset = BitConverter.ToInt64(entryBuffer, 8);
            entriesAfter.Add((k, offset));
        }

        // Seek back to insertion point and write new entry
        _indexStream.Seek(insertPosition * IndexEntrySize, SeekOrigin.Begin);
        _indexStream.Write(BitConverter.GetBytes(key), 0, 8);
        _indexStream.Write(BitConverter.GetBytes(dataOffset), 0, 8);

        // Write back all entries that were after the insertion point
        foreach (var (k, offset) in entriesAfter)
        {
            _indexStream.Write(BitConverter.GetBytes(k), 0, 8);
            _indexStream.Write(BitConverter.GetBytes(offset), 0, 8);
        }

        // Truncate file to correct size
        long newSize = (insertPosition + 1 + entriesAfter.Count) * IndexEntrySize;
        _indexStream.SetLength(newSize);
        _indexStream.Flush();
    }
    
    public void InsertBatch(IEnumerable<(long Key, byte[] Data)> entries)
    {
        lock (_dataStream)
        {
            lock (_indexStream)
            {
                _dataStream.Seek(0, SeekOrigin.End);

                foreach (var (key, data) in entries)
                {
                    if (data == null) continue;

                    long offset = _dataStream.Position;

                    // Write Data: [4b Length][Nb Payload]
                    _dataStream.Write(BitConverter.GetBytes(data.Length), 0, 4);
                    _dataStream.Write(data, 0, data.Length);

                    // Insert index entry in sorted order (with locks already held)
                    InsertIntoSortedIndexInternal(key, offset);
                }

                _dataStream.Flush();
            }
        }
    }

    /// <summary>
    /// Performs a Binary Search on the .idx file to find the 64-bit offset.
    /// </summary>
    public byte[] Get(long targetKey)
    {
        // Lock for thread-safety
        lock (_indexStream)
        {
            long low = 0;
            long high = (_indexStream.Length / IndexEntrySize) - 1;

            while (low <= high)
            {
                long mid = low + (high - low) / 2;
                _indexStream.Seek(mid * IndexEntrySize, SeekOrigin.Begin);
                
                byte[] buffer = new byte[IndexEntrySize];
                _indexStream.Read(buffer, 0, IndexEntrySize);
                
                long currentKey = BitConverter.ToInt64(buffer, 0);
                long dataOffset = BitConverter.ToInt64(buffer, 8);

                if (currentKey == targetKey)
                {
                    return ReadFromDataFile(dataOffset);
                }
                
                if (currentKey < targetKey) low = mid + 1;
                else high = mid - 1;
            }
        }
        return null; // Not found
    }
    
    /// <summary>
    /// Returns the next key-value pair in the storage. 
    /// Returns null when the end of the file is reached.
    /// </summary>
    public (long Key, byte[] Data)? Next()
    {
        lock (_indexStream)
        {
            // Check for EOF
            if (_currentIndexPosition >= _indexStream.Length)
            {
                return null;
            }

            // Read current entry
            _indexStream.Seek(_currentIndexPosition, SeekOrigin.Begin);
            byte[] buffer = new byte[IndexEntrySize];
            int bytesRead = _indexStream.Read(buffer, 0, IndexEntrySize);

            if (bytesRead < IndexEntrySize) return null;

            long key = BitConverter.ToInt64(buffer, 0);
            long dataOffset = BitConverter.ToInt64(buffer, 8);

            // Go to next entry
            _currentIndexPosition += IndexEntrySize;

            // Get and return the data
            byte[] data = ReadFromDataFile(dataOffset);

            return (key, data);
        }
    }

    /// <summary>
    /// Resets the Next() cursor back to the start of the storage.
    /// </summary>
    public void ResetNext()
    {
        lock (_indexStream)
        {
            _currentIndexPosition = 0;
        }
    }

    private byte[] ReadFromDataFile(long offset)
    {
        lock (_dataStream)
        {
            _dataStream.Seek(offset, SeekOrigin.Begin);
            byte[] lenBuffer = new byte[4];
            _dataStream.Read(lenBuffer, 0, 4);
            int length = BitConverter.ToInt32(lenBuffer, 0);

            byte[] data = new byte[length];
            _dataStream.Read(data, 0, length);
            return data;
        }
    }

    public void Dispose()
    {
        _indexStream?.Dispose();
        _dataStream?.Dispose();
    }
}