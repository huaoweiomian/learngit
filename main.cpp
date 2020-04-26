#include <QCoreApplication>
#include <iostream>
#include "httpserver.h"
#include "gtree.h"
#include "config.h"
#include "dbs.h"
using namespace std;
int main(int argc, char *argv[]){
    QCoreApplication a(argc, argv);
    config cfg;
    cfg.init();
    DBS db;
    db.init(cfg);
    HttpServer& obj = HttpServer::instance();
    obj.pDbs = &db;
    obj.run();
    //build::bmain();
    //query::qmain();
    return a.exec();
}
