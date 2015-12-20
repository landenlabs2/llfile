//-----------------------------------------------------------------------------
// llbase - Support routines shared by LL files files.
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

#include "llsupport.h"
#include "lldirSort.h"
#include "llpath.h"
#include "llmsg.h"
#include "llerrMsgs.h"
#include "Handle.h"

#define HAVE_REGEX
#include <regex>
#include <limits.h>

const DWORD sMinus1 = (DWORD)-1;


//-----------------------------------------------------------------------------
class LLConfig
{
public:
    static const WORD FOREGROUND_WHITE = FOREGROUND_RED | FOREGROUND_GREEN |  FOREGROUND_BLUE;
    static const WORD FOREGROUND_YELLOW = FOREGROUND_RED | FOREGROUND_GREEN;

    WORD m_defFgColor;
    WORD m_defBgColor;
    bool m_colorOn;

    LLConfig()
    {
        CONSOLE_SCREEN_BUFFER_INFO consoleScreenBufferInfo;
        GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &consoleScreenBufferInfo);
        m_defFgColor = consoleScreenBufferInfo.wAttributes & 0x07;
        m_defBgColor = consoleScreenBufferInfo.wAttributes & 0x70;
        m_colorOn = true;
    }

    ~LLConfig()
    {
        // Restore default color.
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), m_defFgColor);
    }
};

#define  VerboseMsg() if (m_verbose) ErrorMsg()


//-----------------------------------------------------------------------------
class LLBase
{
public:
    LLBase() :
        m_isDir(false),
        m_fileSize(0),
        m_onlyAttr(0x000FFFFF),     // show only specific file,directory,... types (def: ALL)
        m_onlyRhs(0),               // 0 = all,  show only specific attribute (ReadOnly, Hidden, System, def:ALL)
        m_countInDir(0),            // dir scan count
        m_countInFile(0),           // file scan count
        m_countInReadOnly(0),
        m_onlySize(0),
        m_onlySizeOp(LLSup::eOpNone),
        m_timeOp(LLSup::eOpNone),
        m_separators("/\\"),        // characters to split path on, used by #
        m_screenWidth(80),          // used for wide output
        m_maxFileNameLength(4),
        m_fixedWidth(0),
        m_promptAns(' '),           // -pp sets it to 'p'
        m_prompt(false),            // -p
        m_echo(true),               // -q
        m_exec(true),               // -n
        m_verbose(false),           // -v
        m_byLine(false),
        m_limitOut(0),              // 0=all, number of lines to output
        m_depthLimit(0),            // 0=all, n=more, -n=less 
        m_countOut(0),              // output count
        m_countOutFiles(0),
        m_countOutDir(0),
        m_countError(0),
        m_countIgnored(0),
        m_startTick(GetTickCount())
    {
        m_dirScan.m_add_cb  = EntryCb;
        m_dirScan.m_cb_data = this;

        // sConfigp = &GetConfig();
    }

    virtual ~LLBase() 
    {}

    virtual LLConfig&  GetConfig() = 0;
    static LLConfig* sConfigp;


    static HANDLE ConsoleHnd();

    static void SetColor(WORD color);

    static const char s_ignoreActionMsg[];

    DWORD StartTick() const
    { return m_startTick; }

    // Parse base commands, return false is unknown command.
    bool LLBase::ParseBaseCmds(const char*& cmdOpts);

    virtual int ProcessEntry(const char* pDir, const WIN32_FIND_DATA* pFileData, int depth) = 0;

    static int EntryCb(
        void* cbData,
        const char* pDir,
        const WIN32_FIND_DATA* pFileData,
        int depth)      // 0...n is directory depth, -n end-of nth diretory
    {
        LLBase* pBase = (LLBase*)cbData;
        assert(pBase != NULL);
        return pBase->ProcessEntry(pDir, pFileData, depth);
    }

    // Common pre-filter, called by client ProcessEntry
    //  Filter on:
    //      m_onlyAttr      File or Directory, -F or -D
    //      m_onlyRhs       Attributes, -A=rhs
    //      m_includeList   File patterns,  -F=<filePat>[,<filePat>]...
    //      m_onlySize      File size, -Z op=(Greater|Less|Equal) value=num<units G|M|K>, ex -Zg100M
    //      m_excludeList   Exclude path patterns, -X=<pathPat>[,<pathPat>]...
    //      m_timeOp        Time, -T[acm]<op><value>  ; Test Time a=access, c=creation, m=modified\n
    //
    //  If pass, populate m_srcPath
    bool FilterDir(const char* pDir, const WIN32_FIND_DATA* pFileData, int depth);

    // Increment m_countOut and return true if execeeded output limit.
    bool IsQuit()
    {
        if (++m_countOut >= m_limitOut && m_limitOut != 0)
        {
            m_dirScan.m_abort = true;
            return true;
        }
        return false;
    }


    int ExitStatus(int status)
	{
		VerboseMsg() << "[ExitStatus] " << status << std::endl;
		return status;
	}

    // Populate m_dstPath, replace #n and *n patterns.
    //   If pFileData is not a directory then m_dstPath only contains pDstDir part.
    void MakeDstPath(const char* pDstDir, const WIN32_FIND_DATA* pFileData, const char* pPattern);
    void MakeDstPathEx(const char* pDstDir, const WIN32_FIND_DATA* pFileData, const char* pPattern);

    // Return true if  no grep specified or grep found a match.
    bool FilterGrep();

    // Return true if users wants to quit.
    bool PromptAnsQuit();

// protected:
    // Result of FilterDir
    bool                m_isDir;
    lstring             m_srcPath;
    lstring             m_dstPath;
    ULONGLONG           m_fileSize;

    // Filter settings
    DWORD               m_onlyAttr;         // -F or -D, limit to specific file,directory,... types (def: ALL)
    DWORD               m_onlyRhs;          // -A=[nrhs], limit to specific attribute (ReadOnly, Hidden, System, def:ALL)

    LONGLONG            m_onlySize;         // -Z<op><value>
    LLSup::SizeOp       m_onlySizeOp;       //    op=(Greater|Less|Equal) value=num<units G|M|K>, ex -Zg100M

    LLSup::StringList   m_excludeList;      // -X<pathPat>[,<pathPat>
    LLSup::StringList   m_includeList;      // -F[<filePat>][,<filePat>]

    // Example  -TmcEnow        Select if modify or create time Equal to now\n"
    //          -TmG-4.5        Select if modify time Greater than 4.5 hours ago\n"
    //          -TaL04:00:00    Select if access time Less than 4am today\n"
    LLSup::TimeFields   m_testTimeFields;   // -T[acm]<op><value>  ; Test Time a=access, c=creation, m=modified
    LLSup::SizeOp       m_timeOp;           //   op=(Greater|Less|Equal)  Value= now|+/-hours|yyyy:mm:dd:hh:mm:ss
    FILETIME            m_testTime;         //   Value time parts significant from right to left, so mm:ss or hh:mm:ss, etc

    lstring             m_separators;       // -B=<sep>...   Add additional field separators to use with #n selection
    std::string         m_inFile;           // -I=<inListOfFiles> or -I=-

    DirectoryScan       m_dirScan;
    LLDirSort           m_dirSort;

    // Output colors
    int                 m_screenWidth;      // Used for wide output
    int                 m_maxFileNameLength;
    int                 m_fixedWidth;

    // Misc controls
    char                m_promptAns;        // a/n/q/y or p if -pp specified
    bool                m_prompt;           // -p
    bool                m_echo;             // -q
    bool                m_exec;             // -n/-N
    bool                m_verbose;          // -v

    // Stats
    uint                m_countInDir;       // Dir scan count
    uint                m_countInFile;      // File scan count
    size_t              m_countInReadOnly;  // Readonly files skipped.
    uint                m_limitOut;         // Number of lines to output, 0=all
    long                m_depthLimit;       // 0=all, n=more, -n=less 
    size_t              m_countOut;         // Output line count
    size_t              m_countOutFiles;    // #files processed
    size_t              m_countOutDir;      // #directories processed.
    size_t              m_countError;       // #errors encountered
    size_t              m_countIgnored;     // #ignored due to No prompt
    DWORD               m_startTick;
    lstring             m_exitOpts;         // -E=<exitOptions>, see cmp

    bool                m_byLine;           // true if grep pattern contains ^ or $
    bool                m_backRef;

    std::tr1::regex     m_grepSrcPathPat;   // -P=<filePattern>
    std::tr1::regex     m_grepLinePat;      // -G=<fileContentPattern>
    std::string         m_grepLineStr;

    struct GrepOpt
    {
        GrepOpt() : 
            lineCnt(INT_MAX), matchCnt(INT_MAX), 
            beforeCnt(0), afterCnt(0)
        { 
            hideFilename = hideLineNum = hideMatchCnt = hideText = false;
            ignoreCase = false;
			repeatReplace = false;
            force = update = 0;
        }

        unsigned lineCnt;
        unsigned matchCnt;
        unsigned beforeCnt;
        unsigned afterCnt;

        bool     hideFilename;
        bool     hideLineNum;
        bool     hideMatchCnt;
        bool     hideText;
        bool     ignoreCase;
		bool     repeatReplace;		// -g=R  repeat replace until no more matches. 
        char     force;
        char     update;

        // Parse - return TRUE is valid, else FALSE
        bool Parse(const std::string& str);
     
    };
    GrepOpt  m_grepOpt;                 // g=<grepOptions>
};
