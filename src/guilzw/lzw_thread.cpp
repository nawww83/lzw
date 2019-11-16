#include "lzw_thread.h"

#include <QFile>
#include <QDir>
#include <cmath>
#include <QTime>

lzw_thread::lzw_thread(const QString &in, bool write, int buff_size, int header_size, size_t n_table, size_t n_code) :
   mLzw(Lzw(n_table, n_code))
  , mInputFileName(in)
  , mWrite(write)
  , mBufferSize(buff_size)
  , mHeaderSize(header_size)
  , mNtable(n_table)
  , mNcode(n_code)
{}

void lzw_thread::compress() {
    qint64 Rd = mBufferSize;
    QDir dir(mInputFileName);
    QFile in_file(dir.absolutePath());

    QFile out_file;
    if (mWrite) {
        QString abspath = dir.absolutePath();
        abspath.replace(QString(".bin"), QString(".tmp"), Qt::CaseInsensitive);
        out_file.setFileName(abspath);
        out_file.open(QIODevice::WriteOnly);
    }

    in_file.open(QIODevice::ReadOnly);

    int count_line = static_cast<int>( (in_file.size() / Rd) );
    int res_count_line = static_cast<int>( (in_file.size() % Rd) );
    if (count_line < 1) {
        Rd = in_file.size();
        count_line = 1;
    }
    mRatio.resize(count_line);

    QVector<char> WrB(mHeaderSize);
    auto pLZ = mLzw.getParamLZ();
    fillByteArrayFromHeader(WrB.data(), pLZ);
    if (mWrite)
        out_file.write(WrB.constData(), mHeaderSize);

    qint64 Writed = 0;
    Writed += mHeaderSize;

    in.resize(mBufferSize * 2);
    out.resize(mBufferSize * 4);

    auto Readed = 0 * Rd;
    QTime time;
    time.start();
    for (int i=0; i<count_line; ++i) {
        in_file.read( reinterpret_cast<char *>(in.data()), Rd);
        Readed += Rd;

        int Wr = 0;
        Wr = mLzw.compress(in.data(), static_cast<size_t>(Rd), out.data());

        if (mWrite)
            out_file.write(reinterpret_cast<char*>(out.data()), Wr);

        Writed += Wr;
        mRatio[i] =  1. * Rd / Wr;
        emit progress(100 * (i + 1) / count_line);
    }
    if (res_count_line > 0) {
        in_file.read(reinterpret_cast<char *>(in.data()), res_count_line);
        Readed += res_count_line;

        int Wr = 0;
        Wr = mLzw.compress(in.data(), static_cast<size_t>(res_count_line), out.data());

        if (mWrite)
            out_file.write(reinterpret_cast<char*>(out.data()), Wr);

        Writed += Wr;
    }
    auto elapsed = time.elapsed();

    if (mWrite) {
        in_file.remove();
        auto fn = out_file.fileName();
        auto fn_new = fn.replace(QString(".tmp"), QString(".binz"), Qt::CaseInsensitive);
        out_file.rename(fn_new);
        out_file.close();
    } else
        in_file.close();

    auto mean_ratio = 0.;
    auto max_ratio = mRatio[0];
    auto min_ratio = mRatio[0];
    for(int j=0; j<count_line; ++j) {
        mean_ratio += mRatio[j];
        max_ratio = (mRatio[j] > max_ratio) ? mRatio[j] : max_ratio;
        min_ratio = (mRatio[j] < min_ratio) ? mRatio[j] : min_ratio;
    }
    mean_ratio /= count_line;
    auto sko_ratio = 0.;
    for(int j=0; j<count_line; ++j)
        sko_ratio += (mRatio[j] - mean_ratio) * (mRatio[j] - mean_ratio);
    if (count_line > 1)
        sko_ratio /= (count_line - 1);
    sko_ratio = std::sqrt(sko_ratio);

    vec_int64 res_1;
    res_1.append(Writed);
    res_1.append(Readed);
    res_1.append(elapsed);
    vec_double res_2;
    res_2.append(mean_ratio);
    res_2.append(sko_ratio);
    res_2.append(min_ratio);
    res_2.append(max_ratio);
    emit ready(res_1, res_2);
}

void lzw_thread::fillByteArrayFromHeader(char *vb, const paramLZ &plz) {
    size_t stride = 4;
    auto v = vb;
    memcpy(v, &plz.ntable, stride); v += stride;
    memcpy(v, &plz.ncode, stride); v += stride;
    memcpy(v, &plz.nsymbols, stride); v += stride;
    memcpy(v, &plz.size_code_buff, stride);
}
