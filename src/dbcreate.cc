//
// dbcreate.cc
//
// Author: Jason McHugh (mchughj@cs.stanford.edu)
//
// This shell is provided for the student.

#include <iostream>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include "rm.h"
#include "sm.h"
#include "tinybase.h"

#include "printer.h" //This includes allows us to use the DataAttrInfo struct

using namespace std;

//
// main
//
main(int argc, char *argv[])
{
    char *dbname;
    char command[255] = "mkdir ";

    // Look for 2 arguments. The first is always the name of the program
    // that was executed, and the second should be the name of the
    // database.
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " dbname \n";
        exit(1);
    }

    // The database name is the second argument
    dbname = argv[1];

    //First we check the database name is not already taken
    if (!(chdir(dbname) < 0)) {
        cerr << argv[0] << " Database " << dbname << " already exists!\n";
        exit(1);
    }

    // Create a subdirectory for the database
    system (strcat(command,dbname));

    if (chdir(dbname) < 0) {
        cerr << argv[0] << " chdir error to " << dbname << "\n";
        exit(1);
    }

    /*
     *Create the system catalogs...
     */

    // Initialize components
    PF_Manager pfm;
    RM_Manager rmm(pfm);
    RC rc = 0;

    //Creates the rel catalog
    //Space for relName, tupleLength, attrCount, indexCount
    int relRecordSize = sizeof(char[MAXNAME+1]) + sizeof(int)*3;
    rc = rmm.CreateFile("relcat", relRecordSize);
    if(rc) PrintError(rc);

    //Creates the attr catalog
    //Space for relName, attrName, offset, attrType, attrLength & indexNo
    int attrRecordSize = sizeof(char[MAXNAME+1])*2 + sizeof(int) + sizeof(AttrType) + sizeof(int)*2;
    rc = rmm.CreateFile("attrCat", attrRecordSize);
    if(rc) PrintError(rc);

    /*
     *Now that the two catalogs are created we need to add them to relcat
     */

    //Opens the relcat file
    RM_FileHandle rmfh;
    rc = rmm.OpenFile("relcat", rmfh);
    if(rc) PrintError(rc);
    RID rid;

    //We create a record for rel table entry
    char * pDataRel = malloc(relRecordSize);
    //Values to insert
    char relRelName[MAXNAME+1] = "relcat";
    int relTupleLength = relRecordSize;
    int relAttrCount = 4;
    int relIndexCount = 0;
    //Copies them to our data pointer
    memcpy(pDataRel, &relRelName, sizeof(char[MAXNAME+1]));
    memcpy(pDataRel+sizeof(char[MAXNAME]), &relTupleLength, sizeof(int));
    memcpy(pDataRel+sizeof(char[MAXNAME+1])+sizeof(int), &relAttrCount, sizeof(int));
    memcpy(pDataRel+sizeof(char[MAXNAME+1])+sizeof(int)*2, &relIndexCount, sizeof(int));
    //Inserts the record
    rc = rmfh.InsertRec(pDataRel, rid);
    if(rc) PrintError(rc);
    free(pDataRel);

    //We create a record for attr table entry
    char * pDataAttr = malloc(attrRecordSize);
    //Values to insert
    char attrRelName[MAXNAME+1] = "attrcat";
    int attrTupleLength = attrRecordSize;
    int attrAttrCount = 6;
    int attrIndexCount = 0;
    //Copies them to our data pointer
    memcpy(pDataAttr, &attrRelName, sizeof(char[MAXNAME+1]));
    memcpy(pDataAttr+sizeof(char[MAXNAME+1]), &attrTupleLength, sizeof(int));
    memcpy(pDataAttr+sizeof(char[MAXNAME+1])+sizeof(int), &attrAttrCount, sizeof(int));
    memcpy(pDataAttr+sizeof(char[MAXNAME+1])+sizeof(int)*2, &attrIndexCount, sizeof(int));
    //Inserts the record
    rc = rmfh.InsertRec(pDataAttr, rid);
    if(rc) PrintError(rc);
    free(pDataAttr);

    //Closes the relcat file
    rc = rmm.CloseFile(rmfh);
    if(rc) PrintError(rc);


    /*
     *At this point the tables for our catalogs are created and we inserted two entries
     *in the relcat table for our catalogs.
     *But we also need to insert several entries in attrcat for the attributes of our catalogs
     */

    //Opens the attrcat file
    rc = rmm.OpenFile("attrcat", rmfh);
    if(rc) PrintError(rc);

    //Inserts entries for the 4 attributes of relcat

    //First attribute : relName
    DataAttrInfo relNameAttr;
    relNameAttr.relName = "relcat";
    relNameAttr.attrName = "relname";
    relNameAttr.offset = 0;
    relNameAttr.attrType = STRING;
    relNameAttr.attrLength = MAXNAME+1;
    relNameAttr.indexNo = -1;
    rc = rmfh.InsertRec((char*) &relNameAttr, rid);
    if(rc) PrintError(rc);

    //Second attribute : tupleLength
    DataAttrInfo tupleLengthAttr;
    tupleLengthAttr.relName = "relcat";
    tupleLengthAttr.attrName = "tuplelength";
    tupleLengthAttr.offset = sizeof(char[MAXNAME+1]);
    tupleLengthAttr.attrType = INT;
    tupleLengthAttr.attrLength = sizeof(int);
    tupleLengthAttr.indexNo = -1;
    rc = rmfh.InsertRec((char*) &tupleLengthAttr, rid);
    if(rc) PrintError(rc);

    //Third attribute : attrcount
    DataAttrInfo attrCountAttr;
    attrCountAttr.relName = "relcat";
    attrCountAttr.attrName = "attrcount";
    attrCountAttr.offset = sizeof(char[MAXNAME+1])+sizeof(int);
    attrCountAttr.attrType = INT;
    attrCountAttr.attrLength = sizeof(int);
    attrCountAttr.indexNo = -1;
    rc = rmfh.InsertRec((char*) &attrCountAttr, rid);
    if(rc) PrintError(rc);

    //Fourth attribute : indexcount
    DataAttrInfo indexCountAttr;
    indexCountAttr.relName = "relcat";
    indexCountAttr.attrName = "indexcount";
    indexCountAttr.offset = sizeof(char[MAXNAME+1])+sizeof(int)*2;
    indexCountAttr.attrType = INT;
    indexCountAttr.attrLength = sizeof(int);
    indexCountAttr.indexNo = -1;
    rc = rmfh.InsertRec((char*) &indexCountAttr, rid);
    if(rc) PrintError(rc);

    //Inserts entries for the 6 attributes of attrcat

    //First attribute : relName
    relNameAttr.relName = "attrcat";
    relNameAttr.attrName = "relname";
    relNameAttr.offset = 0;
    relNameAttr.attrType = STRING;
    relNameAttr.attrLength = MAXNAME+1;
    relNameAttr.indexNo = -1;
    rc = rmfh.InsertRec((char*) &relNameAttr, rid);
    if(rc) PrintError(rc);

    //Second attribute : attrname
    DataAttrInfo attrNameAttr;
    attrNameAttr.relName = "attrcat";
    attrNameAttr.attrName = "attrname";
    attrNameAttr.offset = sizeof(char[MAXNAME+1]);
    attrNameAttr.attrType = STRING;
    attrNameAttr.attrLength = MAXNAME+1;
    attrNameAttr.indexNo = -1;
    rc = rmfh.InsertRec((char*) &attrNameAttr, rid);
    if(rc) PrintError(rc);

    //Third attribute : offset
    DataAttrInfo offsetAttr;
    offsetAttr.relName = "attrcat";
    offsetAttr.attrName = "offset";
    offsetAttr.offset = sizeof(char[MAXNAME+1])*2;
    offsetAttr.attrType = INT;
    offsetAttr.attrLength = sizeof(INT);
    offsetAttr.indexNo = -1;
    rc = rmfh.InsertRec((char*) &offsetAttr, rid);
    if(rc) PrintError(rc);

    //Fourth attribute : attrType
    DataAttrInfo attrTypeAttr;
    attrTypeAttr.relName = "attrcat";
    attrTypeAttr.attrName = "attrtype";
    attrTypeAttr.offset = sizeof(char[MAXNAME+1])*2 + sizeof(int);
    attrTypeAttr.attrType = INT; //TODO not sure AttrType's attrtype is int
    attrTypeAttr.attrLength = sizeof(AttrType);
    attrTypeAttr.indexNo = -1;
    rc = rmfh.InsertRec((char*) &attrTypeAttr, rid);
    if(rc) PrintError(rc);

    //Fifth attribute : attrLength
    DataAttrInfo attrLengthAttr;
    attrLengthAttr.relName = "attrcat";
    attrLengthAttr.attrName = "attrlength";
    attrLengthAttr.offset = sizeof(char[MAXNAME+1])*2 + sizeof(int) + sizeof(AttrType);
    attrLengthAttr.attrType = INT;
    attrLengthAttr.attrLength = sizeof(int);
    attrLengthAttr.indexNo = -1;
    rc = rmfh.InsertRec((char*) &attrLengthAttr, rid);
    if(rc) PrintError(rc);

    //Sixth attribute : indexNo
    DataAttrInfo indexNoAttr;
    indexNoAttr.relName = "attrcat";
    indexNoAttr.attrName = "indexno";
    indexNoAttr.offset = sizeof(char[MAXNAME+1])*2 + sizeof(int) + sizeof(AttrType) + sizeof(int);
    indexNoAttr.attrType = INT;
    indexNoAttr.attrLength = sizeof(int);
    indexNoAttr.indexNo = -1;
    rc = rmfh.InsertRec((char*) &indexNoAttr, rid);
    if(rc) PrintError(rc);

    /*
     *We inserted all the entries for the attributes of our cat tables in attrcat so
     *we just have to close the file
     */

    rc = rmm.CloseFile(rmfh);
    if(rc) PrintError(rc);

    return(0);
}
