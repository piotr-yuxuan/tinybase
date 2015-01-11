#include <cstdio>
#include <unistd.h>
#include <iostream>
#include "ix.h"
#include <string.h>


/*
 *TODO faire que le nom du fichier soit composé du filename donné ET du numéro d'index
*/

IX_Manager::IX_Manager(PF_Manager &pfm) {
    this->pfManager = &pfm;
}
IX_Manager::~IX_Manager() {
}

// Create a new Index
RC IX_Manager::CreateIndex(const char *fileName, int indexNo, AttrType attrType,
		int attrLength) {
    RC rc = 0;
    //Creates file with PF
    if( (rc = pfManager->CreateFile(fileName)) ) return rc;
    //Opens it
    PF_FileHandle fileHandle;
    pfManager->OpenFile(fileName, fileHandle);
    //Creates IX_FileHeader for the file
    IX_FileHeader fileHeader;
    fileHeader.attrType = attrType;
    fileHeader.keySize = attrLength;
    fileHeader.rootNb = -1;
    //Allocates page for the header
    PF_PageHandle pageHandle;
    if( (rc = fileHandle.AllocatePage(pageHandle)) ) return rc;
    char * pData;
    if( (rc = pageHandle.GetData(pData)) ) return rc;
    //And writes fileHeader to it
    memcpy(pData, &fileHeader, sizeof(IX_FileHeader));
    PageNum nb;
    if( (rc = pageHandle.GetPageNum(nb)) || (rc = fileHandle.MarkDirty(nb))
            || (rc = fileHandle.UnpinPage(nb)) || (rc = fileHandle.ForcePages()) ) return rc;

    return pfManager->CloseFile(fileHandle);
}

// Destroy and Index
RC IX_Manager::DestroyIndex(const char *fileName, int indexNo) {
    return pfManager->DestroyFile(fileName);
}

// Open an Index
RC IX_Manager::OpenIndex(const char *fileName, int indexNo,
		IX_IndexHandle &indexHandle) {
    RC rc = 0;
    //Checks not already open
    if(indexHandle.bFileOpen) return IX_FILEOPEN;
    //Opens in PF
    PF_FileHandle fileHandle;
    if( (rc = pfManager->OpenFile(fileName, fileHandle)) ) return rc;
    //Marks open
    indexHandle.bFileOpen = true;
    indexHandle.filehandle = new PF_FileHandle(fileHandle);
    //Instanciate fileHeader
    PF_PageHandle pageHandle;
    if( (rc = fileHandle.GetThisPage(0, pageHandle)) || (rc = fileHandle.UnpinPage(0)) ) return rc;
    char* pData;
    if( (rc = pageHandle.GetData(pData)) ) return rc;
    memcpy( &(indexHandle.fileHeader), pData, sizeof(IX_FileHeader)); //Copies from memory

    return 0;
}

// Close an Index
RC IX_Manager::CloseIndex(IX_IndexHandle &indexHandle) {
    RC rc = 0;
    //Already closed?
    if(!indexHandle.bFileOpen) return IX_CLOSEDFILE;

    //We have to write the file header back
    char * pData ;
    PF_PageHandle pageHandle;
    if( (rc = indexHandle.filehandle->GetThisPage(0, pageHandle)) ) return rc;
    if( (rc = pageHandle.GetData(pData)) ) return rc;

    memcpy( pData, &(indexHandle.fileHeader), sizeof(IX_FileHeader)); //Copies to memory

    if( (rc = indexHandle.filehandle->MarkDirty(0))
            || (rc = indexHandle.filehandle->UnpinPage(0))
            || (rc = indexHandle.filehandle->ForcePages(0))
            || (rc = this->pfManager->CloseFile(*(indexHandle.filehandle))) ) return rc;

    indexHandle.bFileOpen = false;

    return 0;
}
