#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <QObject>
#include <QtCore>
#include <QtNetwork>
class HttpServer : public QObject
{
    Q_OBJECT
public:
    explicit HttpServer(QObject *parent = 0);
public:
    static HttpServer &instance();
    void run(const QHostAddress &address = QHostAddress::Any,const quint16 &port = 8001);


    bool parser_json(QByteArray &data, QPair<int, int> &ret);
    QByteArray path(QPair<int, int> &bothendpoints);
private slots:
    void newConnection();
    void readyRead();
private:
    ~HttpServer();
    Q_DISABLE_COPY(HttpServer)
private:
    QTcpServer *m_httpServer;
};


#endif // HTTPSERVER_H
