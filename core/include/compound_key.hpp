#ifndef COMPOSITE_KEY_H
#define COMPOSITE_KEY_H
#include <cstdint>
#include <string>

enum class KeyType : uint8_t {
    POST_BY_ID = 1, // 1|site_id|post_id -> Post data
    COMMENT_BY_ID = 2, // 2|site_id|comment_id -> Comment data
    COMMENTS_BY_POST = 3, // 3|site_id|post_id -> List of comment_ids
    POSTS_BY_TAG = 4, // 4|site_id|tag_id -> List of post_ids
    TAG_INFO = 5, // 5|site_id|tag_id -> Tag metadata
};

enum class SiteID :uint16_t {
    ASK_UBUNTU = 1,
    // More can be added
};

class CompoundKey {
public:
    // These 4 attributes are combined to index posts
    uint8_t key_type;
    uint16_t site_id;
    uint32_t primary_id;
    uint8_t reserved; // Not in use


    CompoundKey(uint8_t key_type, uint16_t site_id, uint32_t primary_id, uint8_t reserved);

    // Converts the attributes into a 64-bit number
    uint64_t pack() const;

    // Print a human-readable representation of the key
    std::string to_string();

    // Converts 64 bit number back to attribute
    static CompoundKey unpack(uint64_t);



    // Comparison operators, will be useful
    bool operator<(const CompoundKey& other) const {
        return pack() < other.pack();
    }

    bool operator==(const CompoundKey& other) const {
        return pack() == other.pack();
    }

    bool operator!=(const CompoundKey& other) const {
        return !(*this == other);
    }
};


#endif //COMPOSITE_KEY_H
