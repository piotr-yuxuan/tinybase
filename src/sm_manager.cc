//
// File:        SM component stubs
// Description: Print parameters of all SM_Manager methods
// Authors:     Dallan Quass (quass@cs.stanford.edu)
//

#include <cstdio>
#include <iostream>
#include "tinybase.h"
#include "sm.h"
#include "ix.h"
#include "rm.h"
#include <unistd.h>
#include "printer.h"
#include <fstream>

using namespace std;

SM_Manager::SM_Manager(IX_Manager &ixm, RM_Manager &rmm) {
	this->rmm = &rmm;
	this->ixm = &ixm;
}

SM_Manager::~SM_Manager() {
}

/*
 * Format names of relations and attributes.
 * Relation and attribute names are case-insensitive, limited to MAXNAME = 24 characters each
 * and must begin with a letter.
 */
RC SM_Manager::FormatName(char *string) {
	int length = 0;
	if (tolower(*string) - 'a' < 0 && 'z' - tolower(*string) > 0) {
		return RC(-1);
	}
	while (*string) {
		*string = tolower(*string);
		string++;
		if (++length == MAXNAME) {
			return RC(-1);
		}
	}
	return RC(0);
}

/*
 * This function, alongst with RelFromR, is not widely used here for
 * performance concerns. It's true a lot of almost-duplicated, very similar
 * code could be avoided but it looks better to tweak the code each time
 * for a specific use.
 *
 * A more generic function, returning a RM_Record, may be more accurate but
 * would still create then destroy a filescan.
 *
 * There is no need to format input for this function is private. Input from
 * the user has been formatted yet.
 *
 * TODO: je ne comprends pas du tout, le compilateur me parle d'une errreur de
 * signature mais pourtant c'est bon, n'est-ce pas ?
 */
RC SM_Manager::AttrFromRA(char * relName, char * attrName, DataAttrInfo * dai) {
	RC rc = 0;
	RM_FileScan relfs;
	DataAttrInfo * pData = NULL;
	RM_Record rec; // Record for rtuple.

	if ((rc = relfs.OpenScan(relcat, // we look for the given relation
			STRING, // looking for its name
			MAXNAME + 1, // the former mayn't be wider than this
			0, // null offset because we search on the column relname
			EQ_OP, // we look for *this* relation precisely
			relName // name of the relation
			))) {
	}

	while ((rc = relfs.GetNextRec(rec))) {
		if ((rc = rec.GetData((char *&) pData))) {
			return rc;
		}
		// Test is necessary to avoid the pointer dai to change to inaccurate
		// value which could be returned if nothing is found at the end of
		// the loop.
		if (strcmp(pData->attrName, attrName)) {
			*dai = *pData;
			return rc;
		}
	}

	return RC(-1); // TODO no such attributes for given relation.
}

/*
 * See documentation for AttrFromRA.
 */
RC SM_Manager::RelFromR(char * relName, DataRelInfo * dri) {
	RC rc = 0;
	RM_FileScan fs;
	RM_Record rec; // Record for rtuple.

	if ((rc = fs.OpenScan(relcat, // we look for the given relation
			STRING, // looking for its name
			MAXNAME + 1, // the former mayn't be wider than this
			0, // null offset because we search on the column relname
			EQ_OP, // we look for *this* relation precisely
			relName // name of the relation
			))) {
		return RC(-1);
	}
	// Should be exactly one.
	if ((rc = fs.GetNextRec(rec))) {
		return RC(-1);
	}
	// Set the pointer
	if ((rc = rec.GetData((char *&) dri))) {
		return RC(-1);
	}

	return rc;
}

/**
 * This method, along with method CloseDb, is called by the code implementing
 * the redbase command line utility. This method should change to the directory
 * for the database named *dbName (using system call chdir), then open the
 * files containing the system catalogs for the database.
 */
RC SM_Manager::OpenDb(const char *dbName) {
	RC rc = 0;

	// (try to) Move to the correct directory.
	// TODO Assumes we're in the parent directory.
	if (chdir(dbName) < 0) {
		cerr << " chdir error to " << *dbName << "\n";
		return RC(-1);
	}

	// Opening the files containing the system catalogs for the database.
	if ((rc = rmm->OpenFile("relcat", relcat))) {
		PrintError(rc);
		return rc;
	}
	if ((rc = rmm->OpenFile("attrcat", attrcat))) {
		PrintError(rc);
		return rc;
	}

	return rc;
}

/**
 *  This method should close all open files in the current database.
 *  Closing the files will automatically cause all relevant buffers to be
 *  flushed to disk.
 */
RC SM_Manager::CloseDb() {
	RC rc = 0;

	// Closing all open files in the current database.
	// TODO right now just two files have been opened (to be changed if need be).
	if ((rc = rmm->CloseFile(relcat))) {
		PrintError(rc);
		return rc;
	}
	if ((rc = rmm->CloseFile(attrcat))) {
		PrintError(rc);
		return rc;
	}

	return rc;
}

/**
 * Three parts:
 * #1: we test entries and format them
 * → relation and attribute names are limited to MAXNAME = 24 characters each,
 * and must begin with a letter.
 * → within a given database every relation name must be unique, and within
 * a given relation every attribute name must be unique.
 * #2: working part which is two-fold:
 * 	→ 2.1 We first update the system catalogs: a tuple for the new relation should
 *  be added to a catalog relation called relcat, and a tuple for each
 *  attribute should be added to a catalog relation called attrcat.
 *  → 2.2 After updating the catalogs, method RM_Manager::CreateFile is called
 * to create a file that will hold the tuples of the new relation.
 * #3: cleaning part, we free the memory from transient objects.
 *
 * TODO: error codes still to be implemented.
 */
RC SM_Manager::CreateTable(const char *relName, int attrCount,
		AttrInfo *attributes) {
	RC rc = 0;

	// Converts to lowercase. Methinks we have to do this trick because the argument is `const`.
	char *lowerRelName = (char*) malloc(MAXNAME + 1);
	strcpy(lowerRelName, relName);
	FormatName((char *) lowerRelName);

	RM_FileHandle fh;
	if (!(rc = rmm->OpenFile(lowerRelName, fh))) {
		// File already exists then another database is already named the same.
		free(lowerRelName);
		return RC(-1);
	}

	if (attrCount < 1 || attrCount > MAXATTRS) {
		return RC(-1);
	}

	cout << "CreateTable\n" << "   relName     =" << lowerRelName << "\n"
			<< "   attrCount   =" << attrCount << "\n";

	int offset = 0;
	DataAttrInfo atuple; // atuple is overwritten each iteration.

	for (int i = 0; i < attrCount; i++) {
		AttrInfo a = attributes[i];
		if ((rc = FormatName(a.attrName))) {
			return rc;
		}

		//Checks attribute's length
		switch (a.attrType) {
		case STRING:
			if (a.attrLength <= 0 || a.attrLength > MAXSTRINGLEN) {
				return RC(-1);
			}
			break;
		case FLOAT:
			if (a.attrLength != sizeof(float)) {
				return RC(-1);
			}
			break;
		case INT:
			if (a.attrLength != sizeof(int)) {
				return RC(-1);
			}
			break;
		default:
			return RC(-1);
		}

		atuple.attrLength = attributes[i].attrLength;
		strcpy(atuple.attrName, attributes[i].attrName); // +1 pour '\0'
		atuple.attrType = attributes[i].attrType;
		atuple.indexNo = -1;
		atuple.offset = offset;
		strcpy(atuple.relName, lowerRelName); // +1 pour '\0'

		// Add a tuple in attrcat for each attribute of the relation.
		RID rid; // vanish
		if ((rc = attrcat.InsertRec((char *) &atuple, rid))) { //TODO check
			return rc;
		}

		offset += a.attrLength;

		cout << "   attributes[" << i << "].attrName=" << attributes[i].attrName
				<< "   attrType="
				<< (attributes[i].attrType == INT ? "INT" :
					attributes[i].attrType == FLOAT ? "FLOAT" : "STRING")
				<< "   attrLength=" << attributes[i].attrLength << "\n";
	}

	// We've got the total width of a record so it should't be called
	// offset anymore.
	const int totalSize = offset;

	// Add a tuple in relcat for the relation.
	DataRelInfo rtuple;
	rtuple.attrCount = attrCount;
	rtuple.indexCount = 0; // we choose later which attributes we index but
	// remember to keep that counter up to date.
	strcpy(rtuple.relName, lowerRelName);
	rtuple.tupleLength = totalSize;

	RID rid; // vanish
	if ((rc = relcat.InsertRec((char *) &rtuple, rid))) {
		return rc;
	}

	if ((rc = rmm->CreateFile(lowerRelName, totalSize))) {
		return rc;
	}

	/*
	 * The catalogs are loaded when the database is opened and closed only
	 * when the database is closed. Then, updates to the catalogs are not
	 * reflected onto disk immediately and this can cause weird interface
	 * behaviour. One solution to this problem is to force pages each time
	 * a catalog is changed.
	 */
	if ((rc = attrcat.ForcePages()) || (rc = relcat.ForcePages())) {
		return rc;
	}

	// reduces memory print
	free(lowerRelName);

	return 0;
}

/*
 * The drop table command destroys relation relName, along with all indexes on
 * the relation.
 *
 * This method should destroy the relation named *relName and all indexes on
 * that relation. The indexes are found by accessing catalog attrcat, and the
 * index files are destroyed by calling method IX_Manager::DestroyIndex. The
 * file for the relation itself is destroyed by calling method
 * RM_Manager::DestroyFile. Information about the destroyed relation should
 * be deleted from catalogs relcat and attrcat.
 *
 * Three parts:
 * #1 Format input and check whether the relation actually exists
 * #2 Erasing data from relcat, attrcat. Deleting index and relation files.
 * #3 Clean and refresh
 */
RC SM_Manager::DropTable(const char *relName) {

	RC rc = 0;

	// format input
	char *lrelName = (char*) malloc(MAXNAME + 1);
	strcpy(lrelName, relName);

	RM_FileScan fs;
	RM_Record rec;
	RID rid;

	// Relation table

	if ((rc = fs.OpenScan(relcat, // we look for the given relation
			STRING, // looking for its name
			MAXNAME + 1, // the former mayn't be wider than this
			0, // null offset because we search on the column relname
			EQ_OP, // we look for *this* relation precisely
			lrelName // name of the relation
			))) {
		return RC(-1);
	}

	if (!(rc = fs.GetNextRec(rec))) {
		return RC(-1);
	}

	if ((rc = rec.GetRid(rid))) {
		return RC(-1);
	}

	if ((rc = relcat.DeleteRec(rid))) {
		return RC(-1);
	}

	// Attribute table and index

	if ((rc = fs.OpenScan(attrcat, // we look for the given relation
			STRING, // looking for its name
			MAXNAME + 1, // the former mayn't be wider than this
			0, // null offset because we search on the column relname
			EQ_OP, // we look for *this* relation precisely
			lrelName // name of the relation
			))) {
		return RC(-1);
	}

	while ((rc = fs.GetNextRec(rec))) {
		DataAttrInfo atuple;
		rec.GetData((char *&) atuple);
		if (atuple.indexNo == -1) { // TODO Define NO_INDEX
			if ((rc = DropIndex(relName, atuple.attrName))) {
				return RC(-1);
			}
		}

		if ((rc = rec.GetRid(rid))) {
			return RC(-1);
		}

		if ((rc = attrcat.DeleteRec(rid))) {
			return RC(-1);
		}
	}

	// This file is unknown by now and everything about its enclosed data
	// has been erased so we can destroy it.
	rmm->DestroyFile(lrelName);

	// reduces memory print
	free(lrelName);

	// Finally prints succes
	cout << "DropTable\n   relName=" << lrelName << "\n";
	return rc;
}

/*
 * Three parts:
 * #1 First we check arguments and preconditions. Only one index may be created
 * for each attribute of a relation.
 * #2 Then we update attrcat to reflect the new index, we create it and we
 * build it:
 * 	2.1 opening the index;
 * 	2.2 using RM component methods to scan through the records to be indexed;
 *  2.3 closing the index.
 * #3 Finally we clean all and reduce the memory print.
 */
RC SM_Manager::CreateIndex(const char *relName, const char *attrName) {
	RC rc = 0;

	// Format input
	char *lrelName = (char*) malloc(MAXNAME + 1);
	strcpy(lrelName, relName);
	char *lattrName = (char*) malloc(MAXNAME + 1);
	strcpy(lattrName, attrName);

	if ((rc = FormatName((char *) lrelName))
			|| (rc = FormatName((char *) lattrName))) {
		return rc;
	}

	RM_FileScan fs;
	RM_Record rec; // This field is about to get overwritten a couple of time.
	RM_Record rrecord; // Record for rtuple.
	DataRelInfo rtuple;
	// → Does such a relation exist?
	if ((rc = fs.OpenScan(relcat, // we look for the given relation
			STRING, // looking for its name
			MAXNAME + 1, // the former mayn't be wider than this
			0, // null offset because we search on the column relname
			EQ_OP, // we look for *this* relation precisely
			lrelName // name of the relation
			)) || (rc = fs.GetNextRec(rrecord)) // Should be exactly one.
			) {
		return rc;
	}
	if ((rc = rrecord.GetData((char *&) rtuple))) {
		return rc;
	}

	// → Has this attribute been indexed already? It has to be none (-1) or we
	// issue an non-zero return code.
	if ((rc = fs.OpenScan(attrcat, // we look for the given relation
			STRING, // looking for its name
			MAXNAME + 1, // the former mayn't be wider than this
			0, // null offset because we search on the column relname
			EQ_OP, // we look for *this* relation precisely
			lrelName // name of the relation
			))) {
		return rc;
	}

	// Does it have an index already?
	RM_Record arecord; // Record for atuple.
	DataAttrInfo atuple; // Its value is found then written in.
	char * pAtuple;
	while ((rc = fs.GetNextRec(arecord)) != RM_EOF) {
		//Gets the data for the tuple
		if ((rc = arecord.GetData(pAtuple))) {
			return rc;
		}
		//Copies it into atuple
		memcpy(&atuple, pAtuple, sizeof(DataAttrInfo));
		if (strcmp(atuple.relName, lrelName) == 0) {
			// We've found the entry for given relation and attribute.
			// Does it have an index already?
			if (atuple.indexNo != -1) {
				return RC(-1); // Has an index already.
			}
			break; // Exit the loop
		}
	}

	// Index creation
	if ((rc = ixm->CreateIndex(atuple.relName, rtuple.indexCount,
			atuple.attrType, atuple.attrLength))) {
		return rc;
	}

	// Update index data.
	atuple.indexNo = rtuple.indexCount++;
	if ((rc = relcat.UpdateRec(rrecord)) || (rc = attrcat.UpdateRec(arecord))
			|| (rc = relcat.ForcePages() || (rc = attrcat.ForcePages()))) {
		return RC(-1);
	}

	/* Opens the index so we'll close it later.*/
	IX_IndexHandle ih;
	/* Opens the relation file which is about to get indexed */
	RM_FileHandle fh;
	/* Instanciate a scanner to yield tuples */
	if ((rc = ixm->OpenIndex(lrelName, atuple.indexNo, ih))
			|| (rc = rmm->OpenFile(lrelName, fh))
			|| (rc = fs.OpenScan(fh, atuple.attrType, atuple.attrLength,
					atuple.offset, NO_OP, NULL))) {
		return rc;
	}

	// Walk throughout the records and insert each of them into the index.
	while ((rc = fs.GetNextRec(rec)) != RM_EOF) {
		char *pData;
		RID rid;
		// Retrieve the record.
		// Insert it into the index.
		if ((rc = rec.GetData(pData) || (rc = rec.GetRid(rid)))
				|| (rc = ih.InsertEntry(pData + atuple.offset, rid))) {
			return rc;
		}
	}

	cout << "CreateIndex\n" << "   relName =" << relName << "\n"
			<< "   attrName=" << attrName << "\n";

	if ((rc = fs.CloseScan()) || (rc = rmm->CloseFile(fh))
			|| (rc = ixm->CloseIndex(ih))) {
		return rc;
	}

	//Desallocates
	free(lrelName);
	free(lattrName);

	return 0;
}

RC SM_Manager::DropIndex(const char *relName, const char *attrName) {
	cout << "DropIndex\n" << "   relName =" << relName << "\n" << "   attrName="
			<< attrName << "\n";
	return (0);
}

RC SM_Manager::Load(const char *relName, const char *fileName) {

	RC rc;
	RM_Record rec;
	RM_FileScan filescan;
	bool flag_exist = false;

	//store the attrlength of each attribute in DataRelInfo relName
	int i = 0;
	int tuplelength = 0;
	int attr_count = 0;
	//open scan of relcat
	if ((rc = filescan.OpenScan(this->relcat, INT, sizeof(int), 0, NO_OP, NULL)))
		return (rc);

	//scan all the records in relcat
	char * data;
	while (rc != RM_EOF) {
		//get records until the end
		rc = filescan.GetNextRec(rec);
		if (rc != 0 && rc != RM_EOF)
			return (rc);
		if (rc != RM_EOF) {
			if ((rc = rec.GetData(data)))
				return (rc);
			if (!strcmp(((DataRelInfo*) data)->relName, relName)) {
				flag_exist = true;
				//get tuplelength of relation
				tuplelength = ((DataRelInfo*) data)->tupleLength;
				attr_count = ((DataRelInfo*) data)->attrCount;
			}
		}
	}

	if (!flag_exist)
		return (SM_INVALIDRELNAME);

	//close the scan
	if ((rc = filescan.CloseScan()))
		return (rc);

	DataAttrInfo d_a[MAXATTRS];
	//open scan of attrcat
	if ((rc = filescan.OpenScan(this->relcat, INT, sizeof(int), 0, NO_OP, NULL)))
		return (rc);

	//scan all the records in attrcat
	while (rc != RM_EOF) {
		//get records until the end
		rc = filescan.GetNextRec(rec);
		if (rc != 0 && rc != RM_EOF)
			return (rc);
		if (rc != RM_EOF) {
			if ((rc = rec.GetData(data)))
				return (rc);
			if (!strcmp(((DataAttrInfo*) data)->relName, relName)) {

				//memcpy(&d_a[i],data,sizeof(DataAttrInfo));
				strcpy(d_a[i].relName, ((DataAttrInfo*) data)->relName);
				strcpy(d_a[i].attrName, ((DataAttrInfo*) data)->attrName);
				d_a[i].attrType = ((DataAttrInfo*) data)->attrType;
				d_a[i].attrLength = ((DataAttrInfo*) data)->attrLength;
				d_a[i].offset = ((DataAttrInfo*) data)->offset;
				d_a[i].indexNo = ((DataAttrInfo*) data)->indexNo;

				i++;
			}
		}
	}

	//close the scan
	if ((rc = filescan.CloseScan()))
		return (rc);

	// open data file named fileName
	ifstream myfile(fileName);

	RM_FileHandle filehandle_r;

	// open relation file to store records read from file
	if ((rc = rmm->OpenFile(relName, filehandle_r)))
		return (rc);

	//load the file
	while (!myfile.eof()) {
		i = 0;
		string str;
		//read file line by line
		getline(myfile, str);
		if (str == "")
			break;
		string delimiter = ",";

		size_t pos = 0;
		char *record_data;
		record_data = new char[((DataRelInfo*) data)->tupleLength];
		//seperate string by commas;
		while ((pos = str.find(delimiter)) != string::npos) {
			const char * token;
			token = str.substr(0, pos).c_str();
			str.erase(0, pos + delimiter.length());
			switch (d_a[i].attrType) {

			case INT: {
				int data_int = atoi(token);
				*((int*) (record_data + d_a[i].offset)) = data_int;
				break;
			}
			case FLOAT: {
				float data_float = atof(token);
				*((float*) (record_data + d_a[i].offset)) = data_float;
				break;
			}

			case STRING: {

				char *data_char;
				data_char = (char *) malloc(
						(d_a[i].attrLength + 1) * sizeof(char));
				memcpy(data_char, token, d_a[i].attrLength);
				memcpy(record_data + d_a[i].offset, data_char,
						d_a[i].attrLength);
				break;
			}
			default:
				// Test: wrong _attrType
				return (SM_INVALIDATTRNAME);
			}
			i++;
		}
		const char * token;
		token = str.c_str();
		switch (d_a[i].attrType) {

		case INT: {
			int data_int = atoi(token);
			*((int*) (record_data + d_a[i].offset)) = data_int;
			break;
		}
		case FLOAT: {
			float data_float = atof(token);
			*((float*) (record_data + d_a[i].offset)) = data_float;
			break;
		}

		case STRING: {
			char *data_char;
			data_char = (char *) malloc((d_a[i].attrLength + 1) * sizeof(char));
			memcpy(data_char, token, d_a[i].attrLength);
			memcpy(record_data + d_a[i].offset, token, d_a[i].attrLength);
			break;
		}
		default:
			//wrong _attrType
			return (SM_INVALIDATTRNAME);
		}

		RID rid;
		//store the record to relation file
		if ((rc = filehandle_r.InsertRec(record_data, rid)))
			return (rc);

		//insert index if index file exists
		for (int k = 0; k < attr_count; k++) {
			if (d_a[k].indexNo != -1) {
				// Call IX_IndexHandle::OpenIndex to open the index file
				IX_IndexHandle indexhandle;
				if ((rc = ixm->OpenIndex(relName, d_a[k].indexNo, indexhandle)))
					return (rc);
				//char data_index[d_a[k].attrLength];
				//memcpy(data_index,record_data+d_a[k].offset,d_a[k].attrLength);
				if ((rc = indexhandle.InsertEntry(data + d_a[k].offset, rid)))
					return (rc);
				//close index file
				if (ixm->CloseIndex(indexhandle))
					return (rc);

			}
		}
		// cout<<"record_data"<<record_data<<endl;
		strcpy(record_data, "");
	}

	//close the data file
	myfile.close();

	//close the relation file
	if ((rc = rmm->CloseFile(filehandle_r)))
		return (rc);

	cout << "Load\n" << "   relName =" << relName << "\n" << "   fileName="
			<< fileName << "\n";
	return (0);
}

RC SM_Manager::Print(const char *relName) {
	/*
	 * Il faut imprimer toute la relation nommée relName, et compter le nombre de tuples
	 */
	RC rc = 0;
	RM_FileHandle rmfh;
	RM_FileScan rmfs;
	RM_Record rec;
	char * _pData;

	/* On met le nom de la relation en petits caractères*/
	char *lowerRelName = (char*) malloc(MAXNAME + 1);
	strcpy(lowerRelName, relName);
	FormatName((char *) lowerRelName);

	rc = this->rmm->OpenFile(lowerRelName, rmfh);
	if (rc)
		return rc;

	/*
	 * Si le fichier correspondant à la relation demandée existe bien,
	 * on imprime TOUT !
	 * On commence par le scanner, puis on printera tous ses records!
	 */

	//Finds the number of of attributes using a scan
	int attrCount;
	RM_FileScan relcatFs;
	RM_Record relRec;

	if ((rc = relcatFs.OpenScan(relcat, STRING, MAXNAME + 1, 0, EQ_OP,
			(void *) relName))) {
		return rc;
	}
	if ((rc = relcatFs.GetNextRec(relRec))) {
		return rc;
	}
	char* pDataRelRec;
	if ((rc = relRec.GetData(pDataRelRec))) {
		return rc;
	}
	//Second param in emcpy is the offset of attrCount in the relcat table
	memcpy(&attrCount, pDataRelRec + MAXNAME + 1 + sizeof(int), sizeof(int));
	if ((rc = relcatFs.CloseScan())) {
		return rc;
	}
	//Fills an array with the attributes of our table
	DataAttrInfo attributes[attrCount];
	RM_FileScan attrcatFs;
	RM_Record attrcatRec;
	if ((rc = attrcatFs.OpenScan(attrcat, STRING, MAXNAME + 1, 0, EQ_OP,
			(void *) relName))) {
		return rc;
	}
	int i = 0; //i is used to check that the nb of Rec retrieved is actually attrCount
	while (attrcatFs.GetNextRec(attrcatRec) != RM_EOF) {
		char* pDataAttrRec;
		if ((rc = attrcatRec.GetData(pDataAttrRec))) {
			return rc;
		}
		//Fills the array
		memcpy(&attributes[i], pDataAttrRec, sizeof(DataAttrInfo));
		i++;
	}
	if ((rc = attrcatFs.CloseScan())) {
		return rc;
	}
	if (i != attrCount) {
		return -1;
	}
	//Creates the printer object
	Printer printer(attributes, attrCount);

	//Now for each tuple of the relation we can print it with the printer object (scan with the first attr for instance)
	rc = rmfs.OpenScan(rmfh, attributes[0].attrType, attributes[0].attrLength,
			attributes[0].offset, NO_OP, NULL);
	if (rc)
		return rc;

	while ((rc = rmfs.GetNextRec(rec)) != RM_EOF) {

		rc = rec.GetData(_pData);
		if (rc)
			return rc;

		printer.Print(cout, _pData);
	}
	//Closes scan & file
	if ((rc = rmfs.CloseScan()) || (rc = rmm->CloseFile(rmfh))) {
		return rc;
	}

	cout << "Print\n" << "   relName=" << relName << "\n";
	return (0);
}

RC SM_Manager::Set(const char *paramName, const char *value) {
	cout << "Set\n" << "   paramName=" << paramName << "\n" << "   value    ="
			<< value << "\n";
	return (0);
}

RC SM_Manager::Help() {
	/* Je me suis inspirée du code donné pour DBCreate,
	 * histoire de comprendre la structure des fichiers
	 * relcat et attrcat. J'ai fait un schéma sur mso ppt.
	 */

	/* On définit les variables */
	RC rc = 0;
	RM_FileScan rmfs;
	RM_Record rec;
	RID rid;
	char * _pData;

	/*
	 * Pour définir le rid dont on a besoin pour la méthode getRecord,
	 * on ouvre un scan avec le RM_FileScan. Ensuite, on trouve le prochain record
	 * qui correspond et on récupère son RID.
	 */

	rc = rmfs.RM_FileScan::OpenScan(relcat, STRING, MAXNAME + 1, 0, NO_OP,
	NULL);
	if (rc)
		return rc;

	//Creates Printer object for the relname attribute only
	DataAttrInfo attributes[1];
	attributes[0].attrLength = MAXNAME + 1;
	strcpy(attributes[0].attrName, "relname"); //attributes[0].attrName = "relname";
	attributes[0].attrType = STRING;
	attributes[0].indexNo = -1;
	attributes[0].offset = 0;
	strcpy(attributes[0].relName, "relcat"); //attributes[0].relName = "relcat";
	Printer printer = Printer(attributes, 1);

	while ((rc = rmfs.GetNextRec(rec)) != RM_EOF) {

		/* 
		 * On parcourt les lignes de relcat!
		 * Je me suis inspirée du code de la méthode CreateIndex()
		 */

		if ((rc = rec.GetData(_pData) || (rc = rec.GetRid(rid)))) {
			return rc;
		}
		//We just print the beginning of the tuple i.e. relname
		printer.Print(cout, _pData);
	}

	if ((rc = rmfs.CloseScan())) {
		return rc;
	}

	return 0;
}

RC SM_Manager::Help(const char *relName) {
	/* On définit les variables */
	RC rc = 0;
	RM_FileScan rmfs;
	RM_Record rec;
	RID rid;
	char * _pData;

	/*
	 *Creates the printer with the six DatAttrInfo for attributes of ATTRCAT table
	 */
	DataAttrInfo attributes[6]; //There are exactly 6 attributes in attrCat table

	RM_FileScan attrFs;
	RM_Record attrRec;
	if ((rc = attrFs.OpenScan(attrcat, STRING, MAXNAME + 1, 0, EQ_OP,
			(void*) "attrcat"))) {
		return rc;
	}
	int i = 0;
	while (attrFs.GetNextRec(attrRec) != RM_EOF) {
		char* pDataAttrRec;
		if ((rc = attrRec.GetData(pDataAttrRec))) {
			return rc;
		}
		memcpy(&attributes[i], pDataAttrRec, sizeof(DataAttrInfo));
		i++;
	}
	if (i != 6) {
		return -1;
	}
	if ((rc = attrFs.CloseScan())) {
		return rc;
	}
	Printer printer = Printer(attributes, 6);

	/*
	 *Scans ATTRCAT table once angain, to find the attributes of relName table and print them
	 */
	if ((rc = rmfs.OpenScan(attrcat, STRING, MAXNAME + 1, 0, EQ_OP,
			(void *) relName))) {
		return rc;
	}

	while ((rc = rmfs.GetNextRec(rec))) {

		if ((rc = rec.GetData(_pData))) {
			return rc;
		}
		//Prints
//		if ((rc = printer.Print(cout, _pData))) {
//			return rc;
//		}
		printer.Print(cout, _pData); // returns null, how could we get rc?
	}

	if ((rc = rmfs.CloseScan())) {
		return rc;
	}

	return (0);
}

void SM_PrintError(RC rc) {
	cout << "SM_PrintError\n   rc=" << rc << "\n";
}

