#ifndef DATA_INDEX_H
#define DATA_INDEX_H
#include <cstdint>
#include <optional>
#include <string>
#include <vector>
#include "fstream"

#include "compound_key.hpp"

class Tag {
public:
    SiteID site_id;
    uint32_t tag_id;

    // Content
    std::string tag_name; // Tag name (e.g., "battery", "notification")

    // Relationships
    std::optional<uint32_t> excerpt_post_id; // Post ID containing tag excerpt/description
    std::optional<uint32_t> wiki_post_id; // Post ID containing tag wiki

    // Metrics
    uint32_t count; // Number of posts using this tag



    // Default constructor
    Tag() : site_id(SiteID::ASK_UBUNTU), tag_id(0), count(0) {
    }
};



// ISAM Data Storage, uses CompoundKey
// Provides a (relatively) easy abstraction to store/access data in an ISAM style
class ISAMStorage {
public:
    // Requires valid path to index and data files
    ISAMStorage(std::string index_file, std::string data_file);

    ~ISAMStorage(); // File descriptors released here

    // Write multiple entries into the files
    void write(std::vector<std::pair<CompoundKey, std::string > > entries);

    // // Write single entry into the files
    // void write(std::pair<CompoundKey, std::string > entry);

    /**
     * @brief Resets the internal file pointer to the beginning for sequential reading.
     */
    void reset_iterator();
    // Get next entry from the files
    std::optional<std::pair<CompoundKey, std::string > > next();

    // Get the data for a specific index, may find nothing
    std::optional<std::pair<CompoundKey, std::string > > read(CompoundKey key);



private:
    std::string index_file;
    std::string data_file;

    // File descriptors
    std::ofstream index_out;
    std::ofstream data_out;

    std::ifstream index_in;
    std::ifstream data_in;

    // Average C++ syntax moment
    std::vector<std::pair<CompoundKey, std::string > > to_write;

    // The index will usually be a few megabytes and thus it is viable to always keep it in memory
    int index_ptr = 0;
    bool index_loaded = false;
    std::vector<std::pair<CompoundKey, uint64_t> > loaded_indexes;

    void load_index_file();
};


#endif //DATA_INDEX_H
