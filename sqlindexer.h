#pragma once
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QtSql/QSqlRecord>
#include <QDebug>
#include <string>
#include <vector>
#include <array>

struct element {
    unsigned int word_id;
    uint8_t type_id;
    unsigned int lesson;
    uint8_t meaning;
    std::string word;
};

struct user{
    std::array<std::string,4> prompt; //here user inputs one string for english, one for romanji and so on...
    std::array<uint8_t, 4> meaning = {0,0,0,0}; //meaning is not universal... preset for one meaning
    unsigned int word_id;
    uint8_t type_id;
    std::string type;
    unsigned int lesson;

    user() = default;

    // Constructor from array of strings
    user(const std::array<std::string, 4>& words) {
        prompt = words;
    }

    // Constructor for a single word with index
    user(const std::array<std::string, 4>& words, int index) {
        prompt = words;
    }

    void assign_mean(uint8_t language, unsigned int current_mean){ //increment existing meaning
        meaning[language] = current_mean++;
    }

    uint8_t get_mean(uint8_t language){ //returning meaning in this language
        return meaning[language];
    }

    // Index operator to access individual strings
    std::string& operator[](size_t index) {
        return prompt[index];
    }

    // Const version of the index operator
    const std::string& operator[](size_t index) const {
        return prompt[index];
    }
};


class SQLIndexer{
private:
    std::vector<element> english;
    std::vector<element> romanji;
    std::vector<element> japanese;
    std::vector<element> full_japanese;


    std::vector<user> prompt;
    //std::vector<std::array<std::string,4>> prompt; //here user inputs one string for english, one for romanji and so on...
    //std::vector<std::array<std::string,4>> pr;

public:
    SQLIndexer() = default;
    ~SQLIndexer() = default;

    bool load_SQL(const std::string filename);
    bool write_SQL(const std::string& filename, const std::string& type);

    void digest_terminal_input(const std::string input);

    unsigned int choose_id();

};

