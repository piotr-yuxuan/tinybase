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
} IX_BucketHeader;

//
// IX_IndexHandle: IX Index File interface
//
class IX_IndexHandle {
    friend class IX_Manager;
    friend class IX_IndexScan;
public:
    static int Order;
    static int SizePointer;
    IX_IndexHandle();
    ~IX_IndexHandle();

    // Insert a new index entry
    RC InsertEntry(void *pData, const RID &rid);

    // Delete a new index entry
    RC DeleteEntry(void *pData, const RID &rid);

    // Force index files to disk
    RC ForcePages();

private:
    bool bFileOpen;
    PF_FileHandle *filehandle;
    IX_FileHeader fileHeader; //Header for the file of the index

    //Private insertion methods
    RC InsertEntryToNode(const PageNum nodeNum, void *pData, const RID &rid);
    RC InsertEntryToLeafNode(const PageNum nodeNum, void *pData, const RID &rid);
    RC InsertEntryToLeafNodeNoSplit(const PageNum nodeNum, void *pData, const RID &rid);
    RC InsertEntryToLeafNodeSplit(const PageNum nodeNum, void *pData, const RID &rid);
    RC InsertEntryToIntlNode(const PageNum nodeNum, const PageNum childNodeNum, char *&splitKey, PageNum &splitNodeNum);
    RC InsertEntryToIntlNodeNoSplit(const PageNum nodeNum, const PageNum childNodeNum, char *&splitKey,PageNum &splitNodeNum);
    RC InsertEntryToIntlNodeSplit(const PageNum nodeNum, const PageNum childNodeNum, char *&splitKey,PageNum &splitNodeNum);

    //Returns true if key number i greater than pData value
    int IsKeyGreater(void *pData, PF_PageHandle pageHandle, int i);
    //For the keys in a given node (don't work for Bucket)
    RC getKey(PF_PageHandle &pageHandle, int i, void *pData);
    RC setKey(PF_PageHandle &pageHandle, int i, void *pData);
    RC getPointer(PF_PageHandle &pageHandle, int i, PageNum &pageNum);
    RC setPointer(PF_PageHandle &pageHandle, int i, PageNum pageNum);
    //Sets the previous node of a particular node
    RC setPreviousNode(PageNum nodeNum, PageNum previousNode);
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
    int bScanOpen;
    IX_IndexHandle *indexHandle;
    PageNum currentLeaf; //Current leaf for the scan
    int currentKey; //Current key in the leaf
    PageNum currentBucket; //Current bucket we are scaning
    int currentBucketPos; //Current position in the bucket
    CompOp compOp;
    void *value;
    //Method to go to the first leaf, first bucket
    RC goToFirstBucket(RID &rid);
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

#define IX_INVIABLERID     (START_IX_WARN + 0) // inviable rid
#define IX_UNREADRECORD    (START_IX_WARN + 1) // unread record
#define IX_INVALIDRECSIZE  (START_IX_WARN + 2) // invalid record size
#define IX_EMPTYNODE       (START_IX_WARN + 3) // node is empty
#define IX_FULLBUCKET      (START_IX_WARN + 4) // the bucket is full
#define IX_INVALIDCOMPOP   (START_IX_WARN + 5) // invalid comparison operator
#define IX_INVALIDATTR     (START_IX_WARN + 6) // invalid attribute parameters
#define IX_NULLPOINTER     (START_IX_WARN + 7) // pointer is null
#define IX_SCANOPEN        (START_IX_WARN + 8) // scan is open
#define IX_CLOSEDSCAN      (START_IX_WARN + 9) // scan is closed
#define IX_CLOSEDFILE      (START_IX_WARN + 10)// file handle is closed
#define IX_FILEOPEN        (START_IX_WARN + 11)// file handle is open
#define IX_LASTWARN        IX_FILEOPEN

#define IX_EOF             PF_EOF              // work-around for ix_test

#endif
