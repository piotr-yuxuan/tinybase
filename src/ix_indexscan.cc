//
// File:        ix_filescan.cc
// Description: IX_IndexScan class implementation
// Author:      Hyunjung Park (hyunjung@stanford.edu)
//

#include "ix_internal.h"

// 
// IX_IndexScan
//
// Desc: Default Constructor
//
IX_IndexScan::IX_IndexScan()
{
   // Initialize member variables
   bScanOpen = FALSE;
   curNodeNum = 0;
   curEntry = 0;

   pIndexHandle = NULL;
   compOp = NO_OP;
   value = NULL;
   pinHint = NO_HINT;
}

// 
// ~IX_IndexScan
//
// Desc: Destructor
//
IX_IndexScan::~IX_IndexScan()         
{
   // Don't need to do anything
}

//
// OpenScan
//
// Desc: Open an index scan with the given indexHandle and scan condition
// In:   indexHandle - IX_IndexHandle object (must be open)
//       _compOp     - EQ_OP|LT_OP|GT_OP|LE_OP|GE_OP|NO_OP (excludes NE_OP)
//       _value      - points to the value which will be compared with
//                     the index keys
//       _pinHint    - not implemented yet
// Ret:  IX_SCANOPEN, IX_CLOSEDFILE, IX_NULLPOINTER, IX_INVALIDCOMPOP
//
RC IX_IndexScan::OpenScan(const IX_IndexHandle &indexHandle,
#ifdef IX_DEFAULT_COMPOP
                          void *_value, CompOp _compOp,
#else
                          CompOp _compOp, void *_value,
#endif
                          ClientHint _pinHint)
{
   RC rc;
   RID zeroRid(0,0);

   // Sanity Check: 'this' should not be open yet
   if (bScanOpen)
      // Test: opened IX_IndexScan
      return (IX_SCANOPEN);

   // Sanity Check: indexHandle must be open
   if (indexHandle.attrLength == 0) // a little tricky here
      // Test: unopened indexHandle
      return (IX_CLOSEDFILE);

   // Sanity Check: compOp, value
   switch (_compOp) {
   case EQ_OP:
   case LT_OP:
   case GT_OP:
   case LE_OP:
   case GE_OP:
      // Sanity Check: value must not be NULL
      if (_value == NULL)
         // Test: null _value
         return (IX_NULLPOINTER);
   case NO_OP:
      break;

   default:
      return (IX_INVALIDCOMPOP);
   }
   
   // Copy parameters to local variable
   pIndexHandle = (IX_IndexHandle *)&indexHandle;
   compOp       = _compOp;
   value        =  _value;
   pinHint      = _pinHint;

   // Set local state variables
   bScanOpen = TRUE;
   curNodeNum = 0;
   curEntry = 0;
   lastRid = zeroRid;

   //
   if (rc = FindEntryAtNode(0))
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
// FindEntryAtNode
//
// Desc: 
// In:   nodeNum -
// Ret:  PF reeturn code
//
RC IX_IndexScan::FindEntryAtNode(PageNum nodeNum)
{
   RC rc;
   PF_PageHandle pageHandle;
   char *pNode;
   int numKeys;

   // Pin
   if (rc = pIndexHandle->pfFileHandle.GetThisPage(nodeNum, pageHandle))
      goto err_return;
   if (rc = pageHandle.GetData(pNode))
      goto err_return;

   // Read numKeys
   numKeys = ((IX_PageHdr *)pNode)->numKeys;

   // Current node is LEAF node
   if (((IX_PageHdr *)pNode)->flags & IX_LEAF_NODE) {
      float cmp;

      // Root leaf node can have no keys at all
      if (numKeys == 0) {
         curNodeNum = IX_NO_MORE_NODE;
         // Unpin
         if (rc = pIndexHandle->pfFileHandle.UnpinPage(nodeNum))
            goto err_return;
         return (0);
      }

      // Find the first entry
      switch (compOp) {
      case NO_OP:
         curEntry = 0;
         curNodeNum = nodeNum;
         break;

      case LE_OP:
         curEntry = 0;
         curNodeNum = (Compare(value, LeafKey(pNode, 0)) >= 0) ? nodeNum 
                                                               : IX_NO_MORE_NODE;
         break;

      case LT_OP:
         curEntry = 0;
         curNodeNum = (Compare(value, LeafKey(pNode, 0)) > 0) ? nodeNum 
                                                              : IX_NO_MORE_NODE;
         break;

      case EQ_OP:
         for (curEntry = 0; curEntry < numKeys; curEntry++)
            if ((cmp = Compare(value, LeafKey(pNode, curEntry))) <= 0)
               break;
         if (curEntry == numKeys || cmp < 0) {
            curNodeNum = IX_NO_MORE_NODE;
         } else {
            if (curEntry == 0 && nodeNum != 0
                && ((IX_PageHdr *)pNode)->prevNode != IX_NO_MORE_NODE) {
               PageNum prevNode = ((IX_PageHdr *)pNode)->prevNode;
               // Unpin
               if (rc = pIndexHandle->pfFileHandle.UnpinPage(nodeNum))
                  goto err_return;
               // Recursively find the first occurrence at the previous node
               if (rc = FindEntryAtNode(prevNode))
                  goto err_return;
               // Key doesn't exist at the previous node
               if (curNodeNum == IX_NO_MORE_NODE) {
                  curEntry = 0;
                  curNodeNum = nodeNum;
               }
               return (0);
            } else {
               curNodeNum = nodeNum;
            }
         }
         break;

      case GE_OP:
         for (curEntry = 0; curEntry < numKeys; curEntry++)
            if (Compare(value, LeafKey(pNode, curEntry)) <= 0)
               break;
         if (curEntry == numKeys) {
            curNodeNum = (nodeNum == 0) ? -1 : ((IX_PageHdr *)pNode)->nextNode;
            curEntry = 0;
         } else {
            if (curEntry == 0 && nodeNum != 0
                && ((IX_PageHdr *)pNode)->prevNode != IX_NO_MORE_NODE) {
               PageNum prevNode = ((IX_PageHdr *)pNode)->prevNode;
               // Unpin
               if (rc = pIndexHandle->pfFileHandle.UnpinPage(nodeNum))
                  goto err_return;
               return FindEntryAtNode(prevNode);
            } else {
               curNodeNum = nodeNum;
            }
         }
         break;

      case GT_OP:
         for (curEntry = 0; curEntry < numKeys; curEntry++)
            if (Compare(value, LeafKey(pNode, curEntry)) < 0)
               break;
#ifdef DEBUG_IX
         if (curEntry == 0 && nodeNum != 0)
            assert(((IX_PageHdr *)pNode)->prevNode == IX_NO_MORE_NODE);
#endif
         if (curEntry == numKeys) {
            curNodeNum = (nodeNum == 0) ? IX_NO_MORE_NODE 
                                        : ((IX_PageHdr *)pNode)->nextNode;
            curEntry = 0;
         } else {
            curNodeNum = nodeNum;
         }
         break;
      }

      // Unpin
      if (rc = pIndexHandle->pfFileHandle.UnpinPage(nodeNum))
         goto err_return;
   }

   // Current node is INTERNAL node
   else {
      PageNum childNodeNum;
      int j;
 
      // Find the appropriate child to traverse
      switch (compOp) {
      case NO_OP:
      case LE_OP:
      case LT_OP:
         memcpy(&childNodeNum, InternalPtr(pNode, 0), sizeof(PageNum));
         break;
      case EQ_OP:
      case GE_OP:
      case GT_OP:
         for (j = 0; j < numKeys; j++)
            if (Compare(value, InternalKey(pNode, j)) < 0)
               break;
         memcpy(&childNodeNum, InternalPtr(pNode, j), sizeof(PageNum));
         break;
      }

      // Unpin
      if (rc = pIndexHandle->pfFileHandle.UnpinPage(nodeNum))
         goto err_return;

      // Recursively call FindEntryAtNode()
      if (rc = FindEntryAtNode(childNodeNum))
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
// GetNextEntry
//
// Desc: 
// Out:  rid - 
// Ret:  IX_CLOSEDSCAN, IX_EOF
//
RC IX_IndexScan::GetNextEntry(RID &rid)
{
   RC rc;
   PF_PageHandle pageHandle;
   char *pNode;
   PageNum nextNodeNum;

   // Sanity Check: 'this' must be open
   if (!bScanOpen)
      // Test: closed IX_IndexScan
      return (IX_CLOSEDSCAN);

   // EOF
   if (curNodeNum == IX_NO_MORE_NODE)
      return (IX_EOF);

   // Pin
pin:
   if (rc = pIndexHandle->pfFileHandle.GetThisPage(curNodeNum, pageHandle)) {
      // When the last leaf node become root node due to deletion,
      // curNodeNum must be invalid.
#ifdef DEBUG_IX
      assert(curNodeNum != 0);
#endif
      curNodeNum = 0;
      goto pin;
   }
   if (rc = pageHandle.GetData(pNode))
      goto err_return;

   if (curEntry == 0) {
      switch (compOp) {
      case EQ_OP:
         if (Compare(value, LeafKey(pNode, curEntry)) != 0) {
            // Unpin
            if (rc = pIndexHandle->pfFileHandle.UnpinPage(curNodeNum))
               goto err_return;
            return (IX_EOF);
         }
         break;
      case LE_OP:
         if (Compare(value, LeafKey(pNode, curEntry)) < 0) {
            // Unpin
            if (rc = pIndexHandle->pfFileHandle.UnpinPage(curNodeNum))
               goto err_return;
            return (IX_EOF);
         }
         break;
      case LT_OP:
         if (Compare(value, LeafKey(pNode, curEntry)) <= 0) {
            // Unpin
            if (rc = pIndexHandle->pfFileHandle.UnpinPage(curNodeNum))
               goto err_return;
            return (IX_EOF);
         }
         break;
      default:
         break;
      }
   }

   // Copy rid
   memcpy(&rid, LeafRID(pNode, curEntry), sizeof(RID));
   if (rid == lastRid) {
      curEntry++;
      memcpy(&rid, LeafRID(pNode, curEntry), sizeof(RID));
   }
   lastRid = rid;

   // Advance the pointer
   if (curEntry == ((IX_PageHdr *)pNode)->numKeys - 1) {
      nextNodeNum = (curNodeNum == 0) ? IX_NO_MORE_NODE 
                                      : ((IX_PageHdr *)pNode)->nextNode;
      curEntry = 0;
   } else {
      nextNodeNum = curNodeNum;
      switch (compOp) {
      case EQ_OP:
         if (Compare(value, LeafKey(pNode, curEntry + 1)) != 0)
            nextNodeNum = IX_NO_MORE_NODE;
         break;
      case LE_OP:
         if (Compare(value, LeafKey(pNode, curEntry + 1)) < 0)
            nextNodeNum = IX_NO_MORE_NODE;
         break;
      case LT_OP:
         if (Compare(value, LeafKey(pNode, curEntry + 1)) <= 0)
            nextNodeNum = IX_NO_MORE_NODE;
         break;
      default:
         break;
      }
   }
   
   // Unpin
   if (rc = pIndexHandle->pfFileHandle.UnpinPage(curNodeNum))
      goto err_return;

   curNodeNum = nextNodeNum;

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
// CloseScan
//
// Desc: Close an index scan
// Ret:  IX_CLOSEDSCAN
//
RC IX_IndexScan::CloseScan()
{
   RID zeroRid(0,0);

   // Sanity Check: 'this' must be open
   if (!bScanOpen)
      // Test: closed IX_IndexScan
      return (IX_CLOSEDSCAN);

   // Reset member variables
   bScanOpen = FALSE;
   curNodeNum = 0;
   curEntry = 0;
   lastRid = zeroRid;

   pIndexHandle = NULL;
   compOp = NO_OP;
   value = NULL;
   pinHint = NO_HINT;

   // Return ok
   return (0);
}

//
// InternalEntrySize, InternalKey, InternalPtr
//
// Desc: Compute various size/pointer for an internal node
// In:   base - pointer returned by PF_PageHandle.GetData()
//       idx - entry index
// Ret:  
//
inline int IX_IndexScan::InternalEntrySize(void)
{
   return sizeof(PageNum) + pIndexHandle->attrLength;
}

inline char* IX_IndexScan::InternalPtr(char *base, int idx)
{
   return base + IX_PAGEHDR_SIZE + idx * InternalEntrySize();
}

inline char* IX_IndexScan::InternalKey(char *base, int idx)
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
inline int IX_IndexScan::LeafEntrySize(void)
{
   return pIndexHandle->attrLength + sizeof(RID);
}

inline char* IX_IndexScan::LeafKey(char *base, int idx)
{
   return base + IX_PAGEHDR_SIZE + idx * LeafEntrySize();
}

inline char* IX_IndexScan::LeafRID(char *base, int idx)
{
   return LeafKey(base, idx) + pIndexHandle->attrLength;
}

//
// Compare
//
float IX_IndexScan::Compare(void *_value, char *value1)
{
   float cmp;
   int i;
   float f;

   // Do comparison according to the attribute type
   switch (pIndexHandle->attrType) {
   case INT:
      memcpy(&i, _value, sizeof(int));
      cmp = i - *((int *)value1);
      break;

   case FLOAT:
      memcpy(&f, _value, sizeof(float));
      cmp = f - *((float *)value1);
      break;

   case STRING:
      cmp = memcmp(_value, value1, pIndexHandle->attrLength);
      break;
   }

   return cmp;
}

