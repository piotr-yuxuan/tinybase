//
// File:        rm_record.cc
// Description: RM_Record class implementation
// Author:      Hyunjung Park (hyunjung@stanford.edu)
//

#include "rm_internal.h"

// 
// RM_Record
//
// Desc: Default Constructor
//
RM_Record::RM_Record()
{
   pData = NULL;
   recordSize = 0;
}

//
// ~RM_Record
// 
// Desc: Destructor
//
RM_Record::~RM_Record()
{
   if (pData)
      delete [] pData;
}

//
// GetData
// 
// Desc: Return data
//       The record object must refer to read record
//       (by RM_FileHandle::GetRec() or RM_FileScan::GetNextRec())
// Out:  _pData - set to this record's data
// Ret:  RM_UNREADRECORD
//
RC RM_Record::GetData(char *&_pData) const
{
   // A record should have been read
   if (pData == NULL)
      return (RM_UNREADRECORD);

   // Set the parameter to this RM_Record's data
   _pData = pData;

   // Return ok
   return (0);
}

//
// GetData
// 
// Desc: Return RID
// Out:  _rid - set to this record's record identifier
// Ret:  RM_UNREADRECORD
//
RC RM_Record::GetRid(RID &_rid) const
{
   // A record should have been read
   if (pData == NULL)
      return (RM_UNREADRECORD);

   // Set the parameter to this RM_Record's record identifier
   _rid = rid;

   // Return ok
   return (0);
}

