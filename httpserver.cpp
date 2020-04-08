#include "httpserver.h"


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

void HttpServer::readyRead()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    QByteArray request = socket->readAll();
    qDebug() << "Request Data:" << request;
    QPair<int,int> parameter;
    if (!parser_json(request,parameter)){
        qDebug()<<"parameter error.";
        socket->write("parameter error.");
        return;
    }
    static int count = 0;
    count++;
    QByteArray response = QString("<h1><center>Hello World %1</center></h1>\r\n").arg(count).toUtf8();


    QString http = "HTTP/1.1 200 OK\r\n";
    http += "Server: nginx\r\n";
    http += "Content-Type: text/html;charset=utf-8\r\n";
    http += "Connection: keep-alive\r\n";
    http += QString("Content-Length: %1\r\n\r\n").arg(QString::number(response.size()));
    http = "[1,2,3,4]";
    socket->write(path(parameter));
    //socket->write(response);
    socket->flush();
    //socket->waitForBytesWritten(http.size() + response.size());
    socket->close();
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
    QJsonArray ar = {bothendpoints.first,2,4,5,6,bothendpoints.second};
    QJsonObject obj;
    obj.insert("path", ar);
    QJsonDocument doc;
    doc.setObject(obj);
    qDebug()<<doc.toJson();
    return doc.toJson();
}
