//
// File:        rm_record.cc
// Description: RM_Record class implementation
// Author:      Yixin Huang (yixin.huang@telecom-paristech.fr)
//


#include "rm.h"

// 
// RM_Record
//
// Desc: Default Constructor
//
RM_Record::RM_Record()
{
   this->pData = NULL;
}

//
// ~RM_Record
// 
// Desc: Destructor
//
RM_Record::~RM_Record()
{
   if(pData != NULL) {
        delete[] (pData);
	pData = NULL;
    }
}

//
// GetData
// 
// Desc: Return data
//       The record object must refer to read record
//       (by RM_FileHandle::GetRec() or RM_FileScan::GetNextRec())
// Out:  pData - set to this record's data
// Ret:  RM_UNREADRECORD
//
RC RM_Record::GetData(char *&pData) const
{
   // If a record not read
   if (pData == NULL)
      return (RM_UNREADRECORD);

   // Set the parameter to this RM_Record's data
   pData = this->pData;

   // Return ok
   return OK_RC;
}

//
// GetRid
// 
// Desc: Return RID
// Out:  rid - set to this record's record identifier
// Ret:  RM_UNREADRECORD
//
RC RM_Record::GetRid(RID &rid) const
{
   // If a record not read
   if (pData == NULL)
      return (RM_UNREADRECORD);

   // Set the parameter to this RM_Record's record identifier
   rid = this->rid;

   // Return ok
   return OK_RC;
}
