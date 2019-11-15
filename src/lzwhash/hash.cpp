#include "hash_p.h"

using namespace hhss;

hash::hash() :
    l_table_hash(nullptr)
  , l_table_hash_decode(nullptr)
  , free_index_table(MIN_N_TABLE - 1)
  , max_current_level(STEPLEVELS - 1)
{
    calc_phi();
    multiplicator = PHI[0];
    setParam(MIN_CODE_BIT, MIN_N_TABLE);
//    par_hash.nkey       = BYTE;
//    par_hash.ncode      = MIN_CODE_BIT;
//    par_hash.ntable     = MIN_N_TABLE;
//    par_hash.nhashtable = 2 * (1 << par_hash.ncode);
}

hash::~hash() {
    if (l_table_hash)
        delete [] l_table_hash;
    if (l_table_hash_decode)
        delete [] l_table_hash_decode;
}

param_hash hash::malloc_(size_t nb, size_t nt) {
    setParam(nb, nt);
    auto nc = static_cast<unsigned int>(par_hash.nhashtable * (max_current_level + 1));
    // code
    if (!l_table_hash)
        l_table_hash  = new element_hash[nc];
    else {
        delete [] l_table_hash;
        l_table_hash = nullptr;
        l_table_hash  = new element_hash[nc];
    }
    auto nd = static_cast<unsigned int>(par_hash.ntable);
    // decode
    if (!l_table_hash_decode)
        l_table_hash_decode  = new element_hash[nd];
    else {
        delete [] l_table_hash_decode;
        l_table_hash_decode = nullptr;
        l_table_hash_decode  = new element_hash[nd];
    }
    return par_hash;
}

void hash::realloc_code() { // new levels, add memory;
    if (!l_table_hash)
        return;

    auto old_level = max_current_level;
    max_current_level += STEPLEVELS;
    auto nc = static_cast<unsigned int>(par_hash.nhashtable * (max_current_level + 1));
    auto *p = new element_hash[nc];
    auto sz = old_level * par_hash.nhashtable;
    for (size_t i=0; i<sz; ++i) {
        p[i].code   = l_table_hash[i].code;
        p[i].symbol = l_table_hash[i].symbol;
    }
    for (size_t i=0; i<par_hash.nhashtable; ++i)
        p[sz + i].code = EMPTY_CODE;

    delete [] l_table_hash;
    l_table_hash = p;
}

void hash::setParam(size_t nc, size_t nt) {
    par_hash.ncode = bound(MIN_CODE_BIT, nc, MAX_CODE_BIT);  // длина кода, бит
    multiplicator = PHI[par_hash.ncode - MIN_CODE_BIT];
    par_hash.nkey       = BYTE + par_hash.ncode;   // длина ключа = длина байт-символа + длина кода, бит
    size_t max_n_table = (1 << par_hash.ncode);
    par_hash.ntable = bound(MIN_N_TABLE, nt, max_n_table);   // обычно, без хэшей
    par_hash.nhashtable = 2 * max_n_table;         // длина хэш-таблицы (в два раза больше, чем максимальный размер обычной таблицы)
}

size_t hash::bound(size_t a, size_t b, size_t c) const {
    return ( (b < a) ? a : ((b > c) ? c : b) );
}

void hash::calc_phi() {
    size_t n = MAX_CODE_BIT + 1 - MIN_CODE_BIT;
    for (size_t i=0; i<n; ++i) {
        auto key = i + MIN_CODE_BIT;
        PHI[i] = static_cast<size_t>( phi0 * static_cast<double>((1 << key)) );
    }
}
