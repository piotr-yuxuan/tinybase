//
// File:        rm_filehandle.cc
// Description: RM_FileHandle class implementation
// Authors:     Pierre de Boissset (pdeboisset@enst.fr)
//

#include "rm_internal.h"
#include "rm_filehandle.h"

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
	// TODO To be populated
}

//
// RM_FileHandle
//
// Desc: Insert the data pointed to by pData as a new record in the file. If successful, the return parameter &rid should point to the record identifier of the newly inserted record.
//
RC RM_FileHandle::InsertRec(const char *pData, RID &rid)
{
	// TODO To be populated
}

//
// RM_FileHandle
//
// Desc: Delete the record with identifier rid from the file. If the page containing the record becomes empty after the deletion, you can choose either to dispose of the page (by calling PF_Manager::DisposePage) or keep the page in the file for use in the future, whichever you feel will be more efficient and/or convenient. 
//
RC RM_FileHandle::DeleteRec(const RID &rid)
{
	// TODO To be populated
}

//
// RM_FileHandle
//
// Desc: Update the contents of the record in the file that is associated with rec (see the RM_Record class description below). This method should replace the existing contents of the record in the file with the current contents of rec.
//
RC RM_FileHandle::UpdateRec(const RM_Record &rec)
{
	// TODO To be populated
}

//
// RM_FileHandle
//
// Desc: Call the corresponding method PF_FileHandle::ForcePages in order to copy the contents of one or all dirty pages of the file from the buffer pool to disk.
//
RC RM_FileHandle::ForcePages(PageNum pageNum = ALL_PAGES) const
{
	// TODO To be populated
}

