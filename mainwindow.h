#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "databasemanager.h"
#include <QMediaPlayer>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE



class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void init_settings();

    void play_audio(QString bvid, int idx);

    void showMiniMenu(const QPoint &pos);

    enum m_PM {
        ListRepeat,
        SingleRepeat,
        RandomPlay
    };

    bool is_play;

private slots:
    void on_searchLine_returnPressed();

    void on_pushButton_mode_clicked();

    void on_pushButton_play_clicked();

    void on_pushButton_volume_clicked();

    void on_pushButton_prev_clicked();

    void on_pushButton_next_clicked();

    void on_pushButton_setting_clicked();

private:
    DatabaseManager * m_db_manager;

    QMediaPlayer * m_player;
    QAudioOutput * m_audioOutput;

    QString formatTime(qint64 ms) {
        qint64 totalSeconds = ms / 1000;
        int minutes = totalSeconds / 60;
        int seconds = totalSeconds % 60;
        // 使用 arg 填充零，保证显示为 02:05 而不是 2:5
        return QString("%1:%2")
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0'));
    }

    // 注册表参数
    int volume;
    m_PM play_mode;
    int last_play_index;
    qint64 last_play_progress;
    QString audios_dir;


private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
