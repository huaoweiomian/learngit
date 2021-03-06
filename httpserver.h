#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <QObject>
#include <QtCore>
#include <QtNetwork>
#include "dbs.h"
class HttpServer : public QObject
{
    Q_OBJECT
public:
    explicit HttpServer(QObject *parent = 0);
public:
    static HttpServer &instance();
    void run(const QHostAddress &address = QHostAddress::Any,const quint16 &port = 8001);
    DBS* pDbs;

    bool parser_json(QByteArray &data, QPair<int, int> &ret, QString& name);
    QByteArray path(QString name, QPair<int, int> &bothendpoints);
    void auth(QByteArray &req, QTcpSocket *socket);
    void history(QByteArray &req, QTcpSocket *socket, bool is_admin);
    void list(QByteArray &req, QTcpSocket *socket);
private slots:
    void newConnection();
    void readyRead();
private:
    void ret_help(QTcpSocket *socket, QByteArray ret);
    bool get_json(QByteArray& req, QByteArray& json);
    ~HttpServer();
    Q_DISABLE_COPY(HttpServer)
private:
    QTcpServer *m_httpServer;
    void signin(QByteArray &req, QTcpSocket *socket);
};


#endif // HTTPSERVER_H
