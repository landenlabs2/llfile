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

// Dump the cluster of a file and insert into dirlist
// fragmented files are specific handled
struct DICT *DumpFile(char *Path, char *filename, __int64 Bytes) {
    PGET_RETRIEVAL_DESCRIPTOR   fileMappings;
    IO_STATUS_BLOCK             ioStatus;
    HANDLE                      sourceFile;
    DWORD                       status;
    ULONGLONG                   startVcn;
    ULONGLONG                   numClusters=0;
    ULONGLONG                   sLcn=0;
    ULONGLONG                   lastLcn=0;
    ULONGLONG                   lastLen=0;
    ULONGLONG                   frags=0;
    ULONGLONG                   sLen=0;
    int                         i;
    BOOL                        compressed = FALSE;
    struct DICT                 *firstFrag=NULL;
    char                        buff[MAX_PATH];
    
    // Open the file
    sourceFile = CreateFile(filename,
    FILE_READ_ATTRIBUTES,
    FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
    NULL, OPEN_EXISTING,
    FILE_FLAG_NO_BUFFERING, NULL);
    
    if( sourceFile == INVALID_HANDLE_VALUE ) {
        sourceFile = CreateFile(filename,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL, OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS, NULL);
        
        if( sourceFile == INVALID_HANDLE_VALUE ) {
            InsDIR(filename, Bytes);
            dirlist->fragments = LLINVALID;
            cLocked++;
            wsprintf(buff, "%s files (%d locked)",
            fmtNumber(cFiles, buff), cLocked);
            UpdateStatusBar(buff, 2, 0);
            return dirlist;
        }
    }
    
    // dump mapping information of file until we hit the EOF
    startVcn = 0;
    fileMappings = (PGET_RETRIEVAL_DESCRIPTOR) FileMap;
    while( !(status = NtFsControlFile( sourceFile, NULL, NULL, 0, &ioStatus,
    FSCTL_GET_RETRIEVAL_POINTERS,
    &startVcn, sizeof( startVcn ),
    fileMappings, FILEMAPSIZE * sizeof(LARGE_INTEGER) ) ) ||
    status == STATUS_BUFFER_OVERFLOW ||
    status == STATUS_PENDING ) {
        
        // If the operation is pending, wait for it to finish
        if( status == STATUS_PENDING ) {
            WaitForSingleObject( sourceFile, INFINITE );
            // Get the status from the status block
            if( ioStatus.Status != STATUS_SUCCESS &&
            ioStatus.Status != STATUS_BUFFER_OVERFLOW ) {
                //PrintNtError( ioStatus.Status );
                // some error, so make this a locked file
                InsDIR(filename, Bytes);
                dirlist->fragments = LLINVALID;
                cLocked++;
                wsprintf(buff, "%s files (%d locked)",
                fmtNumber(cFiles, buff), cLocked);
                UpdateStatusBar(buff, 2, 0);
                return dirlist;
            }
        }
        
        // Loop through the buffer of number/cluster pairs
        startVcn = fileMappings->StartVcn;
        for( i = 0; i < (ULONGLONG) fileMappings->NumberOfPairs; i++ ) {
            // On NT 4.0, a compressed virtual run (0-filled) is
            // identified with a cluster offset of -1
            if( fileMappings->Pair[i].Lcn == LLINVALID ) {
                //printf("   VCN: %I64d VIRTUAL LEN: %I64d\n",
                //  startVcn, fileMappings->Pair[i].Vcn - startVcn );
                compressed = TRUE;
                } else {
                //printf("   VCN: %I64d LCN: %I64d LEN: %I64d\n",
                //  startVcn, fileMappings->Pair[i].Lcn, fileMappings->Pair[i].Vcn - startVcn );
                
                // number of clusters from file in this pair[i]
                numClusters = fileMappings->Pair[i].Vcn - startVcn;
                sLen += numClusters;
                
                // test this cluster if it follows the lastLcn
                // this also happens on the first one because lastLcn is 0
                if ( lastLcn + lastLen != fileMappings->Pair[i].Lcn &&
                !compressed ) // yes, this file/dir is fragmented !!!
                frags++;
                
                if ( frags > 1 &&
                    fileMappings->Pair[i].Lcn != LLINVALID) {
                //&& Bytes != IINVALID ) {
                    if (frags == 2) // remember the first frag
                    firstFrag = InsDIR(filename, Bytes);
                    else            //insert frags that are greater than 2 with fragments = 0
                    InsDIR(filename, Bytes);
                    dirlist->fragmented = TRUE;
                    dirlist->fragments = 0; // will be set later for firstFrag
                    if (lastLen != 0) {
                        dirlist->Lcn = lastLcn;
                        dirlist->Len = lastLen;
                        } else {
                        dirlist->Lcn = fileMappings->Pair[i].Lcn;
                        dirlist->Len = numClusters;
                    }
                }
                
                // show fragmented clusters now in red,
                // because we lose positions
                if (fileMappings->Pair[i].Lcn != LLINVALID &&
                    frags > 1 ) {
                    if (lastLen != 0)
                    DrawBlocks(lastLcn, lastLcn+lastLen, hpRed, hbRed);
                    //DrawBlocks(fileMappings->Pair[i].Lcn,
                    //           fileMappings->Pair[i].Lcn+numClusters, hpRed, hbRed);
                }
                
                // now remember actual cluster for fragment-recognizing
                lastLcn = fileMappings->Pair[i].Lcn;
                lastLen = numClusters;   // on NTFS?
                //lastLen = numClusters-1; // on FAT?
                if (sLcn == 0)
                sLcn = lastLcn;
                
            }
            
            // goto next fragment of file
            startVcn = fileMappings->Pair[i].Vcn;
        }
        // for
        
        //fprintf(datei1, "%d\t%s\n", frags, fileName);
        // If the buffer wasn't overflowed, then we're done
        if( !status ) break;
        
    } // while something found
    CloseHandle( sourceFile );
    
    InsDIR(filename, Bytes);
    dirlist->Lcn = sLcn;
    dirlist->fragments = frags;
    dirlist->Len   = sLen;
    
    if (compressed) {
        dirlist->compressed = TRUE;
        cCompressed++;
        } else {
        if (frags > 1 && sLcn != 0) {
            dirlist->Lcn = lastLcn;
            dirlist->fragmented = TRUE;
            dirlist->fragments = 0;
            dirlist->Len = lastLen;
            
            cFragmented++;
            cFragmentsAllTogether+=(unsigned long)frags;
            
            InsDIR(filename, Bytes);
            dirlist->fragmented = TRUE;
            dirlist->fragments = frags;
            dirlist->Lcn = firstFrag->Lcn;
            dirlist->Len = sLen;
            
            wsprintf(buff, "%d fragmented", cFragmented);
            UpdateStatusBar(buff, 3, 0);
            // return the first frag, dirlist is untouched
        }
    }
    
    //(stricmp(FileName,"System Volume Information") == 0) ||
    //(stricmp(fileName,"C:/Documents and Settings/Administrator/ntuser.dat.LOG") == 0) ||
    // make these files locked
    /*
    if ( lstrcmpi(Path, cSysDir2) == 0 ) {
        int i=strlen(filename)-1, j=0;
        for (i=i; filename[i] != '\\'; i--)
        buff[j++] = filename[i];
        buff[j] = '\0'; strrev(buff);
        if( (lstrcmpi(buff, "AppEvent.Evt") == 0) ||
        (lstrcmpi(buff, "default") == 0) ||
        (lstrcmpi(buff, "default.LOG") == 0) ||
        (lstrcmpi(buff, "SAM") == 0) ||
        (lstrcmpi(buff, "SAM.LOG") == 0) ||
        (lstrcmpi(buff, "SecEvent.Evt") == 0) ||
        (lstrcmpi(buff, "SECURITY") == 0) ||
        (lstrcmpi(buff, "SECURITY.LOG") == 0) ||
        (lstrcmpi(buff, "software") == 0) ||
        (lstrcmpi(buff, "software.LOG") == 0) ||
        (lstrcmpi(buff, "SysEvent.Evt") == 0) ||
        (lstrcmpi(buff, "system") == 0) ||
        (lstrcmpi(buff, "SYSTEM.ALT") == 0) ||
        (lstrcmpi(buff, "system.LOG") == 0) ||
        (lstrcmpi(buff, "pagefile.sys") == 0) ) {
            if (firstFrag != NULL)
            firstFrag->fragments = LLINVALID;
            else
            dirlist->fragments = LLINVALID;
            cLocked++;
            wsprintf(buff, "%s files (%d locked)",
            fmtNumber(cFiles, buff), cLocked);
            UpdateStatusBar(buff, 2, 0);
        }
    }
    */

    if ( compressed )                          // compressed in ocker
        DrawBlocks(sLcn, sLcn+sLen, hpOck, hbOck);
    else if (Bytes == IINVALID)                   // dirs in blue
        DrawBlocks(sLcn, sLcn+sLen, hpBlue, hbBlue);
    else if (Bytes>sizeLimit && frags<2)
        DrawBlocks(sLcn, sLcn+sLen, hpDGreen, hbDGreen);
    else if (bShowMarks && sLen >= clustersPB*1 &&
             dirlist->fragments != LLINVALID) // large files
        DrawMarks(sLcn, sLcn+sLen);
    
    if (firstFrag != NULL)
        return firstFrag;
    
    return dirlist;
}