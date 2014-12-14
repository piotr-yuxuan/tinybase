//
// rm_rid.h
//
//   The Record Id interface
//

#ifndef RM_RID_H
#define RM_RID_H

// We separate the interface of RID from the rest of RM because some
// components will require the use of RID but not the rest of RM.

#include "tinybase.h"

//
// PageNum: uniquely identifies a page in a file
//
typedef int PageNum;

//
// SlotNum: uniquely identifies a record in a page
//
typedef int SlotNum;

//
// RID: Record id interface
//
class RID {
public:
    //Consts for Page and Slot
    static const PageNum NULL_PAGE = -1;
    static const SlotNum NULL_SLOT = -1;

    RID();                                          // Default constructor
    RID(PageNum pageNum, SlotNum slotNum);
    ~RID();                                        // Destructor

    RC GetPageNum(PageNum &pageNum) const;         // Return page number
    RC GetSlotNum(SlotNum &slotNum) const;         // Return slot number

    PageNum GetPage(){
        return page;
    }

    SlotNum GetSlot(){
        return slot;
    }

    bool operator==(const RID & rhs) const;

private:
    PageNum page;
    SlotNum slot;
};

#endif
