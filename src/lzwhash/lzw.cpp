#include "lzw_p.h"
#include "lzw.h"
#include <string.h>

using namespace llzz;
using namespace llzz::Internal;

LzwPrivate::LzwPrivate(const int nt, const int ncode)
    : N_TABLE(nt)
    , N_CODE(ncode)
    , fill_bit(0)
    , readed_bit(0)
    , buffer_need_flush(false)
    , buffer_is_old(true)
{
    hhss::param_hash par_hash = hashtable.malloc_(N_CODE, N_TABLE);
    N_CODE   = par_hash.ncode;
    N_TABLE  = par_hash.ntable;

    size_code_buff = getSizeCodeBuff(N_CODE);
    size_code_buff_bit = size_code_buff*8;
    memset(code_buff, 0, size_code_buff);
}

// OUT COMPRESS: (WRBLOCKS, DATA)
// SIZE IN BYTES: (4, SIZE_CODE_BUFF*WRBLOCKS)
int LzwPrivate::compress(uchar *in, const int Ns, uchar *out)
{    
    wr_bytes = N_SKEEP; // N Bytes for WRBLOCKS
    writeCode(&out[wr_bytes], CLEAR_CODE);
    hashtable.erase(hhss::CODE);
    int codeThis, codePrevStr = hhss::EMPTY_CODE;
    /////////////////////////////
    for (int i=0;i<Ns;++i) {
        const uchar symbol = in[i];        
        if (hashtable.findCode(codePrevStr, symbol, codeThis)) {
            codePrevStr = codeThis;
            continue;
        }
        writeCode(&out[wr_bytes], codePrevStr);
        codeThis = hashtable.getFreeCode();        
        hashtable.addCode(codePrevStr, symbol, codeThis);
        if (hashtable.isFullTable()) {
            hashtable.erase(hhss::CODE);
            writeCode(&out[wr_bytes], CLEAR_CODE);
        }
        codePrevStr = symbol;
    }
    writeCode(&out[wr_bytes], codePrevStr);
    writeCode(&out[wr_bytes], END_CODE);
    ///////////////////////////////////////
    // WRBLOCKS    
    int wrblocks = (wr_bytes - N_SKEEP) / size_code_buff;
    out[0]=wrblocks&0xff;
    out[1]=(wrblocks>>8)&0xff;
    out[2]=(wrblocks>>16)&0xff;
	out[3]=(wrblocks>>24)&0xff;
	//
    return wr_bytes;
}

int LzwPrivate::decompress(uchar *in, uchar *out)
{
    ///////////////////////////////////////////
    wr_bytes   = 0;
    rd_bytes   = N_SKEEP;
    buffer_is_old = true;
    int codeThis, codePrevStr = hhss::EMPTY_CODE;
    readCode(&in[rd_bytes], codeThis);
    hashtable.erase(hhss::DECODE);
    readCode(&in[rd_bytes], codeThis);
    //////////////////////////////////////////
    while (codeThis ^ END_CODE) {
        if (!(codeThis ^ CLEAR_CODE)) {
            hashtable.erase(hhss::DECODE);
            codePrevStr = hhss::EMPTY_CODE;
            readCode(&in[rd_bytes], codeThis);
        }
        if (!(codeThis ^ END_CODE))
            break;
        const int prev_wr_bytes = wr_bytes;
        if (codeThis < hashtable.getFreeCode()) // code in table
            wr_bytes += hashtable.getWord(codeThis, &out[wr_bytes]);
        else {
            wr_bytes += hashtable.getWord(codePrevStr, &out[wr_bytes]);
            out[wr_bytes]=out[prev_wr_bytes];
            ++wr_bytes;
        }
        hashtable.addWord(codePrevStr, out[prev_wr_bytes]);
        codePrevStr = codeThis;
        readCode(&in[rd_bytes], codeThis);
    }
    ////////////////////////////////////////
    return wr_bytes;
}

int LzwPrivate::getSizeCodeBuff(const int ncode)
{
    int sz=8;
    while (sz % ncode)
        sz+=8;
    return sz/8;
}

paramLZ LzwPrivate::getParamLZ()
{
    paramLZ p = {p.ntable = N_TABLE, p.nsymbols       = 0,
                 p.ncode  = N_CODE,  p.size_code_buff = size_code_buff};
    return p;
}

void LzwPrivate::flushBuffer(uchar *v)
{
    copyNCharsWithZeros(v, code_buff, size_code_buff); // обязательно с обнулением
    fill_bit  = 0;
    wr_bytes += size_code_buff;
}

void LzwPrivate::writeToBuffer(const uchar *v)
{
    copyNChars(code_buff, v, size_code_buff);
    readed_bit = 0;
    rd_bytes  += size_code_buff;
}

void LzwPrivate::writeCode(uchar *v, const int code)
{
    const int index_begin = (fill_bit>>3); //  integer div 8
    const int index_end   = ((fill_bit + N_CODE - 1)>>3);
    const int defect_begin = fill_bit & 7; // integer % 8
    const int shift_begin  = N_CODE - (8 - defect_begin);
    const int defect_end   = (fill_bit + N_CODE) & 7;
    const int shift_end    = (8 - defect_end) & 7; // always > 0

    code_buff[index_begin] |= (code >> shift_begin);

    int shift = (shift_begin > 0) ? shift_begin : -shift_begin;
    for (int i=index_begin+1;i<index_end;++i)
        code_buff[i] |= (code >> (shift -= 8));

    if (index_end ^ index_begin)
        code_buff[index_end] |= (code << shift_end);

    fill_bit += N_CODE;

    // buffer is fulled or (code is equal END_CODE)
    buffer_need_flush = !(fill_bit ^ size_code_buff_bit)||!(code ^ END_CODE);
    if (buffer_need_flush)
        flushBuffer(v);
}

void LzwPrivate::readCode(const uchar *v, int &code)
{
    if (buffer_is_old)
        writeToBuffer(v);

    const int index_begin = (readed_bit>>3); //  integer div 8
    const int index_end   = ((readed_bit + N_CODE - 1)>>3);
    const int defect_begin = readed_bit & 7; // integer % 8
    const int shift_begin  = N_CODE - (8 - defect_begin);
    const int defect_end   = (readed_bit + N_CODE) & 7;
    const int shift_end    = (8 - defect_end) & 7; // always > 0

    // zeros left defect_begin bits from code_buff[...] byte.
    const uchar tmp = (( 1 << (8 - defect_begin) ) - 1) & code_buff[index_begin];
    code = tmp << shift_begin;

    int shift = (shift_begin > 0) ? shift_begin : -shift_begin;
    for (int i=index_begin+1;i<index_end;++i)
        code += (code_buff[i] << (shift -= 8));

    if (index_end ^ index_begin)
        code += (code_buff[index_end] >> shift_end);

    readed_bit += N_CODE;
    buffer_is_old = !(readed_bit ^ size_code_buff_bit);
}


Lzw::Lzw(const int nt, const int ncode)
    : pLZW(new LzwPrivate(nt, ncode))
{
}

Lzw::~Lzw()
{
    if (pLZW)
        delete pLZW;
}

int Lzw::compress(unsigned char *in, const int Ns, unsigned char *out)
{
    return pLZW->compress(in, Ns, out);
}

int Lzw::decompress(unsigned char *in, unsigned char *out)
{
    return pLZW->decompress(in, out);
}

paramLZ Lzw::getParamLZ()
{
    return pLZW->getParamLZ();
}
