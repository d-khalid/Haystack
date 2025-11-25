#include "compound_key.hpp"

#include <ios>
#include <sstream>

CompoundKey::CompoundKey(uint8_t key_type, uint16_t site_id, uint32_t primary_id, uint8_t reserved) {
    this->key_type = key_type;
    this->site_id = site_id;
    this->primary_id = primary_id;
    this->reserved = reserved;
}

uint64_t CompoundKey::pack() const{
    return (static_cast<uint64_t>(key_type) << 56) |
              (static_cast<uint64_t>(site_id) << 40) |
              (static_cast<uint64_t>(primary_id) << 8) |
              static_cast<uint64_t>(reserved);
}

std::string CompoundKey::to_string() {
    std::ostringstream oss;
    
    // Key type
    oss << "KeyType:";
    switch (static_cast<KeyType>(key_type)) {
        case KeyType::POST_BY_ID:       oss << "POST_BY_ID"; break;
        case KeyType::COMMENT_BY_ID:    oss << "COMMENT_BY_ID"; break;
        case KeyType::COMMENTS_BY_POST: oss << "COMMENTS_BY_POST"; break;
        case KeyType::POSTS_BY_TAG:     oss << "POSTS_BY_TAG"; break;
        case KeyType::TAG_INFO:         oss << "TAG_INFO"; break;
        default:                        oss << "UNKNOWN(" << static_cast<int>(key_type) << ")"; break;
    }
    
    // Site
    oss << " Site:";
    switch (static_cast<SiteID>(site_id)) {
        case SiteID::ASK_UBUNTU:  oss << "ASK_UBUNTU"; break;
        default:
            oss << "UNKNOWN(" << site_id << ")"; break;
    }
    
    oss << " ID:" << primary_id;
    oss << " (packed:0x" << std::hex << pack() << std::dec << ")";
    
    return oss.str();
}

CompoundKey CompoundKey::unpack(uint64_t packed) {
    return CompoundKey(
        static_cast<uint8_t>(packed >> 56),   // key_type
        static_cast<uint16_t>(packed >> 40),  // site_id
        static_cast<uint32_t>(packed >> 8),   // primary_id
        static_cast<uint8_t>(packed & 0xFF)   // reserved
    );
}


