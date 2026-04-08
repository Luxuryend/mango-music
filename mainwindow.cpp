#include <Python.h>

#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QStandardItemModel>
#include <QTimer>
#include <QHeaderView>
#include <QDir>
#include <QStandardPaths>
#include <QSettings>
#include <QAudioOutput>
#include <QWidgetAction>
#include <QRandomGenerator>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 读取注册表
    init_settings();

    // 播放模式 设置
    switch (play_mode) {
    case ListRepeat:
        ui->pushButton_mode->setText("列表循环");
        break;
    case SingleRepeat:
        ui->pushButton_mode->setText("单曲循环");
        break;
    case RandomPlay:
        ui->pushButton_mode->setText("随机播放");
        break;
    }

    // 播放器初始化
    m_player = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_player->setAudioOutput(m_audioOutput);
    m_audioOutput->setVolume(volume / 100.0f);

    //  python内核启动
    Py_Initialize();
    PyRun_SimpleString("import sys");
    PyRun_SimpleString("sys.path.append(r'E:/2025Pycharm/Projects/crawler')");
    PyRun_SimpleString("sys.path.append(r'E:/2025Pycharm/Projects/crawler/.venv/Lib/site-packages')");

    //  左侧列表与多界面绑定
    connect(ui->listWidget, &QListWidget::currentRowChanged, [=](int index){
        switch (index) {
        case 0: ui->stackedWidget->setCurrentWidget(ui->searchResult);  break;
        case 1: ui->stackedWidget->setCurrentWidget(ui->pageRecommend); break;
        case 2: ui->stackedWidget->setCurrentWidget(ui->pageFavorite);  break;
        case 3: ui->stackedWidget->setCurrentWidget(ui->pageTest);      break;
        }
    });

    // QSqlTableModel * m_model = m_db_manager->get_model();

    // tableViewFavor 右键逻辑 弹出删除按钮
    ui->tableViewFavor->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->tableViewFavor, &QTableView::customContextMenuRequested,this, &MainWindow::showMiniMenu);

    // tableViewFavor 双击逻辑 播放
    connect(ui->tableViewFavor, &QTableView::doubleClicked, this, [=](const QModelIndex &index){
        QSqlTableModel * m_model = m_db_manager->get_model();
        QString bvid = m_model->data(m_model->index(index.row(), 4)).toString();
        play_audio(bvid, index.row());
    });

    // 播放下一首
    connect(m_player, &QMediaPlayer::mediaStatusChanged, this, [=](QMediaPlayer::MediaStatus status) {
        // 当状态变为“媒体播放结束”时
        if (status == QMediaPlayer::EndOfMedia) {
            QSqlTableModel * m_model = m_db_manager->get_model();
            int row;
            QString bvid;
            switch (play_mode) {
            case ListRepeat:
                // 列表循环
                on_pushButton_next_clicked();
                break;
            case SingleRepeat:
                row = ui->tableViewFavor->currentIndex().row();
                bvid = m_model->data(m_model->index(row, 4)).toString();
                play_audio(bvid, row);
                // 单曲循环
                break;
            case RandomPlay:
                // 随机播放
                row = QRandomGenerator::global()->bounded(m_model->rowCount());
                qDebug() << "下一个随机数是" << row;
                bvid = m_model->data(m_model->index(row, 4)).toString();
                play_audio(bvid, row);
                break;
            }
        }
    });

    // tableViewFavor 的展示
    m_db_manager = new DatabaseManager(this);
    ui->tableViewFavor->setModel(m_db_manager->get_model());

    ui->tableViewFavor->hideColumn(0);

    ui->tableViewFavor->setColumnWidth(1, 300);
    ui->tableViewFavor->setColumnWidth(3, 50);
    ui->tableViewFavor->setColumnWidth(4, 120);
    ui->tableViewFavor->setColumnWidth(5, 150);

    ui->tableViewFavor->setSortingEnabled(true);

    // 标签总时长变化（当切换歌曲时触发）
    connect(m_player, &QMediaPlayer::durationChanged, this, [=](qint64 duration) {
        ui->label_TotalTime->setText(formatTime(duration));
    });

    // 标签当前时长 监听当前播放进度变化（播放时每秒触发多次）
    connect(m_player, &QMediaPlayer::positionChanged, this, [=](qint64 pos) {
        ui->label_CurTime->setText(formatTime(pos));
    });

    // 设置进度条的最大值（当歌曲加载完成或时长改变时）
    connect(m_player, &QMediaPlayer::durationChanged, this, [=](qint64 duration) {
        ui->horizontalSlider->setRange(0, duration); // 直接设为总毫秒数
    });

    // 实时更新进度条位置
    connect(m_player, &QMediaPlayer::positionChanged, this, [=](qint64 position) {
        if (!ui->horizontalSlider->isSliderDown()) {
            ui->horizontalSlider->setValue(position);
        }
    });

    // 音乐进度条鼠标拖拽
    connect(ui->horizontalSlider, &QSlider::sliderMoved, this, [=](int position) {
        m_player->setPosition(position);
    });

    // 音乐进度条鼠标点击
    connect(ui->horizontalSlider, &QSlider::sliderPressed, this, [=](){
        int x = ui->horizontalSlider->mapFromGlobal(QCursor::pos()).x();
        double ratio = (double)x / ui->horizontalSlider->width();
        int targetPos = ratio * ui->horizontalSlider->maximum();
        m_player->setPosition(targetPos);
    });
}

MainWindow::~MainWindow()
{
    Py_Finalize();

    delete ui;
}


void MainWindow::init_settings()
{
    QSettings settings;

    volume = settings.value("volume", 50).toInt();

    int modeInt = settings.value("play_mode", ListRepeat).toInt();
    play_mode = static_cast<m_PM>(modeInt);     // 显式类型转换

    last_play_index = settings.value("last_play_index", 0).toInt();

    last_play_progress = settings.value("last_play_progress", 0).toLongLong();

    if (settings.contains("audios_dir")){
        audios_dir = settings.value("audios_dir").toString();
    }
    else{
        QString rootPath;

        if (QDir("D:/").exists()) {         // 检查 D 盘是否存在
            rootPath = "D:/";
        } else {
            rootPath = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
        }

        QDir dir(rootPath);     // 设置根目录
        QString folderName = "m_audios";

        if (dir.mkpath(folderName)) {
            audios_dir = dir.absoluteFilePath(folderName);      // 设置最后的全路径 absoluteFilePath 会自动处理路径分隔符
            settings.setValue("audios_dir", audios_dir);
        } else {
            qDebug() << "mkdir failed";
        }
    }
}


void MainWindow::play_audio(QString bvid, int idx)
{
    is_play = true;
    ui->pushButton_play->setText("暂停");

    QSqlTableModel * m_model = m_db_manager->get_model();
    ui->tableViewFavor->selectRow(idx);
    ui->tableViewFavor->setCurrentIndex(m_model->index(idx, 0));

    QString audio_path = audios_dir + "/" + bvid + ".mp3";
    qDebug() << audio_path;

    m_player->setSource(QUrl::fromLocalFile(audio_path));
    m_player->play();
}


void MainWindow::showMiniMenu(const QPoint &pos)
{
    QModelIndex index = ui->tableViewFavor->indexAt(pos);

    QSqlTableModel * m_model = m_db_manager->get_model();

    // 只有点在有效数据上才弹出（点空白处不弹出）
    if (index.isValid()) {
        QMenu menu(this);

        // 添加删除动作
        QAction * delAction = menu.addAction("删除此项");

        // 弹出菜单并等待点击（exec 会阻塞直到点击或关闭）
        QAction * selectedItem = menu.exec(ui->tableViewFavor->mapToGlobal(pos));

        // 判断用户是否点击了删除
        if (selectedItem == delAction) {
            qDebug() << "触发小菜单删除";
            int songId = m_model->data(m_model->index(index.row(), 0)).toInt();
            m_db_manager->delete_data(songId);
            m_model->select();
        }
    }
}


void MainWindow::on_searchLine_returnPressed()
{
    QString searchText = ui->searchLine->text().trimmed();

    if (searchText.isEmpty()) {
        return;
    }

    PyObject* pModule = PyImport_ImportModule("bilibili");                      // 指定文件
    PyObject* pFunc = PyObject_GetAttrString(pModule, "search_Bilibili");        // 指定函数
    PyObject* pArgs = Py_BuildValue("(s)", searchText.toUtf8().constData());    // 构造参数
    PyObject* pValue = PyObject_CallObject(pFunc, pArgs);                       // 调用并获取返回值

    if (pValue && PyList_Check(pValue)) {
        // 创建模型（建议在类成员中定义，避免重复创建）
        QStandardItemModel* model = new QStandardItemModel(this);
        model->setHorizontalHeaderLabels({"","标题", "作者", "时长", "来源"}); // 设置表头

        // 获取总行数
        Py_ssize_t rowCount = PyList_Size(pValue);
        // 读取二维列表
        for (Py_ssize_t i = 0; i < rowCount; ++i) {
            // 获取每一行（内层列表）
            PyObject* pRow = PyList_GetItem(pValue, i);

            if (pRow && PyList_Check(pRow)) {
                Py_ssize_t colCount = PyList_Size(pRow);
                QList<QStandardItem*> items; // 存放这一行的数据

                items.append(new QStandardItem(""));    // 每一行开始时，先塞一个空 Item 给第 0 列（按钮列）
                for (Py_ssize_t j = 0; j < colCount; ++j) {
                    PyObject* pItem = PyList_GetItem(pRow, j);

                    const char* cellText = "";      // 将 Python 字符串转为 QString
                    if (PyUnicode_Check(pItem)) {
                        cellText = PyUnicode_AsUTF8(pItem);
                    }

                    items.append(new QStandardItem(QString::fromUtf8(cellText)));
                }
                // 将这一行添加到模型
                model->appendRow(items);
            }
        }

        ui->tableViewSearch->setModel(model);   //  绑定到视图

        // 增加左侧按钮
        for (int i = 0; i < model->rowCount(); ++i) {
            QPushButton* btn = new QPushButton("❤️");
            // 强制设置按钮大小，防止它撑大行高
            btn->setFixedSize(24, 24);
            btn->setStyleSheet("QPushButton { border: none; background: transparent; color: gray; }"
                               "QPushButton:hover { color: red; }");

            // 关键：安插在第 0 列
            ui->tableViewSearch->setIndexWidget(model->index(i, 0), btn);

            // 使用 QPersistentModelIndex 防止行号错乱
            QPersistentModelIndex pIndex(model->index(i, 0));
            connect(btn, &QPushButton::clicked, this, [=]() {
                if (!pIndex.isValid()) return;

                int currentRow = pIndex.row();

                // 构造数据并插入数据库
                QString title    = model->item(currentRow, 1)->text();
                QString author   = model->item(currentRow, 2)->text();
                QString duration = model->item(currentRow, 3)->text();
                QString bvid     = model->item(currentRow, 4)->text();

                if (m_db_manager->insert_data(title, author, duration, bvid)) {
                    // 插入成功，按钮变红
                    QPushButton* clickedBtn = qobject_cast<QPushButton*>(sender());
                    clickedBtn->setStyleSheet("QPushButton { border: none; background: transparent; color: red; font-size: 18px; }");

                    qDebug() << "收藏成功： python start running" << title;

                    // 触发 python 下载函数
                    PyObject* pModule = PyImport_ImportModule("bilibili");                      // 指定文件
                    PyObject* pFunc = PyObject_GetAttrString(pModule, "download_audio");        // 指定函数
                    PyObject* pArgs = Py_BuildValue("(ss)", bvid.toUtf8().constData(), audios_dir.toUtf8().constData());    // 构造参数
                    PyObject* pValue = PyObject_CallObject(pFunc, pArgs);                       // 调用并获取返回值
                    Py_DECREF(pValue);

                    qDebug() << "python下载成功:" << audios_dir << title ;

                } else {
                    // 如果返回 false，通常是因为 bvid 重复（UNIQUE 约束生效）
                    qDebug() << "该视频已在收藏列表中";
                }
            });
        }


        // 关键：延迟获取真实宽度并设置
        QTimer::singleShot(0, this, [=]() {
            int totalWidth = ui->tableViewSearch->viewport()->width();

            // 第 0 列（收藏）：固定宽度
            ui->tableViewSearch->setColumnWidth(0, 40);
            // 第 1 列（标题）：占据剩余的一半
            ui->tableViewSearch->setColumnWidth(1, totalWidth / 2);

            ui->tableViewSearch->horizontalHeader()->setStretchLastSection(true);
        });


        Py_DECREF(pValue);   // 释放 pValue
    }

    ui->stackedWidget->setCurrentWidget(ui->searchResult);
}


void MainWindow::on_pushButton_mode_clicked()
{
    switch (play_mode) {
    case ListRepeat:
        play_mode = SingleRepeat;
        ui->pushButton_mode->setText("单曲循环");
        break;
    case SingleRepeat:
        play_mode = RandomPlay;
        ui->pushButton_mode->setText("随机播放");
        break;
    case RandomPlay:
        play_mode = ListRepeat;
        ui->pushButton_mode->setText("列表循环");
        break;
    }
}


void MainWindow::on_pushButton_play_clicked()
{
    if (is_play){
        is_play = false;
        ui->pushButton_play->setText("播放");
        m_player->pause();
    }else{
        is_play = true;
        ui->pushButton_play->setText("暂停");
        m_player->play();
    }
}


void MainWindow::on_pushButton_volume_clicked()
{
    QMenu *menu = new QMenu(this);
    QWidgetAction * action = new QWidgetAction(menu);

    // 创建一个垂直进度条
    QSlider *slider = new QSlider(Qt::Vertical);
    slider->setRange(0, 100);
    slider->setValue(volume); // 同步当前音量
    slider->setFixedHeight(100); // 设置高度

    // 连接音量改变信号
    connect(slider, &QSlider::valueChanged, this, [this](int value){
        m_audioOutput->setVolume(value / 100.0);

        QSettings settings;
        settings.setValue("volume", volume);
    });

    action->setDefaultWidget(slider);
    menu->addAction(action);

    // 在按钮上方弹出
    QPoint pos = ui->pushButton_volume->mapToGlobal(QPoint(0, -slider->height()));
    menu->exec(pos);
}


void MainWindow::on_pushButton_prev_clicked()
{
    int currentRow = ui->tableViewFavor->currentIndex().row();
    int prevRow = currentRow - 1;
    QSqlTableModel * m_model = m_db_manager->get_model();
    if (prevRow <= -1){
        prevRow = m_model->rowCount() - 1;
    }

    QString bvid = m_model->data(m_model->index(prevRow, 4)).toString();
    play_audio(bvid, prevRow);
}


void MainWindow::on_pushButton_next_clicked()
{
    int currentRow = ui->tableViewFavor->currentIndex().row();
    int nextRow = currentRow + 1;
    QSqlTableModel * m_model = m_db_manager->get_model();
    if (nextRow >= m_model->rowCount()){
        nextRow = 0;
    }

    QString bvid = m_model->data(m_model->index(nextRow, 4)).toString();
    play_audio(bvid, nextRow);
}


void MainWindow::on_pushButton_setting_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->pageTest);
}

