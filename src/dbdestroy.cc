//
// File:        dbdestroy.cc
// Description: dbdestroy command line utility
// Author:      Hyunjung Park (hyunjung@stanford.edu)
//

#include <iostream>
#include <cstring>
#include "redbase.h"
#include "sm_internal.h"

using namespace std;

//
// main
//
int main(int argc, char *argv[])
{
   char *dbname;
   char command[8+MAXDBNAME] = "rm -r ";

   // Look for 2 arguments. The first is always the name of the program
   // that was executed, and the second should be the name of the
   // database.
   if (argc != 2) {
      cerr << "Usage: " << argv[0] << " DBname\n";
      goto err_exit;
   }

   // The database name is the second argument
   dbname = argv[1];

   // Sanity Check: Length of the argument should be less than MAXDBNAME
   //               DBname cannot contain ' ' or '/' 
   if (strlen(dbname) > MAXDBNAME
       || strchr(dbname, ' ') || strchr(dbname, '/')) {
      SM_PrintError(SM_INVALIDDBNAME);
      goto err_exit;
   }

   // Remove a subdirectory for the database
   return system(strcat(command, dbname));

   // Return error
err_exit:
   return (1);
}

