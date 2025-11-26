#include<iostream>
#include <pstl/parallel_backend_utils.h>
#include "CLI11.hpp"
#include "compound_key.hpp"
#include "isam_storage.hpp"
#include "pugixml.hpp"
#include "utils.hpp"


int main(int argc, char** argv) {

    CLI::App app{"The Haystack CLI"};
    argv = app.ensure_utf8(argv);


    bool generate_data_index = false;
    bool generate_lexicon = false;

    app.add_flag("--data-gen",
                 generate_data_index,
                 "Generate the ISAM-style data index");
    app.add_flag("--lexicon-gen",
                 generate_lexicon,
                 "Generate the lexicon using the data index");

    CLI11_PARSE(app, argc, argv);


    std::string index_file;
    std::string data_file;
    std::string posts_file, comments_file, lexicon_save_file;
    std::string data_folder;
    if (generate_data_index) {

        std::cout << "Enter path to index file: ";
        std::cin >> index_file;

        std::cout << "Enter path to data file: ";
        std::cin >> data_file;

        std::cout << "Enter path to Posts.xml: ";
        std::cin >> posts_file;

        std::cout << "Enter path to Comments.xml: ";
        std::cin >> comments_file;

        ISAMStorage store(index_file, data_file);

        Utils::generate_data_index(store, posts_file, comments_file);

    }
    else if (generate_lexicon) {
        std::cout << "Enter path to index file: ";
        std::cin >> index_file;

        std::cout << "Enter path to data file: ";
        std::cin >> data_file;

        std::cout << "Enter path to empty lexicon file: ";
        std::cin >> lexicon_save_file;

        ISAMStorage data_index(index_file, data_file);

        Lexicon l = Utils::generate_lexicon(data_index);

        l.save(lexicon_save_file);
    }
    else{
        std::cout << "Enter path to index file: ";
        std::cin >> index_file;

        std::cout << "Enter path to data file: ";
        std::cin >> data_file;

        ISAMStorage data_index(index_file, data_file);

        std::cout << "Entries in Data Index: " << data_index.size() << std::endl;


    }
    return 0;
}
