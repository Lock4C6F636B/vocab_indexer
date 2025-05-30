#pragma once
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QtSql/QSqlRecord>
#include <QDebug>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <array>


struct dual{
    std::string first;
    uint8_t meaning;

    dual() = default;
    dual(const std::string &First):first(First){}
    dual(const std::string_view &First):first(First){}
};

struct base_element {
    unsigned int word_id;
    uint8_t type_id;
    unsigned int lesson;
    uint8_t meaning;
    std::string word;
};

struct lore{
    unsigned int word_id;
    std::string en_usage, jp_usage;
    std::array<std::string,3> en_commentary, jp_commentary; //for both english and japanese, english first
};

struct audio{
    unsigned int word_id;
    std::string path;
    uint8_t meaning;
};


struct user{
    std::array<dual,4> word_array; //here user inputs one string for english, one for romanji and so on... meaning is not universal...
    //std::array<uint8_t, 4> meaning = {0,0,0,0};
    unsigned int word_id = -1; //oveflow means object is not properly initiliazed (value is not set correctly)
    uint8_t type_id;
    std::string type;
    unsigned int lesson;

    //optional data... good to include;
    std::string en_usage, jp_usage;
    std::array<std::string,3> en_commentary, jp_commentary; //for both english and japanese, english first
    std::vector<dual> en_audio_path, jp_audio_path;


    user() = default;

    // Constructor from array of strings
    user(const std::array<std::string, 4>& words) {
        for(uint8_t itr = 0; itr < 4; itr++){ //copy words by value
            word_array[itr].first = words[itr];
        }
    }

    // Constructor from array of strings and metadata
    user(const std::array<std::string, 4>& words, const uint8_t &Type_Id, const std::string &Type, const unsigned int & Lesson): type_id(Type_Id), type(Type), lesson(Lesson){
        for(uint8_t itr = 0; itr < 4; itr++){ //copy words by value
            word_array[itr].first = words[itr];
        }
    }

    // Constructor for a single word with index and metadata
    user(const std::array<std::string, 4>& words, const uint8_t &Type_Id, const std::string &Type, const unsigned int &Lesson, const std::string &En_Usage, const std::string &Jp_Usage, const std::vector<std::string> &En_Commentary, const std::vector<std::string> &Jp_Commentary, const std::vector<std::string> &En_Audio_Path, const std::vector<std::string> &Jp_Audio_Path)
        : type_id(Type_Id), type(Type), lesson(Lesson),
        en_usage(En_Usage), jp_usage(Jp_Usage){

        //word array with meaning
        for(uint8_t itr = 0; itr < 4; itr++){ //copy words by value
            word_array[itr].first = words[itr];
        }

        //commentary
        for(uint8_t i = 0; i < 3; i++){
            en_commentary[i] = i < En_Commentary.size()? En_Commentary[i] : ""; //if i is in range of commentary vector, copy the value, otherwise intialize empty
            jp_commentary[i] = i < Jp_Commentary.size()? Jp_Commentary[i] : "";
        }

        //audio with meaning
        for(uint8_t i = 0; i < En_Audio_Path.size(); i++){
            en_audio_path.emplace_back(En_Audio_Path[i]);
        }

        for(uint8_t i = 0; i < Jp_Audio_Path.size(); i++){
            jp_audio_path.push_back(Jp_Audio_Path[i]);
        }

        std::cout<<"\n";
    }

    uint8_t get_mean(uint8_t language){ //returning meaning in this language
        return word_array[language].meaning;
    }

    // Index operator to access individual strings
    std::string& operator[](size_t index) {
        return word_array[index].first;
    }

    // Const version of the index operator
    const std::string& operator[](size_t index) const {
        return word_array[index].first;
    }
};


static constexpr std::array<std::string_view,9> types{"mixed","noun","adjective","pronoun","number","verb","adverb","preposition","conjuction"};
