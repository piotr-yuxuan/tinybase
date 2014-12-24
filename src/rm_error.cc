//
// File:        rm_error.cc
// Description: RM_PrintError function
// Author:      Hyunjung Park (hyunjung@stanford.edu)
//

#include <cerrno>
#include <cstdio>
#include <iostream>
#include "rm_internal.h"

using namespace std;

//
// Error table
//
static char *RM_WarnMsg[] = {
  (char*)"inviable rid",
  (char*)"unread record",
  (char*)"invalid record size",
  (char*)"invalid slot number",
  (char*)"record not found",
  (char*)"invalid comparison operator",
  (char*)"invalid attribute parameters",
  (char*)"null pointer",
  (char*)"scan open",
  (char*)"scan closed",
  (char*)"file closed"
};

// 
// RM_PrintError
//
// Desc: Send a message corresponding to a RM return code to cerr
// In:   rc - return code for which a message is desired
//
void RM_PrintError(RC rc)
{
  // Check the return code is within proper limits
  if (rc >= START_RM_WARN && rc <= RM_LASTWARN)
    // Print warning
    cerr << "RM warning: " << RM_WarnMsg[rc - START_RM_WARN] << "\n";
  else if (rc == 0)
    cerr << "RM_PrintError called with return code of 0\n";
  else
    cerr << "RM error: " << rc << " is out of bounds\n";
}

