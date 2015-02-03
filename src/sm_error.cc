//
// File:        sm_error.cc
// Description: SM_PrintError function
//

#include <cerrno>
#include <cstdio>
#include <iostream>
#include "sm.h"

using namespace std;

//
// Error table
//
static char *SM_WarnMsg[] = {
  (char*)"invalid attribute name",
  (char*)"invalid database",
  (char*)"couldnt close catalog",
  (char*)"database already exists",
  (char*)"invalid attribute count",
  (char*)"invalid attribute",
  (char*)"invalid attribute length",
  (char*)"invalid attribute type",
  (char*)"catalog error",
  (char*)"couldnt create the file",
  (char*)"couldnt destroy the file",
  (char*)"index error",
  (char*)"couldnt open the file"
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
  else if (rc == 0)
    cerr << "SM_PrintError called with return code of 0\n";
  else
    cerr << "SM error: " << rc << " is out of bounds\n";
}

