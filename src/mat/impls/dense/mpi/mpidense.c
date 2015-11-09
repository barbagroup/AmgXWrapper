
/*
   Basic functions for basic parallel dense matrices.
*/


#include <../src/mat/impls/dense/mpi/mpidense.h>    /*I   "petscmat.h"  I*/
#include <../src/mat/impls/aij/mpi/mpiaij.h>
#include <petscblaslapack.h>

#undef __FUNCT__
#define __FUNCT__ "MatDenseGetLocalMatrix"
/*@

      MatDenseGetLocalMatrix - For a MATMPIDENSE or MATSEQDENSE matrix returns the sequential
              matrix that represents the operator. For sequential matrices it returns itself.

    Input Parameter:
.      A - the Seq or MPI dense matrix

    Output Parameter:
.      B - the inner matrix

    Level: intermediate

@*/
PetscErrorCode MatDenseGetLocalMatrix(Mat A,Mat *B)
{
  Mat_MPIDense   *mat = (Mat_MPIDense*)A->data;
  PetscErrorCode ierr;
  PetscBool      flg;

  PetscFunctionBegin;
  ierr = PetscObjectTypeCompare((PetscObject)A,MATMPIDENSE,&flg);CHKERRQ(ierr);
  if (flg) *B = mat->A;
  else *B = A;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatGetRow_MPIDense"
PetscErrorCode MatGetRow_MPIDense(Mat A,PetscInt row,PetscInt *nz,PetscInt **idx,PetscScalar **v)
{
  Mat_MPIDense   *mat = (Mat_MPIDense*)A->data;
  PetscErrorCode ierr;
  PetscInt       lrow,rstart = A->rmap->rstart,rend = A->rmap->rend;

  PetscFunctionBegin;
  if (row < rstart || row >= rend) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_SUP,"only local rows");
  lrow = row - rstart;
  ierr = MatGetRow(mat->A,lrow,nz,(const PetscInt**)idx,(const PetscScalar**)v);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatRestoreRow_MPIDense"
PetscErrorCode MatRestoreRow_MPIDense(Mat mat,PetscInt row,PetscInt *nz,PetscInt **idx,PetscScalar **v)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (idx) {ierr = PetscFree(*idx);CHKERRQ(ierr);}
  if (v) {ierr = PetscFree(*v);CHKERRQ(ierr);}
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatGetDiagonalBlock_MPIDense"
PetscErrorCode  MatGetDiagonalBlock_MPIDense(Mat A,Mat *a)
{
  Mat_MPIDense   *mdn = (Mat_MPIDense*)A->data;
  PetscErrorCode ierr;
  PetscInt       m = A->rmap->n,rstart = A->rmap->rstart;
  PetscScalar    *array;
  MPI_Comm       comm;
  Mat            B;

  PetscFunctionBegin;
  if (A->rmap->N != A->cmap->N) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_SUP,"Only square matrices supported.");

  ierr = PetscObjectQuery((PetscObject)A,"DiagonalBlock",(PetscObject*)&B);CHKERRQ(ierr);
  if (!B) {
    ierr = PetscObjectGetComm((PetscObject)(mdn->A),&comm);CHKERRQ(ierr);
    ierr = MatCreate(comm,&B);CHKERRQ(ierr);
    ierr = MatSetSizes(B,m,m,m,m);CHKERRQ(ierr);
    ierr = MatSetType(B,((PetscObject)mdn->A)->type_name);CHKERRQ(ierr);
    ierr = MatDenseGetArray(mdn->A,&array);CHKERRQ(ierr);
    ierr = MatSeqDenseSetPreallocation(B,array+m*rstart);CHKERRQ(ierr);
    ierr = MatDenseRestoreArray(mdn->A,&array);CHKERRQ(ierr);
    ierr = MatAssemblyBegin(B,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
    ierr = MatAssemblyEnd(B,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
    ierr = PetscObjectCompose((PetscObject)A,"DiagonalBlock",(PetscObject)B);CHKERRQ(ierr);
    *a   = B;
    ierr = MatDestroy(&B);CHKERRQ(ierr);
  } else *a = B;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatSetValues_MPIDense"
PetscErrorCode MatSetValues_MPIDense(Mat mat,PetscInt m,const PetscInt idxm[],PetscInt n,const PetscInt idxn[],const PetscScalar v[],InsertMode addv)
{
  Mat_MPIDense   *A = (Mat_MPIDense*)mat->data;
  PetscErrorCode ierr;
  PetscInt       i,j,rstart = mat->rmap->rstart,rend = mat->rmap->rend,row;
  PetscBool      roworiented = A->roworiented;

  PetscFunctionBegin;
  for (i=0; i<m; i++) {
    if (idxm[i] < 0) continue;
    if (idxm[i] >= mat->rmap->N) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Row too large");
    if (idxm[i] >= rstart && idxm[i] < rend) {
      row = idxm[i] - rstart;
      if (roworiented) {
        ierr = MatSetValues(A->A,1,&row,n,idxn,v+i*n,addv);CHKERRQ(ierr);
      } else {
        for (j=0; j<n; j++) {
          if (idxn[j] < 0) continue;
          if (idxn[j] >= mat->cmap->N) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Column too large");
          ierr = MatSetValues(A->A,1,&row,1,&idxn[j],v+i+j*m,addv);CHKERRQ(ierr);
        }
      }
    } else if (!A->donotstash) {
      mat->assembled = PETSC_FALSE;
      if (roworiented) {
        ierr = MatStashValuesRow_Private(&mat->stash,idxm[i],n,idxn,v+i*n,PETSC_FALSE);CHKERRQ(ierr);
      } else {
        ierr = MatStashValuesCol_Private(&mat->stash,idxm[i],n,idxn,v+i,m,PETSC_FALSE);CHKERRQ(ierr);
      }
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatGetValues_MPIDense"
PetscErrorCode MatGetValues_MPIDense(Mat mat,PetscInt m,const PetscInt idxm[],PetscInt n,const PetscInt idxn[],PetscScalar v[])
{
  Mat_MPIDense   *mdn = (Mat_MPIDense*)mat->data;
  PetscErrorCode ierr;
  PetscInt       i,j,rstart = mat->rmap->rstart,rend = mat->rmap->rend,row;

  PetscFunctionBegin;
  for (i=0; i<m; i++) {
    if (idxm[i] < 0) continue; /* SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Negative row"); */
    if (idxm[i] >= mat->rmap->N) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Row too large");
    if (idxm[i] >= rstart && idxm[i] < rend) {
      row = idxm[i] - rstart;
      for (j=0; j<n; j++) {
        if (idxn[j] < 0) continue; /* SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Negative column"); */
        if (idxn[j] >= mat->cmap->N) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Column too large");
        ierr = MatGetValues(mdn->A,1,&row,1,&idxn[j],v+i*n+j);CHKERRQ(ierr);
      }
    } else SETERRQ(PETSC_COMM_SELF,PETSC_ERR_SUP,"Only local values currently supported");
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatDenseGetArray_MPIDense"
PetscErrorCode MatDenseGetArray_MPIDense(Mat A,PetscScalar *array[])
{
  Mat_MPIDense   *a = (Mat_MPIDense*)A->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = MatDenseGetArray(a->A,array);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatGetSubMatrix_MPIDense"
static PetscErrorCode MatGetSubMatrix_MPIDense(Mat A,IS isrow,IS iscol,MatReuse scall,Mat *B)
{
  Mat_MPIDense   *mat  = (Mat_MPIDense*)A->data,*newmatd;
  Mat_SeqDense   *lmat = (Mat_SeqDense*)mat->A->data;
  PetscErrorCode ierr;
  PetscInt       i,j,rstart,rend,nrows,ncols,Ncols,nlrows,nlcols;
  const PetscInt *irow,*icol;
  PetscScalar    *av,*bv,*v = lmat->v;
  Mat            newmat;
  IS             iscol_local;

  PetscFunctionBegin;
  ierr = ISAllGather(iscol,&iscol_local);CHKERRQ(ierr);
  ierr = ISGetIndices(isrow,&irow);CHKERRQ(ierr);
  ierr = ISGetIndices(iscol_local,&icol);CHKERRQ(ierr);
  ierr = ISGetLocalSize(isrow,&nrows);CHKERRQ(ierr);
  ierr = ISGetLocalSize(iscol,&ncols);CHKERRQ(ierr);
  ierr = ISGetSize(iscol,&Ncols);CHKERRQ(ierr); /* global number of columns, size of iscol_local */

  /* No parallel redistribution currently supported! Should really check each index set
     to comfirm that it is OK.  ... Currently supports only submatrix same partitioning as
     original matrix! */

  ierr = MatGetLocalSize(A,&nlrows,&nlcols);CHKERRQ(ierr);
  ierr = MatGetOwnershipRange(A,&rstart,&rend);CHKERRQ(ierr);

  /* Check submatrix call */
  if (scall == MAT_REUSE_MATRIX) {
    /* SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_SIZ,"Reused submatrix wrong size"); */
    /* Really need to test rows and column sizes! */
    newmat = *B;
  } else {
    /* Create and fill new matrix */
    ierr = MatCreate(PetscObjectComm((PetscObject)A),&newmat);CHKERRQ(ierr);
    ierr = MatSetSizes(newmat,nrows,ncols,PETSC_DECIDE,Ncols);CHKERRQ(ierr);
    ierr = MatSetType(newmat,((PetscObject)A)->type_name);CHKERRQ(ierr);
    ierr = MatMPIDenseSetPreallocation(newmat,NULL);CHKERRQ(ierr);
  }

  /* Now extract the data pointers and do the copy, column at a time */
  newmatd = (Mat_MPIDense*)newmat->data;
  bv      = ((Mat_SeqDense*)newmatd->A->data)->v;

  for (i=0; i<Ncols; i++) {
    av = v + ((Mat_SeqDense*)mat->A->data)->lda*icol[i];
    for (j=0; j<nrows; j++) {
      *bv++ = av[irow[j] - rstart];
    }
  }

  /* Assemble the matrices so that the correct flags are set */
  ierr = MatAssemblyBegin(newmat,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(newmat,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);

  /* Free work space */
  ierr = ISRestoreIndices(isrow,&irow);CHKERRQ(ierr);
  ierr = ISRestoreIndices(iscol_local,&icol);CHKERRQ(ierr);
  ierr = ISDestroy(&iscol_local);CHKERRQ(ierr);
  *B   = newmat;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatDenseRestoreArray_MPIDense"
PetscErrorCode MatDenseRestoreArray_MPIDense(Mat A,PetscScalar *array[])
{
  Mat_MPIDense   *a = (Mat_MPIDense*)A->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = MatDenseRestoreArray(a->A,array);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatAssemblyBegin_MPIDense"
PetscErrorCode MatAssemblyBegin_MPIDense(Mat mat,MatAssemblyType mode)
{
  Mat_MPIDense   *mdn = (Mat_MPIDense*)mat->data;
  MPI_Comm       comm;
  PetscErrorCode ierr;
  PetscInt       nstash,reallocs;
  InsertMode     addv;

  PetscFunctionBegin;
  ierr = PetscObjectGetComm((PetscObject)mat,&comm);CHKERRQ(ierr);
  /* make sure all processors are either in INSERTMODE or ADDMODE */
  ierr = MPIU_Allreduce((PetscEnum*)&mat->insertmode,(PetscEnum*)&addv,1,MPIU_ENUM,MPI_BOR,comm);CHKERRQ(ierr);
  if (addv == (ADD_VALUES|INSERT_VALUES)) SETERRQ(PetscObjectComm((PetscObject)mat),PETSC_ERR_ARG_WRONGSTATE,"Cannot mix adds/inserts on different procs");
  mat->insertmode = addv; /* in case this processor had no cache */

  ierr = MatStashScatterBegin_Private(mat,&mat->stash,mat->rmap->range);CHKERRQ(ierr);
  ierr = MatStashGetInfo_Private(&mat->stash,&nstash,&reallocs);CHKERRQ(ierr);
  ierr = PetscInfo2(mdn->A,"Stash has %D entries, uses %D mallocs.\n",nstash,reallocs);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatAssemblyEnd_MPIDense"
PetscErrorCode MatAssemblyEnd_MPIDense(Mat mat,MatAssemblyType mode)
{
  Mat_MPIDense   *mdn=(Mat_MPIDense*)mat->data;
  PetscErrorCode ierr;
  PetscInt       i,*row,*col,flg,j,rstart,ncols;
  PetscMPIInt    n;
  PetscScalar    *val;

  PetscFunctionBegin;
  /*  wait on receives */
  while (1) {
    ierr = MatStashScatterGetMesg_Private(&mat->stash,&n,&row,&col,&val,&flg);CHKERRQ(ierr);
    if (!flg) break;

    for (i=0; i<n;) {
      /* Now identify the consecutive vals belonging to the same row */
      for (j=i,rstart=row[j]; j<n; j++) {
        if (row[j] != rstart) break;
      }
      if (j < n) ncols = j-i;
      else       ncols = n-i;
      /* Now assemble all these values with a single function call */
      ierr = MatSetValues_MPIDense(mat,1,row+i,ncols,col+i,val+i,mat->insertmode);CHKERRQ(ierr);
      i    = j;
    }
  }
  ierr = MatStashScatterEnd_Private(&mat->stash);CHKERRQ(ierr);

  ierr = MatAssemblyBegin(mdn->A,mode);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(mdn->A,mode);CHKERRQ(ierr);

  if (!mat->was_assembled && mode == MAT_FINAL_ASSEMBLY) {
    ierr = MatSetUpMultiply_MPIDense(mat);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatZeroEntries_MPIDense"
PetscErrorCode MatZeroEntries_MPIDense(Mat A)
{
  PetscErrorCode ierr;
  Mat_MPIDense   *l = (Mat_MPIDense*)A->data;

  PetscFunctionBegin;
  ierr = MatZeroEntries(l->A);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/* the code does not do the diagonal entries correctly unless the
   matrix is square and the column and row owerships are identical.
   This is a BUG. The only way to fix it seems to be to access
   mdn->A and mdn->B directly and not through the MatZeroRows()
   routine.
*/
#undef __FUNCT__
#define __FUNCT__ "MatZeroRows_MPIDense"
PetscErrorCode MatZeroRows_MPIDense(Mat A,PetscInt N,const PetscInt rows[],PetscScalar diag,Vec x,Vec b)
{
  Mat_MPIDense      *l = (Mat_MPIDense*)A->data;
  PetscErrorCode    ierr;
  PetscInt          i,*owners = A->rmap->range;
  PetscInt          *sizes,j,idx,nsends;
  PetscInt          nmax,*svalues,*starts,*owner,nrecvs;
  PetscInt          *rvalues,tag = ((PetscObject)A)->tag,count,base,slen,*source;
  PetscInt          *lens,*lrows,*values;
  PetscMPIInt       n,imdex,rank = l->rank,size = l->size;
  MPI_Comm          comm;
  MPI_Request       *send_waits,*recv_waits;
  MPI_Status        recv_status,*send_status;
  PetscBool         found;
  const PetscScalar *xx;
  PetscScalar       *bb;

  PetscFunctionBegin;
  ierr = PetscObjectGetComm((PetscObject)A,&comm);CHKERRQ(ierr);
  if (A->rmap->N != A->cmap->N) SETERRQ(comm,PETSC_ERR_SUP,"Only handles square matrices");
  if (A->rmap->n != A->cmap->n) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_SUP,"Only handles matrices with identical column and row ownership");
  /*  first count number of contributors to each processor */
  ierr = PetscCalloc1(2*size,&sizes);CHKERRQ(ierr);
  ierr = PetscMalloc1(N+1,&owner);CHKERRQ(ierr);  /* see note*/
  for (i=0; i<N; i++) {
    idx   = rows[i];
    found = PETSC_FALSE;
    for (j=0; j<size; j++) {
      if (idx >= owners[j] && idx < owners[j+1]) {
        sizes[2*j]++; sizes[2*j+1] = 1; owner[i] = j; found = PETSC_TRUE; break;
      }
    }
    if (!found) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Index out of range");
  }
  nsends = 0;
  for (i=0; i<size; i++) nsends += sizes[2*i+1];

  /* inform other processors of number of messages and max length*/
  ierr = PetscMaxSum(comm,sizes,&nmax,&nrecvs);CHKERRQ(ierr);

  /* post receives:   */
  ierr = PetscMalloc1((nrecvs+1)*(nmax+1),&rvalues);CHKERRQ(ierr);
  ierr = PetscMalloc1(nrecvs+1,&recv_waits);CHKERRQ(ierr);
  for (i=0; i<nrecvs; i++) {
    ierr = MPI_Irecv(rvalues+nmax*i,nmax,MPIU_INT,MPI_ANY_SOURCE,tag,comm,recv_waits+i);CHKERRQ(ierr);
  }

  /* do sends:
      1) starts[i] gives the starting index in svalues for stuff going to
         the ith processor
  */
  ierr = PetscMalloc1(N+1,&svalues);CHKERRQ(ierr);
  ierr = PetscMalloc1(nsends+1,&send_waits);CHKERRQ(ierr);
  ierr = PetscMalloc1(size+1,&starts);CHKERRQ(ierr);

  starts[0] = 0;
  for (i=1; i<size; i++) starts[i] = starts[i-1] + sizes[2*i-2];
  for (i=0; i<N; i++) svalues[starts[owner[i]]++] = rows[i];

  starts[0] = 0;
  for (i=1; i<size+1; i++) starts[i] = starts[i-1] + sizes[2*i-2];
  count = 0;
  for (i=0; i<size; i++) {
    if (sizes[2*i+1]) {
      ierr = MPI_Isend(svalues+starts[i],sizes[2*i],MPIU_INT,i,tag,comm,send_waits+count++);CHKERRQ(ierr);
    }
  }
  ierr = PetscFree(starts);CHKERRQ(ierr);

  base = owners[rank];

  /*  wait on receives */
  ierr  = PetscMalloc2(nrecvs,&lens,nrecvs,&source);CHKERRQ(ierr);
  count = nrecvs;
  slen  = 0;
  while (count) {
    ierr = MPI_Waitany(nrecvs,recv_waits,&imdex,&recv_status);CHKERRQ(ierr);
    /* unpack receives into our local space */
    ierr = MPI_Get_count(&recv_status,MPIU_INT,&n);CHKERRQ(ierr);

    source[imdex] = recv_status.MPI_SOURCE;
    lens[imdex]   = n;
    slen += n;
    count--;
  }
  ierr = PetscFree(recv_waits);CHKERRQ(ierr);

  /* move the data into the send scatter */
  ierr  = PetscMalloc1(slen+1,&lrows);CHKERRQ(ierr);
  count = 0;
  for (i=0; i<nrecvs; i++) {
    values = rvalues + i*nmax;
    for (j=0; j<lens[i]; j++) {
      lrows[count++] = values[j] - base;
    }
  }
  ierr = PetscFree(rvalues);CHKERRQ(ierr);
  ierr = PetscFree2(lens,source);CHKERRQ(ierr);
  ierr = PetscFree(owner);CHKERRQ(ierr);
  ierr = PetscFree(sizes);CHKERRQ(ierr);

  /* fix right hand side if needed */
  if (x && b) {
    ierr = VecGetArrayRead(x,&xx);CHKERRQ(ierr);
    ierr = VecGetArray(b,&bb);CHKERRQ(ierr);
    for (i=0; i<slen; i++) {
      bb[lrows[i]] = diag*xx[lrows[i]];
    }
    ierr = VecRestoreArrayRead(x,&xx);CHKERRQ(ierr);
    ierr = VecRestoreArray(b,&bb);CHKERRQ(ierr);
  }

  /* actually zap the local rows */
  ierr = MatZeroRows(l->A,slen,lrows,0.0,0,0);CHKERRQ(ierr);
  if (diag != 0.0) {
    Mat_SeqDense *ll = (Mat_SeqDense*)l->A->data;
    PetscInt     m   = ll->lda, i;

    for (i=0; i<slen; i++) {
      ll->v[lrows[i] + m*(A->cmap->rstart + lrows[i])] = diag;
    }
  }
  ierr = PetscFree(lrows);CHKERRQ(ierr);

  /* wait on sends */
  if (nsends) {
    ierr = PetscMalloc1(nsends,&send_status);CHKERRQ(ierr);
    ierr = MPI_Waitall(nsends,send_waits,send_status);CHKERRQ(ierr);
    ierr = PetscFree(send_status);CHKERRQ(ierr);
  }
  ierr = PetscFree(send_waits);CHKERRQ(ierr);
  ierr = PetscFree(svalues);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatMult_MPIDense"
PetscErrorCode MatMult_MPIDense(Mat mat,Vec xx,Vec yy)
{
  Mat_MPIDense   *mdn = (Mat_MPIDense*)mat->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = VecScatterBegin(mdn->Mvctx,xx,mdn->lvec,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
  ierr = VecScatterEnd(mdn->Mvctx,xx,mdn->lvec,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
  ierr = MatMult_SeqDense(mdn->A,mdn->lvec,yy);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatMultAdd_MPIDense"
PetscErrorCode MatMultAdd_MPIDense(Mat mat,Vec xx,Vec yy,Vec zz)
{
  Mat_MPIDense   *mdn = (Mat_MPIDense*)mat->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = VecScatterBegin(mdn->Mvctx,xx,mdn->lvec,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
  ierr = VecScatterEnd(mdn->Mvctx,xx,mdn->lvec,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
  ierr = MatMultAdd_SeqDense(mdn->A,mdn->lvec,yy,zz);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatMultTranspose_MPIDense"
PetscErrorCode MatMultTranspose_MPIDense(Mat A,Vec xx,Vec yy)
{
  Mat_MPIDense   *a = (Mat_MPIDense*)A->data;
  PetscErrorCode ierr;
  PetscScalar    zero = 0.0;

  PetscFunctionBegin;
  ierr = VecSet(yy,zero);CHKERRQ(ierr);
  ierr = MatMultTranspose_SeqDense(a->A,xx,a->lvec);CHKERRQ(ierr);
  ierr = VecScatterBegin(a->Mvctx,a->lvec,yy,ADD_VALUES,SCATTER_REVERSE);CHKERRQ(ierr);
  ierr = VecScatterEnd(a->Mvctx,a->lvec,yy,ADD_VALUES,SCATTER_REVERSE);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatMultTransposeAdd_MPIDense"
PetscErrorCode MatMultTransposeAdd_MPIDense(Mat A,Vec xx,Vec yy,Vec zz)
{
  Mat_MPIDense   *a = (Mat_MPIDense*)A->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = VecCopy(yy,zz);CHKERRQ(ierr);
  ierr = MatMultTranspose_SeqDense(a->A,xx,a->lvec);CHKERRQ(ierr);
  ierr = VecScatterBegin(a->Mvctx,a->lvec,zz,ADD_VALUES,SCATTER_REVERSE);CHKERRQ(ierr);
  ierr = VecScatterEnd(a->Mvctx,a->lvec,zz,ADD_VALUES,SCATTER_REVERSE);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatGetDiagonal_MPIDense"
PetscErrorCode MatGetDiagonal_MPIDense(Mat A,Vec v)
{
  Mat_MPIDense   *a    = (Mat_MPIDense*)A->data;
  Mat_SeqDense   *aloc = (Mat_SeqDense*)a->A->data;
  PetscErrorCode ierr;
  PetscInt       len,i,n,m = A->rmap->n,radd;
  PetscScalar    *x,zero = 0.0;

  PetscFunctionBegin;
  ierr = VecSet(v,zero);CHKERRQ(ierr);
  ierr = VecGetArray(v,&x);CHKERRQ(ierr);
  ierr = VecGetSize(v,&n);CHKERRQ(ierr);
  if (n != A->rmap->N) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_SIZ,"Nonconforming mat and vec");
  len  = PetscMin(a->A->rmap->n,a->A->cmap->n);
  radd = A->rmap->rstart*m;
  for (i=0; i<len; i++) {
    x[i] = aloc->v[radd + i*m + i];
  }
  ierr = VecRestoreArray(v,&x);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatDestroy_MPIDense"
PetscErrorCode MatDestroy_MPIDense(Mat mat)
{
  Mat_MPIDense   *mdn = (Mat_MPIDense*)mat->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
#if defined(PETSC_USE_LOG)
  PetscLogObjectState((PetscObject)mat,"Rows=%D, Cols=%D",mat->rmap->N,mat->cmap->N);
#endif
  ierr = MatStashDestroy_Private(&mat->stash);CHKERRQ(ierr);
  ierr = MatDestroy(&mdn->A);CHKERRQ(ierr);
  ierr = VecDestroy(&mdn->lvec);CHKERRQ(ierr);
  ierr = VecScatterDestroy(&mdn->Mvctx);CHKERRQ(ierr);

  ierr = PetscFree(mat->data);CHKERRQ(ierr);
  ierr = PetscObjectChangeTypeName((PetscObject)mat,0);CHKERRQ(ierr);

  ierr = PetscObjectComposeFunction((PetscObject)mat,"MatDenseGetArray_C",NULL);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)mat,"MatDenseRestoreArray_C",NULL);CHKERRQ(ierr);
#if defined(PETSC_HAVE_ELEMENTAL)
  ierr = PetscObjectComposeFunction((PetscObject)mat,"MatConvert_mpidense_elemental_C",NULL);CHKERRQ(ierr);
#endif
  ierr = PetscObjectComposeFunction((PetscObject)mat,"MatGetDiagonalBlock_C",NULL);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)mat,"MatMPIDenseSetPreallocation_C",NULL);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)mat,"MatMatMult_mpiaij_mpidense_C",NULL);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)mat,"MatMatMultSymbolic_mpiaij_mpidense_C",NULL);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)mat,"MatMatMultNumeric_mpiaij_mpidense_C",NULL);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)mat,"MatTransposeMatMult_mpiaij_mpidense_C",NULL);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)mat,"MatTransposeMatMultSymbolic_mpiaij_mpidense_C",NULL);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)mat,"MatTransposeMatMultNumeric_mpiaij_mpidense_C",NULL);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatView_MPIDense_Binary"
static PetscErrorCode MatView_MPIDense_Binary(Mat mat,PetscViewer viewer)
{
  Mat_MPIDense      *mdn = (Mat_MPIDense*)mat->data;
  PetscErrorCode    ierr;
  PetscViewerFormat format;
  int               fd;
  PetscInt          header[4],mmax,N = mat->cmap->N,i,j,m,k;
  PetscMPIInt       rank,tag  = ((PetscObject)viewer)->tag,size;
  PetscScalar       *work,*v,*vv;
  Mat_SeqDense      *a = (Mat_SeqDense*)mdn->A->data;

  PetscFunctionBegin;
  if (mdn->size == 1) {
    ierr = MatView(mdn->A,viewer);CHKERRQ(ierr);
  } else {
    ierr = PetscViewerBinaryGetDescriptor(viewer,&fd);CHKERRQ(ierr);
    ierr = MPI_Comm_rank(PetscObjectComm((PetscObject)mat),&rank);CHKERRQ(ierr);
    ierr = MPI_Comm_size(PetscObjectComm((PetscObject)mat),&size);CHKERRQ(ierr);

    ierr = PetscViewerGetFormat(viewer,&format);CHKERRQ(ierr);
    if (format == PETSC_VIEWER_NATIVE) {

      if (!rank) {
        /* store the matrix as a dense matrix */
        header[0] = MAT_FILE_CLASSID;
        header[1] = mat->rmap->N;
        header[2] = N;
        header[3] = MATRIX_BINARY_FORMAT_DENSE;
        ierr      = PetscBinaryWrite(fd,header,4,PETSC_INT,PETSC_TRUE);CHKERRQ(ierr);

        /* get largest work array needed for transposing array */
        mmax = mat->rmap->n;
        for (i=1; i<size; i++) {
          mmax = PetscMax(mmax,mat->rmap->range[i+1] - mat->rmap->range[i]);
        }
        ierr = PetscMalloc1(mmax*N,&work);CHKERRQ(ierr);

        /* write out local array, by rows */
        m = mat->rmap->n;
        v = a->v;
        for (j=0; j<N; j++) {
          for (i=0; i<m; i++) {
            work[j + i*N] = *v++;
          }
        }
        ierr = PetscBinaryWrite(fd,work,m*N,PETSC_SCALAR,PETSC_FALSE);CHKERRQ(ierr);
        /* get largest work array to receive messages from other processes, excludes process zero */
        mmax = 0;
        for (i=1; i<size; i++) {
          mmax = PetscMax(mmax,mat->rmap->range[i+1] - mat->rmap->range[i]);
        }
        ierr = PetscMalloc1(mmax*N,&vv);CHKERRQ(ierr);
        for (k = 1; k < size; k++) {
          v    = vv;
          m    = mat->rmap->range[k+1] - mat->rmap->range[k];
          ierr = MPIULong_Recv(v,m*N,MPIU_SCALAR,k,tag,PetscObjectComm((PetscObject)mat));CHKERRQ(ierr);

          for (j = 0; j < N; j++) {
            for (i = 0; i < m; i++) {
              work[j + i*N] = *v++;
            }
          }
          ierr = PetscBinaryWrite(fd,work,m*N,PETSC_SCALAR,PETSC_FALSE);CHKERRQ(ierr);
        }
        ierr = PetscFree(work);CHKERRQ(ierr);
        ierr = PetscFree(vv);CHKERRQ(ierr);
      } else {
        ierr = MPIULong_Send(a->v,mat->rmap->n*mat->cmap->N,MPIU_SCALAR,0,tag,PetscObjectComm((PetscObject)mat));CHKERRQ(ierr);
      }
    } else SETERRQ(PETSC_COMM_SELF,PETSC_ERR_SUP,"To store a parallel dense matrix you must first call PetscViewerSetFormat(viewer,PETSC_VIEWER_NATIVE)");
  }
  PetscFunctionReturn(0);
}

extern PetscErrorCode MatView_SeqDense(Mat,PetscViewer);
#include <petscdraw.h>
#undef __FUNCT__
#define __FUNCT__ "MatView_MPIDense_ASCIIorDraworSocket"
static PetscErrorCode MatView_MPIDense_ASCIIorDraworSocket(Mat mat,PetscViewer viewer)
{
  Mat_MPIDense      *mdn = (Mat_MPIDense*)mat->data;
  PetscErrorCode    ierr;
  PetscMPIInt       rank = mdn->rank;
  PetscViewerType   vtype;
  PetscBool         iascii,isdraw;
  PetscViewer       sviewer;
  PetscViewerFormat format;

  PetscFunctionBegin;
  ierr = PetscObjectTypeCompare((PetscObject)viewer,PETSCVIEWERASCII,&iascii);CHKERRQ(ierr);
  ierr = PetscObjectTypeCompare((PetscObject)viewer,PETSCVIEWERDRAW,&isdraw);CHKERRQ(ierr);
  if (iascii) {
    ierr = PetscViewerGetType(viewer,&vtype);CHKERRQ(ierr);
    ierr = PetscViewerGetFormat(viewer,&format);CHKERRQ(ierr);
    if (format == PETSC_VIEWER_ASCII_INFO_DETAIL) {
      MatInfo info;
      ierr = MatGetInfo(mat,MAT_LOCAL,&info);CHKERRQ(ierr);
      ierr = PetscViewerASCIIPushSynchronized(viewer);CHKERRQ(ierr);
      ierr = PetscViewerASCIISynchronizedPrintf(viewer,"  [%d] local rows %D nz %D nz alloced %D mem %D \n",rank,mat->rmap->n,(PetscInt)info.nz_used,(PetscInt)info.nz_allocated,(PetscInt)info.memory);CHKERRQ(ierr);
      ierr = PetscViewerFlush(viewer);CHKERRQ(ierr);
      ierr = PetscViewerASCIIPopSynchronized(viewer);CHKERRQ(ierr);
      ierr = VecScatterView(mdn->Mvctx,viewer);CHKERRQ(ierr);
      PetscFunctionReturn(0);
    } else if (format == PETSC_VIEWER_ASCII_INFO) {
      PetscFunctionReturn(0);
    }
  } else if (isdraw) {
    PetscDraw draw;
    PetscBool isnull;

    ierr = PetscViewerDrawGetDraw(viewer,0,&draw);CHKERRQ(ierr);
    ierr = PetscDrawIsNull(draw,&isnull);CHKERRQ(ierr);
    if (isnull) PetscFunctionReturn(0);
  }

  {
    /* assemble the entire matrix onto first processor. */
    Mat         A;
    PetscInt    M = mat->rmap->N,N = mat->cmap->N,m,row,i,nz;
    PetscInt    *cols;
    PetscScalar *vals;

    ierr = MatCreate(PetscObjectComm((PetscObject)mat),&A);CHKERRQ(ierr);
    if (!rank) {
      ierr = MatSetSizes(A,M,N,M,N);CHKERRQ(ierr);
    } else {
      ierr = MatSetSizes(A,0,0,M,N);CHKERRQ(ierr);
    }
    /* Since this is a temporary matrix, MATMPIDENSE instead of ((PetscObject)A)->type_name here is probably acceptable. */
    ierr = MatSetType(A,MATMPIDENSE);CHKERRQ(ierr);
    ierr = MatMPIDenseSetPreallocation(A,NULL);CHKERRQ(ierr);
    ierr = PetscLogObjectParent((PetscObject)mat,(PetscObject)A);CHKERRQ(ierr);

    /* Copy the matrix ... This isn't the most efficient means,
       but it's quick for now */
    A->insertmode = INSERT_VALUES;

    row = mat->rmap->rstart;
    m   = mdn->A->rmap->n;
    for (i=0; i<m; i++) {
      ierr = MatGetRow_MPIDense(mat,row,&nz,&cols,&vals);CHKERRQ(ierr);
      ierr = MatSetValues_MPIDense(A,1,&row,nz,cols,vals,INSERT_VALUES);CHKERRQ(ierr);
      ierr = MatRestoreRow_MPIDense(mat,row,&nz,&cols,&vals);CHKERRQ(ierr);
      row++;
    }

    ierr = MatAssemblyBegin(A,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
    ierr = MatAssemblyEnd(A,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
    ierr = PetscViewerGetSubViewer(viewer,PETSC_COMM_SELF,&sviewer);CHKERRQ(ierr);
    if (!rank) {
        ierr = MatView_SeqDense(((Mat_MPIDense*)(A->data))->A,sviewer);CHKERRQ(ierr);
    }
    ierr = PetscViewerRestoreSubViewer(viewer,PETSC_COMM_SELF,&sviewer);CHKERRQ(ierr);
    ierr = PetscViewerFlush(viewer);CHKERRQ(ierr);
    ierr = MatDestroy(&A);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatView_MPIDense"
PetscErrorCode MatView_MPIDense(Mat mat,PetscViewer viewer)
{
  PetscErrorCode ierr;
  PetscBool      iascii,isbinary,isdraw,issocket;

  PetscFunctionBegin;
  ierr = PetscObjectTypeCompare((PetscObject)viewer,PETSCVIEWERASCII,&iascii);CHKERRQ(ierr);
  ierr = PetscObjectTypeCompare((PetscObject)viewer,PETSCVIEWERBINARY,&isbinary);CHKERRQ(ierr);
  ierr = PetscObjectTypeCompare((PetscObject)viewer,PETSCVIEWERSOCKET,&issocket);CHKERRQ(ierr);
  ierr = PetscObjectTypeCompare((PetscObject)viewer,PETSCVIEWERDRAW,&isdraw);CHKERRQ(ierr);

  if (iascii || issocket || isdraw) {
    ierr = MatView_MPIDense_ASCIIorDraworSocket(mat,viewer);CHKERRQ(ierr);
  } else if (isbinary) {
    ierr = MatView_MPIDense_Binary(mat,viewer);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatGetInfo_MPIDense"
PetscErrorCode MatGetInfo_MPIDense(Mat A,MatInfoType flag,MatInfo *info)
{
  Mat_MPIDense   *mat = (Mat_MPIDense*)A->data;
  Mat            mdn  = mat->A;
  PetscErrorCode ierr;
  PetscReal      isend[5],irecv[5];

  PetscFunctionBegin;
  info->block_size = 1.0;

  ierr = MatGetInfo(mdn,MAT_LOCAL,info);CHKERRQ(ierr);

  isend[0] = info->nz_used; isend[1] = info->nz_allocated; isend[2] = info->nz_unneeded;
  isend[3] = info->memory;  isend[4] = info->mallocs;
  if (flag == MAT_LOCAL) {
    info->nz_used      = isend[0];
    info->nz_allocated = isend[1];
    info->nz_unneeded  = isend[2];
    info->memory       = isend[3];
    info->mallocs      = isend[4];
  } else if (flag == MAT_GLOBAL_MAX) {
    ierr = MPIU_Allreduce(isend,irecv,5,MPIU_REAL,MPIU_MAX,PetscObjectComm((PetscObject)A));CHKERRQ(ierr);

    info->nz_used      = irecv[0];
    info->nz_allocated = irecv[1];
    info->nz_unneeded  = irecv[2];
    info->memory       = irecv[3];
    info->mallocs      = irecv[4];
  } else if (flag == MAT_GLOBAL_SUM) {
    ierr = MPIU_Allreduce(isend,irecv,5,MPIU_REAL,MPIU_SUM,PetscObjectComm((PetscObject)A));CHKERRQ(ierr);

    info->nz_used      = irecv[0];
    info->nz_allocated = irecv[1];
    info->nz_unneeded  = irecv[2];
    info->memory       = irecv[3];
    info->mallocs      = irecv[4];
  }
  info->fill_ratio_given  = 0; /* no parallel LU/ILU/Cholesky */
  info->fill_ratio_needed = 0;
  info->factor_mallocs    = 0;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatSetOption_MPIDense"
PetscErrorCode MatSetOption_MPIDense(Mat A,MatOption op,PetscBool flg)
{
  Mat_MPIDense   *a = (Mat_MPIDense*)A->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  switch (op) {
  case MAT_NEW_NONZERO_LOCATIONS:
  case MAT_NEW_NONZERO_LOCATION_ERR:
  case MAT_NEW_NONZERO_ALLOCATION_ERR:
    ierr = MatSetOption(a->A,op,flg);CHKERRQ(ierr);
    break;
  case MAT_ROW_ORIENTED:
    a->roworiented = flg;
    ierr = MatSetOption(a->A,op,flg);CHKERRQ(ierr);
    break;
  case MAT_NEW_DIAGONALS:
  case MAT_KEEP_NONZERO_PATTERN:
  case MAT_USE_HASH_TABLE:
    ierr = PetscInfo1(A,"Option %s ignored\n",MatOptions[op]);CHKERRQ(ierr);
    break;
  case MAT_IGNORE_OFF_PROC_ENTRIES:
    a->donotstash = flg;
    break;
  case MAT_SYMMETRIC:
  case MAT_STRUCTURALLY_SYMMETRIC:
  case MAT_HERMITIAN:
  case MAT_SYMMETRY_ETERNAL:
  case MAT_IGNORE_LOWER_TRIANGULAR:
    ierr = PetscInfo1(A,"Option %s ignored\n",MatOptions[op]);CHKERRQ(ierr);
    break;
  default:
    SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_SUP,"unknown option %s",MatOptions[op]);
  }
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "MatDiagonalScale_MPIDense"
PetscErrorCode MatDiagonalScale_MPIDense(Mat A,Vec ll,Vec rr)
{
  Mat_MPIDense   *mdn = (Mat_MPIDense*)A->data;
  Mat_SeqDense   *mat = (Mat_SeqDense*)mdn->A->data;
  PetscScalar    *l,*r,x,*v;
  PetscErrorCode ierr;
  PetscInt       i,j,s2a,s3a,s2,s3,m=mdn->A->rmap->n,n=mdn->A->cmap->n;

  PetscFunctionBegin;
  ierr = MatGetLocalSize(A,&s2,&s3);CHKERRQ(ierr);
  if (ll) {
    ierr = VecGetLocalSize(ll,&s2a);CHKERRQ(ierr);
    if (s2a != s2) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_SIZ,"Left scaling vector non-conforming local size, %d != %d.", s2a, s2);
    ierr = VecGetArray(ll,&l);CHKERRQ(ierr);
    for (i=0; i<m; i++) {
      x = l[i];
      v = mat->v + i;
      for (j=0; j<n; j++) { (*v) *= x; v+= m;}
    }
    ierr = VecRestoreArray(ll,&l);CHKERRQ(ierr);
    ierr = PetscLogFlops(n*m);CHKERRQ(ierr);
  }
  if (rr) {
    ierr = VecGetLocalSize(rr,&s3a);CHKERRQ(ierr);
    if (s3a != s3) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_SIZ,"Right scaling vec non-conforming local size, %d != %d.", s3a, s3);
    ierr = VecScatterBegin(mdn->Mvctx,rr,mdn->lvec,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
    ierr = VecScatterEnd(mdn->Mvctx,rr,mdn->lvec,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
    ierr = VecGetArray(mdn->lvec,&r);CHKERRQ(ierr);
    for (i=0; i<n; i++) {
      x = r[i];
      v = mat->v + i*m;
      for (j=0; j<m; j++) (*v++) *= x;
    }
    ierr = VecRestoreArray(mdn->lvec,&r);CHKERRQ(ierr);
    ierr = PetscLogFlops(n*m);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatNorm_MPIDense"
PetscErrorCode MatNorm_MPIDense(Mat A,NormType type,PetscReal *nrm)
{
  Mat_MPIDense   *mdn = (Mat_MPIDense*)A->data;
  Mat_SeqDense   *mat = (Mat_SeqDense*)mdn->A->data;
  PetscErrorCode ierr;
  PetscInt       i,j;
  PetscReal      sum = 0.0;
  PetscScalar    *v  = mat->v;

  PetscFunctionBegin;
  if (mdn->size == 1) {
    ierr =  MatNorm(mdn->A,type,nrm);CHKERRQ(ierr);
  } else {
    if (type == NORM_FROBENIUS) {
      for (i=0; i<mdn->A->cmap->n*mdn->A->rmap->n; i++) {
        sum += PetscRealPart(PetscConj(*v)*(*v)); v++;
      }
      ierr = MPIU_Allreduce(&sum,nrm,1,MPIU_REAL,MPIU_SUM,PetscObjectComm((PetscObject)A));CHKERRQ(ierr);
      *nrm = PetscSqrtReal(*nrm);
      ierr = PetscLogFlops(2.0*mdn->A->cmap->n*mdn->A->rmap->n);CHKERRQ(ierr);
    } else if (type == NORM_1) {
      PetscReal *tmp,*tmp2;
      ierr = PetscMalloc2(A->cmap->N,&tmp,A->cmap->N,&tmp2);CHKERRQ(ierr);
      ierr = PetscMemzero(tmp,A->cmap->N*sizeof(PetscReal));CHKERRQ(ierr);
      ierr = PetscMemzero(tmp2,A->cmap->N*sizeof(PetscReal));CHKERRQ(ierr);
      *nrm = 0.0;
      v    = mat->v;
      for (j=0; j<mdn->A->cmap->n; j++) {
        for (i=0; i<mdn->A->rmap->n; i++) {
          tmp[j] += PetscAbsScalar(*v);  v++;
        }
      }
      ierr = MPIU_Allreduce(tmp,tmp2,A->cmap->N,MPIU_REAL,MPIU_SUM,PetscObjectComm((PetscObject)A));CHKERRQ(ierr);
      for (j=0; j<A->cmap->N; j++) {
        if (tmp2[j] > *nrm) *nrm = tmp2[j];
      }
      ierr = PetscFree2(tmp,tmp2);CHKERRQ(ierr);
      ierr = PetscLogFlops(A->cmap->n*A->rmap->n);CHKERRQ(ierr);
    } else if (type == NORM_INFINITY) { /* max row norm */
      PetscReal ntemp;
      ierr = MatNorm(mdn->A,type,&ntemp);CHKERRQ(ierr);
      ierr = MPIU_Allreduce(&ntemp,nrm,1,MPIU_REAL,MPIU_MAX,PetscObjectComm((PetscObject)A));CHKERRQ(ierr);
    } else SETERRQ(PetscObjectComm((PetscObject)A),PETSC_ERR_SUP,"No support for two norm");
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatTranspose_MPIDense"
PetscErrorCode MatTranspose_MPIDense(Mat A,MatReuse reuse,Mat *matout)
{
  Mat_MPIDense   *a    = (Mat_MPIDense*)A->data;
  Mat_SeqDense   *Aloc = (Mat_SeqDense*)a->A->data;
  Mat            B;
  PetscInt       M = A->rmap->N,N = A->cmap->N,m,n,*rwork,rstart = A->rmap->rstart;
  PetscErrorCode ierr;
  PetscInt       j,i;
  PetscScalar    *v;

  PetscFunctionBegin;
  if (reuse == MAT_REUSE_MATRIX && A == *matout && M != N) SETERRQ(PetscObjectComm((PetscObject)A),PETSC_ERR_SUP,"Supports square matrix only in-place");
  if (reuse == MAT_INITIAL_MATRIX || A == *matout) {
    ierr = MatCreate(PetscObjectComm((PetscObject)A),&B);CHKERRQ(ierr);
    ierr = MatSetSizes(B,A->cmap->n,A->rmap->n,N,M);CHKERRQ(ierr);
    ierr = MatSetType(B,((PetscObject)A)->type_name);CHKERRQ(ierr);
    ierr = MatMPIDenseSetPreallocation(B,NULL);CHKERRQ(ierr);
  } else {
    B = *matout;
  }

  m    = a->A->rmap->n; n = a->A->cmap->n; v = Aloc->v;
  ierr = PetscMalloc1(m,&rwork);CHKERRQ(ierr);
  for (i=0; i<m; i++) rwork[i] = rstart + i;
  for (j=0; j<n; j++) {
    ierr = MatSetValues(B,1,&j,m,rwork,v,INSERT_VALUES);CHKERRQ(ierr);
    v   += m;
  }
  ierr = PetscFree(rwork);CHKERRQ(ierr);
  ierr = MatAssemblyBegin(B,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(B,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  if (reuse == MAT_INITIAL_MATRIX || *matout != A) {
    *matout = B;
  } else {
    ierr = MatHeaderMerge(A,&B);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}


static PetscErrorCode MatDuplicate_MPIDense(Mat,MatDuplicateOption,Mat*);
extern PetscErrorCode MatScale_MPIDense(Mat,PetscScalar);

#undef __FUNCT__
#define __FUNCT__ "MatSetUp_MPIDense"
PetscErrorCode MatSetUp_MPIDense(Mat A)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr =  MatMPIDenseSetPreallocation(A,0);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatAXPY_MPIDense"
PetscErrorCode MatAXPY_MPIDense(Mat Y,PetscScalar alpha,Mat X,MatStructure str)
{
  PetscErrorCode ierr;
  Mat_MPIDense   *A = (Mat_MPIDense*)Y->data, *B = (Mat_MPIDense*)X->data;

  PetscFunctionBegin;
  ierr = MatAXPY(A->A,alpha,B->A,str);CHKERRQ(ierr);
  ierr = PetscObjectStateIncrease((PetscObject)Y);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatConjugate_MPIDense"
PetscErrorCode  MatConjugate_MPIDense(Mat mat)
{
  Mat_MPIDense   *a = (Mat_MPIDense*)mat->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = MatConjugate(a->A);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatRealPart_MPIDense"
PetscErrorCode MatRealPart_MPIDense(Mat A)
{
  Mat_MPIDense   *a = (Mat_MPIDense*)A->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = MatRealPart(a->A);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatImaginaryPart_MPIDense"
PetscErrorCode MatImaginaryPart_MPIDense(Mat A)
{
  Mat_MPIDense   *a = (Mat_MPIDense*)A->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = MatImaginaryPart(a->A);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

extern PetscErrorCode MatGetColumnNorms_SeqDense(Mat,NormType,PetscReal*);
#undef __FUNCT__
#define __FUNCT__ "MatGetColumnNorms_MPIDense"
PetscErrorCode MatGetColumnNorms_MPIDense(Mat A,NormType type,PetscReal *norms)
{
  PetscErrorCode ierr;
  PetscInt       i,n;
  Mat_MPIDense   *a = (Mat_MPIDense*) A->data;
  PetscReal      *work;

  PetscFunctionBegin;
  ierr = MatGetSize(A,NULL,&n);CHKERRQ(ierr);
  ierr = PetscMalloc1(n,&work);CHKERRQ(ierr);
  ierr = MatGetColumnNorms_SeqDense(a->A,type,work);CHKERRQ(ierr);
  if (type == NORM_2) {
    for (i=0; i<n; i++) work[i] *= work[i];
  }
  if (type == NORM_INFINITY) {
    ierr = MPIU_Allreduce(work,norms,n,MPIU_REAL,MPIU_MAX,A->hdr.comm);CHKERRQ(ierr);
  } else {
    ierr = MPIU_Allreduce(work,norms,n,MPIU_REAL,MPIU_SUM,A->hdr.comm);CHKERRQ(ierr);
  }
  ierr = PetscFree(work);CHKERRQ(ierr);
  if (type == NORM_2) {
    for (i=0; i<n; i++) norms[i] = PetscSqrtReal(norms[i]);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatSetRandom_MPIDense"
static PetscErrorCode  MatSetRandom_MPIDense(Mat x,PetscRandom rctx)
{
  Mat_MPIDense   *d = (Mat_MPIDense*)x->data;
  PetscErrorCode ierr;
  PetscScalar    *a;
  PetscInt       m,n,i;

  PetscFunctionBegin;
  ierr = MatGetSize(d->A,&m,&n);CHKERRQ(ierr);
  ierr = MatDenseGetArray(d->A,&a);CHKERRQ(ierr);
  for (i=0; i<m*n; i++) {
    ierr = PetscRandomGetValue(rctx,a+i);CHKERRQ(ierr);
  }
  ierr = MatDenseRestoreArray(d->A,&a);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

extern PetscErrorCode MatMatMultNumeric_MPIDense(Mat A,Mat,Mat);

#undef __FUNCT__
#define __FUNCT__ "MatMissingDiagonal_MPIDense"
static PetscErrorCode MatMissingDiagonal_MPIDense(Mat A,PetscBool  *missing,PetscInt *d)
{
  PetscFunctionBegin;
  *missing = PETSC_FALSE;
  PetscFunctionReturn(0);
}

/* -------------------------------------------------------------------*/
static struct _MatOps MatOps_Values = { MatSetValues_MPIDense,
                                        MatGetRow_MPIDense,
                                        MatRestoreRow_MPIDense,
                                        MatMult_MPIDense,
                                /*  4*/ MatMultAdd_MPIDense,
                                        MatMultTranspose_MPIDense,
                                        MatMultTransposeAdd_MPIDense,
                                        0,
                                        0,
                                        0,
                                /* 10*/ 0,
                                        0,
                                        0,
                                        0,
                                        MatTranspose_MPIDense,
                                /* 15*/ MatGetInfo_MPIDense,
                                        MatEqual_MPIDense,
                                        MatGetDiagonal_MPIDense,
                                        MatDiagonalScale_MPIDense,
                                        MatNorm_MPIDense,
                                /* 20*/ MatAssemblyBegin_MPIDense,
                                        MatAssemblyEnd_MPIDense,
                                        MatSetOption_MPIDense,
                                        MatZeroEntries_MPIDense,
                                /* 24*/ MatZeroRows_MPIDense,
                                        0,
                                        0,
                                        0,
                                        0,
                                /* 29*/ MatSetUp_MPIDense,
                                        0,
                                        0,
                                        0,
                                        0,
                                /* 34*/ MatDuplicate_MPIDense,
                                        0,
                                        0,
                                        0,
                                        0,
                                /* 39*/ MatAXPY_MPIDense,
                                        MatGetSubMatrices_MPIDense,
                                        0,
                                        MatGetValues_MPIDense,
                                        0,
                                /* 44*/ 0,
                                        MatScale_MPIDense,
                                        MatShift_Basic,
                                        0,
                                        0,
                                /* 49*/ MatSetRandom_MPIDense,
                                        0,
                                        0,
                                        0,
                                        0,
                                /* 54*/ 0,
                                        0,
                                        0,
                                        0,
                                        0,
                                /* 59*/ MatGetSubMatrix_MPIDense,
                                        MatDestroy_MPIDense,
                                        MatView_MPIDense,
                                        0,
                                        0,
                                /* 64*/ 0,
                                        0,
                                        0,
                                        0,
                                        0,
                                /* 69*/ 0,
                                        0,
                                        0,
                                        0,
                                        0,
                                /* 74*/ 0,
                                        0,
                                        0,
                                        0,
                                        0,
                                /* 79*/ 0,
                                        0,
                                        0,
                                        0,
                                /* 83*/ MatLoad_MPIDense,
                                        0,
                                        0,
                                        0,
                                        0,
                                        0,
#if defined(PETSC_HAVE_ELEMENTAL)
                                /* 89*/ MatMatMult_MPIDense_MPIDense,
                                        MatMatMultSymbolic_MPIDense_MPIDense,
#else
                                /* 89*/ 0,
                                        0,
#endif
                                        MatMatMultNumeric_MPIDense,
                                        0,
                                        0,
                                /* 94*/ 0,
                                        0,
                                        0,
                                        0,
                                        0,
                                /* 99*/ 0,
                                        0,
                                        0,
                                        MatConjugate_MPIDense,
                                        0,
                                /*104*/ 0,
                                        MatRealPart_MPIDense,
                                        MatImaginaryPart_MPIDense,
                                        0,
                                        0,
                                /*109*/ 0,
                                        0,
                                        0,
                                        0,
                                        MatMissingDiagonal_MPIDense,
                                /*114*/ 0,
                                        0,
                                        0,
                                        0,
                                        0,
                                /*119*/ 0,
                                        0,
                                        0,
                                        0,
                                        0,
                                /*124*/ 0,
                                        MatGetColumnNorms_MPIDense,
                                        0,
                                        0,
                                        0,
                                /*129*/ 0,
                                        MatTransposeMatMult_MPIDense_MPIDense,
                                        MatTransposeMatMultSymbolic_MPIDense_MPIDense,
                                        MatTransposeMatMultNumeric_MPIDense_MPIDense,
                                        0,
                                /*134*/ 0,
                                        0,
                                        0,
                                        0,
                                        0,
                                /*139*/ 0,
                                        0,
                                        0
};

#undef __FUNCT__
#define __FUNCT__ "MatMPIDenseSetPreallocation_MPIDense"
PetscErrorCode  MatMPIDenseSetPreallocation_MPIDense(Mat mat,PetscScalar *data)
{
  Mat_MPIDense   *a;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  mat->preallocated = PETSC_TRUE;
  /* Note:  For now, when data is specified above, this assumes the user correctly
   allocates the local dense storage space.  We should add error checking. */

  a       = (Mat_MPIDense*)mat->data;
  ierr    = PetscLayoutSetUp(mat->rmap);CHKERRQ(ierr);
  ierr    = PetscLayoutSetUp(mat->cmap);CHKERRQ(ierr);
  a->nvec = mat->cmap->n;

  ierr = MatCreate(PETSC_COMM_SELF,&a->A);CHKERRQ(ierr);
  ierr = MatSetSizes(a->A,mat->rmap->n,mat->cmap->N,mat->rmap->n,mat->cmap->N);CHKERRQ(ierr);
  ierr = MatSetType(a->A,MATSEQDENSE);CHKERRQ(ierr);
  ierr = MatSeqDenseSetPreallocation(a->A,data);CHKERRQ(ierr);
  ierr = PetscLogObjectParent((PetscObject)mat,(PetscObject)a->A);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#if defined(PETSC_HAVE_ELEMENTAL)
#undef __FUNCT__
#define __FUNCT__ "MatConvert_MPIDense_Elemental"
PETSC_EXTERN PetscErrorCode MatConvert_MPIDense_Elemental(Mat A, MatType newtype,MatReuse reuse,Mat *newmat)
{
  Mat            mat_elemental;
  PetscErrorCode ierr;
  PetscScalar    *v;
  PetscInt       m=A->rmap->n,N=A->cmap->N,rstart=A->rmap->rstart,i,*rows,*cols;
  
  PetscFunctionBegin;
  if (reuse == MAT_REUSE_MATRIX) {
    mat_elemental = *newmat;
    ierr = MatZeroEntries(*newmat);CHKERRQ(ierr);
  } else {
    ierr = MatCreate(PetscObjectComm((PetscObject)A), &mat_elemental);CHKERRQ(ierr);
    ierr = MatSetSizes(mat_elemental,PETSC_DECIDE,PETSC_DECIDE,A->rmap->N,A->cmap->N);CHKERRQ(ierr);
    ierr = MatSetType(mat_elemental,MATELEMENTAL);CHKERRQ(ierr);
    ierr = MatSetUp(mat_elemental);CHKERRQ(ierr);
    ierr = MatSetOption(mat_elemental,MAT_ROW_ORIENTED,PETSC_FALSE);CHKERRQ(ierr);
  }

  ierr = PetscMalloc2(m,&rows,N,&cols);CHKERRQ(ierr);
  for (i=0; i<N; i++) cols[i] = i;
  for (i=0; i<m; i++) rows[i] = rstart + i;
  
  /* PETSc-Elemental interaface uses axpy for setting off-processor entries, only ADD_VALUES is allowed */
  ierr = MatDenseGetArray(A,&v);CHKERRQ(ierr);
  ierr = MatSetValues(mat_elemental,m,rows,N,cols,v,ADD_VALUES);CHKERRQ(ierr);
  ierr = MatAssemblyBegin(mat_elemental, MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(mat_elemental, MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatDenseRestoreArray(A,&v);CHKERRQ(ierr);
  ierr = PetscFree2(rows,cols);CHKERRQ(ierr);

  if (reuse == MAT_INPLACE_MATRIX) {
    ierr = MatHeaderReplace(A,&mat_elemental);CHKERRQ(ierr);
  } else {
    *newmat = mat_elemental;
  }
  PetscFunctionReturn(0);
}
#endif

#undef __FUNCT__
#define __FUNCT__ "MatCreate_MPIDense"
PETSC_EXTERN PetscErrorCode MatCreate_MPIDense(Mat mat)
{
  Mat_MPIDense   *a;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr      = PetscNewLog(mat,&a);CHKERRQ(ierr);
  mat->data = (void*)a;
  ierr      = PetscMemcpy(mat->ops,&MatOps_Values,sizeof(struct _MatOps));CHKERRQ(ierr);

  mat->insertmode = NOT_SET_VALUES;
  ierr            = MPI_Comm_rank(PetscObjectComm((PetscObject)mat),&a->rank);CHKERRQ(ierr);
  ierr            = MPI_Comm_size(PetscObjectComm((PetscObject)mat),&a->size);CHKERRQ(ierr);

  /* build cache for off array entries formed */
  a->donotstash = PETSC_FALSE;

  ierr = MatStashCreate_Private(PetscObjectComm((PetscObject)mat),1,&mat->stash);CHKERRQ(ierr);

  /* stuff used for matrix vector multiply */
  a->lvec        = 0;
  a->Mvctx       = 0;
  a->roworiented = PETSC_TRUE;

  ierr = PetscObjectComposeFunction((PetscObject)mat,"MatDenseGetArray_C",MatDenseGetArray_MPIDense);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)mat,"MatDenseRestoreArray_C",MatDenseRestoreArray_MPIDense);CHKERRQ(ierr);
#if defined(PETSC_HAVE_ELEMENTAL)
  ierr = PetscObjectComposeFunction((PetscObject)mat,"MatConvert_mpidense_elemental_C",MatConvert_MPIDense_Elemental);CHKERRQ(ierr);
#endif
  ierr = PetscObjectComposeFunction((PetscObject)mat,"MatGetDiagonalBlock_C",MatGetDiagonalBlock_MPIDense);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)mat,"MatMPIDenseSetPreallocation_C",MatMPIDenseSetPreallocation_MPIDense);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)mat,"MatMatMult_mpiaij_mpidense_C",MatMatMult_MPIAIJ_MPIDense);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)mat,"MatMatMultSymbolic_mpiaij_mpidense_C",MatMatMultSymbolic_MPIAIJ_MPIDense);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)mat,"MatMatMultNumeric_mpiaij_mpidense_C",MatMatMultNumeric_MPIAIJ_MPIDense);CHKERRQ(ierr);

  ierr = PetscObjectComposeFunction((PetscObject)mat,"MatTransposeMatMult_mpiaij_mpidense_C",MatTransposeMatMult_MPIAIJ_MPIDense);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)mat,"MatTransposeMatMultSymbolic_mpiaij_mpidense_C",MatTransposeMatMultSymbolic_MPIAIJ_MPIDense);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)mat,"MatTransposeMatMultNumeric_mpiaij_mpidense_C",MatTransposeMatMultNumeric_MPIAIJ_MPIDense);CHKERRQ(ierr);
  ierr = PetscObjectChangeTypeName((PetscObject)mat,MATMPIDENSE);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/*MC
   MATDENSE - MATDENSE = "dense" - A matrix type to be used for dense matrices.

   This matrix type is identical to MATSEQDENSE when constructed with a single process communicator,
   and MATMPIDENSE otherwise.

   Options Database Keys:
. -mat_type dense - sets the matrix type to "dense" during a call to MatSetFromOptions()

  Level: beginner


.seealso: MatCreateMPIDense,MATSEQDENSE,MATMPIDENSE
M*/

#undef __FUNCT__
#define __FUNCT__ "MatMPIDenseSetPreallocation"
/*@C
   MatMPIDenseSetPreallocation - Sets the array used to store the matrix entries

   Not collective

   Input Parameters:
.  B - the matrix
-  data - optional location of matrix data.  Set data=NULL for PETSc
   to control all matrix memory allocation.

   Notes:
   The dense format is fully compatible with standard Fortran 77
   storage by columns.

   The data input variable is intended primarily for Fortran programmers
   who wish to allocate their own matrix memory space.  Most users should
   set data=NULL.

   Level: intermediate

.keywords: matrix,dense, parallel

.seealso: MatCreate(), MatCreateSeqDense(), MatSetValues()
@*/
PetscErrorCode  MatMPIDenseSetPreallocation(Mat B,PetscScalar *data)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscTryMethod(B,"MatMPIDenseSetPreallocation_C",(Mat,PetscScalar*),(B,data));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatCreateDense"
/*@C
   MatCreateDense - Creates a parallel matrix in dense format.

   Collective on MPI_Comm

   Input Parameters:
+  comm - MPI communicator
.  m - number of local rows (or PETSC_DECIDE to have calculated if M is given)
.  n - number of local columns (or PETSC_DECIDE to have calculated if N is given)
.  M - number of global rows (or PETSC_DECIDE to have calculated if m is given)
.  N - number of global columns (or PETSC_DECIDE to have calculated if n is given)
-  data - optional location of matrix data.  Set data=NULL (PETSC_NULL_SCALAR for Fortran users) for PETSc
   to control all matrix memory allocation.

   Output Parameter:
.  A - the matrix

   Notes:
   The dense format is fully compatible with standard Fortran 77
   storage by columns.

   The data input variable is intended primarily for Fortran programmers
   who wish to allocate their own matrix memory space.  Most users should
   set data=NULL (PETSC_NULL_SCALAR for Fortran users).

   The user MUST specify either the local or global matrix dimensions
   (possibly both).

   Level: intermediate

.keywords: matrix,dense, parallel

.seealso: MatCreate(), MatCreateSeqDense(), MatSetValues()
@*/
PetscErrorCode  MatCreateDense(MPI_Comm comm,PetscInt m,PetscInt n,PetscInt M,PetscInt N,PetscScalar *data,Mat *A)
{
  PetscErrorCode ierr;
  PetscMPIInt    size;

  PetscFunctionBegin;
  ierr = MatCreate(comm,A);CHKERRQ(ierr);
  ierr = MatSetSizes(*A,m,n,M,N);CHKERRQ(ierr);
  ierr = MPI_Comm_size(comm,&size);CHKERRQ(ierr);
  if (size > 1) {
    ierr = MatSetType(*A,MATMPIDENSE);CHKERRQ(ierr);
    ierr = MatMPIDenseSetPreallocation(*A,data);CHKERRQ(ierr);
    if (data) {  /* user provided data array, so no need to assemble */
      ierr = MatSetUpMultiply_MPIDense(*A);CHKERRQ(ierr);
      (*A)->assembled = PETSC_TRUE;
    }
  } else {
    ierr = MatSetType(*A,MATSEQDENSE);CHKERRQ(ierr);
    ierr = MatSeqDenseSetPreallocation(*A,data);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatDuplicate_MPIDense"
static PetscErrorCode MatDuplicate_MPIDense(Mat A,MatDuplicateOption cpvalues,Mat *newmat)
{
  Mat            mat;
  Mat_MPIDense   *a,*oldmat = (Mat_MPIDense*)A->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  *newmat = 0;
  ierr    = MatCreate(PetscObjectComm((PetscObject)A),&mat);CHKERRQ(ierr);
  ierr    = MatSetSizes(mat,A->rmap->n,A->cmap->n,A->rmap->N,A->cmap->N);CHKERRQ(ierr);
  ierr    = MatSetType(mat,((PetscObject)A)->type_name);CHKERRQ(ierr);
  a       = (Mat_MPIDense*)mat->data;
  ierr    = PetscMemcpy(mat->ops,A->ops,sizeof(struct _MatOps));CHKERRQ(ierr);

  mat->factortype   = A->factortype;
  mat->assembled    = PETSC_TRUE;
  mat->preallocated = PETSC_TRUE;

  a->size         = oldmat->size;
  a->rank         = oldmat->rank;
  mat->insertmode = NOT_SET_VALUES;
  a->nvec         = oldmat->nvec;
  a->donotstash   = oldmat->donotstash;

  ierr = PetscLayoutReference(A->rmap,&mat->rmap);CHKERRQ(ierr);
  ierr = PetscLayoutReference(A->cmap,&mat->cmap);CHKERRQ(ierr);

  ierr = MatSetUpMultiply_MPIDense(mat);CHKERRQ(ierr);
  ierr = MatDuplicate(oldmat->A,cpvalues,&a->A);CHKERRQ(ierr);
  ierr = PetscLogObjectParent((PetscObject)mat,(PetscObject)a->A);CHKERRQ(ierr);

  *newmat = mat;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatLoad_MPIDense_DenseInFile"
PetscErrorCode MatLoad_MPIDense_DenseInFile(MPI_Comm comm,PetscInt fd,PetscInt M,PetscInt N,Mat newmat)
{
  PetscErrorCode ierr;
  PetscMPIInt    rank,size;
  const PetscInt *rowners;
  PetscInt       i,m,n,nz,j,mMax;
  PetscScalar    *array,*vals,*vals_ptr;
  Mat_MPIDense   *a = (Mat_MPIDense*)newmat->data;

  PetscFunctionBegin;
  ierr = MPI_Comm_rank(comm,&rank);CHKERRQ(ierr);
  ierr = MPI_Comm_size(comm,&size);CHKERRQ(ierr);

  /* determine ownership of rows and columns */
  m = (newmat->rmap->n < 0) ? PETSC_DECIDE : newmat->rmap->n;
  n = (newmat->cmap->n < 0) ? PETSC_DECIDE : newmat->cmap->n;

  ierr = MatSetSizes(newmat,m,n,M,N);CHKERRQ(ierr);
  if (!a->A || !((Mat_SeqDense*)(a->A->data))->user_alloc) {
    ierr = MatMPIDenseSetPreallocation(newmat,NULL);CHKERRQ(ierr);
  }
  ierr = MatDenseGetArray(newmat,&array);CHKERRQ(ierr);
  ierr = MatGetLocalSize(newmat,&m,NULL);CHKERRQ(ierr);
  ierr = MatGetOwnershipRanges(newmat,&rowners);CHKERRQ(ierr);
  ierr = MPI_Reduce(&m,&mMax,1,MPIU_INT,MPI_MAX,0,comm);CHKERRQ(ierr);
  if (!rank) {
    ierr = PetscMalloc1(mMax*N,&vals);CHKERRQ(ierr);

    /* read in my part of the matrix numerical values  */
    ierr = PetscBinaryRead(fd,vals,m*N,PETSC_SCALAR);CHKERRQ(ierr);

    /* insert into matrix-by row (this is why cannot directly read into array */
    vals_ptr = vals;
    for (i=0; i<m; i++) {
      for (j=0; j<N; j++) {
        array[i + j*m] = *vals_ptr++;
      }
    }

    /* read in other processors and ship out */
    for (i=1; i<size; i++) {
      nz   = (rowners[i+1] - rowners[i])*N;
      ierr = PetscBinaryRead(fd,vals,nz,PETSC_SCALAR);CHKERRQ(ierr);
      ierr = MPIULong_Send(vals,nz,MPIU_SCALAR,i,((PetscObject)(newmat))->tag,comm);CHKERRQ(ierr);
    }
  } else {
    /* receive numeric values */
    ierr = PetscMalloc1(m*N,&vals);CHKERRQ(ierr);

    /* receive message of values*/
    ierr = MPIULong_Recv(vals,m*N,MPIU_SCALAR,0,((PetscObject)(newmat))->tag,comm);CHKERRQ(ierr);

    /* insert into matrix-by row (this is why cannot directly read into array */
    vals_ptr = vals;
    for (i=0; i<m; i++) {
      for (j=0; j<N; j++) {
        array[i + j*m] = *vals_ptr++;
      }
    }
  }
  ierr = MatDenseRestoreArray(newmat,&array);CHKERRQ(ierr);
  ierr = PetscFree(vals);CHKERRQ(ierr);
  ierr = MatAssemblyBegin(newmat,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(newmat,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatLoad_MPIDense"
PetscErrorCode MatLoad_MPIDense(Mat newmat,PetscViewer viewer)
{
  Mat_MPIDense   *a;
  PetscScalar    *vals,*svals;
  MPI_Comm       comm;
  MPI_Status     status;
  PetscMPIInt    rank,size,tag = ((PetscObject)viewer)->tag,*rowners,*sndcounts,m,n,maxnz;
  PetscInt       header[4],*rowlengths = 0,M,N,*cols;
  PetscInt       *ourlens,*procsnz = 0,jj,*mycols,*smycols;
  PetscInt       i,nz,j,rstart,rend;
  int            fd;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  /* force binary viewer to load .info file if it has not yet done so */
  ierr = PetscViewerSetUp(viewer);CHKERRQ(ierr);
  ierr = PetscObjectGetComm((PetscObject)viewer,&comm);CHKERRQ(ierr);
  ierr = MPI_Comm_size(comm,&size);CHKERRQ(ierr);
  ierr = MPI_Comm_rank(comm,&rank);CHKERRQ(ierr);
  ierr = PetscViewerBinaryGetDescriptor(viewer,&fd);CHKERRQ(ierr);
  if (!rank) {
    ierr = PetscBinaryRead(fd,(char*)header,4,PETSC_INT);CHKERRQ(ierr);
    if (header[0] != MAT_FILE_CLASSID) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_FILE_UNEXPECTED,"not matrix object");
  }
  ierr = MPI_Bcast(header+1,3,MPIU_INT,0,comm);CHKERRQ(ierr);
  M    = header[1]; N = header[2]; nz = header[3];

  /* If global rows/cols are set to PETSC_DECIDE, set it to the sizes given in the file */
  if (newmat->rmap->N < 0) newmat->rmap->N = M;
  if (newmat->cmap->N < 0) newmat->cmap->N = N;

  if (newmat->rmap->N != M) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_FILE_UNEXPECTED, "Inconsistent # of rows:Matrix in file has (%D) and input matrix has (%D)",M,newmat->rmap->N);
  if (newmat->cmap->N != N) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_FILE_UNEXPECTED, "Inconsistent # of cols:Matrix in file has (%D) and input matrix has (%D)",N,newmat->cmap->N);

  /*
       Handle case where matrix is stored on disk as a dense matrix
  */
  if (nz == MATRIX_BINARY_FORMAT_DENSE) {
    ierr = MatLoad_MPIDense_DenseInFile(comm,fd,M,N,newmat);CHKERRQ(ierr);
    PetscFunctionReturn(0);
  }

  /* determine ownership of all rows */
  if (newmat->rmap->n < 0) {
    ierr = PetscMPIIntCast(M/size + ((M % size) > rank),&m);CHKERRQ(ierr);
  } else {
    ierr = PetscMPIIntCast(newmat->rmap->n,&m);CHKERRQ(ierr);
  }
  if (newmat->cmap->n < 0) {
    n = PETSC_DECIDE;
  } else {
    ierr = PetscMPIIntCast(newmat->cmap->n,&n);CHKERRQ(ierr);
  }

  ierr       = PetscMalloc1(size+2,&rowners);CHKERRQ(ierr);
  ierr       = MPI_Allgather(&m,1,MPI_INT,rowners+1,1,MPI_INT,comm);CHKERRQ(ierr);
  rowners[0] = 0;
  for (i=2; i<=size; i++) {
    rowners[i] += rowners[i-1];
  }
  rstart = rowners[rank];
  rend   = rowners[rank+1];

  /* distribute row lengths to all processors */
  ierr = PetscMalloc1(rend-rstart,&ourlens);CHKERRQ(ierr);
  if (!rank) {
    ierr = PetscMalloc1(M,&rowlengths);CHKERRQ(ierr);
    ierr = PetscBinaryRead(fd,rowlengths,M,PETSC_INT);CHKERRQ(ierr);
    ierr = PetscMalloc1(size,&sndcounts);CHKERRQ(ierr);
    for (i=0; i<size; i++) sndcounts[i] = rowners[i+1] - rowners[i];
    ierr = MPI_Scatterv(rowlengths,sndcounts,rowners,MPIU_INT,ourlens,rend-rstart,MPIU_INT,0,comm);CHKERRQ(ierr);
    ierr = PetscFree(sndcounts);CHKERRQ(ierr);
  } else {
    ierr = MPI_Scatterv(0,0,0,MPIU_INT,ourlens,rend-rstart,MPIU_INT,0,comm);CHKERRQ(ierr);
  }

  if (!rank) {
    /* calculate the number of nonzeros on each processor */
    ierr = PetscMalloc1(size,&procsnz);CHKERRQ(ierr);
    ierr = PetscMemzero(procsnz,size*sizeof(PetscInt));CHKERRQ(ierr);
    for (i=0; i<size; i++) {
      for (j=rowners[i]; j< rowners[i+1]; j++) {
        procsnz[i] += rowlengths[j];
      }
    }
    ierr = PetscFree(rowlengths);CHKERRQ(ierr);

    /* determine max buffer needed and allocate it */
    maxnz = 0;
    for (i=0; i<size; i++) {
      maxnz = PetscMax(maxnz,procsnz[i]);
    }
    ierr = PetscMalloc1(maxnz,&cols);CHKERRQ(ierr);

    /* read in my part of the matrix column indices  */
    nz   = procsnz[0];
    ierr = PetscMalloc1(nz,&mycols);CHKERRQ(ierr);
    ierr = PetscBinaryRead(fd,mycols,nz,PETSC_INT);CHKERRQ(ierr);

    /* read in every one elses and ship off */
    for (i=1; i<size; i++) {
      nz   = procsnz[i];
      ierr = PetscBinaryRead(fd,cols,nz,PETSC_INT);CHKERRQ(ierr);
      ierr = MPI_Send(cols,nz,MPIU_INT,i,tag,comm);CHKERRQ(ierr);
    }
    ierr = PetscFree(cols);CHKERRQ(ierr);
  } else {
    /* determine buffer space needed for message */
    nz = 0;
    for (i=0; i<m; i++) {
      nz += ourlens[i];
    }
    ierr = PetscMalloc1(nz+1,&mycols);CHKERRQ(ierr);

    /* receive message of column indices*/
    ierr = MPI_Recv(mycols,nz,MPIU_INT,0,tag,comm,&status);CHKERRQ(ierr);
    ierr = MPI_Get_count(&status,MPIU_INT,&maxnz);CHKERRQ(ierr);
    if (maxnz != nz) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_FILE_UNEXPECTED,"something is wrong with file");
  }

  ierr = MatSetSizes(newmat,m,n,M,N);CHKERRQ(ierr);
  a = (Mat_MPIDense*)newmat->data;
  if (!a->A || !((Mat_SeqDense*)(a->A->data))->user_alloc) {
    ierr = MatMPIDenseSetPreallocation(newmat,NULL);CHKERRQ(ierr);
  }

  if (!rank) {
    ierr = PetscMalloc1(maxnz,&vals);CHKERRQ(ierr);

    /* read in my part of the matrix numerical values  */
    nz   = procsnz[0];
    ierr = PetscBinaryRead(fd,vals,nz,PETSC_SCALAR);CHKERRQ(ierr);

    /* insert into matrix */
    jj      = rstart;
    smycols = mycols;
    svals   = vals;
    for (i=0; i<m; i++) {
      ierr     = MatSetValues(newmat,1,&jj,ourlens[i],smycols,svals,INSERT_VALUES);CHKERRQ(ierr);
      smycols += ourlens[i];
      svals   += ourlens[i];
      jj++;
    }

    /* read in other processors and ship out */
    for (i=1; i<size; i++) {
      nz   = procsnz[i];
      ierr = PetscBinaryRead(fd,vals,nz,PETSC_SCALAR);CHKERRQ(ierr);
      ierr = MPI_Send(vals,nz,MPIU_SCALAR,i,((PetscObject)newmat)->tag,comm);CHKERRQ(ierr);
    }
    ierr = PetscFree(procsnz);CHKERRQ(ierr);
  } else {
    /* receive numeric values */
    ierr = PetscMalloc1(nz+1,&vals);CHKERRQ(ierr);

    /* receive message of values*/
    ierr = MPI_Recv(vals,nz,MPIU_SCALAR,0,((PetscObject)newmat)->tag,comm,&status);CHKERRQ(ierr);
    ierr = MPI_Get_count(&status,MPIU_SCALAR,&maxnz);CHKERRQ(ierr);
    if (maxnz != nz) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_FILE_UNEXPECTED,"something is wrong with file");

    /* insert into matrix */
    jj      = rstart;
    smycols = mycols;
    svals   = vals;
    for (i=0; i<m; i++) {
      ierr     = MatSetValues(newmat,1,&jj,ourlens[i],smycols,svals,INSERT_VALUES);CHKERRQ(ierr);
      smycols += ourlens[i];
      svals   += ourlens[i];
      jj++;
    }
  }
  ierr = PetscFree(ourlens);CHKERRQ(ierr);
  ierr = PetscFree(vals);CHKERRQ(ierr);
  ierr = PetscFree(mycols);CHKERRQ(ierr);
  ierr = PetscFree(rowners);CHKERRQ(ierr);

  ierr = MatAssemblyBegin(newmat,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(newmat,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatEqual_MPIDense"
PetscErrorCode MatEqual_MPIDense(Mat A,Mat B,PetscBool  *flag)
{
  Mat_MPIDense   *matB = (Mat_MPIDense*)B->data,*matA = (Mat_MPIDense*)A->data;
  Mat            a,b;
  PetscBool      flg;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  a    = matA->A;
  b    = matB->A;
  ierr = MatEqual(a,b,&flg);CHKERRQ(ierr);
  ierr = MPIU_Allreduce(&flg,flag,1,MPIU_BOOL,MPI_LAND,PetscObjectComm((PetscObject)A));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatDestroy_MatTransMatMult_MPIDense_MPIDense"
PetscErrorCode MatDestroy_MatTransMatMult_MPIDense_MPIDense(Mat A)
{
  PetscErrorCode        ierr;
  Mat_MPIDense          *a = (Mat_MPIDense*)A->data;
  Mat_TransMatMultDense *atb = a->atbdense;

  PetscFunctionBegin;
  ierr = PetscFree3(atb->sendbuf,atb->atbarray,atb->recvcounts);CHKERRQ(ierr);
  ierr = (atb->destroy)(A);CHKERRQ(ierr);
  ierr = PetscFree(atb);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatTransposeMatMultNumeric_MPIDense_MPIDense"
PetscErrorCode MatTransposeMatMultNumeric_MPIDense_MPIDense(Mat A,Mat B,Mat C)
{
  Mat_MPIDense   *a=(Mat_MPIDense*)A->data, *b=(Mat_MPIDense*)B->data, *c=(Mat_MPIDense*)C->data;
  Mat_SeqDense   *aseq=(Mat_SeqDense*)(a->A)->data, *bseq=(Mat_SeqDense*)(b->A)->data;
  Mat_TransMatMultDense *atb = c->atbdense;
  PetscErrorCode ierr;
  MPI_Comm       comm;
  PetscMPIInt    rank,size,*recvcounts=atb->recvcounts;
  PetscScalar    *carray,*atbarray=atb->atbarray,*sendbuf=atb->sendbuf;
  PetscInt       i,cN=C->cmap->N,cM=C->rmap->N,proc,k,j;
  PetscScalar    _DOne=1.0,_DZero=0.0;
  PetscBLASInt   am,an,bn,aN;
  const PetscInt *ranges;

  PetscFunctionBegin;
  ierr = PetscObjectGetComm((PetscObject)A,&comm);CHKERRQ(ierr);
  ierr = MPI_Comm_rank(comm,&rank);CHKERRQ(ierr);
  ierr = MPI_Comm_size(comm,&size);CHKERRQ(ierr);

  /* compute atbarray = aseq^T * bseq */
  ierr = PetscBLASIntCast(a->A->cmap->n,&an);CHKERRQ(ierr);
  ierr = PetscBLASIntCast(b->A->cmap->n,&bn);CHKERRQ(ierr);
  ierr = PetscBLASIntCast(a->A->rmap->n,&am);CHKERRQ(ierr);
  ierr = PetscBLASIntCast(A->cmap->N,&aN);CHKERRQ(ierr);
  PetscStackCallBLAS("BLASgemm",BLASgemm_("T","N",&an,&bn,&am,&_DOne,aseq->v,&aseq->lda,bseq->v,&bseq->lda,&_DZero,atbarray,&aN));
 
  ierr = MatGetOwnershipRanges(C,&ranges);CHKERRQ(ierr);
  for (i=0; i<size; i++) recvcounts[i] = (ranges[i+1] - ranges[i])*cN;
  
  /* arrange atbarray into sendbuf */
  k = 0;
  for (proc=0; proc<size; proc++) {
    for (j=0; j<cN; j++) {
      for (i=ranges[proc]; i<ranges[proc+1]; i++) sendbuf[k++] = atbarray[i+j*cM]; 
    }
  }
  /* sum all atbarray to local values of C */
  ierr = MatDenseGetArray(c->A,&carray);CHKERRQ(ierr);
  ierr = MPI_Reduce_scatter(sendbuf,carray,recvcounts,MPIU_SCALAR,MPIU_SUM,comm);CHKERRQ(ierr);
  ierr = MatDenseRestoreArray(c->A,&carray);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatTransposeMatMultSymbolic_MPIDense_MPIDense"
PetscErrorCode MatTransposeMatMultSymbolic_MPIDense_MPIDense(Mat A,Mat B,PetscReal fill,Mat *C)
{
  PetscErrorCode        ierr;
  Mat                   Cdense;
  MPI_Comm              comm;
  PetscMPIInt           size;
  PetscInt              cm=A->cmap->n,cM,cN=B->cmap->N;
  Mat_MPIDense          *c;
  Mat_TransMatMultDense *atb;

  PetscFunctionBegin;
  ierr = PetscObjectGetComm((PetscObject)A,&comm);CHKERRQ(ierr);
  if (A->rmap->rstart != B->rmap->rstart || A->rmap->rend != B->rmap->rend) {
    SETERRQ4(comm,PETSC_ERR_ARG_SIZ,"Matrix local dimensions are incompatible, A (%D, %D) != B (%D,%D)",A->rmap->rstart,A->rmap->rend,B->rmap->rstart,B->rmap->rend);
  }

  /* create matrix product Cdense */
  ierr = MatCreate(comm,&Cdense);CHKERRQ(ierr);
  ierr = MatSetSizes(Cdense,cm,B->cmap->n,PETSC_DECIDE,PETSC_DECIDE);CHKERRQ(ierr);
  ierr = MatSetType(Cdense,MATMPIDENSE);CHKERRQ(ierr);
  ierr = MatMPIDenseSetPreallocation(Cdense,NULL);CHKERRQ(ierr);
  ierr = MatAssemblyBegin(Cdense,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(Cdense,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  *C   = Cdense;

  /* create data structure for reuse Cdense */
  ierr = MPI_Comm_size(comm,&size);CHKERRQ(ierr);
  ierr = PetscNew(&atb);CHKERRQ(ierr);
  cM = Cdense->rmap->N; 
  ierr = PetscMalloc3(cM*cN,&atb->sendbuf,cM*cN,&atb->atbarray,size,&atb->recvcounts);CHKERRQ(ierr);
  
  c                    = (Mat_MPIDense*)Cdense->data;
  c->atbdense          = atb;
  atb->destroy         = Cdense->ops->destroy;
  Cdense->ops->destroy = MatDestroy_MatTransMatMult_MPIDense_MPIDense;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatTransposeMatMult_MPIDense_MPIDense"
PetscErrorCode MatTransposeMatMult_MPIDense_MPIDense(Mat A,Mat B,MatReuse scall,PetscReal fill,Mat *C)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (scall == MAT_INITIAL_MATRIX) { 
    ierr = MatTransposeMatMultSymbolic_MPIDense_MPIDense(A,B,fill,C);CHKERRQ(ierr);
  } 
  ierr = MatTransposeMatMultNumeric_MPIDense_MPIDense(A,B,*C);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatDestroy_MatMatMult_MPIDense_MPIDense"
PetscErrorCode MatDestroy_MatMatMult_MPIDense_MPIDense(Mat A)
{
  PetscErrorCode   ierr;
  Mat_MPIDense     *a = (Mat_MPIDense*)A->data;
  Mat_MatMultDense *ab = a->abdense;

  PetscFunctionBegin;
  ierr = MatDestroy(&ab->Ce);CHKERRQ(ierr);
  ierr = MatDestroy(&ab->Ae);CHKERRQ(ierr);
  ierr = MatDestroy(&ab->Be);CHKERRQ(ierr);

  ierr = (ab->destroy)(A);CHKERRQ(ierr);
  ierr = PetscFree(ab);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#if defined(PETSC_HAVE_ELEMENTAL)
#undef __FUNCT__
#define __FUNCT__ "MatMatMultNumeric_MPIDense_MPIDense"
PetscErrorCode MatMatMultNumeric_MPIDense_MPIDense(Mat A,Mat B,Mat C)
{
  PetscErrorCode   ierr;
  Mat_MPIDense     *c=(Mat_MPIDense*)C->data;
  Mat_MatMultDense *ab=c->abdense;

  PetscFunctionBegin;
  ierr = MatConvert_MPIDense_Elemental(A,MATELEMENTAL,MAT_REUSE_MATRIX, &ab->Ae);CHKERRQ(ierr);
  ierr = MatConvert_MPIDense_Elemental(B,MATELEMENTAL,MAT_REUSE_MATRIX, &ab->Be);CHKERRQ(ierr);
  ierr = MatMatMultNumeric(ab->Ae,ab->Be,ab->Ce);CHKERRQ(ierr);
  ierr = MatConvert(ab->Ce,MATMPIDENSE,MAT_REUSE_MATRIX,&C);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatMatMultSymbolic_MPIDense_MPIDense"
PetscErrorCode MatMatMultSymbolic_MPIDense_MPIDense(Mat A,Mat B,PetscReal fill,Mat *C)
{
  PetscErrorCode   ierr;
  Mat              Ae,Be,Ce;
  Mat_MPIDense     *c;
  Mat_MatMultDense *ab;

  PetscFunctionBegin;
  if (A->cmap->rstart != B->rmap->rstart || A->cmap->rend != B->rmap->rend) {
    SETERRQ4(PetscObjectComm((PetscObject)A),PETSC_ERR_ARG_SIZ,"Matrix local dimensions are incompatible, A (%D, %D) != B (%D,%D)",A->rmap->rstart,A->rmap->rend,B->rmap->rstart,B->rmap->rend);
  }

  /* convert A and B to Elemental matrices Ae and Be */
  ierr = MatConvert(A,MATELEMENTAL,MAT_INITIAL_MATRIX, &Ae);CHKERRQ(ierr);
  ierr = MatConvert(B,MATELEMENTAL,MAT_INITIAL_MATRIX, &Be);CHKERRQ(ierr);

  /* Ce = Ae*Be */
  ierr = MatMatMultSymbolic(Ae,Be,fill,&Ce);CHKERRQ(ierr);
  ierr = MatMatMultNumeric(Ae,Be,Ce);CHKERRQ(ierr);
 
  /* convert Ce to C */
  ierr = MatConvert(Ce,MATMPIDENSE,MAT_INITIAL_MATRIX,C);CHKERRQ(ierr);

  /* create data structure for reuse Cdense */
  ierr = PetscNew(&ab);CHKERRQ(ierr);
  c                  = (Mat_MPIDense*)(*C)->data;
  c->abdense         = ab;

  ab->Ae             = Ae;
  ab->Be             = Be;
  ab->Ce             = Ce;
  ab->destroy        = (*C)->ops->destroy;
  (*C)->ops->destroy        = MatDestroy_MatMatMult_MPIDense_MPIDense;
  (*C)->ops->matmultnumeric = MatMatMultNumeric_MPIDense_MPIDense;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatMatMult_MPIDense_MPIDense"
PetscErrorCode MatMatMult_MPIDense_MPIDense(Mat A,Mat B,MatReuse scall,PetscReal fill,Mat *C)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (scall == MAT_INITIAL_MATRIX) { /* simbolic product includes numeric product */
    ierr = PetscLogEventBegin(MAT_MatMultSymbolic,A,B,0,0);CHKERRQ(ierr);
    ierr = MatMatMultSymbolic_MPIDense_MPIDense(A,B,fill,C);CHKERRQ(ierr);
    ierr = PetscLogEventEnd(MAT_MatMultSymbolic,A,B,0,0);CHKERRQ(ierr);
  } else {
    ierr = PetscLogEventBegin(MAT_MatMultNumeric,A,B,0,0);CHKERRQ(ierr);
    ierr = MatMatMultNumeric_MPIDense_MPIDense(A,B,*C);CHKERRQ(ierr);
    ierr = PetscLogEventEnd(MAT_MatMultNumeric,A,B,0,0);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}
#endif
