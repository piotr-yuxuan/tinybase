//
//  rm_manager.cpp
//  tinybase
//
//  Created by Pauline RABIS on 16/12/2014.
//  Copyright (c) 2014 Pauline RABIS. All rights reserved.
//

#include "rm.h"
#include <cstdio>
#include <cstring>
#include <cassert>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>


RM_Manager::RM_Manager(PF_Manager &pfm) : pfm(pfm) {}


RM_Manager::~RM_Manager(){} /** Destructor */

RC RM_Manager::CreateFile(const char *fileName, int recordSize){
    /**RC monFichier = PF_Manager::CreateFile(fileName);
     RM_FileHeader* monHeader = new RM_FileHeader();
     RM_FileHandle* monGestionnaire = new RM_FileHandle();
     monGestionnaire->RM_FileHandle::SetFileHeader(monHeader);
     
     monHeader. */
    l0vely*c0mplex
    
}/** create a new file */

RC RM_Manager::DestroyFile(const char *fileName){
    RC RC = pfm.DestroyFile(fileName);
    
    if (RC < 0){
        
        PF_PrintError(RC);
        return RM_PF;
    }
    return 0;
}/**Destroy a file */

RC RM_Manager::OpenFile(const char *fileName, RM_FileHandle &fileHandle){
    PF_FileHandle pfh;
    PF_Manager pfm;
    PF_PageHandle ph;
    char * pData;
    RM_FileHeader hdr;
    RC rc = pfm.OpenFile(fileName, pfh);
    
    if (rc < 0)
    {
        PF_PrintError(rc);
        return RM_PF;
    }
    // header page is at 0
    
    if ((rc = pfh.GetThisPage(0, ph)) || (rc = ph.GetData(pData))){
        return(rc);
    }
    memcpy(&hdr, pData, sizeof(hdr));
    rc = fileHandle.Open(&pfh, hdr.recordSize);
    
    if (rc < 0){
        RM_PrintError(rc);
        return rc;
    }
    rc = pfh.UnpinPage(0);
    
    if (rc < 0){
        PF_PrintError(rc);
        return rc;
    }
    
    return 0;
}/** open a file */

RC RM_Manager::CloseFile(RM_FileHandle& fileHandle){
    RC rc;
    PF_PageHandle ph;
    PageNum page;
    char *pData;
    
    // If header was modified, put the first page into buffer again,
    // and update its contents, marking the page as dirty
    if(fileHandle.header_modified == true){
        
        PF_FileHandle* pif = fileHandle.pf_FileHandle;//->RM_FileHandle::GetPF_FileHandle();
        
        if((rc = pif->GetFirstPage(ph)) || ph.GetPageNum(page))
            return (rc);
        if((rc = ph.GetData(pData))){
            RC rc2;
            if((rc2 = pif->UnpinPage(page)))
                return (rc2);
            return (rc);
        }
        memcpy(pData, &fileHandle.fileHeader, sizeof(struct RM_FileHeader));
        if((rc = pif->MarkDirty(page)) || (rc = pif->UnpinPage(page)))
            return (rc);
        
    }
    
    // Close the file
    if((rc = pfm.CloseFile(pif)))
        return (rc);
    
    return (0);
}/** Close a file */
