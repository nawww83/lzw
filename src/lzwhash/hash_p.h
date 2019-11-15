#ifndef HASH_H
#define HASH_H

#include <algorithm>

typedef unsigned char uchar;

namespace hhss {

constexpr size_t BYTE =        8; // Количество бит в одном кодируемом символе
constexpr size_t EMPTY_CODE   = static_cast<size_t>(-1); // Код отсутствия данных
constexpr size_t NBYTE        = (1 << BYTE); //
constexpr size_t MIN_N_TABLE  = NBYTE + 3; // Минимальный размер таблицы
constexpr size_t STEPLEVELS   = 6; // Приращение уровней при необходимости реалокации памяти под хеш-таблицу
constexpr size_t MIN_CODE_BIT = BYTE + 1; // Минимальная длина кода, в битах
constexpr size_t MAX_CODE_BIT = 16; // Максимальная длина кода, в битах
constexpr size_t LOW16BIT     = (1 << 16) - 1; // Маска для выделения младших 16 бит слова
constexpr size_t HI16BIT      = 16; // Константа для переноса битов на 16 разрядов влево (вверх)

enum {
    CODE = 0,
    DECODE
};

struct element_hash {
    size_t code; // 16 bit for code, 16 bit for code of the previous string
    uchar symbol;
};

struct param_hash {
    size_t ncode;      // Длина кода, в битах
    size_t nkey;       // Длина ключа, в битах
    size_t ntable;     // Размер таблицы, в строках, в диапазоне [MIN_N_TABLE, 2^ncode]
    size_t nhashtable; // Размер хеш-таблицы, в строках, 2 * 2^ncode
};

class hash
{
public:
    hash();
    ~hash();
    param_hash malloc_(size_t nb, size_t nt);
    void realloc_code();
    void erase(const int what) {
        switch (what) {
        case CODE:
            for (size_t i=0; i<par_hash.nhashtable; ++i)
                l_table_hash[i].code = EMPTY_CODE;
            break;
        case DECODE:
            for (size_t i=0; i<par_hash.ntable; ++i)
                l_table_hash_decode[i].code = EMPTY_CODE;
            break;
        default:
            break;
        }
        free_index_table = MIN_N_TABLE - 1; // 258 - first code for otrezok more than 1 Bytes
        // 256 - clear code, 257 - end code
    }

    size_t addCode(const size_t prevcode, const uchar symbol, const size_t code) {
        auto index     = getIndex( ( static_cast<size_t>(symbol) << par_hash.ncode ) | prevcode ); // getIndex(key)
        auto curr_code = l_table_hash[index].code;
        size_t level     = 0;
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

    bool findCode(size_t prevcode, uchar symbol, size_t& code) {
        if (!(prevcode ^ EMPTY_CODE)) {
            code = symbol;
            return true;
        }
        auto index         = getIndex( ( static_cast<size_t>(symbol) << par_hash.ncode) | prevcode );
        auto curr_code     = l_table_hash[index].code;
        auto curr_symbol = l_table_hash[index].symbol;
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

    void addWord(size_t prevcode, uchar symbol) { // code EMPTY_CODE - empty symbol
        if (!(prevcode ^ EMPTY_CODE))
            return; // no add one-symbol words

        l_table_hash_decode[free_index_table].code = prevcode;
        l_table_hash_decode[free_index_table].symbol = symbol;
        ++free_index_table;
    }

    size_t getWord(const size_t code, uchar *u) {
        if (!(code ^ EMPTY_CODE))
            return 0;
        if (!(code >> BYTE)) { // 0 <= code < NBYTE
            u[0] = static_cast<uchar>(code);
            return 1;
        }
        auto i          = code;
        size_t lengthWord = 0;
        while (i ^ EMPTY_CODE) {
            if (!(l_table_hash_decode[i].code ^ EMPTY_CODE)) {
                u[lengthWord] = static_cast<uchar>(i);
                ++lengthWord;
                break;
            }
            u[lengthWord] = l_table_hash_decode[i].symbol;
            ++lengthWord;
            i             = l_table_hash_decode[i].code;
        }
        // reverse of vector u
        for (size_t i=0; i<(lengthWord >> 1); ++i) // 6 => 3; 5 => 2; ...
            std::swap(u[i], u[lengthWord - 1 - i]);

        return lengthWord;
    }

    bool isFullTable() const {
        return (free_index_table == par_hash.ntable);// for 16-bit code   max used index is 2^16 - 1 = 65535 = 0xffff
    }

    size_t getIndex(size_t key) const {
        // hash(key) = floor(length_hash_table*{key*A}), A = phi, 0 < A < 1; Knuth D.
        return ( ((key * multiplicator) & ((1 << par_hash.nkey) - 1)) >> (BYTE - 1) );
    }

    size_t getFreeCode() const {
        return free_index_table;
    }
private:
    void setParam(size_t nc, size_t nt);
    size_t bound(size_t a, size_t b, size_t c) const;

    void calc_phi();

    element_hash *l_table_hash; // таблица ключей, кодирование (упаковка)
    // при кодировании в поле .code хранятся код (не более 16 бит) и код предыдущего отрезка (не более 16 бит)
    // индекс вычисляется на основании кода предыдущего отрезка и текущего символа по формуле getIndex() в addCode() или findCode()
    element_hash *l_table_hash_decode; // таблица ключей, декодирование (распаковка);
    // при декодировании код отрезка - это индекс таблицы, в которой хранится код предыдущего отрезка (.code) и символ (.symbol)
    param_hash par_hash;
    size_t free_index_table;
    size_t max_current_level;  // текущий уровень; бОльший уровень мЕнее верояен за счёт хэш-функции

    //        smult               = (int)( 0.6180339887498948482045868*(double)(1 << par_hash.nkey) ); // floor(phi * 2^nkey) по возможности близко, так,
    size_t multiplicator;
    size_t PHI[MAX_CODE_BIT + 1 - MIN_CODE_BIT] = {}; //= {81005, 162013, 324027, 648055, 1296111, 2592221, 5184443, 10368889}; // что НОД(smult, 2^nkey) = 1, nkey = 9, 10 ... 16
    static constexpr auto phi0 = 0.6180339887498948482045868;
};

}

#endif // HASH_H
