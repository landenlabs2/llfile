//-----------------------------------------------------------------------------
// llexec - Execute command on files provided by DirectoryScan object
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

//
//  TODO - parse output of command execute and optionally do something
//         based on results.
//
//         Also monitor commands exit status and optionally do something.
//
//  Possible implementation:
//
//   le -c p4 *.cpp -IfOutHas "contains client" -c del #f
//   le -c p4 *.cpp -IfStatusGt 0 -c del #f -else foo #f
//
//  -If < Out[Has|Is|Not|None] "text" | Status[Gt|Ge|Eq|Ne|Lt|Le] n>
//


#include <iostream>
#include <assert.h>
#include "llexec.h"

// ---------------------------------------------------------------------------

static const char sHelp[] =
" Execute " LLVERSION "\n"
"\n"
"  !0eSyntax:!0f\n"
"    [<switches>] -c \"command\" <Pattern>... \n"
"\n"
"  !0eWhere switches are:!0f\n"
"   !02-?!0f                  ; Show this help\n"
"   !02-A=!0f[nrhs]           ; Limit files by attribute (n=normal r=readonly, h=hidden, s=system)\n"
"   !02-B=!0fc                ; Add additional field separators to use with #n selection\n"
"   !02-D!0f                  ; Only process directories \n"
"   !02-e=!0f[lgen]<value>[L<loopCnt> ;  Echo if exit status less, greater, equal notEqual to value \n"
"                       ;   Optionally continue Looping executing loopCnt while exit status okay \n"
"   !02-F!0f                  ; Only process files \n"
"   !02-F=!0f<filePat>,...    ; Limit to matching file patterns \n"
"   !02-f!0f                  ; Force to execute on read-only files\n"
"   !02-G=!0f<grepPattern>    ; Execute only if file contains grepPattern \n"
"   !02-g=!0f<grepOptions>    ;  default is search entire file, +n=first n lines \n"
"   !02-I=!0f<file>           ; Read list of files from <file> or - for stdin \n"
"   !02-n!0f                  ; No execution\n"
"   !02-p!0f                  ; prompt before executing command\n"
"   !02-P=!0f<srcPathPat>     ; Optional regular expression pattern on source files full path\n"
"   !02-q!0f                  ; Quiet, default is echo command\n"
"   !02-Q!0f                  ; Quote argument before executing \n"
"   !02-r!0f                  ; Recurse into subdirectories\n"
"   !02-X=!0f<pathPat>,...    ; Exclude patterns  -X=*.lib,*.obj,*.exe\n"
"                       ;  No space in patterns. Pattern applied against fullpath\n"
"                       ;  So *\\ma will exclude a directory ma or file ma \n"
"   !02-V!0f                  ; verbose mode\n"
"   !02-W[bta]=!0fmseconds    ; Wait b=before,t=timeout,a=after milliseconds to exe commmand\n"
"                       ;  Default for timeout -Wt=20000 \n"
"   !02-Z=!0f<op><value>       ; siZe op=(Greater|Less|Equal) value=num<units G|M|K>, ex -Zg100M \n"
"\n Last argument:\n"
"   !02-C=!0f<count>          ; Number of files to append to command, default is 1 \n"
"   !02-c=!0f<command>        ; Execute command, default with file name append to command\n"
"                       ; Use #<arg> to select parts of filename\n"
"   !0eWARNING: Remember to quote command if it has spaces. !0f\n"
"            Also \\\" at the end of quoted command will not work, added another slash.\n"
"              llexec -c \"command arg1 \\arg2 \\arg3\\\\\"  *.dat\n"
"            Example to copy files and prefix destination name with save- \n"
"              llexec -F \"-c=xcopy #q#f#q #qsave-#n#q\"  \n"
"            Example to prompt to set readonly attributes using attrib command \n"
"              llexec -p \"-c=attrib +r \" \n"
"            Example to change directory and execute command \n"
"              llecec -D \"-c=cmd /c cd #d && p4 edit *\" .\\*\\ \n"
"\n"
"         #<arg> = where <arg> is a letter\n"
"         d = directory\n"
"         f = fullname include directories\n"
"         b = base name\n"
"         e = extension\n"
"         n = base.ext \n"
"         l, u, c = lower, upper, captialize name for later use\n"
"         q adds a quote \n"
"\n"
"  !0eWhere Pattern is:!0f\n"
"    <file|Pattern> \n"
"    [<directory|pattern> \\]... <file|Pattern|#n> \n"
"\n"
"  !0ePattern:!0f\n"
"      * = zero or more characters\n"
"      ? = any character\n"
"\n"
"  !0eExamples:!0f\n"
"   Pipe output of ld into le to execute perforce edit on files \n"
"     ld -F=*.java -r .\\ | le -I=- \"-c=p4 edit \"  \n"
"   Loop trying to delete a file \n  "
"   le -e=n0L100 \"-c=lr foo.dat\" \n"
"\n"
"\n";


LLExecConfig LLExec::sConfig;

// ---------------------------------------------------------------------------
LLExec::LLExec() :
    m_pPattern(0),
    m_force(false),
    m_quote(false),
    m_appendCount(1),
    m_gotAppCount(0),
	m_waitBeforeMsec(0),
    m_waitTimeoutMsec(20*1000),    // 20 second wait on execute creation.
	m_waitAfterMsec(0),
    m_command("cmd /c echo "),
	m_exitOp(LLSup::eOpNone),
	m_exitValue(0),
	m_countExitOkayCnt(0),
	m_countExitShowCnt(0),
	m_loopCnt(1)
{
    sConfigp = &GetConfig();
}
    
// ---------------------------------------------------------------------------
LLConfig& LLExec::GetConfig() 
{
    return sConfig;
}

// ---------------------------------------------------------------------------
int LLExec::StaticRun(const char* cmdOpts, int argc, const char* pDirs[])
{
    LLExec llExec;
    return llExec.Run(cmdOpts, argc, pDirs);
}

// ---------------------------------------------------------------------------
int LLExec::Run(const char* cmdOpts, int argc, const char* pDirs[])
{
    std::string str;

    // Initialize run stats
    size_t nFiles = 0;

    if (argc == 0 && *cmdOpts == '\0')
        cmdOpts = "?";

    // Setup default as needed.
    if (argc == 0 && strstr(cmdOpts, "I=") == 0)
    {
        const char* sDefDir[] = {"*"};
        argc  = sizeof(sDefDir)/sizeof(sDefDir[0]);
        pDirs = sDefDir;
    }

    // Parse options
    while (*cmdOpts)
    {
        switch (*cmdOpts)
        {
        case 'C':   // file count to append
            cmdOpts = LLSup::ParseNum(cmdOpts+1, m_appendCount, "Number of files to append to command, -C=<#files>");
            break;
        case 'c':
            cmdOpts = LLSup::ParseString(cmdOpts+1, m_command, "Command to execute, -c=<command>");
            break;
		case 'e':	// echo command/status if exit status [<|=|#|>] <number>
			cmdOpts = LLSup::ParseExitOp(cmdOpts + 1, m_exitOp, m_exitValue, m_loopCnt, "Echo if exit status is, -e=[lgen]<value>[L<loopCnt>");
			break;
        case 'f':
            m_force = true;
            break;
        case 'Q':
            m_quote = true;
            break;
        case 'W':
			switch (tolower(cmdOpts[1]))
			{
			case 'b':	// before
				cmdOpts = LLSup::ParseNum(cmdOpts+2, m_waitBeforeMsec, "Wait milli-second before start process, -Wb=<mseconds>");
				break;
			case 't':
				cmdOpts = LLSup::ParseNum(cmdOpts+2, m_waitTimeoutMsec, "milli-second timeout running process, -Wt=<mseconds>, default -W=20000");
				break;
			case 'a':
				cmdOpts = LLSup::ParseNum(cmdOpts+2, m_waitAfterMsec, "Wait milli-second after process, completes -W=<mseconds>");
				break;
			}
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


    VerboseMsg() << "LLExecute:\n"
              << "  Command:" << m_command << "\n"
              << (m_force ? " Force\n" : "")
              << (m_prompt ? " Prompt\n" : "")
              << (m_quote ? " Quote\n" : "")
              << (m_echo ? " Echo\n" : "Quit\n")
              << (m_exec ? "": " NoExecute\n")
              << " Separators:[" << m_separators << "]\n"
              << "\n";

    if (m_inFile.length() != 0)
    {
        LLSup::ReadFileList(m_inFile.c_str(), EntryCb, this);
    }

    // Iterate over dir patterns.
    for (int argn=0; argn < argc; argn++)
    {
        VerboseMsg() << argn << ": Dir:" << pDirs[argn] << "\n";
        m_pPattern = pDirs[argn];
        m_dirScan.Init(pDirs[argn], NULL);
        nFiles += m_dirScan.GetFilesInDirectory();
    }

    if (m_echo)
    {
        if (m_countError != 0)
            LLMsg::Out() << m_countError << " Errors\n";
        if (m_countIgnored != 0)
            LLMsg::Out() <<  m_countIgnored << " Ignored\n";
		if (m_countExitOkayCnt != 0)
			LLMsg::Out() << m_countExitOkayCnt << " ExitOkay\n";
		if (m_countExitShowCnt != 0)
			LLMsg::Out() << m_countExitShowCnt << " ExitShow\n";

        DWORD mseconds = GetTickCount() - m_startTick;
        LLMsg::PresentMseconds(mseconds);
    }  
	else if (m_exitOp != LLSup::eOpNone)
	{
		const char* sOp[] = { "", "Equal", "Greater", "Less", "NotEqual" };
		LLMsg::Out() << "Exit Test:" << sOp[m_exitOp] << " " << m_exitValue << std::endl;
		LLMsg::Out() << m_countExitOkayCnt << " ExitOkay\n";
		LLMsg::Out() << m_countExitShowCnt << " ExitShow\n";
	}

	return ExitStatus((int)nFiles);
}

#if 0
#include <shellapi.h>
//---------------------------------------------------------------------------
void Explore(HWND mainWnd, const char* doc)
{
    // ShellExecute arguments:
    //  HWND, lpOperation, lpFile, lpParameters, lpDirectory, nShowCmd
    HINSTANCE r = ShellExecute(mainWnd, "", doc, "", "", SW_SHOW);
}
#endif

// ---------------------------------------------------------------------------
int LLExec::ProcessEntry(
        const char* pDir,
        const WIN32_FIND_DATA* pFileData,
        int depth)      // 0...n is directory depth, -n end-of nth diretory
{
    int retStatus = sIgnore;

    if (depth < 0)
        return sIgnore;     // ignore end-of-directory

    if ( !FilterDir(pDir, pFileData, depth))
        return sIgnore;

    VerboseMsg() << m_srcPath << "\n";

    std::string command = m_command;
    bool srcAdded = false;
    if ( !m_isDir && Contains(command, '*'))
    {
        srcAdded = LLSup::ReplaceDstStarWithSrc(command, m_srcPath, m_pPattern, m_separators.c_str());
    }

    if (Contains(command, '#'))
    {
        srcAdded |= LLSup::ReplaceDstWithSrc(command, m_srcPath, m_separators.c_str());
    }

    // TODO - deal with this and using base class MakeDstPath()
    if (!srcAdded)
    {
        command += m_quote ? " \"" : " ";
        command += m_srcPath;

        if (m_quote)
            command += "\"";
    }

    if ( !FilterGrep())
        return sIgnore;

    if (m_appendCount > 1)
    {
        if (m_gotAppCount++ != 0)
        {
            command = m_lastCmd;
            command += m_quote ? " \"" : " ";
            command += m_srcPath;

            if (m_quote)
                command += "\"";
        }

        if (m_appendCount != m_gotAppCount)
        {
            m_lastCmd = command;
            return 0;
        }
        else
        {
            m_gotAppCount = 0;
        }
    }

    // if ((m_force || IsWriteable(pFileData->dwFileAttributes)))
    {
        if (m_echo || m_prompt)
        {
            LLMsg::Out() << command << std::endl;

            if (m_prompt && m_exec && PromptAnsQuit())
            {
                LLMsg::Out() << s_ignoreActionMsg;
                return sIgnore;
            }
        }

// TODO - use new RunCommands class to background execution.

        if (m_exec && !IsQuit())
        {
			uint loopCnt =  m_loopCnt;
			bool looping = true;
			while (looping && loopCnt-- > 0)
			{
				DWORD exitStatus;
				::Sleep(m_waitBeforeMsec);
				if (LLSup::RunCommand(command.c_str(), &exitStatus, m_waitTimeoutMsec) == true)
				{

					bool showExit = false;
					switch (m_exitOp)
					{
					case LLSup::eOpNone:
						showExit = (exitStatus != 0);
						break;
					case LLSup::eOpEqual:
						showExit = (exitStatus == m_exitValue);
						break;
					case LLSup::eOpNotEqual:
						showExit = (exitStatus != m_exitValue);
						break;
					case LLSup::eOpLess:
						showExit = (exitStatus < m_exitValue);
						break;
					case LLSup::eOpGreater:
						showExit = (exitStatus > m_exitValue);
						break;
					}

					if (showExit)
						m_countExitShowCnt++;
					else
					{
						m_countExitOkayCnt++;
						looping = false;
					}
					if (m_echo && showExit)
						LLMsg::Out() << " ExitStatus " << exitStatus << std::endl;
				}
				else
				{
					DWORD error = GetLastError();
					if (error)
					{
						std::string errMsg;

						ErrorMsg() << "Exec " << " Error:("
							<< error
							<< ") " << LLMsg::GetErrorMsg(error)
							<< " " << command
							<< std::endl;

						m_countError++;
					}
				}
				::Sleep(m_waitAfterMsec);
			}
        }

        LLMsg::Out() << std::endl;
    }
#if 0
    else
    {
        m_countReadOnly++;
    }
#endif

    return retStatus;
}

