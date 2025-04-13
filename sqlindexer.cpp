#include "sqlindexer.h"

bool SQLIndexer::load_SQL(const std::string filename){
    //wipe existing vectorsoff the face of earth
    english.clear();
    romanji.clear();
    japanese.clear();
    full_japanese.clear();

    // Set up database connection
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(QString::fromStdString(filename) ); // Replace with your actual database file path

    if (!db.open()) { //throw exception if database could not open
        qDebug() << "Error opening database:" << db.lastError().text();
        return false;
    }

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


    db.close();
    return success;
}


void SQLIndexer::digest_terminal_input(const std::string input){
    uint8_t arr_index = 0, current_metadata = 0;
    size_t last_terminator = 0, next_terminator = 0;
    std::array<std::string, 4> word;
    std::array<std::string, 4> word_copy;

    for(size_t i = 0; i < input.size() - 1; i++ ){
        if(input[i] == '|' || input[i] == ';' || input[i] == '~'){ //firstly find terminator
            if(arr_index == 0){ //first word of command must always be pushed in
                word[arr_index] = input.substr(last_terminator, i-last_terminator); //take current word
            }

            switch(input[i]){
            case '|': {
                next_terminator = input.find_first_of(";|~\n", i+1); //find next terminator to determine length
                word_copy = word; //take already existing words
                word_copy[arr_index] = input.substr(i+1,next_terminator-i); //store next word in copy

                last_terminator = i; //save position of current terminator
                break;
            }
            case ';': {
                next_terminator = input.find_first_of(";|~\n", i+1); //find next terminator to determine length, +1 is important to not much current terminator
                word[arr_index] = input.substr(i+1,next_terminator-i); //store next word

                bool copy_used = std::all_of(word_copy.begin(), word_copy.end(),[](const std::string& s) { return s.empty(); }); //check if word_copy is used in this cycle
                if(copy_used){
                    word_copy[arr_index] = input.substr(i+1,next_terminator-i); //if copy is initialized, need to store in copy as well
                }

                last_terminator = i; //save position of current terminator
                break;
            }
            case '#': { //indicates ending words and start of metadata
                prompt.push_back(user(word));

                bool copy_used = std::all_of(word_copy.begin(), word_copy.end(),[](const std::string& s) { return s.empty(); }); //check if word_copy is used in this cycle
                if(copy_used){ //
                    prompt.push_back(user(word_copy));
                }

                last_terminator = i; //save position of current terminator
                break;
            }
            case ',':{ //separator of metadata
                //can consider data belong to latest word
                switch(current_metadata){
                case 0:
                    prompt[prompt.size()-1].type_id = std::stoi(input.substr(last_terminator, i-last_terminator));
                    break;
                case 1:
                    prompt[prompt.size()-1].type = input.substr(last_terminator, i-last_terminator);
                    break;
                case 2:
                    prompt[prompt.size()-1].lesson = std::stoi(input.substr(last_terminator, i-last_terminator));
                    break;
                }

                current_metadata++;
                last_terminator = i; //save position of current terminator
                break;
            }
            case '~':{ //end of line terminator
                //reset values indicating current index
                current_metadata = 0;

                arr_index = 0; //reset index
                word_copy = {}; //reset copy
            }
            default: continue; //ignore if not terminator
            }

            last_terminator = i;
        }
        }

}

unsigned int SQLIndexer::choose_id(){
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
}


bool SQLIndexer::write_SQL(const std::string& filename, const std::string& type) {
    /*
    // Set up database connection
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(QString::fromStdString(filename));

    if (!db.open()) {
        qDebug() << "Error opening database:" << db.lastError().text();
        return false;
    }

    // Start transaction for better performance and atomicity
    db.transaction();
    bool success = true;

    // Helper function to insert into a specific table
    auto insertIntoTable = [&db, &type, &success](const QString& tableName, const user& entry, int languageIndex) {
        if (entry.prompt[languageIndex].empty()) return; // Skip empty entries

        QSqlQuery query(db);
        query.prepare("INSERT INTO " + tableName + " (word_id, type, type_id, lesson, word, meaning) "
                                                   "VALUES (?, ?, ?, ?, ?, ?)");
        query.addBindValue(entry.word_id);
        query.addBindValue(QString::fromStdString(type));
        query.addBindValue(entry.type_id);
        query.addBindValue(entry.lesson);
        query.addBindValue(QString::fromStdString(entry.prompt[languageIndex]));
        query.addBindValue(entry.meaning[languageIndex]);

        if (!query.exec()) {
            qDebug() << "Error inserting into" << tableName << ":" << query.lastError().text();
            success = false;
        }
    };

    // Process each entry
    for (const auto& entry : prompt) {
        // Insert into each table if the corresponding field is not empty
        insertIntoTable("english", entry, 0);
        if (!success) break;

        insertIntoTable("romanji", entry, 1);
        if (!success) break;

        insertIntoTable("japanese", entry, 2);
        if (!success) break;

        insertIntoTable("full_japanese", entry, 3);
        if (!success) break;
    }

    // Commit or rollback based on success
    if (success) {
        db.commit();
    } else {
        db.rollback();
    }

    db.close();
    return success;
*/
    return true;
}
