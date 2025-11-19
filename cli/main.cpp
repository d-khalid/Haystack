#include <iostream>
#include <bits/ostream.tcc>

#include "lexicon.hpp"

// Test program
int main() {
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