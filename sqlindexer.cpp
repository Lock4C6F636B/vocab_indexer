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
    filepath = "/home/lock/programming/c++/HiKa_linux/sql/vocabulary.db";
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
        filepath = "/home/lock/programming/c++/vocab_indexer/sample.txt";
        //std::getline(std::cin, filepath);


        std::cerr<<"here is what is input into prompt filepath: "<<filepath<<std::endl;

        std::ifstream prompt_file(filepath);
        while(!prompt_file) {
            std::cout << "try again: ";
            std::getline(std::cin, filepath);
        }

        std::string line;
        while (std::getline(prompt_file, line)) {
            if (!line.empty()){
                prompt += line;
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
       //write_SQL();
    }


    return true;
}

void SQLIndexer::stripper(std::string &input) noexcept {
    std::array<char,5> terminators = {';','|','#',',','~'};
    for(size_t i = 0; i < input.size();){
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

    std::cout<<'\n'<<'\n'<<input<<'\n'<<std::endl; //just debug

    return;
}

bool SQLIndexer::digest_input(std::string &input) noexcept {
    if (input.empty() || input.back() != '~') {
        std::cerr << "Command not ended with ~ terminator... bad input, you shall not pass" << std::endl;
        return false;
    }

    stripper(input); //better for function to call stripper by itself
    std::cout<<"stripped input command is: "<<input<<std::endl; //debug after stripper

    //cut input into separate lines
    std::vector<std::string> lines;
    for(size_t i = 0; i < input.size();i++){
        if(input[i] == '~'){
            lines.emplace_back(input.substr(0,i+1));
            input = input.substr(i+1);
            i = 0;
        }
    }

    for(auto item: lines){
        std::cout<<"here is line: "<<item<<std::endl;
    }

    //set variables for digesting input
    std::array<char,5> terminators = {';','|','#',',','~'};

    for(std::string &line : lines){
        //uint8_t current_index = 0, current_metadata = 0;
        size_t last_terminator = 0, next_terminator = 0;
        size_t index = 0;  // Reset language index (0=english, 1=romanji, etc.) and metadata, 0-3 + 3 (type,type_id,lesson)

        // Temporary storage for current word parts
        std::array<std::vector<std::string>, 4> words;
        uint8_t type_id;
        std::string type;
        unsigned int lesson;

        for(size_t i = 0; i < line.size(); i++ ){
            if(std::find(terminators.begin(), terminators.end(), line[i]) != terminators.end()){ //firstly find terminator
                if(index == 0){ //first word of command must always be pushed in
                    words[index].push_back(line.substr(last_terminator, i-last_terminator)); //take current
                    //std::cout<<"here is first word: "<<words[index][words.size()-1]<<std::endl; //debug
                }

                //handle individul terminator cases
                switch(line[i]){
                case '|': {
                    next_terminator = line.find_first_of(";|#~\n", i+1); //find next terminator to determine length
                    words[index].push_back(line.substr(i+1,next_terminator-i-1)); //store next word in on same index

                    last_terminator = i; //save position of current terminator

                    //std::cout<<"here is copy(|) word: "<<words[index][words.size()-1]<<std::endl; //debug
                    break;
                }
                case ';': {
                    index++; //increment to new
                    next_terminator = line.find_first_of(";|#~\n", i+1); //find next terminator to determine length, +1 is important to not much current terminator
                    words[index].push_back(line.substr(i+1,next_terminator-i-1)); //store next word

                    last_terminator = i; //save position of current terminator

                    //std::cout<<"here is next (;) word: "<<words[index][words.size()-1]<<next_terminator<<std::endl;
                    break;
                }
                case '#': { //indicates ending words and start of metadata
                    last_terminator = i; //save position of current terminator
                    break;
                }
                case ',':{ //separator of metadata
                    index++;
                    //can consider data belong to latest word
                    std::cout<<"metadata: "<<line.substr(last_terminator+1, i-last_terminator-1)<<" ,index is:"<<index<<std::endl;
                    switch(index){
                    case 4:
                        type_id = std::stoi(line.substr(last_terminator+1, i-last_terminator-1));
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
                    std::cout<<"metadata: "<<line.substr(last_terminator+1, i-last_terminator-1)<<" ,index is:"<<index<<std::endl;

                    for (size_t i0 = 0; i0 < words[0].size(); i0++) {
                        for (size_t i1 = 0; i1 < words[1].size(); i1++) {
                            for (size_t i2 = 0; i2 < words[2].size(); i2++) {
                                for (size_t i3 = 0; i3 < words[3].size(); i3++) {
                                    prompt.emplace_back(user(std::array<std::string, 4>{words[0][i0], words[1][i1], words[2][i2], words[3][i3]},type_id, type, lesson));
                                }
                            }
                        }
                    }
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
    uint8_t found_match;
    unsigned int max_id = 0;

    //find max word_id in existance
    for (const auto& e : english) max_id = std::max(max_id, e.word_id);
    for (const auto& e : romanji) max_id = std::max(max_id, e.word_id);
    for (const auto& e : japanese) max_id = std::max(max_id, e.word_id);
    for (const auto& e : full_japanese) max_id = std::max(max_id, e.word_id);


    auto find_match= [this](const std::vector<element> &table, const uint8_t language) -> bool{ //find if and where word occures
        for(const element e : table){
            for(user &word : prompt){
                if(word[language] == e.word){
                    word.word_id = e.word_id; //assign found id
                    word.assign_mean(language, e.meaning); //increment meaning for this table and assign
                    return true;
                }
            }
        }

        return false; //overflow is going to be considered "no match found"
    };


    for(user &word : prompt) {
        found_match = find_match(english,0) | find_match(romanji,1) | find_match(japanese,2) | find_match(full_japanese,3);

        if(!found_match){ //if word not found, assign new max_id
            word.word_id = max_id++;
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

    std::cerr<<"passed transaction"<<std::endl;

    // Helper function to insert into a specific table
    auto insertIntoTable = [&db, &insert_success](const QString& tableName, const user& entry, int languageIndex) {
        if (entry.prompt[languageIndex].empty()) return false; // Skip empty entries, though none should be empty

        QSqlQuery query(db);
        query.prepare("INSERT INTO " + tableName + " (word_id, type, type_id, lesson, word, meaning) "
                                                   "VALUES (?, ?, ?, ?, ?, ?)");
        query.addBindValue(entry.word_id);
        query.addBindValue(QString::fromStdString(entry.type));
        query.addBindValue(entry.type_id);
        query.addBindValue(entry.lesson);
        query.addBindValue(QString::fromStdString(entry.prompt[languageIndex]));
        query.addBindValue(entry.meaning[languageIndex]);

        if (!query.exec()) {
            qDebug() << "Error inserting into" << tableName << ":" << query.lastError().text();
            insert_success = false;
            return false;
        }

        insert_success = true;
    };

    // Process each entry
    for (const auto& entry : prompt) {
        // Insert into each table if the corresponding field is not empty
        insertIntoTable("english", entry, 0);
        if(!insert_success){ //attempt insertion into english, if fail end the function
            break;
        }

        insertIntoTable("romanji", entry, 1);
        if(!insert_success){
            break;
        }

        insertIntoTable("japanese", entry, 2);
        if(!insert_success){ //attempt insertion into english, if fail end the function
            break;
        }

        insertIntoTable("full_japanese", entry, 3);
        if(!insert_success){ //attempt insertion into english, if fail end the function
            break;
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
