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
    if (pDbs->auth(name,change)){
        ret_help(socket,s);
        return;
    }
    ret_help(socket,f);
}

void HttpServer::history(QByteArray &req, QTcpSocket *socket, bool is_admin)
{
    auto func = &DBS::history;
    if(is_admin){
        func = &DBS::history_admin;
    }
    const QByteArray f = "{\"history\":[]}";
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
    if (!(pDbs->*func)(admin, name, ret)){
        ret_help(socket, f);
        return;
    }
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
void HttpServer::signin(QByteArray& req,QTcpSocket *socket){
    const QByteArray f = "{\"status\":1}";
    const QByteArray s = "{\"status\":0}";
    QByteArray json;
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
    if (type == 1){//sign up
        if(!pDbs->insert_usr(name,pwd)){
            socket->write(f);
            socket->flush();
            socket->waitForBytesWritten(f.size());
            socket->close();
            return;
        }
    }else{
        if(!pDbs->login(name,pwd)){
            socket->write(f);
            socket->flush();
            socket->waitForBytesWritten(f.size());
            socket->close();
            return;
        }
    }

    socket->write(s);
    socket->flush();
    socket->waitForBytesWritten(s.size());
    socket->close();
}
void HttpServer::readyRead()
{
    QByteArray signin = "/signin";
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    QByteArray request = socket->readAll();
    if (-1 != request.indexOf(signin)){
        this->signin(request,socket);
        return;
    }
    if (-1 != request.indexOf("/historyadmin")){
        this->history(request,socket, true);
        return;
    }
    if (-1 != request.indexOf("/history")){
        this->history(request,socket, false);
        return;
    }
    if (-1 != request.indexOf("/auth")){
        this->auth(request,socket);
        return;
    }
    qDebug() << "Request Data:" << request;
    QPair<int,int> parameter;
    if (!parser_json(request,parameter)){
        qDebug()<<"parameter error.";
        socket->write("parameter error.");
        return;
    }
    static int count = 0;
    count++;
    QByteArray response;
    QString http = "HTTP/1.1 200 OK\r\n";
    http += "Content-Type: application/json\r\n";
    response = path(parameter);
    socket->write( response);
    socket->flush();
    socket->waitForBytesWritten(response.size());
    socket->close();
}

void HttpServer::ret_help(QTcpSocket *socket, QByteArray ret)
{
    socket->write(ret);
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

bool HttpServer::parser_json(QByteArray& data, QPair<int, int>& ret){
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
    qDebug()<<obj["start"].toInt() << "   " <<obj["end"].toInt();
    ret.first = obj["start"].toInt();
    ret.second = obj["end"].toInt();
    return true;
}

QByteArray HttpServer::path(QPair<int, int>& bothendpoints){
    vector<int> paths = query::search(bothendpoints.first,bothendpoints.second);
    QJsonArray ar;
    for (auto v : paths){
        QJsonValue jv(v);
        ar.append(jv);
    }
    QJsonObject obj;
    obj.insert("path", ar);
    QJsonDocument doc;
    doc.setObject(obj);
    qDebug()<<doc.toJson();
    return doc.toJson();
}

