//
// File:        bitmap.cc
// Description: Bitmap class implementation
//
// Note: 1 means that the slot is free, 0 means there is data in it
//


#include "rm.h"
#include <cmath>
#include <cassert>

//Constructor
Bitmap::Bitmap()
{
    this->bitValues = NULL;
}
Bitmap::Bitmap(int nbBits)
{
    this->size = nbBits;
    //Allocates enough memory (could be improved in the future)
    this->bitValues = new char[this->getByteSize()];
    //Sets all bits to 0
    this->reset();
}

//Destructor
Bitmap::~Bitmap()
{
    delete [] this->bitValues;
}

//Sets a specific bit to 1
void Bitmap::set(unsigned int bitNumber){
    assert(bitNumber <= size - 1);
    int byte = bitNumber/8;
    int offset = bitNumber%8;
    //Actually puts the bit to 1
    this->bitValues[byte] |= (1 << offset);
}

//Sets a specific bit to 0
void Bitmap::reset(unsigned int bitNumber){
    assert(bitNumber <= size - 1);
    int byte = bitNumber/8;
    int offset = bitNumber%8;
    //Actually puts the bit to 0
    this->bitValues[byte] &= ~(1 << offset);
}

//Sets all bits to 1
void Bitmap::set(){
    for(unsigned int bitNumber=0; bitNumber<this->size; bitNumber++){
        this->set(bitNumber);
    }
}

//Sets all bits to 0
void Bitmap::reset(){
    for(unsigned int bitNumber=0; bitNumber<this->size; bitNumber++){
        this->reset(bitNumber);
    }
}

//Returns a specific bit value
bool Bitmap::test(unsigned int bitNumber) const{
    assert(bitNumber <= size - 1);
    int byte = bitNumber/8;
    int offset = bitNumber%8;
    //Return the actual value
    return this->bitValues[byte] & (1 << offset);
}

//Writes into a buffer
bool Bitmap::to_buf(char *& buf) const{
    memcpy(buf, &bitValues, sizeof(bitValues));
    return 0;
}

//Reads from a buffer
bool Bitmap::from_buf(const char * buf) {
    memcpy(bitValues, buf, sizeof(bitValues));
    return 0;
}

//Gets the size in Bytes of the map (simplist implementation for now)
int Bitmap::getByteSize() const{
    return (size / 8)+1;
}
