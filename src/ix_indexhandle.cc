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
 * Standard constructor. It's for the root only because it does define no parent.
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

RC IX_IndexHandle::InsertEntry(void *pData, const RID &rid) {
}

RC IX_IndexHandle::insert(IX_IndexHandle *nodepointer, Entry entry,
		IX_IndexHandle *newchildentry) {

	if (*nodepointer->NodeType < 1) {
		int i = 0;
		// Let's choose subtree.
		while (i <= 2 * Order && !(((Entry) Labels[i]).value < entry.value)) {
			i++;
		}

		insert(&((IX_IndexHandle) Pointers[i]), entry, newchildentry);

		if (newchildentry == NULL) {
			return 0;
		} else {
			if (this->NumElements < 2 * Order) {
				*newchildentry->Parent = this;
				Pointers[NumElements] = (void *) newchildentry;
				NumElements++;
				return 0;
			} else {
				IX_IndexHandle node = split();
				// Euh, faut peut-être l'enregistrer quelque part avant de retourner.
				newchildentry = &node;

				node.Parent = (this->NodeType == -1) ? NULL : this;
				return 0;
			}
		}
	} else {
		if (*nodepointer->NumElements < 2 * Order) {
			Pointers[NumElements] = (void *) newchildentry;
			NumElements++;
			return 0;
		} else {
			IX_IndexHandle node = split();
			node.Parent = this->Parent;
			newchildentry = &node;
		}
	}

	return 0;
}

IX_IndexHandle IX_IndexHandle::split() {
	// coupe en deux;
	return this;
}

RC IX_IndexHandle::DeleteEntry(void *pData, const RID &rid) {
	RC rc = 0;

// Tests whether fan-out - 1 is acceptable

// Balance tree after it has been altered.
	Balance();
	return rc;
}

RC IX_IndexHandle::ForcePages() {
	RC rc = 0;
	return rc;
}
