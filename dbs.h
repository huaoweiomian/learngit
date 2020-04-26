#ifndef DBS_H
#define DBS_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include "config.h"
class DBS : public QObject
{
    Q_OBJECT
public:
    explicit DBS(QObject *parent = nullptr);
    ~DBS();
    bool init(config& cfg);
    bool insert_usr(QString name,QString pwd);
    bool usr_exist(QString name);
    bool is_admin(QString name);
    bool login(QString name,QString pwd);
    bool auth(QString name, QString change);
    bool history(QString admin,QString name, QVector<QString>& ret);
    bool history_admin(QString admin, QString name, QVector<QString>& ret);
    QSqlDatabase db;
    QSqlQuery q;
signals:

};

#endif // DBS_H
