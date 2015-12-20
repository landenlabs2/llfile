//-----------------------------------------------------------------------------
// llsupport - Support routines shared by LL files files.
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

#pragma once

#define NOCOMM

#include <iostream>
#include <fstream>
#include <vector>
#include <list>
#include <string>
#include <Windows.h>



#if defined(_UNICODE)
#define LLCHAR  "c16 "
#else
#define LLCHAR "c8 "
#endif

#if defined(__WIN64__) || defined(_WIN64) || defined(WIN64)
#define LLBLDBITS "(x64)"
#else
#define LLBLDBITS "(x32)"
#endif

#define Q(x) #x
#define QUOTE(x) Q(x)

 // C++ version _CPPLIB_VER 
#define LLVERSION "v16.1s" LLBLDBITS LLCHAR " C++_" QUOTE(_CPPLIB_VER)


#include "ll_stdhdr.h"
#include "llstring.h"
#include "hash.h"

// End-Of-Command, used to separate commands.
const char sEOCchr = '\01';
const char sEQchr = '=';
#define sEOCstr "\01"

inline bool IsEOC(char c)
{ return c <= sEOCchr; }
inline bool NotEOC(char c)
{ return c > sEOCchr; }

inline void ClearMemory(void* cPtr, size_t len)
{  memset(cPtr, 0, len); }

template <typename TT>
inline bool Contains(const std::string& str, TT value)
{ return str.find(value) != string::npos; }

#include <conio.h>
inline char GetCh()
{ return (char)_getch(); }
inline char GetChe()
{ return (char)_getche(); }

namespace LLSup
{

// bool GetCmdField(const char*& cmdOpts, char* cmdOutBuffer, size_t maxOutBuffer);
void AdvCmd(const char*& cmdOpts);
int CmdError(const char* cmdOpts);

// Convert wide string to narrow using locale
inline std::string ToNarrow(wchar_t const* wstr, const std::size_t len, std::string& outNarrow)
{
    std::vector<char> sBuffer;
    sBuffer.resize(len + 1);

    std::locale const loc("");
    std::use_facet<std::ctype<wchar_t> >(loc).narrow(wstr, wstr + len, '?', &sBuffer[0]);

	outNarrow = &sBuffer[0];
    return outNarrow;
}

inline std::string ToNarrow(const std::wstring& text, std::string& outNarrow)
{
	return ToNarrow(text.c_str(), text.size(), outNarrow);
}

// Get environment varible, return value else defStr
// pointer returned needs to be freed by caller
std::string GetEnvStr(const char* envName, const char* defStr);

#if 0
char* PadRight(char* str, size_t maxLen, size_t padLen, char padChr);
char* PadLeft(char* str, size_t maxLen, size_t padLen, char padChr);
#endif

// Replace part of destination string with new value.
std::string& ReplaceString(
        std::string& dstStr, uint offset, int removeLen,
        const char* addStr, int addLen);

// Replace parts of destination #<value> with parts of source file.
// Return true if one or more replacements occurred.
bool ReplaceDstWithSrc(
        std::string& pDstFile,
        const char* pSrcFile,
        const char* separators);

// Replace parts of destination *<value> with parts of source file.
// Return true if one or more replacements occurred.
bool ReplaceDstStarWithSrc(
        std::string& pDstFile,
        const char* pSrcFile,
        const char* pPattern,
        const char* separators);

// Parse color<b> into console cfg bits, return true if valid color.
// Ex:  red+blue  or green+whiteb or whitebackgrond+black or whiteblablabla+black.
bool SetColor(WORD& colorCfg, const char* colorOptStr);



// Run command
bool RunCommand(const char* command, DWORD* pExitCode = NULL, int waitMsec=20000);

// Compare physical device type (directory|device|normal)
inline bool CompareDeviceBits(DWORD file, DWORD show)
{
    const DWORD FILE_DEVICE_BITS = FILE_ATTRIBUTE_DIRECTORY |
            FILE_ATTRIBUTE_DEVICE |FILE_ATTRIBUTE_NORMAL;

    // Some files don't have NORMAL bit set, so help them and set it.
    if ((file & FILE_DEVICE_BITS) == 0)
        file |= FILE_ATTRIBUTE_NORMAL;

    return ((file & show & FILE_DEVICE_BITS) != 0);
}


// Read filenames from file and pass to callback
// return 0 if all okay, else error.
typedef int (*DoFileCb)(void* pLLDir, const char* pDir, const WIN32_FIND_DATA* pFileData, int depth);
typedef int (*ErrFileCb)(void* pLLDir, const char* errFile, const WIN32_FIND_DATA* pFileData);
int ReadFileList(const char* fileName, DoFileCb, void* cbData, ErrFileCb = 0);

enum SizeOp { eOpNone, eOpEqual, eOpGreater, eOpLess, eOpNotEqual };
const char* ParseExitOp(const char* cmdOpts, SizeOp& exitOp, LONGLONG& exitValue, uint& loopCnt, const char* errMsg);
// Parse size operator:   -Z=1000K  or -Z<1M or -Z>1G
const char* ParseSizeOp(const char* cmdOpts,  SizeOp& onlySizeOp, LONGLONG& onlySize);
// Return true if size operation passes.
bool SizeOperation(LONGLONG fileSize, SizeOp,  LONGLONG onlySize);

enum  TimeFields { eTestNoTime = 0, eTestAccessTime=1, eTestCreationTime=2, eTestModifyTime=4 };
const char* ParseTimeOp(const char* cmdOpts, bool& valid, TimeFields&, SizeOp&, FILETIME& localTime);
bool ParseTime(const char*& inStr, FILETIME& outFileUtcTime);
// Return true if time operation passes.
bool TimeOperation(const WIN32_FIND_DATA* pDirEntry, SizeOp, TimeFields, FILETIME& refTime);
FILETIME UnixTimeToFileTime(time_t t, FILETIME& fileTime);
std::ostream& Format(std::ostream& out, const FILETIME& utcFT, bool localTz=true);


// Get Fieinformation
bool GetFileInfo(const char* path, BY_HANDLE_FILE_INFORMATION& fileInfo);

// Set Destination modify time to Source modify time.
bool SetFileModTime(const char* dstPath, const char* srcPath);
bool SetFileModTime(const char* dstPath, const FILETIME& fileUtcTime);
// Return difference (ft1-ft2) / resolution
int FileTimeDifference(const FILETIME& ft1, const FILETIME& ft2, const ULONGLONG& resolution = 500e9);

typedef std::vector<std::string> StringList;

//  Parse -F or -X*.exe,*.lib;p???.dat,foo.bar
const char* ParseList(const char* cmdOpts, StringList& strList, const char* emptyErrMsg /* = NULL */);

// Return true if file matches pattern in list.
// Return emptyListResult if patList is empty.
bool PatternListMatches(const StringList& patList, const char* fileName, bool emptyListResult = false);

// Parse:  -A[nrhs]     ; show only (n=normal r=readonly, h=hidden, s=system)\n"
const char* ParseAttributes(const char* cmdOpts,  DWORD& attributes, const char* errMsg);

const char* ParseString(const char* cmdOpts, std::string& outString, const char* errMsg);
const char* ParseNum(const char* cmdOpts, uint& unum, const char* errMsg);
const char* ParseNum(const char* cmdOpts, long& unum, const char* errMsg);
const char* ParseNum(const char* cmdOpts, long long& llnum, const char* errMsg);

// Redirect output to provided file and optionally tee it also to console.
const char* ParseOutput(const char* cmdOpts, bool tee);


// Compare Readonly, Hidden and System bits.
inline bool CompareRhsBits(DWORD file, DWORD show)
{
    const DWORD RHS_BITS = FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM;
    return (show == 0                   // don't care, all match
        || (file & show) != 0           // matching bits
        || ((show & FILE_ATTRIBUTE_NORMAL) != 0 && (file & RHS_BITS)==0));  // normal RHS bits = 0
}


int RemoveFile(const char* filePath, bool DelToRecycleBin);
int RemoveDirectory(const char* dirPath, bool DelToRecycleBin);

// Create directories, nDirs=# to skip from right, return true if any dir made.
bool CreateDirectories(const char* directories, int nDirs, bool doIt);

// Copy directory tree.
// Return <0 error count, else # of files copied.
int CopyDirTree(const char* srcDir, const char* dstDir,
        bool echo, bool prompt, bool copy, LONGLONG& byteCount);

// Append src to dst, return GetLastError on error, else 0.  
// dst can be "-" to write to stdout.
DWORD AppendFile(const char* srcFile, const char* dstFile, LPPROGRESS_ROUTINE  pProgreeCb,
	void* pData, BOOL* pCancel, DWORD flags);

DWORD CopyFollowFile(const char* srcFile, const char* dstFile, LPPROGRESS_ROUTINE  pProgreeCb,
	void* pData, BOOL* pCancel, DWORD flags);
};

// Class to allow commands to run in the background.
class RunCommands
{
public:
    // Run process pointer, NULL if failed to start
    PROCESS_INFORMATION* Run(const char* command);

    // Get Exit code for process, use index returned by RunCommand.
    DWORD GetExitCode(PROCESS_INFORMATION*, bool wait=true);

    void Close(PROCESS_INFORMATION* pProcess);
    void Abort(PROCESS_INFORMATION* pProcess);

    bool AllDone() const;
    void Clear();

    typedef std::list<PROCESS_INFORMATION*> ProcessList;
    ProcessList m_processList;
};


const size_t KB = 1024;
const size_t MB = KB * KB;
const char* SizeToString(LONGLONG size);

