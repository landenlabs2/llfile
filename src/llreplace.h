//-----------------------------------------------------------------------------
// llreplace - Replace files data
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
struct LLReplaceConfig  : public LLConfig
{
    LLReplaceConfig()
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

class LLReplace : public LLBase
{
public:
    LLReplace();
    virtual ~LLReplace() {}

    static int StaticRun(const char* cmdOpts, int argc, const char* pDirs[]);
    int Run(const char* cmdOpts, int argc, const char* pDirs[]);

    bool                m_force;
    std::string         m_replaceStr;
    lstring             m_tmpOutFilename;

    WIN32_FIND_DATA     m_fileData;     // Dir entry for last file found.
    LONGLONG            m_totalInSize;
    size_t              m_matchCnt;
    size_t              m_countInFiles;
	size_t				m_lineCnt;		// #lines read (-E=l)
    long                m_width;

    LLSup::StringList   m_matchFiles;
    LLSup::StringList   m_zipList;      // -z=[<filePat>][,<filePat>]
    bool                m_zipFile;      // -z

    static LLReplaceConfig sConfig;
    virtual LLConfig&   GetConfig();

protected:
    bool            m_showAttr;
    bool            m_showCtime;
    bool            m_showMtime;
    bool            m_showAtime;
    bool            m_showPath;         // show full file path
    bool            m_showSize;
    bool            m_allMustMatch;

    std::vector<char> m_tmpBuffer;

    struct GrepReplaceItem 
    {
        GrepReplaceItem() : m_haveFilePat(false), m_enabled(true), m_onMatch(true), m_replace(false) {}

        std::string         m_grepLineStr;
        std::tr1::regex     m_grepLinePat;      // -G=<grepPattern>
        std::string         m_replaceStr;       // -R=<replacePattern>
        std::tr1::regex     m_filePathPat;      // -M
		std::string         m_beforeStr;		// -Rbefore=<pattern>
		std::string         m_afterStr;         // -Rafter=<pattern>
        bool                m_haveFilePat;
        bool                m_enabled;
        bool                m_onMatch;
        bool                m_replace;          // -R specified, replaceStr may be empty.
    };
    std::vector<GrepReplaceItem> m_grepReplaceList;

protected:
    // Return 1 if output anything, 0 if nothing, -1 if error.
    virtual int ProcessEntry(const char* pDir, const WIN32_FIND_DATA* pFileData, int depth);

    unsigned FindReplace(const WIN32_FIND_DATA* pFileData);
    unsigned FindGrep();
    unsigned FindGrep(std::istream& in);
    void OutFileLine(size_t lineNum, unsigned matchCnt, size_t filePos = 0);
    bool BackupAndRenameFile();
    void RemoveTmpFile();
    void ColorizeReplace(const std::string& str); 
    void EnableFiltersForFile(const std::string& filePath);

    std::ofstream& OpenOutput(std::ofstream& out, std::ifstream& in, std::streampos& inPos);

    int ZipListArchive(const char* zipArchiveName);
    int ZipReadFile(const char* zipFilename,
        const char* fileToExtract, const char* password);
};
