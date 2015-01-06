IX_Manager::IX_Manager(PF_Manager &pfm) {
}
IX_Manager::~IX_Manager() {
}

// Create a new Index
RC IX_Manager::CreateIndex(const char *fileName, int indexNo, AttrType attrType,
		int attrLength) {
	return 0;
}

// Destroy and Index
RC IX_Manager::DestroyIndex(const char *fileName, int indexNo) {
	return 0;
}

// Open an Index
RC IX_Manager::OpenIndex(const char *fileName, int indexNo,
		IX_IndexHandle &indexHandle) {
	return 0;
}

// Close an Index
RC IX_Manager::CloseIndex(IX_IndexHandle &indexHandle) {
	return 0;
}
