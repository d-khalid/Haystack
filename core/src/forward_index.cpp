#include "forward_index.hpp"
#include <nlohmann/json.hpp>

std::vector<ForwardIndex::Entry>
ForwardIndex::build(ISAMStorage& storage, Lexicon& lexicon)
{
    std::vector<Entry> result;

    while (true) {
        auto p = storage.next();
        if (!p.has_value()) break;

        CompoundKey key = p->first;
        std::string raw = p->second;

        nlohmann::json j = nlohmann::json::parse(raw);
        Post post = Post::from_json(j);

       auto tokens = Lexicon::tokenize_text(post.cleaned_body);

        std::vector<int> ids;
        ids.reserve(tokens.size());

        for (auto& t : tokens) {
            int id = lexicon.get_word_id(t);
            if (id != 0)
                ids.push_back(id);
        }

        result.push_back({ key, ids });
    }

    return result;
}

void ForwardIndex::write_to_disk(
    const std::vector<Entry>& entries,
    const std::string& index_file,
    const std::string& data_file
) {
    ISAMStorage store(index_file, data_file);

    std::vector<std::pair<CompoundKey, std::string>> to_write;
    to_write.reserve(entries.size());

    for (auto& e : entries) {
        std::string data;

        for (size_t i = 0; i < e.word_ids.size(); i++) {
            data += std::to_string(e.word_ids[i]);
            if (i + 1 < e.word_ids.size())
                data += ",";
        }

        to_write.push_back({ e.key, data });
    }

    store.write(to_write);
}
