#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>

#include "lzwdefs.h"

class QFile;

const int IN_N = 4096;
const int SZ_HEADER = 16;
const int SZ_NBLOCK = 4;

enum
{
    RekaLine = 1,
    RandomLine = 2,
    RepeatLine = 3
};


namespace Ui {
class Widget;
}

class Widget : public QWidget
{
    Q_OBJECT
    
public:
    explicit Widget(QWidget *parent = 0);
    ~Widget();

    int fillVector(uchar *u, const int n);
    void randomByteVector(const int n, uchar *u);
    void repeatByteVector(const int n, uchar *u, uchar repeat);
    int typeRekaByteVector(uchar *u);
    void copyByteVectorToString(const int n, const uchar *u, QString &str);
    void getHeader(QFile &f, llzz::paramLZ &plz);
    void fillByteArrayFromHeader(char *vb, const llzz::paramLZ &plz);

    void writeSettings();
    void readSettings();

private slots:
    void on_btnSelectFile_clicked();    

    void on_btnCompress_clicked();

    void on_btnTest_clicked();

    void on_btnDeCompress_clicked();

    void on_spinBoxNTable_valueChanged(int arg1);

    void on_checkBoxAutoNBits_clicked();

private:
    Ui::Widget *ui;
    uchar in[IN_N*2];
    uchar out[IN_N*4];
    uchar in_decod[IN_N*2];

    QString str;
    QString path;
    float *ratio;    

    int prevNT, prevNW, prevNB;
};

#endif // WIDGET_H
