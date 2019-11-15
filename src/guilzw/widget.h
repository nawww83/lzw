#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>

#include "lzwdefs.h"

class QFile;

constexpr qint64 IN_N = 4096;
constexpr qint64 SZ_HEADER = 16;
constexpr qint64 SZ_NBLOCK = 4;

enum {
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
    explicit Widget(QWidget *parent = nullptr);
    ~Widget();

    qint64 fillVector(uchar *u, qint64 n);
    void randomByteVector(qint64 n, uchar *u);
    void repeatByteVector(qint64 n, uchar *u, uchar repeat);
    qint64 readRekaVector(uchar *u, qint64 n);
    void copyByteVectorToString(qint64 n, const uchar *u, QString &str);
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
    double *ratio;

    int prevNT, prevNW, prevNB;
};

#endif // WIDGET_H
