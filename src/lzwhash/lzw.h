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
    Lzw(size_t nt, size_t ncode);
    ~Lzw();
    int compress(unsigned char *in, size_t Ns, unsigned char *out);
    int decompress(unsigned char *in, unsigned char *out);

    paramLZ getParamLZ() const;

private:
    Internal::LzwPrivate *pLZW;
};

}

#endif // LZW_H
