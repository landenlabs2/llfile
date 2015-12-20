//-----------------------------------------------------------------------------
// compress - Compression interface
// Author:  DLang   2008
// http://landenlabs.com/
//-----------------------------------------------------------------------------



#include <windows.h>
#include <iostream>

#include "..\dirscan.h"
#include "..\llbase.h"
#include "..\llmsg.h"
#include "minilzo.h"


/* Work-memory needed for compression. Allocate memory in units
 * of 'lzo_align_t' (instead of 'char') to make sure it is properly aligned.
 */

#define HEAP_ALLOC(var,size) \
    lzo_align_t __LZO_MMODEL var [ ((size) + (sizeof(lzo_align_t) - 1)) / sizeof(lzo_align_t) ]

static HEAP_ALLOC(wrkmem, LZO1X_1_MEM_COMPRESS);


// ---------------------------------------------------------------------------
int SaveCompressed(const char* srcFile, const char* dstFile)
{
    // If Windows 8
    // http://msdn.microsoft.com/en-us/library/windows/desktop/hh968104(v=vs.85).aspx

    // http://www.oberhumer.com/opensource/lzo/#download
    if (lzo_init() != LZO_E_OK)
    {
        ErrorMsg() << "LZO init failed, see http://www.oberhumer.com/opensource/lzo \n";
        return sError;
    }

    Handle srcHnd(
         CreateFile(srcFile,      // Open One.txt
                  GENERIC_READ,           // Open for reading
                  0,                      // Do not share
                  NULL,                   // No security
                  OPEN_EXISTING,          // Existing file only
                  FILE_ATTRIBUTE_NORMAL,  // Normal file
                  NULL));                  // No template file
    if ( !srcHnd.IsValid())
        return sError;

    Handle dstHnd(
        CreateFile(dstFile,      // Open Two.txt.
                    GENERIC_WRITE,          // Open for writing
                    0,                      // Do not share
                    NULL,                   // No security
                    OPEN_ALWAYS,            // Open or create
                    FILE_ATTRIBUTE_NORMAL,  // Normal file
                    NULL));                  // No template file
    if ( !dstHnd.IsValid())
        return sError;

    static unsigned char sInBuff[4096];
    static unsigned char sCmpBuff[4096*2];
    DWORD dwBytesRead, dwBytesWritten;
    size_t readCnt = 0;
    size_t writeCnt = 0;
    lzo_uint cmpLen;

    do
    {
        dwBytesWritten = 0;
        if (ReadFile(srcHnd, sInBuff, sizeof(sInBuff), &dwBytesRead, NULL))
        {
            readCnt += dwBytesRead;
            int status = lzo1x_1_compress(sInBuff, dwBytesRead, sCmpBuff, &cmpLen, wrkmem);
            if (status == LZO_E_OK)
            {
                WriteFile(dstHnd, sCmpBuff, (DWORD)cmpLen, &dwBytesWritten, NULL);
                writeCnt += dwBytesWritten;
            }
        }
    }
    while (dwBytesRead == sizeof(sInBuff) && dwBytesWritten != 0);

    std::cout << " lzo compression down to "
        << writeCnt*100/readCnt << " % of original " << writeCnt << std::endl;
    return (writeCnt != 0) ? sOkay : sError;
}


// ---------------------------------------------------------------------------
int SaveUncompressed(const char* srcFile, const char* dstFile)
{
    // If Windows 8
    // http://msdn.microsoft.com/en-us/library/windows/desktop/hh968104(v=vs.85).aspx

    // http://www.oberhumer.com/opensource/lzo/#download
    if (lzo_init() != LZO_E_OK)
    {
        ErrorMsg() << "LZO init failed, see http://www.oberhumer.com/opensource/lzo \n";
        return sError;
    }

    Handle srcHnd(
         CreateFile(srcFile,      // Open One.txt
                  GENERIC_READ,           // Open for reading
                  0,                      // Do not share
                  NULL,                   // No security
                  OPEN_EXISTING,          // Existing file only
                  FILE_ATTRIBUTE_NORMAL,  // Normal file
                  NULL));                  // No template file
    if ( !srcHnd.IsValid())
        return sError;

    Handle dstHnd(
        CreateFile(dstFile,      // Open Two.txt.
                    GENERIC_WRITE,          // Open for writing
                    0,                      // Do not share
                    NULL,                   // No security
                    OPEN_ALWAYS,            // Open or create
                    FILE_ATTRIBUTE_NORMAL,  // Normal file
                    NULL));                  // No template file
    if ( !dstHnd.IsValid())
        return sError;

    unsigned char inBuff[4096];
    unsigned char cmpBuff[4096];
    DWORD dwBytesRead, dwBytesWritten;
    size_t written = 0;
    lzo_uint cmpLen;

    do
    {
        dwBytesWritten = 0;
        if (ReadFile(srcHnd, inBuff, sizeof(inBuff), &dwBytesRead, NULL))
        {
            int status = lzo1x_decompress(inBuff, dwBytesRead, cmpBuff, &cmpLen, NULL);
            if (status == LZO_E_OK)
            {
                WriteFile(dstHnd, cmpBuff, (DWORD)cmpLen, &dwBytesWritten, NULL);
                written += dwBytesWritten;
            }
        }
    }
    while (dwBytesRead == sizeof(inBuff) && dwBytesWritten != 0);

    return (written != 0) ? sOkay : sError;
}


// ---------------------------------------------------------------------------
int CopyCompressed(const char* srcFile, const char* dstFile)
{
    if (DirectoryScan::IsSameFile(srcFile, dstFile))
    {
        ErrorMsg() << "Can't copy identical files: " << srcFile << " and " << dstFile << std::endl;
        return sError;
    }

    const char* srcLzo = strstr(srcFile, ".lzo");
    const char* dstLzo = strstr(dstFile, ".lzo");
    std::string tmpStr;
    if (srcLzo != NULL && dstLzo != NULL)
    {
        // ErrorMsg() << "Can't copy, both compressed: " << srcFile << " and " << dstFile << std::endl;
        // return sError;
        tmpStr = dstFile;
        tmpStr.resize((dstLzo - dstFile) / sizeof(char));
        dstFile = tmpStr.c_str();
    }

    if (srcLzo == NULL && dstLzo == NULL)
    {
        // ErrorMsg() << "Can't copy, both uncompressed: " << srcFile << " and " << dstFile << std::endl;
        // return sError;
        tmpStr = dstFile;
        tmpStr += std::string(".lzo");
        dstFile = tmpStr.c_str();
    }

    return (srcLzo != NULL) ?
        SaveUncompressed(srcFile, dstFile) :
        SaveCompressed(srcFile, dstFile);
}
