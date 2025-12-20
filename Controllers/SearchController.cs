using System;
using System.Collections.Generic;
using System.Linq;
using Microsoft.AspNetCore.Mvc;

[ApiController]
[Route("api/[controller]")]
public class SearchController : ControllerBase
{
    private readonly QueryEngine _queryEngine;
    private readonly AutocompleteService _autocompleteService;
    private readonly ISAMStorage _dataIndex;
    

    public SearchController(QueryEngine queryEngine, AutocompleteService autocompleteService, ISAMStorage dataIndex)
    {
        _queryEngine = queryEngine;
        _autocompleteService = autocompleteService;
        _dataIndex = dataIndex;
    }

    /// <summary>
    /// Searches the index for posts matching the query string.
    /// Returns a list of posts ranked by relevance.
    /// </summary>
    /// <param name="query">The search query text</param>
    /// <param name="limit">Maximum number of results to return (default: 20)</param>
    /// <returns>List of matching posts</returns>
    [HttpGet("query")]
    public ActionResult<SearchQueryResponse> Query([FromQuery] string query, [FromQuery] int limit = 20)
    {
        if (string.IsNullOrWhiteSpace(query))
        {
            return BadRequest(new { error = "Query parameter is required and cannot be empty" });
        }

        if (limit <= 0 || limit > 1000)
        {
            return BadRequest(new { error = "Limit must be between 1 and 1000" });
        }

        try
        {
            // Get post IDs from the query engine
            var postIds = _queryEngine.Search(query, limit);
            Console.WriteLine($"[API] Query '{query}': Found {postIds.Count} post IDs");

            // Retrieve full post objects from the data index
            var posts = new List<Post>();
            int failedCount = 0;
            int nullCount = 0;

            foreach (var postId in postIds)
            {
                // Construct the compound key used to store this post
                CompoundKey ck = new CompoundKey(
                    (byte)KeyType.PostById,
                    (ushort)SiteID.AskUbuntu,
                    postId
                );

                Console.WriteLine($"[API] Looking up post {postId} with compound key: {ck.Pack()}");
                var postData = _dataIndex.Get((long)ck.Pack());

                if (postData != null)
                {
                    try
                    {
                        var post = Post.Deserialize(postData);
                        posts.Add(post);
                        Console.WriteLine($"[API] Successfully retrieved post {postId}");
                    }
                    catch (Exception ex)
                    {
                        // Log and continue if a single post fails to deserialize
                        Console.WriteLine($"[API] Failed to deserialize post {postId}: {ex.Message}");
                        failedCount++;
                    }
                }
                else
                {
                    Console.WriteLine($"[API] Post {postId} data is null from storage");
                    nullCount++;
                }
            }

            Console.WriteLine(
                $"[API] Query '{query}': Retrieved {posts.Count} posts, {failedCount} deserialization failures, {nullCount} null results");

            return Ok(new SearchQueryResponse
            {
                Query = query,
                ResultCount = posts.Count,
                Posts = posts
            });
        }
        catch (Exception ex)
        {
            Console.WriteLine($"[API] Error during query '{query}': {ex}");
            return StatusCode(500,
                new { error = "An error occurred while processing the search", details = ex.Message });
        }
    }

    /// <summary>
    /// Retrieves a single post by its ID.
    /// Returns the complete post object.
    /// </summary>
    /// <param name="postId">The ID of the post to retrieve</param>
    /// <returns>The post object</returns>
    [HttpGet("post/{postId}")]
    public ActionResult<Post> GetPost(uint postId)
    {
        if (postId == 0)
        {
            return BadRequest(new { error = "Post ID must be a positive integer" });
        }

        try
        {
            // Construct the compound key used to store this post
            CompoundKey ck = new CompoundKey(
                (byte)KeyType.PostById,
                (ushort)SiteID.AskUbuntu,
                postId
            );

            Console.WriteLine($"[API] Getting post {postId} with compound key: {ck.Pack()}");
            var postData = _dataIndex.Get((long)ck.Pack());

            if (postData == null)
            {
                Console.WriteLine($"[API] Post {postId} not found in storage");
                return NotFound(new { error = $"Post with ID {postId} not found" });
            }

            try
            {
                var post = Post.Deserialize(postData);
                Console.WriteLine($"[API] Successfully retrieved post {postId}");
                return Ok(post);
            }
            catch (Exception ex)
            {
                Console.WriteLine($"[API] Failed to deserialize post {postId}: {ex.Message}");
                return StatusCode(500, new { error = "Failed to deserialize post data", details = ex.Message });
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"[API] Error getting post {postId}: {ex}");
            return StatusCode(500, new { error = "An error occurred while retrieving the post", details = ex.Message });
        }
    }

    /// <summary>
    /// Provides autocomplete suggestions based on a prefix.
    /// Returns the most likely words that start with the given prefix.
    /// </summary>
    /// <param name="prefix">The search prefix for autocomplete</param>
    /// <param name="limit">Maximum number of suggestions to return (default: 5)</param>
    /// <returns>List of autocomplete suggestions</returns>
    [HttpGet("autocomplete")]
    public ActionResult<AutocompleteResponse> Autocomplete([FromQuery] string prefix, [FromQuery] int limit = 5)
    {
        if (string.IsNullOrWhiteSpace(prefix))
        {
            return BadRequest(new { error = "Prefix parameter is required and cannot be empty" });
        }

        if (limit <= 0 || limit > 100)
        {
            return BadRequest(new { error = "Limit must be between 1 and 100" });
        }

        try
        {
            var suggestions = _autocompleteService.Query(prefix, limit);

            return Ok(new AutocompleteResponse
            {
                Prefix = prefix,
                SuggestionCount = suggestions.Count,
                Suggestions = suggestions
            });
        }
        catch (Exception ex)
        {
            return StatusCode(500,
                new { error = "An error occurred while processing autocomplete", details = ex.Message });
        }
    }

    [HttpPost("add-posts")]
    public ActionResult<AddPostsResponse> AddPosts([FromBody] AddPostsRequest addPostsRequest)
    {
        // 1. Log Request Entry
        int requestedCount = addPostsRequest?.Posts?.Count ?? 0;
        Console.WriteLine($"\n[{(DateTime.Now):HH:mm:ss}] API RECEIVED: AddPosts request with {requestedCount} items.");

        if (addPostsRequest?.Posts == null || requestedCount == 0)
        {
            Console.WriteLine(" --> [REJECTED] Request body was null or empty.");
            return BadRequest(new { error = "Posts list is required and cannot be empty." });
        }

        if (requestedCount > 1000)
        {
            Console.WriteLine($" --> [REJECTED] Batch size {requestedCount} exceeds 1000 limit.");
            return BadRequest(new { error = "Batch size exceeds limit of 1000 posts." });
        }

        var failedPosts = new List<FailedPost>();
        int successCount = 0;

        try
        {
            foreach (var post in addPostsRequest.Posts)
            {
                if (post.PostId <= 0)
                {
                    Console.WriteLine($" --> [WARN] Skipping post with invalid ID: {post.PostId}");
                    failedPosts.Add(new FailedPost { Error = "PostId must be > 0", PostData = post });
                    continue;
                }

                try
                {
                    // Call the QueryEngine to index the post
                    _queryEngine.AddPost(post);
                    successCount++;
                    
                    // Optional: Log every 100th post for large batches to avoid console spam
                    if (successCount % 100 == 0)
                    {
                        Console.WriteLine($" --> [PROGRESS] Indexed {successCount}/{requestedCount}...");
                    }
                }
                catch (Exception ex)
                {
                    Console.WriteLine($" --> [ERROR] Failed to index PostId {post.PostId}: {ex.Message}");
                    failedPosts.Add(new FailedPost { Error = ex.Message, PostData = post });
                }
            }

            return Ok(new AddPostsResponse
            {
                SuccessCount = successCount,
                FailureCount = failedPosts.Count,
                TotalRequested = requestedCount,
                FailedPosts = failedPosts,
                Message = $"Processed {requestedCount} posts."
            });
        }
        catch (Exception ex)
        {
            Console.WriteLine($"!!! [CRITICAL FAILURE] !!! {ex.Message}");
            return StatusCode(500, new { error = "Critical error.", details = ex.Message });
        }
    }

}

/// <summary>
/// Response model for search query endpoint
/// </summary>
public class SearchQueryResponse
{
    public string Query { get; set; }
    public int ResultCount { get; set; }
    public List<Post> Posts { get; set; }
}

/// <summary>
/// Response model for autocomplete endpoint
/// </summary>
public class AutocompleteResponse
{
    public string Prefix { get; set; }
    public int SuggestionCount { get; set; }
    public List<string> Suggestions { get; set; }
}

/// <summary>
/// Request model for adding posts endpoint
/// </summary>
public class AddPostsRequest
{
    public List<Post> Posts { get; set; }
}

/// <summary>
/// Response model for adding posts endpoint
/// </summary>
public class AddPostsResponse
{
    public int SuccessCount { get; set; }
    public int FailureCount { get; set; }
    public int TotalRequested { get; set; }
    public string Message { get; set; }
    public List<FailedPost> FailedPosts { get; set; }
}

/// <summary>
/// Represents a post that failed to be added
/// </summary>
public class FailedPost
{
    public Post PostData { get; set; }
    public string Error { get; set; }
}