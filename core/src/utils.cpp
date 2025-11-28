#include "utils.hpp"
#include <iostream>
#include <sstream>
#include "pugixml.hpp"

// Helper function to parse tags from "|tag1|tag2|" format
std::vector<std::string> Utils::parse_tags(const std::string& tags_str) {
    std::vector<std::string> tags;
    if (tags_str.empty() || tags_str.length() < 2) {
        return tags;
    }

    // Remove leading and trailing |
    std::string clean_tags = tags_str;
    if (clean_tags.front() == '|') clean_tags.erase(0, 1);
    if (clean_tags.back() == '|') clean_tags.pop_back();

    // Split by |
    std::stringstream ss(clean_tags);
    std::string tag;
    while (std::getline(ss, tag, '|')) {
        if (!tag.empty()) {
            tags.push_back(tag);
        }
    }

    return tags;
}

std::string Utils::extract_text_from_html(const std::string& html) {


    auto wrapped_html = "<html><body><div>" + html + "</div></body></html>";
    HtmlParser parser;
    shared_ptr<HtmlDocument> doc = parser.Parse(wrapped_html.c_str(), wrapped_html.size());

    if (!doc) {
        return "";
    }

    // Use XPath to select all text nodes
    std::vector<shared_ptr<HtmlElement>> text_nodes = doc->SelectElement("//");

    std::string result;
    for (const auto& node : text_nodes) {
        std::string text = node->GetValue();
        if (!text.empty()) {
            // Clean up whitespace and concatenate
            result += text + " ";
        }
    }

    // Remove extra whitespace
    if (!result.empty()) {
        result.erase(result.find_last_not_of(" \n\r\t") + 1);
    }

    return result;
}


// Function to parse XML into Post objects with configurable limit
std::vector<Post> Utils::parse_posts_from_xml(const std::string& xml_file_path,
                                      SiteID site_id,
                                      size_t limit) {
    std::vector<Post> posts;

    // Load XML document
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(xml_file_path.c_str());

    if (!result) {
        std::cerr << "XML parse error: " << result.description()
                  << " at offset " << result.offset << std::endl;
        return posts;
    }

    // Find all row elements
    pugi::xml_node posts_node = doc.child("posts");
    if (!posts_node) {
        std::cerr << "No 'posts' root element found" << std::endl;
        return posts;
    }

    size_t count = 0;
    std::cout << "Loading posts into memory..." << std::flush;

    // Iterate through each row
    for (pugi::xml_node row : posts_node.children("row")) {
        if (count >= limit) {
            break; // Stop when we hit the limit
        }

        Post post;

        // Basic identification
        post.site_id = site_id;
        post.post_id = row.attribute("Id").as_uint();
        post.post_type_id = row.attribute("PostTypeId").as_uint();

        // Content
        if (auto title_attr = row.attribute("Title")) {
            post.title = title_attr.as_string();
        }

        if (auto body_attr = row.attribute("Body")) {
            post.body = body_attr.as_string();

            // Use html_parser to get the clean text output.
            HtmlParser parser;
            shared_ptr<HtmlDocument> doc = parser.Parse(post.body.c_str(), post.body.size());

            post.cleaned_body = doc->text();
        }

        if (auto tags_attr = row.attribute("Tags")) {
            post.tags = parse_tags(tags_attr.as_string());
        }

        // Relationships
        if (auto parent_attr = row.attribute("ParentId")) {
            post.parent_id = parent_attr.as_uint();
        }

        if (auto accepted_attr = row.attribute("AcceptedAnswerId")) {
            post.accepted_answer_id = accepted_attr.as_uint();
        }

        // User info
        if (auto owner_attr = row.attribute("OwnerUserId")) {
            post.owner_user_id = owner_attr.as_uint();
        }

        if (auto editor_attr = row.attribute("LastEditorUserId")) {
            post.last_editor_user_id = editor_attr.as_uint();
        }

        // Metrics
        post.score = row.attribute("Score").as_int();
        post.view_count = row.attribute("ViewCount").as_uint();
        post.answer_count = row.attribute("AnswerCount").as_uint();
        post.comment_count = row.attribute("CommentCount").as_uint();

        // Timestamps
        if (auto creation_attr = row.attribute("CreationDate")) {
            post.creation_date = creation_attr.as_string();
        }

        if (auto edit_attr = row.attribute("LastEditDate")) {
            post.last_edit_date = edit_attr.as_string();
        }

        if (auto activity_attr = row.attribute("LastActivityDate")) {
            post.last_activity_date = activity_attr.as_string();
        }

        // Metadata
        if (auto license_attr = row.attribute("ContentLicense")) {
            post.content_license = license_attr.as_string();
        }

        posts.push_back(post);
        std::cout << "\rLoaded " << count + 1 << " posts into memory." << std::flush;
        count++;
    }
    std::cout << std::endl << "Loading Complete!" << std::endl;

    return posts;
}

std::vector<Comment> Utils::parse_comments_from_xml(const std::string& xml_file_path,
                                           SiteID site_id,
                                           size_t limit) {
    std::vector<Comment> comments;

    // Load XML document
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(xml_file_path.c_str());

    if (!result) {
        std::cerr << "XML parse error: " << result.description()
                  << " at offset " << result.offset << std::endl;
        return comments;
    }

    // Find all row elements
    pugi::xml_node comments_node = doc.child("comments");
    if (!comments_node) {
        std::cerr << "No 'comments' root element found" << std::endl;
        return comments;
    }

    size_t count = 0;
    std::cout << "Loading comments into memory..." << std::flush;

    // Iterate through each row
    for (pugi::xml_node row : comments_node.children("row")) {
        if (limit > 0 && count >= limit) {
            break; // Stop when we hit the limit
        }

        Comment comment;

        // Basic identification
        comment.site_id = site_id;
        comment.comment_id = row.attribute("Id").as_uint();
        comment.post_id = row.attribute("PostId").as_uint();

        // Content
        if (auto text_attr = row.attribute("Text")) {
            comment.text = text_attr.as_string();
        }

        // User info
        if (auto user_attr = row.attribute("UserId")) {
            comment.user_id = user_attr.as_uint();
        }

        // Metrics
        comment.score = row.attribute("Score").as_int();

        // Timestamps
        if (auto creation_attr = row.attribute("CreationDate")) {
            comment.creation_date = creation_attr.as_string();
        }

        comments.push_back(comment);
        std::cout << "\rLoaded " << count + 1 << " comments into memory." << std::flush;
        count++;
    }
    std::cout << std::endl << "Loading Complete!" << std::endl;

    return comments;
}

void Utils::generate_data_index(ISAMStorage& data_index, const std::string& post_file, const std::string& comments_file) {
    auto posts = parse_posts_from_xml(post_file, SiteID::ASK_UBUNTU);

    // Generate entries
    std::vector<std::pair<CompoundKey, std::string>> p_entries;
    p_entries.reserve(posts.size());


    KeyType p_type = KeyType::POST_BY_ID;
    SiteID p_site = SiteID::ASK_UBUNTU;
    for (const auto& post : posts) {
        uint32_t p_id = post.post_id;

        CompoundKey k(static_cast<uint8_t>(p_type), static_cast<uint16_t>(p_site), p_id, 0);

        nlohmann::json j = Post::to_json(post);

        p_entries.emplace_back(k, j.dump(4));
    }

    std::cout << "Writing post data...." << std::endl;
    data_index.write(p_entries);
    std::cout << "Data for " << posts.size() << " posts written to data index.\n" << std::endl;

    // COMMENTS
    auto comments = parse_comments_from_xml(comments_file, SiteID::ASK_UBUNTU);

    // Generate entries
    std::vector<std::pair<CompoundKey, std::string>> c_entries;
    c_entries.reserve(comments.size());


    KeyType c_type = KeyType::COMMENT_BY_ID;
    SiteID c_site = SiteID::ASK_UBUNTU;
    for (const auto& comment : comments) {
        uint32_t c_id = comment.post_id;

        CompoundKey k(static_cast<uint8_t>(c_type), static_cast<uint16_t>(c_site), c_id, 0);

        nlohmann::json j = Comment::to_json(comment);

        c_entries.emplace_back(k, j.dump(4));
    }

    std::cout << "Writing comment data...." << std::endl;
    data_index.write(c_entries);
    std::cout << "Data for " << comments.size() << " comments written to data index." << std::endl;
}

Lexicon Utils::generate_lexicon(ISAMStorage &data_index) {
    Lexicon l;

    int count = 0;
    std::cout << "Adding words to lexicon..." << std::endl;
    while (true) {
        auto entry = data_index.next();

        if (!entry.has_value()) break;

        auto k_type = static_cast<KeyType>(entry->first.key_type);

        if (k_type == KeyType::POST_BY_ID) {
            Post p = Post::from_json(nlohmann::json::parse(entry->second));

            if (p.post_type_id == 1) {  // Go over body, title and tags for questions
                l.add_words(Lexicon::tokenize_text(p.title));
                l.add_words(Lexicon::tokenize_text(p.cleaned_body));

                // Add tags after normalization
                std::vector<std::string> n_tags(p.tags.size());
                for (const auto& t : p.tags) {
                    n_tags.emplace_back(Lexicon::normalize_token(t));
                }
                l.add_words(n_tags);
            }
            else if (p.post_type_id == 2) { // Go over body only for answers
                l.add_words(Lexicon::tokenize_text(p.cleaned_body));
            }
        } else if (k_type == KeyType::COMMENT_BY_ID) {
            Comment c = Comment::from_json(nlohmann::json::parse(entry->second));

            // Go over text field only
            l.add_words(Lexicon::tokenize_text(c.text));
        }
        std::cout << "\rLoaded " << count + 1 << " entries, lexicon has " << l.size() << " tokens." << std::flush;
        count++;
    }

    std::cout << std::endl << "Lexicon generation completed." << std::endl;

    return l;
}
