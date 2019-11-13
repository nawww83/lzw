#ifndef HASH_H
#define HASH_H

typedef unsigned char uchar;

namespace hhss {

const int BYTE =        8; // кол-во бит в символе-байте
const int EMPTY_CODE   = -1; // пустой код, no data
const int NBYTE        = 256;
const int MIN_N_TABLE  = (NBYTE + 3);
const int LOW16BIT     = 65535;
const int HI16BIT      = 16;
const int STEPLEVELS   = 6;
const int MIN_CODE_BIT = 9;// при изменении CODE_BIT придётся пересчитать массив PHI[] (см. процедуру setParam())
const int MAX_CODE_BIT = 16;
//        smult               = (int)( 0.6180339887498948482045868*(double)(1 << par_hash.nkey) ); // floor(phi * 2^nkey) по возможности близко, так,
const int PHI[8] = {81005, 162013, 324027, 648055, 1296111, 2592221, 5184443, 10368889}; // что НОД(smult, 2^nkey) = 1, nkey = 9, 10 ... 16

enum {
    CODE=0,
    DECODE
};

struct element_hash {
    int code; // 16 bit for code, 16 bit for code prev string
    uchar symbol;
};

struct param_hash {
    int ncode;
    int nkey;
    int ntable;
    int nhashtable;
};

class hash
{
public:
    hash();
    ~hash();
    param_hash malloc_(const int nb, const int nt);
    void realloc_code();
    void erase(const int what)
    {
        switch (what) {
        case CODE:
            for (int i=0;i<par_hash.nhashtable;++i)
                l_table_hash[i].code=EMPTY_CODE;
            break;
        case DECODE:
            for (int i=0;i<par_hash.ntable;++i)
                l_table_hash_decode[i].code=EMPTY_CODE;
        default:
            break;
        }
        free_index_table = MIN_N_TABLE - 1; // 258 - first code for otrezok more than 1 Bytes
        // 256 - clear code, 257 - end code
    }
    int addCode(const int prevcode, const uchar symbol, const int code)
    {
        int index     = getIndex( ((int)symbol << par_hash.ncode) | prevcode ); // getIndex(key)
        int curr_code = l_table_hash[index].code;
        int level     = 0;
        while (curr_code ^ EMPTY_CODE) {
            ++level;
            index += par_hash.nhashtable;
            curr_code = l_table_hash[index].code;
        }
        if (!(level ^ max_current_level))
            realloc_code();
        l_table_hash[index].code        = (code << HI16BIT) | prevcode; // !!! must be (code != prevcode) if code = 0xffff
        l_table_hash[index].symbol      = symbol;        
        l_table_hash[index+par_hash.nhashtable].code = EMPTY_CODE;
        ++free_index_table;
        return level;
    }
    bool findCode(const int prevcode, const uchar symbol, int& code)
    {
        if (!(prevcode ^ EMPTY_CODE)) {
            code=symbol;
            return true;
        }
        int index         = getIndex( ((int)symbol << par_hash.ncode) | prevcode );
        int curr_code     = l_table_hash[index].code;
        uchar curr_symbol = l_table_hash[index].symbol;
        while (curr_code ^ EMPTY_CODE) {
            if (!( ((curr_code & LOW16BIT) ^ prevcode) || (symbol ^ curr_symbol) ))  {
                code = (curr_code >> HI16BIT);
                return true;
            }
            index      += par_hash.nhashtable;
            curr_code   = l_table_hash[index].code;
            curr_symbol = l_table_hash[index].symbol;
        }
        code = EMPTY_CODE;
        return false;
    }
    void addWord(const int prevcode, const uchar symbol) // code EMPTY_CODE - empty symbol
    {
        if (!(prevcode ^ EMPTY_CODE)) return; // no add one-symbol words

        l_table_hash_decode[free_index_table].code=prevcode;
        l_table_hash_decode[free_index_table].symbol=symbol;
        ++free_index_table;
    }
    int getWord(const int code, uchar *u)
    {
        if (!(code ^ EMPTY_CODE)) return 0;
        if (!(code >> BYTE)) { // 0 <= code < NBYTE
            u[0] = code;
            return 1;
        }
        int i          = code;
        int lengthWord = 0;
        while (i ^ EMPTY_CODE) {
            if (!(l_table_hash_decode[i].code ^ EMPTY_CODE)) {
                u[lengthWord] = i;
                ++lengthWord;
                break;
            }
            u[lengthWord] = l_table_hash_decode[i].symbol;
            ++lengthWord;
            i             = l_table_hash_decode[i].code;
        }
        // reverse of vector u
        for (int i=0;i<(lengthWord>>1);++i) { // 6 => 3; 5 => 2; ...
            const uchar t     = u[lengthWord-1-i];
            u[lengthWord-1-i] = u[i];
            u[i]              = t;
        }
        return lengthWord;
    }
    bool isFullTable()
    {
        return (free_index_table == par_hash.ntable);// for 16-bit code   max used index is 2^16 - 1 = 65535 = 0xffff
    }
    int getIndex(const int key)
    {
        // hash(key) = floor(length_hash_table*{key*A}), A = phi, 0 < A < 1; Knuth D.
        return ( ((key*smult)&((1 << par_hash.nkey) - 1)) >> (BYTE - 1) );
    }
    int getFreeCode()
    {
        return free_index_table;
    }
private:
    void setParam(const int nc, const int nt);
    int bound(int a, int b, int c);

    element_hash *l_table_hash; // таблица ключей, кодирование (упаковка)
    // при кодировании в поле .code хранятся код (не более 16 бит) и код предыдущего отрезка (не более 16 бит)
    // индекс вычисляется на основании кода предыдущего отрезка и текущего символа по формуле getIndex() в addCode() или findCode()
    element_hash *l_table_hash_decode; // таблица ключей, декодирование (распаковка);
    // при декодировании код отрезка - это индекс таблицы, в которой хранится код предыдущего отрезка (.code) и символ (.symbol)
    param_hash par_hash;
    int free_index_table;
    int smult;
    int max_current_level;  // текущий уровень; бОльший уровень мЕнее верояен за счёт хэш-функции
};

}

#endif // HASH_H
