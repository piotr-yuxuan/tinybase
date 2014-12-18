//
// File:        rm_error.cc
// Description: RL_PrintError function
// Authors:     Camille TIENNOT (tiennot@enst.fr)
//

#include <cstdio>
#include <cerrno>
#include <iostream>
#include "rm.h"

using namespace std;

//
// Error table
//
static char *RM_WarnMsg[] = {
  (char*)"File is already open",
  (char*)"End of file",
  (char*)"Record is not read",
  (char*)"invalid RID",
  (char*)"Record size is invalid",
  (char*)"Invalid record"
};

static char *RM_ErrorMsg[] = {
  (char*)"Couldn't create the FileScan object",
  (char*)"File is not open",
  (char*)"",
  (char*)"",
  (char*)"RM_NORECATRID",
  (char*)"",
  (char*)"",
  (char*)"",
  (char*)"",
  (char*)"",
  (char*)"",
  (char*)"",
  (char*)"",
  (char*)"",
  (char*)""
};

//
// RM_PrintError
//
// Desc: Send a message corresponding to a RM return code to cerr
//       Assumes RM_UNIX is last valid RM return code
// In:   rc - return code for which a message is desired
//
void RM_PrintError(RC rc)
{
  // Check the return code is within proper limits
  if (rc >= START_RM_WARN && rc <= RM_LASTWARN)
    // Print warning
    cerr << "RM warning: " << RM_WarnMsg[rc - START_RM_WARN] << "\n";
  // Error codes are negative, so invert everything
  else if (-rc >= -START_RM_ERR && -rc <= -RM_LASTERROR)
    // Print error
    cerr << "RM error: " << RM_ErrorMsg[-rc + START_RM_ERR] << "\n";
  else if (rc == RM_UNIX)
#ifdef PC
      cerr << "OS error\n";
#else
      cerr << strerror(errno) << "\n";
#endif
  else if (rc == 0)
    cerr << "RM_PrintError called with return code of 0\n";
  else
    cerr << "RM error: " << rc << " is out of bounds\n";
}
