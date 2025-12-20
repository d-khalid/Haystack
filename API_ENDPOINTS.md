# Haystack API Endpoints Documentation

## Overview
The SearchController provides three main API endpoints for searching, autocomplete, and post indexing functionality.

## Getting Started

### Starting the Web API

**Option 1: Start with default directory (`stackexchange-data`)**
```bash
./Haystack.exe
```

**Option 2: Start with a specific data directory**
```bash
./Haystack.exe "path/to/data/directory"
```

The API will attempt to load the following files from the specified directory:
- `lexicon.txt` - Indexed vocabulary
- `vocab.txt` - Autocomplete vocabulary
- `data_index/` - ISAM storage files
- `barrel_*.dat` - Inverted index barrels

**Example:**
```bash
# Start API using data from the stackexchange-data directory
./Haystack.exe stackexchange-data
```

## Base URL
```
/api/search
```

---

## Endpoints

### 1. Query Search
**Endpoint:** `GET /api/search/query`

**Description:** Searches the index for posts matching the query string. Returns a list of posts ranked by relevance.

**Query Parameters:**
- `query` (string, required): The search query text
- `limit` (integer, optional): Maximum number of results to return. Range: 1-1000. Default: 20

**Response (200 OK):**
```json
{
  "query": "search term",
  "resultCount": 5,
  "posts": [
    {
      "siteId": 1,
      "postId": 12345,
      "postTypeId": 1,
      "title": "Question Title",
      "body": "Full HTML body...",
      "tags": ["tag1", "tag2"],
      "cleanedBody": "Cleaned text body...",
      "parentId": null,
      "acceptedAnswerId": 67890,
      "ownerUserId": 999,
      "lastEditorUserId": 1000,
      "score": 42,
      "viewCount": 1500,
      "answerCount": 3,
      "commentCount": 5,
      "creationDate": "2023-01-15T10:30:00",
      "lastEditDate": "2023-02-20T14:25:00",
      "lastActivityDate": "2023-02-20T14:25:00",
      "contentLicense": "CC BY-SA 4.0"
    }
  ]
}
```

**Error Responses:**
- `400 Bad Request`: If query is empty or limit is invalid
- `500 Internal Server Error`: If an error occurs during search processing

**Example Requests:**
```bash
# Basic search
GET /api/search/query?query=csharp

# Search with limit
GET /api/search/query?query=async+await&limit=10

# Search with custom limit
GET /api/search/query?query=linq&limit=50
```

---

### 2. Autocomplete
**Endpoint:** `GET /api/search/autocomplete`

**Description:** Provides autocomplete suggestions based on a prefix. Returns the most likely words that start with the given prefix, ranked by frequency.

**Query Parameters:**
- `prefix` (string, required): The search prefix for autocomplete
- `limit` (integer, optional): Maximum number of suggestions to return. Range: 1-100. Default: 5

**Response (200 OK):**
```json
{
  "prefix": "data",
  "suggestionCount": 5,
  "suggestions": [
    "database",
    "dataframe",
    "datatype",
    "dataset",
    "datasource"
  ]
}
```

**Error Responses:**
- `400 Bad Request`: If prefix is empty or limit is invalid
- `500 Internal Server Error`: If an error occurs during autocomplete processing

**Example Requests:**
```bash
# Basic autocomplete
GET /api/search/autocomplete?prefix=async

# Autocomplete with limit
GET /api/search/autocomplete?prefix=data&limit=10

# Autocomplete with maximum limit
GET /api/search/autocomplete?prefix=linq&limit=50
```

---

### 3. Get Post
**Endpoint:** `GET /api/search/post/{postId}`

**Description:** Retrieves a single post by its ID. Returns the complete post object with all details.

**Path Parameters:**
- `postId` (integer, required): The ID of the post to retrieve

**Response (200 OK):**
```json
{
  "siteId": 1,
  "postId": 12345,
  "postTypeId": 1,
  "title": "Question Title",
  "body": "Full HTML body...",
  "tags": ["tag1", "tag2"],
  "cleanedBody": "Cleaned text body...",
  "parentId": null,
  "acceptedAnswerId": 67890,
  "ownerUserId": 999,
  "lastEditorUserId": 1000,
  "score": 42,
  "viewCount": 1500,
  "answerCount": 3,
  "commentCount": 5,
  "creationDate": "2023-01-15T10:30:00",
  "lastEditDate": "2023-02-20T14:25:00",
  "lastActivityDate": "2023-02-20T14:25:00",
  "contentLicense": "CC BY-SA 4.0"
}
```

**Error Responses:**
- `400 Bad Request`: If postId is zero or invalid
- `404 Not Found`: If the post does not exist
- `500 Internal Server Error`: If an error occurs during retrieval

**Example Requests:**
```bash
# Get a specific post
GET /api/search/post/12345

# Get another post
GET /api/search/post/67890
```

---

### 4. Add Posts
**Endpoint:** `POST /api/search/add-posts`

**Description:** Adds new posts to the runtime memory buffer (delta index). Posts are indexed immediately and become searchable, but are not persisted to disk until `FlushDeltaToDisk` is called. This is useful for real-time indexing of newly added content.

**Request Body:**
```json
{
  "posts": [
    {
      "siteId": 1,
      "postId": 99999,
      "postTypeId": 1,
      "title": "New Question Title",
      "body": "<p>HTML body content</p>",
      "tags": ["csharp", "async"],
      "cleanedBody": "Cleaned text without HTML tags",
      "parentId": null,
      "acceptedAnswerId": null,
      "ownerUserId": 555,
      "lastEditorUserId": 555,
      "score": 0,
      "viewCount": 0,
      "answerCount": 0,
      "commentCount": 0,
      "creationDate": "2025-12-19T10:30:00",
      "lastEditDate": "2025-12-19T10:30:00",
      "lastActivityDate": "2025-12-19T10:30:00",
      "contentLicense": "CC BY-SA 4.0"
    },
    {
      "siteId": 1,
      "postId": 100000,
      "postTypeId": 2,
      "title": "Answer to question",
      "body": "<p>Answer content</p>",
      "tags": [],
      "cleanedBody": "This is an answer to the question",
      "parentId": 99999,
      "acceptedAnswerId": null,
      "ownerUserId": 556,
      "lastEditorUserId": 556,
      "score": 5,
      "viewCount": 10,
      "answerCount": 0,
      "commentCount": 1,
      "creationDate": "2025-12-19T11:00:00",
      "lastEditDate": "2025-12-19T11:00:00",
      "lastActivityDate": "2025-12-19T11:00:00",
      "contentLicense": "CC BY-SA 4.0"
    }
  ]
}
```

**Response (200 OK):**
```json
{
  "successCount": 2,
  "failureCount": 0,
  "totalRequested": 2,
  "message": "Successfully added 2 out of 2 posts to the runtime index",
  "failedPosts": []
}
```

**Response with Partial Failures (200 OK):**
```json
{
  "successCount": 1,
  "failureCount": 1,
  "totalRequested": 2,
  "message": "Successfully added 1 out of 2 posts to the runtime index",
  "failedPosts": [
    {
      "postData": { /* post object that failed */ },
      "error": "PostId is required and cannot be zero"
    }
  ]
}
```

**Validation Rules:**
- `PostId` must be non-zero
- At least one of `Title` or `CleanedBody` must be non-empty
- Maximum 1000 posts per request

**Error Responses:**
- `400 Bad Request`: If posts list is empty or contains more than 1000 posts
- `500 Internal Server Error`: If an unexpected error occurs during processing

**Example Request (cURL):**
```bash
curl -X POST http://localhost:5000/api/search/add-posts \
  -H "Content-Type: application/json" \
  -d '{
    "posts": [
      {
        "siteId": 1,
        "postId": 99999,
        "postTypeId": 1,
        "title": "How to use async/await?",
        "body": "<p>Question body</p>",
        "tags": ["csharp", "async"],
        "cleanedBody": "How to use async await in csharp",
        "parentId": null,
        "acceptedAnswerId": null,
        "ownerUserId": 555,
        "lastEditorUserId": 555,
        "score": 0,
        "viewCount": 0,
        "answerCount": 0,
        "commentCount": 0,
        "creationDate": "2025-12-19T10:30:00",
        "lastEditDate": "2025-12-19T10:30:00",
        "lastActivityDate": "2025-12-19T10:30:00",
        "contentLicense": "CC BY-SA 4.0"
      }
    ]
  }'
```

**Important Notes:**
1. Posts added to the runtime buffer are immediately searchable
2. Posts are NOT persisted to disk automatically
3. A background task automatically flushes the delta index to disk every 2 hours
4. To persist added posts immediately without waiting, a separate flush endpoint could be implemented
5. The endpoint provides detailed error information for failed posts so you can retry or fix individual items
6. Use this endpoint for real-time indexing of newly created content

---

## Background Tasks

### Delta Index Auto-Flush
- **Schedule:** Every 2 hours
- **Purpose:** Automatically persists posts added via the `/api/search/add-posts` endpoint to the disk-based barrels
- **Logging:** Console logs with timestamps indicate when flushes start and complete
- **Error Handling:** Failed flushes are logged but don't interrupt the application; the task continues running

## Configuration

The API requires the following configuration in `appsettings.json`:

```json
{
  "DataDirectory": "stackexchange-data",
  "NumBarrels": 4
}
```

**Parameters:**
- `DataDirectory`: Path to the directory containing indexed data files (lexicon.txt, vocab.txt, data_index, etc.)
- `NumBarrels`: Number of index barrels used during indexing

### CORS Configuration

The API is configured to accept requests from the following origins:
- `http://localhost:4200` - Local frontend development server
- `http://20.255.48.227` - Remote frontend server

All HTTP methods and headers are allowed from these origins. To modify the allowed origins, update the CORS policy in `Program.cs`:

```csharp
corsBuilder.WithOrigins("http://localhost:4200", "http://20.255.48.227")
           .AllowAnyMethod()
           .AllowAnyHeader();
```

---

## Service Dependencies

The SearchController depends on the following services (automatically injected):

1. **QueryEngine**: Performs full-text search queries
2. **AutocompleteService**: Provides word suggestions based on prefix
3. **ISAMStorage**: Retrieves post data from the indexed storage

All services are registered in `Program.cs` during application startup.

---

## Deployment Notes

1. Ensure the `stackexchange-data` directory (or configured `DataDirectory`) exists and contains:
   - `lexicon.txt` - Lexicon of indexed words
   - `vocab.txt` - Vocabulary for autocomplete
   - `data_index/` - ISAM storage files
   - `barrel_*.dat` - Inverted index barrels

2. The API runs on the default ASP.NET Core port (typically 5000 for HTTP or 5001 for HTTPS)

3. No authentication is currently implemented. Consider adding authentication before production deployment.
