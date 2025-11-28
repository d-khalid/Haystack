#include "reverse_index.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>

// Define the static empty list
const ReverseIndex::postings_list_t ReverseIndex::EMPTY_POSTINGS_LIST = {};

// Helper function to split the comma-separated string of WordIDs
static std::vector<int> split_word_ids(const std::string& data) {
    std::vector<int> ids;
    std::stringstream ss(data);
    std::string segment;

    while (std::getline(ss, segment, ',')) {
        try {
            ids.push_back(std::stoi(segment));
        } catch (const std::exception& e) {
            std::cerr << "Error parsing WordID: " << segment << " - " << e.what() << std::endl;
        }
    }
    return ids;
}

bool ReverseIndex::build(ISAMStorage& storage, const Lexicon& lexicon) {
    std::cout << "Starting reverse index construction from forward index..." << std::endl;
    int count = 0;

    // Reset the index map before building
    index_.clear();

    // Reset the ISAMStorage pointer to the beginning of the file
    storage.reset_iterator();

    while (true) {
        // Read the next entry from the Forward Index file
        auto entry = storage.next();

        if (!entry.has_value()) {
            break; // End of file
        }

        CompoundKey key = entry->first;
        std::string word_ids_str = entry->second;

        // The Forward Index key is (KeyType, SiteID, DocID, 0)
        // We only care about the DocID part of the key.
        uint32_t doc_id = key.primary_id;

        // The data is a comma-separated list of WordIDs (e.g., "10,25,300")
        std::vector<int> word_ids = split_word_ids(word_ids_str);

        // --- Inversion Step ---
        // For every word in the document, add the DocID to that word's Postings List.
        for (int word_id : word_ids) {
            // Check if the WordID is valid (not 0, which means "not found" in your lexicon)
            if (word_id != 0) {
                // If word_id is not in index_, std::map::operator[] creates it with an empty vector.
                index_[word_id].push_back({doc_id});
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