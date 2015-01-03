package bplustrees;

/*
 * Implementation sketch of a B+ tree. This structure gracefully reorganise
 * itself when needed.
 *
 * <pre>
 * 0    1    2    3    4    5    ← references
 * || 0 || 1 || 2 || 3 || 4 ||   ← keyValues
 * </pre>
 *
 * The node pointer to the left of a key k points to a subtree that contains
 * only data entries less than k. A The node pointer to the right of a key value
 * k points to a subtree that contains only data entries greater than or equal
 * to k.
 */

public class IX_IndexHandle {

	public static int order = 5;

	private int children = 0;
	private int nodeType = -1; // the root
	private IX_IndexHandle parent = null;
	private Object[] references = new IX_IndexHandle[2 * order];
	private Object[] keyValues = new Value[2 * order + 1];

	public IX_IndexHandle() {
		// All has been initialised correctly.
	}

	private IX_IndexHandle(int nodeType, IX_IndexHandle parent) {
		this.nodeType = nodeType;
		this.parent = parent;
		if (nodeType == 1) {
			// leaf
			this.references = new Data[2 * order];
			this.keyValues = null;
		}
	}

	public RC InsertEntry(Data data, RID rid) {
		InternalInsertEntry(data, rid);
		return RC.OK;
	}

	private IX_IndexHandle split() {
		IX_IndexHandle rest = new IX_IndexHandle(1, this.parent);
		int i = order;
		for (; i < 2 * order; i++) {
			rest.references[i - order] = references[i];
			references[i] = null;
		}
		return rest;
	}

	private IX_IndexHandle InternalInsertEntry(Data data, RID rid) {

		// Is it a non-leaf node?
		if (this.nodeType < 1) {
			int i = 0;
			// Let's choose subtree.
			while (i <= 2 * order
					&& !(data.getValue().compareTo((Value) keyValues[i]) > -1)) {
				// We want keyValues[i] <= data.getValue()
				i++;
			}
			// So now we have keyValues[i] <= data.getValue() < keyValues[i+1].

			// Recursively insert entry.
			IX_IndexHandle back = ((IX_IndexHandle) references[i])
					.InternalInsertEntry(data, rid);

			if (back == null) {
				return null;
			} else {
				// Is there space?
				if (children < 2 * order) {
					this.references[children] = back;
					if (back.parent == null) {
						back.parent = this.parent;
					}
					children++;
					return null;
				} else {// TODO
					IX_IndexHandle rest = this.split();
					// If this is the root, the root has just been split.
					rest.parent = (this.nodeType == -1) ? this : null;
					return rest;
				}
			}
		}

		// This is a leaf node and this.nodeType == 1.
		else {
			if (children < 2 * order) {
				// usual case
				references[children] = data;
				// It needs to be quicksorted.
				children++;
				return null;
			} else {
				// once in a while, the leaf is full then split so d entries
				// stay and the rest is moved to a new node.
				IX_IndexHandle rest = this.split();
				return rest;
			}
		}
	}

	public RC DeleteEntry(Data pData, RID rid) {
		return RC.OK;
	}

	public RC ForcePages() {
		return RC.OK;
	}
}
