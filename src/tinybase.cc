//
// redbase.cc
//
// Author: Jason McHugh (mchughj@cs.stanford.edu)
//
// This shell is provided for the student.

#include "tinybase.h"

#include <iostream>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include "rm.h"
#include "sm.h"
#include "ql.h"

using namespace std;

//
// main
//
main(int argc, char *argv[])
{
    char *dbname;
    RC rc;

    // Look for 2 arguments.  The first is always the name of the program
    // that was executed, and the second should be the name of the
    // database.
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " dbname \n";
        exit(1);
    }

    // The database name is the second argument
    dbname = argv[1];

    //Checks to subfolder for the database exists
    if (chdir(dbname) < 0) {
        cerr << argv[0] << " Database " << dbname << " not found\n";
        exit(1);
    }
    //Goes back to directory
    chdir("..");

    // Initialize TinyBase components
    PF_Manager pfm;
    RM_Manager rmm(pfm);
    IX_Manager ixm(pfm);
    SM_Manager smm(ixm, rmm);
    QL_Manager qlm(smm, ixm, rmm);

    // open the database
    rc = smm.OpenDb(dbname);
    if(rc) PrintError(rc);

    // call the parser
    RBparse(pfm, smm, qlm);

    // close the database
    rc = smm.CloseDb();
    if(rc) PrintError(rc);

    cout << "Bye.\n";
}
