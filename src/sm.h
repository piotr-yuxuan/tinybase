//
// sm.h
//   Data Manager Component Interface
//

#ifndef SM_H
#define SM_H

// Please do not include any other files than the ones below in this file.

#include <stdlib.h>
#include <string.h>
#include "redbase.h"  // Please don't change these lines
#include "parser.h"
#include "rm.h"
#include "ix.h"

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
    // Copy constructor
    SM_Manager(const SM_Manager &manager);
    // Overloaded =
    SM_Manager& operator=(const SM_Manager &manager);

    RC GetRelationInfo(const char *relName, RM_Record &rec, char *&data);
    RC SetRelationIndexCount(const char *relName, int value);
    RC GetAttributeInfo(const char *relName, const char *attrName,
                        RM_Record &rec, char *&data);

    IX_Manager *pIxm;
    RM_Manager *pRmm;
    RM_FileHandle fhRelcat;
    RM_FileHandle fhAttrcat;

    int useIndexNo;
};

//
// Print-error function
//
void SM_PrintError(RC rc);

#define SM_INVALIDDBNAME   (START_SM_WARN + 0) // invalid DB name
#define SM_CHDIRFAILED     (START_SM_WARN + 1) // cannot change directory
#define SM_INVALIDRELNAME  (START_SM_WARN + 2) // invalid relation name
#define SM_DUPLICATEDATTR  (START_SM_WARN + 3) // duplicated attribute names
#define SM_RELEXISTS       (START_SM_WARN + 4) // relation already exists
#define SM_RELNOTFOUND     (START_SM_WARN + 5) // relation not found
#define SM_ATTRNOTFOUND    (START_SM_WARN + 6) // relation/attribute not found
#define SM_INDEXEXISTS     (START_SM_WARN + 7) // index already exists
#define SM_INDEXNOTFOUND   (START_SM_WARN + 8) // index not found
#define SM_FILEIOFAILED    (START_SM_WARN + 9) // data file I/O failed
#define SM_INVALIDFORMAT   (START_SM_WARN + 10) // invalid data file format
#define SM_PARAMUNDEFINED  (START_SM_WARN + 11) // parameter undefined
#define SM_LASTWARN        SM_PARAMUNDEFINED

#define SM_NOMEM           (START_SM_ERR - 0)  // no memory
#define SM_LASTERROR       SM_NOMEM

#endif
