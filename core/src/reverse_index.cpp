#include "reverse_index.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>

// Define the static empty list
const ReverseIndex::postings_list_t ReverseIndex::EMPTY_POSTINGS_LIST = {};

// Helper function to parse word IDs and masks from forward index
static std::vector<std::pair<uint32_t, uint8_t>> parse_word_ids_with_masks(const std::string& data) {
    std::vector<std::pair<uint32_t, uint8_t>> word_info;

    if (data.empty()) {
        return word_info;
    }

    std::stringstream ss(data);
    std::string segment;

    while (std::getline(ss, segment, ' ')) {
        // Skip empty segments
        if (segment.empty()) {
            continue;
        }

        // Find the comma separator
        size_t comma_pos = segment.find(',');
        if (comma_pos == std::string::npos) {
            std::cerr << "Warning: No comma found in segment: " << segment << std::endl;
            continue;
        }

        try {
            // Parse word ID (before comma)
            uint32_t word_id = std::stoul(segment.substr(0, comma_pos));

            // Parse mask (after comma)
            uint8_t mask = static_cast<uint8_t>(std::stoi(segment.substr(comma_pos + 1)));

            word_info.emplace_back(word_id, mask);

        } catch (const std::exception& e) {
            std::cerr << "Error parsing segment '" << segment << "': " << e.what() << std::endl;
        }
    }

    return word_info;
}

bool ReverseIndex::build(ISAMStorage& forward_index, const Lexicon& lexicon) {
    std::cout << "Starting reverse index construction from forward index..." << std::endl;
    int count = 0;

    // Reset the index map before building
    index_.clear();

    // Reset the ISAMStorage pointer to the beginning of the file
    forward_index.reset_iterator();

    while (true) {
        // Read the next entry from the Forward Index file
        auto entry = forward_index.next();

        if (!entry.has_value()) {
            break; // End of file
        }

        CompoundKey key = CompoundKey::unpack(entry->first);
        std::string word_ids_str = entry->second;

        // The Forward Index key is (KeyType, SiteID, DocID, 0)
        // We only care about the DocID part of the key.
        uint32_t doc_id = key.primary_id;

        // The data format is (wordID1,mask1 wordID2,mask2 wordID3,mask3)
        auto word_info = parse_word_ids_with_masks(word_ids_str);

        // --- Inversion Step ---
        // NOTE: Masks are currently not being in the inverted index
        // For every word in the document, add the DocID to that word's Postings List.
        for (auto word : word_info) {
            // Check if the WordID is valid (not 0, which means "not found" in your lexicon)
            if (word.first != 0) {
                // If word_id is not in index_, std::map::operator[] creates it with an empty vector.
                index_[word.first].push_back({doc_id});
            }
        }

        count++;
        if (count % 1000 == 0) {
            std::cout << "\rProcessed " << count << " forward index entries..." << std::flush;
        }
    }

    std::cout << std::endl << "Reverse index built successfully." << std::endl;
    std::cout << "Total documents processed: " << count << std::endl;
    std::cout << "Total unique terms indexed: " << index_.size() << std::endl;
    return true;
}

const ReverseIndex::postings_list_t& ReverseIndex::search(int word_id) const {
    // Use find() for searching to avoid creating a new entry if the key doesn't exist
    auto it = index_.find(word_id);
    if (it != index_.end()) {
        return it->second; // Found the postings list
    }

    // Return the static empty list if the word ID is not in the index
    return EMPTY_POSTINGS_LIST;
}

void ReverseIndex::write_to_disk(ISAMStorage& output) {

    std::vector<std::pair<uint64_t, std::string> > data;

    for (auto p: index_) {
        uint64_t word_id = p.first;

        // Convert posting list to string
        std::stringstream ss;

        for (size_t i = 0; i < p.second.size(); ++i) {
            ss << p.second[i].doc_id;
            if (i < p.second.size() - 1) {
                ss << ",";
            }
        }

        data.emplace_back(word_id, ss.str());
    }

    output.write(data);
}