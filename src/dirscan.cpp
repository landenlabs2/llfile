//-----------------------------------------------------------------------------
// dirscan - Scan directories.
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

#include "dirscan.h"
#include "llmsg.h"
// #include "llsupport.h"
// #include "llpath.h"

#pragma warning(disable : 4706) // assignment within conditional expression

// Initialize static members
//
static char sDirChr = '\\';     // Same as LLPath::sDirChr();
static char sDirSlash[] = "\\";   // Same as LLPath::sDirSlash
bool (*DirectoryScan::ChrCmp)(char c1, char c2) = DirectoryScan::NCaseChrCmp;

//-----------------------------------------------------------------------------
// Remove duplicate characters.
static void RemoveDup(char* inStr, char dupChr)
{
    char* outStr = inStr;
    while (*inStr++ != 0)
    {
        if (*inStr != dupChr || *outStr != dupChr)
            *++outStr =  *inStr;
    }
    *++outStr =  *inStr;
}

//-----------------------------------------------------------------------------
// Simple wildcard matcher, used by directory file scanner.
// Patterns supported:
//          ?        ; any single character
//          *        ; zero or more characters

static bool WildCompare(const char* wildStr, int wildOff, const char* rawStr, int rawOff)
{
    const char EOS = '\0';
    while (wildStr[wildOff])
    {
        if (rawStr[rawOff] == EOS)
            return (wildStr[wildOff] == '*');

        if (wildStr[wildOff] == '*')
        {
            if (wildStr[wildOff + 1] == EOS)
                return true;

            do
            {
                // Find match with char after '*'
                while (rawStr[rawOff] &&
                    !DirectoryScan::ChrCmp(rawStr[rawOff], wildStr[wildOff+1]))
                    rawOff++;
                if (rawStr[rawOff] &&
                    WildCompare(wildStr, wildOff + 1, rawStr, rawOff))
                        return true;
                if (rawStr[rawOff])
                    ++rawOff;
            } while (rawStr[rawOff]);

            if (rawStr[rawOff] == EOS)
                return (wildStr[wildOff+1] == '*');
        }
        else if (wildStr[wildOff] == '?')
        {
            if (rawStr[rawOff] == EOS)
                return false;
            rawOff++;
        }
        else
        {
            if ( !DirectoryScan::ChrCmp(rawStr[rawOff], wildStr[wildOff]))
                return false;
            if (wildStr[wildOff] == EOS)
                return true;
            ++rawOff;
        }

        ++wildOff;
    }

    return (wildStr[wildOff] == rawStr[rawOff]);
}

//-----------------------------------------------------------------------------
// Simple wildcard matcher, used by directory file scanner.
// Patterns supported:
//          ?        ; any single character
//          *        ; zero or more characters

bool WildCompare(
        const char* wildStr,
        const char* rawStr,
        PatInfo& patInfo,
        std::vector<PatInfo>& patInfoLst)
{
    const char EOS = '\0';
    // patInfo.rawLen = 0;

    while (wildStr[patInfo.wildOff])
    {
        if (rawStr[patInfo.rawOff] == EOS)
        {
            if (wildStr[patInfo.wildOff] == '*')
            {
                patInfo.rawLen = 0;
                patInfoLst.push_back(patInfo);
                return true;
            }
            return false;
        }

        if (wildStr[patInfo.wildOff] == '*')
        {
            if (wildStr[patInfo.wildOff + 1] == EOS)
            {
                patInfo.rawLen = strlen(rawStr + patInfo.rawOff);
                patInfoLst.push_back(patInfo);
                return true;
            }

            unsigned lstSz = patInfoLst.size();
            patInfo.rawLen = 0;
            patInfoLst.push_back(patInfo);

            do
            {
                // Find match with char after '*'
                while (rawStr[patInfo.rawOff] &&
                    !DirectoryScan::ChrCmp(rawStr[patInfo.rawOff], wildStr[patInfo.wildOff+1]))
                {
                    patInfo.rawOff++;
                    patInfo.rawLen++;
                }

                PatInfo patInfo2(patInfo);
                patInfo2.wildOff++;
                if (rawStr[patInfo.rawOff] &&
                    WildCompare(wildStr, rawStr, patInfo2, patInfoLst))
                {
                    patInfoLst[lstSz].rawLen = patInfo.rawLen;
                    return true;
                }
                patInfoLst.resize(lstSz+1);

                if (rawStr[patInfo.rawOff])
                    ++patInfo.rawOff;
            } while (rawStr[patInfo.rawOff]);

            patInfoLst.resize(lstSz);

            if (rawStr[patInfo.rawOff] == EOS)
            {
                if (wildStr[patInfo.wildOff+1] == '*')
                {
                    patInfo.rawLen = 0;
                    patInfoLst.push_back(patInfo);
                    return true;
                }
                return false;
            }
        }
        else if (wildStr[patInfo.wildOff] == '?')
        {
            if (rawStr[patInfo.rawOff] == EOS)
                return false;

            patInfo.rawLen = 1;
            patInfoLst.push_back(patInfo);
            patInfo.rawOff++;
            // patInfo.rawLen++;
        }
        else
        {
            if ( !DirectoryScan::ChrCmp(rawStr[patInfo.rawOff], wildStr[patInfo.wildOff]))
                return false;
            if (wildStr[patInfo.wildOff] == EOS)
                return true;

            patInfo.rawOff++;
            // patInfo.rawLen++;
        }

        patInfo.wildOff++;
        // patInfo.rawLen++;
    }

    return (wildStr[patInfo.wildOff] == rawStr[patInfo.rawOff]);
}

//-----------------------------------------------------------------------------
bool PatternMatch(const std::string& pattern, const char* str)
{
    return WildCompare(pattern.c_str(), 0, str, 0);
}

//-----------------------------------------------------------------------------
DirectoryScan::DirectoryScan() :
    m_fileFilter(),
    m_recurse(false),
    m_skipJunction(false),
    m_addAllDepths(false),
    m_abort(false),
    m_disableWow64Redirection(true),
    m_oldWow64Redirection(0),
    m_add_cb(0),
    m_cb_data(0),
    m_filesFirst(false),
    m_entryType(eFilesDir)
{
    m_dir[0] = '\0';
}

//-----------------------------------------------------------------------------
DirectoryScan::~DirectoryScan()
{
    if (m_disableWow64Redirection)
        Wow64RevertWow64FsRedirection(m_oldWow64Redirection);
}

//-----------------------------------------------------------------------------
size_t DirectoryScan::GetFilesInDirectory(int depth)
{
    if (m_filesFirst)
    {
        bool recurse = m_recurse;

        m_entryType = eFile;
        m_recurse   = false;
        size_t fileSz = GetFilesInDirectory2(depth);
        size_t dirSz = 0;

        if (recurse)
        {
            m_entryType = eDir;
            m_recurse   = recurse;
            dirSz = GetFilesInDirectory2(depth);
        }
        return fileSz + dirSz;
    }

    m_entryType = eFilesDir;
    return GetFilesInDirectory2(depth);
}

//-----------------------------------------------------------------------------
//  When a 32-bit application reads from one of these folders on a 64-bit OS:
//
//      %windir%\system32\catroot
//      %windir%\system32\catroot2
//      %windir%\system32\drivers\etc
//      %windir%\system32\logfiles
//      %windir%\system32\spool
//
//  Windows actually lists the content of:
//
//      %windir%\SysWOW64\catroot
//      %windir%\SysWOW64\catroot2
//      %windir%\SysWOW64\drivers\etc
//      %windir%\SysWOW64\logfiles
//      %windir%\SysWOW64\spool
//
size_t DirectoryScan::GetFilesInDirectory2(int depth)
{
    WIN32_FIND_DATA FileData;    // Data structure describes the file found
    HANDLE  hSearch;             // Search handle returned by FindFirstFile
    bool    is_more = true;
    size_t  fileCnt = 0;
    size_t  dirLen = strlen(m_dir);

    strcat_s(m_dir, ARRAYSIZE(m_dir), "\\*");
    RemoveDup(m_dir+1, '\\');      // Okay for \\ to appear in front, remove others.

    // Start searching for folder (directories), starting with srcdir directory.

    hSearch = FindFirstFile(m_dir, &FileData);
    if (hSearch == INVALID_HANDLE_VALUE)
    {
        // No files found
        if (depth == 0)
        {
            // PresentError(GetLastError(), "Failed to open directory, ", m_dir);
        }
        return 0;
    }

    m_dir[dirLen] = '\0';
    while (is_more && !m_abort)
    {
        if ((FileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
        {
            if (m_skipJunction && (FileData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0)
            {
                is_more = (FindNextFile(hSearch, &FileData) != 0) ? true : false;
                continue;
            }

            if (m_entryType != eFile &&
               (FileData.cFileName[0] != '.' ||  isalnum(FileData.cFileName[1])))
            {
                if (m_recurse || depth < (int)m_dirFilters.size())
                {
                    int newDepth = depth + 1;
                    if ((int)m_dirFilters.size() < newDepth
                        || PatternMatch(m_dirFilters[depth], FileData.cFileName))
                    {
                        if ((m_entryType == eDir || m_entryType == eFilesDir) &&
                            m_add_cb && depth >= (int)m_dirFilters.size())
                            m_add_cb(m_cb_data, m_dir, &FileData, depth);

                        m_dir[dirLen] = sDirChr;
                        strcpy_s(m_dir + dirLen +1, ARRAYSIZE(m_dir) - dirLen - 1, FileData.cFileName);
                        fileCnt += GetFilesInDirectory(depth+1);
                        m_dir[dirLen] = '\0';
                    }
                }
                else if (m_fileFilter.empty() || PatternMatch(m_fileFilter, FileData.cFileName))
                {
                    if (m_add_cb)
                        m_add_cb(m_cb_data, m_dir, &FileData, depth);
                }
            }
        }
        else
        {
            if (m_entryType != eDir)
            {
                ++fileCnt;
                if (m_fileFilter.empty() || PatternMatch(m_fileFilter, FileData.cFileName))
                {
                    if (m_add_cb && depth >= (int)m_dirFilters.size())
                        m_add_cb(m_cb_data, m_dir, &FileData, depth);
                }
            }
        }

        is_more = (FindNextFile(hSearch, &FileData) != 0) ? true : false;
    }

    // Close directory before calling callback incase client wants to delete dir.
    FindClose(hSearch);

    DWORD err = GetLastError();
    if (err != ERROR_NO_MORE_FILES && !m_abort)
    {
        LLMsg::PresentError(err, "DirScan ", "\n");
    }

    if (m_entryType != eFile)
    {
        // Set attribute for end-of-directory
        FileData.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
        FileData.cFileName[0] = '\0';
        int dirDepth = -1 - depth;

        // dirDepth is positive while walking down into directories and negative when
        // walking back out.
        //      del command will delete directories on the way out.
        if (m_recurse || dirDepth >= 0 || m_addAllDepths)
        if (m_add_cb && (m_dirFilters.size() == 0 || depth >= (int)m_dirFilters.size() || m_addAllDepths))
            m_add_cb(m_cb_data, m_dir, &FileData, dirDepth);
    }

    return fileCnt;
}

// ---------------------------------------------------------------------------
void DirectoryScan::Init(
    const char* pPathPat,
    const char* pCwd,
    bool appendSlash)
{
    // Get current directory as default starting point.
    char currentDir[MAX_PATH];
    if (pCwd == NULL)
    {
#if 1
        GetCurrentDirectory(ARRAYSIZE(currentDir), currentDir);
        pCwd = currentDir;
#else
        pCwd = ".\\";
#endif
    }

    // Clear any previous state.
    m_subDirCnt = 0;
    m_dirFilters.clear();

    // Set default Directory and File filter.
    const char* defDir = pCwd;
    m_fileFilter  = "*";
    m_abort       = false;

    // Make local copy of input pattern
    m_pathPat = pPathPat;

    if (appendSlash && m_pathPat.back() != sDirChr)
    {
        // If input is a directory, append slash
        //    \dir1\dir2  =>  \dir1\dir2\
        //    c:          =>  c:\
        // If input does not exist and does not contain wildcards, append slash
        //    \dir1\foo   =>  \dir1\foo\
        //    \dir1\*     =>  \dir1\*
        //    \d*\dir2    =>  \d*\dir2  (users should avoid this and pass /d*/dir2/*)
        DWORD attr = GetFileAttributes(pPathPat);
        DWORD err = GetLastError();
        if ((attr == INVALID_FILE_ATTRIBUTES && pPathPat[strcspn(pPathPat, "*?")] == '\0')
            ||
            (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY) != 0))
        {
            m_pathPat += sDirSlash;
        }
    }

    //  Modified above
    //  c:          => c:\
    //  \\host\c$   =>  \\host\c$\

    // Pull input pattern apart at directory levels.
    //  Input           m_dirFilters,       m_fileFilter,   m_dir       m_subDircnt
    //  --------------  -----------------   --------------- ----------  -------
    //                  none                *               .\          0
    //   *              none                *               .\          0
    //  d2\             none                *               d2\         1
    //  \d2\            none                *               \d2         2
    //  \d1\d2\d3*\*    d3*                 *               \d1\d2
    //  \d1\d2*\d3*\A*  d2*,d3*             A*              \d1
    //  c:\             none                *               c:
    // \\host\c$        none                *               \\host\c$   4
    //

    pPathPat = m_pathPat.c_str();
    char* pBegDir = (char*)pPathPat;
    char* pEndDir;
    char* posWild = strcspn(pBegDir, "*?") + pBegDir;

    while (pEndDir = strchr(pBegDir, sDirChr))
    {
        m_subDirCnt++;

        if (pEndDir > posWild)
        {
            // Have wildcards in dirPath
            if (pBegDir != pPathPat)
                pBegDir[-1] = '\0';     // terminate each dir level.
            m_dirFilters.push_back(pBegDir);
        }
        else
            defDir = pPathPat;
        pBegDir = pEndDir + 1;
    }

    if (pBegDir != pPathPat)
        pBegDir[-1] = '\0';
    if (*pBegDir)
        m_fileFilter = pBegDir;

    strcpy_s(m_dir, ARRAYSIZE(m_dir), defDir);

    if (m_disableWow64Redirection)
        Wow64DisableWow64FsRedirection(&m_oldWow64Redirection);
}

//=================================================================================================
// Utility function to determine if two paths point to same file.

bool DirectoryScan::IsSameFile(const char* path1, const char* path2)
{
    char fullPath1[MAX_PATH];
    DWORD len1 = GetFullPathName(path1, ARRAYSIZE(fullPath1), fullPath1, NULL);
    char fullPath2[MAX_PATH];
    DWORD len2 = GetFullPathName(path2, ARRAYSIZE(fullPath2), fullPath2, NULL);

    if (len1 == 0 || len2 == 0)
        return false;   // Bad paths.

    if  (strcmp(fullPath1, fullPath2) == 0)
        return true;

    LONGLONG id1 = GetFileId(path1);
    LONGLONG id2 = GetFileId(path2);
    return (id1 == id2);
};

//=================================================================================================
std::string DirectoryScan::GetFullPath(const char* path)
{
    char fullPath[MAX_PATH];
    DWORD len = GetFullPathName(path, ARRAYSIZE(fullPath), fullPath, NULL);
    if (len != 0)
        return std::string(fullPath);
    else
        return std::string(path);
}

//=================================================================================================
// Utility function to determine if two paths point to same file.

LONGLONG DirectoryScan::GetFileId(const char* srcFile)
{
    LARGE_INTEGER fileId;
    fileId.QuadPart = -1;

    BY_HANDLE_FILE_INFORMATION ByHandleFileInformation;
    HANDLE fileHnd =
        CreateFile(srcFile, FILE_READ_ATTRIBUTES, 7, 0, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, 0);
    if (fileHnd != INVALID_HANDLE_VALUE)
    {
        if (GetFileInformationByHandle(fileHnd, &ByHandleFileInformation))
        {
            fileId.LowPart  = ByHandleFileInformation.nFileIndexLow;
            fileId.HighPart = ByHandleFileInformation.nFileIndexHigh;
        }
        CloseHandle(fileHnd);
    }

    return fileId.QuadPart;
}

