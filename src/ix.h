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
} IX_FileHeader;

// Structure for a node header
typedef struct IX_NodeHeader {
    int level; // -1 root, 0 middle node, 1 leaf node
    int maxKeyNb;  //Max key number in the node (means max pointer number is maxKeyNb+1)
    int nbKey;//Number of key in the node (means there are nbKey+1 pointers)
    PageNum pageMere;
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
	static int Order = 5;
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

    //Private insertion methods
    RC InsertEntryToNode(const PageNum nodeNum, void *pData, const RID &rid, char *&, PageNum &);
    RC InsertEntryToLeafNode(const PageNum nodeNum, void *pData, const RID &rid, char *&, PageNum &, int = TRUE);
    RC InsertEntryToLeafNodeNoSplit(const PageNum nodeNum, void *pData, const RID &rid, char *&, PageNum &);
    RC InsertEntryToLeafNodeSplit(const PageNum nodeNum, void *pData, const RID &rid, char *&, PageNum &);
    RC InsertEntryToIntlNode(const PageNum nodeNum, const PageNum childNodeNum, char *&splitKey, PageNum &splitNodeNum);
    RC InsertEntryToIntlNodeNoSplit(const PageNum nodeNum, const PageNum childNodeNum, char *&splitKey,PageNum &splitNodeNum);
    RC InsertEntryToIntlNodeSplit(const PageNum nodeNum, const PageNum childNodeNum, char *&splitKey,PageNum &splitNodeNum);
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
    IX_IndexHandle *pIndexHandle;
    PageNum currentNode; //Current node we are scaning
    int nodePos; //Current position in the node
    int bucketPos; //Current position in the bucket
    CompOp compOp;
    void *value;
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
};

//
// Print-error function
//
void IX_PrintError(RC rc) {
	switch (rc) {
	case 0:
		break;
	default:
		break;
	}
}

#endif
