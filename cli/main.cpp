#include <fstream>
#include <iostream>
#include <bits/ostream.tcc>

#include "lexicon.hpp"
#include "nlohmann/json.hpp"
#include "gtest/gtest.h"

using json = nlohmann::json;

// gtest works!
TEST(FactorialTest, HandlesZeroInput) {
    EXPECT_EQ(1, 2);
}

// Test program
int main() {
    // if (1) {
    //     RUN_ALL_TESTS();
    //     return 0;
    // }

    // nlohmann.json also works!
    // std::ifstream f("/home/alkalinelemon/Downloads/sindhi.json");
    // json data = json::parse(f);
    //
    // std::cout << data.dump() << std::endl;


    return 0;

    Lexicon lexicon;

    // Sample text
    std::vector<std::string> words = lexicon.tokenize_text("The above modes are default modes for these streams. These modes cannot be changed but can be clubbed together with other modes. Now for input, we can also use ifstream as shown");

    // Add words
    for (auto word : words) {
        lexicon.add_word(word);
    }

    if (!lexicon.save("/home/alkalinelemon/Projects/lexicon.txt")) {
        std::cout << "Couldn't save lexicon as file" << std::endl;
        return -1;
    }

    std::cout << "Saved to: " << "/home/alkalinelemon/Projects/lexicon.txt" << std::endl;

    return 0;
}