//
// ix.h
//
//   Index Manager Component Interface
//

#ifndef IX_H
#define IX_H

// Please do not include any other files than the ones below in this file.

#include "tinybase.h"  // Please don't change these lines
#include "rm_rid.h"  // Please don't change these lines
#include "pf.h"

// Structure for the file header
typedef struct IX_FileHeader {
    PageNum rootNb;
    AttrType attrType;
    int keySize;
    int order; //The order of the tree, there will be up to 2*order keys per node
    int sizePointer;
} IX_FileHeader;

// Structure for a node header
typedef struct IX_NodeHeader {
    int level; // -1 root, 0 middle node, 1 leaf node
    int maxKeyNb;  //Max key number in the node (means max pointer number is maxKeyNb+1)
    int nbKey;//Number of key in the node (means there are nbKey+1 pointers)
    PageNum parentPage;
    PageNum prevPage;
    PageNum nextPage;

} IX_NodeHeader;

// Structure for a bucket header
typedef struct IX_BucketHeader {
    int nbRid; //Number of RID stored in the bucket
    int nbRidMax; //Max number of RID storable
    int nextBucket; //PageNum of the next bucket, -1 if no next bucket
} IX_BucketHeader;

//
// IX_IndexHandle: IX Index File interface
//
class IX_IndexHandle {
    friend class IX_Manager;
    friend class IX_IndexScan;
public:
    IX_IndexHandle();
    ~IX_IndexHandle();

    // Insert a new index entry
    RC InsertEntry(void *pData, const RID &rid);

    // Delete a new index entry
    RC DeleteEntry(void *pData, const RID &rid);

    // Force index files to disk
    RC ForcePages();

    // Debugging method
    RC PrintTree();
    RC PrintNode(PageNum nodeNb);
    RC PrintBucket(PageNum bucketNb);

private:
    bool bFileOpen;
    PF_FileHandle *filehandle;
    IX_FileHeader fh; //Header for the file of the index

    //Private insertion methods
    RC InsertEntryToNode(const PageNum nodeNum, void *pData, const RID &rid);
    RC InsertEntryToLeafNode(const PageNum nodeNum, void *pData, const RID &rid);
    RC InsertEntryToLeafNodeNoSplit(const PageNum nodeNum, void *pData, const RID &rid);
    RC InsertEntryToLeafNodeSplit(const PageNum nodeNum, void *pData, const RID &rid);
    RC InsertEntryToIntlNode(const PageNum nodeNum, const PageNum childNodeNum, char *&splitKey, PageNum &splitNodeNum);
    RC InsertEntryToIntlNodeNoSplit(const PageNum nodeNum, const PageNum childNodeNum, char *&splitKey,PageNum &splitNodeNum);
    RC InsertEntryToIntlNodeSplit(const PageNum nodeNum, const PageNum childNodeNum, char *&splitKey,PageNum &splitNodeNum);
    RC InsertEntryToBucket(const PageNum bucketNb, const RID &rid);

    //Private deletion methods
    RC DeleteEntryFromBucket(const PageNum bucketNum, const RID &rid, const PageNum bucketParent, bool nextBucket);
    RC DeleteBucketEntryFromLeafNode(const PageNum leafNum, const PageNum bucketNum);
    RC DeleteEntryFromInternalNode(const PageNum nodeNum, const PageNum entryNum);

    //Returns true if key number i greater than pData value
    int IsKeyGreater(void *pData, PF_PageHandle &pageHandle, int i);
    //For the keys in a given node (don't work for Bucket)
    RC getKey(PF_PageHandle &pageHandle, int i, char *&pData);
    RC setKey(PF_PageHandle &pageHandle, int i, char *pData);
    RC getPointer(PF_PageHandle &pageHandle, int i, PageNum &pageNum);
    RC setPointer(PF_PageHandle &pageHandle, int i, PageNum pageNum);
    //Sets the previous node of a particular node
    RC setPreviousNode(PageNum nodeNum, PageNum previousNode);
    RC setNextNode(PageNum nodeNum, PageNum nextNode);
    RC setParentNode(PageNum child, PageNum parent);
};

//
// IX_IndexScan: condition-based scan of index entries
//
class IX_IndexScan {
public:
    IX_IndexScan();
    ~IX_IndexScan();

    // Open index scan
    RC OpenScan(const IX_IndexHandle &indexHandle, CompOp compOp, void *value,
            ClientHint pinHint = NO_HINT);

    // Get the next matching entry return IX_EOF if no more matching
    // entries.
    RC GetNextEntry(RID &rid);

    // Close index scan
    RC CloseScan();
private:
    const int EndOfBucket = -2;
    int bScanOpen;
    IX_IndexHandle *indexHandle;
    PageNum currentLeaf; //Current leaf for the scan
    void* currentKey; //Current key in the leaf
    PageNum currentBucket; //Current bucket we are scaning
    int currentBucketPos; //Current position in the bucket
    CompOp compOp;
    void *value;
    bool bIsEOF; //If true next GetNextEntry call will return IX_EOF
    //Method to go to the first leaf, first bucket
    RC goToFirstBucket(RID &rid);
    //Method that updates the value of currentKeyNb using currentKey
    RC getCurrentKeyNb(PF_PageHandle phLeaf, int &currentKeyNb);
    //The opposite
    RC saveCurrentKey(PF_PageHandle phLeaf, const int &currentKeyNb);
};

//
// IX_Manager: provides IX index file management
//
class IX_Manager {
public:
    IX_Manager(PF_Manager &pfm);
    ~IX_Manager();

    // Create a new Index
    RC CreateIndex(const char *fileName, int indexNo, AttrType attrType,
            int attrLength);

    // Destroy and Index
    RC DestroyIndex(const char *fileName, int indexNo);

    // Open an Index
    RC OpenIndex(const char *fileName, int indexNo,
            IX_IndexHandle &indexHandle);

    // Close an Index
    RC CloseIndex(IX_IndexHandle &indexHandle);

private:
    PF_Manager *pfManager;
};


//
// Print-error function
//
void IX_PrintError(RC rc);

#define IX_INVALIDINDEXNO  (START_IX_WARN + 0) // invalid index number
#define IX_INVALIDCOMPOP   (START_IX_WARN + 1) // invalid comparison operator
#define IX_INVALIDATTR     (START_IX_WARN + 2) // invalid attribute parameters
#define IX_NULLPOINTER     (START_IX_WARN + 3) // pointer is null
#define IX_SCANOPEN        (START_IX_WARN + 4) // scan is open
#define IX_CLOSEDSCAN      (START_IX_WARN + 5) // scan is closed
#define IX_CLOSEDFILE      (START_IX_WARN + 6) // file handle is closed
#define IX_ENTRYNOTFOUND   (START_IX_WARN + 7) // entry not found
#define IX_ENTRYEXISTS     (START_IX_WARN + 8) // entry already exists
#define IX_EOF             (START_IX_WARN + 9) // end of file
#define IX_LASTWARN        IX_EOF

#define IX_NOMEM           (START_IX_ERR - 0)  // no memory
#define IX_LASTERROR       IX_NOMEM

#endif
