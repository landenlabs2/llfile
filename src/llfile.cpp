//-----------------------------------------------------------------------------
// llfile - File/Directory scan engine
// Author:  DLang   2008
// http://home.comcast.net/~lang.dennis/
//
// This program started as a simple directory wildcard experiment.
// It has grown to slowly replace several of the DOS commands, such as:
//
//      dir
//      copy
//      move
//      del
//      unix find -exec <commdn>
//
//  The program continues to be an experiment, so no guaranties on
//  reliability and completeness.
//
//  Since the program can have several identities, there are two ways to
//  select which command mode to run in.
//
//      1. Command switch -x[c|d|r|e|f|w|p|o|i]
//              c = copy
//              d = dir
//              e = execute
//              f = find
//              g = grep    ; Find or Find and Replace
//              i = install ; setup hardlinks between llfile.exe and its aliases
//              m = move
//              o = compare
//              p = printf
//              r = remove (delete)
//              u = uninstall   
//              w = where
//
//         Example:
//          llfile -xr  *.foo   ; remove files matching pattern *.foo
//          llfile -xd  *.foo   ; list directory of files matching *.foo
//
//      2. Rename or link the executable to command executable names.
//         Also Google fsutil to learn how to setup hardlinks, or run llcopy-install.bat
//
//              copy llfile.exe lldir.exe
//              copy llfile.exe llcopy.exe
//              copy llfile.exe llexec.exe
//              copy llfile.exe lldel.exe
//
//         When you use the specific executables, there name automatically
//         sets the default run mode.
//
//              lldir *.foo         ; runs directory command
//              lldel *.foo         ; runs delete command
//
//
//  The wildcard engine only support ? and *, but allows the wildcard
//  characters to appear multiple times, in the both the filename and directories.
//
//  The copy command also supports substitution of directories and name parts in
//  the destination file path.
//
//  Visit the source code for each command for specific help on that command or
//  run with the -? switch.
//
//      llfile -?       ; default help
//      llfile -xr -?   ; remove(delete) commands help
//      lldel -?        ; assuming lldel is a copy of llfile, show delete commands help
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



#include <stdio.h>
#include <ctype.h>
#include <fstream>
#include <iostream>
#include <iomanip>

#include "ll_stdhdr.h"
#include "dirscan.h"
#include "llbase.h"
#include "llcmp.h"
#include "llcopy.h"
#include "lldir.h"
#include "lldel.h"
#include "llexec.h"
#include "llmove.h"
#include "llfind.h"
#include "llprintf.h"
#include "llreplace.h"

#include <vector>
#include <crtdbg.h>   // memory/heap debugger

using namespace std;
// ---------------------------------------------------------------------------

static const char sHelp[] =
"LLFile " LLVERSION "\n"
"\n"
"  !0eDescription:!0f\n"
"    LLFile is a collection of directory and file processing commands using a flexible pattern matching engine.\n"
"\n"
"  !0eWeb Site:!0f\n"
"    http://home.comcast.net/~lang.dennis/  \n"
"\n"
"  !0eCommands:!0f\n"
"    Normally use use the -xi option to install the mapping to individual command short-cuts\n"
"      LLFile !02-xi!0f  \n"
"    or use llfile -x<c> to use a command directly, for example:\n"
"      LLFile !02-xg!0f !02-G=!0fHello *.txt \n"
"\n"
"  !0eShort-cut file commands:!0f\n"
"    ld   or lldir      ; List Directory info \n"
"    cmp  or llcmp      ; Compare files or directories \n"
"    lc   or llcopy     ; Copy files \n"
"    lr   or lldel      ; Remove files \n"
"    lm   or llmove     ; Move (rename) files \n"
"    lf   or llfind     ; Find files \n"
"    p    or printf     ; Print file names \n"
"    lg   or llgrep     ; Grep find and replace \n"
"    le   or llexec     ; Execute command on files \n"
"\n"   
"  !0eExample:!0f\n"
"    ld                 ; List Directory information \n"
"    cmp file1 file2    ; Compare two files \n"
"    lc *.txt *1.bak    ; Copy files \n"
"\n"
"  !0eCommand syntax (normally just use -xi and then use short-cut commands):!0f\n";


// Possible commands (not all are implemented)
enum Cmd { eNone, eCmp, eCopy, eDir, eDel, eExec, eFind, eMove, eWhere, ePrintf, eGrep, eInstall, eUninstall };
char* CmdName[] = { 
    "None", "Compare", "Copy", "Dir", "Delete", "Execute", "Find", "Move", "Where",
    "Printf", "Grep", "Install", "Uninstall" };

//  Association between names and commands.
struct CmdAlias
{
    const char* name;
    Cmd  cmd;
};

static CmdAlias sCmdAlias[] =
{
    {"llcmp",   eCmp},
    {"cmp",     eCmp},
    {"o",       eCmp},
    {"llcopy",  eCopy},
    {"lc",      eCopy},
    {"c",       eCopy},
    {"lldel",   eDel},
    {"lr",      eDel},    
    {"r",       eDel},
    {"lldir",   eDir},
    {"ld",      eDir},
    {"d",       eDir},
    {"llexec",  eExec},
    {"le",      eExec},
    {"e",       eExec},
    {"llmove",  eMove},
    {"lm",      eMove},
    {"m",       eMove},
    {"llfind",  eFind},
    {"lf",      eFind},
    {"f",       eFind},
    {"where",   eWhere},
    {"w",       eWhere},
    {"p",       ePrintf},
    {"printf",  ePrintf},
    {"llgrep",  eGrep},
 //   {"llreplace",eGrep},
    {"lg",      eGrep},
 //   {"grep",    eGrep},
    {"g",       eGrep},
    {"llinstall",eInstall},
    {"i",       eInstall},
	{"llunstall",eUninstall},
    {"u",       eUninstall},
    {NULL,      eNone},
};

// ---------------------------------------------------------------------------
// Use program name to find default command mode, see table above.

Cmd GetDefaultCommand(const char* argv0)
{
    Cmd cmd = eNone;

    // Isolate program name.  c:/dir/dir/Prognm.exe  =>  prognm.exe
    char prognm[MAX_PATH];
    strcpy_s(prognm, ARRAYSIZE(prognm), argv0);

    int nmLen = (int)strlen(prognm);
    while (nmLen > 0 && strchr("\\/", prognm[nmLen-1]) == NULL)
    {
        nmLen--;
        prognm[nmLen] = ToLower(prognm[nmLen]);
    }

    char* pPrognm = prognm + nmLen;

    for (int i=0; sCmdAlias[i].name != NULL; i++)
    {
        if (_strnicmp(pPrognm, sCmdAlias[i].name, strlen(sCmdAlias[i].name)) == 0)
        {
            cmd = sCmdAlias[i].cmd;
            break;
        }
    }

    // std::cout << "Program:" << pPrognm << " command:" << CmdName[cmd] << std::endl;
    return cmd;
}

// ---------------------------------------------------------------------------
int main(int argc, const char* argv[])
{
    std::ios_base::sync_with_stdio(false);

    int exitStatus = -1;
    Cmd cmd = GetDefaultCommand(argv[0]);

    const int sMaxCmdOpts = 512;
    char cmdOpts[sMaxCmdOpts];
    char* pCmdOpts = cmdOpts;
    char* pCmdEnd  = pCmdOpts + sMaxCmdOpts- 1;
    ClearMemory(cmdOpts, sizeof(cmdOpts));

#ifdef _DEBUG
        // Enable Microsoft built-in memory debugger
        _CrtSetReportMode( _CRT_WARN, _CRTDBG_MODE_FILE );
        _CrtSetReportFile( _CRT_WARN, _CRTDBG_FILE_STDOUT );
        _CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_FILE );
        _CrtSetReportFile( _CRT_ERROR, _CRTDBG_FILE_STDOUT );
        _CrtSetReportMode( _CRT_ASSERT, _CRTDBG_MODE_FILE );
        _CrtSetReportFile( _CRT_ASSERT, _CRTDBG_FILE_STDOUT );

        // Enable memory leak on exit
        _CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF | _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG));

        // All 'CrtCheckMemory()' after every malloc/free
        _CrtSetDbgFlag( _CRTDBG_CHECK_ALWAYS_DF | _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG));
        _CrtCheckMemory( );
#endif

    try
    {
        short argn;
        for (argn = 1; argn < argc; argn++)
        {
            if (*argv[argn] == '-' || *argv[argn] == '+')
            {
                const char* argvPtr;
                for (argvPtr = argv[argn]+1; *argvPtr && pCmdOpts < pCmdEnd; argvPtr++)
                {
                    switch (*argvPtr)
                    {
                    case '?':
                        if (cmd == eNone)
                        {
                            // TODO - Show sHelp for llfile before command list.
                            Colorize(std::cout, sHelp);
                            Colorize(std::cout, "    LLFile !02-x<c>!0f\n\n  !0eWhere commands can be:!0f\n");
                            for (int i=0; sCmdAlias[i].name != NULL; i++)
                                if (sCmdAlias[i].name[1] == '\0')
                                    std::cout << "     " << GreenFg << "-x" << sCmdAlias[i].name
                                        << "      "
                                        << WhiteFg << "; " << CmdName[sCmdAlias[i].cmd]
                                        << "\n";
                            return 0;
                        }
                        *pCmdOpts++ = *argvPtr;
                        break;

                    case 'x':   // -x<c>
                        if (argn == 1 && argvPtr[-1] == '-' && argvPtr[1] != '\0')
                        {
                            argvPtr++;
                            char abbreviatedCmd[2];
                            abbreviatedCmd[0] = *argvPtr;
                            abbreviatedCmd[1] = '\0';
                            cmd = GetDefaultCommand(abbreviatedCmd);
                            break;
                        }

                    default:
                        *pCmdOpts++ = *argvPtr;
                        break;
                    }
                }

                // Divide commands with end-of-command character so individual commands
                // can detect end of one option group and another
                if (pCmdOpts < pCmdEnd)
                    *pCmdOpts++ = sEOCchr;
                else
                    cerr << "Too many options, only processing:" << cmdOpts << std::endl;
            }
            else
            {
                break;
            }
        }

        int passArgc = argc - argn;
        const char** passArgv  = (passArgc == 0) ? NULL : argv + argn;

        switch (cmd)
        {
        case eCmp:
            exitStatus = LLCmp::StaticRun(cmdOpts, passArgc, passArgv);
            break;
        case eCopy:
            exitStatus = LLCopy::StaticRun(cmdOpts, passArgc, passArgv);
            break;
        case eDel:
            exitStatus = LLDel::StaticRun(cmdOpts, passArgc, passArgv);
            break;
        case eNone: // Default to Directory listing
        case eDir:
            exitStatus = LLDir::StaticRun(cmdOpts, passArgc, passArgv);\
            break;
        case eExec:
            exitStatus = LLExec::StaticRun(cmdOpts, passArgc, passArgv);
            break;
        case eFind:
            exitStatus = LLFind::StaticRun(cmdOpts, passArgc, passArgv);
            break;
        case eMove:
            exitStatus = LLMove::StaticRun(cmdOpts, passArgc, passArgv);
            break;
        case eWhere:
            strcat_s(cmdOpts, ARRAYSIZE(cmdOpts), "p" sEOCstr);
            exitStatus = LLFind::StaticRun(cmdOpts, passArgc, passArgv);
            break;
        case ePrintf:
            exitStatus = LLPrintf::StaticRun(cmdOpts, passArgc, passArgv);
            break;
        case eGrep:
            exitStatus = LLReplace::StaticRun(cmdOpts, passArgc, passArgv);
            break;
        case eInstall:
            {
                char dir[_MAX_DIR];
                _splitpath(argv[0], NULL, dir, NULL, NULL);
                SetCurrentDirectory(dir);

                const char* installArgs[2];
                char srcFile[120];
                char dstAlias[20];

                strcpy(srcFile, argv[0]);
                if (strchr(srcFile, '.') == NULL)
                    strcat(srcFile, ".exe");

                installArgs[0] = srcFile;
                installArgs[1] = dstAlias;
                exitStatus = 0;

                for (CmdAlias* pCmdAlias = sCmdAlias;
                    pCmdAlias->cmd != eInstall &&
                    pCmdAlias->cmd != eNone;
                    pCmdAlias++)
                {
                    strcat(strcpy(dstAlias, pCmdAlias->name), ".exe");
                    // Hardlink llfile.exe to <alias>.exe

                    LLMsg::Out() << "llfile -Xm -fOL " <<  installArgs[0] << " " <<  installArgs[1] << std::endl;
                    const char sLinkForceOveride[] = "L" sEOCstr "f"  sEOCstr  "O" sEOCstr;
                    exitStatus |= LLMove::StaticRun(sLinkForceOveride, 2, installArgs);
                }
                break;
            }
		case eUninstall:
            {
                char dir[_MAX_DIR];
                _splitpath(argv[0], NULL, dir, NULL, NULL);
                SetCurrentDirectory(dir);

                const char* installArgs[2];
                char srcFile[120];
                char dstAlias[20];

                strcpy(srcFile, argv[0]);
                if (strchr(srcFile, '.') == NULL)
                    strcat(srcFile, ".exe");

                installArgs[0] = dstAlias;
                exitStatus = 0;

                for (CmdAlias* pCmdAlias = sCmdAlias;
                    pCmdAlias->cmd != eInstall &&
                    pCmdAlias->cmd != eNone;
                    pCmdAlias++)
                {
                    strcat(strcpy(dstAlias, pCmdAlias->name), ".exe");
                    // Hardlink llfile.exe to <alias>.exe

                    LLMsg::Out() << "llfile -Xr -f " << installArgs[0] << std::endl;
                    const char sDelOptions[] = "f" sEOCstr;
                    exitStatus |= LLDel::StaticRun(sDelOptions, 1, installArgs);
                }
                break;
            }
        default:
            break;
        }
    }
    catch (exception e)
    {
        ErrorMsg() << "Program threw exception "
            << e.what() << std::endl;
    }
    catch (...)
    {
        ErrorMsg() << "Program threw exception "
            << ", last error code=" << GetLastError()
            << std::endl;
    }

#ifdef _DEBUG
    _CrtCheckMemory();
#endif

    return exitStatus;
}
