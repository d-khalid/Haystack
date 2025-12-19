using System;
using System.Collections.Generic;
using Newtonsoft.Json;

public class Post
{
    // Indentifiers
    public int SiteId { get; set; }
    public uint PostId { get; set; }
    public uint PostTypeId { get; set; } // 1=Question, 2=Answer

    // Content
    public string Title { get; set; } = string.Empty;
    public string Body { get; set; } = string.Empty;
    public List<string> Tags { get; set; } = new();
    public string CleanedBody { get; set; } = string.Empty;

    // Relationships (null for doesn't exist)
    public uint? ParentId { get; set; }
    public uint? AcceptedAnswerId { get; set; }

    // User info
    public uint? OwnerUserId { get; set; }
    public uint? LastEditorUserId { get; set; }

    // Metrics
    public int Score { get; set; }
    public uint ViewCount { get; set; }
    public uint AnswerCount { get; set; }
    public uint CommentCount { get; set; }

    // Timestamps
    public string CreationDate { get; set; } = string.Empty;
    public string LastEditDate { get; set; } = string.Empty;
    public string LastActivityDate { get; set; } = string.Empty;

    // Metadata
    public string ContentLicense { get; set; } = string.Empty;

    /// <summary>
    /// Converts this object to a raw byte array
    /// </summary>
    public byte[] Serialize()
    {
        string json = JsonConvert.SerializeObject(this);
        return System.Text.Encoding.UTF8.GetBytes(json);
    }

    /// <summary>
    /// Reconstructs a Post from raw byte
    /// </summary>
    public static Post Deserialize(byte[] data)
    {
        string json = System.Text.Encoding.UTF8.GetString(data);
        return JsonConvert.DeserializeObject<Post>(json);
    }
}