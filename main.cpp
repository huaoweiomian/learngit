#include <QCoreApplication>
#include <iostream>
#include "httpserver.h"
using namespace std;
int main(int argc, char *argv[]){
    QCoreApplication a(argc, argv);
    HttpServer::instance().run();
    return a.exec();
}
