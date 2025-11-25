#include<iostream>
#include <pstl/parallel_backend_utils.h>

#include "CLI11.hpp"
#include "compound_key.hpp"
#include "data_index.hpp"
#include "pugixml.hpp"
#include "utils.hpp"


int main(int argc, char** argv) {

    // Testing!
    CLI::App app{"App description"};
    argv = app.ensure_utf8(argv);


    bool generate_data_index = false;
    app.add_flag("--generate-data-index",
                 generate_data_index,
                 "Generate the ISAM-style data index");
    bool generate_lexicon = false;
    app.add_flag("--generate-lexicon",
                 generate_lexicon,
                 "Generate the lexicon using the data index");

    CLI11_PARSE(app, argc, argv);


    std::string index_file;
    std::string data_file;
    std::string posts_file, comments_file, lexicon_save_file;
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
        std::cout << Utils::extract_text_from_html("<p>i am already aware that bad do suppress this message?</p>  power-management notification <p>maybe <a href=\"http://linux.aldeby.org/get-rid-of-your-battery-may-be-broken-notification.htmlz\">these</a> instructions will help you rid of message.</p>");
        // std::cout << "Enter path to index file: ";
        // std::cin >> index_file;
        //
        // std::cout << "Enter path to data file: ";
        // std::cin >> data_file;
        //
        // ISAMStorage store(index_file, data_file);
        //
        //
        // auto p = store.read(CompoundKey::unpack(0x100010000000500));
        // if (!p.has_value()) {
        //     std::cout << "Value not found" << std::endl;
        //     return -1;
        // }
        //
        // std::cout << (*p).first.to_string() << std::endl;;
        //
        // std::cout << "DATA: " << (*p).second;

    }
    return 0;
}
