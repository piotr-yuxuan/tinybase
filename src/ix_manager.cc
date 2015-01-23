//
// File:        ix_manager.cc
// Description: IX_Manager class implementation
// Author:      Hyunjung Park (hyunjung@stanford.edu)
//

#include "ix_internal.h"

// 
// IX_Manager
//
// Desc: Constructor
//
IX_Manager::IX_Manager(PF_Manager &pfm)
{
   // Set the associated PF_Manager object
   pPfm = &pfm;
}

//
// ~IX_Manager
// 
// Desc: Destructor
//
IX_Manager::~IX_Manager()
{
   // Clear the associated PF_Manager object
   pPfm = NULL;
}

//
// CreateIndex
//
// Desc: 
// In:   fileName -
//       indexNo - 
//       attrType - 
//       attrLength -
// Ret:  IX_INVALIDINDEXNO or PF return code
//
RC IX_Manager::CreateIndex(const char *fileName, int indexNo,
                           AttrType attrType, int attrLength)
{
   RC rc;
   char *fileNameIndexNo;
   PF_FileHandle pfFileHandle;
   PF_PageHandle pageHandle;
   char* pNode;

   // Sanity Check: fileName
   if (fileName == NULL)
      // Test: Null fileName
      return (IX_NULLPOINTER);

   // Sanity Check: indexNo should be non-negative
   // Note that PF_Manager::CreateFile() will take care of fileName
   if (indexNo < 0)
      // Test: invalid index number
      return (IX_INVALIDINDEXNO);

   // Sanity Check: attrType, attrLength
   switch (attrType) {
   case INT:
   case FLOAT:
      if (attrLength != 4)
         // Test: wrong attrLength
         return (IX_INVALIDATTR);
      break;

   case STRING:
      if (attrLength < 1 || attrLength > MAXSTRINGLEN)
         // Test: wrong attrLength
         return (IX_INVALIDATTR);
      break;

   default:
      // Test: wrong attrType
      return (IX_INVALIDATTR);
   }

   // Allocate memory for "fileName.indexNo"
   if ((fileNameIndexNo = new char[strlen(fileName) + 16]) == NULL)
      return (IX_NOMEM);

   sprintf(fileNameIndexNo, "%s.%u", fileName, indexNo);

   // Call PF_Manager::CreateFile()
   if (rc = pPfm->CreateFile(fileNameIndexNo))
      // Test: existing fileName, wrong permission
      goto err_return;

   // Call PF_Manager::OpenFile()
   if (rc = pPfm->OpenFile(fileNameIndexNo, pfFileHandle))
      // Should not happen
      goto err_destroy;

   // Allocate the root page (pageNum must be 0)
   if (rc = pfFileHandle.AllocatePage(pageHandle))
      // Should not happen
      goto err_close;

   // Get a pointer where header information will be written
   if (rc = pageHandle.GetData(pNode))
      // Should not happen
      goto err_unpin;

   // Write the root node
   ((IX_PageHdr *)pNode)->flags = IX_LEAF_NODE;
   ((IX_PageHdr *)pNode)->numKeys = 0;
   ((IX_PageHdr *)pNode)->prevNode = attrType;
   ((IX_PageHdr *)pNode)->nextNode = attrLength;

   // Mark the header page as dirty
   if (rc = pfFileHandle.MarkDirty(0))
      // Should not happen
      goto err_unpin;
   
   // Unpin the header page
   if (rc = pfFileHandle.UnpinPage(0))
      // Should not happen
      goto err_close;
   
   // Call PF_Manager::CloseFile()
   if (rc = pPfm->CloseFile(pfFileHandle))
      // Should not happen
      goto err_destroy;

   // Deallocate memory for "fileName.indexNo"
   delete [] fileNameIndexNo;

   // Return ok
   return (0);

   // Recover from inconsistent state due to unexpected error
err_unpin:
   pfFileHandle.UnpinPage(0);
err_close:
   pPfm->CloseFile(pfFileHandle);
err_destroy:
   pPfm->DestroyFile(fileNameIndexNo);
err_return:
   delete [] fileNameIndexNo;
   // Return error
   return (rc);
}

//
// DestroyIndex
//
// Desc: 
// In:   fileName - 
//       indexNo - 
// Ret:  IX_INVALIDINDEXNO or PF return code
//
RC IX_Manager::DestroyIndex(const char *fileName, int indexNo)
{
   RC rc;
   char *fileNameIndexNo;

   // Sanity Check: fileName
   if (fileName == NULL)
      // Test: Null fileName
      return (IX_NULLPOINTER);

   // Sanity Check: indexNo should be non-negative
   // Note that PF_Manager::CreateFile() will take care of fileName
   if (indexNo < 0)
      // Test: invalid index number
      return (IX_INVALIDINDEXNO);

   // Allocate memory for "fileName.indexNo"
   if ((fileNameIndexNo = new char[strlen(fileName) + 16]) == NULL)
      return (IX_NOMEM);

   sprintf(fileNameIndexNo, "%s.%u", fileName, indexNo);

   // Call PF_Manager::DestroyFile()
   if (rc = pPfm->DestroyFile(fileNameIndexNo))
      // Test: non-existing fileName, wrong permission
      goto err_return;

   // Deallocate memory for "fileName.indexNo"
   delete [] fileNameIndexNo;

   // Return ok
   return (0);

   // Recover from inconsistent state due to unexpected error
err_return:
   delete [] fileNameIndexNo;
   // Return error
   return (rc);
}

//
// OpenIndex
//
// Desc: 
// In:   fileName - 
//       indexNo - 
// Out:  indexHandle - 
// Ret:  IX_INVALIDINDEXNO or PF return code
//
RC IX_Manager::OpenIndex(const char *fileName, int indexNo,
                         IX_IndexHandle &indexHandle)
{
   RC rc;
   char *fileNameIndexNo;
   PF_PageHandle pageHandle;
   char* pNode;

   // Sanity Check: fileName
   if (fileName == NULL)
      // Test: Null fileName
      return (IX_NULLPOINTER);

   // Sanity Check: indexNo should be non-negative
   // Note that PF_Manager::CreateFile() will take care of fileName
   if (indexNo < 0)
      // Test: invalid index number
      return (IX_INVALIDINDEXNO);

   // Allocate memory for "fileName.indexNo"
   if ((fileNameIndexNo = new char[strlen(fileName) + 16]) == NULL)
      return (IX_NOMEM);

   sprintf(fileNameIndexNo, "%s.%u", fileName, indexNo);

   // Call PF_Manager::OpenFile()
   if (rc = pPfm->OpenFile(fileNameIndexNo, indexHandle.pfFileHandle))
      // Test: non-existing fileName, opened indexHandle
      goto err_return;

   // Get the root node
   if (rc = indexHandle.pfFileHandle.GetFirstPage(pageHandle))
      // Test: invalid file
      goto err_close;

   // Get a data pointer
   if (rc = pageHandle.GetData(pNode))
      // Should not happen
      goto err_unpin;

   // Read the common header
   indexHandle.attrType   = (AttrType)((IX_PageHdr *)pNode)->prevNode;
   indexHandle.attrLength = ((IX_PageHdr *)pNode)->nextNode;

   // Unpin the header page
   if (rc = indexHandle.pfFileHandle.UnpinPage(0))
      // Should not happen
      goto err_close;

   // Deallocate memory for "fileName.indexNo"
   delete [] fileNameIndexNo;

   // Return ok
   return (0);

   // Recover from inconsistent state due to unexpected error
err_unpin:
   indexHandle.pfFileHandle.UnpinPage(0);
err_close:
   pPfm->CloseFile(indexHandle.pfFileHandle);
err_return:
   delete [] fileNameIndexNo;
   // Return error
   return (rc);
}

//
// CloseIndex
// 
// Desc: 
// In:   indexHandle - 
// Ret:  PF return code
//
RC IX_Manager::CloseIndex(IX_IndexHandle &indexHandle)
{
   RC rc;

   // Call PF_Manager::CloseFile()
   if (rc = pPfm->CloseFile(indexHandle.pfFileHandle))
      // Test: unopened(closed) indexHandle
      goto err_return;

   // Reset member variables
   indexHandle.attrType = INT;
   indexHandle.attrLength = 0;

   // Return ok
   return (0);

err_return:
   // Return error
   return (rc);
}

