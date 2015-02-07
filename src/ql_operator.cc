//
// ql_operator.cc
//

// This file contains implementations for the operators class

#include <cstdio>
#include <iostream>
#include <sys/times.h>
#include <sys/types.h>
#include <cassert>
#include <unistd.h>
#include "tinybase.h"
#include "ql_internal.h"
#include "sm.h"
#include "ix.h"
#include "rm.h"

using namespace std;


/*
 *Following method are method for the QL_TblScanOp
 */

//Constructor
QL_TblScanOp::QL_TblScanOp(const char *tableName, RM_FileHandle &rmfh,
                           const Condition &condition, SM_Manager *smm){
    //Copies the relation name
    this->tableName = (char*) malloc(MAXNAME+10);
    memset(this->tableName, 0, MAXNAME+10);
    strcpy(this->tableName, tableName);
    //Sets pointer for RM_FileHandle
    this->pRmfh = &rmfh; //(& to get the adress)
    //Sets pointer for the condition
    this->pCondition = &condition;
    //Sets pointer for SM_Manager
    this->pSmm = smm;
}

//Initialize opens the fileScan and goes to the first rid to return
RC QL_TblScanOp::Initialize(AttrType attrType, int attrLength, char *value){
    RC rc = 0;
    rmfs = RM_FileScan();
    //Find the attrOffset value
    int attrOffset;
    //TODO
    //If the right side is a value, we just scan with it
    if(pCondition->bRhsIsAttr==false){
        if((rc = rmfs.OpenScan(pRmfh, attrType, attrLength, attrOffset, pCondition->op, pCondition->rhsValue))){
            return rc;
        }
    //Else we open the scan with no value
    }else{
        if((rc = rmfs.OpenScan(pRmfh, attrType, attrLength, attrOffset, NO_OP, NULL))){
            return rc;
        }
    }
}

//Gets the next matching record
RC QL_TblScanOp::GetNext(RM_Record &record){
    RC rc = 0;
    //If the right is a value we just have to find the next one in the rm
    if(this->pCondition->bRhsIsAttr==false){
        return this->rmfs.GetNextRec(record);
    }
    //Else we call next until we find a matching record
    while(rmfs.GetNextRec(record)==0){
        //TODO Values comparaison
    }
}

RC QL_TblScanOp::Finalize(){
    RC rc = 0;
    //We close the scan
    if((rc = rmfs.CloseScan())){
        return rc;
    }
}

//Destrcutor
QL_TblScanOp::~QL_TblScanOp(){
    free(this->tableName);
}
