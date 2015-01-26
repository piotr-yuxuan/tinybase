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

using namespace std;

SM_Manager::SM_Manager(IX_Manager &ixm, RM_Manager &rmm) {
	this->rmm = rmm;
	this->ixm = ixm;
}

SM_Manager::~SM_Manager() {
}

void SM_Manager::ToLowerCase(char *string) {
	while (*string) {
		*string = tolower(*string);
		string++;
	}
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
	if ((rc = rmm.OpenFile("relcat", relcatfh))) {
		PrintError(rc);
		return rc;
	}
	if ((rc = rmm.OpenFile("attrcat", attrcatfh))) {
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
	if ((rc = rmm.CloseFile(relcatfh))) {
		PrintError(rc);
		return rc;
	}
	if ((rc = rmm.CloseFile(attrcatfh))) {
		PrintError(rc);
		return rc;
	}

	return rc;
}

/**
 * TODO Warnning: not finished yet (je la garde).
 */
RC SM_Manager::CreateTable(const char *relName, int attrCount,
		AttrInfo *attributes) {
	RC rc = 0;

	char * lowerRelName = strcpy(lowerRelName, relName);
	ToLowerCase((char *) lowerRelName);
	if (1 > attrCount || attrCount > MAXATTRS) {
		rc = -1; // TODO to be changed
		return rc;
	}

	cout << "CreateTable\n" << "   relName     =" << lowerRelName << "\n"
			<< "   attrCount   =" << attrCount << "\n";

	for (int i = 0; i < attrCount; i++) {
		ToLowerCase(attributes[i].attrName);
		cout << "   attributes[" << i << "].attrName=" << attributes[i].attrName
				<< "   attrType="
				<< (attributes[i].attrType == INT ? "INT" :
					attributes[i].attrType == FLOAT ? "FLOAT" : "STRING")
				<< "   attrLength=" << attributes[i].attrLength << "\n";
	}

	// Add a tuple in relcat for the relation.
	RM_FileHandle rmfh;
	if ((rc = rmm.OpenFile("relcat", rmfh))) {
		PrintError(rc);
	}
	RID rid;

	// Add a tuple in attrcat for each attribute of the relation.

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

