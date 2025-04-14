#include <QCoreApplication>
#include "sqlindexer.h"

int main(int argc, char *argv[]){
    // Create QCoreApplication instance
    QCoreApplication app(argc, argv);
    SQLIndexer sql_indexer;

    std::cout<<"Insert file path: ";
    std::string filepath;
    std::cin>>filepath;
    std::cerr<<"user input: "<<filepath<<" is here"<<std::endl;
    while(!sql_indexer.load_SQL(filepath)){
        std::cout<<"Insert file again: ";
        std::cin>>filepath;
    }

    // Clear input buffer - use ignore instead of clear
    //std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    std::cout<<"Insert command: ";
    std::string input;
    std::cin>>input;
    if(sql_indexer.digest_terminal_input(input)){
        sql_indexer.choose_id();
        sql_indexer.show();
        sql_indexer.write_SQL();
    }

    return app.exec();
}
