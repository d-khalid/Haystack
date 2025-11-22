#include <fstream>
#include <iostream>
#include <bits/ostream.tcc>

#include "lexicon.hpp"
#include "pugixml.hpp"
#include "nlohmann/json.hpp"
#include "gtest/gtest.h"
#include "CLI/CLI.hpp"

#include "html_parser.hpp"

const std::string G_HELP_MESSAGE = "Usage: haystack-cli [options]\n"
                                     "Options:\n"
                                     "\t--generate-lexicon <dir-path> <output-file>\t Generate a Lexicon file by specifying a dump folder with Posts.xml, Comments.xml and Tags.xml\n"
                                     "\t--help\t Shows this message\n";

// class CliConfig {
//     public:
//     bool generate_lexicon = false;
//     std::string output_filename;
//     std::string lexicon_target_dir;
//
//     static std::optional<CliConfig> generate_config(int argc, char** argv) {
//         std::vector<std::string> args;
//
//         // Put cli args in vector for easier parsing
//         for (int i = 0; i < argc; ++i) {
//             args.push_back(argv[i]);
//         }
//
//         if (args.size() < 2)
//
//         switch (args[1]) {
//             case "--generate-lexicon":
//                 break;
//             case "--help":
//                 std::cout << G_HELP_MESSAGE;
//                 break;
//         }
//
//     }
//
// };

// Currently just hardcoded
// TODO: Change this to an action that you can do with CLI args
bool fill_lexicon(Lexicon l) {
    std::string posts_path = "/home/alkalinelemon/Documents/Projects/C/haystack/stackexchange-data/askubuntu.com/Posts.xml";
    std::string comments_path = "/home/alkalinelemon/Documents/Projects/C/haystack/stackexchange-data/askubuntu.com/Comments.xml";
    std::string tags_path = "/home/alkalinelemon/Documents/Projects/C/haystack/stackexchange-data/askubuntu.com/Tags.xml";

    // Put posts with ID 1 or 2 in lexicon (Q and As)
    pugi::xml_document post_doc;
    pugi::xml_parse_result result = post_doc.load_file(posts_path.c_str());

    if (!result) {
        std::cout << result.description() << std::endl;
        return false;
    }

    for (auto post : post_doc.child("posts")) {
        int id = post.attribute("PostTypeId").as_int();

        if (id != 1 && id != 2) {
            continue;
        }
        std::string body = post.attribute("Body").as_string();
        std::string title = post.attribute("Title").as_string();

        std::cout << body << std::endl;
    }


}



int main(int argc, char** argv) {

    CLI::App app{"App description"};

    std::string filename = "default";
    app.add_option("-f,--file", filename, "A help string");

    CLI11_PARSE(app, argc, argv);

    // BASIC TEST
    std::string data("<html><body><div><span id=\"a\" class=\"x\">a</span><span id=\"b\">b</span></div></body></html>");
    HtmlParser parser;
    shared_ptr<HtmlDocument> doc = parser.Parse(data.c_str(), data.size());

    std::vector<shared_ptr<HtmlElement>> x = doc->GetElementByClassName("x");

    if(!x.empty()){
        std::cout << x[0]->GetName() << std::endl;
        std::cout << x[0]->GetAttribute("id") << std::endl;
    }


    return 0;
    // Lexicon lexicon;
    //
    // // Sample text
    // std::vector<std::string> words = lexicon.tokenize_text("The above modes are default modes for these streams. These modes cannot be changed but can be clubbed together with other modes. Now for input, we can also use ifstream as shown");
    //
    // // Add words
    // for (auto word : words) {
    //     lexicon.add_word(word);
    // }
    //
    // if (!lexicon.save("/home/alkalinelemon/Projects/lexicon.txt")) {
    //     std::cout << "Couldn't save lexicon as file" << std::endl;
    //     return -1;
    // }
    //
    // std::cout << "Saved to: " << "/home/alkalinelemon/Projects/lexicon.txt" << std::endl;
    //
    // return 0;
}