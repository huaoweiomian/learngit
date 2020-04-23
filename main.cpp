#include <QCoreApplication>
#include <iostream>
#include "httpserver.h"
#include "gtree.h"
#include "config.h"
using namespace std;
int main(int argc, char *argv[]){
    QCoreApplication a(argc, argv);
    config cfg;
    cfg.init();
    //HttpServer::instance().run();
    //build::bmain();
    //query::qmain();
    return a.exec();
}
