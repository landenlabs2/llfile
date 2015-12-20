//-----------------------------------------------------------------------------
// llmove - Move files provided by DirectoryScan object
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


// TODO - movefileex may support hardlink  (MOVEFILE_CREATE_HARDLINK)

#include <iostream>
#include <iomanip>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>

#include "llmove.h"

// Error codes in  WinError.h


// ---------------------------------------------------------------------------

static const char sHelp[] =
" Move " LLVERSION "\n"
"\n"
"  !0eSyntax:!0f\n"
"    [<switches>] <fromPattern>...  <toPattern>\n"
"\n"
"  !0eWhere switches are:!0f\n"
"   -A=[nrhs]           ; Limit files by attribute (n=normal r=readonly, h=hidden, s=system)\n"
"   -b                  ; Delay move until Boot \n"
"   -cw or -cr          ; Change destination permission to writeable or readonly\n"
"   -f                  ; Force move even if destination is set to read only\n"
"   -I=<file>           ; Read list of files from <file> or - for stdin \n"
"   -n                  ; No move, just echo command\n"
"   -o                  ; Only move if destination is older than source (modify time)\n"
"   -O                  ; Okay to over write existing destination regardless of time\n"
"   -r                  ; Recurse starting at from directory, matching file pattern\n"
"   -p                  ; Prompt before move\n"
"   -pp                 ; Prevent prompt on Override or readonly, just skip file \n"
"   -P=<srcPathPat>     ; Optional regular expression pattern on source files full path\n"
"   -q                  ; Quiet, don't echo command (echo on by default)\n"
"   -B=c                ; Add additional field separators to use with #n selectoin\n"
"   -D                  ; Only move directories\n"
"   -F                  ; Only move files\n"
"   -F=<filePat>,...    ; Limit to matching file patterns \n"
"   -L                  ; Create hard link instead of copy \n"
"   -X=<pathPat>,...    ; Exclude patterns  -X=*.lib,*.obj,*.exe\n"
"                       ;  No space in patterns. Pattern applied against fullpath\n"
"                       ;  So *\\ma will exclude a directory ma or file ma \n"
"   -Z<op><value>       ; siZe op=(Greater|Less|Equal) value=num<units G|M|K>, ex -Zg100M \n"
"   -?                  ; Show this help\n"
"\n"
"  !0eWhere fromPattern is:!0f\n"
"    <file|Pattern> \n"
"    [<directory|pattern> \\]... <file|Pattern|#n> \n"
"\n"
"  !0ePattern:!0f\n"
"      * = zero or more characters \n"
"      ? = any character \n"
"\n"
"  !0eDestination Pattern:\n"
"     *n = Replace destination with source filename wildcard 'n', 1 is first \n"
"          Ex:  lm *\\file-*-.*   file.*1 \n"
"\n"
"     #n = Replace destination with source directory 'n' \n"
"          where n is subdirectories n=1 first from left \n"
"          or -n counts from right\n"
"\n"
"     #<arg> = where <arg> is a letter\n"
"         f = fullpath\n"
"         d = directory\n"
"         b = base name\n"
"         e = extension\n"
"         n = base.ext \n"
"         l, u, c = lower, upper, captialize name for later use\n"
"\n"
"  !0eExample:!0f\n"
"      *      e:\\tmp\\            \n"
"      *.obj  e:\\tmp\\            \n"
"      -R c:\\tmp\\*.obj d:\\tmp\\*.obj  e:\\tmp\\  \n"
"      c:\\t*\\src\\*  e:\\tmp\\    \n"
"\n"
"     ; use # to select subdirectories and force name (#n) to lowercase (#l)\n"
"     ; when any # appear in line, use #n, #b, #e to specify target filename\n"
"      c:\\foo\\bar\\*\\*.DAT  d:\\far\\#-1\\#l#n \n"
"      c:\\foo\\bar\\*.dat   d:\\far\\#b.tmp   ; change extension .dat to .tmp\n"
"\n"
"     ; Move directories quickly \n"
"      -D  c:\\oldStuff\\*  d:\\newStuff\\* \n"
"\n"
"     ; remove underscores from files\n"
"      c:\\*_*_*.dat   d:\\*1*2*3.dat\n"
"     ; Read files from stdin \n"
"     ld -N | grep Foo | lm -i=- .\\backup\\#n  \n"
"\n"
"\n";


LLMoveConfig LLMove::sConfig;


// ---------------------------------------------------------------------------
LLMove::LLMove() :
    m_pPattern(0),
    m_toDir(0),
    m_force(false),
    m_older(false),
    m_overWrite(false),
    m_moveFlags(MOVEFILE_COPY_ALLOWED),
    m_totalBytes(0),
    m_chmod(0)             // change permission, 0=noChange, _S_IWRITE  or _S_IREAD
{
    sConfigp = &GetConfig();
}

// ---------------------------------------------------------------------------
LLConfig& LLMove::GetConfig() 
{
    return sConfig;
}

// ---------------------------------------------------------------------------
int LLMove::StaticRun(const char* cmdOpts, int argc, const char* pDirs[])
{
    LLMove LLMove;
    return LLMove.Run(cmdOpts, argc, pDirs);
}

// ---------------------------------------------------------------------------
int LLMove::Run(const char* cmdOpts, int argc, const char* pDirs[])
{
    lstring str;

    // Initialize run stats
    size_t nFiles = 0;

    if (argc == 0 && *cmdOpts == '\0')
        cmdOpts = "?";

    // Parse options
    while (*cmdOpts)
    {
        switch (*cmdOpts)
        {
        case 'c':   // Chmod - change permission
            if (cmdOpts[1] == 'r')
                m_chmod = _S_IREAD;
            else if (cmdOpts[1] == 'w')
                m_chmod = _S_IWRITE;
            break;
        case 'b':   // delay until boot
            m_moveFlags |= MOVEFILE_DELAY_UNTIL_REBOOT;
            break;
        case 'f':
            m_force = true;
            break;
        case 'L':    // hardlink
            m_moveFlags |= MOVEFILE_CREATE_HARDLINK;
            break;
        case 'o':
            m_older = true;
            break;
        case 'O':
            m_overWrite = true;
            m_moveFlags |= MOVEFILE_REPLACE_EXISTING;
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


    if (m_inFile.length() != 0)
    {
        if (argc >= 1)
        {
            m_toDir = pDirs[argc-1];
            argc--;

            LLSup::ReadFileList(m_inFile.c_str(), EntryCb, this);
        }
        argc = 0;
    }

    if (argc >= 2)
    {
        m_toDir = pDirs[argc-1];
        argc--;

        // Iterate over dir patterns.
        for (int argn=0; argn < argc; argn++)
        {
            m_pPattern = pDirs[argn];
            m_dirScan.Init(pDirs[argn], NULL, false);
            nFiles += m_dirScan.GetFilesInDirectory();
        }
    }

    if (m_countInReadOnly != 0)
        LLMsg::Out() << m_countInReadOnly << " ReadOnly Ignored (use -f to Force)\n";
    if (m_countError != 0)
        LLMsg::Out() << m_countError << " Errors\n";
    if (m_countIgnored != 0)
        LLMsg::Out() << m_countIgnored << " Ignored\n";

    DWORD milliSeconds = (GetTickCount() - m_startTick);

    if (m_totalBytes > 0 && milliSeconds > 500)
    {
        LLMsg::PresentMseconds(milliSeconds);
        LLMsg::Out() << ", "
            << m_countOutFiles << "(Files), "
            << SizeToString(m_totalBytes) << ", "
            << std::setprecision(2)
            << std::fixed << ((m_totalBytes / double(MB)) / (milliSeconds / 1000.0))
            << "(MB/sec)\n";
    }

	return ExitStatus((int)m_countOutFiles);
}

// ---------------------------------------------------------------------------
int LLMove::ProcessEntry(
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
    if (m_isDir &&  m_onlyAttr != FILE_ATTRIBUTE_DIRECTORY)
        return sIgnore;

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
            static const char* sActionMsg[] = { "Upgrade ", "Replace ", "Downgrade ", "Move " };
            uint actIdx = dstExists ? (dstAge < 0 ? 0 : (dstAge == 0 ? 1 : 2)) : 3;
            action = sActionMsg[actIdx];

            switch (action[0])
            {
            case 'U':	// upgrade
                SetColor(
                    LLPath::IsWriteable(dstAttributes) ? sConfig.m_colorUp : sConfig.m_colorROnly);
                break;
            case 'D':	// downgrade
                SetColor(
                    LLPath::IsWriteable(dstAttributes) ? sConfig.m_colorDown : sConfig.m_colorROnly);
                break;
            case 'C':
            case 'R':	// replace
                SetColor(
                    LLPath::IsWriteable(dstAttributes) ? sConfig.m_colorNormal : sConfig.m_colorROnly);
                break;
            }

            LLMsg::Out() << action;
            if ((m_moveFlags & MOVEFILE_CREATE_HARDLINK) != 0)
                LLMsg::Out() << "(hardlink) ";
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
            retStatus = sOkay;
            if (dstExists && !LLPath::IsWriteable(dstAttributes))
                SetFileAttributes(m_dstPath, dstAttributes & ~FILE_ATTRIBUTE_READONLY);


            const int sMaxRetry = 2;
            for (int retry = 0; retry < sMaxRetry; retry++)
            {
                //
                // TODO - movefileex may support hardlink  (c)
                //
                if (MoveFileEx(m_srcPath, m_dstPath, m_moveFlags) == 0)
                {
                    DWORD error = GetLastError();

                    if (error == ERROR_NOT_ENOUGH_SERVER_MEMORY && retry+1 < sMaxRetry)
                    {
                        Sleep(1000 * 1);
                        continue;
                    }

                    if (error == ERROR_PATH_NOT_FOUND && retry == 0)
                    {
                        // Create all but last, which is the file.
                        LLSup::CreateDirectories(m_dstPath, 1, true);
                        continue;
                    }

                    if (error)
                    {
                        std::string errMsg;

                        // TODO - report error in red 'color'
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
					BY_HANDLE_FILE_INFORMATION srcFileInfo;
					bool srcExists = LLSup::GetFileInfo(m_srcPath, srcFileInfo);
					if (srcExists)
					{
						bool showWarning = true;
						if (m_force)
						{
							DWORD srcAttributes = dstFileInfo.dwFileAttributes;
							SetFileAttributes(m_srcPath, srcAttributes & ~FILE_ATTRIBUTE_READONLY);
							showWarning = (LLSup::RemoveFile(m_srcPath, true) != 0);
						}

						if (showWarning)
						{
							ErrorMsg() << action << " Warning: Unable to remove " << m_srcPath << std::endl;
							m_countInReadOnly++;
						}
					}

				 
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
            }
        }

        if (retStatus == sOkay)
            m_countOutFiles++;
    }
    else
    {
        m_countInReadOnly++;
        // TODO - optionally echo files ignored
    }

	return retStatus;
}

