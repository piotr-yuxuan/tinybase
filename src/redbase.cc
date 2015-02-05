//
// File:        redbase.cc
// Description: redbase command line utility
// Author:      Hyunjung Park (hyunjung@stanford.edu)
//

#include <iostream>
#include <cstring>
#include <unistd.h>
#include "redbase.h"
#include "parser.h"
#include "sm.h"
#include "ql.h"

using namespace std;

//
// Global PF_Manager and RM_Manager variables
//
PF_Manager pfm;
RM_Manager rmm(pfm);
IX_Manager ixm(pfm);
SM_Manager smm(ixm, rmm);
QL_Manager qlm(smm, ixm, rmm);

//
// main
//
int main(int argc, char *argv[])
{
   RC rc;                     
   char *dbname;

   // Look for 2 arguments.  The first is always the name of the program
   // that was executed, and the second should be the name of the
   // database.
   if (argc != 2) {
      cerr << "Usage: " << argv[0] << " DBname\n";
      goto err_exit;
   }

   // The database name is the second argument
   dbname = argv[1];

   // Open DB
   if ((rc = smm.OpenDb(dbname))) {
      PrintError(rc);
      goto err_exit;
   }

   RBparse(pfm, smm, qlm);

   // Close DB
   if ((rc = smm.CloseDb())) {
      PrintError(rc);
      goto err_exit;
   }

   cout << "Bye.\n";
   return (0);

   // Return error
err_exit:
   return (1);
}
