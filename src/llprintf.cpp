//-----------------------------------------------------------------------------
// llprintf - Print format (printf + strftime)
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

#pragma warning(disable:4996)

#include <iostream>
#include <assert.h>
#include <conio.h>
#include <tchar.h>

#include <fcntl.h>
#include <io.h>

#include <time.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <sys/stat.h>

#include <regex>
#include <queue>
#include <string>
#include <fstream>

#include "llprintf.h"

// ---------------------------------------------------------------------------

// TODO - add support for Environment variables and input file

//
//  llp -E path -F; -p%s\n
//  dir | llp -i- -p%.20s,\n

static const char sHelp[] =
" Printf " LLVERSION "\n"
" Print format \n"
"\n"
"  !0eSyntax:!0f\n"
"    [<switches>]  \n"
"\n"
"  !0eWhere switches are:!0f\n"
"   -I=<inFile>         ; Read input from file or stdin if -\n"
"                       ;  NOTE: -I must be after the -a options \n"
"                       ; Example to convert to uppercase\n"
"                       ;  echo foo | p -a=pn -I=- -p=%s,%s \n"
"   -i=<text>           ; Parse text per separator and send to formatter\n"
"                       ;  NOTE: -a must be before -i to control case\n"
"                       ;  ex:  -a=nuncn  -i=Hello -p=%s,%s,%s\n"
"   -Bc                 ; Field separator (see -e example below) \n"
"   -C=<colorOpt>       ; Set colors, colors are red,green,blue,intensity, add bg to end of color for background\n"
"                       ;  ex -C=red+blue or -C=green,redbg+bluebg\n"
"   -D                  ; Only directories\n"
"   -e=<envName>        ; Input is environment variable or -e=* or -e for all \n"
"   -F[=<filePat>]      ; Only files\n"
"   -a=[pdn...]         ; File arguments passed to printf\n"
"                       ;   p=full path\n"
"                       ;   d=directory\n"
"                       ;   n=name\n"
"                       ;   r=name root (no extension)\n"
"                       ;   e=extension (right of last .)\n"
"                       ;   s=size (as a string)\n"
"                       ;   _=defcase\n"
"                       ;   l=lowercase\n"
"                       ;   u=uppercase\n"
"                       ;   c=captialize\n"
"                       ;   x=unix slash\n"
"                       ;   X=dos slash\n"
"                       ;  NOTE:  -a must proceed -i or -I \n"
"   -p=<fmt>            ; Printf format, use any valid %s format, %20s \n"
"                       ;  input is either from redirection -i-   \n"
"                       ;  or input is from a file  -i<filename> \n"
"                       ;  or input is from file directory scan (trailing arguments) \n"
"                       ; Use quotes to include spaces in format \n"
"                       ;  or a string string as in \"-pHello World\\n\"  \n"
"   -P=<srcPathPat>     ; Regular expression pattern on source files path or env items\n"
"   -s=<fmt>            ; Scan input with regex\n"
"   -l=<timeFmt>        ; strftime format using local time\n"
"   -z=<timeFmt>        ; strftime format using zTime\n"
"   -q                  ; Quiet, default is echo command\n"
"   -Q=n                ; Quit after 'n' matches\n"
"   -X=<pattern>        ; Exclude patterns  -X *.lib,*.obj,*.exe\n"
"   -1=<outfile>        ; Redirect output to file\n"
"\n"
"  !0eNote:!0f\n"
"    Remember to escape % if used in a batch file with two %% \n"
"  !0eExamples:!0f\n"
"    ; Use -a to place two copies of filename as argument to printf\n"
"      p -a=nn \"-p=p4 move support\\\\%s radar\\\\%s\\n\"  radar\\* \n"
"    ; Generate string to copy file to .bak extension, save in batch file\n"
"      p -a=nr \"-p=copy %s %s.bak\"  *.tax > job.bat \n"
"\n"
"    ; set -p to define printf before -i which specifies the input followed by scan using regex\n"
"      dir | p \"-p=%.0syear=%s mon=%s day=%s\\n\" -I=- \"-s=([0-9]*)/([0-9]*)/([0-9]*).*\"  \n"
"    ; set -p after -I to print contents of input \n"
"      dir | p -I=-  -p=Files=%s\\n\\n  \n"
"      p -I=foo.txt \"-p=Stuff %s %s Stuff\\n\"  \n"
"    ; Change line termination DOS/Unix (use -1=- to force binary output) \n"
"      p -I=unix.txt -1=dos.txt  -p=\"%s\\r\\n\"  \n"
"      p -I=dos.txt  -1=unix.txt -p=\"%s\\n\" \n"
"      blah |  p -I=-  -1=- -p=\"%s\\n\" \n"
"\n"
"    ; List all environment variables \n"
"      -xp -e=* -B== -p=\"%20.20s  = %s\\n\"  \n"
"    ; List environment variable path, split on semicolon\n"
"      -e=PATH   \"-p=%s\\n\"   \n"
"      -e=PATH   \"-p=%4# %s\\n\"         ; Number env path directories\n"
"      -P=Program -e=* -p%s\\n          ; Show env's which contain word Program \n"
"      -e=PATH   \"-p=%s\" *.txt         ; Scan paths for *.txt \n"
"      \"-l=Date is %m/%d/%y %H:%M:%S\"                  ; local time\n"
"      \"-l=Date is %m/%d/%y\" \"-p=\\n\" \"-l=%H:%M:%S\"     ; local time\n"
"\n"
"  !0eSpecial Escape characters:!0f\n"
"      \\g    ; > greater \n"
"      \\l    ; < less \n"
"      \\p    ; | pipe \n"
"\n"
"      \\n    ; newline \n"
"      \\r    ; carrage return \n"
"\n"
"      \\t    ; tab \n"
"      \\q    ; \" quote \n"
"\n"
"  !0estrftime:!0f\n"
"\t%a Abbreviated weekday name\n"
"\t%A Full weekday name\n"
"\t%b Abbreviated month name\n"
"\t%B Full month name \n"
"\t%c Date and time representation appropriate for locale\n"
"\t%d Day of month as decimal number (01 – 31) \n"
"\t%H Hour in 24-hour format (00 – 23)         \n"
"\t%I Hour in 12-hour format (01 – 12)         \n"
"\t%j Day of year as decimal number (001 – 366)\n"
"\t%m Month as decimal number (01 – 12)        \n"
"\t%M Minute as decimal number (00 – 59)       \n"
"\t%p Current locale's A.M./P.M. indicator for 12-hour clock\n"
"\t%S Second as decimal number (00 – 59)       \n"
"\t%U Week of year as decimal number, with Sunday as first day of week (00 – 53) \n"
"\t%w Weekday as decimal number (0 – 6; Sunday is 0) \n"
"\t%W Week of year as decimal number, with Monday as first day of week (00 – 53) \n"
"\t%x Date representation for current locale   \n"
"\t%X Time representation for current locale   \n"
"\t%y Year without century, as decimal number (00 – 99)\n"
"\t%Y Year with century, as decimal number     \n"
"\t%z, %Z Either the time-zone name or time zone abbreviation\n"
"\t%%  Percent sign \n"
"\n"
"\n";


LLPrintfConfig LLPrintf::sConfig;
enum CaseFold { eDefCase, eLowerCase, eUpperCase, eCapitalize, eUnixSlash, eDosSlash};
typedef std::queue<std::string> StringQueue;

// ---------------------------------------------------------------------------
void InvalidParameterHandler(const wchar_t* expression,
   const wchar_t* function,
   const wchar_t* file,
   uint line,
   uintptr_t pReserved)
{
    ErrorMsg() << "Invalid time format string, see help -?\n";
    // printf("Invalid parameter, Function: %s. File: %s Line: %d\n", function, file, line);
    // printf("Expression: %s\n", expression);
}



// http://www.koders.com/c/fid773208C3D6E0A7F3C576C08377D83F5B73917494.aspx?s=sort

#define isodigit(c) ((c) >= '0' && (c) <= '7')
#define hextobin(c) ((c) >= 'a' && (c) <= 'f' ? (c) - 'a' + 10 : \
             (c) >= 'A' && (c) <= 'F' ? (c) - 'A' + 10 : (c) - '0')
#define octtobin(c) ((c) - '0')

typedef int  intMax_t;
typedef uint uintMax_t;

// ---------------------------------------------------------------------------
// Output a single-character \ escape.  
// abfglnpqrtv

static void
print_esc_char (char c)
{
    switch (c)
    {
    case 'a':           // Alert
      c = '\a';
      break;
    case 'b':           // Backspace
      c = '\b';
      break;
    case 'f':           // Form feed
      c = '\f';
      break;
    case 'g':			// greater than
      c = '>';
    case 'l':           // less than
      c = '<';
      break;
    case 'n':           // New line
      c = '\n';
      break;
    case 'p':           // pipe
      c = '|';
	  break;
	case 'q':			// quote
	  c = '"';
      break;
    case 'r':            /* Carriage return. */
      c = '\r';
      break;
    case 't':            /* Horizontal tab. */
      c = '\t';
      break;
    case 'v':            /* Vertical tab. */
      c = '\v';
      break;
    default:
      break;
    }

  putchar(c);
}

const char MissingHexInEscape[] = "Missing hexadecimal number in escape\n";

// ---------------------------------------------------------------------------
#define to_uchar(p)  (unsigned char)(p)

/* Print a \ escape sequence starting at ESCSTART.
   Return the number of characters in the escape sequence
   besides the backslash.
   If OCTAL_0 is nonzero, octal escapes are of the form \0ooo, where o
   is an octal digit; otherwise they are of the form \ooo.  */

static int
print_esc (const char *escstart, bool octal_0)
{
    const char *p = escstart + 1;
    int esc_value = 0;        /* Value of \nnn escape. */
    int esc_length;            /* Length of \nnn escape. */

    if (*p == 'x')
    {
        /* A hexadecimal \xhh escape sequence must have 1 or 2 hex. digits.  */
        for (esc_length = 0, ++p;  esc_length < 2 && isxdigit(*p); ++esc_length, ++p)
            esc_value = esc_value * 16 + hextobin (*p);

        if (esc_length == 0)
            ErrorMsg() << MissingHexInEscape;
        putchar(esc_value);
    }
    else if (isodigit (*p))
    {
        /* Parse \0ooo (if octal_0 && *p == '0') or \ooo (otherwise).
        Allow \ooo if octal_0 && *p != '0'; this is an undocumented
        extension to POSIX that is compatible with Bash 2.05b.  */
        for (esc_length = 0, p += octal_0 && *p == '0'; esc_length < 3 && isodigit (*p);  ++esc_length, ++p)
            esc_value = esc_value * 8 + octtobin (*p);
        putchar(esc_value);
    }
    else if (*p && strchr ("\"\\abfglnpqrtv", *p))
        print_esc_char (*p++);
    else if (*p == 'u' || *p == 'U')
    {
        char esc_char = *p;
        uint uni_value;

        uni_value = 0;
        for (esc_length = (esc_char == 'u' ? 4 : 8), ++p;
            esc_length > 0;
            --esc_length, ++p)
        {
            if ( !isxdigit(*p))
                ErrorMsg() << MissingHexInEscape;

            uni_value = uni_value * 16 + hextobin (*p);
        }

#if 0
        /* A universal character name shall not specify a character short
        identifier in the range 00000000 through 00000020, 0000007F through
        0000009F, or 0000D800 through 0000DFFF inclusive. A universal
        character name shall not designate a character in the required
        character set.  */
        if ((uni_value <= 0x9f
            && uni_value != 0x24 && uni_value != 0x40 && uni_value != 0x60)
            || (uni_value >= 0xd800 && uni_value <= 0xdfff))
            error("invalid universal character name \\%c%0*x",
                esc_char, (esc_char == 'u' ? 4 : 8), uni_value);

        print_unicode_char (stdout, uni_value, 0);
#endif
    }
    else
    {
        putchar('\\');
        if (*p)
        {
            putchar(*p);
            p++;
        }
    }
    return int(p - escstart) - 1;
}

// ---------------------------------------------------------------------------
/* Print string STR, evaluating \ escapes. */
static void
print_esc_string (const char *str)
{
    for (; *str; str++)
        if (*str == '\\')
            str += print_esc (str, true);
        else
            putchar(*str);
}

// ---------------------------------------------------------------------------
/* Evaluate a printf conversion specification.  START is the start of
   the directive, LENGTH is its length, and CONVERSION specifies the
   type of conversion.  LENGTH does not include any length modifier or
   the conversion specifier itself.  FIELD_WIDTH and PRECISION are the
   field width and precision for '*' values, if HAVE_FIELD_WIDTH and
   HAVE_PRECISION are true, respectively.  ARGUMENT is the argument to
   be formatted.  */

char PRIdMAX[] = "d";     // ??? not sure what this is
static void
print_direc (const char *start, size_t length, char conversion,
         bool have_field_width, int field_width,
         bool have_precision, int precision,
         char const *argument)
{
    char *p;        // Null-terminated copy of % directive.

    /* Create a null-terminated copy of the % directive, with an
    intmax_t-wide length modifier substituted for any existing
    integer length modifier.  */
    {
        char *q;
        char const *length_modifier;
        size_t length_modifier_len;

        switch (conversion)
        {
        case 'd': case 'i': case 'o': case 'u': case 'x': case 'X':
            length_modifier = PRIdMAX;
            length_modifier_len = sizeof PRIdMAX - 2;
            break;

        case 'a': case 'e': case 'f': case 'g':
        case 'A': case 'E': case 'F': case 'G':
            length_modifier = "";
            length_modifier_len = 1;
            break;

        default:
            length_modifier = start;  /* Any valid pointer will do.  */
            length_modifier_len = 0;
            break;
        }

        p = new char[length + length_modifier_len + 2];
        q = strncpy(p, start, length) + length;
        q = strncpy(q, length_modifier, length_modifier_len) + length_modifier_len;
        *q++ = conversion;
        *q = '\0';
    }

    switch (conversion)
    {
    case 'd':
    case 'i':
        {
            intMax_t arg = strtol(argument, NULL, 10);
            if ( !have_field_width)
            {
                if ( !have_precision)
                    printf (p, arg);
                else
                    printf (p, precision, arg);
            }
            else
            {
                if ( !have_precision)
                    printf (p, field_width, arg);
                else
                    printf (p, field_width, precision, arg);
            }
        }
        break;

    case 'o':
    case 'u':
    case 'x':
    case 'X':
        {
            uintMax_t arg = strtoul(argument, 0, 10);
            if ( !have_field_width)
            {
                if ( !have_precision)
                    printf (p, arg);
                else
                    printf (p, precision, arg);
            }
            else
            {
                if ( !have_precision)
                    printf (p, field_width, arg);
                else
                    printf (p, field_width, precision, arg);
            }
        }
        break;

    case 'a':
    case 'A':
    case 'e':
    case 'E':
    case 'f':
    case 'F':
    case 'g':
    case 'G':
        {
            long double arg = strtod(argument, NULL);
            if ( !have_field_width)
            {
                if ( !have_precision)
                    printf (p, arg);
                else
                    printf (p, precision, arg);
            }
            else
            {
                if ( !have_precision)
                    printf (p, field_width, arg);
                else
                    printf (p, field_width, precision, arg);
            }
        }
        break;

    case 'c':
        if ( !have_field_width)
            printf (p, *argument);
        else
            printf (p, field_width, *argument);
        break;

    case 's':
        if ( !have_field_width)
        {
            if ( !have_precision)
                printf(p, argument);
            else
                printf(p, precision, argument);
        }
        else
        {
            if ( !have_precision)
                printf(p, field_width, argument);
            else
                printf(p, field_width, precision, argument);
        }
        break;
    }

    delete (p);
}

// ---------------------------------------------------------------------------
// Print the text in FORMAT, using ARGV (with ARGC elements) for
//   arguments to any `%' directives.
//   Return the number of elements of ARGV used.

static int
PrintFormatted(
    const char *format,
    StringQueue& args,
    size_t nLine)
{
    const char *f;					// Pointer into `format'.  
    const char *direc_start;		// Start of % directive.  
    size_t direc_length;			// Length of % directive.   
    bool have_field_width;			// True if FIELD_WIDTH is valid.  
    int field_width = 0;			// Arg to first '*'. 
    bool have_precision;			// True if PRECISION is valid.  
    int precision = 0;				// Arg to second '*'.  
    bool ok[UCHAR_MAX + 1];			// ok['x'] is true if %x is allowed.  

    for (f = format; *f; ++f)
    {
        switch (*f)
        {
        case '%':
            direc_start = f++;
            direc_length = 1;
            have_field_width = have_precision = false;
            if (*f == '%')
            {
                putchar('%');
                break;
            }
            if (*f == 'b')
            {
                /* FIXME: Field width and precision are not supported
                for %b, even though POSIX requires it.  */
                if ( !args.empty())
                {
                    print_esc_string(args.front().c_str());
                    args.pop();
                }
                break;
            }

            memset(ok, false, sizeof(ok));
            ok['a'] = ok['A'] = ok['c'] = ok['d'] = ok['e'] = ok['E'] =
                ok['f'] = ok['F'] = ok['g'] = ok['G'] = ok['i'] = ok['o'] =
                ok['s'] = ok['u'] = ok['x'] = ok['X'] = true;

            for (;; f++, direc_length++)
            {
                switch (*f)
                {
                case '\'':
                    ok['a'] = ok['A'] = ok['c'] = ok['e'] = ok['E'] =
                        ok['o'] = ok['s'] = ok['x'] = ok['X'] = false;
                    break;
                case '-': case '+': case ' ':
                    break;
                case '#':
                    ok['c'] = ok['d'] = ok['i'] = ok['s'] = ok['u'] = false;
                    break;
                case '0':
                    ok['c'] = ok['s'] = false;
                    break;
                default:
                    goto no_more_flag_characters;
                }
            }
no_more_flag_characters:;

            if (*f == '*')
            {
                ++f;
                ++direc_length;
                if ( !args.empty())
                {
                    intMax_t width = atoi(args.front().c_str());
                    if (INT_MIN <= width && width <= INT_MAX)
                        field_width = width;
                    else
                        ErrorMsg() << "Invalid field width:" << args.front() << std::endl;
                    args.pop();
                }
                else
                    field_width = 0;
                have_field_width = true;
            }
            else
                while (isdigit(*f))
                {
                    ++f;
                    ++direc_length;
                }
                if (*f == '.')
                {
                    ++f;
                    ++direc_length;
                    ok['c'] = false;
                    if (*f == '*')
                    {
                        ++f;
                        ++direc_length;
                        if ( !args.empty())
                        {
                            intMax_t prec = atoi(args.front().c_str());
                            if (prec < 0)
                            {
                                /* A negative precision is taken as if the
                                precision were omitted, so -1 is safe
                                here even if prec < INT_MIN.  */
                                precision = -1;
                            }
                            else if (INT_MAX < prec)
                                ErrorMsg() << "Invalid precision:" << args.front() << std::endl;
                            else
                                precision = prec;
                            args.pop();
                        }
                        else
                            precision = 0;
                        have_precision = true;
                    }
                    else
                    {
                        while (isdigit (*f))
                        {
                            ++f;
                            ++direc_length;
                        }
                    }
                }

                if (*f == '#')
                {
                    char nLineStr[20];
                    sprintf_s(nLineStr, ARRAYSIZE(nLineStr), "%zu", nLine);
                    print_direc (direc_start, direc_length, 'u',
                        have_field_width, field_width,
                        have_precision, precision,
                        nLineStr);
                }
                else
                {
                    while (*f == 'l' || *f == 'L' || *f == 'h'
                        || *f == 'j' || *f == 't' || *f == 'z')
                        ++f;
                    {
                        unsigned char conversion = *f;
                        if ( !ok[conversion])
                            ErrorMsg() << "Invalid conversion specification:" // number was used to truncate via %.*s
                                << (int) (f + 1 - direc_start) << direc_start << std::endl;
                    }

                    print_direc (direc_start, direc_length, *f,
                        have_field_width, field_width,
                        have_precision, precision,
                        (args.empty() ? "" : args.front().c_str()));
                    if ( !args.empty())
                        args.pop();
                }
                break;

        case '\\':
            f += print_esc (f, false);
            break;

        default:
            putchar(*f);
        }
    }

    return args.size();
}

// ---------------------------------------------------------------------------
LLPrintf::LLPrintf() :
    m_gmt(false),
    m_printFmt(),
    m_timeFmt(),
    m_pushArgs("p")
{
    sConfigp = &GetConfig();

    CONSOLE_SCREEN_BUFFER_INFO consoleScreenBufferInfo;
    GetConsoleScreenBufferInfo(ConsoleHnd(), &consoleScreenBufferInfo);
    m_defColor = consoleScreenBufferInfo.wAttributes;
#if 0
    CONSOLE_SCREEN_BUFFER_INFO consoleScreenBufferInfo;
    GetConsoleScreenBufferInfo(pLLCopy->ConsoleHnd(), &consoleScreenBufferInfo);
    WriteConsoleA(pLLCopy->ConsoleHnd(), msg, msgLen, &written, 0);
    SetConsoleCursorPosition(pLLCopy->ConsoleHnd(), consoleScreenBufferInfo.dwCursorPosition);

    if (GetConsoleScreenBufferInfo(ConsoleHnd(), &csbiInfo))
        m_screenWidth = csbiInfo.dwMaximumWindowSize.X;
#endif
}

// ---------------------------------------------------------------------------
LLConfig& LLPrintf::GetConfig() 
{
    return sConfig;
}

// ---------------------------------------------------------------------------
int LLPrintf::StaticRun(const char* cmdOpts, int argc, const char* pDirs[])
{
    LLPrintf llPrintf;
    return llPrintf.Run(cmdOpts, argc, pDirs);
}

// ---------------------------------------------------------------------------
const char* ChangeCase(const char* inStr, CaseFold caseFold)
{
    if (caseFold == eDefCase || inStr == NULL || *inStr == '\0')
        return inStr;

    static std::string str;
    str = inStr;
    switch (caseFold)
    {
    case eLowerCase:
        std::transform(str.begin(), str.end(),str.begin(), ToLower);
        break;
    case eUpperCase:
        std::transform(str.begin(), str.end(),str.begin(), ToUpper);
        break;
    case eCapitalize:
        std::transform(str.begin(), str.end(),str.begin(), ToLower);
        str[0] = ToUpper(str[0]);
        break;
    case eDefCase:
        break;
	case eUnixSlash:
		std::transform(str.begin(), str.end(), str.begin(), ToUnixSlash);
		break;
	case eDosSlash:
		std::transform(str.begin(), str.end(), str.begin(), ToDosSlash);
		break;
    }

    return str.c_str();
}

// ---------------------------------------------------------------------------
void PushParts(StringQueue& inList, const char* pPushArgs, const char* file)
{
    CaseFold caseFold = eDefCase;
    char* lastDot = (char*)strrchr(file, '.');
    char* lastDir = (char*)strrchr(file, '\\');
    const char* root = (lastDir ? lastDir+1 : file);

    while (*pPushArgs != '\0')
    {
        //  \directory\root.ext
        //
        //  d = directory
        //  n = name (root.ext)
        //  r = root
        //  e = extension
        //  p = directory and name
        //  s = size
        //  l = lowercase
        //  u = uppercase
        //  c = capitalize
        //  _ = default case
		//  x = unix dir slash
        // Todo:
        //  x = creation date
        //  i = link status
        //
        switch (*pPushArgs)
        {
        case '_':
            caseFold = eDefCase;
            break;
        case 'l':
            caseFold = eLowerCase;
            break;
        case 'u':
            caseFold = eUpperCase;
            break;
        case 'c':
            caseFold = eCapitalize;
            break;
		case 'x':
			caseFold = eUnixSlash;
			break;
		case 'X':
			caseFold = eDosSlash;
			break;

        case 'd':   // directory
            if (lastDir)
            {
                *lastDir = '\0';
                inList.push(ChangeCase(file, caseFold));
                *lastDir = '\\';
            }
            else
                inList.push("");
            break;
        case 'n':   // name = root.exe
            inList.push(ChangeCase(root, caseFold));
            break;
        case 'r':   // root
            if (lastDot)
                *lastDot = '\0';
            inList.push(ChangeCase(root, caseFold));
            if (lastDot)
                *lastDot = '.';
            break;
        case 'e':   // extension
            if (lastDot)
                inList.push(ChangeCase(lastDot+1, caseFold));
            else
                inList.push("");
            break;
        default:
        case 'p':   // full path
            inList.push(ChangeCase(file, caseFold));
            break;
        case 's':
            {
                struct stat fileStat;
                if (0 == stat(file, &fileStat))
                {
                    char buf[20];
                    _snprintf(buf, ARRAYSIZE(buf), "%u", fileStat.st_size);
                    inList.push(buf);
                }
                else
                    inList.push("?");
            }
            break;
        }
        ++pPushArgs;
    }
}

#pragma warning(disable : 4706) // assignment within conditional expression
// ---------------------------------------------------------------------------
static void FillListStr(
        StringQueue& inList,
        const char* ibuf,
        char sep,
        const char* pushArgs)
{
    const char* beg = ibuf;
    const char* nxt = ibuf;
    const char* end =  strchr(beg, '\0');
    size_t len = 0;

    while (beg < end && (nxt = strchr(beg, sep)))
    {
        len = nxt - beg;
        PushParts(inList, pushArgs, std::string(beg, len).c_str());
        beg = nxt+1;
    }

    if (beg < end)
        PushParts(inList, pushArgs, beg);
}

// ---------------------------------------------------------------------------
static void FillListStream(
    StringQueue& inList,
    std::istream& in,
    char sep,
    const char* pushArgs)
{
    if (sep == '\0')
        sep = '\n';

    char buf[4096];
    while (in.getline(buf, ARRAYSIZE(buf), sep).good())
    {
        PushParts(inList, pushArgs, buf);
        // inList.push(buf);
        buf[0] = '\0';
    }

    if (buf[0] != '\0')
    {
        PushParts(inList, pushArgs, buf);
        // inList.push(buf);
    }
}

// ---------------------------------------------------------------------------
static void FillListFile(
        StringQueue& inList,
        const char* inFile,
        char sep,
        const char* pushArgs)
{
    if (inFile == NULL || (inFile[0] == '-' && inFile[1] == '\0'))
    {
        FillListStream(inList, std::cin, sep, pushArgs);
    }
    else
    {
        std::ifstream in(inFile);
        if ( !in)
        {
            perror(inFile);
            return;
        }
        FillListStream(inList, in, sep, pushArgs);
    }
}

//  split on separators 
StringQueue& SlitOnSeparators(StringQueue& inList, const string& separators)
{
	if (separators.size() != 0)
	{
		StringQueue outList;
        std::vector<size_t> offsets;
		while (!inList.empty())
		{
			string listItem = inList.front();
            inList.pop();

            const size_t s_M1 = (size_t)-1;
            offsets.clear();
			for (int sepIdx = 0; sepIdx != (int)separators.length(); sepIdx++)
			{
				size_t sepOff = s_M1;
                while ((sepOff = listItem.find(separators[sepIdx], sepOff+1)) != s_M1)
                    offsets.push_back(sepOff);
			}
			
            if (offsets.empty())
                outList.push(listItem);
            else
            {
                std::sort(offsets.begin(), offsets.end());

                int lastPos = 0;
                for (unsigned idx = 0; idx != offsets.size(); idx++)
			    {
                    int off = offsets[idx];
                    outList.push(listItem.substr(lastPos, off-lastPos));
                    lastPos = off +1;
			    }
                outList.push(listItem.substr(lastPos));
            }
		}

		outList.swap(inList);
	}

	return inList;
}

// ---------------------------------------------------------------------------
unsigned int Count(const string& inStr, const char cFind)
{
	unsigned int cnt = 0;
	size_t pos = 0;
	while ((pos = inStr.find(cFind, pos)) != inStr.npos)
	{
		pos++;
		cnt++;
	}

	return cnt;
}

// ---------------------------------------------------------------------------
int LLPrintf::Run(const char* cmdOpts, int argc, const char* pDirs[])
{
    const char intextOptMsg[] = "Input text, -i=<text>";
    const char argOptMsg[] = "File arguments passed to printf, -a=[pdnres_luc]...";
    const char scanOptMsg[] = "Scan using regex, -s=<regex> \n"
        "Example  dir | p \"-p=%.0syear=%s mon=%s day=%s\\n\" -I=- \"-s=([0-9]*)/([0-9]*)/([0-9]*).*\"  \n";
    const char sMissingPrintFmtMsg[] = "Missing print format, -p=<fmt>\n";
    const char sMissingTimeMsg[] = "Missing time format, -t=<fmt> or -l=<fmt> or -z=<fmt>\n";

    std::string str;

    // Initialize run stats
    size_t nFiles = 0;
    char   sep = 0;

    m_printFmt.clear();
    m_timeFmt.clear();
	m_separators.clear();	// Remove defautl \\ and /

    // List of input strings.
    StringQueue     inList;
    size_t          nLine = 0;

    // Get Time ready
     _tzset();
    time_t    now;
    struct tm today;
    time(&now);
    localtime_s(&today, &now);

    if (argc == 0 && *cmdOpts == '\0')
        cmdOpts = "?";

    // Setup default as needed.
    if (argc == 0)
    {
        const char* sDefDir[] = {"*"};
        // argc  = sizeof(sDefDir)/sizeof(sDefDir[0]);
        pDirs = sDefDir;
    }

    lstring inText;
    char obuf[4096] = "";

    // Parse options
    while (*cmdOpts)
    {
        if (*cmdOpts)
            VerboseMsg() << "Cmd:" << cmdOpts << std::endl;

        switch (*cmdOpts)
        {
    // ---- Common command options -----
        case 'I':   // -I=<inFileList>
            cmdOpts = LLSup::ParseString(cmdOpts+1, m_inFile, missingInFileMsg);
            FillListFile(inList, m_inFile.c_str(), sep, m_pushArgs);
            break;

    // ---- Local command options -----
        // File arguments passed to printf, -a=[pdnres_luc]...
        //   p=full path
        //   d=directory
        //   n=name
        //   r=name root (no extension)
        //   e=extension (right of last .)
        //   s=size (as a string)
        //   _=defcase
        //   l=lowercase
        //   u=uppercase
        //   c=captialize
        //  NOTE:  -a must proceed -i or -I
        case 'a':
            cmdOpts = LLSup::ParseString(cmdOpts+1, m_pushArgs, argOptMsg);
            break;

        case 'e':   // -e=<envName> or -e=* or -e
            cmdOpts = LLSup::ParseString(cmdOpts+1, str, NULL);
            if (str.empty() || str[0] == '*')
            {
                const char* pEnvList = GetEnvironmentStrings();
                if (pEnvList)
                while (*pEnvList)
                {
                    std::string envItem = pEnvList;
                    if (Count(envItem, '=') == 1 &&
						(m_grepSrcPathPat.flags() == 0 ||
                        std::tr1::regex_search(envItem.begin(), envItem.end(), m_grepSrcPathPat)))
                        inList.push(envItem);
                    pEnvList += envItem.length() + 1;
                }
            }
            else
            {
                m_env += LLSup::GetEnvStr(str.c_str(), "");
            }
            VerboseMsg() << "-E=" << str << std::endl;
            break;

        case 'C':
            // Parse color for foreground and background
            //  -C=<color>[b][+<color>[b]>]...
            // Ex:
            //  -C=red+blue        = red and blue foreground
            //  -C=green+redb      = green foreground, red background
            //
            if (cmdOpts[1] == sEQchr)
            {
                cmdOpts++;
                WORD colorFB;
                if (LLSup::SetColor(colorFB, cmdOpts+1))
                    SetColor(colorFB);

                // Skip over this color setting.
                while(cmdOpts[0] > sEOCchr)
                    cmdOpts++;
            }
            else
                SetColor(m_defColor);
            break;


        case 'i':   // inline text, -i=<text>
            cmdOpts = LLSup::ParseString(cmdOpts+1, inText, intextOptMsg);
            if (inText.length() != 0)
                FillListStr(inList, inText, sep, m_pushArgs);
            inText.clear();
            break;

        case 's':   // scan using regex
            // set -p to define printf before -I which specifies the input followed by scan using regex\n"
            // dir | p \"-p=%.0syear=%s mon=%s day=%s\\n\" -I=- \"-s=([0-9]*)/([0-9]*)/([0-9]*).*\"  \n"
            cmdOpts = LLSup::ParseString(cmdOpts+1, str, scanOptMsg);
            if (str.length() != 0)
            {
                m_regex = std::tr1::regex(str);

                if (inText.length() != 0)
                {
                    FillListStr(inList, inText, sep, m_pushArgs);
                    inText.clear();
                }

                // Only scan while we have input.
                if (inList.size() && m_printFmt != 0)
                {
                    while (inList.size() > 0)
                    {
                        std::string s = inList.front();
                        inList.pop();

                        // object that will contain the sequence of sub-matches
                        std::tr1::match_results<std::string::const_iterator> result;
                        if (std::tr1::regex_match(s, result, m_regex))
                        {
                            StringQueue matchResults;
                            for (unsigned rIdx = 0; rIdx < result.size(); ++rIdx)
                            {
                                matchResults.push(result[rIdx]);
                            }

							while (!matchResults.empty())
								PrintFormatted(m_printFmt, matchResults, ++nLine);
                        }
                    }

                    m_printFmt.clear();
                }
            }
            break;
        case 'p':   // print using printf
            cmdOpts = LLSup::ParseString(cmdOpts+1, m_printFmt, sMissingPrintFmtMsg);
            if (!m_printFmt.empty())
            {
                if (inText.length() != 0)
                {
                    FillListStr(inList, inText, sep, m_pushArgs);
                    inText.clear();
                }

                VerboseMsg() << "printFmt=" << m_printFmt << std::endl;
                if (strchr(m_printFmt, '%') == NULL)
                {
                    // If no printf arguments required, print now
					while (!inList.empty())
						PrintFormatted(m_printFmt, inList, ++nLine);
                    m_printFmt.clear();
                }
                else
                {
                    // Only print while we have input.
                    if (inList.size())
                    {
						SlitOnSeparators(inList, m_separators);
                        while (!inList.empty())
                            PrintFormatted(m_printFmt, inList, ++nLine);
                        m_printFmt.clear();
                    }
                }
            }
            break;

        case 'z':   // gmt (z) time
            m_gmt = true;
            gmtime_s(&today, &now);
        case 'l':   // local time
        case 't':
            cmdOpts = LLSup::ParseString(cmdOpts+1, m_timeFmt, sMissingTimeMsg);
            if (!m_timeFmt.empty())
            {
                _invalid_parameter_handler oldHandler;
                oldHandler = _set_invalid_parameter_handler(InvalidParameterHandler);

                if (strftime(obuf, ARRAYSIZE(obuf), m_timeFmt, &today) > 0)
                    LLMsg::Out() <<  obuf;

                _set_invalid_parameter_handler(oldHandler);

                localtime_s(&today, &now);
            }
            break;

        case '?':
            Colorize(std::cout, sHelp);
            return sIgnore;

		case '1':	// -1=outFile.txt
		case '3':	// -3=outFile.txt
			{
				std::string outFile;
				cmdOpts = LLSup::ParseString(cmdOpts+1, outFile, missingInFileMsg);
				if (outFile.length() != 0)
				{
					// Open in Binary mode to allow control over line termination
					//  p -I=unix.txt -1=dos.txt  -p="%s\r\n" 
					//  p -I=dos.txt  -1=unix.txt -p="%s\n"
					if (outFile == "-")
						_setmode( _fileno( stdout ), _O_BINARY );
					else
					{
						if (NULL == freopen(outFile.c_str(), "wb", stdout))
							perror(outFile.c_str());
					}
				}
			}
			break;

        default:
            if ( !ParseBaseCmds(cmdOpts))
                return sError;
        }

        // Advance to next parameter
        LLSup::AdvCmd(cmdOpts);
    }

    if ( !m_env.empty())
    {
        if (argc == 0 && inList.size() == 0)
        {
            Split envDirs(m_env, ";");
            for (uint envIdx = 0; envIdx != envDirs.size(); envIdx++)
            {
                inList.push(envDirs[envIdx]);
            }
        }
        else
        {
            VerboseMsg() << " EnvDir:" << m_env << "\n";
            Split envDirs(m_env, ";");
            for (uint envIdx = 0; envIdx != envDirs.size(); envIdx++)
            {
                lstring dirPath = LLPath::Join(envDirs[envIdx], pDirs[0]);
                VerboseMsg() << "  ScanDir:" << dirPath << std::endl;
                m_dirScan.Init(dirPath, "");
                nFiles += m_dirScan.GetFilesInDirectory();
            }
        }
        m_env.clear();
    }
    else
    {
        // Iterate over dir patterns.
        for (int argn=0; argn < argc; argn++)
        {
            VerboseMsg() << argn << ": Dir:" << pDirs[argn] << std::endl;
            m_dirScan.Init(pDirs[argn], NULL);
            nFiles += m_dirScan.GetFilesInDirectory();
        }
    }

    while (inList.size() > 0)
    {
        StringQueue matchResults;
        std::string listItem = inList.front();
        inList.pop();

        if (m_grepSrcPathPat.flags() == 0 ||
            std::tr1::regex_search(listItem.begin(), listItem.end(), m_grepSrcPathPat))
        {
			matchResults.push(listItem);
			SlitOnSeparators(matchResults, m_separators);
			while (!matchResults.empty())
				PrintFormatted(m_printFmt, matchResults, ++nLine);
        }
    }

    // Restore original color.
    SetColor(m_defColor);
	return ExitStatus((int)nFiles);
}

// ---------------------------------------------------------------------------
int LLPrintf::ProcessEntry(
        const char* pDir,
        const WIN32_FIND_DATA* pFileData,
        int depth)      // 0...n is directory depth, -n end-of nth diretory
{
    int retStatus = sIgnore;

    if (depth < 0)
        return sIgnore;     // ignore end-of-directory

    //  Filter on:
    //      m_onlyAttr      File or Directory, -F or -D
    //      m_onlyRhs       Attributes, -A=rhs
    //      m_includeList   File patterns,  -F=<filePat>[,<filePat>]...
    //      m_onlySize      File size, -Z op=(Greater|Less|Equal) value=num<units G|M|K>, ex -Zg100M
    //      m_excludeList   Exclude path patterns, -X=<pathPat>[,<pathPat>]...
    //      m_timeOp        Time, -T[acm]<op><value>  ; Test Time a=access, c=creation, m=modified\n
    //
    //  If pass, populate m_srcPath
    if ( !FilterDir(pDir, pFileData, depth))
        return sIgnore;

    VerboseMsg() << m_srcPath << std::endl;

    if (m_isDir)
    {
        if (PatternMatch(m_dirScan.m_fileFilter, pFileData->cFileName) == false)
            return sIgnore;
    }

    if (!m_printFmt.empty() && !IsQuit())
    {
        retStatus = sOkay;

        const char* lastDot = strchr(pFileData->cFileName, '.');
        StringQueue inList;
        char buf[256];
        const char* pPushArgs = m_pushArgs;
        CaseFold caseFold = eDefCase;

        while (*pPushArgs != '\0')
        {
            //  \directory\root.ext
            //
            //  d = directory
            //  n = name (root.ext)
            //  r = root
            //  e = extension
            //  p = directory and name
            //  s = size
            //  l = lowercase
            //  u = uppercase
            //  c = capitalize
            //  _ = default case
            // Todo:
            //  x = creation date
            //  i = link status
            //
            switch (*pPushArgs)
            {
            case '_':
                caseFold = eDefCase;
                break;
            case 'l':
                caseFold = eLowerCase;
                break;
            case 'u':
                caseFold = eUpperCase;
                break;
            case 'c':
                caseFold = eCapitalize;
                break;

            case 'd':   // directory
                inList.push(ChangeCase(pDir, caseFold));
                break;
            case 'n':   // name
                inList.push(ChangeCase(pFileData->cFileName, caseFold));
                break;
            case 'r':   // root
                inList.push(ChangeCase(pFileData->cFileName, caseFold));
                if (lastDot)
                    inList.back().resize(inList.back().find_last_of('.'));
                break;
            case 'e':   // extension
                if (lastDot)
                    inList.push(ChangeCase(lastDot+1, caseFold));
                else
                    inList.push("");
                break;
            default:
            case 'p':   // full path
                inList.push(ChangeCase(m_srcPath, caseFold));
                break;
            case 's':
                _snprintf(buf, ARRAYSIZE(buf), "%d", pFileData->nFileSizeLow);
                inList.push(ChangeCase(buf, caseFold));
                break;
            }
            ++pPushArgs;
        }

		while (inList.size() != 0)
			PrintFormatted(m_printFmt, inList, m_countOut);
    }

    if (retStatus == sOkay)
    {
        if (m_isDir)
            m_countOutDir++;
        else
            m_countOutFiles++;
    }

    return retStatus;
}

