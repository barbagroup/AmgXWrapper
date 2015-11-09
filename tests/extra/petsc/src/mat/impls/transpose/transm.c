
#include <petsc/private/matimpl.h>          /*I "petscmat.h" I*/

typedef struct {
  Mat A;
} Mat_Transpose;

#undef __FUNCT__
#define __FUNCT__ "MatMult_Transpose"
PetscErrorCode MatMult_Transpose(Mat N,Vec x,Vec y)
{
  Mat_Transpose  *Na = (Mat_Transpose*)N->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = MatMultTranspose(Na->A,x,y);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatMultAdd_Transpose"
PetscErrorCode MatMultAdd_Transpose(Mat N,Vec v1,Vec v2,Vec v3)
{
  Mat_Transpose  *Na = (Mat_Transpose*)N->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = MatMultTransposeAdd(Na->A,v1,v2,v3);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatMultTranspose_Transpose"
PetscErrorCode MatMultTranspose_Transpose(Mat N,Vec x,Vec y)
{
  Mat_Transpose  *Na = (Mat_Transpose*)N->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = MatMult(Na->A,x,y);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatMultTransposeAdd_Transpose"
PetscErrorCode MatMultTransposeAdd_Transpose(Mat N,Vec v1,Vec v2,Vec v3)
{
  Mat_Transpose  *Na = (Mat_Transpose*)N->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = MatMultAdd(Na->A,v1,v2,v3);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatDestroy_Transpose"
PetscErrorCode MatDestroy_Transpose(Mat N)
{
  Mat_Transpose  *Na = (Mat_Transpose*)N->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = MatDestroy(&Na->A);CHKERRQ(ierr);
  ierr = PetscFree(N->data);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatDuplicate_Transpose"
PetscErrorCode MatDuplicate_Transpose(Mat N, MatDuplicateOption op, Mat* m)
{
  Mat_Transpose  *Na = (Mat_Transpose*)N->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (op == MAT_COPY_VALUES) {
    ierr = MatTranspose(Na->A,MAT_INITIAL_MATRIX,m);CHKERRQ(ierr);
  } else if (op == MAT_DO_NOT_COPY_VALUES) {
    ierr = MatDuplicate(Na->A,MAT_DO_NOT_COPY_VALUES,m);CHKERRQ(ierr);
    ierr = MatTranspose(*m,MAT_REUSE_MATRIX,m);CHKERRQ(ierr);
  } else SETERRQ(PetscObjectComm((PetscObject)N),PETSC_ERR_SUP,"MAT_SHARE_NONZERO_PATTERN not supported for this matrix type");
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "MatCreateTranspose"
/*@
      MatCreateTranspose - Creates a new matrix object that behaves like A'

   Collective on Mat

   Input Parameter:
.   A  - the (possibly rectangular) matrix

   Output Parameter:
.   N - the matrix that represents A'

   Level: intermediate

   Notes: The transpose A' is NOT actually formed! Rather the new matrix
          object performs the matrix-vector product by using the MatMultTranspose() on
          the original matrix

.seealso: MatCreateNormal(), MatMult(), MatMultTranspose(), MatCreate()

@*/
PetscErrorCode  MatCreateTranspose(Mat A,Mat *N)
{
  PetscErrorCode ierr;
  PetscInt       m,n;
  Mat_Transpose  *Na;

  PetscFunctionBegin;
  ierr = MatGetLocalSize(A,&m,&n);CHKERRQ(ierr);
  ierr = MatCreate(PetscObjectComm((PetscObject)A),N);CHKERRQ(ierr);
  ierr = MatSetSizes(*N,n,m,PETSC_DECIDE,PETSC_DECIDE);CHKERRQ(ierr);
  ierr = PetscLayoutSetUp((*N)->rmap);CHKERRQ(ierr);
  ierr = PetscLayoutSetUp((*N)->cmap);CHKERRQ(ierr);
  ierr = PetscObjectChangeTypeName((PetscObject)*N,MATTRANSPOSEMAT);CHKERRQ(ierr);

  ierr       = PetscNewLog(*N,&Na);CHKERRQ(ierr);
  (*N)->data = (void*) Na;
  ierr       = PetscObjectReference((PetscObject)A);CHKERRQ(ierr);
  Na->A      = A;

  (*N)->ops->destroy          = MatDestroy_Transpose;
  (*N)->ops->mult             = MatMult_Transpose;
  (*N)->ops->multadd          = MatMultAdd_Transpose;
  (*N)->ops->multtranspose    = MatMultTranspose_Transpose;
  (*N)->ops->multtransposeadd = MatMultTransposeAdd_Transpose;
  (*N)->ops->duplicate        = MatDuplicate_Transpose;
  (*N)->assembled             = PETSC_TRUE;

  ierr = MatSetBlockSizes(*N,PetscAbs(A->cmap->bs),PetscAbs(A->rmap->bs));CHKERRQ(ierr);
  ierr = MatSetUp(*N);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

