//
// ix.h
//
//   Index Manager Component Interface
//

#ifndef IX_H
#define IX_H

// Please do not include any other files than the ones below in this file.

#include "redbase.h"  // Please don't change these lines
#include "rm_rid.h"  // Please don't change these lines
#include "pf.h"

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

#ifdef DEBUG_IX
    RC PrintNode(const PageNum);
    RC VerifyOrder(PageNum = 0);
    RC VerifyStructure(const PageNum = 0);
    RC GetSmallestKey(const PageNum, char *&);
#endif

private:
    // Copy constructor
    IX_IndexHandle(const IX_IndexHandle &indexHandle);
    // Overloaded =
    IX_IndexHandle& operator=(const IX_IndexHandle &indexHandle);

    inline int InternalEntrySize(void);
    inline char* InternalKey(char *, int);
    inline char* InternalPtr(char *, int);
    inline int LeafEntrySize(void);
    inline char* LeafKey(char *, int);
    inline char* LeafRID(char *, int);
    float Compare(void *, char *);

    RC InsertEntryToNode(const PageNum, void *, const RID &,
                         char *&, PageNum &);
    RC InsertEntryToLeafNode(const PageNum, void *, const RID &,
                             char *&, PageNum &, int = TRUE);
    RC InsertEntryToLeafNodeSplit(const PageNum, void *, const RID &,
                                  char *&, PageNum &);
    RC InsertEntryToLeafNodeNoSplit(const PageNum, void *, const RID &,
                                    char *&, PageNum &);
    RC InsertEntryToIntlNode(const PageNum, const PageNum,
                             char *&, PageNum &);
    RC InsertEntryToIntlNodeSplit(const PageNum, const PageNum,
                                  char *&, PageNum &);
    RC InsertEntryToIntlNodeNoSplit(const PageNum, const PageNum,
                                    char *&, PageNum &);

    RC DeleteEntryAtNode(const PageNum, void *, const RID &,
                         char *&, PageNum &);
    RC DeleteEntryAtLeafNode(const PageNum, void *, const RID &,
                             char *&, PageNum &);
    RC DeleteEntryAtIntlNode(const PageNum, char *&, PageNum &);
    RC LinkTwoNodesEachOther(const PageNum, const PageNum);
    RC FindNewRootNode(const PageNum, PageNum &, char *&);

    PF_FileHandle pfFileHandle;
    AttrType attrType;
    int attrLength;
};

//
// IX_IndexScan: condition-based scan of index entries
//
class IX_IndexScan {
public:
    IX_IndexScan();
    ~IX_IndexScan();

    // Open index scan
    RC OpenScan(const IX_IndexHandle &indexHandle,
#ifdef IX_DEFAULT_COMPOP
                void *value,
                CompOp compOp = EQ_OP,
#else
                CompOp compOp,
                void *value,
#endif
                ClientHint  pinHint = NO_HINT);

    // Get the next matching entry return IX_EOF if no more matching
    // entries.
    RC GetNextEntry(RID &rid);

    // Close index scan
    RC CloseScan();

private:
    // Copy constructor
    IX_IndexScan(const IX_IndexScan &fileScan);
    // Overloaded =
    IX_IndexScan& operator=(const IX_IndexScan &fileScan);

    inline int InternalEntrySize(void);
    inline char* InternalKey(char *, int);
    inline char* InternalPtr(char *, int);
    inline int LeafEntrySize(void);
    inline char* LeafKey(char *, int);
    inline char* LeafRID(char *, int);
    float Compare(void *, char *);
    RC FindEntryAtNode(PageNum);

    int bScanOpen;
    PageNum curNodeNum;
    int curEntry;
    RID lastRid;

    IX_IndexHandle *pIndexHandle;
    CompOp compOp;
    void *value;
    ClientHint pinHint;
};

//
// IX_Manager: provides IX index file management
//
class IX_Manager {
public:
    IX_Manager(PF_Manager &pfm);
    ~IX_Manager();

    // Create a new Index
    RC CreateIndex(const char *fileName, int indexNo,
                   AttrType attrType, int attrLength);

    // Destroy and Index
    RC DestroyIndex(const char *fileName, int indexNo);

    // Open an Index
    RC OpenIndex(const char *fileName, int indexNo,
                 IX_IndexHandle &indexHandle);

    // Close an Index
    RC CloseIndex(IX_IndexHandle &indexHandle);

private:
    // Copy constructor
    IX_Manager(const IX_Manager &manager);
    // Overloaded =
    IX_Manager& operator=(const IX_Manager &manager);

    PF_Manager *pPfm;
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
