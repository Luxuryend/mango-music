#include "dbManager.h"

sql::sql()
{
    // 连接数据库
    if (QSqlDatabase::contains("qt_sql_default_connection")) {
        db = QSqlDatabase::database("qt_sql_default_connection");
    } else {
        db = QSqlDatabase::addDatabase("QSQLITE");
        db.setDatabaseName("music.db");
    }

    if (db.open()) {
        qDebug() << "数据库连接成功！";
        initTable();
    } else {
        qDebug() << "数据库打开失败：" << db.lastError().text();
    }
}

sql::~sql() { db.close(); }

void sql::initTable()
{
    QSqlQuery query;
    // 创建收藏表
    QString str = "CREATE TABLE IF NOT EXISTS favorites ("
                  "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                  "title TEXT, author TEXT, time TEXT, bvid TEXT UNIQUE)";
    query.exec(str);
}

bool sql::addFavorite(QString title, QString author, QString time, QString bvid)
{
    QSqlQuery query;
    query.prepare("INSERT OR IGNORE INTO favorites (title, author, time, bvid) "
                  "VALUES (:title, :author, :time, :bvid)");
    query.bindValue(":title", title);
    query.bindValue(":author", author);
    query.bindValue(":time", time);
    query.bindValue(":bvid", bvid);

    return query.exec();
}
