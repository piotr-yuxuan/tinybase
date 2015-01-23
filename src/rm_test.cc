//
// File:        rm_testshell.cc
// Description: Test RM component
// Authors:     Jan Jannink
//              Dallan Quass (quass@cs.stanford.edu)
//              Jason McHugh (mchughj@cs.stanford.edu)
//
// This test shell contains a number of functions that will be useful
// in testing your RM component code.  In addition, a couple of sample
// tests are provided.  The tests are by no means comprehensive, however,
// and you are expected to devise your own tests to test your code.
//
// 1997:  Tester has been modified to reflect the change in the 1997
// interface.  For example, FileHandle no longer supports a Scan over the
// relation.  All scans are done via a FileScan.
//

#include <cstdio>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <cstdlib>
#include <cassert>

#include "redbase.h"
#include "pf.h"
#include "rm.h"

using namespace std;

//
// Defines
//
#define FILENAME   (char*)"testrel"  // test file name
#define STRLEN      29               // length of string in testrec
#define PROG_UNIT   50               // how frequently to give progress
                                     //   reports when adding lots of recs
#define FEW_RECS   20                // number of records added in

//
// Computes the offset of a field in a record (should be in <stddef.h>)
//
#ifndef offsetof
#       define offsetof(type, field)   ((size_t)&(((type *)0) -> field))
#endif

//
// Structure of the records we will be using for the tests
//
struct TestRec {
   int   num;
   float r;
   char  str[STRLEN];
};

//
// Global PF_Manager and RM_Manager variables
//
PF_Manager pfm;
RM_Manager rmm(pfm);

//
// Function declarations
//
RC Test1(void);
RC Test2(void);
RC Test3(void);
RC Test4(void);
RC Test5(void);
RC Test6(void);
RC Test7(void);

void PrintError(RC rc);
void LsFile(char *fileName);
void PrintRecord(TestRec &recBuf);
RC AddRecs(RM_FileHandle &fh, int numRecs, int startNum=0);
RC VerifyFile(RM_FileHandle &fh, int numRecs);
RC PrintFile(RM_FileHandle &fh);

RC CreateFile(char *fileName, int recordSize);
RC DestroyFile(char *fileName);
RC OpenFile(char *fileName, RM_FileHandle &fh);
RC CloseFile(char *fileName, RM_FileHandle &fh);
RC InsertRec(RM_FileHandle &fh, char *record, RID &rid);
RC UpdateRec(RM_FileHandle &fh, RM_Record &rec);
RC DeleteRec(RM_FileHandle &fh, RID &rid);
RC GetNextRecScan(RM_FileScan &fs, RM_Record &rec);

//
// Array of pointers to the test functions
//
#define NUM_TESTS       7               // number of tests
int (*tests[])() =                      // RC doesn't work on some compilers
{
   Test1,
   Test2,
   Test3,
   Test4,
   Test5,
   Test6,
   Test7
};

//
// main
//
int main(int argc, char *argv[])
{
   RC   rc;
   char *progName = argv[0];   // since we will be changing argv
   int  testNum;

   // Write out initial starting message
   cerr.flush();
   cout.flush();
   cout << "Starting RM component test.\n";
   cout.flush();

   // Delete files from last time
   unlink(FILENAME);

   // If no argument given, do all tests
   if (argc == 1) {
      for (testNum = 0; testNum < NUM_TESTS; testNum++)
         if ((rc = (tests[testNum])())) {

            // Print the error and exit
            PrintError(rc);
            return (1);
         }
   }
   else {

      // Otherwise, perform specific tests
      while (*++argv != NULL) {

         // Make sure it's a number
         if (sscanf(*argv, "%d", &testNum) != 1) {
            cerr << progName << ": " << *argv << " is not a number\n";
            continue;
         }

         // Make sure it's in range
         if (testNum < 1 || testNum > NUM_TESTS) {
            cerr << "Valid test numbers are between 1 and " << NUM_TESTS << "\n";
            continue;
         }

         // Perform the test
         if ((rc = (tests[testNum - 1])())) {

            // Print the error and exit
            PrintError(rc);
            return (1);
         }
      }
   }

   // Write ending message and exit
   cout << "Ending RM component test.\n\n";

   return (0);
}

//
// PrintError
//
// Desc: Print an error message by calling the proper component-specific
//       print-error function
//
void PrintError(RC rc)
{
   if (abs(rc) <= END_PF_WARN)
      PF_PrintError(rc);
   else if (abs(rc) <= END_RM_WARN)
      RM_PrintError(rc);
   else
      cerr << "Error code out of range: " << rc << "\n";
}

////////////////////////////////////////////////////////////////////
// The following functions may be useful in tests that you devise //
////////////////////////////////////////////////////////////////////

//
// LsFile
//
// Desc: list the filename's directory entry
//
void LsFile(char *fileName)
{
   char command[80];

   sprintf(command, "ls -l %s", fileName);
   printf("doing \"%s\"\n", command);
   system(command);
}

//
// PrintRecord
//
// Desc: Print the TestRec record components
//
void PrintRMRecord(RM_Record &rec)
{
   RID rid;
   char *pData;
   PageNum pn;
   SlotNum sn;
   TestRec *recBuf;

   rec.GetRid(rid);
   rec.GetData(pData);
   recBuf = (TestRec *)pData;

   rid.GetPageNum(pn);
   rid.GetSlotNum(sn);

   printf("(%d,%d) [%s, %d, %f]\n",
          pn, sn, recBuf->str, recBuf->num, recBuf->r);
}

//
// PrintRecord
//
// Desc: Print the TestRec record components
//
void PrintRecord(TestRec &recBuf)
{
   printf("[%s, %d, %f]\n", recBuf.str, recBuf.num, recBuf.r);
}

//
// AddRecs
//
// Desc: Add a number of records to the file
//
RC AddRecs(RM_FileHandle &fh, int numRecs, int startNum)
{
   RC      rc;
   int     i;
   TestRec recBuf;
   RID     rid;
   PageNum pageNum;
   SlotNum slotNum;

   // We set all of the TestRec to be 0 initially.  This heads off
   // warnings that Purify will give regarding UMR since sizeof(TestRec)
   // is 40, whereas actual size is 37.
   memset((void *)&recBuf, 0, sizeof(recBuf));

   printf("\nadding %d records\n", numRecs);
   for(i = 0; i < numRecs; i++) {
      memset(recBuf.str, ' ', STRLEN);
      sprintf(recBuf.str, "a%d", i + startNum);
      recBuf.num = (FEW_RECS/2-i)*(FEW_RECS/2-i);
      recBuf.r = (float)i + startNum;
      if ((rc = InsertRec(fh, (char *)&recBuf, rid)) ||
            (rc = rid.GetPageNum(pageNum)) ||
            (rc = rid.GetSlotNum(slotNum)))
         return (rc);

      if((i + 1) % PROG_UNIT == 0){
         printf("%d  ", i + 1);
         fflush(stdout);
      }
   }
   if(i % PROG_UNIT != 0)
      printf("%d\n", i);
   else
      putchar('\n');

   // Return ok
   return (0);
}

//
// VerifyFile
//
// Desc: verify that a file has records as added by AddRecs
//
RC VerifyFile(RM_FileHandle &fh, int numRecs)
{
   RC        rc;
   int       n;
   TestRec   *pRecBuf;
   RID       rid;
   char      stringBuf[STRLEN];
   char      *found;
   RM_Record rec;

   found = new char[numRecs];
   memset(found, 0, numRecs);

   printf("\nverifying file contents\n");

   RM_FileScan fs;
   if ((rc=fs.OpenScan(fh,INT,sizeof(int),offsetof(TestRec, num),
         NO_OP, NULL, NO_HINT)))
      return (rc);

   // For each record in the file
   for (rc = GetNextRecScan(fs, rec), n = 0;
         rc == 0;
         rc = GetNextRecScan(fs, rec), n++) {

      // Make sure the record is correct
      if ((rc = rec.GetData((char *&)pRecBuf)) ||
            (rc = rec.GetRid(rid)))
         goto err;

      memset(stringBuf,' ', STRLEN);
      sprintf(stringBuf, "a%d", pRecBuf->num);

      if (pRecBuf->num < 0 || pRecBuf->num >= numRecs ||
            strcmp(pRecBuf->str, stringBuf) ||
            pRecBuf->r != (float)pRecBuf->num) {
         printf("VerifyFile: invalid record = [%s, %d, %f]\n",
               pRecBuf->str, pRecBuf->num, pRecBuf->r);
         exit(1);
      }

      if (found[pRecBuf->num]) {
         printf("VerifyFile: duplicate record = [%s, %d, %f]\n",
               pRecBuf->str, pRecBuf->num, pRecBuf->r);
         exit(1);
      }

      found[pRecBuf->num] = 1;
   }

   if (rc != RM_EOF)
      goto err;

   if ((rc=fs.CloseScan()))
      return (rc);

   // make sure we had the right number of records in the file
   if (n != numRecs) {
      printf("%d records in file (supposed to be %d)\n",
            n, numRecs);
      exit(1);
   }

   // Return ok
   rc = 0;

err:
   fs.CloseScan();
   delete[] found;
   return (rc);
}

//
// PrintFile
//
// Desc: Print the contents of the file
//
RC PrintFile(RM_FileHandle &fh)
{
   RC        rc;
   int       n;
   TestRec   *pRecBuf;
   RID       rid;
   RM_Record rec;
   int       val = 30;
   PageNum   pn;
   SlotNum   sn;

   printf("\nprinting file contents\n");

   RM_FileScan fs;

   if ((rc=fs.OpenScan(fh,INT,sizeof(int),offsetof(TestRec, num),
         NO_OP, &val, NO_HINT)))
      return (rc);

   // for each record in the file
   for (rc = GetNextRecScan(fs, rec), n = 0;
         rc == 0;
         rc = GetNextRecScan(fs, rec), n++) {

      // Get the record data and record id
      if ((rc = rec.GetData((char *&)pRecBuf)) ||
            (rc = rec.GetRid(rid)))
         return (rc);

      // Print the record contents
      PrintRMRecord(rec);
   }

   if (rc != RM_EOF)
      return (rc);

   if ((rc=fs.CloseScan()))
      return (rc);

   printf("%d records found\n", n);

   // Return ok
   return (0);
}

////////////////////////////////////////////////////////////////////////
// The following functions are wrappers for some of the RM component  //
// methods.  They give you an opportunity to add debugging statements //
// and/or set breakpoints when testing these methods.                 //
////////////////////////////////////////////////////////////////////////

//
// CreateFile
//
// Desc: call RM_Manager::CreateFile
//
RC CreateFile(char *fileName, int recordSize)
{
   printf("\ncreating %s\n", fileName);
   return (rmm.CreateFile(fileName, recordSize));
}

//
// DestroyFile
//
// Desc: call RM_Manager::DestroyFile
//
RC DestroyFile(char *fileName)
{
   printf("\ndestroying %s\n", fileName);
   return (rmm.DestroyFile(fileName));
}

//
// OpenFile
//
// Desc: call RM_Manager::OpenFile
//
RC OpenFile(char *fileName, RM_FileHandle &fh)
{
   printf("\nopening %s\n", fileName);
   return (rmm.OpenFile(fileName, fh));
}

//
// CloseFile
//
// Desc: call RM_Manager::CloseFile
//
RC CloseFile(char *fileName, RM_FileHandle &fh)
{
   if (fileName != NULL)
      printf("\nClosing %s\n", fileName);
   return (rmm.CloseFile(fh));
}

//
// InsertRec
//
// Desc: call RM_FileHandle::InsertRec
//
RC InsertRec(RM_FileHandle &fh, char *record, RID &rid)
{
   return (fh.InsertRec(record, rid));
}

//
// DeleteRec
//
// Desc: call RM_FileHandle::DeleteRec
//
RC DeleteRec(RM_FileHandle &fh, RID &rid)
{
   return (fh.DeleteRec(rid));
}

//
// UpdateRec
//
// Desc: call RM_FileHandle::UpdateRec
//
RC UpdateRec(RM_FileHandle &fh, RM_Record &rec)
{
   return (fh.UpdateRec(rec));
}

//
// GetNextRecScan
//
// Desc: call RM_FileScan::GetNextRec
//
RC GetNextRecScan(RM_FileScan &fs, RM_Record &rec)
{
   return (fs.GetNextRec(rec));
}

/////////////////////////////////////////////////////////////////////
// Sample test functions follow.                                   //
/////////////////////////////////////////////////////////////////////

//
// Test1 tests simple creation, opening, closing, and deletion of files
//
RC Test1(void)
{
   RC            rc;
   RM_FileHandle fh;

   printf("test1 starting ****************\n");

   if ((rc = CreateFile(FILENAME, sizeof(TestRec))) ||
         (rc = OpenFile(FILENAME, fh)) ||
         (rc = CloseFile(FILENAME, fh)))
      return (rc);

   LsFile(FILENAME);

   if ((rc = DestroyFile(FILENAME)))
      return (rc);

   printf("\ntest1 done ********************\n");
   return (0);
}

//
// Test2 tests adding a few records to a file.
//
RC Test2(void)
{
   RC            rc;
   RM_FileHandle fh;

   printf("test2 starting ****************\n");

   if ((rc = CreateFile(FILENAME, sizeof(TestRec))) ||
         (rc = OpenFile(FILENAME, fh)) ||
         (rc = AddRecs(fh, FEW_RECS)) ||
         (rc = CloseFile(FILENAME, fh)))
      return (rc);

   LsFile(FILENAME);

   if ((rc = DestroyFile(FILENAME)))
      return (rc);

   printf("\ntest2 done ********************\n");
   return (0);
}

//
// Test3 tests RM_Manager class
//
RC Test3(void)
{
   RC            rc;
   RM_FileHandle fh;

   printf("test3 starting ****************\n");

   printf("\nTesting CreateFile() with too small record size...\n");
   rc = CreateFile(FILENAME, 0);
   PrintError(rc);
   assert(rc == RM_INVALIDRECSIZE);
   printf("\nOK\n");

   printf("\nTesting CreateFile() with too large record size...\n");
   rc = CreateFile(FILENAME, 4088);
   PrintError(rc);
   assert(rc == RM_INVALIDRECSIZE);
   printf("\nOK\n");

   printf("\nTesting CreateFile() with existing filename...\n");
   rc = CreateFile(FILENAME, 40);
   assert(rc == 0);
   rc = CreateFile(FILENAME, 40);
   PrintError(rc);
   assert(rc == PF_UNIX);
   printf("\nOK\n");

   printf("\nTesting DestroyFile() with non-existing filename...\n");
   rc = DestroyFile(FILENAME);
   assert(rc == 0);
   rc = DestroyFile(FILENAME);
   PrintError(rc);
   assert(rc == PF_UNIX);
   printf("\nOK\n");

   printf("\nTesting OpenFile() with non-existing filename...\n");
   rc = OpenFile(FILENAME, fh);
   PrintError(rc);
   assert(rc == PF_UNIX);
   printf("\nOK\n");

   printf("\nTesting OpenFile() with opened file handle...\n");
   rc = CreateFile(FILENAME, 40);
   assert(rc == 0);
   rc = OpenFile(FILENAME, fh);
   assert(rc == 0);
   rc = OpenFile(FILENAME, fh);
   PrintError(rc);
   assert(rc == PF_FILEOPEN);
   printf("\nOK\n");

   printf("\nTesting CloseFile() with closed file handle...\n");
   rc = CloseFile(FILENAME, fh);
   assert(rc == 0);
   rc = CloseFile(FILENAME, fh);
   PrintError(rc);
   assert(rc == PF_CLOSEDFILE);
   printf("\nOK\n");

   printf("\nCleaning up...\n");
   rc = DestroyFile(FILENAME);
   assert(rc == 0);
   printf("\nOK\n");

   printf("\ntest3 done ********************\n");
   return (0);
}

//
// Test4 tests RM_FileScan class
//
RC Test4(void)
{
   RC            rc;
   RM_FileHandle fh;
   RID           rid;
   RM_Record     rec;
   RM_FileScan   fs;
   int           val = 30;

   printf("test4 starting ****************\n");

   printf("\nTesting OpenScan() with closed file handle...\n");
   rc = CreateFile(FILENAME, sizeof(TestRec));
   assert(rc == 0);
   printf("\nopening file scan\n");
   rc = fs.OpenScan(fh, INT, sizeof(int), offsetof(TestRec, num),
                    NO_OP, &val, NO_HINT);
   PrintError(rc);
   assert(rc == RM_CLOSEDFILE);
   printf("\nOK\n");

   printf("\nTesting OpenScan() with invalid compOp...\n");
   rc = OpenFile(FILENAME, fh);
   assert(rc == 0);
   printf("\nopening file scan\n");
   rc = fs.OpenScan(fh, INT, sizeof(int), offsetof(TestRec, num),
                    (CompOp)500, &val, NO_HINT);
   PrintError(rc);
   assert(rc == RM_INVALIDCOMPOP);
   printf("\nOK\n");

   printf("\nTesting OpenScan() with invalid value pointer...\n");
   printf("\nopening file scan\n");
   rc = fs.OpenScan(fh, INT, sizeof(int), offsetof(TestRec, num),
                    EQ_OP, NULL, NO_HINT);
   PrintError(rc);
   assert(rc == RM_NULLPOINTER);
   printf("\nOK\n");

   printf("\nTesting OpenScan() with invalid attribute type...\n");
   printf("\nopening file scan\n");
   rc = fs.OpenScan(fh, (AttrType)4, sizeof(int), offsetof(TestRec, num),
                    EQ_OP, &val, NO_HINT);
   PrintError(rc);
   assert(rc == RM_INVALIDATTR);
   printf("\nOK\n");

   printf("\nTesting OpenScan() with invalid attribute length for INT...\n");
   printf("\nopening file scan\n");
   rc = fs.OpenScan(fh, INT, sizeof(int)+1, offsetof(TestRec, num),
                    EQ_OP, &val, NO_HINT);
   PrintError(rc);
   assert(rc == RM_INVALIDATTR);
   printf("\nOK\n");

   printf("\nTesting OpenScan() with invalid attribute length for STRING...\n");
   printf("\nopening file scan\n");
   rc = fs.OpenScan(fh, STRING, 0, offsetof(TestRec, str),
                    EQ_OP, &val, NO_HINT);
   PrintError(rc);
   assert(rc == RM_INVALIDATTR);
   printf("\nOK\n");

   printf("\nTesting OpenScan() with invalid attribute offset...\n");
   printf("\nopening file scan\n");
   rc = fs.OpenScan(fh, INT, sizeof(int), -1,
                    EQ_OP, &val, NO_HINT);
   PrintError(rc);
   assert(rc == RM_INVALIDATTR);
   printf("\nOK\n");

   printf("\nTesting OpenScan() with invalid attribute offset...\n");
   printf("\nopening file scan\n");
   rc = fs.OpenScan(fh, FLOAT, sizeof(float), STRLEN+500,
                    EQ_OP, &val, NO_HINT);
   PrintError(rc);
   assert(rc == RM_INVALIDATTR);
   printf("\nOK\n");

   printf("\nTesting OpenScan() with already opened file scan...\n");
   printf("\nopening file scan\n");
   rc = fs.OpenScan(fh, INT, sizeof(int), offsetof(TestRec, num),
                    NO_OP, &val, NO_HINT);
   assert(rc == 0);
   printf("\nopening file scan\n");
   rc = fs.OpenScan(fh, INT, sizeof(int), offsetof(TestRec, num),
                    NO_OP, &val, NO_HINT);
   PrintError(rc);
   assert(rc == RM_SCANOPEN);
   printf("\nclosing file scan\n");
   rc = fs.CloseScan();
   assert(rc == 0);
   printf("\nOK\n");

   printf("\nTesting CloseScan() with closed file scan...\n");
   printf("\nclosing file scan\n");
   rc = fs.CloseScan();
   assert(rc == RM_CLOSEDSCAN);
   printf("\nOK\n");

   printf("\nTesting GetNextRec() with closed file scan...\n");
   printf("\ngetting next record\n");
   rc = fs.GetNextRec(rec);
   PrintError(rc);
   assert(rc == RM_CLOSEDSCAN);
   printf("\nOK\n");

   printf("\nTesting GetNextRec() with closed file handle...\n");
   rc = AddRecs(fh, FEW_RECS);
   assert(rc == 0);
   printf("\nopening file scan\n");
   rc = fs.OpenScan(fh, INT, sizeof(int), offsetof(TestRec, num),
                    NO_OP, &val, NO_HINT);
   assert(rc == 0);
   printf("\ngetting next record\n");
   rc = GetNextRecScan(fs, rec);
   assert(rc == 0);
   rc = CloseFile(FILENAME, fh);
   assert(rc == 0);
   printf("\ngetting next record\n");
   rc = GetNextRecScan(fs, rec);
   PrintError(rc);
   assert(rc == RM_CLOSEDFILE);
   printf("\nOK\n");

   printf("\nCleaning up...\n");
// rc = CloseFile(FILENAME, fh);
// assert(rc == 0);
   rc = DestroyFile(FILENAME);
   assert(rc == 0);
   printf("\nOK\n");

   printf("\ntest4 done ********************\n");
   return (0);
}

//
// Test5 tests RM_FileHandle class
//
RC Test5(void)
{
   RC            rc;
   RM_FileHandle fh;
   RM_FileHandle fh2;
   RM_Record     rec;
   RM_FileScan   fs;
   int           val = 30;
   TestRec       *pRecBuf;
   PageNum       pn;
   SlotNum       sn;

   printf("test5 starting ****************\n");

   rc = CreateFile(FILENAME, sizeof(TestRec));
   assert(rc == 0);

   rc = OpenFile(FILENAME, fh);
   assert(rc == 0);

   rc = AddRecs(fh, FEW_RECS);
   assert(rc == 0);
#if 1
   rc = PrintFile(fh);
   assert(rc == 0);
#endif

   printf("\nTesting GetRec() with closed file handle...\n");
   RID rid1;
   rc = fh.GetRec(rid1, rec);
   PrintError(rc);
   assert(rc == RM_INVIABLERID);
   printf("\nOK\n");

   printf("\nTesting GetRec() with too small slot number...\n");
   RID rid2(1, -1);
   rc = fh.GetRec(rid2, rec);
   PrintError(rc);
   assert(rc == RM_INVALIDSLOTNUM);
   printf("\nOK\n");

   printf("\nTesting GetRec() with too large slot number...\n");
   RID rid3(1, 200);
   rc = fh.GetRec(rid3, rec);
   PrintError(rc);
   assert(rc == RM_INVALIDSLOTNUM);
   printf("\nOK\n");

   printf("\nTesting GetRec() with too small page number...\n");
   RID rid4(-1, 0);
   rc = fh.GetRec(rid4, rec);
   PrintError(rc);
   assert(rc == PF_INVALIDPAGE);
   printf("\nOK\n");

   printf("\nTesting GetRec() with too large page number...\n");
   RID rid5(100, 0);
   rc = fh.GetRec(rid5, rec);
   PrintError(rc);
   assert(rc == PF_INVALIDPAGE);
   printf("\nOK\n");

   printf("\nTesting GetRec() with non-existing rid...\n");
   RID rid6(1, 20);
   rc = fh.GetRec(rid6, rec);
   PrintError(rc);
   assert(rc == RM_RECORDNOTFOUND);
   printf("\nOK\n");

   printf("\nTesting UpdateRec() with unread record...\n");
   RM_Record rec1;
   rc = fh.UpdateRec(rec1);
   PrintError(rc);
   assert(rc == RM_UNREADRECORD);
   printf("\nOK\n");

//   PrintRMRecord(rec);

   rc = CloseFile(FILENAME, fh);
   assert(rc == 0);

   printf("\ntest5 done ********************\n");
   return (0);
}

//
// Test6 tests RM_FileHandle class
//
RC Test6(void)
{
   RC            rc;
   RM_FileHandle fh;
   RM_Record     rec;
   RM_FileScan   fs;
   int           val;
   TestRec       *pRecBuf;

   printf("test6 starting ****************\n");

   rc = CreateFile(FILENAME, sizeof(TestRec)+1500);
   assert(rc == 0);

   rc = OpenFile(FILENAME, fh);
   assert(rc == 0);

   rc = AddRecs(fh, FEW_RECS);
   assert(rc == 0);

   rc = PrintFile(fh);
   assert(rc == 0);

   printf("\nopening file scan\n");
   rc = fs.OpenScan(fh, INT, sizeof(int), offsetof(TestRec, num),
                    NO_OP, &val, NO_HINT);
   assert(rc == 0);

   printf("\ngetting first record\n");
   rc = GetNextRecScan(fs, rec);
   assert(rc == 0);
   PrintRMRecord(rec);

   printf("\ndisposing first page\n");

   RID rid1(1,1);
   rc = DeleteRec(fh, rid1);
   assert(rc == 0);

   RID rid2(1,0);
   rc = DeleteRec(fh, rid2);
   assert(rc == 0);

   printf("\ngetting next record (should proceed to next page quietly)\n");
   rc = GetNextRecScan(fs, rec);
   assert(rc == 0);
   PrintRMRecord(rec);

   printf("\nclosing file scan\n");
   rc = fs.CloseScan();
   assert(rc == 0);

   rc = CloseFile(FILENAME, fh);
   assert(rc == 0);

   printf("\ntest6 done (ignore Purify) ********************\n");
   return (0);
}

//
// Test7 tests RM_FileHandle class
//
RC Test7(void)
{
   RC            rc;
   RM_FileHandle fh;
   RM_Record     rec;
   RM_FileScan   fs;
   int           val = 30;
   TestRec       *pRecBuf;
   PageNum       pn;
   SlotNum       sn;

   printf("test7 starting ****************\n");

   rc = CreateFile(FILENAME, sizeof(TestRec)+500);
   assert(rc == 0);

   rc = OpenFile(FILENAME, fh);
   assert(rc == 0);

   rc = AddRecs(fh, FEW_RECS*2);
   assert(rc == 0);

   rc = PrintFile(fh);
   assert(rc == 0);

   for (int i = 0; i < 7; i++) {
      printf("\ndeleting (3,%d)\n", i);
      RID rid1(3,i);
      rc = DeleteRec(fh, rid1);
      assert(rc == 0);
   }

   for (int i = 0; i < 7; i++) {
      printf("\ndeleting (1,%d)\n", i);
      RID rid1(1,i);
      rc = DeleteRec(fh, rid1);
      assert(rc == 0);
   }

   printf("\ndeleting (2,2)\n");
   RID rid1(2,2);
   rc = DeleteRec(fh, rid1);
   assert(rc == 0);

   rc = PrintFile(fh);
   assert(rc == 0);

   printf("\nadding \n");
   rc = AddRecs(fh, FEW_RECS, FEW_RECS*2);
   assert(rc == 0);

   rc = PrintFile(fh);
   assert(rc == 0);

   printf("\nforce \n");
   rc = fh.ForcePages(200);
   PrintError(rc);
   assert(rc == 0);

   printf("\ntest7 done (ignore Purify) ********************\n");

   return (0);
}

