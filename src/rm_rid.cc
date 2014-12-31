//
// File:        rm_rid.cc
// Description: RID class implementation
// Author:      Hyunjung Park (hyunjung@stanford.edu)
//

#include "rm_internal.h"

// 
// RID
//
// Desc: Default Constructor
//
RID::RID()
{
   pageNum = 0;
   slotNum = 0;
}

// 
// RID
//
// Desc: Constructor
//
RID::RID(PageNum _pageNum, SlotNum _slotNum)
{
   pageNum = _pageNum;
   slotNum = _slotNum;
}

//
// ~RID
// 
// Desc: Destructor
//
RID::~RID()
{
   // Don't need to do anything
}

//
// operator=
//
// Desc: overload = operator
// In:   rid - rid object to set this object equal to
// Ret:  reference to *this
//
RID& RID::operator= (const RID &rid)
{
   // Test for self-assignment
   if (this != &rid) {
      // Just copy the members since there is no memory allocation involved
      this->pageNum = rid.pageNum;
      this->slotNum = rid.slotNum;
   }

   // Return a reference to this
   return (*this);
}

//
// operator==
//
// Desc: overload == operator
// In:   rid - rid object to be compared with this object
// Ret:  true or false
//
bool RID::operator==(const RID &rid) const
{
   return (this->pageNum == rid.pageNum) && (this->slotNum == rid.slotNum);
}

//
// GetPageNum
// 
// Desc: Return page number
// Out:  _pageNum - set to this RID's page number
// Ret:  RM_INVIABLERID
//
RC RID::GetPageNum(PageNum &_pageNum) const
{
   // Should be a viable record identifier
   if (pageNum == 0)
      return (RM_INVIABLERID);

   // Set the parameter to this RID's page number
   _pageNum = pageNum;    

   // Return ok
   return (0);
}

//
// GetSlotNum
// 
// Desc: Return slot number
// Out:  _slotNum - set to this RID's slot number
// Ret:  RM_INVIABLERID
//
RC RID::GetSlotNum(SlotNum &_slotNum) const
{
   // Should be a viable record identifier
   if (pageNum == 0)
      return (RM_INVIABLERID);

   // Set the parameter to this RID's slot number
   _slotNum = slotNum;    

   // Return ok
   return (0);
}

