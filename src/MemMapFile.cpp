//=================================================================================================
// Memory Mapped files access (windows)
//
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
//=================================================================================================

#include <windows.h>

#include "MemMapFile.h"

//=================================================================================================
MemMapFile::MemMapFile(const char* fileName, SIZE_T minViewLength) :
    m_hFile(), 
    m_hFileMapping(NULL), m_view(NULL)
{
	Open(fileName, minViewLength);		
}


//=================================================================================================
MemMapFile::~MemMapFile()
{
	Close();
}

//=================================================================================================
bool MemMapFile::Open(const char* fileName, SIZE_T minViewLength)
{
	Close();

	m_minViewLength = minViewLength;
	::GetSystemInfo(&m_sysInfo);

	m_hFile = ::CreateFile(
		    fileName,
		    GENERIC_READ,
		    FILE_SHARE_READ,
		    NULL,
		    OPEN_EXISTING,
		    FILE_ATTRIBUTE_NORMAL,
		    NULL);

	bool ok = false;
	m_fileSize = 0;
	LARGE_INTEGER fileSize;

    if (m_hFile.IsValid() && ::GetFileSizeEx(m_hFile, &fileSize) != 0)
	{
		// Store the file size.		
		m_fileSize = fileSize.QuadPart;

		m_hFileMapping = ::CreateFileMappingW(
			    m_hFile,
			    NULL,
			    PAGE_READONLY,
			    0,
			    0,
			    NULL);

        ok = m_hFileMapping.IsValid();
	}

	return ok;
}


//=================================================================================================
void MemMapFile::Close()
{
	if (m_view != NULL)
	{
		::UnmapViewOfFile(m_view);
		m_view = NULL;
	}

    m_hFileMapping.Close();
    m_hFile.Close();
}

//=================================================================================================
void* MemMapFile::MapView(unsigned __int64 viewOffset, SIZE_T& viewLength)
{
	char* pChar = 0;

    if (m_hFileMapping.IsValid())
	{
		// If viewLength is 0 we will try to map to the end of the file.
		// Also ensure we don't go beyond the end of the file.
		if (viewLength == 0 ||
			viewOffset + viewLength > m_fileSize)
		{
			// We have to do a few tricks here because of 32bit, 64bit issues.
			unsigned __int64 fileViewLength = m_fileSize - viewOffset;
			viewLength = (SIZE_T)~0;

			if (viewLength >= fileViewLength)
			{
				// The cast is safe!
				viewLength = (SIZE_T)fileViewLength;
			}
			else
			{
				// We can't satisfy the request.
				return NULL;
			}
		}

		// Maybe we don't need to re-map the view?!
		if (m_view != NULL &&
			viewOffset >= m_viewOffset &&
			viewOffset + viewLength <= m_viewOffset + m_viewLength)
		{
			pChar = m_view + (viewOffset - m_viewOffset);
		}
		else // We are out of luck. Remap!
		{
			// Align on a 64k boundary.
			unsigned __int64 offset = viewOffset / m_sysInfo.dwAllocationGranularity;
			offset *= m_sysInfo.dwAllocationGranularity;

			// Adjust the length of the view.
			unsigned __int64 fileViewLength = viewLength + viewOffset - offset;

			if (fileViewLength <= (SIZE_T)~0)
			{
				viewLength = (SIZE_T)fileViewLength;
			}
			else
			{
				return NULL;
			}

			LARGE_INTEGER li;
			li.QuadPart = offset;

			// Honor the minimum view length.
			if (viewLength < MinViewLength)
			{
				viewLength = MinViewLength;

				if (offset + viewLength > m_fileSize)
				{
					// We have to do a few tricks here because of 32bit, 64bit issues.
					// The cast is always safe because of the check above.
					viewLength = (SIZE_T)(m_fileSize - offset);
				}
			}

			::UnmapViewOfFile(m_view);

			m_view = (char*)::MapViewOfFile(
				    m_hFileMapping,
				    FILE_MAP_READ,
				    li.HighPart,
				    li.LowPart,
				    viewLength);

			::MEMORY_BASIC_INFORMATION mbi;
			::VirtualQuery(m_view, &mbi, sizeof(mbi));

			if (m_view != NULL)
			{
				// SUCCESS!
				m_viewOffset = offset;
				m_viewLength = mbi.RegionSize > 0 ? mbi.RegionSize : viewLength;
				pChar = &m_view[viewOffset - offset];
			}
		}
	}

	return pChar;
}
