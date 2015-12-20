//-----------------------------------------------------------------------------
// lldel - Delete (remove) files provided by DirectoryScan object
//
// Author: Dennis Lang - 2015
// http://landenlabs/
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

#include <iostream>
#include <assert.h>
#include "lldel.h"
#include "Security.h"

// ---------------------------------------------------------------------------

static const char sHelp[] =
" Delete " LLVERSION "\n"
"\n"
"  !0eSyntax:!0f\n"
"    [<switches>] <Pattern>...\n"
"\n"
"  !0eWhere switches are:!0f\n"
"   -A=[nrhs]           ; Limit files by attribute (n=normal r=readonly, h=hidden, s=system)\n"
"   -D                  ; Del directories only\n"
"   -F                  ; Del files only\n"
"   -F=<filePat>,...    ; Limit to matching file patterns \n"
"   -f                  ; Force delete even if destination is set to read only\n"
"   -I=<infile>         ; Read filenames from infile or stdin if -\n"
"   -j                  ; Follow junctions (default: skip junctions)\n"
"   -n                  ; No delete, just echo command\n"
"   -p                  ; Prompt before delete\n"
"   -q                  ; Quiet, don't echo command (echo on by default)\n"
"   -r                  ; Recurse starting at from directory, matching file pattern\n"
"   -s                  ; show file size size\n"
"   -t[acm]             ; show Time a=access, c=creation, m=modified, n=none\n"
"   -u                  ; Enable undo, delete files and directories by moving to recycle bin\n"
"   -P=<srcPathPat>     ; Optional regular expression pattern on source files full path\n"
"   -Q=n                ; Quit after 'n' retries if error\n"
"   -T[acm]<op><value>  ; Test Time a=access, c=creation, m=modified\n"
"                       ; op=(Greater|Less|Equal)  Value= now|+/-hours|yyyy:mm:dd:hh:mm:ss \n"
"                       ; Value time parts significant from right to left, so mm:ss or hh:mm:ss, etc\n"
"                       ; Example   -TmcEnow    Show if modify or create time Equal to now\n"
"                       ; Example   -TmG-4.5   Show if modify time Greater than 4.5 hours ago\n"
"                       ; Example   -TaL04:00:00   Show if access time Less than 4am today\n"
"   -X=<pathPat>,...    ; Exclude patterns  -X=*.lib,*.obj,*.exe\n"
"                       ;  No space in patterns. Pattern applied against fullpath\n"
"                       ;  So *\\ma will exclude a directory ma or file ma \n"
"   -Z<op><value>       ; siZe op=(Greater|Less|Equal) value=num<units G|M|K>, ex -Zg100M \n"
"   -1=<output>         ; Redirect output to file \n"
"   -?                  ; Show this help\n"
"\n"
"  !0eWhere fromPattern is:!0f\n"
"    <file|Pattern> \n"
"    [<directory|pattern> \\]... <file|Pattern|#n> \n"
"\n"
"  Note - remember to end directory with \\ as in \n"
"       -rfq .\\foo-*\\   ; Recursively, forcefully quietly remove all files \n"
"                       ; and directories which start in a directory that \n"
"                       ; starts with foo- \n"
"       -rfq .\\foo-*    ; Similar but search ALL directories at current level \n"
"                       ; for files or directories which start with foo- \n"
"\n"
"  !0ePattern:!0f\n"
"      * = zero or more characters\n"
"      ? = any character\n"
"\n"
"\n";


LLDelConfig LLDel::sConfig;

// ---------------------------------------------------------------------------
LLDel::LLDel() :
    m_force(false),
    m_undo(false),      // true, delete moves file/folder into recycle bin.
    m_errQuitCnt(0),
    m_errSleepSec(0)
{
    m_showAttr  =
        m_showCtime =
        m_showMtime =
        m_showAtime =
        m_showPath =
        m_showSize  =  false;

    m_totalSize = 0;

    sConfigp = &GetConfig();
}
        
// ---------------------------------------------------------------------------
LLConfig& LLDel::GetConfig() 
{
    return sConfig;
}

// ---------------------------------------------------------------------------
int LLDel::StaticRun(const char* cmdOpts, int argc, const char* pDirs[])
{
    LLDel LLDel;
    return LLDel.Run(cmdOpts, argc, pDirs);
}

// ---------------------------------------------------------------------------
int LLDel::Run(const char* cmdOpts, int argc, const char* pDirs[])
{
    std::string str;

    // Initialize stuff
    size_t nFiles = 0;
    m_dirScan.m_skipJunction = true;
    bool sortNeedAllData = m_showSize;

    if (argc == 0 && *cmdOpts == '\0')
        cmdOpts = "?";

    // Parse options
    while (*cmdOpts)
    {
        switch (*cmdOpts)
        {
        case 'f':
            m_force = true;
            break;
        case 'j':
            m_dirScan.m_skipJunction = false;
            break;

        case 's':   // Toggle showing size.
            sortNeedAllData |= m_showSize = !m_showSize;
            // m_dirSort.SetSortData(sortNeedAllData);
            break;
        case 't':   // Display Time selection
            {
                bool unknownOpt = false;
                while (cmdOpts[1] && !unknownOpt)
                {
                    cmdOpts++;
                    switch (ToLower(*cmdOpts))
                    {
                    case 'a':
                        sortNeedAllData = m_showAtime = true;
                        break;
                    case 'c':
                        sortNeedAllData = m_showCtime = true;
                        break;
                    case 'm':
                        sortNeedAllData = m_showMtime = true;
                        break;
                    case 'n':   // NoTime
                        sortNeedAllData  = false;
                        m_showAtime = m_showCtime = m_showMtime = false;
                        break;
                    default:
                        cmdOpts--;
                        unknownOpt = true;
                        break;
                    }
                }
                // m_dirSort.SetSortData(sortNeedAllData);
            }
            break;

        case 'u':
            m_undo = true;
            break;
        case '?':
            Colorize(std::cout, sHelp);
            return sIgnore;
        default:
            if ( !ParseBaseCmds(cmdOpts))
                return sError;
        }

        // Advance to next parameter
        LLSup::AdvCmd(cmdOpts);
    }


    if (m_force && !LLSec::IsElevated())
    {
        ErrorMsg() << "Warning - you don't have elevated (admin) privileges\n";
    }

    if (m_inFile.length() != 0)
    {
        LLSup::ReadFileList(m_inFile.c_str(), EntryCb, this);
    }

    // Iterate over dir patterns.
    for (int argn=0; argn < argc; argn++)
    {
        m_dirScan.Init(pDirs[argn], NULL, m_dirScan.m_recurse);
        nFiles += m_dirScan.GetFilesInDirectory();
    }

    if (m_echo)
    {
        LLMsg::Out() << "\n";
        SetColor(sConfig.m_colorHeader);

        if (m_countInReadOnly != 0)
            LLMsg::Out() << m_countInReadOnly << " ReadOnly Ignored (use -f to Force)\n";
        if (m_countError != 0)
            LLMsg::Out() << m_countError << " Errors\n";
        if (m_countIgnored != 0)
            LLMsg::Out() << m_countIgnored << " Ignored\n";

        LLMsg::Out() << ";Deleted ";
        if (m_countOutDir != 0)
            LLMsg::Out() << m_countOutDir << " Directories, ";
        LLMsg::Out() << m_countOutFiles << " Files, " << m_totalSize << " bytes\n";

        // DWORD mseconds = GetTickCount() - m_startTick;
        // PresentMseconds(mseconds);
        SetColor(sConfig.m_colorNormal);
    }

	return ExitStatus((int)m_countOutFiles);
}

// ---------------------------------------------------------------------------
int LLDel::ProcessEntry(
        const char* pDir,
        const WIN32_FIND_DATA* pFileData,
        int depth)      // 0...n is directory depth, -n end-of nth diretory
{
    int retStatus = sIgnore;

    //  Filter on:
    //      m_onlyAttr      File or Directory, -F or -D
    //      m_onlyRhs       Attributes, -A=rhs
    //      m_includeList   File patterns,  -F=<filePat>[,<filePat>]...
    //      m_onlySize      File size, -Z op=(Greater|Less|Equal) value=num<units G|M|K>, ex -Zg100M
    //      m_excludeList   Exclude path patterns, -X=<pathPat>[,<pathPat>]...
    //      m_timeOp        Time, -T[acm]<op><value>  ; Test Time a=access, c=creation, m=modified\n
    //
    //  If pass, populate m_srcPath
    if ( !FilterDir(pDir, pFileData, depth))
        return sIgnore;

    if (m_isDir)
    {
        if (depth >= 0 && m_dirScan.m_recurse)
            return sIgnore;   // Ignore directories going in (depth > 0), wait until going out (depth < 0)

        LLPath::RemoveLastSlash(m_srcPath);
    }

    if (m_force || LLPath::IsWriteable(pFileData->dwFileAttributes))
    {
        // Check for keyboard input.
        if (_kbhit())
        {
            char c = GetCh();
            switch (c)
            {
            case 'e':
                m_echo = !m_echo;
                break;
            case 'p':   // pause
                 std::cout << " paused ";
                 while ( !GetCh())
                 { }
            }
        }

        if (m_echo || m_prompt)
        {
            std::cout << (m_isDir ? "rmdir " : "delete ");

            if (m_showAttr)
            {
                // ShowAttributes(LLMsg::Out(), pDir, *pFileData, false);
                LLMsg::Out() << LLDel::sConfig.m_dirFieldSep;
            }
            if (m_showCtime)
                LLSup::Format(LLMsg::Out(), pFileData->ftCreationTime) << LLDel::sConfig.m_dirFieldSep ;
            if (m_showMtime)
                 LLSup::Format(LLMsg::Out(), pFileData->ftLastWriteTime) << LLDel::sConfig.m_dirFieldSep;
            if (m_showAtime)
                 LLSup::Format(LLMsg::Out(), pFileData->ftLastAccessTime) << LLDel::sConfig.m_dirFieldSep;
            if (m_showSize)
                LLMsg::Out() << std::setw(LLDel::sConfig.m_fzWidth) << m_fileSize << LLDel::sConfig.m_dirFieldSep;

            std::cout << m_srcPath << std::endl;

            if (m_prompt && m_exec && PromptAnsQuit())
            {
                LLMsg::Out() << s_ignoreActionMsg;
                return sIgnore;
            }
        }

        if (m_exec)
        {
            int rmErr = -1;

            if ( !LLPath::IsWriteable(pFileData->dwFileAttributes))
                SetFileAttributes(m_srcPath, pFileData->dwFileAttributes & ~FILE_ATTRIBUTE_READONLY);

            if (m_isDir)
            {
                if (LLSup::RemoveDirectory(m_srcPath, m_undo) == -1)
                {
                    // ignore directory delete errors on entry attempt if recursing.
                    if (depth < 0 || m_dirScan.m_recurse == false)
                    {
                        rmErr = _doserrno;
                        DWORD werr = GetLastError();

                        if (rmErr == EIO)
                        {
                            LLSec::TakeOwnership(m_srcPath);
                            rmErr = LLSup::RemoveDirectory(m_srcPath, m_undo);
                            rmErr = _doserrno;
                            werr = GetLastError();
                        }
                        else if (werr == ERROR_SHARING_VIOLATION)
                        {
                            if (0 != MoveFileEx(m_srcPath, NULL, MOVEFILE_DELAY_UNTIL_REBOOT))
                            {
                                SetColor(LLDel::sConfig.m_colorError);
                                ErrorMsg() << "Object in use, Marked to delete on boot:" << m_srcPath << std::endl;
                                SetColor(LLDel::sConfig.m_colorNormal);
                                rmErr = 0;
                            }
                        }

                        if (rmErr != 0)
                        {
                            if (m_echo)
                            {
                                char errBuf[MAX_PATH];
                                strerror_s(errBuf, sizeof(errBuf), errno);
                                SetColor(LLDel::sConfig.m_colorError);
                                ErrorMsg() << " rmdir " << " error " << errBuf << " on " << m_srcPath << std::endl;
                                SetColor(LLDel::sConfig.m_colorNormal);
                            }
                            m_countError++;
                        }
                    }
                }
                else
                {
                    retStatus = sOkay;
                    m_countOutDir++;
                }
            }
            else
            {
                if ((rmErr = LLSup::RemoveFile(m_srcPath, m_undo)) == -1)
                {
                    rmErr = _doserrno;
                    DWORD werr = GetLastError();

                    if (rmErr == EIO)
                    {
                        LLSec::TakeOwnership(m_srcPath);
                        rmErr = LLSup::RemoveFile(m_srcPath, m_undo);
                    }

                    if (rmErr != 0)
                    {
                        if (m_echo)
                        {
                            std::string errmsg =LLMsg::GetErrorMsg(GetLastError());

                            char errBuf[MAX_PATH];
                            strerror_s(errBuf, sizeof(errBuf), _doserrno);
                            SetColor(LLDel::sConfig.m_colorError);
                            ErrorMsg() << " del " << " error " << errBuf << " " << errmsg << " on " << m_srcPath << std::endl;
                            SetColor(LLDel::sConfig.m_colorNormal);
                        }

                        for (DWORD tryCnt = m_errQuitCnt; rmErr == EACCES && tryCnt > 0; tryCnt--)
                        {
                            Sleep(DWORD(m_errSleepSec * 1000));
                            rmErr = LLSup::RemoveFile(m_srcPath, m_undo);
                        }

                        if (rmErr != 0)
                            m_countError++;
                    }
                }

                if (rmErr == 0)
                {
                    retStatus = sOkay;
                    m_countOutFiles++;
                    m_totalSize += m_fileSize;
                }
            }
        }
    }
    else
    {
        if (m_echo)
        {
            SetColor(LLDel::sConfig.m_colorIgnored);
            LLMsg::Out() << " ReadOnly:" << m_srcPath << std::endl;
            SetColor(LLDel::sConfig.m_colorNormal);
        }
        m_countInReadOnly++;
    }

	return retStatus;	// sOkay = 1, means one file or directory was deleted.
}

