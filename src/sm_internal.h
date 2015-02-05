//
// File:        sm_internal.h
// Description: Declarations internal to the SM component
// Author:      Hyunjung Park (hyunjung@stanford.edu)
//

#ifndef SM_INTERNAL_H
#define SM_INTERNAL_H

#include <stdlib.h>
#include <string.h>
//#include <stdio.h>
#include <unistd.h>
#include <cassert>
#include "sm.h"
#include "printer.h"

//
// Constants and defines
//
#define MAXDBNAME MAXNAME
#define MAXLINE (2048)
#define RELCAT "relcat"
#define ATTRCAT "attrcat"

#define OFFSET(type, member) ((int)&((type *) 0)->member)

//
// SM_RelcatRec : structure for relcat record
// 
struct SM_RelcatRec {
   char relName[MAXNAME];
   int tupleLength;
   int attrCount;
   int indexCount;
};

#define SM_SetRelcatRec(r, _relName, _tupleLength,          \
                       _attrCount, _indexCount)             \
do {                                                        \
   memset(r.relName, '\0', sizeof(r.relName));              \
   strncpy(r.relName, _relName, sizeof(r.relName));         \
   r.tupleLength = _tupleLength;                            \
   r.attrCount = _attrCount;                                \
   r.indexCount = _indexCount;                              \
} while (0)

//
// SM_AttrcatRec : structure for attrcat record
// 
struct SM_AttrcatRec {
   char relName[MAXNAME];
   char attrName[MAXNAME];
   int offset;
   AttrType attrType;
   int attrLength;
   int indexNo;
};

#define SM_SetAttrcatRec(r, _relName, _attrName, _offset,   \
                         _attrType, _attrLength, _indexNo)  \
do {                                                        \
   memset(r.relName, '\0', sizeof(r.relName));              \
   strncpy(r.relName, _relName, MAXNAME);                   \
   memset(r.attrName, '\0', sizeof(r.attrName));            \
   strncpy(r.attrName, _attrName, MAXNAME);                 \
   r.offset = _offset;                                      \
   r.attrType = _attrType;                                  \
   r.attrLength = _attrLength;                              \
   r.indexNo = _indexNo;                                    \
} while (0)

#endif
