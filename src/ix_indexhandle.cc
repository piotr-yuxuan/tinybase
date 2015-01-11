#include <cstdio>
#include <unistd.h>
#include <iostream>
#include "ix.h"
#include "ix_internal.h"

using namespace std;

/*
 * Implementation sketch of a B+ tree. This structure gracefully reorganise
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
    //If the root number is not defined we create a root
    if(this->fileHeader.rootNb==-1){
        //Allocates a new page for the root
        PF_PageHandle pageHandle;
        if(this->filehandle->AllocatePage(pageHandle)) return rc;
        //Creates a header for the root
        IX_NodeHeader rootHeader;
        rootHeader.level = -1;
        rootHeader.maxKeyNb = IX_IndexHandle.Order*2;
        rootHeader.nbKey = 0;
        rootHeader.prevPage = -1;
        rootHeader.nextPage = -1;
        rootHeader.parentPage = -1;
        //Copy the header of the root to memory
        char* pData2;
        if(rc = pageHandle.GetData(pData2)) return rc;
        memcpy(pData2, &rootHeader, sizeof(IX_NodeHeader));
        //Updates the root page number in file header
        PageNum nb;
        if(rc = pageHandle.GetPageNum(nb)) return rc;
        this->fileHeader.rootNb = nb;
        //Marks dirty and unpins
        if( (rc = filehandle->MarkDirty(nb)) || (rc = filehandle->UnpinPage(nb)) ) return rc;
    }
    //The root is either created or exists, we can insert
    return InsertEntryToNode(this->fileHeader.rootNb, pData, rid);
}

//Deletes an entry
RC IX_IndexHandle::DeleteEntry(void *pData, const RID &rid) {
    //TODO
    return -1;
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
        return InsertEntryToLeafNode(nodeNum, pData, rid);
    }else{
        //Case not leaf: looks for the right children
        for(int i=0; i<nodeHeader.nbKey; i++){
            //If we find a key greater than value we have to take the pointer on its left
            if(IsKeyGreater(pData, pageHandle, i)>=0){
                PageNum nb;
                if( (rc = getPointer(pageHandle, i, nb)) ) return rc;
                //Unpin node since we don't need it anymore
                if( (rc = filehandle->UnpinPage(nodeNum)) ) return rc;
                //Reccursive call to insertEntreToNode
                InsertEntryToNode(nb, pData, rid);
            }
        }
    }
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
            //Retrieves the bucket
            PageNum bucket;
            if( (rc = getPointer(pageHandle, i, bucket)) ) return rc;
            PF_PageHandle phBucket;
            if( (rc = filehandle->GetThisPage(bucket, phBucket)) ) return rc;
            char* pDataBucket;
            if( (rc = phBucket.GetData(pDataBucket)) ) return rc;
            //Loads bucket header from memory
            IX_BucketHeader bucketHeader;
            memcpy(&bucketHeader, pDataBucket, sizeof(IX_BucketHeader));
            if(bucketHeader.nbRid>=bucketHeader.nbRidMax){
                return IX_FULLBUCKET;
            }
            //Inserts the RID
            memcpy(pDataBucket+sizeof(IX_BucketHeader)+bucketHeader.nbRid*sizeof(RID), &rid, sizeof(RID));
            //Increments nb of rids and copies back header to memory
            bucketHeader.nbRid++;
            memcpy(pDataBucket, &bucketHeader, sizeof(IX_BucketHeader));
            //Marks dirty, etc.
            if( (rc = filehandle->MarkDirty(bucket))
                    || (rc = filehandle->UnpinPage(bucket))
                    || (rc = filehandle->ForcePages()) ) return rc;
            //Data is inserted it's done
            return 0;
        }
    }
    //We are here means the key doesn't exist already in the leaf
    if(leafHeader.nbKey<leafHeader.maxKeyNb){
        //Calls the method to handle the easy case
        return InsertEntryToLeafNodeNoSplit(nodeNum, pData, rid);
    }else{
        //Calls the method to handle the split case
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
    free(pData3);

    //Sets pos key to our key
    if( (rc = setKey(pageHandle, pos, pData)) ) return rc;

    //Allocates a new page for a brand new bucket
    PF_PageHandle bucketHandler;
    if( (rc = filehandle->AllocatePage(bucketHandler)) ) return rc;
    PageNum bucketPointer;
    if( (rc = bucketHandler.GetPageNum(bucketPointer)) ) return rc;

    //Now we have to create a header for the bucket
    IX_BucketHeader bucketHeader;
    bucketHeader.nbRid = 1; //The RID we are inserting
    bucketHeader.nbRidMax = (PF_PAGE_SIZE-sizeof(bucketHeader))/sizeof(RID);

    //Copy bucket header to memory
    char* pData4;
    if( (rc = bucketHandler.GetData(pData4)) ) return rc;
    memcpy(pData4, &bucketHeader, sizeof(IX_BucketHeader));
    //Copy the rid to the bucket page in memory
    memcpy(pData4+sizeof(IX_BucketHeader), &rid, sizeof(RID));
    //Marks dirty and unpins
    if( (rc = filehandle->MarkDirty(bucketPointer))
            || (rc = filehandle->UnpinPage(bucketPointer)) ) return rc;

    //Sets pos pointer to the bucket
    if( (rc = setPointer(pageHandle, pos, bucketPointer)) ) return rc;
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
    char* pDataLeaf1, pDataLeaf2;

    //Retrieves leaf1 and its header
    if( (rc = filehandle->GetThisPage(nodeNum, phLeaf1)) ) return rc;
    if( (rc = phLeaf1.GetData(pDataLeaf1)) ) return rc;
    memcpy(&leaf1Header, pDataLeaf1, sizeof(IX_NodeHeader));

    //Allocates new page for the new leaf
    if( (rc = filehandle->AllocatePage(phLeaf2)) ) return rc;
    if( (rc = phLeaf2.GetData(pDataLeaf2)) ) return rc;
    if( (rc = phLeaf2.GetPageNum(leaf2PageNum)) ) return rc;
    leaf2Header.level = 1;
    leaf2Header.maxKeyNb = IX_IndexHandle.Order*2;
    leaf2Header.nbKey = 0;

    //Now we need to put the half the keys in the first leaf and the other half in the second one...
    //We know that the number of keys in the first leaf is exactly leaf1Header.maxKeyNb
    int nbKey1 = leaf1Header.maxKeyNb/2;
    int nbKey2 = leaf1Header.maxKeyNb - nbKey1;
    //Copies the key from leaf 1 to leaf 2
    memcpy(pDataLeaf2+sizeof(IX_NodeHeader), pDataLeaf1+sizeof(IX_NodeHeader)+nbKey1*(fileHeader.keySize+SizePointer), nbKey1*(fileHeader.keySize+SizePointer));

    //We call InsertEntryToIntlNode() because the new leaf need to be pointed from above
    char * splitKey; //The split key is the first one of leaf2
    splitKey = pDataLeaf2 + sizeof(IX_NodeHeader);
    if( (rc = InsertEntryToIntlNode(leaf1Header.parentPage, leaf2PageNum, splitKey, leaf2Parent)) ) return rc;

    //Updates the headers
    leaf1Header.nbKey = nbKey1; //We don't need to "erase" the data from leaf1, nbKey update is enough
    leaf2Header.nbKey = nbKey2;
    leaf2Header.nextPage = leaf1Header.nextPage;
    if( (rc = setPreviousNode(leaf2Header.nextPage, leaf2PageNum)) ) return rc;
    leaf2Header.prevPage = nodeNum;
    leaf2Header.parentPage = leaf2Parent;
    leaf1Header.nextPage = leaf2PageNum;
    //And writes them back to memory
    memcpy(pDataLeaf1, &leaf1Header, sizeof(IX_NodeHeader));
    memcpy(pDataLeaf2, &leaf2Header, sizeof(IX_NodeHeader));
    //Marks dirty, unpins
    if( (rc = filehandle->MarkDirty(nodeNum))
            || (rc = filehandle->MarkDirty(leaf2PageNum))
            || (rc = filehandle->UnpinPage(nodeNum))
            || (rc = filehandle->UnpinPage(leaf2PageNum)) ) return rc;

    //Finally we insert the new value using InsertEntryToLeafNodeNoSplit()
    if(IsKeyGreater(pData, phLeaf2, 0)<0){
        //If our new key is lower then the first one of leaf2 we insert in leaf 1
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
    for(int i=0; i<nodeHeader.nbKey; i++) {
        if(IsKeyGreater(splitKey, pageHandle, i)>0){
            pos = i;
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
    free(pData3);

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
    char * pDataNode1, pDataNode2;
    IX_NodeHeader node1Header, node2Header;
    int node2PageNumber, node2Parent;

    //Gets the pageHandle and IX_NodeHeader for the internal node
    if( (rc = filehandle->GetThisPage(nodeNum, phNode1)) ) return rc;
    if( (rc = phNode1.GetData(pDataNode1)) ) return rc;
    memcpy(&node1Header, pDataNode1, sizeof(IX_NodeHeader));

    //Allocates page for our new node
    if( (rc = filehandle->AllocatePage(phNode2)) ) return rc;
    if( (rc = phNode2.GetData(pDataNode2)) ) return rc;
    if( (rc = phNode2.GetPageNum(node2PageNumber)) ) return rc;
    node2Header.level = 0; //Internal node
    node2Header.maxKeyNb = IX_IndexHandle.Order*2;
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
        }else{
            //Puts i1 keys and pointer at i2 in node2
            if( (rc = getKey(phNode1, i1, pData3)) ) return rc;
            if( (rc = setKey(phNode2, i2, pData3)) ) return rc;
            if( (rc = getPointer(phNode1, i1, pointer)))
            if( (rc = setPointer(phNode2, i2, pointer)) ) return rc;
            i1--; i2--;
        }
    }
    //We add the median key above
    if( !splitKeyWritten && IsKeyGreater(splitKey, phNode1, i1) <= 0){
        //In this case the median pushed is the split key
        if( (rc = InsertEntryToIntlNode(node1Header.parentPage, node2PageNumber, splitKey, node2Parent)) ) return rc;
        splitKeyWritten=true;
    }else{
        //In this case the median pushed is next node1 key
        if( (rc = getKey(phNode1, i1, pData3)) ) return rc;
        if( (rc = InsertEntryToIntlNode(node1Header.parentPage, node2PageNumber, pData3, node2Parent)) ) return rc;
        i1--;
    }
    //Now only if splitKey has not been written so far we offset all the keys in node1 until we write it
    while(!splitKeyWritten){
        if( i1<0 || IsKeyGreater(splitKey, phNode1, i1) <= 0){
            //Puts the split value in node1 at i1+1
            if( (rc = setKey(phNode1, i1+1, splitKey)) ) return rc;
            if( (rc = setPointer(phNode1, i1+1, childNodeNum)) ) return rc;
            splitKeyWritten=true;
        }else{
            //Moves i1 to i1+1
            if( (rc = getKey(phNode1, i1, pData3)) ) return rc;
            if( (rc = setKey(phNode1, i1+1, pData3)) ) return rc;
            if( (rc = getPointer(phNode1, i1, pointer)))
            if( (rc = setPointer(phNode1, i1+1, pointer)) ) return rc;
            i1--;
        }
    }
    free(pData3);

    //Updates headers
    node2Header.nextPage = node1Header.nextPage;
    if( (rc = setPreviousNode(node2Header.nextPage, node2PageNumber)) ) return rc;
    node2Header.prevPage = nodeNum;
    node2Header.parentPage = node2Parent;
    node1Header.nextPage = node2PageNumber;
    node2Header.nbKey = node1Header.nbKey-median;
    node1Header.nbKey = median;

    //Writes headers back to memory
    memcpy(pDataNode1, &node1Header, sizeof(IX_NodeHeader));
    memcpy(pDataNode2, &node2Header, sizeof(IX_NodeHeader));

    //Marks dirty, unpins and forces...
    if( (rc = filehandle->MarkDirty(nodeNum))
            || (rc = filehandle->MarkDirty(node2PageNumber))
            || (rc = filehandle->UnpinPage(nodeNum))
            || (rc = filehandle->UnpinPage(node2PageNumber))
            || (rc = filehandle->ForcePages()) ) return rc;

    return 0;
}

//Compares the given value (pData) to number i key on the node (pageHandle)
int IX_IndexHandle::IsKeyGreater(void *pData, PF_PageHandle pageHandle, int i) {
    RC rc = 0;
    char* pData2;
    if( (rc = getKey(pageHandle, i, pData2)) ) return rc;
    //Case integer or float
    if(fileHeader.attrType==INT || fileHeader.attrType==FLOAT){
        int value1, value2; //value1 = value given, value2 = value of the ith key
        memcpy(value1, pData, fileHeader.keySize);
        memcpy(value2, pData2, fileHeader.keySize);
        free(pData2);
        if(value2>value1) return 1;
        if(value1>value2) return -1;
        if(value1==value2) return 0;
        return -1; //Shoudn't be needed
    }
    //Case string
    if(fileHeader.attrType==FLOAT){
        char *string1, *string2;
        string1 = (char*) pData1;
        string2 = (char*) pData2;
        return strncmp(string2,string1,fileHeader.keySize);
    }
    return IX_INVALIDATTR;
}

//Gets the value of the i key of the node to pData
RC IX_IndexHandle::getKey(PF_PageHandle &pageHandle, int i, void *pData){
    //Of course we assume the node is loaded into pageHandle already
    RC rc = 0;
    if( (rc = pageHandle.GetData(pData)) ) return rc;
    IX_NodeHeader nodeHeader;
    memcpy(&nodeHeader, pData, sizeof(IX_NodeHeader));
    //Case root or internal node
    if(nodeHeader.level<1){
        pData += sizeof(IX_NodeHeader); //Offset due to header
        pData += SizePointer; //Offset due to pointer -1
        pData += i*(SizePointer+fileHeader.keySize); //Offset due to key before
        return 0;
    }
    //Case leaf node
    if(nodeHeader.level==1){
        pData += sizeof(IX_NodeHeader); //Offset due to header
        pData += i*(SizePointer+fileHeader.keySize); //Offset due to key before
        return 0;
    }
    //We should never get there so we return -1
    return -1;
}

//Sets the value of the i key of the node to pData
RC IX_IndexHandle::setKey(PF_PageHandle &pageHandle, int i, void *pData){
    //The trick is we just use getKey to get the adress of the i key
    RC rc = 0;
    char* pData2;
    if( (rc = getKey(pageHandle, i, pData2)) ) return rc;
    memcpy(pData2, pData, fileHeader.keySize);
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
        pData += SizePointer; //Offset due to pointer -1
        pData += i*(SizePointer+fileHeader.keySize); //Offset due to key before
        pData += fileHeader.keySize; //The i key
        //Please note that if i=-1 it still works and pData is at the adresse of the -1 pointer
        //Now we copy it into pageNum
        memcpy(&pageNum, pData, fileHeader.keySize);
        return 0;
    }
    //Case leaf node
    if(nodeHeader.level==1){
        pData += sizeof(IX_NodeHeader); //Offset due to header
        pData += i*(SizePointer+fileHeader.keySize); //Offset due to key before
        pData += fileHeader.keySize; //The i key
        //Now we copy it into pageNum
        memcpy(&pageNum, pData, fileHeader.keySize);
        return 0;
    }
    //We should never get there so we return -1
    return -1;
}

//Sets the pointer number i to pageNum in the node of pageHandle
RC IX_IndexHandle::setPointer(PF_PageHandle &pageHandle, int i, PageNum pageNum) {
    //The trick is we just use getPointer to get the adress of the i pointer
    //Of course it also works if i=-1 for root and internal nodes
    RC rc = 0;
    char* pData2;
    if( (rc = getPointer(pageHandle, i, pData2)) ) return rc;
    memcpy(pData2, pData, SizePointer);
    return 0;
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
            || (rc = filehandle->UnpinPage(nodeNum))
            || (rc = filehandle->ForcePages()) ) return rc;
    return 0;
}
