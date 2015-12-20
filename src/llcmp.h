//-----------------------------------------------------------------------------
// llcmp - Compare  two directories
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

#include "llbase.h"

// Forward declaration
struct DirectoryScan;
typedef bool (*CmpMatch)(const LLDirEntry* p1, const LLDirEntry* p2, unsigned levels);
typedef std::vector<LLDirEntry*> DirEntryList;

// ---------------------------------------------------------------------------
struct LLCmpConfig : public LLConfig
{
    LLCmpConfig()
    {
        m_colorDiff     = FOREGROUND_GREEN | FOREGROUND_RED;
        m_colorError    = FOREGROUND_RED;
        m_colorSkip     = FOREGROUND_GREEN;
        m_colorNormal   = m_defFgColor; // FOREGROUND_INTENSITY;
        m_colorLess     = FOREGROUND_RED;
        m_colorMore     = FOREGROUND_GREEN;
    }
    WORD  m_colorDiff;
    WORD  m_colorError;
    WORD  m_colorSkip;
    WORD  m_colorNormal;
    WORD  m_colorLess;
    WORD  m_colorMore;
};

// ---------------------------------------------------------------------------
class IgnoreChar
{
public:
	bool m_charSet[256];

	IgnoreChar(bool ignoreSpace=false, bool ignoreEOL=false)
	{ Init(ignoreSpace, ignoreEOL); }

	void Init(bool ignoreSpace, bool ignoreEOL)
	{
		memset(m_charSet, false, sizeof(m_charSet));
		if (ignoreSpace)
			m_charSet['\t'] = m_charSet[' '] = true;
		if (ignoreEOL)
			m_charSet['\n'] = m_charSet['\r'] = true;
	}

	// Return true if character should be ignored.
	bool Is(char c) const
	{  return m_charSet[c]; }
};

class LLCmp : public LLBase
{
public:
    LLCmp();
    enum CompareDataMode { eCompareBinary, eCompareText, eCompareSpecs };

    static int StaticRun(const char* cmdOpts, int argc, const char* pDirs[]);
    int Run(const char* cmdOpts, int argc, const char* pDirs[]);

    int ProcessEntry(const char* pDir, const WIN32_FIND_DATA* pFileData, int depth)
    {  assert(0); return sError; } // Not implemented.

    void DoCmp(CmpMatch cmpMatch);
    void SortDirEntries();

    static LLCmpConfig sConfig;
    LLConfig&       GetConfig();

    // User provided directory paths.
    std::vector<std::string> m_dirs;

protected:
    bool        m_showDiff;
    bool        m_showEqual;
    bool        m_showSkipLeft;
    bool        m_showSkipRight;
    bool        m_quiet;              // no stats, no color
    bool        m_progress;
    bool        m_showMD5hash;

    CompareDataMode  m_compareDataMode;

    enum MatchMode{ eNameAndData, ePathAndData };
    MatchMode       m_matchMode;

    LONGLONG        m_offset;           // file offset
    uint            m_quitByteLimit;    // number of different bytes to dump if in verbose mode.
    uint            m_levels;           // directory levels to include in path matching.
    uint            m_width;
    lstring         m_colSeparator;     // separator used between filespecs

    size_t          m_inFileCnt;        // Files found during directory scan.
    size_t          m_equalCount;
    size_t          m_diffCount;
    size_t          m_errorCount;
    size_t          m_skipCount[2];     // left and right
    size_t          m_delCount;
    size_t          m_diffLineCount;    // available with -t option.

    enum Delete { eNoDel, eMatchDel, eNoMatchDel, eGreaterDel, eLesserDel, eGreater2Del, eLesser2Del};
    Delete          m_delCmd;
    int             m_delFiles; // While files to delete in group: -1=all files, 0=first, 1=2nd 
    bool            m_noDel;

private:
    struct CompareInfo
    {
        LONGLONG    fileSize1;
        LONGLONG    fileSize2;
        LONGLONG    differAt;
        ULONG       diffCnt;
        DWORD       whereCnt[100];
    };

    enum CompareResult { eCmpSkip = -2, eCmpErr = -1, eCmpEqual = 0, eCmpDiff = 1 };

    typedef CompareResult (LLCmp::*CompareFileMethod)(
        const char*  filePath1,
        const char*  filePath2,
        LLCmp::CompareInfo& compareInfo,
        unsigned        quitAfter,
        std::ostream&  wout);

    CompareFileMethod compareFileMethod;

    // Return sIgnore, sOkay or sError
    int CompareFileSpecs(DirEntryList& dirEntryList);
    void ReportCompareFileSpecs() const;

    // Return sIgnore, sOkay or sError
    int CompareFileData(DirEntryList& dirEntryList);

    CompareResult CompareDataBinary(const char* filePath1, const char* filePath2,
        CompareInfo& comapreInfo, unsigned quitAfter, std::ostream&);
    CompareResult CompareDataText(const char* filePath1, const char* filePath2,
        CompareInfo& comapreInfo, unsigned quitAfter, std::ostream&);

    void DeleteCmpFile(const char* fileToDel);

	IgnoreChar          m_ignoreChar;	// Text compare

    double              m_minPercentChg;
    WIN32_FIND_DATA     m_minFileData0;
    WIN32_FIND_DATA     m_minFileDataN;
    std::string         m_minFile0;
    std::string         m_minFileN;
    double              m_maxPercentChg;
    WIN32_FIND_DATA     m_maxFileData0;
    WIN32_FIND_DATA     m_maxFileDataN;
    std::string         m_maxFile0;
    std::string         m_maxFileN;
    LONGLONG            m_sizeFile0;
    LONGLONG            m_sizeFileN;
};



