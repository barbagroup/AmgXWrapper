
/*
    Defines the basic matrix operations for the ADJ adjacency list matrix data-structure.
*/
#include <../src/mat/impls/adj/mpi/mpiadj.h>    /*I "petscmat.h" I*/
#include <petscsf.h>

#undef __FUNCT__
#define __FUNCT__ "MatView_MPIAdj_ASCII"
PetscErrorCode MatView_MPIAdj_ASCII(Mat A,PetscViewer viewer)
{
  Mat_MPIAdj        *a = (Mat_MPIAdj*)A->data;
  PetscErrorCode    ierr;
  PetscInt          i,j,m = A->rmap->n;
  const char        *name;
  PetscViewerFormat format;

  PetscFunctionBegin;
  ierr = PetscObjectGetName((PetscObject)A,&name);CHKERRQ(ierr);
  ierr = PetscViewerGetFormat(viewer,&format);CHKERRQ(ierr);
  if (format == PETSC_VIEWER_ASCII_INFO) {
    PetscFunctionReturn(0);
  } else if (format == PETSC_VIEWER_ASCII_MATLAB) SETERRQ(PetscObjectComm((PetscObject)A),PETSC_ERR_SUP,"MATLAB format not supported");
  else {
    ierr = PetscViewerASCIIUseTabs(viewer,PETSC_FALSE);CHKERRQ(ierr);
    ierr = PetscViewerASCIIPushSynchronized(viewer);CHKERRQ(ierr);
    for (i=0; i<m; i++) {
      ierr = PetscViewerASCIISynchronizedPrintf(viewer,"row %D:",i+A->rmap->rstart);CHKERRQ(ierr);
      for (j=a->i[i]; j<a->i[i+1]; j++) {
        ierr = PetscViewerASCIISynchronizedPrintf(viewer," %D ",a->j[j]);CHKERRQ(ierr);
      }
      ierr = PetscViewerASCIISynchronizedPrintf(viewer,"\n");CHKERRQ(ierr);
    }
    ierr = PetscViewerASCIIUseTabs(viewer,PETSC_TRUE);CHKERRQ(ierr);
    ierr = PetscViewerFlush(viewer);CHKERRQ(ierr);
    ierr = PetscViewerASCIIPopSynchronized(viewer);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatView_MPIAdj"
PetscErrorCode MatView_MPIAdj(Mat A,PetscViewer viewer)
{
  PetscErrorCode ierr;
  PetscBool      iascii;

  PetscFunctionBegin;
  ierr = PetscObjectTypeCompare((PetscObject)viewer,PETSCVIEWERASCII,&iascii);CHKERRQ(ierr);
  if (iascii) {
    ierr = MatView_MPIAdj_ASCII(A,viewer);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatDestroy_MPIAdj"
PetscErrorCode MatDestroy_MPIAdj(Mat mat)
{
  Mat_MPIAdj     *a = (Mat_MPIAdj*)mat->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
#if defined(PETSC_USE_LOG)
  PetscLogObjectState((PetscObject)mat,"Rows=%D, Cols=%D, NZ=%D",mat->rmap->n,mat->cmap->n,a->nz);
#endif
  ierr = PetscFree(a->diag);CHKERRQ(ierr);
  if (a->freeaij) {
    if (a->freeaijwithfree) {
      if (a->i) free(a->i);
      if (a->j) free(a->j);
    } else {
      ierr = PetscFree(a->i);CHKERRQ(ierr);
      ierr = PetscFree(a->j);CHKERRQ(ierr);
      ierr = PetscFree(a->values);CHKERRQ(ierr);
    }
  }
  ierr = PetscFree(a->rowvalues);CHKERRQ(ierr);
  ierr = PetscFree(mat->data);CHKERRQ(ierr);
  ierr = PetscObjectChangeTypeName((PetscObject)mat,0);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)mat,"MatMPIAdjSetPreallocation_C",NULL);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)mat,"MatMPIAdjCreateNonemptySubcommMat_C",NULL);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatSetOption_MPIAdj"
PetscErrorCode MatSetOption_MPIAdj(Mat A,MatOption op,PetscBool flg)
{
  Mat_MPIAdj     *a = (Mat_MPIAdj*)A->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  switch (op) {
  case MAT_SYMMETRIC:
  case MAT_STRUCTURALLY_SYMMETRIC:
  case MAT_HERMITIAN:
    a->symmetric = flg;
    break;
  case MAT_SYMMETRY_ETERNAL:
    break;
  default:
    ierr = PetscInfo1(A,"Option %s ignored\n",MatOptions[op]);CHKERRQ(ierr);
    break;
  }
  PetscFunctionReturn(0);
}


/*
     Adds diagonal pointers to sparse matrix structure.
*/

#undef __FUNCT__
#define __FUNCT__ "MatMarkDiagonal_MPIAdj"
PetscErrorCode MatMarkDiagonal_MPIAdj(Mat A)
{
  Mat_MPIAdj     *a = (Mat_MPIAdj*)A->data;
  PetscErrorCode ierr;
  PetscInt       i,j,m = A->rmap->n;

  PetscFunctionBegin;
  ierr = PetscMalloc1(m,&a->diag);CHKERRQ(ierr);
  ierr = PetscLogObjectMemory((PetscObject)A,m*sizeof(PetscInt));CHKERRQ(ierr);
  for (i=0; i<A->rmap->n; i++) {
    for (j=a->i[i]; j<a->i[i+1]; j++) {
      if (a->j[j] == i) {
        a->diag[i] = j;
        break;
      }
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatGetRow_MPIAdj"
PetscErrorCode MatGetRow_MPIAdj(Mat A,PetscInt row,PetscInt *nz,PetscInt **idx,PetscScalar **v)
{
  Mat_MPIAdj *a = (Mat_MPIAdj*)A->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  row -= A->rmap->rstart;
  if (row < 0 || row >= A->rmap->n) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Row out of range");
  *nz = a->i[row+1] - a->i[row];
  if (v) {
    PetscInt j;
    if (a->rowvalues_alloc < *nz) {
      ierr = PetscFree(a->rowvalues);CHKERRQ(ierr);
      a->rowvalues_alloc = PetscMax(a->rowvalues_alloc*2, *nz);
      ierr = PetscMalloc1(a->rowvalues_alloc,&a->rowvalues);CHKERRQ(ierr);
    }
    for (j=0; j<*nz; j++){
      a->rowvalues[j] = a->values ? a->values[a->i[row]+j]:1.0;
    }
    *v = (*nz) ? a->rowvalues : NULL;
  }
  if (idx) *idx = (*nz) ? a->j + a->i[row] : NULL;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatRestoreRow_MPIAdj"
PetscErrorCode MatRestoreRow_MPIAdj(Mat A,PetscInt row,PetscInt *nz,PetscInt **idx,PetscScalar **v)
{
  PetscFunctionBegin;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatEqual_MPIAdj"
PetscErrorCode MatEqual_MPIAdj(Mat A,Mat B,PetscBool * flg)
{
  Mat_MPIAdj     *a = (Mat_MPIAdj*)A->data,*b = (Mat_MPIAdj*)B->data;
  PetscErrorCode ierr;
  PetscBool      flag;

  PetscFunctionBegin;
  /* If the  matrix dimensions are not equal,or no of nonzeros */
  if ((A->rmap->n != B->rmap->n) ||(a->nz != b->nz)) {
    flag = PETSC_FALSE;
  }

  /* if the a->i are the same */
  ierr = PetscMemcmp(a->i,b->i,(A->rmap->n+1)*sizeof(PetscInt),&flag);CHKERRQ(ierr);

  /* if a->j are the same */
  ierr = PetscMemcmp(a->j,b->j,(a->nz)*sizeof(PetscInt),&flag);CHKERRQ(ierr);

  ierr = MPIU_Allreduce(&flag,flg,1,MPIU_BOOL,MPI_LAND,PetscObjectComm((PetscObject)A));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatGetRowIJ_MPIAdj"
PetscErrorCode MatGetRowIJ_MPIAdj(Mat A,PetscInt oshift,PetscBool symmetric,PetscBool blockcompressed,PetscInt *m,const PetscInt *inia[],const PetscInt *inja[],PetscBool  *done)
{
  PetscInt   i;
  Mat_MPIAdj *a   = (Mat_MPIAdj*)A->data;
  PetscInt   **ia = (PetscInt**)inia,**ja = (PetscInt**)inja;

  PetscFunctionBegin;
  *m    = A->rmap->n;
  *ia   = a->i;
  *ja   = a->j;
  *done = PETSC_TRUE;
  if (oshift) {
    for (i=0; i<(*ia)[*m]; i++) {
      (*ja)[i]++;
    }
    for (i=0; i<=(*m); i++) (*ia)[i]++;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatRestoreRowIJ_MPIAdj"
PetscErrorCode MatRestoreRowIJ_MPIAdj(Mat A,PetscInt oshift,PetscBool symmetric,PetscBool blockcompressed,PetscInt *m,const PetscInt *inia[],const PetscInt *inja[],PetscBool  *done)
{
  PetscInt   i;
  Mat_MPIAdj *a   = (Mat_MPIAdj*)A->data;
  PetscInt   **ia = (PetscInt**)inia,**ja = (PetscInt**)inja;

  PetscFunctionBegin;
  if (ia && a->i != *ia) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONGSTATE,"ia passed back is not one obtained with MatGetRowIJ()");
  if (ja && a->j != *ja) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONGSTATE,"ja passed back is not one obtained with MatGetRowIJ()");
  if (oshift) {
    for (i=0; i<=(*m); i++) (*ia)[i]--;
    for (i=0; i<(*ia)[*m]; i++) {
      (*ja)[i]--;
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatConvertFrom_MPIAdj"
PetscErrorCode  MatConvertFrom_MPIAdj(Mat A,MatType type,MatReuse reuse,Mat *newmat)
{
  Mat               B;
  PetscErrorCode    ierr;
  PetscInt          i,m,N,nzeros = 0,*ia,*ja,len,rstart,cnt,j,*a;
  const PetscInt    *rj;
  const PetscScalar *ra;
  MPI_Comm          comm;

  PetscFunctionBegin;
  ierr = MatGetSize(A,NULL,&N);CHKERRQ(ierr);
  ierr = MatGetLocalSize(A,&m,NULL);CHKERRQ(ierr);
  ierr = MatGetOwnershipRange(A,&rstart,NULL);CHKERRQ(ierr);

  /* count the number of nonzeros per row */
  for (i=0; i<m; i++) {
    ierr = MatGetRow(A,i+rstart,&len,&rj,NULL);CHKERRQ(ierr);
    for (j=0; j<len; j++) {
      if (rj[j] == i+rstart) {len--; break;}    /* don't count diagonal */
    }
    nzeros += len;
    ierr    = MatRestoreRow(A,i+rstart,&len,&rj,NULL);CHKERRQ(ierr);
  }

  /* malloc space for nonzeros */
  ierr = PetscMalloc1(nzeros+1,&a);CHKERRQ(ierr);
  ierr = PetscMalloc1(N+1,&ia);CHKERRQ(ierr);
  ierr = PetscMalloc1(nzeros+1,&ja);CHKERRQ(ierr);

  nzeros = 0;
  ia[0]  = 0;
  for (i=0; i<m; i++) {
    ierr = MatGetRow(A,i+rstart,&len,&rj,&ra);CHKERRQ(ierr);
    cnt  = 0;
    for (j=0; j<len; j++) {
      if (rj[j] != i+rstart) { /* if not diagonal */
        a[nzeros+cnt]    = (PetscInt) PetscAbsScalar(ra[j]);
        ja[nzeros+cnt++] = rj[j];
      }
    }
    ierr    = MatRestoreRow(A,i+rstart,&len,&rj,&ra);CHKERRQ(ierr);
    nzeros += cnt;
    ia[i+1] = nzeros;
  }

  ierr = PetscObjectGetComm((PetscObject)A,&comm);CHKERRQ(ierr);
  ierr = MatCreate(comm,&B);CHKERRQ(ierr);
  ierr = MatSetSizes(B,m,PETSC_DETERMINE,PETSC_DETERMINE,N);CHKERRQ(ierr);
  ierr = MatSetType(B,type);CHKERRQ(ierr);
  ierr = MatMPIAdjSetPreallocation(B,ia,ja,a);CHKERRQ(ierr);

  if (reuse == MAT_INPLACE_MATRIX) {
    ierr = MatHeaderReplace(A,&B);CHKERRQ(ierr);
  } else {
    *newmat = B;
  }
  PetscFunctionReturn(0);
}

/* -------------------------------------------------------------------*/
static struct _MatOps MatOps_Values = {0,
                                       MatGetRow_MPIAdj,
                                       MatRestoreRow_MPIAdj,
                                       0,
                                /* 4*/ 0,
                                       0,
                                       0,
                                       0,
                                       0,
                                       0,
                                /*10*/ 0,
                                       0,
                                       0,
                                       0,
                                       0,
                                /*15*/ 0,
                                       MatEqual_MPIAdj,
                                       0,
                                       0,
                                       0,
                                /*20*/ 0,
                                       0,
                                       MatSetOption_MPIAdj,
                                       0,
                                /*24*/ 0,
                                       0,
                                       0,
                                       0,
                                       0,
                                /*29*/ 0,
                                       0,
                                       0,
                                       0,
                                       0,
                                /*34*/ 0,
                                       0,
                                       0,
                                       0,
                                       0,
                                /*39*/ 0,
                                       MatGetSubMatrices_MPIAdj,
                                       0,
                                       0,
                                       0,
                                /*44*/ 0,
                                       0,
                                       MatShift_Basic,
                                       0,
                                       0,
                                /*49*/ 0,
                                       MatGetRowIJ_MPIAdj,
                                       MatRestoreRowIJ_MPIAdj,
                                       0,
                                       0,
                                /*54*/ 0,
                                       0,
                                       0,
                                       0,
                                       0,
                                /*59*/ 0,
                                       MatDestroy_MPIAdj,
                                       MatView_MPIAdj,
                                       MatConvertFrom_MPIAdj,
                                       0,
                                /*64*/ 0,
                                       0,
                                       0,
                                       0,
                                       0,
                                /*69*/ 0,
                                       0,
                                       0,
                                       0,
                                       0,
                                /*74*/ 0,
                                       0,
                                       0,
                                       0,
                                       0,
                                /*79*/ 0,
                                       0,
                                       0,
                                       0,
                                       0,
                                /*84*/ 0,
                                       0,
                                       0,
                                       0,
                                       0,
                                /*89*/ 0,
                                       0,
                                       0,
                                       0,
                                       0,
                                /*94*/ 0,
                                       0,
                                       0,
                                       0,
                                       0,
                                /*99*/ 0,
                                       0,
                                       0,
                                       0,
                                       0,
                               /*104*/ 0,
                                       0,
                                       0,
                                       0,
                                       0,
                               /*109*/ 0,
                                       0,
                                       0,
                                       0,
                                       0,
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
                                       0,
                                       0,
                                       0,
                                       MatGetSubMatricesMPI_MPIAdj,
                               /*129*/ 0,
                                       0,
                                       0,
                                       0,
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
#define __FUNCT__ "MatMPIAdjSetPreallocation_MPIAdj"
static PetscErrorCode  MatMPIAdjSetPreallocation_MPIAdj(Mat B,PetscInt *i,PetscInt *j,PetscInt *values)
{
  Mat_MPIAdj     *b = (Mat_MPIAdj*)B->data;
  PetscErrorCode ierr;
#if defined(PETSC_USE_DEBUG)
  PetscInt ii;
#endif

  PetscFunctionBegin;
  ierr = PetscLayoutSetUp(B->rmap);CHKERRQ(ierr);
  ierr = PetscLayoutSetUp(B->cmap);CHKERRQ(ierr);

#if defined(PETSC_USE_DEBUG)
  if (i[0] != 0) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"First i[] index must be zero, instead it is %D\n",i[0]);
  for (ii=1; ii<B->rmap->n; ii++) {
    if (i[ii] < 0 || i[ii] < i[ii-1]) SETERRQ4(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"i[%D]=%D index is out of range: i[%D]=%D",ii,i[ii],ii-1,i[ii-1]);
  }
  for (ii=0; ii<i[B->rmap->n]; ii++) {
    if (j[ii] < 0 || j[ii] >= B->cmap->N) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Column index %D out of range %D\n",ii,j[ii]);
  }
#endif
  B->preallocated = PETSC_TRUE;

  b->j      = j;
  b->i      = i;
  b->values = values;

  b->nz        = i[B->rmap->n];
  b->diag      = 0;
  b->symmetric = PETSC_FALSE;
  b->freeaij   = PETSC_TRUE;

  ierr = MatAssemblyBegin(B,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(B,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

static PetscErrorCode MatGetSubMatrix_MPIAdj_data(Mat,IS,IS, PetscInt **,PetscInt **,PetscInt **);
static PetscErrorCode MatGetSubMatrices_MPIAdj_Private(Mat,PetscInt,const IS[],const IS[],PetscBool,MatReuse,Mat **);

#undef __FUNCT__
#define __FUNCT__ "MatGetSubMatricesMPI_MPIAdj"
PetscErrorCode MatGetSubMatricesMPI_MPIAdj(Mat mat,PetscInt n, const IS irow[],const IS icol[],MatReuse scall,Mat *submat[])
{
  PetscErrorCode     ierr;
  /*get sub-matrices across a sub communicator */
  PetscFunctionBegin;
  ierr = MatGetSubMatrices_MPIAdj_Private(mat,n,irow,icol,PETSC_TRUE,scall,submat);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "MatGetSubMatrices_MPIAdj"
PetscErrorCode MatGetSubMatrices_MPIAdj(Mat mat,PetscInt n,const IS irow[],const IS icol[],MatReuse scall,Mat *submat[])
{
  PetscErrorCode     ierr;

  PetscFunctionBegin;
  /*get sub-matrices based on PETSC_COMM_SELF */
  ierr = MatGetSubMatrices_MPIAdj_Private(mat,n,irow,icol,PETSC_FALSE,scall,submat);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatGetSubMatrices_MPIAdj_Private"
static PetscErrorCode MatGetSubMatrices_MPIAdj_Private(Mat mat,PetscInt n,const IS irow[],const IS icol[],PetscBool subcomm,MatReuse scall,Mat *submat[])
{
  PetscInt           i,irow_n,icol_n,*sxadj,*sadjncy,*svalues;
  PetscInt          *indices,nindx,j,k,loc;
  PetscMPIInt        issame;
  const PetscInt    *irow_indices,*icol_indices;
  MPI_Comm           scomm_row,scomm_col,scomm_mat;
  PetscErrorCode     ierr;

  PetscFunctionBegin;
  nindx = 0;
  /*
   * Estimate a maximum number for allocating memory
   */
  for(i=0; i<n; i++){
    ierr = ISGetLocalSize(irow[i],&irow_n);CHKERRQ(ierr);
    ierr = ISGetLocalSize(icol[i],&icol_n);CHKERRQ(ierr);
    nindx = nindx>(irow_n+icol_n)? nindx:(irow_n+icol_n);
  }
  ierr = PetscCalloc1(nindx,&indices);CHKERRQ(ierr);
  /* construct a submat */
  for(i=0; i<n; i++){
	/*comms */
    if(subcomm){
	  ierr = PetscObjectGetComm((PetscObject)irow[i],&scomm_row);CHKERRQ(ierr);
	  ierr = PetscObjectGetComm((PetscObject)icol[i],&scomm_col);CHKERRQ(ierr);
	  ierr = MPI_Comm_compare(scomm_row,scomm_col,&issame);CHKERRQ(ierr);
	  if(issame != MPI_IDENT) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_INCOMP,"row index set must have the same comm as the col index set\n");
	  ierr = MPI_Comm_compare(scomm_row,PETSC_COMM_SELF,&issame);CHKERRQ(ierr);
	  if(issame == MPI_IDENT) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_INCOMP," can not use PETSC_COMM_SELF as comm when extracting a parallel submatrix\n");
	}else{
	  scomm_row = PETSC_COMM_SELF;
	}
	/*get sub-matrix data*/
	sxadj=0; sadjncy=0; svalues=0;
    ierr = MatGetSubMatrix_MPIAdj_data(mat,irow[i],icol[i],&sxadj,&sadjncy,&svalues);CHKERRQ(ierr);
    ierr = ISGetLocalSize(irow[i],&irow_n);CHKERRQ(ierr);
    ierr = ISGetLocalSize(icol[i],&icol_n);CHKERRQ(ierr);
    ierr = ISGetIndices(irow[i],&irow_indices);CHKERRQ(ierr);
    ierr = PetscMemcpy(indices,irow_indices,sizeof(PetscInt)*irow_n);CHKERRQ(ierr);
    ierr = ISRestoreIndices(irow[i],&irow_indices);CHKERRQ(ierr);
    ierr = ISGetIndices(icol[i],&icol_indices);CHKERRQ(ierr);
    ierr = PetscMemcpy(indices+irow_n,icol_indices,sizeof(PetscInt)*icol_n);CHKERRQ(ierr);
    ierr = ISRestoreIndices(icol[i],&icol_indices);CHKERRQ(ierr);
    nindx = irow_n+icol_n;
    ierr = PetscSortRemoveDupsInt(&nindx,indices);CHKERRQ(ierr);
    /* renumber columns */
    for(j=0; j<irow_n; j++){
      for(k=sxadj[j]; k<sxadj[j+1]; k++){
    	ierr = PetscFindInt(sadjncy[k],nindx,indices,&loc);CHKERRQ(ierr);
#if PETSC_USE_DEBUG
    	if(loc<0) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONG,"can not find col %d \n",sadjncy[k]);
#endif
        sadjncy[k] = loc;
      }
    }
    if(scall==MAT_INITIAL_MATRIX){
      ierr = MatCreateMPIAdj(scomm_row,irow_n,icol_n,sxadj,sadjncy,svalues,submat[i]);CHKERRQ(ierr);
    }else{
       Mat                sadj = *(submat[i]);
       Mat_MPIAdj         *sa  = (Mat_MPIAdj*)((sadj)->data);
       ierr = PetscObjectGetComm((PetscObject)sadj,&scomm_mat);CHKERRQ(ierr);
       ierr = MPI_Comm_compare(scomm_row,scomm_mat,&issame);CHKERRQ(ierr);
       if(issame != MPI_IDENT) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_INCOMP,"submatrix  must have the same comm as the col index set\n");
       ierr = PetscMemcpy(sa->i,sxadj,sizeof(PetscInt)*(irow_n+1));CHKERRQ(ierr);
       ierr = PetscMemcpy(sa->j,sadjncy,sizeof(PetscInt)*sxadj[irow_n]);CHKERRQ(ierr);
       if(svalues){ierr = PetscMemcpy(sa->values,svalues,sizeof(PetscInt)*sxadj[irow_n]);CHKERRQ(ierr);}
       ierr = PetscFree(sxadj);CHKERRQ(ierr);
       ierr = PetscFree(sadjncy);CHKERRQ(ierr);
       if(svalues) {ierr = PetscFree(svalues);CHKERRQ(ierr);}
    }
  }
  ierr = PetscFree(indices);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "MatGetSubMatrix_MPIAdj_data"
/*
 * The interface should be easy to use for both MatGetSubMatrix (parallel sub-matrix) and MatGetSubMatrices (sequential sub-matrices)
 * */
static PetscErrorCode MatGetSubMatrix_MPIAdj_data(Mat adj,IS irows, IS icols, PetscInt **sadj_xadj,PetscInt **sadj_adjncy,PetscInt **sadj_values)
{
  PetscInt        	 nlrows_is,icols_n,i,j,nroots,nleaves,owner,rlocalindex,*ncols_send,*ncols_recv;
  PetscInt           nlrows_mat,*adjncy_recv,Ncols_recv,Ncols_send,*xadj_recv,*values_recv;
  PetscInt          *ncols_recv_offsets,loc,rnclos,*sadjncy,*sxadj,*svalues,isvalue;
  const PetscInt    *irows_indices,*icols_indices,*xadj, *adjncy;
  Mat_MPIAdj        *a = (Mat_MPIAdj*)adj->data;
  PetscLayout        rmap;
  MPI_Comm           comm;
  PetscSF            sf;
  PetscSFNode       *iremote;
  PetscBool          done;
  PetscErrorCode     ierr;

  PetscFunctionBegin;
  /* communicator */
  ierr = PetscObjectGetComm((PetscObject)adj,&comm);CHKERRQ(ierr);
  /* Layouts */
  ierr = MatGetLayouts(adj,&rmap,PETSC_NULL);CHKERRQ(ierr);
  /* get rows information */
  ierr = ISGetLocalSize(irows,&nlrows_is);CHKERRQ(ierr);
  ierr = ISGetIndices(irows,&irows_indices);CHKERRQ(ierr);
  ierr = PetscCalloc1(nlrows_is,&iremote);CHKERRQ(ierr);
  /* construct sf graph*/
  nleaves = nlrows_is;
  for(i=0; i<nlrows_is; i++){
	owner = -1;
	rlocalindex = -1;
    ierr = PetscLayoutFindOwnerIndex(rmap,irows_indices[i],&owner,&rlocalindex);CHKERRQ(ierr);
    iremote[i].rank  = owner;
    iremote[i].index = rlocalindex;
  }
  ierr = MatGetRowIJ(adj,0,PETSC_FALSE,PETSC_FALSE,&nlrows_mat,&xadj,&adjncy,&done);CHKERRQ(ierr);
  ierr = PetscCalloc4(nlrows_mat,&ncols_send,nlrows_is,&xadj_recv,nlrows_is+1,&ncols_recv_offsets,nlrows_is,&ncols_recv);CHKERRQ(ierr);
  nroots = nlrows_mat;
  for(i=0; i<nlrows_mat; i++){
	ncols_send[i] = xadj[i+1]-xadj[i];
  }
  ierr = PetscSFCreate(comm,&sf);CHKERRQ(ierr);
  ierr = PetscSFSetGraph(sf,nroots,nleaves,PETSC_NULL,PETSC_OWN_POINTER,iremote,PETSC_OWN_POINTER);CHKERRQ(ierr);
  ierr = PetscSFSetType(sf,PETSCSFBASIC);CHKERRQ(ierr);
  ierr = PetscSFSetFromOptions(sf);CHKERRQ(ierr);
  ierr = PetscSFBcastBegin(sf,MPIU_INT,ncols_send,ncols_recv);CHKERRQ(ierr);
  ierr = PetscSFBcastEnd(sf,MPIU_INT,ncols_send,ncols_recv);CHKERRQ(ierr);
  ierr = PetscSFBcastBegin(sf,MPIU_INT,xadj,xadj_recv);CHKERRQ(ierr);
  ierr = PetscSFBcastEnd(sf,MPIU_INT,xadj,xadj_recv);CHKERRQ(ierr);
  ierr = PetscSFDestroy(&sf);CHKERRQ(ierr);
  Ncols_recv =0;
  for(i=0; i<nlrows_is; i++){
	 Ncols_recv             += ncols_recv[i];
	 ncols_recv_offsets[i+1] = ncols_recv[i]+ncols_recv_offsets[i];
  }
  Ncols_send = 0;
  for(i=0; i<nlrows_mat; i++){
	Ncols_send += ncols_send[i];
  }
  ierr = PetscCalloc1(Ncols_recv,&iremote);CHKERRQ(ierr);
  ierr = PetscCalloc1(Ncols_recv,&adjncy_recv);CHKERRQ(ierr);
  nleaves = Ncols_recv;
  Ncols_recv = 0;
  for(i=0; i<nlrows_is; i++){
    ierr = PetscLayoutFindOwner(rmap,irows_indices[i],&owner);CHKERRQ(ierr);
    for(j=0; j<ncols_recv[i]; j++){
      iremote[Ncols_recv].rank    = owner;
      iremote[Ncols_recv++].index = xadj_recv[i]+j;
    }
  }
  ierr = ISRestoreIndices(irows,&irows_indices);CHKERRQ(ierr);
  /*if we need to deal with edge weights ???*/
  if(a->values){isvalue=1;}else{isvalue=0;}
  /*involve a global communication */
  /*ierr = MPIU_Allreduce(&isvalue,&isvalue,1,MPIU_INT,MPI_SUM,comm);CHKERRQ(ierr);*/
  if(isvalue){ierr = PetscCalloc1(Ncols_recv,&values_recv);CHKERRQ(ierr);}
  nroots = Ncols_send;
  ierr = PetscSFCreate(comm,&sf);CHKERRQ(ierr);
  ierr = PetscSFSetGraph(sf,nroots,nleaves,PETSC_NULL,PETSC_OWN_POINTER,iremote,PETSC_OWN_POINTER);CHKERRQ(ierr);
  ierr = PetscSFSetType(sf,PETSCSFBASIC);CHKERRQ(ierr);
  ierr = PetscSFSetFromOptions(sf);CHKERRQ(ierr);
  ierr = PetscSFBcastBegin(sf,MPIU_INT,adjncy,adjncy_recv);CHKERRQ(ierr);
  ierr = PetscSFBcastEnd(sf,MPIU_INT,adjncy,adjncy_recv);CHKERRQ(ierr);
  if(isvalue){
	ierr = PetscSFBcastBegin(sf,MPIU_INT,a->values,values_recv);CHKERRQ(ierr);
	ierr = PetscSFBcastEnd(sf,MPIU_INT,a->values,values_recv);CHKERRQ(ierr);
  }
  ierr = PetscSFDestroy(&sf);CHKERRQ(ierr);
  ierr = MatRestoreRowIJ(adj,0,PETSC_FALSE,PETSC_FALSE,&nlrows_mat,&xadj,&adjncy,&done);CHKERRQ(ierr);
  ierr = ISGetLocalSize(icols,&icols_n);CHKERRQ(ierr);
  ierr = ISGetIndices(icols,&icols_indices);CHKERRQ(ierr);
  rnclos = 0;
  for(i=0; i<nlrows_is; i++){
    for(j=ncols_recv_offsets[i]; j<ncols_recv_offsets[i+1]; j++){
      ierr = PetscFindInt(adjncy_recv[j], icols_n, icols_indices, &loc);CHKERRQ(ierr);
      if(loc<0){
        adjncy_recv[j] = -1;
        if(isvalue) values_recv[j] = -1;
        ncols_recv[i]--;
      }else{
    	rnclos++;
      }
    }
  }
  ierr = ISRestoreIndices(icols,&icols_indices);CHKERRQ(ierr);
  ierr = PetscCalloc1(rnclos,&sadjncy);CHKERRQ(ierr);
  if(isvalue) {ierr = PetscCalloc1(rnclos,&svalues);CHKERRQ(ierr);}
  ierr = PetscCalloc1(nlrows_is+1,&sxadj);CHKERRQ(ierr);
  rnclos = 0;
  for(i=0; i<nlrows_is; i++){
	for(j=ncols_recv_offsets[i]; j<ncols_recv_offsets[i+1]; j++){
	  if(adjncy_recv[j]<0) continue;
	  sadjncy[rnclos] = adjncy_recv[j];
	  if(isvalue) svalues[rnclos] = values_recv[j];
	  rnclos++;
	}
  }
  for(i=0; i<nlrows_is; i++){
	sxadj[i+1] = sxadj[i]+ncols_recv[i];
  }
  if(sadj_xadj)  { *sadj_xadj = sxadj;}else    { ierr = PetscFree(sxadj);CHKERRQ(ierr);}
  if(sadj_adjncy){ *sadj_adjncy = sadjncy;}else{ ierr = PetscFree(sadjncy);CHKERRQ(ierr);}
  if(sadj_values){
	if(isvalue) *sadj_values = svalues; else *sadj_values=0;
  }else{
	if(isvalue) {ierr = PetscFree(svalues);CHKERRQ(ierr);}
  }
  ierr = PetscFree4(ncols_send,xadj_recv,ncols_recv_offsets,ncols_recv);CHKERRQ(ierr);
  ierr = PetscFree(adjncy_recv);CHKERRQ(ierr);
  if(isvalue) {ierr = PetscFree(values_recv);CHKERRQ(ierr);}
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "MatMPIAdjCreateNonemptySubcommMat_MPIAdj"
static PetscErrorCode MatMPIAdjCreateNonemptySubcommMat_MPIAdj(Mat A,Mat *B)
{
  Mat_MPIAdj     *a = (Mat_MPIAdj*)A->data;
  PetscErrorCode ierr;
  const PetscInt *ranges;
  MPI_Comm       acomm,bcomm;
  MPI_Group      agroup,bgroup;
  PetscMPIInt    i,rank,size,nranks,*ranks;

  PetscFunctionBegin;
  *B    = NULL;
  ierr  = PetscObjectGetComm((PetscObject)A,&acomm);CHKERRQ(ierr);
  ierr  = MPI_Comm_size(acomm,&size);CHKERRQ(ierr);
  ierr  = MPI_Comm_size(acomm,&rank);CHKERRQ(ierr);
  ierr  = MatGetOwnershipRanges(A,&ranges);CHKERRQ(ierr);
  for (i=0,nranks=0; i<size; i++) {
    if (ranges[i+1] - ranges[i] > 0) nranks++;
  }
  if (nranks == size) {         /* All ranks have a positive number of rows, so we do not need to create a subcomm; */
    ierr = PetscObjectReference((PetscObject)A);CHKERRQ(ierr);
    *B   = A;
    PetscFunctionReturn(0);
  }

  ierr = PetscMalloc1(nranks,&ranks);CHKERRQ(ierr);
  for (i=0,nranks=0; i<size; i++) {
    if (ranges[i+1] - ranges[i] > 0) ranks[nranks++] = i;
  }
  ierr = MPI_Comm_group(acomm,&agroup);CHKERRQ(ierr);
  ierr = MPI_Group_incl(agroup,nranks,ranks,&bgroup);CHKERRQ(ierr);
  ierr = PetscFree(ranks);CHKERRQ(ierr);
  ierr = MPI_Comm_create(acomm,bgroup,&bcomm);CHKERRQ(ierr);
  ierr = MPI_Group_free(&agroup);CHKERRQ(ierr);
  ierr = MPI_Group_free(&bgroup);CHKERRQ(ierr);
  if (bcomm != MPI_COMM_NULL) {
    PetscInt   m,N;
    Mat_MPIAdj *b;
    ierr       = MatGetLocalSize(A,&m,NULL);CHKERRQ(ierr);
    ierr       = MatGetSize(A,NULL,&N);CHKERRQ(ierr);
    ierr       = MatCreateMPIAdj(bcomm,m,N,a->i,a->j,a->values,B);CHKERRQ(ierr);
    b          = (Mat_MPIAdj*)(*B)->data;
    b->freeaij = PETSC_FALSE;
    ierr       = MPI_Comm_free(&bcomm);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatMPIAdjCreateNonemptySubcommMat"
/*@
   MatMPIAdjCreateNonemptySubcommMat - create the same MPIAdj matrix on a subcommunicator containing only processes owning a positive number of rows

   Collective

   Input Arguments:
.  A - original MPIAdj matrix

   Output Arguments:
.  B - matrix on subcommunicator, NULL on ranks that owned zero rows of A

   Level: developer

   Note:
   This function is mostly useful for internal use by mesh partitioning packages that require that every process owns at least one row.

   The matrix B should be destroyed with MatDestroy(). The arrays are not copied, so B should be destroyed before A is destroyed.

.seealso: MatCreateMPIAdj()
@*/
PetscErrorCode MatMPIAdjCreateNonemptySubcommMat(Mat A,Mat *B)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(A,MAT_CLASSID,1);
  ierr = PetscUseMethod(A,"MatMPIAdjCreateNonemptySubcommMat_C",(Mat,Mat*),(A,B));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/*MC
   MATMPIADJ - MATMPIADJ = "mpiadj" - A matrix type to be used for distributed adjacency matrices,
   intended for use constructing orderings and partitionings.

  Level: beginner

.seealso: MatCreateMPIAdj
M*/

#undef __FUNCT__
#define __FUNCT__ "MatCreate_MPIAdj"
PETSC_EXTERN PetscErrorCode MatCreate_MPIAdj(Mat B)
{
  Mat_MPIAdj     *b;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr         = PetscNewLog(B,&b);CHKERRQ(ierr);
  B->data      = (void*)b;
  ierr         = PetscMemcpy(B->ops,&MatOps_Values,sizeof(struct _MatOps));CHKERRQ(ierr);
  B->assembled = PETSC_FALSE;

  ierr = PetscObjectComposeFunction((PetscObject)B,"MatMPIAdjSetPreallocation_C",MatMPIAdjSetPreallocation_MPIAdj);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)B,"MatMPIAdjCreateNonemptySubcommMat_C",MatMPIAdjCreateNonemptySubcommMat_MPIAdj);CHKERRQ(ierr);
  ierr = PetscObjectChangeTypeName((PetscObject)B,MATMPIADJ);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatMPIAdjSetPreallocation"
/*@C
   MatMPIAdjSetPreallocation - Sets the array used for storing the matrix elements

   Logically Collective on MPI_Comm

   Input Parameters:
+  A - the matrix
.  i - the indices into j for the start of each row
.  j - the column indices for each row (sorted for each row).
       The indices in i and j start with zero (NOT with one).
-  values - [optional] edge weights

   Level: intermediate

.seealso: MatCreate(), MatCreateMPIAdj(), MatSetValues()
@*/
PetscErrorCode  MatMPIAdjSetPreallocation(Mat B,PetscInt *i,PetscInt *j,PetscInt *values)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscTryMethod(B,"MatMPIAdjSetPreallocation_C",(Mat,PetscInt*,PetscInt*,PetscInt*),(B,i,j,values));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatCreateMPIAdj"
/*@C
   MatCreateMPIAdj - Creates a sparse matrix representing an adjacency list.
   The matrix does not have numerical values associated with it, but is
   intended for ordering (to reduce bandwidth etc) and partitioning.

   Collective on MPI_Comm

   Input Parameters:
+  comm - MPI communicator
.  m - number of local rows
.  N - number of global columns
.  i - the indices into j for the start of each row
.  j - the column indices for each row (sorted for each row).
       The indices in i and j start with zero (NOT with one).
-  values -[optional] edge weights

   Output Parameter:
.  A - the matrix

   Level: intermediate

   Notes: This matrix object does not support most matrix operations, include
   MatSetValues().
   You must NOT free the ii, values and jj arrays yourself. PETSc will free them
   when the matrix is destroyed; you must allocate them with PetscMalloc(). If you
    call from Fortran you need not create the arrays with PetscMalloc().
   Should not include the matrix diagonals.

   If you already have a matrix, you can create its adjacency matrix by a call
   to MatConvert, specifying a type of MATMPIADJ.

   Possible values for MatSetOption() - MAT_STRUCTURALLY_SYMMETRIC

.seealso: MatCreate(), MatConvert(), MatGetOrdering()
@*/
PetscErrorCode  MatCreateMPIAdj(MPI_Comm comm,PetscInt m,PetscInt N,PetscInt *i,PetscInt *j,PetscInt *values,Mat *A)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = MatCreate(comm,A);CHKERRQ(ierr);
  ierr = MatSetSizes(*A,m,PETSC_DETERMINE,PETSC_DETERMINE,N);CHKERRQ(ierr);
  ierr = MatSetType(*A,MATMPIADJ);CHKERRQ(ierr);
  ierr = MatMPIAdjSetPreallocation(*A,i,j,values);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}







