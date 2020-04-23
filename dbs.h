#ifndef DBS_H
#define DBS_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

class DBS : public QObject
{
    Q_OBJECT
public:
    explicit DBS(QObject *parent = nullptr);

    bool init();
    QSqlDatabase db;
signals:

};

#endif // DBS_H
