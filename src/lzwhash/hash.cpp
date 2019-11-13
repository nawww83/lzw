#include "hash_p.h"

using namespace hhss;

hash::hash() :
    l_table_hash(0)
  , l_table_hash_decode(0)
  , free_index_table(MIN_N_TABLE - 1)
  , smult(PHI[0])
  , max_current_level(STEPLEVELS - 1)

{ 
    par_hash.nkey       = BYTE;
    par_hash.ncode      = MIN_CODE_BIT;
    par_hash.ntable     = MIN_N_TABLE;
    par_hash.nhashtable = 2*(1 << par_hash.ncode);
}

hash::~hash()
{
    if (l_table_hash)
        delete [] l_table_hash;
    if (l_table_hash_decode)
        delete [] l_table_hash_decode;
}

param_hash hash::malloc_(const int nb, const int nt)
{    
    setParam(nb, nt);
    // code
    if (!l_table_hash)
        l_table_hash  = new element_hash[par_hash.nhashtable*(max_current_level+1)];
    else {
        delete [] l_table_hash;
        l_table_hash = 0;
        l_table_hash  = new element_hash[par_hash.nhashtable*(max_current_level+1)];
    }
    // decode
    if (!l_table_hash_decode)
        l_table_hash_decode  = new element_hash[par_hash.ntable];
    else {
        delete [] l_table_hash_decode;
        l_table_hash_decode = 0;
        l_table_hash_decode  = new element_hash[par_hash.ntable];
    }
    return par_hash;
}

void hash::realloc_code() // new levels, add memory;
{
    if (!l_table_hash) return;

    int old_level = max_current_level;
    max_current_level += STEPLEVELS;
    element_hash *p = new element_hash[par_hash.nhashtable*(max_current_level+1)];
    int sz=old_level*par_hash.nhashtable;
    for (int i=0;i<sz;++i) {
        p[i].code   = l_table_hash[i].code;
        p[i].symbol = l_table_hash[i].symbol;
    }
    for (int i=0;i<par_hash.nhashtable;++i)
        p[sz+i].code = EMPTY_CODE;
    delete [] l_table_hash;
    l_table_hash = p;
    //    qDebug() << " REALLOC " << old_level;
}

void hash::setParam(const int nc, const int nt)
{
    par_hash.ncode = bound(MIN_CODE_BIT, nc, MAX_CODE_BIT);  // длина кода, бит
    smult = PHI[par_hash.ncode - MIN_CODE_BIT];
    par_hash.nkey       = BYTE + par_hash.ncode;              // длина ключа = длина байт-символа + длина кода, бит
    int max_n_table = (1 << par_hash.ncode);
    par_hash.ntable = bound(MIN_N_TABLE, nt, max_n_table);   // обычно, без хэшей
    par_hash.nhashtable = 2*max_n_table;                      // длина хэш-таблицы (в два раза больше, чем максимальный размер обычной таблицы)
}

int hash::bound(int a, int b, int c)
{
    return ( (b < a) ? a : ((b > c) ? c : b) );
}
