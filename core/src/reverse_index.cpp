#include "reverse_index.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <filesystem>

// Define the static empty list
const ReverseIndex::postings_list_t ReverseIndex::EMPTY_POSTINGS_LIST = {};

ReverseIndex::ReverseIndex(int num_barrels) : num_barrels_(num_barrels) {
    if (num_barrels_ < 1) num_barrels_ = 1;
    // Resize the vector to hold N separate maps
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

bool ReverseIndex::build(ISAMStorage& forward_index, const Lexicon& lexicon) {
    std::cout << "Starting reverse index construction with " << num_barrels_ << " barrels..." << std::endl;
    int count = 0;

    // Clear existing data
    for(auto& shard : index_shards_) {
        shard.clear();
    }

    forward_index.reset_iterator();

    while (true) {
        auto entry = forward_index.next();
        if (!entry.has_value()) break;

        CompoundKey key = CompoundKey::unpack(entry->first);
        uint32_t doc_id = key.primary_id;
        std::string word_ids_str = entry->second;

        auto word_info = parse_word_ids_with_masks(word_ids_str);

        for (auto word : word_info) {
            uint32_t word_id = word.first;
            if (word_id != 0) {
                // --- BARREL LOGIC ---
                // Calculate which barrel this word belongs to
                int barrel_id = word_id % num_barrels_;

                // Add to the specific map for that barrel
                index_shards_[barrel_id][word_id].push_back({doc_id});
            }
        }

        count++;
        if (count % 1000 == 0) {
            std::cout << "\rProcessed " << count << " forward index entries..." << std::flush;
        }
    }

    std::cout << std::endl << "Reverse index built successfully." << std::endl;
    std::cout << "Total unique terms indexed across all barrels: " << total_terms() << std::endl;
    return true;
}

void ReverseIndex::save_barrels(const std::string& directory) {
    std::cout << "Writing " << num_barrels_ << " barrels to disk..." << std::endl;

    for (int i = 0; i < num_barrels_; ++i) {
        std::string idx_path = directory + "/barrel_" + std::to_string(i) + ".idx";
        std::string dat_path = directory + "/barrel_" + std::to_string(i) + ".dat";

        // Create ISAMStorage for this specific barrel
        // Note: ISAMStorage constructor opens files.
        ISAMStorage barrel_store(idx_path, dat_path);

        std::vector<std::pair<uint64_t, std::string>> data_to_write;
        const auto& current_shard = index_shards_[i];

        // Convert the map for this barrel into data entries
        for (const auto& p : current_shard) {
            uint64_t word_id = p.first;

            std::stringstream ss;
            for (size_t k = 0; k < p.second.size(); ++k) {
                ss << p.second[k].doc_id;
                if (k < p.second.size() - 1) ss << ",";
            }

            data_to_write.emplace_back(word_id, ss.str());
        }

        if (!data_to_write.empty()) {
            barrel_store.write(data_to_write);
            std::cout << "Barrel " << i << ": wrote " << data_to_write.size() << " terms." << std::endl;
        } else {
            std::cout << "Barrel " << i << ": empty, skipping write." << std::endl;
        }
    }
}

ReverseIndex::postings_list_t ReverseIndex::search_barrel(const std::string& directory, int barrel_id, int word_id) {
    std::string idx_path = directory + "/barrel_" + std::to_string(barrel_id) + ".idx";
    std::string dat_path = directory + "/barrel_" + std::to_string(barrel_id) + ".dat";

    // Check if files exist
    if (!std::filesystem::exists(idx_path) || !std::filesystem::exists(dat_path)) {
        return EMPTY_POSTINGS_LIST;
    }

    // Load ONLY the relevant barrel
    // ISAMStorage loads the index into memory on construction
    ISAMStorage barrel_store(idx_path, dat_path);

    auto result = barrel_store.read(word_id);
    if (!result.has_value()) {
        return EMPTY_POSTINGS_LIST;
    }

    // Parse the result string "doc1,doc2,doc3" back into vector
    postings_list_t postings;
    std::stringstream ss(result->second);
    std::string segment;
    while(std::getline(ss, segment, ',')) {
        if(!segment.empty()) {
            postings.push_back({(uint32_t)std::stoul(segment)});
        }
    }

    return postings;
}

size_t ReverseIndex::total_terms() const {
    size_t t = 0;
    for(const auto& shard : index_shards_) t += shard.size();
    return t;
}