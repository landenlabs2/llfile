//-----------------------------------------------------------------------------
// llsupport - Support routines shared by LL files files.
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

#include <vector>
#include <algorithm>
#include <assert.h>
#include <windows.h>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <conio.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#include <direct.h> // rmdir

#include "llsupport.h"
#include "llbase.h"
#include "llmsg.h"
#include "dirscan.h"



namespace LLSup
{
static char sVersion[]= "LLFile - By:DLang "  LLVERSION  " " __DATE__;


// ---------------------------------------------------------------------------
bool GetCmdField(const char*& cmdOpts, char* cmdOutBuffer, size_t maxOutBuffer)
{
    if (NotEOC(*cmdOpts))
        cmdOpts++;

    uint idx = 0;
    for (idx = 0; NotEOC(cmdOpts[idx]); idx++)
    {
        if (idx < maxOutBuffer)
            cmdOutBuffer[idx] = cmdOpts[idx];
    }

    cmdOutBuffer[idx] = '\0';
    cmdOpts += idx;

    return (idx > 0);
}

// ---------------------------------------------------------------------------
// Advance to next parameter

void AdvCmd(const char*& cmdOpts)
{
    cmdOpts++;  // skip over character, null or sEOLchr
#if 0
    if (*cmdOpts == sEOLchr)
        cmdOpts++;
    else if (*cmdOpts != '\0')
    {
        printf("\aIgnoring auxilary command text:%s\n", cmdOpts);
        while (*cmdOpts && *++cmdOpts != sEOLchr)
        {}
    }
#endif
}

// ---------------------------------------------------------------------------
int CmdError(const char* cmdOpts)
{
    ErrorMsg() << "\nUnknown command:" << cmdOpts << std::endl;
    return -1;
}

// ---------------------------------------------------------------------------
std::string GetEnvStr(const char* envName, const char* defStr)
{
    static std::string sEnvStr;

    if (sEnvStr.empty())
    {
        char* pEnv;
        size_t envSize = 0;
        _dupenv_s(&pEnv, &envSize, envName);
        if (pEnv == NULL)
        {
            if (defStr != NULL)
                sEnvStr = defStr;
        }
        else
        {
            sEnvStr = pEnv;
            free(pEnv);
        }
    }

    return sEnvStr;
}

// ---------------------------------------------------------------------------
// Replace part of destination string with new value.
std::string& ReplaceString(
        std::string& dstStr, uint offset, int removeLen,
        const char* addStr, int addLen)
{
    if (addStr[0] == '\\' || addStr[0] == '/')
    {
        addStr++;
        addLen--;
    }

    if (addLen >= 0 && removeLen >= 0)
    {
        dstStr.replace(offset, removeLen, addStr, addLen);
    }

    return dstStr;
}

// ---------------------------------------------------------------------------
// Replace parts of destination *<value> with parts of source file.
// Return true if one or more replacements occurred.

bool ReplaceDstStarWithSrc(
        std::string& dstFile,
        const char* pSrcFile,
        const char* pPattern,
        const char* separators)
{
    bool didReplace = false;
    std::vector<PatInfo> patInfoLst;
    PatInfo patInfo;
    patInfo.rawLen = patInfo.rawOff = patInfo.wildOff = 0;

    for (int sepIdx = 0; separators[sepIdx] != '\0'; sepIdx++)
    {
         const char* pSlash = strrchr(pPattern, separators[sepIdx]);
         if (pSlash != NULL)
             pPattern = pSlash + 1;
    }

    bool okay = WildCompare(
        pPattern,       // wild
        pSrcFile,
        patInfo,
        patInfoLst);

    for (uint dstIdx = 0; dstIdx != dstFile.length(); dstIdx++)
    {
        char* endPtr = NULL;
        if (dstFile[dstIdx] == '*')
        {
            int dirNum;
            const char* pDstNum = dstFile.c_str() + dstIdx + 1;
            if ((dirNum = strtol(pDstNum, &endPtr, 10)), endPtr != pDstNum)
            {
                if (dirNum < 0)
                    dirNum += (uint)patInfoLst.size();    // invert direction.
                else if (dirNum > 0)
                    dirNum--;       // Convert 1 based to 0 based.

                if (dirNum >= (int)patInfoLst.size()) // force overflow to stop on last entry
                    dirNum = patInfoLst.size()-1;

                if (dirNum >= 0)
                {
                    int addLen = patInfoLst[dirNum].rawLen;
                    int remLen = int(endPtr - pDstNum) + 1;

                    ReplaceString(dstFile, dstIdx, remLen,
                            pSrcFile + patInfoLst[dirNum].rawOff, addLen);
                    didReplace = true;
                }
            }

// 7/18/2013 - Disabled the following because it was breaking recursive copy.
#if 0
// Assume a '*' without a number is the same as *0
            else if (patInfoLst.size() != 0)
            {
                dirNum = 0;
                int addLen = patInfoLst[dirNum].rawLen;
                int remLen = int(endPtr - pDstNum) + 1;

                ReplaceString(dstFile, dstIdx, remLen,
                        pSrcFile + patInfoLst[dirNum].rawOff, addLen);
            }
#endif
        }
    }

    return didReplace;
}

// ---------------------------------------------------------------------------
// Replace parts of destination #<value> with parts of source file.
// Return true if one or more replacements occurred.

bool ReplaceDstWithSrc(
        std::string& dstFile,
        const char* pSrcFile,
        const char* separators)
{
    bool didReplace = false;
    std::vector<const char*> dirList;

    dirList.push_back(pSrcFile);
    const char* pSrc;
    for (pSrc=pSrcFile; *pSrc != '\0'; pSrc++)
    {
        if (strchr(separators, *pSrc) != NULL)
            dirList.push_back(pSrc);
    }
    int dirLen = (dirList.size() == 0) ? 0 : int(dirList.back() - pSrcFile);
    dirList.push_back(pSrc);

    for (uint dstIdx = 0; dstIdx < dstFile.length(); dstIdx++)
    {
        char* endPtr = NULL;
        if (dstFile[dstIdx] == '#')
        {
            int dirNum;
            const char* pDstNum = &dstFile[dstIdx+1];
            if ((dirNum = strtol(pDstNum, &endPtr, 10)), endPtr != pDstNum)
            {
                if (dirNum < 0)
                    dirNum += dirList.size()-1; // don't count end, don't count file

                if (dirNum+1 >= (int)dirList.size()) // force overflow to stop on last entry
                    dirNum = dirList.size()-2;

                if (dirNum >= 0)
                {
                    int addLen = int(dirList[dirNum+1] - dirList[dirNum]);
                    int remLen = int(endPtr - pDstNum) + 1;
                    // Skip separator if present.
                    int offset = (strchr(separators, dirList[dirNum][0]) != NULL) ? 1 : 0;

                    // int lenToEnd = pDstFile + maxDstSize - pDst;
                    ReplaceString(dstFile, dstIdx, remLen, dirList[dirNum]+offset, addLen-offset);
                    didReplace = true;
                }
            }
            else
            {
                //  base.ext
                const char* pName = dirList[dirList.size()-2];
                char* pModName = (char*)pName;
                const char* pExtn = strrchr(pName, '.');
                int baseLen = int(pExtn - pName);
                if (pExtn == NULL)
                    pExtn = strchr(pName, '\0');
                else
                    pExtn++;        // skip over dot.
                int remLen = 2;

                bool didPartReplace = true;
                switch (dstFile[dstIdx+1])
                {
                case 'e':   // extn
                    ReplaceString(dstFile, dstIdx, remLen, pExtn, strlen(pExtn));
                    break;
                case 'b':   // base
                    ReplaceString(dstFile, dstIdx, remLen, pName, baseLen);
                    break;
                case 'n':   // name = base.ext
                    ReplaceString(dstFile, dstIdx, remLen, pName, strlen(pName));
                    break;
                case 'f':   // full path
                    ReplaceString(dstFile, dstIdx, remLen, pSrcFile, strlen(pSrcFile));
                    break;
                case 'd':   // directory
                    ReplaceString(dstFile, dstIdx, remLen, pSrcFile, dirLen);
                    break;
                case 'l':   // lowercase name
                    dstFile.erase(dstIdx, 2);
                    dstIdx--;
                    _strlwr_s(pModName, strlen(pModName)+1);
                    break;
                case 'q':   // quote
                    ReplaceString(dstFile, dstIdx, remLen, "\"", 1);
                    break;
                case 'u':   // uppercase name
                    dstFile.erase(dstIdx, 2);
                    dstIdx--;
                    _strupr_s(pModName, strlen(pModName)+1);
                    break;
                case 'c':   // capitalize name
                    dstFile.erase(dstIdx, 2);
                    dstIdx--;
                    _strlwr_s(pModName, strlen(pModName)+1);
                    *pModName = ToUpper(*pName);
                    break;
                default:
                    didPartReplace = false;
                    break;
                }

                didReplace |= didPartReplace;
            }
        }
    }

    return didReplace;
}

// ---------------------------------------------------------------------------
static bool SetColorBit(WORD& colorCfg,
        const char* colorOptStr,
        const char* wordStr,
        WORD fgColor,
        WORD bgColor)
{
    int wordLen = strlen(wordStr);
    const char* match;
    bool ok = false;
    while ((match = strstr(colorOptStr, wordStr)) != NULL)
    {
        if (match[wordLen] == 'b')
            colorCfg |= bgColor;
        else
            colorCfg |= fgColor;
        colorOptStr = match + 1;
        ok = true;
    }

    return ok;
}

// ---------------------------------------------------------------------------
bool SetColor(WORD& colorCfg, const char* colorOptStr)
{
    // Parse keywords Red, Green and Blue
    //  <color>[b][+<color>[b]>]...
    // Ex:
    //  red+blue        = red and blue foreground
    //  green+redb      = green foreground, red background
    //
    colorCfg = 0;
    char colorWord[80];
    int idx;

    // Copy up to '|' or null.
    for (idx = 0; idx < ARRAYSIZE(colorWord)-1 && (NotEOC(colorOptStr[idx]));  idx++)
    {
        colorWord[idx] = (char)tolower(colorOptStr[idx]);
    }

    colorWord[idx] = '\0';

    WORD FOREGROUND_WHITE = FOREGROUND_RED + FOREGROUND_GREEN + FOREGROUND_BLUE;
    WORD BACKGROUND_WHITE = BACKGROUND_RED + BACKGROUND_GREEN + BACKGROUND_BLUE;
    bool valid = false;
    valid |= SetColorBit(colorCfg, colorWord, "red", FOREGROUND_RED, BACKGROUND_RED);
    valid |= SetColorBit(colorCfg, colorWord, "green", FOREGROUND_GREEN, BACKGROUND_GREEN);
    valid |= SetColorBit(colorCfg, colorWord, "blue", FOREGROUND_BLUE, BACKGROUND_BLUE);
    valid |= SetColorBit(colorCfg, colorWord, "white", FOREGROUND_WHITE, BACKGROUND_WHITE);
    valid |= SetColorBit(colorCfg, colorWord, "black", 0, 0);
    valid |= SetColorBit(colorCfg, colorWord, "intensity", FOREGROUND_INTENSITY, BACKGROUND_INTENSITY);
    return valid;
}

// ---------------------------------------------------------------------------
// Has statics - not thread safe.
const char* RunExtension(std::string& exeName)
{
    // Cache results - return previous match
    static const char* s_extn = NULL;
    static std::string s_lastExeName;
    if (s_lastExeName == exeName)
        return s_extn;
    s_lastExeName = exeName;

    /*
    static char ext[_MAX_EXT];
    _splitpath(exeName.c_str(), NULL, NULL, NULL, ext);

    if (ext[0] == '.')
        return ext;
    */

    // Expensive - search PATH for executable.
    char fullPath[MAX_PATH];
    static const char* s_extns[] = { NULL, ".exe", ".com", ".cmd", ".bat", ".ps" };
    for (unsigned idx = 0; idx != ARRAYSIZE(s_extns); idx++)
    {
        s_extn = s_extns[idx];
        DWORD foundPathLen = SearchPath(NULL, exeName.c_str(), s_extn, ARRAYSIZE(fullPath), fullPath, NULL);
        if (foundPathLen != 0)
            return s_extn;
    }

    return NULL;
}

// ---------------------------------------------------------------------------
bool RunCommand(const char* command, DWORD* pExitCode, int waitMsec)
{              
    std::string tmpCommand(command);
    const char* pEndExe = strchr(command, ' ');
    if (pEndExe == NULL)
        pEndExe = strchr(command, '\0');
    std::string exeName(command, pEndExe);
    
    const char* exeExtn = RunExtension(exeName);
    static const char* s_extns[] = { ".cmd", ".bat", ".ps" };
    if (exeExtn != NULL)
    {
        for (unsigned idx = 0; idx != ARRAYSIZE(s_extns); idx++)
        {
            const char* extn = s_extns[idx];
            if (strcmp(exeExtn, extn) == 0)
            {
                // Add .bat or .cmd to executable name.
                tmpCommand = exeName + extn + pEndExe;
                break;
            }
        }
    }

    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ClearMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ClearMemory(&pi, sizeof(pi));

    // Start the child process.
    if( !CreateProcess(
        NULL,   // No module name (use command line)
        (LPSTR)tmpCommand.c_str(), // Command line
        NULL,           // Process handle not inheritable
        NULL,           // Thread handle not inheritable
        FALSE,          // Set handle inheritance to FALSE
        0,              // No creation flags
        NULL,           // Use parent's environment block
        NULL,           // Use parent's starting directory
        &si,            // Pointer to STARTUPINFO structure
        &pi))           // Pointer to PROCESS_INFORMATION structure
    {
        return false;
    }

    // Wait until child process exits.
    DWORD createStatus = WaitForSingleObject(pi.hProcess, waitMsec);

    if (pExitCode)
    {
        *pExitCode = createStatus;
        if (createStatus == 0 && !GetExitCodeProcess(pi.hProcess, pExitCode))
            *pExitCode = (DWORD)-1;
    }

    // Close process and thread handles.
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return true;
}


//-----------------------------------------------------------------------------
// Read filenames from input file or stdin if "-"
int ReadFileList(
    const char* fileListName,
    DoFileCb doFileCb,
    void* cbData,
    ErrFileCb errFileCb)
{
    FILE* fin = stdin;

    if (strcmp(fileListName, "-") != 0 &&
        0 != fopen_s(&fin, fileListName, "rt"))
        return errno;

    char fileName[MAX_PATH];
    BY_HANDLE_FILE_INFORMATION fileInfo;
    WIN32_FIND_DATA findData;

    while (fgets(fileName, ARRAYSIZE(fileName), fin))
    {
        TrimString(fileName);       // remove extra space or control characters.
        if (*fileName == '\0')
            continue;

        Handle hFile = CreateFile(fileName, FILE_READ_ATTRIBUTES, 7, 0, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, 0);
        if (hFile.IsValid())
        {
            bool okay = (GetFileInformationByHandle(hFile, &fileInfo) != 0);
            // CloseHandle(hFile);

            if (okay)
            {
                strcpy_s(findData.cFileName, ARRAYSIZE(findData.cFileName), fileName);
                char* pDir = ".";

                char* pEndDir = strchr(fileName, '\\');
                if (pEndDir)
                {
                    *pEndDir = '\0';
                    strcpy_s(findData.cFileName, ARRAYSIZE(findData.cFileName), pEndDir+1);
                    pDir = fileName;
                }

                findData.dwFileAttributes = fileInfo.dwFileAttributes;
                findData.ftCreationTime = fileInfo.ftCreationTime;
                findData.ftLastAccessTime = fileInfo.ftLastAccessTime;
                findData.ftLastWriteTime = fileInfo.ftLastWriteTime;
                findData.nFileSizeHigh = fileInfo.nFileSizeHigh;
                findData.nFileSizeLow = fileInfo.nFileSizeLow;

                // LLMsg::Out() << "Filename:" << findData.cFileName << std::endl;
                int status = doFileCb(cbData, pDir, &findData, 0);
                if (status == sError && errFileCb)
                    errFileCb(cbData, fileName, &findData);
            }
        }
        else
        {
            if (errFileCb)
                errFileCb(cbData, fileName, NULL);
        }
    }

    fclose(fin);
    return 0;
}

//-----------------------------------------------------------------------------
int StrCspn(const char* inStr, const char* separators)
{
    int idx = 0;
    while (inStr[idx] > sEOCchr && strchr(separators, inStr[idx]) == NULL)
    {
        idx++;
    }
    return idx;
}

//-----------------------------------------------------------------------------

typedef std::vector<std::string> strList;
int GetStringList(
    std::vector<std::string>& outList,
    const char* inStr,
    const char* separators)
{
    if (inStr == NULL || *inStr <= sEOCchr)
        return -1;

    if (separators == NULL || *separators == '\0')
        return -1;

    int idx;
    while ((idx = StrCspn(inStr, separators)) >= 0)
    {
        if (idx > 0)
        {
            std::string token = inStr;
            token.resize(idx);
            outList.push_back(token);
        }
        inStr += idx;
        if (*inStr <= sEOCchr)
            break;
        inStr++;
    }

    if (*inStr > sEOCchr)
        outList.push_back(inStr);

    return (int)outList.size();
}

//-----------------------------------------------------------------------------
bool GetFileInfo(const char* path, BY_HANDLE_FILE_INFORMATION& fileInfo)
{
	if (strcmp(path, "-") == 0)
	{
		memset(&fileInfo, 0, sizeof(fileInfo));
		return false;
	}

    bool okay = false;
    DWORD SHARE_ALL = FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE;

    Handle fileHnd = CreateFile(path, 0 /* GENERIC_READ */, SHARE_ALL, 0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
    bool  dstExists = fileHnd.IsValid();
    if (dstExists)
    {
        if (GetFileInformationByHandle(fileHnd, &fileInfo) != FALSE)
            okay = true;
        else
            ZeroMemory(&fileInfo, sizeof(fileInfo));

        // CloseHandle(fileHnd);
    }

    return okay;
}

#if 0
//-----------------------------------------------------------------------------
bool SetFileModTime(const char *path)
{
    Handle fh = CreateFile(path, GENERIC_READ | FILE_WRITE_ATTRIBUTES, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (fh.NotValid())
        return false;

    FILETIME modtime;
    SYSTEMTIME st;

    if (GetFileTime(fh, NULL, NULL, &modtime) != 0)
    {
        FileTimeToSystemTime(&modtime, &st);
        GetSystemTime(&st);
        SystemTimeToFileTime(&st, &modtime);
        bool okay = (SetFileTime(fh, NULL, NULL, &modtime) != 0);
        // CloseHandle(fh);
        return okay;
    }

    return false;
}
#endif

//-----------------------------------------------------------------------------
bool SetFileModTime(const char* dstPath, const FILETIME& fileUtcTime)
{
    Handle fh = CreateFile(dstPath, GENERIC_READ | FILE_WRITE_ATTRIBUTES, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (fh.NotValid())
        return false;

    bool okay = (SetFileTime(fh, NULL, NULL, &fileUtcTime) != 0);
    // CloseHandle(fh);
    return okay;
}

//-----------------------------------------------------------------------------
// Set Destination modify time to Source modify time.
bool SetFileModTime(const char* dstPath, const char* srcPath)
{
    Handle srcHnd = CreateFile(srcPath, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (srcHnd.NotValid())
        return false;

    FILETIME modtime;

    BY_HANDLE_FILE_INFORMATION srcFileInfo;
    // if (GetFileTime(srcHnd, NULL, NULL, &modtime) != 0)
    // {
    if (GetFileInformationByHandle(srcHnd, &srcFileInfo))
    {
        modtime  = srcFileInfo.ftLastWriteTime;
        // CloseHandle(srcHnd);

        Handle dstHnd = CreateFile(dstPath, GENERIC_READ | FILE_WRITE_ATTRIBUTES, 0,
                NULL, OPEN_EXISTING, 0, NULL);
        if (dstHnd.NotValid())
            return false;

        bool okay = (SetFileTime(dstHnd, NULL, NULL, &modtime) != 0);
        // CloseHandle(dstHnd);

        return okay;
    }

    return false;
}

//-----------------------------------------------------------------------------
// Return difference (ft1-ft2)%mod
int FileTimeDifference(const FILETIME& ft1, const FILETIME& ft2, const ULONGLONG& resolution)
{
    LARGE_INTEGER LIft1, LIft2;
    LIft1.HighPart = ft1.dwHighDateTime;
    LIft1.LowPart = ft1.dwLowDateTime;
    LIft2.HighPart = ft2.dwHighDateTime;
    LIft2.LowPart = ft2.dwLowDateTime;

    LONGLONG delta = LIft1.QuadPart - LIft2.QuadPart;
    delta = delta / resolution;
    return (int)delta;
}

//-----------------------------------------------------------------------------
inline FILETIME UnixTimeToFileTime(time_t t, FILETIME& fileTime)
{
    // Note that LONGLONG is a 64-bit value
    LONGLONG ll;

    ll = Int32x32To64(t, 10000000) + 116444736000000000;
    fileTime.dwLowDateTime = (DWORD)ll;
    fileTime.dwHighDateTime = ll >> 32;
    return fileTime;
}

//-----------------------------------------------------------------------------
//  Parse Time
//      now
//      latest
//      [+-]n(Day|Hour|Minute)  ; relative
//      yyyy:mm:dd:hh:mm:ss     ; fields used from ss to yyyy, ex: hh:mm:ss
//
//
bool ParseTime(const char*& inStr, FILETIME& outFileUtcTime)
{
    time_t utcSeconds = time(0);


    std::vector<std::string> timePieces;

    if (-1 == GetStringList(timePieces, inStr, ":, /"))
        return false;

    if (timePieces.size() <= 1)
    {
        const lstring tmStr = timePieces[0];
        if (_stricmp(tmStr, "now") == 0 || _stricmp(tmStr, "latest") == 0)
        {
            UnixTimeToFileTime(utcSeconds, outFileUtcTime);
            inStr += tmStr.length();
            return true;
        }

        char* nPtr = NULL;
        double tValue = strtod(tmStr, &nPtr);
        if (nPtr == inStr)
        {
            ErrorMsg() << "Invalid time format:" << inStr << "\n";
            return false;       // bad time
        }

        int seconds = 0;
        switch (towlower(*nPtr))
        {
        case 'd':
            seconds += int(tValue * 24*60*60);
            break;
        case sEOCchr:
        case 'h':
            seconds += int(tValue * 60*60);
            break;
        case 'm':
            seconds += int(tValue * 60);
            break;
        default:
            ErrorMsg() << "Invalid time format:" << inStr << "\n"
                    "Try [+-]n(Day|Hour|Minute)  or  yyyy:mm:dd:hh:mm [z]\n";
            return false;
        }

        UnixTimeToFileTime(utcSeconds + seconds, outFileUtcTime);

        inStr += tmStr.length();
        return true;
    }
    else
    {
        // Build time from pieces.
        //      yyyy:mm:dd:hh:mm:ss
        // Use * to leave field as is.
        //      10:12:*:*  dd=10,hh=12,mm=no change,ss=no change.

        struct tm tmLocValues;
        localtime_s(&tmLocValues, &utcSeconds);

        int timeParts[6]; // year, month, day, hour, minute, seconds
        timeParts[0] = tmLocValues.tm_year + 1900;
        timeParts[1] = tmLocValues.tm_mon + 1; // 1..12
        timeParts[2] = tmLocValues.tm_mday;
        timeParts[3] = tmLocValues.tm_hour;
        timeParts[4] = tmLocValues.tm_min;
        timeParts[5] = tmLocValues.tm_sec;

        size_t tIdx = 6 - timePieces.size();
        for (uint tPieceIdx = 0; tPieceIdx != timePieces.size(); ++tPieceIdx)
        {
            char* nPtr = NULL;
            int tValue = strtol(timePieces[tPieceIdx].c_str(), &nPtr, 10);
            if (nPtr != timePieces[tPieceIdx].c_str())
            {
                timeParts[tIdx] = tValue;
                tIdx++;
            }
            else if (timePieces[tPieceIdx] != "*")
            {
                ErrorMsg() << "Bad Time Field:" << timeParts[tIdx]
                        << "\nTry [+-]n(Day|Hour|Minute)  or  yyyy:mm:dd:hh:mm:ss \n";
                return false;
            }
        }

        tmLocValues.tm_year = timeParts[0]- 1900;
        tmLocValues.tm_mon  = timeParts[1]- 1; // 0..11
        tmLocValues.tm_mday = timeParts[2];
        tmLocValues.tm_hour = timeParts[3];
        tmLocValues.tm_min  = timeParts[4];
        tmLocValues.tm_sec  = timeParts[5];

        time_t utcTime = _mkgmtime(&tmLocValues);
        UnixTimeToFileTime(utcTime, outFileUtcTime);

        inStr += strcspn(inStr, sEOCstr);
        return true;
    }
}

// ---------------------------------------------------------------------------
std::ostream& Format(std::ostream& out, const FILETIME& utcFT, bool asLocal)
{
    FILETIME   localtzFT;
    SYSTEMTIME sysTime;

    if (asLocal)
    {
        FileTimeToLocalFileTime(&utcFT, &localtzFT);    // Convert UTC to local Timezone
        FileTimeToSystemTime(&localtzFT, &sysTime);
    }
    else
    {
        FileTimeToSystemTime(&utcFT, &sysTime);
    }

    char szLocalDate[255], szLocalTime[255];
    szLocalDate[0] = szLocalTime[0] = '\0';

    // GetDateFormat(LOCALE_SYSTEM_DEFAULT, DATE_SHORTDATE, &sysTime, NULL, szLocalDate, sizeof(szLocalDate) );
    GetDateFormat(LOCALE_SYSTEM_DEFAULT, 0, &sysTime, "MM'/'dd'/'yyyy", szLocalDate, ARRAYSIZE(szLocalDate) );
    GetTimeFormat(LOCALE_SYSTEM_DEFAULT, TIME_NOSECONDS, &sysTime, "hh':'mm tt", szLocalTime, ARRAYSIZE(szLocalTime) );

    out << PadRight(szLocalDate, sizeof(szLocalDate), 10, ' ')
        << ' '
        << PadRight(szLocalTime, sizeof(szLocalTime), 9, ' ');

    return out;
}

//-----------------------------------------------------------------------------
// if time  -T=[fields]<operator><value>
// fields = c=create, m=modify, a=access
// operator = Greater, Equal, Less
// value = now|+/-hours|yyyy:mm:dd:hh:mm:ss \n"
// Examples:
//   -T=mcGnow       Show if modify or create time Greater than now\n
//   -T=mG-4.5       Show if modify time Greater than 4.5 hours ago\n
//   -T=aL04:00:00   Show if access time Less than 4am today\n
//
const char* ParseTimeOp(
        const char* cmdOpts,
        bool& valid,
        TimeFields& fields,
        SizeOp& timeOp,
        FILETIME& utcTime)
{
    valid = false;
    fields = eTestNoTime;
    bool done = false;

    if (*cmdOpts == '=')    // Skip over optional equal sign
        cmdOpts++;

    while (*cmdOpts && !done)
    {
        switch (ToLower(*cmdOpts))
        {
        case 'a':
            fields = TimeFields(fields | eTestAccessTime);
            break;
        case 'c':
            fields = TimeFields(fields |eTestCreationTime);
            break;
        case 'm':
            fields = TimeFields(fields | eTestModifyTime);
            break;
        default:
            done = true;
            cmdOpts--;
            break;
        }
        cmdOpts++;
    }

    switch (ToLower(*cmdOpts++))
    {
        case '=':
        case 'e':
            timeOp = LLSup::eOpEqual;
            break;
        case '<':
        case 'l':
            timeOp = LLSup::eOpLess;
            break;
        case '>':
        case 'g':
            timeOp = LLSup::eOpGreater;
            break;
        default:
            --cmdOpts;
            ErrorMsg() << "Unknown time option: " << cmdOpts << std::endl;
            return cmdOpts;
    };

#if 0
// Doesnt work - eats up "now" as in -T=mGnow
    while (NotEOC(*cmdOpts) && !(isdigit(*cmdOpts) || *cmdOpts == '-'))
    {
        cmdOpts++;
    }
#endif

    valid = ParseTime(cmdOpts, utcTime);
    if ( !valid)
        ErrorMsg() << "Unknown time option: " << cmdOpts-1 << std::endl;
#if 0
    else
    {
        LLMsg::Out() << "TimeOp time Local ";
        LLSup::Format(LLMsg::Out(), utcTime, true);
        LLMsg::Out() << std::endl;
    }
#endif
    return cmdOpts;
}

//-----------------------------------------------------------------------------
// Use to debug time test
int CompareFileTimeDbg(const FILETIME* ft1, const FILETIME* ft2, const WIN32_FIND_DATA* pDirEntry)
{
    int diff = CompareFileTime(ft1, ft2);

    LLMsg::Out() << pDirEntry->cFileName << " ";
    LLSup::Format(LLMsg::Out(), *ft1, true);
    LLMsg::Out() << " -  Ref:";
    LLSup::Format(LLMsg::Out(), *ft2, true);
    LLMsg::Out() << " diff=" << diff;
    LLMsg::Out() << std::endl;

    return diff;
}

//-----------------------------------------------------------------------------
// Return true if time operation passes.
// If multiple time fields, return true if any pass. 

bool TimeOperation(
    const WIN32_FIND_DATA* pDirEntry,
    SizeOp timeOp,
    TimeFields fields,
    FILETIME& refFtime)
{
    bool isAccTm = (fields & eTestAccessTime) != 0;
    bool isCrtTm = (fields & eTestCreationTime) != 0;
    bool isModTm = (fields & eTestModifyTime) != 0;

    switch (timeOp)
    {
    case eOpEqual:
        if (isAccTm && CompareFileTime(&pDirEntry->ftLastAccessTime, &refFtime) == 0)
            return true;
        if (isCrtTm && CompareFileTime(&pDirEntry->ftCreationTime, &refFtime) == 0)
            return true;
        if (isModTm && CompareFileTime(&pDirEntry->ftLastWriteTime, &refFtime) == 0)
            return true;
        break;
    case eOpGreater:
        if (isAccTm && CompareFileTime(&pDirEntry->ftLastAccessTime, &refFtime) > 0)
            return true;
        if (isCrtTm && CompareFileTime(&pDirEntry->ftCreationTime, &refFtime) > 0)
            return true;
        if (isModTm && CompareFileTime(&pDirEntry->ftLastWriteTime, &refFtime) > 0)
            return true;
        break;
    case eOpLess:
        if (isAccTm && CompareFileTime(&pDirEntry->ftLastAccessTime, &refFtime) < 0)
            return true;
        if (isCrtTm && CompareFileTime(&pDirEntry->ftCreationTime, &refFtime) < 0)
            return true;
        if (isModTm && CompareFileTime(&pDirEntry->ftLastWriteTime, &refFtime) < 0)
            return true;
        break;
    case eOpNone:
         return true;
    }

    return false;
}

//-----------------------------------------------------------------------------
// if exit status 
//  -e<op><value>  where op: [e=]=equals, [<l]=less, [>g]=greaer, [#n]=not equal
//  Examples: -e==10  -e=g0 -e=l-2  -e=#1
//  Looping,  -e==0L10   While exit is 0, execute up to 10 times.

const char* 
ParseExitOp(
	const char* cmdOpts,
	SizeOp& exitOp,
	LONGLONG& exitValue,
	uint& loopCnt,
	const char* errMsg)
{
	const char* exitStr = cmdOpts;
	LLMsg::Out() << "Exit status ";
	switch (exitStr[1])
	{
	case '=':
	case 'e':
		exitOp = LLSup::eOpEqual;
		LLMsg::Out() << "equal ";
		break;
	case '<':
	case 'l':
		exitOp = LLSup::eOpLess;
		LLMsg::Out() << "lessThan ";
		break;
	case '>':
	case 'g':
		exitOp = LLSup::eOpGreater;
		LLMsg::Out() << "greaterThan ";
		break;
	case '#':
	case 'n':
		exitOp = LLSup::eOpNotEqual;
		LLMsg::Out() << "notEqual ";
		break;
	default:
		LLMsg::Out() << errMsg << " Unknown option: " << exitOp << std::endl;
		return cmdOpts;
	};

	char* endPtr = (char*)exitStr;
	exitValue = (exitStr[1] != '\0') ? strtoul(exitStr + 2, &endPtr, 0) : 0;
	loopCnt = 1;
	if (*endPtr == 'L')
		loopCnt = strtoul(endPtr + 1, &endPtr, 0);
	
	cmdOpts = endPtr;
	LLMsg::Out() << " " << exitValue << std::endl;

	return cmdOpts;
}


//-----------------------------------------------------------------------------
// if siZe  -Z=1000K  or -Z<1M or -Z>1G

const char* ParseSizeOp(
        const char* cmdOpts,
        SizeOp& onlySizeOp,
        LONGLONG& onlySize)
{
    const char* sizeOp = cmdOpts;
    LLMsg::Out() << "Compare size ";
    switch (sizeOp[0])
    {
        case '=':
        case 'e':
            onlySizeOp = LLSup::eOpEqual;
            LLMsg::Out() << "equal ";
            break;
        case '<':
        case 'l':
            onlySizeOp = LLSup::eOpLess;
            LLMsg::Out() << "lessThan ";
            break;
        case '>':
        case 'g':
            onlySizeOp = LLSup::eOpGreater;
            LLMsg::Out() << "greaterThan ";
            break;
        default:
            LLMsg::Out() << "Unknown size option: " << sizeOp << std::endl;
            return cmdOpts;
    };

    char* endPtr = (char*)sizeOp;
    onlySize = (sizeOp[0] != '\0') ? strtoul(sizeOp+1, &endPtr, 0) : 0;
    switch (ToLower(*endPtr))
    {
    case 'k':
        onlySize *= (1 << 10);
        break;
    case 'm':
        onlySize *= (1 << 20);
        break;
    case 'g':
        onlySize *= (1 << 30);
        break;
    }

    cmdOpts = endPtr;
    LLMsg::Out() << " " << onlySize << std::endl;

    return cmdOpts;
}

//-----------------------------------------------------------------------------
// Return true if size operation passes.

bool SizeOperation(
    LONGLONG fileSize,
    SizeOp onlySizeOp,
    LONGLONG onlySize)
{
    switch (onlySizeOp)
    {
    case eOpNone:
        return true;

    case eOpEqual:
        if (fileSize != onlySize)
        {
            // LLMsg::Out() << fileSize.QuadPart << " != " << pLLDel->m_onlySize.QuadPart
            //    << " " << srcFile << std::endl;
            return false;
        }
        break;
    case eOpGreater:
        if (fileSize <= onlySize)
        {
            // LLMsg::Out() << fileSize.QuadPart << " <= " << pLLDel->m_onlySize.QuadPart
            //    << " " << srcFile << std::endl;
            return false;
        }
        break;
    case eOpLess:
        if (fileSize >= onlySize)
        {
            // LLMsg::Out() << fileSize.QuadPart << " >= " << pLLDel->m_onlySize.QuadPart
            //   << " " << srcFile << std::endl;
            return false;
        }
        break;
    }

    return true;
}

// ---------------------------------------------------------------------------
//  Parse   -X*.exe,*.lib;p???.dat,foo.bar
//        -F... where ... is not for us or  -F=*.exe,*.lib
//  The equal sign is used to indicate the following text is part of the command
const char* ParseList(
    const char* cmdOpts,
    StringList& excludeList,
    const char* emptyMsg)
{
    if (*cmdOpts == sEQchr)
    {
        cmdOpts++;
        while (*cmdOpts > sEOCchr)
        {
            std::string pattern = cmdOpts;
            int len = strcspn(cmdOpts, ",;" sEOCstr);
            pattern.resize(len);
            if (len != 0)
                excludeList.push_back(pattern);
            cmdOpts += len;

            if (*cmdOpts > sEOCchr)
                cmdOpts++;
        }

        if (excludeList.empty() && emptyMsg)
            ErrorMsg() << emptyMsg;
    }
    else
    {
        cmdOpts--;
        if (emptyMsg)
            ErrorMsg() << emptyMsg;
    }

    return cmdOpts;
}

// ---------------------------------------------------------------------------
bool PatternListMatches(const StringList& patList, const char* fileName, bool emptyResult)
{
    if (patList.empty())
        return emptyResult;

    for (int lstIdx = patList.size() -1; lstIdx >= 0; lstIdx--)
    {
        if (PatternMatch(patList[lstIdx].c_str(), fileName))
            return true;
    }

    return false;
}

// ---------------------------------------------------------------------------
// Parse:  -A=[nrhs]     ; show only (n=normal r=readonly, h=hidden, s=system)\n"
const char* ParseAttributes(const char* cmdOpts,  DWORD& attributes, const char* errMsg)
{
    attributes = 0;

    if (*cmdOpts == sEQchr)
    {
        cmdOpts++;
        while (*cmdOpts > sEOCchr)
        {
            switch (ToLower(*cmdOpts))
            {
            case 'n': attributes |= FILE_ATTRIBUTE_NORMAL; break;   // normal
            case 'w': attributes |= FILE_ATTRIBUTE_NORMAL; break;   // writable
            case 'r': attributes |= FILE_ATTRIBUTE_READONLY; break; // readonly
            case 'h': attributes |= FILE_ATTRIBUTE_HIDDEN; break;   // hidden
            case 's': attributes |= FILE_ATTRIBUTE_SYSTEM; break;   // system
            default:
                if (errMsg) ErrorMsg() << errMsg;
                return cmdOpts;
            }
            cmdOpts++;
        }
    }
    else
    {
        cmdOpts--;
        if (errMsg)
            ErrorMsg() << errMsg;
    }

    return cmdOpts;
}

// ---------------------------------------------------------------------------
//  Parse out a "string" parameter
//  -i=file

const char* ParseString(const char* cmdOpts, std::string& outString, const char* errMsg)
{
    if (*cmdOpts == sEQchr)
    {
        cmdOpts++;
        int len = strcspn(cmdOpts, sEOCstr);
        outString.assign(cmdOpts, len);
        cmdOpts += len;
        // if (*cmdOpts != sEOC)
        //     cmdOpts++;
    }
    else
    {
        cmdOpts--;
        if (errMsg)
            ErrorMsg() << errMsg;
    }

    return cmdOpts;
}

// ---------------------------------------------------------------------------
//  Parse out a "number" parameter

const char* ParseNum(const char* cmdOpts, uint& unum, const char* errMsg)
{
    if (*cmdOpts == sEQchr)
    {
        char* endPtr = 0;
        unum = (uint)strtoul(cmdOpts+1, &endPtr, 10);
        if (endPtr != cmdOpts+1)
            cmdOpts = endPtr-1;
    }
    else
    {
        cmdOpts--;
        if (errMsg)
            ErrorMsg() << errMsg;
    }

    return cmdOpts;
}

// ---------------------------------------------------------------------------
//  Parse out a "number" parameter

const char* ParseNum(const char* cmdOpts, long& num, const char* errMsg)
{
    if (*cmdOpts == sEQchr)
    {
        char* endPtr = 0;
        num = strtol(cmdOpts+1, &endPtr, 10);
        if (endPtr != cmdOpts+1)
            cmdOpts = endPtr-1;
    }
    else
    {
        cmdOpts--;
        if (errMsg)
            ErrorMsg() << errMsg;
    }

    return cmdOpts;
}

// ---------------------------------------------------------------------------
//  Parse out a "number" parameter
//  -o=offset64

const char* ParseNum(const char* cmdOpts, long long& llnum, const char* errMsg)
{
    if (*cmdOpts == sEQchr)
    {
        char* endPtr = 0;
        llnum = _strtoi64(cmdOpts+1, &endPtr, 10);
        if (endPtr != cmdOpts+1)
            cmdOpts = endPtr-1;
    }
    else
    {
        cmdOpts--;
        if (errMsg)
            ErrorMsg() << errMsg;
    }

    return cmdOpts;
}

// ---------------------------------------------------------------------------
const char* ParseOutput(const char* cmdOpts)
{
    static std::ofstream out;

    std::string outFile;
    cmdOpts = ParseString(cmdOpts, outFile, "Missing redirected output filename, -2=<filename>");
    if (outFile.length() != 0)
    {
        out.open(outFile.c_str(), ios::app);
        if (out)
            LLMsg::RedirectOutput(out);
        else
            perror(outFile.c_str());
    }

    return cmdOpts;
}

// ---------------------------------------------------------------------------
const char* ParseOutput(const char* cmdOpts, bool tee)
{
    static std::ofstream out;

    std::string outFile;
    cmdOpts = ParseString(cmdOpts, outFile, "Missing redirected output filename, -1=<filename>");
    if (outFile.length() != 0)
    {
        out.open(outFile.c_str(), ios::app);
        if (out)
            LLMsg::RedirectOutput(out, tee);
        else
            perror(outFile.c_str());
    }

    return cmdOpts;
}

// ---------------------------------------------------------------------------

#include <shellapi.h>
#pragma comment(lib, "shell32")

int RemoveFile(const char* filePath, bool DelToRecycleBin)
{
    if (DelToRecycleBin)
    {
        SHFILEOPSTRUCT fileop;
        ClearMemory(&fileop, sizeof(fileop));
        fileop.wFunc = FO_DELETE;
        fileop.fFlags = FOF_ALLOWUNDO |FOF_FILESONLY | FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOERRORUI;

        int len = strlen(filePath);
        assert(len < MAX_PATH);
        char filePathzz[MAX_PATH];

        strcpy_s(filePathzz, ARRAYSIZE(filePathzz), filePath);
        if (len+2 < MAX_PATH)
        {
            filePathzz[len+1] = '\0';
            fileop.pFrom = filePathzz;
            int result = SHFileOperation(&fileop);

            if (result == 0)
                return 0;

            _doserrno = result;    // EIO, ENOENT, EACCES, EBUSY, EEXIST
            return result;
        }

        // Filename to long
        return -1;
    }
    else
    {
        return remove(filePath);
    }
}

// ---------------------------------------------------------------------------

int RemoveDirectory(const char* dirPath, bool DelToRecycleBin)
{
    if (DelToRecycleBin)
    {
        SHFILEOPSTRUCT fileop;
        ClearMemory(&fileop,sizeof(fileop));
        fileop.wFunc = FO_DELETE;
        fileop.fFlags = FOF_ALLOWUNDO | FOF_NORECURSION | FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOERRORUI;

        int len = strlen(dirPath);
        assert(len < MAX_PATH);
        char dirPathzz[MAX_PATH];

        strcpy_s(dirPathzz, ARRAYSIZE(dirPathzz), dirPath);
        if (len+2 < MAX_PATH)
        {
            dirPathzz[len+1] = '\0';
            fileop.pFrom = dirPathzz;
            int result =  SHFileOperation(&fileop);
            if (result == 0)
                return 0;

            _doserrno = EACCES;    // EIO, ENOENT, EACCES, EBUSY, EEXIST
            return -1;
        }

        // Filename to long
        return -1;
    }
    else
    {
        return _rmdir(dirPath);
    }
}
// ---------------------------------------------------------------------------
bool CreateDirectories(const char* directories, int nDirs, bool doIt)
{
    bool madeDir = false;
    char ourDir[MAX_PATH];
    strcpy_s(ourDir, sizeof(ourDir), directories);
    DWORD error = 0;

    // Remove some directories or paths.
    for (int dirIdx = 0; dirIdx < nDirs; dirIdx++)
    {
        char* pSlash = strrchr(ourDir, '\\');
        if (pSlash)
            *pSlash = '\0';
        else
            break;
    }

    if ( !doIt)
    {
        if (_access(ourDir, 6) == -1 && errno == ENOENT)
            LLMsg::Out() << "MkDir " << ourDir << std::endl;
        return false;
    }

    // Make
    int backCnt = 0;
    while (CreateDirectory(ourDir, NULL) == 0)
    {
        error = GetLastError();
        if (error == ERROR_ALREADY_EXISTS)
            break;

        char* pSlash = strrchr(ourDir, '\\');
        if (pSlash == NULL)
            break;

        *pSlash = '\0';
        backCnt++;
    }

#if 0
    error = GetLastError();
    if (error == 0 || error == ERROR_NO_MORE_FILES)
        LLMsg::Out() << "MkDir " << ourDir << std::endl;
#endif

    while (backCnt-- > 0)
    {
        *strchr(ourDir, '\0') = '\\';
        if (CreateDirectory(ourDir, NULL) == 0)
            LLMsg::PresentError(GetLastError(), "MkDir ", ourDir);
        else
            madeDir = true;
    }

    return madeDir;
}

//-----------------------------------------------------------------------------
struct CopyDirTreeData
{
    bool            m_echo;
    bool            m_prompt;
    bool            m_copy;
    size_t          m_countFiles;
    size_t          m_countErrors;
    LARGE_INTEGER   m_countBytes;
    const char*     m_dstDir;
    int             m_srcDirLen;
    bool            m_makeDir;
};

// ---------------------------------------------------------------------------
static  DWORD __stdcall ProgressCb(
    LARGE_INTEGER totalFilesSize,
    LARGE_INTEGER totalCopied,
    LARGE_INTEGER streamSize,
    LARGE_INTEGER streamCopied,
    DWORD streamNumber,
    DWORD reason,
    HANDLE srcFile,
    HANDLE dstFile,
    LPVOID data)
{
    if (totalFilesSize.QuadPart != 0 && totalFilesSize.QuadPart < 100000)
        return PROGRESS_QUIET;

    char  msg[10];
    DWORD msgLen = 0;

	if (totalFilesSize.QuadPart == 0)
		msgLen = sprintf_s(msg, sizeof(msg), "%u ",(uint)totalCopied.LowPart);
	else
		msgLen = sprintf_s(msg, sizeof(msg), "%u%% ",
			(uint)(totalCopied.QuadPart*100/totalFilesSize.QuadPart));

#if 0
    HANDLE consoleHandle = (HANDLE)data;
    DWORD written;

    CONSOLE_SCREEN_BUFFER_INFO consoleScreenBufferInfo;
    GetConsoleScreenBufferInfo(consoleHandle, &consoleScreenBufferInfo);
    WriteConsoleA(consoleHandle, msg, msgLen, &written, 0);
    SetConsoleCursorPosition(consoleHandle, consoleScreenBufferInfo.dwCursorPosition);
#else
    std::cout << msg << "\r";
#endif

    return (totalCopied.QuadPart < totalFilesSize.QuadPart) ? PROGRESS_CONTINUE : PROGRESS_QUIET;
}

//-----------------------------------------------------------------------------
int CopyDirTreeCb(void * pData, const char* pDir, const WIN32_FIND_DATA * pFileData, int depth)
{
    int outCnt = 0;
    CopyDirTreeData* pCopyData = (CopyDirTreeData*)pData;
    bool isDir = ((pFileData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0);

    char srcFile[MAX_PATH];
    strcpy_s(srcFile, ARRAYSIZE(srcFile), pDir);
    strcat_s(srcFile, ARRAYSIZE(srcFile), "\\");
    strcat_s(srcFile, ARRAYSIZE(srcFile), pFileData->cFileName);

    char dstFile[MAX_PATH];
    strcpy_s(dstFile, ARRAYSIZE(dstFile), pCopyData->m_dstDir);
    strcat_s(dstFile, ARRAYSIZE(dstFile), srcFile + pCopyData->m_srcDirLen);

    if (isDir)
    {
        // Directory created when first file is written.
        pCopyData->m_makeDir = true;
    }
    else
    {
        if (pCopyData->m_echo || pCopyData->m_prompt)
        {
            outCnt = 1;
            LLMsg::Out() << "Copy " << srcFile << " to " << dstFile << std::endl;
        }

        if (pCopyData->m_prompt && pCopyData->m_copy)
        {
            char answer = '\0';
            std::cout << "  Yes/No/All/Quit ? ";
            do
            {
                if (answer) std::cout << " ? ";
                answer = GetChe();
            } while (strchr("ynaq", ToLower(answer)) == NULL);

            switch (ToLower(answer))
            {
            case 'a':
                pCopyData->m_prompt = false;
                std::cout << "ll\n";
                break;
            case 'n':
                std::cout << "o\n";
                // pCopyData->m_countPromptNo++;
                return outCnt;
            case 'q':
                std::cout << "uit!\n";
                exit(0);
                break;
            case 'y':
                std::cout << "es\n";
                break;
            }
        }

        if (pCopyData->m_copy)
        {
            if (pCopyData->m_makeDir)
            {
                pCopyData->m_makeDir = false;
                CreateDirectories(dstFile, 1, true);
            }

            //
            // CopyFileEx() to monitor progress
            //
            BOOL cancel = false;
            DWORD flags = COPY_FILE_FAIL_IF_EXISTS | COPY_FILE_COPY_SYMLINK;
            if (CopyFileEx(srcFile, dstFile, (LPPROGRESS_ROUTINE) &ProgressCb, pCopyData,
                &cancel, flags) == 0)
            {
                DWORD error = GetLastError();
                if (error)
                {
                    std::string errMsg;

                    ErrorMsg() << "Copy Error:("
                        << error
                        << ") " << LLMsg::GetErrorMsg(error)
                        << " " << srcFile << " " << dstFile
                        << std::endl;

                    pCopyData->m_countErrors++;
                }
            }
            else
            {
                pCopyData->m_countFiles++;
                pCopyData->m_countBytes.QuadPart += 1;   // TODO - get real size
            }
        }
    }

    return 1;
}

//-----------------------------------------------------------------------------
// Copy directory tree.
// Return -1 if any errors, else # of files copied.
int CopyDirTree(
        const char* srcDir, const char* dstDir,
        bool echo, bool prompt, bool copy,
        LONGLONG& countBytes)
{
    CopyDirTreeData copyData;
    copyData.m_echo = echo;
    copyData.m_prompt = prompt;
    copyData.m_copy = copy;
    copyData.m_countFiles = 0;
    copyData.m_countErrors = 0;
    copyData.m_countBytes.QuadPart = 0;
    copyData.m_dstDir = dstDir;
    copyData.m_srcDirLen = strlen(srcDir);
    copyData.m_makeDir = true;

    DirectoryScan  dirScan;
    dirScan.m_add_cb  = LLSup::CopyDirTreeCb;
    dirScan.m_cb_data = &copyData;
    dirScan.m_recurse = true;
    dirScan.m_filesFirst = false;

    dirScan.Init(srcDir, srcDir);
    // m_subDirCnt = dirScan.m_subDirCnt;
    size_t nFiles = dirScan.GetFilesInDirectory();

    countBytes = copyData.m_countBytes.QuadPart;

    return (copyData.m_countErrors == 0) ? copyData.m_countFiles : -(int)copyData.m_countErrors;
}

//-----------------------------------------------------------------------------
// Append source to end of destination (if it exists, else create)
DWORD AppendFile( 
		const char* srcFile,
		const char* dstFile,
		LPPROGRESS_ROUTINE  pProgreeCb,
		void* pData,
		BOOL* pCancel,
		DWORD flags)
{
	Handle hFile;
	Handle hAppend;
	DWORD  dwBytesRead, dwBytesWritten, dwPos;
	BYTE   buff[4096];

	hFile = CreateFile(srcFile, // open One.txt
		GENERIC_READ,             // open for reading
		FILE_SHARE_READ | FILE_SHARE_WRITE,   // do not share
		NULL,                     // no security
		OPEN_EXISTING,            // existing file only
		FILE_ATTRIBUTE_NORMAL,    // normal file
		NULL);                    // no attr. template

	if (hFile.NotValid())
		return GetLastError();

	if (strcmp(dstFile, "-") == 0)
		hAppend = GetStdHandle(STD_OUTPUT_HANDLE);
	else
		hAppend = CreateFile(dstFile, // open Two.txt
			FILE_APPEND_DATA,         // open for writing
			FILE_SHARE_READ,          // allow multiple readers
			NULL,                     // no security
			OPEN_ALWAYS,              // open or create
			FILE_ATTRIBUTE_NORMAL,    // normal file
			NULL);                    // no attr. template

	if (hAppend.NotValid())
		return GetLastError();

	// Append the first file to the end of the second file.
	// Lock the second file to prevent another process from
	// accessing it while writing to it. Unlock the
	// file when writing is complete.

	while (ReadFile(hFile, buff, sizeof(buff), &dwBytesRead, NULL)
		&& dwBytesRead > 0)
	{
		dwPos = SetFilePointer(hAppend, 0, NULL, FILE_END);
		// LockFile(hAppend, dwPos, 0, dwBytesRead, 0);
		WriteFile(hAppend, buff, dwBytesRead, &dwBytesWritten, NULL);
		// UnlockFile(hAppend, dwPos, 0, dwBytesRead, 0);
	}

	return 0;
}

//-----------------------------------------------------------------------------
// Append source to end of destination (if it exists, else create)
// https://msdn.microsoft.com/en-us/library/windows/desktop/aa365690(v=vs.85).aspx
DWORD CopyFollowFile(
	const char* srcFile,
	const char* dstFile,
	LPPROGRESS_ROUTINE  pProgreeCb,
	void* pData,
	BOOL* pCancel,
	DWORD flags)
{
	Handle hFile;
	Handle hAppend;
	DWORD  dwBytesRead, dwBytesWritten;
	BYTE   buff[4096];
	LARGE_INTEGER liWritten;
	LARGE_INTEGER liZero;
	LARGE_INTEGER liSrcSize;

	liWritten.QuadPart = liZero.QuadPart = 0;

	*pCancel = FALSE;
	if (strcmp(srcFile, "-") == 0)
		hAppend = GetStdHandle(STD_INPUT_HANDLE);
	else
		hFile = CreateFile(srcFile,		// open One.txt
			GENERIC_READ,				// open for reading
			FILE_SHARE_READ | FILE_SHARE_WRITE, 
			NULL,						// no security
			OPEN_EXISTING,				// existing file only
			FILE_ATTRIBUTE_NORMAL,		// normal file
			NULL);						// no attr. template

	if (hFile.NotValid())
		return GetLastError();

	if (strcmp(dstFile, "-") == 0)
		hAppend = GetStdHandle(STD_OUTPUT_HANDLE);
	else
		hAppend = CreateFile(dstFile,	// open Two.txt
			FILE_APPEND_DATA,			// open for writing
			FILE_SHARE_READ,			// allow multiple readers
			NULL,						// no security
			OPEN_ALWAYS,				// open or create
			FILE_ATTRIBUTE_NORMAL,		// normal file
			NULL);						// no attr. template

	if (hAppend.NotValid())
		return GetLastError();

	liSrcSize.LowPart = GetFileSize(hFile, (DWORD*)&liSrcSize.HighPart);

	const unsigned sProgressPeriodCnt = 50;
	unsigned progressPeriodCnt = sProgressPeriodCnt;
	while (*pCancel == FALSE)
	{
		// Append the first file to the end of the second file.
		// Lock the second file to prevent another process from
		// accessing it while writing to it. Unlock the
		// file when writing is complete.

		while (ReadFile(hFile, buff, sizeof(buff), &dwBytesRead, NULL)
			&& dwBytesRead > 0)
		{
			// dwPos = SetFilePointer(hAppend, 0, NULL, FILE_END);
			// LockFile(hAppend, dwPos, 0, dwBytesRead, 0);
			WriteFile(hAppend, buff, dwBytesRead, &dwBytesWritten, NULL);
			liWritten.QuadPart += dwBytesWritten;
			// UnlockFile(hAppend, dwPos, 0, dwBytesRead, 0);
		}

		::Sleep(100);
		if (--progressPeriodCnt == 0 && pProgreeCb != NULL)
		{
			progressPeriodCnt = sProgressPeriodCnt;
			pProgreeCb(liSrcSize, liWritten, liSrcSize, liWritten, 0, 0, hFile, hAppend, pData);
		}
	}
	return 0;
}

}

// ---------------------------------------------------------------------------
// Run process pointer, NULL if failed to start
PROCESS_INFORMATION* RunCommands::Run(const char* command)
{
    PROCESS_INFORMATION* pProcess = new PROCESS_INFORMATION;

    STARTUPINFO si;
    ClearMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ClearMemory(pProcess, sizeof(*pProcess));

    // Start the child process.
    if( !CreateProcess(
        NULL,   // No module name (use command line)
        (LPSTR)command, // Command line
        NULL,           // Process handle not inheritable
        NULL,           // Thread handle not inheritable
        FALSE,          // Set handle inheritance to FALSE
        0,              // No creation flags
        NULL,           // Use parent's environment block
        NULL,           // Use parent's starting directory
        &si,            // Pointer to STARTUPINFO structure
        pProcess))      // Pointer to PROCESS_INFORMATION structure
    {
        delete pProcess;
        return NULL;
    }

    m_processList.push_back(pProcess);
    return pProcess;
}

// ---------------------------------------------------------------------------
// Get Exit code for process, use index returned by RunCommand.
DWORD RunCommands::GetExitCode(PROCESS_INFORMATION* pProcess, bool wait)
{
    if (wait)
    {
        // Wait until child process exits.
        WaitForSingleObject(pProcess->hProcess, INFINITE);
        Close(pProcess);
    }

    DWORD exitCode = 0;
    if ( !GetExitCodeProcess(pProcess->hProcess, &exitCode))
        exitCode = (DWORD)-1;
    return exitCode;
}

// ---------------------------------------------------------------------------
void RunCommands::Close(PROCESS_INFORMATION* pProcess)
{
    // Close process and thread handles.
    CloseHandle(pProcess->hProcess);
    pProcess->hProcess = INVALID_HANDLE_VALUE;
    CloseHandle(pProcess->hThread);
    pProcess->hThread = INVALID_HANDLE_VALUE;

    ProcessList::iterator iter = std::find(m_processList.begin(), m_processList.end(), pProcess);
    if (iter != m_processList.end())
        delete *iter;
}

// ---------------------------------------------------------------------------
void RunCommands::Abort(PROCESS_INFORMATION* pProcess)
{
    TerminateProcess(pProcess->hProcess, (UINT)-1);
}

// ---------------------------------------------------------------------------
bool RunCommands::AllDone() const
{
    for (ProcessList::const_iterator iter = m_processList.begin();
        iter != m_processList.end();
        iter++)
    {
        const PROCESS_INFORMATION* pProcess = *iter;
        if (pProcess->hProcess != INVALID_HANDLE_VALUE  &&
            WaitForSingleObject(pProcess->hProcess, 0) == WAIT_TIMEOUT)
                return false;
    }

    return true;
}

// ---------------------------------------------------------------------------
void RunCommands::Clear()
{
    for (ProcessList::iterator iter = m_processList.begin();
        iter != m_processList.end();
        iter++)
    {
        Abort(*iter);
        Close(*iter);
        iter = m_processList.erase(iter);
    }
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
const char* SizeToString(LONGLONG size)
{
    static char str[40];

    if (size > MB)
        sprintf_s(str, sizeof(str), "%.1f(MB)", (double)size/MB);
    else if (size > KB)
        sprintf_s(str, sizeof(str), "%.1f(KB)", (double)size/KB);
    else
        sprintf_s(str, sizeof(str), "%u", (uint)(size));

    return str;
}


