//-----------------------------------------------------------------------------
// llbase - Base class for all directory manipulation
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

#include <share.h>
#include <fstream>
#include <string>

#include "llbase.h"

const char LLBase::s_ignoreActionMsg[] = "  Ignored\n";
LLConfig* LLBase::sConfigp = NULL;

// ---------------------------------------------------------------------------
// Parse base command options
bool LLBase::ParseBaseCmds(const char*& cmdOpts)
{
    const char optGrepMsg[] = "Process file if it contains grepPattern, -G=<grepPattern>";
    const char optRangeMsg[] = "Grep options -g<grepRange>\n"
        " ex -g=L10_M10_Hf_B1_A2\n"
        "  Ln=first n lines \n"
        "  Mn=first n matches \n"
        "  H(f|l|m|t) Hide filename|Line#|MatchCnt|Text \n"
        "  Bn=show before n lines \n"
        "  An=show after n lines \n"
        "  F(l|f)  force byLine or byFile \n"
        "  U(i|b)  update inplace or backup \n"
        "  I       ignore case \n"
        ;
    const char missingAttrMsg[] = "Filter on attributes, syntax -A=[nrhs]";
    const char optSepErrMsg[] = "Additional field separators, -B=<separators>";
    const char quitOptErrMsg[] = "Quit after 'n' files, syntax -Q=<#files>";
    const char excludeEmptyMsg[] = "Invalid exclude syntax. Use -X=<pattern>[,<pattern> with no spaces\n";
    const char missingExitMsg[] = "Missing exit options. Use -E=<opts>\n";
    const char missingDepthMsg[] = "Depth, -d=0 (all), -d=-n (less), -d=+n (more)";

    std::string str;
    bool valid;

    switch (*cmdOpts)
    {
    case 'A':   // Limit by attributes, -A=[nrhs]
        cmdOpts = LLSup::ParseAttributes(cmdOpts+1, m_onlyRhs, missingAttrMsg);
        break;
    case 'B':   // Add additional Field separators, -B=<sep>..., used with # substitution.
        cmdOpts = LLSup::ParseString(cmdOpts+1, str, optSepErrMsg);
        m_separators += str;
        break;
    case 'd':   // Limit directory depth to n, 0=all, -d=-n (less than) -d=n (more than)
        cmdOpts = LLSup::ParseNum(cmdOpts+1, m_depthLimit, missingDepthMsg);
        break;
    case 'D':   // Limit to directories, -D or -D=<dirPat>[,<dirPat>]...
		cmdOpts = LLSup::ParseList(cmdOpts + 1, m_includeList, NULL);
        m_onlyAttr = FILE_ATTRIBUTE_DIRECTORY;
        m_dirSort.SetSortAttr(m_onlyAttr);
        break;
    case 'E':   // Exit code, -E=...
         cmdOpts = LLSup::ParseString(cmdOpts+1, m_exitOpts, missingExitMsg);
        break;
    case 'F':   // Limit to files, -F or -F=<filePat>[,<filePat>]...
        cmdOpts = LLSup::ParseList(cmdOpts+1, m_includeList, NULL);
        m_onlyAttr = FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_ARCHIVE; // show only files (see -A or -d)
        m_dirSort.SetSortAttr(m_onlyAttr);
        break;
    case 'G':   // grep pattern  -G=<grepPattern>   , Execute only if file contains grepPattern
        cmdOpts = LLSup::ParseString(cmdOpts+1, str, optGrepMsg);
        if (str.length() != 0)
            m_grepLinePat = m_grepLineStr = str;
        break;
    case 'g':   // grep range -g=<grepOptions>   default is entire file 
        // ex -G=L10M10HfB1A2
        //  +n=max lines
        //  Ln=first n lines  
        //  Mn=first n matches "
        //  H(f|l|m|t) Hide filename|Line#|MatchCnt|Text 
        //  Bn=show before n lines 
        //  An=show after n lines 
        // cmdOpts = LLSup::ParseNum(cmdOpts+1, m_grepRange.maxLine, optRangeMsg);
        cmdOpts = LLSup::ParseString(cmdOpts+1, str, optRangeMsg);
        m_grepOpt.Parse(str);
        break;
    case '0':   
    case 'I':   // Input list of files, -I=<inFilePath>, or -I=-
        cmdOpts = LLSup::ParseString(cmdOpts+1, m_inFile, missingInFileMsg);
        break;
    case 'N':
    case 'n':   // No copy, just show command.
        m_exec = false;
        m_echo = true;
        break;
    case 'p':   // prompt
        if (m_prompt)
            m_promptAns = 'p';      // -pp
        m_prompt = true;
        break;
     case 'P':   // grep pattern on source path  -P=<grepPattern>
        cmdOpts = LLSup::ParseString(cmdOpts+1, str, optGrepMsg);
        if (str.length() != 0)
            m_grepSrcPathPat= str;
        break;
    case 'q':   // quiet
        m_echo = false;
        break;
    case 'Q':   // QuitFileLimit - max lines to output, -Q=<fileLimit>
        cmdOpts = LLSup::ParseNum(cmdOpts+1, m_limitOut, quitOptErrMsg);
        break;
    case 'r':   // Directory scan options
        m_dirScan.m_recurse = !m_dirScan.m_recurse;
        break;
    case 'T':   // -T[acm]<op><value>   ; Limit by Time a=access, c=creation, m=modified
                // -T=[acm]<op><value>  ; Limit by Time a=access, c=creation, m=modified
        cmdOpts = LLSup::ParseTimeOp(cmdOpts+1, valid, m_testTimeFields, m_timeOp, m_testTime);
        return valid;
    case 'v':
    case 'V':
        m_verbose = true;
        break;
    case 'X':   // Exclude, -X=<pathPat>[,<pathPat>...]
        cmdOpts = LLSup::ParseList(cmdOpts+1, m_excludeList, excludeEmptyMsg);
        break;
    case 'Z':   // if siZe  -Z=1000K  or -Z<1M or -Z>1G
        cmdOpts = LLSup::ParseSizeOp(cmdOpts+1, m_onlySizeOp, m_onlySize);
        break;
    case '1':   // Redirect output, -1=<outfile>
        cmdOpts = LLSup::ParseOutput(cmdOpts+1, false);
        break;
    case '3':   // Tee output, -3=<outfile>
        cmdOpts = LLSup::ParseOutput(cmdOpts+1, true);
        break;
    case sEOCchr:
        break;
    default:
        LLSup::CmdError(cmdOpts);
        return false;
    }

    return true;
}

// ---------------------------------------------------------------------------
//  Filter on:
//      m_onlyAttr      File or Directory, -F or -D
//      m_onlyRhs       Attributes, -A=rhs
//      m_includeList   File patterns,  -F=<filePat>[,<filePat>]...
//      m_onlySize      File size,  -Z op=(Greater|Less|Equal) value=num<units G|M|K>, ex -Zg100M
//      m_excludeList   Exclude path patterns, -X=<pathPat>[,<pathPat>]...
//      m_timeOp        Time, -T[acm]<op><value>  ; Limit by Time a=access, c=creation, m=modified\n
//
//  Updates m_isDir, m_srcPath and m_fileSize, m_countInDir, m_countInFiles
bool LLBase::FilterDir(
        const char* pDir,
        const WIN32_FIND_DATA* pFileData,
        int depth)
{
    // if (depth < 0)
    //     return false;         // ignore end-of-directory
           
    if (m_depthLimit != 0)
    {
        if (m_depthLimit > 0 && depth < m_depthLimit)
            return false;
        if (m_depthLimit < 0 && depth > -m_depthLimit)
            return false;
    }

    LARGE_INTEGER fileSize;
    fileSize.HighPart = pFileData->nFileSizeHigh;
    fileSize.LowPart  = pFileData->nFileSizeLow;
    m_fileSize = fileSize.QuadPart;

    m_srcPath = LLPath::Join(pDir, pFileData->cFileName);
    m_isDir   = ((pFileData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0);

    if (m_isDir)
    {
        m_countInDir++;
		/*
		if (m_onlyAttr == FILE_ATTRIBUTE_DIRECTORY &&
			!LLSup::PatternListMatches(m_includeList, pFileData->cFileName, true))
			return false;
		*/
    }
    else
    {
        m_countInFile++;

        if ( !LLSup::PatternListMatches(m_includeList, pFileData->cFileName, true))
            return false;
        if (m_grepSrcPathPat.flags() != 0 &&
            !std::tr1::regex_search(m_srcPath.begin(), m_srcPath.end(), m_grepSrcPathPat))
            return false;
        if ( !SizeOperation(m_fileSize, m_onlySizeOp, m_onlySize))
            return false;
    }

    if ( !LLSup::CompareDeviceBits(pFileData->dwFileAttributes, m_onlyAttr))
        return false;
    if ( !LLSup::CompareRhsBits(pFileData->dwFileAttributes, m_onlyRhs))
        return false;
    if (LLSup::PatternListMatches(m_excludeList, m_srcPath))
        return false;
    if ( !TimeOperation(pFileData, m_timeOp, m_testTimeFields, m_testTime))
        return false;

    return true;
}

// ---------------------------------------------------------------------------
// Populate m_dstPath, replace #n and *n patterns.
// If m_dstPath contains a plan '*' it is not replaced.

void LLBase::MakeDstPath(
    const char* dstDir,
    const WIN32_FIND_DATA* pFileData,
    const char* pPattern)
{
    m_dstPath = dstDir;

    if (Contains(m_dstPath, '#') != NULL)
        LLSup::ReplaceDstWithSrc(m_dstPath, m_srcPath, m_separators);

    // Replace destination '*n' with parts from source file.
    //      srcFile = "fooBar.exe
    //      pattern = "f*B*.exe
    //      dstPath = \xdir\cdir\A*2B*1.exe
    //      dstPath = \xdir\cdir\AarBoo.exe
    if (Contains(m_dstPath, '*'))
        LLSup::ReplaceDstStarWithSrc(m_dstPath, pFileData->cFileName, pPattern, m_separators);
    else if ( !m_isDir && LLPath::IsDirectory(m_dstPath))
        m_dstPath = LLPath::Join(m_dstPath, pFileData->cFileName);
}

// ---------------------------------------------------------------------------
void LLBase::MakeDstPathEx(
    const char* dstDir,
    const WIN32_FIND_DATA* pFileData,
    const char* pPattern)
{
    MakeDstPath(dstDir, pFileData, pPattern);

    uint  wantSubIdx;
    if ((wantSubIdx = m_dstPath.find('*')) != (uint)string::npos)
    {
        // If destination has '*', hopefully at end of filepath, replace it
        // with part of the source directory resulting from recursive scanning.
        int  srcIdx = string::npos;
        int  dirCnt = m_dirScan.m_subDirCnt;
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
        if ((m_onlyAttr & FILE_ATTRIBUTE_DIRECTORY) != 0 && m_onlyAttr != FILE_ATTRIBUTE_DIRECTORY)
        {
            // Special mode, only copy directory tree if destination does not exist
            // and coping files and directories. If just directories don't make destination.
            bool pathExists = LLPath::PathExists(m_dstPath);
            if (!pathExists)
                LLSup::CreateDirectories(m_dstPath, 0, m_exec);
        }
    }
}

// ---------------------------------------------------------------------------
bool LLBase::FilterGrep()
{
    if ( !m_isDir)
    {
        int flags = m_grepLinePat.flags();
        if (flags != 0)
        {
            bool match = false;
            size_t lineCnt = 0;
            try
            {
                std::ifstream in(m_srcPath, std::ios::in, _SH_DENYNO);
                std::string str;
                while (std::getline(in, str))
                {
                    if (std::tr1::regex_search(str.begin(), str.end(), m_grepLinePat))
                    {
                        return true;
                    }

                    lineCnt++;
                    if (lineCnt > m_grepOpt.lineCnt)
                        break;
                }
            }
            catch (...)
            {
            }

            return match;
        }
    }

    return true;
}

// ---------------------------------------------------------------------------
// Return true if user wants to quit.
bool LLBase::PromptAnsQuit()
{
    switch (m_promptAns)
    {
    case 'Y':   // Yes-all
        return false;
    case 'N':   // No-all
        m_countIgnored++;
        return true;
    }

    char answer = '\0';
    std::cout << "  yes/no/YES(all)/NO(all)/quit ? ";
    do
    {
        if (answer)
        { std::cout << " ? "; Beep(750, 130); }
        answer = GetChe();
    } while (strchr("ynaq", ToLower(answer)) == NULL);

    m_promptAns = answer;
    switch (ToLower(m_promptAns))
    {
    case 'a':
        m_prompt = false;
        std::cout << "ll\n";
        break;
    case 'n':
        std::cout << "o\n";
        m_countIgnored++;
        return true;
    case 'q':
        std::cout << "uit!\n";
        Beep(750, 130);
        m_dirScan.m_abort = true;
         m_countIgnored++;
        return true;
        break;
    case 'y':
        std::cout << "es\n";
        return false;
    }

    return false;
}

// ---------------------------------------------------------------------------
HANDLE LLBase::ConsoleHnd()
{
    return GetStdHandle(STD_OUTPUT_HANDLE);
}

// ---------------------------------------------------------------------------
void LLBase::SetColor(WORD color)
{
    if (sConfigp != NULL && sConfigp->m_colorOn)
        SetConsoleTextAttribute(ConsoleHnd(), color);
}

// ---------------------------------------------------------------------------
bool LLBase::GrepOpt::Parse(const std::string& str) 
{
    // ex -g=L10_M10_Hf_B1_A2
    //  Ln=first n lines  
    //  Mn=first n matches "
    //  H(c|f|l|m|t) Hide color|filename|Line#|MatchCnt|Text 
    //  I=ignore case
    //  Bn=show before n lines 
    //  An=show after n lines 
    //  F(l|f)  Force byLine or byFile mode
    //  U(i|b)  Update inplace or make backup.
    //  I ignore case
	//  R repeat replace
    char* strPtr = (char*)str.c_str();
    while (*strPtr)
    {
        switch (*strPtr++)
        {
        case '.':
        case ',':
        case '_':
            break;
        case 'F':
            force = *strPtr++;
            break;
        case 'I':
            ignoreCase = true;
            break;
        case 'U':
            update = *strPtr++;
            break;
        case 'L':
            lineCnt = strtol(strPtr, &strPtr, 10);
            break;
        case 'M':
            matchCnt = strtol(strPtr, &strPtr, 10);
            break;
		case 'R':
			repeatReplace = true;
			break;
        case 'H':	// hide
            while (islower(*strPtr))
            {
                switch (*strPtr++)
                {
				case 'c':
					hideColor = true;
					break;
                case 'f':
                    hideFilename = true;
                    break;
                case 'l':
                    hideLineNum = true;
                    break;
                case 'm':
                    hideMatchCnt = true;
                    break;
                case 't':
                    hideText = true;
                    break;
                default:
                    return false;
                }
            }
            
            break;
        case 'B':
            beforeCnt = strtol(strPtr, &strPtr, 10);
			force = 'l';
            break;
        case 'A':
            afterCnt = strtol(strPtr, &strPtr, 10);
			force = 'l';
            break;
        default:
            return false;
        }
    }

    return true;
}