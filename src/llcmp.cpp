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

#pragma warning(disable: 4477)	// sprintf fmt %s requires char*

#include <iomanip>
#include <assert.h>
#include <sstream>
#include <set>

#include <windows.h>
#include <winioctl.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "llcmp.h"
#include "llpath.h"
#include "llprintf.h"
#include "Security.h"
#include "comma.h"


// ---------------------------------------------------------------------------

static const char sHelp[] =
" Compare Two Files " LLVERSION "\n"
"\n"
" Do binary comparison of files.  When processing a list of files only those\n"
" with matching base name are compared against each other.\n"
"\n"
"  !0eSyntax:!0f\n"
"    <filePattern> <filePattern>\n"
"\n"
"  !0ePattern:!0f\n"
"      * = zero or more characters\n"
"      ? = any character\n"
"\n"
"  !0eCompare mode:!0f\n"
"    -t                 ; Compare text files, defaults to binary \n"
"    -F=<filePattern>   ; Compare filenames \n"
"\n"
"  !0eExample:!0f\n"
"    LLCmp  d1\\*               ; Compare similar files in one directory\n"
"    LLCmp  d1\\* d2\\* d3\\*     ; Compare similar files in three directories\n"
"    LLCmp d1\\*.cpp d2\\*.cpp   ; Compare similar files ending in .cpp\n"
"    LLCmp -r d1 d2            ; Compare all similar files in two directory trees\n"
"    LLCmp c:\\t*\\src\\*.cpp master.cpp   ; Compare files to a single file\n"
"                                       ; and having subdirectory src\n"
"    LLCmp -o20 -v new.data old.data    ;Use offset to skip over date changes \n"
"\n"
"    ; Delete matching 2nd file in matching group, set -l=10 to get all dir levels \n"
"    LLCmp -rD -l=10  -d=e2 c:\\CSharpe\\* d:\\foo\\CSharpe\\* \n"
"\n"
"  !0eOutput controls:!0f\n"
"   -a                  ; Show ALL match result (default is only differences)\n"
"   -e                  ; Show only equal files\n"
"   -s                  ; Show skipped files  (default is only differences)\n"
"   -s=l or -s=r        ; Show skipped Left or Right files \n"
"   -v                  ; Verbose, show detail on first difference per file \n"
"   -V=<count>          ; Verbose, show differences, stop after 'count' per file \n"
"   -w=<width>          ; Compare filesize numeric width, default 12\n"
"   -q                  ; quiet, dont show detail, just set exit status 0=match, 1=differ\n"
"   -P=[pdn...]         ; File arguments passed to printf (default: p)\n"
"                       ;   p=full path\n"
"                       ;   d=directory\n"
"                       ;   n=name\n"
"                       ;   r=name root (no extension)\n"
"                       ;   e=extension (right of last .)\n"
"                       ;   s=size (as a string)\n"
"                       ;   _=defcase\n"
"                       ;   l=lowercase\n"
"                       ;   u=uppercase\n"
"                       ;   c=captialize\n"
"                       ;   x=unix slash\n"
"                       ;   X=dos slash\n"
"   -p=<fmt>            ; Printf format, use any valid %s format, %20s (default: %s) \n"
"                       ; Use quotes to include spaces in format \n"
"                       ; example  \"-p%s\\n\"  \n"
"   -qq                 ; Hide details and only show summary when used with -F \n"
"   -qp                 ; Hide file compare progress \n"
"   -Q=n                ; Quit after 'n' lines output\n"
"   -1=<output>         ; Redirect output to file \n"
"   -,                  ; Disable commas in size and numeric output\n"
"\n"
"  !0eFile selection:!0f\n"
"   -A=[nrhs]           ; Limit files by attribute (n=normal r=readonly, h=hidden, s=system)\n"
"   -D                  ; Force subdirectories and name to match before comparing\n"
"   -F                  ; Match just the filename, size and date and not its contents\n"
"   -I=<infile>         ; Read filenames from infile or stdin if -\n"
"   -r                  ; Recurse directories\n"
"   -l=<levels>         ; Directory levels to include in path matching, default is 1\n"
"   -o=<offset>         ; Start binary file compare at file offset, default is 0 \n"
"   -X=<pathPat>,...    ; Exclude patterns  -X=*.lib,*.obj,*.exe\n"
"                       ;  No space in patterns. Pattern applied against fullpath\n"
"                       ;  So *\\ma will exclude a directory ma or file ma \n"
"   -Z=<op><value>      ; siZe op=(Greater|Less|Equal) value=num<units G|M|K>, ex -Zg100M \n"
"\n"
"  !0eSpecial actions:!0f (files to delete are sorted, not argument order)\n"
"   -h                  ; Show MD5 hash only, no compare \n"
"   -d=e1 | -d=n1       ; Delete matching (-d=e) or not matching files (-d=n) \n"
"                       ;   -d=e all files, -d=e1 first file, -d=e2 second file \n"
"   -d=g | -d=l         ; Delete greatest size file or least size file \n"
"   -d=g2 | -d=l2       ; Delete 2nd file if greatest or least size \n"
"   -n                  ; Don't execute delete, just show command delete command \n"
"\n"
"  !0eExit value (%ERRORLEVEL%):!0f\n"
"   -E=e                ; return #equal \n"
"   -E=d                ; return #diff \n"
"   -E=n                ; return #diff lines (use with -t) \n"
"   -E=dlr              ; return diff + left + right \n"
"\n";


LLCmpConfig LLCmp::sConfig;
DWORD SHARE_ALL = FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE;

// ---------------------------------------------------------------------------
static bool CmpMatchName(const LLDirEntry* p1, const LLDirEntry* p2, unsigned levels)
{
    bool equal = (_stricmp(p1->filenameLStr, p2->filenameLStr) == 0);
    if (levels == 0 || !equal)
        return equal;
    return (_stricmp(p1->szDir+p1->baseDirLen, p2->szDir+p2->baseDirLen) == 0);
}

// ---------------------------------------------------------------------------
static bool CmpMatchPath(const LLDirEntry* p1, const LLDirEntry* p2, unsigned /* levels */)
{
    if (_stricmp(p1->filenameLStr, p2->filenameLStr) == 0)
    {
        return (_stricmp(p1->szDir+p1->baseDirLen, p2->szDir+p2->baseDirLen) == 0);
    }

    return false;
}

// ---------------------------------------------------------------------------
// Uses static buffers, not thread safe.
static const char* DisplayMD4Hash(const char* filePath)
{
    Handle fHnd = CreateFile(filePath, GENERIC_READ, SHARE_ALL, 0,
            OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, 0);

    const uint sBufSize = 4096*16;
    static std::vector<Byte> vBuffer(sBufSize);
    char* buffer = (char*)vBuffer.data();

    md5_state_t state;
    md5_init(&state);

    DWORD totSize = 0;
    DWORD rlen=sBufSize;
    while (rlen == sBufSize && 
        ReadFile(fHnd, buffer, sBufSize, &rlen, 0) != 0)
    {
        md5_append(&state, (const md5_byte_t *)buffer, rlen);
        totSize += rlen;
    }

    md5_byte_t digest[16];
    md5_finish(&state, digest);

    static char hex_output[16*2 + 1 + 15];
    for (int idx = 0; idx < 16; ++idx)
	    sprintf(hex_output + idx * 2, "%02x", digest[idx]);

    sprintf(hex_output + 32, ", %8u,", totSize);
    return hex_output;
}

// ---------------------------------------------------------------------------
LLCmp::LLCmp() :
    m_showDiff(true),
    m_showEqual(false),
    m_showSkipLeft(false),
    m_showSkipRight(false),
    m_quiet(false),         // no stats, no color
    m_progress(true),
    m_showMD5hash(false),
	m_printFmt("%s"),
	m_pushArgs("p"),
    m_compareDataMode(eCompareBinary),
    m_matchMode(eNameAndData),
    m_offset(0),            // start compare at file offset.
    m_quitByteLimit(10),    // number of different bytes to dump in verbose mode.
    m_levels(1),            // number of directory levels to include in path match.
    m_width(12),            // Filespec numeric width
    m_colSeparator("\t"),   // separator used between filespecs
    m_equalCount(0),
    m_diffCount(0),
    m_errorCount(0),
    m_minPercentChg(0),
    m_maxPercentChg(0),
    m_delCmd(eNoDel),
    m_delFiles(-1),         // if eq or ne deleting, delete all files in group.
    m_noDel(false)
{
    m_sizeFile0 = 0;
    m_sizeFileN = 0;
    m_skipCount[0] = m_skipCount[1] = 0;
    m_delCount = 0;
    m_diffLineCount = 0;

    EnableCommaCout();

    sConfigp = &GetConfig();
}

// ---------------------------------------------------------------------------
LLConfig& LLCmp::GetConfig() 
{
    return sConfig;
}

// ---------------------------------------------------------------------------
int LLCmp::StaticRun(const char* cmdOpts, int argc, const char* pDirs[])
{
    LLCmp llcmp;
    return llcmp.Run(cmdOpts, argc, pDirs);
}

// ---------------------------------------------------------------------------
// Return 0 if all files equal
int LLCmp::Run(const char* cmdOpts, int argc, const char* pDirs[])
{
    const char delOptErrMsg[] = "Delete equal or nonEqual option, syntax -d=e or -d=n";
    const char levelOptErrMsg[] = "Directory Levels to compare, syntax -l=<#levels>";
    const char offsetOptErrMsg[] = "Start binary compare at file offset, syntax -o=<#offset>";
    const char quitByteErrMsg[] = "Quit after 'n' differences per file, syntax -Q=<#num>";
    const char widthErrMsg[] = "missing width, syntax -w=<#width>";
	const char argOptMsg[] = "File arguments passed to printf, -a=[pdnres_luc]...";
	const char sMissingPrintFmtMsg[] = "Missing print format, -p=<fmt>\n";

    std::string str;

    if (argc == 0 && *cmdOpts == '\0')
        cmdOpts = "?";

    // Parse options
    while (*cmdOpts)
    {
        switch (*cmdOpts)
        {
    //  Delete options, -d and -n
        case 'd':   // de1 = delete equal, dn1 = delete not equal, 
					// dg = delete greatest file, dg2 delete 2nd file if greatest
					// dl = delete least file, dl2 delete 2nd file is least
            cmdOpts = LLSup::ParseString(cmdOpts+1, str, delOptErrMsg);
            m_delFiles = -1;
			switch (str[0])
			{
			case 'e':
				m_delCmd = eMatchDel;
                if (isdigit(str[1]))
                    m_delFiles = (str[1] == '1') ? 0 : 1;
				break;
			case 'n':
				m_delCmd = eNoMatchDel;
                if (isdigit(str[1]))
                    m_delFiles = (str[1] == '1') ? 0 : 1;
				break;
			case 'g':  // g or g2
				m_delCmd = (str[1] == '2' ? eGreater2Del : eGreaterDel);
				break;
			case 'l':	// l or l2
				m_delCmd = (str[1] == '2' ? eLesser2Del : eLesserDel);
				break;
			default:
				ErrorMsg() << delOptErrMsg << std::endl;
			}
                
            break;
        case 'n':   // no delete, just show command
            m_noDel = true;
            break;

    // File search options, -D, -F, -T, -l, -o, -P
        case 'D':    // force directory and name to match before comparing contents.
            m_matchMode = ePathAndData;
            break;
        case 'F':   // compare just filename, date and size and not its contents.
                    // -F=<filePat>[,<filePat>]...
            cmdOpts = LLSup::ParseList(cmdOpts+1, m_includeList, NULL);
            m_compareDataMode = eCompareSpecs;
            break;
        case 't':
            m_compareDataMode = eCompareText;
            break;
        case 'l':   // -l=<directoryLevels>
            cmdOpts = LLSup::ParseNum(cmdOpts+1, m_levels, levelOptErrMsg);
            break;
        case 'o':   // offset, -o=<offset>
            cmdOpts = LLSup::ParseNum(cmdOpts+1, m_offset, offsetOptErrMsg);
            break;
    
    // Option reporting options, -a, -e, -q, -s, -v, -V, -w, -=, -,
        case 'a':    // show All matches
            m_showDiff = m_showEqual = m_showSkipLeft = m_showSkipRight = true;
            break;

		case 'P':
			// File arguments passed to printf, -a=[pdnres_luc]...
			//   p=full path
			//   d=directory
			//   n=name
			//   r=name root (no extension)
			//   e=extension (right of last .)
			//   s=size (as a string)
			//   _=defcase
			//   l=lowercase
			//   u=uppercase
			//   c=captialize
			//  NOTE:  -a must proceed -i or -I
		
			cmdOpts = LLSup::ParseString(cmdOpts + 1, m_pushArgs, argOptMsg);
			break;
		case 'p':   // print using printf
			cmdOpts = LLSup::ParseString(cmdOpts + 1, m_printFmt, sMissingPrintFmtMsg);
			break;

        case 'e':    // show equal matches only
            m_showEqual = true;
            m_showDiff = false;
            m_showSkipLeft = m_showSkipRight = false;
            break;      
        case 'h':
            m_showMD5hash = true;
            break;
        case 'q':
			if (cmdOpts[1] == 'p')
			{
				m_progress = !m_progress;
				cmdOpts++;
			} 
			else
			{
				m_quiet = !m_quiet;
				m_showEqual = m_showDiff = false;
				m_showSkipLeft = m_showSkipRight = false;
				m_progress = false;
			}
            break;
			

        case 's':    // show skip only, or -s=r or -s=l
            m_showEqual = false;
            m_showDiff = false;
            m_showSkipLeft = m_showSkipRight = true;
            cmdOpts = LLSup::ParseString(cmdOpts+1, str, NULL);
            if (str.length() != 0)
            {
                m_showSkipLeft = m_showSkipRight = false;
                for (uint idx = 0; idx != str.length(); idx++)
                {
                    if (ToLower(str[idx]) == 'r')
                        m_showSkipRight = true;
                    else if (ToLower(str[idx]) == 'l')
                        m_showSkipLeft = true;
                }
            }
            break;
        case 'V':   // QuitByteLimit - max different bytes to output
            cmdOpts = LLSup::ParseNum(cmdOpts+1, m_quitByteLimit, quitByteErrMsg);
            m_verbose = true;
            break;
        case 'w':   // Width
            cmdOpts = LLSup::ParseNum(cmdOpts+1, m_width, widthErrMsg);
            break;
    // Output modifiers
        case '=':   // separator
            cmdOpts = LLSup::ParseString(cmdOpts+1, m_colSeparator, "Compare report column separator, syntax -=<string>");
            break;
        case ',':
            DisableCommaCout();
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

    m_dirSort.SetSort(m_dirScan, "n", false, true);
    m_dirSort.SetSortAttr(FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_ARCHIVE);  // Only show files.
    m_dirSort.SetSortData(m_compareDataMode == eCompareSpecs);

    if (m_inFile.length() != 0)
    {
        LLSup::ReadFileList(m_inFile.c_str(), m_dirScan.m_add_cb, this);
    }

    std::string dirScaned;

    // Iterate over dir patterns.
    for (int argn = 0; argn < argc; argn++)
    {
        if ( !dirScaned.empty())
            dirScaned += ", ";
        dirScaned +=  pDirs[argn];

        m_dirScan.Init(pDirs[argn], NULL);
        m_dirSort.m_baseDirLen = LLPath::GetPathLength(pDirs[argn]);
        m_dirs.push_back(m_dirScan.m_dir);
        m_dirScan.GetFilesInDirectory();
    }

    if (m_showMD5hash)
    {
        char filePath[MAX_PATH];
        unsigned fileIdx = 0;
        LLDirEntry*  pDirEntry = m_dirSort.m_pFirst;
        LLMsg::Out() << "                             MD5, FileSize, File\n";
        while (pDirEntry)
        {
			sprintf_s(filePath, ARRAYSIZE(filePath), "%s\\%s", pDirEntry->szDir, pDirEntry->filenameLStr);
            if ( !LLSup::PatternListMatches(m_excludeList, filePath) &&
				LLSup::PatternListMatches(m_includeList, pDirEntry->filenameLStr, true) &&
                LLSup::CompareRhsBits(pDirEntry->dwFileAttributes, m_onlyRhs))
            {
                LLMsg::Out() << DisplayMD4Hash(filePath) << " " << filePath << std::endl;
            }
            pDirEntry = pDirEntry->pNext;
            fileIdx++;
        }

        return 0;
    }

    try
    {
        m_inFileCnt = m_dirSort.m_count;

        switch (m_matchMode)
        {
        case eNameAndData:
            DoCmp(CmpMatchName);
            break;
        case ePathAndData:
            DoCmp(CmpMatchPath);
            break;
        }
    }
    catch (exception e)
    {
        ErrorMsg() << "Program threw exception " << e.what() << std::endl;
    }
    catch (...)
    {
        ErrorMsg() << "Program threw exception " << LLMsg::GetLastErrorMsg() << std::endl;
    }

    if ( !m_quiet)
    {
        // TODO - Add histogram of percent of file good before 1st difference.
        //  or        histogram of percent of #differences to file size

        LLMsg::Out() << "\n";

        if (m_equalCount != 0)
            LLMsg::Out() << " Equal Files:" << m_equalCount;

        if (m_diffCount != 0)
        {
            LLMsg::Alarm();
            SetColor(sConfig.m_colorDiff);
            LLMsg::Out() << ", Differ Files:" << m_diffCount;
            SetColor(sConfig.m_colorNormal);
        }

        if (m_errorCount != 0)
        {
            LLMsg::Alarm();
            SetColor(sConfig.m_colorError);
            LLMsg::Out() << ", Errors:" << m_errorCount;
            SetColor(sConfig.m_colorNormal);
        }

        SetColor(sConfig.m_colorSkip);
        if (m_skipCount[0] != 0)
            LLMsg::Out() << ", SkippedLeft:" << m_skipCount[0];
        if (m_skipCount[1] != 0)
            LLMsg::Out() << ", SkippedRight:" << m_skipCount[1];
        SetColor(sConfig.m_colorNormal);

        LLMsg::Out() << " " << dirScaned  << std::endl;
    }

    // Decide what to exit code to return, see -E=<opts> command.
    //      -Ee = return #equal
    //      -Ed = return #diff
    //      -En = return #lines differ
    //      -Edlr = diff + left + right
    if (m_exitOpts.empty())
        return int(m_diffCount + m_errorCount + m_skipCount[0] + m_skipCount[1]);

    int retValue = 0;
    for (const char* retOpt = m_exitOpts; *retOpt != '\0'; retOpt++)
    {
        switch (tolower(*retOpt))
        {
        case 'e':
            retValue += m_equalCount;
            break;
        case 'd':
            retValue += m_diffCount;
            break;
            break;
        case 'n':
            retValue += m_diffLineCount;
            break;
            break;
        case 'l':
            retValue += m_skipCount[0];
            break;
        case 'r':
            retValue += m_skipCount[1];
            break;
        }
    }

	return ExitStatus(retValue);
}

// ---------------------------------------------------------------------------
static bool GetFileSizeLL(Handle& hnd, LONGLONG& fileSizeLL)
{
    LARGE_INTEGER fileSizeLI;
    if (GetFileSizeEx(hnd, &fileSizeLI))
    {
        fileSizeLL = fileSizeLI.QuadPart;
        return true;
    }

    return false;
}

// ---------------------------------------------------------------------------
// return:  -2 skip, -1 error, 0 identical, 1 differ
LLCmp::CompareResult LLCmp::CompareDataBinary(
        const char* filePath1,
        const char* filePath2,
        LLCmp::CompareInfo& compareInfo,
        unsigned quitAfter,
        std::ostream& wout)
{
    compareInfo.differAt = 0;
    compareInfo.diffCnt = 0;

    struct _stat statResult;
    if (0 == _stat(filePath1, &statResult) && (statResult.st_mode & _S_IFREG) != _S_IFREG)
        return eCmpEqual;   // Can only compare files, return as if identical.

    const unsigned sMaxRetry = 10;
    Handle f1;

    for (unsigned retry = 0; retry != sMaxRetry; retry++)
    {
        f1 = CreateFile(filePath1, GENERIC_READ, SHARE_ALL, 0,
                OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, 0);
        if (f1.NotValid() && GetLastError() == ERROR_NOT_ENOUGH_SERVER_MEMORY)
            Sleep(1000 * retry);
        else
            break;;
    }

    if (f1.NotValid())
    {
        LLMsg::PresentError(GetLastError(), "Open failed,", filePath1);
        return eCmpErr;
    }

    Handle f2;
    for (unsigned retry = 0; retry != sMaxRetry; retry++)
    {
        f2 = CreateFile(filePath2, GENERIC_READ, SHARE_ALL, 0,
                OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, 0);
        if (f2.NotValid() && GetLastError() == ERROR_NOT_ENOUGH_SERVER_MEMORY)
            Sleep(1000 * retry);
        else
            break;
    }

    if (f2.NotValid())
    {
        LLMsg::PresentError(GetLastError(), "Open failed,", filePath2);
        // CloseHandle(f1);
        return eCmpErr;
    }

    CompareResult result = eCmpDiff;
    if ( !GetFileSizeLL(f1, compareInfo.fileSize1))
        result = eCmpErr;

    if ( !GetFileSizeLL(f2, compareInfo.fileSize2))
        result = eCmpErr;

    if (result != eCmpErr)
    {
        result = (compareInfo.fileSize1 == compareInfo.fileSize2) ? eCmpEqual : eCmpDiff;
        if (m_verbose &&  result != 0)
            wout << " File sizes differ: " << compareInfo.fileSize1 << " != "
                << compareInfo.fileSize2 << std::endl;
    }

    if (result == eCmpEqual)
    {
        if ( !SizeOperation(compareInfo.fileSize1, m_onlySizeOp, m_onlySize))
            result = eCmpSkip;
        if ( !SizeOperation(compareInfo.fileSize2, m_onlySizeOp, m_onlySize))
            result = eCmpSkip;
    }

    if (result == eCmpEqual)
    {
        DWORD whereSize = DWORD(compareInfo.fileSize2 / 100);
        memset(compareInfo.whereCnt, 0, sizeof(compareInfo.whereCnt));

        const uint sBufSize = 4096*16;
        Byte buffer1[sBufSize];
        Byte buffer2[sBufSize];
        DWORD rlen1=0, rlen2=0;

        if (m_offset != 0)
        {
            DWORD moved = 0;
            LARGE_INTEGER fileOffset;
            fileOffset.QuadPart = m_offset;
            SetFilePointer(f1, fileOffset.LowPart, &fileOffset.HighPart, moved);
            SetFilePointer(f2, fileOffset.LowPart, &fileOffset.HighPart, moved);
        }

        LONGLONG filePos = m_offset;
        while (
            (compareInfo.diffCnt == 0 || m_verbose) &&
            ReadFile(f1, buffer1, sizeof(buffer1), &rlen1, 0) != 0 &&
            ReadFile(f2, buffer2, sizeof(buffer2), &rlen2, 0) != 0 &&
            rlen1 == rlen2 &&
            rlen1 != 0)
        {
            if (m_verbose)
            {
                for (unsigned idx = 0; idx < rlen1; idx++)
                {
                    if (buffer1[idx] != buffer2[idx])
                    {
                        compareInfo.differAt = filePos + idx;
                        compareInfo.diffCnt++;
                        compareInfo.whereCnt[compareInfo.differAt/whereSize]++;

                        if (quitAfter != 0)
                        {
                            quitAfter--;
                            wout << "Differ at: " <<  filePos + idx
                                << " Data: "
                                << std::setw(3) << (unsigned)buffer1[idx] << " != "
                                << std::setw(3) << (unsigned)buffer2[idx]
                                << std::endl;
                        }
                    }
                }
            }
            else
            {
#if 1
                if (memcmp(buffer1, buffer2, rlen1) != 0)
#endif
                {
                    unsigned idx;
                    for (idx = 0; idx < rlen1; idx++)
                    {
                        if (buffer1[idx] != buffer2[idx])
                        {
                            compareInfo.diffCnt++;
                            break;
                        }
                    }

                    compareInfo.differAt = filePos + idx;
                }
            }
            filePos += rlen1;

            if (m_progress && compareInfo.fileSize1 > 1024*1024)
                std::cout << std::fixed << std::setw(6) << std::setprecision(2)
                    << (filePos * 100.0) /  compareInfo.fileSize1 << " %\r";
        }

        result = (rlen1 == rlen2 && compareInfo.diffCnt == 0) ? eCmpEqual : eCmpDiff;
    }

    // CloseHandle(f1);
    // CloseHandle(f2);

    return result;
}


// ---------------------------------------------------------------------------
// return:  -2 skip, -1 error, 0 identical, 1 differ
LLCmp::CompareResult  LLCmp::CompareDataText(
        const char* filePath1,
        const char* filePath2,
        LLCmp::CompareInfo& compareInfo,
        unsigned quitAfter,
        std::ostream& wout)
{
	std::ifstream in1(filePath1);
	std::ifstream in2(filePath2);

	std::string line1, line2;
	unsigned int lineCnt = 0;
    unsigned int diffLineCnt = 0;

    LLCmp::CompareResult status = eCmpEqual;
    compareInfo.differAt = 0;
    compareInfo.diffCnt = 0;

	while (std::getline(in1, line1) && std::getline(in2, line2))
	{
		lineCnt++;

		unsigned int idx1=0, idx2=0;
		while (idx1 < line1.length() && idx2 < line2.length())
		{
			while (m_ignoreChar.Is(line1[idx1]) && idx1 < line1.length())
				idx1++;
			while (m_ignoreChar.Is(line2[idx2]) && idx2 < line2.length())
				idx2++;

			if (line1[idx1++] != line2[idx2++])
			{
                if (diffLineCnt++ == 0) 
                    compareInfo.differAt = lineCnt;
                status = eCmpDiff;
                break;
			}
		}

        // if (compareInfo.diffCnt == 0)
        //    compareInfo.differAt = in1.tellg().seekpos;
	}

    compareInfo.diffCnt = diffLineCnt;
    return status;     // 0=equal, 1..n = different line count
}

// ---------------------------------------------------------------------------
void LLCmp::DeleteCmpFile(const char* fileToDel)
{
    if (fileToDel != NULL)
	{
		if (m_noDel)
		{
			LLMsg::Out() << " Delete " << fileToDel << std::endl;
		}
		else
		{
			if (LLSec::RemoveFile(fileToDel, true, true) == 0)
			{
				LLMsg::Out() << " Deleted " << fileToDel << std::endl;
				m_delCount++;
			}
			else
			{
				LLMsg::PresentError(_doserrno, "Delete failed,", fileToDel);
			}
		}
	}
}

// ---------------------------------------------------------------------------
std::ostream& LLCmp::PrintPath(const char* msg, const LLDirEntry* dirEntry0, const LLDirEntry* dirEntry1)
{
	int depth = 0;
	WIN32_FIND_DATA fileData;
	char filePath[MAX_PATH];

	LLMsg::Out() << msg;

	sprintf_s(filePath, ARRAYSIZE(filePath), "%s\\%s",
		dirEntry0->szDir,
		dirEntry0->filenameLStr);
	m_dirSort.m_fillFindData(fileData, *dirEntry0, m_dirSort);
	strncpy(fileData.cFileName, dirEntry0->filenameLStr, MAX_PATH);
	LLPrintf::PrintFile(m_pushArgs, m_printFmt, dirEntry0->szDir, filePath, &fileData, depth);

	LLMsg::Out() << ", ";
	sprintf_s(filePath, ARRAYSIZE(filePath), "%s\\%s",
		dirEntry1->szDir,
		dirEntry1->filenameLStr);
	m_dirSort.m_fillFindData(fileData, *dirEntry1, m_dirSort);
	strncpy(fileData.cFileName, dirEntry1->filenameLStr, MAX_PATH);
	LLPrintf::PrintFile(m_pushArgs, m_printFmt, dirEntry1->szDir, filePath, &fileData, depth);

	return LLMsg::Out();
}

// ---------------------------------------------------------------------------
int LLCmp::CompareFileData(DirEntryList& dirEntryList)
{
    int resultStatus = sIgnore;

    char filePath1[MAX_PATH];
    char filePath2[MAX_PATH];

    if (dirEntryList.size() == 0)
        return resultStatus;

    bool isLeft = (_strnicmp(m_dirs[0].c_str(),  dirEntryList[0]->szDir, m_dirs[0].length()) == 0);

    if (dirEntryList.size() == 1 && m_matchMode == eNameAndData)
    {
        if ((isLeft && m_showSkipLeft) || ( !isLeft && m_showSkipRight))
        {
            sprintf_s(filePath1, ARRAYSIZE(filePath1), "%s\\%s",
                dirEntryList[0]->szDir,
                dirEntryList[0]->filenameLStr);
            LLMsg::Out() << "Skip, " << filePath1 << std::endl;
            resultStatus = sOkay;
        }
        m_skipCount[isLeft ? 0 : 1]++;
        return resultStatus;
    }

    for (size_t fileIdx = 1; fileIdx < dirEntryList.size(); fileIdx++)
    {
        sprintf_s(filePath1, ARRAYSIZE(filePath1), "%s\\%s",
            dirEntryList[0]->szDir,
            dirEntryList[0]->filenameLStr);
        sprintf_s(filePath2, ARRAYSIZE(filePath2), "%s\\%s",
            dirEntryList[fileIdx]->szDir,
            dirEntryList[fileIdx]->filenameLStr);

        CompareInfo compareInfo;
        unsigned quitAfter = m_quitByteLimit;
        std::ostringstream cmpResults;

        // Enable comma formatting of numbers.
        char sep = ',';
        int group = 3;
        cmpResults.imbue(std::locale(std::locale(), new numfmt<char>(sep, group)));

        isLeft = (_strnicmp(m_dirs[0].c_str(),  dirEntryList[0]->szDir, m_dirs[0].length()) == 0);

        bool okayToSkip = ((isLeft && m_showSkipLeft) || ( !isLeft && m_showSkipRight));

        unsigned cmpResult = 
                (this->*compareFileMethod)(filePath1, filePath2, compareInfo, quitAfter, cmpResults);


        switch (cmpResult)
        {
        case eCmpSkip:    // Skipped
            m_skipCount[isLeft ? 0 : 1]++;
            if (okayToSkip)
            {
                // LLMsg::Out() << "Skip, " << filePath1 << ", " << filePath2 << std::endl;
				PrintPath("Skip, ", dirEntryList[0], dirEntryList[fileIdx]) << std::endl;
                LLMsg::Out() << cmpResults.str();
                resultStatus = sOkay;
            }
            break;
        case eCmpErr:    // access error
            m_errorCount++;
            // LLMsg::Out() << "Err, " << filePath1 << ", " << filePath2 << std::endl;
			PrintPath("Err, ", dirEntryList[0], dirEntryList[fileIdx]) << std::endl;
            resultStatus = sError;
            break;
        case eCmpEqual:
            m_equalCount++;
            if (m_showEqual)
            {
                // LLMsg::Out() << "==, " << filePath1 << ", " << filePath2 << std::endl;
				PrintPath("==, ", dirEntryList[0], dirEntryList[fileIdx]) << std::endl;
                LLMsg::Out() << cmpResults.str();
                resultStatus = sOkay;
            }

			if (m_delCmd != eNoDel)
			{
				const char* fileToDel = NULL;

				if (m_delCmd == eMatchDel)
                {
                    switch (m_delFiles)
                    {
                    case -1:
                        if (fileIdx+1 == dirEntryList.size())
                            DeleteCmpFile(filePath1);
                        DeleteCmpFile(filePath2);
                        break;
                    case 0:
                        if (fileIdx+1 == dirEntryList.size())
                            DeleteCmpFile(filePath1);
                        break;
                    case 1:
					    DeleteCmpFile(filePath2);
                        break;
                    }
				}
			}

            break;
        default:
        case eCmpDiff:
            m_diffLineCount += compareInfo.diffCnt;
            m_diffCount++;
            if (m_showDiff)
            {
                resultStatus = sOkay;
                if (m_compareDataMode == eCompareText)
                {
					LLMsg::Out() << "!=, " << filePath1 << ", " << filePath2;
					PrintPath("!=, ", dirEntryList[0], dirEntryList[fileIdx])
                        << ", DiffLineCnt: " << compareInfo.diffCnt
                        << ", FirstDiffLine:" << compareInfo.differAt 
                        << std::endl;
                }
                else
                {
                    // LLMsg::Out() << "!=, " << filePath1 << ", " << filePath2
					PrintPath("!=, ", dirEntryList[0], dirEntryList[fileIdx])
                        << ", Differ At: " << compareInfo.differAt
                        << " (" << compareInfo.differAt*100/compareInfo.fileSize1
                        << "%)";
                    if (m_verbose)
                        LLMsg::Out() << " DiffCnt: " << compareInfo.diffCnt
                            << " (" << compareInfo.diffCnt*100/compareInfo.fileSize1
                            << "%)";
                    LLMsg::Out() << std::endl;
                    LLMsg::Out() << cmpResults.str();
                    if (m_verbose && compareInfo.diffCnt != 0)
                    {
                        LLMsg::Out() << " Where:";
                        DWORD whereSize = DWORD(compareInfo.fileSize2 / 100);
                        for (unsigned whIdx = 0; whIdx != 100; whIdx++)
                        {
                            if (compareInfo.whereCnt[whIdx] == 0)
                                LLMsg::Out() << ' ';
                            else
                                LLMsg::Out() << char('0' + (compareInfo.whereCnt[whIdx] * 10 / whereSize));
                            if ((whIdx % 10) == 9)
                                LLMsg::Out() << "|";
                        }
                        LLMsg::Out() << std::endl;
                    }
                }
            }

			if (m_delCmd != eNoDel)
			{
				const char* fileToDel = NULL;

				switch (m_delCmd)
				{
                case eNoDel:
                    break;
				case eNoMatchDel:
					switch (m_delFiles)
                    {
                    case -1:
                        if (fileIdx+1 == dirEntryList.size())
                            DeleteCmpFile(filePath1);
                        DeleteCmpFile(filePath2);
                        break;
                    case 0:
                        if (fileIdx+1 == dirEntryList.size())
                            DeleteCmpFile(filePath1);
                        break;
                    case 1:
					    DeleteCmpFile(filePath2);
                        break;
                    }
					break;
				case eMatchDel:
					// fileToDel = filePath2;
					break;
				case eGreaterDel:
					fileToDel =  (compareInfo.fileSize1 > compareInfo.fileSize2) ? filePath1 : filePath2;
					break;
				case eLesserDel:
					fileToDel =  (compareInfo.fileSize1 < compareInfo.fileSize2) ? filePath1 : filePath2;
					break;
				case eGreater2Del:
					if (compareInfo.fileSize2 > compareInfo.fileSize1)
						fileToDel = filePath2;
					break;
				case eLesser2Del:
					if (compareInfo.fileSize2 < compareInfo.fileSize1)
						fileToDel =  filePath2;
					break;

				}

				DeleteCmpFile(fileToDel);
			}
            
            break;
        }
    }

    return resultStatus;
}

// ---------------------------------------------------------------------------
int LLCmp::CompareFileSpecs(DirEntryList& dirEntryList)
{
    char filePath0[MAX_PATH];
    char filePathN[MAX_PATH];

    if (dirEntryList.size() == 0)
        return sIgnore;

    sprintf_s(filePath0, ARRAYSIZE(filePath0), "%s\\%s",
            dirEntryList[0]->szDir,
            dirEntryList[0]->filenameLStr);
    WIN32_FIND_DATA fileData0;
    m_dirSort.m_fillFindData(fileData0, *dirEntryList[0], m_dirSort);

    bool allEqual = true;
    bool allDiff = true;
    bool isSkip = false;
    bool isLeft = (_strnicmp(m_dirs[0].c_str(),  dirEntryList[0]->szDir, m_dirs[0].length()) == 0);
    bool okayToSkip = ((isLeft && m_showSkipLeft) || ( !isLeft && m_showSkipRight));

    if (dirEntryList.size() == 1)
    {
        m_skipCount[isLeft ? 0 : 1]++;
        if ( !okayToSkip)
            return sIgnore;

        // LLMsg::Out() << "Skip, " << filePath0 << std::endl;
        isSkip = true;
    }


    LARGE_INTEGER large0;
    large0.LowPart = fileData0.nFileSizeLow;
    large0.HighPart = fileData0.nFileSizeHigh;

    WIN32_FIND_DATA fileDataN;
    LARGE_INTEGER largeN;


    for (size_t fileIdx = 1; fileIdx < dirEntryList.size(); fileIdx++)
    {
        m_dirSort.m_fillFindData(fileDataN, *dirEntryList[fileIdx], m_dirSort);
        largeN.LowPart = fileDataN.nFileSizeLow;
        largeN.HighPart = fileDataN.nFileSizeHigh;

        if (largeN.QuadPart == large0.QuadPart)
        {
            m_equalCount++;
            allDiff = false;
        }
        else
        {
            m_diffCount++;
            allEqual = false;
        }
    }

    if ( !isSkip)
    {
        if ( !m_showEqual && allEqual)
            return sIgnore;
        if ( !m_showDiff && allDiff)
            return sIgnore;
    }

    LLMsg::Out()  << std::setw(m_width) << large0.QuadPart;

    for (size_t fileIdx = 1; fileIdx < dirEntryList.size(); fileIdx++)
    {
        sprintf_s(filePathN, ARRAYSIZE(filePath0), "%s\\%s",
            dirEntryList[fileIdx]->szDir,
            dirEntryList[fileIdx]->filenameLStr);
        m_dirSort.m_fillFindData(fileDataN, *dirEntryList[fileIdx], m_dirSort);
        largeN.LowPart = fileDataN.nFileSizeLow;
        largeN.HighPart = fileDataN.nFileSizeHigh;

        LARGE_INTEGER largeChg;
        largeChg.QuadPart = large0.QuadPart - largeN.QuadPart;

        if (largeChg.QuadPart < 0)
            SetColor(sConfig.m_colorLess);
        else if (largeChg.QuadPart > 0)
            SetColor(sConfig.m_colorMore);

        double percentChg = double(largeChg.QuadPart)*100.0 / double(large0.QuadPart);

        if ( !m_quiet)
        {
            LLMsg::Out() << m_colSeparator << std::setw(m_width) << largeN.QuadPart;
            LLMsg::Out() << m_colSeparator << std::showpos << std::setw(m_width) <<
                largeChg.QuadPart << std::noshowpos;
            if (fabs(percentChg) > 1000)
                LLMsg::Out() << "( alot )";
            else
                LLMsg::Out() << "(" << std::setw(5)
                    << std::showpos << std::fixed << std::setprecision(1)
                    << percentChg
                    << "%)"
                    << std::noshowpos;
        }

        // colorNormal = FOREGROUND_INTENSITY;
        SetColor(sConfig.m_colorNormal);

        if (percentChg < m_minPercentChg)
        {
            m_minPercentChg = percentChg;
            m_minFileData0 = fileData0;
            m_minFileDataN = fileDataN;
            m_minFile0 = filePath0;
            m_minFileN = filePathN;
        }
        if (percentChg > m_maxPercentChg)
        {
            m_maxPercentChg = percentChg;
            m_maxFileData0 = fileData0;
            m_maxFileDataN = fileDataN;
            m_maxFile0 = filePath0;
            m_maxFileN = filePathN;
        }

        LARGE_INTEGER fileSize0;
        fileSize0.LowPart = fileData0.nFileSizeLow;
        fileSize0.HighPart = fileData0.nFileSizeHigh;
        m_sizeFile0 += fileSize0.QuadPart;

        LARGE_INTEGER fileSizeN;
        fileSizeN.LowPart = fileDataN.nFileSizeLow;
        fileSizeN.HighPart = fileDataN.nFileSizeHigh;
        m_sizeFileN += fileSizeN.QuadPart;
    }

    if (isSkip)
    {
        SetColor(FOREGROUND_RED + FOREGROUND_GREEN + FOREGROUND_INTENSITY);
        LLMsg::Out() << std::setw(40) << " Skipped";
        SetColor(sConfig.m_colorNormal);
    }

    LLMsg::Out() << m_colSeparator << filePath0 << std::endl;
    return sOkay;
}

// ---------------------------------------------------------------------------
void LLCmp::ReportCompareFileSpecs() const
{
    if (m_countOut > 1)
    {
        LLMsg::Out() << "Total\n";
        LLMsg::Out() << std::setw(m_width) << m_sizeFile0;

        LARGE_INTEGER largeChg;
        largeChg.QuadPart = m_sizeFile0 - m_sizeFileN;

        if (largeChg.QuadPart < 0)
            SetColor(FOREGROUND_RED);
        else if (largeChg.QuadPart > 0)
            SetColor(FOREGROUND_GREEN);

        LLMsg::Out() << m_colSeparator << std::setw(m_width) << m_sizeFileN;
        LLMsg::Out() << m_colSeparator << std::showpos << std::setw(m_width) << largeChg.QuadPart << std::noshowpos;
        double percentChg = double(largeChg.QuadPart)*100.0 / double(m_sizeFile0);
        if (fabs(percentChg) > 1000)
            LLMsg::Out() << "( alot )";
        else
            LLMsg::Out() << "(" << std::setw(5)
                << std::showpos << std::fixed << std::setprecision(1)
                << percentChg
                << "%)"
                << std::noshowpos;
        LLMsg::Out() << std::endl;

        // colorNormal = FOREGROUND_INTENSITY;
        SetColor(sConfig.m_colorNormal);

        LARGE_INTEGER large0;
        LARGE_INTEGER largeN;

        large0.LowPart = m_minFileData0.nFileSizeLow;
        large0.HighPart = m_minFileData0.nFileSizeHigh;
        largeN.LowPart = m_minFileDataN.nFileSizeLow;
        largeN.HighPart = m_minFileDataN.nFileSizeHigh;

        LLMsg::Out() << "Min change:" << m_minPercentChg << "%\n";
        LLMsg::Out() << "   " << std::setw(m_width) << large0.QuadPart << m_colSeparator << m_minFile0 << std::endl;
        LLMsg::Out() << "   " << std::setw(m_width) << largeN.QuadPart << m_colSeparator << m_minFileN << std::endl;

        large0.LowPart = m_maxFileData0.nFileSizeLow;
        large0.HighPart = m_maxFileData0.nFileSizeHigh;
        largeN.LowPart = m_maxFileDataN.nFileSizeLow;
        largeN.HighPart = m_maxFileDataN.nFileSizeHigh;

        LLMsg::Out() << "Max change:" << m_maxPercentChg << "%\n";
        LLMsg::Out() << "   " << std::setw(m_width) << large0.QuadPart << m_colSeparator << m_maxFile0 << std::endl;
        LLMsg::Out() << "   " << std::setw(m_width) << largeN.QuadPart << m_colSeparator << m_maxFileN << std::endl;
    }
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
class CompareDirLevels
{
public:
    static bool Compare(LLDirEntry* pDirEnt1, LLDirEntry* pDirEnt2);
    static void SetLevels(unsigned levels, std::vector<std::string> dirs)
    {
        s_levels = levels;
        s_dirs = dirs;
        std::sort(s_dirs.begin(), s_dirs.end(),  CmpLen);
    }
    static bool CmpLen(const std::string& a, const std::string& b)
    {   return a.length() > b.length();    }
    static const char* CompareDirLevels::GetDir(const LLDirEntry* pDirEnt);

    static unsigned s_levels;
    static std::vector<std::string> s_dirs;
};

unsigned CompareDirLevels::s_levels = 0;
std::vector<std::string> CompareDirLevels::s_dirs;

// ---------------------------------------------------------------------------
const char* CompareDirLevels::GetDir(const LLDirEntry* pDirEnt)
{
    const int sMaxLevels = 50;
    const char* dirs[sMaxLevels];
    int dirLevel = 0;
    const char* pDirStr = pDirEnt->szDir;
    for (unsigned dirIdx = 0; dirIdx != s_dirs.size(); dirIdx++)
    {
        if (_strnicmp(s_dirs[dirIdx].c_str(), pDirStr,  s_dirs[dirIdx].length()) == 0)
        {
            pDirStr += s_dirs[dirIdx].length();
            if (*pDirStr != '\\')
                pDirStr--;        // backup so it starts on slash
            break;
        }
    }

    while (*pDirStr != 0 && dirLevel < sMaxLevels)
    {
        if (*pDirStr == '\\')
            dirs[dirLevel++] = pDirStr;
        pDirStr++;
    }

    dirLevel--;
    if (dirLevel >= (int)s_levels)
        return dirs[dirLevel - s_levels];
    return (dirLevel < 0) ? "" : dirs[0];
}

// ---------------------------------------------------------------------------
bool CompareDirLevels::Compare(LLDirEntry* pDirEnt1, LLDirEntry* pDirEnt2)
{
    // 5/4/2013 - Change order from dir,name to name then dir to allow
    // matching in uneaven directories.
    // Also add s_dir list to this sorting object to peel off the root directories.

    const char* pDir1 = GetDir(pDirEnt1);
    const char* pDir2 = GetDir(pDirEnt2);
    int nameCmp =  _stricmp(pDirEnt1->filenameLStr, pDirEnt2->filenameLStr);

    int dirCmp = (nameCmp != 0) ? nameCmp : _stricmp(pDir1, pDir2);
    int fullCmp =  (dirCmp != 0) ? dirCmp : _stricmp(pDirEnt1->szDir, pDirEnt2->szDir);
    return fullCmp < 0;
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void  LLCmp::SortDirEntries()
{
    if (m_levels == 0 || m_dirSort.m_pFirst == NULL)
        return;

    CompareDirLevels::SetLevels(m_levels, m_dirs);
    typedef std::set<LLDirEntry*, bool(*)(LLDirEntry*, LLDirEntry*)> SortedSet;
    SortedSet sortedSet(CompareDirLevels::Compare);

    LLDirEntry*  pDirEntry = m_dirSort.m_pFirst;
    while (pDirEntry)
    {
        sortedSet.insert(pDirEntry);
        pDirEntry = pDirEntry->pNext;
    }

    pDirEntry = m_dirSort.m_pFirst = *sortedSet.begin();
    // LLMsg::Out() << pDirEntry->szDir << pDirEntry->filenameLStr.m_str << std::endl;
    SortedSet::iterator iter = sortedSet.begin();
    if (iter != sortedSet.end())
    {
        iter++;
        for ( ; iter != sortedSet.end(); ++iter)
        {
            // LLMsg::Out() << (*iter)->filenameLStr << " " << (*iter)->szDir <<  std::endl;

            pDirEntry->pNext = *iter;
            (*iter)->pPrev = pDirEntry;
            pDirEntry = *iter;
            (*iter)->pNext = NULL;
            // LLMsg::Out() << pDirEntry->szDir << pDirEntry->filenameLStr.m_str << std::endl;
        }
    }
}

// ---------------------------------------------------------------------------
void  LLCmp::DoCmp(CmpMatch cmpMatch)
{
    int resultStatus = sIgnore;
    LLDirEntry* pDirEntry = m_dirSort.m_pFirst;
    DirEntryList cmpList;
	char filePath[MAX_PATH]; 

    size_t dirEntryCnt = 0;
    while (pDirEntry)
    {
        dirEntryCnt++;
        pDirEntry = pDirEntry->pNext;
    }

    if (dirEntryCnt == 2)
    {
        // Special Case, don't match filenames.

        pDirEntry = m_dirSort.m_pFirst;
        while (pDirEntry)
        {
			sprintf_s(filePath, ARRAYSIZE(filePath), "%s\\%s", pDirEntry->szDir, pDirEntry->filenameLStr);

            if ( !LLSup::PatternListMatches(m_excludeList, filePath) &&
                LLSup::PatternListMatches(m_includeList, pDirEntry->filenameLStr, true) &&
                LLSup::CompareRhsBits(pDirEntry->dwFileAttributes, m_onlyRhs))
            {
                cmpList.push_back(pDirEntry);
            }
            pDirEntry = pDirEntry->pNext;
        }

        bool showAny = m_showEqual | m_showDiff | m_showSkipLeft | m_showSkipRight;
        switch (m_compareDataMode)
        {
        case eCompareSpecs:
            if (showAny)
                LLMsg::Out() << "  First Size, [  2nd Size, 1st-2nd(% chg),]...  1st File Path\n";
            resultStatus = CompareFileSpecs(cmpList);
            break;
        case eCompareBinary:
            if (showAny)
                LLMsg::Out() << "Binary Compare\n";
            compareFileMethod = &LLCmp::CompareDataBinary;
            resultStatus = CompareFileData(cmpList);
            break;
        case eCompareText:
            if (showAny)
                LLMsg::Out() << "Text Compare\n";
            compareFileMethod = &LLCmp::CompareDataText;
            resultStatus = CompareFileData(cmpList);
            break;
        }
    }
    else
    {
        bool showAny = m_showEqual | m_showDiff | m_showSkipLeft | m_showSkipRight;
        switch (m_compareDataMode)
        {
        case eCompareSpecs:
            if (showAny)
                LLMsg::Out() << "  First Size, [  2nd Size, 1st-2nd(% chg),]...  1st File Path\n";
            break;
        case eCompareBinary:
            if (showAny)
                LLMsg::Out() << "Binary Compare\n";
            break;
        case eCompareText:
            if (showAny)
                LLMsg::Out() << "Text Compare\n";
            break;
        }

        SortDirEntries();
        unsigned fileIdx = 0;
        pDirEntry = m_dirSort.m_pFirst;
        while (pDirEntry)
        {
            cmpList.clear();

            do
            {
				sprintf_s(filePath, ARRAYSIZE(filePath), "%s\\%s", pDirEntry->szDir, pDirEntry->filenameLStr);
                if ( !LLSup::PatternListMatches(m_excludeList, filePath) &&
					LLSup::PatternListMatches(m_includeList, pDirEntry->filenameLStr, true) &&
                    LLSup::CompareRhsBits(pDirEntry->dwFileAttributes, m_onlyRhs))
                {
                    cmpList.push_back(pDirEntry);
                }
                pDirEntry = pDirEntry->pNext;
                fileIdx++;
            } while (pDirEntry && cmpMatch(pDirEntry, cmpList[0], m_levels));

            if (m_progress)
            {
                // Show file compare progress.
                std::cout << fileIdx * 100 / m_inFileCnt << "% ";
                std::cout << " Eq:" << m_equalCount;
                if (m_diffCount != 0)
                    std::cout << ", Ne:" << m_diffCount;
                if (m_errorCount != 0)
                    std::cout << ", Er:" << m_errorCount;
                if (m_skipCount[0] != 0)
                    std::cout << ", SL:" << m_skipCount[0];
                if (m_skipCount[1] != 0)
                    std::cout << ", SR:" << m_skipCount[1];
                if (m_delCount != 0)
                    std::cout << ", Del:" << m_delCount;
                std::cout << "\r";
            }

            switch (m_compareDataMode)
            {
            case eCompareSpecs:
                resultStatus = CompareFileSpecs(cmpList);
                break;
            case eCompareBinary:
                compareFileMethod = &LLCmp::CompareDataBinary;
                resultStatus = CompareFileData(cmpList);
                break;
            case eCompareText:
                compareFileMethod = &LLCmp::CompareDataText;
                resultStatus = CompareFileData(cmpList);
                break;
            }

            if (resultStatus != sIgnore && IsQuit())
                break;
        }
    }

    switch (m_compareDataMode)
    {
    case eCompareSpecs:
        ReportCompareFileSpecs();
        break;
    case eCompareBinary:
        break;
    case eCompareText:
        break;
    }

}
