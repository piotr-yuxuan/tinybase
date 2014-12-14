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
#include "tinybase.h"
#include "rm_rid.h"
#include "pf.h"

//
// RM_Record: RM Record interface
//
class RM_Record {
public:
    RM_Record ();
    ~RM_Record();

    // Return the data corresponding to the record.  Sets *pData to the
    // record contents.
    RC GetData(char *&pData) const;

    // Return the RID associated with the record
    RC GetRid (RID &rid) const;
    
private:
    char *pData;
    RID  rid;
};

//
// Bitmap: a class to keep track of free record slots on a given page
//
class Bitmap {
public:
  //Constructor & Destructor
  Bitmap(int nbBits);
  ~Bitmap();

  //Sets a specific bit to 1
  void set(unsigned int bitNumber);
  //Sets all the bits to 1
  void set();
  //Sets a specific bit to 0
  void reset(unsigned int bitNumber);
  //Sets all the bits to 0
  void reset();
  //Returns the current value of a bit in the Bitmap
  bool test(unsigned int bitNumber) const;
  //Writes bitmap into a buffer
  int to_buf(char *& buf) const;
  //Reads from a buffer
  int from_buf(const char * buf);
  //Gives the size
  int getSize() const {
      return size;
  }
  //Gives the size in Bytes i.e around size/8 but not exactly
  int Bitmap::getByteSize() const;
private:
    //Size of the bitmap
    unsigned int size;
    //To actually store the values
    char * bitValues;
};

//
// RM_PageHeader: a class to represent the header of a page (not to be mistaken with the first page of a file)
//
class RM_PageHeader {
public:
    //Constructor takes the number of slots as a parameter
    RM_PageHeader(int nbSlots);
    ~RM_PageHeader();

    //Writes and reads from/to the buffer
    int to_buf(char *& buf) const;
    int from_buf(const char * buf);

    //Returns the size of the header IN BYTES
    int size() const;

    //Bitmap attribute
    Bitmap freeSlots;

private:
    int nbSlots;
    int nbFreeSlots;
    int nextFreePage;
};

//
// RM_FileHeader: a class to represent the header of a file (not to be mistaken with the header of a page)
//
class RM_FileHeader {
public:
    //Constructor takes the number of slots as a parameter
    RM_FileHeader();
    ~RM_FileHeader();

    //Writes and reads from/to the buffer
    int to_buf(char *& buf) const;
    int from_buf(char *& buf);

    //Getter for recordSize
    int getRecordSize();
private:
    int firstFreePage; // first free page
    int pagesNumber; // How many pages there are in that file
    int recordSize; // Size of the record.
};

//
// RM_FileHandle: RM File interface
//
class RM_FileHandle {
public:
    RM_FileHandle ();
    ~RM_FileHandle();

    RC GetRec     (const RID &rid, RM_Record &rec) const; // Given a RID, return the record
    RC InsertRec  (const char *pData, RID &rid); // Insert a new record
    RC DeleteRec  (const RID &rid); // Delete a record
    RC UpdateRec  (const RM_Record &rec); // Update a record
    RC ForcePages (PageNum pageNum = ALL_PAGES); // Forces a page (along with any contents stored in this class) from the buffer pool to disk.  Default value forces all pages.
    int getRecordSize() // Size of the record.
    {
        return fileHeader.getRecordSize();
    }

private:
	PF_FileHandle *pf_FileHandle;
	RM_FileHeader fileHeader; // File header
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
    //boolean to represent the current state of the FileScan
    bool scaning;
    //Attributes (for the current ongoing scan)
    RM_FileHandle *fileHandle;
    AttrType attrType;
    int attrLength;
    int attrOffset;
    CompOp compOp;
    void *value;
    //Current RID
    RID currentRID;
    //Method to perform the value comparison
    bool performMatching(char *pData);
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
};

//
// Print-error function
//
void RM_PrintError(RC rc);

//
// Warning codes
//
#define RM_ALREADYOPEN      (START_RM_WARN + 0) // File is already open
#define RM_EOF              (START_RM_WARN + 1) // End Of File
#define RM_UNREADRECORD     (START_RM_WARN + 2) // record is not read

//
// Error codes
//
#define RM_FSCREATEFAIL     (START_RM_ERR - 0) // couldn't create the FileScan object
#define RM_FILENOTOPEN      (START_RM_ERR - 1) // File is not open





#endif
