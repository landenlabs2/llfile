//-----------------------------------------------------------------------------
// dirscan - Class to scan directories with per directory pattern matching.
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

#pragma once

#include <windows.h>
#include <vector>
#include <string>
#include <ctype.h>
#include "ll_stdhdr.h"

// Compare a pattern to a string.
// Patterns supported:
//          ?        ; any single character
//          *        ; zero or more characters
bool PatternMatch(const std::string& pattern, const char* str);

struct PatInfo
{
    unsigned  rawLen;
    unsigned  rawOff;
    unsigned  wildOff;
};

bool WildCompare(
        const char* wildStr,
        const char* rawStr,
        PatInfo& patInfo,
        std::vector<PatInfo>& patInfoLst);

struct DirectoryScan
{
public:
    DirectoryScan();
    ~DirectoryScan();

    // Initialize base directory, appendStar adds "\*" to end if FromFiles is a directory
    void Init(const char* pFromFiles, const char* pCwd, bool appendStar = true);
    size_t GetFilesInDirectory(int depth=0);
    size_t GetFilesInDirectory2(int depth=0);

    // If m_fileFirst true, then process only files in first pass, directories only
    // in 2nd pass, else both in one pass.
    bool        m_filesFirst;
    enum  EntryType { eFilesDir, eFile, eDir };
    EntryType   m_entryType;


    char        m_dir[MAX_PATH];
    std::string m_fileFilter;
    std::vector<const char*> m_dirFilters;
    bool        m_recurse;
    bool        m_skipJunction;
    bool        m_addAllDepths;
    bool        m_abort;
    bool        m_disableWow64Redirection;
    PVOID       m_oldWow64Redirection;

    typedef int (*Add_cb)(void *, const char* pDir, const WIN32_FIND_DATA * pFileData, int depth);
    Add_cb      m_add_cb;
    void *      m_cb_data;

    // Number of slashes in source pattern.
    int         m_subDirCnt;

    // Modified version of users search pattern.
    // Pointers to this are stored in the m_dirFilters list.
    std::string m_pathPat;

    // Text comparison functions.
    static bool YCaseChrCmp(char c1, char c2) { return c1 == c2; }
    static bool NCaseChrCmp(char c1, char c2) { return ToLower(c1) == ToLower(c2); }
    static bool (*ChrCmp)(char c1, char c2);

    // Utility function to determine if two paths point to same file.
    static bool IsSameFile(const char* path1, const char* path2);

    // Return full path
    static std::string GetFullPath(const char* path);

    // Return FileId of exiting file, else -1.
    static LONGLONG DirectoryScan::GetFileId(const char* srcFile);
};