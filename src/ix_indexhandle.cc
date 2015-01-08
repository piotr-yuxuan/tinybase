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

/*
 * Standard constructor
 */
IX_IndexHandle::IX_IndexHandle() {
    this->bFileOpen = false;
}

/*
 * Destructor
 */
IX_IndexHandle::~IX_IndexHandle() {
    //Nothing to do for now
}

/*
 * Inserts a new entry using the insertions methods
 */
RC IX_IndexHandle::InsertEntry(void *pData, const RID &rid) {

}

RC IX_IndexHandle::DeleteEntry(void *pData, const RID &rid) {
	RC rc = 0;
	return rc;
}

/*
 * This implementation design has been inspired by the textbook of INF345:
 * Database Management Systems.
 *
 * To understand that procedure better: First we insert a value into the root,
 * we enter beneath the first if and we recursively insert the value until we
 * reach a leaf node. We keep going and finally reach the maximum number so we
 * split it. Remember the splitting bursts forth from the very bottom – leaf
 * nodes – then propagates above.
 */
RC IX_IndexHandle::insertion(IX_IndexHandle *nodepointer, Entry entry,
		IX_IndexHandle *newchildentry) {

	// If it's not a leaf
	if (*nodepointer->NodeType < 1) {
		int i = 0;
		// Let's choose subtree.
		while (i <= 2 * Order
				&& !(((Entry) *nodepointer->Labels[i]).value < entry.value)) {
			i++;
		}

		// Recursive insert. Split arises from here.
		insertion(&((IX_IndexHandle) *nodepointer->Pointers[i]), entry,
				newchildentry);

		// Do we need to split?
		if (newchildentry == NULL) {
			// No split we're safe.
			return 0;
		} else
		// We need to handle the split. newchildentry contains 'free' entries
		// we need to relocate.
		{
			if (*nodepointer->NumElements < 2 * Order) {
				// We can host new child entry.
				*newchildentry->Parent = nodepointer;
				*nodepointer->Pointers[NumElements] = (void *) newchildentry;
				(*nodepointer->NumElements)++;
				return 0;
			} else
			// The current node is already full so we need to
			// propagate overhead.
			{
				IX_IndexHandle node = split();
				// Euh, faut peut-être l'enregistrer quelque part avant de retourner.
				// Ici on retourne un pointeur vers une variable temporaire, non ?
				newchildentry = &node;

				node.Parent =
						(*nodepointer->NodeType == -1) ? NULL : nodepointer;
				return 0;
			}
		}
	}
	// Else it's a leaf node.
	else {
		if (*nodepointer->NumElements < 2 * Order) {
			// Easy case, we merely add the new entry then simply return.
			*nodepointer->Pointers[NumElements] = (void *) newchildentry;
			(*nodepointer->NumElements)++;
			// TODO Here we should ensure sort. Methinks we don't need any
			// magic stuff like quick-sort, just give the new entry
			// a correct position.
			return 0;
		} else
		// The current leaf is full, let's burst forth!
		{
			IX_IndexHandle node = split();
			node.Parent = *nodepointer->Parent;
			newchildentry = &node;
			return 0;
		}
	}
}

/*
 * This function is called if and only if there are $2 * order + 1$
 * pointers and $2 * order + 2$ labels. Hereby we keep the $order$ first items
 * and return the others packed in a brand new IX_IndexHandle object. This
 * latter don't need its attribute Parent to be set because it will be
 * later assigned. Just remember it's a leaf node.
 */
IX_IndexHandle IX_IndexHandle::split() {
	// TODO
	return this;
}

/*
 * Let's describe the deletion steps used. First we recursively sink from the
 * root to the final leaf which contains the to-be-deleted entry and we
 * eventually delete it. Then, we verify current leaf children number to be
 * acceptable, that's to say strictly greater than $order$. If yes that's all
 * folks. Else we put the children in the *newchildentry pointer and we return.
 *
 * The overhead recursive function checks whether the latter points to NULL or
 * to a real object. If null it means deletion was performed accordingly else
 * it implies the concerned leaf node must be deleted and its children must be
 * stored elsewhere. Then the node inserts the free entries from its parent. It
 * could perform insertion from itself anyway but it may fasten process a bit
 * to do so from the parent.
 */
RC IX_IndexHandle::deletion(IX_IndexHandle *nodepointer, Entry entry,
		IX_IndexHandle *newchildentry) {

	// If it's not a leaf
	if (*nodepointer->NodeType < 1) {
		int i = 0;
		// Let's choose subtree.
		while (i <= 2 * Order
				&& !(((Entry) *nodepointer->Labels[i]).value < entry.value)) {
			i++;
		}

		// Sink beneath until we got the leave. Split arises from here.
		deletion(&((IX_IndexHandle) *nodepointer->Pointers[i]), entry,
				newchildentry);

		// Do we need to reorder?
		if (newchildentry == NULL) {
			// No we don't need so we can quietly return.
			return 0;
		} else {
			// We need to reorder and insert again volatile entries contained
			// in newchildentry.

			/*
			 * Few lines above, the deletion() procedure has made a node whith
			 * not more enough children so we need to delete it.
			 */
			; // remove *nodepointer->Pointers[i] but keep its children in
			  // newchildentry. Remember to set *next and *previous pointers
			  // correctly.
			; // Then, for each child in newchildentry
			{
				Entry freeEntry; // current entry.
				IX_IndexHandle subnewchildentry = NULL;
				// We insert it into nodepointer's parent node. We use the
				// parent to ensure it can't be a leaf node. Then this function
				// will do the job to insert this entry.
				insertion(nodepointer->Parent, freeEntry, &subnewchildentry);
			}
		}
	}
	// Else it's a leaf node.
	else {
		; // We eventually look for the entry and reorder the linked list.
		  // Have we found the entry?
		if (/*found == */1) {
			; // if we've found the entry, we delete it and reorder the
			  //linked list.
		} else {
			; // The entry doesn't exist therefore can't be deleted. This
			  // issues a non-zéro return code.
			return -1; // TODO positive or negative?
		}; // After that we check: has this node enough children or must it be merged?
		if (Order < *nodepointer->NumElements) {
			// It still has enough children so we set *newchildentry to NULL
			// and we return.
			*newchildentry = NULL;
			return 0;
		} else {
			// It doesn't have children enough to survive.
			;// We put its remaining children in newchildentry
			return 0; // and we return.
		}
	}
}

RC IX_IndexHandle::ForcePages() {
	RC rc = 0;
	return rc;
}
