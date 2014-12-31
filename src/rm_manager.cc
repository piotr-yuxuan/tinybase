//
// File:        rm_manager.cc
// Description: RM_Manager class implementation
// Author:      Hyunjung Park (hyunjung@stanford.edu)
//

#include "rm_internal.h"

// 
// RM_Manager
//
// Desc: Constructor
//
RM_Manager::RM_Manager(PF_Manager &pfm)
{
   // Set the associated PF_Manager object
   pPfm = &pfm;
}

//
// ~RM_Manager
// 
// Desc: Destructor
//
RM_Manager::~RM_Manager()
{
   // Clear the associated PF_Manager object
   pPfm = NULL;
}

//
// CreateFile
//
// Desc: Create a new RM file whose name is "fileName"
//       Allocate a file header page and fill out some information
// In:   fileName - name of file to create
//       recordSize - fixed size of records
// Ret:  RM_INVALIDRECSIZE or PF return code
//
RC RM_Manager::CreateFile(const char *fileName, int recordSize)
{
   RC rc;
   PF_FileHandle pfFileHandle;
   PF_PageHandle pageHandle;
   char* pData;
   RM_FileHdr *fileHdr;

   // Sanity Check: recordSize should not be too large (or small)
   // Note that PF_Manager::CreateFile() will take care of fileName
   if (recordSize >= PF_PAGE_SIZE - sizeof(RM_PageHdr) || recordSize < 1)
      // Test: invalid recordSize
      return (RM_INVALIDRECSIZE);

   // Call PF_Manager::CreateFile()
   if (rc = pPfm->CreateFile(fileName))
      // Test: existing fileName, wrong permission
      goto err_return;

   // Call PF_Manager::OpenFile()
   if (rc = pPfm->OpenFile(fileName, pfFileHandle))
      // Should not happen
      goto err_destroy;

   // Allocate the header page (pageNum must be 0)
   if (rc = pfFileHandle.AllocatePage(pageHandle))
      // Should not happen
      goto err_close;

   // Get a pointer where header information will be written
   if (rc = pageHandle.GetData(pData))
      // Should not happen
      goto err_unpin;

   // Write the file header (to the buffer pool)
   fileHdr = (RM_FileHdr *)pData;
   fileHdr->firstFree = RM_PAGE_LIST_END;
   fileHdr->recordSize = recordSize;
   fileHdr->numRecordsPerPage = (PF_PAGE_SIZE - sizeof(RM_PageHdr) - 1) 
                                / (recordSize + 1.0/8);
   if (recordSize * (fileHdr->numRecordsPerPage + 1) 
       + fileHdr->numRecordsPerPage / 8 
       <= PF_PAGE_SIZE - sizeof(RM_PageHdr) - 1)
      fileHdr->numRecordsPerPage++;
   fileHdr->pageHeaderSize = sizeof(RM_PageHdr) 
                             + (fileHdr->numRecordsPerPage + 7) / 8;
   fileHdr->numRecords = 0;

   // Mark the header page as dirty
   if (rc = pfFileHandle.MarkDirty(RM_HEADER_PAGE_NUM))
      // Should not happen
      goto err_unpin;
   
   // Unpin the header page
   if (rc = pfFileHandle.UnpinPage(RM_HEADER_PAGE_NUM))
      // Should not happen
      goto err_close;
   
   // Call PF_Manager::CloseFile()
   if (rc = pPfm->CloseFile(pfFileHandle))
      // Should not happen
      goto err_destroy;

   // Return ok
   return (0);

   // Recover from inconsistent state due to unexpected error
err_unpin:
   pfFileHandle.UnpinPage(RM_HEADER_PAGE_NUM);
err_close:
   pPfm->CloseFile(pfFileHandle);
err_destroy:
   pPfm->DestroyFile(fileName);
err_return:
   // Return error
   return (rc);
}

//
// DestroyFile
//
// Desc: Delete a RM file named fileName (fileName must exist and not be open)
// In:   fileName - name of file to delete
// Ret:  PF return code
//
RC RM_Manager::DestroyFile(const char *fileName)
{
   RC rc;

   // Call PF_Manager::DestroyFile()
   if (rc = pPfm->DestroyFile(fileName))
      // Test: non-existing fileName, wrong permission
      goto err_return;

   // Return ok
   return (0);

err_return:
   // Return error
   return (rc);
}

//
// OpenFile
//
// Desc: Open the paged file whose name is "fileName".
//       Copy the file header information into a private variable in
//       the file handle so that we can unpin the header page immediately
//       and later find out details without reading the header page
// In:   fileName - name of file to open
// Out:  fileHandle - refer to the open file
// Ret:  PF return code
//
RC RM_Manager::OpenFile(const char *fileName, RM_FileHandle &fileHandle)
{
   RC rc;
   PF_PageHandle pageHandle;
   char* pData;
   
   // Call PF_Manager::OpenFile()
   if (rc = pPfm->OpenFile(fileName, fileHandle.pfFileHandle))
      // Test: non-existing fileName, opened fileHandle
      goto err_return;

   // Get the header page
   if (rc = fileHandle.pfFileHandle.GetFirstPage(pageHandle))
      // Test: invalid file
      goto err_close;

   // Get a pointer where header information resides
   if (rc = pageHandle.GetData(pData))
      // Should not happen
      goto err_unpin;

   // Read the file header (from the buffer pool to RM_FileHandle)
   memcpy(&fileHandle.fileHdr, pData, sizeof(fileHandle.fileHdr));

   // Unpin the header page
   if (rc = fileHandle.pfFileHandle.UnpinPage(RM_HEADER_PAGE_NUM))
      // Should not happen
      goto err_close;

   // TODO: cannot guarantee the validity of file header at this time

   // Set file header to be not changed
   fileHandle.bHdrChanged = FALSE;

   // Return ok
   return (0);

   // Recover from inconsistent state due to unexpected error
err_unpin:
   fileHandle.pfFileHandle.UnpinPage(RM_HEADER_PAGE_NUM);
err_close:
   pPfm->CloseFile(fileHandle.pfFileHandle);
err_return:
   // Return error
   return (rc);
}

//
// CloseFile
// 
// Desc: Close file associated with fileHandle
//       Write back the file header (if there was any changes)
// In:   fileHandle - handle of file to close
// Out:  fileHandle - no longer refers to an open file
// Ret:  RM return code
//
RC RM_Manager::CloseFile(RM_FileHandle &fileHandle)
{
   RC rc;
   
   // Write back the file header if any changes made to the header 
   // while the file is open
   if (fileHandle.bHdrChanged) {
      PF_PageHandle pageHandle;
      char* pData;

      // Get the header page
      if (rc = fileHandle.pfFileHandle.GetFirstPage(pageHandle))
         // Test: unopened(closed) fileHandle, invalid file
         goto err_return;

      // Get a pointer where header information will be written
      if (rc = pageHandle.GetData(pData))
         // Should not happen
         goto err_unpin;

      // Write the file header (to the buffer pool)
      memcpy(pData, &fileHandle.fileHdr, sizeof(fileHandle.fileHdr));

      // Mark the header page as dirty
      if (rc = fileHandle.pfFileHandle.MarkDirty(RM_HEADER_PAGE_NUM))
         // Should not happen
         goto err_unpin;

      // Unpin the header page
      if (rc = fileHandle.pfFileHandle.UnpinPage(RM_HEADER_PAGE_NUM))
         // Should not happen
         goto err_return;

      // Set file header to be not changed
      fileHandle.bHdrChanged = FALSE;
   }

   // Call PF_Manager::CloseFile()
   if (rc = pPfm->CloseFile(fileHandle.pfFileHandle))
      // Test: unopened(closed) fileHandle
      goto err_return;

   // Reset member variables
   memset(&fileHandle.fileHdr, 0, sizeof(fileHandle.fileHdr));
   fileHandle.fileHdr.firstFree = RM_PAGE_LIST_END;

   // Return ok
   return (0);

   // Recover from inconsistent state due to unexpected error
err_unpin:
   fileHandle.pfFileHandle.UnpinPage(RM_HEADER_PAGE_NUM);
err_return:
   // Return error
   return (rc);
}

