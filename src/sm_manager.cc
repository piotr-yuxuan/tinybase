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
		rc = -1; // TODO to be changed
		return rc;
	}

	// Opening the files containing the system catalogs for the database.
	if ((rc = rmm->OpenFile("relcat", relcatfh))) {
		PrintError(rc);
		return rc;
	}
	if ((rc = rmm->OpenFile("attrcat", attrcatfh))) {
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
	if ((rc = rmm->CloseFile(relcatfh))) {
		PrintError(rc);
		return rc;
	}
	if ((rc = rmm->CloseFile(attrcatfh))) {
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
		rc = -1; // TODO to be changed
		return rc;
	}

	cout << "CreateTable\n" << "   relName     =" << lowerRelName << "\n"
			<< "   attrCount   =" << attrCount << "\n";

	RM_FileHandle acatalog;
	if ((rc = rmm->OpenFile("attrcat", acatalog))) {
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
		atuple.indexNo; // Euh, que mettre ?
		atuple.offset = offset;
		strcpy(atuple.relName, lowerRelName); // +1 pour '\0'

		// Add a tuple in attrcat for each attribute of the relation.
		RID *rid; // vanish
		if ((rc = acatalog.InsertRec((char *) atuple, *rid))) {
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
	RM_FileHandle rcatalog;
	if ((rc = rmm->OpenFile("relcat", rcatalog))) {
		PrintError(rc);
	}

	RelationTuple rtuple;
	rtuple.attrCount = attrCount;
	rtuple.indexCount = 0; // Let's consider we choose later which attributes
	// we index but don't remember to keep that counter up to date.
	strcpy(rtuple.relName, lowerRelName);
	rtuple.tupleLength = totalSize;

	RID *rid; // vanish
	if ((rc = rcatalog.InsertRec((char *) rtuple, *rid))) {
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
	if ((rc = acatalog.ForcePages()) || (rc = rcatalog.ForcePages())) {
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

RC SM_Manager::CreateIndex(const char *relName, const char *attrName) {
	cout << "CreateIndex\n" << "   relName =" << relName << "\n"
			<< "   attrName=" << attrName << "\n";
	return (0);
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

