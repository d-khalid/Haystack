#ifndef REVERSE_INDEX_HPP
#define REVERSE_INDEX_HPP
#include <map>

#include "isam_storage.hpp"
#include "lexicon.hpp"

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
    bool build(ISAMStorage& forward_index, const Lexicon& lexicon);

    /**
     *  The function Searches the index for a specific word ID.
     *  word_id is The ID of the word to search for.
     *  returns The postings list for that word. Returns an empty list if not found.
     */
    const postings_list_t& search(int word_id) const;

    void write_to_disk(ISAMStorage& output);

    /**
     *  Returns the total number of unique terms indexed.
     */
    size_t size() const { return index_.size(); }

private:
    index_map_t index_;

    // Static empty list returned when a word is not found in the search method
    static const postings_list_t EMPTY_POSTINGS_LIST;
};

#endif //REVERSE_INDEX_HPP
