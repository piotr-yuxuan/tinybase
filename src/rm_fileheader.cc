//
// File:        rm_fileheader.cc
// Description: RM_FileHeader class implementation
//

#include <unistd.h>
#include <sys/types.h>
#include "rm.h"

//
// Class RM_FileHeader declaration
//

//Constructor
RM_FileHeader::RM_FileHeader(){
    //Nothing to do for now
}

//Destructor
RM_FileHeader::~RM_FileHeader(){
    //Nothing to d for now
}

//Writes into a buffer
int RM_FileHeader::to_buf(char *& buf) const{
    int offset(0);
    //Writes the first free page
    memcpy(buf + offset, &firstFreePage, sizeof(firstFreePage));
    offset += sizeOf(firstFreePage);
    //Writes the number of pages
    memcpy(buf + offset, &pagesNumber, sizeof(pagesNumber));
    offset += sizeOf(pagesNumber);
    //Writes the size of records
    memcpy(buf + offset, &recordSize, sizeof(recordSize));
    return 0;
}

//Reads from a buffer
int RM_FileHeader::from_buf(char *& buf) const{
    int offset(0);
    //Reads the first free page
    memcpy(&firstFreePage, buf + offset, sizeof(firstFreePage));
    offset += sizeOf(firstFreePage);
    //Reads the number of pages
    memcpy(&pagesNumber, buf + offset, sizeof(pagesNumber));
    offset += sizeOf(pagesNumber);
    //Reads the size of records
    memcpy(&recordSize, buf + offset, sizeof(recordSize));
    offset += sizeOf(recordSize);
    return 0;
}

//Getter for recordSize
int RM_FileHeader::getRecordSize() const{
    return this->recordSize;
}

//Getter for pagesNumber
int RM_FileHeader::getPagesNumber() const{
    return this->pagesNumber;
}
