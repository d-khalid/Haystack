#ifndef COMMENT_HPP
#define COMMENT_HPP
#include <optional>
#include <nlohmann/json.hpp>

#include "compound_key.hpp"

class Comment {
public:
    SiteID site_id;
    uint32_t comment_id;
    uint32_t post_id; // Which post this comment belongs to

    // Content
    std::string text; // Comment text content, already clean

    // User info
    std::optional<uint32_t> user_id; // User who wrote the comment (optional - deleted users)

    // Metrics
    int32_t score; // Comment score (upvotes)

    // Timestamps
    std::string creation_date; // When comment was created

    // Default constructor
    Comment() : site_id(SiteID::ASK_UBUNTU), comment_id(0), post_id(0), score(0) {
    }
    // Serialization functions
    static nlohmann::json to_json(const Comment &comment) {
        return nlohmann::json{
                    {"site_id", static_cast<int>(comment.site_id)},
                    {"comment_id", comment.comment_id},
                    {"post_id", comment.post_id},
                    {"text", comment.text},
                    {"user_id", comment.user_id},
                    {"score", comment.score},
                    {"creation_date", comment.creation_date}
        };
    }

    static Comment from_json(const nlohmann::json &j) {
        Comment comment;

        j.at("site_id").get_to(comment.site_id);
        j.at("comment_id").get_to(comment.comment_id);
        j.at("post_id").get_to(comment.post_id);
        j.at("text").get_to(comment.text);

        if (j.contains("user_id") && !j["user_id"].is_null()) {
            comment.user_id = j["user_id"].get<uint32_t>();
        } else {
            comment.user_id = std::nullopt;
        }

        j.at("score").get_to(comment.score);
        j.at("creation_date").get_to(comment.creation_date);

        return comment;
    }
};

#endif //COMMENT_HPP
