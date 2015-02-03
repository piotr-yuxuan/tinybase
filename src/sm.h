//
// sm.h
//   Data Manager Component Interface
//

#ifndef SM_H
#define SM_H

// Please do not include any other files than the ones below in this file.

#include <stdlib.h>
#include <string.h>
#include "tinybase.h"  // Please don't change these lines
#include "parser.h"
#include "rm.h"
#include "ix.h"

struct DataRelInfo {

	// Default constructor
	DataRelInfo() {
		memset(relName, 0, MAXNAME + 1);
	}
	;

	// Copy constructor
	DataRelInfo(const DataRelInfo &d) {
		strcpy(relName, d.relName);
		tupleLength = d.tupleLength;
		attrCount = d.attrCount;
		indexCount = d.indexCount;
	}
	;

	char relName[MAXNAME + 1]; /* relation name */
	int tupleLength; /* tuple length in bytes */
	int attrCount; /* number of attributes */
	int indexCount; /* number of indexed attributes */
};

//
// SM_Manager: provides data management
//
class SM_Manager {
	friend class QL_Manager;
public:
	SM_Manager(IX_Manager &ixm, RM_Manager &rmm);
	~SM_Manager();                             // Destructor

	RC OpenDb(const char *dbName);           // Open the database
	RC CloseDb();                             // close the database

	RC CreateTable(const char *relName,           // create relation relName
			int attrCount,          //   number of attributes
			AttrInfo *attributes);       //   attribute data
	RC CreateIndex(const char *relName,           // create an index for
			const char *attrName);         //   relName.attrName
	RC DropTable(const char *relName);          // destroy a relation

	RC DropIndex(const char *relName,           // destroy index on
			const char *attrName);         //   relName.attrName
	RC Load(const char *relName,           // load relName from
			const char *fileName);         //   fileName
	RC Help();                             // Print relations in db
	RC Help(const char *relName);          // print schema of relName

	RC Print(const char *relName);          // print relName contents

	RC Set(const char *paramName,         // set parameter to
			const char *value);            //   value

private:
	RC FormatName(char * string);
    RC ForceToDisk();
	RM_Manager *rmm;
	IX_Manager *ixm;
	RM_FileHandle attrcat;
	RM_FileHandle relcat;
	/*
	 * Retrieve DataAttrInfo From Relation And Attribute
	 * OUT : dai
	 */
	RC AttrRecFromRA(char * relName, char * attrName, RM_Record * record);
	/*
	 * Retrieve DataRelInfo From Relation
	 */
	RC RelFromR(char * relName, RM_Record * record);
};

//
// Print-error function
//
void SM_PrintError(RC rc);

#define SM_INVALIDRELNAME      (START_SM_WARN + 0)  // invalid relation name
#define SM_INVALIDATTRNAME     (START_SM_WARN + 1) // invalid attribute name
#endif
