#include <iostream>
#include <stdio.h>
#include <fstream>
#include <time.h>
#include "lzw.h"

typedef unsigned char uchar;

using namespace std;
using namespace llzz;

enum {
    SRC = 0,
    LOG,
    FFT,
    FFT_16B,
    SRC_ZIP
};

struct FileHeader {
    //    12 char - местоположение радара
    char location[12];
    //    12 char - название радара
    char name[12];
    //    12 char - поляризация сигнала
    char polarization[12];

    // версия протокола = 0
    unsigned int version;
    //    float - угловое разрешение антенны
    float r_angle;
    //  uint8 - усиление
    unsigned char att;
    // номер сигнала
    unsigned char sign;
    //    uint16 - количество сохраняемых линий
    unsigned short numLine;
    //    uint8 - количество осредняемых соседних дальностных строк (1,2,3,4 и т.д)
    unsigned char r_average;
    //    uint8 - тип данных SRC, LOG, FFT
    unsigned char dataType;
    //    float - дискретизация данных по дальности (1.5 м)
    float r_discret;
    //    uint16 - количество точек
    unsigned short r_numPoint;
};

///////////////////
int read_radar_header(const string& file, FileHeader& fh);
string dataType(int type);
string compress(const string& file, int header_size, float &ratio);
string decompress(const string& file, int header_size);
///////////////////

const size_t LEN_4K = 4096;
const size_t LEN_8K = 8192;
const size_t LEN_16K = 16384;

int main(int argc, char* argv[])
{
//    cout << "Arg count " << argc << endl;

    cout << "***********************************" << endl;
    cout << "    Compressor/decompressor of the radar SRC data    " << endl;

    if (argc < 2) {
        cout << "Run '*.exe file1 file2 ...' for compress or '*.exe d file1 file2 ...' for decompress selected files" << endl;
        return 0;
    }

    cout << "" << endl;

    const string sIsDecompress = "d";
    bool isDecompress = false;

    string _str(argv[1]);
    if (_str.compare(sIsDecompress) == 0) {
        cout << " Decompress selected " << endl;
        isDecompress = true;
    } else {
        cout << " Compress selected " << endl;
    }

    for (int i=((isDecompress) ? 2 : 1); i<argc; ++i) {
        cout << " - "<< argv[i] << endl;
    }

    cout << " All selected files will be overwrite after processing! Continue (Y/N)?" << endl;
    string _s;
    cin >> _s;
    if (_s.size() == 1) {
        if ((_s.at(0) == 'y') || (_s.at(0) == 'Y')) {
        } else {
            return 0;
        }
    } else {
        return 0;
    }


    for (int i=((isDecompress) ? 2 : 1); i<argc; ++i) {
        string file(argv[i]);

        cout << "Processing " << file.c_str() << "..." << endl;
        FileHeader fh;
        int result = read_radar_header(file, fh);
        if (result > 0) {
            cout << " Header is Ok. Protocol version is " << (fh.version+1) << "." << endl;
            cout << " Header size is " << result << " bytes." << endl;
            cout << " Data type is " << dataType(fh.dataType) << "." << endl;            

            if ((fh.dataType != SRC) && (fh.dataType != SRC_ZIP)) {
                cout << " Data type " << dataType(fh.dataType) << " is not supported!" << endl;
                continue;
            }
            if ((fh.dataType == SRC)&&(isDecompress)) {
                cout << " Data type " << dataType(fh.dataType) << " can't be decompressed!" << endl;
                continue;
            }
            if ((fh.dataType == SRC_ZIP)&&(!isDecompress)) {
                cout << " Data type " << dataType(fh.dataType) << " can't be compressed!" << endl;
                continue;
            }
        } else {
            cout << " Header is broken!" << endl;
            continue;
        }
        if (!isDecompress) {
            cout << " Compressing file ... " << endl;
            float ratio;
            long t1 = clock();
            string _s = compress(file, result, ratio);
            long t2 = clock();
            if (!_s.empty())
                cout << "  compressed file is " << _s << "; ratio is " << ratio << endl;
            else
                cout << "  compress error in file " << file << endl;
            cout << "    compress time is " << float(t2-t1)/CLOCKS_PER_SEC << " s" << endl;
        }
        else {
            cout << " Decompressing file ... " << endl;
            long t1 = clock();
            string _s = decompress(file, result);
            long t2 = clock();
            if (!_s.empty())
                cout << "  decompressed file is " << _s << endl;
            else
                cout << "  decompress error in file " << file << endl;
            cout << "    decompress time is " << float(t2-t1)/CLOCKS_PER_SEC << " s" << endl;
        }
    }

    cout << "Press enter..." << endl;

    fflush(stdin);
    cin.get();

    return 0;
}

string compress(const string& file, int header_size, float &ratio) {
    static  uchar v_line_in[LEN_16K];
    static  uchar v_line_exp[LEN_8K];
    static  uchar v_line_mant[LEN_8K];
    static uchar v_line_out[LEN_16K];

    const string empty_str = "";

    float all_writed = 0.0f;
    float all_readed = 0.0f;
    ratio = -1.0f;

    string sfw = file + ".tmp";
    ofstream fw(sfw.c_str(), std::ios_base::binary);
    ifstream fr(file.c_str(), std::ios_base::binary);
    if (!(fw.is_open() && fr.is_open())) {
        std::remove(sfw.c_str());
        return empty_str;
    }

    char *header = new char[header_size];
    fr.read(header, header_size);
    all_readed += float(header_size);
    header[49] = SRC_ZIP; // change data type
    fw.write(header, header_size);
    all_writed += float(header_size);
    delete [] header;

    const int ntable = 1024;
    const int ncode  = 10;
    Lzw compander(ntable, ncode);

    const size_t SZ_LZW_HD = 16;
    paramLZ plz = compander.getParamLZ();
    fw.write((char *)&plz.ntable, 4);
    fw.write((char *)&plz.ncode, 4);
    fw.write((char *)&plz.nsymbols, 4);
    fw.write((char *)&plz.size_code_buff, 4);
    all_writed += float(SZ_LZW_HD);

    const size_t SZ_AZ_NUM = 8;
    char aznum[SZ_AZ_NUM];
    fr.seekg(header_size, ios_base::beg);
    while (fr.read(aznum, SZ_AZ_NUM)) {
        all_readed += float(SZ_AZ_NUM);
        fr.read((char *)v_line_in, LEN_16K);
        all_readed += float(LEN_16K);
        for (size_t i=0;i<LEN_8K;++i) {
            v_line_mant[i] = v_line_in[2*i];
            v_line_exp[i] = v_line_in[2*i+1];
        }
        int sz = compander.compress(v_line_exp, LEN_8K, v_line_out);
        fw.write(aznum, SZ_AZ_NUM);
        fw.write((char *)v_line_mant, LEN_8K);
        fw.write((char *)v_line_out, sz);
        all_writed += float(SZ_AZ_NUM + LEN_8K + sz);
    }

    fr.close();
    fw.close();

    std::remove(file.c_str());
    std::rename(sfw.c_str(), file.c_str());

    ratio = all_readed/all_writed;

    return file;
}

string decompress(const string& file, int header_size) {
    static  uchar v_line_in[LEN_16K];
    static  uchar v_line_exp[LEN_8K];
    static  uchar v_line_mant[LEN_8K];
    static uchar v_line_out[LEN_16K];

    const string empty_str = "";

    string sfw = file + ".tmp";
    ofstream fw(sfw.c_str(), std::ios_base::binary);
    ifstream fr(file.c_str(), std::ios_base::binary);
    if (!(fw.is_open() && fr.is_open())) {
        std::remove(sfw.c_str());
        return empty_str;
    }

    char *header = new char[header_size];
    fr.read(header, header_size);
    header[49] = SRC; // change data type
    fw.write(header, header_size);
    delete [] header;

    paramLZ plz;
    const size_t SZ_LZW_HD = 16;
    fr.read((char *)&plz.ntable, 4);
    fr.read((char *)&plz.ncode, 4);
    fr.read((char *)&plz.nsymbols, 4);
    fr.read((char *)&plz.size_code_buff, 4);

    Lzw compander(plz.ntable, plz.ncode);
    paramLZ plz_ = compander.getParamLZ();

    if ((plz_.ncode != plz.ncode) ||
    (plz_.ntable != plz.ntable) ||
    (plz_.size_code_buff != plz.size_code_buff) ||
    (plz_.nsymbols != plz.nsymbols)) {
        std::remove(sfw.c_str());
        return empty_str;
    }

    // bin header + size lzw header
    fr.seekg(header_size + SZ_LZW_HD, ios_base::beg);
    const size_t SZ_WR_BL = 4;
    uchar wr_blocks[SZ_WR_BL];
    const size_t SZ_AZ_NUM = 8;
    char aznum[SZ_AZ_NUM];
    bool error = false;
    while ((fr.read(aznum, SZ_AZ_NUM))&&(!error)) {
        fr.read((char *)v_line_mant, LEN_8K);
        fr.read((char *)wr_blocks, SZ_WR_BL);
        size_t nRd = (wr_blocks[0] + (wr_blocks[1] << 8) + (wr_blocks[2] << 16) + (wr_blocks[3] << 24))*plz.size_code_buff;
        fr.read((char *)&v_line_in[SZ_WR_BL], nRd);
        int sz = compander.decompress(v_line_in, v_line_exp);
        error = (sz != (int)LEN_8K);
        for (size_t i=0;i<LEN_8K;++i) {
            v_line_out[2*i] = v_line_mant[i];
            v_line_out[2*i+1] = v_line_exp[i];
        }
        fw.write(aznum, SZ_AZ_NUM);
        fw.write((char *)v_line_out, LEN_16K);
    }

    fr.close();
    fw.close();

    if (error) {
        std::remove(sfw.c_str());
        return empty_str;
    }

    std::remove(file.c_str());
    std::rename(sfw.c_str(), file.c_str());

    return file;
}

int read_radar_header(const string& file, FileHeader &fh) {
    ifstream f(file.c_str(), std::ios_base::binary);
    if (!f.is_open()) {
        return -1;
    }

    int header_size = 0;

    f.read(fh.location, 12);
    f.read(fh.name, 12);
    f.read(fh.polarization, 12);

    f.read((char *)&fh.version, 4);
    f.read((char *)&fh.r_angle, 4);

    f.read((char *)&fh.att, 1);
    f.read((char *)&fh.sign, 1);

    f.read((char *)&fh.numLine, 2);

    f.read((char *)&fh.r_average, 1);
    f.read((char *)&fh.dataType, 1);

    f.read((char *)&fh.r_discret, 4);

    f.read((char *)&fh.r_numPoint, 2);

    if (f.gcount() < 1)
        return -2;

    header_size += 56;

    unsigned char commentBytes[4];
    if (fh.version == 1) { // protocol version 2
        f.read((char *)commentBytes, 4);
        header_size += 4;
        // Big endian int from Qt library
        int cb = commentBytes[3] + (commentBytes[2] << 8) + (commentBytes[1] << 16) + (commentBytes[0] << 24);
        if (cb < 0)
            cb = 0;
//        cout << " comment bytes is " << cb << endl;
        f.seekg(cb, ios_base::cur);
        header_size += cb;
    }

    f.close();

    return header_size;
}

string dataType(int type) {
    string result;
    switch (type) {
    case SRC:
        result = "SRC";
        break;
    case LOG:
        result = "LOG";
        break;
    case FFT:
        result = "FFT";
        break;
    case FFT_16B:
        result = "FFT_16B";
        break;
    case SRC_ZIP:
        result = "SRC_ZIP";
    default:
        break;
    }

    return result;
}
