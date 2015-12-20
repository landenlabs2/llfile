//-----------------------------------------------------------------------------
// getopt - Unix like parameter parser.
// Author:  DLang   2008
// http://home.comcast.net/~lang.dennis/
//-----------------------------------------------------------------------------


#pragma once

extern "C"
{
extern int optind;
extern char *optarg, optopt;

int getopt(int argc, char *argv[], char *opts);
};