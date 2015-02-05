//
// File:        ql_internal.h
// Description: Declarations internal to the QL component
// Author:      Hyunjung Park (hyunjung@stanford.edu)
//

#ifndef QL_INTERNAL_H
#define QL_INTERNAL_H

#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <cassert>
#include "ql.h"
#include "printer.h"

using namespace std;

extern int compareDataAttrInfo(const void *, const void *);
extern double EstSelectivity(const Condition *, SM_Manager *);

//
// Constants and defines
//
struct QL_RelAttrInfo {
    int offset;
    AttrType attrType;
    int attrLength;
};

class QL_Operator {
public:
    QL_Operator            ()            {};
    virtual ~QL_Operator   ()            {};
    virtual RC Initialize  (AttrType = INT, int = 0, char * = NULL)
                                         { return (0); };
    virtual RC GetNext     (RM_Record &) { return (0); };
    virtual RC Finalize    ()            { return (0); };
    virtual RC SchemaLookup(const RelAttr &, QL_RelAttrInfo &)
                                         { return (0); };
    virtual RC EstimateCard(double &)    { return (0); };
    virtual RC EstimateIO  (double &)    { return (0); };
    virtual void Print     (ostream &, int = 0) {};

    int tupleLength;

private:
    QL_Operator  (const QL_Operator &);
    QL_Operator& operator=(const QL_Operator &);
};

class QL_TblScanOp : public QL_Operator {
public:
    QL_TblScanOp     (const char *, RM_FileHandle &, const Condition &,
                      SM_Manager *);
    ~QL_TblScanOp    ();
    RC Initialize    (AttrType, int, char *);
    RC GetNext       (RM_Record &);
    RC Finalize      ();
    RC SchemaLookup  (const RelAttr &, QL_RelAttrInfo &);
    RC EstimateCard  (double &);
    RC EstimateIO    (double &);
    void Print       (ostream &, int);

private:
    QL_TblScanOp  (const QL_TblScanOp &);
    QL_TblScanOp& operator=(const QL_TblScanOp &);

    const char       *tableName;
    RM_FileHandle    *pRmfh;
    const Condition  *pCondition;
    SM_Manager       *pSmm;

    CompOp           op;
    RM_FileScan      rmfs;
    QL_RelAttrInfo   fsAttrInfo;
    char             *valueData;
    char             *leftData;
    bool             alwaysEOF;
    ClientHint       pinHint;
};

class QL_IxScanOp : public QL_Operator {
public:
    QL_IxScanOp      (const char *, RM_FileHandle &, const Condition &,
                      IX_Manager *, SM_Manager *);
    ~QL_IxScanOp     ();
    RC Initialize    (AttrType, int, char *);
    RC GetNext       (RM_Record &);
    RC Finalize      ();
    RC SchemaLookup  (const RelAttr &, QL_RelAttrInfo &);
    RC EstimateCard  (double &);
    RC EstimateIO    (double &);
    void Print       (ostream &, int);

private:
    QL_IxScanOp  (const QL_IxScanOp &);
    QL_IxScanOp& operator=(const QL_IxScanOp &);

    const char       *tableName;
    RM_FileHandle    *pRmfh;
    const Condition  *pCondition;
    IX_Manager       *pIxm;
    SM_Manager       *pSmm;

    CompOp           op;
    IX_IndexHandle   ixih;
    IX_IndexScan     ixis;
    QL_RelAttrInfo   isAttrInfo;
    char             *valueData;
    char             *leftData;
    bool             alwaysEOF;
};

class QL_FilterOp : public QL_Operator {
public:
    QL_FilterOp      (QL_Operator *, const Condition &,
                      SM_Manager *);
    ~QL_FilterOp     ();
    RC Initialize    (AttrType, int, char *);
    RC GetNext       (RM_Record &);
    RC Finalize      ();
    RC SchemaLookup  (const RelAttr &, QL_RelAttrInfo &);
    RC EstimateCard  (double &);
    RC EstimateIO    (double &);
    void Print       (ostream &, int);

private:
    QL_FilterOp  (const QL_FilterOp &);
    QL_FilterOp& operator=(const QL_FilterOp &);

    QL_Operator      *pChild;
    const Condition  *pCondition;
    SM_Manager       *pSmm;
    QL_RelAttrInfo   lhsAttrInfo;
    QL_RelAttrInfo   rhsAttrInfo;
};

class QL_ProjectOp : public QL_Operator {
public:
    QL_ProjectOp     (QL_Operator *, int, const RelAttr []);
    ~QL_ProjectOp    ();
    RC Initialize    (AttrType, int, char *);
    RC GetNext       (RM_Record &);
    RC Finalize      ();
    RC SchemaLookup  (const RelAttr &, QL_RelAttrInfo &);
    RC EstimateCard  (double &);
    RC EstimateIO    (double &);
    void Print       (ostream &, int);

private:
    QL_ProjectOp  (const QL_ProjectOp &);
    QL_ProjectOp& operator=(const QL_ProjectOp &);

    QL_Operator      *pChild;
    int              nProjAttrs;
    RelAttr          *projAttrs;
    QL_RelAttrInfo   *projAttrInfos;
};

class QL_NLJOp : public QL_Operator {
public:
    QL_NLJOp         (QL_Operator *, QL_Operator *, const Condition &,
                      SM_Manager *);
    ~QL_NLJOp        ();
    RC Initialize    (AttrType, int, char *);
    RC GetNext       (RM_Record &);
    RC Finalize      ();
    RC SchemaLookup  (const RelAttr &, QL_RelAttrInfo &);
    RC EstimateCard  (double &);
    RC EstimateIO    (double &);
    void Print       (ostream &, int);

private:
    QL_NLJOp  (const QL_NLJOp &);
    QL_NLJOp& operator=(const QL_NLJOp &);

    QL_Operator      *pLeftChild;
    QL_Operator      *pRightChild;
    const Condition  *pCondition;
    SM_Manager       *pSmm;

    CompOp           op;
    QL_RelAttrInfo   leftAttrInfo;
    RM_Record        leftRec;
    char             *leftData;
    QL_RelAttrInfo   rightAttrInfo;
};

class QL_NBJOp : public QL_Operator {
public:
    QL_NBJOp         (QL_Operator *, QL_Operator *, const Condition &,
                      int, PF_Manager *, SM_Manager *);
    ~QL_NBJOp        ();
    RC Initialize    (AttrType, int, char *);
    RC GetNext       (RM_Record &);
    RC Finalize      ();
    RC SchemaLookup  (const RelAttr &, QL_RelAttrInfo &);
    RC EstimateCard  (double &);
    RC EstimateIO    (double &);
    void Print       (ostream &, int);

private:
    QL_NBJOp  (const QL_NBJOp &);
    QL_NBJOp& operator=(const QL_NBJOp &);

    QL_Operator      *pLeftChild;
    QL_Operator      *pRightChild;
    const Condition  *pCondition;
    SM_Manager       *pSmm;

    CompOp           op;
    QL_RelAttrInfo   leftAttrInfo;
    QL_RelAttrInfo   rightAttrInfo;
    RM_Record        rightRec;
    char             *rightData;

    PF_Manager       *pPfm;
    int              numBufs;
    char             **bufs;
    int              numTuplesPerBuf;
    int              numTuplesInBufs;
    int              curTupleNum;
};

#endif

