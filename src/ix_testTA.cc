//
// File:        ix_test.cc
// Description: Test IX component
// Authors:     Jan Jannink
//              Dallan Quass (quass@cs.stanford.edu)
//              Jason McHugh (mchughj@cs.stanford.edu)
//
// The program contains 11 tests, numbered 1 - 11, for the IX component.
//
// If the program is given no command-line arguments, it runs all 12 tests.
// You can run only some of the tests by giving the numbers of the tests
// you want to run as command-line arguments.  They can appear in any order.
//

// Changes 1997: This file has been modified to work with Linear Hashing
// without overflow buckets.  Tester 10 will test this stuff.  In
// addition, some code is added to handle the new interface.
//
#include <cstdio>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <cassert>

#include "redbase.h"
#include "pf.h"
#include "rm.h"
#include "ix.h"

using namespace std;

// The PF_STATS indicates that we will be tracking statistics for the PF
// Layer.  The Manager is defined within pf_buffermgr.cc.  Here we must
// place the initializer and then the final call to printout the statistics
// once main has finished
#ifdef PF_STATS
#include "statistics.h"

// This is defined within the pf_buffermgr.cc
extern StatisticsMgr *pStatisticsMgr;

#endif    // PF_STATS

//
// Defines
//
#define FILENAME      "testrel"        // test file name
#define BADFILE       "/abc/def/xyz"   // bad file name
#define STRLEN        95               // length of strings to index
#define FEW_ENTRIES   100    
#define SOME_ENTRIES  2000              // cause a btree to grow to 3 levels
#define MEDIUM_ENTRIES  1000
#define MANY_ENTRIES  2000
#define HUGE_ENTRIES  10000
#define NENTRIES      20000             // Size of values array
#define PROG_UNIT     200              // how frequently to give progress
                                       // reports when adding lots of entries

//
// Values array we will be using for our tests
//
int values[NENTRIES];

//
// Global component manager variables
//
PF_Manager pfm;
RM_Manager rmm(pfm);
IX_Manager ixm(pfm);

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
RC Test8(void);
RC Test9(void);
RC Test10(void);
RC Test11(void);
RC Test12(void);
RC Test13(void);

void PrintError(RC rc);
void LsFiles(char *fileName);
void ran(int n);
RC InsertIntEntries(IX_IndexHandle &ih, int nEntries);
RC InsertFloatEntries(IX_IndexHandle &ih, int nEntries);
RC InsertStringEntries(IX_IndexHandle &ih, int nEntries);
RC AddRecs(RM_FileHandle &fh, int nRecs);
RC DeleteIntEntries(IX_IndexHandle &ih, int nEntries);
RC DeleteFloatEntries(IX_IndexHandle &ih, int nEntries);
RC DeleteStringEntries(IX_IndexHandle &ih, int nEntries);
RC VerifyIntIndex(IX_IndexHandle &ih, int nStart, int nEntries, int bExists);
void CountPages(char *fileName, int index);

// 
// Array of pointers to the test functions
//
#define NUM_TESTS       11              // Number of tests
int (*tests[])() =                      // RC doesn't work on some compilers
{
  Test1, 
  Test2,
  Test3,
  Test4,
  Test5,
  Test6,
  Test7,
  Test8,
  Test9,
  Test10,
  Test11
//  Test12,
//  Test13
};

//
// main
//
int main(int argc, char *argv[])
{
   RC   rc;
   char *progName = argv[0];   // since we will be changing argv
   int  testNum;

#ifdef PF_STATS
   // We must initialize the statistics manager
   pStatisticsMgr = new StatisticsMgr();
#endif

   // Write out initial starting message
   printf("Starting IX component test.\n\n");

   // Init randomize function
   srand( (unsigned)time(NULL));

   // Delete files from last time (if found)
   // Don't check the return codes, since we expect to get an error
   // if the files are not found.
   rmm.DestroyFile(FILENAME);
   ixm.DestroyIndex(FILENAME, 0);
   ixm.DestroyIndex(FILENAME, 1);
   ixm.DestroyIndex(FILENAME, 2);
   ixm.DestroyIndex(FILENAME, 3);

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
   printf("Ending IX component test.\n\n");
   printf("All done with the TA TEST!\n\n");

#ifdef PF_STATS
   delete pStatisticsMgr;
#endif

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
   else if (abs(rc) <= END_IX_WARN)
      IX_PrintError(rc);
   else
      cerr << "Error code out of range: " << rc << "\n";
}

////////////////////////////////////////////////////////////////////
// The following functions may be useful in tests that you devise //
////////////////////////////////////////////////////////////////////

//
// LsFiles
//
// Desc: list the filename's directory entry
// 
void LsFiles(char *fileName)
{
   char command[80];

   sprintf(command, "ls -l *%s*", fileName);
   printf("doing \"%s\"\n", command);
   system(command);
}

//
// Create an array of random nos. between 0 and n-1, without duplicates.
// put the nos. in values[]
//
void ran(int n)
{
   int i, r, t, m;

   // init values array 
   for (i = 0; i < NENTRIES; i++)
      values[i] = i;

   // randomize first n entries in values array
   for (i = 0, m = n; i < n-1; i++) {
      r = (int)(rand() % m--);
      t = values[m];
      values[m] = values[r];
      values[r] = t;
   }
}

//
// InsertIntEntries
//
// Desc: Add a number of integer entries to the index
//
RC InsertIntEntries(IX_IndexHandle &ih, int nEntries)
{
   RC      rc;
   int     i;
   int     value;

   printf("             adding %d int entries\n", nEntries);
   ran(nEntries);
   for(i = 0; i < nEntries; i++) {
     value = values[i] + 1;
     SlotNum slotNum = (SlotNum) ((value * 2) % PF_PAGE_SIZE);
     assert(slotNum >= 0 && slotNum < PF_PAGE_SIZE);
     RID rid(value, slotNum);
      if ((rc = ih.InsertEntry((void *)&value, rid)))
         return (rc);

      if((i + 1) % PROG_UNIT == 0){
         // cast to long for PC's
         printf("\r\t%d%%    ", (int)((i+1)*100L/nEntries));
         fflush(stdout);
      }
   }
   printf("\r\t%d%%      \n", (int)(i*100L/nEntries));

   // Return ok
   return (0);
}

//
// Desc: Add a number of float entries to the index
//
RC InsertFloatEntries(IX_IndexHandle &ih, int nEntries)
{
   RC      rc;
   int     i;
   float   value;

   printf("             adding %d float entries\n", nEntries);
   ran(nEntries);
   for (i = 0; i < nEntries; i++) {
      value = values[i] + 1;
      SlotNum slotNum = (SlotNum) ((((int) (value)) * 2) % PF_PAGE_SIZE);
      assert(slotNum >= 0 && slotNum < PF_PAGE_SIZE);
      RID rid((PageNum) value, slotNum);
      if ((rc = ih.InsertEntry((void *)&value, rid)))
         return (rc);

      if((i + 1) % PROG_UNIT == 0){
         printf("\r\t%d%%    ", (int)((i+1)*100L/nEntries));
         fflush(stdout);
      }
   }
   printf("\r\t%d%%      \n", (int)(i*100L/nEntries));

   // Return ok
   return (0);
}

//
// Desc: Add a number of string entries to the index
//
RC InsertStringEntries(IX_IndexHandle &ih, int nEntries)
{
   RC      rc;
   int     i;
   char    value[STRLEN];

   printf("             adding %d string entries\n", nEntries);
   ran(nEntries);
   for (i = 0; i < nEntries; i++) {
      memset(value, ' ', STRLEN);
      sprintf(value, "number %d", values[i] + 1);
      SlotNum slotNum = (SlotNum) (((values[i] + 1) * 2) % PF_PAGE_SIZE);
      assert(slotNum >= 0 && slotNum < PF_PAGE_SIZE);
      RID rid(values[i] + 1, slotNum);
      if ((rc = ih.InsertEntry(value, rid)))
         return (rc);

      if((i + 1) % PROG_UNIT == 0){
         printf("\r\t%d%%    ", (int)((i+1)*100L/nEntries));
         fflush(stdout);
      }
   }
   printf("\r\t%d%%      \n", (int)(i*100L/nEntries));

   // Return ok
   return (0);
}

//
// AddRecs
//
// Desc: Add a number of integer records to an RM file
//
RC AddRecs(RM_FileHandle &fh, int nRecs)
{
   RC      rc;
   int     i;
   int     value;
   RID     rid;
   PageNum pageNum;
   SlotNum slotNum;

   printf("           adding %d int records to RM file\n", nRecs);
   ran(nRecs);
   for(i = 0; i < nRecs; i++) {
      value = values[i] + 1;
      if ((rc = fh.InsertRec((char *)&value, rid)) ||
            (rc = rid.GetPageNum(pageNum)) ||
            (rc = rid.GetSlotNum(slotNum)))
         return (rc);

      if((i + 1) % PROG_UNIT == 0){
         printf("\r\t%d%%      ", (int)((i+1)*100L/nRecs));
         fflush(stdout);
      }
   }
   printf("\r\t%d%%      \n", (int)(i*100L/nRecs));

   // Return ok
   return (0);
}

//
// DeleteIntEntries: delete a number of integer entries from an index
//
RC DeleteIntEntries(IX_IndexHandle &ih, int nEntries)
{
   RC  rc;
   int i;
   int value;

   printf("        Deleting %d int entries\n", nEntries);
   ran(nEntries);
   for (i = 0; i < nEntries; i++) {
      value = values[i] + 1;
      SlotNum slotNum = (SlotNum) ((value * 2) % PF_PAGE_SIZE);
      assert(slotNum >= 0 && slotNum < PF_PAGE_SIZE);
      RID rid(value, slotNum);
      if ((rc = ih.DeleteEntry((void *)&value, rid)))
         return (rc);

      if((i + 1) % PROG_UNIT == 0){
         printf("\r\t%d%%      ", (int)((i+1)*100L/nEntries));
         fflush(stdout);
      }
   }
   printf("\r\t%d%%      \n", (int)(i*100L/nEntries));

   return (0);
}

//
// DeleteFloatEntries: delete a number of float entries from an index
//
RC DeleteFloatEntries(IX_IndexHandle &ih, int nEntries)
{
   RC  rc;
   int i;
   float value;

   printf("        Deleting %d float entries\n", nEntries);
   ran(nEntries);
   for (i = 0; i < nEntries; i++) {
      value = values[i] + 1;
      SlotNum slotNum = (SlotNum) ((((int) (value)) * 2) % PF_PAGE_SIZE);
      assert(slotNum >= 0 && slotNum < PF_PAGE_SIZE);
      RID rid((PageNum)value, slotNum);
      if ((rc = ih.DeleteEntry((void *)&value, rid)))
         return (rc);

      if((i + 1) % PROG_UNIT == 0){
         printf("\r\t%d%%      ", (int)((i+1)*100L/nEntries));
         fflush(stdout);
      }
   }
   printf("\r\t%d%%      \n", (int)(i*100L/nEntries));

   return (0);
}

//
// Desc: Delete a number of string entries from an index
//
RC DeleteStringEntries(IX_IndexHandle &ih, int nEntries)
{
   RC      rc;
   int     i;
   char    value[STRLEN+1];

   printf("             Deleting %d float entries\n", nEntries);
   ran(nEntries);
   for (i = 0; i < nEntries; i++) {
      sprintf(value, "number %d", values[i] + 1);
      SlotNum slotNum = (SlotNum) (((values[i] + 1) * 2) % PF_PAGE_SIZE);
      assert(slotNum >= 0 && slotNum < PF_PAGE_SIZE);
      RID rid(values[i] + 1, slotNum);
      if ((rc = ih.DeleteEntry(value, rid)))
         return (rc);

      if((i + 1) % PROG_UNIT == 0){
         printf("\r\t%d%%    ", (int)((i+1)*100L/nEntries));
         fflush(stdout);
      }
   }
   printf("\r\t%d%%      \n", (int)(i*100L/nEntries));

   // Return ok
   return (0);
}

//
// VerifyIntIndex
//   - nStart is the starting point in the values array to check
//   - nEntries is the number of entries in the values array to check
//   - If bExists == 1, verify that an index has entries as added
//     by InsertIntEntries.  
//     If bExists == 0, verify that entries do NOT exist (you can 
//     use this to test deleting entries).
//
RC VerifyIntIndex(IX_IndexHandle &ih, int nStart, int nEntries, int bExists)
{
   RC      rc;
   int     i;
   RID     rid;
   IX_IndexScan scan;
   PageNum pageNum;
   SlotNum slotNum;

   // Assume values still contains the array of values inserted/deleted

   printf("Verifying index contents\n");

   for (i = nStart; i < nStart + nEntries; i++) {
      int value = values[i] + 1;

      if ((rc = scan.OpenScan(ih, EQ_OP, &value))) {
         printf("Verify error: opening scan\n");
         return (rc);
      }

      rc = scan.GetNextEntry(rid);
      if (!bExists && rc == 0) {
         printf("Verify error: found non-existent entry %d\n", value);
         return (IX_EOF);  // What should be returned here?
      }
      else if (bExists && rc == IX_EOF) {
         printf("Verify error: entry %d not found\n", value);
         return (IX_EOF);  // What should be returned here?
      }
      else if (rc != 0 && rc != IX_EOF)
         return (rc);

      if (bExists && rc == 0) {
         // Did we get the right entry?
         if ((rc = rid.GetPageNum(pageNum)) ||
               (rc = rid.GetSlotNum(slotNum)))
            return (rc);

	 SlotNum correct_snum = (SlotNum) ((value * 2) % PF_PAGE_SIZE);
         if (pageNum != value || slotNum != correct_snum) {
            printf("Verify error: incorrect rid (%d,%d) found for entry %d\n",
                  pageNum, slotNum, value);             
            return (IX_EOF);  // What should be returned here?
         }

         // Is there another entry?
         rc = scan.GetNextEntry(rid);
         if (rc == 0) {
            printf("Verify error: found two entries with same value %d\n", value);
            return (IX_EOF);  // What should be returned here?
         }
         else if (rc != IX_EOF)
            return (rc);
      }

      if ((rc = scan.CloseScan())) {
         printf("Verify error: closing scan\n");
         return (rc);
      }
   }

   return (0);
}

//
// 
//
void CountPages(char *fileName, int index)
{
   RC            rc;
   PF_FileHandle fh;
   PF_PageHandle ph;
   int           count = 0;
   char          ixName[100];
   char          suffix[10];

   // Try some reasonable combinations to come up with an index filename

   strcpy(ixName, fileName);
   sprintf(suffix, ".%03d", index);
   strcat(ixName, suffix);
   if (pfm.OpenFile(ixName, fh)) {

      strcpy(ixName, fileName);
      sprintf(suffix, ".%d", index);
      strcat(ixName, suffix);
      if (pfm.OpenFile(ixName, fh)) {

         strcpy(ixName, fileName);
         sprintf(suffix, ".ix%d", index);
         strcat(ixName, suffix);
         if (pfm.OpenFile(ixName, fh)) {

            strcpy(ixName, fileName);
            sprintf(suffix, ".index%d", index);
            strcat(ixName, suffix);
            if (pfm.OpenFile(ixName, fh))
               return;
         }
      }
   }

   rc = fh.GetFirstPage(ph);
   while (rc == 0) {
      PageNum pageNum;

      count++;
      if ((rc = ph.GetPageNum(pageNum)) ||
            (rc = fh.UnpinPage(pageNum)) ||
            (rc = fh.GetNextPage(pageNum, ph)))
         break;
   }
   if (rc != PF_EOF) {
      pfm.CloseFile(fh);
      return;
   }

   if (pfm.CloseFile(fh))
      return;

   printf("Number of pages in the file: %d\n", count);
   return;
}

/////////////////////////////////////////////////////////////////////
// Sample test functions follow.                                   //
/////////////////////////////////////////////////////////////////////

//
// Test1 tests simple creation, opening, closing, and deletion of indices
//
RC Test1(void)
{                  
  RC             rc;
   IX_IndexHandle ih;
   int            index=0;

   printf("Test 1: create, open, close, destroy an index... \n");

   if ((rc = ixm.CreateIndex(FILENAME, index, INT, sizeof(int))) ||
         (rc = ixm.OpenIndex(FILENAME, index, ih)) ||
         (rc = ixm.CloseIndex(ih)))
      return (rc);

   LsFiles(FILENAME);

   if ((rc = ixm.DestroyIndex(FILENAME, index)))
      return (rc);

   printf("Passed Test 1\n\n");
   return (0);
}

//
// Test2 tests inserting a few integer entries into the index.
//
RC Test2(void)
{                               
   RC             rc;
   IX_IndexHandle ih;
   int            index=0;

   printf("Test2: insert a few integer entries into an index and verify... \n");

   if ((rc = ixm.CreateIndex(FILENAME, index, INT, sizeof(int))) ||
         (rc = ixm.OpenIndex(FILENAME, index, ih)) ||
         (rc = InsertIntEntries(ih, FEW_ENTRIES)) ||
         (rc = ixm.CloseIndex(ih)) ||
         (rc = ixm.OpenIndex(FILENAME, index, ih)) ||

         // ensure inserted entries are all there
         (rc = VerifyIntIndex(ih, 0, FEW_ENTRIES, 1)) ||

         // ensure an entry not inserted is not there
         (rc = VerifyIntIndex(ih, FEW_ENTRIES, 1, 0)) ||
         (rc = ixm.CloseIndex(ih)))
      return (rc);

   LsFiles(FILENAME);

   if ((rc = ixm.DestroyIndex(FILENAME, index)))
      return (rc);

   printf("Passed Test 2\n\n");
   return (0);
}

//
// Test3 tests deleting a few integer entries from an index
//
RC Test3(void)
{
   RC             rc;
   IX_IndexHandle ih;
   int            index=0;
   int            nDelete = FEW_ENTRIES * 8/10;

   printf("Test3: delete a few integer entries from an index and verify... \n");

   if ((rc = ixm.CreateIndex(FILENAME, index, INT, sizeof(int))) ||
         (rc = ixm.OpenIndex(FILENAME, index, ih)) ||
         (rc = InsertIntEntries(ih, FEW_ENTRIES)) ||
         (rc = DeleteIntEntries(ih, nDelete)) ||
         (rc = ixm.CloseIndex(ih)) ||
         (rc = ixm.OpenIndex(FILENAME, index, ih)) ||
         // ensure deleted entries are gone
         (rc = VerifyIntIndex(ih, 0, nDelete, 0)) ||
         // ensure non-deleted entries still exist
         (rc = VerifyIntIndex(ih, nDelete, FEW_ENTRIES - nDelete, 1)) ||
         (rc = ixm.CloseIndex(ih)))
      return (rc);

   LsFiles(FILENAME);

   if ((rc = ixm.DestroyIndex(FILENAME, index)))
      return (rc);

   printf("Passed Test 3\n\n");
   return (0);
}

//
// Test4 tests inserting different types of entries
//
RC Test4(void)
{
   RC             rc;
   IX_IndexHandle ih1, ih2, ih3;
   int            index1 = 1, index2 = 2, index3 = 3;

   printf("Test4: insert a few entries into integer, float, string indices...\n");

   if ((rc = ixm.CreateIndex(FILENAME, index1, INT, sizeof(int))) ||
         (rc = ixm.OpenIndex(FILENAME, index1, ih1)) ||
         (rc = InsertIntEntries(ih1, MEDIUM_ENTRIES)) ||
         (rc = ixm.CloseIndex(ih1)))
      return (rc);

   if ((rc = ixm.CreateIndex(FILENAME, index2, FLOAT, sizeof(float))) ||
         (rc = ixm.OpenIndex(FILENAME, index2, ih2)) ||
         (rc = InsertFloatEntries(ih2, MEDIUM_ENTRIES)) ||
         (rc = ixm.CloseIndex(ih2)))
      return (rc);

   if ((rc = ixm.CreateIndex(FILENAME, index3, STRING, STRLEN)) ||
         (rc = ixm.OpenIndex(FILENAME, index3, ih3)) ||
         (rc = InsertStringEntries(ih3, FEW_ENTRIES)) ||
         (rc = ixm.CloseIndex(ih3)))
      return (rc);

   LsFiles(FILENAME);

   if ((rc = ixm.DestroyIndex(FILENAME, index1)) ||
         (rc = ixm.DestroyIndex(FILENAME, index2)) ||
         (rc = ixm.DestroyIndex(FILENAME, index3)))
      return (rc);

   printf("Passed Test 4\n\n");
   return (0);
}

//
// Test5 tests inserting many integer entries
//
RC Test5(void)
{
   RC             rc;
   IX_IndexHandle ih;
   int            index=0;

   printf("Test5: insert many integer entries into an index and verify... \n");

   if ((rc = ixm.CreateIndex(FILENAME, index, INT, sizeof(int))) ||
         (rc = ixm.OpenIndex(FILENAME, index, ih)) ||
         (rc = InsertIntEntries(ih, HUGE_ENTRIES)) ||
         (rc = ixm.CloseIndex(ih)) ||
         (rc = ixm.OpenIndex(FILENAME, index, ih)) ||

         // ensure inserted entries are all there
         (rc = VerifyIntIndex(ih, 0, HUGE_ENTRIES, 1)) ||

         // ensure an entry not inserted is not there
         (rc = VerifyIntIndex(ih, HUGE_ENTRIES, 1, 0)) ||
       (rc = ixm.CloseIndex(ih)))
      return (rc);

   LsFiles(FILENAME);
   
   // if ((rc = ixm.DestroyIndex(FILENAME, index)))
   // return (rc);

   printf("Passed Test 5\n\n");
   
   int            nDelete = HUGE_ENTRIES - 10;
   printf(
     "Test8: delete all but a few integer entries from an index and verify... \n");
   
   if((rc = ixm.OpenIndex(FILENAME, index, ih)) ||
      (rc = DeleteIntEntries(ih, nDelete)) ||
      (rc = ixm.CloseIndex(ih)))
     return (rc);

   if ((rc = ixm.OpenIndex(FILENAME, index, ih)) ||
       // ensure deleted entries are gone
       (rc = VerifyIntIndex(ih, 0, nDelete, 0)) ||
       // ensure non-deleted entries still exist
       (rc = VerifyIntIndex(ih, nDelete, HUGE_ENTRIES - nDelete, 1)) ||
       (rc = ixm.CloseIndex(ih)))
     return (rc);
   
   LsFiles(FILENAME);
   
   if ((rc = ixm.DestroyIndex(FILENAME, index)))
     return (rc);
   
   printf("Passed Test 8\n\n");
   
   return (0);
}

//
// Test6 tests inserting many large string entries (test 3-level Btree)
//
RC Test6(void)
{
   RC             rc;
   IX_IndexHandle ih;
   int            index=0;
   char           value[STRLEN];
   int            iValue = 17;   
   RID            rid;
   PageNum        pageNum;
   SlotNum        slotNum;

   printf("Test6: insert large string entries into an index and verify... \n");
   printf("       (try to get a btree to grow to three levels) \n");

   if ((rc = ixm.CreateIndex(FILENAME, index, STRING, STRLEN)) ||
         (rc = ixm.OpenIndex(FILENAME, index, ih)) ||
         (rc = InsertStringEntries(ih, SOME_ENTRIES)) ||
//         (rc = InsertStringEntries(ih, MEDIUM_ENTRIES)) ||
         (rc = ixm.CloseIndex(ih)))
      return (rc);

   if ((rc = ixm.OpenIndex(FILENAME, index, ih)))
      return (rc);

   // Search for an inserted entry
   memset(value, ' ', STRLEN);
   sprintf(value, "number %d", iValue);     // try looking up an entry

   IX_IndexScan scan;

   if ((rc = scan.OpenScan(ih, EQ_OP, &value)))
      return (rc);

   rc = scan.GetNextEntry(rid);
   if (rc == IX_EOF) {
      printf("Verify error: entry %s not found\n", value);
      return (IX_EOF);  // What should be returned here?
   }
   else if (rc != 0)
      return (rc);

   // Did we get the right entry?
   if ((rc = rid.GetPageNum(pageNum)) ||
         (rc = rid.GetSlotNum(slotNum)))
      return (rc);

   if (pageNum != iValue || slotNum != (iValue*2)) {
      printf("Verify error: incorrect rid (%d,%d) found for entry %s\n",
            pageNum, slotNum, value);             
      return (IX_EOF);  // What should be returned here?
   }

   // Is there another entry?
   rc = scan.GetNextEntry(rid);
   if (rc == 0) {
      printf("Verify error: found two entries with same value %s\n", value);
      return (IX_EOF);  // What should be returned here?
   }
   else if (rc != IX_EOF)
      return (rc);

   // Now try looking up an entry that shouldn't be there
   sprintf(value, "number %d", 0);     // 0 should not exist

   if ((rc = scan.CloseScan()))
      return (rc);

   if ((rc = scan.OpenScan(ih, EQ_OP, &value)))
      return (rc);

   rc = scan.GetNextEntry(rid);
   if (rc == 0) {
      printf("Verify error: found non-existent entry %s\n", value);
      return (IX_EOF);  // What should be returned here?
   }
   else if (rc != IX_EOF)
      return (rc);

   if ((rc = scan.CloseScan()))
      return (rc);

   if ((rc = ixm.CloseIndex(ih)))
      return (rc);

   LsFiles(FILENAME);

   if ((rc = ixm.DestroyIndex(FILENAME, index)))
      return (rc);

   printf("Passed Test 6\n\n");
   return (0);
}

//
// Test 7 tests inserting duplicates
//
RC Test7(void)
{
   RC             rc;
   IX_IndexHandle ih;
   int            index=0;
   int            i;
   int            value=100;
   int            nDups=10;

   printf("Test7: insert duplicate-key integer entries into an index... \n");

   if ((rc = ixm.CreateIndex(FILENAME, index, INT, sizeof(int))) ||
         (rc = ixm.OpenIndex(FILENAME, index, ih)) ||
         (rc = InsertIntEntries(ih, MANY_ENTRIES)))
      return (rc);

   // Insert duplicate-key entries
   printf("inserting %d duplicate-key entries\n", nDups);
   for (i = 0; i < nDups; i++) {
      RID rid(value, value*2 + i + 1);
      if ((rc = ih.InsertEntry((void *)&value, rid)))
         return (rc);
   }

   // Now count all entries for value
   RID rid;
   IX_IndexScan scan;

   if ((rc = scan.OpenScan(ih, EQ_OP, &value)))
      return (rc);

   i = 0;
   while (!(rc = scan.GetNextEntry(rid))) {
      i++;
   }
   if (rc != IX_EOF)
      return (rc);

   if (i != nDups + 1) {
      printf("Didn't find correct number of duplicates\n");
      return (IX_EOF); // What should be returned here?
   }

   if ((rc = scan.CloseScan()))
      return (rc);

   if ((rc = ixm.CloseIndex(ih)))
      return (rc);

   LsFiles(FILENAME);

   if ((rc = ixm.DestroyIndex(FILENAME, index)))
      return (rc);

   printf("Passed Test 7\n\n");
   
   printf("Test7b: insert lots of duplicate-key integer entries into an index... \n");
   nDups = MEDIUM_ENTRIES;

   if ((rc = ixm.CreateIndex(FILENAME, index, INT, sizeof(int))) ||
         (rc = ixm.OpenIndex(FILENAME, index, ih)) ||
         (rc = InsertIntEntries(ih, MANY_ENTRIES)))
      return (rc);

   // Insert duplicate-key entries
   printf("inserting %d duplicate-key entries\n", nDups);
   for (i = 0; i < nDups; i++) {
      RID rid(value, value*2 + i + 1);
      if ((rc = ih.InsertEntry((void *)&value, rid)))
         return (rc);
   }

   // Now count all entries for value

   if ((rc = scan.OpenScan(ih, EQ_OP, &value)))
      return (rc);

   i = 0;
   while (!(rc = scan.GetNextEntry(rid))) {
      i++;
   }
   if (rc != IX_EOF)
      return (rc);

   if (i != nDups + 1) {
      printf("Didn't find correct number of duplicates\n");
      return (IX_EOF); // What should be returned here?
   }

   if ((rc = scan.CloseScan()))
      return (rc);

   if ((rc = ixm.CloseIndex(ih)))
      return (rc);

   LsFiles(FILENAME);

   if ((rc = ixm.DestroyIndex(FILENAME, index)))
      return (rc);

   printf("Passed Test 7\n\n");
   return (0);
}

//
// Test8 tests deleting many integer entries from an index
// Currently done as part of Test5
//
RC Test8(void)
{ return 0; }

//
// Test 9 tests scan and delete loop
//
RC Test9(void)
{
   RC             rc;
   IX_IndexHandle ih;
   int            index=0;
   int            i;
   int            value=100;
   int            nDups=40;
   RID            rid;

   printf("Test9: scan and delete loop... \n");

   if ((rc = ixm.CreateIndex(FILENAME, index, INT, sizeof(int))) ||
         (rc = ixm.OpenIndex(FILENAME, index, ih)) ||
         (rc = InsertIntEntries(ih, MANY_ENTRIES)))
      return (rc);

   // Insert duplicate-key entries
   for (i = 0; i < nDups; i++) {
      RID rid(value, value*2 + i + 1);
      if ((rc = ih.InsertEntry((void *)&value, rid)))
         return (rc);
   }

   // Now scan and delete all entries for value
   IX_IndexScan scan;

   if ((rc = scan.OpenScan(ih, EQ_OP, &value)))
      return (rc);

   i = 0;
   while (!(rc = scan.GetNextEntry(rid))) {
      if ((rc = ih.DeleteEntry((void *)&value, rid)))
         return (rc);
      i++;
      if (i > nDups + 1) {
         printf("Scan and delete loop found too many entries\n");
         return (IX_EOF); // What should be returned here?
      }
   }
   if (rc != IX_EOF)
      return (rc);

   if (i != nDups + 1) {
      printf("Scan and delete loop didn't find correct number of entries\n");
      return (IX_EOF); // What should be returned here?
   }

   if ((rc = scan.CloseScan()))
      return (rc);

   if ((rc = ixm.CloseIndex(ih)))
      return (rc);

   LsFiles(FILENAME);

   if ((rc = ixm.DestroyIndex(FILENAME, index)))
      return (rc);

   printf("Passed Test 9\n\n");
   return (0);
}

//
// Test 10 tests a few inequality scans on Btree indices
//
RC Test10(void)
{
   RC             rc;
   IX_IndexHandle ih;
   int            index=0;      
   int            i;
   int            value=MANY_ENTRIES/2;
   RID            rid;

   printf("Test10: inequality scans... \n");

   if ((rc = ixm.CreateIndex(FILENAME, index, INT, sizeof(int))) ||
         (rc = ixm.OpenIndex(FILENAME, index, ih)) ||
         (rc = InsertIntEntries(ih, MANY_ENTRIES)))
      return (rc);

   // Scan <
   IX_IndexScan scanlt; //(ih, LT_OP, &value);
   if ((rc = scanlt.OpenScan(ih, LT_OP, &value)))
       return (rc);

   i = 0;
   while (!(rc = scanlt.GetNextEntry(rid))) {
      i++;
   }
//   if (rc == IX_UNSUPPORTED) {
//
//      // They must have implemented dynamic hashing
//      printf ("Dynamic hash indices are exempt from test 10\n\n");
//      if ((rc = ixm.CloseIndex(ih)) ||
//            (rc = ixm.DestroyIndex(FILENAME, index)))
//         return (rc);
//      else
//         return (0);
//   }
   if (rc != IX_EOF)
      return (rc);

   if (i != value - 1) {
      printf("LT_OP scan error: found %d entries, expected %d\n", i, value - 1);
      return (IX_EOF); // What should be returned here?
   }

   // Scan <=
   IX_IndexScan scanle;
   if ((rc = scanle.OpenScan(ih, LE_OP, &value)))
       return (rc);

   i = 0;
   while (!(rc = scanle.GetNextEntry(rid))) {
      i++;
   }
   if (rc != IX_EOF)
      return (rc);

   if (i != value) {
      printf("LE_OP scan error: found %d entries, expected %d\n", i, value);
      return (IX_EOF); // What should be returned here?
   }

   // Scan >
   IX_IndexScan scangt;
   if ((rc = scangt.OpenScan(ih, GT_OP, &value)))
       return (rc);

   i = 0;
   while (!(rc = scangt.GetNextEntry(rid))) {
      i++;
   }
   if (rc != IX_EOF)
      return (rc);

   if (i != MANY_ENTRIES - value) {
      printf("GT_OP scan error: found %d entries, expected %d\n", 
            i, MANY_ENTRIES - value);
      return (IX_EOF); // What should be returned here?
   }

   // Scan >=
   IX_IndexScan scange;
   if ((rc = scange.OpenScan(ih, GE_OP, &value)))
       return (rc);

   i = 0;
   while (!(rc = scange.GetNextEntry(rid))) {
      i++;
   }
   if (rc != IX_EOF)
      return (rc);

   if (i != MANY_ENTRIES - value + 1) {
      printf("GE_OP scan error: found %d entries, expected %d\n", 
            i, MANY_ENTRIES - value + 1);
      return (IX_EOF); // What should be returned here?
   }

   if ((rc = ixm.CloseIndex(ih)))
      return (rc);

   LsFiles(FILENAME);

   if ((rc = ixm.DestroyIndex(FILENAME, index)))
      return (rc);

   printf("Passed Test 10\n\n");
   return (0);
}

#if 0
//
// Test 11 tests building an index
//
RC Test11(void)
{
   RC             rc;
   IX_IndexHandle ih;
   RM_FileHandle  fh;
   int            index=0;
   RID            rid;
   RM_Record      rec;

   printf("Test11: build an index... \n");

   if ((rc = rmm.CreateFile(FILENAME, sizeof(int))) ||
         (rc = rmm.OpenFile(FILENAME, fh)) ||
         (rc = AddRecs(fh, MANY_ENTRIES)) ||
         (rc = rmm.CloseFile(fh)))
      return (rc);

   if ((rc = ixm.CreateIndex(FILENAME, index, INT, sizeof(int))) ||
         (rc = ixm.BuildIndex(FILENAME, index, 0)) ||
         (rc = ixm.OpenIndex(FILENAME, index, ih)) ||
         (rc = rmm.OpenFile(FILENAME, fh)))
      return (rc);

   LsFiles(FILENAME);

   // Verify all records indexed
   for (int i = 0; i < MANY_ENTRIES; i++) {
      int value = values[i] + 1;
      IX_IndexScan scan(ih, EQ_OP, &value);

      rc = scan.GetNextEntry(rid);
      if (rc == IX_EOF) {
         printf("Error: entry %d not found\n", value);
         return (IX_EOF);  // What should be returned here?
      }
      else if (rc != 0)
         return (rc);

      // Look up the record and compare value
      int *pValue;
      if ((rc = fh.GetThisRec(rid, rec)) ||
            (rc = rec.GetData((char **)&pValue)))
         return (rc);

      if (value != *pValue) {
         printf("Error: index entry %d references record %d\n", value, *pValue);
         return (IX_EOF);  // What should be returned here?
      }
   }

   if ((rc = ixm.CloseIndex(ih)) ||
         (rc = rmm.CloseFile(fh)) ||
         (rc = ixm.DestroyIndex(FILENAME, index)) ||
         (rc = rmm.DestroyFile(FILENAME)))
      return (rc);

   printf("Passed Test 11\n\n");
   return (0);
}

//
// Test 10 tests extendible hashing without overflow buckets.  This test
// will attempt to insert too many duplicate key entries.  It should not
// be able to insert all of the entries and the student's component
// should flag and return a warning at some point.
//
// 1998 :: The test has been altered since the student's are
// implementing Linear hashing with overflow buckets.  So this just
// tests insertion of lots of duplicates.
//
RC Test10(void)
{
   RC             rc;
   IX_IndexHandle ih;
   int            index=0;
   int            i;
   int            value=100;
   int            nDups=2000;

   printf("Test10: Insert too many duplicate-key integer entries into an index... \n");

   if ((rc = ixm.CreateIndex(FILENAME, index, INT, sizeof(int))) ||
         (rc = ixm.OpenIndex(FILENAME, index, ih)))
      return (rc);

   // Initially no error
   rc = 0;

   // Insert duplicate-key entries
   printf("Inserting %d duplicate-key entries\n", nDups);
   for (i = 0; i < nDups; i++) {
      RID rid(value, value*2 + i + 1);
      if ((rc = ih.InsertEntry((void *)&value, rid)))
         break;

      if((i + 1) % PROG_UNIT == 0){
         printf("\r\t%d%%      ", (int)((i+1)*100L/nDups));
         fflush(stdout);
      }
   }
   printf("\r\t%d%%      \n", (int)(i*100L/nDups));

   /* 
    * 1997::Uncomment this out.  Without overflow then the below is
    * valid. 

   // Now there should have been an error and i had better not be ==
   // nDups.
   if (!rc) {
      printf("ERROR: The component was able to insert all the entries.\n");
      printf("Without overflow buckets this should have happened.\n");
      return (-1);
   }

   */

   printf ("Inserted %d entries.\n", i);

   if ((rc = ixm.CloseIndex(ih)))
      return (rc);

   if ((rc = ixm.DestroyIndex(FILENAME, index)))
      return (rc);

   printf("Passed Test 10\n\n");
   return (0);
}
#endif

//
// Test 11 tests error conditions
//
RC Test11(void)
{
   RC             rc;
   IX_IndexHandle ih;
   int            index=0;
   int            value=17;

   printf("Test11: Test a few error conditions... \n");

   printf("\nCreate an index with a bad file name\n");
   if (!(rc = ixm.CreateIndex(BADFILE, index, INT, sizeof(int)))) {
      printf("ERROR: CreateIndex should have failed.\n");
      ixm.DestroyIndex(BADFILE, index);
   }
   else
      PrintError(rc);

   printf("\nBad attribute type\n");
   if (!(rc = ixm.CreateIndex(FILENAME, index, (AttrType)9, sizeof(int)))) {
      printf("ERROR: CreateIndex should have failed.\n");
      ixm.DestroyIndex(FILENAME, index);
   }
   else
      PrintError(rc);

   printf("\nBad attribute length\n");
   if (!(rc = ixm.CreateIndex(FILENAME, index, STRING, 0))) {
      printf("ERROR: CreateIndex should have failed.\n");
      ixm.DestroyIndex(FILENAME, index);
   }
   else
      PrintError(rc);

   printf("\nOpen non-existing index\n");
   if (!(rc = ixm.OpenIndex(FILENAME, 99, ih))) {
      printf("ERROR: OpenIndex should have failed.\n");
   }
   else
      PrintError(rc);

   // Create and populate an index
   printf("\nCreate and populate an index\n");
   if ((rc = ixm.CreateIndex(FILENAME, index, INT, sizeof(int))) ||
         (rc = ixm.OpenIndex(FILENAME, index, ih)) ||
         (rc = InsertIntEntries(ih, FEW_ENTRIES)))
      return (rc);

   printf("\nInsert a duplicate entry\n");
   RID rid(value, value*2);
   if (!(rc = ih.InsertEntry((void *)&value, rid)))
      printf("ERROR: insert duplicate entry should have failed.\n");
   else
      PrintError(rc);

   printf("\nDelete a non-existing entry\n");
   RID rid2(value, value*2 - 1);
   if (!(rc = ih.DeleteEntry((void *)&value, rid2)))
      printf("ERROR: delete a non-existing entry should have failed.\n");
   else
      PrintError(rc);

   // Create an index scan
   IX_IndexScan scan;
   if ((rc = scan.OpenScan(ih, EQ_OP, &value))) {
      printf("ERROR: Can't open scan.\n");
      PrintError(rc);
   }
   else {
      // Close the index
      printf("\nclose the index while a scan is still active\n");
      if (!(rc = ixm.CloseIndex(ih)) &&
            !(rc = scan.GetNextEntry(rid)))
         printf("ERROR: Scan on a closed index should have failed.\n");
      else
         PrintError(rc);
      scan.CloseScan();
   }

   // Close the index again in case an error was returned the last time
   ixm.CloseIndex(ih);

   printf("\ninsert into a closed index\n");
   RID rid3(1, 1);
   if (!(rc = ih.InsertEntry((void *)&value, rid3)))
      printf("ERROR: Insert into a closed index should have failed.\n");
   else
      PrintError(rc);

   printf("\ndelete from a closed index\n");
   RID rid4(value, value*2);
   if (!(rc = ih.DeleteEntry((void *)&value, rid4)))
      printf("ERROR: Delete from a closed index should have failed.\n");
   else
      PrintError(rc);

   // Clean up
   if ((rc = ixm.DestroyIndex(FILENAME, index)))
      return (rc);

   printf("Passed Test 11\n\n");
   return (0);
}

//
// Test 12 tests scan and update (delete + insert) loop, where the
// new values are different
//
RC Test12(void)
{
   RC             rc;
   IX_IndexHandle ih;
   int            index=0;
   int            i;
   int            value_1=100;
   int            value_2=200;
   int            nDups=2;
   RID            rid;

   printf("Test9: scan and update loop... \n");

   if ((rc = ixm.CreateIndex(FILENAME, index, INT, sizeof(int))) ||
         (rc = ixm.OpenIndex(FILENAME, index, ih)) ||
         (rc = InsertIntEntries(ih, MANY_ENTRIES)))
      return (rc);

   // Insert duplicate-key entries
   for (i = 0; i < nDups; i++) {
      RID rid(value_1, value_1*2 + i + 1);
      if ((rc = ih.InsertEntry((void *)&value_1, rid)))
         return (rc);
   }

   // Now scan and delete all entries for value
   IX_IndexScan scan;

   if ((rc = scan.OpenScan(ih, EQ_OP, &value_1)))
      return (rc);

   // update scan
   i = 0;
   while (!(rc = scan.GetNextEntry(rid))) {

     if ((rc = ih.DeleteEntry((void *)&value_1, rid)))
       return (rc);
     
     if ((rc = ih.InsertEntry((void *)&value_2, rid)))
       return (rc);
     
     i++;
     if (i > nDups + 1) {
       printf("Point 1: Scan and update loop found too many entries, i == %d\n", i);
       return (IX_EOF); // What should be returned here?
     }
   }
   if (rc != IX_EOF)
     return (rc);
   
   if (i != nDups + 1) {
      printf("Scan and update loop didn't find correct number of entries\n");
      return (IX_EOF); // What should be returned here?
   }
   
   if ((rc = scan.CloseScan()))
     return (rc);
   
   if ((rc = scan.OpenScan(ih, EQ_OP, &value_2)))
     return (rc);
   
   i = 0;
   while (!(rc = scan.GetNextEntry(rid))) {
     i++;
     if (i > nDups + 2) {
       printf("Point 2: Scan and update loop found too many entries, i == %d\n",i);
       return (IX_EOF); // What should be returned here?
     }
   }
   if (rc != IX_EOF)
     return (rc);

   if (i != nDups + 2) {
     printf("Scan and update loop didn't find correct number of entries\n");
     return (IX_EOF); // What should be returned here?
   }

   if ((rc = scan.CloseScan()))
     return (rc);
   
   if ((rc = ixm.CloseIndex(ih)))
      return (rc);

   LsFiles(FILENAME);

   if ((rc = ixm.DestroyIndex(FILENAME, index)))
      return (rc);

   printf("Passed Test 12\n\n");
   return (0);
}

//
// Test 13 tests scan and update (delete + insert) loop, where the
// new values are the same
//
RC Test13(void)
{
   RC             rc;
   IX_IndexHandle ih;
   int            index=0;
   int            i;
   int            value_1=100;
   int            nDups=2;
   RID            rid;

   printf("Test13: Second scan and update loop... \n");

   if ((rc = ixm.CreateIndex(FILENAME, index, INT, sizeof(int))) ||
         (rc = ixm.OpenIndex(FILENAME, index, ih)) ||
         (rc = InsertIntEntries(ih, MANY_ENTRIES)))
      return (rc);

   // Insert duplicate-key entries
   for (i = 0; i < nDups; i++) {
      RID rid(value_1, value_1*2 + i + 1);
      if ((rc = ih.InsertEntry((void *)&value_1, rid)))
         return (rc);
   }

   // Now scan and delete all entries for value
   IX_IndexScan scan;

   if ((rc = scan.OpenScan(ih, EQ_OP, &value_1)))
      return (rc);

   // update scan
   i = 0;
   while (!(rc = scan.GetNextEntry(rid))) {

     if ((rc = ih.DeleteEntry((void *)&value_1, rid)))
       return (rc);
     
     if ((rc = ih.InsertEntry((void *)&value_1, rid)))
       return (rc);
     
     i++;
     if (i > nDups + 1) {
       printf("Point 1: Scan and update loop found too many entries, i == %d\n", i);
       return (IX_EOF); // What should be returned here?
     }
   }
   if (rc != IX_EOF)
     return (rc);
   
   if (i != nDups + 1) {
      printf("Scan and update loop didn't find correct number of entries\n");
      return (IX_EOF); // What should be returned here?
   }
   
   if ((rc = scan.CloseScan()))
     return (rc);
   
   if ((rc = scan.OpenScan(ih, EQ_OP, &value_1)))
     return (rc);
   
   i = 0;
   while (!(rc = scan.GetNextEntry(rid))) {
     i++;
     if (i > nDups + 1) {
       printf("Point 2: Scan and update loop found too many entries, i == %d\n",i);
       return (IX_EOF); // What should be returned here?
     }
   }
   if (rc != IX_EOF)
     return (rc);

   if (i != nDups + 1) {
     printf("Scan and update loop didn't find correct number of entries\n");
     return (IX_EOF); // What should be returned here?
   }

   if ((rc = scan.CloseScan()))
     return (rc);
   
   if ((rc = ixm.CloseIndex(ih)))
      return (rc);

   LsFiles(FILENAME);

   if ((rc = ixm.DestroyIndex(FILENAME, index)))
      return (rc);

   printf("Passed Test 13\n\n");
   return (0);
}
