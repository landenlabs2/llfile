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


#include "llmsg.h"
#include <iostream>
#include <iomanip>


// ------------------------------------------------------------------------------------------------
// TODO - get default color.
static WORD sDefColor = 0x0F;
static WORD sBgColor = 0;
static WORD sFgColor = FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE;

std::ostream& SetColor(std::ostream& out, WORD fg, WORD bg)
{
    HANDLE hOut = INVALID_HANDLE_VALUE;
    if ((void*)std::cout.rdbuf() == (void*)out.rdbuf())
        hOut = GetStdHandle(STD_OUTPUT_HANDLE); 
    else if ((void*)std::cerr.rdbuf() == (void*)out.rdbuf())
        hOut = GetStdHandle(STD_ERROR_HANDLE); 
    if (hOut != INVALID_HANDLE_VALUE) 
    {
        sFgColor = fg;
        sBgColor = bg;
        SetConsoleTextAttribute(hOut, sFgColor | sBgColor);
    }
    return out;
}

 std::ostream& BlueFg(std::ostream& out)
{   return SetColor(out, FOREGROUND_BLUE, sBgColor); }
 std::ostream& GreenFg(std::ostream& out)
{   return SetColor(out, FOREGROUND_GREEN, sBgColor); }
 std::ostream& RedFg(std::ostream& out)
{   return SetColor(out, FOREGROUND_RED, sBgColor); }
 std::ostream& YellowFg(std::ostream& out)
{   return SetColor(out, FOREGROUND_RED|FOREGROUND_GREEN, sBgColor); }
 std::ostream& CyanFg(std::ostream& out)
{   return SetColor(out, FOREGROUND_BLUE|FOREGROUND_GREEN, sBgColor); }
 std::ostream& PinkFg(std::ostream& out)
{   return SetColor(out, FOREGROUND_BLUE|FOREGROUND_RED, sBgColor); }
 std::ostream& WhiteFg(std::ostream& out)
{   return SetColor(out, FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE, sBgColor); }
 std::ostream& BlacFg(std::ostream& out)
{   return SetColor(out, 0, sBgColor); }



// ------------------------------------------------------------------------------------------------
// Output console text with inline colorization via %BF B=background, F=foreground
//  
// Color hex codes (!0c = red foreground, !1c - red foreground dark blue background)
//     ---- Hex Color Values ---
//     0 Black           8 DarkGray    
//     1 DarkBlue        9 Blue        
//     2 DarkGreen       a Green      
//     3 DarkCyan        b Cyan       
//     4 DarkRed         c Red        
//     5 DarkMagenta     d Magenta    
//     6 DarkYellow      e Yellow     
//     7 Gray            f White   
//     
//  Example (Red Hello, Green World):
//    Colorize(cout, "!0cHello %aWorld!0f");
std::ostream& Colorize(std::ostream& out, const char* str) 
{
    const char* colorPtr;
    const char* prevPtr = str;
    while ((colorPtr = strchr(prevPtr, '!')) != NULL)
    {
        unsigned int color; 
        if (sscanf(colorPtr+1, "%02x", &color) == 1)
        {
            if (colorPtr != prevPtr) 
            {
                out.write(prevPtr, colorPtr - prevPtr);
                out.flush();
            }
            SetColor(out, color & 0x0f, color & 0xf0);
            prevPtr = colorPtr+3;
        }
        else
        {            
            if (colorPtr != prevPtr) 
                out.write(prevPtr, colorPtr - prevPtr);
            prevPtr = colorPtr+1;
        }
    }

    if (colorPtr != prevPtr) 
        out << prevPtr;
    SetColor(out, sDefColor & 0x0f, sDefColor & 0xf0);
    return out;
}


// ------------------------------------------------------------------------------------------------
namespace LLMsg
{

struct Nullbuf : public std::streambuf { };

static Nullbuf nullbuf;
static std::streambuf* origbuf = NULL;

const WORD FOREGROUND_WHITE = FOREGROUND_RED + FOREGROUND_GREEN + FOREGROUND_BLUE;
const WORD BACKGROUND_WHITE = BACKGROUND_RED + BACKGROUND_GREEN + BACKGROUND_BLUE;

//
// http://www.cs.technion.ac.il/~imaman/programs/teestream.html
//
template<typename Elem, typename Traits = std::char_traits<Elem> >
struct basic_TeeStream : std::basic_ostream<Elem,Traits>
{
   typedef std::basic_ostream<Elem,Traits> SuperType;

   basic_TeeStream(std::ostream& o1, std::ostream& o2)
      :  SuperType(o1.rdbuf()), o1_(o1), o2_(o2) { }

   basic_TeeStream& operator<<(SuperType& (__cdecl *manip)(SuperType& ))
   {
      o1_ << manip;
      o2_ << manip;
      return *this;
   }

   template<typename T>
   basic_TeeStream& operator<<(const T& t)
   {
      o1_ << t;
      o2_ << t;
      return *this;
   }

private:
   std::ostream& o1_;
   std::ostream& o2_;
};

// ---------------------------------------------------------------------------
std::ostream& Err()
{
    return std::cerr;
}

// ---------------------------------------------------------------------------
// Output can be redirected or disabled via RedirectOutput or SetNullStream
std::ostream& Out()
{
    if (std::cout.bad())
        std::cout.clear();
    return std::cout;
}

// ---------------------------------------------------------------------------
void RedirectOutput(std::ostream& out, bool tee)
{
    if (tee)
        static basic_TeeStream<char> teeStream(std::cout, out);
    else
        std::cout.rdbuf(out.rdbuf());
}

// ---------------------------------------------------------------------------
void SetNullStream()
{
    origbuf = Out().rdbuf(&nullbuf);
}

// ---------------------------------------------------------------------------
void RestoreStream()
{
    Out().rdbuf(origbuf);
}

// ---------------------------------------------------------------------------
void PresentMseconds(DWORD mseconds)
{
    double seconds = mseconds / 1000.0;

    if (seconds < 60)
        Out() << std::setprecision(4) << seconds << " (seconds)";
    else if (seconds < 3600)
        Out() << std::setprecision(2) << seconds/60 << " (minutes)";
    else
        Out() << std::setprecision(2) << seconds/3600 << " (hours)";

}

// ------------------------------------------------------------------------------------------------
// Convert windows error number to error message.

std::string GetErrorMsg(DWORD error)
{
    std::string errMsg;
    if (error != 0)
    {
        LPTSTR pszMessage;
        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            error,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR)&pszMessage,
            0, NULL );

        errMsg = pszMessage;
        LocalFree(pszMessage);
        int eolPos = (int)errMsg.find_first_of('\r');
        errMsg.resize(eolPos);
        errMsg.append(" ");
    }
    return errMsg;
}

// ------------------------------------------------------------------------------------------------
std::string GetLastErrorMsg()
{
    return GetErrorMsg(GetLastError());
}

//-----------------------------------------------------------------------------
void PresentError(DWORD error,
    const char* frontMsg,
    const char* tailMsg,
    const char* eol)
{
    ErrorMsg() << frontMsg << GetErrorMsg(error) << tailMsg << eol;
    Alarm();
}

// ------------------------------------------------------------------------------------------------
void Alarm()
{
     Beep(750, 300);
}
}

// ------------------------------------------------------------------------------------------------
BaseMsg::~BaseMsg()
{
    std::string msg = m_sout.str();
    try
    {
        if (m_logType == eError)
        {
            int pos = (int)msg.find('\01');
            if (pos != -1)
                msg.resize(pos);
            OutputDebugString(msg.c_str());

            HANDLE outHandle = GetStdHandle(STD_OUTPUT_HANDLE);
            SetConsoleTextAttribute(outHandle, FOREGROUND_RED + FOREGROUND_INTENSITY);
            LLMsg::Err() << msg;
            SetConsoleTextAttribute(outHandle, LLMsg::FOREGROUND_WHITE);
        }
        else
            LLMsg::Out() << msg;
    }
    catch (...) {}
}


