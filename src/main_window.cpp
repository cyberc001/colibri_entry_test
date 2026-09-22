#include "main_window.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <climits>

MainWindow::MainWindow(QWidget *parent):
    QMainWindow(parent),
    _thr(new WorkerThread{this})
{
    QWidget* center_wgt = new QWidget;
    QVBoxLayout* lay_main = new QVBoxLayout(center_wgt);

    _bar_progress->setRange(0, 100);
    lay_main->addWidget(_bar_progress);
    lay_main->addWidget(_lbl_status);

    QHBoxLayout* lay_controls = new QHBoxLayout;
    lay_controls->addWidget(_butn_start_pause);
    lay_main->addLayout(lay_controls);

    QHBoxLayout* lay_in_dir = new QHBoxLayout;
    lay_in_dir->addWidget(_tbox_in_dir);
    lay_in_dir->addWidget(_butn_browse_in);
    _tbox_in_filter->setFixedWidth(80);
    lay_in_dir->addWidget(_tbox_in_filter);
    lay_main->addLayout(lay_in_dir);
    lay_main->addWidget(_chk_repeat);
    QHBoxLayout* lay_timer = new QHBoxLayout;
    lay_timer->addWidget(new QLabel(tr("Период опроса, сек.:")));
    _spin_timer->setMinimum(1);
    _spin_timer->setMaximum(INT_MAX);
    lay_timer->addWidget(_spin_timer);
    lay_main->addLayout(lay_timer);
    lay_main->addWidget(_chk_delete_input);

    QHBoxLayout* lay_out_dir = new QHBoxLayout;
    lay_out_dir->addWidget(_tbox_out_dir);
    lay_out_dir->addWidget(_butn_browse_out);
    lay_main->addLayout(lay_out_dir);
    QHBoxLayout* lay_dup_action = new QHBoxLayout;
    lay_dup_action->addWidget(new QLabel(tr("Дублирующиеся входные файлы:")));
    _cbox_dup_action->addItems({tr("Перезаписать"), tr("Добавить счётчик")});
    lay_dup_action->addWidget(_cbox_dup_action);
    lay_main->addLayout(lay_dup_action);

    QHBoxLayout* lay_hex_val = new QHBoxLayout;
    lay_hex_val->addWidget(new QLabel(tr("Операнд операции XOR:")));
    lay_hex_val->addWidget(_tbox_xor_val);
    lay_main->addLayout(lay_hex_val);
    lay_main->addWidget(_lbl_xor_err);

    center_wgt->setLayout(lay_main);
    setCentralWidget(center_wgt);

    connectSignals();
}

void MainWindow::closeEvent(QCloseEvent*)
{
    if(_thr->isRunning()){
        _thr->close();
        _thr->wait();
    }
}

void MainWindow::connectSignals()
{
    connect(_butn_start_pause, &QPushButton::clicked, this, &MainWindow::onStartPause);
    connect(_butn_browse_in, &QPushButton::clicked, this, &MainWindow::onBrowse);
    connect(_butn_browse_out, &QPushButton::clicked, this, &MainWindow::onBrowse);

    connect(_tbox_in_dir, &QLineEdit::textChanged, this, &MainWindow::onInDirChanged);
    onInDirChanged();
    connect(_tbox_in_filter, &QLineEdit::textChanged, this, &MainWindow::onInFilterChanged);
    onInFilterChanged();
    connect(_tbox_out_dir, &QLineEdit::textChanged, this, &MainWindow::onOutDirChanged);
    onOutDirChanged();

    connect(_chk_repeat, &QCheckBox::stateChanged, this, &MainWindow::onIsRepeatingChanged);
    onIsRepeatingChanged();
    connect(_spin_timer, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onRepeatDelayChanged);

    connect(_chk_delete_input, &QCheckBox::stateChanged, this, &MainWindow::onDeleteInputFilesChanged);
    onDeleteInputFilesChanged();

    connect(_tbox_xor_val, &QLineEdit::textChanged, this, &MainWindow::onXORValueChanged);
    onXORValueChanged();

    connect(_cbox_dup_action, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onDupActionChanged);
    onDupActionChanged(_cbox_dup_action->currentIndex());

    connect(_thr, &WorkerThread::paused, this, &MainWindow::onThreadPaused);
    connect(_thr, &WorkerThread::statusChanged, this, &MainWindow::onThreadStatusChanged);
    connect(_thr, &WorkerThread::progressChanged, this, &MainWindow::onThreadProgressChanged);
}


void MainWindow::onStartPause()
{
    if(!_thr->isRunning()){
        _thr->start();
        _butn_start_pause->setText(tr("Пауза"));
    } else if(_thr->isPaused()) {
        _thr->resume();
        _butn_start_pause->setText(tr("Пауза"));
    } else {
        _thr->pause();
        _butn_start_pause->setText(tr("Возобновить"));
    }
}
void MainWindow::onBrowse()
{
    QString dir = QFileDialog::getExistingDirectory();
    if(!dir.isEmpty()){
        if(sender() == _butn_browse_in)
            _tbox_in_dir->setText(dir);
        else if(sender() == _butn_browse_out)
            _tbox_out_dir->setText(dir);
    }
}

void MainWindow::onInDirChanged()
{
    _thr->setInDirectoryPath(_tbox_in_dir->text());
}
void MainWindow::onInFilterChanged()
{
    _thr->setFilter(_tbox_in_filter->text());
}
void MainWindow::onOutDirChanged()
{
    _thr->setOutDirectoryPath(_tbox_out_dir->text());
}

void MainWindow::onIsRepeatingChanged()
{
    _spin_timer->setDisabled(!_chk_repeat->isChecked());
    if(!_chk_repeat->isChecked())
        _thr->setRepeatDelay(0);
    else
        _thr->setRepeatDelay(_spin_timer->value());
}
void MainWindow::onRepeatDelayChanged(int val)
{
    if(_chk_repeat->isChecked())
        _thr->setRepeatDelay(val);
}
void MainWindow::onDeleteInputFilesChanged()
{
    _thr->setDeleteInputFiles(_chk_delete_input->isChecked());
}

void MainWindow::onXORValueChanged()
{
    bool ok;
    quint64 val = _tbox_xor_val->text().toULongLong(&ok, 16);
    if(ok){
        _thr->setXORValue(val);
        _lbl_xor_err->setText("");
    } else
        _lbl_xor_err->setText(tr("Неправильное значение для XOR! Последнее: ") + QString::number(_thr->getXORValue(), 16));
}
void MainWindow::onDupActionChanged(int idx)
{
    _thr->setDuplicatePolicy(idx == 0 ? WorkerThread::DuplicatePolicy::OVERWRITE :
                                        WorkerThread::DuplicatePolicy::COUNTER);
}


void MainWindow::onThreadPaused()
{
    _butn_start_pause->setText(tr("Возобновить"));
}
void MainWindow::onThreadStatusChanged(QString status)
{
    _lbl_status->setText(status);
}
void MainWindow::onThreadProgressChanged(float progress)
{
    _bar_progress->setValue(progress * 100);
}
