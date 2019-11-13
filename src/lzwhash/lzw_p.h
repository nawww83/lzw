#ifndef LZW_P_H
#define LZW_P_H

#include "hash_p.h"
#include "lzwdefs.h"

namespace llzz {

namespace Internal {

class LzwPrivate
{
public:
    LzwPrivate(const int nt, const int ncode);
    int compress(uchar *in, const int Ns, uchar *out);
    int decompress(uchar *in, uchar *out);

    paramLZ getParamLZ();
    
private:

    enum {
        N_SKEEP = 4, // N Bytes for WRBLOCKS
        CLEAR_CODE = 256,
        END_CODE
    };

    int getSizeCodeBuff(const int ncode);

    void copyNChars(uchar *Wr, const uchar *Rd, const int n)
    {
        for (int i=0;i<n;++i)
            Wr[i]=Rd[i];
    }

    void copyNCharsWithZeros(uchar *Wr, uchar *Rd, const int n)
    {
        for (int i=0;i<n;++i) {
            Wr[i]=Rd[i];
            Rd[i]=0;
        }
    }

    void flushBuffer(uchar *v);
    void writeToBuffer(const uchar *v);
    void writeCode(uchar *v, const int code);
    void readCode(const uchar *v, int& code);

    int N_TABLE;
    int N_CODE;

    int fill_bit;
    int readed_bit;
    bool buffer_need_flush;
    bool buffer_is_old;

    int size_code_buff;
    int size_code_buff_bit;

    int wr_bytes;    
    int rd_bytes;

    uchar code_buff[hhss::MAX_CODE_BIT*8];
    hhss::hash hashtable;
};

}
}

#endif // LZW_P_H
