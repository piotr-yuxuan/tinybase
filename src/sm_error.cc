//
// File:        sm_error.cc
// Description: SM_PrintError function
// Author:      Hyunjung Park (hyunjung@stanford.edu)
//

#include <cerrno>
#include <cstdio>
#include <iostream>
#include "sm_internal.h"

using namespace std;

//
// Error table
//
static char *SM_WarnMsg[] = {
  (char*)"invalid DB name",
  (char*)"cannot change directory",
  (char*)"invalid relation name",
  (char*)"duplicated attribute names",
  (char*)"relation already exists",
  (char*)"relation not found",
  (char*)"relation/attribute not found",
  (char*)"index already exists",
  (char*)"index not found",
  (char*)"data file I/O failed",
  (char*)"invalid data file format",
  (char*)"parameter undefined"
};

static char *SM_ErrorMsg[] = {
  (char*)"no memory"
};

// 
// SM_PrintError
//
// Desc: Send a message corresponding to a SM return code to cerr
// In:   rc - return code for which a message is desired
//
void SM_PrintError(RC rc)
{
  // Check the return code is within proper limits
  if (rc >= START_SM_WARN && rc <= SM_LASTWARN)
    // Print warning
    cerr << "SM warning: " << SM_WarnMsg[rc - START_SM_WARN] << "\n";
  // Error codes are negative, so invert everything
  else if (-rc >= -START_SM_ERR && -rc < -SM_LASTERROR)
    // Print error
    cerr << "SM error: " << SM_ErrorMsg[-rc + START_SM_ERR] << "\n";
  else if (rc == 0)
    cerr << "SM_PrintError called with return code of 0\n";
  else
    cerr << "SM error: " << rc << " is out of bounds\n";
}

