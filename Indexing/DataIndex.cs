using System;
using System.Xml;
using System.Collections.Generic;
using System.Text.RegularExpressions;
using System.Web;

public static class DataIndex
{

    public static void Generate(ISAMStorage dataIndex, string xmlPath, SiteID siteId)
    {
        int count = 0;
        var batch = new List<(long Key, byte[] Data)>();
        const int BatchSize = 5000;

        using (XmlReader reader = XmlReader.Create(xmlPath))
        {
            while (reader.Read())
            {
                // Each post is contained in a <row /> element
                if (reader.NodeType == XmlNodeType.Element && reader.Name == "row")
                {
                    Post post = MapRowToPost(reader, siteId);
                    
                    // Create the CompoundKey for the ISAM index
                    CompoundKey ck = new CompoundKey(
                        (byte)KeyType.PostById, 
                        (ushort)siteId, 
                        post.PostId
                    );

                    batch.Add(((long)ck.Pack(), post.Serialize()));
                    count++;

                    if (count % BatchSize == 0)
                    {
                        dataIndex.InsertBatch(batch);
                        batch.Clear();
                        Console.Write($"\rImported {count} posts into DataIndex...");
                    }

                   
                }
            }
        }

        // Final flush
        if (batch.Count > 0)
        {
            dataIndex.InsertBatch(batch);
        }

        Console.WriteLine($"\nCompleted. Total Posts: {count}");
    }

    private static Post MapRowToPost(XmlReader reader, SiteID siteId)
    {
        string rawBody = reader.GetAttribute("Body") ?? "";
        
        Post post = new Post
        {
            SiteId = (int)siteId,
            PostId = uint.Parse(reader.GetAttribute("Id")),
            PostTypeId = uint.Parse(reader.GetAttribute("PostTypeId")),
            Title = reader.GetAttribute("Title") ?? "",
            Body = rawBody,
            CleanedBody = CleanHtml(rawBody),
            Score = int.Parse(reader.GetAttribute("Score") ?? "0"),
            ViewCount = uint.Parse(reader.GetAttribute("ViewCount") ?? "0"),
            AnswerCount = uint.Parse(reader.GetAttribute("AnswerCount") ?? "0"),
            CommentCount = uint.Parse(reader.GetAttribute("CommentCount") ?? "0"),
            CreationDate = reader.GetAttribute("CreationDate") ?? "",
            LastEditDate = reader.GetAttribute("LastEditDate") ?? "",
            LastActivityDate = reader.GetAttribute("LastActivityDate") ?? "",
            ContentLicense = reader.GetAttribute("ContentLicense") ?? ""
        };

        // Handle Tags: "|tag1|tag2|" -> ["tag1", "tag2"]
        string tagsAttr = reader.GetAttribute("Tags") ?? "";
        if (!string.IsNullOrEmpty(tagsAttr))
        {
            post.Tags = new List<string>(tagsAttr.Split('|', StringSplitOptions.RemoveEmptyEntries));
        }

        // Handle Nullables
        if (uint.TryParse(reader.GetAttribute("ParentId"), out uint pId)) post.ParentId = pId;
        if (uint.TryParse(reader.GetAttribute("AcceptedAnswerId"), out uint aId)) post.AcceptedAnswerId = aId;
        if (uint.TryParse(reader.GetAttribute("OwnerUserId"), out uint uId)) post.OwnerUserId = uId;
        if (uint.TryParse(reader.GetAttribute("LastEditorUserId"), out uint leId)) post.LastEditorUserId = leId;

        return post;
    }

    // Clean HTML using Regex and HTTP Utility
    private static string CleanHtml(string html)
    {
        if (string.IsNullOrWhiteSpace(html)) return "";
        // Remove HTML tags and decode entities (e.g., &quot; -> ")
        string noHtml = Regex.Replace(html, "<.*?>", " ");
        return HttpUtility.HtmlDecode(noHtml);
    }
}