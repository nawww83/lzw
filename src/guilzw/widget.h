#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>

#include "lzw_thread.h"

class QFile;

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
    void copyByteVectorToString(qint64 n, const uchar *u, QString &mStr);

    void writeSettings();
    void readSettings();

public slots:
    void handleCompressResults(vec_int64 res_1, vec_double res_2);
    void handleDeCompressResults(vec_int64 res_1, vec_double res_2);
    void progress(int percent);

private slots:
    void on_btnSelectFile_clicked();    

    void on_btnCompress_clicked();

    void on_btnTest_clicked();

    void on_btnDeCompress_clicked();

    void on_spinBoxNTable_valueChanged(int arg1);

    void on_checkBoxAutoNBits_clicked();

    void on_sbxBufferSize_valueChanged(int arg1);

private:
    Ui::Widget *ui;

    void updateFilePath();

    int mBlockSize{4096}; // Размер блока непрерывно сжимаемых данных
    QVector<uchar> mIn;
    QVector<uchar> mOut;
    QVector<uchar> mIn_decod;

    QString mPath;
};

#endif // WIDGET_H
