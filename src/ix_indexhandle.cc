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
 * Standard constructor. It's for root only because it doesn't define parent.
 */
IX_IndexHandle::IX_IndexHandle() {
	NumElements = 0;
	this->NodeType = -1; // the root
	Pointers = LinkList<void>();
	Labels = new void*[2 * Order];
	Parent = NULL;
}

IX_IndexHandle::IX_IndexHandle(int NodeType, IX_IndexHandle &Parent) {
	NumElements = 0;
	this->NodeType = NodeType;
	Pointers = LinkList<void>();
	Labels = new void*[2 * Order];
	this->Parent = Parent;
}

/*
 * Terminator
 */
IX_IndexHandle::~IX_IndexHandle() {
	// Je ne sais pas trop quoi mettre dedans.
}

/*
 * TODO This syntax must be controlled. We get a parameter with & and we also
 * send the address &newchildentry as a pointer.
 */
RC IX_IndexHandle::InsertEntry(void *pData, const RID &rid) {
	void *value;
	Entry entry = { value, rid };
	// Whatever it points to, this initial value will be never read.
	IX_IndexHandle newchildentry = NULL;
	insert(this, entry, &newchildentry);
}

/*
 * This implementation design has been inspired by the textbook of INF345:
 * Database Management Systems.
 *
 * TODO Warning, check order definition to know whether a leaf node can host
 * the maximum number of order or 2 * order entry values. Change must be
 * applied thoroughly.
 *
 * To understand that procedure better: First we insert a value into the root,
 * we enter beneath the first if and we recursively insert the value until we
 * reach a leaf node. We keep going and finally reach the maximum number so we
 * split it. Remember the splitting bursts forth from the very bottom – leaf
 * nodes – then propagates above.
 */
RC IX_IndexHandle::insert(IX_IndexHandle *nodepointer, Entry entry,
		IX_IndexHandle *newchildentry) {

	// If it's not a leaf
	if (*nodepointer->NodeType < 1) {
		int i = 0;
		// Let's choose subtree.
		while (i <= 2 * Order && !(((Entry) Labels[i]).value < entry.value)) {
			i++;
		}

		// Recursive insert. Split arises from here.
		insert(&((IX_IndexHandle) Pointers[i]), entry, newchildentry);

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
				Pointers[NumElements] = (void *) newchildentry;
				NumElements++;
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
	// Else it's a leaf
	else {
		if (*nodepointer->NumElements < 2 * Order) {
			// Easy case, we merely add the new entry then simply return.
			Pointers[NumElements] = (void *) newchildentry;
			NumElements++;
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
		}
	}

	return 0;
}

/*
 * This function is called if and only if there are $2 * order + 1$
 * pointers and $2 * order + 2$ labels. Hereby we keep the $order$ first items
 * and return the others packed in a brand new IX_IndexHandle object. This
 * latter don't need its attribute Parent to be set because it will be
 * later assigned. Just remember it's a leaf node.
 */
IX_IndexHandle IX_IndexHandle::split() {
	// coupe en deux;
	return this;
}

RC IX_IndexHandle::DeleteEntry(void *pData, const RID &rid) {
	RC rc = 0;
	return rc;
}

RC IX_IndexHandle::ForcePages() {
	RC rc = 0;
	return rc;
}
