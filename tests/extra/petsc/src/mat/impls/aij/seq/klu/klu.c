
/*
   Provides an interface to the KLUv1.2 sparse solver

   When build with PETSC_USE_64BIT_INDICES this will use SuiteSparse_long as the
   integer type in KLU, otherwise it will use int. This means
   all integers in this file are simply declared as PetscInt. Also it means
   that KLU SuiteSparse_long version MUST be built with 64 bit integers when used.

*/
#include <../src/mat/impls/aij/seq/aij.h>

#if defined(PETSC_USE_64BIT_INDICES)
#define klu_K_defaults                klu_l_defaults
#define klu_K_analyze                 klu_l_analyze
#define klu_K_analyze_given           klu_l_analyze_given
#define klu_K_free_symbolic           klu_l_free_symbolic
#define klu_K_free_numeric            klu_l_free_numeric
#define klu_K_common                  klu_l_common
#define klu_K_symbolic                klu_l_symbolic
#define klu_K_numeric                 klu_l_numeric
#if defined(PETSC_USE_COMPLEX)
#define klu_K_factor                  klu_zl_factor
#define klu_K_solve                   klu_zl_solve
#define klu_K_tsolve                  klu_zl_tsolve
#define klu_K_refactor                klu_zl_refactor
#define klu_K_sort                    klu_zl_sort
#define klu_K_flops                   klu_zl_flops
#define klu_K_rgrowth                 klu_zl_rgrowth
#define klu_K_condest                 klu_zl_condest
#define klu_K_rcond                   klu_zl_rcond
#define klu_K_scale                   klu_zl_scale
#else
#define klu_K_factor                  klu_l_factor
#define klu_K_solve                   klu_l_solve
#define klu_K_tsolve                  klu_l_tsolve
#define klu_K_refactor                klu_l_refactor
#define klu_K_sort                    klu_l_sort
#define klu_K_flops                   klu_l_flops
#define klu_K_rgrowth                 klu_l_rgrowth
#define klu_K_condest                 klu_l_condest
#define klu_K_rcond                   klu_l_rcond
#define klu_K_scale                   klu_l_scale
#endif
#else
#define klu_K_defaults                klu_defaults
#define klu_K_analyze                 klu_analyze
#define klu_K_analyze_given           klu_analyze_given
#define klu_K_free_symbolic           klu_free_symbolic
#define klu_K_free_numeric            klu_free_numeric
#define klu_K_common                  klu_common
#define klu_K_symbolic                klu_symbolic
#define klu_K_numeric                 klu_numeric
#if defined(PETSC_USE_COMPLEX)
#define klu_K_factor                  klu_z_factor
#define klu_K_solve                   klu_z_solve
#define klu_K_tsolve                  klu_z_tsolve
#define klu_K_refactor                klu_z_refactor
#define klu_K_sort                    klu_z_sort
#define klu_K_flops                   klu_z_flops
#define klu_K_rgrowth                 klu_z_rgrowth
#define klu_K_condest                 klu_z_condest
#define klu_K_rcond                   klu_z_rcond
#define klu_K_scale                   klu_z_scale
#else
#define klu_K_factor                  klu_factor
#define klu_K_solve                   klu_solve
#define klu_K_tsolve                  klu_tsolve
#define klu_K_refactor                klu_refactor
#define klu_K_sort                    klu_sort
#define klu_K_flops                   klu_flops
#define klu_K_rgrowth                 klu_rgrowth
#define klu_K_condest                 klu_condest
#define klu_K_rcond                   klu_rcond
#define klu_K_scale                   klu_scale
#endif
#endif


#define SuiteSparse_long long long
#define SuiteSparse_long_max LONG_LONG_MAX
#define SuiteSparse_long_id "%lld"

EXTERN_C_BEGIN
#include <klu.h>
EXTERN_C_END

static const char *KluOrderingTypes[] = {"AMD","COLAMD","PETSC"};
static const char *scale[] ={"NONE","SUM","MAX"};

typedef struct {
  klu_K_common   Common;
  klu_K_symbolic *Symbolic;
  klu_K_numeric  *Numeric;
  PetscInt     *perm_c,*perm_r;
  MatStructure flg;
  PetscBool    PetscMatOrdering;

  /* Flag to clean up KLU objects during Destroy */
  PetscBool CleanUpKLU;
} Mat_KLU;

#undef __FUNCT__
#define __FUNCT__ "MatDestroy_KLU"
static PetscErrorCode MatDestroy_KLU(Mat A)
{
  PetscErrorCode ierr;
  Mat_KLU    *lu=(Mat_KLU*)A->spptr;

  PetscFunctionBegin;
  if (lu && lu->CleanUpKLU) {
    klu_K_free_symbolic(&lu->Symbolic,&lu->Common);
    klu_K_free_numeric(&lu->Numeric,&lu->Common);
    ierr = PetscFree2(lu->perm_r,lu->perm_c);CHKERRQ(ierr);
  }
  ierr = PetscFree(A->spptr);CHKERRQ(ierr);
  ierr = MatDestroy_SeqAIJ(A);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatSolveTranspose_KLU"
static PetscErrorCode MatSolveTranspose_KLU(Mat A,Vec b,Vec x)
{
  Mat_KLU       *lu = (Mat_KLU*)A->spptr;
  PetscScalar    *xa;
  PetscErrorCode ierr;
  PetscInt       status;

  PetscFunctionBegin;
  /* KLU uses a column major format, solve Ax = b by klu_*_solve */
  /* ----------------------------------*/
  ierr = VecCopy(b,x); /* klu_solve stores the solution in rhs */
  ierr = VecGetArray(x,&xa);
  status = klu_K_solve(lu->Symbolic,lu->Numeric,A->rmap->n,1,(PetscReal*)xa,&lu->Common);
  if (status != 1) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_LIB,"KLU Solve failed");
  ierr = VecRestoreArray(x,&xa);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatSolve_KLU"
static PetscErrorCode MatSolve_KLU(Mat A,Vec b,Vec x)
{
  Mat_KLU       *lu = (Mat_KLU*)A->spptr;
  PetscScalar    *xa;
  PetscErrorCode ierr;
  PetscInt       status;

  PetscFunctionBegin;
  /* KLU uses a column major format, solve A^Tx = b by klu_*_tsolve */
  /* ----------------------------------*/
  ierr = VecCopy(b,x); /* klu_solve stores the solution in rhs */
  ierr = VecGetArray(x,&xa);
#if defined(PETSC_USE_COMPLEX)
  PetscInt conj_solve=1;
  status = klu_K_tsolve(lu->Symbolic,lu->Numeric,A->rmap->n,1,(PetscReal*)xa,conj_solve,&lu->Common); /* conjugate solve */
#else
  status = klu_K_tsolve(lu->Symbolic,lu->Numeric,A->rmap->n,1,xa,&lu->Common);
#endif  
  if (status != 1) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_LIB,"KLU Solve failed");
  ierr = VecRestoreArray(x,&xa);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatLUFactorNumeric_KLU"
static PetscErrorCode MatLUFactorNumeric_KLU(Mat F,Mat A,const MatFactorInfo *info)
{
  Mat_KLU        *lu = (Mat_KLU*)(F)->spptr;
  Mat_SeqAIJ     *a  = (Mat_SeqAIJ*)A->data;
  PetscInt       *ai = a->i,*aj=a->j;
  PetscScalar    *av = a->a;

  PetscFunctionBegin;
  /* numeric factorization of A' */
  /* ----------------------------*/

  if (lu->flg == SAME_NONZERO_PATTERN && lu->Numeric) {
    klu_K_free_numeric(&lu->Numeric,&lu->Common);
  }
  lu->Numeric = klu_K_factor(ai,aj,(PetscReal*)av,lu->Symbolic,&lu->Common);
  if(!lu->Numeric) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_LIB,"KLU Numeric factorization failed");

  lu->flg                = SAME_NONZERO_PATTERN;
  lu->CleanUpKLU         = PETSC_TRUE;
  F->ops->solve          = MatSolve_KLU;
  F->ops->solvetranspose = MatSolveTranspose_KLU;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatLUFactorSymbolic_KLU"
static PetscErrorCode MatLUFactorSymbolic_KLU(Mat F,Mat A,IS r,IS c,const MatFactorInfo *info)
{
  Mat_SeqAIJ     *a  = (Mat_SeqAIJ*)A->data;
  Mat_KLU       *lu = (Mat_KLU*)(F->spptr);
  PetscErrorCode ierr;
  PetscInt       i,*ai = a->i,*aj = a->j,m=A->rmap->n,n=A->cmap->n;
  const PetscInt *ra,*ca;

  PetscFunctionBegin;
  if (lu->PetscMatOrdering) {
    ierr = ISGetIndices(r,&ra);CHKERRQ(ierr);
    ierr = ISGetIndices(c,&ca);CHKERRQ(ierr);
    ierr = PetscMalloc2(m,&lu->perm_r,n,&lu->perm_c);CHKERRQ(ierr);
    /* we cannot simply memcpy on 64 bit archs */
    for (i = 0; i < m; i++) lu->perm_r[i] = ra[i];
    for (i = 0; i < n; i++) lu->perm_c[i] = ca[i];
    ierr = ISRestoreIndices(r,&ra);CHKERRQ(ierr);
    ierr = ISRestoreIndices(c,&ca);CHKERRQ(ierr);
  }

  /* symbolic factorization of A' */
  /* ---------------------------------------------------------------------- */
  if (lu->PetscMatOrdering) { /* use Petsc ordering */
    lu->Symbolic = klu_K_analyze_given(n,ai,aj,lu->perm_c,lu->perm_r,&lu->Common);
  } else { /* use klu internal ordering */
    lu->Symbolic = klu_K_analyze(n,ai,aj,&lu->Common);
  }
  if (!lu->Symbolic) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_LIB,"KLU Symbolic Factorization failed");

  lu->flg                   = DIFFERENT_NONZERO_PATTERN;
  lu->CleanUpKLU            = PETSC_TRUE;
  (F)->ops->lufactornumeric = MatLUFactorNumeric_KLU;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatFactorInfo_KLU"
static PetscErrorCode MatFactorInfo_KLU(Mat A,PetscViewer viewer)
{
  Mat_KLU       *lu= (Mat_KLU*)A->spptr;
  klu_K_numeric *Numeric=(klu_K_numeric*)lu->Numeric;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  /* check if matrix is KLU type */
  if (A->ops->solve != MatSolve_KLU) PetscFunctionReturn(0);

  ierr = PetscViewerASCIIPrintf(viewer,"KLU stats:\n");CHKERRQ(ierr);
  ierr = PetscViewerASCIIPrintf(viewer,"  Number of diagonal blocks: %d\n",Numeric->nblocks);
  ierr = PetscViewerASCIIPrintf(viewer,"  Total nonzeros=%d\n",Numeric->lnz+Numeric->unz);CHKERRQ(ierr);

  ierr = PetscViewerASCIIPrintf(viewer,"KLU runtime parameters:\n");CHKERRQ(ierr);

  /* Control parameters used by numeric factorization */
  ierr = PetscViewerASCIIPrintf(viewer,"  Partial pivoting tolerance: %g\n",lu->Common.tol);CHKERRQ(ierr);
  /* BTF preordering */
  ierr = PetscViewerASCIIPrintf(viewer,"  BTF preordering enabled: %d\n",lu->Common.btf);CHKERRQ(ierr);
  /* mat ordering */
  if (!lu->PetscMatOrdering) {
    ierr = PetscViewerASCIIPrintf(viewer,"  Ordering: %s (not using the PETSc ordering)\n",KluOrderingTypes[(int)lu->Common.ordering]);CHKERRQ(ierr);
  } else {
    ierr = PetscViewerASCIIPrintf(viewer,"  Using PETSc ordering\n");CHKERRQ(ierr);
  }
  /* matrix row scaling */
  ierr = PetscViewerASCIIPrintf(viewer, "  Matrix row scaling: %s\n",scale[(int)lu->Common.scale]);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatView_KLU"
static PetscErrorCode MatView_KLU(Mat A,PetscViewer viewer)
{
  PetscErrorCode    ierr;
  PetscBool         iascii;
  PetscViewerFormat format;

  PetscFunctionBegin;
  ierr = MatView_SeqAIJ(A,viewer);CHKERRQ(ierr);

  ierr = PetscObjectTypeCompare((PetscObject)viewer,PETSCVIEWERASCII,&iascii);CHKERRQ(ierr);
  if (iascii) {
    ierr = PetscViewerGetFormat(viewer,&format);CHKERRQ(ierr);
    if (format == PETSC_VIEWER_ASCII_INFO) {
      ierr = MatFactorInfo_KLU(A,viewer);CHKERRQ(ierr);
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatFactorGetSolverPackage_seqaij_klu"
PetscErrorCode MatFactorGetSolverPackage_seqaij_klu(Mat A,const MatSolverPackage *type)
{
  PetscFunctionBegin;
  *type = MATSOLVERKLU;
  PetscFunctionReturn(0);
}


/*MC
  MATSOLVERKLU = "klu" - A matrix type providing direct solvers (LU) for sequential matrices
  via the external package KLU.

  ./configure --download-suitesparse to install PETSc to use KLU

  Use -pc_type lu -pc_factor_mat_solver_package klu to us this direct solver

  Consult KLU documentation for more information on the options database keys below.

  Options Database Keys:
+ -mat_klu_pivot_tol <0.001>                  - Partial pivoting tolerance
. -mat_klu_use_btf <1>                        - Use BTF preordering
. -mat_klu_ordering <AMD>                     - KLU reordering scheme to reduce fill-in (choose one of) AMD COLAMD PETSC
- -mat_klu_row_scale <NONE>                   - Matrix row scaling (choose one of) NONE SUM MAX 

   Note: KLU is part of SuiteSparse http://faculty.cse.tamu.edu/davis/suitesparse.html

   Level: beginner

.seealso: PCLU, MATSOLVERUMFPACK, MATSOLVERCHOLMOD, PCFactorSetMatSolverPackage(), MatSolverPackage
M*/

#undef __FUNCT__
#define __FUNCT__ "MatGetFactor_seqaij_klu"
PETSC_EXTERN PetscErrorCode MatGetFactor_seqaij_klu(Mat A,MatFactorType ftype,Mat *F)
{
  Mat            B;
  Mat_KLU       *lu;
  PetscErrorCode ierr;
  PetscInt       m=A->rmap->n,n=A->cmap->n,idx,status;
  PetscBool      flg;

  PetscFunctionBegin;
  /* Create the factorization matrix F */
  ierr = MatCreate(PetscObjectComm((PetscObject)A),&B);CHKERRQ(ierr);
  ierr = MatSetSizes(B,PETSC_DECIDE,PETSC_DECIDE,m,n);CHKERRQ(ierr);
  ierr = MatSetType(B,((PetscObject)A)->type_name);CHKERRQ(ierr);
  ierr = MatSeqAIJSetPreallocation(B,0,NULL);CHKERRQ(ierr);
  ierr = PetscNewLog(B,&lu);CHKERRQ(ierr);

  B->spptr                 = lu;
  B->ops->lufactorsymbolic = MatLUFactorSymbolic_KLU;
  B->ops->destroy          = MatDestroy_KLU;
  B->ops->view             = MatView_KLU;

  ierr = PetscObjectComposeFunction((PetscObject)B,"MatFactorGetSolverPackage_C",MatFactorGetSolverPackage_seqaij_klu);CHKERRQ(ierr);

  B->factortype   = MAT_FACTOR_LU;
  B->assembled    = PETSC_TRUE;           /* required by -ksp_view */
  B->preallocated = PETSC_TRUE;

  /* initializations */
  /* ------------------------------------------------*/
  /* get the default control parameters */
  status = klu_K_defaults(&lu->Common);
  if(status <= 0) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_LIB,"KLU Initialization failed");

  lu->Common.scale = 0; /* No row scaling */

  ierr = PetscOptionsBegin(PetscObjectComm((PetscObject)A),((PetscObject)A)->prefix,"KLU Options","Mat");CHKERRQ(ierr);
  /* Partial pivoting tolerance */
  ierr = PetscOptionsReal("-mat_klu_pivot_tol","Partial pivoting tolerance","None",lu->Common.tol,&lu->Common.tol,NULL);CHKERRQ(ierr);
  /* BTF pre-ordering */
  ierr = PetscOptionsInt("-mat_klu_use_btf","Enable BTF preordering","None",lu->Common.btf,&lu->Common.btf,NULL);CHKERRQ(ierr);
  /* Matrix reordering */
  ierr = PetscOptionsEList("-mat_klu_ordering","Internal ordering method","None",KluOrderingTypes,sizeof(KluOrderingTypes)/sizeof(KluOrderingTypes[0]),KluOrderingTypes[0],&idx,&flg);CHKERRQ(ierr);
  if (flg) {
    if ((int)idx == 2) lu->PetscMatOrdering = PETSC_TRUE;   /* use Petsc mat ordering (note: size is for the transpose, and PETSc r = Klu perm_c) */
    else lu->Common.ordering = (int)idx;
  }
  /* Matrix row scaling */
  ierr = PetscOptionsEList("-mat_klu_row_scale","Matrix row scaling","None",scale,3,scale[0],&idx,&flg);CHKERRQ(ierr);
  PetscOptionsEnd();
  *F = B;
  PetscFunctionReturn(0);
}
