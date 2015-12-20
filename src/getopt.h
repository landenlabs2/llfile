//-----------------------------------------------------------------------------
// getopt - Unix like parameter parser.
// Author:  DLang   2008
// http://landenlabs/
//-----------------------------------------------------------------------------


#pragma once

extern "C"
{
extern int optind;
extern char *optarg, optopt;

int getopt(int argc, char *argv[], char *opts);
};