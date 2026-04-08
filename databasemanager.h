#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QString>
#include <QSqlDatabase>
#include <QSqlTableModel>

class DatabaseManager : public QObject
{
    Q_OBJECT    // 一堆函数
public:
    explicit DatabaseManager(QObject * parent = nullptr);   // 防止隐式类型转换
    ~DatabaseManager();

    bool insert_data(const QString& title, const QString& author, const QString& duration, const QString& bvid);
    void delete_data(int id);

    QSqlTableModel * get_model() const { return m_model; }  // 不可修改


private:
    QSqlDatabase m_db;
    QString m_db_name = "music.db";
    QString m_tb_name = "music";

    QSqlTableModel * m_model;    // 视图
};


class MyTableModel : public QSqlTableModel {
public:
    using QSqlTableModel::QSqlTableModel;

    Qt::ItemFlags flags(const QModelIndex &index) const override {
        // 首先获取基类默认的 flags
        Qt::ItemFlags f = QSqlTableModel::flags(index);

        // 假设 4 是 bvid 列的索引
        if (index.column() != 5) {
            // 如果是 bvid 列，强制移除“可编辑”标志
            f &= ~Qt::ItemIsEditable;
        } else {
            // 其他列（title, author 等）确保它们具有“可编辑”标志
            // 注意：某些列（如 ID）如果也是自动生成的，建议也加上判断
            f |= Qt::ItemIsEditable;
        }

        return f;
    }
};

#endif // DATABASEMANAGER_H
