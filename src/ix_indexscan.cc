#include <cstdio>
#include <unistd.h>
#include <iostream>
#include "ix.h"
#include "ix_internal.h"

using namespace std;

IX_IndexScan::IX_IndexScan() {
    //Don't need to do anything
}

IX_IndexScan::~IX_IndexScan() {
    //Don't need to do anything for now
}

RC IX_IndexScan::OpenScan(const IX_IndexHandle &indexHandle, CompOp compOp,
		void *value, ClientHint pinHint = NO_HINT) {

    // Sanity Check: 'this' should not be open yet
    if (bScanOpen)
       // Test: opened IX_IndexScan
       return (IX_SCANOPEN);

    // TODO
    //Check IndexHandle is open

    // Sanity Check: compOp
    switch (_compOp) {
    case EQ_OP:
    case LT_OP:
    case GT_OP:
    case LE_OP:
    case GE_OP:
    //case NE_OP: Disabled
    case NO_OP:
       break;
    default:
       return (IX_INVALIDCOMPOP);
    }

    if (compOp != NO_OP) {
       // Sanity Check: value must not be NULL
       if (value == NULL)
          // Test: null _value
          return (IX_NULLPOINTER);
    }

    // Copy parameters to local variable
    this->pIndexHandle = (IX_IndexHandle *)&indexHandle;
    this->compOp = compOp;
    this->value = value;

    // Set local state variables
    bScanOpen = TRUE;

    // Return ok
    return (0);
}

RC IX_IndexScan::GetNextEntry(RID &rid) {
    // Sanity Check: 'this' must be open
    if (!bScanOpen)
       return (IX_CLOSEDSCAN);

    // TODO: Sanity Check: indexHandle must be open

    //First case is an equal scan
    if(this->compOp==EQ_OP){
        //Goes down until it finds a leaf node with matching value
        while(this->currentNode->NodeType!=1){
            for(int i=0; i<currentNode->NumElements; i++) {
                //When we find greater value go to the node before
                if(currentNode->Labels[i]>value){
                    currentNode=currentNode->Pointers[i];
                    break;
                }
                //If no greater value found we go to the last node
                currentNode=currentNode->Pointers[currentNode->NumElements];
            }
        }
        //At this point currentNode should be a leaf node
        //We go through the leaf node looking for our value
        for(int i=0; i<currentNode->NumElements; i++){
            if(currentNode->Labels[i]==value){
                //
                IX_Bucket bucket = currentNode->Pointers[i];
                //Returns the next RID in the bucket
                if(currentPos<bucket.length){
                    bucket.getRID(currentPos, rid);
                    currentPos++;
                    return 0;
                }else{
                    currentPos = 0;
                }
            }
        }
    }

    return 0;
}
RC IX_IndexScan::CloseScan() {
	return 0;
}
