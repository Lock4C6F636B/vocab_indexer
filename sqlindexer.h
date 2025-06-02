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

    void show_prompt() const noexcept;

    void show_tables() const noexcept;
};

