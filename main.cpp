#include <QCoreApplication>
#include "sqlindexer.h"

int main(int argc, char *argv[]){
    // Create QCoreApplication instance
    QCoreApplication app(argc, argv);
    SQLIndexer sql_indexer;

    if(sql_indexer.process()){
        std::cout<<"process went well"<<std::endl;
    } else std::cout<<"FOOOL!!"<<std::endl;

    return app.exec();
}
