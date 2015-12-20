//-----------------------------------------------------------------------------
// Security - Security routines
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

#include "Security.h"

#include <windows.h>
#include <stdio.h>
#include <accctrl.h>
#include <aclapi.h>

#include "llsupport.h"
#include "llmsg.h"

namespace LLSec
{

//-----------------------------------------------------------------------------
// http://msdn.microsoft.com/en-us/library/windows/desktop/aa446619(v=vs.85).aspx
// #pragma comment(lib, "cmcfg32.lib")
#pragma comment(lib, "advapi32.lib")

bool SetPrivilege(
    HANDLE hToken,              // access token handle
    const char* lpszPrivilege,  // name of privilege to enable/disable
    bool bEnablePrivilege)      // to enable or disable privilege
{
    TOKEN_PRIVILEGES tp;
    LUID luid;

    if ( !LookupPrivilegeValue(
        NULL,            // lookup privilege on local system
        lpszPrivilege,   // privilege to lookup
        &luid ) )        // receives LUID of privilege
    {
        ErrorMsg() << "LookupPrivilegeValue error:" <<  GetLastError() << std::endl;
        return false;
    }

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    if (bEnablePrivilege)
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    else
        tp.Privileges[0].Attributes = 0;

    // Enable the privilege or disable all privileges.

    if ( !AdjustTokenPrivileges(
        hToken,
        FALSE,
        &tp,
        sizeof(TOKEN_PRIVILEGES),
        (PTOKEN_PRIVILEGES) NULL,
        (PDWORD) NULL) )
    {
        LLMsg::Out() << "AdjustTokenPrivileges error:" <<  GetLastError() << std::endl;
        return false;
    }

    if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)
    {
        LLMsg::Out() << "The token does not have the specified privilege. \n";
        return false;
    }

    return true;
}


//-----------------------------------------------------------------------------
// http://msdn.microsoft.com/en-us/library/windows/desktop/aa379620(v=vs.85).aspx
bool TakeOwnership(const char* lpszOwnFile)
{
    bool bRetval = false;

    HANDLE hToken = NULL;
    PSID pSIDAdmin = NULL;
    PSID pSIDEveryone = NULL;
    PACL pACL = NULL;
    SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY SIDAuthNT = SECURITY_NT_AUTHORITY;
    const int NUM_ACES  = 2;
    EXPLICIT_ACCESS ea[NUM_ACES];
    DWORD dwRes;

    // Specify the DACL to use.
    // Create a SID for the Everyone group.
    if ( !AllocateAndInitializeSid(&SIDAuthWorld, 1,
        SECURITY_WORLD_RID,
        0,
        0, 0, 0, 0, 0, 0,
        &pSIDEveryone))
    {
        LLMsg::Out() << lpszOwnFile << " AllocateAndInitializeSid (Everyone) error " <<  GetLastError() << std::endl;
        goto Cleanup;
    }

    // Create a SID for the BUILTIN\Administrators group.
    if ( !AllocateAndInitializeSid(&SIDAuthNT, 2,
        SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS,
        0, 0, 0, 0, 0, 0,
        &pSIDAdmin))
    {
        LLMsg::Out() << lpszOwnFile << " AllocateAndInitializeSid (Admin) error " <<   GetLastError() << std::endl;
        goto Cleanup;
    }

    ClearMemory(&ea, NUM_ACES * sizeof(EXPLICIT_ACCESS));

    // Set read access for Everyone.
//    ea[0].grfAccessPermissions = GENERIC_READ;
    ea[0].grfAccessPermissions = GENERIC_ALL;
    ea[0].grfAccessMode = SET_ACCESS;
    ea[0].grfInheritance = NO_INHERITANCE;
    ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea[0].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
    ea[0].Trustee.ptstrName = (LPTSTR) pSIDEveryone;

    // Set full control for Administrators.
    ea[1].grfAccessPermissions = GENERIC_ALL;
    ea[1].grfAccessMode = SET_ACCESS;
    ea[1].grfInheritance = NO_INHERITANCE;
    ea[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea[1].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
    ea[1].Trustee.ptstrName = (LPTSTR) pSIDAdmin;

    if (ERROR_SUCCESS != SetEntriesInAcl(NUM_ACES,
        ea,
        NULL,
        &pACL))
    {
        LLMsg::Out() << lpszOwnFile << " Failed SetEntriesInAcl\n";
        goto Cleanup;
    }

    // Try to modify the object's DACL.
    dwRes = SetNamedSecurityInfo(
        (char*)lpszOwnFile,             // name of the object
        SE_FILE_OBJECT,                 // type of object
        DACL_SECURITY_INFORMATION,      // change only the object's DACL
        NULL, NULL,                     // do not change owner or group
        pACL,                           // DACL specified
        NULL);                          // do not change SACL

    if (ERROR_SUCCESS == dwRes)
    {
        // LLMsg::Out() << "Successfully changed DACL\n";
        bRetval = TRUE;
        // No more processing needed.
        goto Cleanup;
    }

    if (dwRes != ERROR_ACCESS_DENIED)
    {
        LLMsg::Out() << lpszOwnFile << " First SetNamedSecurityInfo call failed:" << dwRes << std::endl;
        goto Cleanup;
    }

    // If the preceding call failed because access was denied,
    // enable the SE_TAKE_OWNERSHIP_NAME privilege, create a SID for
    // the Administrators group, take ownership of the object, and
    // disable the privilege. Then try again to set the object's DACL.

    // Open a handle to the access token for the calling process.
    if ( !OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken))
    {
        LLMsg::Out() << lpszOwnFile << " OpenProcessToken failed:" << GetLastError() << std::endl;
        goto Cleanup;
    }

    // Enable the SE_TAKE_OWNERSHIP_NAME privilege.
    if ( !SetPrivilege(hToken, SE_TAKE_OWNERSHIP_NAME, TRUE))
    {
        LLMsg::Out() << "You must be logged on as Administrator.\n";
        goto Cleanup;
    }

    // Set the owner in the object's security descriptor.
    dwRes = SetNamedSecurityInfo(
        (char*)lpszOwnFile,             // name of the object
        SE_FILE_OBJECT,                 // type of object
        OWNER_SECURITY_INFORMATION,     // change only the object's owner
        pSIDAdmin,                      // SID of Administrator group
        NULL,
        NULL,
        NULL);

    if (dwRes != ERROR_SUCCESS)
    {
        LLMsg::Out() << lpszOwnFile << " Could not set owner. Error:" <<  dwRes << std::endl;
        goto Cleanup;
    }

    // Disable the SE_TAKE_OWNERSHIP_NAME privilege.
    if ( !SetPrivilege(hToken, SE_TAKE_OWNERSHIP_NAME, FALSE))
    {
        LLMsg::Out() << lpszOwnFile << " Failed SetPrivilege call unexpectedly.\n";
        goto Cleanup;
    }

    // Try again to modify the object's DACL,
    // now that we are the owner.
    dwRes = SetNamedSecurityInfo(
        (char*)lpszOwnFile,             // name of the object
        SE_FILE_OBJECT,                 // type of object
        DACL_SECURITY_INFORMATION,      // change only the object's DACL
        NULL, NULL,                     // do not change owner or group
        pACL,                           // DACL specified
        NULL);                       // do not change SACL

    if (dwRes == ERROR_SUCCESS)
    {
        // LLMsg::Out() << "Successfully changed DACL\n";
        bRetval = true;
    }
    else
    {
        LLMsg::Out() << lpszOwnFile << " Second SetNamedSecurityInfo call failed:" <<  dwRes << std::endl;
    }

Cleanup:
    if (pSIDAdmin)
        FreeSid(pSIDAdmin);

    if (pSIDEveryone)
        FreeSid(pSIDEveryone);

    if (pACL)
        LocalFree(pACL);

    if (hToken)
        CloseHandle(hToken);

    return bRetval;
}

//-----------------------------------------------------------------------------
int RemoveFile(const char* filePath, bool DelToRecycleBin, bool force)
{
    DWORD attributes = GetFileAttributes(filePath);
    if ((attributes & FILE_ATTRIBUTE_READONLY) != 0)
        SetFileAttributes(filePath, attributes & ~FILE_ATTRIBUTE_READONLY);

    int result = LLSup::RemoveFile(filePath, DelToRecycleBin);
    if (_doserrno == EIO && force)
    {
        TakeOwnership((char*)filePath);
        result = LLSup::RemoveFile(filePath,DelToRecycleBin);
    }
    return result;
}

//-----------------------------------------------------------------------------
bool IsElevated( )
{
    bool fRet = false;
    HANDLE hToken = NULL;

    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
    {
        TOKEN_ELEVATION Elevation;
        DWORD cbSize = sizeof(TOKEN_ELEVATION);
        if (GetTokenInformation(hToken, TokenElevation, &Elevation, sizeof(Elevation), &cbSize))
        {
            fRet = Elevation.TokenIsElevated != FALSE;
        }

        TOKEN_ELEVATION_TYPE elevationType;
        if (GetTokenInformation(hToken, TokenElevationType, &elevationType, sizeof(elevationType), &cbSize))
        {
            switch (elevationType)
            {
            case TokenElevationTypeDefault:
                break;
            case TokenElevationTypeFull:
                break;
            case TokenElevationTypeLimited:
                break;
            }
        }

    }

    if (hToken)
    {
        CloseHandle( hToken );
    }

    return fRet;
}

#if 0
//-----------------------------------------------------------------------------
// http://social.msdn.microsoft.com/Forums/en-US/vcgeneral/thread/187d190d-95e4-46f8-a6bd-4f8f089b6463/
void WhoUsesFile( LPCTSTR lpFileName, BOOL bFullPathCheck )
{
    BOOL bShow = FALSE;
    CString name;
    CString processName;
    CString deviceFileName;
    CString fsFilePath;
    SystemProcessInformation:YSTEM_PROCESS_INFORMATION* p;
    SystemProcessInformation pi;
    SystemHandleInformation hi;

    if ( bFullPathCheck )
    {
        if ( !SystemInfoUtils::GetDeviceFileName( lpFileName, deviceFileName ) )
        {
            _tprintf( _T("GetDeviceFileName() failed.\n") );
            return;
        }
    }

    hi.SetFilter( _T("File"), TRUE );

    if ( hi.m_HandleInfos.GetHeadPosition() == NULL )
    {
        _tprintf( _T("No handle information\n") );
        return;
    }

    pi.Refresh();

    _tprintf( _T("%-6s  %-20s  %s\n"), _T("PID"), _T("Name"), _T("Path") );
    _tprintf( _T("------------------------------------------------------\n") );

    for ( POSITION pos = hi.m_HandleInfos.GetHeadPosition(); pos != NULL; )
    {
        SystemHandleInformation:YSTEM_HANDLE& h = hi.m_HandleInfos.GetNext(pos);

        if ( pi.m_ProcessInfos.Lookup( h.ProcessID, p ) )
        {
            SystemInfoUtils::Unicode2CString( &p->usName, processName );
        }
        else
            processName = "";

        //NT4 Stupid thing if it is the services.exe and I call GetName (
        if ( INtDll:wNTMajorVersion == 4 && _tcsicmp( processName, _T("services.exe" ) ) == 0 )
            continue;

        hi.GetName( (HANDLE)h.HandleNumber, name, h.ProcessID );

        if ( bFullPathCheck )
            bShow =    _tcsicmp( name, deviceFileName ) == 0;
        else
            bShow =    _tcsicmp( GetFileNamePosition(name), lpFileName ) == 0;

        if ( bShow )
        {
            if ( !bFullPathCheck )
            {
                fsFilePath = "";
                SystemInfoUtils::GetFsFileName( name, fsFilePath );
            }

            _tprintf( _T("0x%04X  %-20s  %s\n"),
                h.ProcessID,
                processName,
                !bFullPathCheck ? fsFilePath : lpFileName );
        }
    }
}

#endif

}
