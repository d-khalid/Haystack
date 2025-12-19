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

            // Retrieve full post objects from the data index
            var posts = new List<Post>();
            foreach (var postId in postIds)
            {
                var postData = _dataIndex.Get(postId);
                if (postData != null)
                {
                    try
                    {
                        var post = Post.Deserialize(postData);
                        posts.Add(post);
                    }
                    catch (Exception ex)
                    {
                        // Log and continue if a single post fails to deserialize
                        Console.WriteLine($"Failed to deserialize post {postId}: {ex.Message}");
                    }
                }
            }

            return Ok(new SearchQueryResponse
            {
                Query = query,
                ResultCount = posts.Count,
                Posts = posts
            });
        }
        catch (Exception ex)
        {
            return StatusCode(500, new { error = "An error occurred while processing the search", details = ex.Message });
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
            return StatusCode(500, new { error = "An error occurred while processing autocomplete", details = ex.Message });
        }
    }

    /// <summary>
    /// Adds new posts to the runtime memory buffer (delta index).
    /// Posts are indexed immediately but are not persisted to disk until FlushDeltaToDisk is called.
    /// </summary>
    /// <param name="addPostsRequest">Request containing a list of post objects to add</param>
    /// <returns>Response indicating success and number of posts added</returns>
    [HttpPost("add-posts")]
    public ActionResult<AddPostsResponse> AddPosts([FromBody] AddPostsRequest addPostsRequest)
    {
        if (addPostsRequest?.Posts == null || addPostsRequest.Posts.Count == 0)
        {
            return BadRequest(new { error = "Posts list is required and cannot be empty" });
        }

        if (addPostsRequest.Posts.Count > 1000)
        {
            return BadRequest(new { error = "Cannot add more than 1000 posts in a single request" });
        }

        try
        {
            int successCount = 0;
            var failedPosts = new List<FailedPost>();

            foreach (var post in addPostsRequest.Posts)
            {
                try
                {
                    // Validate essential post fields
                    if (post.PostId == 0)
                    {
                        failedPosts.Add(new FailedPost
                        {
                            PostData = post,
                            Error = "PostId is required and cannot be zero"
                        });
                        continue;
                    }

                    if (string.IsNullOrWhiteSpace(post.Title) && string.IsNullOrWhiteSpace(post.CleanedBody))
                    {
                        failedPosts.Add(new FailedPost
                        {
                            PostData = post,
                            Error = "At least one of Title or CleanedBody must be non-empty"
                        });
                        continue;
                    }

                    // Add the post to the query engine's runtime buffer
                    _queryEngine.AddPost(post);
                    successCount++;
                }
                catch (Exception ex)
                {
                    failedPosts.Add(new FailedPost
                    {
                        PostData = post,
                        Error = $"Failed to add post: {ex.Message}"
                    });
                }
            }

            return Ok(new AddPostsResponse
            {
                SuccessCount = successCount,
                FailureCount = failedPosts.Count,
                TotalRequested = addPostsRequest.Posts.Count,
                FailedPosts = failedPosts,
                Message = $"Successfully added {successCount} out of {addPostsRequest.Posts.Count} posts to the runtime index"
            });
        }
        catch (Exception ex)
        {
            return StatusCode(500, new { error = "An error occurred while adding posts", details = ex.Message });
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
