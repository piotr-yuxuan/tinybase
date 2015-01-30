//
// File:        SM component stubs
// Description: Print parameters of all SM_Manager methods
// Authors:     Dallan Quass (quass@cs.stanford.edu)
//

#include <cstdio>
#include <iostream>
#include "tinybase.h"
#include "sm.h"
#include "ix.h"
#include "rm.h"
#include <unistd.h>

using namespace std;

SM_Manager::SM_Manager(IX_Manager &ixm, RM_Manager &rmm) {
	this->rmm = &rmm;
	this->ixm = &ixm;
}

SM_Manager::~SM_Manager() {
}

/*
 * Format names of relations and attributes.
 * Relation and attribute names are case-insensitive, limited to MAXNAME = 24 characters each
 * and must begin with a letter.
 */
RC SM_Manager::FormatName(char *string) {
	int length = 0;
	if (tolower(*string) - 'a' < 0 && 'z' - tolower(*string) > 0) {
		return RC(-1);
	}
	while (*string) {
		*string = tolower(*string);
		string++;
		if (++length == MAXNAME) {
			return RC(-1);
		}
	}
	return RC(0);
}

/**
 * This method, along with method CloseDb, is called by the code implementing
 * the redbase command line utility. This method should change to the directory
 * for the database named *dbName (using system call chdir), then open the
 * files containing the system catalogs for the database.
 */
RC SM_Manager::OpenDb(const char *dbName) {
	RC rc = 0;

	// (try to) Move to the correct directory.
	// TODO Assumes we're in the parent directory.
	if (chdir(dbName) < 0) {
		cerr << " chdir error to " << *dbName << "\n";
		return RC(-1);
	}

	// Opening the files containing the system catalogs for the database.
	if ((rc = rmm->OpenFile("relcat", relcat))) {
		PrintError(rc);
		return rc;
	}
	if ((rc = rmm->OpenFile("attrcat", attrcat))) {
		PrintError(rc);
		return rc;
	}

	return rc;
}

/**
 *  This method should close all open files in the current database.
 *  Closing the files will automatically cause all relevant buffers to be
 *  flushed to disk.
 */
RC SM_Manager::CloseDb() {
	RC rc = 0;

	// Closing all open files in the current database.
	// TODO right now just two files have been opened (to be changed if need be).
	if ((rc = rmm->CloseFile(relcat))) {
		PrintError(rc);
		return rc;
	}
	if ((rc = rmm->CloseFile(attrcat))) {
		PrintError(rc);
		return rc;
	}

	return rc;
}

/**
 * Three parts:
 * #1: we test entries and format them
 * → relation and attribute names are limited to MAXNAME = 24 characters each,
 * and must begin with a letter.
 * → within a given database every relation name must be unique, and within
 * a given relation every attribute name must be unique.
 * #2: working part which is two-fold:
 * 	→ 2.1 We first update the system catalogs: a tuple for the new relation should
 *  be added to a catalog relation called relcat, and a tuple for each
 *  attribute should be added to a catalog relation called attrcat.
 *  → 2.2 After updating the catalogs, method RM_Manager::CreateFile is called
 * to create a file that will hold the tuples of the new relation.
 * #3: cleaning part, we free the memory from transient objects.
 *
 * TODO: error codes still to be implemented.
 */
RC SM_Manager::CreateTable(const char *relName, int attrCount,
		AttrInfo *attributes) {
	RC rc = 0;

	// Converts to lowercase. Methinks we have to do this trick because the argument is `const`.
	char *lowerRelName = (char*) malloc(MAXNAME + 1);
	strcpy(lowerRelName, relName);
	FormatName((char *) lowerRelName);

	RM_FileHandle *fh;
	if ((rc = rmm->OpenFile(lowerRelName, *fh))) {
		// File already exists then another database is already named the same.
		free(lowerRelName);
		free(fh);
		return RC(-1);
	}

	if (1 < attrCount || attrCount > MAXATTRS) {
		return RC(-1);
	}

	cout << "CreateTable\n" << "   relName     =" << lowerRelName << "\n"
			<< "   attrCount   =" << attrCount << "\n";

	if ((rc = rmm->OpenFile("attrcat", attrcat))) {
		PrintError(rc);
	}

	int offset = 0;
	AttributeTuple atuple = malloc(sizeof(AttributeTuple)); // atuple is overwritten each iteration.

	for (int i = 0; i < attrCount; i++) {
		AttrInfo a = attributes[i];
		if ((rc = FormatName(a.attrName))) {
			return rc;
		}

		switch (a.attrType) {
		case STRING:
			if (a.attrLength > 0 && a.attrLength < MAXSTRINGLEN) {
				return RC(-1);
			}
			break;
		case FLOAT:
		case INT:
			if (a.attrLength != 4) {
				return RC(-1);
			}
			break;
		default:
			return RC(-1);
		}

		atuple.attrLength = attributes[i].attrLength;
		strcpy(atuple.attrName, attributes[i].attrName); // +1 pour '\0'
		atuple.attrType = attributes[i].attrType;
		atuple.indexNo = -1;
		atuple.offset = offset;
		strcpy(atuple.relName, lowerRelName); // +1 pour '\0'

		// Add a tuple in attrcat for each attribute of the relation.
		RID *rid; // vanish
		if ((rc = attrcat.InsertRec((char *) atuple, *rid))) {
			return RC(-1);
		}

		offset += a.attrLength;

		cout << "   attributes[" << i << "].attrName=" << attributes[i].attrName
				<< "   attrType="
				<< (attributes[i].attrType == INT ? "INT" :
					attributes[i].attrType == FLOAT ? "FLOAT" : "STRING")
				<< "   attrLength=" << attributes[i].attrLength << "\n";
	}

	// We've got the total width of a record so it should't be called
	// offset anymore.
	const int totalSize = offset;

	// Add a tuple in relcat for the relation.
	if ((rc = rmm->OpenFile("relcat", relcat))) {
		PrintError(rc);
	}

	RelationTuple rtuple;
	rtuple.attrCount = attrCount;
	rtuple.indexCount = 0; // we choose later which attributes we index but
	// remember to keep that counter up to date.
	strcpy(rtuple.relName, lowerRelName);
	rtuple.tupleLength = totalSize;

	RID *rid; // vanish
	if ((rc = relcat.InsertRec((char *) rtuple, *rid))) {
		return RC(-1);
	}

	if ((rc = rmm->CreateFile(lowerRelName, totalSize))) {
		return RC(-1);
	}

	/*
	 * The catalogs are loaded when the database is opened and closed only
	 * when the database is closed. Then, updates to the catalogs are not
	 * reflected onto disk immediately and this can cause weird interface
	 * behaviour. One solution to this problem is to force pages each time
	 * a catalog is changed.
	 */
	if ((rc = attrcat.ForcePages()) || (rc = relcat.ForcePages())) {
		return RC(-1);
	}

	free(lowerRelName);
	atuple = NULL;
	rtuple = NULL;

	return rc;
}

RC SM_Manager::DropTable(const char *relName) {
	cout << "DropTable\n   relName=" << relName << "\n";
	return (0);
}

/*
 * Three parts:
 * #1 First we check arguments and preconditions. Only one index may be created
 * for each attribute of a relation.
 * #2 Then we update attrcat to reflect the new index, we create it and we
 * build it:
 * 	2.1 opening the index;
 * 	2.2 using RM component methods to scan through the records to be indexed;
 *  2.3 closing the index.
 * #3 Finally we clean all and reduce the memory print.
 */
RC SM_Manager::CreateIndex(const char *relName, const char *attrName) {
	RC rc = 0;

	// Format input
	char *lrelName = (char*) malloc(MAXNAME + 1);
	strcpy(lrelName, relName);
	char *lattrName = (char*) malloc(MAXNAME + 1);
	strcpy(lattrName, attrName);

	if ((rc = FormatName((char *) lrelName))
			|| (rc = FormatName((char *) lattrName))) {
		return RC(-1);
	}

	RM_FileScan fs;
	RM_Record rec; // This field is about to get overwritten a couple of time.
	RM_Record rrecord; // Record for rtuple.
	RelationTuple rtuple;
	// → Does such a relation exist?
	if ((rc = fs.OpenScan(relcat, // we look for the given relation
			STRING, // looking for its name
			MAXNAME + 1, // the former mayn't be wider than this
			0, // null offset because we search on the column relname
			EQ_OP, // we look for *this* relation precisely
			lrelName // name of the relation
			)) || (rc = fs.GetNextRec(rrecord)) // Should be exactly one.
			) {
		return RC(-1);
	}
	if ((rc = rec.GetData((char *&) rtuple))) {
		return RC(-1);
	}

	// → Has this attribute been indexed already? It has to be none (-1) or we
	// issue an non-zero return code.
	if ((rc = fs.OpenScan(attrcat, // we look for the given relation
			STRING, // looking for its name
			MAXNAME + 1, // the former mayn't be wider than this
			0, // null offset because we search on the column relname
			EQ_OP, // we look for *this* relation precisely
			lrelName // name of the relation
			))) {
		return RC(-1);
	}

	// Does it have an index already?
	RM_Record arecord; // Record for atuple.
	AttributeTuple atuple; // Its value is found then written in.
	while ((rc = fs.GetNextRec(arecord))) {
		if ((rc = arecord.GetData((char *&) atuple))) {
			return RC(-1);
		}
		if (strcmp(atuple.relName, lrelName)) {
			// We've found the entry for given relation and attribute.
			// Does it have an index already?
			if (atuple.indexNo != -1) {
				return RC(-1); // Has an index already.
			} else {
				break; // Exit the loop
			}
		}
	}

	// Index creation
	if ((rc = ixm->CreateIndex(atuple.relName, rtuple.indexCount,
			atuple.attrType, atuple.attrLength))) {
		return RC(-1);
	}

	// Update index data.
	atuple.indexNo = rtuple.indexCount++;
	if ((rc = relcat.UpdateRec(rrecord)) || (rc = attrcat.UpdateRec(arecord))
			|| (rc = relcat.ForcePages() || (rc = attrcat.ForcePages()))) {
		return RC(-1);
	}

	/* Opens the index so we'll close it later.*/
	IX_IndexHandle ih;
	/* Opens the relation file which is about to get indexed */
	RM_FileHandle fh;
	/* Instanciate a scanner to yield tuples */
	RM_FileScan fs;
	if ((rc = ixm->OpenIndex(lrelName, atuple.indexNo, ih))
			|| (rc = rmm->OpenFile(lrelName, fh))
			|| (rc = fs.OpenScan(fh, atuple.attrType, atuple.attrLength,
					atuple.offset, NO_OP, NULL))) {
		return RC(-1);
	}

	// Walk throughout the records and insert each of them into the index.
	while ((rc = fs.GetNextRec(rec))) {
		char *pData;
		RID rid;
		// Retrieve the record.
		// Insert it into the index.
		if ((rc = rec.GetData(pData) || (rc = rec.GetRid(rid)))
				|| (rc = ih.InsertEntry(pData + atuple.offset, rid))) {
			return RC(-1);
		}
	}

	cout << "CreateIndex\n" << "   relName =" << relName << "\n"
			<< "   attrName=" << attrName << "\n";

	if ((rc = fs.CloseScan()) || (rc = rmm->CloseFile(fh))
			|| (rc = ixm->CloseIndex(ih))) {
		return RC(-1);
	}

	// TODO Any free() to do here?

	return rc;
}

RC SM_Manager::DropIndex(const char *relName, const char *attrName) {
	cout << "DropIndex\n" << "   relName =" << relName << "\n" << "   attrName="
			<< attrName << "\n";
	return (0);
}

RC SM_Manager::Load(const char *relName, const char *fileName) {
	cout << "Load\n" << "   relName =" << relName << "\n" << "   fileName="
			<< fileName << "\n";
	return (0);
}

RC SM_Manager::Print(const char *relName) {
	cout << "Print\n" << "   relName=" << relName << "\n";
	return (0);
}

RC SM_Manager::Set(const char *paramName, const char *value) {
	cout << "Set\n" << "   paramName=" << paramName << "\n" << "   value    ="
			<< value << "\n";
	return (0);
}

RC SM_Manager::Help() {
	cout << "Help\n";
	return (0);
}

RC SM_Manager::Help(const char *relName) {
	cout << "Help\n" << "   relName=" << relName << "\n";
	return (0);
}

void SM_PrintError(RC rc) {
	cout << "SM_PrintError\n   rc=" << rc << "\n";
}

