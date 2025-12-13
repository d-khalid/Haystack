#include "reverse_index.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <filesystem>

const ReverseIndex::postings_list_t ReverseIndex::EMPTY_POSTINGS_LIST = {};

ReverseIndex::ReverseIndex(int num_barrels) : num_barrels_(num_barrels) {
    if (num_barrels_ < 1) num_barrels_ = 1;
    index_shards_.resize(num_barrels_);
}

// Helper function to parse word IDs and masks from forward index
static std::vector<std::pair<uint32_t, uint8_t>> parse_word_ids_with_masks(const std::string& data) {
    std::vector<std::pair<uint32_t, uint8_t>> word_info;
    if (data.empty()) return word_info;

    std::stringstream ss(data);
    std::string segment;

    while (std::getline(ss, segment, ' ')) {
        if (segment.empty()) continue;
        size_t comma_pos = segment.find(',');
        if (comma_pos == std::string::npos) continue;

        try {
            uint32_t word_id = std::stoul(segment.substr(0, comma_pos));
            uint8_t mask = static_cast<uint8_t>(std::stoi(segment.substr(comma_pos + 1)));
            word_info.emplace_back(word_id, mask);
        } catch (...) { continue; }
    }
    return word_info;
}

//  Build Reverse Index 
bool ReverseIndex::build(ISAMStorage& forward_index, const Lexicon& lexicon) {
    std::cout << "Starting reverse index construction with " << num_barrels_ << " barrels..." << std::endl;
    int count = 0;

    for (auto& shard : index_shards_) shard.clear();
    all_words_.clear();

    forward_index.reset_iterator();
    while (true) {
        auto entry = forward_index.next();
        if (!entry.has_value()) break;

        CompoundKey key = CompoundKey::unpack(entry->first);
        uint32_t doc_id = key.primary_id;
        std::string word_ids_str = entry->second;

        
        auto word_info = parse_word_ids_with_masks(word_ids_str);

        // Only do this if autocomplete is enabled
Lexicon& lex = const_cast<Lexicon&>(lexicon);  // Remove const to call get_word safely

for (auto word : word_info) {
    uint32_t word_id = word.first;
    if (word_id != 0) {
        //  BARREL LOGIC
        int barrel_id = word_id % num_barrels_;
        index_shards_[barrel_id][word_id].push_back({doc_id});

        //  AUTOCOMPLETE LOGIC 
        std::string w = lex.get_word(word_id);  
        if(!w.empty()) all_words_.insert(w);
    }
}


        
        count++;
        if (count % 1000 == 0) std::cout << "\rProcessed " << count << " forward index entries..." << std::flush;
    }

    std::cout << std::endl << "Reverse index built successfully." << std::endl;
    std::cout << "Total unique terms indexed across all barrels: " << total_terms() << std::endl;

    // Build autocomplete trie
    build_autocomplete_trie();

    return true;
}

//  Save Barrels 
void ReverseIndex::save_barrels(const std::string& directory) {
    std::cout << "Writing " << num_barrels_ << " barrels to disk..." << std::endl;

    for (int i = 0; i < num_barrels_; ++i) {
        std::string idx_path = directory + "/barrel_" + std::to_string(i) + ".idx";
        std::string dat_path = directory + "/barrel_" + std::to_string(i) + ".dat";
        ISAMStorage barrel_store(idx_path, dat_path);

        std::vector<std::pair<uint64_t, std::string>> data_to_write;
        const auto& current_shard = index_shards_[i];

        for (const auto& p : current_shard) {
            uint64_t word_id = p.first;
            std::stringstream ss;
            for (size_t k = 0; k < p.second.size(); ++k) {
                ss << p.second[k].doc_id;
                if (k < p.second.size() - 1) ss << ",";
            }
            data_to_write.emplace_back(word_id, ss.str());
        }

        if (!data_to_write.empty()) barrel_store.write(data_to_write);
    }
}

//  Search Barrel 
ReverseIndex::postings_list_t ReverseIndex::search_barrel(const std::string& directory, int barrel_id, int word_id) {
    std::string idx_path = directory + "/barrel_" + std::to_string(barrel_id) + ".idx";
    std::string dat_path = directory + "/barrel_" + std::to_string(barrel_id) + ".dat";

    if (!std::filesystem::exists(idx_path) || !std::filesystem::exists(dat_path)) return EMPTY_POSTINGS_LIST;

    ISAMStorage barrel_store(idx_path, dat_path);
    auto result = barrel_store.read(word_id);
    if (!result.has_value()) return EMPTY_POSTINGS_LIST;

    postings_list_t postings;
    std::stringstream ss(result->second);
    std::string segment;
    while(std::getline(ss, segment, ',')) {
        if(!segment.empty()) postings.push_back({(uint32_t)std::stoul(segment)});
    }

    return postings;
}

// Total Terms
size_t ReverseIndex::total_terms() const {
    size_t t = 0;
    for (const auto& shard : index_shards_) t += shard.size();
    return t;
}

//Autocomplete 
void ReverseIndex::build_autocomplete_trie() {
    for (const auto& w : all_words_) {
        autocomplete_trie_.insert(w);
    }
    std::cout << "Autocomplete Trie built with " << all_words_.size() << " words." << std::endl;
}

std::vector<std::string> ReverseIndex::autocomplete(const std::string& prefix, int limit) const {
    std::vector<std::string> results;
    autocomplete_trie_.autocomplete(prefix, results, limit);
    return results;
}
