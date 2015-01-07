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
    case NE_OP:
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
	return 0;
}
RC IX_IndexScan::CloseScan() {
	return 0;
}
