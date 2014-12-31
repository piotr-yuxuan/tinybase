class IX_IndexScan {
public:
	IX_IndexScan(); // Constructor
	~IX_IndexScan(); // Destructor
	RC OpenScan(const IX_IndexHandle &indexHandle, // Initialize index scan
			CompOp compOp, void *value, ClientHint pinHint = NO_HINT);
	RC GetNextEntry(RID &rid); // Get next matching entry
	RC CloseScan(); // Terminate index scan
};
