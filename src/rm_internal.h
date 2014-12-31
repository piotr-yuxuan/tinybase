//
// File:        rm_internal.h
// Description: Declarations internal to the RM component
// Author:      Hyunjung Park (hyunjung@stanford.edu)
//

#ifndef RM_INTERNAL_H
#define RM_INTERNAL_H

#include <stdlib.h>
#include <string.h>
#include <cassert>
#include "rm.h"

//
// Constants and defines
//
const int RM_HEADER_PAGE_NUM = 0;

#define RM_PAGE_LIST_END  -1       // end of list of free pages
#define RM_PAGE_FULL      -2       // all slots in the page are full

//
// RM_PageHdr: Header structure for pages
//
struct RM_PageHdr {
   PageNum nextFree;
};

#endif
