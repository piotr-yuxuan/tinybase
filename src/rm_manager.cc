//
//  rm_manager.cpp
//  tinybase
//
//  Created by Pauline RABIS on 16/12/2014.
//  Copyright (c) 2014 Pauline RABIS. All rights reserved.
//

#include "rm.h"
#include <cstdio>
#include <cassert>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

RM_Manager::RM_Manager(PF_Manager &pfm) :
		pfm(pfm) {
}

RM_Manager::~RM_Manager() {
} /** Destructor */

// create a new file for a given filename and a recordSize
RC RM_Manager::CreateFile(const char *fileName, int recordSize) {

	RM_PageHeader pageHeader(0); //A page Header with 0 slot

	//First makes sur recordSize is a strictly positive integer
	if (recordSize <= 0) {
        return RM_INVALIDRECSIZE;
	}
	//Then just in case recordSize were too big
	if (recordSize >= PF_PAGE_SIZE - pageHeader.size()) {
        return RM_INVALIDRECSIZE;
	}

	//Creates the file in PageFile
	RC rc = 0;
	rc = this->pfm.CreateFile(fileName);
	if (rc < 0) {
		PF_PrintError(rc);
		return rc;
	}

	//Opens it
	PF_FileHandle pfh;
	rc = pfm.OpenFile(fileName, pfh);
	if (rc < 0) {
		PF_PrintError(rc);
		return rc;
	}

	PF_PageHandle headerPage;
	char * pData;

	rc = pfh.AllocatePage(headerPage);
	if (rc < 0) {
		PF_PrintError(rc);
		return RM_PFERROR;
	}

	rc = headerPage.GetData(pData);
	if (rc < 0) {
		PF_PrintError(rc);
		return RM_PFERROR;
	}

	RM_FileHeader fileHeader;
    fileHeader.setFirstFreePage(RM_PAGES_END);
	fileHeader.setPagesNumber(1); // hdr page
	fileHeader.setRecordSize(recordSize);

	fileHeader.to_buf(pData);
	//memcpy(pData, &hdr, sizeof(hdr));

	PageNum headerPageNum;
	headerPage.GetPageNum(headerPageNum);
	assert(headerPageNum == 0);

	rc = pfh.MarkDirty(headerPageNum);
	if (rc < 0) {
		PF_PrintError(rc);
		return RM_PFERROR;
	}

	rc = pfh.UnpinPage(headerPageNum);
	if (rc < 0) {
		PF_PrintError(rc);
		return RM_PFERROR;
	}

	rc = pfm.CloseFile(pfh);
	if (rc < 0) {
		PF_PrintError(rc);
		return RM_PFERROR;
	}

	return 0;
}

// destroys the fileName file
RC RM_Manager::DestroyFile(const char *fileName) {
	int RC;

	//Handles destruction at the lower level ie PF
	if ((RC = pfm.DestroyFile(fileName)) < 0) {
		PF_PrintError(RC);
        return RC;
	}

	return 0;
}

//Opens a given file and passes it to given fileHandle
RC RM_Manager::OpenFile(const char *fileName, RM_FileHandle &fileHandle) {
	PF_FileHandle pfh;
	PF_PageHandle ph;
	char * pData;
	RM_FileHeader hdr;

    RC rc(0);
    fileHandle.GetPF_FileHandle(pfh);
    if( (rc = pfm.OpenFile(fileName, pfh)) ) {
		PF_PrintError(rc);
        return rc;
	}
	// header page is at 0

	if ((rc = pfh.GetThisPage(0, ph)) || (rc = ph.GetData(pData))) {
		return (rc);
	}

    //Loads the header from the data
    rc = hdr.from_buf(pData);
    if (rc != 0) {
        return rc;
    }

	rc = fileHandle.Open(&pfh, hdr.getRecordSize());

	if (rc < 0) {
		RM_PrintError(rc);
		return rc;
	}
	rc = pfh.UnpinPage(0);

	if (rc < 0) {
		PF_PrintError(rc);
		return rc;
	}

	return 0;
}

//Close a file
RC RM_Manager::CloseFile(RM_FileHandle& fileHandle) {
	RC rc;
	PF_PageHandle ph;


    //pif is the PF_FileHandle associated with fileHandle (reference)
    PF_FileHandle& pif = *(fileHandle.pf_FileHandle);

    /*Old version
     *if ((rc = fileHandle.GetPF_FileHandle(pif)) < 0) {
		return rc;
    }*/

	// If header was modified, put the first page into buffer again,
	// and update its contents, marking the page as dirty
	if (fileHandle.headerModified() == true) {

		//ph is the PF_PageHandle for the header
		if ((rc = pif.GetFirstPage(ph)) < 0) {
			return rc;
		}

		//Writes into the file
		if ((rc = fileHandle.SetFileHeader(ph)) < 0) {
			return rc;
		}

		//Marks header dirty and unip
		if (((rc = pif.MarkDirty(0)) || (rc = pif.UnpinPage(0))) < 0) {
			return rc;
		}

		//Forces pages
		if ((rc = fileHandle.ForcePages()) < 0) {
			return rc;
		}
	}

	// Close the file
    fileHandle.bFileOpen = false;
    if ( (rc = pfm.CloseFile(pif)) ) {
		return rc;
	}

	return 0;
}
