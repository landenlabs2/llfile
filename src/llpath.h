//-----------------------------------------------------------------------------
// llpath - Support routines shared by LL files files.
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

#include "llsupport.h"
#include <Windows.h>

class LLPath
{
public:
    static lstring sDirSlash;
    static char sDirChr()
    {  return *sDirSlash; }

    static std::string Join(const std::string& basePath, const char* filename)
    {
        lstring joinWith = ((basePath.length() != 0 && basePath.back() == sDirChr())
           || (filename != NULL && filename[0] == sDirChr())) ? "" : sDirSlash.c_str();
        std::string fullPath(basePath + joinWith + filename);
        return fullPath;
    }

    // Return true if path points to a file.
    static bool IsFile(const char* path)
    {
        DWORD attr = GetFileAttributes(path);
        return (attr != INVALID_FILE_ATTRIBUTES &&  (attr & FILE_ATTRIBUTE_DIRECTORY) == 0);
    }

    // Return true if path points to a directory.
    static bool IsDirectory(const char* path)
    {
        DWORD attr = GetFileAttributes(path);
        return (attr != INVALID_FILE_ATTRIBUTES &&  (attr & FILE_ATTRIBUTE_DIRECTORY) != 0);
    }

    // Return file extension or NULL
    static const char* GetExtension(const char* path)
    {
        const char* pExtn = strrchr(path, '.');
        return (pExtn == NULL) ? pExtn : pExtn+1;
    }

    // Return file name+extension or NULL
    static const char* GetNameAndExt(const char* path)
    {
        const char* pDir = strrchr(path, sDirChr());
        return (pDir == NULL) ? pDir : pDir+1;
    }

    // Return true if path points a file or directory
    static bool PathExists(const char* path)
    {
        DWORD attr = GetFileAttributes(path);
        return (attr != INVALID_FILE_ATTRIBUTES);
    }

    // Return true if attributes are not ReadOnly, not System and not Hidden.
    static bool IsWriteable(DWORD attr)
    {
        const DWORD readOnly = (FILE_ATTRIBUTE_READONLY|FILE_ATTRIBUTE_DEVICE|FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_SYSTEM);
        return (attr == INVALID_FILE_ATTRIBUTES || (attr & readOnly) == 0);
    }

    // Return position of last '/' or '\'
    static uint GetPathLength(const char* path)
    {
        uint pos = 0;
        for (uint idx = 0; path[idx] != '\0'; idx++)
        {
            if (path[idx] == sDirChr() || path[idx] == '/')
                pos = idx;
        }

        return pos;
    }

    // Remove trailing slash
    static std::string RemoveLastSlash(std::string& path)
    {
        uint len = path.length();
        if (len > 0 && path[len-1] == sDirChr())
            path[len-1] = '\0';
        return path;
    }
};


