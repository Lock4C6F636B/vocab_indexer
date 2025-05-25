#pragma once
#include "database_structures.h"

class SQLIndexer{
private:
    std::string file_path; //store filepath for ease of closing

    std::vector<base_element> english;
    std::vector<base_element> romanji;
    std::vector<base_element> japanese;
    std::vector<base_element> full_japanese;
    std::vector<lore> lore_keeper;
    std::vector<audio> en_audio_crypt;
    std::vector<audio> jp_audio_crypt;

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

    inline void show_prompt() const noexcept {
        std::cout<<"starting show: \n"<<std::endl;

        for(const user &useru : prompt){
            auto commentary =[](const std::array<std::string,3> &comments){
                std::cout<<"commentary: ";
                for(const std::string &comment : comments){
                    std::cout<<comment<<" | ";
                }
                std::cout<<"  ";
            };

            for(uint8_t i = 0; i < useru.word_array.size(); i++){
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
                 std::cout<<useru[i]<<" meaning: "<< static_cast<int>(useru.word_array[i].meaning)<<std::endl; //print word + meaning
            }

            std::cout<<"word_id: "<<useru.word_id<<std::endl;
            std::cout<<"type_id: "<< static_cast<int>(useru.type_id) << std::endl;
            std::cout<<"type: "<<useru.type<<std::endl;
            std::cout<<"lesson: "<<useru.lesson<<std::endl;

            //optional data
            //lore
            std::cout<<"en usage: "<<useru.en_usage<<" | jp usage: "<<useru.jp_usage<<std::endl;
            commentary(useru.en_commentary);
            commentary(useru.jp_commentary);
            std::cout<<""<<std::endl;

            //audio
            for(const dual &audio_path : useru.en_audio_path){
                std::cout<<"\nen audio path: "<<audio_path.first<<" meaning: "<<audio_path.meaning;
            }
            std::cout<<'\n';
            for(const dual &audio_path : useru.jp_audio_path){
                std::cout<<"\en audio path: "<<audio_path.first<<" meaning: "<<audio_path.meaning;;
            }
        }
    }
};

