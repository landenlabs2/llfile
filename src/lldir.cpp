//-----------------------------------------------------------------------------
// lldir - Display directory information from provided by DirectoryScan object
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

//
//  TODO:
//      display compressed file size
//      display  Account Control Lists ACL  (cacls.exe)
//          GetFileSecurity()
//              Article on aclinfo sample program.
//              http://www.devx.com/cplus/Article/16711/1954
//


#include <iostream>
#include <iomanip>
#include <map>
#include <assert.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>

#include "lldir.h"
#include "comma.h"


// reparse point
#define NOMCX
#define NOIME
#define NOSERVICE
#define NOGDI
#define NOUSER
#define NOHELP
#define NOSYSPARAMSINFO
#define NOWINABLE

#include <windows.h>
#include <winioctl.h>
#include <aclapi.h>
#include <authz.h>

// ---------------------------------------------------------------------------

static const char sHelp[] =
" List Directory " LLVERSION "\n"
"\n"
"  !0eSyntax:!0f\n"
"    <filePattern> \n"
"    [<directory|pattern> \\ ]... <filePattern> \n"
"\n"
"  !0ePattern:!0f\n"
"      * = zero or more characters\n"
"      ? = any character\n"
"\n"
"  !0eExample:!0f\n"
"      *                ; List all filse in current directory\n"
"      *.obj            ; List only files ending in .obj in current directory\n"
"    -R c:\\tmp\\*.obj    ; List all .obj files in and under c:\\tmp directory\n"
"   c:\\t*\\src\\*         ; List all files off directory starting with t\n"
"                       ;  and having subdirectory src\n"
"\n"
"   -a                  ; Show attributes  (-aw  show as words, -as show security)\n"
"   -cw or -cr          ; Change permission to writeable or readonly\n"
"   -ct                 ; Change time (modify)\n"

"   -h or -H            ; Hide heading\n"
"   -l                  ; Long format  <attributes> <size> <modify time> <file name>\n"
"   -r or -R            ; Recurse directories\n"
"   -s                  ; Hide file size size\n"
"   -u                  ; Show disk usage summary (size, count), use instead of -r\n"
"   -U                  ; Show disk usage summary (size, count) by extension\n"
"   -UU or -UUU         ; Show disk usage by Name (-UU) or Directory (-UUU) \n"
"   -w                  ; Show in wide format\n"
"   -A=[nrhs]           ; Limit files by attribute (n=normal r=readonly, h=hidden, s=system)\n"
"   -D                  ; Show only directories\n"
"   -F                  ; Show only files\n"
"   -i                  ; Show fileId (use with -L)\n"
"   -L                  ; Show hard link count and any Alternate Data Streams\n"
"   -N or -n            ; Show just names, same as -h -s -tn -q\n"
"   -p                  ; Show full file path\n"
"   -P=<srcPathPat>     ; Optional regular expression pattern on source files full path\n"
"   -q                  ; Quiet, dont show stats, no color\n"
"   -Q=n                ; Quit after 'n' lines output\n"
"   -S=[-][acmnenpst]   ; Sort on a=access,c=creation,m=modify time, e=ext, n=name, p=path, s=size or t=type. -=reverse\n"
"   -t=[acm]            ; Show Time a=access, c=creation, m=modified, n=none\n"
"   -T=[acm]<op><value> ; Test Time a=access, c=creation, m=modified\n"
"                       ; op=(Greater|Less|Equal)  Value= now|+/-num[d|h|m]|yyyy:mm:dd:hh:mm:ss \n"
"                       ; Value time parts significant from right to left, so mm:ss or hh:mm:ss, etc\n"
"                       ;  Example   -TmcEnow    Show if modify or create time Equal to now\n"
"                       ;            -TmG-4.5h   Show if modify time Greater than 4.5 hours ago\n"
"                       ;                        +/- values followed by d=days, h=hours or m=minutes\n"
"                       ;            -TaL04:00:00   Show if access time Less than 4am today\n"
"   -V=<pathPat>,...    ; List directory if no matching files exist \n"
"   -W=[admr]           ; Watch directories for any change, options limit wach to\n"
"                       ;  a=Add, d=Delete, m=Modified, r=renamed, default is any change\n"
"   -X=<pathPat>,...    ; Exclude patterns  -X=*.lib,*.obj,*.exe\n"
"                       ;  No space in patterns. Pattern applied against fullpath\n"
"                       ;  So *\\ma will exclude a directory ma or file ma \n"
"   -Z<op><value>       ; siZe op=(Greater|Less|Equal) value=num<units G|M|K>, ex -Zg100M \n"
"   -C=<colorOpt>       ; Set colors, colors are red,green,blue,intensity, add bg to end of color for background\n"
"    -C=r<colors>       ;   readonly  ex -C=r+red+blue or -C=r+bluebg\n"
"    -C=d<colors>       ;   directory\n"
"    -C=h<colors>       ;   hidden file or directory\n"
"   -,                  ; Disable commas in size and numeric output\n"
"   -=<sep>             ; Set output field separator string\n"
"   -#                  ; Change date format show milliseconds \n"
"   -~                  ; Change date foramt from relative to absolute \n"
"   -?                  ; Show this help\n"
"   -??                 ; Show extended help\n"
"   -I=<infFileList>    ; -I and -0 same, -I=- for stdin \n"
"   -0=<inFileList>     ; File contains list of files to list \n"
"   -1=<outFile>        ; Redirect output to file \n"
"   -3=<outFile>        ; Tee output to file \n"
"\n";

static const char sMoreHelp[] =
" Attributes\n"
" Long form             Single Char      \n"
" Left-To-Right         Last match wins  \n"
"----------------       ---------------- \n"
" READONLY      R        NORMAL          \n"
" HIDDEN        H        NOT_INDEXED   I \n"
" SYSTEM        S        ARCHIVE         \n"
" DIRECTORY     D        READONLY      r \n"
" ARCHIVE       A        HIDDEN        h \n"
" DEVICE        d        SYSTEM        s \n"
" NORMAL        N        DIRECTORY     \\ \n"
" TEMPORARY     T        DEVICE        d \n"
" SPARSE_FILE   S        TEMPORARY     t \n"
" REPARSE_POINT r        SPARSE_FILE   S \n"
" COMPRESSED    C        REPARSE_POINT > \n"
" OFFLINE       O        COMPRESSED    % \n"
" NOT_INDEXED   I        OFFLINE       O \n"
" ENCRYPTED     E        ENCRYPTED     E \n"
" VIRTUAL       V        VIRTUAL       V \n"
"\n"
" Set defaults by assigning them to the LLDIR environment variable\n"
" Use pipe to divide options instead of minus and end with pipe.\n"
" Ex:  (you must quote string since pipe is a shell character)\n"
"     ; Set hidden file color to pink\n"
"     set \"lldir=Ch+red+blue|\" \n"
"     ; Don't show size\n"
"     set \"lldir=s|\" \n"
"\n";


LLDirConfig LLDir::sConfig;
LONG LLDir::sWatchCount = 0;
LONG LLDir::sWatchLimit = 0;


struct Usage
{
    size_t      m_fileCount;
    size_t      m_dirCount;
    LONGLONG    m_totalSize;
    LONGLONG    m_minFileSize;
    LONGLONG    m_maxFileSize;

    typedef std::map<std::string, Usage> ExtUsage;
    ExtUsage extUsage;

    Usage() : m_fileCount(0), m_dirCount(0), m_totalSize(0),
        m_minFileSize(MAXLONGLONG), m_maxFileSize(0)
    {
    }
    void Clear()
    {
        m_fileCount = m_dirCount = 0;
        m_totalSize  = m_maxFileSize = 0;
        m_minFileSize = MAXLONGLONG;
        extUsage.clear();
    }

    void Add(LONGLONG fileSize)
    {
        m_fileCount++;
        m_totalSize += fileSize;
        m_minFileSize = min(fileSize, m_minFileSize);
        m_maxFileSize = max(fileSize, m_maxFileSize);
    }

    void Add(LONGLONG fileSize, const char* ext)
    {
        Add(fileSize);
        if (ext == NULL)
            ext = "";
        extUsage[ext].Add(fileSize);
    }

    void Add(const Usage& other)
    {
        m_dirCount += other.m_dirCount;
        m_fileCount += other.m_fileCount;
        m_totalSize += other.m_totalSize;
        m_minFileSize = min(m_minFileSize, other.m_minFileSize);
        m_maxFileSize = max(m_maxFileSize, other.m_maxFileSize);

        for (ExtUsage::const_iterator iter = other.extUsage.begin();
            iter != other.extUsage.end();
            ++iter)
        {
            extUsage[iter->first].Add(iter->second);
        }
        // sDepthUsage[depth].Clear();
    }
};

std::vector<Usage> sDepthUsage;

// ---------------------------------------------------------------------------
static FILETIME operator-(const FILETIME& leftFt, const FILETIME& rightFt)
{
    FILETIME fTime;
    ULARGE_INTEGER v_ui;

    v_ui.LowPart = leftFt.dwLowDateTime;
    v_ui.HighPart = leftFt.dwHighDateTime;
    __int64 v_left = v_ui.QuadPart;

    v_ui.LowPart = rightFt.dwLowDateTime;
    v_ui.HighPart = rightFt.dwHighDateTime;
    __int64 v_right = v_ui.QuadPart;

    __int64 v_res = v_left - v_right;

    v_ui.QuadPart = v_res;
    fTime.dwLowDateTime = v_ui.LowPart;
    fTime.dwHighDateTime = v_ui.HighPart;
    return fTime;
}

// ---------------------------------------------------------------------------
// returns a UNIX-like timestamp (in seconds since 1970) with sub-second resolution.
static double time_d(const FILETIME& ft)
{
	// GetSystemTimeAsFileTime(&ft);
	const __int64* val = (const __int64*)&ft;
	return static_cast<double>(*val) / 10000000.0 - 11644473600.0;   // epoch is Jan. 1, 1601: 134774 days to Jan. 1, 1970
}

// ---------------------------------------------------------------------------
static std::ostream& Format(std::ostream& out, const FILETIME& utcFT)
{
    FILETIME   locTmFT;
    SYSTEMTIME sysTime;


	if (LLDir::sConfig.m_refDateTime)
	{
		SYSTEMTIME nowStUtc;
		GetSystemTime(&nowStUtc);
		FILETIME nowFtUtc;
		SystemTimeToFileTime(&nowStUtc, &nowFtUtc);

		FILETIME deltaFt = nowFtUtc - utcFT;
		__int64* delta64 = (__int64*)&deltaFt;
		double seconds = static_cast<double>(*delta64) / 10000000.0;
		double minutes = seconds / 60.0;
		double hours = minutes / 60.0;
		double days = hours / 24.0;
		double years = days / 365.25;
		const int width = 8;
		if (years >= 1.0)
			out << std::fixed << std::setw(width) << std::setprecision(1) << years   << " years ";
		else if (days >= 1.0)
			out << std::fixed << std::setw(width) << std::setprecision(1) << days    << " days  ";
		else if (hours >= 1.0)
			out << std::fixed << std::setw(width) << std::setprecision(1) << hours   << " hours ";
		else if (minutes >= 1.0)
			out << std::fixed << std::setw(width) << std::setprecision(1) << minutes << " mins  ";
		else 
			out << std::fixed << std::setw(width) << std::setprecision(1) << seconds << " secs  ";

		return out;
	}

    if (LLDir::sConfig.m_fileTimeLocalTZ)
    {
        // Convert UTC to local Timezone
        FileTimeToLocalFileTime(&utcFT, &locTmFT);
        FileTimeToSystemTime(&locTmFT, &sysTime);
    }
    else
    {
        FileTimeToSystemTime(&utcFT, &sysTime);
    }
    
	// "yy/mm/dd hh:mm:ss.sss "
    if (LLDir::sConfig.m_numDateTime)
    {
        out << std::setfill(LLDir::sConfig.m_fillNumDT)
            << std::setw(2)
            << sysTime.wYear%100
            << LLDir::sConfig.m_dateSep

            << std::setfill(LLDir::sConfig.m_fillNumDT)
            << std::setw(2)
            << sysTime.wMonth
            << LLDir::sConfig.m_dateSep

            << std::setfill(LLDir::sConfig.m_fillNumDT)
            << std::setw(2)
            << sysTime.wDay
            << LLDir::sConfig.m_dateTimeSep

            << std::setfill(LLDir::sConfig.m_fillNumDT)
            << std::setw(2)
            << sysTime.wHour
            << LLDir::sConfig.m_timeSep

            << std::setfill(LLDir::sConfig.m_fillNumDT)
            << std::setw(2)
            << sysTime.wMinute
            << LLDir::sConfig.m_timeSep

            << std::setfill(LLDir::sConfig.m_fillNumDT)
            << std::setw(2)
            << sysTime.wSecond

            << "."
            << std::setfill(LLDir::sConfig.m_fillNumDT)
            << std::setw(3)
            << sysTime.wMilliseconds

            << std::setfill(' ');
    }
    else
    {
        LLSup::Format(out, utcFT, LLDir::sConfig.m_fileTimeLocalTZ);
    }

    return out;
}

// ---------------------------------------------------------------------------
std::ostream& LLDir::ShowAttributes(
        std::ostream& out,
        const char * pDir,
        const WIN32_FIND_DATA& fileData,
        bool showWords)
{
    DWORD fileAttribute = fileData.dwFileAttributes;

    if (showWords)
    {
        const char* pWord = NULL;
        if ((fileAttribute & FILE_ATTRIBUTE_NOT_CONTENT_INDEXED)!=0)
            pWord = "NotIndexed";
        if ((fileAttribute & FILE_ATTRIBUTE_ARCHIVE      )!=0)
            pWord = "Archive";

        if ((fileAttribute & FILE_ATTRIBUTE_READONLY     )!=0)
            pWord = "ReadOnly";
        if ((fileAttribute & FILE_ATTRIBUTE_HIDDEN       )!=0)
            pWord = "Hidden";
        if ((fileAttribute & FILE_ATTRIBUTE_SYSTEM       )!=0)
            pWord = "System";
        if ((fileAttribute & FILE_ATTRIBUTE_DIRECTORY    )!=0)
            pWord = "Directory";

        if ((fileAttribute & FILE_ATTRIBUTE_DEVICE       )!=0)
            pWord = "Device";
        if ((fileAttribute & FILE_ATTRIBUTE_NORMAL       )!=0)
            pWord = "Normal";
        if ((fileAttribute & FILE_ATTRIBUTE_TEMPORARY    )!=0)
            pWord = "Temporary";
        if ((fileAttribute & FILE_ATTRIBUTE_SPARSE_FILE  )!=0)
            pWord = "Sparse";

        if ((fileAttribute & FILE_ATTRIBUTE_COMPRESSED   )!=0)
            pWord = "Compressed";
        if ((fileAttribute & FILE_ATTRIBUTE_OFFLINE      )!=0)
            pWord = "OffLine";

        if ((fileAttribute & FILE_ATTRIBUTE_ENCRYPTED    )!=0)
            pWord = "Encrypted";
        if ((fileAttribute & FILE_ATTRIBUTE_VIRTUAL      )!=0)
            pWord = "Virtual";

        char wordPad[16];
        strcpy_s(wordPad, ARRAYSIZE(wordPad), pWord);
        wordPad[15] = '\0';
        PadRight(wordPad, ARRAYSIZE(wordPad), 15, ' ');
        out << wordPad;
    }
    else
    {
        char XX = LLDir::sConfig.m_attrFiller;
        char* attrChr = LLDir::sConfig.m_attrChr;
        //                012345678901234
        char attrOut[] = "RHSDAdNTSrCOIEV";
        char* pA = attrOut;

        *pA++ = (((fileAttribute & FILE_ATTRIBUTE_READONLY     )!=0) ? attrChr[ 0]:XX);    // R
        *pA++ = (((fileAttribute & FILE_ATTRIBUTE_HIDDEN       )!=0) ? attrChr[ 1]:XX);    // H
        *pA++ = (((fileAttribute & FILE_ATTRIBUTE_SYSTEM       )!=0) ? attrChr[ 2]:XX);    // S
        *pA++ = (((fileAttribute & FILE_ATTRIBUTE_DIRECTORY    )!=0) ? attrChr[ 3]:XX);    // D
        *pA++ = (((fileAttribute & FILE_ATTRIBUTE_ARCHIVE      )!=0) ? attrChr[ 4]:XX);    // A
        *pA++ = (((fileAttribute & FILE_ATTRIBUTE_DEVICE       )!=0) ? attrChr[ 5]:XX);    // d
        *pA++ = (((fileAttribute & FILE_ATTRIBUTE_NORMAL       )!=0) ? attrChr[ 6]:XX);    // N
        *pA++ = (((fileAttribute & FILE_ATTRIBUTE_TEMPORARY    )!=0) ? attrChr[ 7]:XX);    // T
        *pA++ = (((fileAttribute & FILE_ATTRIBUTE_SPARSE_FILE  )!=0) ? attrChr[ 8]:XX);    // S
        *pA++ = (((fileAttribute & FILE_ATTRIBUTE_REPARSE_POINT)!=0) ? attrChr[ 9]:XX);    // r
        *pA++ = (((fileAttribute & FILE_ATTRIBUTE_COMPRESSED   )!=0) ? attrChr[ 10]:XX);   // C
        *pA++ = (((fileAttribute & FILE_ATTRIBUTE_OFFLINE      )!=0) ? attrChr[11]:XX);    // O
        *pA++ = (((fileAttribute & FILE_ATTRIBUTE_NOT_CONTENT_INDEXED)!=0) ? attrChr[12]:XX);  // I
        *pA++ = (((fileAttribute & FILE_ATTRIBUTE_ENCRYPTED    )!=0) ? attrChr[13]:XX);    // E
        *pA++ = (((fileAttribute & FILE_ATTRIBUTE_VIRTUAL      )!=0) ? attrChr[14]:XX);    // V
        out << attrOut;
    }

    return out;
}

// ---------------------------------------------------------------------------
std::ostream& LLDir::ShowAttributeAsChar(
        std::ostream& out,
        const char * pDir,
        const WIN32_FIND_DATA& fileData,
        bool showWords)
{
    DWORD fileAttribute = fileData.dwFileAttributes;
    //               "RHSD AdNTSrCOIEV";
    char attrChr[] = "rhs\\ d tS>%OIEV";
    char attrOut = ' ';

    if ((fileAttribute & FILE_ATTRIBUTE_NORMAL       )!=0)    attrOut = attrChr[ 6]; //
    if ((fileAttribute & FILE_ATTRIBUTE_NOT_CONTENT_INDEXED)!=0) attrOut = attrChr[12];   // I
    if ((fileAttribute & FILE_ATTRIBUTE_ARCHIVE      )!=0)    attrOut = attrChr[ 4]; //

    if ((fileAttribute & FILE_ATTRIBUTE_READONLY     )!=0)    attrOut = attrChr[ 0]; // r
    if ((fileAttribute & FILE_ATTRIBUTE_HIDDEN       )!=0)    attrOut = attrChr[ 1]; // h
    if ((fileAttribute & FILE_ATTRIBUTE_SYSTEM       )!=0)    attrOut = attrChr[ 2]; // s
    if ((fileAttribute & FILE_ATTRIBUTE_DIRECTORY    )!=0)    attrOut = attrChr[ 3]; // backslash
    if ((fileAttribute & FILE_ATTRIBUTE_DEVICE       )!=0)    attrOut = attrChr[ 5]; // d
    if ((fileAttribute & FILE_ATTRIBUTE_TEMPORARY    )!=0)    attrOut = attrChr[ 7]; // t
    if ((fileAttribute & FILE_ATTRIBUTE_SPARSE_FILE  )!=0)    attrOut = attrChr[ 8]; // S
    if ((fileAttribute & FILE_ATTRIBUTE_REPARSE_POINT)!=0)    attrOut = attrChr[ 9]; // >
    if ((fileAttribute & FILE_ATTRIBUTE_COMPRESSED   )!=0)    attrOut = attrChr[10]; // %
    if ((fileAttribute & FILE_ATTRIBUTE_OFFLINE      )!=0)    attrOut = attrChr[11]; // O
    if ((fileAttribute & FILE_ATTRIBUTE_ENCRYPTED    )!=0)    attrOut = attrChr[13]; // E
    if ((fileAttribute & FILE_ATTRIBUTE_VIRTUAL      )!=0)    attrOut = attrChr[14]; // V

    out << " " << attrOut << " ";

    return out;
}

// ---------------------------------------------------------------------------
// Return true if file is a  Soft links (reparse points), and set
// outLinkPath to the link target.

bool LLDir::GetSoftLink(
        std::string& outLinkPath,
        const char * pDir,
        const WIN32_FIND_DATA& fileData)
{
    bool isLink = false;
    DWORD fileAttribute = fileData.dwFileAttributes;

    if ((fileAttribute & FILE_ATTRIBUTE_REPARSE_POINT)!=0)
    {
        if (IsReparseTagMicrosoft(fileData.dwReserved0) != 0)
        {
            char fullFile[MAX_PATH];
            strcpy_s(fullFile, ARRAYSIZE(fullFile), pDir);
            strcat_s(fullFile, ARRAYSIZE(fullFile), "\\");
            strcat_s(fullFile, ARRAYSIZE(fullFile), fileData.cFileName);

            Handle reparseHnd =
                CreateFile(fullFile, GENERIC_READ, FILE_SHARE_READ, NULL,
                OPEN_EXISTING,
                FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
                NULL);

            if (reparseHnd.IsValid())
            {
                struct TMN_REPARSE_DATA_BUFFER
                {
                    DWORD  ReparseTag;
                    WORD   ReparseDataLength;
                    WORD   Reserved;

                    // IO_REPARSE_TAG_MOUNT_POINT specifics follow
                    WORD   SubstituteNameOffset;
                    WORD   SubstituteNameLength;
                    WORD   PrintNameOffset;
                    WORD   PrintNameLength;
                    DWORD  Flags;
                    wchar_t PathBuffer[1];
                };

                char reparseDataBuffer[MAXIMUM_REPARSE_DATA_BUFFER_SIZE];
                TMN_REPARSE_DATA_BUFFER& rdb = *(TMN_REPARSE_DATA_BUFFER*)reparseDataBuffer;
                DWORD dwBytesReturned;

                if (DeviceIoControl(
                    reparseHnd,
                    FSCTL_GET_REPARSE_POINT,            // dwIoControlCode
                    NULL,                               // lpInBuffer
                    0,                                  // nInBufferSize
                    (LPVOID) &rdb,                      // output buffer
                    MAXIMUM_REPARSE_DATA_BUFFER_SIZE,   // size of output buffer
                    (LPDWORD) &dwBytesReturned,         // number of bytes returned
                    0))                                 // OVERLAPPED structure
                {
#if defined(_UNICODE)
                    std::string filenm;
                    filenm.append(rdb.PathBuffer, rdb.PrintNameLength / sizeof(char ));
                    out << " -> " << filenm;
                    isLink = true;
#else
#if 0
                    char filenm[MAX_PATH];
                    int outLen = WideCharToMultiByte(CP_THREAD_ACP,
                         0,
                         rdb.PathBuffer,
                         rdb.SubstituteNameLength / sizeof(wchar_t),
                         filenm,
                         sizeof(filenm),
                         "",
                         FALSE);
                    if (outLen > 0)
                    {
                        filenm[outLen] = '\0';
                        out << " -> " << filenm;
                        isLink = true;
                    }
#else
                    if (rdb.PathBuffer[rdb.SubstituteNameOffset/sizeof(wchar_t)] == '?')
                    {
                        // skip over \?
                        rdb.SubstituteNameOffset += sizeof(wchar_t)*2;
                        rdb.SubstituteNameLength -= sizeof(wchar_t)*2;
                    }
                    LLSup::ToNarrow(
                        &rdb.PathBuffer[rdb.SubstituteNameOffset/sizeof(wchar_t)],
                        rdb.SubstituteNameLength/sizeof(wchar_t), outLinkPath);
                    isLink = true;
#endif
#endif
                }
                // CloseHandle(reparseHnd);
            }
        }
    }

    return isLink;
}

// ---------------------------------------------------------------------------

bool LLDir::ShowAlternateDataStreams(
        const char * pDir,
        const WIN32_FIND_DATA& fileData,
        int depth)
{
    // DWORD fileAttribute = fileData.dwFileAttributes;

    char fullFile[MAX_PATH];
    strcpy_s(fullFile, ARRAYSIZE(fullFile), pDir);
    strcat_s(fullFile, ARRAYSIZE(fullFile), "\\");
    strcat_s(fullFile, ARRAYSIZE(fullFile), fileData.cFileName);

    Handle hFile =
        CreateFile(fullFile, GENERIC_READ, FILE_SHARE_READ, NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        NULL);

    if (hFile.NotValid())
    {
        return false;
    }

    WIN32_STREAM_ID     streamId;
    const int           streamIdSize = 20;
    LPVOID                pContext = NULL;

    DWORD dwBytesRead;
    BOOL bResult = TRUE;

    ShowStream(false);

    WIN32_FIND_DATA& modFileData = (WIN32_FIND_DATA&)fileData;
    modFileData.dwFileAttributes |= FILE_ATTRIBUTE_HIDDEN;
    uint filenameLen = strlen(fileData.cFileName);

    while (bResult)
    {
        // Read the header part
        if ( !BackupRead(hFile,
                (BYTE*)&streamId,
                streamIdSize,
                &dwBytesRead,
                FALSE, // am I done?
                FALSE,
                &pContext ) )
        {
            bResult = false;
        }
        else
        {
            if (dwBytesRead == 0)
            {
                bResult = false;
            }
            else
            {
                // saving the header info
                DWORD dwNameSize = streamId.dwStreamNameSize;
                LARGE_INTEGER liDataSize = streamId.Size;

                if (dwNameSize > 0 )
                {
                    // read the stream name
                    wchar_t   strStreamName[MAX_PATH]; //  = new char [dwNameSize + sizeof(char )];

                    if (dwNameSize > sizeof(strStreamName) ||
                        !BackupRead(hFile,
                                (BYTE*)strStreamName,
                                dwNameSize,
                                &dwBytesRead,
                                FALSE, // am I done?
                                FALSE,
                                &pContext ) )
                    {
                        bResult = FALSE;
                    }

                    modFileData.nFileSizeHigh = liDataSize.HighPart;
                    modFileData.nFileSizeLow = liDataSize.LowPart;
					std::string narrowStr;
                    strncpy(modFileData.cFileName + filenameLen, 
						LLSup::ToNarrow(strStreamName, dwBytesRead/2,narrowStr).c_str(),
                        sizeof(fileData.cFileName) - filenameLen);

                    char* metaPtr = strstr(modFileData.cFileName, ":$");
                    if (metaPtr != NULL && !m_verbose)
                        *metaPtr = '\0';
                    EntryCb(this, pDir, &modFileData, depth);
                }

                // Read the stream data, first check if any data is present or not
                if (liDataSize.QuadPart != 0)
                {
                    LARGE_INTEGER seeked;

                    bResult = BackupSeek(hFile,
                            liDataSize.LowPart,
                            liDataSize.HighPart,
                            &seeked.LowPart,
                            (DWORD*)&seeked.HighPart,
                            &pContext);

                    if (seeked.QuadPart > 0)
                    {
                        bResult = true;
                    }
                }
            }
        }
    } // while

    BackupRead(    hFile,
                NULL,
                0,
                &dwBytesRead,
                TRUE, // am I done?
                FALSE,
                &pContext );

    // CloseHandle(hFile);
    ShowStream(true);

    return true;
}

// ---------------------------------------------------------------------------
LLDir::LLDir()
{
    // Default everything off
    m_showAttr  =
        m_showCtime =
        m_showMtime =
        m_showAtime =
        m_showStuff =
        m_showSize  =
        m_showLink =
        m_showFileId =
        m_showAttrWords =
        m_quiet =
        m_showWide =
        m_showHeader =
        m_showUsage =
        m_showUsageExt =
        m_showUsageName =
        m_showUsageDir =
        m_watchDir =
        m_showStream =
        m_setTime =
        m_showSecurity = false;

    m_showPath  = 0;

    // Trun on defaults
    m_showHeader =
        m_showSize =
        m_showMtime = true;

    m_chmod = 0;

    m_minCTime.dwHighDateTime = m_minCTime.dwLowDateTime = sMinus1;
    m_maxATime.dwHighDateTime = m_minCTime.dwLowDateTime = 0;

    SYSTEMTIME utcTime;
    GetSystemTime(&utcTime);
    SystemTimeToFileTime(&utcTime, &m_setUtcTime);

    sConfigp = &GetConfig();

    CONSOLE_SCREEN_BUFFER_INFO csbiInfo;
    if (GetConsoleScreenBufferInfo(ConsoleHnd(), &csbiInfo))
        m_screenWidth = csbiInfo.dwMaximumWindowSize.X;
}

// ---------------------------------------------------------------------------
static int InvertEntryCb(void* cbData,  const char* pDir,  const WIN32_FIND_DATA* pFileData,
                int depth)      // 0...n is directory depth, -n end-of nth diretory
{
    LLBase* pBase= (LLBase*)cbData;
    assert(pDir != NULL);
    LLDir* pDirBase = (LLDir*)cbData;

    const int s_MaxDir = 10;
    static int s_depth = -1;
    static struct DirEntry 
    {
        bool okay;
        std::string dir;
        WIN32_FIND_DATA data;
    } s_dirEntries[s_MaxDir];


    if (s_depth == -1) 
    {
        s_depth = 0;
        s_dirEntries[0].okay = true;
        s_dirEntries[0].dir = pDir;
        s_dirEntries[0].data = *pFileData;
    }

    int result = pDirBase->FilterDir(pDir, pFileData, depth);
    if (result == sIgnore)
        return sIgnore;

    // std::cout << "d=" << depth << "/" << s_depth 
    //     << " " << ((s_depth == -1 || !s_okay[s_depth]) ? "F" : "T")
    //    << " " << (pBase->m_isDir ? "Dir" : "   ") << " " << pBase->m_srcPath << std::endl; 
    if (pBase->m_isDir && depth >= 0)
    {
        s_depth = depth + 1;
        s_dirEntries[s_depth].okay = true;
        s_dirEntries[s_depth].dir = pDir;
        s_dirEntries[s_depth].data = *pFileData;
    }
    else if (depth < 0 && s_depth >= 0)
    {
        if (s_dirEntries[s_depth].okay)
        {
            pBase->ProcessEntry(s_dirEntries[s_depth].dir.c_str(), &s_dirEntries[s_depth].data, s_depth);  
            s_dirEntries[s_depth].okay = false;
            s_depth--;
        }
    }

    if (s_depth >= 0 && !pBase->m_isDir && LLSup::PatternListMatches(pDirBase->m_invertedList, pBase->m_srcPath))
        s_dirEntries[s_depth].okay = false;

    return result;
}
        
// ---------------------------------------------------------------------------
LLConfig& LLDir::GetConfig() 
{
    return sConfig;
}

// ---------------------------------------------------------------------------
int LLDir::StaticRun(const char* cmdOpts, int argc, const char* pDirs[])
{
    LLDir lldir;
    return lldir.Run(cmdOpts, argc, pDirs);
}

// ---------------------------------------------------------------------------
int LLDir::Run(const char* cmdOpts, int argc, const char* pDirs[])
{
    const char optOutSepErrMsg[] = "Set output field separators, -=<string>";
    const char invertEmptyMsg[] = "Invalid inverted list syntax. Use -V=<pattern>[,<pattern> with no spaces\n";

    // Initialize run stats
    m_totalSize = 0;
    m_dirCount = m_fileCount = 0;
    m_minCTime.dwHighDateTime = m_minCTime.dwLowDateTime = sMinus1;
    m_maxATime.dwHighDateTime = m_maxATime.dwLowDateTime = 0;
    size_t nFiles = 0;

    // Setup default pattern as needed.
    if (argc == 0 && strstr(cmdOpts, "I=") == 0)
    {
        const char* sDefDir[] = {"*"};
        argc  = sizeof(sDefDir)/sizeof(sDefDir[0]);
        pDirs = sDefDir;
    }

    bool sortNeedAllData = m_showSize;
    LLDir::sConfig.m_numDateTime = false;
    LLDir::sConfig.m_refDateTime = true;
    EnableCommaCout();


    // Parse options
    while (*cmdOpts)
    {
        switch (*cmdOpts)
        {
        case 'a':   // -a, -aw or -as   show Attributes
            sortNeedAllData |= m_showAttr = m_showStuff = !m_showAttr;
            if (cmdOpts[1] == 'w')
            {
                m_showAttrWords = true;
                cmdOpts++;
            }
            else if (cmdOpts[1] == 's')
            {
                m_showSecurity = true;
                cmdOpts++;
            }
            break;

        case 'c':   // Chmod - change permission, -cr, -cw, -ct
            if (cmdOpts[1] == 'r')
            {
                m_chmod = _S_IREAD;
                cmdOpts++;
            }
            else if (cmdOpts[1] == 'w')
            {
                m_chmod = _S_IWRITE;
                cmdOpts++;
            }
            else if (cmdOpts[1] == 't') // -ct or -ct=now, -ct=<+-n>[d|h|m] -ct=yyyy:mm:dd:hh:mm:ss
            {
                m_setTime = true;
                cmdOpts++;

                SYSTEMTIME utcTime;
                GetSystemTime(&utcTime);
                SystemTimeToFileTime(&utcTime, &m_setUtcTime);

                if (cmdOpts[1] == sEQchr)
                {
                    cmdOpts += 2;
                    if (!LLSup::ParseTime(cmdOpts, m_setUtcTime))
                        cmdOpts--;
                }

                if (m_verbose)
                {
                    LLMsg::Out() << "SetTime to Local=";
                    LLSup::Format(LLMsg::Out(), m_setUtcTime, true) << std::endl;
                }
            }
            break;
        case 'C':   // Color options -C=r.red -C=d.blue -C=d.red+blue
            // -C=none or -C=0 disable colors
            if (cmdOpts[1] == sEQchr)
            {
                cmdOpts += 2;
                switch (ToLower(*cmdOpts))
                {
                case 'r':  LLSup::SetColor(sConfig.m_colorROnly,  cmdOpts+1); break;
                case 'h':  LLSup::SetColor(sConfig.m_colorHidden, cmdOpts+1); break;
                case 'd':  LLSup::SetColor(sConfig.m_colorDir,    cmdOpts+1); break;
                case '0':
                case 'n':
                    sConfig.m_colorOn = false;
                    break;
                default:
                    ErrorMsg() << "Unknown color option: -C" << *cmdOpts << std::endl;
                }

                // Move to end of color options
                while(*cmdOpts > sEOCchr)
                    cmdOpts++;
            }
            break;

        case 'h':
        case 'H':
            m_showHeader = !m_showHeader;
            break;
        case 'l':   // Enable Long output, turn everything on.
            sortNeedAllData |= m_showAttr = m_showMtime = m_showSize = m_showLink = m_showStuff = true;
            m_dirSort.SetSortData(sortNeedAllData);
            break;
        case 'L':   // Show hardlink count
            m_showLink = true;
            m_showStream = true;
            break;
        case 'i':   // Show file ID
            m_showFileId = true;
            m_showLink   = true;
            break;
        case 'n':
        case 'N':   // Show just name
            m_showSize = false;
            m_showHeader = false;
            m_quiet = true;
            m_showAtime = m_showCtime = m_showMtime = false;
            break;

        case 'p':   // -p show relative path, -pp show full path
            m_showPath++;
            break;
        case 's':   // Toggle showing size.
            sortNeedAllData |= m_showSize = m_showStuff = !m_showSize;
            m_dirSort.SetSortData(sortNeedAllData);
            break;

        case 'S':   // Sort options
            {
            if (m_dirScan.m_recurse == true)
                m_showHeader = false;   // heading does not work well with sorting

            m_dirSort.SetSort(m_dirScan, m_showPath ? "p" : "n", sortNeedAllData, true);
            bool sortInc = true;
            bool unknownOpt = false;
            char sortOpt[] = "x";
			if (cmdOpts[1] == '=')	// Skip optional leading =, as in -S=n
				cmdOpts++;
            while (cmdOpts[1] && !unknownOpt)
            {
                cmdOpts++;
                sortOpt[0] = ToLower(*cmdOpts);
                switch (sortOpt[0])
                {
                case '-':
                    sortInc = false;
                    break;
                case '+':
                    sortInc = true;
                    break;

                case 'e':   // extension
                case 'n':   // name
                case 'p':   // path
                case 's':   // size
                case 't':   // type (file, directory)

                case 'm':   // modify Time
                case 'a':   // access Time
                case 'c':   // create Time
                    m_dirSort.SetSort(m_dirScan, sortOpt, sortNeedAllData, sortInc);
                    break;
                default:    unknownOpt = true;   cmdOpts--; break;
                }
            }
            }
            break;

        case 't':   // Display Time selection
            {
                bool unknownOpt = false;
				if (cmdOpts[1] == '=')	// Skip optional leading =, as in -t=n
					cmdOpts++;
				while (cmdOpts[1] && !unknownOpt)
                {
                    cmdOpts++;
                    switch (ToLower(*cmdOpts))
                    {
                    case 'a':
                        sortNeedAllData = m_showAtime = m_showStuff = true;
                        break;
                    case 'c':
                        sortNeedAllData = m_showCtime = m_showStuff = true;
                        break;
                    case 'm':
                        sortNeedAllData = m_showMtime = m_showStuff = true;
                        break;
                    case 'n':   // NoTime
                        sortNeedAllData = m_showStuff = false;
                        m_showAtime = m_showCtime = m_showMtime = false;
                        break;
                    default:
                        cmdOpts--;
                        unknownOpt = true;
                        break;
                    }
                }
                m_dirSort.SetSortData(sortNeedAllData);
            }
            break;

        case 'U':   // Disk usage by extension, -U=ext, -UU=name, -UUU=dir
            if (m_showUsageExt)
            {
                if (m_showUsageName)
                    m_showUsageDir = true;
                else
                    m_showUsageName = true;
            }
            m_showUsageExt = true;

        case 'u':   // Disk usage
            m_dirScan.m_recurse = true;
            m_dirScan.m_addAllDepths = true;
            m_showUsage = true;
            m_onlyAttr = 0x000FFFFF;
            break;

        case 'V':   // Inverted filematch -V=<pathPat>[,<pathPat>...]
            cmdOpts = LLSup::ParseList(cmdOpts+1, m_invertedList, invertEmptyMsg);
            if (!m_invertedList.empty())
                m_dirScan.m_add_cb = InvertEntryCb;
            break;

        case 'w':   // Show in DOS like wide format, wrap output to fill line.
            m_showWide = true;
            break;

        case 'W':   // Watch directory, -W[admr]  or -W=[admr]
            {
                m_watchDir = true;
                m_WatchBits = (cmdOpts[1] > sEOCchr) ? 0 : -1;
                bool unknownOpt = false;
                while (cmdOpts[1] > sEOCchr && !unknownOpt)
                {
                    cmdOpts++;
                    switch (ToLower(*cmdOpts))
                    {
                    case '=':
                        break;
                    case 'a':
                        m_WatchBits |= 1 << FILE_ACTION_ADDED;
                        break;
                    case 'd':
                        m_WatchBits |= 1 << FILE_ACTION_REMOVED;
                        break;
                    case 'm':
                        m_WatchBits |= 1 << FILE_ACTION_MODIFIED;
                        break;
                    case 'r':
                        m_WatchBits |= 1 << FILE_ACTION_RENAMED_OLD_NAME;
                        m_WatchBits |= 1 << FILE_ACTION_RENAMED_NEW_NAME;
                        break;
                    default:
                        cmdOpts--;
                        unknownOpt = true;
                        if (m_WatchBits == 0)
                            m_WatchBits = sMinus1;
                        break;
                    }
                }
            }
            break;

        // Output modifiers
        case '=':
            cmdOpts = LLSup::ParseString(cmdOpts, LLDir::sConfig.m_dirFieldSep, optOutSepErrMsg);
            break;
        case ',':
            DisableCommaCout();
            break;
        case '#':   // Toggle numeric date and use default 'locale' date presentation.
            LLDir::sConfig.m_numDateTime = !LLDir::sConfig.m_numDateTime;
            break;
        case '~':   // Toggle relative time display 
            LLDir::sConfig.m_refDateTime = !LLDir::sConfig.m_refDateTime;
            break;

        case '?':
            Colorize(std::cout, sHelp);
            if (cmdOpts[1] == '?')
                std::cout << sMoreHelp;
            return sIgnore;
        default:
            if ( !ParseBaseCmds(cmdOpts))
                return sError;
        }

        // Advance to next parameter
        LLSup::AdvCmd(cmdOpts);
    }

    m_dirSort.SetSortAttr(m_onlyAttr);
    m_dirScan.m_filesFirst = m_dirScan.m_recurse && !m_showUsage;

    // If just showing path, remove leading space.
    if (m_showPath && !m_showSize && !m_showAttr && !m_showAtime  && !m_showCtime && !m_showMtime)
        LLDir::sConfig.m_dirFieldSep = "";

    if (m_quiet && m_watchDir)
    {
        WatchDir(argc, pDirs);
        return nFiles;
    }

    LLMsg::Out() << std::endl;

    if (m_inFile.length() != 0)
    {
        if (m_invertedList.empty()) 
            LLSup::ReadFileList(m_inFile.c_str(), EntryCb, this);
        else 
        {
            // Handle inverted search.
            LLSup::ReadFileList(m_inFile.c_str(), InvertEntryCb, this);
        }
    }

    // Iterate over dir patterns.
    for (int argn=0; argn < argc; argn++)
    {
        if (m_showWide)
        {
            size_t dirCount  = m_dirCount;
            size_t fileCount = m_fileCount;

            LLMsg::SetNullStream();
            m_dirScan.Init(pDirs[argn], NULL);
            m_dirScan.GetFilesInDirectory();
            m_fixedWidth = 0;
            LLMsg::RestoreStream();

            m_dirCount = dirCount;
            m_fileCount= fileCount;
        }

        if ( !m_quiet)
            SetColor(sConfig.m_colorHeader);
        m_dirScan.Init(pDirs[argn], NULL);
        if ( !m_quiet)
            SetColor(sConfig.m_colorNormal);

        sDepthUsage.clear();
        nFiles += m_dirScan.GetFilesInDirectory();
    }

    if (m_dirScan.m_add_cb != EntryCb && m_dirScan.m_add_cb != InvertEntryCb)
        m_dirSort.ShowSorted(EntryCb, this);

    if ( !m_quiet)
    {
        SetColor(sConfig.m_colorHeader);
        LLMsg::Out() << "\n;";
        LLMsg::Out() << "Directories:" << m_dirCount << " Files:" << m_fileCount;
        if (m_showSize)
            LLMsg::Out() << " TotalSize:" << m_totalSize << " ";
        if (m_showCtime)
            LLMsg::Out() << " MinCreationTime:", LLSup::Format(LLMsg::Out(), m_minCTime) << " " ;
        if (m_showAtime)
            LLMsg::Out() << " MaxAccessTime:", LLSup::Format(LLMsg::Out(), m_maxATime) << " ";

        LLMsg::Out() << std::endl;
        // SetColor(sConfig.m_colorNormal);
    }

    if (m_watchDir)
        WatchDir(argc, pDirs);

	return ExitStatus((int)m_fileCount);
}

// ---------------------------------------------------------------------------
// Return 1 if directory engine presented something to output,
// else 0 if nothing presented,
// or -1 if an error occured.

int LLDir::DirFile(const char* fullFile, int depth)
{
    WIN32_FIND_DATA fileData;
    char name[MAX_PATH];
    char dir[MAX_PATH];

    const char* namePos = strrchr(fullFile, '\\');
    if (namePos != NULL)
    {
        strncpy_s(dir, ARRAYSIZE(dir), fullFile, namePos - fullFile);
        strcpy_s(name, ARRAYSIZE(name), namePos+1);
    }
    else
    {
        GetCurrentDirectory(ARRAYSIZE(dir), dir);
        strcpy_s(name, ARRAYSIZE(name), fullFile);
    }

    ClearMemory(&fileData, sizeof(fileData));
    strcpy_s(fileData.cFileName, ARRAYSIZE(fileData.cFileName), name);

    struct _stat64i32 fileStat;
    if (_stat(fullFile, &fileStat) == 0)
    {
        fileData.nFileSizeLow = fileStat.st_size;

        LLSup::UnixTimeToFileTime(fileStat.st_ctime, fileData.ftCreationTime);
        LLSup::UnixTimeToFileTime(fileStat.st_atime, fileData.ftLastAccessTime);
        LLSup::UnixTimeToFileTime(fileStat.st_mtime, fileData.ftLastWriteTime);
        fileData.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
        fileData.dwFileAttributes |= ((fileStat.st_mode & _S_IFDIR) != 0) ? FILE_ATTRIBUTE_DIRECTORY : 0;
        fileData.dwFileAttributes |= ((fileStat.st_mode & _S_IWRITE) == 0) ? FILE_ATTRIBUTE_READONLY : 0;
    }

    return ProcessEntry(dir, &fileData, depth);
}

/* ------------------------------------------------------------------------- */
struct MonitorDirInfo
{
    MonitorDirInfo() : pLLDir(NULL), dir(NULL), abort(false), subDir(true), count(0) {}

    LLDir*          pLLDir;
    const char*     dir;
    bool            abort;
    bool            subDir;
    LONG            count;
    LONG            limit;
};

/* ------------------------------------------------------------------------- */

DWORD WINAPI MonitorDirThread(void* data)
{
    MonitorDirInfo* pDirInfo = (MonitorDirInfo*)data;
    std::string dir = pDirInfo->dir;
    std::string filter = "*";

    // Directory Watcher only works on Directories, so trim dir path until it works.
    DWORD fileAttr = GetFileAttributes(dir.c_str());
    while (fileAttr == INVALID_FILE_ATTRIBUTES || (fileAttr & FILE_ATTRIBUTE_DIRECTORY) == 0)
    {
        int pos = dir.find_last_of('\\');
        if (pos == -1)
            break;
        filter = dir.substr(pos+1);
        if (dir[pos-1] == ':')
            pos++;
        dir.resize(pos);
        fileAttr = GetFileAttributes(dir.c_str());
    }

    DWORD err=0;
    do
    {
        Handle hDir = CreateFile(dir.c_str(),       // pointer to the file name
                FILE_LIST_DIRECTORY,                // access (read/write) mode
                FILE_SHARE_READ|FILE_SHARE_DELETE,  // share mode
                NULL,                               // security descriptor
                OPEN_EXISTING,                      // how to create
                FILE_FLAG_BACKUP_SEMANTICS,         // file attributes
                NULL                                // file with attributes to copy
                );

        if (hDir.NotValid())
        {
            err = GetLastError();
            LLMsg::Out() << "Failed to monitor directory:" << dir << ", error=" << err << std::endl;
            return sMinus1;
        }

        // FILE_NOTIFY_INFORMATION
        char Buffer[10240];
        DWORD BytesReturned;

        while (pDirInfo->abort == false &&
            ReadDirectoryChangesW(
                hDir,                                 // handle to directory
                &Buffer,                              // read results buffer
                sizeof(Buffer),                       // length of buffer
                pDirInfo->subDir,
                FILE_NOTIFY_CHANGE_CREATION
                |FILE_NOTIFY_CHANGE_DIR_NAME
                |FILE_NOTIFY_CHANGE_FILE_NAME
                |FILE_NOTIFY_CHANGE_ATTRIBUTES
                |FILE_NOTIFY_CHANGE_SIZE
                |FILE_NOTIFY_CHANGE_LAST_WRITE  // filter conditions
                ,
                &BytesReturned,                 // bytes returned
                NULL,                           // overlapped buffer
                NULL// completion routine
                ))
        {
            char* pBuffer = Buffer;
            FILE_NOTIFY_INFORMATION* pNotifyInfo;

            do
            {
                pNotifyInfo = (FILE_NOTIFY_INFORMATION*)pBuffer;
                pBuffer += pNotifyInfo->NextEntryOffset;

                if (((1 << pNotifyInfo->Action) & pDirInfo->pLLDir->WatchBits()) == 0)
                    continue;

                const char* actionStr = "Unknown";
                switch(pNotifyInfo->Action)
                {
                  case FILE_ACTION_ADDED:
                      actionStr = "   Added";
                      break;
                  case FILE_ACTION_REMOVED:
                      actionStr = " Deleted";
                      break;
                  case FILE_ACTION_MODIFIED:
                      actionStr = "Modified";
                      break;
                  case FILE_ACTION_RENAMED_OLD_NAME:
                      actionStr = " Renamed";
                      break;
                  case FILE_ACTION_RENAMED_NEW_NAME:
                      actionStr = " Renamed";
                      break;
                  default:
                      actionStr = " Changed";
                      break;
                }

                std::wstring wFilename(pNotifyInfo->FileName, pNotifyInfo->FileNameLength/2);
                // std::string fileName(wFilename.begin(), wFilename.end());
				std::string fileName;
				LLSup::ToNarrow(wFilename, fileName);

                std::string filePath = dir + std::string("\\") + fileName;

                if (filter.length() == 0 || PatternMatch(filter.c_str(), fileName.c_str()))
                {
                    if ( !pDirInfo->pLLDir->Quiet())
                    {
                        // TODO - add date/time line counter
                        time_t now;
                        struct tm today;
                        time(&now);
                        localtime_s(&today, &now);
                        char obuf[512];
                        if (strftime(obuf, sizeof(obuf), "%m/%d/%y, %H:%M:%S, ", &today) > 0)
                            LLMsg::Out() <<  obuf;
                        LLMsg::Out() << std::setw(4) << pDirInfo->count++  << ", ";
                        LLMsg::Out() << actionStr << ", ";
                    }
                    // m_fileCount++;
                    if (pDirInfo->pLLDir->DirFile(filePath.c_str()) <= 0)
                        LLMsg::Out() << std::endl;
                    LLMsg::Out().flush();
                }

                InterlockedIncrement(&LLDir::sWatchCount);
                if (LLDir::sWatchLimit != 0 && LLDir::sWatchCount >= LLDir::sWatchLimit)
                {
                    pDirInfo->abort = true;
                }
            }
            while ( !pDirInfo->abort && pNotifyInfo->NextEntryOffset != 0);

            ClearMemory(Buffer, sizeof(Buffer));
        }

        err = GetLastError();
        // CloseHandle(hDir);

        if (err == ERROR_INVALID_PARAMETER)
        {
            LLMsg::Out() << "Error: Invalid parameter\n";
        }

        if (err != 0)
        {
            LLMsg::Out() << "Error:" << err << std::endl;
            Sleep(2000);
        }

        // Retry if error is:
        //  64 "The specified network name is no longer available."
    } while (err == 64);

    return(0);
}

// ---------------------------------------------------------------------------

void LLDir::WatchDir(int argc, const char* pDirs[])
{
    m_watchDir = false;
    m_showHeader = false;
    sWatchLimit = m_limitOut;
    m_limitOut = 0;

    if ( !Quiet())
        LLMsg::Out() << "\nBegin to monitor " << argc << " directories\n\n";

    HANDLE* pThread = new HANDLE[argc];
    DWORD*  pThreadId = new DWORD[argc];
    MonitorDirInfo* pDirInfo = new MonitorDirInfo[argc];

    for (int argIdx = 0; argIdx < argc; argIdx++)
    {
        pDirInfo->pLLDir = this;
        pDirInfo->dir = pDirs[argIdx];
        pThread[argIdx] = CreateThread(NULL, 0, MonitorDirThread, (PVOID)pDirInfo, 0, &pThreadId[argIdx]);
    }

    // Sleep(...)
    // sMonitorDirAbort = true;
    DWORD status = WaitForMultipleObjects(argc, pThread, true, INFINITE);
    if ( !m_quiet)
        LLMsg::Out() << "erron=" << errno << " LastError=" << GetLastError() << " status=" << status << std::endl;
}

// ---------------------------------------------------------------------------
// http://www.codeproject.com/KB/cs/console_apps__colour_text.aspx

bool ColorStart(DWORD fileAttribute)
{
    bool colorOn = false;
    bool isDir = ((fileAttribute & FILE_ATTRIBUTE_DIRECTORY) != 0);
    bool isReadOnlye = ((fileAttribute & FILE_ATTRIBUTE_READONLY) != 0);
    bool isHidden = ((fileAttribute & FILE_ATTRIBUTE_HIDDEN) != 0);

    WORD colorAttr = LLDir::sConfig.m_colorNormal;
    if (isReadOnlye)
    {
        colorAttr = LLDir::sConfig.m_colorROnly;
        colorOn = true;
    }

    if (isDir)
    {
        colorAttr = LLDir::sConfig.m_colorDir;
        colorOn = true;
    }

    if (isHidden)
    {
        colorAttr = LLDir::sConfig.m_colorHidden;
        colorOn = true;
    }

    LLBase::SetColor(colorAttr);
    return colorOn;
}

// ---------------------------------------------------------------------------
void ColorEnd(bool colorOn, DWORD fileAttribute)
{
    if (colorOn)
    {
        LLBase::SetColor(LLDir::sConfig.m_colorNormal);
    }
}

// ---------------------------------------------------------------------------
#pragma warning(push)
#pragma warning(disable : 4996)

void LLDir::ShowHeader(const char* pDir, const WIN32_FIND_DATA* pFileData)
{
    char pad[80];

    if ( !m_quiet)
        SetColor(sConfig.m_colorHeader);

    LLMsg::Out() << std::endl;
    LLMsg::Out() << " Directory:" << pDir << std::endl;
    if (m_showAttr)
        LLMsg::Out() << PadRight(strcpy(pad, "Attributes"), ARRAYSIZE(pad), 15, ' ') << LLDir::sConfig.m_dirFieldSep;
    if (m_showCtime)
        LLMsg::Out() << PadRight(strcpy(pad, "Creation"), ARRAYSIZE(pad), 20, ' ') << LLDir::sConfig.m_dirFieldSep;
    if (m_showMtime)
        LLMsg::Out() << PadRight(strcpy(pad, "Modification"), ARRAYSIZE(pad), 20, ' ') << LLDir::sConfig.m_dirFieldSep;
    if (m_showAtime)
        LLMsg::Out() << PadRight(strcpy(pad, "Access"), ARRAYSIZE(pad), 20, ' ') << LLDir::sConfig.m_dirFieldSep;
    if (m_showSize)
        LLMsg::Out() << PadLeft(strcpy(pad, "Size"), ARRAYSIZE(pad), sConfig.m_fzWidth, ' ') << LLDir::sConfig.m_dirFieldSep;
    if (m_showLink)
        LLMsg::Out() << "Lnk" << LLDir::sConfig.m_dirFieldSep;
    if (m_showFileId)
        LLMsg::Out() << PadRight(strcpy(pad, " FileId"), ARRAYSIZE(pad), 9, ' ') << LLDir::sConfig.m_dirFieldSep;
    // if (m_showStuff)
    //     LLMsg::Out() << LLDir::sConfig.m_dirFieldSep;
    if ( !m_showAttr && !m_quiet)
        LLMsg::Out() << " A " << LLDir::sConfig.m_dirFieldSep;
    // else
    //    LLMsg::Out() << LLDir::sConfig.m_dirFieldSep;

    LLMsg::Out() << "File";
    LLMsg::Out() << std::endl;  // better performance with \n to avoid flush using std::endl;

    if ( !m_quiet)
        SetColor(sConfig.m_colorNormal);
}
#pragma warning(pop)



// ---------------------------------------------------------------------------
// Return 1 if output anything, 0 if nothing, -1 if error.

int LLDir::ProcessEntry(
        const char* pDir,
        const WIN32_FIND_DATA* pFileData,
        int depth)      // 0...n is directory depth, -n end-of nth diretory
{
    int retStatus = sIgnore;

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
     
#if 0
    // If inverted mode - only show directories.
    if (!m_invertedList.empty() && !m_isDir)
        return sInvert;
#endif

    if (m_showUsage)
    {
        if (depth < 0)
        {
            depth = -1 - depth;  // negative depth is off by one, so adjust

            if (depth < 2)
            {
                if ((m_countOut == 0 && !IsQuit()) || m_showUsageExt)
                {
                    SetColor(sConfig.m_colorHeader);
                    LLMsg::Out() << "  #Dirs,     #Files,   Min File Size,   Max File Size,      Total Size, Directory\n";
                    SetColor(sConfig.m_colorNormal);
                }

                if (depth < (int)sDepthUsage.size())
                {
                    if (!IsQuit())
                    {
                        if (sDepthUsage[depth].extUsage.size() != 0)
                        {
                            for (Usage::ExtUsage::const_iterator iter = sDepthUsage[depth].extUsage.begin();
                                iter != sDepthUsage[depth].extUsage.end() && !IsQuit();
                                ++iter)
                            {
                                const Usage& usage = iter->second;
                                LLMsg::Out()
                                    << setw(7)  << usage.m_dirCount
                                    << ", "
                                    << setw(10) << usage.m_fileCount
                                    << ", "
                                    << setw(15) << usage.m_minFileSize
                                    << ", "
                                    << setw(15) << usage.m_maxFileSize
                                    << ", "
                                    << setw(15) << usage.m_totalSize
                                    << ", "
                                    << iter->first << std::endl;
                            }
                        }

                        LLMsg::Out()
                            << setw(7)   << sDepthUsage[depth].m_dirCount
                            << ", "
                            << setw(10) << sDepthUsage[depth].m_fileCount
                            << ", ";
                        if (sDepthUsage[depth].m_maxFileSize != 0)
                        {
                            LLMsg::Out()
                                << setw(15) << sDepthUsage[depth].m_minFileSize
                                << ", "
                                << setw(15) << sDepthUsage[depth].m_maxFileSize;
                        }
                        else
                        {
                             LLMsg::Out()
                                << setw(15) << " "
                                << ", "
                                << setw(15) << " ";
                        }

                        LLMsg::Out()
                            << ", "
                            << setw(15) << sDepthUsage[depth].m_totalSize
                            << ", ";
                        LLMsg::Out() << pDir << std::endl;
                    }
                }
                else if (m_quiet == false)
                {
                    LLMsg::Out() << "Empty ";
                    LLMsg::Out() << pDir << std::endl;
                }
            }

            if (depth > 0 && depth < (int)sDepthUsage.size())
            {
                // Update parent's total
                sDepthUsage[depth-1].Add(sDepthUsage[depth]);
                /*
                sDepthUsage[depth-1].dirCount += sDepthUsage[depth].dirCount;
                sDepthUsage[depth-1].fileCount += sDepthUsage[depth].fileCount;
                sDepthUsage[depth-1].fileSize.QuadPart += sDepthUsage[depth].fileSize.QuadPart;
                sDepthUsage[depth-1].minFileSize.QuadPart = min(
                    sDepthUsage[depth-1].minFileSize.QuadPart, sDepthUsage[depth].minFileSize.QuadPart);
                sDepthUsage[depth-1].maxFileSize.QuadPart = max(
                    sDepthUsage[depth-1].maxFileSize.QuadPart, sDepthUsage[depth].maxFileSize.QuadPart);
                // sDepthUsage[depth].Clear();
                */
                sDepthUsage.erase(sDepthUsage.begin() + depth);
            }
        }
        else
        {
            while ((int)sDepthUsage.size() <= depth)
            {
                Usage usage;
                sDepthUsage.push_back(usage);
            }

            // sDepthUsage[depth].fileSize.QuadPart += fileSize.QuadPart;

            m_totalSize += m_fileSize;

            if (m_isDir)
            {
                sDepthUsage[depth].m_dirCount++;
                m_dirCount++;
            }
            else
            {
               if (m_showUsageDir)
                {
                    if (m_fileSize != 0)
                    {
                        // TODO - match command line pattern to determine depth to select dir to key off of.
                        //  or add -U<number> to select directory level to key off of.
                        const char* pBaseDir = strchr(pDir, '\\');
                        if (pBaseDir == NULL)
                            pBaseDir =  pDir;
                        sDepthUsage[0].Add(m_fileSize, pBaseDir);
                    }
                }
                else if (m_showUsageName)
                    sDepthUsage[0].Add(m_fileSize, pFileData->cFileName);
                else if (m_showUsageExt)
                    sDepthUsage[depth].Add(m_fileSize, LLPath::GetExtension(pFileData->cFileName));
                else
                    sDepthUsage[depth].Add(m_fileSize);

                m_fileCount++;
            }
        }

        return sIgnore;
    }
    else
    {
        if (depth < 0)
            return sIgnore;     // ignore end-of-directory

        if (IsQuit())
            return sIgnore;

        if (m_chmod != 0)
        {
            // sprintf_s(m_srcPath, sizeof(m_srcPath), "%s\\%s", pDir, pFileData->cFileName);
            if (_chmod(m_srcPath, m_chmod) == -1)
            {
                perror(m_srcPath);
            }
        }

        std::string errMsg;
        if (m_setTime)
        {
            if (!LLSup::SetFileModTime(m_srcPath, m_setUtcTime))
                errMsg += " TimeSet Failed";
        }

        std::string linkPath;
        bool hasSoftLink = GetSoftLink(linkPath, pDir, *pFileData);

        static char prevDir[MAX_PATH] = "";
        if (m_showHeader &&
            (strcmp(pDir, prevDir)!=0 || m_dirCount + m_fileCount == 0) )
        {
            strcpy_s(prevDir, ARRAYSIZE(prevDir), pDir);
            ShowHeader(pDir, pFileData);
        }

        bool colorOn = (m_quiet == false &&
            ColorStart(pFileData->dwFileAttributes));

        if (m_showAttr)
        {
            ShowAttributes(LLMsg::Out(), pDir, *pFileData, m_showAttrWords);
            LLMsg::Out() << LLDir::sConfig.m_dirFieldSep;
        }
        if (m_showCtime)
            Format(LLMsg::Out(), pFileData->ftCreationTime) << LLDir::sConfig.m_dirFieldSep ;
        if (m_showMtime)
            Format(LLMsg::Out(),pFileData->ftLastWriteTime) << LLDir::sConfig.m_dirFieldSep;
        if (m_showAtime)
            Format(LLMsg::Out(), pFileData->ftLastAccessTime) << LLDir::sConfig.m_dirFieldSep;

        if (m_showSize)
        {
            if (hasSoftLink)
            {
                struct _stat64i32 fileStat;
                if (_stat(linkPath.c_str(), &fileStat) == 0)
                    m_fileSize = fileStat.st_size;
            }
            LLMsg::Out() << std::setw(LLDir::sConfig.m_fzWidth) << m_fileSize << LLDir::sConfig.m_dirFieldSep;
        }
        if (m_showSecurity)
        {
            // http://www.ionicwind.com/forums/index.php?topic=3526.5;wap2
            // Also look at GetFileSecurity(), GetNamedSecurityInfo()

            Handle fileHnd =
                CreateFile(m_srcPath, MAXIMUM_ALLOWED, 7, 0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
                // CreateFile(m_srcPath, FILE_READ_ATTRIBUTES, 7, 0, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, 0);
            if (fileHnd.IsValid())
            {
                PACL dacl = NULL;
                PSECURITY_DESCRIPTOR pSecurityDesc = NULL;
                // const DWORD ERROR_SUCCESS = 0;
                if (ERROR_SUCCESS ==
                    GetSecurityInfo(fileHnd, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, &dacl, NULL, &pSecurityDesc))
                {
                    ACCESS_MASK access;
                    TRUSTEE tr;
                    tr.pMultipleTrustee = NULL;
                    tr.TrusteeForm = TRUSTEE_IS_NAME;
                    tr.TrusteeType = TRUSTEE_IS_USER;
                    tr.ptstrName = "CURRENT_USER";

                    if (ERROR_SUCCESS ==
                        GetEffectiveRightsFromAcl(dacl, &tr, &access))
                    {
                        std::string msg;
                        // if (access & DELETE_CONTROL) msg += "DELETE\n";
                        if (access & FILE_ADD_FILE) msg  += "\tFILE_ADD_FILE\n";
                        if (access & FILE_ADD_SUBDIRECTORY) msg  += "\tFILE_ADD_SUBDIRECTORY\n";
                        if (access & FILE_ALL_ACCESS) msg  += "\tFILE_ALL_ACCESS\n";
                        if (access & FILE_DELETE_CHILD) msg  += "\tFILE_DELETE_CHILD\n";
                        if (access & FILE_LIST_DIRECTORY) msg  += "\tFILE_LIST_DIRECTORY\n";
                        if (access & FILE_READ_ATTRIBUTES) msg  += "\tFILE_READ_ATTRIBUTES\n";
                        if (access & FILE_READ_EA) msg  += "\tFILE_READ_EA\n";
                        if (access & FILE_TRAVERSE) msg  += "\tFILE_TRAVERSE\n";
                        if (access & FILE_WRITE_ATTRIBUTES) msg  += "\tFILE_WRITE_ATTRIBUTES\n";
                        if (access & FILE_WRITE_EA) msg  += "\tFILE_WRITE_EA\n";
                        if (access & STANDARD_RIGHTS_READ) msg  += "\tSTANDARD_RIGHTS_READ\n";
                        if (access & STANDARD_RIGHTS_WRITE) msg  += "\tSTANDARD_RIGHTS_WRITE\n";

                        if (msg.length() != 0)
                            LLMsg::Out() << std::endl <<  msg.c_str() << LLDir::sConfig.m_dirFieldSep;
                    }

                    LocalFree(pSecurityDesc);
                }

                // CloseHandle(fileHnd);
            }

        }

        if (m_showLink)
        {
            int nLinks = 1;
            LARGE_INTEGER fileId;
            fileId.QuadPart = 0;

            BY_HANDLE_FILE_INFORMATION ByHandleFileInformation;
            // sprintf_s(m_srcPath, sizeof(m_srcPath), "%s\\%s", pDir, pFileData->cFileName);
            Handle fileHnd =
                CreateFile(m_srcPath, FILE_READ_ATTRIBUTES, 7, 0, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, 0);
            if (fileHnd.IsValid())
            {
                if (GetFileInformationByHandle(fileHnd, &ByHandleFileInformation))
                {
                    nLinks = ByHandleFileInformation.nNumberOfLinks;
                    fileId.LowPart  = ByHandleFileInformation.nFileIndexLow;
                    fileId.HighPart = ByHandleFileInformation.nFileIndexHigh;

#ifdef VISTA
                    FILE_ID_DESCRIPTOR fileIdDes;
                    fileIdDes.dwSize = sizeof(fileId);
                    fileIdDes.Type = FileIdType;
                    fileIdDes.FileId = fileId;

                    Handle linkHnd = OpenFileById(
                        fileHnd,
                        &fileIdDes,
                        GENERIC_READ,
                        7,
                        0,
                        0);

                    FILE_NAME_INFO fileNameInfo;
                    if (linkHnd.IsValid())
                    {
                        if (GetFileInformationByHandleEx(linkHnd, FileNameInfo,
                            &fileNameInfo, sizeof(fileNameInfo)) != 0)
                        {
                            cout << "-H->" << fileNameInfo.FileName << " ";
                        }
                        // CloseHandle(linkHnd);
                    }
#endif
                }
                // CloseHandle(fileHnd);
            }

            LLMsg::Out() << std::setw(3) << nLinks << LLDir::sConfig.m_dirFieldSep;

            if (m_showFileId)
            {
                std::locale prevLocal = LLMsg::Out().imbue(std::locale(std::locale()));
                LLMsg::Out() << std::setw(9) << (void*)fileId.QuadPart;
                LLMsg::Out().imbue(prevLocal);
                LLMsg::Out() << LLDir::sConfig.m_dirFieldSep;
            }
        }


        if ( !m_showAttr && !m_quiet)
        {
            ShowAttributeAsChar(LLMsg::Out(), pDir, *pFileData, m_showAttrWords);
            LLMsg::Out() << LLDir::sConfig.m_dirFieldSep;
        }

        if (m_showPath == 1)
            LLMsg::Out() << pDir << LLPath::sDirSlash;
        else if (m_showPath > 1)
            LLMsg::Out() << DirectoryScan::GetFullPath(pDir) << LLPath::sDirSlash;

        LLMsg::Out() << pFileData->cFileName;

        if (hasSoftLink)
            LLMsg::Out() << " -> " << linkPath;

        if (!errMsg.empty())
            LLMsg::Out() << errMsg;

        if (m_showWide)
        {
            // Decide if its time for a newLine

            int nameLen = (m_showPath ? strlen(pDir)+1 : 0) + strlen(pFileData->cFileName);

            LLMsg::Out().flush();
            static CONSOLE_SCREEN_BUFFER_INFO outInfo;
            GetConsoleScreenBufferInfo(ConsoleHnd(), &outInfo);

            m_maxFileNameLength = max(m_maxFileNameLength,
                nameLen);

            if (m_fixedWidth == 0)
                m_fixedWidth  = outInfo.dwCursorPosition.X - nameLen;

            int pad = m_maxFileNameLength - nameLen + 4;
            int endNext = outInfo.dwCursorPosition.X + pad +
                m_fixedWidth + m_maxFileNameLength + 4;

            if (m_screenWidth - endNext < 0)
                LLMsg::Out() << std::endl;
            else
            {
                while (pad--)
                    LLMsg::Out() << " ";
            }
        }
        else
        {
            LLMsg::Out() << std::endl;  // avoid std::endl so output is buffered.
        }

        ColorEnd(colorOn, pFileData->dwFileAttributes);

        if (m_showStream)
            ShowAlternateDataStreams(pDir, *pFileData, depth);
    }

    // update stats
    m_totalSize += m_fileSize;

    if (m_isDir)
        m_dirCount++;
    else
        m_fileCount++;

    if (CompareFileTime(&pFileData->ftCreationTime, &m_minCTime) < 0)
        m_minCTime = pFileData->ftCreationTime;
    if (CompareFileTime(&pFileData->ftLastAccessTime, &m_maxATime) > 0)
        m_maxATime = pFileData->ftLastAccessTime;

    return retStatus;   // sOkay = 1, means one file or directory was displayed.
}

