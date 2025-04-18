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
    std::string file_path; //store filepath for ease of closing

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
    bool write_SQL();

    bool digest_input(std::string &input) noexcept;
    void stripper(std::string &input) noexcept;

    bool process() noexcept;

    inline void show() const noexcept {
        std::cout<<"\n"<<std::endl;

        for(const user &useru : prompt){
            for(uint8_t i = 0; i < useru.prompt.size(); i++){
                switch(i){
                    case 0:
                        std::cout<<"english: ";
                        break;
                    case 1:
                        std::cout<<"romanji: ";
                        break;
                    case 2:
                        std::cout<<"japanese: ";
                        break;
                    case 3:
                        std::cout<<"full japanese: ";
                        break;
                }
                 std::cout<<useru[i]<<" meaning: "<<useru.meaning[i]<<std::endl; //print word + meaning
            }

            std::cout<<"word_id "<<useru.word_id<<std::endl;
            std::cout<<"type_id "<<useru.type_id<<std::endl;
            std::cout<<"type "<<useru.type<<std::endl;
            std::cout<<"lesson "<<useru.lesson<<'\n'<<std::endl;
        }

        std::cout<<"\n";
    }

    unsigned int choose_id() noexcept;

};

