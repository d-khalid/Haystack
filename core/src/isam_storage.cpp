#include "isam_storage.hpp"

#include <algorithm>

#include "iostream"

#include <filesystem>

ISAMStorage::ISAMStorage(std::string index_file, std::string data_file) {
    // Check if exists
    // if (!std::filesystem::exists(index_file)) {
    //     throw std::invalid_argument("The file [" + index_file + "] does not exist.");
    // }
    // if (!std::filesystem::exists(data_file)) {
    //     throw std::invalid_argument("The file [" + data_file + "] does not exist.");
    // }

    index_out.open(index_file, std::ios::binary | std::ios::out | std::ios::app);
    data_out.open(data_file, std::ios::binary | std::ios::out | std::ios::app);

    // Open descriptors
    index_in.open(index_file, std::ios::binary | std::ios::in);
    data_in.open(data_file, std::ios::binary | std::ios::in);



    load_index_file();
}

uint32_t ISAMStorage::size() {
    return loaded_indexes.size();
}


void ISAMStorage::load_index_file() {

    if (index_loaded) {
        std::cout << "STORAGE: The index has already been loaded into memory. No operation performed." << std::endl;
        return;
    }



    while (index_in.peek() != EOF) {
        uint64_t raw_key;
        uint64_t offset;

        index_in.read(reinterpret_cast<char*>(&raw_key), sizeof(uint64_t));
        index_in.read(reinterpret_cast<char*>(&offset), sizeof(uint64_t));

        // Check if reads succeeded
        if (!index_in) break;

        loaded_indexes.emplace_back(raw_key, offset);
    }

    if (!std::is_sorted(loaded_indexes.begin(), loaded_indexes.end(),
        [](const std::pair<uint64_t, uint64_t>& a, const std::pair<uint64_t, uint64_t>& b) {
            return a.first < b.first;
        })) {

        std::sort(loaded_indexes.begin(), loaded_indexes.end(),
        [](const std::pair<uint64_t, uint64_t>& a, const std::pair<uint64_t, uint64_t>& b) {
            return a.first < b.first;
        });
        std::cout << "STORAGE: Index was unsorted, sorted " << loaded_indexes.size() << " entries." << std::endl;
    } else {
        std::cout << "STORAGE: Index already sorted, skipping sort" << std::endl;
    }

    index_loaded = true;
}


ISAMStorage::~ISAMStorage() {
    index_in.close();
    index_out.close();

    data_in.close();
    data_out.close();
}

void ISAMStorage::write(std::vector<std::pair<uint64_t, std::string > > entries) {

    std::vector<std::pair<uint64_t, uint64_t> > new_indexes;

    // Write all data, store indexes
    for (auto p : entries) {
        uint32_t len = p.second.size();

        // Store index
        new_indexes.emplace_back(p.first, data_out.tellp());

        // Write the 4-byte length field
        data_out.write(reinterpret_cast<const char*>(&len), sizeof(uint32_t));

        // Write data
        data_out.write(p.second.c_str(), len);
    }
    data_out.flush();

    // Sort new indexes
    std::sort(new_indexes.begin(), new_indexes.end(),
    [](const std::pair<uint64_t, uint64_t>& a, const std::pair<uint64_t, uint64_t>& b) {
        return a.first < b.first; // This is an overloaded operator that compares 64-bit keys
    });

    // Merge with old indexes using std::merge
    std::vector<std::pair<uint64_t, uint64_t>> merged_indexes;
    merged_indexes.reserve(loaded_indexes.size() + new_indexes.size());

    std::merge(loaded_indexes.begin(), loaded_indexes.end(),
               new_indexes.begin(), new_indexes.end(),
               std::back_inserter(merged_indexes),
               [](const std::pair<uint64_t, uint64_t>& a, const std::pair<uint64_t, uint64_t>& b) {
                   return a.first < b.first;
               });

    // Assign merged result back to loaded_indexes
    loaded_indexes = std::move(merged_indexes);


    // Write new indexes to file
    index_out.seekp(0);
    for (const auto& entry : loaded_indexes) {
        uint64_t key = entry.first;
        uint64_t offset = entry.second;
        index_out.write(reinterpret_cast<const char*>(&key), sizeof(uint64_t));
        index_out.write(reinterpret_cast<const char*>(&offset), sizeof(uint64_t));
    }
    index_out.flush();

}

std::optional<std::pair<uint64_t, std::string > > ISAMStorage::next() {
    if (index_ptr >= loaded_indexes.size()) {
        return std::nullopt;
    }

    std::pair<uint64_t, uint64_t > current_index = loaded_indexes[index_ptr++];

    // Get the data, using offset (TODO: change data type of offset to long long)
    data_in.seekg((long long)current_index.second);

    // Read length
    uint32_t len;
    data_in.read(reinterpret_cast<char*>(&len), sizeof(uint32_t));

    std::string data(len, '\0');
    data_in.read(reinterpret_cast<char*>(&data[0]), len);

    return std::make_pair(current_index.first, data);

}


std::optional<std::pair<uint64_t, std::string > > ISAMStorage::read(uint64_t key) {

    // Find the index, std::lower_bound uses binary search
    auto it = std::lower_bound(loaded_indexes.begin(), loaded_indexes.end(), key,
           [](const std::pair<uint64_t, uint64_t>& element, const uint64_t& target) {
               return element.first < target;
           });

    // If there is a match, read the data and return
    if (it != loaded_indexes.end() && it->first == key) {
        uint64_t offset = it->second;
        data_in.seekg(offset);

        uint32_t len;
        data_in.read(reinterpret_cast<char*>(&len), sizeof(uint32_t));

        std::string data(len, '\0');
        data_in.read(&data[0], len);

        return std::make_pair(key, data);
    }

    return std::nullopt;
}

void ISAMStorage::reset_iterator() {
    index_ptr = 0;
}





