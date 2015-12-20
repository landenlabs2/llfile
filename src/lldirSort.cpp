//-----------------------------------------------------------------------------
// lldirsort - Display directory information from provided by DirectoryScan object
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

#include <iostream>
#include <iomanip>
#include "lldirSort.h"
#include "llsupport.h"


LLPool<const char> LLString::m_pool;

int (*CompareData)(const LLDirEntry* pDirLeft, const LLDirEntry* pDirRight) = 0;

// ---------------------------------------------------------------------------
void FillEntryName(LLDirEntry& dirEntry, const WIN32_FIND_DATA& findData, const LLDirSort& dirSort)
{
    dirEntry.lvalue[0].ft = findData.ftCreationTime;
    dirEntry.lvalue[1].ft = findData.ftLastAccessTime;
    dirEntry.lvalue[2].ft = findData.ftLastWriteTime;
    dirEntry.lvalue[3].li.HighPart = findData.nFileSizeHigh;
    dirEntry.lvalue[3].li.LowPart = findData.nFileSizeLow;
}

// ---------------------------------------------------------------------------
void FillFindDataName(WIN32_FIND_DATA& findData, const LLDirEntry& dirEntry, const LLDirSort& dirSort)
{
    findData.ftCreationTime   = dirEntry.lvalue[0].ft;
    findData.ftLastAccessTime = dirEntry.lvalue[1].ft;
    findData.ftLastWriteTime  = dirEntry.lvalue[2].ft;
    findData.nFileSizeHigh    = dirEntry.lvalue[3].li.HighPart;
    findData.nFileSizeLow     = dirEntry.lvalue[3].li.LowPart;
}

// ---------------------------------------------------------------------------
int CompareDataExtInc(const LLDirEntry* pDirLeft, const LLDirEntry* pDirRight)
{
    const char* pLeftExt = strrchr(pDirLeft->filenameLStr, '.');
    const char* pRightExt = strrchr(pDirRight->filenameLStr, '.');
    if (pLeftExt == 0 || pRightExt == 0)
        return (int)((size_t)pLeftExt & 0x8fff) - (int)((size_t)pRightExt & 0x8fff);
    int diffExt = _stricmp(pLeftExt, pRightExt);
    return diffExt ? diffExt : _stricmp(pDirLeft->filenameLStr, pDirRight->filenameLStr);
}

// ---------------------------------------------------------------------------
int CompareDataExtDec(const LLDirEntry* pDirLeft, const LLDirEntry* pDirRight)
{
    return -CompareDataExtInc(pDirLeft, pDirRight);
}

// ---------------------------------------------------------------------------
int CompareDataNameInc(const LLDirEntry* pDirLeft, const LLDirEntry* pDirRight)
{
    return _stricmp(pDirLeft->filenameLStr, pDirRight->filenameLStr);
}

// ---------------------------------------------------------------------------
int CompareDataNameDec(const LLDirEntry* pDirLeft, const LLDirEntry* pDirRight)
{
    return -CompareDataNameInc(pDirLeft, pDirRight);
}

// ---------------------------------------------------------------------------
int CompareDataPathInc(const LLDirEntry* pDirLeft, const LLDirEntry* pDirRight)
{
    // int dirCmp = _stricmp(pDirLeft->szDir, pDirRight->szDir);
    // return (dirCmp == 0) ? _stricmp(pDirLeft->filenameLStr, pDirRight->filenameLStr) : dirCmp;
    return _stricmp(pDirLeft->filenameLStr, pDirRight->filenameLStr);
}

// ---------------------------------------------------------------------------
int CompareDataPathDec(const LLDirEntry* pDirLeft, const LLDirEntry* pDirRight)
{
    return -CompareDataPathInc(pDirLeft, pDirRight);
}

// ---------------------------------------------------------------------------
int CompareDataTypeInc(const LLDirEntry* pDirLeft, const LLDirEntry* pDirRight)
{
    int attCmp = pDirLeft->dwFileAttributes - pDirRight->dwFileAttributes;
    return (attCmp == 0) ? CompareDataNameInc(pDirLeft, pDirRight) : attCmp;
}

// ---------------------------------------------------------------------------
int CompareDataTypeDec(const LLDirEntry* pDirLeft, const LLDirEntry* pDirRight)
{
    return -CompareDataTypeInc(pDirLeft, pDirRight);
}


// ---------------------------------------------------------------------------
void FillEntryAtime(LLDirEntry& dirEntry, const WIN32_FIND_DATA& findData, const LLDirSort& dirSort)
{
    // Sort on value[0], set to accessTime
    dirEntry.lvalue[0].ft = findData.ftLastAccessTime;

    // Also copy non-sort fields
    assert(dirSort.m_values == 0 || dirSort.m_values == 4);
    if (dirSort.m_values == 4)
    {
        dirEntry.lvalue[1].ft = findData.ftCreationTime;
        dirEntry.lvalue[2].ft = findData.ftLastWriteTime;
        dirEntry.lvalue[3].li.HighPart = findData.nFileSizeHigh;
        dirEntry.lvalue[3].li.LowPart  = findData.nFileSizeLow;
    }
}

// ---------------------------------------------------------------------------
void FillFindDataAtime(WIN32_FIND_DATA& findData, const LLDirEntry& dirEntry, const LLDirSort& dirSort)
{
    findData.ftLastAccessTime = dirEntry.lvalue[0].ft;

    assert(dirSort.m_values == 0 || dirSort.m_values == 4);
    if (dirSort.m_values == 4)
    {
        findData.ftCreationTime   = dirEntry.lvalue[1].ft;
        findData.ftLastWriteTime  = dirEntry.lvalue[2].ft;
        findData.nFileSizeHigh    = dirEntry.lvalue[3].li.HighPart;
        findData.nFileSizeLow     = dirEntry.lvalue[3].li.LowPart;
    }
}

// ---------------------------------------------------------------------------
void FillEntryCtime(LLDirEntry& dirEntry, const WIN32_FIND_DATA& findData, const LLDirSort& dirSort)
{
    // Sort on value[0], set to creationTime
    dirEntry.lvalue[0].ft = findData.ftCreationTime;

    // Also copy non-sort fields
    assert(dirSort.m_values == 0 || dirSort.m_values == 4);
    if (dirSort.m_values == 4)
    {
        dirEntry.lvalue[1].ft = findData.ftLastWriteTime;
        dirEntry.lvalue[2].ft = findData.ftLastAccessTime;
        dirEntry.lvalue[3].li.HighPart = findData.nFileSizeHigh;
        dirEntry.lvalue[3].li.LowPart  = findData.nFileSizeLow;
    }
}

// ---------------------------------------------------------------------------
void FillFindDataCtime(WIN32_FIND_DATA& findData, const LLDirEntry& dirEntry, const LLDirSort& dirSort)
{
    findData.ftCreationTime = dirEntry.lvalue[0].ft;

    assert(dirSort.m_values == 0 || dirSort.m_values == 4);
    if (dirSort.m_values == 4)
    {
        findData.ftLastWriteTime  = dirEntry.lvalue[1].ft;
        findData.ftLastAccessTime = dirEntry.lvalue[2].ft;
        findData.nFileSizeHigh    = dirEntry.lvalue[3].li.HighPart;
        findData.nFileSizeLow     = dirEntry.lvalue[3].li.LowPart;
    }
}

// ---------------------------------------------------------------------------
void FillEntryMtime(LLDirEntry& dirEntry, const WIN32_FIND_DATA& findData, const LLDirSort& dirSort)
{
    // Sort on value[0], set to writeTime
    dirEntry.lvalue[0].ft = findData.ftLastWriteTime;

    // Also copy non-sort fields
    assert(dirSort.m_values == 0 || dirSort.m_values == 4);
    if (dirSort.m_values == 4)
    {
        dirEntry.lvalue[1].ft = findData.ftCreationTime;
        dirEntry.lvalue[2].ft = findData.ftLastAccessTime;
        dirEntry.lvalue[3].li.HighPart = findData.nFileSizeHigh;
        dirEntry.lvalue[3].li.LowPart = findData.nFileSizeLow;
    }
}

// ---------------------------------------------------------------------------
void FillFindDataMtime(WIN32_FIND_DATA& findData, const LLDirEntry& dirEntry, const LLDirSort& dirSort)
{
    findData.ftLastWriteTime = dirEntry.lvalue[0].ft;

    assert(dirSort.m_values == 0 || dirSort.m_values == 4);
    if (dirSort.m_values == 4)
    {
        findData.ftCreationTime   = dirEntry.lvalue[1].ft;
        findData.ftLastAccessTime = dirEntry.lvalue[2].ft;
        findData.nFileSizeHigh    = dirEntry.lvalue[3].li.HighPart;
        findData.nFileSizeLow     = dirEntry.lvalue[3].li.LowPart;
    }
}

// ---------------------------------------------------------------------------
int CompareDataFtimeInc(const LLDirEntry* pDirLeft, const LLDirEntry* pDirRight)
{
    int tmDiff =  CompareFileTime(&pDirLeft->lvalue[0].ft, &pDirRight->lvalue[0].ft);
    return (tmDiff == 0) ? CompareDataNameInc(pDirLeft, pDirRight) : tmDiff;
}

// ---------------------------------------------------------------------------
int CompareDataFtimeDec(const LLDirEntry* pDirLeft, const LLDirEntry* pDirRight)
{
    return -CompareDataFtimeInc(pDirLeft, pDirRight);
}

// ---------------------------------------------------------------------------
void FillEntrySize(LLDirEntry& dirEntry, const WIN32_FIND_DATA& findData, const LLDirSort& dirSort)
{
    // Sort on value[0], set to file size.
    dirEntry.lvalue[0].li.HighPart = findData.nFileSizeHigh;
    dirEntry.lvalue[0].li.LowPart = findData.nFileSizeLow;

    // Also copy non-sort fields
    assert(dirSort.m_values == 4);
    if (dirSort.m_values == 4)
    {
        dirEntry.lvalue[1].ft = findData.ftCreationTime;
        dirEntry.lvalue[2].ft = findData.ftLastAccessTime;
        dirEntry.lvalue[3].ft = findData.ftLastWriteTime;
    }
}

// ---------------------------------------------------------------------------
void FillFindDataSize(WIN32_FIND_DATA& findData, const LLDirEntry& dirEntry, const LLDirSort& dirSort)
{
    findData.nFileSizeHigh = dirEntry.lvalue[0].li.HighPart;
    findData.nFileSizeLow = dirEntry.lvalue[0].li.LowPart;

    assert(dirSort.m_values == 4);
    if (dirSort.m_values == 4)
    {
        findData.ftCreationTime   = dirEntry.lvalue[1].ft;
        findData.ftLastAccessTime = dirEntry.lvalue[2].ft;
        findData.ftLastWriteTime  = dirEntry.lvalue[3].ft;
    }
}

// ---------------------------------------------------------------------------
int CompareDataSizeInc(const LLDirEntry* pDirLeft, const LLDirEntry* pDirRight)
{
    if (pDirLeft->lvalue[0].li.QuadPart >  pDirRight->lvalue[0].li.QuadPart)
        return 1;
    if (pDirLeft->lvalue[0].li.QuadPart ==  pDirRight->lvalue[0].li.QuadPart)
        return CompareDataNameInc(pDirLeft, pDirRight);
    return -1;
}

// ---------------------------------------------------------------------------
int CompareDataSizeDec(const LLDirEntry* pDirLeft, const LLDirEntry* pDirRight)
{
    return -CompareDataSizeInc(pDirLeft, pDirRight);
}

// ---------------------------------------------------------------------------
LLDirEntry::LLDirEntry(
        const char* fileName,
        const char* dir,
        int baseDirLength,
        const WIN32_FIND_DATA& findData,
        FillEntryData_t fillEntryData,
        LLDirSort& dirSort) :
    pPrev(dirSort.m_pLast),
    pNext(0),
    filenameLStr(fileName),
    szDir(dir),
    baseDirLen(baseDirLength),
    dwFileAttributes(findData.dwFileAttributes)
{
    if ((findData.dwFileAttributes & dirSort.m_onlyAttr) == 0)
        return;

        _CrtCheckMemory( );
    fillEntryData(*this, findData, dirSort);
        _CrtCheckMemory( );

    if (dirSort.m_pFirst == NULL)
        dirSort.m_pFirst = dirSort.m_pLast = this;
    else
    {
        // Insertion sort
        //   1. If less then first, replace first
        //   2. If greater then last, replace last
        //   3. Scan list using cloest starting point (first or last)
        int dFirst = CompareData(this, dirSort.m_pFirst);
        if (dFirst <= 0)
        {
            // Replace first
            pNext = dirSort.m_pFirst;
            dirSort.m_pFirst->pPrev = this;
            dirSort.m_pFirst = this;
            return;
        }

        int dLast  = CompareData(dirSort.m_pLast, this);
        if (dLast <= 0)
        {
            // Replace Last
            pPrev = dirSort.m_pLast;
            dirSort.m_pLast->pNext = this;
            dirSort.m_pLast = this;
            return;
        }

        if (dFirst < dLast)
        {
            LLDirEntry* pDirEntry = dirSort.m_pFirst;
            while (pDirEntry->pNext)
            {
                pDirEntry = pDirEntry->pNext;
                if (CompareData(pDirEntry, this) >= 0)
                {
                    pNext = pDirEntry;
                    pPrev = pDirEntry->pPrev;
                    pDirEntry->pPrev = this;
                    if (pPrev)
                        pPrev->pNext = this;
                    return;
                }
            }
        }
        else
        {
            LLDirEntry* pDirEntry = dirSort.m_pLast;
            while (pDirEntry->pPrev)
            {
                pDirEntry = pDirEntry->pPrev;
                if (CompareData(pDirEntry, this) <= 0)
                {
                    pPrev = pDirEntry;
                    pNext = pDirEntry->pNext;
                    pDirEntry->pNext = this;
                    if (pNext)
                        pNext->pPrev = this;
                    return;
                }
            }
        }
    }
}

// ---------------------------------------------------------------------------
int LLDirSort::SortCb(
        void* cbData,
        const char* pDir,
        const WIN32_FIND_DATA* pFileData,
        int depth)
{
    static LLString commonDir(pDir);
    LLDirSort* pDirSort = (LLDirSort*)cbData;

    if (depth < 0)
        return 0;     // ignore end-of-directory

    if (strcmp(pDir, commonDir) != 0)
    {
        commonDir = LLString(pDir);
    }

    LLDirEntry* pDirEntry = NULL;
    try
    {
		if ((pFileData->dwFileAttributes & pDirSort->m_onlyAttr) != 0)
		{
			_CrtCheckMemory( );
			// Create dir entry (adds string to pool, and dir to link list.
			pDirEntry = new (pDirSort->m_pool, pDirSort->m_ourSize)
				LLDirEntry(pFileData->cFileName, commonDir, pDirSort->m_baseDirLen, *pFileData,
				pDirSort->m_fillEntryData, *pDirSort);
			pDirSort->m_count++;
			_CrtCheckMemory( );
		}
    }
    catch (...)
    {
        delete pDirEntry;
    }

    return 1;
}

// ---------------------------------------------------------------------------
void LLDirSort::SetSort(
        DirectoryScan& dirScan,
        const char* sortOpt,
        bool needAllData,
        bool sortIncreasing)
{
    m_values = -1;

    for (; sortOpt && *sortOpt; sortOpt++)
    {
        switch (*sortOpt)
        {
        case 'a':
            // Sort access time
            m_values = needAllData ? 4 : 1;
            m_fillFindData  = FillFindDataAtime;
            m_fillEntryData = FillEntryAtime;
            CompareData     = sortIncreasing ? CompareDataFtimeInc : CompareDataFtimeDec;
            break;
        case 'c':
            // Sort creation time
            m_values = needAllData ? 4 : 1;
            m_fillFindData  = FillFindDataCtime;
            m_fillEntryData = FillEntryCtime;
            CompareData     = sortIncreasing ? CompareDataFtimeInc : CompareDataFtimeDec;
            break;
        case 'm':
            // Sort modify time
            m_values = needAllData ? 4 : 1;
            m_fillFindData  = FillFindDataMtime;
            m_fillEntryData = FillEntryMtime;
            CompareData     = sortIncreasing ? CompareDataFtimeInc : CompareDataFtimeDec;
            break;
        case 's':
            // Sort size
            m_values = needAllData ? 4 : 1;
            m_fillFindData  = FillFindDataSize;
            m_fillEntryData = FillEntrySize;
            CompareData     = sortIncreasing ? CompareDataSizeInc : CompareDataSizeDec;
            break;
        case 'n':
        default:
            // Sort fileName
            m_values = needAllData ? 4 : 0;
            m_fillFindData  = FillFindDataMtime;
            m_fillEntryData = FillEntryMtime;
            CompareData     = sortIncreasing ? CompareDataNameInc : CompareDataNameDec;
            break;
        case 'e':
            // Sort extension
            m_values = needAllData ? 4 : 0;
            m_fillFindData  = FillFindDataMtime;
            m_fillEntryData = FillEntryMtime;
            CompareData     = sortIncreasing ? CompareDataExtInc : CompareDataExtDec;
            break;
        case 'p':
            // Sort file path  (dir+name)
            m_values = needAllData ? 4 : 0;
            m_fillFindData  = FillFindDataMtime;
            m_fillEntryData = FillEntryMtime;
            CompareData     = sortIncreasing ? CompareDataPathInc : CompareDataPathDec;
            break;
        case 't':
            // Sort attributes, type (file, dir, etc)
            m_values = needAllData ? 4 : 0;
            m_fillFindData  = FillFindDataMtime;
            m_fillEntryData = FillEntryMtime;
            CompareData     = sortIncreasing ? CompareDataTypeInc : CompareDataTypeDec;
            break;
        }
    }

    assert(m_values != -1);

    if (m_values != -1)
    {
        LLDirEntry* pDirEntry = NULL;
        m_ourSize  = sizeof(LLDirEntry) - sizeof(pDirEntry->lvalue);
        m_ourSize += sizeof(pDirEntry->lvalue[0]) * m_values;
        dirScan.m_add_cb     = LLDirSort::SortCb;
        dirScan.m_cb_data    = this;
    }
}

// ---------------------------------------------------------------------------
void LLDirSort::SetSortData(bool needAllData)
{
    // TODO - should decide size based on currently active sort field.
    //      - need to save sort field id and use it in a switch statement.

    if (needAllData)
    {
        m_values = 4;
        LLDirEntry* pDirEntry = NULL;
        m_ourSize  = sizeof(LLDirEntry) - sizeof(pDirEntry->lvalue);
        m_ourSize += sizeof(pDirEntry->lvalue[0]) * m_values;
    }
}

// ---------------------------------------------------------------------------
/// Set which Directory attributes we care about.

void LLDirSort::SetSortAttr(DWORD showOnlyAttr)
{
    const DWORD FILE_DEVICE_BITS = FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_ARCHIVE |
            FILE_ATTRIBUTE_DEVICE |FILE_ATTRIBUTE_NORMAL;

    m_onlyAttr = showOnlyAttr & FILE_DEVICE_BITS;
}

// ---------------------------------------------------------------------------
void LLDirSort::Clear()
{
    LLDirEntry* pDirEntry = m_pFirst;
    LLDirEntry* pTmpEntry = NULL;

    while (pDirEntry)
    {
        pTmpEntry = pDirEntry;
        pDirEntry = pDirEntry->pNext;
        delete pTmpEntry;
    }
}

// ---------------------------------------------------------------------------
void LLDirSort::ShowSorted(DirCb dirCb, void* cbData)
{
    LLDirEntry* pDirEntry = m_pFirst;

    //
    //  Display data
    //
    WIN32_FIND_DATA findData;
    ClearMemory(&findData, sizeof(findData));

    while (pDirEntry)
    {
        // Copy sorted data in to findData structure
        strncpy_s(findData.cFileName, ARRAYSIZE(findData.cFileName), pDirEntry->filenameLStr, _TRUNCATE);
        findData.dwFileAttributes = pDirEntry->dwFileAttributes;
        m_fillFindData(findData, *pDirEntry, *this);

        // Call standard display function.
        dirCb(cbData, pDirEntry->szDir, &findData, 0);
        pDirEntry = pDirEntry->pNext;
    }
}
