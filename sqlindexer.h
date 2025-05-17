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

struct lore{
    unsigned int word_id;
    std::string en_usage, jp_usage;
    std::array<std::string,3> en_commentary, jp_commentary; //for both english and japanese, english first
};

struct audio{
    unsigned int word_id;
    std::string en_audio_path, jp_audio_path;
};

struct user{
    std::array<std::string,4> prompt; //here user inputs one string for english, one for romanji and so on...
    std::array<uint8_t, 4> meaning = {0,0,0,0}; //meaning is not universal... preset for one meaning
    unsigned int word_id = -1; //oveflow means object is not properly initiliazed (value is not set correctly)
    uint8_t type_id;
    std::string type;
    unsigned int lesson;

    //optional data... good to include
    std::string en_usage, jp_usage;
    std::array<std::string,3> en_commentary, jp_commentary; //for both english and japanese, english first
    std::string en_audio_path, jp_audio_path;

    user() = default;

    // Constructor from array of strings
    user(const std::array<std::string, 4>& words) {
        prompt = words;
    }

    // Constructor for a single word with index
    user(const std::array<std::string, 4>& words, int index) {
        prompt = words;
    }

    // Constructor from array of strings and metadata
    user(const std::array<std::string, 4>& words, const uint8_t &Type_Id, const std::string &Type, const unsigned int & Lesson): type_id(Type_Id), type(Type), lesson(Lesson){
        prompt = words;
    }

    // Constructor for a single word with index and metadata
    user(const std::array<std::string, 4>& words, const uint8_t &Type_Id, const std::string &Type, const unsigned int &Lesson, const std::string &En_Usage, const std::string &Jp_Usage, const std::vector<std::string> &En_Commentary, const std::vector<std::string> &Jp_Commentary, const std::string &En_Audio_Path, const std::string &Jp_Audio_Path)
        : type_id(Type_Id), type(Type), lesson(Lesson),
        en_usage(En_Usage), jp_usage(Jp_Usage),
        en_audio_path(En_Audio_Path), jp_audio_path(Jp_Audio_Path) {
            prompt = words;

            for(uint8_t i = 0; i < 3; i++){
                en_commentary[i] = i < En_Commentary.size()? En_Commentary[i] : ""; //if i is in range of commentary vector, copy the value, otherwise intialize empty
                jp_commentary[i] = i < Jp_Commentary.size()? Jp_Commentary[i] : "";
            }

            std::cout<<"size en: "<<En_Commentary.size()<<", size jp: "<<Jp_Commentary.size();
            for(const auto comment: en_commentary){
                std::cout<<"["<<comment<<"] ";
            }

            for(const auto comment: jp_commentary){
                std::cout<<"["<<comment<<"] ";
            }
            std::cout<<"\n";
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


static constexpr std::array<std::string_view,9> types{"mixed","noun","adjective","pronoun","number","verb","adverb","preposition","conjuction"};

class SQLIndexer{
private:
    std::string file_path; //store filepath for ease of closing

    std::vector<element> english;
    std::vector<element> romanji;
    std::vector<element> japanese;
    std::vector<element> full_japanese;
    std::vector<lore> lore_keeper;
    std::vector<audio> voice_crypt;

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

    unsigned int choose_id() noexcept;

    inline void show() const noexcept {
        std::cout<<"starting show: \n"<<std::endl;

        for(const user &useru : prompt){
            auto commentary =[](const std::array<std::string,3> &comments){
                std::cout<<"commentary: ";
                for(const std::string &comment : comments){
                    std::cout<<comment<<" | ";
                }
                std::cout<<"  ";
            };

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
                 std::cout<<useru[i]<<" meaning: "<< static_cast<int>(useru.meaning[i])<<std::endl; //print word + meaning
            }

            std::cout<<"word_id: "<<useru.word_id<<std::endl;
            std::cout<<"type_id: "<< static_cast<int>(useru.type_id) << std::endl;
            std::cout<<"type: "<<useru.type<<std::endl;
            std::cout<<"lesson: "<<useru.lesson<<std::endl;
            std::cout<<"en usage: "<<useru.en_usage<<" | jp usage: "<<useru.jp_usage<<std::endl;
            commentary(useru.en_commentary);
            commentary(useru.jp_commentary);
            std::cout<<"\nen audio path: "<<useru.en_audio_path<<" | jp audio path: "<<useru.jp_audio_path<<"\n"<<std::endl;

        }

        std::cout<<"\n";
    }
};

