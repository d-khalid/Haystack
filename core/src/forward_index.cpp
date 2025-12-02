#include "forward_index.hpp"

#include <iostream>

#include "isam_storage.hpp"
#include "lexicon.hpp"
#include "post.hpp"
#include "nlohmann/json.hpp"

void
ForwardIndex::generate(ISAMStorage& output_store,
                       ISAMStorage& data_index,
                       Lexicon& lexicon)
{
    std::vector<std::pair<uint64_t, std::string>> result;
    result.reserve(data_index.size());

    int c = 0;
    while (true) {
        auto p = data_index.next();
        if (!p.has_value()) break;

        std::string raw = p->second;

        nlohmann::json j = nlohmann::json::parse(raw);
        Post post = Post::from_json(j);

        std::string data = "";

        // Format:
        // wordID,Mask (space separated)
        //
        // Masks:
        // 1 = Title
        // 2 = Body
        // 3 = Tag

        // ---------- TITLE ----------
        if (post.post_type_id == 1) {
            auto t_tokens = Lexicon::tokenize_text(post.title);
            for (auto& t : t_tokens) {
                uint64_t wid = lexicon.get_word_id(t);
                data += std::to_string(wid) + ",1 ";
            }
        }

        // ---------- BODY ----------
        auto b_tokens = Lexicon::tokenize_text(post.cleaned_body);
        for (auto& t : b_tokens) {
            uint64_t wid = lexicon.get_word_id(t);
            data += std::to_string(wid) + ",2 ";
        }

        // ---------- TAGS ----------
        for (auto& t : post.tags) {
            uint64_t wid = lexicon.get_word_id(t);
            data += std::to_string(wid) + ",3 ";
        }

        result.emplace_back(p->first, data);

        std::cout << "Indexed " << (c + 1)
                  << " entries in forward index." << std::flush;

        c++;
    }
    std::cout << std::endl;

    std::cout << "Writing entries to disk";
    output_store.write(result);
    std::cout << "done." << std::endl;
}
