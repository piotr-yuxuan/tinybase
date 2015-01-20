#include <cstdio>
#include <unistd.h>
#include <iostream>
#include "ix.h"
#include <string.h>

using namespace std;

/*
 * Implementation sketch of a B+ tree. This structure gracefully reorganizes
 * itself when needed.
 *
 * <pre>
 * 0    1    2    3    4    5    ← pointers
 * || 0 || 1 || 2 || 3 || 4 ||   ← labels
 * </pre>
 *
 * The node pointer to the left of a key k points to a subtree that contains
 * only data entries less than k. A The node pointer to the right of a key value
 * k points to a subtree that contains only data entries greater than or equal
 * to k.
 */

//Constructor
IX_IndexHandle::IX_IndexHandle() {
    this->bFileOpen = false;
}

//Destructor
IX_IndexHandle::~IX_IndexHandle() {
    //Nothing to do for now
}

//Inserts a new entry using the insertions methods
RC IX_IndexHandle::InsertEntry(void *pData, const RID &rid) {
    RC rc = 0;
    //Sanity check: pData must not be NULL
    if(pData==NULL){
        return IX_NULLPOINTER;
    }

    //If the root number is not defined we create a root
    if(this->fh.rootNb==-1){
        //Allocates a new page for the root
        PF_PageHandle phRoot;
        if(this->filehandle->AllocatePage(phRoot)) return rc;
        //Creates a header for the root
        IX_NodeHeader rootHeader;
        rootHeader.level = -1;
        rootHeader.maxKeyNb = fh.order * 2;
        rootHeader.nbKey = 0; //Due to the first leaf (see below)
        rootHeader.prevPage = -1;
        rootHeader.nextPage = -1;
        rootHeader.parentPage = -1;
        //Copy the header of the root to memory
        char* pData2;
        if( (rc = phRoot.GetData(pData2)) ) return rc;
        memcpy(pData2, &rootHeader, sizeof(IX_NodeHeader));
        //Updates the root page number in file header
        PageNum nb;
        if( (rc = phRoot.GetPageNum(nb)) ) return rc;
        this->fh.rootNb = nb;
        /*
        * We also have to create the first leaf node and make it a child of the root
        * (root will have no key, only the -1 pointer
        */
        PF_PageHandle phLeaf;
        IX_NodeHeader leafHeader;
        PageNum leafNodeNb;
        char * pDataLeaf;
        if( (rc = filehandle->AllocatePage(phLeaf)) ) return rc;
        if( (rc = phLeaf.GetPageNum(leafNodeNb)) ) return rc;
        if( (rc = phLeaf.GetData(pDataLeaf)) ) return rc;
        leafHeader.level = 1;
        leafHeader.maxKeyNb = fh.order * 2;
        leafHeader.nbKey = 0;
        leafHeader.nextPage = -1;
        leafHeader.prevPage = -1;
        leafHeader.parentPage = fh.rootNb;
        //Writes leafHeader to memory
        memcpy(pDataLeaf, &leafHeader, sizeof(IX_NodeHeader));
        //Sets the leaf as the -1 pointer of the root
        if( (rc = setPointer(phRoot, -1, leafNodeNb)) ) return rc;
        //Mark dirty and unpins for both root and first leaf
        if( (rc = filehandle->MarkDirty(leafNodeNb)) ) return rc;
        if( (rc = filehandle->UnpinPage(leafNodeNb)) ) return rc;
        if( (rc = filehandle->MarkDirty(fh.rootNb)) ) return rc;
        if( (rc = filehandle->UnpinPage(fh.rootNb)) ) return rc;
        if( (rc = filehandle->ForcePages()) ) return rc;
    }
    //The root is either created or exists, we can insert
    return InsertEntryToNode(this->fh.rootNb, pData, rid);
}

//Deletes an entry
RC IX_IndexHandle::DeleteEntry(void *pData, const RID &rid) {
    RC rc = 0;
    //Sanity check: pData must not be NULL
    if(pData==NULL){
        return IX_NULLPOINTER;
    }

    //If the root is not set we return error
    if(this->fh.rootNb<0){
        return IX_ENTRYNOTFOUND;
    }

    //We have to go to the location of the RID in tree
    PF_PageHandle pageHandle;
    IX_NodeHeader nodeHeader;
    PageNum nodeNum = fh.rootNb; //Starts research at root
    char* pDataNode;
    while(1>0){
        if( (rc = filehandle->GetThisPage(nodeNum, pageHandle)) ) return rc;
        if( (rc = pageHandle.GetData(pDataNode)) ) return rc;
        //Copies header from memory
        memcpy(&nodeHeader, pDataNode, sizeof(IX_NodeHeader));
        //If node is a leaf, moves on to following
        if(nodeHeader.level==1) break;
        //Looks for the right child
        int pos = 0;
        for(; pos<nodeHeader.nbKey; pos++){
            if(IsKeyGreater(pData, pageHandle, pos)>0){
                break;
            }
        }
        //If we didn't break anywhere pos==nodeHeader.nbKey
        //Gives its new value to nodeNum
        PageNum newNodeNum;
        if( (rc = getPointer(pageHandle, pos-1, newNodeNum)) ) return rc;
        //Unpins
        if( (rc = filehandle->UnpinPage(nodeNum)));
        nodeNum = newNodeNum;
    }

    /*
     *At this point we should be at the right leaf where rid is located
    */
    //Looks for the pData value
    int i=0;
    for(;i<nodeHeader.nbKey; i++){
        if(IsKeyGreater(pData, pageHandle, i)==0) break;
    }
    //If we didn't break anywhere, means the value doesn't exist
    if(i==nodeHeader.nbKey){
        return IX_ENTRYNOTFOUND;
    }
    //Else retrieves the bucket page number
    PageNum bucketNum;
    if( (rc = getPointer(pageHandle, i, bucketNum)) ) return rc;
    //Removes the entry from the bucket
    if( (rc = filehandle->UnpinPage(nodeNum)) ) return rc;
    return DeleteEntryFromBucket(bucketNum, rid, nodeNum, false);
}

//Deletes an entry in a specified bucket
RC IX_IndexHandle::DeleteEntryFromBucket(const PageNum bucketNum, const RID &rid, const PageNum bucketParent, bool nextBucket){
    /*
     *The boolean argument nextBucket value is true if the bucket is not the first one associated with
     *its leaf. In this case, bucketParent is the pageNum of the previous bucket (and not of the leaf)
    */
    RC rc;
    //Retrieves the bucket from PF
    PF_PageHandle phBucket;
    IX_BucketHeader bucketHeader;
    char* pDataBucket;

    if( (rc = filehandle->GetThisPage(bucketNum, phBucket)) ) return rc;
    if( (rc = phBucket.GetData(pDataBucket)) ) return rc;
    memcpy(&bucketHeader, pDataBucket, sizeof(IX_BucketHeader));

    //Now we have to go through all the RIDs of the bucket
    int pos=0;
    for(; pos<bucketHeader.nbRid; pos++){
        //Reads rid n°pos of the bucket
        RID bucketRid;
        memcpy(&bucketRid, pDataBucket+sizeof(IX_BucketHeader)+pos*sizeof(RID), sizeof(RID));
        //Compares the rids
        if(rid==bucketRid) break;
    }
    //If we didn't break means the rid doesn't exist in this bucket
    if(pos==bucketHeader.nbRid){
        if(bucketHeader.nextBucket!=-1){
            if( (rc = filehandle->UnpinPage(bucketNum)) ) return rc;
            //Recursive call with the bucket as parent of the next bucket
            return DeleteEntryFromBucket(bucketHeader.nextBucket, rid, bucketNum, true);
        }else{
            return IX_ENTRYNOTFOUND;
        }
    }
    //Else we have to remove the rid (i.e. offset the following rids)
    for(int i=pos; i<bucketHeader.nbRid; i++){
        char* ridPos;
        ridPos = pDataBucket+sizeof(IX_BucketHeader)+i*sizeof(RID);
        memcpy(ridPos, ridPos+sizeof(RID), sizeof(RID));
    }
    //Decrements the number of rids of the bucket
    bucketHeader.nbRid--;

    //Now if the nb of rid == 0 we have to remove the bucket
    if(bucketHeader.nbRid==0){
        if( (rc = filehandle->UnpinPage(bucketNum)) ) return rc;
        //Deallocate the page
        if( (rc = filehandle->DisposePage(bucketNum)) ) return rc;
        //We act according to the value of the nextBucket bool
        if(nextBucket){
            //Retrieves the previous bucket
            PF_PageHandle phPrevBucket;
            char* pDataPrevBucket;
            IX_BucketHeader prevBucketHeader;
            if( (rc = filehandle->GetThisPage(bucketParent, phPrevBucket)) ) return rc;
            if( (rc = phPrevBucket.GetData(pDataPrevBucket)) ) return rc;
            memcpy(&prevBucketHeader, pDataPrevBucket, sizeof(IX_BucketHeader));
            //In case there is another bucket behind
            prevBucketHeader.nextBucket = bucketHeader.nextBucket;
            //Saves to memory and unpins
            memcpy(pDataPrevBucket, &prevBucketHeader, sizeof(IX_BucketHeader));
            if( (rc = filehandle->MarkDirty(bucketParent)) ) return rc;
            if( (rc = filehandle->UnpinPage(bucketParent)) ) return rc;
            return 0;
        }else{
            //If we are here that means the bucket is the first one attached to the leaf
            if(bucketHeader.nextBucket==-1){
                //Recursive call to propagate above in the tree
                return DeleteBucketEntryFromLeafNode(bucketParent, bucketNum);
            }else{
                //In this case we have to attach the next bucket to the parent leaf
                PF_PageHandle phParentLeaf;
                char* pDataParentLeaf;
                IX_NodeHeader parentLeafHeader;
                if( (rc = filehandle->GetThisPage(bucketParent, phParentLeaf)) ) return rc;
                if( (rc = phParentLeaf.GetData(pDataParentLeaf)) ) return rc;
                memcpy(&parentLeafHeader, pDataParentLeaf, sizeof(IX_NodeHeader));
                //Looks for the pointer to the removed bucket in parent leaf
                for(int i=0; i<parentLeafHeader.nbKey; i++){
                    PageNum pointer;
                    if( (rc = getPointer(phParentLeaf, i, pointer)) ) return rc;
                    if(pointer==bucketNum){
                        //We found the right one, we update parent leaf
                        if( (rc = setPointer(phParentLeaf, i, bucketHeader.nextBucket)) ) return rc;
                        if( (rc = filehandle->MarkDirty(bucketParent)) ) return rc;
                        if( (rc = filehandle->UnpinPage(bucketParent)) ) return rc;
                        return 0;
                    }
                }
                return IX_ENTRYNOTFOUND;
            }
        }
    }
    //Else we just write the header back to memory and unpin the bucket
    memcpy(pDataBucket, &bucketHeader, sizeof(IX_BucketHeader));
    if( (rc = filehandle->MarkDirty(bucketNum))
            ||(rc = filehandle->UnpinPage(bucketNum)) ) return rc;

    return 0;
}

//Deletes the bucket entry in a leaf
RC IX_IndexHandle::DeleteBucketEntryFromLeafNode(const PageNum leafNum, const PageNum bucketNum){
    RC rc = 0;
    PF_PageHandle phLeaf;
    IX_NodeHeader leafHeader;
    char * pDataLeaf;

    //Retrieves the leaf from PF
    if( (rc = filehandle->GetThisPage(leafNum, phLeaf)) ) return rc;
    if( (rc = phLeaf.GetData(pDataLeaf)) ) return rc;
    memcpy(&leafHeader, pDataLeaf, sizeof(IX_NodeHeader));

    //Loops through the leaf to find the entry to delete
    int pos = 0;
    for(; pos<leafHeader.nbKey; pos++){
        PageNum leafPointer;
        if( (rc = getPointer(phLeaf, pos, leafPointer)) ) return rc;
        //If we find the right pointer, break
        if(leafPointer==bucketNum) break;
    }
    //If we didn't break, means we didn't find the bucket pointer
    if(pos==leafHeader.nbKey){
        return IX_ENTRYNOTFOUND;
    }
    //Else we remove it, i.e. offset the following keys & pointers
    for(int i=pos; i<leafHeader.nbKey; i++){
        int offset = fh.sizePointer+fh.keySize;
        memcpy(pDataLeaf+sizeof(IX_NodeHeader)+i*offset, pDataLeaf+sizeof(IX_NodeHeader)+(i+1)*(offset), offset);
    }

    //Updates nb of keys
    leafHeader.nbKey--;

    //If nbKey==0 we have to remove the leaf
    if(leafHeader.nbKey==0){
        //Updates prev and next pages headers
        if(leafHeader.nextPage>0){
            if( (rc = setPreviousNode(leafHeader.nextPage, leafHeader.prevPage)) ) return 0;
        }
        if(leafHeader.prevPage>0){
            if( (rc = setNextNode(leafHeader.prevPage, leafHeader.nextPage)) ) return 0;
        }
        //Unpins, deallocate and removes
        PageNum parentNode = leafHeader.parentPage;
        if( (rc = filehandle->UnpinPage(leafNum)) ) return rc;
        if( (rc = filehandle->DisposePage(leafNum)) ) return rc;
        //Removes from parent
        return DeleteEntryFromInternalNode(parentNode, leafNum);
    }
    //Else we just copy header back to memory and unpin
    memcpy(pDataLeaf, &leafHeader, sizeof(IX_NodeHeader));
    if( (rc = filehandle->MarkDirty(leafNum))
            || (rc = filehandle->UnpinPage(leafNum)) ) return rc;
    return 0;
}

//Deletes the entry in an internal node
RC IX_IndexHandle::DeleteEntryFromInternalNode(const PageNum nodeNum, const PageNum entryNum){
    RC rc = 0;
    PF_PageHandle pageHandle;
    IX_NodeHeader nodeHeader;
    char* pData;

    //Retrieves the node from PF
    if( (rc = filehandle->GetThisPage(nodeNum, pageHandle))) return rc;
    if( (rc= pageHandle.GetData(pData)) ) return rc;
    memcpy(&nodeHeader, pData, sizeof(IX_NodeHeader));

    //Retrieves the -1 pointer
    PageNum nodePointer;
    if( (rc = getPointer(pageHandle, -1, nodePointer)) ) return rc;
    if(nodePointer==entryNum){
        //if our entry is the -1 pointer
        //We offset all that is behind
        memcpy(pData+sizeof(IX_NodeHeader), pData+sizeof(IX_NodeHeader)+fh.sizePointer+fh.keySize,
               nodeHeader.nbKey*(fh.sizePointer+fh.keySize));
    }else{
        //Else we find our entry
        int pos = 0;
        for(; pos<nodeHeader.nbKey; pos++){
            if( (rc = getPointer(pageHandle, pos, nodePointer)) ) return rc;
            if(nodePointer==entryNum) break;
        }
        //If we didn't break anywhere means we couldn't find it
        if(pos==nodeHeader.nbKey){
            return IX_ENTRYNOTFOUND;
        }
        //We offset the keys and pointers after
        memcpy(pData+sizeof(IX_NodeHeader)+fh.sizePointer+pos*(fh.sizePointer+fh.keySize),
               pData+sizeof(IX_NodeHeader)+fh.sizePointer+(pos+1)*(fh.sizePointer+fh.keySize),
               (nodeHeader.nbKey-1-pos)*(fh.sizePointer+fh.keySize)
               );
    }
    //In every case we have to decrement nb of keys
    nodeHeader.nbKey--;

    //Then if nbKey==-1 (i.e. no key left and no pointer -1 left) we remove the node
    if( (nodeHeader.nbKey==-1) ){
        //Updates prev and next pages headers
        if(nodeHeader.nextPage>0){
            if( (rc = setPreviousNode(nodeHeader.nextPage, nodeHeader.prevPage)) ) return 0;
        }
        if(nodeHeader.prevPage>0){
            if( (rc = setNextNode(nodeHeader.prevPage, nodeHeader.nextPage)) ) return 0;
        }
        //Unpins, deallocate and removes
        PageNum parentNode = nodeHeader.parentPage;
        if( (rc = filehandle->UnpinPage(nodeNum)) ) return rc;
        if( (rc = filehandle->DisposePage(nodeNum)) ) return rc;
        //Removes from parent (recursive call)
        if(nodeHeader.level!=-1){
            return DeleteEntryFromInternalNode(parentNode, nodeNum);
        }else{
            //If the node was the root we have to tell there is no more root
            this->fh.rootNb = -1;
            return 0;
        }
    }
    //Else we just write header back to memory, mark dirty and unpins
    memcpy(pData, &nodeHeader, sizeof(IX_NodeHeader));
    if( (rc = filehandle->MarkDirty(nodeNum))
            || (rc = filehandle->UnpinPage(nodeNum)) ) return rc;
    return 0;
}


//Inserts a new entry to a specified node
RC IX_IndexHandle::InsertEntryToNode(const PageNum nodeNum, void *pData, const RID &rid) {
    RC rc = 0;
    //Retrieves the IX_NodeHeader of the node
    PF_PageHandle pageHandle;
    IX_NodeHeader nodeHeader;
    if( (rc = filehandle->GetThisPage(nodeNum, pageHandle)) ) return rc;
    char* pData2;
    if( (rc = pageHandle.GetData(pData2)) ) return rc;
    memcpy(&nodeHeader, pData2, sizeof(IX_NodeHeader)); //"Loads" the nodeHeader from memory
    //Acts according to the type of node
    if(nodeHeader.level==1){
        //Case leaf
        if( (rc=filehandle->UnpinPage(nodeNum)) ) return rc;
        return InsertEntryToLeafNode(nodeNum, pData, rid);
    }else{
        //Case not leaf: looks for the right children
        for(int i=0; i<nodeHeader.nbKey; i++){
            //If we find a key greater than value we have to take the pointer on its left
            if(IsKeyGreater(pData, pageHandle, i)>0){
                PageNum nb;
                if( (rc = getPointer(pageHandle, i-1, nb)) ) return rc;
                //Unpin node since we don't need it anymore
                if( (rc = filehandle->UnpinPage(nodeNum)) ) return rc;
                //Recursive call to insertEntreToNode
                return InsertEntryToNode(nb, pData, rid);
            }
        }
        //If we found nothing that means the insertion must be done with the last pointer of node
        PageNum nb;
        if( (rc = getPointer(pageHandle, nodeHeader.nbKey-1, nb)) ) return rc;
        //Unpin node since we don't need it anymore
        if( (rc = filehandle->UnpinPage(nodeNum)) ) return rc;
        //Recursive call to insertEntreToNode
        return InsertEntryToNode(nb, pData, rid);
    }
    //Should never get there
    return -1;
}

//Inserts a new entry to a leaf node
RC IX_IndexHandle::InsertEntryToLeafNode(const PageNum nodeNum, void *pData, const RID &rid) {
    RC rc = 0;
    //Retrieves the header of the leaf
    IX_NodeHeader leafHeader;
    PF_PageHandle pageHandle;
    char* pData2;
    if( (rc = filehandle->GetThisPage(nodeNum, pageHandle)) ) return rc;
    if( (rc = pageHandle.GetData(pData2)) ) return rc;
    memcpy(&leafHeader, pData2, sizeof(IX_NodeHeader));
    //First we check if the key already exists
    for(int i=0; i<leafHeader.nbKey; i++){
        if(IsKeyGreater(pData, pageHandle, i)==0){
            //Retrieves the bucket PageNum
            PageNum bucket;
            if( (rc = getPointer(pageHandle, i, bucket)) ) return rc;
            //Unpins the leaf node
            if( (rc = filehandle->UnpinPage(nodeNum)) ) return rc;
            //Inserts into the bucket
            return InsertEntryToBucket(bucket, rid);
        }
    }
    //We are here means the key doesn't exist already in the leaf
    if(leafHeader.nbKey<leafHeader.maxKeyNb){
        //Calls the method to handle the easy case
        if( (rc=filehandle->UnpinPage(nodeNum)) ) return rc;
        return InsertEntryToLeafNodeNoSplit(nodeNum, pData, rid);
    }else{
        //Calls the method to handle the split case
        if( (rc=filehandle->UnpinPage(nodeNum)) ) return rc;
        return InsertEntryToLeafNodeSplit(nodeNum, pData, rid);
    }
}

//Inserts a new entry to a leaf node without split
RC IX_IndexHandle::InsertEntryToLeafNodeNoSplit(const PageNum nodeNum, void *pData, const RID &rid){
    RC rc = 0;
    //Retrieves the page
    PF_PageHandle pageHandle;
    if( (rc = filehandle->GetThisPage(nodeNum, pageHandle)) ) return rc;
    //And the header of the leaf node
    IX_NodeHeader leafHeader;
    char *pData2;
    if( (rc = pageHandle.GetData(pData2)) ) return rc;
    memcpy(&leafHeader, pData2, sizeof(IX_NodeHeader));

    //Then we find the right position to insert
    int pos = 0;
    for(; pos<leafHeader.nbKey; pos++){
        if(IsKeyGreater(pData, pageHandle, pos)>=0) break;
    }

    //Moves the keys after pos
    char * pData3;
    for(int i=leafHeader.nbKey-1; i>=pos; i--){
        //Replaces i+1 key with i key
        if( (rc = getKey(pageHandle, i, pData3)) ) return rc;
        if( (rc = setKey(pageHandle, i+1, pData3))) return rc;
        //Replaces i+1 pointer with i pointer
        PageNum pointer;
        if( (rc = getPointer(pageHandle, i, pointer)) ) return rc;
        if( (rc = setPointer(pageHandle, i+1, pointer))) return rc;
    }

    //Sets pos key to our key
    char* pData4;
    memcpy(&pData4, &pData, fh.keySize); //Cast issue
    if( (rc = setKey(pageHandle, pos, pData4)) ) return rc;

    //Allocates a new page for a brand new bucket
    PF_PageHandle phBucket;
    if( (rc = filehandle->AllocatePage(phBucket)) ) return rc;
    PageNum bucketPageNum;
    if( (rc = phBucket.GetPageNum(bucketPageNum)) ) return rc;

    //Now we have to create a header for the bucket
    IX_BucketHeader bucketHeader;
    bucketHeader.nbRid = 1; //The RID we are inserting
    bucketHeader.nbRidMax = (PF_PAGE_SIZE-sizeof(IX_BucketHeader))/sizeof(RID);
    bucketHeader.nextBucket = -1; //No next bucket yet

    //Copy bucket header to memory
    if( (rc = phBucket.GetData(pData4)) ) return rc;
    memcpy(pData4, &bucketHeader, sizeof(IX_BucketHeader));
    //Copy the rid to the bucket page in memory
    memcpy(pData4+sizeof(IX_BucketHeader), &rid, sizeof(RID));
    //Marks dirty and unpins
    if( (rc = filehandle->MarkDirty(bucketPageNum))
            || (rc = filehandle->UnpinPage(bucketPageNum)) ) return rc;

    //Sets pos pointer to the bucket
    if( (rc = setPointer(pageHandle, pos, bucketPageNum)) ) return rc;
    //Increments nb of keys, writes back to file
    leafHeader.nbKey++;
    memcpy(pData2, &leafHeader, sizeof(IX_NodeHeader));
    if( (rc = filehandle->MarkDirty(nodeNum))
            || (rc = filehandle->UnpinPage(nodeNum))
            || (rc = filehandle->ForcePages()) ) return rc;

    return 0;
}

//Insert a new entry to a leaf node with split
RC IX_IndexHandle::InsertEntryToLeafNodeSplit(const PageNum nodeNum, void *pData, const RID &rid){
    //Variables we'll use for leaf1 (the old one) and leaf2 (the new one)
    RC rc = 0;
    PF_PageHandle phLeaf1, phLeaf2;
    IX_NodeHeader leaf1Header, leaf2Header;
    PageNum leaf2PageNum, leaf2Parent;
    char *pDataLeaf1, *pDataLeaf2;

    //Retrieves leaf1 and its header
    if( (rc = filehandle->GetThisPage(nodeNum, phLeaf1)) ) return rc;
    if( (rc = phLeaf1.GetData(pDataLeaf1)) ) return rc;
    memcpy(&leaf1Header, pDataLeaf1, sizeof(IX_NodeHeader));

    //Allocates new page for the new leaf
    if( (rc = filehandle->AllocatePage(phLeaf2)) ) return rc;
    if( (rc = phLeaf2.GetData(pDataLeaf2)) ) return rc;
    if( (rc = phLeaf2.GetPageNum(leaf2PageNum)) ) return rc;
    leaf2Header.level = 1;
    leaf2Header.maxKeyNb = fh.order*2;
    leaf2Header.nbKey = 0;

    //Now we need to put the half the keys in the first leaf and the other half in the second one...
    //We know that the number of keys in the first leaf is exactly leaf1Header.maxKeyNb
    int nbKey1 = leaf1Header.maxKeyNb/2;
    int nbKey2 = leaf1Header.maxKeyNb - nbKey1;
    //Copies the key from leaf 1 to leaf 2
    memcpy(pDataLeaf2+sizeof(IX_NodeHeader), pDataLeaf1+sizeof(IX_NodeHeader)+nbKey1*(fh.keySize+fh.sizePointer), nbKey2*(fh.keySize+fh.sizePointer));

    //Updates the headers
    leaf1Header.nbKey = nbKey1; //We don't need to "erase" the data from leaf1, nbKey update is enough
    leaf2Header.nbKey = nbKey2;
    leaf2Header.nextPage = leaf1Header.nextPage;
    if( (rc = setPreviousNode(leaf2Header.nextPage, leaf2PageNum)) ) return rc;
    leaf2Header.prevPage = nodeNum;
    leaf1Header.nextPage = leaf2PageNum;
    //And writes them back to memory
    memcpy(pDataLeaf1, &leaf1Header, sizeof(IX_NodeHeader));
    memcpy(pDataLeaf2, &leaf2Header, sizeof(IX_NodeHeader));
    //Marks dirty, unpins
    if( (rc = filehandle->MarkDirty(nodeNum))
            || (rc = filehandle->MarkDirty(leaf2PageNum))
            || (rc = filehandle->UnpinPage(nodeNum))
            || (rc = filehandle->UnpinPage(leaf2PageNum)) ) return rc;

    //We call InsertEntryToIntlNode() because the new leaf need to be pointed from above
    char * splitKey; //The split key is the first one of leaf2
    splitKey = pDataLeaf2 + sizeof(IX_NodeHeader);
    if( (rc = InsertEntryToIntlNode(leaf1Header.parentPage, leaf2PageNum, splitKey, leaf2Parent)) ) return rc;

    //Updates the parent of leaf2
    if( (rc = setParentNode(leaf2PageNum, leaf2Parent)) ) return rc;

    //Finally we insert the new value using InsertEntryToLeafNodeNoSplit()
    if(IsKeyGreater(pData, phLeaf2, 0)>0){
        //If our new key is lower than the first one of leaf2 we insert in leaf 1
        return InsertEntryToLeafNodeNoSplit(nodeNum, pData, rid);
    }else{
        //If it is higher we insert in the new leaf (leaf 2)
        return InsertEntryToLeafNodeNoSplit(leaf2PageNum, pData, rid);
    }
}

//Insert a new entry in an internal node
RC IX_IndexHandle::InsertEntryToIntlNode(
        const PageNum nodeNum, const PageNum childNodeNum, char *&splitKey, PageNum &splitNodeNum){
    RC rc = 0;

    //Gets the pageHandle and IX_NodeHeader for the internal node
    PF_PageHandle pageHandle;
    if( (rc = filehandle->GetThisPage(nodeNum, pageHandle)) ) return rc;
    IX_NodeHeader nodeHeader;
    char * pData2;
    if( (rc = pageHandle.GetData(pData2)) ) return rc;
    memcpy(&nodeHeader, pData2, sizeof(IX_NodeHeader));

    //Determines which method to use (full or not)
    if( (rc=filehandle->UnpinPage(nodeNum)) ) return rc;
    if(nodeHeader.nbKey<nodeHeader.maxKeyNb){
        return InsertEntryToIntlNodeNoSplit(nodeNum, childNodeNum, splitKey, splitNodeNum);
    }else{
        return InsertEntryToIntlNodeSplit(nodeNum, childNodeNum, splitKey, splitNodeNum);
    }
}

//Insert a new entry to an internal node without split
RC IX_IndexHandle::InsertEntryToIntlNodeNoSplit(
        const PageNum nodeNum, const PageNum childNodeNum, char *&splitKey, PageNum &splitNodeNum){
    RC rc = 0;

    //Gets the pageHandle and IX_NodeHeader for the internal node
    PF_PageHandle pageHandle;
    if( (rc = filehandle->GetThisPage(nodeNum, pageHandle)) ) return rc;
    IX_NodeHeader nodeHeader;
    char * pData2;
    if( (rc = pageHandle.GetData(pData2)) ) return rc;
    memcpy(&nodeHeader, pData2, sizeof(IX_NodeHeader));

    //Goes through the keys to find the right one
    int pos = 0; //Position of the node after the value to insert
    for(; pos<nodeHeader.nbKey; pos++) {
        if(IsKeyGreater(splitKey, pageHandle, pos)>0){
            break;
        }
    }

    //We offset pos and all the keys after
    char * pData3;
    for(int i=nodeHeader.nbKey-1; i>=pos; i--){
        //Replaces i+1 key with i key
        if( (rc = getKey(pageHandle, i, pData3)) ) return rc;
        if( (rc = setKey(pageHandle, i+1, pData3))) return rc;
        //Replaces i+1 pointer with i pointer
        PageNum pointer;
        if( (rc = getPointer(pageHandle, i, pointer)) ) return rc;
        if( (rc = setPointer(pageHandle, i+1, pointer))) return rc;
    }

    //Sets pos key to our key
    if( (rc = setKey(pageHandle, pos, splitKey)) ) return rc;
    //Sets pos pointer to the child node
    if( (rc = setPointer(pageHandle, pos, childNodeNum)) ) return rc;

    //Increments nb of keys, writes back to file
    nodeHeader.nbKey++;
    memcpy(pData2, &nodeHeader, sizeof(IX_NodeHeader));
    if( (rc = filehandle->MarkDirty(nodeNum))
            || (rc = filehandle->UnpinPage(nodeNum))
            || (rc = filehandle->ForcePages()) ) return rc;

    //splitNodeNumber is the same node since there is no split
    splitNodeNum = nodeNum;

    return 0;
}

//Insert a new entry to an internal node with split
RC IX_IndexHandle::InsertEntryToIntlNodeSplit(
        const PageNum nodeNum, const PageNum childNodeNum, char *&splitKey, PageNum &splitNodeNum){
    RC rc = 0;

    //Variables for the internal node (node1) and the new node (node2)
    PF_PageHandle phNode1, phNode2;
    char *pDataNode1, *pDataNode2;
    IX_NodeHeader node1Header, node2Header;
    int node2PageNumber, node2Parent;

    //Gets the pageHandle and IX_NodeHeader for the internal node
    if( (rc = filehandle->GetThisPage(nodeNum, phNode1)) ) return rc;
    if( (rc = phNode1.GetData(pDataNode1)) ) return rc;
    memcpy(&node1Header, pDataNode1, sizeof(IX_NodeHeader));

    /*
     *We have to handle the case in which the node is also the root
     *In this case we have to create a new root and make it the parent of our node
    */
    if(node1Header.level==-1){
        PF_PageHandle phRoot;
        IX_NodeHeader rootHeader;
        char* pDataRoot;
        PageNum rootPageNb;
        if( (rc = filehandle->AllocatePage(phRoot)) ) return rc;
        if( (rc = phRoot.GetData(pDataRoot)) ) return rc;
        if( (rc = phRoot.GetPageNum(rootPageNb)) ) return rc;
        rootHeader.level = -1;
        rootHeader.maxKeyNb = fh.order*2;
        rootHeader.nbKey = 0;
        rootHeader.nextPage = -1;
        rootHeader.prevPage = -1;
        rootHeader.parentPage = -1;
        //Write rootHeader to memory
        memcpy(pDataRoot, &rootHeader, sizeof(IX_NodeHeader));
        if( (rc = filehandle->MarkDirty(rootPageNb)) ) return rc;
        //We don't forget to set the root as root in our fileHeader
        this->fh.rootNb = rootPageNb;
        //Inserts our node as the very left pointer of the new root
        if( (rc = setPointer(phRoot, -1, nodeNum)) ) return rc;
        //Marks root as the parent of our node
        node1Header.parentPage = rootPageNb;
        node1Header.level = 0; //It's no longer a root
        //Marks the root as dirty and unpins it
        if( (rc = filehandle->MarkDirty(rootPageNb))
                || (rc = filehandle->UnpinPage(rootPageNb)) ) return rc;
    }

    //Allocates page for our new node
    if( (rc = filehandle->AllocatePage(phNode2)) ) return rc;
    if( (rc = phNode2.GetData(pDataNode2)) ) return rc;
    if( (rc = phNode2.GetPageNum(node2PageNumber)) ) return rc;
    node2Header.level = 0; //Internal node
    node2Header.maxKeyNb = fh.order*2;
    node2Header.nbKey = 0;

    //Median key
    int median = node1Header.nbKey/2; //nbKey should be the same as maxNbKey

    //Goes through keys after median and add them to node2
    /*
     *The total number of key we have is node1Header.nbKey
     *key number median will be in upper node
     *the median keys before go in node1
     *the node1Header.nbKey-median keys after go to node2
    */
    char* pData3; PageNum pointer;
    int i1=node1Header.nbKey-1, i2=node1Header.nbKey-median-1;
    bool splitKeyWritten = false;
    //We fill node2 with the right stuff
    while(i2>=0){
        if( !splitKeyWritten && IsKeyGreater(splitKey, phNode1, i1) <= 0){
            //Puts the split value in node2
            if( (rc = setKey(phNode2, i2, splitKey)) ) return rc;
            if( (rc = setPointer(phNode2, i2, childNodeNum)) ) return rc;
            i2--; splitKeyWritten=true;
            //And if the splitKey is written in node2, means the splitNode is node2
            splitNodeNum = node2PageNumber;
        }else{
            //Puts i1 keys and pointer at i2 in node2
            if( (rc = getKey(phNode1, i1, pData3)) ) return rc;
            if( (rc = setKey(phNode2, i2, pData3)) ) return rc;
            if( (rc = getPointer(phNode1, i1, pointer)) ) return rc;
            if( (rc = setPointer(phNode2, i2, pointer)) ) return rc;
            i1--; i2--;
            //We also need to change the parent in the new child of node2
            if( (rc = setParentNode(pointer, node2PageNumber)) ) return rc;
        }
    }


    //Updates headers
    node2Header.nextPage = node1Header.nextPage;
    if( (rc = setPreviousNode(node2Header.nextPage, node2PageNumber)) ) return rc;
    node2Header.prevPage = nodeNum;
    node1Header.nextPage = node2PageNumber;
    node2Header.nbKey = node1Header.nbKey-median;
    node1Header.nbKey = median;

    //Writes headers back to memory (needs to be done before any recursive call)
    memcpy(pDataNode1, &node1Header, sizeof(IX_NodeHeader));
    memcpy(pDataNode2, &node2Header, sizeof(IX_NodeHeader));


    //We add the median key above
    PageNum medianNodeNb;
    if( !splitKeyWritten && IsKeyGreater(splitKey, phNode1, i1) <= 0){
        //In this case the median pushed is the split key
        medianNodeNb = childNodeNum;
        if( (rc = InsertEntryToIntlNode(node1Header.parentPage, node2PageNumber, splitKey, node2Parent)) ) return rc;
        splitKeyWritten=true;
        //And if the splitKey is the median, means the splitNode is node2
        splitNodeNum = node2PageNumber;
    }else{
        //In this case the median pushed is next node1 key
        if( (rc = getPointer(phNode1, i1, medianNodeNb)) ) return rc;
        if( (rc = getKey(phNode1, i1, pData3)) ) return rc;
        if( (rc = InsertEntryToIntlNode(node1Header.parentPage, node2PageNumber, pData3, node2Parent)) ) return rc;
        i1--;
    }

    //Sets node2 parent
    if( (rc = setParentNode(node2PageNumber, node2Parent)) ) return rc;

    //We don't forget to set the median node as the -1 pointer in node2...
    if( (rc = setPointer(phNode2, -1, medianNodeNb))) return rc;
    //...And to set node2 as the parent of medianNodeNb
    if( (rc = setParentNode(medianNodeNb, node2PageNumber)) ) return rc;

    //Now only if splitKey has not been written so far we offset all the keys in node1 until we write it
    while(!splitKeyWritten){
        if( i1<0 || IsKeyGreater(splitKey, phNode1, i1) <= 0){
            //Puts the split value in node1 at i1+1
            if( (rc = setKey(phNode1, i1+1, splitKey)) ) return rc;
            if( (rc = setPointer(phNode1, i1+1, childNodeNum)) ) return rc;
            splitKeyWritten=true;
            //And if the splitKey is written in node1, means the splitNode is node1
            splitNodeNum = nodeNum;
        }else{
            //Moves i1 to i1+1
            if( (rc = getKey(phNode1, i1, pData3)) ) return rc;
            if( (rc = setKey(phNode1, i1+1, pData3)) ) return rc;
            if( (rc = getPointer(phNode1, i1, pointer)) ) return rc;
            if( (rc = setPointer(phNode1, i1+1, pointer)) ) return rc;
            i1--;
        }
    }

    //Marks dirty, unpins and forces...
    if( (rc = filehandle->MarkDirty(nodeNum))
            || (rc = filehandle->MarkDirty(node2PageNumber))
            || (rc = filehandle->UnpinPage(nodeNum))
            || (rc = filehandle->UnpinPage(node2PageNumber))
            || (rc = filehandle->ForcePages()) ) return rc;

    return 0;
}

//Inserts a rid in a bucket
RC IX_IndexHandle::InsertEntryToBucket(const PageNum bucketNb, const RID &rid){
    RC rc = 0;
    //Retrieves the bucket
    PF_PageHandle phBucket;
    if( (rc = filehandle->GetThisPage(bucketNb, phBucket)) ) return rc;
    char* pDataBucket;
    if( (rc = phBucket.GetData(pDataBucket)) ) return rc;

    //Loads bucket header from memory
    IX_BucketHeader bucketHeader;
    memcpy(&bucketHeader, pDataBucket, sizeof(IX_BucketHeader));

    //Checks the existing RIDs in the bucket to see if entry already exist
    for(int i=0; i<bucketHeader.nbRid; i++){
        RID bucketRid;
        memcpy(&bucketRid, pDataBucket+sizeof(IX_BucketHeader)+i*sizeof(RID), sizeof(RID));
        if(bucketRid==rid){
            return IX_ENTRYEXISTS;
        }
    }

    //If bucket is full
    if(bucketHeader.nbRid>=bucketHeader.nbRidMax){
        //If there is a next bucket, fine we insert in it
        if(bucketHeader.nextBucket!=-1){
            if( (rc = filehandle->UnpinPage(bucketNb)) ) return rc;
            return InsertEntryToBucket(bucketHeader.nextBucket, rid);
        }
        //Else we create the next bucket
        PF_PageHandle phNewBucket;
        char* pDataNewBucket;
        PageNum newBucketPageNum;
        IX_BucketHeader newBucketHeader;
        //Allocates a page for new bucket
        if( (rc = filehandle->AllocatePage(phNewBucket)) ) return rc;
        if( (rc = phNewBucket.GetData(pDataNewBucket)) ) return rc;
        if( (rc = phNewBucket.GetPageNum(newBucketPageNum)) ) return rc;
        newBucketHeader.nbRid = 0;
        newBucketHeader.nbRidMax = (PF_PAGE_SIZE-sizeof(IX_BucketHeader))/sizeof(RID)-1;
        newBucketHeader.nextBucket = -1;
        //Copies header of the new bucket to memory
        memcpy(pDataNewBucket, &newBucketHeader, sizeof(IX_BucketHeader));
        //Unpins and marks dirty
        if( (rc = filehandle->MarkDirty(newBucketPageNum)) ) return rc;
        if( (rc = filehandle->UnpinPage(newBucketPageNum)) ) return rc;
        //Also updates header and unpins, mark dirty the other bucket
        bucketHeader.nextBucket = newBucketPageNum;
        memcpy(pDataBucket, &bucketHeader, sizeof(IX_BucketHeader));
        if( (rc = filehandle->MarkDirty(bucketNb))) return rc;
        if( (rc = filehandle->UnpinPage(bucketNb))) return rc;
        //And recursive call to insert the rid
        return InsertEntryToBucket(newBucketPageNum, rid);
    }

    //Inserts the RID
    memcpy(pDataBucket+sizeof(IX_BucketHeader)+bucketHeader.nbRid*sizeof(RID), &rid, sizeof(RID));
    //Increments nb of rids and copies back header to memory
    bucketHeader.nbRid++;
    memcpy(pDataBucket, &bucketHeader, sizeof(IX_BucketHeader));

    //Marks dirty, etc.
    if( (rc = filehandle->MarkDirty(bucketNb))
            || (rc = filehandle->UnpinPage(bucketNb))
            || (rc = filehandle->ForcePages()) ) return rc;
    return 0;
}

//Compares the given value (pData) to number i key on the node (pageHandle)
int IX_IndexHandle::IsKeyGreater(void *pData, PF_PageHandle &pageHandle, int i) {
    RC rc = 0;
    char* pData2;
    rc = getKey(pageHandle, i, pData2);
    //Case integer
    if(fh.attrType==INT){
        int value1, value2; //value1 = value given, value2 = value of the ith key
        memcpy(&value1, pData, fh.keySize);
        memcpy(&value2, pData2, fh.keySize);
        if(value2>value1) return 1;
        if(value1>value2) return -1;
        if(value1==value2) return 0;
        return -1; //Shoudn't be needed
    }
    //Case float
    if(fh.attrType==FLOAT){
        float value1, value2; //value1 = value given, value2 = value of the ith key
        memcpy(&value1, pData, fh.keySize);
        memcpy(&value2, pData2, fh.keySize);
        if(value2>value1) return 1;
        if(value1>value2) return -1;
        if(value1==value2) return 0;
        return -1; //Shoudn't be needed
    }
    //Case string
    if(fh.attrType==STRING){
        char *string1, *string2;
        string1 = (char*) pData;
        string2 = (char*) pData2;
        return strncmp(string2,string1,fh.keySize);
    }
    return IX_INVALIDATTR;
}

//Gets the value of the i key of the node to pData
RC IX_IndexHandle::getKey(PF_PageHandle &pageHandle, int i, char *&pData){
    //Of course we assume the node is loaded into pageHandle already
    RC rc = 0;
    char* pData2;
    if( (rc = pageHandle.GetData(pData2)) ) return rc;
    IX_NodeHeader nodeHeader;
    memcpy(&nodeHeader, pData2, sizeof(IX_NodeHeader));
    //Case root or internal node
    if(nodeHeader.level<1){
        pData2 += sizeof(IX_NodeHeader); //Offset due to header
        pData2 += fh.sizePointer; //Offset due to pointer -1
        pData2 += i*(fh.sizePointer+fh.keySize); //Offset due to key before
        memcpy(&pData, &pData2, fh.keySize);
        return 0;
    }
    //Case leaf node
    if(nodeHeader.level==1){
        pData2 += sizeof(IX_NodeHeader); //Offset due to header
        pData2 += i*(fh.sizePointer+fh.keySize); //Offset due to key before
        memcpy(&pData, &pData2, fh.keySize);
        return 0;
    }
    return -1;
}

//Sets the value of the i key of the node to pData
RC IX_IndexHandle::setKey(PF_PageHandle &pageHandle, int i, char *pData){
    //The trick is we just use getKey to get the adress of the i key
    RC rc = 0;
    char* pData2;
    if( (rc = getKey(pageHandle, i, pData2)) ) return rc;
    memcpy(pData2, pData, fh.keySize);
    return 0;
}

//Gets the pointer number i to pageNum in the node of pageHandle
RC IX_IndexHandle::getPointer(PF_PageHandle &pageHandle, int i, PageNum &pageNum) {
    //Of course we assume the node is loaded into pageHandle already
    RC rc = 0; char* pData;
    if( (rc = pageHandle.GetData(pData)) ) return rc;
    IX_NodeHeader nodeHeader;
    memcpy(&nodeHeader, pData, sizeof(IX_NodeHeader));
    //Case root or internal node
    if(nodeHeader.level<1){
        pData += sizeof(IX_NodeHeader); //Offset due to header
        pData += fh.sizePointer; //Offset due to pointer -1
        pData += i*(fh.sizePointer+fh.keySize) + fh.keySize; //keys before & i key
        //Please note that if i=-1 it still works and pData is at the adresse of the -1 pointer
        //Now we copy it into pageNum
        memcpy(&pageNum, pData, fh.keySize);
        return 0;
    }
    //Case leaf node
    if(nodeHeader.level==1){
        pData += sizeof(IX_NodeHeader); //Offset due to header
        pData += i*(fh.sizePointer+fh.keySize); //Offset due to key before
        pData += fh.keySize; //The i key
        //Now we copy it into pageNum
        memcpy(&pageNum, pData, fh.keySize);
        return 0;
    }
    //We should never get there so we return -1
    return -1;
}

//Sets the pointer number i to pageNum in the node of pageHandle
RC IX_IndexHandle::setPointer(PF_PageHandle &pageHandle, int i, PageNum pageNum) {
    //Of course we assume the node is loaded into pageHandle already
    RC rc = 0; char* pData;
    if( (rc = pageHandle.GetData(pData)) ) return rc;
    IX_NodeHeader nodeHeader;
    memcpy(&nodeHeader, pData, sizeof(IX_NodeHeader));
    //Case root or internal node
    if(nodeHeader.level<1){
        pData += sizeof(IX_NodeHeader); //Offset due to header
        pData += fh.sizePointer; //Offset due to pointer -1
        pData += i*(fh.sizePointer+fh.keySize); //Offset due to key before
        pData += fh.keySize; //The i key
        //Please note that if i=-1 it still works and pData is at the adresse of the -1 pointer
        //Now we copy it into pageNum
        memcpy(pData, &pageNum, fh.keySize);
        return 0;
    }
    //Case leaf node
    if(nodeHeader.level==1){
        pData += sizeof(IX_NodeHeader); //Offset due to header
        pData += i*(fh.sizePointer+fh.keySize); //Offset due to key before
        pData += fh.keySize; //The i key
        //Now we copy it into pageNum
        memcpy(pData, &pageNum, fh.keySize);
        return 0;
    }
    //We should never get there so we return -1
    return -1;
}

//Sets the previous node in the header of a particular node
RC IX_IndexHandle::setPreviousNode(PageNum nodeNum, PageNum previousNode) {
    //If the page doesn't exist just returns
    if(nodeNum<0) return 0;
    //Else loads, change prevPage and writes back
    RC rc = 0;
    PF_PageHandle pageHandle;
    IX_NodeHeader nodeHeader;
    char* pData;
    if( (rc = filehandle->GetThisPage(nodeNum, pageHandle))
            || (rc = pageHandle.GetData(pData)) ) return rc;
    memcpy(&nodeHeader, pData, sizeof(IX_NodeHeader));
    nodeHeader.prevPage = previousNode;
    memcpy(pData, &nodeHeader, sizeof(IX_NodeHeader));
    if( (rc = filehandle->MarkDirty(nodeNum))
            || (rc = filehandle->UnpinPage(nodeNum)) ) return rc;
    return 0;
}

//Sets the previous node in the header of a particular node
RC IX_IndexHandle::setNextNode(PageNum nodeNum, PageNum nextNode) {
    //If the page doesn't exist just returns
    if(nodeNum<0) return 0;
    //Else loads, change nextPage and writes back
    RC rc = 0;
    PF_PageHandle pageHandle;
    IX_NodeHeader nodeHeader;
    char* pData;
    if( (rc = filehandle->GetThisPage(nodeNum, pageHandle))
            || (rc = pageHandle.GetData(pData)) ) return rc;
    memcpy(&nodeHeader, pData, sizeof(IX_NodeHeader));
    nodeHeader.nextPage = nextNode;
    memcpy(pData, &nodeHeader, sizeof(IX_NodeHeader));
    if( (rc = filehandle->MarkDirty(nodeNum))
            || (rc = filehandle->UnpinPage(nodeNum)) ) return rc;
    return 0;
}

//Sets the parent node in the header of a particular node
RC IX_IndexHandle::setParentNode(PageNum child, PageNum parent) {
    //If the page doesn't exist just returns
    if(child<0) return 0;
    //Else loads, change prevPage and writes back
    RC rc = 0;
    PF_PageHandle pageHandle;
    IX_NodeHeader childHeader;
    char* pData;
    if( (rc = filehandle->GetThisPage(child, pageHandle))
            || (rc = pageHandle.GetData(pData)) ) return rc;
    memcpy(&childHeader, pData, sizeof(IX_NodeHeader));
    childHeader.parentPage = parent;
    memcpy(pData, &childHeader, sizeof(IX_NodeHeader));
    if( (rc = filehandle->MarkDirty(child))
            || (rc = filehandle->UnpinPage(child)) ) return rc;
    return 0;
}


RC IX_IndexHandle::PrintTree(){
    RC rc = 0;
    printf("\n------------------------------\n");
    if(fh.rootNb==-1){
        printf("The tree is empty\n");
    }else{
        if((rc = PrintNode(fh.rootNb))) return rc;
    }
    printf("\n------------------------------\n");
    return 0;
}

RC IX_IndexHandle::PrintNode(PageNum nodeNb){
    printf("----\n");
    if(nodeNb==0){
        printf("Error: Requesting the node 0, which is the file header!\n");
        return 0;
    }
    PF_PageHandle ph;
    RC rc;
    char * pData;
    IX_NodeHeader nodeHeader;
    if((rc = filehandle->GetThisPage(nodeNb, ph)) ) return rc;
    if( (rc = ph.GetData(pData))) return rc;
    memcpy(&nodeHeader, pData, sizeof(IX_NodeHeader));
    printf("Node nb %d | level %d | nb keys %d\n", nodeNb, nodeHeader.level, nodeHeader.nbKey);
    printf("prev %d | next %d | parent %d\n", nodeHeader.prevPage, nodeHeader.nextPage, nodeHeader.parentPage);
    pData+=sizeof(IX_NodeHeader);
    int value;
    if(nodeHeader.level!=1){
        memcpy(&value, pData, 4); printf("%d ", value); //prints pointer -1
        pData+=4;
    }
    for(int i=0; i<nodeHeader.nbKey; i++){
        memcpy(&value, pData, 4); printf("(%d,", value); //prints key i
        pData+=4;
        memcpy(&value, pData, 4); printf("%d) ", value); //prints pointer i
        pData+=4;
    }
    printf("\n");
    if( (rc = ph.GetData(pData))) return rc;
    pData += sizeof(IX_NodeHeader);
    if(nodeHeader.level==1){
        for(int i=0; i<nodeHeader.nbKey; i++){
            //memcpy(&value, pData, 4); printf("Key%d %d ", i, value); //prints key i
            pData+=4;
            PageNum bNb;
            memcpy(&bNb, pData, 4);
            PrintBucket(bNb);//prints pointer i
            pData+=4;
        }
    }
    printf("----\n");
    //Prints all the children if internal node
    if(nodeHeader.level!=1){
        for(int i=-1; i<nodeHeader.nbKey; i++){
            PageNum childNb;
            getPointer(ph, i, childNb);
            printf("Displaying node %d\n", childNb);
            PrintNode(childNb);
        }
    }
    if( (rc=filehandle->UnpinPage(nodeNb))) return rc;
    return 0;
}

RC IX_IndexHandle::PrintBucket(PageNum bucketNb){
    PF_PageHandle ph;
    RC rc;
    char * pData;
    IX_BucketHeader bucketHeader;
    if((rc = filehandle->GetThisPage(bucketNb, ph)) ) return rc;
    if( (rc = ph.GetData(pData))) return rc;
    memcpy(&bucketHeader, pData, sizeof(IX_BucketHeader));
    printf("Bucket nb %d | nb rid %d | Next Bucket %d (max nb rid = %d)\n", bucketNb, bucketHeader.nbRid, bucketHeader.nextBucket, bucketHeader.nbRidMax);
    if( (rc=filehandle->UnpinPage(bucketNb))) return rc;
    return bucketHeader.nextBucket==-1 ? 0 : PrintBucket(bucketHeader.nextBucket);
}
