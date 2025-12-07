#ifndef REVERSE_INDEX_HPP
#define REVERSE_INDEX_HPP
#include <map>
#include <vector>
#include <string>

#include "isam_storage.hpp"
#include "lexicon.hpp"

struct Posting {
    uint32_t doc_id;
};

class ReverseIndex {
public:
    // Type aliases for clarity
    using postings_list_t = std::vector<Posting>;
    // The core index maps a Word ID (int) to a list of Posting structures
    using index_map_t = std::map<int, postings_list_t>;

    // Constructor now allows specifying number of barrels (default 1)
    explicit ReverseIndex(int num_barrels = 1);
    ~ReverseIndex() = default;

    /**
     * The function Loads and builds the reverse index by reading the forward index data.
     * Splits data into 'num_barrels' buckets based on WordID.
     */
    bool build(ISAMStorage& forward_index, const Lexicon& lexicon);

    /**
     * Writes the generated barrels to the specified directory.
     * Files will be named barrel_0.idx, barrel_0.dat, barrel_1.idx, etc.
     */
    void save_barrels(const std::string& directory);

    /**
     * Static helper to search a specific barrel file for a word.
     * This is efficient because it only loads the relevant barrel.
     */
    static postings_list_t search_barrel(const std::string& directory, int barrel_id, int word_id);

    size_t total_terms() const;

private:
    int num_barrels_;

    // Vector of maps. index_shards_[0] is Barrel 0, index_shards_[1] is Barrel 1, etc.
    std::vector<index_map_t> index_shards_;

    static const postings_list_t EMPTY_POSTINGS_LIST;
};

#endif //REVERSE_INDEX_HPP