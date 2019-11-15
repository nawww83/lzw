#include "widget.h"
#include "ui_widget.h"

#include "lzw.h"

#include <QFileDialog>
#include <QSettings>
#include <QTime>
#include <cmath>

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
        if (n == reka_line_size)
            sz = readRekaVector(u, n);
        else sz = 0;
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
    for (size_t i=0; i<n; ++i)
        u[i] = static_cast<uchar>( qrand() );
}

void Widget::repeatByteVector(qint64 n, uchar *u, uchar repeat) {
    for (size_t i=0; i<n; ++i)
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
    for (size_t i=0; i<n; ++i)
        str.append(QString("%1 ").arg(u[i]));
}

// NTABLE NCODE NSYMBOLS SIZE_CODE_BUFF
// SIZE IN BYTES: 4, 4, 4, 4
void Widget::getHeader(QFile &f, llzz::paramLZ &plz) {
    size_t countBytes = 4;
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
    size_t countBytes = 4;
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

void Widget::on_btnSelectFile_clicked() {
    QString str;
    QDir dir(path);
    str = QFileDialog::getOpenFileName(nullptr, "", dir.path(), "*.bin *.binz");
    ui->lineEdit->setText(str);
    if (str.contains(".binz")) {
        ui->btnDeCompress->setEnabled(true);
        ui->btnCompress->setEnabled(false);
    } else if (str.contains(".bin")) {
        ui->btnDeCompress->setEnabled(false);
        ui->btnCompress->setEnabled(true);
    }
}

void Widget::on_btnTest_clicked() {
    ui->textBrowser->clear();
    ui->textBrowser->append(QString("LZW test running..."));

    auto sz = fillVector(in, IN_N);

    if (sz!= IN_N) {
        ui->textBrowser->append(QString("Size error!"));
        return;
    }

    copyByteVectorToString(sz, in, str);
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
        Wr = lzmpw.compress(in, sz_, out);
    ui->textBrowser->append(QString("Compression time is %1 ms per line").arg(1. * tm.elapsed() / n_repeat));

    copyByteVectorToString(Wr, out, str);
    ui->textBrowser->append(QString("Compressor output: %1 bytes").arg(Wr));
    ui->textBrowser->append(str);

    Wr=lzmpw.decompress(out, in_decod);

    copyByteVectorToString(Wr, in_decod, str);
    ui->textBrowser->append(QString("Decompressor output: %1 bytes").arg(Wr));
    ui->textBrowser->append(str);

    ui->textBrowser->append(QString("Memcmp() test is %1").arg(memcmp(in, in_decod, static_cast<size_t>(sz))));
}

void Widget::on_btnCompress_clicked() {
    bool WRITE = ui->checkBoxWriteToFile->isChecked();    
    QDir dir;
    QFile in_file;
    QFile out_file;    
    ui->textBrowser->clear();
    auto Rd = IN_N;

    if (ui->lineEdit->text().isEmpty()) return;
    dir.setPath(ui->lineEdit->text());
    in_file.setFileName(dir.absolutePath());
    if (!in_file.exists())
        return;

    if (WRITE) {
        QString abspath = dir.absolutePath();
        abspath.replace(QString(".bin"), QString(".tmp"), Qt::CaseInsensitive);
        out_file.setFileName(abspath);
        out_file.open(QIODevice::WriteOnly);
    }

    ui->textBrowser->append(in_file.fileName());

    in_file.open(QIODevice::ReadOnly);
    ui->pbRun->setValue(0);

    auto count_line = in_file.size() / Rd;
    auto res_count_line = in_file.size() % Rd;
    if (count_line < 1) {
        Rd = in_file.size();
        count_line = 1;
    }
    ratio = new double[ static_cast<size_t>(count_line)];

    char WrB[SZ_HEADER];    

    ui->textBrowser->append(QString("LZW compress running ..."));

    auto tt = static_cast<size_t>(ui->spinBoxNTable->value());
    auto bb = static_cast<size_t>(ui->spinBoxNBits->value());
    Lzw lzmpw(tt, bb);
    paramLZ pLZ;

    pLZ = lzmpw.getParamLZ();

    int Writed = 0;

    fillByteArrayFromHeader(WrB, pLZ);
    if (WRITE)
        out_file.write(WrB, SZ_HEADER);

    Writed += SZ_HEADER;

    QTime tm;
    tm.start();

    int Readed = 0;
    for (size_t i=0; i<count_line; ++i) {
        in_file.read(reinterpret_cast<char *>(in), Rd);
        Readed += Rd;

        int Wr = 0;        
        Wr = lzmpw.compress(in, static_cast<size_t>(Rd), out);

        if (WRITE)
            out_file.write(reinterpret_cast<char*>(out), Wr);

        Writed += Wr;
        ratio[i] =  1. * Rd / Wr;
        ui->pbRun->setValue((i + 1) * 100 / count_line);
    }
    if (res_count_line > 0) {
        in_file.read(reinterpret_cast<char *>(in), res_count_line);
        Readed += res_count_line;

        int Wr = 0;
        Wr = lzmpw.compress(in, static_cast<size_t>(res_count_line), out);

        if (WRITE)
            out_file.write(reinterpret_cast<char*>(out), Wr);

        Writed += Wr;
    }

    if (WRITE) {
        in_file.remove();
        auto fn = out_file.fileName();
        auto fn_new = fn.replace(QString(".tmp"), QString(".binz"), Qt::CaseInsensitive);
        out_file.rename(fn_new);
        out_file.close();
    } else
        in_file.close();

    ui->textBrowser->append(QString::fromUtf8("Time elapsed %1 ms").arg(1. * tm.elapsed()));
    ui->textBrowser->append(QString::fromUtf8("In %1 bytes").arg(Readed));
    ui->textBrowser->append(QString::fromUtf8("Out %1 bytes").arg(Writed));
    ui->textBrowser->append(QString::fromUtf8("Total compress ratio in/out is %1").arg( 1. * Readed / Writed));
    double mean_ratio = 0.;
    double max_ratio = ratio[0];
    double min_ratio = ratio[0];
    for(size_t j=0; j<count_line; ++j) {
        mean_ratio += ratio[j];
        max_ratio = (ratio[j] > max_ratio) ? ratio[j] : max_ratio;
        min_ratio = (ratio[j] < min_ratio) ? ratio[j] : min_ratio;
    }
    mean_ratio /= count_line;
    double sko_ratio = 0.;
    for(size_t j=0; j<count_line; ++j)
        sko_ratio += (ratio[j] - mean_ratio) * (ratio[j] - mean_ratio);
    if (count_line > 1)
        sko_ratio /= (count_line - 1);
    sko_ratio = std::sqrt(sko_ratio);

    ui->textBrowser->append(QString::fromUtf8("Compress ratio R:"));
    ui->textBrowser->append(QString::fromUtf8("Mean R is %1").arg(mean_ratio));
    ui->textBrowser->append(QString::fromUtf8("SKO R is %1").arg(sko_ratio));
    ui->textBrowser->append(QString::fromUtf8("Min R; Max R is %1; %2").arg(min_ratio).arg(max_ratio));

    ui->btnCompress->setEnabled(false);

    delete [] ratio;
}

void Widget::on_btnDeCompress_clicked() {
    bool WRITE = ui->checkBoxWriteToFile->isChecked();
    ui->textBrowser->clear();
    if (ui->lineEdit->text().isEmpty())
        return;

    QDir dir;
    QFile in_file;
    QFile out_file;
    dir.setPath(ui->lineEdit->text());
    in_file.setFileName(dir.absolutePath());
    if (!in_file.exists())
        return;

    QString abspath = dir.absolutePath();
    abspath.replace(QString(".binz"), QString(".tmp"), Qt::CaseInsensitive);
    out_file.setFileName(abspath);

    if (WRITE)
        out_file.open(QIODevice::WriteOnly);

    ui->textBrowser->append(in_file.fileName());
    in_file.open(QIODevice::ReadOnly);
    ui->pbRun->setValue(0);

    ui->textBrowser->append(QString("LZW decompression..."));

    paramLZ pLZ;
    getHeader(in_file, pLZ);

    int Readed = 0;
    Readed += SZ_HEADER;

    ui->spinBoxNBits->setValue( static_cast<int>(pLZ.ncode) );
    ui->spinBoxNTable->setValue( static_cast<int>(pLZ.ntable) );

    Lzw lzmpw(pLZ.ntable, pLZ.ncode);

    int Writed = 0;

    QTime tm;
    tm.start();

    size_t NBlocks = 0; // обязательно обнулить!
    while (!in_file.atEnd()) {
        quint32 *p32 = &NBlocks;
        QByteArray NBl = in_file.read(SZ_NBLOCK);
        memcpy(p32, NBl.data(), SZ_NBLOCK);

        auto Rd = NBlocks * pLZ.size_code_buff;
        Readed += SZ_NBLOCK;

        in_file.read(reinterpret_cast<char *>(&in[SZ_NBLOCK]), Rd);
        for (size_t i=0; i<SZ_NBLOCK; ++i)
            in[i] = reinterpret_cast<const uchar *>(NBl.constData())[i];

        Readed += Rd;
        int Wr = lzmpw.decompress(in, out);
        if (WRITE)
            out_file.write(reinterpret_cast<char *>(out), Wr);
        Writed += Wr;

        ui->pbRun->setValue( static_cast<int>( in_file.pos() / in_file.size() ) * 100);
    }

    if (WRITE) {
        in_file.remove();
        auto fn = out_file.fileName();
        auto fn_new = fn.replace(QString(".tmp"), QString(".bin"), Qt::CaseInsensitive);
        out_file.rename(fn_new);
        out_file.close();
    } else
        in_file.close();

    ui->textBrowser->append(QString::fromUtf8("Time elapsed %1 ms").arg(1. * tm.elapsed()));
    ui->textBrowser->append(QString::fromUtf8("In %1 bytes").arg(Readed));
    ui->textBrowser->append(QString::fromUtf8("Out %1 bytes").arg(Writed));

    ui->btnDeCompress->setEnabled(false);
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
