#include "sqlindexer.h"

bool SQLIndexer::load_SQL(const std::string filename){
    file_path = filename;

    //wipe existing vectorsoff the face of earth
    english.clear();
    romanji.clear();
    japanese.clear();
    full_japanese.clear();

    // Set up database connection
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "vocab_connection");
    db.setDatabaseName(QString::fromStdString(file_path));

    //notify how opening went
    if (!db.open()) { //throw exception if database could not open
        qDebug() << "Error opening database:" << db.lastError().text();
        return false;
    } else std::cout<<"opened database"<<std::endl;

    // Helper function to load a table into a vector
    auto loadTable = [&db](const QString& tableName, std::vector<element> &targetVector) {
        QSqlQuery query(db);
        if (!query.exec("SELECT word_id, type, type_id, lesson, word, meaning FROM " + tableName)) {
            qDebug() << "Error executing query on" << tableName << ":" << query.lastError().text();
            return false;
        }

        while (query.next()) {
            element e;
            e.word_id = query.value(0).toUInt();
            e.type_id = query.value(2).toUInt();
            e.lesson = query.value(3).toUInt();
            e.meaning = query.value(5).toUInt();
            e.word = query.value(4).toString().toStdString(); //need to convert to std::string

            targetVector.push_back(e);
        }
        return true;
    };

    // Load each table
    bool success = loadTable("english", english) && loadTable("romanji", romanji) && loadTable("japanese", japanese) && loadTable("full_japanese", full_japanese);

    std::cout<<"loading tables success"<<std::endl;

    db.close();
    return success;
}

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
        filepath = "/home/kasumi/programming/c++/vocab_indexer/sample.txt";
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



    if(digest_input(prompt)){
        choose_id();
        show();

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

void SQLIndexer::stripper(std::string &input) noexcept {
    size_t comment_start_i = -1; //overflow means it's not initialized
    std::array<char,5> terminators = {';','|','#',',','~'};
    for(size_t i = 0; i < input.size();){
        if(input[i] == '/' && (i != input.size()-1) && input[i+1] == '/'){ //(i != input.size()-1) prevents checking i+1 out of bounds in case where last line is /
            comment_start_i = i;
        }

        if(comment_start_i != -1){
            if(input[i] == '\n' || i == input.size()-1){ //cut comment ad the end of line or end of file
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
            else if(((input[i] == ' ') || (input[i] == '\n') || (input[i] == '\t')) &&  ((std::find(terminators.begin(), terminators.end(), input[i+1]) != terminators.end()) || (std::find(terminators.begin(), terminators.end(), input[i-1]) != terminators.end()))){
                input.erase(i, 1); //erase current whitespace
            }
            else i++; //upon erasing on currnet index... the indexes shift to lower (i+1 == current i)... hence need to increment conditionally
        }
    }

    std::cout<<'\n'<<'\n'<<input<<'\n'<<std::endl; //just debug

    return;
}

bool SQLIndexer::digest_input(std::string &input) noexcept {
    if (input.empty()) {
        std::cerr << "Command not ended with ~ terminator... bad input, you shall not pass" << std::endl;
        return false;
    }

    stripper(input); //better for function to call stripper by itself

    //cut input into separate lines
    std::vector<std::string> lines;
    for(size_t i = 0; i < input.size();i++){
        if(input[i] == '~'){
            lines.emplace_back(input.substr(0,i+1));
            input = input.substr(i+1);
            i = 0;
        }
    }

    //set variables for digesting input
    std::array<char,8> terminators = {';','|','#',',','~','@','<','$'};

    for(std::string &line : lines){
        //uint8_t current_index = 0, current_metadata = 0;
        size_t last_terminator = 0, next_terminator = 0;
        size_t index = 0;  // Reset language index (0=english, 1=romanji, etc.) and metadata, 0-3 + 3 (type,type_id,lesson)

        // Temporary storage for current word parts
        std::array<std::vector<std::string>, 4> words;
        uint8_t type_ID;
        std::string type;
        unsigned int lesson;

        for(size_t i = 0; i < line.size(); i++){
            if(std::find(terminators.begin(), terminators.end(), line[i]) != terminators.end()){ //firstly find terminator
                if(index == 0 && last_terminator == 0){ //first word of command must always be pushed in
                    words[index].push_back(line.substr(last_terminator, i-last_terminator)); //take current
                }

                //handle individul terminator cases
                switch(line[i]){
                case '|': {
                    next_terminator = line.find_first_of(";|#~\n", i+1); //find next terminator to determine length
                    words[index].push_back(line.substr(i+1,next_terminator-i-1)); //store next word in on same index

                    last_terminator = i; //save position of current terminator
                    i = next_terminator -1; //land on next terminator in next loop, saves time
                    break;
                }
                case ';': {
                    index++; //increment to new
                    next_terminator = line.find_first_of(";|#~\n", i+1); //find next terminator to determine length, +1 is important to not much current terminator
                    words[index].push_back(line.substr(i+1,next_terminator-i-1)); //store next word

                    last_terminator = i; //save position of current terminator
                    i = next_terminator -1; //land on next terminator in next loop, saves time
                    break;
                }
                case '#': { //indicates ending words and start of metadata
                    last_terminator = i; //save position of current terminator
                    break;
                }
                case ',':{ //separator of metadata
                    index++;
                    switch(index){
                    case 4:
                        type_ID = std::stoi(line.substr(last_terminator+1, i-last_terminator-1));
                        break;
                    case 5:
                        type = line.substr(last_terminator+1, i-last_terminator-1);
                        break;
                    default:
                        std::cout<<"you're not supposed to be here"<<std::endl;
                    }

                    last_terminator = i; //save position of current terminator
                    break;
                }
                case '~':{ //end of line terminator
                    //need to resolve lesson metadata here
                    lesson = std::stoi(line.substr(last_terminator+1, i-last_terminator-1));

                    for (size_t i0 = 0; i0 < words[0].size(); i0++) {
                        for (size_t i1 = 0; i1 < words[1].size(); i1++) {
                            for (size_t i2 = 0; i2 < words[2].size(); i2++) {
                                for (size_t i3 = 0; i3 < words[3].size(); i3++) {
                                    prompt.emplace_back(user(std::array<std::string, 4>{words[0][i0], words[1][i1], words[2][i2], words[3][i3]},type_ID, type, lesson));
                                }
                            }
                        }
                    }
                }
                case '@':{
                    if(i+3 < input.size()){ //ensure rest of operator @en: or @jp: is not out of bounds, after @ are 3 more characters
                        next_terminator = line.find_first_of("@<$\n", i+1); //find next terminator to determine length

                        if (next_terminator == std::string::npos) { //if no terminator found, use end of line
                            next_terminator = line.length();
                        }

                        if(input.substr(i+1, 3) == "en:"){ //check if rest of sequency is japanese or english
                            prompt[prompt.size()-1].en_usage =  line.substr(i+3,next_terminator-i+2);
                        }
                        else if(input.substr(i+1, 3) == "jp:"){ //check if rest of sequency is japanese or english
                            prompt[prompt.size()-1].jp_usage =  line.substr(i+3,next_terminator-i+2);
                        }
                        else{
                            std::cerr<<"this is not correct usage syntax"<<std::endl;
                        }
                    }
                    i = next_terminator -1; //land on next terminator in next loop, saves time
                }
                case '<':{
                    if(i+3 < input.size()){ //ensure rest of operator @en: or @jp: is not out of bounds, after @ are 3 more characters
                        next_terminator = line.find_first_of("@<$\n", i+1); //find next terminator to determine length

                        if (next_terminator == std::string::npos) { //if no terminator found, use end of line
                            next_terminator = line.length();
                        }

                        if(input.substr(i+1, 3) == "en>"){ //check if rest of sequency is japanese or english
                            if(prompt[prompt.size()-1].en_commentary.size() < 4) prompt[prompt.size()-1].en_commentary.push_back(line.substr(i+3,next_terminator-i+2)); //ensure there is no more commentary than 3
                        }
                        else if(input.substr(i+1, 3) == "jp>"){ //check if rest of sequency is japanese or english
                            if(prompt[prompt.size()-1].jp_commentary.size() < 4) prompt[prompt.size()-1].jp_commentary.push_back(line.substr(i+3,next_terminator-i+2));
                        }
                        else{
                            std::cerr<<"this is not correct usage syntax"<<std::endl;
                        }
                    }
                    i = next_terminator -1; //land on next terminator in next loop, saves time
                }
                case '$':{

                }
                default: continue; //ignore if not terminator

                }

                last_terminator = i;
            }
        }
    }


    return true;
}

unsigned int SQLIndexer::choose_id() noexcept {
    uint8_t found_match = false;
    unsigned int max_id = 0;

    //find max word_id in existance
    for (const auto& e : english) max_id = std::max(max_id, e.word_id);
    for (const auto& e : romanji) max_id = std::max(max_id, e.word_id);
    for (const auto& e : japanese) max_id = std::max(max_id, e.word_id);
    for (const auto& e : full_japanese) max_id = std::max(max_id, e.word_id);

    if(max_id == -1){ //leave if overflow of max_id
        std::cerr<<"max_id is overflow"<<std::endl;
        return 1; //
    }

    auto find_match_in_table= [this](const std::vector<element> &table, const uint8_t language, const unsigned int &word_index) -> bool{ //find if and where word occures
        for(const element e : table){
                if(prompt[word_index].prompt[language] == e.word){ //if the searched word of prompt is in table
                    prompt[word_index].word_id = e.word_id; //assign found id
                    return true;
            }
        }

        return false;
    };

    auto assign_mean_in_table = [this](user &line){ //find if and where word occures
            uint8_t high_mean = 0;
            for(const element e: english){ //find highest meaning
                if(line.word_id == e.word_id && high_mean < e.meaning){
                        high_mean = e.meaning;
                }
            }
            line.meaning[0] = ++high_mean; //assign new highest meaning to

            high_mean = 0; //reset high mean
            for(const element e: romanji){ //find highest meaning
                if(line.word_id == e.word_id && high_mean < e.meaning){
                        high_mean = e.meaning;
                }
            }
            line.meaning[1] = ++high_mean; //assign new highest meaning to

            high_mean = 0; //reset high mean
            for(const element e: japanese){ //find highest meaning
                if(line.word_id == e.word_id && high_mean < e.meaning){
                        high_mean = e.meaning;
                }
            }
            line.meaning[2] = ++high_mean; //assign new highest meaning to

            high_mean = 0; //reset high mean
            for(const element e: full_japanese){ //find highest meaning
                if(line.word_id == e.word_id && high_mean < e.meaning){
                        high_mean = e.meaning;
                }
            }
            line.meaning[3] = ++high_mean; //assign new highest meaning to
    };


    auto find_match_in_prompt = [this](size_t &index) -> bool{ //check if the word is already included in before current index
        for(size_t itr = 0; itr < index; itr++) {
            if((prompt[itr].word_id != -1) && (prompt[itr][0] == prompt[index][0]) || (prompt[itr][1] == prompt[index][1]) || (prompt[itr][2] == prompt[index][2]) || (prompt[itr][3] == prompt[index][3])){ //check if one at least one INITIALIZED word in prompt before current index appear
                prompt[index].word_id = prompt[itr].word_id; //take word_id from matching object before
                return true;
            }
        }
        return false;
    };

    auto assign_mean_in_prompt = [this](user &line){
        uint8_t high_mean = 0;
        for(const user &word : prompt){ //search through english part of prompt
            if(line.word_id == word.word_id && high_mean < word.meaning[0]){ //
                high_mean = word.meaning[0];
            }
        }
        line.meaning[0] = ++high_mean; //assign new highest mean

        high_mean = 0;
        for(const user &word : prompt){ //search through romanji part of prompt
            if(line.word_id == word.word_id && high_mean < word.meaning[1]){ //
                high_mean = word.meaning[1];
            }
        }
        line.meaning[1] = ++high_mean; //assign new highest mean

        high_mean = 0;
        for(const user &word : prompt){ //search through j std::cout<<"found match for word_id "<<word.word_id<<std::endl;apanese part of prompt
            if(line.word_id == word.word_id && high_mean < word.meaning[2]){ //
                high_mean = word.meaning[2];
            }
        }
        line.meaning[2] = ++high_mean; //assign new highest mean

        high_mean = 0;
        for(const user &word : prompt){ //search through romanji part of prompt
            if(line.word_id == word.word_id && high_mean < word.meaning[3]){ //
                high_mean = word.meaning[3];
            }
        }
        line.meaning[3] = ++high_mean; //assign new highest mean
    };

    for(size_t i = 0; i < prompt.size(); i++) {
        //find match in prompt
        found_match = find_match_in_prompt(i);
        if(found_match){assign_mean_in_prompt(prompt[i]);}

        //find match in table, skip in match found in prompt
        if(!found_match){
            found_match = find_match_in_table(english,0,i) | find_match_in_table(romanji,1,i) | find_match_in_table(japanese,2,i) | find_match_in_table(full_japanese,3,i);
            if(found_match){ assign_mean_in_table(prompt[i]); } //assign meaning if match found in table
        }

        if(!found_match){ //if word not found, assign new max_id
            prompt[i].word_id = max_id++;
            prompt[i].meaning = {0,0,0,0}; //set all meaning to 0 for new words
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

    // Start transaction for better performance and atomicity
    db.transaction();
    bool insert_success = true;

    auto is_redundant = [this](const user& entry, const uint8_t languageIndex, const size_t &currentIndex) -> bool {
        for(size_t itr = 0; itr < currentIndex; itr++) {
            if(entry[languageIndex] == prompt[itr][languageIndex]) {
                return true;
            }
        }

        switch(languageIndex){
        case 0:
            for(const element &e : english){
                if(entry[languageIndex] == e.word){
                    return true;
                }
            }
            break;

        case 1:
            for(const element &e : romanji){
                if(entry[languageIndex] == e.word){
                    return true;
                }
            }
            break;

        case 2:
            for(const element &e : japanese){
                if(entry[languageIndex] == e.word){
                    return true;
                }
            }
            break;

        case 3:
            for(const element &e : full_japanese){
                if(entry[languageIndex] == e.word){
                    return true;
                }
            }
            break;

        default:
            std::cout<<"not langague index in is_redundant() lambda"<<std::endl;
        }

        return false;
    };

    // Helper function to insert into a specific table
    auto insertIntoTable = [&db, &insert_success](const QString& tableName, const user& entry, const int languageIndex) {
        if (entry.prompt[languageIndex].empty()) return false; // Skip empty entries, though none should be empty

        // After preparing the query but before executing it:
        std::cout << "Attempting to insert into " << tableName.toStdString()
                  << " word_id: " << entry.word_id
                  << " type: " << entry.type
                  << " type: " << static_cast<int>(entry.type_id)
                  << " word: " << entry.prompt[languageIndex]
                  << " meaning: " << static_cast<int>(entry.meaning[languageIndex]) << std::endl;

        QSqlQuery query(db);
        query.prepare("INSERT INTO " + tableName + " (word_id, type, type_id, lesson, word, meaning) " "VALUES (?, ?, ?, ?, ?, ?)");
        std::cout<<"INSERT INTO " + tableName.toStdString() + "(word_id, type, type_id, lesson, word, meaning) VALUES ("<<entry.word_id<<","<<entry.type<<","<<static_cast<int>(entry.type_id)<<","<<entry.lesson<<","<<entry.prompt[languageIndex]<<","<<static_cast<int>(entry.meaning[languageIndex])<<")"<<std::endl;

        query.addBindValue(entry.word_id);
        query.addBindValue(QString::fromStdString(entry.type));
        query.addBindValue(entry.type_id);
        query.addBindValue(entry.lesson);
        query.addBindValue(QString::fromStdString(entry.prompt[languageIndex]));
        query.addBindValue(entry.meaning[languageIndex]);

        if (!query.exec()) {
            qDebug() << "Error inserting into" << tableName << ":" << query.lastError().text();
            return false;
        }

        return true;
    };

    // Process each entry
    for (size_t i = 0; i < prompt.size(); ++i) {
        // Insert into each table if the corresponding field is not empty
        if(!is_redundant(prompt[i],0,i)){ //insert only if the word not already included in table
            insert_success = insertIntoTable("english", prompt[i], 0);
            if(!insert_success){ //attempt insertion into english, if fail end the function
                break;
            }
        }

        if(!is_redundant(prompt[i],1,i)){ //insert only if the word not already included in table
            insert_success = insertIntoTable("romanji", prompt[i], 1);
            if(!insert_success){
                break;
            }
        }

        if(!is_redundant(prompt[i],2,i)){ //insert only if the word not already included in table
            insert_success = insertIntoTable("japanese", prompt[i], 2);
            if(!insert_success){ //attempt insertion into english, if fail end the function
                break;
            }
        }

        if(!is_redundant(prompt[i],3,i)){ //insert only if the word not already included in table
            insert_success = insertIntoTable("full_japanese", prompt[i], 3);
            if(!insert_success){ //attempt insertion into english, if fail end the function
                break;
            }
        }
    }

    if(insert_success){
        db.commit();
        std::cout<<"inserting into database went well"<<std::endl;
    }else db.rollback();

    db.close();
    std::cout<<"closed database"<<std::endl;
    return true;
}
