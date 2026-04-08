#ifndef DBMANAGER_H
#define DBMANAGER_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QString>
#include <QDebug>

class sql
{
public:
    sql();
    ~sql();

    // 插入数据：传入标题、作者、时长、BV号
    bool addFavorite(QString title, QString author, QString time, QString bvid);

private:
    void initTable(); // 初始化创建表
    QSqlDatabase db;
};

#endif // DBMANAGER_H
