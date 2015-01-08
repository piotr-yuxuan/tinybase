#include <cstdio>
#include <unistd.h>
#include <iostream>
#include "ix.h"
#include "ix_internal.h"

using namespace std;

/*
 * Implementation sketch of a B+ tree. This structure gracefully reorganise
 * itself when needed.
 *
 * <pre>
 * 0    1    2    3    4    5    ← pointers
 * || 0 || 1 || 2 || 3 || 4 ||   ← labels
 * </pre>
 *
 * The node pointer to the left of a key k points to a subtree that contains
 * only data entries less than k. A The node pointer to the right of a key value
 * k points to a subtree that contains only data entries greater than or equal
 * to k.
 */

//Constructor
IX_IndexHandle::IX_IndexHandle() {
    this->bFileOpen = false;
}

//Destructor
IX_IndexHandle::~IX_IndexHandle() {
    //Nothing to do for now
}

//Inserts a new entry using the insertions methods
RC IX_IndexHandle::InsertEntry(void *pData, const RID &rid) {
    RC rc = 0;
    //If the root number is not defined we create a root
    if(this->fileHeader.rootNb==-1){
        //Allocates a new page for the root
        PF_PageHandle pageHandle;
        if(this->filehandle->AllocatePage(pageHandle)) return rc;
        //Creates a header for the root
        IX_NodeHeader rootHeader;
        rootHeader.level = -1;
        rootHeader.maxKeyNb = IX_IndexHandle.Order*2;
        rootHeader.nbKey = 0;
        rootHeader.prevPage = -1;
        rootHeader.nextPage = -1;
        rootHeader.parentPage = -1;
        //Copy the header of the root to memory
        char* pData2;
        if(rc = pageHandle.GetData(pData2)) return rc;
        memcpy(pData2, &rootHeader, sizeof(IX_NodeHeader));
        //Updates the root page number in file header
        PageNum nb;
        if(rc = pageHandle.GetPageNum(nb)) return rc;
        this->fileHeader.rootNb = nb;
        //Marks dirty and unpins
        if( (rc = filehandle->MarkDirty(nb)) || (rc = filehandle->UnpinPage(nb)) ) return rc;
    }
    //The root is either created or exists, we can insert
    return InsertEntryToNode(this->fileHeader.rootNb, pData, rid);
}

//Inserts a new entry to a specified node
RC IX_IndexHandle::InsertEntryToNode(const PageNum nodeNum, void *pData, const RID &rid) {
    RC rc = 0;
    //Retrieves the IX_NodeHeader of the node
    PF_PageHandle pageHandle;
    IX_NodeHeader nodeHeader;
    if( (rc = filehandle->GetThisPage(nodeNum, pageHandle)) ) return rc;
    char* pData2;
    if( (rc = pageHandle.GetData(pData2)) ) return rc;
    memcpy(&nodeHeader, pData2, sizeof(IX_NodeHeader)); //"Loads" the nodeHeader from memory
    //Acts according to the type of node
    if(nodeHeader.level==1){
        //Case leaf
        return InsertEntryToLeafNode(nodeNum, pData, rid);
    }else{
        //Case not leaf: looks for the right children
        for(int i=0; i<nodeHeader.nbKey; i++){
            //If we find a key greater than value we have to take the pointer on its left
            if(IsKeyGreater(pData, pageHandle, i)>=0){
                PageNum nb;
                if( (rc = getPageNumber(pageHandle, i, nb)) ) return rc;
                //Reccursive call to insertEntreToNode
                InsertEntryToNode(nb, pData, rid);
            }
        }
    }
}

//Inserts a new entry to a leaf node
RC IX_IndexHandle::InsertEntryToLeafNode(const PageNum nodeNum, void *pData, const RID &rid) {
    RC rc = 0;
    //Retrieves the header of the leaf
    IX_NodeHeader leafHeader;
    PF_PageHandle pageHandle;
    char* pData2;
    if( (rc = filehandle->GetThisPage(nodeNum, pageHandle)) ) return rc;
    if( (rc = pageHandle.GetData(pData2)) ) return rc;
    memcpy(&leafHeader, pData2, sizeof(IX_NodeHeader));
    //First we check if the key already exists
    for(int i=0; i<leafHeader.nbKey; i++){
        if(IsKeyGreater(pData, pageHandle, i)==0){
            //TODO
            /*Ici il faudra récupérer l'adresse du bucket associé à i
             *et y insérer la valeur
            */
        }
    }
    //We are here means the key doesn't exist already in the leaf
    if(leafHeader.nbKey<leafHeader.maxKeyNb){
        //Calls the method to handle the easy case
        return InsertEntryToLeafNodeNoSplit(nodeNum, pData, rid);
    }else{
        //Calls the method to handle the split case
        return InsertEntryToLeafNodeSplit(nodeNum, pData, rid);
    }
}

//Compares the given value (pData) to number i key on the node (pageHandle)
int IX_IndexHandle::IsKeyGreater(void *pData, PF_PageHandle pageHandle, int i){
    //TODO
    /*
     *C'est méthode la il faudra l'implémenter ça ne devrait pas être très compliqué
     *On nous donne pData qui est la data de la donnée à insérer et on a un pageHandle
     *pour le noeud en cours, ainsi que i qui dit quelle clé du noeud en cour son regarde
     *Et la méthode doit comparer la clé i du noeud à la valeur donnée par pData et nous
     *retourner un entier positif si la clé i est plus grande, négatif si plus petite
     *et nul si les deux clés sont égales
    */
}

//Gives the PageNumber (i.e. the pointer) for number i key on the node (pageHandle)
RC IX_IndexHandle::getPageNumber(PF_PageHandle pageHandle, int i, PageNum &pageNumber){
    //TODO
    /*
     *Cette méthode est dans la même veine que la précédente. On nous donne
     *le pageHandle du noeud, l'entier i qui désigne la clé à laquelle on
     *s'intéresse dans le noeud, et une variable pageNumber dans laquelle la
     *fonction doit mettre la valeur du numéro de la page associé à la clé i
    */
}






