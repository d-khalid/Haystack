#include<iostream>
// #include <pstl/parallel_backend_utils.h> // Can be removed if not used
#include "CLI11.hpp"
#include "compound_key.hpp"
#include "forward_index.hpp"
#include "isam_storage.hpp"
#include "pugixml.hpp"
#include "reverse_index.hpp"
#include "utils.hpp"



int main(int argc, char** argv) {
    CLI::App app{"The Haystack CLI"};
    argv = app.ensure_utf8(argv);

    // Input directory (required)
    std::string input_dir;
    app.add_option("-i,--input", input_dir, "Input directory containing Stack Exchange data")
        ->required()
        ->check(CLI::ExistingDirectory);


    // Generation flags
    bool gen_data_index = false;
    app.add_flag("--data-index-gen", gen_data_index, "Generate data index");

    bool show_data_index = false;
    app.add_flag("--data-index-show", show_data_index, "Show data index");

    bool gen_lexicon = false;
    app.add_flag("--lexicon-gen", gen_lexicon, "Generate lexicon");

    bool gen_forward_index = false;
    app.add_flag("--forward-index-gen", gen_forward_index, "Generate forward index");

    bool show_f_index = false;
    app.add_flag("--forward-index-show", show_f_index, "Show forward index");

    bool gen_reverse_index = false;
    app.add_flag("--reverse-index-gen", gen_reverse_index, "Generate reverse index (Barrels)");

    // New flag for number of barrels
    int num_barrels = 1;
    app.add_option("-b,--barrels", num_barrels, "Number of barrels to split the index into")->default_val(1);

    // Search Test
    int search_word_id = 0;
    app.add_option("--search-id", search_word_id, "Search for a WordID using Barrels");

    CLI11_PARSE(app, argc, argv);


    if (gen_data_index) {
        std::string path = input_dir + "/Posts.xml";
        std::cout << "Generating data index from: " << path << "\n";
        ISAMStorage data_index(input_dir + "/data_index.idx", input_dir + "/data_index.dat");
        Utils::generate_data_index(data_index, path);
    }

    if (show_data_index) {
        ISAMStorage data_index(input_dir + "/data_index.idx", input_dir + "/data_index.dat");
        while (true) {
            auto entry = data_index.next();
            if (!entry.has_value()) break;
            CompoundKey k = CompoundKey::unpack(entry->first);
            std::cout << "KEY: " << k.to_string() << std::endl;
            std::cout << "DATA: " << entry->second << std::endl << std::endl;
        }
    }

    if (gen_lexicon) {
        std::cout << "Generating lexicon from data index" << "\n";
        ISAMStorage data_index(input_dir + "/data_index.idx", input_dir + "/data_index.dat");
        Lexicon l = Utils::generate_lexicon(data_index);
        l.save(input_dir + "/lexicon.txt");
    }

    if (gen_forward_index) {
        std::cout << "Generating forward index" << "\n";
        ISAMStorage forward_index(input_dir + "/forward_index.idx", input_dir + "/forward_index.dat");
        ISAMStorage data_index(input_dir + "/data_index.idx", input_dir + "/data_index.dat");
        Lexicon l;
        l.load(input_dir + "/lexicon.txt");
        ForwardIndex::generate(forward_index, data_index, l);
    }

    if (show_f_index) {
        ISAMStorage forward_index(input_dir + "/forward_index.idx", input_dir + "/forward_index.dat");
        while (true) {
            auto entry = forward_index.next();
            if (!entry.has_value()) break;
            CompoundKey k = CompoundKey::unpack(entry->first);
            std::cout << "KEY: " << k.to_string() << std::endl;
            std::cout << "DATA: " << entry->second << std::endl << std::endl;
        }
    }

    if (gen_reverse_index) {
        std::cout << "Generating reverse index from: " << input_dir << " into " << num_barrels << " barrels.\n";

        ISAMStorage forward_index(input_dir + "/forward_index.idx", input_dir + "/forward_index.dat");
        Lexicon l;
        l.load(input_dir + "/lexicon.txt"); // Not strictly needed for build logic but good for checks

        // Create ReverseIndex with N barrels
        ReverseIndex r(num_barrels);

        // Build in memory (split into shards)
        r.build(forward_index, l);

        // Write shards to disk
        r.save_barrels(input_dir);
    }

    if (search_word_id > 0) {
        std::cout << "Searching for WordID: " << search_word_id << std::endl;

        // Calculate which barrel contains this word
        int target_barrel = search_word_id % num_barrels;
        std::cout << "Looking in Barrel: " << target_barrel << std::endl;

        auto postings = ReverseIndex::search_barrel(input_dir, target_barrel, search_word_id);

        std::cout << "Found " << postings.size() << " documents:" << std::endl;
        for(auto& p : postings) {
            std::cout << p.doc_id << " ";
        }
        std::cout << std::endl;
    }

    return 0;
}