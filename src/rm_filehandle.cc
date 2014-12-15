//
// File:        rm_filehandle.cc
// Description: RM_FileHandle class implementation
// Authors:     Pierre de Boissset (pdeboisset@enst.fr)
//

#include "rm.h"

RM_FileHandle::RM_FileHandle() :pf_FileHandle(NULL), bFileOpen(false), bHdrChanged(false) {}

RC RM_FileHandle::Open(PF_FileHandle* pfh, int size) {
	if (bFileOpen || pf_FileHandle != NULL) {
		return RM_HANDLEOPEN;
	}
	
	if (pfh == NULL || size <= 0) {
		return RM_FCREATEFAIL;
	}
	
	bFileOpen = true;
	pf_FileHandle = new PF_FileHandle;
	*pf_FileHandle = *pfh ;
	PF_PageHandle ph;
	pf_FileHandle->GetThisPage(0, ph);
	// Needs to be called everytime GetThisPage is called.
	pf_FileHandle->UnpinPage(0);
	
	this->GetFileHeader(ph); // write into fileHeader

	bHdrChanged = true;
	return 0;
}

RC RM_FileHandle::GetPF_FileHandle(PF_FileHandle &lvalue) const {
	lvalue = *pf_FileHandle;
	return 0;
}

RC RM_FileHandle::GetNextFreeSlot(PF_PageHandle & ph, PageNum& pageNum, SlotNum& slotNum) {
	RM_PageHeader pageHeader(this->GetNumSlots());
	RC rc;
	if ((rc= this->GetNextFreePage(pageNum))
		|| (rc = pf_FileHandle->GetThisPage(pageNum, ph))
		|| (rc = pf_FileHandle->UnpinPage(pageNum))
		|| (rc = this->GetPageHeader(ph, pageHeader))) {
		return rc;
	}

	bitmap b(pageHeader.freeSlotMap, this->GetNumSlots());
	
	for (int i = 0; i < this->GetNumSlots(); i++) {
		if (b.test(i)) {
			slotNum = i;
			return 0;
		}
	}
	// This page is full
	return -1; // unexpected error
}

RC RM_FileHandle::GetNextFreePage(PageNum& pageNum) {
	PF_PageHandle ph;
	RM_PageHeader pageHeader(this->GetNumSlots());
	PageNum p;
	if(fileHeader.firstFree != RM_PAGE_LIST_END) {
	// this last page on the free list might actually be full
	RC rc;
	if ((rc = pf_FileHandle->GetThisPage(fileHeader.firstFree, ph))
		|| (rc = ph.GetPageNum(p))
		|| (rc = pf_FileHandle->MarkDirty(p))
		// Needs to be called everytime GetThisPage is called.
		|| (rc = pf_FileHandle->UnpinPage(fileHeader.firstFree))
		|| (rc = this->GetPageHeader(ph, pageHeader))) {
		return rc;
		}
	}
	if (fileHeader.numPages == 0
		|| fileHeader.firstFree == RM_PAGE_LIST_END
		|| (pageHeader.numFreeSlots == 0)) {

		char *pData;
		if ((rc = pf_FileHandle->AllocatePage(ph))
			|| (rc = ph.GetData(pData))
			|| (rc = ph.GetPageNum(pageNum))) {
			return(rc);
		}
		// Add page header
		RM_PageHeader pageHeader(this->GetNumSlots());
		pageHeader.nextFree = RM_PAGE_LIST_END;
		bitmap b(this->GetNumSlots());
		b.set(); // Initially all slots are free
		b.to_char_buf(pageHeader.freeSlotMap, b.numChars());

		pageHeader.to_buf(pData);

		// the default behavior of the buffer pool is to pin pages
		// let us make sure that we unpin explicitly after setting
		// things up
		if ((rc = pf_FileHandle->UnpinPage(pageNum))) {
			return rc;
		}

		// add page to the free list
		fileHeader.firstFree = pageNum;
		fileHeader.numPages++;
		assert(fileHeader.numPages > 1); // page num 1 would be header page

		bHdrChanged = true;
		return 0; // pageNum is set correctly
	}
	// return existing free page
	pageNum = fileHeader.firstFree;
	return 0;
}

RC RM_FileHandle::GetPageHeader(PF_PageHandle ph, RM_PageHeader& pageHeader) const {
	char * buf;
    RC rc = ph.GetData(buf);
    pageHeader.from_buf(buf);
	return rc;
}

RC RM_FileHandle::SetPageHeader(PF_PageHandle ph, const RM_PageHeader& pageHeader) {
	char * buf;
	RC rc;
	if((rc = ph.GetData(buf)))
	return rc;
	pageHeader.to_buf(buf);
	return 0;
}

// get header from the first page of a newly opened file
RC RM_FileHandle::GetFileHeader(PF_PageHandle ph) {
	char * buf;
	RC rc = ph.GetData(buf);
	memcpy(&fileHeader, buf, sizeof(fileHeader));
	return rc;
}

// persist header into the first page of a file for later
RC RM_FileHandle::SetFileHeader(PF_PageHandle ph) const {
	char * buf;
	RC rc = ph.GetData(buf);
	memcpy(buf, &fileHeader, sizeof(fileHeader));
	return rc;
}

RC RM_FileHandle::GetSlotPointer(PF_PageHandle ph, SlotNum s, char *& pData) const {
	RC rc = ph.GetData(pData);
	if (rc >= 0 ) {
		bitmap b(this->GetNumSlots());
		pData = pData + (RM_PageHeader(this->GetNumSlots()).size());
		pData = pData + s * this->fullRecordSize();
	}
	return rc;
}

//Gives the number of slots in one page
int RM_FileHandle::GetNumSlots() const{
    //Makes sure RecordSize is > 0
    if(this->getRecordSize()==0){
        return RM_NULLRECORDSIZE;
    }
    //Increments slotsNb until we find the max value
    int slotsNb = 0;
    while(1>0){
        slotsNb++;
        RM_PageHeader pageHeader(slotsNb);
        if(pageHeader.size()+ slotsNb*this->getRecordSize() > PF_PAGE_SIZE){
            return slotsNb - 1;
        }
        delete pageHeader;
    }
}

int RM_FileHandle::GetNumPages() const {
    return this->fileHeader.getPagesNumber();
}

RM_FileHandle::~RM_FileHandle() {
	if(pf_FileHandle != NULL)
	delete pf_FileHandle;
}

// Given a RID, return the record
RC RM_FileHandle::GetRec (const RID &rid, RM_Record &rec) const {

	PageNum p;
	SlotNum s;
	rid.GetPageNum(p);
	rid.GetSlotNum(s);
	RC rc = 0;
	PF_PageHandle ph;
	RM_PageHeader pageHeader(this->GetNumSlots());
	if((rc = pf_FileHandle->GetThisPage(p, ph))
		|| (rc = pf_FileHandle->UnpinPage(p))
		|| (rc = this->GetPageHeader(ph, pageHeader))) {
		return rc;
	}
	
	bitmap b(pageHeader.freeSlotMap, this->GetNumSlots());
	if(b.test(s)) { // already free
		return RM_NORECATRID;
	}
	
	char * pData = NULL;
	if(rc = this->GetSlotPointer(ph, s, pData)) {
		return rc;
	}
	rec.Set(pData, fileHeader.extRecordSize, rid);
	return 0;
}

RC RM_FileHandle::InsertRec (const char *pData, RID &rid) {
	if(pData == NULL) {
		return RM_NULLRECORD;
	}
	PF_PageHandle ph;
	RM_PageHeader pageHeader(this->GetNumSlots());
	PageNum p;
	SlotNum s;
	RC rc;
	char * pSlot;
	if((rc = this->GetNextFreeSlot(ph, p, s))
		|| (rc = this->GetPageHeader(ph, pageHeader))) {
		return rc;
	}
	
	bitmap b(pageHeader.freeSlotMap, this->GetNumSlots());
	
	if((rc = this->GetSlotPointer(ph, s, pSlot))) {
		return rc;
	}
	
	rid = RID(p, s);
	memcpy(pSlot, pData, this->fullRecordSize());
	b.reset(s); // slot s is no longer free
	pageHeader.numFreeSlots--;
	
	if(pageHeader.numFreeSlots == 0) {
		// remove from free list
		fileHeader.firstFree = pageHeader.nextFree;
		pageHeader.nextFree = RM_PAGE_FULLY_USED;
	}
	
	b.to_char_buf(pageHeader.freeSlotMap, b.numChars());
	rc = this->SetPageHeader(ph, pageHeader);
	return rc;
}

RC RM_FileHandle::DeleteRec (const RID &rid) {
	PageNum p;
	SlotNum s;
	rid.GetPageNum(p);
	rid.GetSlotNum(s);
	RC rc = 0;
	PF_PageHandle ph;
	RM_PageHeader pageHeader(this->GetNumSlots());
	
	if((rc = pf_FileHandle->GetThisPage(p, ph))
		|| (rc = pf_FileHandle->MarkDirty(p))
		|| (rc = pf_FileHandle->UnpinPage(p))
		|| (rc = this->GetPageHeader(ph, pageHeader))) {
		return rc;
	}
	
	bitmap b(pageHeader.freeSlotMap, this->GetNumSlots());
	
	if(b.test(s)) { // already free
		return RM_NORECATRID;
	}
	
	b.set(s); // s is now free
	if(pageHeader.numFreeSlots == 0) {
		// this page used to be full and used to not be on the free list
		// add it to the free list now.
		pageHeader.nextFree = hdr.firstFree;
		hdr.firstFree = p;
	}
	
	pageHeader.numFreeSlots++;
	
	b.to_char_buf(pageHeader.freeSlotMap, b.numChars());
	rc = this->SetPageHeader(ph, pageHeader);
	
	return rc;
}

RC RM_FileHandle::UpdateRec (const RM_Record &rec) {
	RID rid;
	rec.GetRid(rid);
	PageNum p;
	SlotNum s;
	rid.GetPageNum(p);
	rid.GetSlotNum(s);
	PF_PageHandle ph;
	char * pSlot;
	RC rc;

	RM_PageHeader pageHeader(this->GetNumSlots());

	if ((rc = pf_FileHandle->GetThisPage(p, ph))
		|| (rc = pf_FileHandle->MarkDirty(p))
		|| (rc = pf_FileHandle->UnpinPage(p))
		|| (rc = this->GetPageHeader(ph, pageHeader))) {
		return rc;
	}
	
	bitmap b(pageHeader.freeSlotMap, this->GetNumSlots());
	
	if(b.test(s)) { // free - cannot update
		return RM_NORECATRID;
	}
	
	char * pData = NULL;
	rec.GetData(pData);
	
	if(RC rc = this->GetSlotPointer(ph, s, pSlot)) {
		return rc;
	}
	
	memcpy(pSlot, pData, this->fullRecordSize());
	
	return rc;
}

//Allows to get the PF_FileHandle attribute
RC RM_FileHandle::GetPF_FileHandle(PF_FileHandle pf_FileHandle) const
{
  fileHandle = *pf_FileHandle;
  return 0;
}

// Forces a page (along with any contents stored in this class)
// from the buffer pool to disk. Default value forces all pages.
RC RM_FileHandle::ForcePages (PageNum pageNum) {
	return (!this->IsValidPageNum(pageNum) && pageNum != ALL_PAGES) ? RM_BAD_RID : pf_FileHandle->ForcePages(pageNum);
}

//
// IsValidPageNum
//
// Desc: Internal. Return TRUE if pageNum is a valid page number
// in the file, FALSE otherwise
// In: pageNum - page number to test
// Ret: TRUE or FALSE
//
bool RM_FileHandle::IsValidPageNum(const PageNum pageNum) const {
	return (bFileOpen && pageNum >= 0 && pageNum < fileHeader.numPages);
}

//
// IsValidRID
//
//
bool RM_FileHandle::IsValidRID(const RID rid) const {
	PageNum p;
	SlotNum s;
	rid.GetPageNum(p);
	rid.GetSlotNum(s);
	return IsValidPageNum(p) && s >= 0 && s < this->GetNumSlots();
}

RC RM_FileHandle::IsValid() const {
	if((pf_FileHandle == NULL) || !bFileOpen) {
		return RM_FNOTOPEN;
	}
	
	if(GetNumSlots() <= 0) {
		return RM_RECSIZEMISMATCH;
	}

	return 0;
}

