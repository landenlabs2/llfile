// ------------------------------------------------------------------------------------------------
// LocaleFmt - Handle Locale (Language and Country) specific formatting.
//
// This was derived from publicly available source code XFormatNumber
// http://www.codeproject.com/script/Articles/ViewDownloads.aspx?aid=2492
//
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
// ------------------------------------------------------------------------------------------------

#pragma once

#include <Windows.h>
#include <winnls.h>

namespace LocaleFmt
{
// Format number and automatically add thousand separators.
//
//  Ex:
//      char str[20];    // big enough for final value with commas.
//      float    fValue;
//      int      dValue;
//      LONGLONG llvalue;
//      LocaleFmt::snprintf(str, ARRAYSIZE(str), "%f", fValue);
//      LocaleFmt::snprintf(str, ARRAYSIZE(str), "%.2f", fValue);
//      LocaleFmt::snprintf(str, ARRAYSIZE(str), "%d", dValue);
//      LocaleFmt::snprintf(str, ARRAYSIZE(str), "%lld", llValue);

extern char* snprintf(char* str, unsigned maxChar, char* fmt, ...);

// Get Locale number format (used internally by snprintf).
extern const NUMBERFMT& GetNumberFormat();
}

