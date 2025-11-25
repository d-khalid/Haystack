#ifndef POST_HPP
#define POST_HPP
#include <vector>
#include "nlohmann/json.hpp"
#include "compound_key.hpp"

class Post {
public:
    SiteID site_id;
    uint32_t post_id;
    uint32_t post_type_id; // 1=Question, 2=Answer

    // Content
    std::string title; // Post title (for questions only)
    std::string body; // HTML body content
    std::vector<std::string> tags; // Tags (for questions only)

    std::string cleaned_body; // The actual text in the html, computed (not in the data)

    // Relationships
    std::optional<uint32_t> parent_id; // For answers: parent question ID
    std::optional<uint32_t> accepted_answer_id; // For questions: accepted answer ID

    // User info
    std::optional<uint32_t> owner_user_id; // Original poster
    std::optional<uint32_t> last_editor_user_id; // Last person to edit

    // Metrics
    int32_t score; // Upvotes - downvotes
    uint32_t view_count; // Number of views (questions only)
    uint32_t answer_count; // Number of answers (questions only)
    uint32_t comment_count; // Number of comments

    // Timestamps
    std::string creation_date; // When post was created
    std::string last_edit_date; // When last edited (optional)
    std::string last_activity_date; // Last activity on post

    // Metadata
    std::string content_license; // License (e.g., "CC BY-SA 3.0")


    // Default constructor
    Post() : site_id(SiteID::ASK_UBUNTU), post_id(0), post_type_id(1),
             score(0), view_count(0), answer_count(0), comment_count(0) {
    }

    // Serialization functions (because C++ doesn't have reflection!)
    static nlohmann::json to_json(const Post &post) {
        return nlohmann::json{
            {"site_id", static_cast<int>(post.site_id)},
            {"post_id", post.post_id},
            {"post_type_id", post.post_type_id},
            {"title", post.title},
            {"body", post.body},
            {"tags", post.tags},
            {"cleaned_body", post.cleaned_body},
            {"parent_id", post.parent_id},
            {"accepted_answer_id", post.accepted_answer_id},
            {"owner_user_id", post.owner_user_id},
            {"last_editor_user_id", post.last_editor_user_id},
            {"score", post.score},
            {"view_count", post.view_count},
            {"answer_count", post.answer_count},
            {"comment_count", post.comment_count},
            {"creation_date", post.creation_date},
            {"last_edit_date", post.last_edit_date},
            {"last_activity_date", post.last_activity_date},
            {"content_license", post.content_license}
        };
    }

    static Post from_json(const nlohmann::json &j) {
        Post post;

        j.at("site_id").get_to(post.site_id);
        j.at("post_id").get_to(post.post_id);
        j.at("post_type_id").get_to(post.post_type_id);
        j.at("title").get_to(post.title);
        j.at("body").get_to(post.body);
        j.at("tags").get_to(post.tags);
        j.at("cleaned_body").get_to(post.cleaned_body);

        if (j.contains("parent_id") && !j["parent_id"].is_null()) {
            post.parent_id = j["parent_id"].get<uint32_t>();
        } else {
            post.parent_id = std::nullopt;
        }

        if (j.contains("accepted_answer_id") && !j["accepted_answer_id"].is_null()) {
            post.accepted_answer_id = j["accepted_answer_id"].get<uint32_t>();
        } else {
            post.accepted_answer_id = std::nullopt;
        }

        if (j.contains("owner_user_id") && !j["owner_user_id"].is_null()) {
            post.owner_user_id = j["owner_user_id"].get<uint32_t>();
        } else {
            post.owner_user_id = std::nullopt;
        }

        if (j.contains("last_editor_user_id") && !j["last_editor_user_id"].is_null()) {
            post.last_editor_user_id = j["last_editor_user_id"].get<uint32_t>();
        } else {
            post.last_editor_user_id = std::nullopt;
        }

        j.at("score").get_to(post.score);
        j.at("view_count").get_to(post.view_count);
        j.at("answer_count").get_to(post.answer_count);
        j.at("comment_count").get_to(post.comment_count);
        j.at("creation_date").get_to(post.creation_date);
        j.at("last_edit_date").get_to(post.last_edit_date);
        j.at("last_activity_date").get_to(post.last_activity_date);
        j.at("content_license").get_to(post.content_license);

        return post;
    }
};


#endif //POST_HPP
