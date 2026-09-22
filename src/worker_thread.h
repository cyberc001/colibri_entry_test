#ifndef WORKER_THREAD
#define WORKER_THREAD

#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QDirIterator>

class WorkerThread : public QThread
{
    Q_OBJECT
public:
    explicit WorkerThread(QObject *parent = nullptr): QThread(parent) {}

    void pause();
    void resume();
    bool isPaused();

    void close();

    void setInDirectoryPath(QString);
    void setOutDirectoryPath(QString);
    void setFilter(QString);
    void setXORValue(quint64);
    quint64 getXORValue();
    enum DuplicatePolicy {
        OVERWRITE, COUNTER
    };
    void setDuplicatePolicy(DuplicatePolicy);
    void setDeleteInputFiles(bool);
    void setRepeatDelay(unsigned); // в секундах; 0 - без повторов

signals:
    void paused(); // Пауза при окончании обработки директории
    void statusChanged(QString);
    void progressChanged(float); // [0; 1]

private:
    void run();

    static QString getCounterFileName(const QDirIterator&, QString path, size_t);

    QMutex _mut_pause;
    QWaitCondition _cond_pause;
    bool _paused = false;
    bool _closing = false;

    QMutex _mut_params;

    QString _in_path, _out_path;
    QString _filter;
    quint64 _xor_val = 0;
    bool _delete_input = false;
    unsigned _repeat_delay = 0;

    DuplicatePolicy _duplicate_policy = DuplicatePolicy::OVERWRITE;

    char _buf[4096];
};


#endif // WORKER_THREAD

