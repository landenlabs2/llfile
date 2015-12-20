//-----------------------------------------------------------------------------
// lldir - Display directory information from provided by DirectoryScan object
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
struct LLDirConfig  : public LLConfig
{
    LLDirConfig()
    {
        m_fillNumDT     = '0';
        m_dateSep       = "/";
        m_timeSep       = ":";
        m_dateTimeSep   = " ";
        m_refDateTime   = false;
        m_numDateTime   = true;
        m_fileTimeLocalTZ = true;
        m_attrChr       = "RHSDAdNTSrCOIEV";
        m_attrFiller    = '.';
        m_dirFieldSep   = " ";
        m_dirBeg        = "";  //  "\n";
        m_fzWidth       = 16;

        m_colorDir      = FOREGROUND_INTENSITY + FOREGROUND_YELLOW;
        m_colorROnly    = FOREGROUND_INTENSITY + FOREGROUND_RED;
        m_colorHidden   = FOREGROUND_INTENSITY + FOREGROUND_BLUE;
        m_colorNormal   = m_defFgColor;     // FOREGROUND_INTENSITY;
        m_colorHeader   = FOREGROUND_INTENSITY + FOREGROUND_WHITE;
    }
    char        m_fillNumDT;
    char*       m_dateSep;
    char*       m_timeSep;
    char*       m_dateTimeSep;
    bool        m_refDateTime;
    bool        m_numDateTime;
    bool        m_fileTimeLocalTZ;  // Present time in Local Timezone
    char*       m_attrChr;
    char        m_attrFiller;
    std::string m_dirFieldSep;
    char*       m_dirBeg;
    int         m_fzWidth;

    WORD        m_colorDir;
    WORD        m_colorROnly;
    WORD        m_colorHidden;
    WORD        m_colorNormal;
    WORD        m_colorHeader;
};

class LLDir : public LLBase
{
public:
    LLDir();

    static int StaticRun(const char* cmdOpts, int argc, const char* pDirs[]);
    int Run(const char* cmdOpts, int argc, const char* pDirs[]);

    // Return 1 if output anything, 0 if nothing, -1 if error.
    int DirFile(const char* fullFile, int depth=0);


    void ShowStream(bool b)
    { m_showStream = b; };

    bool Quiet() const
    { return m_quiet; }

    DWORD WatchBits() const
    { return m_WatchBits; }

    static LLDirConfig    sConfig;
    LLConfig&       GetConfig();

    static LONG            sWatchCount;
    static LONG            sWatchLimit;

protected:
    bool        m_showAttr;
    bool        m_showCtime;
    bool        m_showMtime;
    bool        m_showAtime;
    bool        m_showStuff;
    bool        m_showSize;
    bool        m_showHeader;
    bool        m_showAttrWords;
    bool        m_showLink;
    bool        m_showFileId;
    bool        m_quiet;            // no stats, no color
    bool        m_showWide;
    bool        m_showUsage;
    bool        m_showUsageExt;
    bool        m_showUsageName;
    bool        m_showUsageDir;
    bool        m_watchDir;
    bool        m_showStream;       // show alternate data stream;
    bool        m_setTime;       // touch file and set access & modify times.
    bool        m_showSecurity;

    unsigned    m_showPath;         // show full file path

    DWORD       m_chmod;            // change permission, 0=noChange, _S_IWRITE  or _S_IREAD
    DWORD       m_WatchBits;        // FILE_ACTION_ADDED, FILE_ACTION_REMOVED, FILE_ACTION_MODIFIED

    size_t      m_fileCount;
    size_t      m_dirCount;
    LONGLONG    m_totalSize;
    FILETIME    m_minCTime;
    FILETIME    m_maxATime;
    FILETIME    m_setUtcTime;

public:
    LLSup::StringList   m_invertedList;      // -V=<pathPat>[,<pathPat>

private:
    void ShowHeader(const char* pDir, const WIN32_FIND_DATA* pFileData);
    void WatchDir(int argc, const char* pDirs[]);

    std::ostream& ShowAttributes(
            std::ostream& out,
            const char * pDir,
            const WIN32_FIND_DATA& fileData,
            bool showWords);

    std::ostream& ShowAttributeAsChar(
            std::ostream& out,
            const char * pDir,
            const WIN32_FIND_DATA& fileData,
            bool showWords);

    // Return true if directory entry is a Soft Link (Reparse point), set outLinkPath to link target.
    bool GetSoftLink(
            std::string& outLinkPath,
            const char * pDir,
            const WIN32_FIND_DATA& fileData);

    bool ShowAlternateDataStreams(
            const char * pDir,
            const WIN32_FIND_DATA& fileData,
            int depth);

protected:
    // Return 1 if output anything, 0 if nothing, -1 if error.
    virtual int ProcessEntry(const char* pDir, const WIN32_FIND_DATA* pFileData, int depth);
};


