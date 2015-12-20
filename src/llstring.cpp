// ---------------------------------------------------------------------------
// llstring - Simple string wrapper
//
// Author: Dennis Lang - 2015
// http://landenlabs.com/
//
// This file is part of LLFile project.
//
// ----- License ----
//
// Copyright (c) 2015 Dennis Lang
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is furnished to do
// so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// ---------------------------------------------------------------------------


#pragma once

#include <assert.h>
#include "llstring.h"


// ---------------------------------------------------------------------------
char* PadRight(char* str, size_t maxLen, size_t padLen, char padChr)
{
    if (padLen > maxLen)
        padLen = maxLen;

    size_t wlen = strlen(str);
    if (padLen > wlen)
    {
        memset(str + wlen, padChr, padLen - wlen);
        str[padLen] = '\0';
    }
    return str;
}

// ---------------------------------------------------------------------------
char* PadLeft(char* str, size_t maxLen, size_t padLen, char padChr)
{
    if (padLen > maxLen)
        padLen = maxLen;

    size_t wlen = strlen(str);
    if (padLen > wlen)
    {
        memmove(str + padLen-wlen, str, wlen+1);
        memset(str, padChr, padLen - wlen);
    }
    return str;
}

//  warning C4706: assignment within conditional expression, line #56
#pragma warning (disable : 4706)

//-----------------------------------------------------------------------------
char* TrimString(char* str)
{
    char* pDst = str;
    char* pSrc = str;

    while (*pSrc && isspace(*pSrc))
        pSrc++;         // remove leader

    while (*pDst++ = *pSrc++)
    {}

    pDst--;
    while (pDst != str && isspace(pDst[-1]))
        *--pDst = '\0'; // remove trailer

    return str;
}
