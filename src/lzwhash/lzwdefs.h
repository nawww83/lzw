#ifndef LZWDEFS_H
#define LZWDEFS_H

#include <cstdlib>

namespace llzz {

struct paramLZ {
    size_t ntable;
    size_t ncode;
    size_t nsymbols;
    size_t size_code_buff;
};

}

#endif // LZWDEFS_H
