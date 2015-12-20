//-----------------------------------------------------------------------------
// llmove - Move files provided by DirectoryScan
//
// Author: Dennis Lang - 2015
// http://home.comcast.net/~lang.dennis/
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

// Forward declaration
struct DirectoryScan;

// ---------------------------------------------------------------------------
struct LLMoveConfig  : public LLConfig
{
    LLMoveConfig()
    {
        m_colorUp       = FOREGROUND_GREEN;
        m_colorDown     = FOREGROUND_BLUE | FOREGROUND_GREEN;
        m_colorROnly    = FOREGROUND_RED;
        m_colorNormal   = m_defFgColor;     // FOREGROUND_INTENSITY;
        m_colorHeader   = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    }

    WORD  m_colorUp;
    WORD  m_colorDown;
    WORD  m_colorROnly;
    WORD  m_colorNormal;
    WORD  m_colorHeader;
};

class LLMove : public LLBase
{
public:
    LLMove();

    static int StaticRun(const char* cmdOpts, int argc, const char* pDirs[]);
    int Run(const char* cmdOpts, int argc, const char* pDirs[]);

    bool        m_force;
    bool        m_older;           // old copy if destination is older
    bool        m_overWrite;
    DWORD       m_moveFlags;
    DWORD       m_chmod;          // change permission, 0=noChange, _S_IWRITE  or _S_IREAD

    const char* m_toDir;
    const char* m_pPattern;

    LONGLONG    m_totalBytes;

    static LLMoveConfig sConfig;
    LLConfig&       GetConfig();

protected:
    // Return 1 if output anything, 0 if nothing, -1 if error.
    virtual int ProcessEntry(const char* pDir, const WIN32_FIND_DATA* pFileData, int depth);
};


