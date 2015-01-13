#include <cstdio>
#include <unistd.h>
#include <iostream>
#include "ix.h"
#include <string.h>

using namespace std;

IX_IndexScan::IX_IndexScan() {
    currentBucket = -1;
    currentBucketPos = -1;
    currentKey = -1;
    currentLeaf = -1;
    bScanOpen = false;
}

IX_IndexScan::~IX_IndexScan() {
    //Don't need to do anything for now
}

//Opens the scan
RC IX_IndexScan::OpenScan(const IX_IndexHandle &indexHandle, CompOp compOp,
        void *value, ClientHint pinHint) {

    // Sanity Check: 'this' should not be open yet
    if (bScanOpen)
       // Test: opened IX_IndexScan
       return (IX_SCANOPEN);

    // TODO
    //Check IndexHandle is open

    // Sanity Check: compOp
    switch (compOp) {
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
    this->compOp = compOp;
    this->value = value;
    this->indexHandle = (IX_IndexHandle *)&indexHandle;;

    // Set local state variables
    bScanOpen = true;

    // Return ok
    return (0);
}

//Gets the next entry relevant for the scan
RC IX_IndexScan::GetNextEntry(RID &rid) {
    RC rc = 0;
    // Sanity Check: 'this' must be open
    if (!bScanOpen) return IX_CLOSEDSCAN;

    //Sanity Check: indexHandle must be open
    if(indexHandle->bFileOpen==false) return IX_CLOSEDFILE;

    //If it's the first time we look for the first entry to give
    if(currentLeaf==-1){
        return goToFirstBucket(rid);
    }

    //At this point currentBucket, currentKey, currentBucketPos and currentLeaf should be set
    //Or IX_EOF was returned

    PF_PageHandle phBucket;
    IX_BucketHeader bucketHeader;
    char * pData;

    if( (rc = indexHandle->filehandle->GetThisPage(currentBucket, phBucket)) ) return rc;
    if( (rc = phBucket.GetData(pData)) ) return rc;
    memcpy(&bucketHeader, pData, sizeof(IX_BucketHeader));

    //If we are not at the end of the bucket now
    if( currentBucketPos<bucketHeader.nbRid){
        memcpy(&rid, pData+sizeof(IX_BucketHeader)+currentBucketPos*sizeof(RID), sizeof(RID));
        currentBucketPos++;
        return 0;
    }

    //Else we need to go to the next matching bucket
    if(compOp==EQ_OP){
        //Unpins current bucket and leaf
        if( (rc = indexHandle->filehandle->UnpinPage(currentBucket))
                || (rc = indexHandle->filehandle->UnpinPage(currentLeaf)) ) return rc;
        //There is no more than one bucket with a given value
        return IX_EOF;
    }
    if(compOp==LT_OP || compOp==LE_OP){
        //We have to go left in the leaf
        PF_PageHandle phLeaf;
        IX_NodeHeader leafHeader;
        char* pData2;
        if( (rc = indexHandle->filehandle->GetThisPage(currentLeaf, phLeaf)) ) return rc;
        if( (rc = phLeaf.GetData(pData2)) ) return rc;
        memcpy(&leafHeader, pData2, sizeof(IX_NodeHeader));
        //If we are not at the first key of the leaf we decrement the key
        if(currentKey>0){
            currentKey--;
            //The bucket of the new key becomes the current bucket
            if( (rc = indexHandle->getPointer(phLeaf, currentKey, currentBucket)) ) return rc;
            currentBucketPos = 0;
            //Reccursive call
            return GetNextEntry(rid);
        }

        //Unpins leaf and bucket
        if( (rc = indexHandle->filehandle->UnpinPage(currentLeaf)) ) return rc;
        if( (rc = indexHandle->filehandle->UnpinPage(currentBucket)) ) return rc;
        //Else we have to go to the leaf on the left
        if( leafHeader.prevPage<0) return IX_EOF;
        currentLeaf = leafHeader.prevPage;

        //Loads the new leaf
        if( (rc = indexHandle->filehandle->GetThisPage(currentLeaf, phLeaf)) ) return rc;
        if( (rc = phLeaf.GetData(pData2)) ) return rc;
        memcpy(&leafHeader, pData2, sizeof(IX_NodeHeader));
        currentKey = leafHeader.nbKey-1; //Sets the currentKey to the last key
        //Gets the new bucket number
        if( (rc = indexHandle->getPointer(phLeaf, currentKey, currentBucket)) ) return rc;
        currentBucketPos = 0; //Sets the bucket pos to the beginning

        //Reccursive call
        return GetNextEntry(rid);
    }
    if(compOp==GT_OP || compOp==GE_OP){
        /*
         *Basically the same as above except we go right instead of left
        */
        //We have to go right in the leaf
        PF_PageHandle phLeaf;
        IX_NodeHeader leafHeader;
        char* pData2;
        if( (rc = indexHandle->filehandle->GetThisPage(currentLeaf, phLeaf)) ) return rc;
        if( (rc = phLeaf.GetData(pData2)) ) return rc;
        memcpy(&leafHeader, pData2, sizeof(IX_NodeHeader));
        //If we are not at the last key of the leaf we increment the key
        if(currentKey<leafHeader.nbKey-1){
            currentKey++;
            //The bucket of the new key becomes the current bucket
            if( (rc = indexHandle->getPointer(phLeaf, currentKey, currentBucket)) ) return rc;
            currentBucketPos = 0;
            //Reccursive call
            return GetNextEntry(rid);
        }

        //Unpins leaf and bucket
        if( (rc = indexHandle->filehandle->UnpinPage(currentLeaf)) ) return rc;
        if( (rc = indexHandle->filehandle->UnpinPage(currentBucket)) ) return rc;
        //Else we have to go to the leaf on the left
        if( leafHeader.nextPage<0) return IX_EOF;
        currentLeaf = leafHeader.nextPage;

        //Loads the new leaf
        if( (rc = indexHandle->filehandle->GetThisPage(currentLeaf, phLeaf)) ) return rc;
        if( (rc = phLeaf.GetData(pData2)) ) return rc;
        memcpy(&leafHeader, pData2, sizeof(IX_NodeHeader));
        currentKey = 0; //Sets the currentKey to the first key
        //Gets the new bucket number
        if( (rc = indexHandle->getPointer(phLeaf, currentKey, currentBucket)) ) return rc;
        currentBucketPos = 0; //Sets the bucket pos to the beginning

        //Reccursive call
        return GetNextEntry(rid);
    }

    //We should never get there
    return IX_INVALIDCOMPOP;
}

//Closes the scan
RC IX_IndexScan::CloseScan() {
    if(bScanOpen==false){
        return IX_CLOSEDSCAN;
    }
    currentBucket = -1;
    currentLeaf = -1;
    currentBucketPos = -1;
    currentKey = -1;
    bScanOpen = false;
    return 0;
}

//Goes to the right leaf and bucket
RC IX_IndexScan::goToFirstBucket(RID &rid){
    RC rc = 0;
    //If no root nothing to do
    if(indexHandle->fileHeader.rootNb<0) return IX_EOF;
    currentLeaf = indexHandle->fileHeader.rootNb;

    //PageHandle and NodeHeader for the node
    PF_PageHandle pageHandle;
    IX_NodeHeader nodeHeader;
    char* pData;

    //Looks for a leaf node
    while(1>0){
        if( (rc = indexHandle->filehandle->GetThisPage(currentLeaf, pageHandle)) ) return rc;
        if( (rc = pageHandle.GetData(pData)) ) return rc;
        memcpy(&nodeHeader, pData, sizeof(IX_NodeHeader));

        //If it's a leaf node we break
        if(nodeHeader.level==1){
            break;
        }

        //The node shouldn't be empty
        if(nodeHeader.nbKey<=0) return IX_EMPTYNODE;

        //Browse the node
        int i=0;
        for(; i<nodeHeader.nbKey; i++){
            if(indexHandle->IsKeyGreater(value, pageHandle, i)>=0){
                break;
            }
        }
        //Unpins, marks dirty
        if( (rc = indexHandle->filehandle->MarkDirty(currentLeaf))
                || indexHandle->filehandle->UnpinPage(currentLeaf) ) return rc;
        //Assigns new value to currentLeaf
        if( (rc = indexHandle->getPointer(pageHandle, i-1, currentLeaf)) ) return rc;
    }

    //Now we are at the leaf where the value is if it exists in the three

    //We have to find the first entry according to the compop
    if(compOp==EQ_OP){
        //Browse the node and looks for the value
        int i=0;
        for(; i<nodeHeader.nbKey; i++){
            if(indexHandle->IsKeyGreater(value, pageHandle, i)==0){
                break;
            }
        }
        //If we didn't break anywhere => no matching entry
        if(i==nodeHeader.nbKey){
            if( (rc = indexHandle->filehandle->UnpinPage(currentLeaf)) ) return rc;
            return IX_EOF;
        }
        //Else finds the right bucket
        if( (rc = indexHandle->getPointer(pageHandle, i, currentBucket)) ) return rc;
        currentBucketPos = 0;
        currentKey = i;
        //Reccursive call
        return GetNextEntry(rid);
    }
    if(compOp==LT_OP){
        //Browse the node and looks for a lower value
        int i=nodeHeader.nbKey-1;
        for(; i>=0; i--){
            if(indexHandle->IsKeyGreater(value, pageHandle, i)<0){
                break;
            }
        }
        //If we didn't break anywhere => we have to look at the very right value of prev leaf
        if(i==nodeHeader.nbKey){
            //If the leaf was the very left one no match
            if(nodeHeader.prevPage<0){
                if( (rc = indexHandle->filehandle->UnpinPage(currentLeaf)) ) return rc;
                return IX_EOF;
            }
            //Unpins, marks dirty
            if( (rc = indexHandle->filehandle->MarkDirty(currentLeaf))
                    || indexHandle->filehandle->UnpinPage(currentLeaf) ) return rc;
            //The leaf on the left becomes the current leaf
            currentLeaf = nodeHeader.prevPage;
            if( (rc = indexHandle->filehandle->GetThisPage(currentLeaf, pageHandle)) ) return rc;
            if( (rc = pageHandle.GetData(pData)) ) return rc;
            memcpy(&nodeHeader, pData, sizeof(IX_NodeHeader));
            //We look at the last key on the leaf
            if(indexHandle->IsKeyGreater(value, pageHandle, nodeHeader.nbKey-1)<0){
                if( (rc = indexHandle->getPointer(pageHandle, nodeHeader.nbKey-1, currentBucket)) ) return rc;
                currentBucketPos = 0;
                currentKey = nodeHeader.nbKey-1;
                //Reccursive call
                return GetNextEntry(rid);
            }else{
                if( (rc = indexHandle->filehandle->UnpinPage(currentLeaf)) ) return rc;
                return IX_EOF;
            }
        }
        //Else finds the right bucket
        if( (rc = indexHandle->getPointer(pageHandle, i, currentBucket)) ) return rc;
        currentBucketPos = 0;
        currentKey = i;
        //Reccursive call
        return GetNextEntry(rid);
    }
    if(compOp==GT_OP){
        /*
         *This case is the same as LT_OP except we browse right instead of left
        */
        //Browse the node and looks for a lower value
        int i=0;
        for(; i<nodeHeader.nbKey; i++){
            if(indexHandle->IsKeyGreater(value, pageHandle, i)>0){
                break;
            }
        }
        //If we didn't break anywhere => we have to look at the very left value of prev leaf
        if(i==nodeHeader.nbKey){
            //If the leaf was the very right one no match
            if(nodeHeader.nextPage<0){
                if( (rc = indexHandle->filehandle->UnpinPage(currentLeaf)) ) return rc;
                return IX_EOF;
            }
            //Unpins, marks dirty
            if( (rc = indexHandle->filehandle->MarkDirty(currentLeaf))
                    || indexHandle->filehandle->UnpinPage(currentLeaf) ) return rc;
            //The leaf on the left becomes the current leaf
            currentLeaf = nodeHeader.nextPage;
            if( (rc = indexHandle->filehandle->GetThisPage(currentLeaf, pageHandle)) ) return rc;
            if( (rc = pageHandle.GetData(pData)) ) return rc;
            memcpy(&nodeHeader, pData, sizeof(IX_NodeHeader));
            //We look at the last key on the leaf
            if(indexHandle->IsKeyGreater(value, pageHandle, 0)<0){
                if( (rc = indexHandle->getPointer(pageHandle, 0, currentBucket)) ) return rc;
                currentBucketPos = 0;
                currentKey = 0;
                //Reccursive call
                return GetNextEntry(rid);
            }else{
                if( (rc = indexHandle->filehandle->UnpinPage(currentLeaf)) ) return rc;
                return IX_EOF;
            }
        }
        //Else finds the right bucket
        if( (rc = indexHandle->getPointer(pageHandle, i, currentBucket)) ) return rc;
        currentBucketPos = 0;
        currentKey = i;
        //Reccursive call
        return GetNextEntry(rid);
    }
    if(compOp==LE_OP){
        /*
         *This case is the same as LT_OP except that when we use
         *IsKeyCreater() we use <= instead of <
        */
        int i=nodeHeader.nbKey-1;
        for(; i>=0; i--){
            if(indexHandle->IsKeyGreater(value, pageHandle, i)<=0){
                break;
            }
        }
        //If we didn't break anywhere => we have to look at the very right value of prev leaf
        if(i==nodeHeader.nbKey){
            //If the leaf was the very left one no match
            if(nodeHeader.prevPage<0){
                if( (rc = indexHandle->filehandle->UnpinPage(currentLeaf)) ) return rc;
                return IX_EOF;
            }
            //Unpins, marks dirty
            if( (rc = indexHandle->filehandle->MarkDirty(currentLeaf))
                    || indexHandle->filehandle->UnpinPage(currentLeaf) ) return rc;
            //The leaf on the left becomes the current leaf
            currentLeaf = nodeHeader.prevPage;
            if( (rc = indexHandle->filehandle->GetThisPage(currentLeaf, pageHandle)) ) return rc;
            if( (rc = pageHandle.GetData(pData)) ) return rc;
            memcpy(&nodeHeader, pData, sizeof(IX_NodeHeader));
            //We look at the last key on the leaf
            if(indexHandle->IsKeyGreater(value, pageHandle, nodeHeader.nbKey-1)<=0){
                if( (rc = indexHandle->getPointer(pageHandle, nodeHeader.nbKey-1, currentBucket)) ) return rc;
                currentBucketPos = 0;
                currentKey = nodeHeader.nbKey-1;
                //Reccursive call
                return GetNextEntry(rid);
            }else{
                if( (rc = indexHandle->filehandle->UnpinPage(currentLeaf)) ) return rc;
                return IX_EOF;
            }
        }
        //Else finds the right bucket
        if( (rc = indexHandle->getPointer(pageHandle, i, currentBucket)) ) return rc;
        currentBucketPos = 0;
        currentKey = i;
        //Reccursive call
        return GetNextEntry(rid);
    }
    if(compOp==GE_OP){
        /*
         *This case is the same as GT_OP except that when we use
         *IsKeyCreater() we use >= instead of >
        */
        //Browse the node and looks for a lower value
        int i=0;
        for(; i<nodeHeader.nbKey; i++){
            if(indexHandle->IsKeyGreater(value, pageHandle, i)>=0){
                break;
            }
        }
        //If we didn't break anywhere => we have to look at the very left value of prev leaf
        if(i==nodeHeader.nbKey){
            //If the leaf was the very right one no match
            if(nodeHeader.nextPage<0){
                if( (rc = indexHandle->filehandle->UnpinPage(currentLeaf)) ) return rc;
                return IX_EOF;
            }
            //Unpins, marks dirty
            if( (rc = indexHandle->filehandle->MarkDirty(currentLeaf))
                    || indexHandle->filehandle->UnpinPage(currentLeaf) ) return rc;
            //The leaf on the left becomes the current leaf
            currentLeaf = nodeHeader.nextPage;
            if( (rc = indexHandle->filehandle->GetThisPage(currentLeaf, pageHandle)) ) return rc;
            if( (rc = pageHandle.GetData(pData)) ) return rc;
            memcpy(&nodeHeader, pData, sizeof(IX_NodeHeader));
            //We look at the last key on the leaf
            if(indexHandle->IsKeyGreater(value, pageHandle, 0)<=0){
                if( (rc = indexHandle->getPointer(pageHandle, 0, currentBucket)) ) return rc;
                currentBucketPos = 0;
                currentKey = 0;
                //Reccursive call
                return GetNextEntry(rid);
            }else{
                if( (rc = indexHandle->filehandle->UnpinPage(currentLeaf)) ) return rc;
                return IX_EOF;
            }
        }
        //Else finds the right bucket
        if( (rc = indexHandle->getPointer(pageHandle, i, currentBucket)) ) return rc;
        currentBucketPos = 0;
        currentKey = i;
        //Reccursive call
        return GetNextEntry(rid);
    }

    //We should never get there
    return IX_INVALIDCOMPOP;
}
