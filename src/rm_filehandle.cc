//
// File:        rm_filehandle.cc
// Description: RM_FileHandle class implementation
// Authors:     Pierre de Boissset (pdeboisset@enst.fr)
//

#include "rm.h"

//
// RM_FileHandle
//
// Desc: Constructor for RM_FileHandle object.
//
RM_FileHandle::RM_FileHandle()
{
	// TODO To be populated
}

//
// RM_FileHandle
//
// Desc: Destructor
//
RM_FileHandle::~RM_FileHandle()
{
	// TODO To be populated
}

//
// RM_FileHandle
//
// Desc: For this and the following methods, it should be a (positive) error if the RM_FileHandle object for which the method is called does not refer to an open file. This method should retrieve the record with identifier rid from the file. It should be a (positive) error if rid does not identify an existing record in the file. If the method succeeds, rec should contain a copy of the specified record along with its record identifier.
//
// Misc: Euh, j'ai un vrai doute de langage : que signifie le const de la fin de la signature ?
RC RM_FileHandle::GetRec(const RID &rid, RM_Record &rec) const
{	
    //We get page and slot number
    PageNum p; rid.GetPageNum(p);
	SlotNum s; rid.GetSlotNum(s);

    //Retrieves the pageHandle for page p
    PF_PageHandle ph();
    RC rc1 = this->pf_FileHandle->GetThisPage(p, ph);
    if(rc1!=0){
        return rc1;
    }
    char *& pData;
    RC rc2 = ph.GetData(pData);
    if(rc2!=0){
        return rc2;
    }

    //PageHeader for the page p
    PageHeader pHeader(this->GetNumSlots());

    //If the slot is free means the rid given is wrong
    if(pHeader.freeSlots.test(s)){
        return RM_INVALIDRID;
    }

    //Should be edited according to what Yixin will do for RM_Record class
    //hdr should a RM_FileHeader attribute of RM_FileHandle, not defined yet
    rec.Set(pData, hdr.extRecordSize, rid);
	
    return 0;
}

//
// RM_FileHandle
//
// Desc: Insert the data pointed to by pData as a new record in the file. If successful, the return parameter &rid should point to the record identifier of the newly inserted record.
//
RC RM_FileHandle::InsertRec(const char *pData, RID &rid)
{
	RC rc = 0;
	// TODO To be populated
	return rc;
}

//
// RM_FileHandle
//
// Desc: Delete the record with identifier rid from the file. If the page containing the record becomes empty after the deletion, you can choose either to dispose of the page (by calling PF_Manager::DisposePage) or keep the page in the file for use in the future, whichever you feel will be more efficient and/or convenient. 
//
RC RM_FileHandle::DeleteRec(const RID &rid)
{
	RC rc = 0;
	// TODO To be populated
	return rc;
}

//
// RM_FileHandle
//
// Desc: Update the contents of the record in the file that is associated with rec (see the RM_Record class description below). This method should replace the existing contents of the record in the file with the current contents of rec.
//
RC RM_FileHandle::UpdateRec(const RM_Record &rec)
{
	RC rc = 0;
	// TODO To be populated
	return rc;
}

//
// RM_FileHandle
//
// Desc: Call the corresponding method PF_FileHandle::ForcePages in order to copy the contents of one or all dirty pages of the file from the buffer pool to disk.
//
RC RM_FileHandle::ForcePages(PageNum pageNum = ALL_PAGES) const
{
	RC rc = 0;
	// TODO To be populated
	return rc;
}

