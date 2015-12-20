//-----------------------------------------------------------------------------
// llexec - Execute command on files provided by DirectoryScan
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
struct LLExecConfig  : public LLConfig
{
};

class LLExec : public LLBase
{
public:
    LLExec();

    static int StaticRun(const char* cmdOpts, int argc, const char* pDirs[]);
    int Run(const char* cmdOpts, int argc, const char* pDirs[]);

    bool			m_force;
    bool			m_quote;

    uint			m_appendCount;
    uint			m_gotAppCount;
    uint			m_waitBeforeMsec;
	uint			m_waitTimeoutMsec;
	uint			m_waitAfterMsec;

	LLSup::SizeOp	m_exitOp;
	LONGLONG		m_exitValue;
	uint			m_countExitOkayCnt;
	uint			m_countExitShowCnt;
	uint            m_loopCnt;

    std::string		m_lastCmd;
    std::string		m_command;
    const char*		m_pPattern;

    static LLExecConfig sConfig;
    LLConfig&       GetConfig();

protected:
    // Return 1 if output anything, 0 if nothing, -1 if error.
    virtual int ProcessEntry(const char* pDir, const WIN32_FIND_DATA* pFileData, int depth);
};


