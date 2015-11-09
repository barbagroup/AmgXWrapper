
/*
        Provides an interface to the IBM RS6000 Essl sparse solver

*/
#include <../src/mat/impls/aij/seq/aij.h>

/* #include <essl.h> This doesn't work!  */

PETSC_EXTERN void dgss(int*,int*,double*,int*,int*,int*,double*,double*,int*);
PETSC_EXTERN void dgsf(int*,int*,int*,double*,int*,int*,int*,int*,double*,double*,double*,int*);

typedef struct {
  int         n,nz;
  PetscScalar *a;
  int         *ia;
  int         *ja;
  int         lna;
  int         iparm[5];
  PetscReal   rparm[5];
  PetscReal   oparm[5];
  PetscScalar *aux;
  int         naux;

  PetscBool CleanUpESSL;
} Mat_Essl;

#undef __FUNCT__
#define __FUNCT__ "MatDestroy_Essl"
PetscErrorCode MatDestroy_Essl(Mat A)
{
  PetscErrorCode ierr;
  Mat_Essl       *essl=(Mat_Essl*)A->spptr;

  PetscFunctionBegin;
  if (essl && essl->CleanUpESSL) {
    ierr = PetscFree4(essl->a,essl->aux,essl->ia,essl->ja);CHKERRQ(ierr);
  }
  ierr = PetscFree(A->spptr);CHKERRQ(ierr);
  ierr = MatDestroy_SeqAIJ(A);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatSolve_Essl"
PetscErrorCode MatSolve_Essl(Mat A,Vec b,Vec x)
{
  Mat_Essl       *essl = (Mat_Essl*)A->spptr;
  PetscScalar    *xx;
  PetscErrorCode ierr;
  int            nessl,zero = 0;

  PetscFunctionBegin;
  ierr = PetscBLASIntCast(A->cmap->n,&nessl);CHKERRQ(ierr);
  ierr = VecCopy(b,x);CHKERRQ(ierr);
  ierr = VecGetArray(x,&xx);CHKERRQ(ierr);
  dgss(&zero,&nessl,essl->a,essl->ia,essl->ja,&essl->lna,xx,essl->aux,&essl->naux);
  ierr = VecRestoreArray(x,&xx);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatLUFactorNumeric_Essl"
PetscErrorCode MatLUFactorNumeric_Essl(Mat F,Mat A,const MatFactorInfo *info)
{
  Mat_SeqAIJ     *aa  =(Mat_SeqAIJ*)(A)->data;
  Mat_Essl       *essl=(Mat_Essl*)(F)->spptr;
  PetscErrorCode ierr;
  int            nessl,i,one = 1;

  PetscFunctionBegin;
  ierr = PetscBLASIntCast(A->rmap->n,&nessl);CHKERRQ(ierr);
  /* copy matrix data into silly ESSL data structure (1-based Frotran style) */
  for (i=0; i<A->rmap->n+1; i++) essl->ia[i] = aa->i[i] + 1;
  for (i=0; i<aa->nz; i++) essl->ja[i] = aa->j[i] + 1;

  ierr = PetscMemcpy(essl->a,aa->a,(aa->nz)*sizeof(PetscScalar));CHKERRQ(ierr);

  /* set Essl options */
  essl->iparm[0] = 1;
  essl->iparm[1] = 5;
  essl->iparm[2] = 1;
  essl->iparm[3] = 0;
  essl->rparm[0] = 1.e-12;
  essl->rparm[1] = 1.0;

  ierr = PetscOptionsGetReal(((PetscObject)A)->prefix,"-matessl_lu_threshold",&essl->rparm[1],NULL);CHKERRQ(ierr);

  dgsf(&one,&nessl,&essl->nz,essl->a,essl->ia,essl->ja,&essl->lna,essl->iparm,essl->rparm,essl->oparm,essl->aux,&essl->naux);

  F->ops->solve     = MatSolve_Essl;
  (F)->assembled    = PETSC_TRUE;
  (F)->preallocated = PETSC_TRUE;
  PetscFunctionReturn(0);
}




#undef __FUNCT__
#define __FUNCT__ "MatLUFactorSymbolic_Essl"
PetscErrorCode MatLUFactorSymbolic_Essl(Mat B,Mat A,IS r,IS c,const MatFactorInfo *info)
{
  Mat_SeqAIJ     *a = (Mat_SeqAIJ*)A->data;
  PetscErrorCode ierr;
  Mat_Essl       *essl;
  PetscReal      f = 1.0;

  PetscFunctionBegin;
  essl = (Mat_Essl*)(B->spptr);

  /* allocate the work arrays required by ESSL */
  f    = info->fill;
  ierr = PetscBLASIntCast(a->nz,&essl->nz);CHKERRQ(ierr);
  ierr = PetscBLASIntCast((PetscInt)(a->nz*f),&essl->lna);CHKERRQ(ierr);
  ierr = PetscBLASIntCast(100 + 10*A->rmap->n,&essl->naux);CHKERRQ(ierr);

  /* since malloc is slow on IBM we try a single malloc */
  ierr = PetscMalloc4(essl->lna,&essl->a,essl->naux,&essl->aux,essl->lna,&essl->ia,essl->lna,&essl->ja);CHKERRQ(ierr);

  essl->CleanUpESSL = PETSC_TRUE;

  ierr = PetscLogObjectMemory((PetscObject)B,essl->lna*(2*sizeof(int)+sizeof(PetscScalar)) + essl->naux*sizeof(PetscScalar));CHKERRQ(ierr);

  B->ops->lufactornumeric = MatLUFactorNumeric_Essl;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatFactorGetSolverPackage_essl"
PetscErrorCode MatFactorGetSolverPackage_essl(Mat A,const MatSolverPackage *type)
{
  PetscFunctionBegin;
  *type = MATSOLVERESSL;
  PetscFunctionReturn(0);
}

/*MC
  MATSOLVERESSL - "essl" - Provides direct solvers (LU) for sequential matrices
                              via the external package ESSL.

  If ESSL is installed (see the manual for
  instructions on how to declare the existence of external packages),

  Works with MATSEQAIJ matrices

   Level: beginner

.seealso: PCLU, PCFactorSetMatSolverPackage(), MatSolverPackage
M*/

#undef __FUNCT__
#define __FUNCT__ "MatGetFactor_seqaij_essl"
PETSC_EXTERN PetscErrorCode MatGetFactor_seqaij_essl(Mat A,MatFactorType ftype,Mat *F)
{
  Mat            B;
  PetscErrorCode ierr;
  Mat_Essl       *essl;

  PetscFunctionBegin;
  if (A->cmap->N != A->rmap->N) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_SIZ,"matrix must be square");
  ierr = MatCreate(PetscObjectComm((PetscObject)A),&B);CHKERRQ(ierr);
  ierr = MatSetSizes(B,PETSC_DECIDE,PETSC_DECIDE,A->rmap->n,A->cmap->n);CHKERRQ(ierr);
  ierr = MatSetType(B,((PetscObject)A)->type_name);CHKERRQ(ierr);
  ierr = MatSeqAIJSetPreallocation(B,0,NULL);CHKERRQ(ierr);

  ierr = PetscNewLog(B,&essl);CHKERRQ(ierr);

  B->spptr                 = essl;
  B->ops->lufactorsymbolic = MatLUFactorSymbolic_Essl;

  ierr = PetscObjectComposeFunction((PetscObject)B,"MatFactorGetSolverPackage_C",MatFactorGetSolverPackage_essl);CHKERRQ(ierr);

  B->factortype = MAT_FACTOR_LU;
  *F            = B;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatSolverPackageRegister_Essl"
PETSC_EXTERN PetscErrorCode MatSolverPackageRegister_Essl(void)
{
  PetscErrorCode ierr;
  PetscFunctionBegin;
  ierr = MatSolverPackageRegister(MATSOLVERESSL,MATSEQAIJ,          MAT_FACTOR_LU,MatGetFactor_seqaij_essl);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}
