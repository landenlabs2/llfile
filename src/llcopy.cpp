//-----------------------------------------------------------------------------
// llcopy - Copy files provided by DirectoryScan object
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

#include <iostream>
#include <iomanip>
#include <assert.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>

#include "llcopy.h"

extern int CopyCompressed(const char* srcFile, const char* m_dstPath);

// ---------------------------------------------------------------------------

static const char sHelp[] =
" Copy " LLVERSION "\n"
"\n"
"  !0eSyntax:!0f\n"
"    [<switches>] <fromPattern>...  <toPattern>\n"
"\n"
"  !0eDestination controls:!0f\n"
"   -a                  ; Append to destination \n"
"   -cw or -cr          ; Change destination permission to writeable or readonly\n"
"   -f                  ; Force copy even if destination is set to read only\n"
"   -n                  ; No copy, just echo command\n"
"   -o                  ; Only copy if destination is older than source (modify time)\n"
"   -O                  ; Okay to over write existing destination regardless of time\n"
"   -p                  ; Prompt before copy\n"
"   -pp                 ; Prevent prompt on Override or readonly, just skip file \n"
"   -P=<srcPathPat>     ; Optional regular expression pattern on source files\n"
"   -z                  ; Un/Compress (compressed file ends in .lzo) \n"
"  !0eSource selection:!0f\n"
"   -A=[nrhs]           ; Limit files by attribute (n=normal r=readonly, h=hidden, s=system)\n"
"   -D                  ; Copy directory tree if destination does not exist\n"
"   -r                  ; Recurse starting at from directory, matching file pattern\n"
"   -I=<file>           ; Read list of files from <file> or - for stdin \n"
"   -W                  ; Watch (follow) source file \n"
"   -F=<filePat>,...    ; Limit to matching file patterns \n"
"   -X=<pathPat>,...    ; Exclude patterns  -X=*.lib,*.obj,*.exe\n"
"                       ;  No space in patterns. Pattern applied against fullpath\n"
"                       ;  So *\\ma will exclude a directory ma or file ma \n"
"   -Z<op><value>       ; siZe op=(Greater|Less|Equal) value=num<units G|M|K>, ex -Zg100M \n"
"  !0eMisc options:!0f\n"
"   -B=c                ; Add additional field separators to use with #n selection\n"
"   -q                  ; Quiet, don't echo command (echo on by default)\n"
"   -1=<output>         ; Redirect output to file \n"
"\n"
"  !0eWhere fromPattern is:!0f\n"
"    <file|Pattern> \n"
"    [<directory|pattern> \\]... <file|Pattern|#n> \n"
"\n"
"  !0ePattern:!0f\n"
"      * = zero or more characters\n"
"      ? = any character\n"
"\n"
"  !0eDestination Pattern:!0f\n"
"     *n = Replace destination with source filename wildcard 'n', 1 is first \n"
"          Ex:  lc *\\file-*-.*   file.*1 \n"
"\n"
"     #n = replace destination with source directory 'n'\n"
"          where n is subdirectories n=1 first from left\n"
"          -n counts from right\n"
"\n"
"     #l = where 'l' is a letter:\n"
"         b = base name\n"
"         e = extension\n"
"         n = base.ext \n"
"         l, u, c = lower, upper, captialize name for later use\n"
"\n"
"  !0eExample:!0f\n"
"      lc *      e:\\tmp\\            \n"
"      lc *.obj  e:\\tmp\\            \n"
"      lc -r c:\\src\\*.obj d:\\src\\*.obj  e:\\dst\\  \n"
"      lc c:\\t*\\src\\*  e:\\tmp\\    \n"
"     ; Copy files from all val* directories and flatten into single destination directory \n"
"      lc -r .\\val*\\* dst\\ \n"
"\n"
"     ; Use # to select subdirectories or parts of source path name. Also to convert case.\n"
"     ; When any # appear in line, use #n, #b, #e to specify target filename\n"
"      lc c:\\foo\\bar\\*\\*.DAT  d:\\far\\#-1\\#l#n \n"
"      lc c:\\foo\\bar\\*.dat   d:\\far\\#b.tmp   ; change extension .dat to .tmp\n"
"\n"
"     ; Remove underscores from files\n"
"      lc -B_ c:\\*_*_*.dat   d:\\*1*2*3.dat\n"
"\n"
"     ; Copy full recursive directory\n"
"      lc -r srcDir  backup\\*   ; With * clone directory tree into backup \n"
"      lc -r srcDir  backup\\    ; Without * flatten files into backup \n"
"     ; Copy just dat files\n"
"      lc -r srcDir\\*.dat  backup\\* \n"
"     ; Copy and compress files \n"
"      lc -rz \\*.dat  compressed\\* \n"
"     ; Uncompress files \n"
"      lc -rz compressed\\*.lzo  srcDir\\* \n"
"\n"
"\n";


LLCopyConfig LLCopy::sConfig;

// ---------------------------------------------------------------------------
LLCopy::LLCopy() :
    m_pPattern(0),
    m_toDir(0),
    m_force(false),
    m_older(false),
    m_overWrite(false),
    m_cancel(false),
    m_chmod(0),				//  copy and change permission, 0=noChange, _S_IWRITE  or _S_IREAD
    m_compress(false),
	m_append(false),		// -a
	m_follow(false),		// -W (watch)
    m_totalBytes(0)
{
    sConfigp = &GetConfig();
}
    
// ---------------------------------------------------------------------------
LLConfig& LLCopy::GetConfig() 
{
    return sConfig;
}

// ---------------------------------------------------------------------------
int LLCopy::StaticRun(const char* cmdOpts, int argc, const char* pDirs[])
{
    LLCopy LLCopy;
    return LLCopy.Run(cmdOpts, argc, pDirs);
}

// ---------------------------------------------------------------------------
int LLCopy::Run(const char* cmdOpts, int argc, const char* pDirs[])
{
    std::string str;

    // Initialize run stats
    size_t nFiles = 0;

    if (argc == 0 && *cmdOpts == '\0')
        cmdOpts = "?";

    // Parse options
    while (*cmdOpts)
    {
        switch (*cmdOpts)
        {
		case 'a':
			m_append = true;
			m_overWrite = true;
			break;
        case 'c':   // Chmod - change permission
            if (cmdOpts[1] == 'r')
                m_chmod = _S_IREAD;
            else if (cmdOpts[1] == 'w')
                m_chmod = _S_IWRITE;
            break;
        case 'f':
            m_force = true;
            break;
        case 'o':   // overwrite if destination is older than source
            m_older = true;
            break;
        case 'O':   // Overwrite
            m_overWrite = true;
            break;
        case 'z':   // compress
            m_compress = true;
            break;
		case 'W':	// watch (follow)
			m_follow = true;
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

    if (argc >= 1)
    {
        std::string toDir = pDirs[argc-1];
        argc--;


#if 1
        // Help user and add * to end of destination path.
        if (m_onlyAttr == FILE_ATTRIBUTE_DIRECTORY && toDir.find('*') == -1)
        {
            if (toDir.length() != 0 && toDir.back() != '\\')
                toDir += '\\';
            toDir += '*';
        }
#endif
        m_toDir = toDir.c_str();

        if (m_inFile.length() != 0)
        {
            if (argc >=0)
            {
                LLSup::ReadFileList(m_inFile.c_str(), EntryCb, this);
            }
            argc = 0;
        }

        if (argc >= 1)
        {
            // Iterate over dir patterns.
            for (int argn=0; argn < argc; argn++)
            {
                m_pPattern = pDirs[argn];
                m_dirScan.Init(pDirs[argn], NULL);
                m_subDirCnt = m_dirScan.m_subDirCnt;
                nFiles += m_dirScan.GetFilesInDirectory();
            }
        }
    }

    if (m_countInReadOnly != 0 && !m_force)
        LLMsg::Out() << m_countInReadOnly << " ReadOnly Ignored (use -f to Force and -O  to over-write)\n";
    if (m_countError != 0)
        LLMsg::Out() << m_countError << " Errors\n";
    if ( m_countIgnored != 0)
        LLMsg::Out() <<  m_countIgnored << " Ignored\n";

    DWORD milliSeconds = GetTickCount() - m_startTick;

    if (m_totalBytes > 0 && milliSeconds > 500)
    {
        LLMsg::PresentMseconds(milliSeconds);
        LLMsg::Out() << ", "
            << m_countOutFiles << "(Files), "
            << SizeToString(m_totalBytes) << ", "
            << std::setprecision(2)
            << std::fixed << (m_totalBytes / double(MB)) / (milliSeconds / 1000.0)
            << "(MB/sec)\n";
    }

	return ExitStatus((int)m_countOutFiles);
}

// ---------------------------------------------------------------------------
static DWORD WINAPI CopyProgressCb(
    LARGE_INTEGER totalFilesSize,
    LARGE_INTEGER totalCopied,
    LARGE_INTEGER streamSize,
    LARGE_INTEGER streamCopied,
    DWORD streamNumber,
    DWORD reason,
    HANDLE srcFile,
    HANDLE m_dstPath,
    LPVOID data)
{
    if (totalFilesSize.QuadPart < 100000)
        return PROGRESS_QUIET;

    LLCopy* pLLCopy = (LLCopy*)data;
    DWORD written;
    char  msg[40];
    DWORD msgLen = 0;
    const LONGLONG MB = 1LL << 20;
    double ticks = (GetTickCount() - pLLCopy->m_startTick) / 1000.0;

    double bytesPerSec = (ticks==0) ? 0.0 : ((pLLCopy->m_totalBytes + totalCopied.QuadPart) / MB) / ticks;

    msgLen = sprintf_s(msg, sizeof(msg), "#%u %.2f MBytes/Sec,  %3u%%   ",
        (uint)pLLCopy->m_countOutFiles, bytesPerSec,
        (uint)(totalCopied.QuadPart*100/totalFilesSize.QuadPart));

    CONSOLE_SCREEN_BUFFER_INFO consoleScreenBufferInfo;
    GetConsoleScreenBufferInfo(pLLCopy->ConsoleHnd(), &consoleScreenBufferInfo);
    WriteConsoleA(pLLCopy->ConsoleHnd(), msg, msgLen, &written, 0);
    SetConsoleCursorPosition(pLLCopy->ConsoleHnd(), consoleScreenBufferInfo.dwCursorPosition);

    // return (totalCopied.QuadPart < totalFilesSize.QuadPart) ? PROGRESS_CONTINUE : PROGRESS_QUIET;
    return PROGRESS_CONTINUE;
}

// ---------------------------------------------------------------------------
bool LLCopy::CopyFile(
	const char* srcFile,
	const char* dstFile,
	LPPROGRESS_ROUTINE  pProgreeCb,
	void* pData,
	BOOL* pCancel,
	DWORD flags)
{
	if (m_follow)
		return LLSup::CopyFollowFile(srcFile, dstFile, pProgreeCb, pData, pCancel, flags) == 0;
	else if (m_append)
		return LLSup::AppendFile(srcFile, dstFile, pProgreeCb, pData, pCancel, flags) == 0;

	return CopyFileEx(srcFile, dstFile, pProgreeCb, pData, pCancel, flags) != 0;
}

// ---------------------------------------------------------------------------
int LLCopy::ProcessEntry(
        const char* pDir,
        const WIN32_FIND_DATA* pFileData,
        int depth)      // 0...n is directory depth, -n end-of nth diretory
{
    int retStatus = sIgnore;

    if (depth < 0)
        return sIgnore;     // ignore end-of-directory

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

    // Populate m_dstPath, replace #n and *n patterns.
    //   If pFileData is not a directory then m_dstPath only contains pDstDir part.
    MakeDstPathEx(m_toDir, pFileData, m_pPattern);
#if 1
    if (m_isDir)
        return sIgnore;
#else
    uint  wantSubIdx;
    if ((wantSubIdx = m_dstPath.find('*')) != (uint)string::npos)
    {
        // If destination has '*', hopefully at end of filepath, replace it
        // with part of the source directory resulting from recursive scanning.
        int  srcIdx = string::npos;
        int  dirCnt = m_subDirCnt;
        while (dirCnt > 0 && (srcIdx = m_srcPath.find(LLPath::sDirChr(), srcIdx+1)) != (int)string::npos)
        {
            dirCnt--;
            srcIdx++;
        }

        m_dstPath.resize(wantSubIdx);       // Remove trailing '*'
        if (srcIdx == (int)string::npos)
            m_dstPath = LLPath::Join(m_dstPath, pFileData->cFileName);
        else
            m_dstPath = LLPath::Join(m_dstPath, m_srcPath + srcIdx);    // Add part of source path.

        // Create all but last, which is the file.
        LLSup::CreateDirectories(m_dstPath, 1, m_exec);
    }
    else
    {
        // m_dstPath = LLPath::Join(m_dstPath, pFileData->cFileName);
    }

    if (m_isDir)
    {
        int countFiles = 0;
        if ((m_onlyAttr & FILE_ATTRIBUTE_DIRECTORY) != 0)
        {
            // Special mode, only copy directory tree if destination does not exist.
            bool pathExists = LLPath::PathExists(m_dstPath);
#if 1
            if (!pathExists)
            {
                LLSup::CreateDirectories(m_dstPath, 0, m_exec);
            }
#else
            if (!pathExists || m_overWrite)
            {
                LONGLONG copiedBytes;
                copiedBytes = 0;
                countFiles = LLSup::CopyDirTree(m_srcPath, m_dstPath, m_echo, m_prompt, m_exec, copiedBytes);
                if (countFiles > 0)
                {
                    m_totalBytes += copiedBytes;
                    m_countOutFiles += countFiles;
                }
                else
                {
                    m_countError += -countFiles;
                }
            }

#endif
        }

        return countFiles;
    }
#endif

    BY_HANDLE_FILE_INFORMATION dstFileInfo;
    bool dstExists = LLSup::GetFileInfo(m_dstPath, dstFileInfo);
    DWORD dstAttributes = dstFileInfo.dwFileAttributes;

    // Get Destination age relative to source file
    //  > 0 newer
    //  < 0 older
    int dstAge = CompareFileTime(&dstFileInfo.ftLastWriteTime, &pFileData->ftLastWriteTime);
    // int dstAge = LLSup::FileTimeDifference(dstWriteTime, pFileData->ftLastWriteTime);
    const char* action = "";

	if (dstExists)
	{
		if (m_older && dstAge >= 0)
			return sIgnore;
		if (!m_overWrite)
			return sIgnore;
	}
	bool conflict = dstExists && ( !m_force && !LLPath::IsWriteable(dstAttributes));

    bool prompt = (m_promptAns != 'a') && (m_prompt || conflict);

    // If a Conflict and running with -PP to ignore prompting, then skip file.
    if (!conflict || m_promptAns != 'p')
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
                m_prompt = prompt = true;
                m_promptAns = 'n';
            }
        }

        if (m_echo || prompt)
        {
            static const char* sActionMsg[] = { "Upgrade ", "Replace ", "Downgrade ", "Copy " };
            uint actIdx = dstExists ? (dstAge < 0 ? 0 : (dstAge == 0 ? 1 : 2)) : 3;
            action = sActionMsg[actIdx];

            switch (action[0])
            {
            case 'U':
                SetColor(
                    LLPath::IsWriteable(dstAttributes) ? sConfig.m_colorUp : sConfig.m_colorROnly);
                break;
            case 'D':
                SetColor(
                    LLPath::IsWriteable(dstAttributes) ? sConfig.m_colorDown : sConfig.m_colorROnly);
                break;
            case 'C':
                break;
            case 'R':
                SetColor(
                    LLPath::IsWriteable(dstAttributes) ? sConfig.m_colorNormal : sConfig.m_colorROnly);
                break;
            }

            LLMsg::Out() << action;
            SetColor(sConfig.m_colorNormal);
            LLMsg::Out() << m_srcPath << " " << m_dstPath << std::endl;

            if (prompt && m_exec && PromptAnsQuit())
            {
                LLMsg::Out() << s_ignoreActionMsg;
                return sIgnore;
            }
        }

        if (m_exec)
        {
            if (dstExists && !LLPath::IsWriteable(dstAttributes))
                SetFileAttributes(m_dstPath, dstAttributes & ~FILE_ATTRIBUTE_READONLY);

            //
            // CopyFileEx() to monitor progress
            //
            m_cancel = false;
            const int sMaxRetry = 10;
            for (int retry = 0; retry < sMaxRetry; retry++)
            {
                m_copyTick = GetTickCount();

                if (m_compress)
                {
                     retStatus = CopyCompressed(m_srcPath, m_dstPath);
                     break;
                }
                // (LPPROGRESS_ROUTINE)
                else if (CopyFile(m_srcPath, m_dstPath, &CopyProgressCb, this, &m_cancel, 0) == false)
                {
                    if (retry+1 == sMaxRetry)
                    {
                        ErrorMsg() << "Failed to copy " << m_srcPath  << " to " << m_dstPath << std::endl;
                    }

                    DWORD error = GetLastError();
                    if (error == ERROR_NOT_ENOUGH_SERVER_MEMORY && retry+1 < sMaxRetry)
                    {
                        LLMsg::Out() << retry << "\r";
                        Sleep(1000 * retry);
                        continue;
                    }

                    if (error == ERROR_PATH_NOT_FOUND && retry == 0)
                    {
                        // Create all but last, which is the file.
                        if (LLSup::CreateDirectories(m_dstPath, 1, true))
                            m_countOutDir++;
                        continue;
                    }

                    if (error)
                    {
                        std::string errMsg;

                        ErrorMsg() << action << " Error:("
                            << error
                            << ") " << LLMsg::GetErrorMsg(error)
                            << " " << m_srcPath << " " << m_dstPath
                            << std::endl;

                        m_countError++;
                        break;
                    }
                }
                else
                {
                    retStatus = sOkay;
                    m_totalBytes += m_fileSize;

                    LLSup::SetFileModTime(m_dstPath, pFileData->ftLastWriteTime);

                    if (m_chmod != 0)
                    {
                        if (_chmod(m_dstPath, m_chmod) == -1)
                        {
                            perror(m_dstPath);
                        }
                    }

                    break;
                }
            } // end-of retry forloop
        }

        if (retStatus == sOkay)
            m_countOutFiles++;
    }
    else
    {
        m_countInReadOnly++;
    }

	return retStatus;	// sOkay = 1, means one file was copied.
}