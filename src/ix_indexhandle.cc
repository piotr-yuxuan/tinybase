//
// File:        ix_filehandle.cc
// Description: IX_IndexHandle class implementation
// Author:      Hyunjung Park (hyunjung@stanford.edu)
//

#include "ix_internal.h"

// 
// IX_IndexHandle
//
// Desc: Default constructor 
//
IX_IndexHandle::IX_IndexHandle()
{
   // Initialize member variables
   attrType = INT;
   attrLength = 0;
}

//
// ~IX_IndexHandle
//
// Desc: Destructor
//
IX_IndexHandle::~IX_IndexHandle()
{
   // Don't need to do anything
}

//
// InsertEntry
//
// Desc: Insert a new index entry to index.
// In:   pData - key value
//       rid - record identifier
// Ret:  IX_NULLPOINTER, RM_INVIABLERID
//
RC IX_IndexHandle::InsertEntry(void *pData, const RID &rid)
{
   RC rc;
   PageNum pageNum;
   char* tmp;
   
   // Sanity Check: pData must not be NULL
   if (pData == NULL)
      // Test: NULL pData
      return (IX_NULLPOINTER);
 
   // Sanity Check: RID must be viable
   if (rc = rid.GetPageNum(pageNum))
      // Test: inviable rid
      goto err_return;

   // Insert the new entry to the B+ tree
   if (rc = InsertEntryToNode(0, pData, rid, tmp, pageNum))
      // Test: unopened indexHandle
      goto err_return;

   // Return ok
   return (0);

err_return:
   // Return error
   return (rc);
}

//
// InsertEntryToNode
//
// Desc: Insert a new index entry to a specific node or the subtree
//       accessible from it.
//       
//       If this node is internal: 
//         1. Find a subtree where the entry should be inserted.
//         2. Recursively call InsertEntryToNode() for the subtree.
//         3. If the traversed child node is not splitted, we're DONE.
//         4. Otherwise, insert a new key/pointer pair propagated from
//            child to this node. If no space, split this node and
//            propagate split information to parent node.
//
//       If this node is leaf:
//         1. If enough space, just add the new entry into this node,
//            and we're DONE.
//         2. Otherwise, split this node and propagate split information 
//            to parent node.
//
// In:   nodeNum - 
//       pData/rid - index entry to be inserted
// Out:  splitKey/    - key/pointer pair which should be inserted to
//       splitNodeNum   the parent node (as a result of split)
// Ret:  PF return code
//
RC IX_IndexHandle::InsertEntryToNode(const PageNum nodeNum,
                                     void *pData, const RID &rid,
                                     char *&splitKey,
                                     PageNum &splitNodeNum)
{
   RC rc;
   PF_PageHandle pageHandle;
   char *pNode;
   int j;

   // Pin
   if (rc = pfFileHandle.GetThisPage(nodeNum, pageHandle))
      goto err_return;
   if (rc = pageHandle.GetData(pNode))
      goto err_return;

   // Current node is LEAF node
   if (((IX_PageHdr *)pNode)->flags & IX_LEAF_NODE) {
      // Unpin
      if (rc = pfFileHandle.UnpinPage(nodeNum))
         goto err_return;

      // Insert new entry to leaf node
      if (rc = InsertEntryToLeafNode(nodeNum, pData, rid,
                                     splitKey, splitNodeNum))
         goto err_return;
   }

   // Current node is INTERNAL node
   else {
      PageNum childNodeNum;

      // Find the right child to traverse
      for (j = 0; j < ((IX_PageHdr *)pNode)->numKeys; j++) {
         if (Compare(pData, InternalKey(pNode, j)) < 0)
            break;
      }
      memcpy(&childNodeNum, InternalPtr(pNode, j), sizeof(PageNum));

      // Unpin
      if (rc = pfFileHandle.UnpinPage(nodeNum))
         goto err_return;

      // Recursively call InsertEntryToNode()
      // No clue whether the child is internal or leaf
      if (rc = InsertEntryToNode(childNodeNum, pData, rid, 
                                 splitKey, splitNodeNum))
         goto err_return;

      // Need to add new entry to this node
      // (traveresed child node has been splitted)
      if (splitNodeNum != IX_DONT_SPLIT)
         if (rc = InsertEntryToIntlNode(nodeNum, childNodeNum,
                                        splitKey, splitNodeNum))
            // Should not happen
            goto err_return;
   }

   // Return ok
   return (0);

   // Return error
err_return:
   return (rc);
}

//
// InsertEntryToIntlNode
//
RC IX_IndexHandle::InsertEntryToIntlNode(const PageNum nodeNum,
                                         const PageNum childNodeNum,
                                         char *&splitKey,
                                         PageNum &splitNodeNum)
{
   RC rc;
   PF_PageHandle pageHandle;
   char *pNode;
   int numKeys;

   // Pin
   if (rc = pfFileHandle.GetThisPage(nodeNum, pageHandle))
      goto err_return;
   if (rc = pageHandle.GetData(pNode))
      goto err_return;

   // Read numKeys
   numKeys = ((IX_PageHdr *)pNode)->numKeys;

   // Unpin
   if (rc = pfFileHandle.UnpinPage(nodeNum))
      goto err_return;

   // Just add new entry if possible
   if (IX_PAGEHDR_SIZE + (numKeys + 2) * InternalEntrySize() <= PF_PAGE_SIZE) {
      if (rc = InsertEntryToIntlNodeNoSplit(nodeNum, childNodeNum,
                                            splitKey, splitNodeNum))
         goto err_return;
   }
   // Split INTERNAL node
   else {
      if (rc = InsertEntryToIntlNodeSplit(nodeNum, childNodeNum,
                                          splitKey, splitNodeNum))
         goto err_return;
   }

   // Return ok
   return (0);

   // Return error
err_return:
#ifdef DEBUG_IX
   assert(0);
#endif
   return (rc);
}

//
// InsertEntryToIntlNodeNoSplit
//
RC IX_IndexHandle::InsertEntryToIntlNodeNoSplit(const PageNum nodeNum,
                                                const PageNum childNodeNum,
                                                char *&splitKey,
                                                PageNum &splitNodeNum)
{
   RC rc;
   PF_PageHandle pageHandle;
   char *pNode;
   int j;

   // Pin
   if (rc = pfFileHandle.GetThisPage(nodeNum, pageHandle))
      goto err_return;
   if (rc = pageHandle.GetData(pNode))
      goto err_return;

   // Should respect the original order among duplicated keys
   for (j = 0; j < ((IX_PageHdr *)pNode)->numKeys; j++)
      if (memcmp(InternalPtr(pNode, j), &childNodeNum, sizeof(PageNum)) == 0) {
         // Make a "hole" for new entry
         memmove(InternalKey(pNode, j + 1),
                 InternalKey(pNode, j),
                 (((IX_PageHdr *)pNode)->numKeys - j) * InternalEntrySize());
         break;
      }

   // Fill out new key
   memcpy(InternalKey(pNode, j), splitKey, attrLength);
   // Fill out node associated with new key
   memcpy(InternalPtr(pNode, j+1), &splitNodeNum, sizeof(PageNum));
   // Increment #keys
   ((IX_PageHdr *)pNode)->numKeys++;

   // Unpin
   if (rc = pfFileHandle.MarkDirty(nodeNum))
      goto err_return;
   if (rc = pfFileHandle.UnpinPage(nodeNum))
      goto err_return;

   // Reset
   splitNodeNum = IX_DONT_SPLIT;
   delete [] splitKey;
   splitKey = NULL;

   // Return ok
   return (0);

   // Return error
err_return:
#ifdef DEBUG_IX
   assert(0);
#endif
   return (rc);
}

//
// InsertEntryToIntlNodeSplit
//
RC IX_IndexHandle::InsertEntryToIntlNodeSplit(const PageNum nodeNum,
                                              const PageNum childNodeNum,
                                              char *&splitKey,
                                              PageNum &splitNodeNum)
{
   RC rc;
   PF_PageHandle pageHandle;
   char *pNode;
   PF_PageHandle newPageHandle;
   char *pNewNode;
   PageNum newNodeNum;
   int insertLocation, pivot;
   char *newSplitKey;
   int j;

   // Pin
   if (rc = pfFileHandle.GetThisPage(nodeNum, pageHandle))
      goto err_return;
   if (rc = pageHandle.GetData(pNode))
      goto err_return;

   // Allocate a new INTERNAL node
   if (rc = pfFileHandle.AllocatePage(newPageHandle))
      goto err_return;
   if (rc = newPageHandle.GetData(pNewNode))
      goto err_return;
   if (rc = newPageHandle.GetPageNum(newNodeNum))
      goto err_return;

   // Should respect the original order among duplicated keys
   for (j = 0; j <= ((IX_PageHdr *)pNode)->numKeys; j++)
      if (memcmp(InternalPtr(pNode, j), &childNodeNum, sizeof(PageNum)) == 0)
         break;

#ifdef DEBUG_IX
   assert(j <= ((IX_PageHdr *)pNode)->numKeys);
#endif

   newSplitKey = new char[MAXSTRINGLEN];

   // Select a boundary so that "half full" constraint is met
   pivot = (((IX_PageHdr *)pNode)->numKeys + 1) / 2;
   if (j > pivot)
      insertLocation =  1; // right
   else if (j < pivot) {
      pivot--;
      insertLocation = -1; // left
   } else
      insertLocation =  0; // to parent

   // Move a half of data to new node
   if (insertLocation == 0) {
      memcpy(InternalKey(pNewNode, 0),
             InternalKey(pNode, pivot),
             (((IX_PageHdr *)pNode)->numKeys - pivot) * InternalEntrySize());
      memcpy(InternalPtr(pNewNode, 0),
             &splitNodeNum,
             sizeof(PageNum));
      memcpy(newSplitKey, splitKey, MAXSTRINGLEN);
   }
   else {
      memcpy(InternalPtr(pNewNode, 0),
             InternalPtr(pNode, pivot + 1),
             (((IX_PageHdr *)pNode)->numKeys - pivot) * InternalEntrySize());
      memcpy(newSplitKey, InternalKey(pNode, pivot), attrLength);
   }

   // Write node headers
   memcpy(pNewNode, pNode, sizeof(IX_PageHdr));
   ((IX_PageHdr *)pNewNode)->numKeys = ((IX_PageHdr *)pNode)->numKeys - pivot
                                       - insertLocation * insertLocation;
   ((IX_PageHdr *)pNode)->numKeys = pivot;

   // Maintain doubly linked list
   ((IX_PageHdr *)pNewNode)->prevNode = nodeNum;
   ((IX_PageHdr *)pNode)->nextNode = newNodeNum;

   if (nodeNum != 0 && ((IX_PageHdr *)pNewNode)->nextNode != IX_NO_MORE_NODE) {
      PF_PageHandle ph;
      char *pn;

      // Pin
      if (rc = pfFileHandle.GetThisPage(((IX_PageHdr *)pNewNode)->nextNode, ph))
         goto err_return;
      if (rc = ph.GetData(pn))
         goto err_return;

      ((IX_PageHdr *)pn)->prevNode = newNodeNum;

      // Unpin
      if (rc = pfFileHandle.MarkDirty(((IX_PageHdr *)pNewNode)->nextNode))
         goto err_return;
      if (rc = pfFileHandle.UnpinPage(((IX_PageHdr *)pNewNode)->nextNode))
         goto err_return;
   }

   // Insert
   if (insertLocation > 0) {
      if (rc = InsertEntryToIntlNodeNoSplit(newNodeNum, childNodeNum,
                                            splitKey, splitNodeNum))
         goto err_return;
   } else if (insertLocation < 0) {
      if (rc = InsertEntryToIntlNodeNoSplit(nodeNum, childNodeNum,
                                            splitKey, splitNodeNum))
         goto err_return;
   }

   //
   memcpy(&splitNodeNum, &newNodeNum, sizeof(PageNum));
   delete [] splitKey;
   splitKey = newSplitKey;

#ifdef DEBUG_IX
   assert(Compare(newSplitKey,
                  InternalKey(pNode, ((IX_PageHdr *)pNode)->numKeys - 1)) >= 0);
   assert(Compare(newSplitKey,
                  InternalKey(pNewNode, 0)) <= 0);
#endif

   // This INTERNAL node is ROOT!
   if (nodeNum == 0) {
      PF_PageHandle new2PageHandle;
      char *pNew2Node;
      PageNum new2NodeNum;

      // Allocate a new LEAF node
      if (rc = pfFileHandle.AllocatePage(new2PageHandle))
         goto err_return;
      if (rc = new2PageHandle.GetData(pNew2Node))
         goto err_return;
      if (rc = new2PageHandle.GetPageNum(new2NodeNum))
         goto err_return;

      // To assign page number 0 to root node
      memcpy(pNew2Node, pNode, PF_PAGE_SIZE);
      memset(pNode, 0, PF_PAGE_SIZE); 
      ((IX_PageHdr *)pNode)->flags = IX_INTERNAL_NODE;
      ((IX_PageHdr *)pNode)->numKeys = 1;
      ((IX_PageHdr *)pNode)->prevNode = attrType;
      ((IX_PageHdr *)pNode)->nextNode = attrLength;
      ((IX_PageHdr *)pNew2Node)->prevNode = IX_NO_MORE_NODE;
      ((IX_PageHdr *)pNewNode)->nextNode  = IX_NO_MORE_NODE;
      
      memcpy(InternalPtr(pNode, 0), &new2NodeNum, sizeof(PageNum));
      memcpy(InternalKey(pNode, 0), splitKey, attrLength);
      memcpy(InternalPtr(pNode, 1), &newNodeNum, sizeof(PageNum));

      //
      ((IX_PageHdr *)pNewNode)->prevNode = new2NodeNum;

      // Unpin
      if (rc = pfFileHandle.MarkDirty(new2NodeNum))
         goto err_return;
      if (rc = pfFileHandle.UnpinPage(new2NodeNum))
         goto err_return;

      // Reset
      splitNodeNum = IX_DONT_SPLIT;
      delete [] splitKey;
      splitKey = NULL;
   }

   // Unpin
   if (rc = pfFileHandle.MarkDirty(nodeNum))
      goto err_return;
   if (rc = pfFileHandle.MarkDirty(newNodeNum))
      goto err_return;
   if (rc = pfFileHandle.UnpinPage(nodeNum))
      goto err_return;
   if (rc = pfFileHandle.UnpinPage(newNodeNum))
      goto err_return;

   // Return ok
   return (0);

   // Return error
err_return:
#ifdef DEBUG_IX
   assert(0);
#endif
   return (rc);
}

//
// InsertEntryToLeafNode
//
RC IX_IndexHandle::InsertEntryToLeafNode(const PageNum nodeNum,
                                         void *pData, const RID &rid,
                                         char *&splitKey,
                                         PageNum &splitNodeNum,
                                         int originalLeaf)
{
   RC rc;
   PF_PageHandle pageHandle;
   char *pNode;
   int numKeys;
   int j;

   // Pin
   if (rc = pfFileHandle.GetThisPage(nodeNum, pageHandle))
      goto err_return;
   if (rc = pageHandle.GetData(pNode))
      goto err_return;

   // Read numKeys
   numKeys = ((IX_PageHdr *)pNode)->numKeys;

   // Key already exists?
   for (j = numKeys - 1; j >= 0; j--)
      if (Compare(pData, LeafKey(pNode, j)) == 0)
         break;

   if (j == -1) {
      // Key not found: we're good
      if (originalLeaf)
         goto do_unpin;
      // No more recursion, don't insert here and go back
      else {
         // Unpin
         if (rc = pfFileHandle.UnpinPage(nodeNum))
            goto err_return;

         return (-9999);
      }
   }

   // (Key,RID) already exists?
   for (; j >= 0; j--) {
      if (Compare(pData, LeafKey(pNode, j)) > 0)
         goto do_unpin;

      // Found
      if (memcmp(&rid, LeafRID(pNode, j), sizeof(RID)) == 0) {
         // Unpin
         if (rc = pfFileHandle.UnpinPage(nodeNum))
            goto err_return;

         return (IX_ENTRYEXISTS);
      }
   }

   // Need to proceed to the previous leaf node
   if (j == -1 && nodeNum != 0
       && ((IX_PageHdr *)pNode)->prevNode != IX_NO_MORE_NODE) {
      // Unpin
      if (rc = pfFileHandle.UnpinPage(nodeNum))
         goto err_return;

      rc = InsertEntryToLeafNode(((IX_PageHdr *)pNode)->prevNode,
                                 pData, rid,
                                 splitKey, splitNodeNum, FALSE);
      if (rc == -9999)
         goto do_insert;
      else if (rc)
         goto err_return;

      return (0);
   }

do_unpin:
   // Unpin
   if (rc = pfFileHandle.UnpinPage(nodeNum))
      goto err_return;
do_insert:
   // Just add new entry if possible
   if (IX_PAGEHDR_SIZE + (numKeys + 1) * LeafEntrySize() <= PF_PAGE_SIZE) {
      if (rc = InsertEntryToLeafNodeNoSplit(nodeNum, pData, rid,
                                            splitKey, splitNodeNum))
         goto err_return;
   }
   // Not enough room
   else {
      // Split LEAF node
      if (originalLeaf) {
         if (rc = InsertEntryToLeafNodeSplit(nodeNum, pData, rid, 
                                             splitKey, splitNodeNum))
            goto err_return;
      }
      // Don't split any other leaf nodes but the original one 
      else
         return (-9999);
   }

   // Return ok
   return (0);

   // Return error
err_return:
   return (rc);
}

//
// InsertEntryToLeafNodeNoSplit
//
RC IX_IndexHandle::InsertEntryToLeafNodeNoSplit(const PageNum nodeNum,
                                                void *pData, const RID &rid,
                                                char *&splitKey,
                                                PageNum &splitNodeNum)
{
   RC rc;
   PF_PageHandle pageHandle;
   char *pNode;
   int j;

   // Pin
   if (rc = pfFileHandle.GetThisPage(nodeNum, pageHandle))
      goto err_return;
   if (rc = pageHandle.GetData(pNode))
      goto err_return;

   // Find the right place
   for (j = 0; j < ((IX_PageHdr *)pNode)->numKeys; j++) {
      if (Compare(pData, LeafKey(pNode, j)) < 0) {
         // Make a "hole" for new entry
         memmove(LeafKey(pNode, j + 1),
                 LeafKey(pNode, j),
                 (((IX_PageHdr *)pNode)->numKeys - j)
                 * LeafEntrySize());
         break;
      }
   }

   // Fill out new key
   memcpy(LeafKey(pNode, j), pData, attrLength);
   // Fill out RID associated with new key
   memcpy(LeafRID(pNode, j), &rid, sizeof(RID));
   // Increment #keys
   ((IX_PageHdr *)pNode)->numKeys++;

   // Unpin
   if (rc = pfFileHandle.MarkDirty(nodeNum))
      goto err_return;
   if (rc = pfFileHandle.UnpinPage(nodeNum))
      goto err_return;

   // Reset
   splitNodeNum = IX_DONT_SPLIT;
   splitKey = NULL;

   // Return ok
   return (0);

   // Return error
err_return:
#ifdef DEBUG_IX
   assert(0);
#endif
   return (rc);
}

//
// InsertEntryToLeafNodeSplit
//
RC IX_IndexHandle::InsertEntryToLeafNodeSplit(const PageNum nodeNum,
                                              void *pData, const RID &rid,
                                              char *&splitKey,
                                              PageNum &splitNodeNum)
{
   RC rc;
   PF_PageHandle pageHandle;
   char *pNode;
   PF_PageHandle newPageHandle;
   char *pNewNode;
   PageNum newNodeNum;
   int insertLocation, pivot;

   // Pin
   if (rc = pfFileHandle.GetThisPage(nodeNum, pageHandle))
      goto err_return;
   if (rc = pageHandle.GetData(pNode))
      goto err_return;

   // Allocate a new LEAF node
   if (rc = pfFileHandle.AllocatePage(newPageHandle))
      goto err_return;
   if (rc = newPageHandle.GetData(pNewNode))
      goto err_return;
   if (rc = newPageHandle.GetPageNum(newNodeNum))
      goto err_return;

   // Select a boundary so that "half full" constraint is met
   pivot = ((IX_PageHdr *)pNode)->numKeys / 2;
   if (Compare(pData, LeafKey(pNode, pivot)) > 0) {
      pivot++;
      insertLocation = 1;
   } else
      insertLocation = -1;

   // Move a half of data to new node
   memcpy(pNewNode + IX_PAGEHDR_SIZE,
          LeafKey(pNode, pivot),
          (((IX_PageHdr *)pNode)->numKeys - pivot) * LeafEntrySize());

   // Write node headers
   memcpy(pNewNode, pNode, sizeof(IX_PageHdr));
   ((IX_PageHdr *)pNewNode)->numKeys = ((IX_PageHdr *)pNode)->numKeys - pivot;
   ((IX_PageHdr *)pNode)->numKeys = pivot;

   // Maintain doubly linked list
   ((IX_PageHdr *)pNewNode)->prevNode = nodeNum;
   ((IX_PageHdr *)pNode)->nextNode = newNodeNum;

   if (nodeNum != 0 && ((IX_PageHdr *)pNewNode)->nextNode != IX_NO_MORE_NODE) {
      PF_PageHandle ph;
      char *pn;

      // Pin
      if (rc = pfFileHandle.GetThisPage(((IX_PageHdr *)pNewNode)->nextNode, ph))
         goto err_return;
      if (rc = ph.GetData(pn))
         goto err_return;

      ((IX_PageHdr *)pn)->prevNode = newNodeNum;

      // Unpin
      if (rc = pfFileHandle.MarkDirty(((IX_PageHdr *)pNewNode)->nextNode))
         goto err_return;
      if (rc = pfFileHandle.UnpinPage(((IX_PageHdr *)pNewNode)->nextNode))
         goto err_return;
   }
      
   // Insert
   if (insertLocation > 0) {
      if (rc = InsertEntryToLeafNodeNoSplit(newNodeNum, pData, rid,
                                            splitKey, splitNodeNum))
         goto err_return;
   }
   else {
      if (rc = InsertEntryToLeafNodeNoSplit(nodeNum, pData, rid,
                                            splitKey, splitNodeNum))
         goto err_return;
   }

   // Choose a key to propagate to parent node
   memcpy(&splitNodeNum, &newNodeNum, sizeof(PageNum));
   splitKey = new char[MAXSTRINGLEN];
   memcpy(splitKey, LeafKey(pNewNode, 0), attrLength);

#ifdef DEBUG_IX
   assert(Compare(splitKey,
                  LeafKey(pNode, ((IX_PageHdr *)pNode)->numKeys - 1)) >= 0);
   assert(Compare(splitKey, LeafKey(pNewNode, 0)) <= 0);
#endif

   // This LEAF node is ROOT!
   if (nodeNum == 0) {
      PF_PageHandle new2PageHandle;
      char *pNew2Node;
      PageNum new2NodeNum;

      // Allocate a new LEAF node
      if (rc = pfFileHandle.AllocatePage(new2PageHandle))
         goto err_return;
      if (rc = new2PageHandle.GetData(pNew2Node))
         goto err_return;
      if (rc = new2PageHandle.GetPageNum(new2NodeNum))
         goto err_return;

      // To assign page number 0 to root node
      memcpy(pNew2Node, pNode, PF_PAGE_SIZE);
      memset(pNode, 0, PF_PAGE_SIZE); 
      ((IX_PageHdr *)pNode)->flags = IX_INTERNAL_NODE;
      ((IX_PageHdr *)pNode)->numKeys = 1;
      ((IX_PageHdr *)pNode)->prevNode = attrType;
      ((IX_PageHdr *)pNode)->nextNode = attrLength;
      ((IX_PageHdr *)pNew2Node)->prevNode = IX_NO_MORE_NODE;
      ((IX_PageHdr *)pNewNode)->nextNode  = IX_NO_MORE_NODE;
      
      memcpy(InternalPtr(pNode, 0), &new2NodeNum, sizeof(PageNum));
      memcpy(InternalKey(pNode, 0), splitKey, attrLength);
      memcpy(InternalPtr(pNode, 1), &newNodeNum, sizeof(PageNum));

      //
      ((IX_PageHdr *)pNewNode)->prevNode = new2NodeNum;

      // Unpin
      if (rc = pfFileHandle.MarkDirty(new2NodeNum))
         goto err_return;
      if (rc = pfFileHandle.UnpinPage(new2NodeNum))
         goto err_return;

      // Reset
      splitNodeNum = IX_DONT_SPLIT;
      delete [] splitKey;
      splitKey = NULL;
   }

   // Unpin
   if (rc = pfFileHandle.MarkDirty(nodeNum))
      goto err_return;
   if (rc = pfFileHandle.MarkDirty(newNodeNum))
      goto err_return;
   if (rc = pfFileHandle.UnpinPage(nodeNum))
      goto err_return;
   if (rc = pfFileHandle.UnpinPage(newNodeNum))
      goto err_return;

   // Return ok
   return (0);

   // Return error
err_return:
#ifdef DEBUG_IX
   assert(0);
#endif
   return (rc);
}

//
// DeleteEntry
//
// Desc: Delete an existing index entry from index.
//       pData/rid pair should exist in the index.
// In:   pData - key value
//       rid - record identifier
// Ret:  IX_NULLPOINTER, RM_INVIABLERID
//
RC IX_IndexHandle::DeleteEntry(void *pData, const RID &rid)
{
   RC rc;
   PageNum pageNum;
   char *tmp;
   
   // Sanity Check: pData must not be NULL
   if (pData == NULL)
      // Test: NULL pData
      return (IX_NULLPOINTER);
 
   // Sanity Check: RID must be viable
   if (rc = rid.GetPageNum(pageNum))
      // Test: inviable rid
      goto err_return;

   // Delete the entry from the B+ tree
   if (rc = DeleteEntryAtNode(0, pData, rid, tmp, pageNum))
      // Test: unopened indexHandle
      goto err_return;

#ifdef DEBUG_IX
   assert(tmp == NULL);
#endif

   // Return ok
   return (0);

   // Return error
err_return:
   return (rc);
}

//
// DeleteEntryAtNode
//
// Desc: Delete an existing index entry from a specific node or the
//       subtree accessible from it.
//       Lazy deletion.
//
//       If this node is internal: 
//         1. Find a subtree using the key.
//         2. Recursively call DeleteEntryAtNode() for the subtree.
//         3. If the smallest value of the subtree is changed, update
//            the value (if the traversed subtree is leftmost one,
//            just propagate this infomation to parent)
//         4. If the subtree is deleted, delete the corresponding 
//            key/pointer pair. At this step, the smallest value of
//            the subtree might be changed, too. If no pointer left, 
//            delete this node and propagate this information to parent.
//         Note: Different from insertion, the changed subtree might
//               not be accessible from pointers in this node because 
//               of duplicates. When necessary, the doubly linked 
//               list is used to handle this situation.
//
//       If this node is leaf:
//         1. Find the entry. It can be at previous node since we
//            traversed to the rightmost leaf node with key.
//         2. If 2+ entries, just delete the new entry. If the smallest
//            value of the leaf node is changed, propagate the new
//            smallest key to parent node.
//         3. Otherwise, delete the leaf node and propagate information 
//            to parent node so that the parent node can be updated
//            accordingly.
//
// In:   nodeNum - 
//       pData/rid - index entry to be deleted
// Out:  smallestKey - key/pointer pair which should be inserted to
//       deletedNodeNum - 
// Ret:  IX_ENTRYNOTFOUND, PF return code
//
RC IX_IndexHandle::DeleteEntryAtNode(const PageNum nodeNum,
                                     void *pData, const RID &rid,
                                     char *&smallestKey,
                                     PageNum &deletedNodeNum)
{
   RC rc;
   PF_PageHandle pageHandle;
   char *pNode;
   int j;

   // Pin
   if (rc = pfFileHandle.GetThisPage(nodeNum, pageHandle))
      goto err_return;
   if (rc = pageHandle.GetData(pNode))
      goto err_return;

   // Current node is LEAF node
   if (((IX_PageHdr *)pNode)->flags & IX_LEAF_NODE) {
      // Unpin
      if (rc = pfFileHandle.UnpinPage(nodeNum))
         goto err_return;

      //
      if (rc = DeleteEntryAtLeafNode(nodeNum, pData, rid,
                                     smallestKey, deletedNodeNum))
         goto err_return;
   }

   // Current node is INTERNAL node
   else {
      PageNum childNodeNum;

      // Find the right child to traverse
      for (j = 0; j < ((IX_PageHdr *)pNode)->numKeys; j++) {
         if (Compare(pData, InternalKey(pNode, j)) < 0)
            break;
      }
      memcpy(&childNodeNum, InternalPtr(pNode, j), sizeof(PageNum));

      // Unpin
      if (rc = pfFileHandle.UnpinPage(nodeNum))
         goto err_return;

      // Recursion
      if (rc = DeleteEntryAtNode(childNodeNum, pData, rid,
                                 smallestKey, deletedNodeNum))
         goto err_return;

      // Update the smallest key value
      if (smallestKey != NULL && j > 0) {
#ifdef DEBUG_IX
         assert(deletedNodeNum == IX_NOT_DELETED);
#endif
         // Pin
         if (rc = pfFileHandle.GetThisPage(nodeNum, pageHandle))
            goto err_return;
         if (rc = pageHandle.GetData(pNode))
            goto err_return;

         //
         memcpy(InternalKey(pNode, j - 1), smallestKey, attrLength);
         delete [] smallestKey;
         smallestKey = NULL;

         // Unpin
         if (rc = pfFileHandle.MarkDirty(nodeNum))
            goto err_return;
         if (rc = pfFileHandle.UnpinPage(nodeNum))
            goto err_return;
      }

      // Traversed child node was disposed
      if (deletedNodeNum != IX_NOT_DELETED)
         if (rc = DeleteEntryAtIntlNode(nodeNum, smallestKey, deletedNodeNum))
            goto err_return;
   }

   // Return ok
   return (0);

   // Return error
err_return:
   return (rc);
}

//
// DeleteEntryAtLeafNode
//
RC IX_IndexHandle::DeleteEntryAtIntlNode(const PageNum nodeNum,
                                         char *&smallestKey,
                                         PageNum &deletedNodeNum)
{
   RC rc;
   PF_PageHandle pageHandle;
   char *pNode;
   int j;

   // Pin
   if (rc = pfFileHandle.GetThisPage(nodeNum, pageHandle))
      goto err_return;
   if (rc = pageHandle.GetData(pNode))
      goto err_return;

   // Should respect the original order among duplicated keys
   for (j = ((IX_PageHdr *)pNode)->numKeys; j >= 0; j--)
      if (memcmp(InternalPtr(pNode, j), &deletedNodeNum, sizeof(PageNum)) == 0)
         break;

   // Need to proceed to the previous internal node
   if (j == -1 && nodeNum != 0) {
      PageNum prevNode = ((IX_PageHdr *)pNode)->prevNode;

      // Unpin
      if (rc = pfFileHandle.UnpinPage(nodeNum))
         goto err_return;

#ifdef DEBUG_IX
      assert(prevNode != IX_NO_MORE_NODE);
#endif
      return DeleteEntryAtIntlNode(prevNode, smallestKey, deletedNodeNum);
   }
   // Found
   else {
      // The only pointer
      if (((IX_PageHdr *)pNode)->numKeys == 0) {
         assert(nodeNum != 0);
         deletedNodeNum = nodeNum;
         LinkTwoNodesEachOther(((IX_PageHdr *)pNode)->prevNode,
                               ((IX_PageHdr *)pNode)->nextNode);

         // Dispose
         if (rc = pfFileHandle.UnpinPage(nodeNum))
            goto err_return;
         if (rc = pfFileHandle.DisposePage(nodeNum))
            goto err_return;
      }
      // Not the only key
      else {
         deletedNodeNum = IX_NOT_DELETED;

         // Compute the smallest value 
         if (j == 0 && nodeNum != 0
             && ((IX_PageHdr *)pNode)->prevNode != IX_NO_MORE_NODE) {
            smallestKey = new char[MAXSTRINGLEN];
            memcpy(smallestKey, InternalKey(pNode, 0), attrLength);
            
            // Remove the found entry
            memmove(InternalPtr(pNode, 0),
                    InternalPtr(pNode, 1),
                    (((IX_PageHdr *)pNode)->numKeys) * InternalEntrySize());
         } else {
            smallestKey = NULL;

            // Remove the found entry
            memmove(InternalKey(pNode, j - 1),
                    InternalKey(pNode, j),
                    (((IX_PageHdr *)pNode)->numKeys - j) * InternalEntrySize());
         }

         // Decrement #keys
         ((IX_PageHdr *)pNode)->numKeys--;

         // Only one pointer remained at the ROOT node!
         if (nodeNum == 0 && ((IX_PageHdr *)pNode)->numKeys == 0) {
            PageNum childNodeNum;
            PageNum rootNodeNum;
            char *pRootNode;

            memcpy(&childNodeNum, InternalPtr(pNode, 0), sizeof(PageNum));
            if (rc = FindNewRootNode(childNodeNum, rootNodeNum, pRootNode))
               goto err_return;

            // Move the new root node to page 0
            memcpy(pNode, pRootNode, PF_PAGE_SIZE);
            ((IX_PageHdr *)pNode)->prevNode = attrType;
            ((IX_PageHdr *)pNode)->nextNode = attrLength;

            // Unpin
            if (rc = pfFileHandle.UnpinPage(rootNodeNum))
               goto err_return;
            if (rc = pfFileHandle.DisposePage(rootNodeNum))
               goto err_return;
         }

         // Unpin
         if (rc = pfFileHandle.MarkDirty(nodeNum))
            goto err_return;
         if (rc = pfFileHandle.UnpinPage(nodeNum))
            goto err_return;
      }
   }

   // Return ok
   return (0);

   // Return error
err_return:
#ifdef DEBUG_IX
   assert(0);
#endif
   return (rc);
}

//
// DeleteEntryAtLeafNode
//
RC IX_IndexHandle::DeleteEntryAtLeafNode(const PageNum nodeNum,
                                         void *pData, const RID &rid,
                                         char *&smallestKey,
                                         PageNum &deletedNodeNum)
{
   RC rc;
   PF_PageHandle pageHandle;
   char *pNode;
   int j;

   // Pin
   if (rc = pfFileHandle.GetThisPage(nodeNum, pageHandle))
      goto err_return;
   if (rc = pageHandle.GetData(pNode))
      goto err_return;

   // Find the right place (by search key)
   for (j = ((IX_PageHdr *)pNode)->numKeys - 1; j >= 0; j--)
      if (Compare(pData, LeafKey(pNode, j)) == 0)
         break;

   // Search key not found
   if (j == -1) {
      // Unpin
      if (rc = pfFileHandle.UnpinPage(nodeNum))
         goto err_return;

      return (IX_ENTRYNOTFOUND);
   }

   // Find the right place (by RID)
   for (; j >= 0; j--) {
      if (Compare(pData, LeafKey(pNode, j)) > 0) {
         // Unpin
         if (rc = pfFileHandle.UnpinPage(nodeNum))
            goto err_return;

         return (IX_ENTRYNOTFOUND);
      }

      // Found
      if (memcmp(&rid, LeafRID(pNode, j), sizeof(RID)) == 0)
         break;
   }

   // Need to proceed to the previous leaf node
   if (j == -1 && nodeNum != 0) {
      PageNum prevNode = ((IX_PageHdr *)pNode)->prevNode;
      
      // Unpin
      if (rc = pfFileHandle.UnpinPage(nodeNum))
         goto err_return;

      // No more node
      if (prevNode == IX_NO_MORE_NODE)
         return (IX_ENTRYNOTFOUND);

      return DeleteEntryAtLeafNode(prevNode, pData, rid,
                                   smallestKey, deletedNodeNum);
   }
   // Found
   else {
      // The only key
      if (((IX_PageHdr *)pNode)->numKeys == 1 && nodeNum != 0) {
#ifdef DEBUG_IX
         assert(j == 0);
#endif
         deletedNodeNum = nodeNum;
         smallestKey = NULL;

         LinkTwoNodesEachOther(((IX_PageHdr *)pNode)->prevNode,
                               ((IX_PageHdr *)pNode)->nextNode);

         // Dispose
         if (rc = pfFileHandle.UnpinPage(nodeNum))
            goto err_return;
         if (rc = pfFileHandle.DisposePage(nodeNum))
            goto err_return;
      }
      // Not the only key
      else {
         deletedNodeNum = IX_NOT_DELETED;

         // Compute the smallest value 
         if (j == 0 && nodeNum != 0
             && ((IX_PageHdr *)pNode)->prevNode != IX_NO_MORE_NODE
             && Compare(LeafKey(pNode, 0), LeafKey(pNode, 1)) < 0) {
            smallestKey = new char[MAXSTRINGLEN];
            memcpy(smallestKey, LeafKey(pNode, 1), attrLength);
         } else
            smallestKey = NULL;

         // Remove the found entry
         memmove(LeafKey(pNode, j),
                 LeafKey(pNode, j + 1),
                 (((IX_PageHdr *)pNode)->numKeys - j - 1) * LeafEntrySize());
         // Decrement #keys
         ((IX_PageHdr *)pNode)->numKeys--;

         // Unpin
         if (rc = pfFileHandle.MarkDirty(nodeNum))
            goto err_return;
         if (rc = pfFileHandle.UnpinPage(nodeNum))
            goto err_return;
      }
   }

   // Return ok
   return (0);

   // Return error
err_return:
#ifdef DEBUG_IX
   assert(0);
#endif
   return (rc);
}

//
// LinkTwoNodesEachOther
//
// Desc: Link two nodes (at the same depth) each other
//       Maintains doubly linked list when a node is deleted
//       Called by DeleteEntryAtIntlNode() and DeleteEntryAtLeafNode()
// Ret:  PF return code
//
RC IX_IndexHandle::LinkTwoNodesEachOther(const PageNum prevNode,
                                         const PageNum nextNode)
{
   RC rc;
   PF_PageHandle ph;
   char *pn;
   
   if (prevNode != IX_NO_MORE_NODE) {
      // Pin
      if (rc = pfFileHandle.GetThisPage(prevNode, ph))
         goto err_return;
      if (rc = ph.GetData(pn))
         goto err_return;
   
      // Link
      ((IX_PageHdr *)pn)->nextNode = nextNode;
   
      // Unpin
      if (rc = pfFileHandle.MarkDirty(prevNode))
         goto err_return;
      if (rc = pfFileHandle.UnpinPage(prevNode))
         goto err_return;
   }
   
   if (nextNode != IX_NO_MORE_NODE) {
      // Pin
      if (rc = pfFileHandle.GetThisPage(nextNode, ph))
         goto err_return;
      if (rc = ph.GetData(pn))
         goto err_return;
   
      // Link
      ((IX_PageHdr *)pn)->prevNode = prevNode;
   
      // Unpin
      if (rc = pfFileHandle.MarkDirty(nextNode))
         goto err_return;
      if (rc = pfFileHandle.UnpinPage(nextNode))
         goto err_return;
   }

   // Return ok
   return (0);

   // Return error
err_return:
#ifdef DEBUG_IX
   assert(0);
#endif
   return (rc);
}

//
// FindNewRootNode
//
// Desc: Recursively find the new root node when the old root node has 
//       only one pointer due to deletions.
//       Dispose every node on the path from the old root to the new root.
//       Unpin is handled by caller.
//       Called by DeleteEntryAtIntlNode()
// In:   nodeNum
// Out:  rootNodeNum - set to the page number of new root node
//       pRootNode - set to the data pointer of new root node
// Ret:  PF return code
//
RC IX_IndexHandle::FindNewRootNode(const PageNum nodeNum,
                                   PageNum &rootNodeNum,
                                   char *&pRootNode)
{
   RC rc;
   PF_PageHandle pageHandle;
   char *pNode;

   // Pin
   if (rc = pfFileHandle.GetThisPage(nodeNum, pageHandle))
      goto err_return;
   if (rc = pageHandle.GetData(pNode))
      goto err_return;

   // Current node is LEAF node
   if (((IX_PageHdr *)pNode)->flags & IX_LEAF_NODE) {
      // New root (leaf node)
      rootNodeNum = nodeNum;
      pRootNode = pNode;
   }

   // Current node is INTERNAL node
   else {
      // New root (internal node)
      if (((IX_PageHdr *)pNode)->numKeys > 0) {
         rootNodeNum = nodeNum;
         pRootNode = pNode;
      }
      // Dispose the current node and recurse if there is only one pointer
      else {
         PageNum childNodeNum;
         memcpy(&childNodeNum, InternalPtr(pNode, 0), sizeof(PageNum));

         // Dispose
         if (rc = pfFileHandle.UnpinPage(nodeNum))
            goto err_return;
         if (rc = pfFileHandle.DisposePage(nodeNum))
            goto err_return;

         // Recursion
         if (rc = FindNewRootNode(childNodeNum, rootNodeNum, pRootNode))
            goto err_return;
      }
   }

   // Return ok
   return (0);

   // Return error
err_return:
   return (rc);
}

//
// ForcePages
//
// Desc: 
// Ret:  PF return code
//
RC IX_IndexHandle::ForcePages()
{
   RC rc;
   
   // Call PF_FileHandle::ForcePages()
   if (rc = pfFileHandle.ForcePages())
      goto err_return;

   // Return ok
   return (0);

   // Return error
err_return:
   return (rc);
}

//
// InternalEntrySize, InternalKey, InternalPtr
//
// Desc: Compute various size/pointer for an internal node
// In:   base - pointer returned by PF_PageHandle.GetData()
//       idx - entry index
// Ret:  
//
inline int IX_IndexHandle::InternalEntrySize(void)
{
   return sizeof(PageNum) + attrLength;
}

inline char* IX_IndexHandle::InternalPtr(char *base, int idx)
{
   return base + IX_PAGEHDR_SIZE + idx * InternalEntrySize();
}

inline char* IX_IndexHandle::InternalKey(char *base, int idx)
{
   return InternalPtr(base, idx) + sizeof(PageNum);
}

//
// LeafEntrySize, LeafKey, LeafRID
//
// Desc: Compute various size/pointer for a leaf node
// In:   base - pointer returned by PF_PageHandle.GetData()
//       idx - entry index
// Ret:  
//
inline int IX_IndexHandle::LeafEntrySize(void)
{
   return attrLength + sizeof(RID);
}

inline char* IX_IndexHandle::LeafKey(char *base, int idx)
{
   return base + IX_PAGEHDR_SIZE + idx * LeafEntrySize();
}

inline char* IX_IndexHandle::LeafRID(char *base, int idx)
{
   return LeafKey(base, idx) + attrLength;
}

//
// Compare
//
float IX_IndexHandle::Compare(void *_value, char *value)
{
   float cmp;
   int i;
   float f;

   // Do comparison according to the attribute type
   switch (attrType) {
   case INT:
      memcpy(&i, _value, sizeof(int));
      cmp = i - *((int *)value);
      break;

   case FLOAT:
      memcpy(&f, _value, sizeof(float));
      cmp = f - *((float *)value);
      break;

   case STRING:
      cmp = memcmp(_value, value, attrLength);
      break;
   }

   return cmp;
}

#ifdef DEBUG_IX
//
// PrintNode
//
RC IX_IndexHandle::PrintNode(const PageNum pNum)
{
   RC rc;
   PF_PageHandle pageHandle;
   char *pNode;
   int j;
   int i;
   float f;
   char s[MAXSTRINGLEN+1];

   // Pin
   if (rc = pfFileHandle.GetThisPage(pNum, pageHandle))
      goto err_return;
   if (rc = pageHandle.GetData(pNode))
      goto err_return;

   // Current node is LEAF node
   if (((IX_PageHdr *)pNode)->flags & IX_LEAF_NODE) {
      RID rid;
      PageNum pn;
      SlotNum sn;

      printf("Leaf (%d) %u (%d): ", 
             ((IX_PageHdr *)pNode)->prevNode, pNum,
             ((IX_PageHdr *)pNode)->nextNode);

      for (j = 0; j < ((IX_PageHdr *)pNode)->numKeys; j++) {
         switch (attrType) {
         case INT:
            memcpy(&i, LeafKey(pNode, j), sizeof(int));
            printf("[%d] ", i);
            break;
         case FLOAT:
            memcpy(&f, LeafKey(pNode, j), sizeof(float));
            printf("[%f] ", f);
            break;
         case STRING:
            strncpy(s, LeafKey(pNode, j), attrLength);
            printf("[%s] ", s);
            break;
         }

         memcpy(&rid, LeafRID(pNode, j), sizeof(RID));
         rid.GetPageNum(pn);
         rid.GetSlotNum(sn);
         printf("(%d,%d) ", pn, sn);
      }
      printf("\n");
   }
   // Current node is INTERNAL node
   else {
      PageNum pn;

      printf("Intl (%d) %u (%d): ", 
             ((IX_PageHdr *)pNode)->prevNode, pNum,
             ((IX_PageHdr *)pNode)->nextNode);

      for (j = 0; j < ((IX_PageHdr *)pNode)->numKeys; j++) {
         memcpy(&pn, InternalPtr(pNode, j), sizeof(PageNum));
         printf("%u ", pn);

         switch (attrType) {
         case INT:
            memcpy(&i, InternalKey(pNode, j), sizeof(int));
            printf("<%d> ", i);
            break;
         case FLOAT:
            memcpy(&f, InternalKey(pNode, j), sizeof(float));
            printf("<%f> ", f);
            break;
         case STRING:
            strncpy(s, InternalKey(pNode, j), attrLength);
            printf("<%s> ", s);
            break;
         }
      }
      memcpy(&pn, InternalPtr(pNode, j), sizeof(PageNum));
      printf("%u\n", pn);
   }

   // Unpin
   if (rc = pfFileHandle.UnpinPage(pNum))
      goto err_return;

   // Return ok
   return (0);

   // Return error
err_return:
   return (rc);
}

//
// VerifyOrder
//
RC IX_IndexHandle::VerifyOrder(PageNum nodeNum)
{
   RC rc;
   PF_PageHandle pageHandle;
   char *pNode;
   int j;
   int i;
   int iOld = 0x80000000;
   float f;
   float fOld = -1e32;
   char s[MAXSTRINGLEN]; 
   PageNum nextNode;
   PageNum prevNode = IX_NO_MORE_NODE;

   // Pin
   if (rc = pfFileHandle.GetThisPage(nodeNum, pageHandle))
      goto err_return;
   if (rc = pageHandle.GetData(pNode))
      goto err_return;

   // Current node is LEAF node
   if (((IX_PageHdr *)pNode)->flags & IX_LEAF_NODE) {
      // Unpin
      if (rc = pfFileHandle.UnpinPage(nodeNum))
         goto err_return;

      do {
         // Pin
         if (rc = pfFileHandle.GetThisPage(nodeNum, pageHandle))
            goto err_return;
         if (rc = pageHandle.GetData(pNode))
            goto err_return;
   
         //
         assert(nodeNum == 0 || ((IX_PageHdr *)pNode)->prevNode == prevNode);

         for (j = 0; j < ((IX_PageHdr *)pNode)->numKeys; j++) {
            switch (attrType) {
            case INT:
               memcpy(&i, LeafKey(pNode, j), sizeof(int));
//             printf("[%d] ", i);
               assert(iOld <= i);
               iOld = i;
               break;
            case FLOAT:
               memcpy(&f, LeafKey(pNode, j), sizeof(float));
//             printf("[%f] ", f);
               assert(fOld <= f);
               fOld = f;
               break;
            case STRING:
               strncpy(s, LeafKey(pNode, j), attrLength);
//             printf("[%s] ", s);
               break;
            }
   
            RID rid;
            PageNum pn;
            SlotNum sn;

            memcpy(&rid, LeafRID(pNode, j), sizeof(RID));
            rid.GetPageNum(pn);
            rid.GetSlotNum(sn);
//          printf("(%d,%d)\n", pn, sn);
         }
         nextNode = ((IX_PageHdr *)pNode)->nextNode;
         
         // Unpin
         if (rc = pfFileHandle.UnpinPage(nodeNum))
            goto err_return;
   
         if (nodeNum == 0)
            break;

         prevNode = nodeNum;
         nodeNum = nextNode;
      }
      while (nodeNum != IX_NO_MORE_NODE);
   }

   // Current node is INTERNAL node
   else {
      PageNum childNodeNum;
      memcpy(&childNodeNum, InternalPtr(pNode, 0), sizeof(PageNum));

      // Unpin
      if (rc = pfFileHandle.UnpinPage(nodeNum))
         goto err_return;

      // Recursion
      if (rc = VerifyOrder(childNodeNum))
         goto err_return;
   }

   // Return ok
   return (0);

   // Return error
err_return:
   assert(0);
   return (rc);
}

//
// VerifyStructure
//
RC IX_IndexHandle::VerifyStructure(const PageNum nodeNum)
{
   RC rc;
   PF_PageHandle pageHandle;
   char *pNode;

   // Pin
   if (rc = pfFileHandle.GetThisPage(nodeNum, pageHandle))
      goto err_return;
   if (rc = pageHandle.GetData(pNode))
      goto err_return;

   // Current node is LEAF node
   if (((IX_PageHdr *)pNode)->flags & IX_LEAF_NODE) {
      // Unpin
      if (rc = pfFileHandle.UnpinPage(nodeNum))
         goto err_return;
   }

   // Current node is INTERNAL node
   else {
      int j;

      for (j = 0; j < ((IX_PageHdr *)pNode)->numKeys; j++) {
         PageNum childNodeNum;
         char *key;

         memcpy(&childNodeNum, InternalPtr(pNode, j + 1), sizeof(PageNum));
         if (rc = GetSmallestKey(childNodeNum, key))
            goto err_return;
         assert(memcmp(key, InternalKey(pNode, j), attrLength) == 0);
         delete [] key;
      }

      for (j = 0; j <= ((IX_PageHdr *)pNode)->numKeys; j++) {
         PageNum childNodeNum;

         memcpy(&childNodeNum, InternalPtr(pNode, j), sizeof(PageNum));
         if (rc = VerifyStructure(childNodeNum))
            goto err_return;
      }

      // Unpin
      if (rc = pfFileHandle.UnpinPage(nodeNum))
         goto err_return;
   }

   // Return ok
   return (0);

   // Return error
err_return:
   assert(0);
   return (rc);
}

//
// GetSmallestKey
//
RC IX_IndexHandle::GetSmallestKey(const PageNum nodeNum, char *&key)
{
   RC rc;
   PF_PageHandle pageHandle;
   char *pNode;

   // Pin
   if (rc = pfFileHandle.GetThisPage(nodeNum, pageHandle))
      goto err_return;
   if (rc = pageHandle.GetData(pNode))
      goto err_return;

   // Current node is LEAF node
   if (((IX_PageHdr *)pNode)->flags & IX_LEAF_NODE) {
      // Copy the smallest key
      key = new char[MAXSTRINGLEN];
      memcpy(key, LeafKey(pNode, 0), attrLength);

      // Unpin
      if (rc = pfFileHandle.UnpinPage(nodeNum))
         goto err_return;
   }

   // Current node is INTERNAL node
   else {
      // Follow the most left pointer
      PageNum childNodeNum;
      memcpy(&childNodeNum, InternalPtr(pNode, 0), sizeof(PageNum));

      // Unpin
      if (rc = pfFileHandle.UnpinPage(nodeNum))
         goto err_return;

      // Recursion
      if (rc = GetSmallestKey(childNodeNum, key))
         goto err_return;
   }

   // Return ok
   return (0);

   // Return error
err_return:
   assert(0);
   return (rc);
}
#endif

