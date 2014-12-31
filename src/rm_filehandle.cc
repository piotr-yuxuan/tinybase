//
// File:        rm_filehandle.cc
// Description: RM_FileHandle class implementation
// Author:      Hyunjung Park (hyunjung@stanford.edu)
//

#include "rm_internal.h"

// 
// RM_FileHandle
//
// Desc: Default constructor 
//
RM_FileHandle::RM_FileHandle()
{
   // Initialize member variables
   bHdrChanged = FALSE;
   memset(&fileHdr, 0, sizeof(fileHdr));
   fileHdr.firstFree = RM_PAGE_LIST_END;
}

//
// ~RM_FileHandle
//
// Desc: Destructor
//
RM_FileHandle::~RM_FileHandle()
{
   // Don't need to do anything
}

//
// GetRec
//
// Desc: Given a RID, return the record
// In:   rid - 
// Out:  rec - 
// Ret:  RM return code
//
RC RM_FileHandle::GetRec(const RID &rid, RM_Record &rec) const
{
   RC rc;
   PageNum pageNum;
   SlotNum slotNum;
   PF_PageHandle pageHandle;
   char *pData;

   // Extract page number from rid
   if (rc = rid.GetPageNum(pageNum))
      // Test: inviable rid
      goto err_return;

   // Extract slot number from rid
   if (rc = rid.GetSlotNum(slotNum))
      // Should not happen
      goto err_return;

   // Sanity Check: slotNum bound check
   // Note that PF_FileHandle.GetThisPage() will take care of pageNum
   if (slotNum >= fileHdr.numRecordsPerPage || slotNum < 0)
      // Test: rid with invalid slotNum
      return (RM_INVALIDSLOTNUM);
   
   // Get the page where rid points
   if (rc = pfFileHandle.GetThisPage(pageNum, pageHandle))
      // Test: rid with invalid pageNum
      goto err_return;

   // Get the data
   if (rc = pageHandle.GetData(pData))
      // Should not happen
      goto err_unpin;

   // Sanity Check: a record corresponding to rid should exist
   if (!GetBitmap(pData + sizeof(RM_PageHdr), slotNum)) {
      // Test: non-existing rid
      pfFileHandle.UnpinPage(pageNum);
      return (RM_RECORDNOTFOUND);
   }

   // Copy the record to RM_Record
   rec.rid = rid;
   if (rec.pData)
      delete [] rec.pData;
   rec.recordSize = fileHdr.recordSize;
   rec.pData = new char[rec.recordSize];
   memcpy(rec.pData,
          pData + fileHdr.pageHeaderSize + slotNum * fileHdr.recordSize, 
          fileHdr.recordSize);

   // Unpin the page
   if (rc = pfFileHandle.UnpinPage(pageNum))
      // Should not happen
      goto err_return;

   // Return ok
   return (0);

   // Recover from inconsistent state due to unexpected error
err_unpin:
   pfFileHandle.UnpinPage(pageNum);
err_return:
   // Return error
   return (rc);
}

//
// InsertRec
//
// Desc: Insert a new record
// In:   pData - 
// Out:  rid - 
// Ret:  RM return code
//
RC RM_FileHandle::InsertRec(const char *pRecordData, RID &rid)
{
   RC rc;
   PageNum pageNum;
   SlotNum slotNum;
   PF_PageHandle pageHandle;
   char *pData;
   RID *pRid;

   // Sanity Check: pRecordData must not be NULL
   if (pRecordData == NULL)
      return (RM_NULLPOINTER);
 
   // Allocate a new page if free page list is empty
   if (fileHdr.firstFree == RM_PAGE_LIST_END) {
      // Call PF_FileHandle::AllocatePage()
      if (rc = pfFileHandle.AllocatePage(pageHandle))
         // Unopened file handle
         goto err_return;

      // Get page number
      if (rc = pageHandle.GetPageNum(pageNum))
         // Should not happen
         goto err_unpin;

      // Get data pointer
      if (rc = pageHandle.GetData(pData))
         // Should not happen
         goto err_unpin;

      // Set page header
      // Note that allocated page was already zeroed out
      // (We don't have to initialize the bitmap)
      ((RM_PageHdr *)pData)->nextFree = RM_PAGE_LIST_END;

      // Mark the page dirty since we changed the next pointer
      if (rc = pfFileHandle.MarkDirty(pageNum))
         // Should not happen
         goto err_unpin;

      // Unpin the page
      if (rc = pfFileHandle.UnpinPage(pageNum))
         // Should not happen
         goto err_return;

      // Place into the free page list
      fileHdr.firstFree = pageNum;
      bHdrChanged = TRUE;
   }
   // Pick the first page on the list
   else {
      pageNum = fileHdr.firstFree;
   }

   // Pin the page where new record will be inserted
   if (rc = pfFileHandle.GetThisPage(pageNum, pageHandle))
      goto err_return;

   // Get data pointer
   if (rc = pageHandle.GetData(pData))
      goto err_unpin;

   // Find an empty slot
   for (slotNum = 0; slotNum < fileHdr.numRecordsPerPage; slotNum++)
      if (!GetBitmap(pData + sizeof(RM_PageHdr), slotNum))
         break;

   // There should be a free slot
   assert(slotNum < fileHdr.numRecordsPerPage);
   
   // Assign rid
   pRid = new RID(pageNum, slotNum);
   rid = *pRid;
   delete pRid;

   // Copy the given record data to the buffer pool
   memcpy(pData + fileHdr.pageHeaderSize + slotNum * fileHdr.recordSize, 
          pRecordData, fileHdr.recordSize);

   // Set bit
   SetBitmap(pData + sizeof(RM_PageHdr), slotNum);

   // Find an empty slot
   for (slotNum = 0; slotNum < fileHdr.numRecordsPerPage; slotNum++)
      if (!GetBitmap(pData + sizeof(RM_PageHdr), slotNum))
         break;

   // Remove the page from the free page list if necessary
   if (slotNum == fileHdr.numRecordsPerPage) {
      fileHdr.firstFree = ((RM_PageHdr *)pData)->nextFree;
      bHdrChanged = TRUE;
      ((RM_PageHdr *)pData)->nextFree = RM_PAGE_FULL;
   }

   // Mark the header page as dirty because we changed bitmap at least
   if (rc = pfFileHandle.MarkDirty(pageNum))
      // Should not happen
      goto err_unpin;

   // Unpin the page
   if (rc = pfFileHandle.UnpinPage(pageNum))
      // Should not happen
      goto err_return;

   // Return ok
   return (0);

   // Recover from inconsistent state due to unexpected error
err_unpin:
   pfFileHandle.UnpinPage(pageNum);
err_return:
   // Return error
   return (rc);
}

//
// DeleteRec
//
// Desc: Delete a record
// In:   rid - 
// Ret:  RM return code
//
RC RM_FileHandle::DeleteRec(const RID &rid)
{
   RC rc;
   PageNum pageNum;
   SlotNum slotNum;
   PF_PageHandle pageHandle;
   char *pData;

   // Extract page number from rid
   if (rc = rid.GetPageNum(pageNum))
      // Test: inviable rid
      goto err_return;

   // Extract slot number from rid
   if (rc = rid.GetSlotNum(slotNum))
      // Should not happen
      goto err_return;

   // Sanity Check: slotNum bound check
   // Note that PF_FileHandle.GetThisPage() will take care of pageNum
   if (slotNum >= fileHdr.numRecordsPerPage || slotNum < 0)
      // Test: rid with invalid slotNum
      return (RM_INVALIDSLOTNUM);
   
   // Get the page where rid points
   if (rc = pfFileHandle.GetThisPage(pageNum, pageHandle))
      // Test: rid with invalid pageNum
      goto err_return;

   // Get the data
   if (rc = pageHandle.GetData(pData))
      // Should not happen
      goto err_unpin;

   // Sanity Check: a record corresponding to rid should exist
   if (!GetBitmap(pData + sizeof(RM_PageHdr), slotNum)) {
      // Test: non-existing rid
      pfFileHandle.UnpinPage(pageNum);
      return (RM_RECORDNOTFOUND);
   }

   // Clear bit
   ClrBitmap(pData + sizeof(RM_PageHdr), slotNum);
   
   // Not necessary
   memset(pData + fileHdr.pageHeaderSize + slotNum * fileHdr.recordSize, 
          '\0', fileHdr.recordSize);

   // Find an empty slot
   for (slotNum = 0; slotNum < fileHdr.numRecordsPerPage; slotNum++)
      if (GetBitmap(pData + sizeof(RM_PageHdr), slotNum))
         break;

   // Dispose the page if empty (the deleted record was the last one)
   // This will help the total number of occupied pages to be remained
   // as small as possible
   if (slotNum == fileHdr.numRecordsPerPage) {
      fileHdr.firstFree = ((RM_PageHdr *)pData)->nextFree;
      bHdrChanged = TRUE;
      
      // Mark the header page as dirty
      if (rc = pfFileHandle.MarkDirty(pageNum))
         // Should not happen
         goto err_return;
      
      // Unpin the page
      if (rc = pfFileHandle.UnpinPage(pageNum))
         // Should not happen
         goto err_return;

      // Call PF_FileHandle.DisposePage()
      return pfFileHandle.DisposePage(pageNum);
   }

   // Insert the page into the free page list if necessary
   if (((RM_PageHdr *)pData)->nextFree == RM_PAGE_FULL) {
      ((RM_PageHdr *)pData)->nextFree = fileHdr.firstFree;
      fileHdr.firstFree = pageNum;
      bHdrChanged = TRUE;
   }

   // Mark the header page as dirty because we changed bitmap at least
   if (rc = pfFileHandle.MarkDirty(pageNum))
      // Should not happen
      goto err_unpin;

   // Unpin the page
   if (rc = pfFileHandle.UnpinPage(pageNum))
      // Should not happen
      goto err_return;
 
   // Return ok
   return (0);

   // Recover from inconsistent state due to unexpected error
err_unpin:
   pfFileHandle.UnpinPage(pageNum);
err_return:
   // Return error
   return (rc);
}

//
// UpdateRec
//
// Desc: Update a record
// In:   rec -
// Ret:  RM return code
//
RC RM_FileHandle::UpdateRec(const RM_Record &rec)
{
   RC rc;
   RID rid;
   PageNum pageNum;
   SlotNum slotNum;
   PF_PageHandle pageHandle;
   char *pData;
   char *pRecordData;

   // Get rid
   if (rc = rec.GetRid(rid))
      // Test: unread record
      goto err_return;

   // Get record data
   if (rc = rec.GetData(pRecordData))
      // Should not happen
      goto err_return;

   // Extract page number from rid
   if (rc = rid.GetPageNum(pageNum))
      // Should not happen
      goto err_return;

   // Extract slot number from rid
   if (rc = rid.GetSlotNum(slotNum))
      // Should not happen
      goto err_return;

   // Sanity Check: slotNum bound check
   // Note that PF_FileHandle.GetThisPage() will take care of pageNum
   if (slotNum >= fileHdr.numRecordsPerPage || slotNum < 0)
      // Test: apply R1's record to R2
      return (RM_INVALIDSLOTNUM);

   // Sanity Check: recordSize of updating record and file handle
   //               must match
   if (rec.recordSize != fileHdr.recordSize)
      // Test: apply R1's record to R2
      return (RM_INVALIDRECSIZE);

   // Get the page where rid points
   if (rc = pfFileHandle.GetThisPage(pageNum, pageHandle))
      // Test: rid with invalid pageNum
      goto err_return;

   // Get the data
   if (rc = pageHandle.GetData(pData))
      // Should not happen
      goto err_unpin;

   // Sanity Check: a record corresponding to rid should exist
   if (!GetBitmap(pData + sizeof(RM_PageHdr), slotNum)) {
      // Test: non-existing rid
      pfFileHandle.UnpinPage(pageNum);
      return (RM_RECORDNOTFOUND);
   }

   // Update
   memcpy(pData + fileHdr.pageHeaderSize + slotNum * fileHdr.recordSize, 
          pRecordData,
          fileHdr.recordSize);

   // Mark the header page as dirty
   if (rc = pfFileHandle.MarkDirty(pageNum))
      // Should not happen
      goto err_unpin;

   // Unpin the page
   if (rc = pfFileHandle.UnpinPage(pageNum))
      // Should not happen
      goto err_return;
 
   // Return ok
   return (0);

   // Recover from inconsistent state due to unexpected error
err_unpin:
   pfFileHandle.UnpinPage(pageNum);
err_return:
   // Return error
   return (rc);
}

//
// ForcePages
//
// Desc: Forces a page (along with any contents stored in this class)
//       from the buffer pool to disk.  Default value forces all pages.
// In:   pageNum - 
// Ret:  RM return code
//
RC RM_FileHandle::ForcePages(PageNum pageNum)
{
   RC rc;
   
   // Write back the file header if any changes made to the header 
   // while the file is open
   if (bHdrChanged) {
      PF_PageHandle pageHandle;
      char* pData;

      // Get the header page
      if (rc = pfFileHandle.GetFirstPage(pageHandle))
         // Test: unopened(closed) fileHandle, invalid file
         goto err_return;

      // Get a pointer where header information will be written
      if (rc = pageHandle.GetData(pData))
         // Should not happen
         goto err_unpin;

      // Write the file header (to the buffer pool)
      memcpy(pData, &fileHdr, sizeof(fileHdr));

      // Mark the header page as dirty
      if (rc = pfFileHandle.MarkDirty(RM_HEADER_PAGE_NUM))
         // Should not happen
         goto err_unpin;

      // Unpin the header page
      if (rc = pfFileHandle.UnpinPage(RM_HEADER_PAGE_NUM))
         // Should not happen
         goto err_return;

      if (rc = pfFileHandle.ForcePages(RM_HEADER_PAGE_NUM))
         // Should not happen
         goto err_return;

      // Set file header to be not changed
      bHdrChanged = FALSE;
   }

   // Call PF_FileHandle::ForcePages()
   if (rc = pfFileHandle.ForcePages(pageNum))
      goto err_return;

   // Return ok
   return (0);

   // Recover from inconsistent state due to unexpected error
err_unpin:
   pfFileHandle.UnpinPage(RM_HEADER_PAGE_NUM);
err_return:
   // Return error
   return (rc);
}

//
// GetBitmap
//
// Desc: Return a bit corresponding to the given index
// In:   map - address of bitmap
//       idx - bit index
// Ret:  TRUE or FALSE
//
int RM_FileHandle::GetBitmap(char *map, int idx) const
{
   return (map[idx / 8] & (1 << (idx % 8))) != 0;
}

//
// SetBitmap
//
// Desc: Set a bit corresponding to the given index
// In:   map - address of bitmap
//       idx - bit index
//
void RM_FileHandle::SetBitmap(char *map, int idx) const
{
   map[idx / 8] |= (1 << (idx % 8));
}

//
// ClrBitmap
//
// Desc: Clear a bit corresponding to the given index
// In:   map - address of bitmap
//       idx - bit index
//
void RM_FileHandle::ClrBitmap(char *map, int idx) const
{
   map[idx / 8] &= ~(1 << (idx % 8));
}

