#include "worker_thread.h"
#include <QDir>
#include <QDateTime>

#define __bswap_constant_64(x)				\
  ((((x) & 0xff00000000000000ull) >> 56)	\
   | (((x) & 0x00ff000000000000ull) >> 40)	\
   | (((x) & 0x0000ff0000000000ull) >> 24)	\
   | (((x) & 0x000000ff00000000ull) >> 8)	\
   | (((x) & 0x00000000ff000000ull) << 8)	\
   | (((x) & 0x0000000000ff0000ull) << 24)	\
   | (((x) & 0x000000000000ff00ull) << 40)	\
   | (((x) & 0x00000000000000ffull) << 56))

void WorkerThread::pause()
{
    _mut_pause.lock();
    _paused = true;
    _mut_pause.unlock();
}
void WorkerThread::resume()
{
    _mut_pause.lock();
    _paused = false;
    _mut_pause.unlock();
    _cond_pause.notify_one();
}
bool WorkerThread::isPaused()
{
    _mut_pause.lock();
    bool ret = _paused;
    _mut_pause.unlock();
    return ret;
}

void WorkerThread::close()
{
    _mut_pause.lock();
    _paused = false;
    _closing = true;
    _mut_pause.unlock();
    _cond_pause.notify_one();
}


void WorkerThread::setInDirectoryPath(QString path)
{
    _mut_params.lock();
    _in_path = path;
    _mut_params.unlock();
}
void WorkerThread::setOutDirectoryPath(QString path)
{
    _mut_params.lock();
    _out_path = path;
    _mut_params.unlock();
}
void WorkerThread::setFilter(QString filter)
{
    _mut_params.lock();
    _filter = filter;
    _mut_params.unlock();
}
void WorkerThread::setXORValue(quint64 val)
{
    #ifdef Q_BIG_ENDIAN
        val = __bswap_constant_64(val);
    #endif
    _mut_params.lock();
    _xor_val = val;
    _mut_params.unlock();
}
quint64 WorkerThread::getXORValue()
{
    _mut_params.lock();
    quint64 ret = _xor_val;
    _mut_params.unlock();
    #ifdef Q_BIG_ENDIAN
        ret = __bswap_constant_64(ret);
    #endif
    return ret;
}
void WorkerThread::setDuplicatePolicy(DuplicatePolicy policy)
{
    _mut_params.lock();
    _duplicate_policy = policy;
    _mut_params.unlock();
}
void WorkerThread::setDeleteInputFiles(bool del)
{
    _mut_params.lock();
    _delete_input = del;
    _mut_params.unlock();
}
void WorkerThread::setRepeatDelay(unsigned delay)
{
    _mut_params.lock();
    _repeat_delay = delay;
    _mut_params.unlock();
}

void WorkerThread::run()
{
    bool closing = false;
    while(!closing){
        _mut_params.lock();
        QDirIterator it(_in_path, {_filter}, QDir::Files);
        _mut_params.unlock();

        while(it.hasNext()){
            _mut_params.lock();
            quint64 xor_val = _xor_val;
            QString out_path = _out_path;
            DuplicatePolicy dup_pol = _duplicate_policy;
            bool delete_input = _delete_input;
            _mut_params.unlock();

            QFile fd_in(it.next());
            if(!fd_in.open(QIODevice::ReadOnly))
                continue;

            size_t counter = 0;
            QString fd_out_name;
            while(QFile::exists(fd_out_name = getCounterFileName(it, out_path, counter++)) &&
                  dup_pol == DuplicatePolicy::COUNTER)
                ;
            QFile fd_out(fd_out_name);
            if(!fd_out.open(QIODevice::WriteOnly))
                continue;

            emit statusChanged(tr("Обработка файла '%1'...").arg(fd_in.fileName()));

            qint64 fsize = fd_in.size();
            qint64 total = 0;
            qint64 read;
            do {
                _mut_pause.lock();
                if(_paused)
                    emit statusChanged(tr("Пауза."));
                while(_paused)
                    _cond_pause.wait(&_mut_pause);
                closing = _closing;
                _mut_pause.unlock();

                read = fd_in.read(_buf, sizeof(_buf));
                quint64 i;
                for(i = 0; i < read - 7; i += 8)
                    *reinterpret_cast<quint64*>(_buf + i) ^= xor_val;
                for(int j = 0; i < read; ++i, ++j)
                    _buf[i] ^= (xor_val >> (j * 8)) & 0xFF;

                fd_out.write(_buf, read);

                total += read;
                emit progressChanged(static_cast<float>(total) / fsize);
            } while(read == sizeof(_buf) && !closing);

            if(delete_input)
                fd_in.remove();
        }

        if(closing)
            break;

        _mut_params.lock();
        unsigned repeat_delay = _repeat_delay;
        _mut_params.unlock();
        if(repeat_delay == 0){
            emit statusChanged(tr("Пауза."));
            _mut_pause.lock();
            _paused = true;
            emit paused();
            while(_paused)
                _cond_pause.wait(&_mut_pause);
            closing = _closing;
            _mut_pause.unlock();
        } else {
            emit statusChanged(tr("Ожидание опроса директории..."));
            qint64 target_epoch = QDateTime::currentMSecsSinceEpoch() + _repeat_delay * 1000;
            do {
                _mut_pause.lock();
                closing = _closing;
                _mut_pause.unlock();
                QThread::msleep(250);
            } while(!closing && QDateTime::currentMSecsSinceEpoch() < target_epoch);
        }
    }
}

QString WorkerThread::getCounterFileName(const QDirIterator& iter, QString path, size_t counter)
{
    QFileInfo inf = iter.fileInfo();
    if(counter == 0) return path + inf.fileName();
    return QString("%1%2_%3.%4").arg(path).arg(inf.baseName()).arg(counter).arg(inf.suffix());
}
