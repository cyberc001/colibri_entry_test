#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QComboBox>
#include <QSpinBox>
#include <QProgressBar>

#include "worker_thread.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);

private:
    virtual void closeEvent(QCloseEvent*);

    QProgressBar* _bar_progress = new QProgressBar;
    QLabel* _lbl_status = new QLabel;

    QPushButton* _butn_start_pause = new QPushButton(tr("Старт"));

    QLineEdit* _tbox_in_dir = new QLineEdit("C:/Users/cyb3rc001/Projects/test/");
    QPushButton* _butn_browse_in = new QPushButton(tr("Обзор"));
    QLineEdit* _tbox_in_filter = new QLineEdit("*.*");
    QCheckBox* _chk_repeat = new QCheckBox(tr("Периодически опрашивать входной каталог"));
    QSpinBox* _spin_timer = new QSpinBox;
    QCheckBox* _chk_delete_input = new QCheckBox(tr("Удалять входные файлы"));

    QLineEdit* _tbox_out_dir = new QLineEdit("C:/Users/cyb3rc001/Projects/test_out/");
    QPushButton* _butn_browse_out = new QPushButton(tr("Обзор"));
    QComboBox* _cbox_dup_action = new QComboBox;

    QLineEdit* _tbox_xor_val = new QLineEdit("1234567890ABCDEF");
    QLabel* _lbl_xor_err = new QLabel;

    void connectSignals();

    WorkerThread* _thr;

private slots:
    void onStartPause();
    void onBrowse();

    void onInDirChanged();
    void onInFilterChanged();
    void onOutDirChanged();
    void onIsRepeatingChanged();
    void onRepeatDelayChanged(int);
    void onDeleteInputFilesChanged();
    void onXORValueChanged();
    void onDupActionChanged(int);

    void onThreadPaused();
    void onThreadStatusChanged(QString);
    void onThreadProgressChanged(float progress);
};

#endif // MAINWINDOW_H
