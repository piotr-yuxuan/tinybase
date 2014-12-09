//
// File:        rm_filescan.cc
// Description: RM_FileScan class implementation
// Authors:     Camille TIENNOT (camille.tiennot@telecom-paristech.fr)
//

#include <unistd.h>
#include <sys/types.h>
#include "rm_internal.h"

//
// Class RM_FileScan declaration
//

//Constructor
RM_FileScan::RM_FileScan(){

}

//Destructor
RM_FileScan::~RM_FileScan(){
    //Don't need to do anything
}

//Initializes a scan
RC RM_FileScan::OpenScan(const RM_FileHandle &fileHandle,
                         AttrType attrType,
                         int attrLength,
                         int attrOffset,
                         CompOp compOp,
                         void *value,
                         ClientHint pinHint)
{
    //If already open we return a warning
    if(this->scaning){
       return RM_ALREADYOPEN;
    }

    //Copies all the paramters into attributes
    this->fileHandle = &fileHandle;
    this->attrType = attrType;
    this->attrOffset = attrOffset;
    this->compOp = compOp;

    //Copies the value to our value attribute
    this->value = *value;

    //If the value is not NULL we check all the parameters
    if(value!=NULL){
        //Check the type of attribute
        if(attrType!=INT && attrType!=FLOAT && attrType!=STRING){
            return RM_FSCREATEFAIL;
        }
        //Checks the comparison operator
        if(compOp<EQ_OP || compOp>GE_OP){
            return RM_FSCREATEFAIL;
        }
        //Checks the length
        if(attrType==INT && attrLength!=4){
            return RM_FSCREATEFAIL;
        }else if(attrType==FLOAT && attrLength!=4){
            return RM_FSCREATEFAIL;
        }else if(attrType==STRING && (attrLength<1 || attrLength>MAXSTRINGLEN) ){
            return RM_FSCREATEFAIL;
        }
        //Checks the offset is not larger than record size
        //-> To do
    }

    return 0;
}

//Gets next matching record
RC RM_FileScan::GetNextRec(RM_Record &rec){
    while()
    //If there is no more matching record we return RM_EOF
    return RM_EOF;
}

//Terminates a file scan
RC RM_FileScan::CloseScan(){
    // Don't need to do anything
}
