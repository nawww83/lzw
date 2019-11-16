#ifndef LZW_THREAD_H
#define LZW_THREAD_H

#include <QString>
#include <QVector>

#include <QObject>

#include "lzw.h"

#include <QThread>
#include <QMutex>


using namespace llzz;

typedef QVector<qint64> vec_int64;
typedef QVector<double> vec_double;

class lzw_thread : public QObject
{
    Q_OBJECT
public:
    explicit lzw_thread(const QString& in, bool write, int buff_size, int header_size, size_t n_table, size_t n_code);

public slots:
    void compress();

signals:
    void ready(vec_int64 res_1, vec_double res_2);
    void progress(int percent);

private:
    void fillByteArrayFromHeader(char *vb, const paramLZ &plz);

    Lzw mLzw;
    QString mInputFileName;

    bool mWrite;

    int mBufferSize;
    int mHeaderSize;

    size_t mNtable;
    size_t mNcode;

    QVector<uchar> in;
    QVector<uchar> out;

    QVector<double> mRatio;

    bool mCompress;

    QMutex mMutex;
};

#endif // LZW_THREAD_H
