#include <iostream>
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
    app.add_option("-i,--input", input_dir,
                   "Input directory containing Stack Exchange data")
        ->required()
        ->check(CLI::ExistingDirectory);

    // Generation flags
    bool gen_data_index = false;
    app.add_flag("--data-index-gen", gen_data_index,
                 "Generate data index");

    bool show_data_index = false;
    app.add_flag("--data-index-show", show_data_index,
                 "Show data index");

    bool gen_lexicon = false;
    app.add_flag("--lexicon-gen", gen_lexicon,
                 "Generate lexicon");

    bool gen_forward_index = false;
    app.add_flag("--forward-index-gen", gen_forward_index,
                 "Generate forward index");

    bool show_f_index = false;
    app.add_flag("--forward-index-show", show_f_index,
                 "Show forward index");

    bool gen_reverse_index = false;
    app.add_flag("--reverse-index-gen", gen_reverse_index,
                 "Generate reverse index (Barrels)");

    // Number of barrels
    int num_barrels = 1;
    app.add_option("-b,--barrels", num_barrels,
                   "Number of barrels")
        ->default_val(1);

    // Search by WordID
    int search_word_id = 0;
    app.add_option("--search-id", search_word_id,
                   "Search for a WordID using Barrels");

    // AUTOCOMPLETE OPTIONS 
    bool run_autocomplete = false;
    std::string autocomplete_prefix;

    app.add_flag("--autocomplete", run_autocomplete,
                 "Run autocomplete using reverse index");

    app.add_option("--prefix", autocomplete_prefix,
                   "Prefix string for autocomplete");

    CLI11_PARSE(app, argc, argv);

    //  DATA INDEX 
    if (gen_data_index) {
        std::string path = input_dir + "/Posts.xml";
        std::cout << "Generating data index from: " << path << "\n";
        ISAMStorage data_index(input_dir + "/data_index.idx",
                               input_dir + "/data_index.dat");
        Utils::generate_data_index(data_index, path);
    }

    if (show_data_index) {
        ISAMStorage data_index(input_dir + "/data_index.idx",
                               input_dir + "/data_index.dat");
        while (true) {
            auto entry = data_index.next();
            if (!entry.has_value()) break;
            CompoundKey k = CompoundKey::unpack(entry->first);
            std::cout << "KEY: " << k.to_string() << "\n";
            std::cout << "DATA: " << entry->second << "\n\n";
        }
    }

    // LEXICON 
    if (gen_lexicon) {
        std::cout << "Generating lexicon\n";
        ISAMStorage data_index(input_dir + "/data_index.idx",
                               input_dir + "/data_index.dat");
        Lexicon l = Utils::generate_lexicon(data_index);
        l.save(input_dir + "/lexicon.txt");
    }
    //forward index
    if (gen_forward_index) {
        std::cout << "Generating forward index\n";
        ISAMStorage forward_index(input_dir + "/forward_index.idx",
                                  input_dir + "/forward_index.dat");
        ISAMStorage data_index(input_dir + "/data_index.idx",
                               input_dir + "/data_index.dat");
        Lexicon l;
        l.load(input_dir + "/lexicon.txt");
        ForwardIndex::generate(forward_index, data_index, l);
    }

    if (show_f_index) {
        ISAMStorage forward_index(input_dir + "/forward_index.idx",
                                  input_dir + "/forward_index.dat");
        while (true) {
            auto entry = forward_index.next();
            if (!entry.has_value()) break;
            CompoundKey k = CompoundKey::unpack(entry->first);
            std::cout << "KEY: " << k.to_string() << "\n";
            std::cout << "DATA: " << entry->second << "\n\n";
        }
    }

    //  REVERSE INDEX
    if (gen_reverse_index) {
        std::cout << "Generating reverse index into "
                  << num_barrels << " barrels\n";

        ISAMStorage forward_index(input_dir + "/forward_index.idx",
                                  input_dir + "/forward_index.dat");

        Lexicon l;
        l.load(input_dir + "/lexicon.txt");

        ReverseIndex r(num_barrels);
        r.build(forward_index, l);
        r.save_barrels(input_dir);
    }

if (run_autocomplete) {
    if (autocomplete_prefix.empty()) {
        std::cerr << "Error: --prefix is required for --autocomplete\n";
        return 1;
    }

    std::cout << "Autocomplete for prefix: "
              << autocomplete_prefix << "\n";

    // Forward index needed to rebuild in-memory structures
    ISAMStorage forward_index(input_dir + "/forward_index.idx",
                              input_dir + "/forward_index.dat");

    Lexicon l;
    l.load(input_dir + "/lexicon.txt");

    ReverseIndex r(num_barrels);
    r.build(forward_index, l);   // builds trie + word set

    auto results = r.autocomplete(autocomplete_prefix, 10);  

    std::cout << "Found " << results.size()
              << " suggestions:\n";
    for (const auto& w : results) {
        std::cout << w << "\n";
    }

    return 0;
}

    // SEARCH 
    if (search_word_id > 0) {
        std::cout << "Searching for WordID: "
                  << search_word_id << "\n";

        int target_barrel = search_word_id % num_barrels;
        std::cout << "Looking in Barrel: "
                  << target_barrel << "\n";

        auto postings = ReverseIndex::search_barrel(
            input_dir, target_barrel, search_word_id);

        std::cout << "Found " << postings.size()
                  << " documents:\n";
        for (auto& p : postings) {
            std::cout << p.doc_id << " ";
        }
        std::cout << "\n";
    }

    return 0;
}
