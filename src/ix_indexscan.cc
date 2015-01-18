#include <cstdio>
#include <unistd.h>
#include <stdlib.h>
#include <iostream>
#include "ix.h"
#include <string.h>

using namespace std;

IX_IndexScan::IX_IndexScan() {
    currentBucket = -1;
    currentBucketPos = -1;
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
    bIsEOF = false;

    //Allocates memory for our
    currentKey = malloc(indexHandle.fileHeader.keySize);

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

    //If the boolean is true, means we have to return EOF
    if(bIsEOF){
        return IX_EOF;
    }

    //If it's the first time we look for the first entry to give
    if(currentLeaf==-1){
        return goToFirstBucket(rid);
    }

    /*
     *GoToFirstBucket was called just before so we are 100% sure that currentBucket and
     *currentBucketPos describe an existing and matching RID.
     *Hence we copy this RID to the rid parameter right now and move on to the next
     *matching RID if there is one, or set pIsEOF to true if there is not.
     *This way the next time getNextEntry will be called it will give the rid we moved
     *on to, which makes sure there is no problem if the scan is used to
     *perform deletion.
    */

    //At this point currentBucket, currentKey, currentBucketPos and currentLeaf should be set
    //Or IX_EOF was returned

    PF_PageHandle phBucket;
    IX_BucketHeader bucketHeader;
    char * pData;

    //Gets the current bucket
    if( (rc = indexHandle->filehandle->GetThisPage(currentBucket, phBucket)) ) return rc;
    if( (rc = phBucket.GetData(pData)) ) return rc;
    memcpy(&bucketHeader, pData, sizeof(IX_BucketHeader));

    //Just a trick to avoid loading the bucket from memory each time
    if(currentBucketPos == EndOfBucket){
        currentBucketPos = bucketHeader.nbRid-1;
    }

    //This is the moment we actually put the nextEntry in rid
    memcpy(&rid, pData+sizeof(IX_BucketHeader)+currentBucketPos*sizeof(RID), sizeof(RID));

    /*
     *All that follows is about moving on to the next bucket to prepare the next call
    */
    currentBucketPos--;

    //If we are not at the beginning of the bucket now
    if( currentBucketPos>=0 ){
        //We just have to unpin and return
        if( (rc = indexHandle->filehandle->UnpinPage(currentBucket)) ) return rc;
        return 0;
    //Or if the bucket has a next bucket
    }else if(bucketHeader.nextBucket!=-1){
        //We unpin the bucket
        if( (rc = indexHandle->filehandle->UnpinPage(currentBucket)) ) return rc;
        currentBucket = bucketHeader.nextBucket;
        //BucketPos starts at the end again
        currentBucketPos = EndOfBucket;
        return 0;
    }

    //Else we need to go to the next matching bucket
    if(compOp==EQ_OP){
        //Unpins current bucket and leaf
        if( (rc = indexHandle->filehandle->UnpinPage(currentBucket)) ) return rc;
        //No more than one key with a given value so next call will return IX_EOF
        bIsEOF = true;
        return 0;
    }
    if(compOp==LT_OP || compOp==LE_OP){
        //We have to go left in the leaf
        PF_PageHandle phLeaf;
        IX_NodeHeader leafHeader;
        char* pData2;
        if( (rc = indexHandle->filehandle->GetThisPage(currentLeaf, phLeaf)) ) return rc;
        if( (rc = phLeaf.GetData(pData2)) ) return rc;
        memcpy(&leafHeader, pData2, sizeof(IX_NodeHeader));

        //Retrieves the current key number
        int currentKeyNb;
        if( (rc = getCurrentKeyNb(phLeaf, currentKeyNb)) ) return rc;

        //If we are not at the first key of the leaf we decrement the key
        if(currentKeyNb>0){
            currentKeyNb--;
            //The bucket of the new key becomes the current bucket
            if( (rc = indexHandle->filehandle->UnpinPage(currentBucket)) ) return rc;
            if( (rc = indexHandle->getPointer(phLeaf, currentKeyNb, currentBucket)) ) return rc;
            currentBucketPos = EndOfBucket;
            //Saves current key, Unpins and returns
            if( (rc = saveCurrentKey(phLeaf, currentKeyNb))) return rc;
            if( (rc = indexHandle->filehandle->UnpinPage(currentLeaf)) ) return rc;
            return 0;
        }

        //Unpins leaf and bucket
        if( (rc = indexHandle->filehandle->UnpinPage(currentLeaf)) ) return rc;
        if( (rc = indexHandle->filehandle->UnpinPage(currentBucket)) ) return rc;
        //Else we have to go to the leaf on the left
        if( leafHeader.prevPage<0){
            bIsEOF = true;
            return 0;
        }
        currentLeaf = leafHeader.prevPage;

        //Loads the new leaf
        if( (rc = indexHandle->filehandle->GetThisPage(currentLeaf, phLeaf)) ) return rc;
        if( (rc = phLeaf.GetData(pData2)) ) return rc;
        memcpy(&leafHeader, pData2, sizeof(IX_NodeHeader));
        currentKeyNb = leafHeader.nbKey-1; //Sets the currentKeyNb to the last key
        //Gets the new bucket number
        if( (rc = indexHandle->getPointer(phLeaf, currentKeyNb, currentBucket)) ) return rc;
        currentBucketPos = EndOfBucket; //Sets the bucket pos to the end

        //Saves the current key, Unpins, and return
        if( (rc = saveCurrentKey(phLeaf, currentKeyNb))) return rc;
        if( (rc = indexHandle->filehandle->UnpinPage(currentLeaf)) ) return rc;
        return 0;
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

        //Retrieves the current key number
        int currentKeyNb;
        if( (rc = getCurrentKeyNb(phLeaf, currentKeyNb)) ) return rc;

        //If we are not at the last key of the leaf we increment the key
        if(currentKeyNb<leafHeader.nbKey-1){
            currentKeyNb++;
            //The bucket of the new key becomes the current bucket
            if( (rc = indexHandle->filehandle->UnpinPage(currentBucket)) ) return rc;
            if( (rc = indexHandle->getPointer(phLeaf, currentKeyNb, currentBucket)) ) return rc;
            currentBucketPos = EndOfBucket;
            //Saves the current key, Unpins, and return
            if( (rc = saveCurrentKey(phLeaf, currentKeyNb))) return rc;
            if( (rc = indexHandle->filehandle->UnpinPage(currentLeaf)) ) return rc;
            return 0;
        }

        //Unpins leaf and bucket
        if( (rc = indexHandle->filehandle->UnpinPage(currentLeaf)) ) return rc;
        if( (rc = indexHandle->filehandle->UnpinPage(currentBucket)) ) return rc;
        //Else we have to go to the leaf on the right
        if( leafHeader.nextPage<0){
            bIsEOF = true;
            return 0;
        }
        currentLeaf = leafHeader.nextPage;

        //Loads the new leaf
        if( (rc = indexHandle->filehandle->GetThisPage(currentLeaf, phLeaf)) ) return rc;
        if( (rc = phLeaf.GetData(pData2)) ) return rc;
        memcpy(&leafHeader, pData2, sizeof(IX_NodeHeader));
        currentKeyNb = 0; //Sets the currentKeyNb to the first key
        //Gets the new bucket number
        if( (rc = indexHandle->getPointer(phLeaf, currentKeyNb, currentBucket)) ) return rc;
        currentBucketPos = EndOfBucket; //Sets the bucket pos to the end

        //Saves the current key, Unpins, and return
        if( (rc = saveCurrentKey(phLeaf, currentKeyNb))) return rc;
        if( (rc = indexHandle->filehandle->UnpinPage(currentLeaf)) ) return rc;
        return 0;
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
    free(currentKey); //Desallocates memory for the current key
    bScanOpen = false;
    return 0;
}

//Goes to the right leaf and bucket
RC IX_IndexScan::goToFirstBucket(RID &rid){
    RC rc = 0;
    //If no root nothing to do
    if(indexHandle->fileHeader.rootNb<0){
        bIsEOF = true;
        return GetNextEntry(rid);
    }
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

        //Browse the node
        int i=0;
        for(; i<nodeHeader.nbKey; i++){
            if(indexHandle->IsKeyGreater(value, pageHandle, i)>0){
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
            bIsEOF = true;
            return GetNextEntry(rid);
        }
        //Else finds the right bucket
        if( (rc = indexHandle->getPointer(pageHandle, i, currentBucket)) ) return rc;
        currentBucketPos = EndOfBucket;
        //Updates current key
        if( (rc = saveCurrentKey(pageHandle, i)) ) return rc;
        //Reccursive call
        if( (rc = indexHandle->filehandle->UnpinPage(currentLeaf)) ) return rc;
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
        if(i==-1){
            //If the leaf was the very left one no match
            if(nodeHeader.prevPage<0){
                if( (rc = indexHandle->filehandle->UnpinPage(currentLeaf)) ) return rc;
                bIsEOF = true;
                return GetNextEntry(rid);
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
                currentBucketPos = EndOfBucket;
                //Updates current key
                if( (rc = saveCurrentKey(pageHandle, nodeHeader.nbKey-1)) ) return rc;
                //Reccursive call
                if( (rc = indexHandle->filehandle->UnpinPage(currentLeaf)) ) return rc;
                return GetNextEntry(rid);
            }else{
                if( (rc = indexHandle->filehandle->UnpinPage(currentLeaf)) ) return rc;
                bIsEOF = true;
                return GetNextEntry(rid);
            }
        }
        //Else finds the right bucket
        if( (rc = indexHandle->getPointer(pageHandle, i, currentBucket)) ) return rc;
        currentBucketPos = EndOfBucket;
        //Updates current key
        if( (rc = saveCurrentKey(pageHandle, i)) ) return rc;
        //Reccursive call
        if( (rc = indexHandle->filehandle->UnpinPage(currentLeaf)) ) return rc;
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
        //If we didn't break anywhere => we have to look at the very left value of next leaf
        if(i==nodeHeader.nbKey){
            //If the leaf was the very right one no match
            if(nodeHeader.nextPage<0){
                if( (rc = indexHandle->filehandle->UnpinPage(currentLeaf)) ) return rc;
                bIsEOF = true;
                return GetNextEntry(rid);
            }
            //Unpins, marks dirty
            if( (rc = indexHandle->filehandle->MarkDirty(currentLeaf))
                    || indexHandle->filehandle->UnpinPage(currentLeaf) ) return rc;
            //The leaf on the right becomes the current leaf
            currentLeaf = nodeHeader.nextPage;
            if( (rc = indexHandle->filehandle->GetThisPage(currentLeaf, pageHandle)) ) return rc;
            if( (rc = pageHandle.GetData(pData)) ) return rc;
            memcpy(&nodeHeader, pData, sizeof(IX_NodeHeader));
            //We look at the first key on the leaf
            if(indexHandle->IsKeyGreater(value, pageHandle, 0)>0){
                if( (rc = indexHandle->getPointer(pageHandle, 0, currentBucket)) ) return rc;
                if( (rc = indexHandle->filehandle->UnpinPage(currentLeaf)) ) return rc;
                currentBucketPos = EndOfBucket;
                //Updates current key
                if( (rc = saveCurrentKey(pageHandle, 0)) ) return rc;
                //Reccursive call
                return GetNextEntry(rid);
            }else{
                if( (rc = indexHandle->filehandle->UnpinPage(currentLeaf)) ) return rc;
                bIsEOF = true;
                return GetNextEntry(rid);
            }
        }
        //Else finds the right bucket
        if( (rc = indexHandle->getPointer(pageHandle, i, currentBucket)) ) return rc;
        currentBucketPos = EndOfBucket;
        //Updates current key
        if( (rc = saveCurrentKey(pageHandle, i)) ) return rc;
        //Reccursive call
        if( (rc = indexHandle->filehandle->UnpinPage(currentLeaf)) ) return rc;
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
        if(i==-1){
            //If the leaf was the very left one no match
            if(nodeHeader.prevPage<0){
                if( (rc = indexHandle->filehandle->UnpinPage(currentLeaf)) ) return rc;
                bIsEOF = true;
                return GetNextEntry(rid);
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
                currentBucketPos = EndOfBucket;
                //Updates current key
                if( (rc = saveCurrentKey(pageHandle, nodeHeader.nbKey-1)) ) return rc;
                //Reccursive call
                if( (rc = indexHandle->filehandle->UnpinPage(currentLeaf)) ) return rc;
                return GetNextEntry(rid);
            }else{
                if( (rc = indexHandle->filehandle->UnpinPage(currentLeaf)) ) return rc;
                bIsEOF = true;
                return GetNextEntry(rid);
            }
        }
        //Else finds the right bucket
        if( (rc = indexHandle->getPointer(pageHandle, i, currentBucket)) ) return rc;
        currentBucketPos = EndOfBucket;
        //Updates current key
        if( (rc = saveCurrentKey(pageHandle, i)) ) return rc;
        //Reccursive call
        if( (rc = indexHandle->filehandle->UnpinPage(currentLeaf)) ) return rc;
        return GetNextEntry(rid);
    }
    if(compOp==GE_OP){
        /*
         *This case is the same as GT_OP except that when we use
         *IsKeyCreater() we use >= instead of >
        */
        //Browse the node and looks for a greater value
        int i=0;
        for(; i<nodeHeader.nbKey; i++){
            if(indexHandle->IsKeyGreater(value, pageHandle, i)>=0){
                break;
            }
        }
        //If we didn't break anywhere => we have to look at the very left value of next leaf
        if(i==nodeHeader.nbKey){
            //If the leaf was the very right one no match
            if(nodeHeader.nextPage<0){
                if( (rc = indexHandle->filehandle->UnpinPage(currentLeaf)) ) return rc;
                bIsEOF = true;
                return GetNextEntry(rid);
            }
            //Unpins, marks dirty
            if( (rc = indexHandle->filehandle->MarkDirty(currentLeaf))
                    || indexHandle->filehandle->UnpinPage(currentLeaf) ) return rc;
            //The leaf on the right becomes the current leaf
            currentLeaf = nodeHeader.nextPage;
            if( (rc = indexHandle->filehandle->GetThisPage(currentLeaf, pageHandle)) ) return rc;
            if( (rc = pageHandle.GetData(pData)) ) return rc;
            memcpy(&nodeHeader, pData, sizeof(IX_NodeHeader));
            //We look at the first key on the leaf
            if(indexHandle->IsKeyGreater(value, pageHandle, 0)>=0){
                if( (rc = indexHandle->getPointer(pageHandle, 0, currentBucket)) ) return rc;
                currentBucketPos = EndOfBucket;
                //Updates current key
                if( (rc = saveCurrentKey(pageHandle, 0)) ) return rc;
                //Reccursive call
                if( (rc = indexHandle->filehandle->UnpinPage(currentLeaf)) ) return rc;
                return GetNextEntry(rid);
            }else{
                if( (rc = indexHandle->filehandle->UnpinPage(currentLeaf)) ) return rc;
                bIsEOF = true;
                return GetNextEntry(rid);
            }
        }
        //Else finds the right bucket
        if( (rc = indexHandle->getPointer(pageHandle, i, currentBucket)) ) return rc;
        currentBucketPos = EndOfBucket;
        //Updates current key
        if( (rc = saveCurrentKey(pageHandle, i)) ) return rc;
        //Reccursive call
        if( (rc = indexHandle->filehandle->UnpinPage(currentLeaf)) ) return rc;
        return GetNextEntry(rid);
    }

    //We should never get there
    return IX_INVALIDCOMPOP;
}


//Get the key number in leaf thanks to its value
RC IX_IndexScan::getCurrentKeyNb(PF_PageHandle phLeaf, int &currentKeyNb){
    RC rc;
    //Assumes the ph is open etc.
    char * pData;
    IX_NodeHeader leafHeader;
    if( (rc = phLeaf.GetData(pData)) ) return rc;
    memcpy(&leafHeader, pData, sizeof(IX_NodeHeader));
    for(currentKeyNb = 0; currentKeyNb < leafHeader.nbKey; currentKeyNb++){
        if(indexHandle->IsKeyGreater(currentKey, phLeaf, currentKeyNb)==0){
            //We found the key we return
            return 0;
        }
    }
    //If we didn't find the key in the leaf, error
    return IX_ENTRYNOTFOUND;
}


//Save the currenKeyNb key to our currentKey attribute
RC IX_IndexScan::saveCurrentKey(PF_PageHandle phLeaf, const int &currentKeyNb){
    RC rc;
    //Assumes ph is open, etc.
    //Also please note this method doesn't ensure the key exists it just copies
    char * pData;
    if( (rc = phLeaf.GetData(pData)) ) return rc;
    //Copies to our currentKey
    pData += sizeof(IX_NodeHeader)+currentKeyNb*(indexHandle->SizePointer+indexHandle->fileHeader.keySize);
    memcpy(currentKey, pData, indexHandle->fileHeader.keySize);
    return 0;
}









