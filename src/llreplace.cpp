//-----------------------------------------------------------------------------
// llreplace - Replace file data or Grep.
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



#include <iostream>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <io.h>
#include <map>

#include <algorithm>
#define ZipLib
#ifdef ZipLib
// https://bitbucket.org/wbenny/ziplib/wiki/Home
#include "../ZipLib/ZipFile.h"
// #pragma comment(lib, "zlib.lib")
// #pragma comment(lib, "lzma.lib")
// #pragma comment(lib, "bzip2.lib")
#endif

#include "LLReplace.h"
#include "MemMapFile.h"

#ifdef ZipLib
int LLReplace::ZipListArchive(const char* zipArchiveName)
{
    // ZipArchive::Ptr archive = ZipFile::Open(zipArchiveName);

    std::ifstream* zipFile = new std::ifstream();
    zipFile->open(zipArchiveName, std::ios::binary);
    if (!zipFile->is_open())
        return -1;
    ZipArchive::Ptr archive = ZipArchive::Create(zipFile, true);
    if (archive == nullptr)
        return -1;

    size_t entries = archive->GetEntriesCount();

    if (m_verbose)
    {
        LLMsg::Out() << zipArchiveName << ", Entries:" << entries << std::endl;
        LLMsg::Out() << archive->GetComment() << std::endl;
    }

    for (size_t idx = 0; idx < entries; ++idx)
    {
        auto entry = archive->GetEntry(int(idx));
		if (m_zipList.size() == 1 && m_zipList[0] == "-")
		{
			std::stringstream in(entry->GetFullName());
			m_matchCnt += FindGrep(in);
			m_totalInSize += entry->GetSize();
			m_countInFiles++;
		}
        else if (LLSup::PatternListMatches(m_zipList, entry->GetFullName().c_str(), true))
        {
            if (m_verbose)
            {
                LLMsg::Out() << std::setw(3) << idx << ":"
                    << std::setw(8) << entry->GetSize() << " "
                    << entry->GetFullName() << std::endl;
                /*
                uncompressed size   entry->GetSize());
                compressed size:    entry->GetCompressedSize());
                password protected: entry->IsPasswordProtected() ? "yes" : "no");
                compression method: entry->GetCompressionMethod()
                comment:            entry->GetComment()
                crc32:              entry->GetCrc32());
                */
            }


            std::istream* decompressStream = entry->GetDecompressionStream();
#if 1
            if (decompressStream != nullptr)
            {
                m_matchCnt += FindGrep(*decompressStream);
                m_totalInSize += entry->GetSize();
                m_countInFiles++;
            }
#else
            std::string line;
            while (std::getline(*decompressStream, line))
            {
                LLMsg::Out() << line << std::endl;
            }
#endif
        }
    }

    return sOkay;
}

int LLReplace::ZipReadFile(
    const char* zipFilename,
    const char* fileToExtract,
    const char* password)
{
    ZipArchive::Ptr archive = ZipFile::Open(zipFilename);

    ZipArchiveEntry::Ptr entry = archive->GetEntry(fileToExtract);
    assert(entry != nullptr);

    entry->SetPassword(password);
    std::istream* decompressStream = entry->GetDecompressionStream();
    assert(decompressStream == nullptr);

    std::string line;
    while (std::getline(*decompressStream, line))
    {
        LLMsg::Out() << line << std::endl;
    }

    return sIgnore; 
}
#endif
// ---------------------------------------------------------------------------
// Add regular expression to help document
// https://msdn.microsoft.com/en-us/library/bb982727.aspx#regexgrammar
// http://www.cplusplus.com/reference/regex/ECMAScript/

static const char sHelp[] =
" Replace " LLVERSION "\n"
" Replace a file[s] data\n"
"\n"
"  !0eSyntax:!0f\n"
"    [<switches>] <Pattern>... \n"
"\n"
"  !0eWhere switches are:!0f\n"
"   -?                  ; Show this help\n"
"   -A=[nrhs]           ; Limit files by attribute (n=normal r=readonly, h=hidden, s=system)\n"
"   -D                  ; Only directories in matching, default is all types\n"
"   -p                  ; Search PATH environment directories for pattern\n"
"   -e=<envName>[,...]  ; Search env environment directories for pattern\n"
"   -F                  ; Only files in matching, default is all types\n"
"   -F=<filePat>,...    ; Limit to matching file patterns \n"
"   -G=<grepPattern>    ; Return line matching grepPattern \n"
"   -g=<grepOptions>    ; Use with -G \n"
"                       ; Default is search entire file \n"
"                       ;     Ln=first n lines \n"
"                       ;     Mn=first n matches \n"
"                       ;     H(f|l|m|t) Hide filename|Line#|MatchCnt|Text \n"
"                       ;     I=ignore case \n"
"                       ;     R=repeat replace \n"
"                       ;     Bn=show before n lines \n"
"                       ;     An=show after n lines \n"
"                       ;     F(l|f) force byLine or byFile \n"
"                       ;     U(i|b) update inline or backup \n"
"   -i                  ; Ignore case, same as -g=I \n"
"   -I=<file>           ; Read list of files from this file\n"
"   -M=<file>           ; Match (and replace) list of patterns in file \n"
"                       ;  First Line Seperator:<char> like , \n"
"                       ;  Remainder <findPat><seperator><replacePat>[,<filePathPat>]  \n"
"   -p                  ; Short cut for -e=PATH, search path \n"
"   -P=<srcPathPat>     ; Optional regular expression pattern on source files full path\n"
"   -q                  ; Quiet, default is echo command\n"
"   -Q=n                ; Quit after 'n' file matches\n"
"   -r                  ; Recurse into subdirectories\n"
"   -R=<replacePattern> ; Use with -G to replace match\n"
"   -Rbefore=<pattern>  ; TODO Use with -R to move a replacement\n"
"   -Rafter=<pattern>   ; TODO Use with -R to move a replacement\n"
"   -s                  ; Show file size size\n"
"   -t[acm]             ; Show Time a=access, c=creation, m=modified, n=none\n"
"   -X=<pathPat>,...    ; Exclude patterns  -X=*.lib,*.obj,*.exe\n"
"                       ;  No space in patterns. Pattern applied against fullpath\n"
"                       ;  So *\\ma will exclude a directory ma or file ma \n"
"   -v                  ; Verbose \n"
"   -V=<grepPattern>    ; Return inverse line matching grep matches \n"
"   -w=<width>          ; Limit output to width characters per match \n"
"   -z=<filePattern>    ; Limit zip/jar/gz file match, use - to search names \n"
"\n"
"   -E=[cFDdsamlL]       ; Return exit code, c=File+Dir count, F=file count, D=dir Count\n"
"                       ;    d=depth, s=size, a=age, m=#matches, l=#lines, L=list of matching files \n"
"   -1=<file>           ; Redirect output to append to file \n"
"   -3=<file>           ; Tee output to append to file \n"
"\n"
"  !0eWhere Pattern is:!0f\n"
"    <file|Pattern> \n"
"    [<directory|pattern> \\]... <file|Pattern|#n> \n"
"\n"
"  !0ePattern:!0f\n"
"      * = zero or more characters\n"
"      ? = any character\n"
"\n"
"  !0eExample:!0f\n"
"    llfile -xG \"-G=(H|h)ello\\r\\n\" *.txt     ; -xG force grep command\n"
"    lg '-G=String' -g=I -r -F=*.cpp src     ; Ignore case  String in cpp files \n"
"    lg -Ar -i=c:\\fileList.txt >nul          ; check for non-readonly files from list \n"
"\n"
"    lg -F=*.txt,*.log '-G=[0-9]+' .\\*       ; search files for numbers \n"
"    lg -X=*.obj,*.exe -G=foo build\\*        ; search none object or exe files\n"
"    lg -z=* -G=class java\\*.jar             ; search jar internal files for class \n"
"    lg -z=foo* -G=class java\\*.jar          ; search jar internal foo* files for class \n"
"    lg -z=- -G=class java\\*.jar             ; search filenames in jar files for class \n"
"    lg -z   -G=hello -r libs                ; search files for hello and look inside any archive \n"
"    lg \"-G= foo \" *.txt | lg -G=bar         ; Same as following\n"
"    lg \"-G= foo \" -G=bar *.txt              ;  two -G, either can match per line\n"
"\n"
"     ; Use -E=L to match multiple words in same file but not same line \n"
"    lg -G=foo -r -F=*.txt -E=L | lg -I=- -G=bar  \n"
"\n"
"    lg -G=\"'([^']+)',.*\" -R=\"$1');\" foo.txt ; Grep and Replace \n"
"    lg -G=\\n\\n  -R=\\n -g=R   foo.txt      ; remove blank lines \n"
"  !0ePattern across multiple lines:!0f\n"
"    The pattern engine does not match line terminators with . so use some group\n"
"    which does not appear in the normal text, like [^?]* \n"
"    Example to remove XML tag group. \n"
"     \"-G= *<SurveySettings\\>[^?]*\\</SurveySettings>\" \"-R=\" file.xml \n"
"\n";

LLReplaceConfig LLReplace::sConfig;
 
WORD FILE_COLOR = FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN |  FOREGROUND_BLUE;
WORD MATCH_COLOR = FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN;
WORD MATCH_COLORS[] = 
{
    FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN,
    FOREGROUND_INTENSITY | FOREGROUND_RED,
    FOREGROUND_INTENSITY | FOREGROUND_GREEN,
    FOREGROUND_INTENSITY | FOREGROUND_GREEN | FOREGROUND_BLUE,
};

const char sForceByLine = 'l';
const char sForceByFile = 'f';

///////////////////////////////////////////////////////////////////////////////
// Expands c-style character constants in the input string; returns new size
// (strlen won't work, since the string may contain premature \0's)

static std::string ConvertSpecialChar(std::string& inOut)
{
	int len = 0;
    int x, n;
	const char *inPtr = inOut.c_str();
    char* outPtr = (char*)inPtr;
	while (*inPtr)
	{
		if (*inPtr == '\\')
		{
			inPtr++;
			switch (*inPtr)
			{
			case 'n': *outPtr++ = '\n'; break;
			case 't': *outPtr++ = '\t'; break;
			case 'v': *outPtr++ = '\v'; break;
			case 'b': *outPtr++ = '\b'; break;
			case 'r': *outPtr++ = '\r'; break;
			case 'f': *outPtr++ = '\f'; break;
			case 'a': *outPtr++ = '\a'; break;
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
				sscanf(inPtr,"%3o%n",&x,&n);
				inPtr += n-1;
				*outPtr++ = (char)x;
				break;
			case 'x':								// hexadecimal
				sscanf(inPtr+1,"%2x%n",&x,&n);
				if (n>0)
				{
					inPtr += n;
					*outPtr++ = (char)x;
					break;
				}
				// seep through
			default:  
				throw( "Warning: unrecognized escape sequence" );
			case '\\':
			case '\?':
			case '\'':
			case '\"':
				*outPtr++ = *inPtr;
			}
			inPtr++;
		}
		else
			*outPtr++ = *inPtr++;
		len++;
	}

    inOut.resize(len);
	return inOut;;
}


// ---------------------------------------------------------------------------
// Return true if character in pattern string is part of regular expression
// and not escaped out or inside a closour group []
static bool isPattern(std::string str, int pos)
{
    if (pos < 0)
        return false;
    if (pos == 0)
        return true;
    if (str[pos-1] == '\\')
        return false;
    while (--pos >= 0)
    {
        if (str[pos] == ']')
            return true;
        if (str[pos] == '[')
            return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
LLReplace::LLReplace() :
    m_force(false),
    m_totalInSize(0),
    m_countInFiles(0),
	m_lineCnt(0),
    m_matchCnt(0),
    m_width(0),
    m_zipFile(false)
{
    m_exitOpts = "c";
    m_showAttr  =
        m_showCtime =
        m_showMtime =
        m_showAtime =
        m_showPath =
        m_showSize =
        m_allMustMatch = false;

    memset(&m_fileData, 0, sizeof(m_fileData));
    m_dirScan.m_recurse = false;

    sConfigp = &GetConfig();
}

// ---------------------------------------------------------------------------
LLConfig& LLReplace::GetConfig() 
{
    return sConfig;
}

// ---------------------------------------------------------------------------
int LLReplace::StaticRun(const char* cmdOpts, int argc, const char* pDirs[])
{
    LLReplace llFind;
    return llFind.Run(cmdOpts, argc, pDirs);
}

// ---------------------------------------------------------------------------
int LLReplace::Run(const char* cmdOpts, int argc, const char* pDirs[])
{
    const char missingRetMsg[] = "Missing return value, c=file count, d=depth, s=size, a=age, syntax -0=<code>";
    const char missingEnvMsg[] = "Environment variable name, syntax -E=<envName> ";
    const char optReplaceMsg[] = "Replace matches (see -G=grepPattern) with replacement, -R=<replacement>";
    const char optGrepMsg[] = "Find in file grepPattern, -G=<grepPattern>" ;
    const char missingRepGrp[] = "Replace (-R=replace) must follow Grep (-G=patttern)";
    const char widthErrMsg[] = "missing width, syntax -w=<#width>";

    const std::string endPathStr(";");
    LLSup::StringList envList;
    std::string str;

    // Initialize stuff
    size_t nFiles = 0;
    bool sortNeedAllData = m_showSize;

#if 0
    // Setup default as needed.
    if (argc == 0 && strstr(cmdOpts, "I=") == 0)
    {
        const char* sDefDir[] = {"*"};
        argc  = sizeof(sDefDir)/sizeof(sDefDir[0]);
        pDirs = sDefDir;
    }
#endif

    std::string matchFilename;
    GrepReplaceItem grepRep;
    unsigned findCnt = 0;
    unsigned replaceCnt = 0;

    // Parse options
    while (*cmdOpts)
    {
        grepRep.m_onMatch = true;
        switch (*cmdOpts)
        {
        case 'C':  // -C=none or -C=0 disable colors
            if (cmdOpts[1] == sEQchr)
            {
                cmdOpts += 2;
                switch (ToLower(*cmdOpts))
                {
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
        case 'i':   // ignore case, same as =g=I
            m_grepOpt.ignoreCase = true;
            break;

        case 'V':   // inverse grep pattern  -V=<grepPattern>, show if lines does not contain grepPattern
            grepRep.m_onMatch = false;
            m_byLine = true;
            m_allMustMatch = true;
        case 'G':   // grep pattern  -G=<grepPattern>, show if line contains grepPattern
            cmdOpts = LLSup::ParseString(cmdOpts+1, str, optGrepMsg);
            if (str.length() != 0) {
                if (replaceCnt != 0 && findCnt != replaceCnt)
                {
                    LLMsg::PresentError(0, missingRepGrp, "\n");
                    return sError;
                }

                try {
                    findCnt++;
                    grepRep.m_grepLinePat = grepRep.m_grepLineStr = str;   
                    m_grepReplaceList.push_back(grepRep);
                    // If pattern has explict test for beginning or end of line
                    // process search/replace byLine rather then byEntireFile.
                    if (!m_byLine && isPattern(str, str.find('^')))
                        m_byLine = true;
                    if (!m_byLine && isPattern(str, str.find('$')))
                        m_byLine = true;
                    if (!m_byLine && isPattern(str, str.find('(')))
                        m_backRef = true;
                } 
                catch (std::regex_error& ex)
                {
                    LLMsg::PresentError(ex.code(), ex.what(), str.c_str());
                    return sError;
                }
            }
            break;

        case 'R':   // Get Replacement string
			if (isalpha(cmdOpts[1]))
			{
				// Check for special case -Rafter=... or -Rbefore=...
				const char* eqPos = strchr(cmdOpts+2, '=');
				if (eqPos != NULL)
				{
					int len = int(eqPos - cmdOpts + 2);
					if (_strnicmp(cmdOpts+2, "after", len) == 0)
					{
						cmdOpts = LLSup::ParseString(cmdOpts+1, str, optReplaceMsg);
						m_grepReplaceList.back().m_afterStr = str;
						m_grepOpt.force ='l';		// force by-line
						break;
					}
					else if (_strnicmp(cmdOpts+2, "before", len) == 0)
					{
						cmdOpts = LLSup::ParseString(cmdOpts+1, str, optReplaceMsg);
						m_grepReplaceList.back().m_beforeStr = str;
						m_grepOpt.force ='l';		// force by-line
						break;
					}
				}
			}

            cmdOpts = LLSup::ParseString(cmdOpts+1, str, optReplaceMsg);
            if (replaceCnt + 1 == findCnt)
            {
                replaceCnt++;
                m_grepReplaceList.back().m_replace = true;
                m_grepReplaceList.back().m_replaceStr = ConvertSpecialChar(str);
            }
            else
            {
                LLMsg::PresentError(0, missingRepGrp, "\n");
                return sError;
            }
            break;

        case 'm':   // Reverse match using file list
            grepRep.m_onMatch = false;
            m_allMustMatch = true;

        case 'M':   // Match file list
            cmdOpts = LLSup::ParseString(cmdOpts+1, matchFilename, missingInFileMsg);
            if (matchFilename.length() != 0)
            {
                FILE* fin = stdin;

                if (strcmp(matchFilename.c_str(), "-") == 0 ||
                    0 == fopen_s(&fin, matchFilename.c_str(), "rt"))
                {
                    const char sepTag[] = "Seperator:";
                    const unsigned sepLen = sizeof(sepTag) - 1;
                    char userSep[] = ",";
                    char lineBuf[MAX_PATH];
                    if (fgets(lineBuf, ARRAYSIZE(lineBuf), fin))
                    {
                        unsigned lineLen = strlen(lineBuf);
                        if (strncmp(sepTag, lineBuf, sepLen) == 0 && lineLen > sepLen)
                        {
                            userSep[0] = lineBuf[sepLen];   // TODO - support multiple separators
                            if (!fgets(lineBuf, ARRAYSIZE(lineBuf), fin))
                                return sError;
                        }
                        else
                        {
                            LLMsg::PresentError(0, "Match file should start with Seperator:<char>", " Assuming comma separator \n");
                            // return sError;
                        }

                        do
                        {
                            // TrimString(lineBuf);       // remove extra space or control characters.
                            if (*lineBuf == '\0')
                                continue;
                            unsigned len = strlen(lineBuf);
                            lineBuf[len - 1] = '\0';     // remove EOL

                            Split fields(lineBuf, userSep);
                            if (fields.size() > 0) 
                            {
                                if (replaceCnt != 0 && findCnt != replaceCnt)
                                {
                                    LLMsg::PresentError(0, missingRepGrp, "\n");
                                    return sError;
                                }
                                findCnt++;
                                grepRep.m_grepLinePat = grepRep.m_grepLineStr = fields[0]; 
                                if (fields.size() > 1)
                                {
                                    replaceCnt++;
                                    grepRep.m_grepLinePat = ConvertSpecialChar(fields[1]);  // Should this be the replacement pattern ?
                                }
                                if (fields.size() > 2)
                                {
                                    grepRep.m_filePathPat = std::tr1::regex(fields[2], regex_constants::icase);
                                    grepRep.m_haveFilePat = true;
                                }
                                m_grepReplaceList.push_back(grepRep);
                            }
                        } while (fgets(lineBuf, ARRAYSIZE(lineBuf), fin));

                        std::cerr << " Grep patterns:" << m_grepReplaceList.size() << std::endl;
                    }
                }
            }
            break;

        case 's':   // Toggle showing size.
            sortNeedAllData |= m_showSize = !m_showSize;
            // m_dirSort.SetSortData(sortNeedAllData);
            break;

        case 't':   // Display Time selection
            {
                bool unknownOpt = false;
                while (cmdOpts[1] && !unknownOpt)
                {
                    cmdOpts++;
                    switch (ToLower(*cmdOpts))
                    {
                    case 'a':
                        sortNeedAllData = m_showAtime = true;
                        break;
                    case 'c':
                        sortNeedAllData = m_showCtime = true;
                        break;
                    case 'm':
                        sortNeedAllData = m_showMtime = true;
                        break;
                    case 'n':   // NoTime
                        sortNeedAllData  = false;
                        m_showAtime = m_showCtime = m_showMtime = false;
                        break;
                    default:
                        cmdOpts--;
                        unknownOpt = true;
                        break;
                    }
                }
                // m_dirSort.SetSortData(sortNeedAllData);
            }

            break;

        case 'z':   // Limit zip files, -z or z=<filePat>[,<filePat>]...
            cmdOpts = LLSup::ParseList(cmdOpts + 1, m_zipList, NULL);
            m_zipFile = true;
            break;

        case 'w':   // Width
            cmdOpts = LLSup::ParseNum(cmdOpts+1, m_width, widthErrMsg);
            break;

        case '?':
            Colorize(std::cout, sHelp);
            return sIgnore;
        case 'E':
            if ( !ParseBaseCmds(cmdOpts))
                return sError;
            if (m_exitOpts.find_first_of("L") != string::npos)
                m_grepOpt.hideFilename = m_grepOpt.hideLineNum =  m_grepOpt.hideMatchCnt = m_grepOpt.hideText = true;
			if (m_exitOpts.find_first_of("l") != string::npos)
				m_grepOpt.force = sForceByLine;
            break;

        default:
            if ( !ParseBaseCmds(cmdOpts))
                return sError;
        }

        // Advance to next parameter
        LLSup::AdvCmd(cmdOpts);
    }

    if (m_grepOpt.ignoreCase && !m_grepReplaceList.empty())
    {
        for (unsigned idx = 0; idx != m_grepReplaceList.size();  idx++)
        {
            GrepReplaceItem& grepReplaceItem = m_grepReplaceList[idx];
            grepReplaceItem.m_grepLinePat = 
                std::tr1::regex(grepReplaceItem.m_grepLineStr, regex_constants::icase);
        }
    }

    if (m_grepReplaceList.empty())
    {
        Colorize(std::cout, sHelp);
        return sIgnore;
    }

    // Move arguments and input files into inFileList.
    std::vector<std::string> inFileList;

    for (int argn=0; argn < argc; argn++)
    {
        inFileList.push_back(pDirs[argn]);
    }

    if (m_inFile.length() != 0)
    {
        FILE* fin = stdin;

        if (strcmp(m_inFile.c_str(), "-") == 0 ||
            0 == fopen_s(&fin, m_inFile.c_str(), "rt"))
        {
            char fileName[MAX_PATH];
            while (fgets(fileName, ARRAYSIZE(fileName), fin))
            {
                TrimString(fileName);       // remove extra space or control characters.
                if (*fileName == '\0')
                    continue;
                inFileList.push_back(fileName);
            }
        }
    }


    if (m_grepOpt.force != 0)
    {
        if (m_grepOpt.force == sForceByLine)
            m_byLine = true;
        else if (m_grepOpt.force == sForceByFile)
            m_byLine = false;
    }

    if (inFileList.empty())
    {
        // Grep from standard-in
		_setmode(_fileno(stdin), _O_BINARY);
		_setmode(_fileno(stdout), _O_BINARY);
        m_matchCnt += FindGrep(cin); 
    }
    else
    {
        // Iterate over dir patterns.
        for (unsigned  argn=0; argn < inFileList.size(); argn++)
        {
            VerboseMsg() <<" Dir:" << inFileList[argn] << std::endl;
            m_dirScan.Init(inFileList[argn].c_str(), NULL);

            nFiles += m_dirScan.GetFilesInDirectory();
        }
    }

    if (m_verbose)
    {
        // InfoMsg() << ";Matches:" << m_matchCnt << ", Files:" << m_countOutFiles << std::endl;
        SetColor(MATCH_COLOR);
        LLMsg::Out() << ";Matches:" << m_matchCnt << ", MatchFiles:" << m_countOutFiles 
            << ", ScanFiles:" << m_countInFiles 
            << std::endl;
        SetColor(sConfig.m_colorNormal);
    }

    // Return status, c=file count, d=depth, s=size, a=age
    if (m_exitOpts.length() != 0)
    switch ((char)m_exitOpts[0u])
    {
    case 'a':   // age,  -0=a
        {
        FILETIME   ltzFT;
        SYSTEMTIME sysTime;
        FileTimeToLocalFileTime(&m_fileData.ftLastWriteTime, &ltzFT);    // convert UTC to local Timezone
        FileTimeToSystemTime(&ltzFT, &sysTime);

        // TODO - compare to local time and return age.
        return 0;
        }
        break;
    case 'm':   // matchCnt -g=<grepPat>
        VerboseMsg() << ";Matches:" << m_matchCnt << std::endl;
        return (int)m_matchCnt;
    case 's':   // file size, -0=s
        VerboseMsg() << ";Size:" << m_totalInSize << std::endl;
        return (int)m_totalInSize;
    // case 'd':   // file depth, -o=d
    //    VerboseMsg() << ";Depth (-E=d depth currently not implemented,use -E=D for #directories):" << 0 << std::endl;
    //    return 0;    // TODO - return maximum file depth.
    // case 'D':   // Directory count, -o=D
    //     VerboseMsg() << ";Directory Count:" << m_countOutDir << std::endl;
    //     return m_countOutDir;
    case 'F':   // File count, -o=F
        VerboseMsg() << ";File Count:" << m_countOutFiles << std::endl;
        return m_countOutFiles;
    case 'l':   // #lines
		LLMsg::Out() << ";Line Count:" << m_lineCnt << std::endl;
		break;

    case 'L':   // List of matching files
        for (unsigned idx = 0; idx != m_matchFiles.size(); idx++)
            LLMsg::Out() << m_matchFiles[idx] << std::endl;
        return m_matchFiles.size();
    case 'c':   // File and directory count, -o=c
    default:
        VerboseMsg() << ";File + Directory Count:" << (m_countOutDir + m_countOutFiles) << std::endl;
        return (int)(m_countOutDir + m_countOutFiles);
    }

	return ExitStatus(0);
}

// ---------------------------------------------------------------------------
int LLReplace::ProcessEntry(
        const char* pDir,
        const WIN32_FIND_DATA* pFileData,
        int depth)      // 0...n is directory depth, -n end-of nth diretory
{
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

    VerboseMsg() << m_srcPath << "\n";

    if (m_isDir)
        return sIgnore;

    if (m_zipFile)
    {
        int matchStatus = ZipListArchive(m_srcPath);
        if (matchStatus >= 0)
            return matchStatus;
    }

    unsigned matchCnt = FindReplace(pFileData);
    if ( matchCnt != 0)
        m_matchFiles.push_back(m_srcPath);

#if 0
    if (m_echo && !IsQuit())
    {
        if (m_showAttr)
        {
            // ShowAttributes(LLMsg::Out(), pDir, *pFileData, false);
            LLMsg::Out() << LLReplace::sConfig.m_dirFieldSep;
        }
        if (m_showCtime)
            LLSup::Format(LLMsg::Out(), pFileData->ftCreationTime) << LLReplace::sConfig.m_dirFieldSep ;
        if (m_showMtime)
            LLSup::Format(LLMsg::Out(), pFileData->ftLastWriteTime) << LLReplace::sConfig.m_dirFieldSep;
        if (m_showAtime)
            LLSup::Format(LLMsg::Out(), pFileData->ftLastAccessTime) << LLReplace::sConfig.m_dirFieldSep;
        if (m_showSize)
            LLMsg::Out() << std::setw(LLReplace::sConfig.m_fzWidth) << m_fileSize << LLReplace::sConfig.m_dirFieldSep;

        LLMsg::Out() << m_srcPath << std::endl;
    }
#endif

    m_matchCnt += matchCnt;
    m_fileData = *pFileData;
    m_totalInSize += m_fileSize;
    m_countInFiles++;
    if (matchCnt != 0)
        m_countOutFiles++;

    return (matchCnt != 0) ? sOkay : sIgnore;
}

// ---------------------------------------------------------------------------
void LLReplace::OutFileLine(size_t lineNum, unsigned matchCnt, size_t filePos) 
{
    if (m_echo) 
    {
        SetColor(FILE_COLOR);
        if (!m_grepOpt.hideFilename)
            LLMsg::Out() << m_srcPath << ":";
        if (lineNum != 0 && !m_grepOpt.hideLineNum)
            LLMsg::Out() << lineNum << "L:";
        if (matchCnt != 0 && !m_grepOpt.hideMatchCnt)
            LLMsg::Out() << matchCnt << "M:";
        if (filePos != 0 && !m_grepOpt.hideLineNum)
            LLMsg::Out() << filePos << "P:";
        SetColor(sConfig.m_colorNormal);
        if (m_grepOpt.hideText && !(m_grepOpt.hideFilename && m_grepOpt.hideLineNum && m_grepOpt.hideMatchCnt))
            LLMsg::Out() << std::endl;
    }
}

// ---------------------------------------------------------------------------
unsigned LLReplace::FindGrep()
{
    unsigned matchCnt = 0;
    
    if (!m_grepReplaceList.empty())
    {
        std::regex_constants::match_flag_type flags = std::regex_constants::match_default;
        size_t lineCnt = 0;

        try
        {
            if (m_byLine || m_grepReplaceList.size() > 1) 
            {
                EnableFiltersForFile(m_srcPath);
                std::ifstream in(m_srcPath, std::ios::in, _SH_DENYNO);
                matchCnt += FindGrep(in);    
            }
            else
            {        
                MemMapFile mapFile;
                void* mapPtr;
                SIZE_T viewLength = INT_MAX;
                if (mapFile.Open(m_srcPath) && (mapPtr = mapFile.MapView(0, viewLength)) != NULL)
                {
                    std::tr1::match_results <const char*> match;
                    const char* begPtr = (const char*)mapPtr;
                    const char* endPtr = begPtr + viewLength;
                    const char* strPtr = begPtr;
                    std::tr1::regex grepLinePat = m_grepReplaceList[0].m_grepLinePat;
            
                    while (std::tr1::regex_search(strPtr, endPtr, match, grepLinePat, flags))
                    {
                        matchCnt++;
                        const char* begLine = match.prefix().second;
                        while (begLine -1 >= begPtr && begLine[-1] != '\n')
                            begLine--;
                        const char* endLine = match.suffix().first;
                        while (*endLine != '\n' && endLine < endPtr)
                            endLine++;

                        if (m_echo)
                        {
                            OutFileLine(0, matchCnt);

                            if (!m_grepOpt.hideText) 
                            {
                                do {
                                    std::string prefix = std::string(begLine, match.prefix().second);
                                    // std::string suffix = std::string(match.suffix().first, endLine);;
                                    LLMsg::Out() << prefix;
                                    SetColor(MATCH_COLOR);
                                    LLMsg::Out() << match.str();
                                    SetColor(sConfig.m_colorNormal);
                                    begLine = strPtr = match.suffix().first; 
                                } while (std::tr1::regex_search(strPtr, endLine, match, grepLinePat, flags));
                                std::string suffix = std::string(match.suffix().first, endLine);
                                LLMsg::Out() << suffix << std::endl;
                            }
                        }
                        strPtr = endLine;

                        if (matchCnt >= m_grepOpt.matchCnt)
                            break;
                    }
                }
                else
                {
                    LLMsg::PresentError(GetLastError(), "Open failed,", m_srcPath);
                }
            }
        }
        catch (...)
        {
        }
		m_lineCnt += lineCnt;
    }

    return matchCnt;
}

// ---------------------------------------------------------------------------
struct ColorInfo
{
    uint len;
    WORD color;
    ColorInfo() : 
        len(0), color(0)
    { }
    ColorInfo(uint _len, WORD _color) :
         len(_len), color(_color)
    { }
};
typedef  std::map<uint, ColorInfo> ColorMap;

 
// ---------------------------------------------------------------------------
// Determine if input stream is binary.
class BinaryState
{
public:
    size_t binaryCnt = 0;
    size_t printCnt = 0;
    const size_t minCnt = 1024;

    bool isBinary(const std::string& str)
    {
        for (unsigned idx = 0; idx != str.length(); idx++)
        {
            char c = str[idx];
            if (isprint(c) || c == '\r' || c == '\n' || c == '\t')
                printCnt++;
            else
                binaryCnt++;

            if (binaryCnt + printCnt > minCnt)
                break;
        }
        return binaryCnt > printCnt;
    }
};

// ---------------------------------------------------------------------------
unsigned LLReplace::FindGrep(std::istream& in)
{
    unsigned matchCnt = 0;
    unsigned lineCnt = 0;
    std::tr1::smatch match;
    std::regex_constants::match_flag_type flags = std::regex_constants::match_default;
	std::vector<string> beforeLines(m_grepOpt.beforeCnt);
	
	unsigned addBeforeIdx = 0;
	unsigned afterLines = 0;

    BinaryState binaryState;

    std::string str;
    while (std::getline(in, str))
    {
        lineCnt++;
	
        if (binaryState.isBinary(str))
        {
            if (m_verbose)
                LLMsg::Out() << "Ignore Binary\n";
            return matchCnt;
        }

        // All patterns have to match for the line to match.
        ColorMap  colorMap;
        unsigned itemMatchCnt = 0;
        std::tr1::regex grepLinePat;
        for (unsigned patIdx = 0; patIdx != m_grepReplaceList.size(); patIdx++)
        {
            GrepReplaceItem& grepRepItem = m_grepReplaceList[patIdx];
            if (grepRepItem.m_enabled)
            {
                bool itemMatches = false;
                grepLinePat = grepRepItem.m_grepLinePat;
                std::string replaceStr = grepRepItem.m_replaceStr;
                if (grepRepItem.m_replace)
                {
                    // Loop to get multiple matches on a line.
                    size_t off = 0;
                    do 
                    {
                        std::string::const_iterator begIter = str.begin();
                        std::string::const_iterator endIter = str.end();
                        std::advance(begIter, off);
						 
                        if (begIter < endIter && 
                            std::tr1::regex_search(begIter, endIter, match, grepLinePat, flags|std::regex_constants::format_first_only))
                        {
							std::string subStr = str.substr(off);
							std::string newStr = std::regex_replace(subStr, grepLinePat, replaceStr, flags|std::regex_constants::format_first_only);
                            int repLen = match.length() + newStr.length() - str.length();
							if (newStr != subStr)
							{
								// str.swap(newStr);
								unsigned begPos = off + match.position();
								str.replace(begPos, str.length() - begPos, newStr, match.position(), newStr.length() - match.position());
     
                                 itemMatches = true;
                                 if (repLen > 0)
                                     colorMap[(uint)match.position()] = ColorInfo((uint)repLen, MATCH_COLORS[patIdx % ARRAYSIZE(MATCH_COLORS)]);
                                 else
                                     colorMap[0] = ColorInfo(str.length(), MATCH_COLORS[patIdx % ARRAYSIZE(MATCH_COLORS)]);
                                 // std::advance (begIter, grepLinePat.length());
                                 // off += grepLinePat.length();
								off += match.position() + 1;
							} else 
								off = 0;

                        } else
							off = 0;
                    } while (off != 0);
                }
                else if (grepRepItem.m_onMatch)
                {
                    // Loop to get multiple matches on a line.
                    std::string::const_iterator begIter = str.begin();
                    std::string::const_iterator endIter = str.end();
                    size_t off = 0;
                    while (off < str.length() && 
                        std::tr1::regex_search(begIter, endIter, match, grepLinePat, flags))
                    {
                        itemMatches = true;
                        colorMap[uint(match.position() + off)] = ColorInfo((uint)match.length(), MATCH_COLORS[patIdx % ARRAYSIZE(MATCH_COLORS)]);
                        std::advance (begIter, match.length());
                        off += match.length();
                    }
                } 
                else
                {
                    // Reverse match
                    if (std::tr1::regex_search(str, match, grepLinePat, flags) == false)
                    {
                        itemMatches = true;
                        if (m_grepReplaceList.size() == 1)
                            colorMap[0] = ColorInfo(str.length(), MATCH_COLORS[patIdx % ARRAYSIZE(MATCH_COLORS)]);
                    }
                }

                if (itemMatches)
                    itemMatchCnt++;
            }
        }

        if (m_allMustMatch && itemMatchCnt != m_grepReplaceList.size())
            colorMap.clear();
        else if (m_allMustMatch && colorMap.size() == 0)
            LLMsg::Out() << str << std::endl;

        if (colorMap.size() != 0)
        {
            matchCnt++;

            if (m_echo)
            {
                if (m_width != 0)
                {
                    // Clamp output to user desired width.
                    for (ColorMap::iterator iter = colorMap.begin(); iter != colorMap.end(); iter++)
                        iter->second.len = min(iter->second.len, (uint)m_width);
                }

                OutFileLine(lineCnt, matchCnt);
                if (!m_grepOpt.hideText) 
                { 
					afterLines = m_grepOpt.afterCnt;
					for (unsigned bidx = 0; bidx != beforeLines.size(); bidx++)
					{
						std::string beforeStr = beforeLines[(bidx + addBeforeIdx) % m_grepOpt.beforeCnt];
						if (beforeStr.length() != 0)
							LLMsg::Out() << beforeStr << std::endl;
					}

                    ColorMap::const_iterator iter = colorMap.begin();
                    uint pos = 0;
                    const char* cstr = str.c_str();
                    while (iter != colorMap.end())
                    {
                        if (iter->first >= pos)
                        {
                            LLMsg::Out().write(cstr + pos, iter->first - pos);
                            pos = iter->first;
                            SetColor(iter->second.color);
                            LLMsg::Out().write(cstr + iter->first, iter->second.len);
                            SetColor(sConfig.m_colorNormal);
                            pos = iter->first + iter->second.len;
                        }
                        iter++;
                    }
                    LLMsg::Out()  << (cstr + pos);
                    LLMsg::Out() << std::endl;
                }
            }

            if (matchCnt >= m_grepOpt.matchCnt)
                break;
        }
		else if (afterLines != 0)
		{
			 if (!m_grepOpt.hideText) 
			    LLMsg::Out() << str << std::endl;
			 afterLines--;
		}

		if (m_grepOpt.beforeCnt > 0) 
			beforeLines[addBeforeIdx++ % m_grepOpt.beforeCnt] = str;
        
        str.clear();
    }

	m_lineCnt += lineCnt;
    return matchCnt;
}

// ---------------------------------------------------------------------------
void LLReplace::EnableFiltersForFile(const std::string& filePath)
{
    std::tr1::smatch match;
    std::regex_constants::match_flag_type flags = std::regex_constants::match_default;
    for (unsigned patIdx = 0; patIdx != m_grepReplaceList.size(); patIdx++)
    {
        GrepReplaceItem& item = m_grepReplaceList[patIdx];
        item.m_enabled = !item.m_haveFilePat || std::tr1::regex_match(filePath, match, item.m_filePathPat, flags);
    }
}

// ---------------------------------------------------------------------------
void LLReplace::ColorizeReplace(const std::string& str) 
{
    ColorMap  colorMap;
    std::string replaceStr;
    for (unsigned patIdx = 0; patIdx != m_grepReplaceList.size(); patIdx++)
    {
        if (m_grepReplaceList[patIdx].m_enabled)
        {
            replaceStr = m_grepReplaceList[patIdx].m_replaceStr;
			if (replaceStr.length() != 0)
			{
				int pos = -1;
				while ((pos = (int)str.find(replaceStr, pos+1)) != (int)std::string::npos)
				{
					colorMap[pos] = ColorInfo(replaceStr.length(), MATCH_COLORS[patIdx % ARRAYSIZE(MATCH_COLORS)]);
				}
			}
        }
    }

    if (colorMap.size() != 0)
    {
        ColorMap::const_iterator iter = colorMap.begin();
        uint pos = 0;
        const char* cstr = str.c_str();
        while (iter != colorMap.end())
        {
            if (iter->first >= pos)
            {
                LLMsg::Out().write(cstr + pos, iter->first - pos);
                pos = iter->first;
                SetColor(iter->second.color);
                LLMsg::Out().write(cstr + iter->first, iter->second.len);
                SetColor(sConfig.m_colorNormal);
                pos = iter->first + iter->second.len;
            }
            iter++;
        }
        LLMsg::Out()  << (cstr + pos);
    }
}

// ---------------------------------------------------------------------------
unsigned LLReplace::FindReplace(const WIN32_FIND_DATA* pFileData)
{
    if (pFileData->nFileSizeLow == 0 && pFileData->nFileSizeHigh == 0)
        return 0;

    if (m_grepReplaceList[0].m_replace == false)
        return FindGrep();

    unsigned matchCnt = 0;
    //  U(i|b)  Update inplace or make backup.
    bool okayToWrite =  (m_grepOpt.update != 'i') || (m_force || LLPath::IsWriteable(pFileData->dwFileAttributes));

    if (okayToWrite)
    {
        std::regex_constants::match_flag_type flags = std::regex_constants::match_default;
        size_t lineCnt = 0;

        try
        {
            int inMode = std::ios::in | std::ios::binary;
            if (m_byLine || m_grepReplaceList.size() > 1) 
            {
                EnableFiltersForFile(m_srcPath);

                // ----- Find and Replace by line -----
                std::tr1::smatch match;
                std::ifstream in(m_srcPath, inMode, _SH_DENYNO);
                std::ofstream out;
                std::streampos inPos = in.tellg();

                std::string str;
                while (std::getline(in, str))
                {
                    lineCnt++;
                    if (lineCnt < m_grepOpt.lineCnt)
                    {
                        for (unsigned patIdx = 0; patIdx != m_grepReplaceList.size(); patIdx++)
                        {
                            if (m_grepReplaceList[patIdx].m_enabled)
                            {
                                std::tr1::regex grepLinePat = m_grepReplaceList[patIdx].m_grepLinePat;
                                if (std::tr1::regex_search(str, match, grepLinePat, flags))
                                {
                                    std::string replaceStr = m_grepReplaceList[patIdx].m_replaceStr;

                                    matchCnt++;
                                    if (matchCnt == 1)
                                        OpenOutput(out, in, inPos);

                                    std::string newStr = std::regex_replace(str, grepLinePat, replaceStr, flags);
                                    str.swap(newStr);

                                    if (m_echo)
                                    {
                                        OutFileLine(lineCnt, matchCnt);
                                        if (!m_grepOpt.hideText) 
                                        {
                                            ColorizeReplace(str);
                                            LLMsg::Out() << std::endl;
                                        }
                                    }

                                    if (matchCnt >= m_grepOpt.matchCnt)
                                        break;
                                }
                            }
                        }
                    }
                    else if (!out)
                    {
                        break;
                    }

                    if (out)
                        out << str << std::endl;

                    inPos = in.tellg();
                }

                in.close();
                out.close();
                BackupAndRenameFile();
            }
            else
            {       
                // ----- Find and Replace by memory mapped file -----
				bool didReplace;
				do {
					didReplace = false;

					std::ifstream in;
					std::streampos inPos(0);
					std::ofstream out;
       
					MemMapFile mapFile;
					void* mapPtr;
					SIZE_T viewLength = INT_MAX;
					if (mapFile.Open(m_srcPath) && (mapPtr = mapFile.MapView(0, viewLength)) != NULL)
					{
						std::tr1::match_results <const char*> match;
						const char* begPtr = (const char*)mapPtr;
						const char* endPtr = begPtr + viewLength;
						const char* strPtr = begPtr;
						std::tr1::regex grepLinePat = m_grepReplaceList[0].m_grepLinePat;
						std::string replaceStr = m_grepReplaceList[0].m_replaceStr;

						if (std::tr1::regex_search(strPtr, endPtr, match, grepLinePat, flags))
						{
							matchCnt++;
							didReplace = true;

							if (m_echo)
							{
								OutFileLine(lineCnt, matchCnt, match.position(0));
								if (!m_grepOpt.hideText) 
									LLMsg::Out() << std::endl;
							}

							OpenOutput(out, in, inPos);     
							out.write(begPtr, match.position(0));
							begPtr = (const char*)mapPtr;
							begPtr += match.position(0);

							std::regex_replace(
								std::ostreambuf_iterator<char>(out),
								begPtr,
								endPtr,
								grepLinePat, replaceStr, flags);

							mapFile.Close();
							out.close();
							if (!BackupAndRenameFile())
								break;
						}
					}
					else
					{
						LLMsg::PresentError(GetLastError(), "Open failed,", m_srcPath);
					}
				} while (didReplace && m_grepOpt.repeatReplace);
            }
        }
        catch (...)
        {
        }
    }
    else
    {
        LLMsg::PresentError(0, "Replace ignored,", pFileData->cFileName, " Not writeable\n");
    }

    return matchCnt;
}
      
// ---------------------------------------------------------------------------
bool LLReplace::BackupAndRenameFile()
{
    //  U(i|b)  Update inplace or make backup.
    if (m_grepOpt.update == 'b')
    {
        int error = rename(m_srcPath, (m_srcPath + ".bak").c_str());
        if (error != 0) 
        {
            LLMsg::PresentError(GetLastError(), "Failed to make backup\n", m_srcPath);
            RemoveTmpFile();
            return false;
        }
    }

    if (!m_tmpOutFilename.empty() && m_tmpOutFilename != m_srcPath)
    {
        // int error = renameFiles(m_tmpOutFilename.c_str(), m_srcPath.c_str());
        if (0 == MoveFileEx(m_tmpOutFilename.c_str(), m_srcPath.c_str(), MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING))
        {
            DWORD lastError = GetLastError();
            LLMsg::PresentError(lastError, "Renaming tmp failed\n", m_srcPath);
            RemoveTmpFile();
            return false;
        }
    }

    return true;
}

// ---------------------------------------------------------------------------
void LLReplace::RemoveTmpFile()
{
     if (m_tmpOutFilename != m_srcPath)
         DeleteFile(m_tmpOutFilename.c_str());
}

// ---------------------------------------------------------------------------
std::ofstream& LLReplace::OpenOutput(std::ofstream& out, std::ifstream& in, std::streampos& inPos)
{
    int outMode = std::ios::out | std::ios::binary;

    //  U(i|b)  Update inplace or make backup.
    if (m_grepOpt.update == 'i')
    {
        m_tmpOutFilename = m_srcPath;
        out.open(m_srcPath, outMode, _SH_DENYNO);
        out.seekp(inPos);
    }
    else
    {
        m_tmpOutFilename = m_srcPath + "_tmp_XXXXXX";
        m_tmpOutFilename.push_back('\0');
        _mktemp_s((char*)m_tmpOutFilename.c_str(), m_tmpOutFilename.length());
        out.open(m_tmpOutFilename, outMode, _SH_DENYNO);
    }

    streamoff inOff = inPos;
    if (out && in && inOff > 0)
    {
        size_t inLen = (size_t)inOff;
        m_tmpBuffer.resize(min(8192, inLen));
        std::streamsize inCnt = 1;
        while (inLen > 0 && inCnt > 0)
        {
            inCnt = in.read(m_tmpBuffer.data(), m_tmpBuffer.size()).gcount();
            if (inCnt > 0)
            {
                out.write(m_tmpBuffer.data(), inCnt);
                inLen -= (size_t)inCnt;
                m_tmpBuffer.resize(min(8192, inLen));
            }
        }
    }

    return out;
}
