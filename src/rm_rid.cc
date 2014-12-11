//
// File:        rm_rid.cc
// Description: RID class implementation
//

#include "rm.h"

//
// Class RM_FileScan declaration
//

//Constructor
RID::RID(){
    //Nothing to do for now
}

//Second constructor
RID::RID(PageNum pageNum, SlotNum slotNum){

}

//Destructor
RID::~RID(){
    //Nothing to do for now
}

//Get the page number of the rid
RC RID::GetPageNum(PageNum &pageNum) const{
    pageNum = this->page;
    return 0;
}

//Get the slot number for the RID
RC RID::GetSlotNum(SlotNum &slotNum) const{
    slotNum = this->slot;
    return 0;
}

//Allows to compare two RIDs with ==
bool RID::operator==(const RID & rhs) const
  {
    PageNum p;
    SlotNum s;
    rhs.GetPageNum(p);
    rhs.GetSlotNum(s);
    return (p == page && s == slot);
  }
