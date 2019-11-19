#include "widget.h"
#include "ui_widget.h"

#include "lzw.h"

#include <QFileDialog>
#include <QSettings>
#include <QTime>
#include <QThread>

#include <cmath>
#include <iostream>

using namespace llzz;


Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);
    ui->buttonGroup->setId(ui->rbTypeReka, RekaLine);
    ui->buttonGroup->setId(ui->rbRandom, RandomLine);
    ui->buttonGroup->setId(ui->rbRepeat, RepeatLine);
    readSettings();
    ui->checkBoxAutoNBits->setChecked(true);
    on_checkBoxAutoNBits_clicked();

    ui->sbxBufferSize->setValue(mBlockSize);

    qRegisterMetaType<vec_int64>("vec_int64");
    qRegisterMetaType<vec_int64>("vec_double");
}

Widget::~Widget() {
    writeSettings();
    delete ui;
}

qint64 Widget::fillVector(uchar *u, qint64 n) {
    int id = ui->buttonGroup->checkedId();
    qint64 sz = 0;
    constexpr qint64 reka_line_size = 4096;
    switch (id) {
    case RekaLine:
        sz = readRekaVector(u, reka_line_size);
        break;
    case RandomLine:
        randomByteVector(n, u);
        sz = n;
        break;
    case RepeatLine:
        qsrand( static_cast<uint>(QTime::currentTime().msec()) );
        repeatByteVector( n, u, static_cast<uchar>(qrand()) );
        sz=n;
        break;
    default:
        break;
    }
    return sz;
}

void Widget::randomByteVector(qint64 n, uchar *u) {
    qsrand( static_cast<uint>(QTime::currentTime().msec()) );
    for (auto i=0; i<n; ++i)
        u[i] = static_cast<uchar>( qrand() );
}

void Widget::repeatByteVector(qint64 n, uchar *u, uchar repeat) {
    for (auto i=0; i<n; ++i)
        u[i] = repeat;
}

qint64 Widget::readRekaVector(uchar *u, qint64 n) {
    QFile f(":/data/line.dat");
    f.open(QIODevice::ReadOnly);
    auto t = reinterpret_cast<char *>(u);
    auto sz = f.read(t, n);
    f.close();
    return sz;
}

void Widget::copyByteVectorToString(qint64 n, const uchar *u, QString &str) {
    str.clear();
    for (auto i=0; i<n; ++i)
        str.append(QString("%1 ").arg(u[i]));
}

// NTABLE NCODE NSYMBOLS SIZE_CODE_BUFF
// SIZE IN BYTES: 4, 4, 4, 4
void Widget::getHeader(QFile &f, llzz::paramLZ &plz) {
    constexpr auto countBytes = 4;
    QByteArray vb(f.read(countBytes));
    memcpy(&plz.ntable, vb.data(), countBytes);
    vb = f.read(countBytes);
    memcpy(&plz.ncode, vb.data(), countBytes);
    vb = f.read(countBytes);
    memcpy(&plz.nsymbols, vb.data(), countBytes);
    vb = f.read(countBytes);
    memcpy(&plz.size_code_buff, vb.data(), countBytes);
}

void Widget::fillByteArrayFromHeader(char *vb, const llzz::paramLZ &plz) {
    constexpr auto countBytes = 4;
    auto v = vb;
    memcpy(v, &plz.ntable, countBytes); v += countBytes;
    memcpy(v, &plz.ncode, countBytes); v += countBytes;
    memcpy(v, &plz.nsymbols, countBytes); v += countBytes;
    memcpy(v, &plz.size_code_buff, countBytes);
}

void Widget::writeSettings() {
    if (!ui->lineEdit->text().isEmpty()) {
        QSettings settings("compressh.ini", QSettings::IniFormat);
        settings.setValue("path", ui->lineEdit->text());
    }
}

void Widget::readSettings() {
    QSettings settings("compressh.ini", QSettings::IniFormat);
    path = settings.value("path", "").toString();
}

void Widget::handleCompressResults(vec_int64 res_1, vec_double res_2) {
    ui->textBrowser->append(QString::fromUtf8("Time elapsed %1 ms").arg(1. * res_1.at(2)));
    ui->textBrowser->append(QString::fromUtf8("Readed %1 bytes").arg(res_1.at(1)));
    ui->textBrowser->append(QString::fromUtf8("Writed %1 bytes").arg(res_1.at(0)));
    ui->textBrowser->append(QString::fromUtf8("Total compress ratio in/out is %1").arg( 1. * res_1.at(1) / res_1.at(0) ));

    ui->textBrowser->append(QString::fromUtf8("Compress ratio R:"));
    ui->textBrowser->append(QString::fromUtf8("Mean R is %1").arg(res_2.at(0)));
    ui->textBrowser->append(QString::fromUtf8("Stdev. R is %1").arg(res_2.at(1)));
    ui->textBrowser->append(QString::fromUtf8("R in [%1; %2]").arg(res_2.at(2)).arg(res_2.at(3)));

    ui->btnCompress->setEnabled(true);
}

void Widget::handleDeCompressResults(vec_int64 res_1, vec_double res_2) {
    ui->textBrowser->append(QString::fromUtf8("Time elapsed %1 ms").arg(1. * res_1.at(2)));
    ui->textBrowser->append(QString::fromUtf8("Readed %1 bytes").arg(res_1.at(1)));
    ui->textBrowser->append(QString::fromUtf8("Writed %1 bytes").arg(res_1.at(0)));

    ui->spinBoxNBits->setValue( static_cast<int>(res_2.at(0) + 0.5) );
    ui->spinBoxNTable->setValue( static_cast<int>(res_2.at(1) + 0.5) );

    ui->btnDeCompress->setEnabled(true);
}

void Widget::progress(int percent) {
    ui->pbRun->setValue(percent);
}

void Widget::on_btnSelectFile_clicked() {
    QString str;
    QDir dir(path);
    str = QFileDialog::getOpenFileName(nullptr, "", dir.path(), "*.bin *.binz");
    ui->lineEdit->setText(str);
    auto bin_z = str.contains(QRegExp(".binz$"));
    auto bin = str.contains(QRegExp(".bin$"));
    ui->btnDeCompress->setEnabled(bin_z);
    ui->btnCompress->setEnabled(bin);
}

void Widget::on_btnTest_clicked() {
    ui->textBrowser->clear();
    ui->textBrowser->append(QString("LZW test running..."));

    auto sz = fillVector(in.data(), mBlockSize);

    copyByteVectorToString(sz, in.constData(), str);
    ui->textBrowser->append(QString("Input %1 bytes").arg(sz));
    ui->textBrowser->append(str);


    auto ntable = static_cast<size_t>(ui->spinBoxNTable->value());
    auto ncode = static_cast<size_t>(ui->spinBoxNBits->value());
    Lzw lzmpw(ntable, ncode);

    QTime tm;
    tm.start();
    int Wr = 0;
    auto n_repeat = static_cast<size_t>(ui->spinBox->value());
    auto sz_ = static_cast<size_t>(sz);
    for (size_t i=0; i<n_repeat; ++i)
        Wr = lzmpw.compress(in.data(), sz_, out.data());
    ui->textBrowser->append(QString("Compression time is %1 ms per line").arg(1. * tm.elapsed() / n_repeat));

    copyByteVectorToString(Wr, out.constData(), str);
    ui->textBrowser->append(QString("Compressor output: %1 bytes").arg(Wr));
    ui->textBrowser->append(str);

    Wr=lzmpw.decompress(out.data(), in_decod.data());

    copyByteVectorToString(Wr, in_decod.constData(), str);
    ui->textBrowser->append(QString("Decompressor output: %1 bytes").arg(Wr));
    ui->textBrowser->append(str);

    ui->textBrowser->append(QString("Memcmp() test is %1").arg(memcmp(in.data(), in_decod.data(), static_cast<size_t>(sz))));
}

void Widget::on_btnCompress_clicked() {
    bool write = ui->checkBoxWriteToFile->isChecked();
    ui->textBrowser->clear();

    auto path = ui->lineEdit->text();

    if (!QFile::exists(path))
        return;

    auto n_table = static_cast<size_t>(ui->spinBoxNTable->value());
    auto n_code = static_cast<size_t>(ui->spinBoxNBits->value());

    auto lzw = new lzw_thread(path, write, mBlockSize, n_table, n_code);
    QThread *workerThread = new QThread();
    lzw->moveToThread(workerThread);

    connect(workerThread, &QThread::started, lzw, &lzw_thread::compress);
    connect(lzw, SIGNAL(progress(int)), this, SLOT(progress(int)));

    auto ready = SIGNAL(compress_ready(vec_int64, vec_double));
    connect(lzw, ready, SLOT(handleCompressResults(vec_int64, vec_double)));
    connect(lzw, &lzw_thread::compress_ready, [=]() {
        lzw->deleteLater();
        workerThread->quit();
    });
    connect(workerThread, &QThread::finished, workerThread, &QThread::deleteLater);

    ui->btnCompress->setEnabled(false);

    workerThread->start();
}

void Widget::on_btnDeCompress_clicked() {
    bool WRITE = ui->checkBoxWriteToFile->isChecked();
    ui->textBrowser->clear();

    auto path = ui->lineEdit->text();

    if (!QFile::exists(path))
        return;

    auto n_table = static_cast<size_t>(ui->spinBoxNTable->value());
    auto n_code = static_cast<size_t>(ui->spinBoxNBits->value());

    auto lzw = new lzw_thread(path, WRITE, mBlockSize, n_table, n_code);
    QThread *workerThread = new QThread();
    lzw->moveToThread(workerThread);

    connect(workerThread, &QThread::started, lzw, &lzw_thread::decompress);
    connect(lzw, SIGNAL(progress(int)), this, SLOT(progress(int)));

    auto ready = SIGNAL(decompress_ready(vec_int64, vec_double));
    connect(lzw, ready, SLOT(handleDeCompressResults(vec_int64, vec_double)));
    connect(lzw, &lzw_thread::decompress_ready, [=]() {
        lzw->deleteLater();
        workerThread->quit();
    });
    connect(workerThread, &QThread::finished, workerThread, &QThread::deleteLater);

    ui->btnDeCompress->setEnabled(false);

    workerThread->start();
}

void Widget::on_spinBoxNTable_valueChanged(int arg1) {
    if (!ui->checkBoxAutoNBits->isChecked())
        return;

    int nt = arg1 - 1; // max code value
    int bit_counter = 0;
    while (nt > 0) {
        nt = (nt >> 1);
        ++bit_counter;
    }
    ui->spinBoxNBits->setValue(bit_counter);
}

void Widget::on_checkBoxAutoNBits_clicked() {
    if (ui->checkBoxAutoNBits->isChecked()) {
        on_spinBoxNTable_valueChanged(ui->spinBoxNTable->value());
        ui->spinBoxNBits->setEnabled(false);
    } else
        ui->spinBoxNBits->setEnabled(true);
}

void Widget::on_sbxBufferSize_valueChanged(int arg1) {
    mBlockSize = arg1;

    in.resize(mBlockSize * 2);
    out.resize(mBlockSize * 4);
    in_decod.resize(mBlockSize * 2);
}
