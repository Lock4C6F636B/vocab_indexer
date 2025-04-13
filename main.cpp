#include <QCoreApplication>
#include "sqlindexer.h"

int main(int argc, char *argv[]){
    // Create QCoreApplication instance
    QCoreApplication app(argc, argv);
    SQLIndexer sql_indexer;

    std::cout<<"Insert file path: ";
    std::string input;
    std::cin>>input;
    std::cerr<<"user input: "<<input<<" is here"<<std::endl;
    sql_indexer.load_SQL(input);

    // Clear input buffer - use ignore instead of clear
    //std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    std::cout<<"Insert command: ";
    std::cin>>input;
    sql_indexer.digest_terminal_input(input);
    sql_indexer.choose_id();
    sql_indexer.write_SQL();


    return app.exec();
}
