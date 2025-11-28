#ifndef FORWARD_INDEX_HPP
#define FORWARD_INDEX_HPP

#include <vector>
#include <string>
#include "compound_key.hpp"
#include "data_index.hpp"
#include "lexicon.hpp"

// Forward declaration to avoid circular dependencies if needed
class ISAMStorage;
class Lexicon;

/**
 * @brief The ForwardIndex class manages the mapping of Document ID (CompoundKey)
 * to the list of Word IDs contained within that document.
 */
class ForwardIndex {
public:
    /**
     * @brief The data structure representing a processed document entry.
     * This is the core output of the Forward Index construction process.
     */
    struct Entry {
        CompoundKey key;         // Key used to identify the document (DocID, SiteID, etc.)
        std::vector<int> word_ids; // List of normalized, indexed word IDs from the document
    };

    /**
     * @brief Reads data from ISAMStorage (which contains Post/Comment data) and converts
     * it into a list of ForwardIndex entries using the Lexicon.
     * * @param storage The ISAM storage instance reading raw Post/Comment JSON.
     * @param lexicon The Lexicon instance used to get WordIDs from tokens.
     * @return std::vector<Entry> A list of all processed document entries.
     */
    static std::vector<Entry> build(ISAMStorage& storage, Lexicon& lexicon);

    /**
     * @brief Writes the list of ForwardIndex entries to disk, storing the CompoundKey
     * and the comma-separated string of Word IDs into the ISAM format.
     * * @param entries The list of all constructed ForwardIndex entries.
     * @param index_file Path to the ISAM index file.
     * @param data_file Path to the ISAM data file.
     */
    static void write_to_disk(
        const std::vector<Entry>& entries,
        const std::string& index_file,
        const std::string& data_file
    );
};

#endif // FORWARD_INDEX_HPP