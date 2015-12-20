//=================================================================================================
// Memory Mapped files access (windows)
//
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
//=================================================================================================

#pragma once

#include <windows.h>
#include "Handle.h"

class MemMapFile
{
	enum
	{
		MinViewLength = 512 * 1024 // 512KB seems reasonable
	};

	::SYSTEM_INFO       m_sysInfo;

	Handle              m_hFile;
	Handle              m_hFileMapping;

	unsigned __int64    m_fileSize;
	unsigned __int64    m_viewOffset;
	SIZE_T              m_viewLength;
	SIZE_T              m_minViewLength;
	char*               m_view;

public:
	MemMapFile()
		: m_hFile(), m_hFileMapping(NULL), m_view(NULL)
	{ }

	MemMapFile(const char* fileName, SIZE_T minViewLength = MinViewLength);
	~MemMapFile(void);

	bool Open(const char* fileName, SIZE_T minViewLength = MinViewLength);
	void Close();

	void* MapView(unsigned __int64 viewOffset, SIZE_T& viewLength);
};
