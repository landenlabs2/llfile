//-----------------------------------------------------------------------------
// lldel - Delete files provided by DirectoryScan
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
//-----------------------------------------------------------------------------


#pragma once

#include <iostream>
#include <windows.h>

#include "llbase.h"

// ---------------------------------------------------------------------------
struct LLDelConfig  : public LLConfig
{
    LLDelConfig()
    {
        m_dirFieldSep   = " ";
        m_fzWidth       = 16;

        m_colorIgnored  = FOREGROUND_RED | FOREGROUND_GREEN;
        m_colorForced   = FOREGROUND_BLUE | FOREGROUND_GREEN;
        m_colorError    = FOREGROUND_RED;
        m_colorNormal   = m_defFgColor;     // FOREGROUND_INTENSITY;
        m_colorHeader   = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    }

    std::string m_dirFieldSep;
    int         m_fzWidth;

    WORD  m_colorIgnored;
    WORD  m_colorForced;
    WORD  m_colorError;
    WORD  m_colorNormal;
    WORD  m_colorHeader;
};

class LLDel : public LLBase
{
public:
    LLDel();

    static int StaticRun(const char* cmdOpts, int argc, const char* pDirs[]);
    int Run(const char* cmdOpts, int argc, const char* pDirs[]);

    bool        m_force;
    bool        m_undo;            // true, delete moves file/folder into recycle bin.

    uint        m_errQuitCnt;
    float       m_errSleepSec;

    LONGLONG    m_totalSize;

    static LLDelConfig sConfig;
    LLConfig&       GetConfig();

protected:
    bool        m_showAttr;
    bool        m_showCtime;
    bool        m_showMtime;
    bool        m_showAtime;
    bool        m_showPath;         // show full file path
    bool        m_showSize;

protected:
    // Return 1 if output anything, 0 if nothing, -1 if error.
    virtual int ProcessEntry(const char* pDir, const WIN32_FIND_DATA* pFileData, int depth);
};


