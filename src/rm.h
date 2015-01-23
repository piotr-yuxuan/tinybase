//
// rm.h
//
//   Record Manager component interface
//
// This file does not include the interface for the RID class.  This is
// found in rm_rid.h
//

#ifndef RM_H
#define RM_H

// Please DO NOT include any files other than redbase.h and pf.h in this
// file.  When you submit your code, the test program will be compiled
// with your rm.h and your redbase.h, along with the standard pf.h that
// was given to you.  Your rm.h, your redbase.h, and the standard pf.h
// should therefore be self-contained (i.e., should not depend upon
// declarations in any other file).

// Do not change the following includes
#include "redbase.h"
#include "rm_rid.h"
#include "pf.h"

//
// RM_Record: RM Record interface
//
class RM_Record {
    friend class RM_FileHandle;
    friend class RM_FileScan;
public:
    RM_Record ();
    ~RM_Record();

    // Return the data corresponding to the record.  Sets *pData to the
    // record contents.
    RC GetData(char *&pData) const;

    // Return the RID associated with the record
    RC GetRid (RID &rid) const;

private:
    // Copy constructor
    RM_Record  (const RM_Record &record);
    // Overloaded =
    RM_Record& operator=(const RM_Record &record);

    char *pData;
    int  recordSize;
    RID  rid;
};

//
// RM_FileHdr: Header structure for files
//
struct RM_FileHdr {
    PageNum firstFree;     // first free page in the linked list
    int recordSize;        // fixed record size
    int numRecordsPerPage; // # of records in each page
    int pageHeaderSize;    // page header size
    int numRecords;        // # of pages in the file
};

//
// RM_FileHandle: RM File interface
//
class RM_FileHandle {
    friend class RM_Manager;
    friend class RM_FileScan;
public:
    RM_FileHandle ();
    ~RM_FileHandle();

    // Given a RID, return the record
    RC GetRec     (const RID &rid, RM_Record &rec) const;

    RC InsertRec  (const char *pData, RID &rid);       // Insert a new record

    RC DeleteRec  (const RID &rid);                    // Delete a record
    RC UpdateRec  (const RM_Record &rec);              // Update a record

    // Forces a page (along with any contents stored in this class)
    // from the buffer pool to disk.  Default value forces all pages.
    RC ForcePages (PageNum pageNum = ALL_PAGES);

private:
    // Copy constructor
    RM_FileHandle  (const RM_FileHandle &fileHandle);
    // Overloaded =
    RM_FileHandle& operator=(const RM_FileHandle &fileHandle);

    // Bitmap Manipulation
    int GetBitmap  (char *map, int idx) const;
    void SetBitmap (char *map, int idx) const;
    void ClrBitmap (char *map, int idx) const;

    PF_FileHandle pfFileHandle;
    RM_FileHdr fileHdr;                                   // file header
    int bHdrChanged;                                      // dirty flag for file hdr
};

//
// RM_FileScan: condition-based scan of records in the file
//
class RM_FileScan {
public:
    RM_FileScan  ();
    ~RM_FileScan ();

    RC OpenScan  (const RM_FileHandle &fileHandle,
                  AttrType   attrType,
                  int        attrLength,
                  int        attrOffset,
                  CompOp     compOp,
                  void       *value,
                  ClientHint pinHint = NO_HINT); // Initialize a file scan
    RC GetNextRec(RM_Record &rec);               // Get next matching record
    RC CloseScan ();                             // Close the scan

private:
    // Copy constructor
    RM_FileScan   (const RM_FileScan &fileScan);
    // Overloaded =
    RM_FileScan&  operator=(const RM_FileScan &fileScan);

    void FindNextRecInCurPage(char *pData);

    int bScanOpen;
    PageNum curPageNum;
    SlotNum curSlotNum;

    RM_FileHandle *pFileHandle;
    AttrType attrType;
    int attrLength;
    int attrOffset;
    CompOp compOp;
    void *value;
    ClientHint pinHint;
};

//
// RM_Manager: provides RM file management
//
class RM_Manager {
public:
    RM_Manager    (PF_Manager &pfm);
    ~RM_Manager   ();

    RC CreateFile (const char *fileName, int recordSize);
    RC DestroyFile(const char *fileName);
    RC OpenFile   (const char *fileName, RM_FileHandle &fileHandle);

    RC CloseFile  (RM_FileHandle &fileHandle);

private:
    // Copy constructor
    RM_Manager     (const RM_Manager &manager);
    // Overloaded =
    RM_Manager&    operator=(const RM_Manager &manager);

    PF_Manager *pPfm;
};

//
// Print-error function
//
void RM_PrintError(RC rc);

#define RM_INVIABLERID     (START_RM_WARN + 0) // inviable rid
#define RM_UNREADRECORD    (START_RM_WARN + 1) // unread record
#define RM_INVALIDRECSIZE  (START_RM_WARN + 2) // invalid record size
#define RM_INVALIDSLOTNUM  (START_RM_WARN + 3) // invalid slot number
#define RM_RECORDNOTFOUND  (START_RM_WARN + 4) // record not found
#define RM_INVALIDCOMPOP   (START_RM_WARN + 5) // invalid comparison operator
#define RM_INVALIDATTR     (START_RM_WARN + 6) // invalid attribute parameters
#define RM_NULLPOINTER     (START_RM_WARN + 7) // pointer is null
#define RM_SCANOPEN        (START_RM_WARN + 8) // scan is open
#define RM_CLOSEDSCAN      (START_RM_WARN + 9) // scan is closed
#define RM_CLOSEDFILE      (START_RM_WARN + 10)// file handle is closed
#define RM_LASTWARN        RM_CLOSEDFILE

#define RM_EOF             PF_EOF              // work-around for rm_test

#endif
