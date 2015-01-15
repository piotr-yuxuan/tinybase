//
// File:        IX_error.cc
// Description: IX_PrintError function
//

#include <cerrno>
#include <cstdio>
#include <iostream>
#include "ix.h"

using namespace std;

//
// Error table
//
//
// Error table
//
static char *IX_WarnMsg[] = {
  (char*)"invalid index number",
  (char*)"invalid comparison operator",
  (char*)"invalid attribute parameters",
  (char*)"null pointer",
  (char*)"scan open",
  (char*)"scan closed",
  (char*)"file closed",
  (char*)"entry not found",
  (char*)"entry already exists",
  (char*)"end of file"
};

static char *IX_ErrorMsg[] = {
  (char*)"no memory"
};

// 
// RM_PrintError
//
// Desc: Send a message corresponding to a RM return code to cerr
// In:   rc - return code for which a message is desired
//
void IX_PrintError(RC rc)
{
  // Check the return code is within proper limits
  if (rc >= START_IX_WARN && rc <= IX_LASTWARN)
    // Print warning
    cerr << "IX warning: " << IX_WarnMsg[rc - START_IX_WARN] << "\n";
  else if (rc == 0)
    cerr << "IX_PrintError called with return code of 0\n";
  else
    cerr << "IX error: " << rc << " is out of bounds\n";
}

