#ifndef LZW_P_H
#define LZW_P_H

#include "hash_p.h"
#include "lzwdefs.h"

namespace llzz {

namespace Internal {

class LzwPrivate
{
public:
    LzwPrivate(size_t nt, size_t ncode);
    int compress(uchar *in, size_t Ns, uchar *out);
    int decompress(uchar *in, uchar *out);

    paramLZ getParamLZ() const;
    
private:

    enum {
        N_SKEEP = 4, // N Bytes for WRBLOCKS
        CLEAR_CODE = 256,
        END_CODE
    };

    size_t getSizeCodeBuff(size_t ncode) const;

    void copyNChars(uchar *Wr, const uchar *Rd, size_t n) {
        for (size_t i=0; i<n; ++i)
            Wr[i] = Rd[i];
    }

    void copyNCharsWithZeros(uchar *Wr, uchar *Rd, size_t n)
    {
        for (size_t i=0; i<n; ++i) {
            Wr[i] = Rd[i];
            Rd[i] = 0;
        }
    }

    void flushBuffer(uchar *v);
    void writeToBuffer(const uchar *v);
    void writeCode(uchar *v, const size_t code);
    void readCode(const uchar *v, size_t &code);

    size_t N_TABLE;
    size_t N_CODE;

    int fill_bit;
    int readed_bit;
    bool buffer_need_flush;
    bool buffer_is_old;

    size_t size_code_buff;
    size_t size_code_buff_bit;

    int wr_bytes;
    int rd_bytes;

    uchar code_buff[hhss::MAX_CODE_BIT*8];
    hhss::hash hashtable;
};

}
}

#endif // LZW_P_H
