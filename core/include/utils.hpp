#ifndef UTILS_H
#define UTILS_H
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "compound_key.hpp"
#include "isam_storage.hpp"
#include "post.hpp"
#include "comment.hpp"
#include "lexicon.hpp"


// A class with helpful static functions, and functions to generate the needed files
class Utils {
public:
    // Write comments and posts into data index as JSON
    static void generate_data_index(ISAMStorage& data_index, const std::string& post_file);

    // Create the lexicon from the data index
    static Lexicon generate_lexicon(ISAMStorage& data_index);


    static std::vector<std::string> parse_tags(const std::string& tags_str);

    static std::string extract_text_from_html(const std::string& html);


    // Parse Posts.xml into Post objects with configurable limit
    static std::vector<Post> parse_posts_from_xml(const std::string& xml_file_path,
                                          SiteID site_id,
                                          size_t limit = SIZE_MAX);

    // Parse Comments.xml into Commment objects with configurable limit
    static std::vector<Comment> parse_comments_from_xml(const std::string& xml_file_path,
                                           SiteID site_id,
                                           size_t limit = SIZE_MAX);


};
#endif //UTILS_H
