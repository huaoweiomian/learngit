#include "httpserver.h"
#include "gtree.h"

HttpServer &HttpServer::instance()
{
    static HttpServer obj;
    return obj;
}

void HttpServer::run(const QHostAddress &address, const quint16 &port)
{
    m_httpServer->listen(address,port);
}

void HttpServer::newConnection()
{
    qDebug() << "newConnection";
    QTcpSocket *m_socket = m_httpServer->nextPendingConnection();
    QObject::connect(m_socket,&QTcpSocket::readyRead,this,&HttpServer::readyRead);
}
void HttpServer::auth(QByteArray& req,QTcpSocket *socket){
    const QByteArray f = "{\"status\":1}";
    const QByteArray s = "{\"status\":0}";
    QByteArray json;
    if (!get_json(req, json)){
        ret_help(socket, f);
        return;
    }
    //获取参数
    QJsonParseError pe;
    QJsonDocument doc = QJsonDocument::fromJson(json,&pe);
    if (pe.error != QJsonParseError::NoError){
        ret_help(socket, f);
        qDebug()<<"parer error.";
        return;
    }
    QJsonObject obj = doc.object();
    qDebug()<<obj["name"].toString() << "   " <<obj["change"].toString();
    QString name = obj["name"].toString();
    QString change = obj["change"].toString();
    //修改普通用户权限
    if (pDbs->auth(name,change)){
        ret_help(socket,s);
        return;
    }
    ret_help(socket,f);
}

void HttpServer::history(QByteArray &req, QTcpSocket *socket, bool is_admin)
{
    //默认普通用户查询历史
    auto func = &DBS::history;
    if(is_admin){//如果是管理员，就切换成管理查询历史
        func = &DBS::history_admin;
    }
    const QByteArray f = "{\"history\":[]}";
    //解析查询参数
    QByteArray json;
    if (!get_json(req, json)){
        ret_help(socket, f);
        return;
    }

    QJsonParseError pe;
    QJsonDocument doc = QJsonDocument::fromJson(json,&pe);
    if (pe.error != QJsonParseError::NoError){
        ret_help(socket, f);
        qDebug()<<"parer error.";
        return;
    }
    QJsonObject obj = doc.object();
    qDebug()<<obj["name"].toString() ;
    QString name = obj["name"].toString();
    QString admin = obj["admin"].toString();

    QVector<QString> ret;
    //查询
    if (!(pDbs->*func)(admin, name, ret)){
        ret_help(socket, f);
        return;
    }
    //组织返回值
    QJsonArray ar;
    for (auto v:ret){
        QJsonValue tmp(v);
        ar.push_back(tmp);
    }

    obj = QJsonObject();
    QJsonValue v(ar);
    obj["history"] = v;
    doc = QJsonDocument(obj);
    ret_help(socket,doc.toJson());
}
void HttpServer::list(QByteArray& req,QTcpSocket *socket){

    const QByteArray f = "{\"names\":[]}";
    QJsonObject ret;
    //查询
    if (!(pDbs->list(ret))){
        ret_help(socket, f);
        return;
    }
    //组织返回值
    QJsonArray ar;
    for (auto v:ret){
        QJsonValue tmp(v);
        ar.push_back(tmp);
    }

    QJsonObject obj = QJsonObject();
    QJsonValue v(ar);
    obj["names"] = ret;
    QJsonDocument doc = QJsonDocument(obj);
    ret_help(socket,doc.toJson());
}
void HttpServer::signin(QByteArray& req,QTcpSocket *socket){
    const QByteArray f = "{\"status\":1}";
    QString s = "{\"status\":0 \"admin\":%1}";
    QByteArray json;
    //解析登陆或注册参数
    if (!get_json(req, json)){
        socket->write(f);
        socket->flush();
        socket->waitForBytesWritten(f.size());
        socket->close();
        return;
    }

    QJsonParseError pe;
    QJsonDocument doc = QJsonDocument::fromJson(json,&pe);
    if (pe.error != QJsonParseError::NoError){
        socket->write(f);
        socket->flush();
        socket->waitForBytesWritten(f.size());
        socket->close();
        qDebug()<<"parer error.";
        return;
    }
    QJsonObject obj = doc.object();
    qDebug()<<obj["name"].toString() << "   " <<obj["pwd"].toString()<<"   "<<obj["type"].toInt();
    QString name = obj["name"].toString();
    QString pwd = obj["pwd"].toString();
    int type = obj["type"].toInt();
    //注册
    int admin(0);
    if (type == 1){//sign up
        if(!pDbs->insert_usr(name,pwd)){
            ret_help (socket,f);
            return;
        }
    }else{//登陆

        if(!pDbs->login(name,pwd, admin)){
            ret_help (socket,f);
            return;
        }
    }
    s = s.arg(admin);
    ret_help(socket,s.toUtf8());
}
void HttpServer::readyRead()
{
    QByteArray signin = "/signin";
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    QByteArray request = socket->readAll();
    //登陆或注册服务
    if (-1 != request.indexOf(signin)){
        this->signin(request,socket);
        return;
    }
    //管理员查询历史
    if (-1 != request.indexOf("/historyadmin")){
        this->history(request,socket, true);
        return;
    }
    //普通用户查询历史
    if (-1 != request.indexOf("/history")){
        this->history(request,socket, false);
        return;
    }
    //普通用户变成管理员
    if (-1 != request.indexOf("/auth")){
        this->auth(request,socket);
        return;
    }
    //用户列表
    if (-1 != request.indexOf("/list")){
        this->list(request,socket);
        return;
    }
    //路径查询
    qDebug() << "Request Data:" << request;
    QPair<int,int> parameter;
    QString name;
    //解析路径查询参数
    if (!parser_json(request,parameter,name)){
        qDebug()<<"parameter error.";
        socket->write("parameter error.");
        return;
    }
    QByteArray response;
    //根据参数调用查询算法进行查询
    response = path(name,parameter);
    ret_help(socket,response);
}

void HttpServer::ret_help(QTcpSocket *socket, QByteArray ret)
{
    QString http = "HTTP/1.1 200 OK\r\n";
    http += "Server: qtsvr\r\n";
    http += "Content-Type: application/json;charset=utf-8\r\n";
    http += "Connection: keep-alive\r\n";
    http += QString("Content-Length: %1\r\n\r\n").arg(ret.size());

    socket->write(http.toUtf8()+ret);
    socket->flush();
    socket->waitForBytesWritten(ret.size());
    socket->close();
}

bool HttpServer::get_json(QByteArray &req, QByteArray &json)
{
    int pos = req.indexOf('{');
    if (pos == -1){
        return false;
    }
    json = req.right(req.length()-pos);
    qDebug()<<json;
    return true;
}

HttpServer::HttpServer(QObject *parent) : QObject(parent)
{
    m_httpServer = new QTcpServer(this);
    m_httpServer->setMaxPendingConnections(1024);//设置最大允许连接数
    QObject::connect(m_httpServer,&QTcpServer::newConnection,this,&HttpServer::newConnection);
}

HttpServer::~HttpServer()
{

}

bool HttpServer::parser_json(QByteArray& data, QPair<int, int>& ret,QString& name){
    QString tmp = data;
    int pos = tmp.indexOf("{");
    if (pos == -1){
        return false;
    }
    tmp = tmp.right(tmp.length()-pos);
    qDebug()<<tmp;
    QJsonParseError pe;
    QJsonDocument doc = QJsonDocument::fromJson(tmp.toUtf8(),&pe);
    if (pe.error != QJsonParseError::NoError){
        qDebug()<<"parer error.";
        return false;
    }
    QJsonObject obj = doc.object();
    qDebug()<<obj["name"].toString()<<" "<<obj["start"].toInt() << "   " <<obj["end"].toInt();
    ret.first = obj["start"].toInt();
    ret.second = obj["end"].toInt();
    name = obj["name"].toString();
    if(name.isEmpty())
        return false;
    return true;
}

QByteArray HttpServer::path(QString name,QPair<int, int>& bothendpoints){
    vector<int> paths = query::search(bothendpoints.first,bothendpoints.second);
    QJsonArray ar;
    QString pathstr;
    bool flag = true;
    for (auto v : paths){
        QJsonValue jv(v);
        ar.append(jv);
        QString tmp;
        tmp.setNum(v);
        if(flag){
            flag = false;
            pathstr += tmp;
        }else{
            pathstr = pathstr + "," + tmp;
        }
    }

    QJsonObject obj;
    obj.insert("path", ar);
    QJsonDocument doc;
    doc.setObject(obj);
    qDebug()<<doc.toJson();
    pDbs->insert_path(name,pathstr);
    return doc.toJson();
}

