
#include <petsc/private/matimpl.h>  /*I   "petscmat.h"  I*/

#undef __FUNCT__
#define __FUNCT__ "MatAXPY"
/*@
   MatAXPY - Computes Y = a*X + Y.

   Logically  Collective on Mat

   Input Parameters:
+  a - the scalar multiplier
.  X - the first matrix
.  Y - the second matrix
-  str - either SAME_NONZERO_PATTERN, DIFFERENT_NONZERO_PATTERN
         or SUBSET_NONZERO_PATTERN (nonzeros of X is a subset of Y's)

   Level: intermediate

.keywords: matrix, add

.seealso: MatAYPX()
 @*/
PetscErrorCode MatAXPY(Mat Y,PetscScalar a,Mat X,MatStructure str)
{
  PetscErrorCode ierr;
  PetscInt       m1,m2,n1,n2;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(X,MAT_CLASSID,3);
  PetscValidHeaderSpecific(Y,MAT_CLASSID,1);
  PetscValidLogicalCollectiveScalar(Y,a,2);
  ierr = MatGetSize(X,&m1,&n1);CHKERRQ(ierr);
  ierr = MatGetSize(Y,&m2,&n2);CHKERRQ(ierr);
  if (m1 != m2 || n1 != n2) SETERRQ4(PETSC_COMM_SELF,PETSC_ERR_ARG_SIZ,"Non conforming matrix add: %D %D %D %D",m1,m2,n1,n2);

  ierr = PetscLogEventBegin(MAT_AXPY,Y,0,0,0);CHKERRQ(ierr);
  if (Y->ops->axpy) {
    ierr = (*Y->ops->axpy)(Y,a,X,str);CHKERRQ(ierr);
  } else {
    ierr = MatAXPY_Basic(Y,a,X,str);CHKERRQ(ierr);
  }
  ierr = PetscLogEventEnd(MAT_AXPY,Y,0,0,0);CHKERRQ(ierr);
#if defined(PETSC_HAVE_CUSP)
  if (Y->valid_GPU_matrix != PETSC_CUSP_UNALLOCATED) {
    Y->valid_GPU_matrix = PETSC_CUSP_CPU;
  }
#endif
#if defined(PETSC_HAVE_VIENNACL)
  if (Y->valid_GPU_matrix != PETSC_VIENNACL_UNALLOCATED) {
    Y->valid_GPU_matrix = PETSC_VIENNACL_CPU;
  }
#endif
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatAXPY_Basic"
PetscErrorCode MatAXPY_Basic(Mat Y,PetscScalar a,Mat X,MatStructure str)
{
  PetscInt          i,start,end,j,ncols,m,n;
  PetscErrorCode    ierr;
  const PetscInt    *row;
  PetscScalar       *val;
  const PetscScalar *vals;

  PetscFunctionBegin;
  ierr = MatGetSize(X,&m,&n);CHKERRQ(ierr);
  ierr = MatGetOwnershipRange(X,&start,&end);CHKERRQ(ierr);
  if (a == 1.0) {
    for (i = start; i < end; i++) {
      ierr = MatGetRow(X,i,&ncols,&row,&vals);CHKERRQ(ierr);
      ierr = MatSetValues(Y,1,&i,ncols,row,vals,ADD_VALUES);CHKERRQ(ierr);
      ierr = MatRestoreRow(X,i,&ncols,&row,&vals);CHKERRQ(ierr);
    }
  } else {
    ierr = PetscMalloc1(n+1,&val);CHKERRQ(ierr);
    for (i=start; i<end; i++) {
      ierr = MatGetRow(X,i,&ncols,&row,&vals);CHKERRQ(ierr);
      for (j=0; j<ncols; j++) {
        val[j] = a*vals[j];
      }
      ierr = MatSetValues(Y,1,&i,ncols,row,val,ADD_VALUES);CHKERRQ(ierr);
      ierr = MatRestoreRow(X,i,&ncols,&row,&vals);CHKERRQ(ierr);
    }
    ierr = PetscFree(val);CHKERRQ(ierr);
  }
  ierr = MatAssemblyBegin(Y,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(Y,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatAXPY_BasicWithPreallocation"
PetscErrorCode MatAXPY_BasicWithPreallocation(Mat B,Mat Y,PetscScalar a,Mat X,MatStructure str)
{
  PetscInt          i,start,end,j,ncols,m,n;
  PetscErrorCode    ierr;
  const PetscInt    *row;
  PetscScalar       *val;
  const PetscScalar *vals;

  PetscFunctionBegin;
  ierr = MatGetSize(X,&m,&n);CHKERRQ(ierr);
  ierr = MatGetOwnershipRange(X,&start,&end);CHKERRQ(ierr);
  if (a == 1.0) {
    for (i = start; i < end; i++) {
      ierr = MatGetRow(Y,i,&ncols,&row,&vals);CHKERRQ(ierr);
      ierr = MatSetValues(B,1,&i,ncols,row,vals,ADD_VALUES);CHKERRQ(ierr);
      ierr = MatRestoreRow(Y,i,&ncols,&row,&vals);CHKERRQ(ierr);

      ierr = MatGetRow(X,i,&ncols,&row,&vals);CHKERRQ(ierr);
      ierr = MatSetValues(B,1,&i,ncols,row,vals,ADD_VALUES);CHKERRQ(ierr);
      ierr = MatRestoreRow(X,i,&ncols,&row,&vals);CHKERRQ(ierr);
    }
  } else {
    ierr = PetscMalloc1(n+1,&val);CHKERRQ(ierr);
    for (i=start; i<end; i++) {
      ierr = MatGetRow(Y,i,&ncols,&row,&vals);CHKERRQ(ierr);
      ierr = MatSetValues(B,1,&i,ncols,row,vals,ADD_VALUES);CHKERRQ(ierr);
      ierr = MatRestoreRow(Y,i,&ncols,&row,&vals);CHKERRQ(ierr);

      ierr = MatGetRow(X,i,&ncols,&row,&vals);CHKERRQ(ierr);
      for (j=0; j<ncols; j++) {
        val[j] = a*vals[j];
      }
      ierr = MatSetValues(B,1,&i,ncols,row,val,ADD_VALUES);CHKERRQ(ierr);
      ierr = MatRestoreRow(X,i,&ncols,&row,&vals);CHKERRQ(ierr);
    }
    ierr = PetscFree(val);CHKERRQ(ierr);
  }
  ierr = MatAssemblyBegin(B,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(B,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatShift"
/*@
   MatShift - Computes Y =  Y + a I, where a is a PetscScalar and I is the identity matrix.

   Neighbor-wise Collective on Mat

   Input Parameters:
+  Y - the matrices
-  a - the PetscScalar

   Level: intermediate

   Notes: If the matrix Y is missing some diagonal entries this routine can be very slow. To make it fast one should initially
   fill the matrix so that all diagonal entries have a value (with a value of zero for those locations that would not have an
   entry).

   Developers Note: If the local "diagonal part" of the matrix Y has no entries then the local diagonal part is
    preallocated with 1 nonzero per row for the to be added values. This allows for fast shifting of an empty matrix.

.keywords: matrix, add, shift

.seealso: MatDiagonalSet()
 @*/
PetscErrorCode  MatShift(Mat Y,PetscScalar a)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(Y,MAT_CLASSID,1);
  if (!Y->assembled) SETERRQ(PetscObjectComm((PetscObject)Y),PETSC_ERR_ARG_WRONGSTATE,"Not for unassembled matrix");
  if (Y->factortype) SETERRQ(PetscObjectComm((PetscObject)Y),PETSC_ERR_ARG_WRONGSTATE,"Not for factored matrix");
  MatCheckPreallocated(Y,1);

  ierr = (*Y->ops->shift)(Y,a);CHKERRQ(ierr);

#if defined(PETSC_HAVE_CUSP)
  if (Y->valid_GPU_matrix != PETSC_CUSP_UNALLOCATED) {
    Y->valid_GPU_matrix = PETSC_CUSP_CPU;
  }
#endif
#if defined(PETSC_HAVE_VIENNACL)
  if (Y->valid_GPU_matrix != PETSC_VIENNACL_UNALLOCATED) {
    Y->valid_GPU_matrix = PETSC_VIENNACL_CPU;
  }
#endif
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatDiagonalSet_Default"
PetscErrorCode  MatDiagonalSet_Default(Mat Y,Vec D,InsertMode is)
{
  PetscErrorCode ierr;
  PetscInt       i,start,end;
  PetscScalar    *v;

  PetscFunctionBegin;
  ierr = MatGetOwnershipRange(Y,&start,&end);CHKERRQ(ierr);
  ierr = VecGetArray(D,&v);CHKERRQ(ierr);
  for (i=start; i<end; i++) {
    ierr = MatSetValues(Y,1,&i,1,&i,v+i-start,is);CHKERRQ(ierr);
  }
  ierr = VecRestoreArray(D,&v);CHKERRQ(ierr);
  ierr = MatAssemblyBegin(Y,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(Y,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatDiagonalSet"
/*@
   MatDiagonalSet - Computes Y = Y + D, where D is a diagonal matrix
   that is represented as a vector. Or Y[i,i] = D[i] if InsertMode is
   INSERT_VALUES.

   Input Parameters:
+  Y - the input matrix
.  D - the diagonal matrix, represented as a vector
-  i - INSERT_VALUES or ADD_VALUES

   Neighbor-wise Collective on Mat and Vec

   Notes: If the matrix Y is missing some diagonal entries this routine can be very slow. To make it fast one should initially
   fill the matrix so that all diagonal entries have a value (with a value of zero for those locations that would not have an
   entry).

   Level: intermediate

.keywords: matrix, add, shift, diagonal

.seealso: MatShift()
@*/
PetscErrorCode  MatDiagonalSet(Mat Y,Vec D,InsertMode is)
{
  PetscErrorCode ierr;
  PetscInt       matlocal,veclocal;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(Y,MAT_CLASSID,1);
  PetscValidHeaderSpecific(D,VEC_CLASSID,2);
  ierr = MatGetLocalSize(Y,&matlocal,NULL);CHKERRQ(ierr);
  ierr = VecGetLocalSize(D,&veclocal);CHKERRQ(ierr);
  if (matlocal != veclocal) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_INCOMP,"Number local rows of matrix %D does not match that of vector for diagonal %D",matlocal,veclocal);
  if (Y->ops->diagonalset) {
    ierr = (*Y->ops->diagonalset)(Y,D,is);CHKERRQ(ierr);
  } else {
    ierr = MatDiagonalSet_Default(Y,D,is);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatAYPX"
/*@
   MatAYPX - Computes Y = a*Y + X.

   Logically on Mat

   Input Parameters:
+  a - the PetscScalar multiplier
.  Y - the first matrix
.  X - the second matrix
-  str - either SAME_NONZERO_PATTERN, DIFFERENT_NONZERO_PATTERN or SUBSET_NONZERO_PATTERN

   Level: intermediate

.keywords: matrix, add

.seealso: MatAXPY()
 @*/
PetscErrorCode  MatAYPX(Mat Y,PetscScalar a,Mat X,MatStructure str)
{
  PetscScalar    one = 1.0;
  PetscErrorCode ierr;
  PetscInt       mX,mY,nX,nY;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(X,MAT_CLASSID,3);
  PetscValidHeaderSpecific(Y,MAT_CLASSID,1);
  PetscValidLogicalCollectiveScalar(Y,a,2);
  ierr = MatGetSize(X,&mX,&nX);CHKERRQ(ierr);
  ierr = MatGetSize(X,&mY,&nY);CHKERRQ(ierr);
  if (mX != mY || nX != nY) SETERRQ4(PETSC_COMM_SELF,PETSC_ERR_ARG_SIZ,"Non conforming matrices: %D %D first %D %D second",mX,mY,nX,nY);

  ierr = MatScale(Y,a);CHKERRQ(ierr);
  ierr = MatAXPY(Y,one,X,str);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatComputeExplicitOperator"
/*@
    MatComputeExplicitOperator - Computes the explicit matrix

    Collective on Mat

    Input Parameter:
.   inmat - the matrix

    Output Parameter:
.   mat - the explict preconditioned operator

    Notes:
    This computation is done by applying the operators to columns of the
    identity matrix.

    Currently, this routine uses a dense matrix format when 1 processor
    is used and a sparse format otherwise.  This routine is costly in general,
    and is recommended for use only with relatively small systems.

    Level: advanced

.keywords: Mat, compute, explicit, operator
@*/
PetscErrorCode  MatComputeExplicitOperator(Mat inmat,Mat *mat)
{
  Vec            in,out;
  PetscErrorCode ierr;
  PetscInt       i,m,n,M,N,*rows,start,end;
  MPI_Comm       comm;
  PetscScalar    *array,zero = 0.0,one = 1.0;
  PetscMPIInt    size;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(inmat,MAT_CLASSID,1);
  PetscValidPointer(mat,2);

  ierr = PetscObjectGetComm((PetscObject)inmat,&comm);CHKERRQ(ierr);
  ierr = MPI_Comm_size(comm,&size);CHKERRQ(ierr);

  ierr = MatGetLocalSize(inmat,&m,&n);CHKERRQ(ierr);
  ierr = MatGetSize(inmat,&M,&N);CHKERRQ(ierr);
  ierr = MatCreateVecs(inmat,&in,&out);CHKERRQ(ierr);
  ierr = VecSetOption(in,VEC_IGNORE_OFF_PROC_ENTRIES,PETSC_TRUE);CHKERRQ(ierr);
  ierr = VecGetOwnershipRange(out,&start,&end);CHKERRQ(ierr);
  ierr = PetscMalloc1(m,&rows);CHKERRQ(ierr);
  for (i=0; i<m; i++) rows[i] = start + i;

  ierr = MatCreate(comm,mat);CHKERRQ(ierr);
  ierr = MatSetSizes(*mat,m,n,M,N);CHKERRQ(ierr);
  if (size == 1) {
    ierr = MatSetType(*mat,MATSEQDENSE);CHKERRQ(ierr);
    ierr = MatSeqDenseSetPreallocation(*mat,NULL);CHKERRQ(ierr);
  } else {
    ierr = MatSetType(*mat,MATMPIAIJ);CHKERRQ(ierr);
    ierr = MatMPIAIJSetPreallocation(*mat,n,NULL,N-n,NULL);CHKERRQ(ierr);
  }

  for (i=0; i<N; i++) {

    ierr = VecSet(in,zero);CHKERRQ(ierr);
    ierr = VecSetValues(in,1,&i,&one,INSERT_VALUES);CHKERRQ(ierr);
    ierr = VecAssemblyBegin(in);CHKERRQ(ierr);
    ierr = VecAssemblyEnd(in);CHKERRQ(ierr);

    ierr = MatMult(inmat,in,out);CHKERRQ(ierr);

    ierr = VecGetArray(out,&array);CHKERRQ(ierr);
    ierr = MatSetValues(*mat,m,rows,1,&i,array,INSERT_VALUES);CHKERRQ(ierr);
    ierr = VecRestoreArray(out,&array);CHKERRQ(ierr);

  }
  ierr = PetscFree(rows);CHKERRQ(ierr);
  ierr = VecDestroy(&out);CHKERRQ(ierr);
  ierr = VecDestroy(&in);CHKERRQ(ierr);
  ierr = MatAssemblyBegin(*mat,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(*mat,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatChop"
/*@
  MatChop - Set all values in the matrix less than the tolerance to zero

  Input Parameters:
+ A   - The matrix
- tol - The zero tolerance

  Output Parameters:
. A - The chopped matrix

  Level: intermediate

.seealso: MatCreate(), MatZeroEntries()
 @*/
PetscErrorCode MatChop(Mat A, PetscReal tol)
{
  PetscScalar    *newVals;
  PetscInt       *newCols;
  PetscInt       rStart, rEnd, numRows, maxRows, r, colMax = 0;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = MatGetOwnershipRange(A, &rStart, &rEnd);CHKERRQ(ierr);
  for (r = rStart; r < rEnd; ++r) {
    PetscInt ncols;

    ierr   = MatGetRow(A, r, &ncols, NULL, NULL);CHKERRQ(ierr);
    colMax = PetscMax(colMax, ncols);CHKERRQ(ierr);
    ierr   = MatRestoreRow(A, r, &ncols, NULL, NULL);CHKERRQ(ierr);
  }
  numRows = rEnd - rStart;
  ierr    = MPIU_Allreduce(&numRows, &maxRows, 1, MPIU_INT, MPI_MAX, PetscObjectComm((PetscObject)A));CHKERRQ(ierr);
  ierr    = PetscMalloc2(colMax,&newCols,colMax,&newVals);CHKERRQ(ierr);
  for (r = rStart; r < rStart+maxRows; ++r) {
    const PetscScalar *vals;
    const PetscInt    *cols;
    PetscInt           ncols, newcols, c;

    if (r < rEnd) {
      ierr = MatGetRow(A, r, &ncols, &cols, &vals);CHKERRQ(ierr);
      for (c = 0; c < ncols; ++c) {
        newCols[c] = cols[c];
        newVals[c] = PetscAbsScalar(vals[c]) < tol ? 0.0 : vals[c];
      }
      newcols = ncols;
      ierr = MatRestoreRow(A, r, &ncols, &cols, &vals);CHKERRQ(ierr);
      ierr = MatSetValues(A, 1, &r, newcols, newCols, newVals, INSERT_VALUES);CHKERRQ(ierr);
    }
    ierr = MatAssemblyBegin(A, MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
    ierr = MatAssemblyEnd(A, MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  }
  ierr = PetscFree2(newCols,newVals);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}
