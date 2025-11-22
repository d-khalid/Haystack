#include "lexicon.hpp"

#include <algorithm>
#include <filesystem>

#include "pugixml.hpp"
#include <iostream>
#include <fstream>

// Constructor
Lexicon::Lexicon() {
    // Reserve id 0 for not found
    id_to_word.emplace_back("");
    next_id = 1;
}

uint64_t Lexicon::add_word(std::string word) {

    // Check if exists before adding
    if (word_to_id.count(word) > 0) {
        return word_to_id[word];
    }

    // Create hashmap entry and vector entry
    id_to_word.emplace_back(word);
    word_to_id.emplace(word, next_id);

    return next_id++;
}

std::string Lexicon::get_word(int word_id) {
    // Vector bounds check
    if (word_id < 1 || word_id >= id_to_word.size()) {
        return "";
    }
    return id_to_word[word_id];
}

uint64_t Lexicon::get_word_id(std::string word) {
    return word_to_id.count(word) == 0 ? 0 : word_to_id[word];
}

// Might need better exception handling/checks
bool Lexicon::save(std::string file_path) {
    std::fstream lex_file(file_path, std::ios::app | std::ios::in); // In Append+Input mode


    // Write words seperated by spaces
    for (int i = 1; i < id_to_word.size() - 1; i++) {
        lex_file << id_to_word[i] << " ";
    }
    if (id_to_word.size() > 1) {
        lex_file << id_to_word.back();
    }

    lex_file.close();
    return true;
}

bool Lexicon::load(std::string file_path) {
    if (!std::filesystem::exists(file_path)) {
        std::cerr << "File " << file_path << " does not exist" << std::endl;
        return false;
    }
    if (id_to_word.size() > 1) {
        std::cerr << "Lexicon already has data! File " << file_path << "not loaded." << std::endl;
    }

    std::fstream lex_file(file_path, std::ios::out); // Output mode

    std::string word_buffer;

    // Load words into lexicon from file
    while (lex_file >> word_buffer) {
        add_word(word_buffer);
    }

    lex_file.close();
}

std::vector<std::string> Lexicon::tokenize_text(std::string text, char delim) {
    int last = 0;
    std::vector<std::string> tokens;

    // Trim before parsing
    text = trim(text);

    for (int i = 0; i < text.size(); i++) {
        if (text[i] == delim) {
            tokens.push_back(normalize_token(
                text.substr(last, (i + 1) - last)
            ));
            last = i + 1;
        }
    }
    // Push whatever remains
    tokens.push_back(normalize_token(
        text.substr(last)
    ));

    return tokens;
}

// TODO: Handle symbols properly,
// problem is that some symbols are meaningful parts of tech terms (e.g C#, .NET, dots in versioning, etc)
std::string Lexicon::normalize_token(std::string token) {
    // Trim, remove commas and periods
    token = trim(token, ".,", ",.");

    // To lowercase
    std::transform(token.begin(), token.end(), token.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    return token;
}

// You can specify more characters to be trimmed from left or right as needed
std::string Lexicon::trim(const std::string &source, std::string remove_left, std::string remove_right) {
    std::string s(source);
    s.erase(0, s.find_first_not_of(remove_left.append(" \n\r\t")));
    s.erase(s.find_last_not_of(remove_right.append(" \n\r\t")) + 1);
    return s;
}
