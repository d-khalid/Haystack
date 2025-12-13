#ifndef REVERSE_INDEX_HPP
#define REVERSE_INDEX_HPP

#include <map>
#include <vector>
#include <string>
#include <set>
#include <memory>

#include "isam_storage.hpp"
#include "lexicon.hpp"

//  Posting
struct Posting {
    uint32_t doc_id;
};

// Trie Node for Autocomplete
struct TrieNode {
    bool is_end;
    std::map<char, std::unique_ptr<TrieNode>> children;

    TrieNode() : is_end(false) {}
};

class Trie {
public:
    Trie() : root_(std::make_unique<TrieNode>()) {}

    void insert(const std::string& word) {
        TrieNode* node = root_.get();
        for (char c : word) {
            if (!node->children[c]) node->children[c] = std::make_unique<TrieNode>();
            node = node->children[c].get();
        }
        node->is_end = true;
    }

    void autocomplete(const std::string& prefix, std::vector<std::string>& results, int limit = 10) const {
        const TrieNode* node = root_.get();
        for (char c : prefix) {
            auto it = node->children.find(c);
            if (it == node->children.end()) return;
            node = it->second.get();
        }
        dfs(node, prefix, results, limit);
    }

private:
    std::unique_ptr<TrieNode> root_;

    void dfs(const TrieNode* node, const std::string& path, std::vector<std::string>& results, int limit) const {
        if (results.size() >= (size_t)limit) return;
        if (node->is_end) results.push_back(path);
        for (const auto& [c, child] : node->children) {
            dfs(child.get(), path + c, results, limit);
        }
    }
};

// Reverse Index 
class ReverseIndex {
public:
    using postings_list_t = std::vector<Posting>;
    using index_map_t = std::map<int, postings_list_t>;

    explicit ReverseIndex(int num_barrels = 1);
    ~ReverseIndex() = default;

    bool build(ISAMStorage& forward_index, const Lexicon& lexicon);
    void save_barrels(const std::string& directory);
    static postings_list_t search_barrel(const std::string& directory, int barrel_id, int word_id);
    size_t total_terms() const;

    // Autocomplete
    void build_autocomplete_trie();
    std::vector<std::string> autocomplete(const std::string& prefix, int limit = 10) const;

private:
    int num_barrels_;
    std::vector<index_map_t> index_shards_;
    static const postings_list_t EMPTY_POSTINGS_LIST;

    // Autocomplete members
    Trie autocomplete_trie_;
    std::set<std::string> all_words_;
};

#endif // REVERSE_INDEX_HPP
