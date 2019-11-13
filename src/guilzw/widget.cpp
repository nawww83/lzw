#include "widget.h"
#include "ui_widget.h"

#include "lzw.h"

#include <QFileDialog>
#include <QFile>
#include <QSettings>
#include <QTime>
#include <math.h>

#include <QDebug>

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

Widget::~Widget()
{
    writeSettings();
    delete ui;
}

int Widget::fillVector(uchar *u, const int n)
{
    int id = ui->buttonGroup->checkedId();
    int sz=0;
    switch (id) {
    case RekaLine:
        if (n == 4096)
            sz = typeRekaByteVector(u);
        else sz = 0;
        break;
    case RandomLine:
        randomByteVector(n, u);
        sz=n;
        break;
    case RepeatLine:
        qsrand(qrand());
        repeatByteVector(n, u, (uchar)(qrand()>>7));
        sz=n;
        break;
    default:
        break;
    }
    return sz;
}

void Widget::randomByteVector(const int n, uchar *u)
{
    qsrand(qrand());
    for (int i=0;i<n;++i)
        u[i]=(uchar)(qrand()>>7); // 0...255
}

void Widget::repeatByteVector(const int n, uchar *u, uchar repeat)
{
    for (int i=0;i<n;++i)
        u[i]=repeat;
}

int Widget::typeRekaByteVector(uchar *u)
{
    QFile f(":/data/line.dat");
    f.open(QIODevice::ReadOnly);
    char *v = (char *)u;
    int sz = f.size();
    f.read(v, sz);
    f.close();
    return sz;
}

void Widget::copyByteVectorToString(const int n, const uchar *u, QString &str)
{
    str.clear();
    for (int i=0;i<n;++i)
        str.append(QString("%1 ").arg(u[i]));
}

// NTABLE NCODE NSYMBOLS SIZE_CODE_BUFF
// SIZE IN BYTES: 4, 4, 4, 4
void Widget::getHeader(QFile &f, llzz::paramLZ &plz)
{
    const int countBytes = 4;
    QByteArray vb(f.read(countBytes));
    memcpy(&plz.ntable, vb.data(), countBytes);
    vb = f.read(countBytes);
    memcpy(&plz.ncode, vb.data(), countBytes);
    vb = f.read(countBytes);
    memcpy(&plz.nsymbols, vb.data(), countBytes);
    vb = f.read(countBytes);
    memcpy(&plz.size_code_buff, vb.data(), countBytes);
}

void Widget::fillByteArrayFromHeader(char *vb, const llzz::paramLZ &plz)
{
    const int countBytes = 4;
    char *v = vb;
    memcpy(v, &plz.ntable, countBytes); v+=countBytes;
    memcpy(v, &plz.ncode, countBytes); v+=countBytes;
    memcpy(v, &plz.nsymbols, countBytes); v+=countBytes;
    memcpy(v, &plz.size_code_buff, countBytes);
}

void Widget::writeSettings()
{
    if (!ui->lineEdit->text().isEmpty())
    {
        QSettings settings("compressh.ini", QSettings::IniFormat);
        settings.setValue("path", ui->lineEdit->text());
    }
}

void Widget::readSettings()
{
    QSettings settings("compressh.ini", QSettings::IniFormat);
    path = settings.value("path", "").toString();
}

void Widget::on_btnSelectFile_clicked()
{
    QString str;
    QDir dir(path);
    str = QFileDialog::getOpenFileName(0,"",dir.path(),"*.cutf; *.lzw");
    ui->lineEdit->setText(str);
    bool archive = str.endsWith(".lzw", Qt::CaseInsensitive);
    ui->btnDeCompress->setEnabled(archive);
    ui->btnCompress->setEnabled(!archive);
}

void Widget::on_btnTest_clicked() // test LZW
{
    ui->textBrowser->clear();
    ui->textBrowser->append(QString("LZW test running ..."));

    int sz = IN_N;

    sz = fillVector(in, sz);

    if (sz!= IN_N) {
        ui->textBrowser->append(QString("Error size!"));
        return;
    }

    copyByteVectorToString(sz, in, str);
    ui->textBrowser->append(QString("Input %1 bytes").arg(sz));
    ui->textBrowser->append(str);


    int ntable = ui->spinBoxNTable->value();
    int ncode = ui->spinBoxNBits->value();
    Lzw lzmpw(ntable, ncode);

    QTime tm;
    tm.start();
    int Wr=0; int n=(ui->spinBox->value() < 1) ? 1 : ui->spinBox->value();
    for (int i=0;i<n;++i)
        Wr = lzmpw.compress(in, sz, out);
    ui->textBrowser->append(QString("Compress %1 times is about %2 ms").arg(n).arg(tm.elapsed()));

    copyByteVectorToString(Wr, out, str);
    ui->textBrowser->append(QString("Output compress %1 bytes").arg(Wr));
    ui->textBrowser->append(str);

    Wr=lzmpw.decompress(out, in_decod);

    copyByteVectorToString(Wr, in_decod, str);
    ui->textBrowser->append(QString("Decompress %1 bytes").arg(Wr));
    ui->textBrowser->append(str);

    ui->textBrowser->append(QString("Memcmp() is %1").arg(memcmp(in, in_decod, sz)));
}

void Widget::on_btnCompress_clicked()
{
    bool WRITE = ui->checkBoxWriteToFile->isChecked();    
    QDir dir;
    QFile in_file;
    QFile out_file;    
    ui->textBrowser->clear();
    int Rd = IN_N; // порция, линия

    if (ui->lineEdit->text().isEmpty()) return;
    dir.setPath(ui->lineEdit->text());
    in_file.setFileName(dir.absolutePath());
    if (!in_file.exists()) return;

    if (WRITE) {
        QString abspath = dir.absolutePath();
        abspath.replace(QString(".cutf"),QString(".lzw"),Qt::CaseInsensitive);
        out_file.setFileName(abspath);
        out_file.open(QIODevice::WriteOnly);
    }

    ui->textBrowser->append(in_file.fileName());

    in_file.open(QIODevice::ReadOnly);
    ui->pbRun->setValue(0);

    int count_line = in_file.size()/Rd;
    if (count_line < 1) {
        Rd = in_file.size();
        count_line = 1;
    }
    ratio = new float[count_line];

    char WrB[SZ_HEADER];    

    ui->textBrowser->append(QString("LZW compress running ..."));

    int tt = ui->spinBoxNTable->value();
    int bb = ui->spinBoxNBits->value();
    Lzw lzmpw(tt,bb);
    paramLZ pLZ;
    int size_code_buff;

    pLZ = lzmpw.getParamLZ();
    size_code_buff = pLZ.size_code_buff;

    int AllWr=0;

    fillByteArrayFromHeader(WrB, pLZ);
    out_file.write(WrB, SZ_HEADER);
    AllWr+=SZ_HEADER;

    QTime tm;
    tm.start();

    int AllRd=0;
    for (int i=0;i<count_line;++i) {
        in_file.read((char *)in, Rd);
        AllRd+=Rd;

        int Wr = 0;        
        Wr=lzmpw.compress(in, Rd, out);

        if (WRITE)
            out_file.write((char *)out, Wr);        

        AllWr += Wr;
        ratio[i]=float(Rd)/float(Wr);     
        ui->pbRun->setValue((i+1)*100/count_line);
    }
    in_file.close();    

    if (WRITE)
        out_file.close();

    if (WRITE) {
        in_file.setFileName(QString("ratioLZW.dat"));
        in_file.open(QIODevice::WriteOnly);
        in_file.write((char *)ratio, count_line*sizeof(ratio[0]));
        in_file.close();
    }

    ui->textBrowser->append(QString::fromUtf8("Time elapsed %1 ms").arg(tm.elapsed()));
    ui->textBrowser->append(QString::fromUtf8("In %1 bytes").arg(AllRd));
    ui->textBrowser->append(QString::fromUtf8("Out %1 bytes").arg(AllWr));
    ui->textBrowser->append(QString::fromUtf8("Total compress ratio in/out is %1").arg(float(AllRd)/float(AllWr)));
    float mean_ratio=0;
    float max_ratio=ratio[0];
    float min_ratio=ratio[0];
    for(int j=0;j<count_line;++j) {
        mean_ratio += ratio[j];
        max_ratio = (ratio[j] > max_ratio) ? ratio[j] : max_ratio;
        min_ratio = (ratio[j] < min_ratio) ? ratio[j] : min_ratio;
    }
    mean_ratio /= count_line;
    float sko_ratio = 0.0f;
    for(int j=0;j<count_line;++j)
        sko_ratio += (ratio[j]-mean_ratio)*(ratio[j]-mean_ratio);
    if (count_line > 1)
        sko_ratio /= (count_line-1);
    sko_ratio = sqrtf(sko_ratio);

    ui->textBrowser->append(QString::fromUtf8("Compress ratio R:"));
    ui->textBrowser->append(QString::fromUtf8("Mean R is %1").arg(mean_ratio));
    ui->textBrowser->append(QString::fromUtf8("SKO R is %1").arg(sko_ratio));
    ui->textBrowser->append(QString::fromUtf8("Min R; Max R is %1; %2").arg(min_ratio).arg(max_ratio));

    delete [] ratio;
}

void Widget::on_btnDeCompress_clicked()
{
    bool WRITE = ui->checkBoxWriteToFile->isChecked();
    ui->textBrowser->clear();
    if (ui->lineEdit->text().isEmpty()) return;

    QDir dir;
    QFile in_file;
    QFile out_file;
    dir.setPath(ui->lineEdit->text());
    in_file.setFileName(dir.absolutePath());
    if (!in_file.exists()) return;

    QString abspath = dir.absolutePath();
    abspath.replace(QString(".lzw"),QString(".dlzw"),Qt::CaseInsensitive);
    out_file.setFileName(abspath);

    if (WRITE)
        out_file.open(QIODevice::WriteOnly);

    ui->textBrowser->append(in_file.fileName());
    in_file.open(QIODevice::ReadOnly);
    ui->pbRun->setValue(0);

    ui->textBrowser->append(QString("LZW decompress ..."));

    paramLZ pLZ;
    getHeader(in_file, pLZ);

    int AllWr=0, AllRd=0;
    AllRd+=SZ_HEADER;

    ui->spinBoxNBits->setValue(pLZ.ncode);
    ui->spinBoxNTable->setValue(pLZ.ntable);

    Lzw lzmpw(pLZ.ntable, pLZ.ncode);

    QTime tm;
    tm.start();

    quint32 NBlocks=0; // обязательно обнулить!
    while (!in_file.atEnd()) {
        quint32 *p32 = &NBlocks;
        QByteArray NBl = in_file.read(SZ_NBLOCK);
        memcpy(p32, NBl.data(), SZ_NBLOCK);

        int Rd = NBlocks*pLZ.size_code_buff;
        AllRd += SZ_NBLOCK;

        in_file.read((char*)&in[SZ_NBLOCK], Rd);
        for (int i=0;i<SZ_NBLOCK;++i)
            in[i]=NBl.at(i);

        AllRd+=Rd;
        int Wr = lzmpw.decompress(in, out);
        if (WRITE)
            out_file.write((char *)out, Wr);
        AllWr+=Wr;

        ui->pbRun->setValue(in_file.pos()*100/in_file.size());
    }

    in_file.close();
    if (WRITE)
        out_file.close();
    ui->textBrowser->append(QString::fromUtf8("Time elapsed %1 ms").arg(tm.elapsed()));
    ui->textBrowser->append(QString::fromUtf8("In %1 bytes").arg(AllRd));
    ui->textBrowser->append(QString::fromUtf8("Out %1 bytes").arg(AllWr));
}

void Widget::on_spinBoxNTable_valueChanged(int arg1)
{
    if (!(ui->checkBoxAutoNBits->isChecked())) return;
    int nt = arg1 - 1; // max code
    int counter=0;
    while (nt > 0) {
        nt=(nt>>1);
        ++counter;
    }
    ui->spinBoxNBits->setValue(counter);
}

void Widget::on_checkBoxAutoNBits_clicked()
{
    if (ui->checkBoxAutoNBits->isChecked()) {
        on_spinBoxNTable_valueChanged(ui->spinBoxNTable->value());
        ui->spinBoxNBits->setEnabled(false);
    } else {
        ui->spinBoxNBits->setEnabled(true);
    }
}
