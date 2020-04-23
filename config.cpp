#include "config.h"
#include <QFile>
#include <QTextStream>
#include<QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
config::config(QObject *parent) : QObject(parent)
{

}

bool config::init()
{
    QFile f("conf.json");
    f.open(QFile::ReadOnly);
    QTextStream ts(&f);
    auto s = ts.readAll();
    qDebug()<<s;

    QJsonDocument doc = QJsonDocument::fromJson(s.toUtf8());
    if (doc.isNull()){
        qDebug()<<"json parser error.";
        return false;
    }
    QJsonObject obj = doc.object();
    ip = obj["ip"].toString();
    uname = obj["uname"].toString();
    password = obj["password"].toString();
    db = obj["db"].toString();
    port = obj["port"].toInt();
    return true;
}

