//
// File:        dbcreate.cc
// Description: dbcreate command line utility
// Author:      Hyunjung Park (hyunjung@stanford.edu)
//

#include <iostream>
#include <cstring>
#include <unistd.h>
#include "redbase.h"
#include "rm.h"
#include "sm_internal.h"

using namespace std;

//
// Global PF_Manager and RM_Manager variables
//
PF_Manager pfm;
RM_Manager rmm(pfm);

//
// createRelcat
//
// Desc: Create the relation catalog
//
RC createRelcat(void)
{
   RC rc;
   RM_FileHandle fh;
   SM_RelcatRec relcatRec;
   RID rid;

   if (rc = rmm.CreateFile(RELCAT, sizeof(SM_RelcatRec)))
      goto err_return;

   if (rc = rmm.OpenFile(RELCAT, fh))
      goto err_return;

   SM_SetRelcatRec(relcatRec,
                   RELCAT, sizeof(SM_RelcatRec), 4, 0);

   if (rc = fh.InsertRec((char *)&relcatRec, rid))
      goto err_close;

   SM_SetRelcatRec(relcatRec,
                   ATTRCAT, sizeof(SM_AttrcatRec), 6, 0);

   if (rc = fh.InsertRec((char *)&relcatRec, rid))
      goto err_close;

   if (rc = rmm.CloseFile(fh))
      goto err_return;

   // Return ok
   return (0);

   // Return error
err_close:
   rmm.CloseFile(fh);
err_return:
   return (rc);
}

//
// createAttrcat
//
// Desc: Create the attribute catalog
//
RC createAttrcat(void)
{
   RC rc;
   RM_FileHandle fh;
   SM_AttrcatRec attrcatRec;
   RID rid;

   if (rc = rmm.CreateFile(ATTRCAT, sizeof(SM_AttrcatRec)))
      goto err_return;

   if (rc = rmm.OpenFile(ATTRCAT, fh))
      goto err_return;

   SM_SetAttrcatRec(attrcatRec, 
                    RELCAT, "relName", OFFSET(SM_RelcatRec, relName),
                    STRING, MAXNAME, -1);

   if (rc = fh.InsertRec((char *)&attrcatRec, rid))
      goto err_close;

   SM_SetAttrcatRec(attrcatRec, 
                    RELCAT, "tupleLength", OFFSET(SM_RelcatRec, tupleLength),
                    INT, sizeof(int), -1);

   if (rc = fh.InsertRec((char *)&attrcatRec, rid))
      goto err_close;

   SM_SetAttrcatRec(attrcatRec, 
                    RELCAT, "attrCount", OFFSET(SM_RelcatRec, attrCount),
                    INT, sizeof(int), -1);

   if (rc = fh.InsertRec((char *)&attrcatRec, rid))
      goto err_close;

   SM_SetAttrcatRec(attrcatRec, 
                    RELCAT, "indexCount", OFFSET(SM_RelcatRec, indexCount),
                    INT, sizeof(int), -1);

   if (rc = fh.InsertRec((char *)&attrcatRec, rid))
      goto err_close;

   SM_SetAttrcatRec(attrcatRec, 
                    ATTRCAT, "relName", OFFSET(SM_AttrcatRec, relName),
                    STRING, MAXNAME, -1);

   if (rc = fh.InsertRec((char *)&attrcatRec, rid))
      goto err_close;

   SM_SetAttrcatRec(attrcatRec, 
                    ATTRCAT, "attrName", OFFSET(SM_AttrcatRec, attrName),
                    STRING, MAXNAME, -1);

   if (rc = fh.InsertRec((char *)&attrcatRec, rid))
      goto err_close;

   SM_SetAttrcatRec(attrcatRec, 
                    ATTRCAT, "offset", OFFSET(SM_AttrcatRec, offset),
                    INT, sizeof(int), -1);

   if (rc = fh.InsertRec((char *)&attrcatRec, rid))
      goto err_close;

   SM_SetAttrcatRec(attrcatRec, 
                    ATTRCAT, "attrType", OFFSET(SM_AttrcatRec, attrType),
                    INT, sizeof(int), -1);

   if (rc = fh.InsertRec((char *)&attrcatRec, rid))
      goto err_close;

   SM_SetAttrcatRec(attrcatRec, 
                    ATTRCAT, "attrLength", OFFSET(SM_AttrcatRec, attrLength),
                    INT, sizeof(int), -1);

   if (rc = fh.InsertRec((char *)&attrcatRec, rid))
      goto err_close;

   SM_SetAttrcatRec(attrcatRec, 
                    ATTRCAT, "indexNo", OFFSET(SM_AttrcatRec, indexNo),
                    INT, sizeof(int), -1);

   if (rc = fh.InsertRec((char *)&attrcatRec, rid))
      goto err_close;

   if (rc = rmm.CloseFile(fh))
      goto err_return;
 
   // Return ok
   return (0);

   // Return error
err_close:
   rmm.CloseFile(fh);
err_return:
   return (rc);
}

//
// main
//
int main(int argc, char *argv[])
{
   RC rc;         
   char *dbname;
   char command[8+MAXDBNAME] = "mkdir ";

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

   // Create a subdirectory for the database
   if (system(strcat(command, dbname)))
      goto err_exit;

   // Change the working directory
   if (chdir(dbname) < 0) {
      SM_PrintError(SM_CHDIRFAILED);
      goto err_rm;
   }

   // Create the relation catalog
   if (rc = createRelcat()) {
      PrintError(rc);
      goto err_rm;
   }

   // Create the attributes catalog
   if (rc = createAttrcat()) {
      PrintError(rc);
      goto err_rm;
   }

   // Return ok
   return (0);

   // Return error
err_rm:
   strcpy(command, "rm -r ");
   system(strcat(command, dbname));
err_exit:
   return (1);
}

