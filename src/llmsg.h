// ------------------------------------------------------------------------------------------------
//  llmsg - Simple info, debug and error message presentation classes
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
// ------------------------------------------------------------------------------------------------

#pragma once
#ifndef LOGMSG_H
#define LOGMSG_H

#include <Windows.h>
#include <iostream>
#include <sstream>


// Normal console output, can be redirected or disabled.
namespace LLMsg
{
// Output can be redirected or disabled via RedirectOutput or SetNullStream
std::ostream& Out();

void RedirectOutput(std::ostream& out, bool tee = false);
void SetNullStream();
void RestoreStream();

// Present formatted time string to Out()
void PresentMseconds(DWORD mseconds);

// Get message from error number.
std::string GetErrorMsg(DWORD error);
std::string GetLastErrorMsg();

// Present error message (calls Alarm)
void PresentError(DWORD error, const char* frontMsg, const char* tailMsg, const char* eol="\n");

void Alarm();
}

// ------------------------------------------------------------------------------------------------
// Base log class.

class BaseMsg
{
public:
    enum LogType { eInfo, eDebug, eError };

    BaseMsg(LogType logType) : m_logType(logType)
    {}

    ~BaseMsg();

    template <typename TT>
    std::ostringstream& operator<< (const TT& t)
    { m_sout << t; return m_sout; }

    std::ostringstream m_sout;
    LogType m_logType;
};

// ------------------------------------------------------------------------------------------------
// Format and present debug messages, disabled in release build.

class DebugMsg : public BaseMsg
{
public:
    DebugMsg() : BaseMsg(eDebug) {}
    template <typename TT>
    std::ostringstream& operator<< (const TT& t)
    {
#ifdef _DEBUG
        m_sout << t;
#endif
        return m_sout;
    }
};

// ------------------------------------------------------------------------------------------------
// Format and present information messages.

class InfoMsg : public BaseMsg
{
public:
    InfoMsg() : BaseMsg(eInfo) {}
};

// ------------------------------------------------------------------------------------------------
// Format and present error messages.

class ErrorMsg : public BaseMsg
{
public:
    ErrorMsg() : BaseMsg(eError) {}
};

// ------------------------------------------------------------------------------------------------
// Output console text with inline colorization via %BF B=background, F=foreground
//  
// Color hex codes (!0c = red foreground, !1c - red foreground dark blue background)
//     ---- Hex Color Values ---
//     00 Black           08 DarkGray    
//     01 DarkBlue        09 Blue        
//     02 DarkGreen       0a Green      
//     03 DarkCyan        0b Cyan       
//     04 DarkRed         0c Red        
//     05 DarkMagenta     0d Magenta    
//     06 DarkYellow      0e Yellow     
//     07 Gray            0f White   
//     
//  Example (Red Hello, Green World):
//    Colorize(cout, "!0cHello %aWorld!0f");
std::ostream& Colorize(std::ostream& out, const char*);
     
// ------------------------------------------------------------------------------------------------
std::ostream& BlueFg(std::ostream& out);
std::ostream& GreenFg(std::ostream& out);
std::ostream& RedFg(std::ostream& out);
std::ostream& YellowFg(std::ostream& out);
std::ostream& CyanFg(std::ostream& out);
std::ostream& PinkFg(std::ostream& out);
std::ostream& BlackFg(std::ostream& out);
std::ostream& WhiteFg(std::ostream& out);
#endif
