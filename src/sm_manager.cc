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
    AttributeTuple atuple; // atuple is overwritten each iteration.

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
        if ((rc = attrcat.InsertRec( (char * ) &atuple, rid))) { //TODO check
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
	RelationTuple rtuple;
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

    //Desallocates lowerRelName
	free(lowerRelName);

    return 0;
}

RC SM_Manager::DropTable(const char *relName) {
	cout << "DropTable\n   relName=" << relName << "\n";
	return (0);
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
		return RC(-1);
	}

	RM_FileScan fs;
	RM_Record rec; // This field is about to get overwritten a couple of time.
	RM_Record rrecord; // Record for rtuple.
	RelationTuple rtuple;
	// → Does such a relation exist?
	if ((rc = fs.OpenScan(relcat, // we look for the given relation
			STRING, // looking for its name
			MAXNAME + 1, // the former mayn't be wider than this
			0, // null offset because we search on the column relname
			EQ_OP, // we look for *this* relation precisely
			lrelName // name of the relation
			)) || (rc = fs.GetNextRec(rrecord)) // Should be exactly one.
			) {
		return RC(-1);
	}
	if ((rc = rec.GetData((char *&) rtuple))) {
		return RC(-1);
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
		return RC(-1);
	}

	// Does it have an index already?
	RM_Record arecord; // Record for atuple.
    DataAttrInfo atuple; // Its value is found then written in.
    char * pAtuple;
	while ((rc = fs.GetNextRec(arecord))) {
        if ((rc = arecord.GetData(pAtuple))) {
			return RC(-1);
		}
        memcpy(&atuple, pAtuple, sizeof(DataAttrInfo));
        if (strcmp(atuple.relName, lrelName)) {
			// We've found the entry for given relation and attribute.
			// Does it have an index already?
			if (atuple.indexNo != -1) {
				return RC(-1); // Has an index already.
			} else {
				break; // Exit the loop
			}
		}
	}

	// Index creation
	if ((rc = ixm->CreateIndex(atuple.relName, rtuple.indexCount,
			atuple.attrType, atuple.attrLength))) {
		return RC(-1);
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
		return RC(-1);
	}

	// Walk throughout the records and insert each of them into the index.
	while ((rc = fs.GetNextRec(rec))) {
		char *pData;
		RID rid;
		// Retrieve the record.
		// Insert it into the index.
		if ((rc = rec.GetData(pData) || (rc = rec.GetRid(rid)))
				|| (rc = ih.InsertEntry(pData + atuple.offset, rid))) {
			return RC(-1);
		}
	}

	cout << "CreateIndex\n" << "   relName =" << relName << "\n"
			<< "   attrName=" << attrName << "\n";

	if ((rc = fs.CloseScan()) || (rc = rmm->CloseFile(fh))
			|| (rc = ixm->CloseIndex(ih))) {
		return RC(-1);
	}

	// TODO Any free() to do here?

	return rc;
}

RC SM_Manager::DropIndex(const char *relName, const char *attrName) {
	cout << "DropIndex\n" << "   relName =" << relName << "\n" << "   attrName="
			<< attrName << "\n";
	return (0);
}

RC SM_Manager::Load(const char *relName,
                    const char *fileName)
{
    
    RC rc;
    RM_Record rec;
    RM_FileScan filescan;
    bool flag_exist = false;

    //store the attrlength of each attribute in RelationTuple relName
    int i=0;
    int tuplelength = 0;
    int attr_count = 0;
    //open scan of relcat
    if((rc=filescan.OpenScan(this->relcat, INT, sizeof(int), 0, NO_OP , NULL))) return (rc);
    
    //scan all the records in relcat
    char * data;
    while(rc != RM_EOF){
        //get records until the end
        rc = filescan.GetNextRec(rec);
        if(rc != 0 && rc!= RM_EOF) return (rc);
        if(rc != RM_EOF){
            if((rc = rec.GetData(data))) return (rc);
            if(!strcmp(((RelationTuple*)data)->relName,relName)) {
                flag_exist = true;
                //get tuplelength of relation
                tuplelength = ((RelationTuple*)data)->tupleLength;
                attr_count = ((RelationTuple*)data)->attrCount;
            }
        }
    }
    
    if(!flag_exist) return (SM_INVALIDRELNAME);
    
    //close the scan
    if((rc = filescan.CloseScan())) return (rc);
    
    DataAttrInfo d_a[MAXATTRS];
    //open scan of attrcat
    if((rc = filescan.OpenScan(this->relcat,INT, sizeof(int), 0, NO_OP , NULL))) return (rc);
    
    //scan all the records in attrcat
    while(rc != RM_EOF){
        //get records until the end
        rc = filescan.GetNextRec(rec);
        if(rc != 0 && rc != RM_EOF) return (rc);
        if(rc != RM_EOF){
            if((rc = rec.GetData(data))) return (rc);
            if(!strcmp(((DataAttrInfo*)data)->relName,relName)) {
                
                //memcpy(&d_a[i],data,sizeof(DataAttrInfo));
                strcpy(d_a[i].relName,((DataAttrInfo*)data)->relName);
                strcpy(d_a[i].attrName,((DataAttrInfo*)data)->attrName);
                d_a[i].attrType = ((DataAttrInfo*)data)->attrType;
                d_a[i].attrLength = ((DataAttrInfo*)data)->attrLength;
                d_a[i].offset = ((DataAttrInfo*)data)->offset;
                d_a[i].indexNo = ((DataAttrInfo*)data)->indexNo;

                i++;
            }
        }
    }
    
    //close the scan
    if((rc = filescan.CloseScan())) return (rc);
    
    
    // open data file named fileName
    ifstream myfile (fileName);
    
    RM_FileHandle filehandle_r;
    
    // open relation file to store records read from file
    if((rc = rmm->OpenFile(relName, filehandle_r))) return (rc);
    
    
    //load the file
    while(!myfile.eof()){
        i = 0;
        string str;
        //read file line by line
        getline(myfile, str);
        if(str == "") break;
        string delimiter = ",";
        
        size_t pos = 0;
		char *record_data;
		record_data = new char[((RelationTuple*)data)->tupleLength];
        //seperate string by commas;
        while ((pos = str.find(delimiter)) != string::npos) {
            const char * token;
            token = str.substr(0, pos).c_str();
            str.erase(0, pos + delimiter.length());
            switch (d_a[i].attrType) {
                    
                case INT:{
                    int data_int = atoi(token);
                    *((int*)(record_data + d_a[i].offset)) = data_int;
                    break;
                }
                case FLOAT:{
                    float data_float = atof(token);
                    *((float*)(record_data+d_a[i].offset)) = data_float;
                    break;
                }
                    
                case STRING:{

                    char *data_char;
                    data_char = (char *)malloc((d_a[i].attrLength+1)*sizeof(char));
                    memcpy(data_char,token,d_a[i].attrLength);
                    memcpy(record_data+d_a[i].offset,data_char,d_a[i].attrLength);
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
                
            case INT:{
                int data_int = atoi(token);
                *((int*)(record_data+d_a[i].offset)) = data_int;
                break;
            }
            case FLOAT:{
                float data_float = atof(token);
                *((float*)(record_data+d_a[i].offset)) = data_float;
                break;
            }
                
            case STRING:{
                char *data_char;
                data_char = (char *)malloc((d_a[i].attrLength+1)*sizeof(char));
                memcpy(data_char,token,d_a[i].attrLength);
                memcpy(record_data+d_a[i].offset,token,d_a[i].attrLength);
                break;
            }
            default:
                //wrong _attrType
                return (SM_INVALIDATTRNAME);
        }

        RID rid;
        //store the record to relation file
        if((rc = filehandle_r.InsertRec(record_data, rid))) return (rc);
        
        //insert index if index file exists
        for (int k = 0;k<attr_count;k++) {
            if(d_a[k].indexNo != -1){
                // Call IX_IndexHandle::OpenIndex to open the index file
                IX_IndexHandle indexhandle;
                if((rc = ixm->OpenIndex(relName, d_a[k].indexNo, indexhandle))) return (rc);
                //char data_index[d_a[k].attrLength];
                //memcpy(data_index,record_data+d_a[k].offset,d_a[k].attrLength);
                if((rc = indexhandle.InsertEntry(data+d_a[k].offset, rid))) return (rc);
                //close index file
                if(ixm->CloseIndex(indexhandle)) return (rc);

            }
        }
       // cout<<"record_data"<<record_data<<endl;
        strcpy(record_data,"");
    }

    
    //close the data file
    myfile.close();
    
    //close the relation file
    if((rc=rmm->CloseFile(filehandle_r))) return (rc);
    
    cout << "Load\n"
    << "   relName =" << relName << "\n"
    << "   fileName=" << fileName << "\n";
    return (0);
}

RC SM_Manager::Print(const char *relName) {
	/*
	 * Il faut imprimer toute la relation nommée relName, et compter le nombre de tuples
	 */
	RC rc = 0;
	RM_FileHandle *rmfh;
	RM_FileScan rmfs;
	RM_Record rec;
	char * _pData;

	/* On met le nom de la relation en petits caractères*/
	char *lowerRelName = (char*)malloc(MAXNAME + 1);
	strcpy(lowerRelName, relName);
	FormatName((char *)lowerRelName);

	rc = this->rmm->OpenFile(lowerRelName, rmfh);
	if (rc) return rc;

		/* 
		 * Si le fichier correspondant à la relation demandée existe bien,
		 * on imprime TOUT !
		 * On commence par le scanner, puis on printera tous ses records!
		 */
	rc = rmfs.RM_FileScan::OpenScan(rmfh, STRING, 1 + MAXSTRINGLEN, 0, NO_OP, NULL);
	if (rc) return rc;

	while ((rc = rmfs.GetNextRec(rec))) {

		rc = rec.GetData(_pData);
		if (rc) return rc;

		Print(rmfh, _pData);
	}

	if ((rc = rmfs.CloseScan()) || (rc = rmm->CloseFile(fh))) {
		return RC(-1);
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
	RM_FileHandle rmfh;
	RM_FileScan rmfs;
	RM_Record rec;
	RID rid;
    char * _pData;

	/* On ouvre le fichier qui contient la table  relcat*/
    rc = this->rmm->OpenFile("relcat", rmfh);
    if (rc) return rc;

	/*
	* Pour définir le rid dont on a besoin pour la méthode getRecord,
	* on ouvre un scan avec le RM_FileScan. Ensuite, on trouve le prochain record
	* qui correspond et on récupère son RID.
	*/
	
	rc = rmfs.RM_FileScan::OpenScan(rmfh, STRING, 1 + MAXSTRINGLEN, 0, NO_OP, NULL);
	if (rc) return rc;

	/*rc = rmfs.GetNextRec(rec);
	if (rc) return rc;
	rc = rec.GetRid(rid);
	if (rc) return rc;
	rc = rmfh.RM_FileHandle::GetRec(rid, rec);
	if (rc) return rc;
	rc = rec.GetData(_pData);
	if (rc) return rc;

	DataAttrInfo relNameAttr;
	relNameAttr.relName = _pData;
	relNameAttr.attrName = "relname";
	relNameAttr.offset = 0;
	relNameAttr.attrType = STRING;
	relNameAttr.attrLength = MAXNAME + 1;
	relNameAttr.indexNo = -1;
	*/
	while ((rc = rmfs.GetNextRec(rec))) {
		
		/* 
		 * On parcourt les lignes de relcat!
		 * Je me suis inspirée du code de la méthode CreateIndex()
		 */

		if ((rc = rec.GetData(_pData) || (rc = rec.GetRid(rid)))) {
			return RC(-1);
		}
		/*
		 * On a maintenant le contenu entier du record exploré dans pData.
		 * Il faudrait donc le couper pour en sortir le nom de la relation
		 * qui est la donnée interessante pour cette méthode. Il suffit d'utiliser la structure
		 * DataAttrInfo
		 */

		Printer((DataAttrInfo)_pData, 1);
	}

	if ((rc = rmfs.CloseScan()) || (rc = rmm->CloseFile(fh))) {
		return RC(-1);
}

	return 0;
}

}

RC SM_Manager::Help(const char *relName) {
	/* On définit les variables */
	RC rc = 0;
	RM_FileHandle rmfh;
	RM_FileScan rmfs;
	RM_Record rec;
	RID rid;
	char * _pData;

	/* On ouvre le fichier qui contient la table  attrcat*/
	rc = this->rmm->OpenFile("attrcat", rmfh);
	if (rc) return rc;
	
	if ((rc = fs.OpenScan(rmfh, STRING, MAXNAME + 1, 0, EQ_OP, relName)) || (rc = fs.GetNextRec(rec))) {
		return RC(-1);
	}

	while ((rc = rmfs.GetNextRec(rec))) {

		if ((rc = rec.GetData(_pData) || (rc = rec.GetRid(rid)))) {
			return RC(-1);
		}
		
		/*
		 * On suppose que les attributs sont comptés à partir de 0
		 */
		Printer((DataAttrInfo)_pData, 0);
		Printer((DataAttrInfo)_pData, 1);
		Printer((DataAttrInfo)_pData, 2);
		Printer((DataAttrInfo)_pData, 3);
	}
	
	if ((rc = rmfs.CloseScan()) || (rc = rmm->CloseFile(fh))) {
		return RC(-1);
	}


	return (0);
}

void SM_PrintError(RC rc) {
	cout << "SM_PrintError\n   rc=" << rc << "\n";
}

