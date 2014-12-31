#include <cstdio>
#include <unistd.h>
#include <iostream>
#include "ix.h"

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
}

RC IX_IndexHandle::InsertEntry(void *pData, const RID &rid) {
	RC rc = 0;

	if (NodeType < 1) {
		// Get the leaf-level child pointer whithin the entry should be inserted in.
	} else {
		// So it's a leaf-level child

		if (NumAcceptable(NumElements + 1)) {
			; // Performs effective insertion
			NumElements++; // Updates NumElements
			Balance();
		} else {
			; // Create new child;
			Parent.Balance(); // Balance the result
		}
	}
	return rc;
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
