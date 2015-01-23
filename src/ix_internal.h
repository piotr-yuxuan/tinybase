//
// File:        ix_internal.h
// Description: Declarations internal to the IX component
// Author:      Hyunjung Park (hyunjung@stanford.edu)
//

#ifndef IX_INTERNAL_H
#define IX_INTERNAL_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <cassert>
#include "ix.h"

//
// Constants and defines
//
#define IX_DONT_SPLIT     -1       // for splitNodeNum
#define IX_NOT_DELETED    -1       // for deletedNodeNum
#define IX_NO_MORE_NODE   -1       // for prevNode, nextNode

#define IX_INTERNAL_NODE  0x00     // internal node of B+ tree
#define IX_LEAF_NODE      0x01     // leaf node of B+ tree

//
// IX_PageHdr: Header structure for pages
//
struct IX_PageHdr {
   unsigned short flags;
   unsigned short numKeys;
   PageNum prevNode;
   PageNum nextNode;
};

#if 1
#define IX_PAGEHDR_SIZE sizeof(IX_PageHdr)
#else
#define IX_PAGEHDR_SIZE (4092-42)
#endif

#endif
