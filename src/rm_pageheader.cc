//
// File:        RM_PageHeader.cc
// Description: RM_PageHeader class implementation
//

#include <unistd.h>
#include <sys/types.h>
#include "rm.h"

//
// Class RM_PageHeader declaration
//

//Constructor
RM_PageHeader::RM_PageHeader(int nbSlots){
    freeSlots = Bitmap(nbSlots);
}

//Destructor
RM_PageHeader::~RM_PageHeader(){
    //Nothing to do at the moment
}

//Returns size of the header in bytes
int RM_PageHeader::size() const{
    return sizeof(nextFreePage)+sizeof(nbSlots)+sizeof(nbFreeSlots)+freeSlots.getSize();
}

//Writes into a buffer
int RM_PageHeader::to_buf(char *& buf) const{
    int offset(0);
    //Writes the next free page
    memcpy(buf + offset, &nextFreePage, sizeof(nextFreePage));
    offset += sizeof(nextFreePage);
    //Writes the number of slots
    memcpy(buf + offset, &nbSlots, sizeof(nbSlots));
    offset += sizeof(nbSlots);
    //Writes the number of free slots
    memcpy(buf + offset, &nbFreeSlots, sizeof(nbFreeSlots));
    offset += sizeof(nbFreeSlots);
    //Writes the bitMap using Bitmap method
    buf += offset;
    this->freeSlots.to_buf(buf);
    buf -= offset;
    return 0;
}

//Reads from a buffer
int RM_PageHeader::from_buf(const char * buf) {
    int offset(0);
    //Reads the next free page
    memcpy(&nextFreePage, buf + offset, sizeof(nextFreePage));
    offset += sizeof(nextFreePage);
    //Reads the number of slots
    memcpy(&nbSlots, buf + offset, sizeof(nbSlots));
    offset += sizeof(nbSlots);
    //Reads the number of free slots
    memcpy(&nbFreeSlots, buf + offset, sizeof(nbFreeSlots));
    offset += sizeof(nbFreeSlots);
    //Reads the bitMap using Bitmap method
    this->freeSlots.from_buf(buf+offset);
    return 0;
}


//Returns the number of free slots
int RM_PageHeader::getNbFreeSlots() {
    int freeSlotsCount = 0;
    for(int i=0; i<this->size(); i++){
        freeSlotsCount += freeSlots.test(i);
    }
    return freeSlotsCount;
}

//Getter for nextFreePage
int RM_PageHeader::getNextFreePage(){
    return this->nextFreePage;
}

//Setter for nextFreePage
RC RM_PageHeader::setNextFreePage(int i) {
    this->nextFreePage = i;
    return 0;
}
