// ------------------------------------------------------------------------------------------------
// Simple smart pointer to delete shared pointer when last instance goes out of scope.
//
// Project: LLFile
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
// ------------------------------------------------------------------------------------------------

#pragma once

#include <assert.h>

/// Simple smart pointer with reference counting.
template < typename T > 
class SharePtr
{
public:
    SharePtr() :
        m_pShareItem(0) 
        {}

    SharePtr(T* pItem) :
        m_pShareItem(new ShareItem(pItem)) 
        {}

    SharePtr(const SharePtr& rhs) :
        m_pShareItem(rhs.m_pShareItem != NULL ? rhs.m_pShareItem->Add() : NULL) 
        {}

    SharePtr& operator=(const SharePtr& rhs)
    {
        if (this != &rhs)
        {
            m_pShareItem = rhs.m_pShareItem != NULL ? rhs.m_pShareItem->Add() : NULL;
        }
        return *this;
    }

    ~SharePtr() 
    {
        if (m_pShareItem != NULL)
        {
            int refCnt = m_pShareItem->Dec();
            assert(refCnt >= 0);

            if (0 == refCnt)
            {
                delete m_pShareItem;
            }
        }
    }

    class ShareItem
    {
    public:
        ShareItem(T* pItem) : m_pItem(pItem), m_refCnt(1), m_marker(123456) {}
        ~ShareItem() 
        { 
            assert(m_refCnt == 0);
            assert(m_marker == 123456);

            m_refCnt = 0;
            m_marker = 654321;
            
            delete m_pItem;
            m_pItem = 0;
        }

        ShareItem* Add() 
        { m_refCnt++; return this; }    // not thread safe

        int Dec() 
        { return --m_refCnt; }          // not thread safe

        T*          m_pItem;
        mutable int m_refCnt;
        int         m_marker;
    };


    bool IsNull() const
    {
        return m_pShareItem == NULL || m_pShareItem->m_pItem == NULL;
    }

    void Clear()
    {  m_pShareItem = NULL; }

    operator T*() const
    { 
        return (m_pShareItem != NULL) ? m_pShareItem->m_pItem : NULL;
    }

    T* operator-> () const
    { 
        return (m_pShareItem != NULL) ? m_pShareItem->m_pItem : NULL;
    }

   
private:
    ShareItem*  m_pShareItem;
};

