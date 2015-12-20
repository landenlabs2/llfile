//-----------------------------------------------------------------------------
// lldirsort - Display directory information from provided by DirectoryScan object
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

#include <iostream>
#include <iomanip>
#include "dirscan.h"

#include "llstring.h"

// Forward ref
struct LLDirEntry;
class LLDirSort;

typedef void (*FillEntryData_t)(LLDirEntry& dirEntry, const WIN32_FIND_DATA& findData, const LLDirSort& dirSort);
typedef void (*FillFindData_t)(WIN32_FIND_DATA& findData, const LLDirEntry& dirEntry, const LLDirSort& dirSort);

typedef LLPool<LLDirEntry>  LLDirEntryPool;

// Forward declaration
class LLDirSort;

// ---------------------------------------------------------------------------
struct LLDirEntry
{
    LLDirEntry(const char* fileName, const char* dir, int baseDirLen,
        const WIN32_FIND_DATA&, FillEntryData_t fillEntryData,
        LLDirSort& dirSort);

    ~LLDirEntry()
    {}

    LLDirEntry*     pPrev;
    LLDirEntry*     pNext;
    LLString        filenameLStr;
    const char*     szDir;
    int             baseDirLen;
    DWORD           dwFileAttributes;
    union
    {
        LARGE_INTEGER   li;
        FILETIME        ft;
    } lvalue[4];    // ctime, atime, mtime, size;

    void *operator new(size_t /* size */, LLDirEntryPool& pool, size_t ourSize)
    {
        return (void*)pool.Add(0, ourSize);
    }

    void operator delete(void*)
    {
    }

private:
    LLDirEntry& operator=(const LLDirEntry&);
    LLDirEntry(const LLDirEntry&);
};


typedef int (*DirCb)(
        void* cbData,
        const char* pDir,
        const WIN32_FIND_DATA* pFileData,
        int depth);


// ---------------------------------------------------------------------------
class LLDirSort
{
public:
    LLDirSort() : m_ourSize(0), m_count(0), m_pool(),
        m_pFirst(NULL), m_pLast(NULL),
        m_onlyAttr(DWORD(-1)), m_values(0)
    {}

    ~LLDirSort() { Clear(); }

    void Clear();

    static int SortCb(
        void* cbData,
        const char* pDir,
        const WIN32_FIND_DATA* pFileData,
        int depth);

    /// Set sort options,
    ///    needAllData  true to keep dirEntry size, c/a/mtime
    ///    sortIncrease true to sort increasing order
    void SetSort(DirectoryScan& dirScan, const char* sortOptStr,
        bool needAllData,
        bool sortIncreasing);

    void SetSortData(bool needAllData);

    /// Set which Directory attributes we care about (reduce memory usage)
    void SetSortAttr(DWORD showOnlyAttr);

    void SetColor(WORD& colorCfg, const char* colorOptStr);
    void ShowSorted(DirCb dirCb, void* cbData);

public:
    size_t             m_ourSize;
    size_t             m_count;
    int                m_baseDirLen;
    LLDirEntry*        m_pFirst;
    LLDirEntry*        m_pLast;
    LLDirEntryPool     m_pool;
    DWORD              m_onlyAttr;
    FillEntryData_t    m_fillEntryData;
    FillFindData_t     m_fillFindData;
    int                m_values;
};

