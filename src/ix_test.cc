//
// File:        ix_testshell.cc
// Description: Test IX component
// Authors:     Jan Jannink
//              Dallan Quass (quass@cs.stanford.edu)
//
// This test shell contains a number of functions that will be useful in
// testing your IX component code.  In addition, a couple of sample
// tests are provided.  The tests are by no means comprehensive, you are
// expected to devise your own tests to test your code.
//
// 1997:  Tester has been modified to reflect the change in the 1997
// interface.
// 2000:  Tester has been modified to reflect the change in the 2000
// interface.

#include <cstdio>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <ctime>

#include "tinybase.h"
#include "pf.h"
#include "rm.h"
#include "ix.h"

using namespace std;

//
// Defines
//
#define FILENAME     "testrel"        // test file name
#define BADFILE      "/abc/def/xyz"   // bad file name
#define STRLEN       39               // length of strings to index
#define FEW_ENTRIES  20
#define MANY_ENTRIES 1000
#define NENTRIES     5000             // Size of values array
#define PROG_UNIT    200              // how frequently to give progress
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
RC PrintIndex(IX_IndexHandle &ih);

//
// Array of pointers to the test functions
//
#define NUM_TESTS       10               // number of tests
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
    Test10
};

//
// main
//
int main(int argc, char *argv[]) {
	RC rc;
	char *progName = argv[0];   // since we will be changing argv
	int testNum;

	// Write out initial starting message
	printf("Starting IX component test.\n\n");

	// Init randomize function
	srand((unsigned) time(NULL));

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
	} else {
		// Otherwise, perform specific tests
		while (*++argv != NULL) {

			// Make sure it's a number
			if (sscanf(*argv, "%d", &testNum) != 1) {
				cerr << progName << ": " << *argv << " is not a number\n";
				continue;
			}

			// Make sure it's in range
			if (testNum < 1 || testNum > NUM_TESTS) {
				cerr << "Valid test numbers are between 1 and " << NUM_TESTS
						<< "\n";
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

	return (0);
}

//
// PrintError
//
// Desc: Print an error message by calling the proper component-specific
//       print-error function
//
void PrintError(RC rc) {
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
void LsFiles(char *fileName) {
	char command[80];

	sprintf(command, "ls -l *%s*", fileName);
	printf("Doing \"%s\"\n", command);
	system(command);
}

//
// Create an array of random nos. between 0 and n-1, without duplicates.
// put the nos. in values[]
//
void ran(int n) {
	int i, r, t, m;

	// Initialize values array
	for (i = 0; i < NENTRIES; i++)
		values[i] = i;

	// Randomize first n entries in values array
	for (i = 0, m = n; i < n - 1; i++) {
		r = (int) (rand() % m--);
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
RC InsertIntEntries(IX_IndexHandle &ih, int nEntries) {
	RC rc;
	int i;
	int value;

	printf("             Adding %d int entries\n", nEntries);
	ran(nEntries);
	for (i = 0; i < nEntries; i++) {
		value = values[i] + 1;
		RID rid(value, value * 2);
		if ((rc = ih.InsertEntry((void *) &value, rid)))
			return (rc);

		if ((i + 1) % PROG_UNIT == 0) {
			// cast to long for PC's
			printf("\r\t%d%%    ", (int) ((i + 1) * 100L / nEntries));
			fflush (stdout);
		}
	}
	printf("\r\t%d%%      \n", (int) (i * 100L / nEntries));

	// Return ok
	return (0);
}

//
// Desc: Add a number of float entries to the index
//
RC InsertFloatEntries(IX_IndexHandle &ih, int nEntries) {
	RC rc;
	int i;
	float value;

	printf("             Adding %d float entries\n", nEntries);
	ran(nEntries);
	for (i = 0; i < nEntries; i++) {
		value = values[i] + 1;
		RID rid((PageNum) value, (SlotNum) value * 2);
		if ((rc = ih.InsertEntry((void *) &value, rid)))
			return (rc);

		if ((i + 1) % PROG_UNIT == 0) {
			printf("\r\t%d%%    ", (int) ((i + 1) * 100L / nEntries));
			fflush (stdout);
		}
	}
	printf("\r\t%d%%      \n", (int) (i * 100L / nEntries));

	// Return ok
	return (0);
}

//
// Desc: Add a number of string entries to the index
//
RC InsertStringEntries(IX_IndexHandle &ih, int nEntries) {
	RC rc;
	int i;
	char value[STRLEN];

	printf("             Adding %d string entries\n", nEntries);
	ran(nEntries);
	for (i = 0; i < nEntries; i++) {
		memset(value, ' ', STRLEN);
		sprintf(value, "number %d", values[i] + 1);
		RID rid(values[i] + 1, (values[i] + 1) * 2);
		if ((rc = ih.InsertEntry(value, rid)))
			return (rc);

		if ((i + 1) % PROG_UNIT == 0) {
			printf("\r\t%d%%    ", (int) ((i + 1) * 100L / nEntries));
			fflush (stdout);
		}
	}
	printf("\r\t%d%%      \n", (int) (i * 100L / nEntries));

	// Return ok
	return (0);
}

//
// AddRecs
//
// Desc: Add a number of integer records to an RM file
//
RC AddRecs(RM_FileHandle &fh, int nRecs) {
	RC rc;
	int i;
	int value;
	RID rid;
	PageNum pageNum;
	SlotNum slotNum;

	printf("           Adding %d int records to RM file\n", nRecs);
	ran(nRecs);
	for (i = 0; i < nRecs; i++) {
		value = values[i] + 1;
		if ((rc = fh.InsertRec((char *) &value, rid))
				|| (rc = rid.GetPageNum(pageNum))
				|| (rc = rid.GetSlotNum(slotNum)))
			return (rc);

		if ((i + 1) % PROG_UNIT == 0) {
			printf("\r\t%d%%      ", (int) ((i + 1) * 100L / nRecs));
			fflush (stdout);
		}
	}
	printf("\r\t%d%%      \n", (int) (i * 100L / nRecs));

	// Return ok
	return (0);
}

//
// DeleteIntEntries: delete a number of integer entries from an index
//
RC DeleteIntEntries(IX_IndexHandle &ih, int nEntries) {
	RC rc;
	int i;
	int value;

	printf("        Deleting %d int entries\n", nEntries);
	ran(nEntries);
	for (i = 0; i < nEntries; i++) {
		value = values[i] + 1;
		RID rid(value, value * 2);
		if ((rc = ih.DeleteEntry((void *) &value, rid)))
			return (rc);

		if ((i + 1) % PROG_UNIT == 0) {
			printf("\r\t%d%%      ", (int) ((i + 1) * 100L / nEntries));
			fflush (stdout);
		}
	}
	printf("\r\t%d%%      \n", (int) (i * 100L / nEntries));

	return (0);
}

//
// DeleteFloatEntries: delete a number of float entries from an index
//
RC DeleteFloatEntries(IX_IndexHandle &ih, int nEntries) {
	RC rc;
	int i;
	float value;

	printf("        Deleting %d float entries\n", nEntries);
	ran(nEntries);
	for (i = 0; i < nEntries; i++) {
		value = values[i] + 1;
		RID rid((PageNum) value, (SlotNum) value * 2);
		if ((rc = ih.DeleteEntry((void *) &value, rid)))
			return (rc);

		if ((i + 1) % PROG_UNIT == 0) {
			printf("\r\t%d%%      ", (int) ((i + 1) * 100L / nEntries));
			fflush (stdout);
		}
	}
	printf("\r\t%d%%      \n", (int) (i * 100L / nEntries));

	return (0);
}

//
// Desc: Delete a number of string entries from an index
//
RC DeleteStringEntries(IX_IndexHandle &ih, int nEntries) {
	RC rc;
	int i;
	char value[STRLEN + 1];

	printf("             Deleting %d float entries\n", nEntries);
	ran(nEntries);
	for (i = 0; i < nEntries; i++) {
		sprintf(value, "number %d", values[i] + 1);
		RID rid(values[i] + 1, (values[i] + 1) * 2);
		if ((rc = ih.DeleteEntry(value, rid)))
			return (rc);

		if ((i + 1) % PROG_UNIT == 0) {
			printf("\r\t%d%%    ", (int) ((i + 1) * 100L / nEntries));
			fflush (stdout);
		}
	}
	printf("\r\t%d%%      \n", (int) (i * 100L / nEntries));

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
RC VerifyIntIndex(IX_IndexHandle &ih, int nStart, int nEntries, int bExists) {
	RC rc;
	int i;
	RID rid;
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
            ih.PrintTree();
			return (IX_EOF);  // What should be returned here?
		} else if (bExists && rc == IX_EOF) {
            printf("Verify error: entry N°%d (%d) not found\n", i, value);
            ih.PrintTree();
			return (IX_EOF);  // What should be returned here?
        } else if (rc != 0 && rc != IX_EOF){
            ih.PrintTree();
            printf("Value looked for: %d\n", value);
            return (rc);
        }
		if (bExists && rc == 0) {
			// Did we get the right entry?
			if ((rc = rid.GetPageNum(pageNum))
					|| (rc = rid.GetSlotNum(slotNum)))
				return (rc);

			if (pageNum != value || slotNum != (value * 2)) {
				printf(
						"Verify error: incorrect rid (%d,%d) found for entry %d\n",
						pageNum, slotNum, value);
				return (IX_EOF);  // What should be returned here?
			}

			// Is there another entry?
			rc = scan.GetNextEntry(rid);
			if (rc == 0) {
				printf("Verify error: found two entries with same value %d\n",
						value);
				return (IX_EOF);  // What should be returned here?
			} else if (rc != IX_EOF)
				return (rc);
		}

		if ((rc = scan.CloseScan())) {
			printf("Verify error: closing scan\n");
			return (rc);
		}
	}

	return (0);
}

/////////////////////////////////////////////////////////////////////
// Sample test functions follow.                                   //
/////////////////////////////////////////////////////////////////////

//
// Test1 tests simple creation, opening, closing, and deletion of indices
//
RC Test1(void) {
	RC rc;
	int index = 0;
	IX_IndexHandle ih;

	printf("Test 1: create, open, close, delete an index... \n");

    if ((rc = ixm.CreateIndex(FILENAME, index, INT, sizeof(int)))
			|| (rc = ixm.OpenIndex(FILENAME, index, ih)) || (rc =
					ixm.CloseIndex(ih)))
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
RC Test2(void) {
	RC rc;
	IX_IndexHandle ih;
	int index = 0;

	printf("Test2: Insert a few integer entries into an index... \n");

    if ((rc = ixm.CreateIndex(FILENAME, index, INT, sizeof(int)))
			|| (rc = ixm.OpenIndex(FILENAME, index, ih)) || (rc =
                    InsertIntEntries(ih, MANY_ENTRIES))
			|| (rc = ixm.CloseIndex(ih))
			|| (rc = ixm.OpenIndex(FILENAME, index, ih))
			||

			// ensure inserted entries are all there
            (rc = VerifyIntIndex(ih, 0, MANY_ENTRIES, TRUE))
			||

			// ensure an entry not inserted is not there
            (rc = VerifyIntIndex(ih, MANY_ENTRIES, 1, FALSE))
			|| (rc = ixm.CloseIndex(ih)))
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
RC Test3(void) {
	RC rc;
	int index = 0;
    int nDelete = MANY_ENTRIES * 8 / 10;
	IX_IndexHandle ih;

	printf("Test3: Delete a few integer entries from an index... \n");

	if ((rc = ixm.CreateIndex(FILENAME, index, INT, sizeof(int)))
			|| (rc = ixm.OpenIndex(FILENAME, index, ih)) || (rc =
                    InsertIntEntries(ih, MANY_ENTRIES)) || (rc =
					DeleteIntEntries(ih, nDelete)) || (rc = ixm.CloseIndex(ih))
			|| (rc = ixm.OpenIndex(FILENAME, index, ih))
			||
			// ensure deleted entries are gone
			(rc = VerifyIntIndex(ih, 0, nDelete, FALSE))
			||
			// ensure non-deleted entries still exist
            (rc = VerifyIntIndex(ih, nDelete, MANY_ENTRIES - nDelete, TRUE))
			|| (rc = ixm.CloseIndex(ih)))
		return (rc);

	LsFiles(FILENAME);

	if ((rc = ixm.DestroyIndex(FILENAME, index)))
		return (rc);

	printf("Passed Test 3\n\n");
	return (0);
}

//
// Test 4 tests a few inequality scans on Btree indices
//
RC Test4(void) {
	RC rc;
	IX_IndexHandle ih;
	int index = 0;
	int i;
	int value = FEW_ENTRIES / 2;
	RID rid;

	printf("Test4: Inequality scans... \n");

	if ((rc = ixm.CreateIndex(FILENAME, index, INT, sizeof(int)))
			|| (rc = ixm.OpenIndex(FILENAME, index, ih)) || (rc =
					InsertIntEntries(ih, FEW_ENTRIES)))
		return (rc);

    // Scan <
	IX_IndexScan scanlt;
	if ((rc = scanlt.OpenScan(ih, LT_OP, &value))) {
		printf("Scan error: opening scan\n");
		return (rc);
	}

	i = 0;
	while (!(rc = scanlt.GetNextEntry(rid))) {
		i++;
	}

	if (rc != IX_EOF)
		return (rc);

	printf("Found %d entries in <-scan.", i);

	// Scan <=
	IX_IndexScan scanle;
	if ((rc = scanle.OpenScan(ih, LE_OP, &value))) {
		printf("Scan error: opening scan\n");
		return (rc);
	}

	i = 0;
	while (!(rc = scanle.GetNextEntry(rid))) {
		i++;
	}
	if (rc != IX_EOF)
		return (rc);

	printf("Found %d entries in <=-scan.\n", i);

	// Scan >
	IX_IndexScan scangt;
	if ((rc = scangt.OpenScan(ih, GT_OP, &value))) {
		printf("Scan error: opening scan\n");
		return (rc);
	}

	i = 0;
	while (!(rc = scangt.GetNextEntry(rid))) {
		i++;
	}
	if (rc != IX_EOF)
		return (rc);

	printf("Found %d entries in >-scan.\n", i);

	// Scan >=
	IX_IndexScan scange;
	if ((rc = scange.OpenScan(ih, GE_OP, &value))) {
		printf("Scan error: opening scan\n");
		return (rc);
	}

	i = 0;
	while (!(rc = scange.GetNextEntry(rid))) {
		i++;
	}
	if (rc != IX_EOF)
		return (rc);

	printf("Found %d entries in >=-scan.\n", i);

	if ((rc = ixm.CloseIndex(ih)))
		return (rc);

	LsFiles(FILENAME);

	if ((rc = ixm.DestroyIndex(FILENAME, index)))
		return (rc);

	printf("Passed Test 4\n\n");
	return (0);
}



RC Test5(void) {
    RC rc;
    IX_IndexHandle ih;
    PF_Manager pfm;
    IX_Manager m(pfm);
    IX_IndexScan is;

    printf("Test 5\n\n");
    char ok;
    printf("Creating index...\n");
    if( (rc = m.CreateIndex(FILENAME, 1, INT, sizeof(int))) ) return rc;
    printf("Opening index...\n");
    if( (rc = m.OpenIndex(FILENAME, 1, ih)) ) return rc;
    int values [20] = {13,3,5,2,1,7,15,16,14,10,19,8,6,18,11,4,9,12,17,20};
    for(int i=0; i<20; i++){
        printf("Inserting an entry (n°%d, value=%d)\n", i, values[i]);
        int value = values[i];
        RID rid(value+100, value+200);
        if( (rc = ih.InsertEntry((void *) &value, rid)) ) return rc;
        printf("Checking the entry (n°%d)\n", i);
        if( (rc = is.OpenScan(ih, EQ_OP, (void *) &value)) ) return rc;
        if( (rc = is.GetNextEntry(rid)) ) return rc;
        if( (rc = is.CloseScan())) return rc;
    }
    std::cout << "-----> Print the tree?" << std::endl;
    cin >> ok;
    if(ok=='y'){
        printf("Printing Tree\n");
        if( (rc = ih.PrintTree())) return rc;
    }
    printf("Checking all the entries...\n");
    for(int i=0; i<20; i++){
        printf("    - Checking %d\n", i);
        RID rid;
        int value = values[i];
        if( (rc = is.OpenScan(ih, EQ_OP, (void *) &value)) ) return rc;
        if( (rc = is.GetNextEntry(rid)) ) return rc;
        if( (rc = is.CloseScan())) return rc;
    }
    printf("Closing index...\n");
    if( (rc = m.CloseIndex(ih)) ) return rc;
    printf("Passed test5!\n\n");
    return 0;
}


RC Test6(void) {
    RC rc;
    IX_IndexHandle ih;
    PF_Manager pfm;
    IX_Manager m(pfm);
    IX_IndexScan is;

    printf("Test 6\n");
    printf("In this test we will insert as many entries as we want to test the insertion process\n\n");
    char ok; int givenValue;
    printf("Creating index...\n");
    if( (rc = m.CreateIndex(FILENAME, 1, INT, sizeof(int))) ) return rc;
    printf("Opening index...\n");
    if( (rc = m.OpenIndex(FILENAME, 1, ih)) ) return rc;
    while(1>0){
        std::cout << "-> Value to insert? (0 for rand)";
        cin >> givenValue;
        int value = givenValue>0 ? givenValue : (rand() % 1000)+1; //A value between 1 and 1001
        RID rid(23,46);
        if( (ih.InsertEntry((void *) &value, rid)) ) return rc;
        std::cout << "-> Inserts value " << value << ". Print tree? (press y/n or q to quit)";
        cin >> ok;
        if(ok=='y'){
            if( (rc = ih.PrintTree())) return rc;
        }
        if(ok=='q'){
            break;
        }
        printf("Checking the entry\n");
        if( (rc = is.OpenScan(ih, EQ_OP, (void *) &value)) ) return rc;
        if( (rc = is.GetNextEntry(rid)) ){
            printf("Error happened!\n");
            if( (rc = ih.PrintTree())) return rc;
        }
        if( (rc = is.CloseScan())) return rc;
    }
    printf("Closing index...\n");
    if( (rc = m.CloseIndex(ih)) ) return rc;
    printf("Passed test6!\n\n");
    return 0;
}


RC Test7(void) {
    printf("Test 7\n");
    printf("In this test we will try to catch an error\n\n");
    for(int k=1; k<2; k++){
        printf("-----Begins iteration %d-----\n\n", k);
        RC rc;
        IX_IndexHandle ih;
        PF_Manager pfm;
        IX_Manager m(pfm);
        IX_IndexScan is;
        printf("Creating index...\n");
        if( (rc = m.CreateIndex(FILENAME, 1, INT, sizeof(int))) ) return rc;
        printf("Opening index...\n");
        if( (rc = m.OpenIndex(FILENAME, 1, ih)) ) return rc;

        int nbValues = 1000;
        int values [nbValues];
        for(int i=0; i<nbValues; i++){
            values[i] = (rand() % 1000)+1;
        }
        RID rid(23,46);
        for(int i=0; i<nbValues; i++){
            printf("Inserting %d\n", values[i]);
            if( (ih.InsertEntry((void *) &values[i], rid)) ) return rc;
            printf("Checking the entries\n");
            for(int j=0; j<=i; j++){
                if( (rc = is.OpenScan(ih, EQ_OP, (void *) &values[j])) ) return rc;
                if( (rc = is.GetNextEntry(rid)) ){
                    printf("Error happened when checking entry n°%d (value=%d)!\n", j, values[j]);
                    if( (rc = ih.PrintTree())) return rc;
                    return IX_EOF;
                }
                if( (rc = is.CloseScan())) return rc;
            }
        }
        printf("Closing index...\n");
        if( (rc = m.CloseIndex(ih)) ) return rc;
        printf("Destroying index...\n");
        if( (rc = m.DestroyIndex(FILENAME, 1)) ) return rc;
        printf("-----Ends iteration-----\n\n");
    }
    printf("Passed test7!\n\n");
    return 0;
}


RC Test8(void) {
    printf("Test 8\n");
    printf("Dans ce test on met a l'epreuve le bucket chaining\n\n");
    for(int k=1; k<2; k++){
        printf("-----Begins iteration %d-----\n\n", k);
        RC rc;
        IX_IndexHandle ih;
        PF_Manager pfm;
        IX_Manager m(pfm);
        IX_IndexScan is;
        printf("Creating index...\n");
        if( (rc = m.CreateIndex(FILENAME, 1, INT, sizeof(int))) ) return rc;
        printf("Opening index...\n");
        if( (rc = m.OpenIndex(FILENAME, 1, ih)) ) return rc;

        int nbValues = 1200;
        int values [nbValues];
        for(int i=0; i<nbValues; i++){
            values[i] = (rand() % 2)+1;
        }
        RID rid(23,46);
        for(int i=0; i<nbValues; i++){
            printf("Insertion %d/%d, value = %d\n", i, nbValues, values[i]);
            if( (rc = ih.InsertEntry((void *) &values[i], rid)) ) return rc;
            printf("Checking the entries\n");
            for(int j=0; j<=i; j++){
                if( (rc = is.OpenScan(ih, EQ_OP, (void *) &values[j])) ) return rc;
                if( (rc = is.GetNextEntry(rid)) ){
                    printf("Error happened when checking entry n°%d (value=%d)!\n", j, values[j]);
                    if( (rc = ih.PrintTree())) return rc;
                    return IX_EOF;
                }
                if( (rc = is.CloseScan())) return rc;
            }
        }
        printf("Closing index...\n");
        if( (rc = m.CloseIndex(ih)) ) return rc;
        printf("Destroying index...\n");
        if( (rc = m.DestroyIndex(FILENAME, 1)) ) return rc;
        printf("-----Ends iteration-----\n\n");
    }
    printf("Passed test8!\n\n");
    return 0;
}


RC Test9(void) {
    printf("Test 9\n");
    printf("Dans ce test on met vraiment a l'epreuve le bucket chaining\n\n");
    RC rc;
    IX_IndexHandle ih;
    PF_Manager pfm;
    IX_Manager m(pfm);
    IX_IndexScan is;
    printf("Creating index...\n");
    if( (rc = m.CreateIndex(FILENAME, 1, INT, sizeof(int))) ) return rc;
    printf("Opening index...\n");
    if( (rc = m.OpenIndex(FILENAME, 1, ih)) ) return rc;

    int nbValues = 20000;
    int values [nbValues];
    int rid1 [nbValues];
    int rid2 [nbValues];
    bool deleted [nbValues];
    //Random initialization
    for(int i=0; i<nbValues; i++){
        values[i] = (rand() % 30)+1;
        rid1[i] = (rand() % 1000)+1;
        rid1[i] = (rand() % 300)+1;
        deleted[i] = false;
    }
    //Insertion
    for(int i=0; i<nbValues; i++){
        printf("Insertion %d/%d, value = %d\n", i, nbValues, values[i]);
        RID rid(rid1[i], rid2[i]);
        if( (rc = ih.InsertEntry((void *) &values[i], rid)) ) return rc;
        printf("Checking the entries\n");
        //Entries checking
        for(int j=0; j<=i; j++){
            if( (rc = is.OpenScan(ih, EQ_OP, (void *) &values[j])) ) return rc;
            if( (rc = is.GetNextEntry(rid)) ){
                printf("Error happened when checking entry n°%d (value=%d)!\n", j, values[j]);
                if( (rc = ih.PrintTree())) return rc;
                return IX_EOF;
            }
            if( (rc = is.CloseScan())) return rc;
        }
    }

    //Let's have some fun deleting random entries
    int deletedCount = 0;
    while(deletedCount < nbValues*2/3){
        int i = (rand() % nbValues);
        if(deleted[i]) continue;
        printf("Deleting entry n°%d\n", i);
        RID rid(rid1[i], rid2[i]);
        if( (rc = ih.DeleteEntry((void *) &values[i], rid)) ) return rc;
        deleted[i] = true;
        deletedCount++;
    }
    printf("Closing index...\n");
    if( (rc = m.CloseIndex(ih)) ) return rc;
    printf("Destroying index...\n");
    if( (rc = m.DestroyIndex(FILENAME, 1)) ) return rc;

    printf("Passed test9!\n\n");
    return 0;
}


RC Test10(void) {
    printf("Test 10\n");
    printf("This is a tricky, hard one\n\n");
    RC rc;
    IX_IndexHandle ih;
    PF_Manager pfm;
    IX_Manager m(pfm);
    IX_IndexScan is;

    for(int k=0; k<100; k++){
        printf("----------------\nIteration n°%d\n----------------\n\n", k);
        printf("Creating index...\n");
        if( (rc = m.CreateIndex(FILENAME, 1, INT, sizeof(int))) ) return rc;
        printf("Opening index...\n");
        if( (rc = m.OpenIndex(FILENAME, 1, ih)) ) return rc;

        printf("Initializing values for insertion\n\n");

        int nbValues = 100;
        int values [nbValues];
        int rid1 [nbValues];

        int value = (rand()%100) + 1;
        int valueCount [5] = {0,0,0,0,0}; //For LT, LE, EQ, GE, GT

        //Random initialization
        for(int i=0; i<nbValues; i++){
            values[i] = (rand() % 100)+1;
            rid1[i] = (rand() % 1000)+1;
            //Counts for different compop
            if(values[i]<value) valueCount[0]++;
            if(values[i]<=value) valueCount[1]++;
            if(values[i]==value) valueCount[2]++;
            if(values[i]>=value) valueCount[3]++;
            if(values[i]>value) valueCount[4]++;
        }

        printf("Value chosen = %d\n", value);
        printf("Found %d LT, %d LE, %d EQ, %d GE, %d GT\n\n", valueCount[0], valueCount[1], valueCount[2], valueCount[3], valueCount[4]);

        //Insertion
        printf("Inserting %d values\n", nbValues);
        for(int i=0; i<nbValues; i++){
            RID rid(rid1[i], values[i]);
            if( (rc = ih.InsertEntry((void *) &values[i], rid)) ) return rc;
        }
        printf("Values inserted and checked after each insertion\n\n");


        //We check that the scan works
        printf("Checking scan features...\n");
        for(int i=0; i<nbValues; i++){
            if( (rc = is.OpenScan(ih, EQ_OP, (void * ) &values[i])) ) return rc;
            RID scanRid;
            if( (rc = is.GetNextEntry(scanRid)) ) return rc;
            PageNum slotNb;
            if( (rc = scanRid.GetSlotNum(slotNb)) ) return rc;
            if(slotNb!=values[i]){
                printf("Scan error: slotnb doesn't match! (slotNb=%d, value=%d) \N", slotNb, values[i]);
                return -1;
            }
            if( (rc = is.CloseScan()) ) return rc;
        }
        printf("Ends checking scan features...\n\n");

        //Now we take some values and delete while scaning
        CompOp operators [5] = {LT_OP, LE_OP, EQ_OP, GE_OP, GT_OP};
        int opInt = rand() % 5;
        CompOp opChosen = operators[opInt];
        switch(opChosen){
        case LT_OP:
            printf("Starting onscan deletion with LT_OP\n");
            break;
        case LE_OP:
            printf("Starting onscan deletion with LE_OP\n");
            break;
        case EQ_OP:
            printf("Starting onscan deletion with EQ_OP\n");
            break;
        case GE_OP:
            printf("Starting onscan deletion with GE_OP\n");
            break;
        case GT_OP:
            printf("Starting onscan deletion with GT_OP\n");
            break;
        }

        if( (rc = is.OpenScan(ih, opChosen, (void * ) &value)) ) return rc;
        RID rid;
        int deleteCount = 0;
        printf("Scan & delete : ");
        while( 1 > 0){
            printf("S");
            rc = is.GetNextEntry(rid);
            if(rc == IX_EOF) break;
            if(rc != 0) return rc;
            //Deletes the record
            int ridValue;
            if( (rc =rid.GetSlotNum(ridValue)) ) return rc;
            printf("D(%d) ", ridValue);
            if( (rc = ih.DeleteEntry((void*) &ridValue, rid)) ){
                ih.PrintTree();
                return rc;
            }
            deleteCount++;
        }
        printf("\n");

        if(deleteCount!=valueCount[opInt]){
            ih.PrintTree();
            printf("Error, not enough rid deleted (%d/%d)!\n", deleteCount, valueCount[opInt]);
            return -1;
        }else{
            printf("Deleted %d entries successfully!\n", deleteCount);
        }

        if( (rc = is.CloseScan()) ) return rc;
        printf("Closing index...\n");
        if( (rc = m.CloseIndex(ih)) ) return rc;
        printf("Destroying index...\n");
        if( (rc = m.DestroyIndex(FILENAME, 1)) ) return rc;

        printf("End of iteration n°%d\n\n", k);
    }
    printf("Passed test10!\n\n");
    return 0;
}
