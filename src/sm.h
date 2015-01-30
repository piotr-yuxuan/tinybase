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

struct RelationTuple {
	char * relName; 	/* relation name */
	int tupleLength;	/* tuple length in bytes */
	int attrCount;		/* number of attributes */
	int indexCount; 	/* number of indexed attributes */
};

struct AttributeTuple {
	char * relName; 	/* this attribute's relation */
	char * attrName; 	/* attribute name */
	int offset;			/* offset in bytes from beginning of tuple */
	AttrType attrType;	/* attribute type */
	int attrLength; 	/* attribute length */
	int indexNo; 		/* index number, or -1 if not indexed */
};

//
// SM_Manager: provides data management
//
class SM_Manager {
    friend class QL_Manager;
public:
    SM_Manager    (IX_Manager &ixm, RM_Manager &rmm);
    ~SM_Manager   ();                             // Destructor

    RC OpenDb     (const char *dbName);           // Open the database
    RC CloseDb    ();                             // close the database

    RC CreateTable(const char *relName,           // create relation relName
                   int        attrCount,          //   number of attributes
                   AttrInfo   *attributes);       //   attribute data
    RC CreateIndex(const char *relName,           // create an index for
                   const char *attrName);         //   relName.attrName
    RC DropTable  (const char *relName);          // destroy a relation

    RC DropIndex  (const char *relName,           // destroy index on
                   const char *attrName);         //   relName.attrName
    RC Load       (const char *relName,           // load relName from
                   const char *fileName);         //   fileName
    RC Help       ();                             // Print relations in db
    RC Help       (const char *relName);          // print schema of relName

    RC Print      (const char *relName);          // print relName contents

    RC Set        (const char *paramName,         // set parameter to
                   const char *value);            //   value

private:
    RC FormatName(char * string);
    RM_Manager *rmm;
    IX_Manager *ixm;
    RM_FileHandle attrcat;
    RM_FileHandle relcat;
};

//
// Print-error function
//
void SM_PrintError(RC rc);

#endif
