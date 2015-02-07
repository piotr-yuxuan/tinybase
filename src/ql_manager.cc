//
// ql_manager.cc
//

// Note that for the SM component (HW3) the QL is actually a
// simple stub that will allow everything to compile.  Without
// a QL stub, we would need two parsers.

#include <cstdio>
#include <iostream>
#include <sys/times.h>
#include <sys/types.h>
#include <cassert>
#include <unistd.h>
#include "tinybase.h"
#include "printer.h"
#include "ql.h"
#include "sm.h"
#include "ix.h"
#include "rm.h"

using namespace std;

//
// QL_Manager::QL_Manager(SM_Manager &smm, IX_Manager &ixm, RM_Manager &rmm)
//
// Constructor for the QL Manager
//
QL_Manager::QL_Manager(SM_Manager &smm, IX_Manager &ixm, RM_Manager &rmm)
{
    // Can't stand unused variable warnings!
    assert (&smm && &ixm && &rmm);
    //Sets pointers
    this->pSmm = &smm;
    this->pIxm = &ixm;
    this->pRmm = &rmm;
}

//
// QL_Manager::~QL_Manager()
//
// Destructor for the QL Manager
//
QL_Manager::~QL_Manager()
{
}

//
// Handle the select clause
//
RC QL_Manager::Select(int nSelAttrs, const RelAttr selAttrs[],
                      int nRelations, const char * const relations[],
                      int nConditions, const Condition conditions[])
{
    int i;

    cout << "Select\n";

    cout << "   nSelAttrs = " << nSelAttrs << "\n";
    for (i = 0; i < nSelAttrs; i++)
        cout << "   selAttrs[" << i << "]:" << selAttrs[i] << "\n";

    cout << "   nRelations = " << nRelations << "\n";
    for (i = 0; i < nRelations; i++)
        cout << "   relations[" << i << "] " << relations[i] << "\n";

    cout << "   nCondtions = " << nConditions << "\n";
    for (i = 0; i < nConditions; i++)
        cout << "   conditions[" << i << "]:" << conditions[i] << "\n";

    return 0;
}

//
// Insert the values into relName
//
RC QL_Manager::Insert(const char *relName,
                      int nValues, const Value values[])
{
    int i;

    cout << "Insert\n";

    cout << "   relName = " << relName << "\n";
    cout << "   nValues = " << nValues << "\n";
    for (i = 0; i < nValues; i++)
        cout << "   values[" << i << "]:" << values[i] << "\n";

    RC rc = 0;

    //Opens the relation in fileHandle
    RM_FileHandle rmfh;
    if((rc = this->pRmm->OpenFile(relName, rmfh))){
        return rc;
    }

    //Gets the tuple length for the relation
    char * pData;
    RM_Record record;
    if((rc = this->pSmm->GetRelationInfo(relName, record, pData))){
        return rc;
    }
    int tupleLength;
    memcpy(&tupleLength, pData+MAXNAME+1, sizeof(int));

    //Allocates the data for the new record
    char * pDataRecord = (char *) malloc(tupleLength);

    //Creates list of attributes
    DataAttrInfo attributes[nValues];
    RM_FileScan attrCatFh;
    if((rc = attrCatFh.OpenScan(pSmm->fhAttrcat, STRING, MAXNAME+1, 0, EQ_OP, relName))){
        return rc;
    }
    int i=0;
    RM_Record attrRecord;
    while(attrCatFh.GetNextRec(attrRecord)!=RM_EOF){
        char * pAttrData;
        if((rc = attrRecord.GetData(pAttrData))){
            return rc;
        }
        memcpy(&attributes[i], pAttrData, sizeof(DataAttrInfo));
        i++;
    }

    int pdrOffset = 0;
    for (i = 0; i < nValues; i++){
        memcpy(pDataRecord+attributes[i].offset, (char * ) values[i].data, attributes[i].attrLength);
    }

    RID rid;
    //Inserts the record
    if((rc = rmfh.InsertRec(pDataRecord, rid))){
        return rc;
    }

    free(pDataRecord);

    return 0;
}

//
// Delete from the relName all tuples that satisfy conditions
//
RC QL_Manager::Delete(const char *relName,
                      int nConditions, const Condition conditions[])
{
    int i;

    cout << "Delete\n";

    cout << "   relName = " << relName << "\n";
    cout << "   nCondtions = " << nConditions << "\n";
    for (i = 0; i < nConditions; i++)
        cout << "   conditions[" << i << "]:" << conditions[i] << "\n";

    return 0;
}


//
// Update from the relName all tuples that satisfy conditions
//
RC QL_Manager::Update(const char *relName,
                      const RelAttr &updAttr,
                      const int bIsValue,
                      const RelAttr &rhsRelAttr,
                      const Value &rhsValue,
                      int nConditions, const Condition conditions[])
{
    int i;

    cout << "Update\n";

    cout << "   relName = " << relName << "\n";
    cout << "   updAttr:" << updAttr << "\n";
    if (bIsValue)
        cout << "   rhs is value: " << rhsValue << "\n";
    else
        cout << "   rhs is attribute: " << rhsRelAttr << "\n";

    cout << "   nCondtions = " << nConditions << "\n";
    for (i = 0; i < nConditions; i++)
        cout << "   conditions[" << i << "]:" << conditions[i] << "\n";

    return 0;
}

//
// void QL_PrintError(RC rc)
//
// This function will accept an Error code and output the appropriate
// error.
//
void QL_PrintError(RC rc)
{
    cout << "QL_PrintError\n   rc=" << rc << "\n";
}
