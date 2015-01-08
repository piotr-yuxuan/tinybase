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

//Inserts a new entry to a specified node
RC IX_IndexHandle::InsertEntryToNode(const PageNum nodeNum, void *pData, const RID &rid) {
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
    }else
        //Case not leaf: look sfor the right children
        for(int i=0; i<nodeHeader.nbKey; i++){
        }
    }
}






