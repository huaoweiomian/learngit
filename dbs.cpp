#include "dbs.h"
#include <QDebug>
DBS::DBS(QObject *parent) : QObject(parent)
{

}

DBS::~DBS()
{
    db.close();
}

bool DBS::init(config &cfg){
    db = QSqlDatabase::addDatabase("QMYSQL");  //连接的MYSQL的数据库驱动
    db.setHostName(cfg.ip);         //主机名
    db.setPort(cfg.port);                    //端口
    db.setDatabaseName(cfg.db);          //数据库名
    db.setUserName(cfg.uname);              //用户名
    db.setPassword(cfg.password);            //密码

    //测试连接

    if(!db.open())
    {
        qDebug()<<"不能连接"<<"connect to mysql error"<<db.lastError().text();
        return false ;
    }
    else
    {
         qDebug()<<"连接成功"<<"connect to mysql OK";
    }
    q = QSqlQuery(db);
    return true;
}

bool DBS::insert_usr(QString name, QString pwd)
{
    if(usr_exist(name)){
        qDebug()<<"usr name exist.";
        return false;
    }
    q.prepare("INSERT INTO usr (name, pwd, admin) VALUES (?, ?, ?);");
    q.addBindValue(name);
    q.addBindValue(pwd);
    q.addBindValue(0);
    bool ret = q.exec();
    if (!ret){
        qDebug()<<q.lastError();
    }
    return ret;
}

bool DBS::usr_exist(QString name)
{
    if(!q.exec("select name from usr where name = '"+name+"'")){
                 qDebug()<<q.lastError();
                 return false;
    }
    return q.size() != 0;
}

bool DBS::is_admin(QString name)
{
    if(!q.exec("select name from usr where name = '"+name+"' and admin = 1")){
                 qDebug()<<q.lastError();
                 return false;
    }
    return q.size() != 0;
}

bool DBS::login(QString name, QString pwd)
{
    if(!q.exec("select name from usr where name = '"+name+"' and pwd = '"+pwd+"'")){
                 qDebug()<<q.lastError();
                 return false;
    }
    return q.size() != 0;
}

bool DBS::auth(QString name, QString change)
{
    if (!is_admin(name)){
        qDebug()<<"not admin";
        return false;
    }
    q.prepare("update  usr set admin = 1 where name = ?;");
    q.addBindValue(change);
    bool ret = q.exec();
    if (!ret){
        qDebug()<<q.lastError();
    }
    return ret;
}

bool DBS::history(QString admin,QString name, QVector<QString>& ret)
{
    if(!q.exec("select path from history where name = '"+name+"'")){
                 qDebug()<<q.lastError();
                 return false;
    }
    while (q.next()) {
        QString tmp = q.value(0).toString();
        ret.push_back(tmp);
    }
    return true;
}

bool DBS::history_admin(QString admin, QString name, QVector<QString>& ret)
{
    if(!is_admin(admin)){
        return false;
    }
    return history(admin,name, ret);
}
