#include <QCoreApplication>
#include "sqlindexer.h"

int main(int argc, char *argv[]){
    // Create QCoreApplication instance
    QCoreApplication app(argc, argv);
    SQLIndexer sql_indexer;

    sql_indexer.process();

    return app.exec();
}
