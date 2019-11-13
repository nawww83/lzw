#ifndef LZW_H
#define LZW_H

#include "lzw_global.h"
#include "lzwdefs.h"

namespace llzz {

namespace Internal {
class LzwPrivate;
}

class LZW_API Lzw
{
public:
    Lzw(const int nt, const int ncode);
    ~Lzw();
    int compress(unsigned char *in, const int Ns, unsigned char *out);
    int decompress(unsigned char *in, unsigned char *out);

    paramLZ getParamLZ();

private:
    Internal::LzwPrivate *pLZW;
};

}

#endif // LZW_H
