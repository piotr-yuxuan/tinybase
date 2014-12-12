//
// File:        rm_filehandle.h
// Description: RM_FileHandle class interface
// Authors:     Pierre de Boissset (pdeboisset@enst.fr)
//

#ifndef RM_FILEHANDLE_H
#define RM_FILEHANDLE_H

#include "rm_filehandle.h"

//
// RM_FileHandle - manipulate the records in an open RM component file
//
class RM_FileHandle {
  public:
       RM_FileHandle  ();                                  // Constructor
       ~RM_FileHandle ();                                  // Destructor
    RC GetRec         (const RID &rid, RM_Record &rec) const;
                                                           // Get a record
    RC InsertRec      (const char *pData, RID &rid);       // Insert a new record,
                                                           //   return record id
    RC DeleteRec      (const RID &rid);                    // Delete a record
    RC UpdateRec      (const RM_Record &rec);              // Update a record
    RC ForcePages     (PageNum pageNum = ALL_PAGES) const; // Write dirty page(s)
                                                           //   to disk
 };

#endif
