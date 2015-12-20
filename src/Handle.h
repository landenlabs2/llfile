//-----------------------------------------------------------------------------
// Handle - Support routines shared by LL files files.
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

#pragma once

//-----------------------------------------------------------------------------
// Simple handle wrapper to close when it goes out of scope.
class Handle
{
public:
    Handle() :
        m_handle(INVALID_HANDLE_VALUE)
    { }

    Handle(const Handle& other) :
        m_handle(other.Duplicate())
    { }

    Handle(HANDLE handle) :
        m_handle(handle)
    { }

    Handle& operator=(const Handle& other)
    {
        if (m_handle != other.m_handle)
        {
            Close();
            m_handle = other.Duplicate();
        }
        return *this;
    }

    Handle& operator=(HANDLE other)
    {
        if (m_handle != other)
        {
            Close();
            m_handle = other;
        }
        return *this;
    }

    ~Handle()
    { Close(); }

    bool IsValid() const
    { return (m_handle != INVALID_HANDLE_VALUE); }
     bool NotValid() const
    { return  (m_handle == INVALID_HANDLE_VALUE); }

    operator HANDLE() const
    { return m_handle; }

    void Close()
    {
        if (IsValid())
        {
            ::CloseHandle(m_handle);
            m_handle = INVALID_HANDLE_VALUE;
        }
    }

protected:
    HANDLE Duplicate() const
    {
        if (IsValid())
        {
            HANDLE handle;
            if (::DuplicateHandle(
                    ::GetCurrentProcess(),
                    m_handle,
                    ::GetCurrentProcess(),
                    &handle,
                    0,
                    FALSE,
                    DUPLICATE_SAME_ACCESS))
            {
                return handle;
            }
        }

        return INVALID_HANDLE_VALUE;
    }

protected:
    HANDLE m_handle;
};
