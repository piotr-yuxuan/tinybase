//
// File:        rm_pageheader.cc
// Description: RM_PageHeader class implementation
//

#include <unistd.h>
#include <sys/types.h>
#include "rm.h"

//
// Class RM_PageHeader declaration
//

//Constructor
PageHeader::PageHeader(int nbSlots){
    this->freeSlots = new Bitmap(nbSlots);
}

//Destructor
PageHeader::~PageHeader(){
    delete freeSlots;
}

//Writes into a buffer
int PageHeader::to_buf(char *& buf) const{
    int offset(0);
    //Writes the next free page
    memcpy(buf + offset, &nextFreePage, sizeof(nextFreePage));
    offset += sizeOf(nextFreePage);
    //Writes the number of slots
    memcpy(buf + offset, &nbSlots, sizeof(nbSlots));
    offset += sizeOf(nbSlots);
    //Writes the number of free slots
    memcpy(buf + offset, &nbFreeSlots, sizeof(nbFreeSlots));
    offset += sizeOf(nbFreeSlots);
    //Writes the bitMap using Bitmap method
    this->freeSlots->to_buf(buf+offset);
    return 0;
}

//Reads from a buffer
int PageHeader::from_buf(char *& buf) const{
    int offset(0);
    //Reads the next free page
    memcpy(&nextFreePage, buf + offset, sizeof(nextFreePage));
    offset += sizeOf(nextFreePage);
    //Reads the number of slots
    memcpy(&nbSlots, buf + offset, sizeof(nbSlots));
    offset += sizeOf(nbSlots);
    //Reads the number of free slots
    memcpy(&nbFreeSlots, buf + offset, sizeof(nbFreeSlots));
    offset += sizeOf(nbFreeSlots);
    //Reads the bitMap using Bitmap method
    this->freeSlots->from_buf(buf+offset);
    return 0;
}
