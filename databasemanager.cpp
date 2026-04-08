#include "databasemanager.h"

#include <QSqlQuery>
#include <QSqlError>

DatabaseManager::DatabaseManager(QObject * parent)
    : QObject(parent)
{
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(m_db_name);

    if (!m_db.open()) {
        qDebug() << "error: Failed to connect database." << m_db.lastError().text();
    }
    else {
        qDebug() << "Database connected successfully!";

        QSqlQuery query;
        QString sql = QString(
                          "CREATE TABLE IF NOT EXISTS %1 ("
                          "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                          "title TEXT NOT NULL, "
                          "author TEXT, "
                          "duration TEXT, "
                          "bvid TEXT UNIQUE NOT NULL, "
                          "remark TEXT, "
                          "lasted_time DATETIME DEFAULT (datetime('now', 'localtime')), " // 使用本地时间
                          "frequency INTEGER DEFAULT 0)"
                          ).arg(m_tb_name);

        if (!query.exec(sql)) {
            qDebug() << "error: Failed to create table." << query.lastError().text();
        }
        else{
            m_model = new MyTableModel(this, m_db);
            m_model->setTable(m_tb_name);
            m_model->setEditStrategy(QSqlTableModel::OnFieldChange);

            m_model->setHeaderData(1, Qt::Horizontal, "标题");
            m_model->setHeaderData(2, Qt::Horizontal, "作者");
            m_model->setHeaderData(3, Qt::Horizontal, "时长");
            m_model->setHeaderData(4, Qt::Horizontal, "来源");
            m_model->setHeaderData(5, Qt::Horizontal, "备注");

            m_model->removeColumn(7);
            m_model->removeColumn(6);

            m_model->select();
        }
    }
}

DatabaseManager::~DatabaseManager()
{
    if (m_db.isOpen()) {
        m_db.close();
    }
    qDebug() << "DatabaseManager: Resources have been safely released.";
}


bool DatabaseManager::insert_data(const QString &title, const QString &author, const QString &duration, const QString &bvid)
{
    QSqlQuery query;
    query.prepare(QString("INSERT INTO %1 (title, author, duration, bvid) "
                          "VALUES (:title, :author, :duration, :bvid)")
                      .arg(m_tb_name));

    query.bindValue(":title", title);
    query.bindValue(":author", author);
    query.bindValue(":duration", duration);
    query.bindValue(":bvid", bvid);

    if (!query.exec()) {
        qDebug() << "插入数据失败:" << query.lastError().text();
        return false;
    } else {
        qDebug() << "数据插入成功，BVID:" << bvid;
        m_model->select();
        return true;
    }
}


void DatabaseManager::delete_data(int id)
{
    QSqlQuery query;
    query.prepare(QString("DELETE FROM %1 WHERE id = :id")
                      .arg(m_tb_name));
    query.bindValue(":id", id);

    if (!query.exec()) {
        qDebug() << "删除数据失败，ID:" << id << "错误:" << query.lastError().text();
    } else {
        qDebug() << "成功删除 ID 为" << id << "的数据";
        m_model->select();
    }
}
