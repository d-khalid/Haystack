#ifndef HAYSTACK_REVERSE_INDEX_HPP
#define HAYSTACK_REVERSE_INDEX_HPP

#include <vector>
#include <map>
#include <string>
#include <cstdint>

#include "compound_key.hpp"
#include "data_index.hpp"
#include "lexicon.hpp"

// Forward declaration to avoid circular includes if ISAMStorage.hpp doesn't include it
class ISAMStorage;
class Lexicon;

/**
 * it Represents a single document entry in the Postings List.
 * Currently only contains the Document ID (DocID), which is derived from the CompoundKey's primary_id.
 */
struct Posting {
    uint32_t doc_id;
    // float score; // Can be added later for ranking/relevance
};

class ReverseIndex {
public:
    // Type aliases for clarity
    using postings_list_t = std::vector<Posting>;
    // The core index maps a Word ID (int) to a list of Posting structures
    using index_map_t = std::map<int, postings_list_t>;

    ReverseIndex() = default;
    ~ReverseIndex() = default;

    /**
     *  The function Loads and builds the reverse index by reading the forward index data.
     *  parameter ISAMStorage instance used to read the forward index file.
     *  parameterLexicon instance used to manage word-to-ID mappings (if needed for debugging/query).
     *  returns true if building was successful, false otherwise.
     */
    bool build(ISAMStorage& storage, const Lexicon& lexicon);

    /**
     *  The function Searches the index for a specific word ID.
     *  word_id is The ID of the word to search for.
     *  returns The postings list for that word. Returns an empty list if not found.
     */
    const postings_list_t& search(int word_id) const;

    /**
     *  Returns the total number of unique terms indexed.
     */
    size_t size() const { return index_.size(); }

private:
    index_map_t index_;

    // Static empty list returned when a word is not found in the search method
    static const postings_list_t EMPTY_POSTINGS_LIST;
};

#endif // HAYSTACK_REVERSE_INDEX_HPP