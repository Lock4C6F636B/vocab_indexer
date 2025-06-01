#include "sqlindexer.h"

bool SQLIndexer::load_SQL(const std::string filename){
    file_path = filename;

    //wipe existing vectors off the face of earth
    english.clear();
    romanji.clear();
    japanese.clear();
    full_japanese.clear();
    lore_keeper.clear();
    en_audio_crypt.clear();
    jp_audio_crypt.clear();
    prompt.clear();

    // Set up database connection
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "vocab_connection");
    db.setDatabaseName(QString::fromStdString(file_path));

    //notify how opening went
    if (!db.open()) { //throw exception if database could not open
        qDebug() << "Error opening database:" << db.lastError().text();
        return false;
    } else std::cout<<"opened database"<<std::endl;

    // Helper function to load a table into a vector
    auto loadBaseTable = [&db](const QString& tableName, std::vector<base_element> &targetVector) {
        QSqlQuery query(db);
        if (!query.exec("SELECT word_id, type, type_id, lesson, word, meaning FROM "+tableName)) {
            qDebug() << "Error executing query on" << tableName << ":" << query.lastError().text();
            return false;
        }

        while (query.next()) {
            base_element e;
            e.word_id = query.value(0).toUInt();
            e.type_id = query.value(2).toUInt();
            e.lesson = query.value(3).toUInt();
            e.meaning = query.value(5).toUInt();
            e.word = query.value(4).toString().toStdString(); //need to convert to std::string

            targetVector.push_back(e);
        }

        return true;
    };

    auto loadLoreTable = [&db, this]() {
        QSqlQuery query(db);
        if (!query.exec("SELECT word_id, en_usage, jp_usage, en_commentary_1, en_commentary_2, en_commentary_3, jp_commentary_1, jp_commentary_2, jp_commentary_3 FROM lore_keeper")) {
            qDebug() << "Error executing query on lore_keeper:" << query.lastError().text();
            return false;
        }

        while (query.next()) {
            lore lore;
            lore.word_id = query.value(0).toUInt();
            lore.en_usage = query.value(1).toString().toStdString();
            lore.jp_usage = query.value(2).toString().toStdString();
            lore.en_commentary[0] = query.value(3).toString().toStdString();
            lore.en_commentary[1] = query.value(4).toString().toStdString();
            lore.en_commentary[2] = query.value(5).toString().toStdString();
            lore.jp_commentary[0] = query.value(6).toString().toStdString();
            lore.jp_commentary[1] = query.value(7).toString().toStdString();
            lore.jp_commentary[2] = query.value(8).toString().toStdString();

            lore_keeper.push_back(lore);
        }
        return true;
    };

    auto loadAudioTable = [&db, this](const QString &table, std::vector<audio> &targetVector) {
        QSqlQuery query(db);
        if (!query.exec("SELECT word_id, path, meaning FROM "+table)) {
            qDebug() << "Error executing query on "<<table<<":"<< query.lastError().text();
            return false;
        }

        while (query.next()) {
            audio audio;
            audio.word_id = query.value(0).toUInt();
            audio.path = query.value(1).toString().toStdString();
            audio.meaning = query.value(2).toInt();

            targetVector.push_back(audio);
        }
        return true;
    };

    // Load each table
    bool success = loadBaseTable("english", english) && loadBaseTable("romanji", romanji) && loadBaseTable("japanese", japanese) && loadBaseTable("full_japanese", full_japanese) && loadLoreTable() && loadAudioTable("en_audio_crypt",en_audio_crypt) && loadAudioTable("jp_audio_crypt",jp_audio_crypt);

    std::cout<<"loading tables success"<<std::endl;

    db.close();
    return success;
}

//main control
bool SQLIndexer::process() noexcept{
    std::cout<<"Insert database filepath: ";
    std::string filepath;
    filepath = "/home/kasumi/programming/c++/HiKa_linux/sql/vocabulary.db";
    //std::cin>>filepath;
    std::cerr<<"user input: "<<filepath<<" is here"<<std::endl;
    while(!load_SQL(filepath)){
        std::cout<<"Insert file again: ";
        std::cin>>filepath;
    }

    // Clear input buffer - use ignore instead of clear
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::string prompt;


    std::cout<<"want to user file/terminal [y/n]: ";
    char terminal_file;
    std::cin>>terminal_file;
    while(terminal_file != 'y' && terminal_file != 'Y' && terminal_file != 'n' && terminal_file != 'N'){
        std::cout<<"you shall not pass: ";
        std::cin>>terminal_file;
    }

    //clear input buffer - use ignore instead of clear
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    if(terminal_file == 'y' or terminal_file == 'Y'){
        std::cout << "Enter the prompt of filepath: ";
        filepath = "/home/kasumi/programming/c++/vocab_indexer/test_simple.txt";
        //std::getline(std::cin, filepath);

        std::ifstream prompt_file(filepath);
        while(!prompt_file) {
            std::cout << "try again: ";
            std::getline(std::cin, filepath);
        }

        std::string line;
        while (std::getline(prompt_file, line)) {
            if (!line.empty()){
                prompt += line + '\n'; //keep new line to check for comments
            }
        }

        prompt_file.close();
    }
    else {
        std::cout<<"Insert command: ";
        std::getline(std::cin, prompt);
    }

    show_tables();

    if(digest_input(prompt)){
        std::cout<<"\ndigested"<<std::endl;
        choose_id();
        std::cout<<"\nid choosen"<<std::endl;
        show_prompt();

        char decision;
        do {
            std::cout<<"do you want to insert these into table? [y/n]: ";
            std::cin>>decision;

            if(decision == 'y'){
                write_SQL();
            }
            else if(decision == 'n'){
                return true;
            }
            else std::cout<<"\try again [y/n]: ";
        } while(decision != 'y' && decision !='n');
    }


    return true;
}

//strip additional whitespace
void SQLIndexer::stripper(std::string &input) noexcept {
    size_t comment_start_i = -1; //overflow means it's not initialized
    std::array<char,10> terminators = {';','|','#',',','~','@','<','>',':','$'};
    for(size_t i = 0; i < input.size();){
        if(input[i] == '/' && (i != input.size()-1) && input[i+1] == '/'){ //(i != input.size()-1) prevents checking i+1 out of bounds in case where last line is /
            comment_start_i = i;
        }

        if(comment_start_i != -1){ //if comment start is initialized
            if(input[i] == '\n' || i == input.size()-1){ //cut comment until end of line
                input.erase(comment_start_i,i-comment_start_i); //cut of comment line
                i = comment_start_i; //index must actually be synchronized to new size, otherwise out of bounds
                comment_start_i = -1; //reset value to not used
            }
            else i++; //ignore until end of the line
        }
        else{
            if(i == 0){ //ignore first member, so checking for i-1 doesn't go out of bounds
                if(std::find(terminators.begin(), terminators.end(), input[i+1]) != terminators.end()){
                    input.erase(i,1); //remove next to last character
                    continue;
                } else i++;
            }
            else if(i == input.size()-1){ //ensure last character i+1 doesn't reach out of bounds
                if(std::find(terminators.begin(), terminators.end(), input[i-1]) != terminators.end()){
                    input.erase(i,1); //remove next to last character
                    continue;
                } else i++; //or i can do break
            }
            else if(((input[i] == ' ') || (input[i] == '\t')) &&  ((std::find(terminators.begin(), terminators.end(), input[i+1]) != terminators.end()) || (std::find(terminators.begin(), terminators.end(), input[i-1]) != terminators.end()))){
                input.erase(i, 1); //erase current whitespace
            }
            else i++; //upon erasing on currnet index... the indexes shift to lower (i+1 == current i)... hence need to increment conditionally
        }
    }

    return;
}

//read user syntax
bool SQLIndexer::digest_input(std::string &input) noexcept {
    if (input.empty()) {
        std::cerr << "Command not ended with ~ terminator... bad input, you shall not pass" << std::endl;
        return false;
    }

    stripper(input); //better call stripper here

    std::vector<std::string> lines;
    std::stringstream ss(input);
    std::string line;
    while(std::getline(ss, line)) {
        if(!line.empty() && line.back() == '~') {
            lines.push_back(line);
        }
    }

    //set variables for digesting input
    std::array<char,8> terminators = {';','|','#',',','~','@','<','$'};

    for(std::string &line : lines){
        //uint8_t current_index = 0, current_metadata = 0;
        size_t last_terminator = 0, next_terminator = 0;
        size_t current_data = 0, current_comment = 0;  // Reset language index (0=english, 1=romanji, etc.) and metadata, 0-3 + 3 (type,type_id,lesson)

        // Temporary storage for current word parts... NEED for multiple entries of same word (multiple meanings)
        std::array<std::vector<std::string>, 4> words;
        uint8_t type_ID;
        std::string Type;
        unsigned int Lesson;
        std::string en_Usage, jp_Usage;
        std::vector<std::string> en_Commentary, jp_Commentary; //for both english and japanese, english first
        std::vector<std::string> en_Audio_path, jp_Audio_path;

        for(size_t i = 0; i < line.size(); i++){
            if(std::find(terminators.begin(), terminators.end(), line[i]) != terminators.end()){ //firstly find terminator
                if(current_data == 0 && last_terminator == 0){ //first word of command must always be pushed in
                    words[current_data].push_back(line.substr(last_terminator, i-last_terminator)); //take current
                }

                //handle individul terminator cases
                switch(line[i]){
                case '|': {
                    next_terminator = line.find_first_of(";|#@<$~", i+1); //find next terminator to determine length
                    words[current_data].push_back(line.substr(i+1,next_terminator-1 - i)); //store next word in on same index

                    last_terminator = i; //save position of current terminator
                    i = next_terminator -1; //land on next terminator in next loop, saves time
                    break;
                }
                case ';': {
                    current_data++; //increment to new
                    next_terminator = line.find_first_of(";|#@<$~", i+1); //find next terminator to determine length, +1 is important to not much current terminator
                    words[current_data].push_back(line.substr(i+1,next_terminator-1 -i)); //store next word

                    last_terminator = i; //save position of current terminator
                    i = next_terminator -1; //land on next terminator in next loop, saves time
                    break;
                }
                case '#': { //indicates ending words and start of metadata
                    last_terminator = i; //save position of current terminator
                    break;
                }
                case ',':{ //separator of metadata
                    current_data++;
                    switch(current_data){
                    case 4:
                        type_ID = std::stoi(line.substr(last_terminator+1, i-last_terminator-1)); //load type_id
                        break;
                    case 5:
                        Type = line.substr(last_terminator+1, i-last_terminator-1); //load type
                        if(type_ID >= types.size() || types[type_ID] != Type){  //don't initiate this entry, if either type or type_id doesnt not exist in system 0-8
                            goto skip_line;
                        }

                        next_terminator = line.find_first_of(";|#@<$~", i+1); //find next terminator for lesson data
                        Lesson = std::stoi(line.substr(i+1,next_terminator-1 -i)); //load lesson
                        break;
                    default:
                        std::cerr<<words[0][0]<<" "<<current_data<<" "<<line.substr(last_terminator+1, i-last_terminator-1)<<std::endl;
                        std::cout<<"you're not supposed to be here"<<std::endl;
                    }

                    last_terminator = i; //save position of current terminator
                    break;
                }
                case '@':{
                    if(i+3 < line.size()){ //ensure rest of operator @en: or @jp: is not out of bounds, after @ are 3 more characters
                        next_terminator = line.find_first_of(";|#@<$~", i+1); //find next terminator to determine length

                        if (next_terminator == std::string::npos) { //if no terminator found, use end of line
                            next_terminator = line.length();
                        }

                        if(line.substr(i+1, 3) == "en:"){ //check if rest of sequency is japanese or english
                            en_Usage =  line.substr(i+4,next_terminator-4 - i);
                        }
                        else if(line.substr(i+1, 3) == "jp:"){ //check if rest of sequency is japanese or english
                            jp_Usage =  line.substr(i+4,next_terminator-4 - i);
                        }
                        else{
                            std::cerr<<"@ this is not correct usage syntax"<<line.substr(i, 4)<<std::endl;
                        }
                    }

                    i = next_terminator -1; //land on next terminator in next loop, saves time
                    break;
                }
                case '<':{
                    if(i+3 < line.size() && line[i+3] == '>'){ //ensure rest of operator @en: or @jp: is not out of bounds, after @ are 3 more characters
                        next_terminator = line.find_first_of(";|#@<$~", i+4); //find next terminator to determine length

                        if (next_terminator == std::string::npos) { //if no terminator found, use end of line
                            next_terminator = line.length();
                        }

                        if(line.substr(i+1, 3) == "en>"){ //check if rest of sequency is japanese or english
                            if(en_Commentary.size() < 4) en_Commentary.push_back(line.substr(i+4,next_terminator-4 - i)); //ensure there is no more commentary than 3
                        }
                        else if(line.substr(i+1, 3) == "jp>"){ //check if rest of sequency is japanese or english
                            if(jp_Commentary.size() < 4) jp_Commentary.push_back(line.substr(i+4,next_terminator-4 - i));
                        }
                        else{
                            std::cerr<<"< this is not correct usage syntax"<<line.substr(i, 4)<<std::endl;
                        }
                    }

                    i = next_terminator -1; //land on next terminator in next loop, saves time
                    break;
                }
                case '$':{
                    if(i+3 < line.size() && line[i+3] == '$'){ //ensure rest of operator @en: or @jp: is not out of bounds, after @ are 3 more characters
                        next_terminator = line.find_first_of(";|#@<$~", i+4); //find next terminator to determine length

                        if (next_terminator == std::string::npos) { //if no terminator found, use end of line
                            next_terminator = line.length();
                        }

                        if(line.substr(i+1, 3) == "en$"){ //check if rest of sequency is japanese or english
                            en_Audio_path.push_back(line.substr(i+4,next_terminator-4 - i)); //push audio path as new meaning
                        }
                        else if(line.substr(i+1, 3) == "jp$"){ //check if rest of sequency is japanese or english
                            jp_Audio_path.push_back(line.substr(i+4,next_terminator-4 - i)); //push audio path as new meaning
                        }
                        else{
                            std::cerr<<"$ this is not correct usage syntax "<<line.substr(i, 4)<<std::endl;
                        }
                    }

                    i = next_terminator -1; //land on next terminator in next loop, saves time
                    break;
                }
                case '~':{ //end of line terminator
                    for (size_t i0 = 0; i0 < words[0].size(); i0++) {
                        for (size_t i1 = 0; i1 < words[1].size(); i1++) {
                            for (size_t i2 = 0; i2 < words[2].size(); i2++) {
                                for (size_t i3 = 0; i3 < words[3].size(); i3++) {
                                    prompt.emplace_back(user(std::array<std::string, 4>{words[0][i0], words[1][i1], words[2][i2], words[3][i3]},type_ID, Type, Lesson, en_Usage, jp_Usage, en_Commentary, jp_Commentary, en_Audio_path, jp_Audio_path));
                                }
                            }
                        }
                    }

                    break;
                }
                default: continue; //ignore if not terminator\

                }
            }
        }
        skip_line: //do absolutely nothing, just skip the line
    }

    return true;
}

unsigned int SQLIndexer::choose_id() noexcept {
    uint8_t found_match = false;
    unsigned int max_id = 0;

    //find max word_id in existance
    for (const auto &e : english) max_id = std::max(max_id, e.word_id);
    for (const auto &e : romanji) max_id = std::max(max_id, e.word_id);
    for (const auto &e : japanese) max_id = std::max(max_id, e.word_id);
    for (const auto &e : full_japanese) max_id = std::max(max_id, e.word_id);

    if(max_id == -1){ //leave if overflow of max_id
        std::cerr<<"max_id is overflow"<<std::endl;
        return 1; //
    }

    //find if word hasn't appeared in table and assign word_id
    auto assign_word_id_from_table= [this](const std::vector<base_element> &table, const uint8_t language, const unsigned int &word_index) -> bool{ //find if and where word occures
        for(const base_element &e : table){
                if(prompt[word_index].word_array[language].first == e.word){ //if the searched word of prompt is in table
                    prompt[word_index].word_id = e.word_id; //assign found id
                    return true;
            }
        }

        return false;
    };

    auto assign_mean_from_table = [this](user &line){ //find if and where word occures
            uint8_t high_mean = 0;
            for(const base_element &e: english){ //find highest meaning
                if(line.word_id == e.word_id && high_mean < e.meaning){
                        high_mean = e.meaning;
                }
            }
            line.word_array[0].meaning = ++high_mean; //assign new highest meaning to

            high_mean = 0; //reset high mean
            for(const base_element &e: romanji){ //find highest meaning
                if(line.word_id == e.word_id && high_mean < e.meaning){
                        high_mean = e.meaning;
                }
            }
            line.word_array[1].meaning = ++high_mean; //assign new highest meaning to

            high_mean = 0; //reset high mean
            for(const base_element &e: japanese){ //find highest meaning
                if(line.word_id == e.word_id && high_mean < e.meaning){
                        high_mean = e.meaning;
                }
            }
            line.word_array[2].meaning = ++high_mean; //assign new highest meaning to

            high_mean = 0; //reset high mean
            for(const base_element &e: full_japanese){ //find highest meaning
                if(line.word_id == e.word_id && high_mean < e.meaning){
                        high_mean = e.meaning;
                }
            }
            line.word_array[3].meaning = ++high_mean; //assign new highest meaning to
    };

    auto assign_audio_mean_from_table = [this](user &line){ //find if and where word occures
        uint8_t high_mean = 0;
        ///english
        if(!line.en_audio_path.empty()){
            for(const audio &path: en_audio_crypt){ //find highest meaning
                if(line.word_id == path.word_id && high_mean < path.meaning ){
                    high_mean = path.meaning;
                }
            }
            line.en_audio_path[0].meaning = ++high_mean; //assign new highest meaning to to first

            for(uint8_t itr = 1; itr < line.en_audio_path.size(); itr++){ //after finding first meaning, just increment rest from it
                line.en_audio_path[itr].meaning = ++line.en_audio_path[itr-1].meaning;
            }
        }

        //japanese
        if(!line.jp_audio_path.empty()){
            high_mean = 0;
            for(const audio &path: jp_audio_crypt){ //find highest meaning
                if(line.word_id == path.word_id && high_mean < path.meaning ){
                    high_mean = path.meaning;
                }
            }
            line.jp_audio_path[0].meaning = ++high_mean; //assign new highest meaning to to first

            for(uint8_t itr = 1; itr < line.jp_audio_path.size(); itr++){ //after finding first meaning, just increment rest from it
                line.jp_audio_path[itr].meaning = ++line.jp_audio_path[itr-1].meaning;
            }
        }
    };

    auto assign_word_id_from_prompt = [this](size_t &index) -> bool{ //check if the word is already included in before current index
        for(size_t itr = 0; itr < index; itr++) {
            if((prompt[itr].word_id != -1) && (prompt[itr][0] == prompt[index][0]) || (prompt[itr][1] == prompt[index][1]) || (prompt[itr][2] == prompt[index][2]) || (prompt[itr][3] == prompt[index][3])){ //check if one at least one INITIALIZED word in prompt before current index appear
                prompt[index].word_id = prompt[itr].word_id; //take word_id from matching object before
                return true;
            }
        }
        return false;
    };

    auto assign_mean_from_prompt = [this](user &line){
        uint8_t high_mean = 0;
        for(const user &word : prompt){ //search through english part of prompt
            if(line.word_id == word.word_id && high_mean < word.word_array[0].meaning){ //
                high_mean = word.word_array[0].meaning;
            }
        }
        line.word_array[0].meaning = ++high_mean; //assign new highest mean

        high_mean = 0;
        for(const user &word : prompt){ //search through romanji part of prompt
            if(line.word_id == word.word_id && high_mean < word.word_array[1].meaning){ //
                high_mean = word.word_array[1].meaning;
            }
        }
        line.word_array[1].meaning = ++high_mean; //assign new highest mean

        high_mean = 0;
        for(const user &word : prompt){ //search through j std::cout<<"found match for word_id "<<word.word_id<<std::endl;apanese part of prompt
            if(line.word_id == word.word_id && high_mean < word.word_array[2].meaning){ //
                high_mean = word.word_array[2].meaning;
            }
        }
        line.word_array[2].meaning = ++high_mean; //assign new highest mean

        high_mean = 0;
        for(const user &word : prompt){ //search through romanji part of prompt
            if(line.word_id == word.word_id && high_mean < word.word_array[3].meaning){ //
                high_mean = word.word_array[3].meaning;
            }
        }
        line.word_array[3].meaning = ++high_mean; //assign new highest mean
    };

    auto assign_audio_mean_from_prompt = [this](user &line){
        for(const user &entry : prompt){ //search through english part of prompt
            if(line.word_id == entry.word_id){
                //pressume that previous entry is identical
                //english
                for(uint8_t itr = 0; itr < entry.en_audio_path.size();itr++){
                    line.en_audio_path[itr].meaning = (itr < line.en_audio_path.size())? entry.en_audio_path[itr].meaning : -1; //-1 to signal wrongly accused meaning
                }

                //japanese
                for(uint8_t itr = 0; itr < entry.jp_audio_path.size();itr++){ //pressume that previous entry is identical
                    line.jp_audio_path[itr].meaning = (itr < line.jp_audio_path.size())? entry.jp_audio_path[itr].meaning : -1; //-1 to signal wrongly accused meaning
                }
            }
        }
    };


    for(size_t i = 0; i < prompt.size(); i++) {
        //find match in prompt
        found_match = assign_word_id_from_prompt(i);
        if(found_match){
            assign_mean_from_prompt(prompt[i]);
            assign_audio_mean_from_prompt(prompt[i]);
        }

        //find match in table, skip in match found in prompt
        if(!found_match){
            found_match = assign_word_id_from_table(english,0,i) | assign_word_id_from_table(romanji,1,i) | assign_word_id_from_table(japanese,2,i) | assign_word_id_from_table(full_japanese,3,i);
            if(found_match){ //assign meaning if match found in table
                assign_mean_from_table(prompt[i]);
                assign_audio_mean_from_table(prompt[i]);
            }
        }

        if(!found_match){ //if word not found, assign new max_id
            prompt[i].word_id = max_id++;
            //NOTE meanings should get intialized to first meaning 0 by constructing... no need to be explicit here
        }
    }

    return 0;
}


bool SQLIndexer::write_SQL() {
    // Set up database connection
    QSqlDatabase db = QSqlDatabase::database("vocab_connection", false);
    if (!db.isValid()) {
        db = QSqlDatabase::addDatabase("QSQLITE", "vocab_connection");
        db.setDatabaseName(QString::fromStdString(file_path));
    }

    if (!db.open()) {
        qDebug() << "Error opening database:" << db.lastError().text();
        return false;
    } std::cout<<"opening database for writting success"<<std::endl;

    // Start transaction for better performance and atomicity?
    db.transaction();
    bool insert_success = true;

    auto is_redundant_word = [this](const uint8_t languageIndex, const size_t &currentIndex) -> bool { //lagnguageIndex 0- english 1-romanji 2- japanese 3- full_japanese
        for(size_t itr = 0; itr < currentIndex; itr++) {
            if(prompt[currentIndex][languageIndex] == prompt[itr][languageIndex]) {
                return true;
            }
        }

        switch(languageIndex){
        case 0:
            for(const base_element &e : english){
                if(prompt[currentIndex][languageIndex] == e.word){
                    return true;
                }
            }
            break;

        case 1:
            for(const base_element &e : romanji){
                if(prompt[currentIndex][languageIndex] == e.word){
                    return true;
                }
            }
            break;

        case 2:
            for(const base_element &e : japanese){
                if(prompt[currentIndex][languageIndex] == e.word){
                    return true;
                }
            }
            break;

        case 3:
            for(const base_element &e : full_japanese){
                if(prompt[currentIndex][languageIndex] == e.word){
                    return true;
                }
            }
            break;

        default:
            std::cout<<"not langague index in is_redundant() lambda"<<std::endl;
        }

        return false;
    };

    auto is_redundant_lore = [this](const size_t &currentIndex) -> bool {
        for(size_t itr = 0; itr < currentIndex; itr++) { //check user prompt if word_id for lore_keeper hasn't already appeared before
            if(prompt[itr].word_id == prompt[currentIndex].word_id) {
                return true;
            }
        }

        for(size_t itr = 0; itr < lore_keeper.size(); itr++) { //check user sql table vector if word_id for lore_keeper hasn't already appeared before
            if(lore_keeper[itr].word_id == prompt[currentIndex].word_id) {
                return true;
            }
        }

        return false;
    };

    auto is_redundant_audio = [this](const bool languageIndex, const size_t &currentIndex, const size_t &currentInnerIndex) -> bool { //for sound there is only 2, 0 - english 1 - japanese
        //check user prompt if word_id for audio hasn't already appeared before
        switch(static_cast<uint8_t>(languageIndex)){ //determine if to check in english or japanese, doing both is needless
            case 0:
                //search prompt until previous entry in audio vector of same user entry
                for(size_t itr = 0; itr <= currentIndex; itr++) { //run through previous entries in prompt
                    for(size_t i = 0; i < currentInnerIndex; i++){ //run through audio vectors in each prompt entry
                        if(prompt[currentIndex].en_audio_path[currentInnerIndex].first== prompt[itr].en_audio_path[i].first) {
                            return true;
                        }
                    }
                }
                break;
            case 1:
                //search prompt until previous entry in audio vector of same user entry
                for(size_t itr = 0; itr <= currentIndex; itr++) { //run through previous entries in prompt
                    for(size_t i = 0; i < currentInnerIndex; i++){ //run through audio vectors in each prompt entry
                        if(prompt[currentIndex].en_audio_path[currentInnerIndex].first == prompt[itr].jp_audio_path[i].first) {
                            return true;
                        }
                    }
                }
                break;
        }


        //check audio database in sql... sort of
        switch(static_cast<uint8_t>(languageIndex)){
            case 0:
                for(const audio &path : en_audio_crypt){
                    if(prompt[currentIndex].en_audio_path[currentInnerIndex].first == path.path){
                        return true;
                   }
                }
            case 1:
                for(const audio &path : jp_audio_crypt){
                    if(prompt[currentIndex].jp_audio_path[currentInnerIndex].first == path.path){
                        return true;
                    }
                }
            }

            return false;
    };

    // Helper function to insert into a specific table
    auto insertIntoTable_word = [&db, &insert_success](const uint8_t languageIndex, const QString& tableName, const user& entry) {
        if (languageIndex > 3 || entry.word_array[languageIndex].first == "") return false; // Skip empty entries, though none should be empty

        /* After preparing the query but before executing it:
        std::cout << "Attempting to insert into " << tableName.toStdString()
                  << " word_id: " << entry.word_id
                  << " type: " << entry.type
                  << " type: " << static_cast<int>(entry.type_id)
                  << " word: " << entry.word_array[languageIndex].first
                  << " meaning: " << static_cast<int>(entry.word_array[languageIndex].meaning)
                  << std::endl;
        */

        QSqlQuery query(db);
        query.prepare("INSERT INTO " + tableName + " (word_id, type, type_id, lesson, word, meaning) " "VALUES (?, ?, ?, ?, ?, ?)");

        query.addBindValue(entry.word_id);
        query.addBindValue(QString::fromStdString(entry.type));
        query.addBindValue(entry.type_id);
        query.addBindValue(entry.lesson);
        query.addBindValue(QString::fromStdString(entry.word_array[languageIndex].first));
        query.addBindValue(entry.word_array[languageIndex].meaning);

        if (!query.exec()) {
            qDebug() << "Error inserting into" << tableName << ":" << query.lastError().text();
            return false;
        }

        return true;
    };

    auto insertIntoTable_add_lore = [&db, &insert_success](const user& entry) {
        QSqlQuery query(db);

        //insert word_id alone first
        query.prepare("INSERT INTO lore_keeper (word_id) " "VALUES (?)");
        query.addBindValue(entry.word_id);
        if (!query.exec()) {
            qDebug() << "Error inserting into lore_keeper:" << query.lastError().text();
            return false;
        }

        //lambda to parse ooptional values
        auto add_optional_element = [&query, &insert_success, &word_id = entry.word_id](const std::string element_name, const std::string &value){
            query.prepare("UPDATE lore_keeper SET "+ QString::fromStdString(element_name) + " = ? WHERE word_id = ?");
            query.addBindValue(QString::fromStdString(value));
            query.addBindValue(word_id);
            if (!query.exec()) {
                qDebug() << "Error updating"+ QString::fromStdString(element_name)+":"<< query.lastError().text();
                return;
            }
        };

        //insert only if not empty
        if(entry.en_usage != "") add_optional_element("en_usage", entry.en_usage);
        if(entry.jp_usage != "") add_optional_element("jp_usage", entry.jp_usage);
        if(entry.en_commentary[0] != "") add_optional_element("en_commentary_1",entry.en_commentary[0]);
        if(entry.en_commentary[1] != "") add_optional_element("en_commentary_2",entry.en_commentary[1]);
        if(entry.en_commentary[2] != "") add_optional_element("en_commentary_3",entry.en_commentary[2]);
        if(entry.jp_commentary[0] != "") add_optional_element("jp_commentary_1",entry.jp_commentary[0]);
        if(entry.jp_commentary[1] != "") add_optional_element("jp_commentary_2",entry.jp_commentary[1]);
        if(entry.jp_commentary[2] != "") add_optional_element("jp_commentary_3",entry.jp_commentary[2]);

        std::cout<<"ending lore for "<<entry.word_array[0].first<<std::endl;

        return true;
    };

    auto insertIntoTable_add_audio = [&db, &insert_success](const QString& tableName, const dual &audio_path, const unsigned &word_ID) {
        /* After preparing the query but before executing it:
        std::cout << "INSERT INTO lore_keeper (word_id, path, meaning) VALUES (" //output to make sure
                  << word_ID << ","
                  << "'" << audio_path.first << "',"
                  << "'" << audio_path.meaning << "',"

                  << ")" << std::endl;
        */

        QSqlQuery query(db);

        query.prepare("INSERT INTO " + tableName + " (word_id, path, meaning) " "VALUES (?, ?, ?)");
        query.addBindValue(word_ID);
        query.addBindValue(QString::fromStdString(audio_path.first));
        query.addBindValue(audio_path.meaning);

        if (!query.exec()) {
            qDebug() << "Error inserting into " + tableName + ":" << query.lastError().text();
            return false;
        }

        return true;
    };

    // Process each entry
    for (size_t i = 0; i < prompt.size(); ++i) {
        // Insert into each table if the corresponding field is not empty
        if(prompt[i].word_id == -1) goto skip_entry;

        //english
        if(!is_redundant_word(0,i)){ //insert only if the word not already included in table
            insert_success = insertIntoTable_word(0, "english", prompt[i]);
            if(!insert_success){ //attempt insertion into english, if fail end the function
                break;
            }
        }

        //romanji
        if(!is_redundant_word(1,i)){ //insert only if the word not already included in table
            insert_success = insertIntoTable_word(1, "romanji", prompt[i]);
            if(!insert_success){
                break;
            }
        }

        //japanese
        if(!is_redundant_word(2,i)){ //insert only if the word not already included in table
            insert_success = insertIntoTable_word(2, "japanese", prompt[i]);
            if(!insert_success){ //attempt insertion into english, if fail end the function
                break;
            }
        }

        //full_japanese
        if(!is_redundant_word(3,i)){ //insert only if the word not already included in table
            insert_success = insertIntoTable_word(3, "full_japanese", prompt[i]);
            if(!insert_success){ //attempt insertion into english, if fail end the function
                break;
            }
        }

        //lore_keeper
        if(!is_redundant_lore(i) && (prompt[i].en_usage != "" || prompt[i].jp_usage != "" || prompt[i].en_commentary[0] != "" || prompt[i].jp_commentary[0] != "")){ //insert only if the word not already included in table
            insert_success = insertIntoTable_add_lore(prompt[i]);
            if(!insert_success){ //attempt insertion into english, if fail end the function
                std::cout<<"yolo"<<std::endl;
                break;
            }
        }

        //en voice_crypt
        for(uint8_t itr = 0; itr < prompt[i].en_audio_path.size(); itr++){
            if(!is_redundant_audio(0, i, itr) && prompt[i].en_audio_path[itr].first != ""){ //insert only if the word not already included in table or empty
                insert_success = insertIntoTable_add_audio("en_audio_crypt", prompt[i].en_audio_path[itr], prompt[i].word_id);
                if(!insert_success){ //attempt insertion into english, if fail end the function
                    break;
                }
            }
        }

        //jp voice_crypt
        for(uint8_t itr = 0; itr < prompt[i].jp_audio_path.size(); itr++){
            if(!is_redundant_audio(0, i, itr) && prompt[i].jp_audio_path[itr].first != ""){ //insert only if the word not already included in table or empty
                insert_success = insertIntoTable_add_audio("jp_audio_crypt", prompt[i].jp_audio_path[itr], prompt[i].word_id);
                if(!insert_success){ //attempt insertion into english, if fail end the function
                    break;
                }
            }
        }

        skip_entry:
    }

    if(insert_success){
        db.commit();
        std::cout<<"inserting into database went well"<<std::endl;
    }else db.rollback();

    db.close();
    std::cout<<"closed database"<<std::endl;
    return true;
}


void SQLIndexer::show_tables() const noexcept {
    std::cout << "\n=== DEBUG: LOADED DATA FROM DATABASE ===" << std::endl;

    // English table
    std::cout << "\n--- ENGLISH TABLE (" << english.size() << " entries) ---" << std::endl;
    for (const auto& e : english) {
        std::cout << "ID:" << e.word_id << " Type:" << static_cast<int>(e.type_id)
        << " Lesson:" << e.lesson << " Meaning:" << static_cast<int>(e.meaning)
        << " Word:\"" << e.word << "\"" << std::endl;
    }

    // Romanji table
    std::cout << "\n--- ROMANJI TABLE (" << romanji.size() << " entries) ---" << std::endl;
    for (const auto& e : romanji) {
        std::cout << "ID:" << e.word_id << " Type:" << static_cast<int>(e.type_id)
        << " Lesson:" << e.lesson << " Meaning:" << static_cast<int>(e.meaning)
        << " Word:\"" << e.word << "\"" << std::endl;
    }

    // Japanese table
    std::cout << "\n--- JAPANESE TABLE (" << japanese.size() << " entries) ---" << std::endl;
    for (const auto& e : japanese) {
        std::cout << "ID:" << e.word_id << " Type:" << static_cast<int>(e.type_id)
        << " Lesson:" << e.lesson << " Meaning:" << static_cast<int>(e.meaning)
        << " Word:\"" << e.word << "\"" << std::endl;
    }

    // Full Japanese table
    std::cout << "\n--- FULL_JAPANESE TABLE (" << full_japanese.size() << " entries) ---" << std::endl;
    for (const auto& e : full_japanese) {
        std::cout << "ID:" << e.word_id << " Type:" << static_cast<int>(e.type_id)
        << " Lesson:" << e.lesson << " Meaning:" << static_cast<int>(e.meaning)
        << " Word:\"" << e.word << "\"" << std::endl;
    }

    // Lore keeper table
    std::cout << "\n--- LORE_KEEPER TABLE (" << lore_keeper.size() << " entries) ---" << std::endl;
    for (const auto& l : lore_keeper) {
        std::cout << "ID:" << l.word_id << std::endl;
        std::cout << "  EN_Usage: \"" << l.en_usage << "\"" << std::endl;
        std::cout << "  JP_Usage: \"" << l.jp_usage << "\"" << std::endl;
        std::cout << "  EN_Comments: [\"" << l.en_commentary[0] << "\", \""
                  << l.en_commentary[1] << "\", \"" << l.en_commentary[2] << "\"]" << std::endl;
        std::cout << "  JP_Comments: [\"" << l.jp_commentary[0] << "\", \""
                  << l.jp_commentary[1] << "\", \"" << l.jp_commentary[2] << "\"]" << std::endl;
    }

    // English audio
    std::cout << "\n--- EN_AUDIO_CRYPT TABLE (" << en_audio_crypt.size() << " entries) ---" << std::endl;
    for (const auto& a : en_audio_crypt) {
        std::cout << "ID:" << a.word_id << " Meaning:" << static_cast<int>(a.meaning)
        << " Path:\"" << a.path << "\"" << std::endl;
    }

    // Japanese audio
    std::cout << "\n--- JP_AUDIO_CRYPT TABLE (" << jp_audio_crypt.size() << " entries) ---" << std::endl;
    for (const auto& a : jp_audio_crypt) {
        std::cout << "ID:" << a.word_id << " Meaning:" << static_cast<int>(a.meaning)
        << " Path:\"" << a.path << "\"" << std::endl;
    }

    std::cout << "\n=== END DEBUG OUTPUT ===" << std::endl;
}
