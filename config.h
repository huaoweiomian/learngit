#ifndef CONFIG_H
#define CONFIG_H

#include <QObject>

class config : public QObject
{
    Q_OBJECT
public:
    explicit config(QObject *parent = nullptr);
    bool init();
    QString ip, uname, password,db;
    int port;

signals:

};

#endif // CONFIG_H
