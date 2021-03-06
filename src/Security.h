//-----------------------------------------------------------------------------
// Security - Security routines
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

namespace LLSec
{

// http://msdn.microsoft.com/en-us/library/windows/desktop/aa446619(v=vs.85).aspx

bool SetPrivilege(
    HANDLE hToken,              // access token handle
    const char* lpszPrivilege,  // name of privilege to enable/disable
    bool bEnablePrivilege       // to enable or disable privilege
    );

// http://msdn.microsoft.com/en-us/library/windows/desktop/aa379620(v=vs.85).aspx
bool TakeOwnership(const char* lpszOwnFile);

// Remove a file,
//    if DelToRecyleBin true, then delete file by transfering it to recyle bin.
//  if force true, then set file readable and take ownership.
int RemoveFile(const char* filePath, bool DelToRecycleBin, bool force);

bool IsElevated( );
};