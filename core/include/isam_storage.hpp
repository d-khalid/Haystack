#ifndef ISAM_STORAGE_H
#define ISAM_STORAGE_H
#include <cstdint>
#include <optional>
#include <string>
#include <vector>
#include "fstream"

#include "compound_key.hpp"


// ISAM Data Storage, maps 64 bit keys + offsets to string data
// Provides a (relatively) easy abstraction to store/access data in an ISAM style
class ISAMStorage {
public:
    // Requires valid path to index and data files
    ISAMStorage(std::string index_file, std::string data_file);

    ~ISAMStorage(); // File descriptors released here

    // Write multiple entries into the files
    void write(std::vector<std::pair<uint64_t, std::string > > entries);

    uint32_t size();

    // Bring iterator for next() back to beginning of index file
    void reset_iterator();

    // Get next entry from the files
    std::optional<std::pair<uint64_t, std::string > > next();

    // Get the data for a specific index, may find nothing
    std::optional<std::pair<uint64_t, std::string > > read(uint64_t key);



private:
    std::string index_file;
    std::string data_file;

    // File descriptors
    std::ofstream index_out;
    std::ofstream data_out;

    std::ifstream index_in;
    std::ifstream data_in;

    // Average C++ syntax moment
    std::vector<std::pair<uint64_t, std::string > > to_write;

    // The index will usually be a few megabytes and thus it is viable to always keep it in memory
    int index_ptr = 0;
    bool index_loaded = false;
    std::vector<std::pair<uint64_t, uint64_t> > loaded_indexes;

    void load_index_file();
};


#endif //ISAM_STORAGE_H
