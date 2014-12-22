//
// File:        rm_filehandle.cc
// Description: RM_FileHandle class implementation
// Authors:     Pierre de Boissset (pdeboisset@enst.fr)
//

#include "rm.h"


//Constructor for RM_FileHandle
RM_FileHandle::RM_FileHandle() :
		pf_FileHandle(NULL), bFileOpen(false), bHdrChanged(false) {
}

//Opens the RM_FileHandle with a given PF_FileHandle pointer
RC RM_FileHandle::Open(PF_FileHandle* pfh, int size) {
    if (bFileOpen || pf_FileHandle != NULL) {
        return RM_HANDLEOPEN;
    }

	// The file handle must be not free.
    if (pfh == NULL) {
        return RM_NULLPOINTER;
	}
	
	// Negative size is inconstitant.
    if (size <= 0){
        return RM_INVALIDRECSIZE;
    }

	bFileOpen = true;
	pf_FileHandle = new PF_FileHandle;
	*pf_FileHandle = *pfh;
	PF_PageHandle ph;
	pf_FileHandle->GetThisPage(0, ph);
	
	// Must be called everytime GetThisPage is called. The buffer pool pins
	// pages by default. We unpin it explicitly after setting things up.
	pf_FileHandle->UnpinPage(0);

	this->GetFileHeader(ph); // write into fileHeader

	bHdrChanged = true;
	return 0;
}

//Allows to get the PF_FileHandle attribute
RC RM_FileHandle::GetPF_FileHandle(PF_FileHandle &lvalue) const {
    RC invalid = IsValid(); if(invalid) return invalid;
    lvalue = *pf_FileHandle;
    return 0;
}


//Given a page number and a slot number, passes next free page into ph
//and next free slot position into pageNum & slotNum
RC RM_FileHandle::GetNextFreeSlot(PF_PageHandle & ph, PageNum& pageNum,
		SlotNum& slotNum) {
	RC rc;

    //Passes the number of next free page into pageNum
    if ( (rc = this->GetNextFreePage(pageNum)) ) {
         return rc;
    }

    //Ph becomes PF_PageHandle for the next free page
    if( (rc = pf_FileHandle->GetThisPage(pageNum, ph)) ) {
        return rc;
    }

    //Unpins next free page
    if( (rc = pf_FileHandle->UnpinPage(pageNum)) ) {
        return rc;
    }

    //Retrieves the header of the next freepage into a RM_PageHeader object
    RM_PageHeader pageHeader(this->GetNumSlots());
    if( (rc = this->GetPageHeader(ph, pageHeader)) ) {
		return rc;
	}

    //Goes through the bitmap of the pageHeader to find the next free slot
	for (int i = 0; i < this->GetNumSlots(); i++) {
        if (pageHeader.freeSlots.test(i)) {
			slotNum = i;
			return 0;
		}
	}
    /*
     *Something should have been returned at this point since we went through all the slot
     *Of the next FREE page (so there must be an empty slot).
     *Anyway we return -1 just in case to signal an unexpected error.
    */
	return -1; // unexpected error
}


//Passes the number of the next free page to pageNum
RC RM_FileHandle::GetNextFreePage(PageNum& pageNum) {
	RC rc;
	PF_PageHandle ph;
	RM_PageHeader pageHeader(this->GetNumSlots());
	PageNum p;
    if (fileHeader.getFirstFreePage() != RM_PAGE_LIST_END) {
		// this last page on the free list might actually be full
        if ( (rc = pf_FileHandle->GetThisPage(fileHeader.getFirstFreePage(), ph)) ) {
        	return rc;
        }
        
        // Sets p to its actual value.
        if ( (rc = ph.GetPageNum(p)) ) {
        	return rc;
        }
        
        // Remembers this page has been altered, so save it before overwriting it.
        if ( (rc = pf_FileHandle->MarkDirty(p)) ) {
        	return rc;
        }
        
        // Needs to be called everytime GetThisPage is called.
        if ( (rc = pf_FileHandle->UnpinPage(fileHeader.getFirstFreePage())) ) {
        	return rc;
        }
        
        // Retrieves the header.
        if ( (rc = this->GetPageHeader(ph, pageHeader))) {
			return rc;
		}
	}
    if (fileHeader.getPagesNumber() == 0
            || fileHeader.getFirstFreePage() == RM_PAGE_LIST_END
            || (pageHeader.getNbFreeSlots() == 0)) {

		char *pData;
		
		// Allocate a new page in the file.
		if ( (rc = pf_FileHandle->AllocatePage(ph)) ) {
			return (rc);
		}
		
		// Puts ph to point to the actual data of ph.
		if ( (rc = ph.GetData(pData)) ) {
			return (rc);
		}
		
		// Get the page number.
		if ( (rc = ph.GetPageNum(pageNum))) {
			return (rc);
		}
		
		// Add page header
		RM_PageHeader pageHeader(this->GetNumSlots());
        pageHeader.setNextFreePage(RM_PAGE_LIST_END);
        pageHeader.freeSlots.set(); // Initially all slots are free

		pageHeader.to_buf(pData);

		// the default behavior of the buffer pool is to pin pages
		// let us make sure that we unpin explicitly after setting
		// things up
		if ((rc = pf_FileHandle->UnpinPage(pageNum))) {
			return rc;
		}

		// add page to the free list
        fileHeader.setFirstFreePage(pageNum);
        fileHeader.setPagesNumber(fileHeader.getPagesNumber() + 1);

		bHdrChanged = true;
		return 0; // pageNum is set correctly
	}
	// return existing free page
    pageNum = fileHeader.getFirstFreePage();
	return 0;
}

//Given ph, loads header page in private pageHeader attribute
RC RM_FileHandle::GetPageHeader(PF_PageHandle ph,
		RM_PageHeader& pageHeader) const {
	char * buf;
	RC rc = 0;
	
	// Puts ph to point to the actual data of ph
    if ( (rc = ph.GetData(buf) ) ) {
		return rc;
	}
	
	// Reads header from the buffer.
	pageHeader.from_buf(buf);
	return rc;
}

//Writes pageHeader to the page handled by ph
RC RM_FileHandle::SetPageHeader(PF_PageHandle ph,
		const RM_PageHeader& pageHeader) {
	char * buf;
	RC rc;
	
	//Puts ph to point to the actual data of ph
	if ((rc = ph.GetData(buf))) {
		return rc;
	}
	
	pageHeader.to_buf(buf);
	return 0;
}

// get header from the first page of a newly opened file
RC RM_FileHandle::GetFileHeader(PF_PageHandle ph) {
    char * buf;
    RC rc(0);
    
    //Puts ph to point to the actual data of ph
    if( (rc = ph.GetData(buf)) ){
        return rc;
    }
    
    //Loads the fileHeader from data
    if( (rc = this->fileHeader.from_buf(buf)) ){
        return rc;
    }
    return 0;
}

// Writes the fileHeader attribute into ph (should be the first page of the file)
RC RM_FileHandle::SetFileHeader(PF_PageHandle ph) const {
    char * buf;
    RC rc(0);
    //Puts ph to point to the actual data of ph
    if( (rc = ph.GetData(buf)) ){
        return rc;
    }
    
    //Loads the fileHeader from data
    if( (rc = this->fileHeader.to_buf(buf)) ){
        return rc;
    }
    return 0;
}

//Put pData to point to the slot number s in page ph
RC RM_FileHandle::GetSlotPointer(PF_PageHandle ph, SlotNum s, char *& pData) const {
    //Puts ph to point to the actual data of ph
	RC rc = ph.GetData(pData);
    if (rc < 0) {
        return rc;
    }
    
    //Offset because of the page header
    pData = pData + (RM_PageHeader(this->GetNumSlots()).size());
    //Offset because of the s slots before the one we want
    pData = pData + s * this->getRecordSize();
	return rc;
}

//Gives the number of slots in one page
int RM_FileHandle::GetNumSlots() const {
	//Makes sure RecordSize is > 0
	if (this->getRecordSize() == 0) {
		return RM_NULLRECORDSIZE;
	}
	//Increments slotsNb until we find the max value. It's sure it can be found.
	int slotsNb = 0;
	while (1 > 0) {
		slotsNb++;
		RM_PageHeader pageHeader(slotsNb);
		if (pageHeader.size() + slotsNb * this->getRecordSize()
				> PF_PAGE_SIZE) {
			return slotsNb - 1;
		}
	}
}

//Just returns the number of pages in the file
int RM_FileHandle::GetNumPages() const {
	return this->fileHeader.getPagesNumber();
}

//Destructor for RM_FileHandle
RM_FileHandle::~RM_FileHandle() {
	if (pf_FileHandle != NULL)
		delete pf_FileHandle;
}

// Given a RID, return the record
RC RM_FileHandle::GetRec(const RID &rid, RM_Record &rec) const {

    //Checks file is open
    if(!this->bFileOpen){
        return RM_CLOSEDFILE;
    }

	// Initialize p and s and give them their actual values
	PageNum p; rid.GetPageNum(p);
	SlotNum s; rid.GetSlotNum(s);

    //Tests RID
    if(p==-1 && s==-1){
        return RM_INVIABLERID;
    }
    if(s<0 || s>=this->GetNumSlots()){
        return RM_INVALIDSLOTNUM;
    }

	RC rc = 0;
	PF_PageHandle ph;
	RM_PageHeader pageHeader(this->GetNumSlots());
	
	if ( (rc = pf_FileHandle->GetThisPage(p, ph)) ) {
		return rc;
	}
	
	// Must be called everytime GetThisPage is called. The buffer pool pins
	// pages by default. We unpin explicitly after setting things up.
	if ( (rc = pf_FileHandle->UnpinPage(p)) ) {
		return rc;
	}
	
	// Loads the header page.
	if ( (rc = this->GetPageHeader(ph, pageHeader)) ) {
		return rc;
	}

	// Is it free?
    if (pageHeader.freeSlots.test(s)) { // already free
        return RM_RECORDNOTFOUND;
	}

	char * pData = NULL;
	// Gets pSlot pointing on slot number s in page ph
    if ( (rc = this->GetSlotPointer(ph, s, pData)) ) {
		return rc;
	}
	
	// Sets the return parameter.
    rec.Set(pData, fileHeader.getRecordSize(), rid);
	return 0;
}

//Inserts a record buffered into pData in the file
RC RM_FileHandle::InsertRec(const char *pData, RID &rid) {
	if (pData == NULL) {
		return RM_NULLRECORD;
	}
	PF_PageHandle ph;
	RM_PageHeader pageHeader(this->GetNumSlots());
	PageNum p;
	SlotNum s;
	RC rc;
	char * pSlot;

    //Gets next free slot position in p & s variables
    if ( (rc = this->GetNextFreeSlot(ph, p, s)) ) {
		return rc;
	}

    // Loads the header page
    if( (rc = this->GetPageHeader(ph, pageHeader)) ) {
        return rc;
    }

	// Gets pSlot pointing on slot number s in page ph
	if ((rc = this->GetSlotPointer(ph, s, pSlot))) {
		return rc;
	}

	rid = RID(p, s);
	
	// Copy a record from pData to pSlot.
	memcpy(pSlot, pData, this->getRecordSize());
	
	// slot s is no longer free
    pageHeader.freeSlots.reset(s);

	// If this page has been filled
    if (pageHeader.getNbFreeSlots() == 0) {
		// remove it from free list
        fileHeader.setFirstFreePage(pageHeader.getNextFreePage());
        pageHeader.setNextFreePage(RM_PAGE_FULLY_USED);
	}

    //Writes pageHeader into page handled by ph
    rc = this->SetPageHeader(ph, pageHeader);
	return rc;
}

//Deletes a given RID from the file
RC RM_FileHandle::DeleteRec(const RID &rid) {
	// Initialize p and s and give them their actual values
	PageNum p; rid.GetPageNum(p);
	SlotNum s; rid.GetSlotNum(s);
	
	RC rc = 0;
	
	PF_PageHandle ph;
	RM_PageHeader pageHeader(this->GetNumSlots());

	// ph becomes page handler of the next free page
	if ( (rc = pf_FileHandle->GetThisPage(p, ph)) ) {
		return rc;
	}
	
	// Remembers this page has been altered, so save it before overwriting it.
	if ( (rc = pf_FileHandle->MarkDirty(p)) ) {
		return rc;
	}
	
	// Must be called everytime GetThisPage is called. The buffer pool pins
	// pages by default. We unpin explicitly after setting things up.
	if ( (rc = pf_FileHandle->UnpinPage(p)) ) {
		return rc;
	}
	
	// Loads the header page.
	if ( (rc = this->GetPageHeader(ph, pageHeader)) ) {
		return rc;
	}
	
    if (pageHeader.freeSlots.test(s)) { // already free
		return RM_NORECATRID;
	}

    pageHeader.freeSlots.set(s); // s is now free
    
    if (pageHeader.getNbFreeSlots() == 0) {
		// this page used to be full and used to not be on the free list
		// add it to the free list now.
        pageHeader.setNextFreePage(fileHeader.getFirstFreePage());
        fileHeader.setFirstFreePage(p);
	}

	// Update the header page.
	rc = this->SetPageHeader(ph, pageHeader);

	return rc;
}

//Update a given record in the file
RC RM_FileHandle::UpdateRec(const RM_Record &rec) {
	
	// Declares and gets the if of the given record.
	RID rid; rec.GetRid(rid);
	
	// Initializes p and s and give them their actual values
	PageNum p; rid.GetPageNum(p);
	SlotNum s; rid.GetSlotNum(s);
    
    if (p == -1 && s == -1) {
        return RM_UNREADRECORD;
    }
    
	PF_PageHandle ph;
	char * pSlot;
	RC rc = 0;

	RM_PageHeader pageHeader(this->GetNumSlots());

	// ph becomes page handler of the next free page.
	if ( (rc = pf_FileHandle->GetThisPage(p, ph)) ) {
		return rc;
	}
	
	// Remembers this page has been altered, so save it before overwriting it.
	if ( (rc = pf_FileHandle->MarkDirty(p)) ) {
		return rc;
	}

	// Must be called everytime GetThisPage is called. The buffer pool pins
	// pages by default. We unpin explicitly after setting things up.
	if ( (rc = pf_FileHandle->UnpinPage(p)) ) {
		return rc;
	}
	
	// Loads the header page.
	if ( (rc = this->GetPageHeader(ph, pageHeader)) ) {
		return rc;
	}

	// Is it free?
    if (pageHeader.freeSlots.test(s)) { // free - cannot update
        return RM_RECORDNOTFOUND;
	}

	char * pData = NULL;

	// Puts ph to point to the actual data of ph
    if ( (rc = rec.GetData(pData) ) ) {
		return rc;
	}

	// Gets pSlot pointing on slot number s in page ph
    if ( (rc = this->GetSlotPointer(ph, s, pSlot)) ) {
		return rc;
	}

	// Performs the copy, updates the record.
	memcpy(pSlot, pData, this->getRecordSize());

	return rc;
}

// Forces a page (along with any contents stored in this class)
// from the buffer pool to disk. Default value forces all pages.
RC RM_FileHandle::ForcePages(PageNum pageNum) {
    return pf_FileHandle->ForcePages(pageNum);
}

//Checks the fileHandle itself
RC RM_FileHandle::IsValid() const {
	if ((pf_FileHandle == NULL) || !bFileOpen) {
		return RM_FNOTOPEN;
	}

	if (GetNumSlots() <= 0) {
		return RM_RECSIZEMISMATCH;
	}

	return 0;
}


//Tells if header has been modified
bool RM_FileHandle::headerModified() {
    return this->bHdrChanged;
}
