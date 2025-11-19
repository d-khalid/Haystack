#ifndef LEXICON_H
#define LEXICON_H

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

class Lexicon {
public:
    // Adds a word and returns its id
    uint64_t add_word(std::string word);

    // Returns empty string if no word exists
    std::string get_word(int word_id);

    // Returns 0 if no id exists
    uint64_t get_word_id(std::string word);

    // Converts a given chunk of text into normalized tokens
    std::vector<std::string> tokenize_text(std::string text, char delim = ' ');

    // Returns true on success, false on failure
    bool save(std::string file_path);

    // Returns true on success, false on failure
    bool load(std::string file_path);

    Lexicon();


private:
    std::vector<std::string> id_to_word;
    std::unordered_map<std::string, uint64_t> word_to_id;

    uint64_t next_id = 1; // 0 is for not found

    // Normalizes the given token by converting to lowercase, trimming and removing certain symbols
    std::string normalize_token(std::string token);

    // Removes leading and lagging spaces
    // TODO: Make a separate file for such utility functions
    static std::string trim(const std::string & source, std::string remove_left = "", std::string remove_right = "");
};

#endif //LEXICON_H
