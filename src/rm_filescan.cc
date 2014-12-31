//
// File:        rm_filescan.cc
// Description: RM_FileScan class implementation
// Author:      Hyunjung Park (hyunjung@stanford.edu)
//

#include "rm_internal.h"

// 
// RM_FileScan
//
// Desc: Default Constructor
//
RM_FileScan::RM_FileScan()
{
   // Initialize member variables
   bScanOpen = FALSE;
   curPageNum = RM_HEADER_PAGE_NUM;
   curSlotNum = 0;

   pFileHandle = NULL;
   attrType = INT;
   attrLength = sizeof(int);
   attrOffset = 0;
   compOp = NO_OP;
   value = NULL;
   pinHint = NO_HINT;
}

// 
// RM_FileScan
//
// Desc: Destructor
//
RM_FileScan::~RM_FileScan()         
{
   // Don't need to do anything
}

//
// OpenScan
//
// Desc: Open a file scan with the given fileHandle and scan condition
// In:   fileHandle  - RM_FileHandle object (must be open)
//       _attrType   - INT|FLOAT|STRING
//       _attrLength - 4 for INT|FLOAT, 1~MAXSTRING for STRING
//       _attrOffset - indicates the location of the attribute
//       _compOp     - EQ_OP|LT_OP|GT_OP|LE_OP|GE_OP|NE_OP|NO_OP
//       _value      - points to the value which will be compared with
//                     the given attribute
//       _pinHint    - not implemented yet
// Ret:  RM_SCANOPEN, RM_VALUENULL, RM_INVALIDATTR, RM_CLOSEDFILE
//
RC RM_FileScan::OpenScan(const RM_FileHandle &fileHandle, 
                         AttrType   _attrType,
                         int        _attrLength,
                         int        _attrOffset,
                         CompOp     _compOp,
                         void       *_value,
                         ClientHint _pinHint)
{
   // Sanity Check: 'this' should not be open yet
   if (bScanOpen)
      // Test: opened RM_FileScan
      return (RM_SCANOPEN);

   // Sanity Check: fileHandle must be open
   if (fileHandle.fileHdr.recordSize == 0) // a little tricky here
      // Test: unopened fileHandle
      return (RM_CLOSEDFILE);

   // Sanity Check: compOp
   switch (_compOp) {
   case EQ_OP:
   case LT_OP:
   case GT_OP:
   case LE_OP:
   case GE_OP:
   case NE_OP:
   case NO_OP:
      break;

   default:
      return (RM_INVALIDCOMPOP);
   }
   
   if (_compOp != NO_OP) {
      // Sanity Check: value must not be NULL
      if (_value == NULL)
         // Test: null _value
         return (RM_NULLPOINTER);

      // Sanity Check: attrType, attrLength
      switch (_attrType) {
      case INT:
      case FLOAT:
         if (_attrLength != 4)
            // Test: wrong _attrLength
            return (RM_INVALIDATTR);
         break;

      case STRING:
         if (_attrLength < 1 || _attrLength > MAXSTRINGLEN)
            // Test: wrong _attrLength
            return (RM_INVALIDATTR);
         break;

      default:
         // Test: wrong _attrType
         return (RM_INVALIDATTR);
      }

      // Sanity Check: attrOffset
      if (_attrOffset < 0 
          || _attrOffset + _attrLength > fileHandle.fileHdr.recordSize)
         // Test: wrong _attrOffset/_attrLength
         return (RM_INVALIDATTR);
   }

   // Copy parameters to local variable
   pFileHandle = (RM_FileHandle *)&fileHandle;
   attrType    = _attrType;
   attrLength  = _attrLength;
   attrOffset  = _attrOffset;
   compOp      = _compOp;
   value       =  _value;
   pinHint     = _pinHint;

   // Set local state variables
   bScanOpen = TRUE;
   curPageNum = RM_HEADER_PAGE_NUM;
   curSlotNum = pFileHandle->fileHdr.numRecordsPerPage;

   // Return ok
   return (0);
}

//
// GetNextRec
//
// Desc: Retrieve a copy of the next record that satisfies the scan condition
// Out:  rec - Set to the next matching record
// Ret:  RM or PF return code
//
RC RM_FileScan::GetNextRec(RM_Record &rec)
{
   RC rc;
   PF_PageHandle pageHandle;
   char *pData;
   RID *curRid;

   // Sanity Check: 'this' must be open
   if (!bScanOpen)
      return (RM_CLOSEDSCAN);

   // Sanity Check: fileHandle must be open
   if (pFileHandle->fileHdr.recordSize == 0) // a little tricky here
      // Test: unopened fileHandle
      return (RM_CLOSEDFILE);

   // Fetch another page if required
   if (curSlotNum == pFileHandle->fileHdr.numRecordsPerPage) {
repeat:
      // Get next page
      if (rc = pFileHandle->pfFileHandle.GetNextPage(curPageNum, pageHandle))
         // Test: EOF
         goto err_return;

      // Update curPageNum
      if (rc = pageHandle.GetPageNum(curPageNum))
         // Should not happen
         // In fact, we need to unpin the page, but don't know the page number
         goto err_return;

      // Reset curSlotNum
      curSlotNum = 0;
   }
   // We didn't process the whole page in the previous GetNextRec() call
   else {
      if (rc = pFileHandle->pfFileHandle.GetThisPage(curPageNum, pageHandle))
         if (rc == PF_INVALIDPAGE)
            // We can get PF_INVALIDPAGE if curPageNum was disposed
            goto repeat;
         else
            // Test: closed fileHandle
            goto err_return;
   }

   //
   if (rc = pageHandle.GetData(pData))
      // Should not happen
      goto err_unpin;

   // Find the next record based on scan condition
   FindNextRecInCurPage(pData);

   // No HIT in this page, go to next page
   if (curSlotNum == pFileHandle->fileHdr.numRecordsPerPage) {
      // Unpin this page
      if (rc = pFileHandle->pfFileHandle.UnpinPage(curPageNum))
         // Should not happen
         goto err_return;
      
      goto repeat;
   }
               
   // HIT: copy the record to the given location
   curRid = new RID(curPageNum, curSlotNum);
   rec.rid = *curRid;
   delete curRid;

   if (rec.pData)
      delete [] rec.pData;
   rec.recordSize = pFileHandle->fileHdr.recordSize;
   rec.pData = new char[rec.recordSize];
   memcpy(rec.pData,
          pData + pFileHandle->fileHdr.pageHeaderSize 
          + curSlotNum * pFileHandle->fileHdr.recordSize,
          pFileHandle->fileHdr.recordSize);

   // Increment curSlotNum
   curSlotNum++;

   // If there is no more matching record in this page,
   // we don't have to access this page in the next GetNextRec() call.
   FindNextRecInCurPage(pData);

   // Unpin this page
   if (rc = pFileHandle->pfFileHandle.UnpinPage(curPageNum))
      // Should not happen
      goto err_return;

   // Return ok
   return (0);

   // Recover from inconsistent state due to unexpected error
err_unpin:
   pFileHandle->pfFileHandle.UnpinPage(curPageNum);
err_return:
   // Return error
   return (rc);
}

//
// FineNextRecInCurPage
//
// Desc: Iterates slots in the current page (until hit or end)
// In:   pData - points a data page buffer
//
void RM_FileScan::FindNextRecInCurPage(char *pData)
{
   for ( ; curSlotNum < pFileHandle->fileHdr.numRecordsPerPage; curSlotNum++) {
      float cmp;
      int i;
      float f;

      // Skip empty slots
      if (!pFileHandle->GetBitmap(pData + sizeof(RM_PageHdr), curSlotNum))
         continue;
      
      // Hit if NO_OP
      if (compOp == NO_OP)
         break;

      // Do comparison according to the attribute type
      switch (attrType) {
      case INT:
         memcpy(&i,
                pData + pFileHandle->fileHdr.pageHeaderSize
                + curSlotNum * pFileHandle->fileHdr.recordSize 
                + attrOffset,
                sizeof(int));
         cmp = i - *((int *)value);
         break;

      case FLOAT:
         memcpy(&f,
                pData + pFileHandle->fileHdr.pageHeaderSize
                + curSlotNum * pFileHandle->fileHdr.recordSize 
                + attrOffset,
                sizeof(float));
         cmp = f - *((float *)value);
         break;

      case STRING:
         cmp = memcmp(pData + pFileHandle->fileHdr.pageHeaderSize
                      + curSlotNum * pFileHandle->fileHdr.recordSize 
                      + attrOffset,
                      value,
                      attrLength);
         break;
      }

      // Make decision according to comparison operator
      if ((compOp == EQ_OP && cmp == 0)
          || (compOp == LT_OP && cmp < 0)
          || (compOp == GT_OP && cmp > 0)
          || (compOp == LE_OP && cmp <= 0)
          || (compOp == GE_OP && cmp >= 0)
          || (compOp == NE_OP && cmp != 0))
         break;
   }
}

//
// CloseScan
//
// Desc: Close a file scan with the given fileHandle and scan condition
// Ret:  RM_CLOSEDSCAN
//
RC RM_FileScan::CloseScan()
{
   // Sanity Check: 'this' must be open
   if (!bScanOpen)
      // Test: closed RM_FileScan
      return (RM_CLOSEDSCAN);

   // Reset member variables
   bScanOpen = FALSE;
   curPageNum = RM_HEADER_PAGE_NUM;
   curSlotNum = 0;
   pFileHandle = NULL;
   attrType = INT;
   attrLength = sizeof(int);
   attrOffset = 0;
   compOp = NO_OP;
   value = NULL;
   pinHint = NO_HINT;

   // Return ok
   return (0);
}

