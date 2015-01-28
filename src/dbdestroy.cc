//
// dbdestroy.cc
//

#include <iostream>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include "rm.h"
#include "sm.h"
#include "tinybase.h"

#include "printer.h" //This includes allows us to use the DataAttrInfo struct

using namespace std;

//
// main
//
main(int argc, char *argv[])
{
    char *dbname;
    char command[80] = "rm -r ";
    RC rc;

    // Look for 2 arguments. The first is always the name of the program
    // that was executed, and the second should be the name of the
    // database.
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " dbname \n";
        exit(1);
    }

    // The database name is the second argument
    dbname = argv[1];

    //Checks the directory exists before removing
    if (chdir(dbname) < 0) {
        cerr << argv[0] << " Database " << dbname << " not found\n";
        exit(1);
    }
    chdir("..");

    // Remove the subdirectory of the database
    system(strcat(command, dbname));

    //Checks the directory was actually removed
    if (!(chdir(dbname) < 0)) {
        cerr << argv[0] << " chdir error to " << dbname << "\n";
        exit(1);
    }

    return(0);
}
