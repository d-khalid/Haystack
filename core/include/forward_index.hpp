#ifndef FORWARD_INDEX_HPP
#define FORWARD_INDEX_HPP
#include <vector>

#include "isam_storage.hpp"
#include "lexicon.hpp"

// Masks
enum class HitMask: int {
    NONE        = 0,
    TITLE       = 1 << 0,   // 1
    BODY        = 1 << 1,   // 2
    TAG        = 1 << 2,   // 4
    ANSWER     = 1 << 3,   // 8
};


class ForwardIndex {
    public:
    static void generate(ISAMStorage& output_store, ISAMStorage& data_index, Lexicon& lexicon);

};

#endif //FORWARD_INDEX_HPP
