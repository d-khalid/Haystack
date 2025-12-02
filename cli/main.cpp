#include<iostream>
#include <pstl/parallel_backend_utils.h>
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
    app.add_flag("--reverse-index-gen", gen_reverse_index, "Generate reverse index");

    bool show_r_index = false;
    app.add_flag("--reverse-index-show", show_r_index, "Show reverse index");

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
        std::cout << "Generating reverse index from: " << input_dir << "\n";

        ISAMStorage reverse_index(input_dir + "/reverse_index.idx", input_dir + "/reverse_index.dat");
        ISAMStorage forward_index(input_dir + "/forward_index.idx", input_dir + "/forward_index.dat");
        Lexicon l;
        l.load(input_dir + "/lexicon.txt");

        ReverseIndex r;
        r.build(forward_index,  l);
        r.write_to_disk(reverse_index);
    }

    if (show_r_index) {
        ISAMStorage reverse_index(input_dir + "/reverse_index.idx", input_dir + "/reverse_index.dat");
        Lexicon l;
        l.load(input_dir + "/lexicon.txt");

        while (true) {
            auto entry = reverse_index.next();
            if (!entry.has_value()) break;


            std::cout << "WORD: " << "(" << entry->first << "," << l.get_word(entry->first) << ")" << std::endl;

            std::cout << "POST-IDs: " << entry->second << std::endl << std::endl;
        }
    }

    return 0;
}