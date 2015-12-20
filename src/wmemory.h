

#pragma once

// #include <tchar.h>
#if 0
// character length to Wide character byte length.
inline unsigned cToW(unsigned charLen)
{
    return charLen * sizeof(char);
}

inline void Wmemset(char* pDst, char value, unsigned charLen)
{
    char* pEnd = pDst + charLen;
    while (pDst != pEnd)
        *pDst++ = value;
}

inline void* Wmemmove(char* dst, const char* src, unsigned charLen)
{
    return memmove(dst, src, cToW(charLen));
}

inline errno_t Wmemmove_s(char* dst, unsigned maxDstCharLen, const char* src, unsigned charLen)
{
    return memmove_s(dst, cToW(maxDstCharLen), src, cToW(charLen));
}

inline errno_t Wmemcpy_s(char* dst, unsigned maxDstCharLen, const char* src, unsigned charLen)
{
    return memcpy_s(dst, cToW(maxDstCharLen), src, cToW(charLen));
}
#endif