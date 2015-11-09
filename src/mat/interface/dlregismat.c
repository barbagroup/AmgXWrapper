
#include <petsc/private/matimpl.h>

const char       *MatOptions[] = {"NEW_NONZERO_LOCATION_ERR",
                                  "UNUSED_NONZERO_LOCATION_ERR",
                                  "NEW_NONZERO_ALLOCATION_ERR",
                                  "ROW_ORIENTED",
                                  "NEW_NONZERO_LOCATIONS",
                                  "SYMMETRIC",
                                  "STRUCTURALLY_SYMMETRIC",
                                  "NEW_DIAGONALS",
                                  "IGNORE_OFF_PROC_ENTRIES",
                                  "USE_HASH_TABLE",
                                  "KEEP_NONZERO_PATTERN",
                                  "IGNORE_ZERO_ENTRIES",
                                  "USE_INODES",
                                  "HERMITIAN",
                                  "SYMMETRY_ETERNAL",
                                  "DUMMY",
                                  "IGNORE_LOWER_TRIANGULAR",
                                  "ERROR_LOWER_TRIANGULAR",
                                  "GETROW_UPPERTRIANGULAR",
                                  "SPD",
                                  "NO_OFF_PROC_ZERO_ROWS",
                                  "NO_OFF_PROC_ENTRIES",
                                  "NEW_NONZERO_LOCATIONS",
                                  "MatOption","MAT_",0};
const char *const MatFactorShiftTypes[] = {"NONE","NONZERO","POSITIVE_DEFINITE","INBLOCKS","MatFactorShiftType","PC_FACTOR_",0};
const char *const MatFactorShiftTypesDetail[] = {NULL,"diagonal shift to prevent zero pivot","Manteuffel shift","diagonal shift on blocks to prevent zero pivot"};
const char *const MPPTScotchStrategyTypes[] = {"QUALITY","SPEED","BALANCE","SAFETY","SCALABILITY","MPPTScotchStrategyType","MP_PTSCOTCH_",0};
const char *const MPChacoGlobalTypes[] = {"","MULTILEVEL","SPECTRAL","","LINEAR","RANDOM","SCATTERED","MPChacoGlobalType","MP_CHACO_",0};
const char *const MPChacoLocalTypes[] = {"","KERNIGHAN","NONE","MPChacoLocalType","MP_CHACO_",0};
const char *const MPChacoEigenTypes[] = {"LANCZOS","RQI","MPChacoEigenType","MP_CHACO_",0};

extern PetscErrorCode  MatMFFDInitializePackage(void);
extern PetscErrorCode  MatSolverPackageDestroy(void);
static PetscBool MatPackageInitialized = PETSC_FALSE;
#undef __FUNCT__
#define __FUNCT__ "MatFinalizePackage"
/*@C
  MatFinalizePackage - This function destroys everything in the Petsc interface to the Mat package. It is
  called from PetscFinalize().

  Level: developer

.keywords: Petsc, destroy, package, mathematica
.seealso: PetscFinalize()
@*/
PetscErrorCode  MatFinalizePackage(void)
{
  MatBaseName    nnames,names = MatBaseNameList;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = MatSolverPackageDestroy();CHKERRQ(ierr);
  while (names) {
    nnames = names->next;
    ierr   = PetscFree(names->bname);CHKERRQ(ierr);
    ierr   = PetscFree(names->sname);CHKERRQ(ierr);
    ierr   = PetscFree(names->mname);CHKERRQ(ierr);
    ierr   = PetscFree(names);CHKERRQ(ierr);
    names  = nnames;
  }
  ierr = PetscFunctionListDestroy(&MatList);CHKERRQ(ierr);
  ierr = PetscFunctionListDestroy(&MatOrderingList);CHKERRQ(ierr);
  ierr = PetscFunctionListDestroy(&MatColoringList);CHKERRQ(ierr);
  ierr = PetscFunctionListDestroy(&MatPartitioningList);CHKERRQ(ierr);
  ierr = PetscFunctionListDestroy(&MatCoarsenList);CHKERRQ(ierr);
  MatBaseNameList                  = NULL;
  MatPackageInitialized            = PETSC_FALSE;
  MatRegisterAllCalled             = PETSC_FALSE;
  MatOrderingRegisterAllCalled     = PETSC_FALSE;
  MatColoringRegisterAllCalled     = PETSC_FALSE;
  MatPartitioningRegisterAllCalled = PETSC_FALSE;
  MatCoarsenRegisterAllCalled      = PETSC_FALSE;
  PetscFunctionReturn(0);
}

#if defined(PETSC_HAVE_MUMPS)
PETSC_EXTERN PetscErrorCode MatSolverPackageRegister_MUMPS(void);
#endif
#if defined(PETSC_HAVE_CUSP)
PETSC_EXTERN PetscErrorCode MatSolverPackageRegister_CUSPARSE(void);
#endif
#if defined(PETSC_HAVE_ELEMENTAL)
PETSC_EXTERN PetscErrorCode MatSolverPackageRegister_Elemental(void);
#endif
#if defined(PETSC_HAVE_MATLAB_ENGINE)
PETSC_EXTERN PetscErrorCode MatSolverPackageRegister_Matlab(void);
#endif
#if defined(PETSC_HAVE_PETSC_HAVE_ESSL) && !defined(PETSC_USE_COMPLEX) && !defined(PETSC_USE_REAL_SINGLE) && !defined(PETSC_USE_REAL___FLOAT128)
PETSC_EXTERN PetscErrorCode MatSolverPackageRegister_Essl(void);
#endif
#if defined(PETSC_HAVE_SUPERLU)
PETSC_EXTERN PetscErrorCode MatSolverPackageRegister_SuperLU(void);
#endif
#if defined(PETSC_HAVE_PASTIX)
PETSC_EXTERN PetscErrorCode MatSolverPackageRegister_Pastix(void);
#endif
#if defined(PETSC_HAVE_SUPERLU_DIST)
PETSC_EXTERN PetscErrorCode MatSolverPackageRegister_SuperLU_DIST(void);
#endif
#if defined(PETSC_HAVE_CLIQUE)
PETSC_EXTERN PetscErrorCode MatSolverPackageRegister_Clique(void);
#endif
#if defined(PETSC_HAVE_MKL_PARDISO)
PETSC_EXTERN PetscErrorCode MatSolverPackageRegister_MKL_Pardiso(void);
#endif
#if defined(PETSC_HAVE_MKL_CPARDISO)
PETSC_EXTERN PetscErrorCode MatSolverPackageRegister_MKL_CPardiso(void);
#endif
#if defined(PETSC_HAVE_SUITESPARSE)
PETSC_EXTERN PetscErrorCode MatSolverPackageRegister_SuiteSparse(void);
#endif
#if defined(PETSC_HAVE_LUSOL)
PETSC_EXTERN PetscErrorCode MatSolverPackageRegister_Lusol(void);
#endif

PETSC_EXTERN PetscErrorCode MatGetFactor_seqaij_petsc(Mat,MatFactorType,Mat*);
PETSC_EXTERN PetscErrorCode MatGetFactor_seqbaij_petsc(Mat,MatFactorType,Mat*);
PETSC_EXTERN PetscErrorCode MatGetFactor_seqsbaij_petsc(Mat,MatFactorType,Mat*);
PETSC_EXTERN PetscErrorCode MatGetFactor_seqdense_petsc(Mat,MatFactorType,Mat*);
PETSC_EXTERN PetscErrorCode MatGetFactor_seqaij_bas(Mat,MatFactorType,Mat*);
PETSC_EXTERN PetscErrorCode MatGetFactor_seqbaij_bstrm(Mat,MatFactorType,Mat*);
PETSC_EXTERN PetscErrorCode MatGetFactor_seqsbaij_sbstrm(Mat,MatFactorType,Mat*);

#undef __FUNCT__
#define __FUNCT__ "MatInitializePackage"
/*@C
  MatInitializePackage - This function initializes everything in the Mat package. It is called
  from PetscDLLibraryRegister() when using dynamic libraries, and on the first call to MatCreate()
  when using static libraries.

  Level: developer

.keywords: Mat, initialize, package
.seealso: PetscInitialize()
@*/
PetscErrorCode  MatInitializePackage(void)
{
  char           logList[256];
  char           *className;
  PetscBool      opt;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (MatPackageInitialized) PetscFunctionReturn(0);
  MatPackageInitialized = PETSC_TRUE;
  /* Inialize subpackage */
  ierr = MatMFFDInitializePackage();CHKERRQ(ierr);
  /* Register Classes */
  ierr = PetscClassIdRegister("Matrix",&MAT_CLASSID);CHKERRQ(ierr);
  ierr = PetscClassIdRegister("Matrix FD Coloring",&MAT_FDCOLORING_CLASSID);CHKERRQ(ierr);
  ierr = PetscClassIdRegister("Matrix Coloring",&MAT_COLORING_CLASSID);CHKERRQ(ierr);
  ierr = PetscClassIdRegister("Matrix MatTranspose Coloring",&MAT_TRANSPOSECOLORING_CLASSID);CHKERRQ(ierr);
  ierr = PetscClassIdRegister("Matrix Partitioning",&MAT_PARTITIONING_CLASSID);CHKERRQ(ierr);
  ierr = PetscClassIdRegister("Matrix Coarsen",&MAT_COARSEN_CLASSID);CHKERRQ(ierr);
  ierr = PetscClassIdRegister("Matrix Null Space",&MAT_NULLSPACE_CLASSID);CHKERRQ(ierr);
  /* Register Constructors */
  ierr = MatRegisterAll();CHKERRQ(ierr);
  ierr = MatOrderingRegisterAll();CHKERRQ(ierr);
  ierr = MatColoringRegisterAll();CHKERRQ(ierr);
  ierr = MatPartitioningRegisterAll();CHKERRQ(ierr);
  ierr = MatCoarsenRegisterAll();CHKERRQ(ierr);
  /* Register Events */
  ierr = PetscLogEventRegister("MatMult",          MAT_CLASSID,&MAT_Mult);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatMults",         MAT_CLASSID,&MAT_Mults);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatMultConstr",    MAT_CLASSID,&MAT_MultConstrained);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatMultAdd",       MAT_CLASSID,&MAT_MultAdd);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatMultTranspose", MAT_CLASSID,&MAT_MultTranspose);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatMultTrConstr",  MAT_CLASSID,&MAT_MultTransposeConstrained);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatMultTrAdd",     MAT_CLASSID,&MAT_MultTransposeAdd);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatSolve",         MAT_CLASSID,&MAT_Solve);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatSolves",        MAT_CLASSID,&MAT_Solves);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatSolveAdd",      MAT_CLASSID,&MAT_SolveAdd);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatSolveTranspos", MAT_CLASSID,&MAT_SolveTranspose);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatSolveTrAdd",    MAT_CLASSID,&MAT_SolveTransposeAdd);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatSOR",           MAT_CLASSID,&MAT_SOR);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatForwardSolve",  MAT_CLASSID,&MAT_ForwardSolve);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatBackwardSolve", MAT_CLASSID,&MAT_BackwardSolve);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatLUFactor",      MAT_CLASSID,&MAT_LUFactor);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatLUFactorSym",   MAT_CLASSID,&MAT_LUFactorSymbolic);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatLUFactorNum",   MAT_CLASSID,&MAT_LUFactorNumeric);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatCholeskyFctr",  MAT_CLASSID,&MAT_CholeskyFactor);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatCholFctrSym",   MAT_CLASSID,&MAT_CholeskyFactorSymbolic);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatCholFctrNum",   MAT_CLASSID,&MAT_CholeskyFactorNumeric);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatILUFactor",     MAT_CLASSID,&MAT_ILUFactor);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatILUFactorSym",  MAT_CLASSID,&MAT_ILUFactorSymbolic);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatICCFactorSym",  MAT_CLASSID,&MAT_ICCFactorSymbolic);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatCopy",          MAT_CLASSID,&MAT_Copy);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatConvert",       MAT_CLASSID,&MAT_Convert);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatScale",         MAT_CLASSID,&MAT_Scale);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatResidual",      MAT_CLASSID,&MAT_Residual);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatAssemblyBegin", MAT_CLASSID,&MAT_AssemblyBegin);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatAssemblyEnd",   MAT_CLASSID,&MAT_AssemblyEnd);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatSetValues",     MAT_CLASSID,&MAT_SetValues);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatGetValues",     MAT_CLASSID,&MAT_GetValues);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatGetRow",        MAT_CLASSID,&MAT_GetRow);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatGetRowIJ",      MAT_CLASSID,&MAT_GetRowIJ);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatGetSubMatrice", MAT_CLASSID,&MAT_GetSubMatrices);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatGetSubMatrix",  MAT_CLASSID,&MAT_GetSubMatrix);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatGetOrdering",   MAT_CLASSID,&MAT_GetOrdering);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatIncreaseOvrlp", MAT_CLASSID,&MAT_IncreaseOverlap);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatPartitioning",  MAT_PARTITIONING_CLASSID,&MAT_Partitioning);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatCoarsen",  MAT_COARSEN_CLASSID,&MAT_Coarsen);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatZeroEntries",   MAT_CLASSID,&MAT_ZeroEntries);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatLoad",          MAT_CLASSID,&MAT_Load);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatView",          MAT_CLASSID,&MAT_View);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatAXPY",          MAT_CLASSID,&MAT_AXPY);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatFDColorCreate", MAT_FDCOLORING_CLASSID,&MAT_FDColoringCreate);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatFDColorSetUp",  MAT_FDCOLORING_CLASSID,&MAT_FDColoringSetUp);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatFDColorApply",  MAT_FDCOLORING_CLASSID,&MAT_FDColoringApply);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatFDColorFunc",   MAT_FDCOLORING_CLASSID,&MAT_FDColoringFunction);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatTranspose",     MAT_CLASSID,&MAT_Transpose);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatMatMult",       MAT_CLASSID,&MAT_MatMult);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatMatSolve",      MAT_CLASSID,&MAT_MatSolve);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatMatMultSym",    MAT_CLASSID,&MAT_MatMultSymbolic);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatMatMultNum",    MAT_CLASSID,&MAT_MatMultNumeric);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatMatMatMult",    MAT_CLASSID,&MAT_MatMatMult);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatMatMatMultSym", MAT_CLASSID,&MAT_MatMatMultSymbolic);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatMatMatMultNum", MAT_CLASSID,&MAT_MatMatMultNumeric);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatPtAP",          MAT_CLASSID,&MAT_PtAP);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatPtAPSymbolic",  MAT_CLASSID,&MAT_PtAPSymbolic);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatPtAPNumeric",   MAT_CLASSID,&MAT_PtAPNumeric);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatRARt",          MAT_CLASSID,&MAT_RARt);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatRARtSym",       MAT_CLASSID,&MAT_RARtSymbolic);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatRARtNum",       MAT_CLASSID,&MAT_RARtNumeric);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatMatTransMult",  MAT_CLASSID,&MAT_MatTransposeMult);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatMatTrnMultSym", MAT_CLASSID,&MAT_MatTransposeMultSymbolic);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatMatTrnMultNum", MAT_CLASSID,&MAT_MatTransposeMultNumeric);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatTrnMatMult",    MAT_CLASSID,&MAT_TransposeMatMult);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatTrnMatMultSym", MAT_CLASSID,&MAT_TransposeMatMultSymbolic);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatTrnMatMultNum", MAT_CLASSID,&MAT_TransposeMatMultNumeric);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatTrnColorCreate", MAT_CLASSID,&MAT_TransposeColoringCreate);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatRedundantMat",  MAT_CLASSID,&MAT_RedundantMat);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatGetSeqNZStrct", MAT_CLASSID,&MAT_GetSequentialNonzeroStructure);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatGetMultiProcBlock", MAT_CLASSID,&MAT_GetMultiProcBlock);CHKERRQ(ierr);

  /* these may be specific to MPIAIJ matrices */
  ierr = PetscLogEventRegister("MatMPISumSeqNumeric",MAT_CLASSID,&MAT_Seqstompinum);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatMPISumSeqSymbolic",MAT_CLASSID,&MAT_Seqstompisym);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatMPISumSeq",MAT_CLASSID,&MAT_Seqstompi);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatMPIConcateSeq",MAT_CLASSID,&MAT_Merge);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatGetLocalMat",MAT_CLASSID,&MAT_Getlocalmat);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatGetLocalMatCondensed",MAT_CLASSID,&MAT_Getlocalmatcondensed);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatGetBrowsOfAcols",MAT_CLASSID,&MAT_GetBrowsOfAcols);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatGetBrAoCol",MAT_CLASSID,&MAT_GetBrowsOfAocols);CHKERRQ(ierr);

  ierr = PetscLogEventRegister("MatApplyPAPt_Symbolic",MAT_CLASSID,&MAT_Applypapt_symbolic);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatApplyPAPt_Numeric",MAT_CLASSID,&MAT_Applypapt_numeric);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatApplyPAPt",MAT_CLASSID,&MAT_Applypapt);CHKERRQ(ierr);

  ierr = PetscLogEventRegister("MatGetSymTrans",MAT_CLASSID,&MAT_Getsymtranspose);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatGetSymTransR",MAT_CLASSID,&MAT_Getsymtransreduced);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatTranspose_SeqAIJ_FAST",MAT_CLASSID,&MAT_Transpose_SeqAIJ);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatCUSPCopyTo",MAT_CLASSID,&MAT_CUSPCopyToGPU);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatCUSPARSECopyTo",MAT_CLASSID,&MAT_CUSPARSECopyToGPU);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatViennaCLCopyTo",MAT_CLASSID,&MAT_ViennaCLCopyToGPU);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatSetValBatch",MAT_CLASSID,&MAT_SetValuesBatch);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatSetValBatch1",MAT_CLASSID,&MAT_SetValuesBatchI);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatSetValBatch2",MAT_CLASSID,&MAT_SetValuesBatchII);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatSetValBatch3",MAT_CLASSID,&MAT_SetValuesBatchIII);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatSetValBatch4",MAT_CLASSID,&MAT_SetValuesBatchIV);CHKERRQ(ierr);

  ierr = PetscLogEventRegister("MatColoringApply",MAT_COLORING_CLASSID,&Mat_Coloring_Apply);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatColoringComm",MAT_COLORING_CLASSID,&Mat_Coloring_Comm);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatColoringLocal",MAT_COLORING_CLASSID,&Mat_Coloring_Local);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatColoringIS",MAT_COLORING_CLASSID,&Mat_Coloring_ISCreate);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatColoringSetUp",MAT_COLORING_CLASSID,&Mat_Coloring_SetUp);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("MatColoringWeights",MAT_COLORING_CLASSID,&Mat_Coloring_Weights);CHKERRQ(ierr);

  /* Turn off high traffic events by default */
  ierr = PetscLogEventSetActiveAll(MAT_SetValues, PETSC_FALSE);CHKERRQ(ierr);
  /* Process info exclusions */
  ierr = PetscOptionsGetString(NULL,NULL, "-info_exclude", logList, 256, &opt);CHKERRQ(ierr);
  if (opt) {
    ierr = PetscStrstr(logList, "mat", &className);CHKERRQ(ierr);
    if (className) {
      ierr = PetscInfoDeactivateClass(MAT_CLASSID);CHKERRQ(ierr);
    }
  }
  /* Process summary exclusions */
  ierr = PetscOptionsGetString(NULL,NULL, "-log_summary_exclude", logList, 256, &opt);CHKERRQ(ierr);
  if (opt) {
    ierr = PetscStrstr(logList, "mat", &className);CHKERRQ(ierr);
    if (className) {
      ierr = PetscLogEventDeactivateClass(MAT_CLASSID);CHKERRQ(ierr);
    }
  }

  /*
     Register the external package factorization based solvers
        Eventually we don't want to have these hardwired here at compile time of PETSc
  */
#if defined(PETSC_HAVE_MUMPS)
  ierr = MatSolverPackageRegister_MUMPS();CHKERRQ(ierr);
#endif
#if defined(PETSC_HAVE_CUSP)
  ierr = MatSolverPackageRegister_CUSPARSE();CHKERRQ(ierr);
#endif
#if defined(PETSC_HAVE_ELEMENTAL)
  ierr = MatSolverPackageRegister_Elemental();CHKERRQ(ierr);
#endif
#if defined(PETSC_HAVE_MATLAB_ENGINE)
  ierr = MatSolverPackageRegister_Matlab();CHKERRQ(ierr);
#endif
#if defined(PETSC_HAVE_PETSC_HAVE_ESSL) && !defined(PETSC_USE_COMPLEX) && !defined(PETSC_USE_REAL_SINGLE) && !defined(PETSC_USE_REAL___FLOAT128)
  ierr = MatSolverPackageRegister_Essl();CHKERRQ(ierr);
#endif
#if defined(PETSC_HAVE_SUPERLU)
  ierr = MatSolverPackageRegister_SuperLU();CHKERRQ(ierr);
#endif
#if defined(PETSC_HAVE_PASTIX)
  ierr = MatSolverPackageRegister_Pastix();CHKERRQ(ierr);
#endif
#if defined(PETSC_HAVE_SUPERLU_DIST)
  ierr = MatSolverPackageRegister_SuperLU_DIST();CHKERRQ(ierr);
#endif
#if defined(PETSC_HAVE_CLIQUE)
  ierr = MatSolverPackageRegister_Clique();CHKERRQ(ierr);
#endif
#if defined(PETSC_HAVE_MKL_PARDISO)
  ierr = MatSolverPackageRegister_MKL_Pardiso();CHKERRQ(ierr);
#endif
#if defined(PETSC_HAVE_MKL_CPARDISO)
  ierr = MatSolverPackageRegister_MKL_CPardiso();CHKERRQ(ierr);
#endif
#if defined(PETSC_HAVE_SUITESPARSE)
  ierr = MatSolverPackageRegister_SuiteSparse();CHKERRQ(ierr);
#endif
#if defined(PETSC_HAVE_LUSOL)
  ierr = MatSolverPackageRegister_Lusol();CHKERRQ(ierr);
#endif
#if defined(PETSC_HAVE_CLIQUE)
  ierr = MatSolverPackageRegister_Clique();CHKERRQ(ierr);
#endif

  /* Register the PETSc built in factorization based solvers */
  ierr = MatSolverPackageRegister(MATSOLVERBAS,   MATSEQAIJ,        MAT_FACTOR_ICC,MatGetFactor_seqaij_bas);CHKERRQ(ierr);

  ierr = MatSolverPackageRegister(MATSOLVERPETSC, MATSEQAIJ,        MAT_FACTOR_LU,MatGetFactor_seqaij_petsc);CHKERRQ(ierr);
  ierr = MatSolverPackageRegister(MATSOLVERPETSC, MATSEQAIJ,        MAT_FACTOR_CHOLESKY,MatGetFactor_seqaij_petsc);CHKERRQ(ierr);
  ierr = MatSolverPackageRegister(MATSOLVERPETSC, MATSEQAIJ,        MAT_FACTOR_ILU,MatGetFactor_seqaij_petsc);CHKERRQ(ierr);
  ierr = MatSolverPackageRegister(MATSOLVERPETSC, MATSEQAIJ,        MAT_FACTOR_ICC,MatGetFactor_seqaij_petsc);CHKERRQ(ierr);

  ierr = MatSolverPackageRegister(MATSOLVERPETSC, MATSEQAIJPERM,    MAT_FACTOR_LU,MatGetFactor_seqaij_petsc);CHKERRQ(ierr);
  ierr = MatSolverPackageRegister(MATSOLVERPETSC, MATSEQAIJPERM,    MAT_FACTOR_CHOLESKY,MatGetFactor_seqaij_petsc);CHKERRQ(ierr);
  ierr = MatSolverPackageRegister(MATSOLVERPETSC, MATSEQAIJPERM,    MAT_FACTOR_ILU,MatGetFactor_seqaij_petsc);CHKERRQ(ierr);
  ierr = MatSolverPackageRegister(MATSOLVERPETSC, MATSEQAIJPERM,    MAT_FACTOR_ICC,MatGetFactor_seqaij_petsc);CHKERRQ(ierr);

  ierr = MatSolverPackageRegister(MATSOLVERPETSC, MATSEQAIJCRL,     MAT_FACTOR_LU,MatGetFactor_seqaij_petsc);CHKERRQ(ierr);
  ierr = MatSolverPackageRegister(MATSOLVERPETSC, MATSEQAIJCRL,     MAT_FACTOR_CHOLESKY,MatGetFactor_seqaij_petsc);CHKERRQ(ierr);
  ierr = MatSolverPackageRegister(MATSOLVERPETSC, MATSEQAIJCRL,     MAT_FACTOR_ILU,MatGetFactor_seqaij_petsc);CHKERRQ(ierr);
  ierr = MatSolverPackageRegister(MATSOLVERPETSC, MATSEQAIJCRL,     MAT_FACTOR_ICC,MatGetFactor_seqaij_petsc);CHKERRQ(ierr);

  ierr = MatSolverPackageRegister(MATSOLVERPETSC, MATSEQBAIJ,       MAT_FACTOR_LU,MatGetFactor_seqbaij_petsc);CHKERRQ(ierr);
  ierr = MatSolverPackageRegister(MATSOLVERPETSC, MATSEQBAIJ,       MAT_FACTOR_CHOLESKY,MatGetFactor_seqbaij_petsc);CHKERRQ(ierr);
  ierr = MatSolverPackageRegister(MATSOLVERPETSC, MATSEQBAIJ,       MAT_FACTOR_ILU,MatGetFactor_seqbaij_petsc);CHKERRQ(ierr);
  ierr = MatSolverPackageRegister(MATSOLVERPETSC, MATSEQBAIJ,       MAT_FACTOR_ICC,MatGetFactor_seqbaij_petsc);CHKERRQ(ierr);

  ierr = MatSolverPackageRegister(MATSOLVERBSTRM, MATSEQBAIJ,       MAT_FACTOR_LU,MatGetFactor_seqbaij_bstrm);CHKERRQ(ierr);
  ierr = MatSolverPackageRegister(MATSOLVERBSTRM, MATSEQBAIJ,       MAT_FACTOR_ILU,MatGetFactor_seqbaij_bstrm);CHKERRQ(ierr);

  ierr = MatSolverPackageRegister(MATSOLVERSBSTRM,MATSEQSBAIJ,      MAT_FACTOR_CHOLESKY,MatGetFactor_seqsbaij_sbstrm);CHKERRQ(ierr);
  ierr = MatSolverPackageRegister(MATSOLVERSBSTRM,MATSEQSBAIJ,      MAT_FACTOR_ICC,MatGetFactor_seqsbaij_sbstrm);CHKERRQ(ierr);

  ierr = MatSolverPackageRegister(MATSOLVERPETSC, MATSEQSBAIJ,      MAT_FACTOR_CHOLESKY,MatGetFactor_seqsbaij_petsc);CHKERRQ(ierr);
  ierr = MatSolverPackageRegister(MATSOLVERPETSC, MATSEQSBAIJ,      MAT_FACTOR_ICC,MatGetFactor_seqsbaij_petsc);CHKERRQ(ierr);

  ierr = MatSolverPackageRegister(MATSOLVERPETSC, MATSEQDENSE,      MAT_FACTOR_LU,MatGetFactor_seqdense_petsc);CHKERRQ(ierr);
  ierr = MatSolverPackageRegister(MATSOLVERPETSC, MATSEQDENSE,      MAT_FACTOR_CHOLESKY,MatGetFactor_seqdense_petsc);CHKERRQ(ierr);

  ierr = PetscRegisterFinalize(MatFinalizePackage);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#if defined(PETSC_HAVE_DYNAMIC_LIBRARIES)
#undef __FUNCT__
#define __FUNCT__ "PetscDLLibraryRegister_petscmat"
/*
  PetscDLLibraryRegister - This function is called when the dynamic library it is in is opened.

  This one registers all the matrix methods that are in the basic PETSc Matrix library.

 */
PETSC_EXTERN PetscErrorCode PetscDLLibraryRegister_petscmat(void)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = MatInitializePackage();CHKERRQ(ierr);
  PetscFunctionReturn(0);
}


#endif /* PETSC_HAVE_DYNAMIC_LIBRARIES */
