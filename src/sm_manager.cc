//
// File:        sm_manager.cc
// Description: SM_Manager class implementation
// Author:      Hyunjung Park (hyunjung@stanford.edu)
//

#include "sm_internal.h"
#include <stdio.h>
using namespace std;

// 
// SM_Manager
//
// Desc: Constructor
//
SM_Manager::SM_Manager(IX_Manager &ixm, RM_Manager &rmm)
{
   // Set the associated {IX|RM}_Manager object
   pIxm = &ixm;
   pRmm = &rmm;
   
   //
   useIndexNo = -1;
}

//
// ~SM_Manager
// 
// Desc: Destructor
//
SM_Manager::~SM_Manager()
{
   // Clear the associated {IX|RM}_Manager object
   pIxm = NULL;
   pRmm = NULL;
}

//
// OpenDb
//
// Desc: Open a DB
// In:   dbName - name of DB to open
// Ret:  SM_INVALIDDBNAME, SM_CHDIRFAILED, RM return code
//
RC SM_Manager::OpenDb(const char *dbName)
{
   RC rc;

   // Sanity Check: Length of the argument should be less than MAXDBNAME
   //               DBname cannot contain ' ' or '/' 
   if (strlen(dbName) > MAXDBNAME
       || strchr(dbName, ' ') || strchr(dbName, '/')) {
      rc = SM_INVALIDDBNAME;
      goto err_return;
   }
   
   // Change the working directory
   if (chdir(dbName) < 0) {
      rc = SM_CHDIRFAILED;
      goto err_return;
   }

   // Open a file scan for RELCAT
   if (rc = pRmm->OpenFile(RELCAT, fhRelcat))
      goto err_return;

   // Open a file scan for ATTRCAT
   if (rc = pRmm->OpenFile(ATTRCAT, fhAttrcat))
      goto err_close;

   // Return ok
   return (0);

   // Return error
err_close:
   pRmm->CloseFile(fhRelcat);
err_return:
   return (rc);
}

//
// CloseDb
//
// Desc: Close a DB 
// Ret:  RM return code
//
RC SM_Manager::CloseDb()
{
   RC rc;

   // Close a file scan for ATTRCAT
   if (rc = pRmm->CloseFile(fhAttrcat))
      goto err_close;

   // Close a file scan for RELCAT
   if (rc = pRmm->CloseFile(fhRelcat))
      goto err_return;

   // Return ok
   return (0);

   // Return error
err_close:
   pRmm->CloseFile(fhRelcat);
err_return:
   return (rc);
}

//
// CreateTable
//
// Desc: Create a table
// In:   relName - 
//       attrCount - 
//       attributes -
// Ret:  SM_INVALIDRELNAME, SM_DUPLICATEDATTR, SM_RELEXISTS, RM return code
//
RC SM_Manager::CreateTable(const char *relName,
                           int attrCount, AttrInfo *attributes)
{
   RC rc;
   RM_Record tmpRec;
   char *relcatData;
   int tupleLength = 0;
   int offset = 0;
   SM_RelcatRec relcatRec;
   SM_AttrcatRec attrcatRec;
   RID rid;

   // Sanity Check: relName should not be RELCAT or ATTRCAT
   if (strcmp(relName, RELCAT) == 0 || strcmp(relName, ATTRCAT) == 0) {
      rc = SM_INVALIDRELNAME;
      goto err_return;
   }

   // Compute tupleLength by summing up attrLength
   // Sanity Check: duplicated attribute names
   for (int i = 0; i < attrCount; i++) {
      tupleLength += attributes[i].attrLength;
      for (int j = i + 1; j < attrCount; j++) {
         if (strcmp(attributes[i].attrName, attributes[j].attrName) == 0) {
            rc = SM_DUPLICATEDATTR;
            goto err_return;
         }
      }
   }

   // Sanity Check: relName should not exist
   if ((rc = GetRelationInfo(relName, tmpRec, relcatData)) != SM_RELNOTFOUND) {
      rc = (rc == 0) ? SM_RELEXISTS : rc;
      goto err_return;
   }

   // Update RELCAT
   SM_SetRelcatRec(relcatRec, relName, tupleLength, attrCount, 0);
   if (rc = fhRelcat.InsertRec((char *)&relcatRec, rid))
      goto err_return;
   if (rc = fhRelcat.ForcePages())
      goto err_return;

   // Update ATTRCAT
   for (int i = 0; i < attrCount; i++) {
      SM_SetAttrcatRec(attrcatRec, 
                       relName, attributes[i].attrName, offset,
                       attributes[i].attrType, attributes[i].attrLength, -1);
      offset += attributes[i].attrLength;
      if (rc = fhAttrcat.InsertRec((char *)&attrcatRec, rid))
         goto err_return;
   }
   if (rc = fhAttrcat.ForcePages())
      goto err_return;

   // Create file
   if (rc = pRmm->CreateFile(relName, tupleLength))
      goto err_return;

   // Return ok
   return (0);

   // Return error
err_return:
   return (rc);
}

//
// DropTable
//
// Desc: Drop a table
// In:   relName - 
// Ret:  SM_INVALIDRELNAME, SM_RELNOTFOUND, RM return code
//
RC SM_Manager::DropTable(const char *relName)
{
   RC rc;
   RM_Record rec, _rec;
   char *relcatData;
   RID rid;
   char _relName[MAXNAME];
   RM_FileScan fs;
   int i = 0;

   // Sanity Check: relName should not be RELCAT or ATTRCAT
   if (strcmp(relName, RELCAT) == 0 || strcmp(relName, ATTRCAT) == 0) {
      rc = SM_INVALIDRELNAME;
      goto err_return;
   }

   // Update RELCAT
   if (rc = GetRelationInfo(relName, rec, relcatData))
      goto err_return;
   if (rc = rec.GetRid(rid))
      goto err_return;
   if (rc = fhRelcat.DeleteRec(rid))
      goto err_return;
   if (rc = fhRelcat.ForcePages())
      goto err_return;

   // Update ATTRCAT
   memset(_relName, '\0', sizeof(_relName));
   strncpy(_relName, relName, MAXNAME);
   if (rc = fs.OpenScan(fhAttrcat, STRING, MAXNAME,
                        OFFSET(SM_AttrcatRec, relName), EQ_OP, _relName))
      goto err_return;

   while ((rc = fs.GetNextRec(_rec)) != RM_EOF) {
      char *attrcatData;

      if (rc != 0)
         goto err_closescan;

      // Delete the index on this attribute, if any
      if (rc = _rec.GetData(attrcatData))
         goto err_closescan;
      if (((SM_AttrcatRec *)attrcatData)->indexNo != -1)
         pIxm->DestroyIndex(relName,((SM_AttrcatRec *)attrcatData)->indexNo);

      // Delete the record from ATTRCAT
      if (rc = _rec.GetRid(rid))
         goto err_closescan;
      if (rc = fhAttrcat.DeleteRec(rid))
         goto err_closescan;

      if (++i == ((SM_RelcatRec *)relcatData)->attrCount)
         break;
   }

   if (rc = fs.CloseScan())
      goto err_return;
   if (rc = fhAttrcat.ForcePages())
      goto err_return;

   // Destroy file
   if (rc = pRmm->DestroyFile(relName))
      goto err_return;

   // Return ok
   return (0);

   // Return error
err_closescan:
   fs.CloseScan();
err_return:
   return (rc);
}

//
// CreateIndex
//
// Desc: 
// In:   relName - 
//       attrName - 
// Ret:  SM_ATTRNOTFOUND, SM_INDEXEXISTS, IX return code
//
RC SM_Manager::CreateIndex(const char *relName, const char *attrName)
{
   RC rc;
   RM_Record rec;
   char *attrcatData;
   int indexNo = -1;
   RM_FileScan fs;
   RM_FileHandle fh;
   RM_Record dataRec;
   IX_IndexHandle ih;
   
   // Sanity Check: relName/attrName should exist, but its index should not
   if (rc = GetAttributeInfo(relName, attrName, rec, attrcatData))
      goto err_return;
   if (((SM_AttrcatRec *)attrcatData)->indexNo != -1) {
      rc = SM_INDEXEXISTS;
      goto err_return;
   }
   // Determine indexNo
   indexNo = ((SM_AttrcatRec *)attrcatData)->offset;

   // Build index
   if (rc = pIxm->CreateIndex(relName, indexNo, 
                              ((SM_AttrcatRec *)attrcatData)->attrType,
                              ((SM_AttrcatRec *)attrcatData)->attrLength))
      goto err_return;
   if (rc = pIxm->OpenIndex(relName, indexNo, ih))
      goto err_destroyindex;

   if (rc = pRmm->OpenFile(relName, fh))
      goto err_closeindex;
   if (rc = fs.OpenScan(fh, INT, sizeof(int), 0, NO_OP, NULL))
      goto err_closefile;

   while ((rc = fs.GetNextRec(dataRec)) != RM_EOF) {
      char *data;
      RID rid;

      if (rc != 0)
         goto err_closescan;

      if (rc = dataRec.GetData(data))
         goto err_closescan;
      if (rc = dataRec.GetRid(rid))
         goto err_closescan;

      if (rc = ih.InsertEntry(data + ((SM_AttrcatRec *)attrcatData)->offset, rid))
         goto err_closescan;
   }

   if (rc = fs.CloseScan())
      goto err_closefile;
   if (rc = pRmm->CloseFile(fh))
      goto err_closeindex;

   if (rc = pIxm->CloseIndex(ih))
      goto err_destroyindex;

   // Update indexNo
   ((SM_AttrcatRec *)attrcatData)->indexNo = indexNo;
   if (rc = fhAttrcat.UpdateRec(rec))
      goto err_return;
   if (rc = fhAttrcat.ForcePages())
      goto err_return;

   // Update RELCAT
   if (rc = SetRelationIndexCount(relName, +1))
      goto err_return;

   // Return ok
   return (0);

   // Return error
err_closescan:
   fs.CloseScan();
err_closefile:
   pRmm->CloseFile(fh);
err_closeindex:
   pIxm->CloseIndex(ih);
err_destroyindex:
   pIxm->DestroyIndex(relName, indexNo);
err_return:
   return (rc);
}

//
// DropIndex
//
// Desc: 
// In:   relName - 
//       attrName - 
// Ret:  SM_ATTRNOTFOUND, SM_INDEXNOTFOUND, IX return code
//
RC SM_Manager::DropIndex(const char *relName, const char *attrName)
{
   RC rc;
   RM_Record rec;
   char *attrcatData;
   RM_FileScan fs;

   // Sanity Check: relName/attrName and its index should exist
   if (rc = GetAttributeInfo(relName, attrName, rec, attrcatData))
      goto err_return;
   if (((SM_AttrcatRec *)attrcatData)->indexNo == -1) {
      rc = SM_INDEXNOTFOUND;
      goto err_return;
   }

   // Destroy the index file
   if (rc = pIxm->DestroyIndex(relName, ((SM_AttrcatRec *)attrcatData)->indexNo))
      goto err_return;

   // Update indexNo
   ((SM_AttrcatRec *)attrcatData)->indexNo = -1;
   if (rc = fhAttrcat.UpdateRec(rec))
      goto err_return;
   if (rc = fhAttrcat.ForcePages())
      goto err_return;

   // Update RELCAT
   if (rc = SetRelationIndexCount(relName, -1))
      goto err_return;

   // Return ok
   return (0);

   // Return error
err_return:
   return (rc);
}

//
// Load
//
// Desc: 
// In:   relName - 
//       fileName - 
// Ret:  SM_RELNOTFOUND, RM return code
//
RC SM_Manager::Load(const char *relName, const char *fileName)
{
   RC rc;
   RM_Record tmpRec;
   char *relcatData;
   char _relName[MAXNAME];
   SM_AttrcatRec *attributes;
   IX_IndexHandle *ihs;
   RM_FileHandle fh;
   RM_FileScan fs;
   RM_Record rec;
   char *data;
   FILE *fp;
   char *buf;
   int i = 0;

   // Sanity Check: relName should not be RELCAT or ATTRCAT
   if (strcmp(relName, RELCAT) == 0 || strcmp(relName, ATTRCAT) == 0) {
      rc = SM_INVALIDRELNAME;
      goto err_return;
   }

   // Get the attribute count
   if (rc = GetRelationInfo(relName, tmpRec, relcatData))
      goto err_return;

   // Allocate indexhandle array
   ihs = new IX_IndexHandle[((SM_RelcatRec *)relcatData)->attrCount];
   if (ihs == NULL) {
      rc = SM_NOMEM;
      goto err_return;
   }

   // Allocate attributes array
   attributes = new SM_AttrcatRec[((SM_RelcatRec *)relcatData)->attrCount];
   if (attributes == NULL) {
      rc = SM_NOMEM;
      goto err_deleteihs;
   }

   // Allocate buffer
   buf = new char[MAXLINE];
   if (buf == NULL) {
      rc = SM_NOMEM;
      goto err_deleteattributes;
   }

   // Allocate data
   data = new char[((SM_RelcatRec *)relcatData)->tupleLength];
   if (data == NULL) {
      rc = SM_NOMEM;
      goto err_deletebuf;
   }

   // Open a file scan for ATTRCAT
   memset(_relName, '\0', sizeof(_relName));
   strncpy(_relName, relName, MAXNAME);
   if (rc = fs.OpenScan(fhAttrcat, STRING, MAXNAME,
                        OFFSET(SM_AttrcatRec, relName), EQ_OP, _relName))
      goto err_deletedata;

   // Fill out attributes array
   while ((rc = fs.GetNextRec(rec)) != RM_EOF) {
      char *_data;

      if (rc != 0) {
         fs.CloseScan();
         goto err_deletedata;
      }
      if (rc = rec.GetData(_data)) {
         fs.CloseScan();
         goto err_deletedata;
      }

      memcpy(&attributes[i], _data, sizeof(SM_AttrcatRec));
      if (++i == ((SM_RelcatRec *)relcatData)->attrCount)
         break;
   }

   // Close a file scan for ATTRCAT
   if (rc = fs.CloseScan())
      goto err_deletedata;

   // Open data file
   fp = fopen(fileName, "r");
   if (fp == NULL) {
      rc = SM_FILEIOFAILED;
      goto err_deletedata;
   }
  
   // Open relation file
   if (rc = pRmm->OpenFile(relName, fh))
      goto err_fclose;

   // Open indexes
   for (i = 0; i < ((SM_RelcatRec *)relcatData)->attrCount; i++) {
      if (attributes[i].indexNo == -1)
         continue;
      if (rc = pIxm->OpenIndex(relName, attributes[i].indexNo, ihs[i]))
         goto err_closeindexes;
   }
   
   // Process every line
   while (fgets(buf, MAXLINE, fp)) {
      int numDelim = 0;
      char *attr = buf;
      RID rid;

      // Count commas and confirm the whole line was read
      for (i = 0; i < MAXLINE - 1; i++) {
         if (buf[i] == '\n')
            break;
         else if (buf[i] == ',')
            numDelim++;
      }

      // Allow blank lines
      if (i == 0)
         continue;

      if (buf[i] != '\n' || buf[i+1] != '\0'
          || numDelim + 1 != ((SM_RelcatRec *)relcatData)->attrCount) {
         rc = SM_INVALIDFORMAT;
         goto err_closeindexes;
      }

      // Make record data
      for (i = 0; i < ((SM_RelcatRec *)relcatData)->attrCount; i++) {
         int _i;
         float _f;
         char *delim = strpbrk(attr, ",\n");
         *delim = '\0';

         switch (attributes[i].attrType) {
         case INT:
            _i = atoi(attr);
            memcpy(data + attributes[i].offset, &_i, sizeof(int));
            break;
         case FLOAT:
            _f = atof(attr);
            memcpy(data + attributes[i].offset, &_f, sizeof(float));
            break;
         case STRING:
            memset(data + attributes[i].offset, '\0', 
                   attributes[i].attrLength);
            strncpy(data + attributes[i].offset, attr, 
                    attributes[i].attrLength);
            break;
         }
         attr = delim + 1;
      }

      // Insert the record
      if (rc = fh.InsertRec(data, rid))
         goto err_closeindexes;

      // Update indexes
      for (i = 0; i < ((SM_RelcatRec *)relcatData)->attrCount; i++) {
         if (attributes[i].indexNo == -1)
            continue;
         if (rc = ihs[i].InsertEntry(data + attributes[i].offset, rid))
            goto err_closeindexes;
      }
   }

   // Close indexes
   for (i = 0; i < ((SM_RelcatRec *)relcatData)->attrCount; i++) {
      if (attributes[i].indexNo == -1)
         continue;
      if (rc = pIxm->CloseIndex(ihs[i]))
         goto err_closeindexes;
   }
   
   // Close relation file
   if (rc = pRmm->CloseFile(fh))
      goto err_fclose;

   // Close data file
   fclose(fp);

   // Deallocate 
   delete [] data;
   delete [] buf;
   delete [] attributes;
   delete [] ihs;

   // Return ok
   return (0);

   // Return error
err_closeindexes:
   for (i = 0; i < ((SM_RelcatRec *)relcatData)->attrCount; i++)
      if (attributes[i].indexNo != -1)
         pIxm->CloseIndex(ihs[i]);
//err_closefile:
   pRmm->CloseFile(fh);
err_fclose:
   fclose(fp);
err_deletedata:
   delete [] data;
err_deletebuf:
   delete [] buf;
err_deleteattributes:
   delete [] attributes;
err_deleteihs:
   delete [] ihs;
err_return:
   return (rc);
}

//
// compareDataAttrInfo
//
// Desc: for qsort
//
static int compareDataAttrInfo(const void *p1, const void *p2)
{
   int offset1 = ((DataAttrInfo *)p1)->offset;
   int offset2 = ((DataAttrInfo *)p2)->offset;

   return offset1 - offset2;
}

//
// Print
//
// Desc: 
// In:   relName - 
// Ret:  SM_RELNOTFOUND, RM return code
//
RC SM_Manager::Print(const char *relName)
{
   RC rc;
   RM_Record tmpRec;
   char *relcatData;
   DataAttrInfo *attributes;
   char _relName[MAXNAME];
   RM_FileScan fs;
   RM_Record rec;
   RM_FileHandle fh;
   int i = 0;

   // Get the attribute count
   if (rc = GetRelationInfo(relName, tmpRec, relcatData))
      return (rc);

   // Allocate attributes array
   attributes = new DataAttrInfo[((SM_RelcatRec *)relcatData)->attrCount];
   if (attributes == NULL)
      return (SM_NOMEM);

   // Open a file scan for ATTRCAT
   memset(_relName, '\0', sizeof(_relName));
   strncpy(_relName, relName, MAXNAME);
   if (rc = fs.OpenScan(fhAttrcat, STRING, MAXNAME,
                        OFFSET(SM_AttrcatRec, relName), EQ_OP, _relName)) {
      delete [] attributes;
      return (rc);
   }

   // Fill out attributes array
   while ((rc = fs.GetNextRec(rec)) != RM_EOF) {
      char *data;

      if (rc != 0) {
         fs.CloseScan();
         delete [] attributes;
         return (rc);
      }

      if (rc = rec.GetData(data)) {
         fs.CloseScan();
         delete [] attributes;
         return (rc);
      }

      SM_SetAttrcatRec(attributes[i],
                       ((SM_AttrcatRec *)data)->relName,
                       ((SM_AttrcatRec *)data)->attrName,
                       ((SM_AttrcatRec *)data)->offset,
                       ((SM_AttrcatRec *)data)->attrType,
                       ((SM_AttrcatRec *)data)->attrLength,
                       ((SM_AttrcatRec *)data)->indexNo);
      if (++i == ((SM_RelcatRec *)relcatData)->attrCount)
         break;
   }

   // Close a file scan for ATTRCAT
   if (rc = fs.CloseScan()) {
      delete [] attributes;
      return (rc);
   }

   // Instantiate a Printer object
   qsort(attributes, ((SM_RelcatRec *)relcatData)->attrCount,
         sizeof(DataAttrInfo), compareDataAttrInfo);
   Printer p(attributes, ((SM_RelcatRec *)relcatData)->attrCount);

   // Open relation file
   if (rc = pRmm->OpenFile(relName, fh))
      goto err_delete;
   // Print the header information
   p.PrintHeader(cout);

   // Normal Print
   if (useIndexNo < 0) {
      if (rc = fs.OpenScan(fh, INT, sizeof(int), 0, NO_OP, NULL))
         goto err_closefile;

      while ((rc = fs.GetNextRec(rec)) != RM_EOF) {
         char *data;

         if (rc != 0)
            goto err_closescan;

         if (rc = rec.GetData(data))
            goto err_closescan;

         p.Print(cout, data);
      }

      if (rc = fs.CloseScan())
         goto err_closefile;
   }
   // Sorted Print
   else {
      IX_IndexHandle ih;
      IX_IndexScan is;
      RID rid;

      if (rc = pIxm->OpenIndex(relName, useIndexNo, ih))
         goto err_closefile;

      if (rc = is.OpenScan(ih, NO_OP, NULL)) {
         pIxm->CloseIndex(ih);
         goto err_closefile;
      }

      while ((rc = is.GetNextEntry(rid)) != IX_EOF) {
         char *data;

         if (rc != 0) {
            is.CloseScan();
            pIxm->CloseIndex(ih);
            goto err_closefile;
         }
          
         if (rc = fh.GetRec(rid, rec)) {
            is.CloseScan();
            pIxm->CloseIndex(ih);
            goto err_closefile;
         }

         if (rc = rec.GetData(data))
            goto err_closescan;

         p.Print(cout, data);
      }

      if (rc = is.CloseScan()) {
         pIxm->CloseIndex(ih);
         goto err_closefile;
      }
      if (rc = pIxm->CloseIndex(ih))
         goto err_closefile;
   }

   // Print the footer information
   p.PrintFooter(cout);
   // Close relation file
   if (rc = pRmm->CloseFile(fh))
      goto err_delete;

   // Deallocate attributes
   delete [] attributes;

   // Return ok
   return (0);

   // Return error
err_closescan:
   fs.CloseScan();
err_closefile:
   pRmm->CloseFile(fh);
err_delete:
   delete [] attributes;
//err_return:
   return (rc);
}

//
// Set
//
// Desc: 
// In:   paramName - 
//       value - 
// Ret:  
//
RC SM_Manager::Set(const char *paramName, const char *value)
{
   if (strcasecmp(paramName, "useindex") == 0)
      useIndexNo = atoi(value);
   else
      return (SM_PARAMUNDEFINED);

   // Return ok
   return (0);
}

//
// Help
//
// Desc: 
// Ret:  RM return code
//
RC SM_Manager::Help()
{
   RC rc;
   DataAttrInfo attributes[4];
   RM_FileScan fs;
   RM_Record rec;

   // Instantiate a Printer object
   SM_SetAttrcatRec(attributes[0],
                    RELCAT, "relName", OFFSET(SM_RelcatRec, relName),
                    STRING, MAXNAME, -1);
   SM_SetAttrcatRec(attributes[1],
                    RELCAT, "tupleLength", OFFSET(SM_RelcatRec, tupleLength),
                    INT, sizeof(int), -1);
   SM_SetAttrcatRec(attributes[2],
                    RELCAT, "attrCount", OFFSET(SM_RelcatRec, attrCount),
                    INT, sizeof(int), -1);
   SM_SetAttrcatRec(attributes[3],
                    RELCAT, "indexCount", OFFSET(SM_RelcatRec, indexCount),
                    INT, sizeof(int), -1);
   Printer p(attributes, 4);

   // Open a file scan for RELCAT
   if (rc = fs.OpenScan(fhRelcat, INT, sizeof(int), 0, NO_OP, NULL))
      goto err_return;

   // Print the header information
   p.PrintHeader(cout);

   // Print each tuple
   while ((rc = fs.GetNextRec(rec)) != RM_EOF) {
      char *data;

      if (rc != 0)
         goto err_closescan;

      if (rc = rec.GetData(data))
         goto err_closescan;

      p.Print(cout, data);
   }

   // Print the footer information
   p.PrintFooter(cout);

   // Close a file scan for RELCAT
   if (rc = fs.CloseScan())
      goto err_return;

   // Return ok
   return (0);

   // Return error
err_closescan:
   fs.CloseScan();
err_return:
   return (rc);
}

//
// Help
//
// Desc: 
// In:   relName -
// Ret:  SM_RELNOTFOUND, RM return code
//
RC SM_Manager::Help(const char *relName)
{
   RC rc;
   RM_Record tmpRec;
   char *relcatData;
   DataAttrInfo attributes[6];
   char _relName[MAXNAME];
   RM_FileScan fs;
   RM_Record rec;
   int i = 0;

   // Get the attribute count
   if (rc = GetRelationInfo(relName, tmpRec, relcatData))
      return (rc);

   // Instantiate a Printer object
   SM_SetAttrcatRec(attributes[0],
                    ATTRCAT, "relName", OFFSET(SM_AttrcatRec, relName),
                    STRING, MAXNAME, -1);
   SM_SetAttrcatRec(attributes[1],
                    ATTRCAT, "attrName", OFFSET(SM_AttrcatRec, attrName),
                    STRING, MAXNAME, -1);
   SM_SetAttrcatRec(attributes[2],
                    ATTRCAT, "offset", OFFSET(SM_AttrcatRec, offset),
                    INT, sizeof(int), -1);
   SM_SetAttrcatRec(attributes[3],
                    ATTRCAT, "attrType", OFFSET(SM_AttrcatRec, attrType),
                    INT, sizeof(int), -1);
   SM_SetAttrcatRec(attributes[4],
                    ATTRCAT, "attrLength", OFFSET(SM_AttrcatRec, attrLength),
                    INT, sizeof(int), -1);
   SM_SetAttrcatRec(attributes[5],
                    ATTRCAT, "indexNo", OFFSET(SM_AttrcatRec, indexNo),
                    INT, sizeof(int), -1);
   Printer p(attributes, 6);

   // Open a file scan for ATTRCAT
   memset(_relName, '\0', sizeof(_relName));
   strncpy(_relName, relName, MAXNAME);
   if (rc = fs.OpenScan(fhAttrcat, STRING, MAXNAME,
                        OFFSET(SM_AttrcatRec, relName), EQ_OP, _relName))
      goto err_return;

   // Print the header information
   p.PrintHeader(cout);

   // Print each tuple
   while ((rc = fs.GetNextRec(rec)) != RM_EOF) {
      char *data;

      if (rc != 0)
         goto err_closescan;

      if (rc = rec.GetData(data))
         goto err_closescan;

      p.Print(cout, data);
      if (++i == ((SM_RelcatRec *)relcatData)->attrCount)
         break;
   }

   // Print the footer information
   p.PrintFooter(cout);

   // Close a file scan for ATTRCAT
   if (rc = fs.CloseScan())
      goto err_return;

   // Return ok
   return (0);

   // Return error
err_closescan:
   fs.CloseScan();
err_return:
   return (rc);
}

//
// GetRelationInfo
//
// Desc: Get relation informtion by accessing catalog RELCAT
// In:   relName -
// Out:  rec - 
//       data -
// Ret:  SM_RELNOTFOUND, RM return code
//
RC SM_Manager::GetRelationInfo(const char *relName, 
                               RM_Record &rec, char *&data)
{
   RC rc;
   char _relName[MAXNAME];
   RM_FileScan fs;

   // Open a file scan for RELCAT
   memset(_relName, '\0', sizeof(_relName));
   strncpy(_relName, relName, MAXNAME);
   if (rc = fs.OpenScan(fhRelcat, STRING, MAXNAME,
                        OFFSET(SM_RelcatRec, relName), EQ_OP, _relName))
      goto err_return;

   // Find the matching record
   if (rc = fs.GetNextRec(rec)) {
      rc = (rc == RM_EOF) ? SM_RELNOTFOUND : rc;
      goto err_closescan;
   } else {
      if (rc = rec.GetData(data))
         goto err_closescan;
   }

   // Close a file scan for RELCAT
   if (rc = fs.CloseScan())
      goto err_return;

   // Return ok
   return (0);

   // Return error
err_closescan:
   fs.CloseScan();
err_return:
   return (rc);
}

//
// SetRelationIndexCount
//
// Desc: sets attribute count by accessing catalog RELCAT
// In:   relName - should exist
// Out:  value - value to be added to indexCount
//               indexCount is the only attribute which can be updated
// Ret:  SM_RELNOTFOUND, RM return code
//
RC SM_Manager::SetRelationIndexCount(const char *relName, int value)
{
   RC rc;
   RM_Record rec;
   char *relcatData;

   if (rc = GetRelationInfo(relName, rec, relcatData))
      goto err_return;
      
   // Update indexCount
   ((SM_RelcatRec *)relcatData)->indexCount += value;
   if (rc = fhRelcat.UpdateRec(rec))
      goto err_return;
   if (rc = fhRelcat.ForcePages())
      goto err_return;

   // Return ok
   return (0);

   // Return error
err_return:
   return (rc);
}

//
// GetAttributeInfo
//
// Desc: Get attribute informtion by accessing catalog ATTRCAT
// In:   relName -
//       attrName -
// Out:  rec - 
//       data -
// Ret:  SM_ATTRNOTFOUND, RM return code
//
RC SM_Manager::GetAttributeInfo(const char *relName, const char *attrName,
                                RM_Record &rec, char *&data)
{
   RC rc;
   char _relattrName[MAXNAME*2];
   RM_FileScan fs;

   // Open a file scan for ATTRCAT
   memset(_relattrName, '\0', sizeof(_relattrName));
   strncpy(_relattrName, relName, MAXNAME);
   strncpy(_relattrName + MAXNAME, attrName, MAXNAME);
   if (rc = fs.OpenScan(fhAttrcat, STRING, MAXNAME*2,
                        OFFSET(SM_AttrcatRec, relName), EQ_OP, _relattrName))
      goto err_return;

   // Find the matching record
   if (rc = fs.GetNextRec(rec)) {
      rc = (rc == RM_EOF) ? SM_ATTRNOTFOUND : rc;
      goto err_closescan;
   } else {
      if (rc = rec.GetData(data))
         goto err_closescan;
   }

   // Close a file scan for ATTRCAT
   if (rc = fs.CloseScan())
      goto err_return;

   // Return ok
   return (0);

   // Return error
err_closescan:
   fs.CloseScan();
err_return:
   return (rc);
}

