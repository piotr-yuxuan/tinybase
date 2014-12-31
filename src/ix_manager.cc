class IX_Manager {
public:
	IX_Manager(PF_Manager &pfm);
	~IX_Manager();

	// Create a new Index
	RC CreateIndex(const char *fileName, int indexNo, AttrType attrType,
			int attrLength);

	// Destroy and Index
	RC DestroyIndex(const char *fileName, int indexNo);

	// Open an Index
	RC OpenIndex(const char *fileName, int indexNo,
			IX_IndexHandle &indexHandle);

	// Close an Index
	RC CloseIndex(IX_IndexHandle &indexHandle);
};
