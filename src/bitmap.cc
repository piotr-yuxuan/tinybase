#include "rm.h"
#include <cmath>
#include <cstring>
#include <cassert>

//Constructor
Bitmap::bitmap(int nbBits):
{
    this->size(nbBits);
    //Allocates enough memory (could be improved in the future)
    this->bitValues = new char[(size / 8)+1];
    //Sets all bits to 0
    this->reset();
}

//Destructor
Bitmap::~bitmap()
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
bool Bitmap::test(unsigned int bitNumber){
    assert(bitNumber <= size - 1);
    int byte = bitNumber/8;
    int offset = bitNumber%8;
    //Return the actual value
    return this->bitValues[byte] & (1 << offset);
}
