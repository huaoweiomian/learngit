#include "dbs.h"
#include <QDebug>
DBS::DBS(QObject *parent) : QObject(parent)
{

}

bool DBS::init(){
    db = QSqlDatabase::addDatabase("QMYSQL");  //连接的MYSQL的数据库驱动
    db.setHostName("localhost");         //主机名
    db.setPort(3306);                    //端口
    db.setDatabaseName("test");          //数据库名
    db.setUserName("root");              //用户名
    db.setPassword("123456");            //密码
    db.open();

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

    db.close();
    return true;
}
