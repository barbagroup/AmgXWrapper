
/*
    Defines the basic matrix operations for the AIJ (compressed row)
  matrix storage format.
*/


#include <../src/mat/impls/aij/seq/aij.h>          /*I "petscmat.h" I*/
#include <petscblaslapack.h>
#include <petscbt.h>
#include <petsc/private/kernels/blocktranspose.h>

#undef __FUNCT__
#define __FUNCT__ "MatGetColumnNorms_SeqAIJ"
PetscErrorCode MatGetColumnNorms_SeqAIJ(Mat A,NormType type,PetscReal *norms)
{
  PetscErrorCode ierr;
  PetscInt       i,m,n;
  Mat_SeqAIJ     *aij = (Mat_SeqAIJ*)A->data;

  PetscFunctionBegin;
  ierr = MatGetSize(A,&m,&n);CHKERRQ(ierr);
  ierr = PetscMemzero(norms,n*sizeof(PetscReal));CHKERRQ(ierr);
  if (type == NORM_2) {
    for (i=0; i<aij->i[m]; i++) {
      norms[aij->j[i]] += PetscAbsScalar(aij->a[i]*aij->a[i]);
    }
  } else if (type == NORM_1) {
    for (i=0; i<aij->i[m]; i++) {
      norms[aij->j[i]] += PetscAbsScalar(aij->a[i]);
    }
  } else if (type == NORM_INFINITY) {
    for (i=0; i<aij->i[m]; i++) {
      norms[aij->j[i]] = PetscMax(PetscAbsScalar(aij->a[i]),norms[aij->j[i]]);
    }
  } else SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONG,"Unknown NormType");

  if (type == NORM_2) {
    for (i=0; i<n; i++) norms[i] = PetscSqrtReal(norms[i]);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatFindOffBlockDiagonalEntries_SeqAIJ"
PetscErrorCode MatFindOffBlockDiagonalEntries_SeqAIJ(Mat A,IS *is)
{
  Mat_SeqAIJ      *a  = (Mat_SeqAIJ*)A->data;
  PetscInt        i,m=A->rmap->n,cnt = 0, bs = A->rmap->bs;
  const PetscInt  *jj = a->j,*ii = a->i;
  PetscInt        *rows;
  PetscErrorCode  ierr;

  PetscFunctionBegin;
  for (i=0; i<m; i++) {
    if ((ii[i] != ii[i+1]) && ((jj[ii[i]] < bs*(i/bs)) || (jj[ii[i+1]-1] > bs*((i+bs)/bs)-1))) {
      cnt++;
    }
  }
  ierr = PetscMalloc1(cnt,&rows);CHKERRQ(ierr);
  cnt  = 0;
  for (i=0; i<m; i++) {
    if ((ii[i] != ii[i+1]) && ((jj[ii[i]] < bs*(i/bs)) || (jj[ii[i+1]-1] > bs*((i+bs)/bs)-1))) {
      rows[cnt] = i;
      cnt++;
    }
  }
  ierr = ISCreateGeneral(PETSC_COMM_SELF,cnt,rows,PETSC_OWN_POINTER,is);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatFindZeroDiagonals_SeqAIJ_Private"
PetscErrorCode MatFindZeroDiagonals_SeqAIJ_Private(Mat A,PetscInt *nrows,PetscInt **zrows)
{
  Mat_SeqAIJ      *a  = (Mat_SeqAIJ*)A->data;
  const MatScalar *aa = a->a;
  PetscInt        i,m=A->rmap->n,cnt = 0;
  const PetscInt  *jj = a->j,*diag;
  PetscInt        *rows;
  PetscErrorCode  ierr;

  PetscFunctionBegin;
  ierr = MatMarkDiagonal_SeqAIJ(A);CHKERRQ(ierr);
  diag = a->diag;
  for (i=0; i<m; i++) {
    if ((jj[diag[i]] != i) || (aa[diag[i]] == 0.0)) {
      cnt++;
    }
  }
  ierr = PetscMalloc1(cnt,&rows);CHKERRQ(ierr);
  cnt  = 0;
  for (i=0; i<m; i++) {
    if ((jj[diag[i]] != i) || (aa[diag[i]] == 0.0)) {
      rows[cnt++] = i;
    }
  }
  *nrows = cnt;
  *zrows = rows;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatFindZeroDiagonals_SeqAIJ"
PetscErrorCode MatFindZeroDiagonals_SeqAIJ(Mat A,IS *zrows)
{
  PetscInt       nrows,*rows;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  *zrows = NULL;
  ierr   = MatFindZeroDiagonals_SeqAIJ_Private(A,&nrows,&rows);CHKERRQ(ierr);
  ierr   = ISCreateGeneral(PetscObjectComm((PetscObject)A),nrows,rows,PETSC_OWN_POINTER,zrows);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatFindNonzeroRows_SeqAIJ"
PetscErrorCode MatFindNonzeroRows_SeqAIJ(Mat A,IS *keptrows)
{
  Mat_SeqAIJ      *a = (Mat_SeqAIJ*)A->data;
  const MatScalar *aa;
  PetscInt        m=A->rmap->n,cnt = 0;
  const PetscInt  *ii;
  PetscInt        n,i,j,*rows;
  PetscErrorCode  ierr;

  PetscFunctionBegin;
  *keptrows = 0;
  ii        = a->i;
  for (i=0; i<m; i++) {
    n = ii[i+1] - ii[i];
    if (!n) {
      cnt++;
      goto ok1;
    }
    aa = a->a + ii[i];
    for (j=0; j<n; j++) {
      if (aa[j] != 0.0) goto ok1;
    }
    cnt++;
ok1:;
  }
  if (!cnt) PetscFunctionReturn(0);
  ierr = PetscMalloc1(A->rmap->n-cnt,&rows);CHKERRQ(ierr);
  cnt  = 0;
  for (i=0; i<m; i++) {
    n = ii[i+1] - ii[i];
    if (!n) continue;
    aa = a->a + ii[i];
    for (j=0; j<n; j++) {
      if (aa[j] != 0.0) {
        rows[cnt++] = i;
        break;
      }
    }
  }
  ierr = ISCreateGeneral(PETSC_COMM_SELF,cnt,rows,PETSC_OWN_POINTER,keptrows);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatDiagonalSet_SeqAIJ"
PetscErrorCode  MatDiagonalSet_SeqAIJ(Mat Y,Vec D,InsertMode is)
{
  PetscErrorCode    ierr;
  Mat_SeqAIJ        *aij = (Mat_SeqAIJ*) Y->data;
  PetscInt          i,m = Y->rmap->n;
  const PetscInt    *diag;
  MatScalar         *aa = aij->a;
  const PetscScalar *v;
  PetscBool         missing;

  PetscFunctionBegin;
  if (Y->assembled) {
    ierr = MatMissingDiagonal_SeqAIJ(Y,&missing,NULL);CHKERRQ(ierr);
    if (!missing) {
      diag = aij->diag;
      ierr = VecGetArrayRead(D,&v);CHKERRQ(ierr);
      if (is == INSERT_VALUES) {
        for (i=0; i<m; i++) {
          aa[diag[i]] = v[i];
        }
      } else {
        for (i=0; i<m; i++) {
          aa[diag[i]] += v[i];
        }
      }
      ierr = VecRestoreArrayRead(D,&v);CHKERRQ(ierr);
      PetscFunctionReturn(0);
    }
    ierr = MatSeqAIJInvalidateDiagonal(Y);CHKERRQ(ierr);
  }
  ierr = MatDiagonalSet_Default(Y,D,is);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatGetRowIJ_SeqAIJ"
PetscErrorCode MatGetRowIJ_SeqAIJ(Mat A,PetscInt oshift,PetscBool symmetric,PetscBool inodecompressed,PetscInt *m,const PetscInt *ia[],const PetscInt *ja[],PetscBool  *done)
{
  Mat_SeqAIJ     *a = (Mat_SeqAIJ*)A->data;
  PetscErrorCode ierr;
  PetscInt       i,ishift;

  PetscFunctionBegin;
  *m = A->rmap->n;
  if (!ia) PetscFunctionReturn(0);
  ishift = 0;
  if (symmetric && !A->structurally_symmetric) {
    ierr = MatToSymmetricIJ_SeqAIJ(A->rmap->n,a->i,a->j,ishift,oshift,(PetscInt**)ia,(PetscInt**)ja);CHKERRQ(ierr);
  } else if (oshift == 1) {
    PetscInt *tia;
    PetscInt nz = a->i[A->rmap->n];
    /* malloc space and  add 1 to i and j indices */
    ierr = PetscMalloc1(A->rmap->n+1,&tia);CHKERRQ(ierr);
    for (i=0; i<A->rmap->n+1; i++) tia[i] = a->i[i] + 1;
    *ia = tia;
    if (ja) {
      PetscInt *tja;
      ierr = PetscMalloc1(nz+1,&tja);CHKERRQ(ierr);
      for (i=0; i<nz; i++) tja[i] = a->j[i] + 1;
      *ja = tja;
    }
  } else {
    *ia = a->i;
    if (ja) *ja = a->j;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatRestoreRowIJ_SeqAIJ"
PetscErrorCode MatRestoreRowIJ_SeqAIJ(Mat A,PetscInt oshift,PetscBool symmetric,PetscBool inodecompressed,PetscInt *n,const PetscInt *ia[],const PetscInt *ja[],PetscBool  *done)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (!ia) PetscFunctionReturn(0);
  if ((symmetric && !A->structurally_symmetric) || oshift == 1) {
    ierr = PetscFree(*ia);CHKERRQ(ierr);
    if (ja) {ierr = PetscFree(*ja);CHKERRQ(ierr);}
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatGetColumnIJ_SeqAIJ"
PetscErrorCode MatGetColumnIJ_SeqAIJ(Mat A,PetscInt oshift,PetscBool symmetric,PetscBool inodecompressed,PetscInt *nn,const PetscInt *ia[],const PetscInt *ja[],PetscBool  *done)
{
  Mat_SeqAIJ     *a = (Mat_SeqAIJ*)A->data;
  PetscErrorCode ierr;
  PetscInt       i,*collengths,*cia,*cja,n = A->cmap->n,m = A->rmap->n;
  PetscInt       nz = a->i[m],row,*jj,mr,col;

  PetscFunctionBegin;
  *nn = n;
  if (!ia) PetscFunctionReturn(0);
  if (symmetric) {
    ierr = MatToSymmetricIJ_SeqAIJ(A->rmap->n,a->i,a->j,0,oshift,(PetscInt**)ia,(PetscInt**)ja);CHKERRQ(ierr);
  } else {
    ierr = PetscCalloc1(n+1,&collengths);CHKERRQ(ierr);
    ierr = PetscMalloc1(n+1,&cia);CHKERRQ(ierr);
    ierr = PetscMalloc1(nz+1,&cja);CHKERRQ(ierr);
    jj   = a->j;
    for (i=0; i<nz; i++) {
      collengths[jj[i]]++;
    }
    cia[0] = oshift;
    for (i=0; i<n; i++) {
      cia[i+1] = cia[i] + collengths[i];
    }
    ierr = PetscMemzero(collengths,n*sizeof(PetscInt));CHKERRQ(ierr);
    jj   = a->j;
    for (row=0; row<m; row++) {
      mr = a->i[row+1] - a->i[row];
      for (i=0; i<mr; i++) {
        col = *jj++;

        cja[cia[col] + collengths[col]++ - oshift] = row + oshift;
      }
    }
    ierr = PetscFree(collengths);CHKERRQ(ierr);
    *ia  = cia; *ja = cja;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatRestoreColumnIJ_SeqAIJ"
PetscErrorCode MatRestoreColumnIJ_SeqAIJ(Mat A,PetscInt oshift,PetscBool symmetric,PetscBool inodecompressed,PetscInt *n,const PetscInt *ia[],const PetscInt *ja[],PetscBool  *done)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (!ia) PetscFunctionReturn(0);

  ierr = PetscFree(*ia);CHKERRQ(ierr);
  ierr = PetscFree(*ja);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/*
 MatGetColumnIJ_SeqAIJ_Color() and MatRestoreColumnIJ_SeqAIJ_Color() are customized from
 MatGetColumnIJ_SeqAIJ() and MatRestoreColumnIJ_SeqAIJ() by adding an output
 spidx[], index of a->a, to be used in MatTransposeColoringCreate_SeqAIJ() and MatFDColoringCreate_SeqXAIJ()
*/
#undef __FUNCT__
#define __FUNCT__ "MatGetColumnIJ_SeqAIJ_Color"
PetscErrorCode MatGetColumnIJ_SeqAIJ_Color(Mat A,PetscInt oshift,PetscBool symmetric,PetscBool inodecompressed,PetscInt *nn,const PetscInt *ia[],const PetscInt *ja[],PetscInt *spidx[],PetscBool  *done)
{
  Mat_SeqAIJ     *a = (Mat_SeqAIJ*)A->data;
  PetscErrorCode ierr;
  PetscInt       i,*collengths,*cia,*cja,n = A->cmap->n,m = A->rmap->n;
  PetscInt       nz = a->i[m],row,*jj,mr,col;
  PetscInt       *cspidx;

  PetscFunctionBegin;
  *nn = n;
  if (!ia) PetscFunctionReturn(0);

  ierr = PetscCalloc1(n+1,&collengths);CHKERRQ(ierr);
  ierr = PetscMalloc1(n+1,&cia);CHKERRQ(ierr);
  ierr = PetscMalloc1(nz+1,&cja);CHKERRQ(ierr);
  ierr = PetscMalloc1(nz+1,&cspidx);CHKERRQ(ierr);
  jj   = a->j;
  for (i=0; i<nz; i++) {
    collengths[jj[i]]++;
  }
  cia[0] = oshift;
  for (i=0; i<n; i++) {
    cia[i+1] = cia[i] + collengths[i];
  }
  ierr = PetscMemzero(collengths,n*sizeof(PetscInt));CHKERRQ(ierr);
  jj   = a->j;
  for (row=0; row<m; row++) {
    mr = a->i[row+1] - a->i[row];
    for (i=0; i<mr; i++) {
      col = *jj++;
      cspidx[cia[col] + collengths[col] - oshift] = a->i[row] + i; /* index of a->j */
      cja[cia[col] + collengths[col]++ - oshift]  = row + oshift;
    }
  }
  ierr   = PetscFree(collengths);CHKERRQ(ierr);
  *ia    = cia; *ja = cja;
  *spidx = cspidx;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatRestoreColumnIJ_SeqAIJ_Color"
PetscErrorCode MatRestoreColumnIJ_SeqAIJ_Color(Mat A,PetscInt oshift,PetscBool symmetric,PetscBool inodecompressed,PetscInt *n,const PetscInt *ia[],const PetscInt *ja[],PetscInt *spidx[],PetscBool  *done)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = MatRestoreColumnIJ_SeqAIJ(A,oshift,symmetric,inodecompressed,n,ia,ja,done);CHKERRQ(ierr);
  ierr = PetscFree(*spidx);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatSetValuesRow_SeqAIJ"
PetscErrorCode MatSetValuesRow_SeqAIJ(Mat A,PetscInt row,const PetscScalar v[])
{
  Mat_SeqAIJ     *a  = (Mat_SeqAIJ*)A->data;
  PetscInt       *ai = a->i;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscMemcpy(a->a+ai[row],v,(ai[row+1]-ai[row])*sizeof(PetscScalar));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/*
    MatSeqAIJSetValuesLocalFast - An optimized version of MatSetValuesLocal() for SeqAIJ matrices with several assumptions

      -   a single row of values is set with each call
      -   no row or column indices are negative or (in error) larger than the number of rows or columns
      -   the values are always added to the matrix, not set
      -   no new locations are introduced in the nonzero structure of the matrix

     This does NOT assume the global column indices are sorted

*/

#include <petsc/private/isimpl.h>
#undef __FUNCT__
#define __FUNCT__ "MatSeqAIJSetValuesLocalFast"
PetscErrorCode MatSeqAIJSetValuesLocalFast(Mat A,PetscInt m,const PetscInt im[],PetscInt n,const PetscInt in[],const PetscScalar v[],InsertMode is)
{
  Mat_SeqAIJ     *a = (Mat_SeqAIJ*)A->data;
  PetscInt       low,high,t,row,nrow,i,col,l;
  const PetscInt *rp,*ai = a->i,*ailen = a->ilen,*aj = a->j;
  PetscInt       lastcol = -1;
  MatScalar      *ap,value,*aa = a->a;
  const PetscInt *ridx = A->rmap->mapping->indices,*cidx = A->cmap->mapping->indices;

  row = ridx[im[0]];
  rp   = aj + ai[row];
  ap = aa + ai[row];
  nrow = ailen[row];
  low  = 0;
  high = nrow;
  for (l=0; l<n; l++) { /* loop over added columns */
    col = cidx[in[l]];
    value = v[l];

    if (col <= lastcol) low = 0;
    else high = nrow;
    lastcol = col;
    while (high-low > 5) {
      t = (low+high)/2;
      if (rp[t] > col) high = t;
      else low = t;
    }
    for (i=low; i<high; i++) {
      if (rp[i] == col) {
        ap[i] += value;
        low = i + 1;
        break;
      }
    }
  }
  return 0;
}

#undef __FUNCT__
#define __FUNCT__ "MatSetValues_SeqAIJ"
PetscErrorCode MatSetValues_SeqAIJ(Mat A,PetscInt m,const PetscInt im[],PetscInt n,const PetscInt in[],const PetscScalar v[],InsertMode is)
{
  Mat_SeqAIJ     *a = (Mat_SeqAIJ*)A->data;
  PetscInt       *rp,k,low,high,t,ii,row,nrow,i,col,l,rmax,N;
  PetscInt       *imax = a->imax,*ai = a->i,*ailen = a->ilen;
  PetscErrorCode ierr;
  PetscInt       *aj = a->j,nonew = a->nonew,lastcol = -1;
  MatScalar      *ap,value,*aa = a->a;
  PetscBool      ignorezeroentries = a->ignorezeroentries;
  PetscBool      roworiented       = a->roworiented;

  PetscFunctionBegin;
  for (k=0; k<m; k++) { /* loop over added rows */
    row = im[k];
    if (row < 0) continue;
#if defined(PETSC_USE_DEBUG)
    if (row >= A->rmap->n) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Row too large: row %D max %D",row,A->rmap->n-1);
#endif
    rp   = aj + ai[row]; ap = aa + ai[row];
    rmax = imax[row]; nrow = ailen[row];
    low  = 0;
    high = nrow;
    for (l=0; l<n; l++) { /* loop over added columns */
      if (in[l] < 0) continue;
#if defined(PETSC_USE_DEBUG)
      if (in[l] >= A->cmap->n) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Column too large: col %D max %D",in[l],A->cmap->n-1);
#endif
      col = in[l];
      if (roworiented) {
        value = v[l + k*n];
      } else {
        value = v[k + l*m];
      }
      if ((value == 0.0 && ignorezeroentries) && (is == ADD_VALUES)) continue;

      if (col <= lastcol) low = 0;
      else high = nrow;
      lastcol = col;
      while (high-low > 5) {
        t = (low+high)/2;
        if (rp[t] > col) high = t;
        else low = t;
      }
      for (i=low; i<high; i++) {
        if (rp[i] > col) break;
        if (rp[i] == col) {
          if (is == ADD_VALUES) ap[i] += value;
          else ap[i] = value;
          low = i + 1;
          goto noinsert;
        }
      }
      if (value == 0.0 && ignorezeroentries) goto noinsert;
      if (nonew == 1) goto noinsert;
      if (nonew == -1) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Inserting a new nonzero at (%D,%D) in the matrix",row,col);
      MatSeqXAIJReallocateAIJ(A,A->rmap->n,1,nrow,row,col,rmax,aa,ai,aj,rp,ap,imax,nonew,MatScalar);
      N = nrow++ - 1; a->nz++; high++;
      /* shift up all the later entries in this row */
      for (ii=N; ii>=i; ii--) {
        rp[ii+1] = rp[ii];
        ap[ii+1] = ap[ii];
      }
      rp[i] = col;
      ap[i] = value;
      low   = i + 1;
      A->nonzerostate++;
noinsert:;
    }
    ailen[row] = nrow;
  }
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "MatGetValues_SeqAIJ"
PetscErrorCode MatGetValues_SeqAIJ(Mat A,PetscInt m,const PetscInt im[],PetscInt n,const PetscInt in[],PetscScalar v[])
{
  Mat_SeqAIJ *a = (Mat_SeqAIJ*)A->data;
  PetscInt   *rp,k,low,high,t,row,nrow,i,col,l,*aj = a->j;
  PetscInt   *ai = a->i,*ailen = a->ilen;
  MatScalar  *ap,*aa = a->a;

  PetscFunctionBegin;
  for (k=0; k<m; k++) { /* loop over rows */
    row = im[k];
    if (row < 0) {v += n; continue;} /* SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Negative row: %D",row); */
    if (row >= A->rmap->n) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Row too large: row %D max %D",row,A->rmap->n-1);
    rp   = aj + ai[row]; ap = aa + ai[row];
    nrow = ailen[row];
    for (l=0; l<n; l++) { /* loop over columns */
      if (in[l] < 0) {v++; continue;} /* SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Negative column: %D",in[l]); */
      if (in[l] >= A->cmap->n) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Column too large: col %D max %D",in[l],A->cmap->n-1);
      col  = in[l];
      high = nrow; low = 0; /* assume unsorted */
      while (high-low > 5) {
        t = (low+high)/2;
        if (rp[t] > col) high = t;
        else low = t;
      }
      for (i=low; i<high; i++) {
        if (rp[i] > col) break;
        if (rp[i] == col) {
          *v++ = ap[i];
          goto finished;
        }
      }
      *v++ = 0.0;
finished:;
    }
  }
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "MatView_SeqAIJ_Binary"
PetscErrorCode MatView_SeqAIJ_Binary(Mat A,PetscViewer viewer)
{
  Mat_SeqAIJ     *a = (Mat_SeqAIJ*)A->data;
  PetscErrorCode ierr;
  PetscInt       i,*col_lens;
  int            fd;
  FILE           *file;

  PetscFunctionBegin;
  ierr = PetscViewerBinaryGetDescriptor(viewer,&fd);CHKERRQ(ierr);
  ierr = PetscMalloc1(4+A->rmap->n,&col_lens);CHKERRQ(ierr);

  col_lens[0] = MAT_FILE_CLASSID;
  col_lens[1] = A->rmap->n;
  col_lens[2] = A->cmap->n;
  col_lens[3] = a->nz;

  /* store lengths of each row and write (including header) to file */
  for (i=0; i<A->rmap->n; i++) {
    col_lens[4+i] = a->i[i+1] - a->i[i];
  }
  ierr = PetscBinaryWrite(fd,col_lens,4+A->rmap->n,PETSC_INT,PETSC_TRUE);CHKERRQ(ierr);
  ierr = PetscFree(col_lens);CHKERRQ(ierr);

  /* store column indices (zero start index) */
  ierr = PetscBinaryWrite(fd,a->j,a->nz,PETSC_INT,PETSC_FALSE);CHKERRQ(ierr);

  /* store nonzero values */
  ierr = PetscBinaryWrite(fd,a->a,a->nz,PETSC_SCALAR,PETSC_FALSE);CHKERRQ(ierr);

  ierr = PetscViewerBinaryGetInfoPointer(viewer,&file);CHKERRQ(ierr);
  if (file) {
    fprintf(file,"-matload_block_size %d\n",(int)PetscAbs(A->rmap->bs));
  }
  PetscFunctionReturn(0);
}

extern PetscErrorCode MatSeqAIJFactorInfo_Matlab(Mat,PetscViewer);

#undef __FUNCT__
#define __FUNCT__ "MatView_SeqAIJ_ASCII"
PetscErrorCode MatView_SeqAIJ_ASCII(Mat A,PetscViewer viewer)
{
  Mat_SeqAIJ        *a = (Mat_SeqAIJ*)A->data;
  PetscErrorCode    ierr;
  PetscInt          i,j,m = A->rmap->n;
  const char        *name;
  PetscViewerFormat format;

  PetscFunctionBegin;
  ierr = PetscViewerGetFormat(viewer,&format);CHKERRQ(ierr);
  if (format == PETSC_VIEWER_ASCII_MATLAB) {
    PetscInt nofinalvalue = 0;
    if (m && ((a->i[m] == a->i[m-1]) || (a->j[a->nz-1] != A->cmap->n-1))) {
      /* Need a dummy value to ensure the dimension of the matrix. */
      nofinalvalue = 1;
    }
    ierr = PetscViewerASCIIUseTabs(viewer,PETSC_FALSE);CHKERRQ(ierr);
    ierr = PetscViewerASCIIPrintf(viewer,"%% Size = %D %D \n",m,A->cmap->n);CHKERRQ(ierr);
    ierr = PetscViewerASCIIPrintf(viewer,"%% Nonzeros = %D \n",a->nz);CHKERRQ(ierr);
#if defined(PETSC_USE_COMPLEX)
    ierr = PetscViewerASCIIPrintf(viewer,"zzz = zeros(%D,4);\n",a->nz+nofinalvalue);CHKERRQ(ierr);
#else
    ierr = PetscViewerASCIIPrintf(viewer,"zzz = zeros(%D,3);\n",a->nz+nofinalvalue);CHKERRQ(ierr);
#endif
    ierr = PetscViewerASCIIPrintf(viewer,"zzz = [\n");CHKERRQ(ierr);

    for (i=0; i<m; i++) {
      for (j=a->i[i]; j<a->i[i+1]; j++) {
#if defined(PETSC_USE_COMPLEX)
        ierr = PetscViewerASCIIPrintf(viewer,"%D %D  %18.16e %18.16e\n",i+1,a->j[j]+1,(double)PetscRealPart(a->a[j]),(double)PetscImaginaryPart(a->a[j]));CHKERRQ(ierr);
#else
        ierr = PetscViewerASCIIPrintf(viewer,"%D %D  %18.16e\n",i+1,a->j[j]+1,(double)a->a[j]);CHKERRQ(ierr);
#endif
      }
    }
    if (nofinalvalue) {
#if defined(PETSC_USE_COMPLEX)
      ierr = PetscViewerASCIIPrintf(viewer,"%D %D  %18.16e %18.16e\n",m,A->cmap->n,0.,0.);CHKERRQ(ierr);
#else
      ierr = PetscViewerASCIIPrintf(viewer,"%D %D  %18.16e\n",m,A->cmap->n,0.0);CHKERRQ(ierr);
#endif
    }
    ierr = PetscObjectGetName((PetscObject)A,&name);CHKERRQ(ierr);
    ierr = PetscViewerASCIIPrintf(viewer,"];\n %s = spconvert(zzz);\n",name);CHKERRQ(ierr);
    ierr = PetscViewerASCIIUseTabs(viewer,PETSC_TRUE);CHKERRQ(ierr);
  } else if (format == PETSC_VIEWER_ASCII_FACTOR_INFO || format == PETSC_VIEWER_ASCII_INFO) {
    PetscFunctionReturn(0);
  } else if (format == PETSC_VIEWER_ASCII_COMMON) {
    ierr = PetscViewerASCIIUseTabs(viewer,PETSC_FALSE);CHKERRQ(ierr);
    for (i=0; i<m; i++) {
      ierr = PetscViewerASCIIPrintf(viewer,"row %D:",i);CHKERRQ(ierr);
      for (j=a->i[i]; j<a->i[i+1]; j++) {
#if defined(PETSC_USE_COMPLEX)
        if (PetscImaginaryPart(a->a[j]) > 0.0 && PetscRealPart(a->a[j]) != 0.0) {
          ierr = PetscViewerASCIIPrintf(viewer," (%D, %g + %g i)",a->j[j],(double)PetscRealPart(a->a[j]),(double)PetscImaginaryPart(a->a[j]));CHKERRQ(ierr);
        } else if (PetscImaginaryPart(a->a[j]) < 0.0 && PetscRealPart(a->a[j]) != 0.0) {
          ierr = PetscViewerASCIIPrintf(viewer," (%D, %g - %g i)",a->j[j],(double)PetscRealPart(a->a[j]),(double)-PetscImaginaryPart(a->a[j]));CHKERRQ(ierr);
        } else if (PetscRealPart(a->a[j]) != 0.0) {
          ierr = PetscViewerASCIIPrintf(viewer," (%D, %g) ",a->j[j],(double)PetscRealPart(a->a[j]));CHKERRQ(ierr);
        }
#else
        if (a->a[j] != 0.0) {ierr = PetscViewerASCIIPrintf(viewer," (%D, %g) ",a->j[j],(double)a->a[j]);CHKERRQ(ierr);}
#endif
      }
      ierr = PetscViewerASCIIPrintf(viewer,"\n");CHKERRQ(ierr);
    }
    ierr = PetscViewerASCIIUseTabs(viewer,PETSC_TRUE);CHKERRQ(ierr);
  } else if (format == PETSC_VIEWER_ASCII_SYMMODU) {
    PetscInt nzd=0,fshift=1,*sptr;
    ierr = PetscViewerASCIIUseTabs(viewer,PETSC_FALSE);CHKERRQ(ierr);
    ierr = PetscMalloc1(m+1,&sptr);CHKERRQ(ierr);
    for (i=0; i<m; i++) {
      sptr[i] = nzd+1;
      for (j=a->i[i]; j<a->i[i+1]; j++) {
        if (a->j[j] >= i) {
#if defined(PETSC_USE_COMPLEX)
          if (PetscImaginaryPart(a->a[j]) != 0.0 || PetscRealPart(a->a[j]) != 0.0) nzd++;
#else
          if (a->a[j] != 0.0) nzd++;
#endif
        }
      }
    }
    sptr[m] = nzd+1;
    ierr    = PetscViewerASCIIPrintf(viewer," %D %D\n\n",m,nzd);CHKERRQ(ierr);
    for (i=0; i<m+1; i+=6) {
      if (i+4<m) {
        ierr = PetscViewerASCIIPrintf(viewer," %D %D %D %D %D %D\n",sptr[i],sptr[i+1],sptr[i+2],sptr[i+3],sptr[i+4],sptr[i+5]);CHKERRQ(ierr);
      } else if (i+3<m) {
        ierr = PetscViewerASCIIPrintf(viewer," %D %D %D %D %D\n",sptr[i],sptr[i+1],sptr[i+2],sptr[i+3],sptr[i+4]);CHKERRQ(ierr);
      } else if (i+2<m) {
        ierr = PetscViewerASCIIPrintf(viewer," %D %D %D %D\n",sptr[i],sptr[i+1],sptr[i+2],sptr[i+3]);CHKERRQ(ierr);
      } else if (i+1<m) {
        ierr = PetscViewerASCIIPrintf(viewer," %D %D %D\n",sptr[i],sptr[i+1],sptr[i+2]);CHKERRQ(ierr);
      } else if (i<m) {
        ierr = PetscViewerASCIIPrintf(viewer," %D %D\n",sptr[i],sptr[i+1]);CHKERRQ(ierr);
      } else {
        ierr = PetscViewerASCIIPrintf(viewer," %D\n",sptr[i]);CHKERRQ(ierr);
      }
    }
    ierr = PetscViewerASCIIPrintf(viewer,"\n");CHKERRQ(ierr);
    ierr = PetscFree(sptr);CHKERRQ(ierr);
    for (i=0; i<m; i++) {
      for (j=a->i[i]; j<a->i[i+1]; j++) {
        if (a->j[j] >= i) {ierr = PetscViewerASCIIPrintf(viewer," %D ",a->j[j]+fshift);CHKERRQ(ierr);}
      }
      ierr = PetscViewerASCIIPrintf(viewer,"\n");CHKERRQ(ierr);
    }
    ierr = PetscViewerASCIIPrintf(viewer,"\n");CHKERRQ(ierr);
    for (i=0; i<m; i++) {
      for (j=a->i[i]; j<a->i[i+1]; j++) {
        if (a->j[j] >= i) {
#if defined(PETSC_USE_COMPLEX)
          if (PetscImaginaryPart(a->a[j]) != 0.0 || PetscRealPart(a->a[j]) != 0.0) {
            ierr = PetscViewerASCIIPrintf(viewer," %18.16e %18.16e ",(double)PetscRealPart(a->a[j]),(double)PetscImaginaryPart(a->a[j]));CHKERRQ(ierr);
          }
#else
          if (a->a[j] != 0.0) {ierr = PetscViewerASCIIPrintf(viewer," %18.16e ",(double)a->a[j]);CHKERRQ(ierr);}
#endif
        }
      }
      ierr = PetscViewerASCIIPrintf(viewer,"\n");CHKERRQ(ierr);
    }
    ierr = PetscViewerASCIIUseTabs(viewer,PETSC_TRUE);CHKERRQ(ierr);
  } else if (format == PETSC_VIEWER_ASCII_DENSE) {
    PetscInt    cnt = 0,jcnt;
    PetscScalar value;
#if defined(PETSC_USE_COMPLEX)
    PetscBool   realonly = PETSC_TRUE;

    for (i=0; i<a->i[m]; i++) {
      if (PetscImaginaryPart(a->a[i]) != 0.0) {
        realonly = PETSC_FALSE;
        break;
      }
    }
#endif

    ierr = PetscViewerASCIIUseTabs(viewer,PETSC_FALSE);CHKERRQ(ierr);
    for (i=0; i<m; i++) {
      jcnt = 0;
      for (j=0; j<A->cmap->n; j++) {
        if (jcnt < a->i[i+1]-a->i[i] && j == a->j[cnt]) {
          value = a->a[cnt++];
          jcnt++;
        } else {
          value = 0.0;
        }
#if defined(PETSC_USE_COMPLEX)
        if (realonly) {
          ierr = PetscViewerASCIIPrintf(viewer," %7.5e ",(double)PetscRealPart(value));CHKERRQ(ierr);
        } else {
          ierr = PetscViewerASCIIPrintf(viewer," %7.5e+%7.5e i ",(double)PetscRealPart(value),(double)PetscImaginaryPart(value));CHKERRQ(ierr);
        }
#else
        ierr = PetscViewerASCIIPrintf(viewer," %7.5e ",(double)value);CHKERRQ(ierr);
#endif
      }
      ierr = PetscViewerASCIIPrintf(viewer,"\n");CHKERRQ(ierr);
    }
    ierr = PetscViewerASCIIUseTabs(viewer,PETSC_TRUE);CHKERRQ(ierr);
  } else if (format == PETSC_VIEWER_ASCII_MATRIXMARKET) {
    PetscInt fshift=1;
    ierr = PetscViewerASCIIUseTabs(viewer,PETSC_FALSE);CHKERRQ(ierr);
#if defined(PETSC_USE_COMPLEX)
    ierr = PetscViewerASCIIPrintf(viewer,"%%%%MatrixMarket matrix coordinate complex general\n");CHKERRQ(ierr);
#else
    ierr = PetscViewerASCIIPrintf(viewer,"%%%%MatrixMarket matrix coordinate real general\n");CHKERRQ(ierr);
#endif
    ierr = PetscViewerASCIIPrintf(viewer,"%D %D %D\n", m, A->cmap->n, a->nz);CHKERRQ(ierr);
    for (i=0; i<m; i++) {
      for (j=a->i[i]; j<a->i[i+1]; j++) {
#if defined(PETSC_USE_COMPLEX)
        ierr = PetscViewerASCIIPrintf(viewer,"%D %D %g %g\n", i+fshift,a->j[j]+fshift,(double)PetscRealPart(a->a[j]),(double)PetscImaginaryPart(a->a[j]));CHKERRQ(ierr);
#else
        ierr = PetscViewerASCIIPrintf(viewer,"%D %D %g\n", i+fshift, a->j[j]+fshift, (double)a->a[j]);CHKERRQ(ierr);
#endif
      }
    }
    ierr = PetscViewerASCIIUseTabs(viewer,PETSC_TRUE);CHKERRQ(ierr);
  } else {
    ierr = PetscViewerASCIIUseTabs(viewer,PETSC_FALSE);CHKERRQ(ierr);
    if (A->factortype) {
      for (i=0; i<m; i++) {
        ierr = PetscViewerASCIIPrintf(viewer,"row %D:",i);CHKERRQ(ierr);
        /* L part */
        for (j=a->i[i]; j<a->i[i+1]; j++) {
#if defined(PETSC_USE_COMPLEX)
          if (PetscImaginaryPart(a->a[j]) > 0.0) {
            ierr = PetscViewerASCIIPrintf(viewer," (%D, %g + %g i)",a->j[j],(double)PetscRealPart(a->a[j]),(double)PetscImaginaryPart(a->a[j]));CHKERRQ(ierr);
          } else if (PetscImaginaryPart(a->a[j]) < 0.0) {
            ierr = PetscViewerASCIIPrintf(viewer," (%D, %g - %g i)",a->j[j],(double)PetscRealPart(a->a[j]),(double)(-PetscImaginaryPart(a->a[j])));CHKERRQ(ierr);
          } else {
            ierr = PetscViewerASCIIPrintf(viewer," (%D, %g) ",a->j[j],(double)PetscRealPart(a->a[j]));CHKERRQ(ierr);
          }
#else
          ierr = PetscViewerASCIIPrintf(viewer," (%D, %g) ",a->j[j],(double)a->a[j]);CHKERRQ(ierr);
#endif
        }
        /* diagonal */
        j = a->diag[i];
#if defined(PETSC_USE_COMPLEX)
        if (PetscImaginaryPart(a->a[j]) > 0.0) {
          ierr = PetscViewerASCIIPrintf(viewer," (%D, %g + %g i)",a->j[j],(double)PetscRealPart(1.0/a->a[j]),(double)PetscImaginaryPart(1.0/a->a[j]));CHKERRQ(ierr);
        } else if (PetscImaginaryPart(a->a[j]) < 0.0) {
          ierr = PetscViewerASCIIPrintf(viewer," (%D, %g - %g i)",a->j[j],(double)PetscRealPart(1.0/a->a[j]),(double)(-PetscImaginaryPart(1.0/a->a[j])));CHKERRQ(ierr);
        } else {
          ierr = PetscViewerASCIIPrintf(viewer," (%D, %g) ",a->j[j],(double)PetscRealPart(1.0/a->a[j]));CHKERRQ(ierr);
        }
#else
        ierr = PetscViewerASCIIPrintf(viewer," (%D, %g) ",a->j[j],(double)(1.0/a->a[j]));CHKERRQ(ierr);
#endif

        /* U part */
        for (j=a->diag[i+1]+1; j<a->diag[i]; j++) {
#if defined(PETSC_USE_COMPLEX)
          if (PetscImaginaryPart(a->a[j]) > 0.0) {
            ierr = PetscViewerASCIIPrintf(viewer," (%D, %g + %g i)",a->j[j],(double)PetscRealPart(a->a[j]),(double)PetscImaginaryPart(a->a[j]));CHKERRQ(ierr);
          } else if (PetscImaginaryPart(a->a[j]) < 0.0) {
            ierr = PetscViewerASCIIPrintf(viewer," (%D, %g - %g i)",a->j[j],(double)PetscRealPart(a->a[j]),(double)(-PetscImaginaryPart(a->a[j])));CHKERRQ(ierr);
          } else {
            ierr = PetscViewerASCIIPrintf(viewer," (%D, %g) ",a->j[j],(double)PetscRealPart(a->a[j]));CHKERRQ(ierr);
          }
#else
          ierr = PetscViewerASCIIPrintf(viewer," (%D, %g) ",a->j[j],(double)a->a[j]);CHKERRQ(ierr);
#endif
        }
        ierr = PetscViewerASCIIPrintf(viewer,"\n");CHKERRQ(ierr);
      }
    } else {
      for (i=0; i<m; i++) {
        ierr = PetscViewerASCIIPrintf(viewer,"row %D:",i);CHKERRQ(ierr);
        for (j=a->i[i]; j<a->i[i+1]; j++) {
#if defined(PETSC_USE_COMPLEX)
          if (PetscImaginaryPart(a->a[j]) > 0.0) {
            ierr = PetscViewerASCIIPrintf(viewer," (%D, %g + %g i)",a->j[j],(double)PetscRealPart(a->a[j]),(double)PetscImaginaryPart(a->a[j]));CHKERRQ(ierr);
          } else if (PetscImaginaryPart(a->a[j]) < 0.0) {
            ierr = PetscViewerASCIIPrintf(viewer," (%D, %g - %g i)",a->j[j],(double)PetscRealPart(a->a[j]),(double)-PetscImaginaryPart(a->a[j]));CHKERRQ(ierr);
          } else {
            ierr = PetscViewerASCIIPrintf(viewer," (%D, %g) ",a->j[j],(double)PetscRealPart(a->a[j]));CHKERRQ(ierr);
          }
#else
          ierr = PetscViewerASCIIPrintf(viewer," (%D, %g) ",a->j[j],(double)a->a[j]);CHKERRQ(ierr);
#endif
        }
        ierr = PetscViewerASCIIPrintf(viewer,"\n");CHKERRQ(ierr);
      }
    }
    ierr = PetscViewerASCIIUseTabs(viewer,PETSC_TRUE);CHKERRQ(ierr);
  }
  ierr = PetscViewerFlush(viewer);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#include <petscdraw.h>
#undef __FUNCT__
#define __FUNCT__ "MatView_SeqAIJ_Draw_Zoom"
PetscErrorCode MatView_SeqAIJ_Draw_Zoom(PetscDraw draw,void *Aa)
{
  Mat               A  = (Mat) Aa;
  Mat_SeqAIJ        *a = (Mat_SeqAIJ*)A->data;
  PetscErrorCode    ierr;
  PetscInt          i,j,m = A->rmap->n,color;
  PetscReal         xl,yl,xr,yr,x_l,x_r,y_l,y_r,maxv = 0.0;
  PetscViewer       viewer;
  PetscViewerFormat format;

  PetscFunctionBegin;
  ierr = PetscObjectQuery((PetscObject)A,"Zoomviewer",(PetscObject*)&viewer);CHKERRQ(ierr);
  ierr = PetscViewerGetFormat(viewer,&format);CHKERRQ(ierr);

  ierr = PetscDrawGetCoordinates(draw,&xl,&yl,&xr,&yr);CHKERRQ(ierr);
  /* loop over matrix elements drawing boxes */

  if (format != PETSC_VIEWER_DRAW_CONTOUR) {
    /* Blue for negative, Cyan for zero and  Red for positive */
    color = PETSC_DRAW_BLUE;
    for (i=0; i<m; i++) {
      y_l = m - i - 1.0; y_r = y_l + 1.0;
      for (j=a->i[i]; j<a->i[i+1]; j++) {
        x_l = a->j[j]; x_r = x_l + 1.0;
        if (PetscRealPart(a->a[j]) >=  0.) continue;
        ierr = PetscDrawRectangle(draw,x_l,y_l,x_r,y_r,color,color,color,color);CHKERRQ(ierr);
      }
    }
    color = PETSC_DRAW_CYAN;
    for (i=0; i<m; i++) {
      y_l = m - i - 1.0; y_r = y_l + 1.0;
      for (j=a->i[i]; j<a->i[i+1]; j++) {
        x_l = a->j[j]; x_r = x_l + 1.0;
        if (a->a[j] !=  0.) continue;
        ierr = PetscDrawRectangle(draw,x_l,y_l,x_r,y_r,color,color,color,color);CHKERRQ(ierr);
      }
    }
    color = PETSC_DRAW_RED;
    for (i=0; i<m; i++) {
      y_l = m - i - 1.0; y_r = y_l + 1.0;
      for (j=a->i[i]; j<a->i[i+1]; j++) {
        x_l = a->j[j]; x_r = x_l + 1.0;
        if (PetscRealPart(a->a[j]) <=  0.) continue;
        ierr = PetscDrawRectangle(draw,x_l,y_l,x_r,y_r,color,color,color,color);CHKERRQ(ierr);
      }
    }
  } else {
    /* use contour shading to indicate magnitude of values */
    /* first determine max of all nonzero values */
    PetscInt  nz = a->nz,count;
    PetscDraw popup;
    PetscReal scale;

    for (i=0; i<nz; i++) {
      if (PetscAbsScalar(a->a[i]) > maxv) maxv = PetscAbsScalar(a->a[i]);
    }
    scale = (245.0 - PETSC_DRAW_BASIC_COLORS)/maxv;
    ierr  = PetscDrawGetPopup(draw,&popup);CHKERRQ(ierr);
    if (popup) {
      ierr = PetscDrawScalePopup(popup,0.0,maxv);CHKERRQ(ierr);
    }
    count = 0;
    for (i=0; i<m; i++) {
      y_l = m - i - 1.0; y_r = y_l + 1.0;
      for (j=a->i[i]; j<a->i[i+1]; j++) {
        x_l   = a->j[j]; x_r = x_l + 1.0;
        color = PETSC_DRAW_BASIC_COLORS + (PetscInt)(scale*PetscAbsScalar(a->a[count]));
        ierr  = PetscDrawRectangle(draw,x_l,y_l,x_r,y_r,color,color,color,color);CHKERRQ(ierr);
        count++;
      }
    }
  }
  PetscFunctionReturn(0);
}

#include <petscdraw.h>
#undef __FUNCT__
#define __FUNCT__ "MatView_SeqAIJ_Draw"
PetscErrorCode MatView_SeqAIJ_Draw(Mat A,PetscViewer viewer)
{
  PetscErrorCode ierr;
  PetscDraw      draw;
  PetscReal      xr,yr,xl,yl,h,w;
  PetscBool      isnull;

  PetscFunctionBegin;
  ierr = PetscViewerDrawGetDraw(viewer,0,&draw);CHKERRQ(ierr);
  ierr = PetscDrawIsNull(draw,&isnull);CHKERRQ(ierr);
  if (isnull) PetscFunctionReturn(0);

  ierr = PetscObjectCompose((PetscObject)A,"Zoomviewer",(PetscObject)viewer);CHKERRQ(ierr);
  xr   = A->cmap->n; yr = A->rmap->n; h = yr/10.0; w = xr/10.0;
  xr  += w;    yr += h;  xl = -w;     yl = -h;
  ierr = PetscDrawSetCoordinates(draw,xl,yl,xr,yr);CHKERRQ(ierr);
  ierr = PetscDrawZoom(draw,MatView_SeqAIJ_Draw_Zoom,A);CHKERRQ(ierr);
  ierr = PetscObjectCompose((PetscObject)A,"Zoomviewer",NULL);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatView_SeqAIJ"
PetscErrorCode MatView_SeqAIJ(Mat A,PetscViewer viewer)
{
  PetscErrorCode ierr;
  PetscBool      iascii,isbinary,isdraw;

  PetscFunctionBegin;
  ierr = PetscObjectTypeCompare((PetscObject)viewer,PETSCVIEWERASCII,&iascii);CHKERRQ(ierr);
  ierr = PetscObjectTypeCompare((PetscObject)viewer,PETSCVIEWERBINARY,&isbinary);CHKERRQ(ierr);
  ierr = PetscObjectTypeCompare((PetscObject)viewer,PETSCVIEWERDRAW,&isdraw);CHKERRQ(ierr);
  if (iascii) {
    ierr = MatView_SeqAIJ_ASCII(A,viewer);CHKERRQ(ierr);
  } else if (isbinary) {
    ierr = MatView_SeqAIJ_Binary(A,viewer);CHKERRQ(ierr);
  } else if (isdraw) {
    ierr = MatView_SeqAIJ_Draw(A,viewer);CHKERRQ(ierr);
  }
  ierr = MatView_SeqAIJ_Inode(A,viewer);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatAssemblyEnd_SeqAIJ"
PetscErrorCode MatAssemblyEnd_SeqAIJ(Mat A,MatAssemblyType mode)
{
  Mat_SeqAIJ     *a = (Mat_SeqAIJ*)A->data;
  PetscErrorCode ierr;
  PetscInt       fshift = 0,i,j,*ai = a->i,*aj = a->j,*imax = a->imax;
  PetscInt       m      = A->rmap->n,*ip,N,*ailen = a->ilen,rmax = 0;
  MatScalar      *aa    = a->a,*ap;
  PetscReal      ratio  = 0.6;

  PetscFunctionBegin;
  if (mode == MAT_FLUSH_ASSEMBLY) PetscFunctionReturn(0);

  if (m) rmax = ailen[0]; /* determine row with most nonzeros */
  for (i=1; i<m; i++) {
    /* move each row back by the amount of empty slots (fshift) before it*/
    fshift += imax[i-1] - ailen[i-1];
    rmax    = PetscMax(rmax,ailen[i]);
    if (fshift) {
      ip = aj + ai[i];
      ap = aa + ai[i];
      N  = ailen[i];
      for (j=0; j<N; j++) {
        ip[j-fshift] = ip[j];
        ap[j-fshift] = ap[j];
      }
    }
    ai[i] = ai[i-1] + ailen[i-1];
  }
  if (m) {
    fshift += imax[m-1] - ailen[m-1];
    ai[m]   = ai[m-1] + ailen[m-1];
  }

  /* reset ilen and imax for each row */
  a->nonzerorowcnt = 0;
  for (i=0; i<m; i++) {
    ailen[i] = imax[i] = ai[i+1] - ai[i];
    a->nonzerorowcnt += ((ai[i+1] - ai[i]) > 0);
  }
  a->nz = ai[m];
  if (fshift && a->nounused == -1) SETERRQ3(PETSC_COMM_SELF,PETSC_ERR_PLIB, "Unused space detected in matrix: %D X %D, %D unneeded", m, A->cmap->n, fshift);

  ierr = MatMarkDiagonal_SeqAIJ(A);CHKERRQ(ierr);
  ierr = PetscInfo4(A,"Matrix size: %D X %D; storage space: %D unneeded,%D used\n",m,A->cmap->n,fshift,a->nz);CHKERRQ(ierr);
  ierr = PetscInfo1(A,"Number of mallocs during MatSetValues() is %D\n",a->reallocs);CHKERRQ(ierr);
  ierr = PetscInfo1(A,"Maximum nonzeros in any row is %D\n",rmax);CHKERRQ(ierr);

  A->info.mallocs    += a->reallocs;
  a->reallocs         = 0;
  A->info.nz_unneeded = (PetscReal)fshift;
  a->rmax             = rmax;

  ierr = MatCheckCompressedRow(A,a->nonzerorowcnt,&a->compressedrow,a->i,m,ratio);CHKERRQ(ierr);
  ierr = MatAssemblyEnd_SeqAIJ_Inode(A,mode);CHKERRQ(ierr);
  ierr = MatSeqAIJInvalidateDiagonal(A);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatRealPart_SeqAIJ"
PetscErrorCode MatRealPart_SeqAIJ(Mat A)
{
  Mat_SeqAIJ     *a = (Mat_SeqAIJ*)A->data;
  PetscInt       i,nz = a->nz;
  MatScalar      *aa = a->a;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  for (i=0; i<nz; i++) aa[i] = PetscRealPart(aa[i]);
  ierr = MatSeqAIJInvalidateDiagonal(A);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatImaginaryPart_SeqAIJ"
PetscErrorCode MatImaginaryPart_SeqAIJ(Mat A)
{
  Mat_SeqAIJ     *a = (Mat_SeqAIJ*)A->data;
  PetscInt       i,nz = a->nz;
  MatScalar      *aa = a->a;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  for (i=0; i<nz; i++) aa[i] = PetscImaginaryPart(aa[i]);
  ierr = MatSeqAIJInvalidateDiagonal(A);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatZeroEntries_SeqAIJ"
PetscErrorCode MatZeroEntries_SeqAIJ(Mat A)
{
  Mat_SeqAIJ     *a = (Mat_SeqAIJ*)A->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscMemzero(a->a,(a->i[A->rmap->n])*sizeof(PetscScalar));CHKERRQ(ierr);
  ierr = MatSeqAIJInvalidateDiagonal(A);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatDestroy_SeqAIJ"
PetscErrorCode MatDestroy_SeqAIJ(Mat A)
{
  Mat_SeqAIJ     *a = (Mat_SeqAIJ*)A->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
#if defined(PETSC_USE_LOG)
  PetscLogObjectState((PetscObject)A,"Rows=%D, Cols=%D, NZ=%D",A->rmap->n,A->cmap->n,a->nz);
#endif
  ierr = MatSeqXAIJFreeAIJ(A,&a->a,&a->j,&a->i);CHKERRQ(ierr);
  ierr = ISDestroy(&a->row);CHKERRQ(ierr);
  ierr = ISDestroy(&a->col);CHKERRQ(ierr);
  ierr = PetscFree(a->diag);CHKERRQ(ierr);
  ierr = PetscFree(a->ibdiag);CHKERRQ(ierr);
  ierr = PetscFree2(a->imax,a->ilen);CHKERRQ(ierr);
  ierr = PetscFree3(a->idiag,a->mdiag,a->ssor_work);CHKERRQ(ierr);
  ierr = PetscFree(a->solve_work);CHKERRQ(ierr);
  ierr = ISDestroy(&a->icol);CHKERRQ(ierr);
  ierr = PetscFree(a->saved_values);CHKERRQ(ierr);
  ierr = ISColoringDestroy(&a->coloring);CHKERRQ(ierr);
  ierr = PetscFree2(a->compressedrow.i,a->compressedrow.rindex);CHKERRQ(ierr);
  ierr = PetscFree(a->matmult_abdense);CHKERRQ(ierr);

  ierr = MatDestroy_SeqAIJ_Inode(A);CHKERRQ(ierr);
  ierr = PetscFree(A->data);CHKERRQ(ierr);

  ierr = PetscObjectChangeTypeName((PetscObject)A,0);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)A,"MatSeqAIJSetColumnIndices_C",NULL);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)A,"MatStoreValues_C",NULL);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)A,"MatRetrieveValues_C",NULL);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)A,"MatConvert_seqaij_seqsbaij_C",NULL);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)A,"MatConvert_seqaij_seqbaij_C",NULL);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)A,"MatConvert_seqaij_seqaijperm_C",NULL);CHKERRQ(ierr);
#if defined(PETSC_HAVE_ELEMENTAL)
  ierr = PetscObjectComposeFunction((PetscObject)A,"MatConvert_seqaij_elemental_C",NULL);CHKERRQ(ierr);
#endif
  ierr = PetscObjectComposeFunction((PetscObject)A,"MatConvert_seqaij_seqdense_C",NULL);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)A,"MatIsTranspose_C",NULL);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)A,"MatSeqAIJSetPreallocation_C",NULL);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)A,"MatSeqAIJSetPreallocationCSR_C",NULL);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)A,"MatReorderForNonzeroDiagonal_C",NULL);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatSetOption_SeqAIJ"
PetscErrorCode MatSetOption_SeqAIJ(Mat A,MatOption op,PetscBool flg)
{
  Mat_SeqAIJ     *a = (Mat_SeqAIJ*)A->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  switch (op) {
  case MAT_ROW_ORIENTED:
    a->roworiented = flg;
    break;
  case MAT_KEEP_NONZERO_PATTERN:
    a->keepnonzeropattern = flg;
    break;
  case MAT_NEW_NONZERO_LOCATIONS:
    a->nonew = (flg ? 0 : 1);
    break;
  case MAT_NEW_NONZERO_LOCATION_ERR:
    a->nonew = (flg ? -1 : 0);
    break;
  case MAT_NEW_NONZERO_ALLOCATION_ERR:
    a->nonew = (flg ? -2 : 0);
    break;
  case MAT_UNUSED_NONZERO_LOCATION_ERR:
    a->nounused = (flg ? -1 : 0);
    break;
  case MAT_IGNORE_ZERO_ENTRIES:
    a->ignorezeroentries = flg;
    break;
  case MAT_SPD:
  case MAT_SYMMETRIC:
  case MAT_STRUCTURALLY_SYMMETRIC:
  case MAT_HERMITIAN:
  case MAT_SYMMETRY_ETERNAL:
    /* These options are handled directly by MatSetOption() */
    break;
  case MAT_NEW_DIAGONALS:
  case MAT_IGNORE_OFF_PROC_ENTRIES:
  case MAT_USE_HASH_TABLE:
    ierr = PetscInfo1(A,"Option %s ignored\n",MatOptions[op]);CHKERRQ(ierr);
    break;
  case MAT_USE_INODES:
    /* Not an error because MatSetOption_SeqAIJ_Inode handles this one */
    break;
  default:
    SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_SUP,"unknown option %d",op);
  }
  ierr = MatSetOption_SeqAIJ_Inode(A,op,flg);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatGetDiagonal_SeqAIJ"
PetscErrorCode MatGetDiagonal_SeqAIJ(Mat A,Vec v)
{
  Mat_SeqAIJ     *a = (Mat_SeqAIJ*)A->data;
  PetscErrorCode ierr;
  PetscInt       i,j,n,*ai=a->i,*aj=a->j,nz;
  PetscScalar    *aa=a->a,*x,zero=0.0;

  PetscFunctionBegin;
  ierr = VecGetLocalSize(v,&n);CHKERRQ(ierr);
  if (n != A->rmap->n) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_SIZ,"Nonconforming matrix and vector");

  if (A->factortype == MAT_FACTOR_ILU || A->factortype == MAT_FACTOR_LU) {
    PetscInt *diag=a->diag;
    ierr = VecGetArray(v,&x);CHKERRQ(ierr);
    for (i=0; i<n; i++) x[i] = 1.0/aa[diag[i]];
    ierr = VecRestoreArray(v,&x);CHKERRQ(ierr);
    PetscFunctionReturn(0);
  }

  ierr = VecSet(v,zero);CHKERRQ(ierr);
  ierr = VecGetArray(v,&x);CHKERRQ(ierr);
  for (i=0; i<n; i++) {
    nz = ai[i+1] - ai[i];
    if (!nz) x[i] = 0.0;
    for (j=ai[i]; j<ai[i+1]; j++) {
      if (aj[j] == i) {
        x[i] = aa[j];
        break;
      }
    }
  }
  ierr = VecRestoreArray(v,&x);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#include <../src/mat/impls/aij/seq/ftn-kernels/fmult.h>
#undef __FUNCT__
#define __FUNCT__ "MatMultTransposeAdd_SeqAIJ"
PetscErrorCode MatMultTransposeAdd_SeqAIJ(Mat A,Vec xx,Vec zz,Vec yy)
{
  Mat_SeqAIJ        *a = (Mat_SeqAIJ*)A->data;
  PetscScalar       *y;
  const PetscScalar *x;
  PetscErrorCode    ierr;
  PetscInt          m = A->rmap->n;
#if !defined(PETSC_USE_FORTRAN_KERNEL_MULTTRANSPOSEAIJ)
  const MatScalar   *v;
  PetscScalar       alpha;
  PetscInt          n,i,j;
  const PetscInt    *idx,*ii,*ridx=NULL;
  Mat_CompressedRow cprow    = a->compressedrow;
  PetscBool         usecprow = cprow.use;
#endif

  PetscFunctionBegin;
  if (zz != yy) {ierr = VecCopy(zz,yy);CHKERRQ(ierr);}
  ierr = VecGetArrayRead(xx,&x);CHKERRQ(ierr);
  ierr = VecGetArray(yy,&y);CHKERRQ(ierr);

#if defined(PETSC_USE_FORTRAN_KERNEL_MULTTRANSPOSEAIJ)
  fortranmulttransposeaddaij_(&m,x,a->i,a->j,a->a,y);
#else
  if (usecprow) {
    m    = cprow.nrows;
    ii   = cprow.i;
    ridx = cprow.rindex;
  } else {
    ii = a->i;
  }
  for (i=0; i<m; i++) {
    idx = a->j + ii[i];
    v   = a->a + ii[i];
    n   = ii[i+1] - ii[i];
    if (usecprow) {
      alpha = x[ridx[i]];
    } else {
      alpha = x[i];
    }
    for (j=0; j<n; j++) y[idx[j]] += alpha*v[j];
  }
#endif
  ierr = PetscLogFlops(2.0*a->nz);CHKERRQ(ierr);
  ierr = VecRestoreArrayRead(xx,&x);CHKERRQ(ierr);
  ierr = VecRestoreArray(yy,&y);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatMultTranspose_SeqAIJ"
PetscErrorCode MatMultTranspose_SeqAIJ(Mat A,Vec xx,Vec yy)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = VecSet(yy,0.0);CHKERRQ(ierr);
  ierr = MatMultTransposeAdd_SeqAIJ(A,xx,yy,yy);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#include <../src/mat/impls/aij/seq/ftn-kernels/fmult.h>

#undef __FUNCT__
#define __FUNCT__ "MatMult_SeqAIJ"
PetscErrorCode MatMult_SeqAIJ(Mat A,Vec xx,Vec yy)
{
  Mat_SeqAIJ        *a = (Mat_SeqAIJ*)A->data;
  PetscScalar       *y;
  const PetscScalar *x;
  const MatScalar   *aa;
  PetscErrorCode    ierr;
  PetscInt          m=A->rmap->n;
  const PetscInt    *aj,*ii,*ridx=NULL;
  PetscInt          n,i;
  PetscScalar       sum;
  PetscBool         usecprow=a->compressedrow.use;

#if defined(PETSC_HAVE_PRAGMA_DISJOINT)
#pragma disjoint(*x,*y,*aa)
#endif

  PetscFunctionBegin;
  ierr = VecGetArrayRead(xx,&x);CHKERRQ(ierr);
  ierr = VecGetArray(yy,&y);CHKERRQ(ierr);
  aj   = a->j;
  aa   = a->a;
  ii   = a->i;
  if (usecprow) { /* use compressed row format */
    ierr = PetscMemzero(y,m*sizeof(PetscScalar));CHKERRQ(ierr);
    m    = a->compressedrow.nrows;
    ii   = a->compressedrow.i;
    ridx = a->compressedrow.rindex;
    for (i=0; i<m; i++) {
      n           = ii[i+1] - ii[i];
      aj          = a->j + ii[i];
      aa          = a->a + ii[i];
      sum         = 0.0;
      PetscSparseDensePlusDot(sum,x,aa,aj,n);
      /* for (j=0; j<n; j++) sum += (*aa++)*x[*aj++]; */
      y[*ridx++] = sum;
    }
  } else { /* do not use compressed row format */
#if defined(PETSC_USE_FORTRAN_KERNEL_MULTAIJ)
    fortranmultaij_(&m,x,ii,aj,aa,y);
#else
    for (i=0; i<m; i++) {
      n           = ii[i+1] - ii[i];
      aj          = a->j + ii[i];
      aa          = a->a + ii[i];
      sum         = 0.0;
      PetscSparseDensePlusDot(sum,x,aa,aj,n);
      y[i] = sum;
    }
#endif
  }
  ierr = PetscLogFlops(2.0*a->nz - a->nonzerorowcnt);CHKERRQ(ierr);
  ierr = VecRestoreArrayRead(xx,&x);CHKERRQ(ierr);
  ierr = VecRestoreArray(yy,&y);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatMultMax_SeqAIJ"
PetscErrorCode MatMultMax_SeqAIJ(Mat A,Vec xx,Vec yy)
{
  Mat_SeqAIJ        *a = (Mat_SeqAIJ*)A->data;
  PetscScalar       *y;
  const PetscScalar *x;
  const MatScalar   *aa;
  PetscErrorCode    ierr;
  PetscInt          m=A->rmap->n;
  const PetscInt    *aj,*ii,*ridx=NULL;
  PetscInt          n,i,nonzerorow=0;
  PetscScalar       sum;
  PetscBool         usecprow=a->compressedrow.use;

#if defined(PETSC_HAVE_PRAGMA_DISJOINT)
#pragma disjoint(*x,*y,*aa)
#endif

  PetscFunctionBegin;
  ierr = VecGetArrayRead(xx,&x);CHKERRQ(ierr);
  ierr = VecGetArray(yy,&y);CHKERRQ(ierr);
  aj   = a->j;
  aa   = a->a;
  ii   = a->i;
  if (usecprow) { /* use compressed row format */
    m    = a->compressedrow.nrows;
    ii   = a->compressedrow.i;
    ridx = a->compressedrow.rindex;
    for (i=0; i<m; i++) {
      n           = ii[i+1] - ii[i];
      aj          = a->j + ii[i];
      aa          = a->a + ii[i];
      sum         = 0.0;
      nonzerorow += (n>0);
      PetscSparseDenseMaxDot(sum,x,aa,aj,n);
      /* for (j=0; j<n; j++) sum += (*aa++)*x[*aj++]; */
      y[*ridx++] = sum;
    }
  } else { /* do not use compressed row format */
    for (i=0; i<m; i++) {
      n           = ii[i+1] - ii[i];
      aj          = a->j + ii[i];
      aa          = a->a + ii[i];
      sum         = 0.0;
      nonzerorow += (n>0);
      PetscSparseDenseMaxDot(sum,x,aa,aj,n);
      y[i] = sum;
    }
  }
  ierr = PetscLogFlops(2.0*a->nz - nonzerorow);CHKERRQ(ierr);
  ierr = VecRestoreArrayRead(xx,&x);CHKERRQ(ierr);
  ierr = VecRestoreArray(yy,&y);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatMultAddMax_SeqAIJ"
PetscErrorCode MatMultAddMax_SeqAIJ(Mat A,Vec xx,Vec yy,Vec zz)
{
  Mat_SeqAIJ        *a = (Mat_SeqAIJ*)A->data;
  PetscScalar       *y,*z;
  const PetscScalar *x;
  const MatScalar   *aa;
  PetscErrorCode    ierr;
  PetscInt          m = A->rmap->n,*aj,*ii;
  PetscInt          n,i,*ridx=NULL;
  PetscScalar       sum;
  PetscBool         usecprow=a->compressedrow.use;

  PetscFunctionBegin;
  ierr = VecGetArrayRead(xx,&x);CHKERRQ(ierr);
  ierr = VecGetArrayPair(yy,zz,&y,&z);CHKERRQ(ierr);

  aj = a->j;
  aa = a->a;
  ii = a->i;
  if (usecprow) { /* use compressed row format */
    if (zz != yy) {
      ierr = PetscMemcpy(z,y,m*sizeof(PetscScalar));CHKERRQ(ierr);
    }
    m    = a->compressedrow.nrows;
    ii   = a->compressedrow.i;
    ridx = a->compressedrow.rindex;
    for (i=0; i<m; i++) {
      n   = ii[i+1] - ii[i];
      aj  = a->j + ii[i];
      aa  = a->a + ii[i];
      sum = y[*ridx];
      PetscSparseDenseMaxDot(sum,x,aa,aj,n);
      z[*ridx++] = sum;
    }
  } else { /* do not use compressed row format */
    for (i=0; i<m; i++) {
      n   = ii[i+1] - ii[i];
      aj  = a->j + ii[i];
      aa  = a->a + ii[i];
      sum = y[i];
      PetscSparseDenseMaxDot(sum,x,aa,aj,n);
      z[i] = sum;
    }
  }
  ierr = PetscLogFlops(2.0*a->nz);CHKERRQ(ierr);
  ierr = VecRestoreArrayRead(xx,&x);CHKERRQ(ierr);
  ierr = VecRestoreArrayPair(yy,zz,&y,&z);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#include <../src/mat/impls/aij/seq/ftn-kernels/fmultadd.h>
#undef __FUNCT__
#define __FUNCT__ "MatMultAdd_SeqAIJ"
PetscErrorCode MatMultAdd_SeqAIJ(Mat A,Vec xx,Vec yy,Vec zz)
{
  Mat_SeqAIJ        *a = (Mat_SeqAIJ*)A->data;
  PetscScalar       *y,*z;
  const PetscScalar *x;
  const MatScalar   *aa;
  PetscErrorCode    ierr;
  const PetscInt    *aj,*ii,*ridx=NULL;
  PetscInt          m = A->rmap->n,n,i;
  PetscScalar       sum;
  PetscBool         usecprow=a->compressedrow.use;

  PetscFunctionBegin;
  ierr = VecGetArrayRead(xx,&x);CHKERRQ(ierr);
  ierr = VecGetArrayPair(yy,zz,&y,&z);CHKERRQ(ierr);

  aj = a->j;
  aa = a->a;
  ii = a->i;
  if (usecprow) { /* use compressed row format */
    if (zz != yy) {
      ierr = PetscMemcpy(z,y,m*sizeof(PetscScalar));CHKERRQ(ierr);
    }
    m    = a->compressedrow.nrows;
    ii   = a->compressedrow.i;
    ridx = a->compressedrow.rindex;
    for (i=0; i<m; i++) {
      n   = ii[i+1] - ii[i];
      aj  = a->j + ii[i];
      aa  = a->a + ii[i];
      sum = y[*ridx];
      PetscSparseDensePlusDot(sum,x,aa,aj,n);
      z[*ridx++] = sum;
    }
  } else { /* do not use compressed row format */
#if defined(PETSC_USE_FORTRAN_KERNEL_MULTADDAIJ)
    fortranmultaddaij_(&m,x,ii,aj,aa,y,z);
#else
    for (i=0; i<m; i++) {
      n   = ii[i+1] - ii[i];
      aj  = a->j + ii[i];
      aa  = a->a + ii[i];
      sum = y[i];
      PetscSparseDensePlusDot(sum,x,aa,aj,n);
      z[i] = sum;
    }
#endif
  }
  ierr = PetscLogFlops(2.0*a->nz);CHKERRQ(ierr);
  ierr = VecRestoreArrayRead(xx,&x);CHKERRQ(ierr);
  ierr = VecRestoreArrayPair(yy,zz,&y,&z);CHKERRQ(ierr);
#if defined(PETSC_HAVE_CUSP)
  /*
  ierr = VecView(xx,0);CHKERRQ(ierr);
  ierr = VecView(zz,0);CHKERRQ(ierr);
  ierr = MatView(A,0);CHKERRQ(ierr);
  */
#endif
  PetscFunctionReturn(0);
}

/*
     Adds diagonal pointers to sparse matrix structure.
*/
#undef __FUNCT__
#define __FUNCT__ "MatMarkDiagonal_SeqAIJ"
PetscErrorCode MatMarkDiagonal_SeqAIJ(Mat A)
{
  Mat_SeqAIJ     *a = (Mat_SeqAIJ*)A->data;
  PetscErrorCode ierr;
  PetscInt       i,j,m = A->rmap->n;

  PetscFunctionBegin;
  if (!a->diag) {
    ierr = PetscMalloc1(m,&a->diag);CHKERRQ(ierr);
    ierr = PetscLogObjectMemory((PetscObject)A, m*sizeof(PetscInt));CHKERRQ(ierr);
  }
  for (i=0; i<A->rmap->n; i++) {
    a->diag[i] = a->i[i+1];
    for (j=a->i[i]; j<a->i[i+1]; j++) {
      if (a->j[j] == i) {
        a->diag[i] = j;
        break;
      }
    }
  }
  PetscFunctionReturn(0);
}

/*
     Checks for missing diagonals
*/
#undef __FUNCT__
#define __FUNCT__ "MatMissingDiagonal_SeqAIJ"
PetscErrorCode MatMissingDiagonal_SeqAIJ(Mat A,PetscBool  *missing,PetscInt *d)
{
  Mat_SeqAIJ *a = (Mat_SeqAIJ*)A->data;
  PetscInt   *diag,*ii = a->i,i;

  PetscFunctionBegin;
  *missing = PETSC_FALSE;
  if (A->rmap->n > 0 && !ii) {
    *missing = PETSC_TRUE;
    if (d) *d = 0;
    PetscInfo(A,"Matrix has no entries therefore is missing diagonal\n");
  } else {
    diag = a->diag;
    for (i=0; i<A->rmap->n; i++) {
      if (diag[i] >= ii[i+1]) {
        *missing = PETSC_TRUE;
        if (d) *d = i;
        PetscInfo1(A,"Matrix is missing diagonal number %D\n",i);
        break;
      }
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatInvertDiagonal_SeqAIJ"
/*
   Negative shift indicates do not generate an error if there is a zero diagonal, just invert it anyways
*/
PetscErrorCode  MatInvertDiagonal_SeqAIJ(Mat A,PetscScalar omega,PetscScalar fshift)
{
  Mat_SeqAIJ     *a = (Mat_SeqAIJ*) A->data;
  PetscErrorCode ierr;
  PetscInt       i,*diag,m = A->rmap->n;
  MatScalar      *v = a->a;
  PetscScalar    *idiag,*mdiag;

  PetscFunctionBegin;
  if (a->idiagvalid) PetscFunctionReturn(0);
  ierr = MatMarkDiagonal_SeqAIJ(A);CHKERRQ(ierr);
  diag = a->diag;
  if (!a->idiag) {
    ierr = PetscMalloc3(m,&a->idiag,m,&a->mdiag,m,&a->ssor_work);CHKERRQ(ierr);
    ierr = PetscLogObjectMemory((PetscObject)A, 3*m*sizeof(PetscScalar));CHKERRQ(ierr);
    v    = a->a;
  }
  mdiag = a->mdiag;
  idiag = a->idiag;

  if (omega == 1.0 && PetscRealPart(fshift) <= 0.0) {
    for (i=0; i<m; i++) {
      mdiag[i] = v[diag[i]];
      if (!PetscAbsScalar(mdiag[i])) { /* zero diagonal */
        if (PetscRealPart(fshift)) {
          ierr = PetscInfo1(A,"Zero diagonal on row %D\n",i);CHKERRQ(ierr);
          A->errortype = MAT_FACTOR_NUMERIC_ZEROPIVOT;
        } else {
          SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_ARG_INCOMP,"Zero diagonal on row %D",i);
        }
      }
      idiag[i] = 1.0/v[diag[i]];
    }
    ierr = PetscLogFlops(m);CHKERRQ(ierr);
  } else {
    for (i=0; i<m; i++) {
      mdiag[i] = v[diag[i]];
      idiag[i] = omega/(fshift + v[diag[i]]);
    }
    ierr = PetscLogFlops(2.0*m);CHKERRQ(ierr);
  }
  a->idiagvalid = PETSC_TRUE;
  PetscFunctionReturn(0);
}

#include <../src/mat/impls/aij/seq/ftn-kernels/frelax.h>
#undef __FUNCT__
#define __FUNCT__ "MatSOR_SeqAIJ"
PetscErrorCode MatSOR_SeqAIJ(Mat A,Vec bb,PetscReal omega,MatSORType flag,PetscReal fshift,PetscInt its,PetscInt lits,Vec xx)
{
  Mat_SeqAIJ        *a = (Mat_SeqAIJ*)A->data;
  PetscScalar       *x,d,sum,*t,scale;
  const MatScalar   *v = a->a,*idiag=0,*mdiag;
  const PetscScalar *b, *bs,*xb, *ts;
  PetscErrorCode    ierr;
  PetscInt          n = A->cmap->n,m = A->rmap->n,i;
  const PetscInt    *idx,*diag;

  PetscFunctionBegin;
  its = its*lits;

  if (fshift != a->fshift || omega != a->omega) a->idiagvalid = PETSC_FALSE; /* must recompute idiag[] */
  if (!a->idiagvalid) {ierr = MatInvertDiagonal_SeqAIJ(A,omega,fshift);CHKERRQ(ierr);}
  a->fshift = fshift;
  a->omega  = omega;

  diag  = a->diag;
  t     = a->ssor_work;
  idiag = a->idiag;
  mdiag = a->mdiag;

  ierr = VecGetArray(xx,&x);CHKERRQ(ierr);
  ierr = VecGetArrayRead(bb,&b);CHKERRQ(ierr);
  /* We count flops by assuming the upper triangular and lower triangular parts have the same number of nonzeros */
  if (flag == SOR_APPLY_UPPER) {
    /* apply (U + D/omega) to the vector */
    bs = b;
    for (i=0; i<m; i++) {
      d   = fshift + mdiag[i];
      n   = a->i[i+1] - diag[i] - 1;
      idx = a->j + diag[i] + 1;
      v   = a->a + diag[i] + 1;
      sum = b[i]*d/omega;
      PetscSparseDensePlusDot(sum,bs,v,idx,n);
      x[i] = sum;
    }
    ierr = VecRestoreArray(xx,&x);CHKERRQ(ierr);
    ierr = VecRestoreArrayRead(bb,&b);CHKERRQ(ierr);
    ierr = PetscLogFlops(a->nz);CHKERRQ(ierr);
    PetscFunctionReturn(0);
  }

  if (flag == SOR_APPLY_LOWER) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_SUP,"SOR_APPLY_LOWER is not implemented");
  else if (flag & SOR_EISENSTAT) {
    /* Let  A = L + U + D; where L is lower trianglar,
    U is upper triangular, E = D/omega; This routine applies

            (L + E)^{-1} A (U + E)^{-1}

    to a vector efficiently using Eisenstat's trick.
    */
    scale = (2.0/omega) - 1.0;

    /*  x = (E + U)^{-1} b */
    for (i=m-1; i>=0; i--) {
      n   = a->i[i+1] - diag[i] - 1;
      idx = a->j + diag[i] + 1;
      v   = a->a + diag[i] + 1;
      sum = b[i];
      PetscSparseDenseMinusDot(sum,x,v,idx,n);
      x[i] = sum*idiag[i];
    }

    /*  t = b - (2*E - D)x */
    v = a->a;
    for (i=0; i<m; i++) t[i] = b[i] - scale*(v[*diag++])*x[i];

    /*  t = (E + L)^{-1}t */
    ts   = t;
    diag = a->diag;
    for (i=0; i<m; i++) {
      n   = diag[i] - a->i[i];
      idx = a->j + a->i[i];
      v   = a->a + a->i[i];
      sum = t[i];
      PetscSparseDenseMinusDot(sum,ts,v,idx,n);
      t[i] = sum*idiag[i];
      /*  x = x + t */
      x[i] += t[i];
    }

    ierr = PetscLogFlops(6.0*m-1 + 2.0*a->nz);CHKERRQ(ierr);
    ierr = VecRestoreArray(xx,&x);CHKERRQ(ierr);
    ierr = VecRestoreArrayRead(bb,&b);CHKERRQ(ierr);
    PetscFunctionReturn(0);
  }
  if (flag & SOR_ZERO_INITIAL_GUESS) {
    if (flag & SOR_FORWARD_SWEEP || flag & SOR_LOCAL_FORWARD_SWEEP) {
      for (i=0; i<m; i++) {
        n   = diag[i] - a->i[i];
        idx = a->j + a->i[i];
        v   = a->a + a->i[i];
        sum = b[i];
        PetscSparseDenseMinusDot(sum,x,v,idx,n);
        t[i] = sum;
        x[i] = sum*idiag[i];
      }
      xb   = t;
      ierr = PetscLogFlops(a->nz);CHKERRQ(ierr);
    } else xb = b;
    if (flag & SOR_BACKWARD_SWEEP || flag & SOR_LOCAL_BACKWARD_SWEEP) {
      for (i=m-1; i>=0; i--) {
        n   = a->i[i+1] - diag[i] - 1;
        idx = a->j + diag[i] + 1;
        v   = a->a + diag[i] + 1;
        sum = xb[i];
        PetscSparseDenseMinusDot(sum,x,v,idx,n);
        if (xb == b) {
          x[i] = sum*idiag[i];
        } else {
          x[i] = (1-omega)*x[i] + sum*idiag[i];  /* omega in idiag */
        }
      }
      ierr = PetscLogFlops(a->nz);CHKERRQ(ierr); /* assumes 1/2 in upper */
    }
    its--;
  }
  while (its--) {
    if (flag & SOR_FORWARD_SWEEP || flag & SOR_LOCAL_FORWARD_SWEEP) {
      for (i=0; i<m; i++) {
        /* lower */
        n   = diag[i] - a->i[i];
        idx = a->j + a->i[i];
        v   = a->a + a->i[i];
        sum = b[i];
        PetscSparseDenseMinusDot(sum,x,v,idx,n);
        t[i] = sum;             /* save application of the lower-triangular part */
        /* upper */
        n   = a->i[i+1] - diag[i] - 1;
        idx = a->j + diag[i] + 1;
        v   = a->a + diag[i] + 1;
        PetscSparseDenseMinusDot(sum,x,v,idx,n);
        x[i] = (1. - omega)*x[i] + sum*idiag[i]; /* omega in idiag */
      }
      xb   = t;
      ierr = PetscLogFlops(2.0*a->nz);CHKERRQ(ierr);
    } else xb = b;
    if (flag & SOR_BACKWARD_SWEEP || flag & SOR_LOCAL_BACKWARD_SWEEP) {
      for (i=m-1; i>=0; i--) {
        sum = xb[i];
        if (xb == b) {
          /* whole matrix (no checkpointing available) */
          n   = a->i[i+1] - a->i[i];
          idx = a->j + a->i[i];
          v   = a->a + a->i[i];
          PetscSparseDenseMinusDot(sum,x,v,idx,n);
          x[i] = (1. - omega)*x[i] + (sum + mdiag[i]*x[i])*idiag[i];
        } else { /* lower-triangular part has been saved, so only apply upper-triangular */
          n   = a->i[i+1] - diag[i] - 1;
          idx = a->j + diag[i] + 1;
          v   = a->a + diag[i] + 1;
          PetscSparseDenseMinusDot(sum,x,v,idx,n);
          x[i] = (1. - omega)*x[i] + sum*idiag[i];  /* omega in idiag */
        }
      }
      if (xb == b) {
        ierr = PetscLogFlops(2.0*a->nz);CHKERRQ(ierr);
      } else {
        ierr = PetscLogFlops(a->nz);CHKERRQ(ierr); /* assumes 1/2 in upper */
      }
    }
  }
  ierr = VecRestoreArray(xx,&x);CHKERRQ(ierr);
  ierr = VecRestoreArrayRead(bb,&b);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "MatGetInfo_SeqAIJ"
PetscErrorCode MatGetInfo_SeqAIJ(Mat A,MatInfoType flag,MatInfo *info)
{
  Mat_SeqAIJ *a = (Mat_SeqAIJ*)A->data;

  PetscFunctionBegin;
  info->block_size   = 1.0;
  info->nz_allocated = (double)a->maxnz;
  info->nz_used      = (double)a->nz;
  info->nz_unneeded  = (double)(a->maxnz - a->nz);
  info->assemblies   = (double)A->num_ass;
  info->mallocs      = (double)A->info.mallocs;
  info->memory       = ((PetscObject)A)->mem;
  if (A->factortype) {
    info->fill_ratio_given  = A->info.fill_ratio_given;
    info->fill_ratio_needed = A->info.fill_ratio_needed;
    info->factor_mallocs    = A->info.factor_mallocs;
  } else {
    info->fill_ratio_given  = 0;
    info->fill_ratio_needed = 0;
    info->factor_mallocs    = 0;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatZeroRows_SeqAIJ"
PetscErrorCode MatZeroRows_SeqAIJ(Mat A,PetscInt N,const PetscInt rows[],PetscScalar diag,Vec x,Vec b)
{
  Mat_SeqAIJ        *a = (Mat_SeqAIJ*)A->data;
  PetscInt          i,m = A->rmap->n - 1,d = 0;
  PetscErrorCode    ierr;
  const PetscScalar *xx;
  PetscScalar       *bb;
  PetscBool         missing;

  PetscFunctionBegin;
  if (x && b) {
    ierr = VecGetArrayRead(x,&xx);CHKERRQ(ierr);
    ierr = VecGetArray(b,&bb);CHKERRQ(ierr);
    for (i=0; i<N; i++) {
      if (rows[i] < 0 || rows[i] > m) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"row %D out of range", rows[i]);
      bb[rows[i]] = diag*xx[rows[i]];
    }
    ierr = VecRestoreArrayRead(x,&xx);CHKERRQ(ierr);
    ierr = VecRestoreArray(b,&bb);CHKERRQ(ierr);
  }

  if (a->keepnonzeropattern) {
    for (i=0; i<N; i++) {
      if (rows[i] < 0 || rows[i] > m) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"row %D out of range", rows[i]);
      ierr = PetscMemzero(&a->a[a->i[rows[i]]],a->ilen[rows[i]]*sizeof(PetscScalar));CHKERRQ(ierr);
    }
    if (diag != 0.0) {
      ierr = MatMissingDiagonal_SeqAIJ(A,&missing,&d);CHKERRQ(ierr);
      if (missing) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONGSTATE,"Matrix is missing diagonal entry in row %D",d);
      for (i=0; i<N; i++) {
        a->a[a->diag[rows[i]]] = diag;
      }
    }
  } else {
    if (diag != 0.0) {
      for (i=0; i<N; i++) {
        if (rows[i] < 0 || rows[i] > m) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"row %D out of range", rows[i]);
        if (a->ilen[rows[i]] > 0) {
          a->ilen[rows[i]]    = 1;
          a->a[a->i[rows[i]]] = diag;
          a->j[a->i[rows[i]]] = rows[i];
        } else { /* in case row was completely empty */
          ierr = MatSetValues_SeqAIJ(A,1,&rows[i],1,&rows[i],&diag,INSERT_VALUES);CHKERRQ(ierr);
        }
      }
    } else {
      for (i=0; i<N; i++) {
        if (rows[i] < 0 || rows[i] > m) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"row %D out of range", rows[i]);
        a->ilen[rows[i]] = 0;
      }
    }
    A->nonzerostate++;
  }
  ierr = MatAssemblyEnd_SeqAIJ(A,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatZeroRowsColumns_SeqAIJ"
PetscErrorCode MatZeroRowsColumns_SeqAIJ(Mat A,PetscInt N,const PetscInt rows[],PetscScalar diag,Vec x,Vec b)
{
  Mat_SeqAIJ        *a = (Mat_SeqAIJ*)A->data;
  PetscInt          i,j,m = A->rmap->n - 1,d = 0;
  PetscErrorCode    ierr;
  PetscBool         missing,*zeroed,vecs = PETSC_FALSE;
  const PetscScalar *xx;
  PetscScalar       *bb;

  PetscFunctionBegin;
  if (x && b) {
    ierr = VecGetArrayRead(x,&xx);CHKERRQ(ierr);
    ierr = VecGetArray(b,&bb);CHKERRQ(ierr);
    vecs = PETSC_TRUE;
  }
  ierr = PetscCalloc1(A->rmap->n,&zeroed);CHKERRQ(ierr);
  for (i=0; i<N; i++) {
    if (rows[i] < 0 || rows[i] > m) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"row %D out of range", rows[i]);
    ierr = PetscMemzero(&a->a[a->i[rows[i]]],a->ilen[rows[i]]*sizeof(PetscScalar));CHKERRQ(ierr);

    zeroed[rows[i]] = PETSC_TRUE;
  }
  for (i=0; i<A->rmap->n; i++) {
    if (!zeroed[i]) {
      for (j=a->i[i]; j<a->i[i+1]; j++) {
        if (zeroed[a->j[j]]) {
          if (vecs) bb[i] -= a->a[j]*xx[a->j[j]];
          a->a[j] = 0.0;
        }
      }
    } else if (vecs) bb[i] = diag*xx[i];
  }
  if (x && b) {
    ierr = VecRestoreArrayRead(x,&xx);CHKERRQ(ierr);
    ierr = VecRestoreArray(b,&bb);CHKERRQ(ierr);
  }
  ierr = PetscFree(zeroed);CHKERRQ(ierr);
  if (diag != 0.0) {
    ierr = MatMissingDiagonal_SeqAIJ(A,&missing,&d);CHKERRQ(ierr);
    if (missing) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONGSTATE,"Matrix is missing diagonal entry in row %D",d);
    for (i=0; i<N; i++) {
      a->a[a->diag[rows[i]]] = diag;
    }
  }
  ierr = MatAssemblyEnd_SeqAIJ(A,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatGetRow_SeqAIJ"
PetscErrorCode MatGetRow_SeqAIJ(Mat A,PetscInt row,PetscInt *nz,PetscInt **idx,PetscScalar **v)
{
  Mat_SeqAIJ *a = (Mat_SeqAIJ*)A->data;
  PetscInt   *itmp;

  PetscFunctionBegin;
  if (row < 0 || row >= A->rmap->n) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Row %D out of range",row);

  *nz = a->i[row+1] - a->i[row];
  if (v) *v = a->a + a->i[row];
  if (idx) {
    itmp = a->j + a->i[row];
    if (*nz) *idx = itmp;
    else *idx = 0;
  }
  PetscFunctionReturn(0);
}

/* remove this function? */
#undef __FUNCT__
#define __FUNCT__ "MatRestoreRow_SeqAIJ"
PetscErrorCode MatRestoreRow_SeqAIJ(Mat A,PetscInt row,PetscInt *nz,PetscInt **idx,PetscScalar **v)
{
  PetscFunctionBegin;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatNorm_SeqAIJ"
PetscErrorCode MatNorm_SeqAIJ(Mat A,NormType type,PetscReal *nrm)
{
  Mat_SeqAIJ     *a  = (Mat_SeqAIJ*)A->data;
  MatScalar      *v  = a->a;
  PetscReal      sum = 0.0;
  PetscErrorCode ierr;
  PetscInt       i,j;

  PetscFunctionBegin;
  if (type == NORM_FROBENIUS) {
    for (i=0; i<a->nz; i++) {
      sum += PetscRealPart(PetscConj(*v)*(*v)); v++;
    }
    *nrm = PetscSqrtReal(sum);
  } else if (type == NORM_1) {
    PetscReal *tmp;
    PetscInt  *jj = a->j;
    ierr = PetscCalloc1(A->cmap->n+1,&tmp);CHKERRQ(ierr);
    *nrm = 0.0;
    for (j=0; j<a->nz; j++) {
      tmp[*jj++] += PetscAbsScalar(*v);  v++;
    }
    for (j=0; j<A->cmap->n; j++) {
      if (tmp[j] > *nrm) *nrm = tmp[j];
    }
    ierr = PetscFree(tmp);CHKERRQ(ierr);
  } else if (type == NORM_INFINITY) {
    *nrm = 0.0;
    for (j=0; j<A->rmap->n; j++) {
      v   = a->a + a->i[j];
      sum = 0.0;
      for (i=0; i<a->i[j+1]-a->i[j]; i++) {
        sum += PetscAbsScalar(*v); v++;
      }
      if (sum > *nrm) *nrm = sum;
    }
  } else SETERRQ(PETSC_COMM_SELF,PETSC_ERR_SUP,"No support for two norm");
  PetscFunctionReturn(0);
}

/* Merged from MatGetSymbolicTranspose_SeqAIJ() - replace MatGetSymbolicTranspose_SeqAIJ()? */
#undef __FUNCT__
#define __FUNCT__ "MatTransposeSymbolic_SeqAIJ"
PetscErrorCode MatTransposeSymbolic_SeqAIJ(Mat A,Mat *B)
{
  PetscErrorCode ierr;
  PetscInt       i,j,anzj;
  Mat_SeqAIJ     *a=(Mat_SeqAIJ*)A->data,*b;
  PetscInt       an=A->cmap->N,am=A->rmap->N;
  PetscInt       *ati,*atj,*atfill,*ai=a->i,*aj=a->j;

  PetscFunctionBegin;
  /* Allocate space for symbolic transpose info and work array */
  ierr = PetscCalloc1(an+1,&ati);CHKERRQ(ierr);
  ierr = PetscMalloc1(ai[am],&atj);CHKERRQ(ierr);
  ierr = PetscMalloc1(an,&atfill);CHKERRQ(ierr);

  /* Walk through aj and count ## of non-zeros in each row of A^T. */
  /* Note: offset by 1 for fast conversion into csr format. */
  for (i=0;i<ai[am];i++) ati[aj[i]+1] += 1;
  /* Form ati for csr format of A^T. */
  for (i=0;i<an;i++) ati[i+1] += ati[i];

  /* Copy ati into atfill so we have locations of the next free space in atj */
  ierr = PetscMemcpy(atfill,ati,an*sizeof(PetscInt));CHKERRQ(ierr);

  /* Walk through A row-wise and mark nonzero entries of A^T. */
  for (i=0;i<am;i++) {
    anzj = ai[i+1] - ai[i];
    for (j=0;j<anzj;j++) {
      atj[atfill[*aj]] = i;
      atfill[*aj++]   += 1;
    }
  }

  /* Clean up temporary space and complete requests. */
  ierr = PetscFree(atfill);CHKERRQ(ierr);
  ierr = MatCreateSeqAIJWithArrays(PetscObjectComm((PetscObject)A),an,am,ati,atj,NULL,B);CHKERRQ(ierr);
  ierr = MatSetBlockSizes(*B,PetscAbs(A->cmap->bs),PetscAbs(A->rmap->bs));CHKERRQ(ierr);

  b          = (Mat_SeqAIJ*)((*B)->data);
  b->free_a  = PETSC_FALSE;
  b->free_ij = PETSC_TRUE;
  b->nonew   = 0;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatTranspose_SeqAIJ"
PetscErrorCode MatTranspose_SeqAIJ(Mat A,MatReuse reuse,Mat *B)
{
  Mat_SeqAIJ     *a = (Mat_SeqAIJ*)A->data;
  Mat            C;
  PetscErrorCode ierr;
  PetscInt       i,*aj = a->j,*ai = a->i,m = A->rmap->n,len,*col;
  MatScalar      *array = a->a;

  PetscFunctionBegin;
  if (reuse == MAT_REUSE_MATRIX && A == *B && m != A->cmap->n) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_SIZ,"Square matrix only for in-place");

  if (reuse == MAT_INITIAL_MATRIX || *B == A) {
    ierr = PetscCalloc1(1+A->cmap->n,&col);CHKERRQ(ierr);

    for (i=0; i<ai[m]; i++) col[aj[i]] += 1;
    ierr = MatCreate(PetscObjectComm((PetscObject)A),&C);CHKERRQ(ierr);
    ierr = MatSetSizes(C,A->cmap->n,m,A->cmap->n,m);CHKERRQ(ierr);
    ierr = MatSetBlockSizes(C,PetscAbs(A->cmap->bs),PetscAbs(A->rmap->bs));CHKERRQ(ierr);
    ierr = MatSetType(C,((PetscObject)A)->type_name);CHKERRQ(ierr);
    ierr = MatSeqAIJSetPreallocation_SeqAIJ(C,0,col);CHKERRQ(ierr);
    ierr = PetscFree(col);CHKERRQ(ierr);
  } else {
    C = *B;
  }

  for (i=0; i<m; i++) {
    len    = ai[i+1]-ai[i];
    ierr   = MatSetValues_SeqAIJ(C,len,aj,1,&i,array,INSERT_VALUES);CHKERRQ(ierr);
    array += len;
    aj    += len;
  }
  ierr = MatAssemblyBegin(C,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(C,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);

  if (reuse == MAT_INITIAL_MATRIX || *B != A) {
    *B = C;
  } else {
    ierr = MatHeaderMerge(A,&C);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatIsTranspose_SeqAIJ"
PetscErrorCode  MatIsTranspose_SeqAIJ(Mat A,Mat B,PetscReal tol,PetscBool  *f)
{
  Mat_SeqAIJ     *aij = (Mat_SeqAIJ*) A->data,*bij = (Mat_SeqAIJ*) A->data;
  PetscInt       *adx,*bdx,*aii,*bii,*aptr,*bptr;
  MatScalar      *va,*vb;
  PetscErrorCode ierr;
  PetscInt       ma,na,mb,nb, i;

  PetscFunctionBegin;
  bij = (Mat_SeqAIJ*) B->data;

  ierr = MatGetSize(A,&ma,&na);CHKERRQ(ierr);
  ierr = MatGetSize(B,&mb,&nb);CHKERRQ(ierr);
  if (ma!=nb || na!=mb) {
    *f = PETSC_FALSE;
    PetscFunctionReturn(0);
  }
  aii  = aij->i; bii = bij->i;
  adx  = aij->j; bdx = bij->j;
  va   = aij->a; vb = bij->a;
  ierr = PetscMalloc1(ma,&aptr);CHKERRQ(ierr);
  ierr = PetscMalloc1(mb,&bptr);CHKERRQ(ierr);
  for (i=0; i<ma; i++) aptr[i] = aii[i];
  for (i=0; i<mb; i++) bptr[i] = bii[i];

  *f = PETSC_TRUE;
  for (i=0; i<ma; i++) {
    while (aptr[i]<aii[i+1]) {
      PetscInt    idc,idr;
      PetscScalar vc,vr;
      /* column/row index/value */
      idc = adx[aptr[i]];
      idr = bdx[bptr[idc]];
      vc  = va[aptr[i]];
      vr  = vb[bptr[idc]];
      if (i!=idr || PetscAbsScalar(vc-vr) > tol) {
        *f = PETSC_FALSE;
        goto done;
      } else {
        aptr[i]++;
        if (B || i!=idc) bptr[idc]++;
      }
    }
  }
done:
  ierr = PetscFree(aptr);CHKERRQ(ierr);
  ierr = PetscFree(bptr);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatIsHermitianTranspose_SeqAIJ"
PetscErrorCode  MatIsHermitianTranspose_SeqAIJ(Mat A,Mat B,PetscReal tol,PetscBool  *f)
{
  Mat_SeqAIJ     *aij = (Mat_SeqAIJ*) A->data,*bij = (Mat_SeqAIJ*) A->data;
  PetscInt       *adx,*bdx,*aii,*bii,*aptr,*bptr;
  MatScalar      *va,*vb;
  PetscErrorCode ierr;
  PetscInt       ma,na,mb,nb, i;

  PetscFunctionBegin;
  bij = (Mat_SeqAIJ*) B->data;

  ierr = MatGetSize(A,&ma,&na);CHKERRQ(ierr);
  ierr = MatGetSize(B,&mb,&nb);CHKERRQ(ierr);
  if (ma!=nb || na!=mb) {
    *f = PETSC_FALSE;
    PetscFunctionReturn(0);
  }
  aii  = aij->i; bii = bij->i;
  adx  = aij->j; bdx = bij->j;
  va   = aij->a; vb = bij->a;
  ierr = PetscMalloc1(ma,&aptr);CHKERRQ(ierr);
  ierr = PetscMalloc1(mb,&bptr);CHKERRQ(ierr);
  for (i=0; i<ma; i++) aptr[i] = aii[i];
  for (i=0; i<mb; i++) bptr[i] = bii[i];

  *f = PETSC_TRUE;
  for (i=0; i<ma; i++) {
    while (aptr[i]<aii[i+1]) {
      PetscInt    idc,idr;
      PetscScalar vc,vr;
      /* column/row index/value */
      idc = adx[aptr[i]];
      idr = bdx[bptr[idc]];
      vc  = va[aptr[i]];
      vr  = vb[bptr[idc]];
      if (i!=idr || PetscAbsScalar(vc-PetscConj(vr)) > tol) {
        *f = PETSC_FALSE;
        goto done;
      } else {
        aptr[i]++;
        if (B || i!=idc) bptr[idc]++;
      }
    }
  }
done:
  ierr = PetscFree(aptr);CHKERRQ(ierr);
  ierr = PetscFree(bptr);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatIsSymmetric_SeqAIJ"
PetscErrorCode MatIsSymmetric_SeqAIJ(Mat A,PetscReal tol,PetscBool  *f)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = MatIsTranspose_SeqAIJ(A,A,tol,f);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatIsHermitian_SeqAIJ"
PetscErrorCode MatIsHermitian_SeqAIJ(Mat A,PetscReal tol,PetscBool  *f)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = MatIsHermitianTranspose_SeqAIJ(A,A,tol,f);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatDiagonalScale_SeqAIJ"
PetscErrorCode MatDiagonalScale_SeqAIJ(Mat A,Vec ll,Vec rr)
{
  Mat_SeqAIJ     *a = (Mat_SeqAIJ*)A->data;
  PetscScalar    *l,*r,x;
  MatScalar      *v;
  PetscErrorCode ierr;
  PetscInt       i,j,m = A->rmap->n,n = A->cmap->n,M,nz = a->nz,*jj;

  PetscFunctionBegin;
  if (ll) {
    /* The local size is used so that VecMPI can be passed to this routine
       by MatDiagonalScale_MPIAIJ */
    ierr = VecGetLocalSize(ll,&m);CHKERRQ(ierr);
    if (m != A->rmap->n) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_SIZ,"Left scaling vector wrong length");
    ierr = VecGetArray(ll,&l);CHKERRQ(ierr);
    v    = a->a;
    for (i=0; i<m; i++) {
      x = l[i];
      M = a->i[i+1] - a->i[i];
      for (j=0; j<M; j++) (*v++) *= x;
    }
    ierr = VecRestoreArray(ll,&l);CHKERRQ(ierr);
    ierr = PetscLogFlops(nz);CHKERRQ(ierr);
  }
  if (rr) {
    ierr = VecGetLocalSize(rr,&n);CHKERRQ(ierr);
    if (n != A->cmap->n) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_SIZ,"Right scaling vector wrong length");
    ierr = VecGetArray(rr,&r);CHKERRQ(ierr);
    v    = a->a; jj = a->j;
    for (i=0; i<nz; i++) (*v++) *= r[*jj++];
    ierr = VecRestoreArray(rr,&r);CHKERRQ(ierr);
    ierr = PetscLogFlops(nz);CHKERRQ(ierr);
  }
  ierr = MatSeqAIJInvalidateDiagonal(A);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatGetSubMatrix_SeqAIJ"
PetscErrorCode MatGetSubMatrix_SeqAIJ(Mat A,IS isrow,IS iscol,PetscInt csize,MatReuse scall,Mat *B)
{
  Mat_SeqAIJ     *a = (Mat_SeqAIJ*)A->data,*c;
  PetscErrorCode ierr;
  PetscInt       *smap,i,k,kstart,kend,oldcols = A->cmap->n,*lens;
  PetscInt       row,mat_i,*mat_j,tcol,first,step,*mat_ilen,sum,lensi;
  const PetscInt *irow,*icol;
  PetscInt       nrows,ncols;
  PetscInt       *starts,*j_new,*i_new,*aj = a->j,*ai = a->i,ii,*ailen = a->ilen;
  MatScalar      *a_new,*mat_a;
  Mat            C;
  PetscBool      stride;

  PetscFunctionBegin;

  ierr = ISGetIndices(isrow,&irow);CHKERRQ(ierr);
  ierr = ISGetLocalSize(isrow,&nrows);CHKERRQ(ierr);
  ierr = ISGetLocalSize(iscol,&ncols);CHKERRQ(ierr);

  ierr = PetscObjectTypeCompare((PetscObject)iscol,ISSTRIDE,&stride);CHKERRQ(ierr);
  if (stride) {
    ierr = ISStrideGetInfo(iscol,&first,&step);CHKERRQ(ierr);
  } else {
    first = 0;
    step  = 0;
  }
  if (stride && step == 1) {
    /* special case of contiguous rows */
    ierr = PetscMalloc2(nrows,&lens,nrows,&starts);CHKERRQ(ierr);
    /* loop over new rows determining lens and starting points */
    for (i=0; i<nrows; i++) {
      kstart = ai[irow[i]];
      kend   = kstart + ailen[irow[i]];
      starts[i] = kstart;
      for (k=kstart; k<kend; k++) {
        if (aj[k] >= first) {
          starts[i] = k;
          break;
        }
      }
      sum = 0;
      while (k < kend) {
        if (aj[k++] >= first+ncols) break;
        sum++;
      }
      lens[i] = sum;
    }
    /* create submatrix */
    if (scall == MAT_REUSE_MATRIX) {
      PetscInt n_cols,n_rows;
      ierr = MatGetSize(*B,&n_rows,&n_cols);CHKERRQ(ierr);
      if (n_rows != nrows || n_cols != ncols) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_SIZ,"Reused submatrix wrong size");
      ierr = MatZeroEntries(*B);CHKERRQ(ierr);
      C    = *B;
    } else {
      PetscInt rbs,cbs;
      ierr = MatCreate(PetscObjectComm((PetscObject)A),&C);CHKERRQ(ierr);
      ierr = MatSetSizes(C,nrows,ncols,PETSC_DETERMINE,PETSC_DETERMINE);CHKERRQ(ierr);
      ierr = ISGetBlockSize(isrow,&rbs);CHKERRQ(ierr);
      ierr = ISGetBlockSize(iscol,&cbs);CHKERRQ(ierr);
      ierr = MatSetBlockSizes(C,rbs,cbs);CHKERRQ(ierr);
      ierr = MatSetType(C,((PetscObject)A)->type_name);CHKERRQ(ierr);
      ierr = MatSeqAIJSetPreallocation_SeqAIJ(C,0,lens);CHKERRQ(ierr);
    }
    c = (Mat_SeqAIJ*)C->data;

    /* loop over rows inserting into submatrix */
    a_new = c->a;
    j_new = c->j;
    i_new = c->i;

    for (i=0; i<nrows; i++) {
      ii    = starts[i];
      lensi = lens[i];
      for (k=0; k<lensi; k++) {
        *j_new++ = aj[ii+k] - first;
      }
      ierr       = PetscMemcpy(a_new,a->a + starts[i],lensi*sizeof(PetscScalar));CHKERRQ(ierr);
      a_new     += lensi;
      i_new[i+1] = i_new[i] + lensi;
      c->ilen[i] = lensi;
    }
    ierr = PetscFree2(lens,starts);CHKERRQ(ierr);
  } else {
    ierr = ISGetIndices(iscol,&icol);CHKERRQ(ierr);
    ierr = PetscCalloc1(oldcols,&smap);CHKERRQ(ierr);
    ierr = PetscMalloc1(1+nrows,&lens);CHKERRQ(ierr);
    for (i=0; i<ncols; i++) {
#if defined(PETSC_USE_DEBUG)
      if (icol[i] >= oldcols) SETERRQ3(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Requesting column beyond largest column icol[%D] %D <= A->cmap->n %D",i,icol[i],oldcols);
#endif
      smap[icol[i]] = i+1;
    }

    /* determine lens of each row */
    for (i=0; i<nrows; i++) {
      kstart  = ai[irow[i]];
      kend    = kstart + a->ilen[irow[i]];
      lens[i] = 0;
      for (k=kstart; k<kend; k++) {
        if (smap[aj[k]]) {
          lens[i]++;
        }
      }
    }
    /* Create and fill new matrix */
    if (scall == MAT_REUSE_MATRIX) {
      PetscBool equal;

      c = (Mat_SeqAIJ*)((*B)->data);
      if ((*B)->rmap->n  != nrows || (*B)->cmap->n != ncols) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_SIZ,"Cannot reuse matrix. wrong size");
      ierr = PetscMemcmp(c->ilen,lens,(*B)->rmap->n*sizeof(PetscInt),&equal);CHKERRQ(ierr);
      if (!equal) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_SIZ,"Cannot reuse matrix. wrong no of nonzeros");
      ierr = PetscMemzero(c->ilen,(*B)->rmap->n*sizeof(PetscInt));CHKERRQ(ierr);
      C    = *B;
    } else {
      PetscInt rbs,cbs;
      ierr = MatCreate(PetscObjectComm((PetscObject)A),&C);CHKERRQ(ierr);
      ierr = MatSetSizes(C,nrows,ncols,PETSC_DETERMINE,PETSC_DETERMINE);CHKERRQ(ierr);
      ierr = ISGetBlockSize(isrow,&rbs);CHKERRQ(ierr);
      ierr = ISGetBlockSize(iscol,&cbs);CHKERRQ(ierr);
      ierr = MatSetBlockSizes(C,rbs,cbs);CHKERRQ(ierr);
      ierr = MatSetType(C,((PetscObject)A)->type_name);CHKERRQ(ierr);
      ierr = MatSeqAIJSetPreallocation_SeqAIJ(C,0,lens);CHKERRQ(ierr);
    }
    c = (Mat_SeqAIJ*)(C->data);
    for (i=0; i<nrows; i++) {
      row      = irow[i];
      kstart   = ai[row];
      kend     = kstart + a->ilen[row];
      mat_i    = c->i[i];
      mat_j    = c->j + mat_i;
      mat_a    = c->a + mat_i;
      mat_ilen = c->ilen + i;
      for (k=kstart; k<kend; k++) {
        if ((tcol=smap[a->j[k]])) {
          *mat_j++ = tcol - 1;
          *mat_a++ = a->a[k];
          (*mat_ilen)++;

        }
      }
    }
    /* Free work space */
    ierr = ISRestoreIndices(iscol,&icol);CHKERRQ(ierr);
    ierr = PetscFree(smap);CHKERRQ(ierr);
    ierr = PetscFree(lens);CHKERRQ(ierr);
    /* sort */
    for (i = 0; i < nrows; i++) {
      PetscInt ilen;

      mat_i = c->i[i];
      mat_j = c->j + mat_i;
      mat_a = c->a + mat_i;
      ilen  = c->ilen[i];
      ierr  = PetscSortIntWithMatScalarArray(ilen,mat_j,mat_a);CHKERRQ(ierr);
    }
  }
  ierr = MatAssemblyBegin(C,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(C,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);

  ierr = ISRestoreIndices(isrow,&irow);CHKERRQ(ierr);
  *B   = C;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatGetMultiProcBlock_SeqAIJ"
PetscErrorCode  MatGetMultiProcBlock_SeqAIJ(Mat mat,MPI_Comm subComm,MatReuse scall,Mat *subMat)
{
  PetscErrorCode ierr;
  Mat            B;

  PetscFunctionBegin;
  if (scall == MAT_INITIAL_MATRIX) {
    ierr    = MatCreate(subComm,&B);CHKERRQ(ierr);
    ierr    = MatSetSizes(B,mat->rmap->n,mat->cmap->n,mat->rmap->n,mat->cmap->n);CHKERRQ(ierr);
    ierr    = MatSetBlockSizesFromMats(B,mat,mat);CHKERRQ(ierr);
    ierr    = MatSetType(B,MATSEQAIJ);CHKERRQ(ierr);
    ierr    = MatDuplicateNoCreate_SeqAIJ(B,mat,MAT_COPY_VALUES,PETSC_TRUE);CHKERRQ(ierr);
    *subMat = B;
  } else {
    ierr = MatCopy_SeqAIJ(mat,*subMat,SAME_NONZERO_PATTERN);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatILUFactor_SeqAIJ"
PetscErrorCode MatILUFactor_SeqAIJ(Mat inA,IS row,IS col,const MatFactorInfo *info)
{
  Mat_SeqAIJ     *a = (Mat_SeqAIJ*)inA->data;
  PetscErrorCode ierr;
  Mat            outA;
  PetscBool      row_identity,col_identity;

  PetscFunctionBegin;
  if (info->levels != 0) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_SUP,"Only levels=0 supported for in-place ilu");

  ierr = ISIdentity(row,&row_identity);CHKERRQ(ierr);
  ierr = ISIdentity(col,&col_identity);CHKERRQ(ierr);

  outA             = inA;
  outA->factortype = MAT_FACTOR_LU;

  ierr = PetscObjectReference((PetscObject)row);CHKERRQ(ierr);
  ierr = ISDestroy(&a->row);CHKERRQ(ierr);

  a->row = row;

  ierr = PetscObjectReference((PetscObject)col);CHKERRQ(ierr);
  ierr = ISDestroy(&a->col);CHKERRQ(ierr);

  a->col = col;

  /* Create the inverse permutation so that it can be used in MatLUFactorNumeric() */
  ierr = ISDestroy(&a->icol);CHKERRQ(ierr);
  ierr = ISInvertPermutation(col,PETSC_DECIDE,&a->icol);CHKERRQ(ierr);
  ierr = PetscLogObjectParent((PetscObject)inA,(PetscObject)a->icol);CHKERRQ(ierr);

  if (!a->solve_work) { /* this matrix may have been factored before */
    ierr = PetscMalloc1(inA->rmap->n+1,&a->solve_work);CHKERRQ(ierr);
    ierr = PetscLogObjectMemory((PetscObject)inA, (inA->rmap->n+1)*sizeof(PetscScalar));CHKERRQ(ierr);
  }

  ierr = MatMarkDiagonal_SeqAIJ(inA);CHKERRQ(ierr);
  if (row_identity && col_identity) {
    ierr = MatLUFactorNumeric_SeqAIJ_inplace(outA,inA,info);CHKERRQ(ierr);
  } else {
    ierr = MatLUFactorNumeric_SeqAIJ_InplaceWithPerm(outA,inA,info);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatScale_SeqAIJ"
PetscErrorCode MatScale_SeqAIJ(Mat inA,PetscScalar alpha)
{
  Mat_SeqAIJ     *a     = (Mat_SeqAIJ*)inA->data;
  PetscScalar    oalpha = alpha;
  PetscErrorCode ierr;
  PetscBLASInt   one = 1,bnz;

  PetscFunctionBegin;
  ierr = PetscBLASIntCast(a->nz,&bnz);CHKERRQ(ierr);
  PetscStackCallBLAS("BLASscal",BLASscal_(&bnz,&oalpha,a->a,&one));
  ierr = PetscLogFlops(a->nz);CHKERRQ(ierr);
  ierr = MatSeqAIJInvalidateDiagonal(inA);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatGetSubMatrices_SeqAIJ"
PetscErrorCode MatGetSubMatrices_SeqAIJ(Mat A,PetscInt n,const IS irow[],const IS icol[],MatReuse scall,Mat *B[])
{
  PetscErrorCode ierr;
  PetscInt       i;

  PetscFunctionBegin;
  if (scall == MAT_INITIAL_MATRIX) {
    ierr = PetscMalloc1(n+1,B);CHKERRQ(ierr);
  }

  for (i=0; i<n; i++) {
    ierr = MatGetSubMatrix_SeqAIJ(A,irow[i],icol[i],PETSC_DECIDE,scall,&(*B)[i]);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatIncreaseOverlap_SeqAIJ"
PetscErrorCode MatIncreaseOverlap_SeqAIJ(Mat A,PetscInt is_max,IS is[],PetscInt ov)
{
  Mat_SeqAIJ     *a = (Mat_SeqAIJ*)A->data;
  PetscErrorCode ierr;
  PetscInt       row,i,j,k,l,m,n,*nidx,isz,val;
  const PetscInt *idx;
  PetscInt       start,end,*ai,*aj;
  PetscBT        table;

  PetscFunctionBegin;
  m  = A->rmap->n;
  ai = a->i;
  aj = a->j;

  if (ov < 0) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"illegal negative overlap value used");

  ierr = PetscMalloc1(m+1,&nidx);CHKERRQ(ierr);
  ierr = PetscBTCreate(m,&table);CHKERRQ(ierr);

  for (i=0; i<is_max; i++) {
    /* Initialize the two local arrays */
    isz  = 0;
    ierr = PetscBTMemzero(m,table);CHKERRQ(ierr);

    /* Extract the indices, assume there can be duplicate entries */
    ierr = ISGetIndices(is[i],&idx);CHKERRQ(ierr);
    ierr = ISGetLocalSize(is[i],&n);CHKERRQ(ierr);

    /* Enter these into the temp arrays. I.e., mark table[row], enter row into new index */
    for (j=0; j<n; ++j) {
      if (!PetscBTLookupSet(table,idx[j])) nidx[isz++] = idx[j];
    }
    ierr = ISRestoreIndices(is[i],&idx);CHKERRQ(ierr);
    ierr = ISDestroy(&is[i]);CHKERRQ(ierr);

    k = 0;
    for (j=0; j<ov; j++) { /* for each overlap */
      n = isz;
      for (; k<n; k++) { /* do only those rows in nidx[k], which are not done yet */
        row   = nidx[k];
        start = ai[row];
        end   = ai[row+1];
        for (l = start; l<end; l++) {
          val = aj[l];
          if (!PetscBTLookupSet(table,val)) nidx[isz++] = val;
        }
      }
    }
    ierr = ISCreateGeneral(PETSC_COMM_SELF,isz,nidx,PETSC_COPY_VALUES,(is+i));CHKERRQ(ierr);
  }
  ierr = PetscBTDestroy(&table);CHKERRQ(ierr);
  ierr = PetscFree(nidx);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/* -------------------------------------------------------------- */
#undef __FUNCT__
#define __FUNCT__ "MatPermute_SeqAIJ"
PetscErrorCode MatPermute_SeqAIJ(Mat A,IS rowp,IS colp,Mat *B)
{
  Mat_SeqAIJ     *a = (Mat_SeqAIJ*)A->data;
  PetscErrorCode ierr;
  PetscInt       i,nz = 0,m = A->rmap->n,n = A->cmap->n;
  const PetscInt *row,*col;
  PetscInt       *cnew,j,*lens;
  IS             icolp,irowp;
  PetscInt       *cwork = NULL;
  PetscScalar    *vwork = NULL;

  PetscFunctionBegin;
  ierr = ISInvertPermutation(rowp,PETSC_DECIDE,&irowp);CHKERRQ(ierr);
  ierr = ISGetIndices(irowp,&row);CHKERRQ(ierr);
  ierr = ISInvertPermutation(colp,PETSC_DECIDE,&icolp);CHKERRQ(ierr);
  ierr = ISGetIndices(icolp,&col);CHKERRQ(ierr);

  /* determine lengths of permuted rows */
  ierr = PetscMalloc1(m+1,&lens);CHKERRQ(ierr);
  for (i=0; i<m; i++) lens[row[i]] = a->i[i+1] - a->i[i];
  ierr = MatCreate(PetscObjectComm((PetscObject)A),B);CHKERRQ(ierr);
  ierr = MatSetSizes(*B,m,n,m,n);CHKERRQ(ierr);
  ierr = MatSetBlockSizesFromMats(*B,A,A);CHKERRQ(ierr);
  ierr = MatSetType(*B,((PetscObject)A)->type_name);CHKERRQ(ierr);
  ierr = MatSeqAIJSetPreallocation_SeqAIJ(*B,0,lens);CHKERRQ(ierr);
  ierr = PetscFree(lens);CHKERRQ(ierr);

  ierr = PetscMalloc1(n,&cnew);CHKERRQ(ierr);
  for (i=0; i<m; i++) {
    ierr = MatGetRow_SeqAIJ(A,i,&nz,&cwork,&vwork);CHKERRQ(ierr);
    for (j=0; j<nz; j++) cnew[j] = col[cwork[j]];
    ierr = MatSetValues_SeqAIJ(*B,1,&row[i],nz,cnew,vwork,INSERT_VALUES);CHKERRQ(ierr);
    ierr = MatRestoreRow_SeqAIJ(A,i,&nz,&cwork,&vwork);CHKERRQ(ierr);
  }
  ierr = PetscFree(cnew);CHKERRQ(ierr);

  (*B)->assembled = PETSC_FALSE;

  ierr = MatAssemblyBegin(*B,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(*B,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = ISRestoreIndices(irowp,&row);CHKERRQ(ierr);
  ierr = ISRestoreIndices(icolp,&col);CHKERRQ(ierr);
  ierr = ISDestroy(&irowp);CHKERRQ(ierr);
  ierr = ISDestroy(&icolp);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatCopy_SeqAIJ"
PetscErrorCode MatCopy_SeqAIJ(Mat A,Mat B,MatStructure str)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  /* If the two matrices have the same copy implementation, use fast copy. */
  if (str == SAME_NONZERO_PATTERN && (A->ops->copy == B->ops->copy)) {
    Mat_SeqAIJ *a = (Mat_SeqAIJ*)A->data;
    Mat_SeqAIJ *b = (Mat_SeqAIJ*)B->data;

    if (a->i[A->rmap->n] != b->i[B->rmap->n]) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_INCOMP,"Number of nonzeros in two matrices are different");
    ierr = PetscMemcpy(b->a,a->a,(a->i[A->rmap->n])*sizeof(PetscScalar));CHKERRQ(ierr);
  } else {
    ierr = MatCopy_Basic(A,B,str);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatSetUp_SeqAIJ"
PetscErrorCode MatSetUp_SeqAIJ(Mat A)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr =  MatSeqAIJSetPreallocation_SeqAIJ(A,PETSC_DEFAULT,0);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatSeqAIJGetArray_SeqAIJ"
PetscErrorCode MatSeqAIJGetArray_SeqAIJ(Mat A,PetscScalar *array[])
{
  Mat_SeqAIJ *a = (Mat_SeqAIJ*)A->data;

  PetscFunctionBegin;
  *array = a->a;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatSeqAIJRestoreArray_SeqAIJ"
PetscErrorCode MatSeqAIJRestoreArray_SeqAIJ(Mat A,PetscScalar *array[])
{
  PetscFunctionBegin;
  PetscFunctionReturn(0);
}

/*
   Computes the number of nonzeros per row needed for preallocation when X and Y
   have different nonzero structure.
*/
#undef __FUNCT__
#define __FUNCT__ "MatAXPYGetPreallocation_SeqX_private"
PetscErrorCode MatAXPYGetPreallocation_SeqX_private(PetscInt m,const PetscInt *xi,const PetscInt *xj,const PetscInt *yi,const PetscInt *yj,PetscInt *nnz)
{
  PetscInt       i,j,k,nzx,nzy;

  PetscFunctionBegin;
  /* Set the number of nonzeros in the new matrix */
  for (i=0; i<m; i++) {
    const PetscInt *xjj = xj+xi[i],*yjj = yj+yi[i];
    nzx = xi[i+1] - xi[i];
    nzy = yi[i+1] - yi[i];
    nnz[i] = 0;
    for (j=0,k=0; j<nzx; j++) {                   /* Point in X */
      for (; k<nzy && yjj[k]<xjj[j]; k++) nnz[i]++; /* Catch up to X */
      if (k<nzy && yjj[k]==xjj[j]) k++;             /* Skip duplicate */
      nnz[i]++;
    }
    for (; k<nzy; k++) nnz[i]++;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatAXPYGetPreallocation_SeqAIJ"
PetscErrorCode MatAXPYGetPreallocation_SeqAIJ(Mat Y,Mat X,PetscInt *nnz)
{
  PetscInt       m = Y->rmap->N;
  Mat_SeqAIJ     *x = (Mat_SeqAIJ*)X->data;
  Mat_SeqAIJ     *y = (Mat_SeqAIJ*)Y->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  /* Set the number of nonzeros in the new matrix */
  ierr = MatAXPYGetPreallocation_SeqX_private(m,x->i,x->j,y->i,y->j,nnz);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatAXPY_SeqAIJ"
PetscErrorCode MatAXPY_SeqAIJ(Mat Y,PetscScalar a,Mat X,MatStructure str)
{
  PetscErrorCode ierr;
  Mat_SeqAIJ     *x = (Mat_SeqAIJ*)X->data,*y = (Mat_SeqAIJ*)Y->data;
  PetscBLASInt   one=1,bnz;

  PetscFunctionBegin;
  ierr = PetscBLASIntCast(x->nz,&bnz);CHKERRQ(ierr);
  if (str == SAME_NONZERO_PATTERN) {
    PetscScalar alpha = a;
    PetscStackCallBLAS("BLASaxpy",BLASaxpy_(&bnz,&alpha,x->a,&one,y->a,&one));
    ierr = MatSeqAIJInvalidateDiagonal(Y);CHKERRQ(ierr);
    ierr = PetscObjectStateIncrease((PetscObject)Y);CHKERRQ(ierr);
  } else if (str == SUBSET_NONZERO_PATTERN) { /* nonzeros of X is a subset of Y's */
    ierr = MatAXPY_Basic(Y,a,X,str);CHKERRQ(ierr);
  } else {
    Mat      B;
    PetscInt *nnz;
    ierr = PetscMalloc1(Y->rmap->N,&nnz);CHKERRQ(ierr);
    ierr = MatCreate(PetscObjectComm((PetscObject)Y),&B);CHKERRQ(ierr);
    ierr = PetscObjectSetName((PetscObject)B,((PetscObject)Y)->name);CHKERRQ(ierr);
    ierr = MatSetSizes(B,Y->rmap->n,Y->cmap->n,Y->rmap->N,Y->cmap->N);CHKERRQ(ierr);
    ierr = MatSetBlockSizesFromMats(B,Y,Y);CHKERRQ(ierr);
    ierr = MatSetType(B,(MatType) ((PetscObject)Y)->type_name);CHKERRQ(ierr);
    ierr = MatAXPYGetPreallocation_SeqAIJ(Y,X,nnz);CHKERRQ(ierr);
    ierr = MatSeqAIJSetPreallocation(B,0,nnz);CHKERRQ(ierr);
    ierr = MatAXPY_BasicWithPreallocation(B,Y,a,X,str);CHKERRQ(ierr);
    ierr = MatHeaderReplace(Y,&B);CHKERRQ(ierr);
    ierr = PetscFree(nnz);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatConjugate_SeqAIJ"
PetscErrorCode  MatConjugate_SeqAIJ(Mat mat)
{
#if defined(PETSC_USE_COMPLEX)
  Mat_SeqAIJ  *aij = (Mat_SeqAIJ*)mat->data;
  PetscInt    i,nz;
  PetscScalar *a;

  PetscFunctionBegin;
  nz = aij->nz;
  a  = aij->a;
  for (i=0; i<nz; i++) a[i] = PetscConj(a[i]);
#else
  PetscFunctionBegin;
#endif
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatGetRowMaxAbs_SeqAIJ"
PetscErrorCode MatGetRowMaxAbs_SeqAIJ(Mat A,Vec v,PetscInt idx[])
{
  Mat_SeqAIJ     *a = (Mat_SeqAIJ*)A->data;
  PetscErrorCode ierr;
  PetscInt       i,j,m = A->rmap->n,*ai,*aj,ncols,n;
  PetscReal      atmp;
  PetscScalar    *x;
  MatScalar      *aa;

  PetscFunctionBegin;
  if (A->factortype) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONGSTATE,"Not for factored matrix");
  aa = a->a;
  ai = a->i;
  aj = a->j;

  ierr = VecSet(v,0.0);CHKERRQ(ierr);
  ierr = VecGetArray(v,&x);CHKERRQ(ierr);
  ierr = VecGetLocalSize(v,&n);CHKERRQ(ierr);
  if (n != A->rmap->n) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_SIZ,"Nonconforming matrix and vector");
  for (i=0; i<m; i++) {
    ncols = ai[1] - ai[0]; ai++;
    x[i]  = 0.0;
    for (j=0; j<ncols; j++) {
      atmp = PetscAbsScalar(*aa);
      if (PetscAbsScalar(x[i]) < atmp) {x[i] = atmp; if (idx) idx[i] = *aj;}
      aa++; aj++;
    }
  }
  ierr = VecRestoreArray(v,&x);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatGetRowMax_SeqAIJ"
PetscErrorCode MatGetRowMax_SeqAIJ(Mat A,Vec v,PetscInt idx[])
{
  Mat_SeqAIJ     *a = (Mat_SeqAIJ*)A->data;
  PetscErrorCode ierr;
  PetscInt       i,j,m = A->rmap->n,*ai,*aj,ncols,n;
  PetscScalar    *x;
  MatScalar      *aa;

  PetscFunctionBegin;
  if (A->factortype) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONGSTATE,"Not for factored matrix");
  aa = a->a;
  ai = a->i;
  aj = a->j;

  ierr = VecSet(v,0.0);CHKERRQ(ierr);
  ierr = VecGetArray(v,&x);CHKERRQ(ierr);
  ierr = VecGetLocalSize(v,&n);CHKERRQ(ierr);
  if (n != A->rmap->n) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_SIZ,"Nonconforming matrix and vector");
  for (i=0; i<m; i++) {
    ncols = ai[1] - ai[0]; ai++;
    if (ncols == A->cmap->n) { /* row is dense */
      x[i] = *aa; if (idx) idx[i] = 0;
    } else {  /* row is sparse so already KNOW maximum is 0.0 or higher */
      x[i] = 0.0;
      if (idx) {
        idx[i] = 0; /* in case ncols is zero */
        for (j=0;j<ncols;j++) { /* find first implicit 0.0 in the row */
          if (aj[j] > j) {
            idx[i] = j;
            break;
          }
        }
      }
    }
    for (j=0; j<ncols; j++) {
      if (PetscRealPart(x[i]) < PetscRealPart(*aa)) {x[i] = *aa; if (idx) idx[i] = *aj;}
      aa++; aj++;
    }
  }
  ierr = VecRestoreArray(v,&x);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatGetRowMinAbs_SeqAIJ"
PetscErrorCode MatGetRowMinAbs_SeqAIJ(Mat A,Vec v,PetscInt idx[])
{
  Mat_SeqAIJ     *a = (Mat_SeqAIJ*)A->data;
  PetscErrorCode ierr;
  PetscInt       i,j,m = A->rmap->n,*ai,*aj,ncols,n;
  PetscReal      atmp;
  PetscScalar    *x;
  MatScalar      *aa;

  PetscFunctionBegin;
  if (A->factortype) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONGSTATE,"Not for factored matrix");
  aa = a->a;
  ai = a->i;
  aj = a->j;

  ierr = VecSet(v,0.0);CHKERRQ(ierr);
  ierr = VecGetArray(v,&x);CHKERRQ(ierr);
  ierr = VecGetLocalSize(v,&n);CHKERRQ(ierr);
  if (n != A->rmap->n) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_SIZ,"Nonconforming matrix and vector, %D vs. %D rows", A->rmap->n, n);
  for (i=0; i<m; i++) {
    ncols = ai[1] - ai[0]; ai++;
    if (ncols) {
      /* Get first nonzero */
      for (j = 0; j < ncols; j++) {
        atmp = PetscAbsScalar(aa[j]);
        if (atmp > 1.0e-12) {
          x[i] = atmp;
          if (idx) idx[i] = aj[j];
          break;
        }
      }
      if (j == ncols) {x[i] = PetscAbsScalar(*aa); if (idx) idx[i] = *aj;}
    } else {
      x[i] = 0.0; if (idx) idx[i] = 0;
    }
    for (j = 0; j < ncols; j++) {
      atmp = PetscAbsScalar(*aa);
      if (atmp > 1.0e-12 && PetscAbsScalar(x[i]) > atmp) {x[i] = atmp; if (idx) idx[i] = *aj;}
      aa++; aj++;
    }
  }
  ierr = VecRestoreArray(v,&x);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatGetRowMin_SeqAIJ"
PetscErrorCode MatGetRowMin_SeqAIJ(Mat A,Vec v,PetscInt idx[])
{
  Mat_SeqAIJ      *a = (Mat_SeqAIJ*)A->data;
  PetscErrorCode  ierr;
  PetscInt        i,j,m = A->rmap->n,ncols,n;
  const PetscInt  *ai,*aj;
  PetscScalar     *x;
  const MatScalar *aa;

  PetscFunctionBegin;
  if (A->factortype) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONGSTATE,"Not for factored matrix");
  aa = a->a;
  ai = a->i;
  aj = a->j;

  ierr = VecSet(v,0.0);CHKERRQ(ierr);
  ierr = VecGetArray(v,&x);CHKERRQ(ierr);
  ierr = VecGetLocalSize(v,&n);CHKERRQ(ierr);
  if (n != A->rmap->n) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_SIZ,"Nonconforming matrix and vector");
  for (i=0; i<m; i++) {
    ncols = ai[1] - ai[0]; ai++;
    if (ncols == A->cmap->n) { /* row is dense */
      x[i] = *aa; if (idx) idx[i] = 0;
    } else {  /* row is sparse so already KNOW minimum is 0.0 or lower */
      x[i] = 0.0;
      if (idx) {   /* find first implicit 0.0 in the row */
        idx[i] = 0; /* in case ncols is zero */
        for (j=0; j<ncols; j++) {
          if (aj[j] > j) {
            idx[i] = j;
            break;
          }
        }
      }
    }
    for (j=0; j<ncols; j++) {
      if (PetscRealPart(x[i]) > PetscRealPart(*aa)) {x[i] = *aa; if (idx) idx[i] = *aj;}
      aa++; aj++;
    }
  }
  ierr = VecRestoreArray(v,&x);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#include <petscblaslapack.h>
#include <petsc/private/kernels/blockinvert.h>

#undef __FUNCT__
#define __FUNCT__ "MatInvertBlockDiagonal_SeqAIJ"
PetscErrorCode MatInvertBlockDiagonal_SeqAIJ(Mat A,const PetscScalar **values)
{
  Mat_SeqAIJ     *a = (Mat_SeqAIJ*) A->data;
  PetscErrorCode ierr;
  PetscInt       i,bs = PetscAbs(A->rmap->bs),mbs = A->rmap->n/bs,ipvt[5],bs2 = bs*bs,*v_pivots,ij[7],*IJ,j;
  MatScalar      *diag,work[25],*v_work;
  PetscReal      shift = 0.0;
  PetscBool      allowzeropivot,zeropivotdetected=PETSC_FALSE;

  PetscFunctionBegin;
  allowzeropivot = PetscNot(A->erroriffailure);
  if (a->ibdiagvalid) {
    if (values) *values = a->ibdiag;
    PetscFunctionReturn(0);
  }
  ierr = MatMarkDiagonal_SeqAIJ(A);CHKERRQ(ierr);
  if (!a->ibdiag) {
    ierr = PetscMalloc1(bs2*mbs,&a->ibdiag);CHKERRQ(ierr);
    ierr = PetscLogObjectMemory((PetscObject)A,bs2*mbs*sizeof(PetscScalar));CHKERRQ(ierr);
  }
  diag = a->ibdiag;
  if (values) *values = a->ibdiag;
  /* factor and invert each block */
  switch (bs) {
  case 1:
    for (i=0; i<mbs; i++) {
      ierr    = MatGetValues(A,1,&i,1,&i,diag+i);CHKERRQ(ierr);
      if (PetscAbsScalar(diag[i] + shift) < PETSC_MACHINE_EPSILON) {
        if (allowzeropivot) {
          A->errortype = MAT_FACTOR_NUMERIC_ZEROPIVOT;
          ierr = PetscInfo1(A,"Zero pivot, row %D\n",i);CHKERRQ(ierr);
        } else SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_MAT_LU_ZRPVT,"Zero pivot, row %D",i);
      } 
      diag[i] = (PetscScalar)1.0 / (diag[i] + shift);
    }
    break;
  case 2:
    for (i=0; i<mbs; i++) {
      ij[0] = 2*i; ij[1] = 2*i + 1;
      ierr  = MatGetValues(A,2,ij,2,ij,diag);CHKERRQ(ierr);
      ierr  = PetscKernel_A_gets_inverse_A_2(diag,shift,allowzeropivot,&zeropivotdetected);CHKERRQ(ierr);
      if (zeropivotdetected) A->errortype = MAT_FACTOR_NUMERIC_ZEROPIVOT;
      ierr  = PetscKernel_A_gets_transpose_A_2(diag);CHKERRQ(ierr);
      diag += 4;
    }
    break;
  case 3:
    for (i=0; i<mbs; i++) {
      ij[0] = 3*i; ij[1] = 3*i + 1; ij[2] = 3*i + 2;
      ierr  = MatGetValues(A,3,ij,3,ij,diag);CHKERRQ(ierr);
      ierr  = PetscKernel_A_gets_inverse_A_3(diag,shift,allowzeropivot,&zeropivotdetected);CHKERRQ(ierr);
      if (zeropivotdetected) A->errortype = MAT_FACTOR_NUMERIC_ZEROPIVOT;
      ierr  = PetscKernel_A_gets_transpose_A_3(diag);CHKERRQ(ierr);
      diag += 9;
    }
    break;
  case 4:
    for (i=0; i<mbs; i++) {
      ij[0] = 4*i; ij[1] = 4*i + 1; ij[2] = 4*i + 2; ij[3] = 4*i + 3;
      ierr  = MatGetValues(A,4,ij,4,ij,diag);CHKERRQ(ierr);
      ierr  = PetscKernel_A_gets_inverse_A_4(diag,shift,allowzeropivot,&zeropivotdetected);CHKERRQ(ierr);
      if (zeropivotdetected) A->errortype = MAT_FACTOR_NUMERIC_ZEROPIVOT;
      ierr  = PetscKernel_A_gets_transpose_A_4(diag);CHKERRQ(ierr);
      diag += 16;
    }
    break;
  case 5:
    for (i=0; i<mbs; i++) {
      ij[0] = 5*i; ij[1] = 5*i + 1; ij[2] = 5*i + 2; ij[3] = 5*i + 3; ij[4] = 5*i + 4;
      ierr  = MatGetValues(A,5,ij,5,ij,diag);CHKERRQ(ierr);
      ierr  = PetscKernel_A_gets_inverse_A_5(diag,ipvt,work,shift,allowzeropivot,&zeropivotdetected);CHKERRQ(ierr);
      if (zeropivotdetected) A->errortype = MAT_FACTOR_NUMERIC_ZEROPIVOT;
      ierr  = PetscKernel_A_gets_transpose_A_5(diag);CHKERRQ(ierr);
      diag += 25;
    }
    break;
  case 6:
    for (i=0; i<mbs; i++) {
      ij[0] = 6*i; ij[1] = 6*i + 1; ij[2] = 6*i + 2; ij[3] = 6*i + 3; ij[4] = 6*i + 4; ij[5] = 6*i + 5;
      ierr  = MatGetValues(A,6,ij,6,ij,diag);CHKERRQ(ierr);
      ierr  = PetscKernel_A_gets_inverse_A_6(diag,shift,allowzeropivot,&zeropivotdetected);CHKERRQ(ierr);
      if (zeropivotdetected) A->errortype = MAT_FACTOR_NUMERIC_ZEROPIVOT;
      ierr  = PetscKernel_A_gets_transpose_A_6(diag);CHKERRQ(ierr);
      diag += 36;
    }
    break;
  case 7:
    for (i=0; i<mbs; i++) {
      ij[0] = 7*i; ij[1] = 7*i + 1; ij[2] = 7*i + 2; ij[3] = 7*i + 3; ij[4] = 7*i + 4; ij[5] = 7*i + 5; ij[5] = 7*i + 6;
      ierr  = MatGetValues(A,7,ij,7,ij,diag);CHKERRQ(ierr);
      ierr  = PetscKernel_A_gets_inverse_A_7(diag,shift,allowzeropivot,&zeropivotdetected);CHKERRQ(ierr);
      if (zeropivotdetected) A->errortype = MAT_FACTOR_NUMERIC_ZEROPIVOT;
      ierr  = PetscKernel_A_gets_transpose_A_7(diag);CHKERRQ(ierr);
      diag += 49;
    }
    break;
  default:
    ierr = PetscMalloc3(bs,&v_work,bs,&v_pivots,bs,&IJ);CHKERRQ(ierr);
    for (i=0; i<mbs; i++) {
      for (j=0; j<bs; j++) {
        IJ[j] = bs*i + j;
      }
      ierr  = MatGetValues(A,bs,IJ,bs,IJ,diag);CHKERRQ(ierr);
      ierr  = PetscKernel_A_gets_inverse_A(bs,diag,v_pivots,v_work,allowzeropivot,&zeropivotdetected);CHKERRQ(ierr);
      if (zeropivotdetected) A->errortype = MAT_FACTOR_NUMERIC_ZEROPIVOT;
      ierr  = PetscKernel_A_gets_transpose_A_N(diag,bs);CHKERRQ(ierr);
      diag += bs2;
    }
    ierr = PetscFree3(v_work,v_pivots,IJ);CHKERRQ(ierr);
  }
  a->ibdiagvalid = PETSC_TRUE;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatSetRandom_SeqAIJ"
static PetscErrorCode  MatSetRandom_SeqAIJ(Mat x,PetscRandom rctx)
{
  PetscErrorCode ierr;
  Mat_SeqAIJ     *aij = (Mat_SeqAIJ*)x->data;
  PetscScalar    a;
  PetscInt       m,n,i,j,col;

  PetscFunctionBegin;
  if (!x->assembled) {
    ierr = MatGetSize(x,&m,&n);CHKERRQ(ierr);
    for (i=0; i<m; i++) {
      for (j=0; j<aij->imax[i]; j++) {
        ierr = PetscRandomGetValue(rctx,&a);CHKERRQ(ierr);
        col  = (PetscInt)(n*PetscRealPart(a));
        ierr = MatSetValues(x,1,&i,1,&col,&a,ADD_VALUES);CHKERRQ(ierr);
      }
    }
  } else SETERRQ(PETSC_COMM_SELF,PETSC_ERR_SUP,"Not yet coded");
  ierr = MatAssemblyBegin(x,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(x,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatShift_SeqAIJ"
PetscErrorCode MatShift_SeqAIJ(Mat Y,PetscScalar a)
{
  PetscErrorCode ierr;
  Mat_SeqAIJ     *aij = (Mat_SeqAIJ*)Y->data;

  PetscFunctionBegin;
  if (!Y->preallocated || !aij->nz) {
    ierr = MatSeqAIJSetPreallocation(Y,1,NULL);CHKERRQ(ierr);
  }
  ierr = MatShift_Basic(Y,a);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/* -------------------------------------------------------------------*/
static struct _MatOps MatOps_Values = { MatSetValues_SeqAIJ,
                                        MatGetRow_SeqAIJ,
                                        MatRestoreRow_SeqAIJ,
                                        MatMult_SeqAIJ,
                                /*  4*/ MatMultAdd_SeqAIJ,
                                        MatMultTranspose_SeqAIJ,
                                        MatMultTransposeAdd_SeqAIJ,
                                        0,
                                        0,
                                        0,
                                /* 10*/ 0,
                                        MatLUFactor_SeqAIJ,
                                        0,
                                        MatSOR_SeqAIJ,
                                        MatTranspose_SeqAIJ,
                                /*1 5*/ MatGetInfo_SeqAIJ,
                                        MatEqual_SeqAIJ,
                                        MatGetDiagonal_SeqAIJ,
                                        MatDiagonalScale_SeqAIJ,
                                        MatNorm_SeqAIJ,
                                /* 20*/ 0,
                                        MatAssemblyEnd_SeqAIJ,
                                        MatSetOption_SeqAIJ,
                                        MatZeroEntries_SeqAIJ,
                                /* 24*/ MatZeroRows_SeqAIJ,
                                        0,
                                        0,
                                        0,
                                        0,
                                /* 29*/ MatSetUp_SeqAIJ,
                                        0,
                                        0,
                                        0,
                                        0,
                                /* 34*/ MatDuplicate_SeqAIJ,
                                        0,
                                        0,
                                        MatILUFactor_SeqAIJ,
                                        0,
                                /* 39*/ MatAXPY_SeqAIJ,
                                        MatGetSubMatrices_SeqAIJ,
                                        MatIncreaseOverlap_SeqAIJ,
                                        MatGetValues_SeqAIJ,
                                        MatCopy_SeqAIJ,
                                /* 44*/ MatGetRowMax_SeqAIJ,
                                        MatScale_SeqAIJ,
                                        MatShift_SeqAIJ,
                                        MatDiagonalSet_SeqAIJ,
                                        MatZeroRowsColumns_SeqAIJ,
                                /* 49*/ MatSetRandom_SeqAIJ,
                                        MatGetRowIJ_SeqAIJ,
                                        MatRestoreRowIJ_SeqAIJ,
                                        MatGetColumnIJ_SeqAIJ,
                                        MatRestoreColumnIJ_SeqAIJ,
                                /* 54*/ MatFDColoringCreate_SeqXAIJ,
                                        0,
                                        0,
                                        MatPermute_SeqAIJ,
                                        0,
                                /* 59*/ 0,
                                        MatDestroy_SeqAIJ,
                                        MatView_SeqAIJ,
                                        0,
                                        MatMatMatMult_SeqAIJ_SeqAIJ_SeqAIJ,
                                /* 64*/ MatMatMatMultSymbolic_SeqAIJ_SeqAIJ_SeqAIJ,
                                        MatMatMatMultNumeric_SeqAIJ_SeqAIJ_SeqAIJ,
                                        0,
                                        0,
                                        0,
                                /* 69*/ MatGetRowMaxAbs_SeqAIJ,
                                        MatGetRowMinAbs_SeqAIJ,
                                        0,
                                        MatSetColoring_SeqAIJ,
                                        0,
                                /* 74*/ MatSetValuesAdifor_SeqAIJ,
                                        MatFDColoringApply_AIJ,
                                        0,
                                        0,
                                        0,
                                /* 79*/ MatFindZeroDiagonals_SeqAIJ,
                                        0,
                                        0,
                                        0,
                                        MatLoad_SeqAIJ,
                                /* 84*/ MatIsSymmetric_SeqAIJ,
                                        MatIsHermitian_SeqAIJ,
                                        0,
                                        0,
                                        0,
                                /* 89*/ MatMatMult_SeqAIJ_SeqAIJ,
                                        MatMatMultSymbolic_SeqAIJ_SeqAIJ,
                                        MatMatMultNumeric_SeqAIJ_SeqAIJ,
                                        MatPtAP_SeqAIJ_SeqAIJ,
                                        MatPtAPSymbolic_SeqAIJ_SeqAIJ_DenseAxpy,
                                /* 94*/ MatPtAPNumeric_SeqAIJ_SeqAIJ,
                                        MatMatTransposeMult_SeqAIJ_SeqAIJ,
                                        MatMatTransposeMultSymbolic_SeqAIJ_SeqAIJ,
                                        MatMatTransposeMultNumeric_SeqAIJ_SeqAIJ,
                                        0,
                                /* 99*/ 0,
                                        0,
                                        0,
                                        MatConjugate_SeqAIJ,
                                        0,
                                /*104*/ MatSetValuesRow_SeqAIJ,
                                        MatRealPart_SeqAIJ,
                                        MatImaginaryPart_SeqAIJ,
                                        0,
                                        0,
                                /*109*/ MatMatSolve_SeqAIJ,
                                        0,
                                        MatGetRowMin_SeqAIJ,
                                        0,
                                        MatMissingDiagonal_SeqAIJ,
                                /*114*/ 0,
                                        0,
                                        0,
                                        0,
                                        0,
                                /*119*/ 0,
                                        0,
                                        0,
                                        0,
                                        MatGetMultiProcBlock_SeqAIJ,
                                /*124*/ MatFindNonzeroRows_SeqAIJ,
                                        MatGetColumnNorms_SeqAIJ,
                                        MatInvertBlockDiagonal_SeqAIJ,
                                        0,
                                        0,
                                /*129*/ 0,
                                        MatTransposeMatMult_SeqAIJ_SeqAIJ,
                                        MatTransposeMatMultSymbolic_SeqAIJ_SeqAIJ,
                                        MatTransposeMatMultNumeric_SeqAIJ_SeqAIJ,
                                        MatTransposeColoringCreate_SeqAIJ,
                                /*134*/ MatTransColoringApplySpToDen_SeqAIJ,
                                        MatTransColoringApplyDenToSp_SeqAIJ,
                                        MatRARt_SeqAIJ_SeqAIJ,
                                        MatRARtSymbolic_SeqAIJ_SeqAIJ,
                                        MatRARtNumeric_SeqAIJ_SeqAIJ,
                                 /*139*/0,
                                        0,
                                        0,
                                        MatFDColoringSetUp_SeqXAIJ,
                                        MatFindOffBlockDiagonalEntries_SeqAIJ,
                                 /*144*/MatCreateMPIMatConcatenateSeqMat_SeqAIJ
};

#undef __FUNCT__
#define __FUNCT__ "MatSeqAIJSetColumnIndices_SeqAIJ"
PetscErrorCode  MatSeqAIJSetColumnIndices_SeqAIJ(Mat mat,PetscInt *indices)
{
  Mat_SeqAIJ *aij = (Mat_SeqAIJ*)mat->data;
  PetscInt   i,nz,n;

  PetscFunctionBegin;
  nz = aij->maxnz;
  n  = mat->rmap->n;
  for (i=0; i<nz; i++) {
    aij->j[i] = indices[i];
  }
  aij->nz = nz;
  for (i=0; i<n; i++) {
    aij->ilen[i] = aij->imax[i];
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatSeqAIJSetColumnIndices"
/*@
    MatSeqAIJSetColumnIndices - Set the column indices for all the rows
       in the matrix.

  Input Parameters:
+  mat - the SeqAIJ matrix
-  indices - the column indices

  Level: advanced

  Notes:
    This can be called if you have precomputed the nonzero structure of the
  matrix and want to provide it to the matrix object to improve the performance
  of the MatSetValues() operation.

    You MUST have set the correct numbers of nonzeros per row in the call to
  MatCreateSeqAIJ(), and the columns indices MUST be sorted.

    MUST be called before any calls to MatSetValues();

    The indices should start with zero, not one.

@*/
PetscErrorCode  MatSeqAIJSetColumnIndices(Mat mat,PetscInt *indices)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(mat,MAT_CLASSID,1);
  PetscValidPointer(indices,2);
  ierr = PetscUseMethod(mat,"MatSeqAIJSetColumnIndices_C",(Mat,PetscInt*),(mat,indices));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/* ----------------------------------------------------------------------------------------*/

#undef __FUNCT__
#define __FUNCT__ "MatStoreValues_SeqAIJ"
PetscErrorCode  MatStoreValues_SeqAIJ(Mat mat)
{
  Mat_SeqAIJ     *aij = (Mat_SeqAIJ*)mat->data;
  PetscErrorCode ierr;
  size_t         nz = aij->i[mat->rmap->n];

  PetscFunctionBegin;
  if (!aij->nonew) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ORDER,"Must call MatSetOption(A,MAT_NEW_NONZERO_LOCATIONS,PETSC_FALSE);first");

  /* allocate space for values if not already there */
  if (!aij->saved_values) {
    ierr = PetscMalloc1(nz+1,&aij->saved_values);CHKERRQ(ierr);
    ierr = PetscLogObjectMemory((PetscObject)mat,(nz+1)*sizeof(PetscScalar));CHKERRQ(ierr);
  }

  /* copy values over */
  ierr = PetscMemcpy(aij->saved_values,aij->a,nz*sizeof(PetscScalar));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatStoreValues"
/*@
    MatStoreValues - Stashes a copy of the matrix values; this allows, for
       example, reuse of the linear part of a Jacobian, while recomputing the
       nonlinear portion.

   Collect on Mat

  Input Parameters:
.  mat - the matrix (currently only AIJ matrices support this option)

  Level: advanced

  Common Usage, with SNESSolve():
$    Create Jacobian matrix
$    Set linear terms into matrix
$    Apply boundary conditions to matrix, at this time matrix must have
$      final nonzero structure (i.e. setting the nonlinear terms and applying
$      boundary conditions again will not change the nonzero structure
$    ierr = MatSetOption(mat,MAT_NEW_NONZERO_LOCATIONS,PETSC_FALSE);
$    ierr = MatStoreValues(mat);
$    Call SNESSetJacobian() with matrix
$    In your Jacobian routine
$      ierr = MatRetrieveValues(mat);
$      Set nonlinear terms in matrix

  Common Usage without SNESSolve(), i.e. when you handle nonlinear solve yourself:
$    // build linear portion of Jacobian
$    ierr = MatSetOption(mat,MAT_NEW_NONZERO_LOCATIONS,PETSC_FALSE);
$    ierr = MatStoreValues(mat);
$    loop over nonlinear iterations
$       ierr = MatRetrieveValues(mat);
$       // call MatSetValues(mat,...) to set nonliner portion of Jacobian
$       // call MatAssemblyBegin/End() on matrix
$       Solve linear system with Jacobian
$    endloop

  Notes:
    Matrix must already be assemblied before calling this routine
    Must set the matrix option MatSetOption(mat,MAT_NEW_NONZERO_LOCATIONS,PETSC_FALSE); before
    calling this routine.

    When this is called multiple times it overwrites the previous set of stored values
    and does not allocated additional space.

.seealso: MatRetrieveValues()

@*/
PetscErrorCode  MatStoreValues(Mat mat)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(mat,MAT_CLASSID,1);
  if (!mat->assembled) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONGSTATE,"Not for unassembled matrix");
  if (mat->factortype) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONGSTATE,"Not for factored matrix");
  ierr = PetscUseMethod(mat,"MatStoreValues_C",(Mat),(mat));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatRetrieveValues_SeqAIJ"
PetscErrorCode  MatRetrieveValues_SeqAIJ(Mat mat)
{
  Mat_SeqAIJ     *aij = (Mat_SeqAIJ*)mat->data;
  PetscErrorCode ierr;
  PetscInt       nz = aij->i[mat->rmap->n];

  PetscFunctionBegin;
  if (!aij->nonew) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ORDER,"Must call MatSetOption(A,MAT_NEW_NONZERO_LOCATIONS,PETSC_FALSE);first");
  if (!aij->saved_values) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ORDER,"Must call MatStoreValues(A);first");
  /* copy values over */
  ierr = PetscMemcpy(aij->a,aij->saved_values,nz*sizeof(PetscScalar));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatRetrieveValues"
/*@
    MatRetrieveValues - Retrieves the copy of the matrix values; this allows, for
       example, reuse of the linear part of a Jacobian, while recomputing the
       nonlinear portion.

   Collect on Mat

  Input Parameters:
.  mat - the matrix (currently on AIJ matrices support this option)

  Level: advanced

.seealso: MatStoreValues()

@*/
PetscErrorCode  MatRetrieveValues(Mat mat)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(mat,MAT_CLASSID,1);
  if (!mat->assembled) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONGSTATE,"Not for unassembled matrix");
  if (mat->factortype) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONGSTATE,"Not for factored matrix");
  ierr = PetscUseMethod(mat,"MatRetrieveValues_C",(Mat),(mat));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}


/* --------------------------------------------------------------------------------*/
#undef __FUNCT__
#define __FUNCT__ "MatCreateSeqAIJ"
/*@C
   MatCreateSeqAIJ - Creates a sparse matrix in AIJ (compressed row) format
   (the default parallel PETSc format).  For good matrix assembly performance
   the user should preallocate the matrix storage by setting the parameter nz
   (or the array nnz).  By setting these parameters accurately, performance
   during matrix assembly can be increased by more than a factor of 50.

   Collective on MPI_Comm

   Input Parameters:
+  comm - MPI communicator, set to PETSC_COMM_SELF
.  m - number of rows
.  n - number of columns
.  nz - number of nonzeros per row (same for all rows)
-  nnz - array containing the number of nonzeros in the various rows
         (possibly different for each row) or NULL

   Output Parameter:
.  A - the matrix

   It is recommended that one use the MatCreate(), MatSetType() and/or MatSetFromOptions(),
   MatXXXXSetPreallocation() paradgm instead of this routine directly.
   [MatXXXXSetPreallocation() is, for example, MatSeqAIJSetPreallocation]

   Notes:
   If nnz is given then nz is ignored

   The AIJ format (also called the Yale sparse matrix format or
   compressed row storage), is fully compatible with standard Fortran 77
   storage.  That is, the stored row and column indices can begin at
   either one (as in Fortran) or zero.  See the users' manual for details.

   Specify the preallocated storage with either nz or nnz (not both).
   Set nz=PETSC_DEFAULT and nnz=NULL for PETSc to control dynamic memory
   allocation.  For large problems you MUST preallocate memory or you
   will get TERRIBLE performance, see the users' manual chapter on matrices.

   By default, this format uses inodes (identical nodes) when possible, to
   improve numerical efficiency of matrix-vector products and solves. We
   search for consecutive rows with the same nonzero structure, thereby
   reusing matrix information to achieve increased efficiency.

   Options Database Keys:
+  -mat_no_inode  - Do not use inodes
-  -mat_inode_limit <limit> - Sets inode limit (max limit=5)

   Level: intermediate

.seealso: MatCreate(), MatCreateAIJ(), MatSetValues(), MatSeqAIJSetColumnIndices(), MatCreateSeqAIJWithArrays()

@*/
PetscErrorCode  MatCreateSeqAIJ(MPI_Comm comm,PetscInt m,PetscInt n,PetscInt nz,const PetscInt nnz[],Mat *A)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = MatCreate(comm,A);CHKERRQ(ierr);
  ierr = MatSetSizes(*A,m,n,m,n);CHKERRQ(ierr);
  ierr = MatSetType(*A,MATSEQAIJ);CHKERRQ(ierr);
  ierr = MatSeqAIJSetPreallocation_SeqAIJ(*A,nz,nnz);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatSeqAIJSetPreallocation"
/*@C
   MatSeqAIJSetPreallocation - For good matrix assembly performance
   the user should preallocate the matrix storage by setting the parameter nz
   (or the array nnz).  By setting these parameters accurately, performance
   during matrix assembly can be increased by more than a factor of 50.

   Collective on MPI_Comm

   Input Parameters:
+  B - The matrix
.  nz - number of nonzeros per row (same for all rows)
-  nnz - array containing the number of nonzeros in the various rows
         (possibly different for each row) or NULL

   Notes:
     If nnz is given then nz is ignored

    The AIJ format (also called the Yale sparse matrix format or
   compressed row storage), is fully compatible with standard Fortran 77
   storage.  That is, the stored row and column indices can begin at
   either one (as in Fortran) or zero.  See the users' manual for details.

   Specify the preallocated storage with either nz or nnz (not both).
   Set nz=PETSC_DEFAULT and nnz=NULL for PETSc to control dynamic memory
   allocation.  For large problems you MUST preallocate memory or you
   will get TERRIBLE performance, see the users' manual chapter on matrices.

   You can call MatGetInfo() to get information on how effective the preallocation was;
   for example the fields mallocs,nz_allocated,nz_used,nz_unneeded;
   You can also run with the option -info and look for messages with the string
   malloc in them to see if additional memory allocation was needed.

   Developers: Use nz of MAT_SKIP_ALLOCATION to not allocate any space for the matrix
   entries or columns indices

   By default, this format uses inodes (identical nodes) when possible, to
   improve numerical efficiency of matrix-vector products and solves. We
   search for consecutive rows with the same nonzero structure, thereby
   reusing matrix information to achieve increased efficiency.

   Options Database Keys:
+  -mat_no_inode  - Do not use inodes
.  -mat_inode_limit <limit> - Sets inode limit (max limit=5)
-  -mat_aij_oneindex - Internally use indexing starting at 1
        rather than 0.  Note that when calling MatSetValues(),
        the user still MUST index entries starting at 0!

   Level: intermediate

.seealso: MatCreate(), MatCreateAIJ(), MatSetValues(), MatSeqAIJSetColumnIndices(), MatCreateSeqAIJWithArrays(), MatGetInfo()

@*/
PetscErrorCode  MatSeqAIJSetPreallocation(Mat B,PetscInt nz,const PetscInt nnz[])
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(B,MAT_CLASSID,1);
  PetscValidType(B,1);
  ierr = PetscTryMethod(B,"MatSeqAIJSetPreallocation_C",(Mat,PetscInt,const PetscInt[]),(B,nz,nnz));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatSeqAIJSetPreallocation_SeqAIJ"
PetscErrorCode  MatSeqAIJSetPreallocation_SeqAIJ(Mat B,PetscInt nz,const PetscInt *nnz)
{
  Mat_SeqAIJ     *b;
  PetscBool      skipallocation = PETSC_FALSE,realalloc = PETSC_FALSE;
  PetscErrorCode ierr;
  PetscInt       i;

  PetscFunctionBegin;
  if (nz >= 0 || nnz) realalloc = PETSC_TRUE;
  if (nz == MAT_SKIP_ALLOCATION) {
    skipallocation = PETSC_TRUE;
    nz             = 0;
  }

  ierr = PetscLayoutSetUp(B->rmap);CHKERRQ(ierr);
  ierr = PetscLayoutSetUp(B->cmap);CHKERRQ(ierr);

  if (nz == PETSC_DEFAULT || nz == PETSC_DECIDE) nz = 5;
  if (nz < 0) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"nz cannot be less than 0: value %D",nz);
  if (nnz) {
    for (i=0; i<B->rmap->n; i++) {
      if (nnz[i] < 0) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"nnz cannot be less than 0: local row %D value %D",i,nnz[i]);
      if (nnz[i] > B->cmap->n) SETERRQ3(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"nnz cannot be greater than row length: local row %D value %d rowlength %D",i,nnz[i],B->cmap->n);
    }
  }

  B->preallocated = PETSC_TRUE;

  b = (Mat_SeqAIJ*)B->data;

  if (!skipallocation) {
    if (!b->imax) {
      ierr = PetscMalloc2(B->rmap->n,&b->imax,B->rmap->n,&b->ilen);CHKERRQ(ierr);
      ierr = PetscLogObjectMemory((PetscObject)B,2*B->rmap->n*sizeof(PetscInt));CHKERRQ(ierr);
    }
    if (!nnz) {
      if (nz == PETSC_DEFAULT || nz == PETSC_DECIDE) nz = 10;
      else if (nz < 0) nz = 1;
      for (i=0; i<B->rmap->n; i++) b->imax[i] = nz;
      nz = nz*B->rmap->n;
    } else {
      nz = 0;
      for (i=0; i<B->rmap->n; i++) {b->imax[i] = nnz[i]; nz += nnz[i];}
    }
    /* b->ilen will count nonzeros in each row so far. */
    for (i=0; i<B->rmap->n; i++) b->ilen[i] = 0;

    /* allocate the matrix space */
    /* FIXME: should B's old memory be unlogged? */
    ierr    = MatSeqXAIJFreeAIJ(B,&b->a,&b->j,&b->i);CHKERRQ(ierr);
    ierr    = PetscMalloc3(nz,&b->a,nz,&b->j,B->rmap->n+1,&b->i);CHKERRQ(ierr);
    ierr    = PetscLogObjectMemory((PetscObject)B,(B->rmap->n+1)*sizeof(PetscInt)+nz*(sizeof(PetscScalar)+sizeof(PetscInt)));CHKERRQ(ierr);
    b->i[0] = 0;
    for (i=1; i<B->rmap->n+1; i++) {
      b->i[i] = b->i[i-1] + b->imax[i-1];
    }
    b->singlemalloc = PETSC_TRUE;
    b->free_a       = PETSC_TRUE;
    b->free_ij      = PETSC_TRUE;
  } else {
    b->free_a  = PETSC_FALSE;
    b->free_ij = PETSC_FALSE;
  }

  b->nz               = 0;
  b->maxnz            = nz;
  B->info.nz_unneeded = (double)b->maxnz;
  if (realalloc) {
    ierr = MatSetOption(B,MAT_NEW_NONZERO_ALLOCATION_ERR,PETSC_TRUE);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef  __FUNCT__
#define __FUNCT__  "MatSeqAIJSetPreallocationCSR"
/*@
   MatSeqAIJSetPreallocationCSR - Allocates memory for a sparse sequential matrix in AIJ format.

   Input Parameters:
+  B - the matrix
.  i - the indices into j for the start of each row (starts with zero)
.  j - the column indices for each row (starts with zero) these must be sorted for each row
-  v - optional values in the matrix

   Level: developer

   The i,j,v values are COPIED with this routine; to avoid the copy use MatCreateSeqAIJWithArrays()

.keywords: matrix, aij, compressed row, sparse, sequential

.seealso: MatCreate(), MatCreateSeqAIJ(), MatSetValues(), MatSeqAIJSetPreallocation(), MatCreateSeqAIJ(), SeqAIJ
@*/
PetscErrorCode MatSeqAIJSetPreallocationCSR(Mat B,const PetscInt i[],const PetscInt j[],const PetscScalar v[])
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(B,MAT_CLASSID,1);
  PetscValidType(B,1);
  ierr = PetscTryMethod(B,"MatSeqAIJSetPreallocationCSR_C",(Mat,const PetscInt[],const PetscInt[],const PetscScalar[]),(B,i,j,v));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef  __FUNCT__
#define __FUNCT__  "MatSeqAIJSetPreallocationCSR_SeqAIJ"
PetscErrorCode  MatSeqAIJSetPreallocationCSR_SeqAIJ(Mat B,const PetscInt Ii[],const PetscInt J[],const PetscScalar v[])
{
  PetscInt       i;
  PetscInt       m,n;
  PetscInt       nz;
  PetscInt       *nnz, nz_max = 0;
  PetscScalar    *values;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (Ii[0]) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE, "Ii[0] must be 0 it is %D", Ii[0]);

  ierr = PetscLayoutSetUp(B->rmap);CHKERRQ(ierr);
  ierr = PetscLayoutSetUp(B->cmap);CHKERRQ(ierr);

  ierr = MatGetSize(B, &m, &n);CHKERRQ(ierr);
  ierr = PetscMalloc1(m+1, &nnz);CHKERRQ(ierr);
  for (i = 0; i < m; i++) {
    nz     = Ii[i+1]- Ii[i];
    nz_max = PetscMax(nz_max, nz);
    if (nz < 0) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE, "Local row %D has a negative number of columns %D", i, nnz);
    nnz[i] = nz;
  }
  ierr = MatSeqAIJSetPreallocation(B, 0, nnz);CHKERRQ(ierr);
  ierr = PetscFree(nnz);CHKERRQ(ierr);

  if (v) {
    values = (PetscScalar*) v;
  } else {
    ierr = PetscCalloc1(nz_max, &values);CHKERRQ(ierr);
  }

  for (i = 0; i < m; i++) {
    nz   = Ii[i+1] - Ii[i];
    ierr = MatSetValues_SeqAIJ(B, 1, &i, nz, J+Ii[i], values + (v ? Ii[i] : 0), INSERT_VALUES);CHKERRQ(ierr);
  }

  ierr = MatAssemblyBegin(B,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(B,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);

  if (!v) {
    ierr = PetscFree(values);CHKERRQ(ierr);
  }
  ierr = MatSetOption(B,MAT_NEW_NONZERO_LOCATION_ERR,PETSC_TRUE);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#include <../src/mat/impls/dense/seq/dense.h>
#include <petsc/private/kernels/petscaxpy.h>

#undef __FUNCT__
#define __FUNCT__ "MatMatMultNumeric_SeqDense_SeqAIJ"
/*
    Computes (B'*A')' since computing B*A directly is untenable

               n                       p                          p
        (              )       (              )         (                  )
      m (      A       )  *  n (       B      )   =   m (         C        )
        (              )       (              )         (                  )

*/
PetscErrorCode MatMatMultNumeric_SeqDense_SeqAIJ(Mat A,Mat B,Mat C)
{
  PetscErrorCode    ierr;
  Mat_SeqDense      *sub_a = (Mat_SeqDense*)A->data;
  Mat_SeqAIJ        *sub_b = (Mat_SeqAIJ*)B->data;
  Mat_SeqDense      *sub_c = (Mat_SeqDense*)C->data;
  PetscInt          i,n,m,q,p;
  const PetscInt    *ii,*idx;
  const PetscScalar *b,*a,*a_q;
  PetscScalar       *c,*c_q;

  PetscFunctionBegin;
  m    = A->rmap->n;
  n    = A->cmap->n;
  p    = B->cmap->n;
  a    = sub_a->v;
  b    = sub_b->a;
  c    = sub_c->v;
  ierr = PetscMemzero(c,m*p*sizeof(PetscScalar));CHKERRQ(ierr);

  ii  = sub_b->i;
  idx = sub_b->j;
  for (i=0; i<n; i++) {
    q = ii[i+1] - ii[i];
    while (q-->0) {
      c_q = c + m*(*idx);
      a_q = a + m*i;
      PetscKernelAXPY(c_q,*b,a_q,m);
      idx++;
      b++;
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatMatMultSymbolic_SeqDense_SeqAIJ"
PetscErrorCode MatMatMultSymbolic_SeqDense_SeqAIJ(Mat A,Mat B,PetscReal fill,Mat *C)
{
  PetscErrorCode ierr;
  PetscInt       m=A->rmap->n,n=B->cmap->n;
  Mat            Cmat;

  PetscFunctionBegin;
  if (A->cmap->n != B->rmap->n) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_SIZ,"A->cmap->n %D != B->rmap->n %D\n",A->cmap->n,B->rmap->n);
  ierr = MatCreate(PetscObjectComm((PetscObject)A),&Cmat);CHKERRQ(ierr);
  ierr = MatSetSizes(Cmat,m,n,m,n);CHKERRQ(ierr);
  ierr = MatSetBlockSizesFromMats(Cmat,A,B);CHKERRQ(ierr);
  ierr = MatSetType(Cmat,MATSEQDENSE);CHKERRQ(ierr);
  ierr = MatSeqDenseSetPreallocation(Cmat,NULL);CHKERRQ(ierr);

  Cmat->ops->matmultnumeric = MatMatMultNumeric_SeqDense_SeqAIJ;

  *C = Cmat;
  PetscFunctionReturn(0);
}

/* ----------------------------------------------------------------*/
#undef __FUNCT__
#define __FUNCT__ "MatMatMult_SeqDense_SeqAIJ"
PetscErrorCode MatMatMult_SeqDense_SeqAIJ(Mat A,Mat B,MatReuse scall,PetscReal fill,Mat *C)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (scall == MAT_INITIAL_MATRIX) {
    ierr = PetscLogEventBegin(MAT_MatMultSymbolic,A,B,0,0);CHKERRQ(ierr);
    ierr = MatMatMultSymbolic_SeqDense_SeqAIJ(A,B,fill,C);CHKERRQ(ierr);
    ierr = PetscLogEventEnd(MAT_MatMultSymbolic,A,B,0,0);CHKERRQ(ierr);
  }
  ierr = PetscLogEventBegin(MAT_MatMultNumeric,A,B,0,0);CHKERRQ(ierr);
  ierr = MatMatMultNumeric_SeqDense_SeqAIJ(A,B,*C);CHKERRQ(ierr);
  ierr = PetscLogEventEnd(MAT_MatMultNumeric,A,B,0,0);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}


/*MC
   MATSEQAIJ - MATSEQAIJ = "seqaij" - A matrix type to be used for sequential sparse matrices,
   based on compressed sparse row format.

   Options Database Keys:
. -mat_type seqaij - sets the matrix type to "seqaij" during a call to MatSetFromOptions()

  Level: beginner

.seealso: MatCreateSeqAIJ(), MatSetFromOptions(), MatSetType(), MatCreate(), MatType
M*/

/*MC
   MATAIJ - MATAIJ = "aij" - A matrix type to be used for sparse matrices.

   This matrix type is identical to MATSEQAIJ when constructed with a single process communicator,
   and MATMPIAIJ otherwise.  As a result, for single process communicators,
  MatSeqAIJSetPreallocation is supported, and similarly MatMPIAIJSetPreallocation is supported
  for communicators controlling multiple processes.  It is recommended that you call both of
  the above preallocation routines for simplicity.

   Options Database Keys:
. -mat_type aij - sets the matrix type to "aij" during a call to MatSetFromOptions()

  Developer Notes: Subclasses include MATAIJCUSP, MATAIJPERM, MATAIJCRL, and also automatically switches over to use inodes when
   enough exist.

  Level: beginner

.seealso: MatCreateAIJ(), MatCreateSeqAIJ(), MATSEQAIJ,MATMPIAIJ
M*/

/*MC
   MATAIJCRL - MATAIJCRL = "aijcrl" - A matrix type to be used for sparse matrices.

   This matrix type is identical to MATSEQAIJCRL when constructed with a single process communicator,
   and MATMPIAIJCRL otherwise.  As a result, for single process communicators,
   MatSeqAIJSetPreallocation() is supported, and similarly MatMPIAIJSetPreallocation() is supported
  for communicators controlling multiple processes.  It is recommended that you call both of
  the above preallocation routines for simplicity.

   Options Database Keys:
. -mat_type aijcrl - sets the matrix type to "aijcrl" during a call to MatSetFromOptions()

  Level: beginner

.seealso: MatCreateMPIAIJCRL,MATSEQAIJCRL,MATMPIAIJCRL, MATSEQAIJCRL, MATMPIAIJCRL
M*/

PETSC_EXTERN PetscErrorCode MatConvert_SeqAIJ_SeqAIJCRL(Mat,MatType,MatReuse,Mat*);
#if defined(PETSC_HAVE_ELEMENTAL)
PETSC_EXTERN PetscErrorCode MatConvert_SeqAIJ_Elemental(Mat,MatType,MatReuse,Mat*);
#endif
PETSC_EXTERN PetscErrorCode MatConvert_SeqAIJ_SeqDense(Mat,MatType,MatReuse,Mat*);

#if defined(PETSC_HAVE_MATLAB_ENGINE)
PETSC_EXTERN PetscErrorCode  MatlabEnginePut_SeqAIJ(PetscObject,void*);
PETSC_EXTERN PetscErrorCode  MatlabEngineGet_SeqAIJ(PetscObject,void*);
#endif


#undef __FUNCT__
#define __FUNCT__ "MatSeqAIJGetArray"
/*@C
   MatSeqAIJGetArray - gives access to the array where the data for a SeqSeqAIJ matrix is stored

   Not Collective

   Input Parameter:
.  mat - a MATSEQAIJ matrix

   Output Parameter:
.   array - pointer to the data

   Level: intermediate

.seealso: MatSeqAIJRestoreArray(), MatSeqAIJGetArrayF90()
@*/
PetscErrorCode  MatSeqAIJGetArray(Mat A,PetscScalar **array)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscUseMethod(A,"MatSeqAIJGetArray_C",(Mat,PetscScalar**),(A,array));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatSeqAIJGetMaxRowNonzeros"
/*@C
   MatSeqAIJGetMaxRowNonzeros - returns the maximum number of nonzeros in any row

   Not Collective

   Input Parameter:
.  mat - a MATSEQAIJ matrix

   Output Parameter:
.   nz - the maximum number of nonzeros in any row

   Level: intermediate

.seealso: MatSeqAIJRestoreArray(), MatSeqAIJGetArrayF90()
@*/
PetscErrorCode  MatSeqAIJGetMaxRowNonzeros(Mat A,PetscInt *nz)
{
  Mat_SeqAIJ     *aij = (Mat_SeqAIJ*)A->data;

  PetscFunctionBegin;
  *nz = aij->rmax;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatSeqAIJRestoreArray"
/*@C
   MatSeqAIJRestoreArray - returns access to the array where the data for a MATSEQAIJ matrix is stored obtained by MatSeqAIJGetArray()

   Not Collective

   Input Parameters:
.  mat - a MATSEQAIJ matrix
.  array - pointer to the data

   Level: intermediate

.seealso: MatSeqAIJGetArray(), MatSeqAIJRestoreArrayF90()
@*/
PetscErrorCode  MatSeqAIJRestoreArray(Mat A,PetscScalar **array)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscUseMethod(A,"MatSeqAIJRestoreArray_C",(Mat,PetscScalar**),(A,array));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatCreate_SeqAIJ"
PETSC_EXTERN PetscErrorCode MatCreate_SeqAIJ(Mat B)
{
  Mat_SeqAIJ     *b;
  PetscErrorCode ierr;
  PetscMPIInt    size;

  PetscFunctionBegin;
  ierr = MPI_Comm_size(PetscObjectComm((PetscObject)B),&size);CHKERRQ(ierr);
  if (size > 1) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Comm must be of size 1");

  ierr = PetscNewLog(B,&b);CHKERRQ(ierr);

  B->data = (void*)b;

  ierr = PetscMemcpy(B->ops,&MatOps_Values,sizeof(struct _MatOps));CHKERRQ(ierr);

  b->row                = 0;
  b->col                = 0;
  b->icol               = 0;
  b->reallocs           = 0;
  b->ignorezeroentries  = PETSC_FALSE;
  b->roworiented        = PETSC_TRUE;
  b->nonew              = 0;
  b->diag               = 0;
  b->solve_work         = 0;
  B->spptr              = 0;
  b->saved_values       = 0;
  b->idiag              = 0;
  b->mdiag              = 0;
  b->ssor_work          = 0;
  b->omega              = 1.0;
  b->fshift             = 0.0;
  b->idiagvalid         = PETSC_FALSE;
  b->ibdiagvalid        = PETSC_FALSE;
  b->keepnonzeropattern = PETSC_FALSE;

  ierr = PetscObjectChangeTypeName((PetscObject)B,MATSEQAIJ);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)B,"MatSeqAIJGetArray_C",MatSeqAIJGetArray_SeqAIJ);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)B,"MatSeqAIJRestoreArray_C",MatSeqAIJRestoreArray_SeqAIJ);CHKERRQ(ierr);

#if defined(PETSC_HAVE_MATLAB_ENGINE)
  ierr = PetscObjectComposeFunction((PetscObject)B,"PetscMatlabEnginePut_C",MatlabEnginePut_SeqAIJ);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)B,"PetscMatlabEngineGet_C",MatlabEngineGet_SeqAIJ);CHKERRQ(ierr);
#endif

  ierr = PetscObjectComposeFunction((PetscObject)B,"MatSeqAIJSetColumnIndices_C",MatSeqAIJSetColumnIndices_SeqAIJ);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)B,"MatStoreValues_C",MatStoreValues_SeqAIJ);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)B,"MatRetrieveValues_C",MatRetrieveValues_SeqAIJ);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)B,"MatConvert_seqaij_seqsbaij_C",MatConvert_SeqAIJ_SeqSBAIJ);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)B,"MatConvert_seqaij_seqbaij_C",MatConvert_SeqAIJ_SeqBAIJ);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)B,"MatConvert_seqaij_seqaijperm_C",MatConvert_SeqAIJ_SeqAIJPERM);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)B,"MatConvert_seqaij_seqaijcrl_C",MatConvert_SeqAIJ_SeqAIJCRL);CHKERRQ(ierr);
#if defined(PETSC_HAVE_ELEMENTAL)
  ierr = PetscObjectComposeFunction((PetscObject)B,"MatConvert_seqaij_elemental_C",MatConvert_SeqAIJ_Elemental);CHKERRQ(ierr);
#endif
  ierr = PetscObjectComposeFunction((PetscObject)B,"MatConvert_seqaij_seqdense_C",MatConvert_SeqAIJ_SeqDense);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)B,"MatIsTranspose_C",MatIsTranspose_SeqAIJ);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)B,"MatIsHermitianTranspose_C",MatIsTranspose_SeqAIJ);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)B,"MatSeqAIJSetPreallocation_C",MatSeqAIJSetPreallocation_SeqAIJ);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)B,"MatSeqAIJSetPreallocationCSR_C",MatSeqAIJSetPreallocationCSR_SeqAIJ);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)B,"MatReorderForNonzeroDiagonal_C",MatReorderForNonzeroDiagonal_SeqAIJ);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)B,"MatMatMult_seqdense_seqaij_C",MatMatMult_SeqDense_SeqAIJ);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)B,"MatMatMultSymbolic_seqdense_seqaij_C",MatMatMultSymbolic_SeqDense_SeqAIJ);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)B,"MatMatMultNumeric_seqdense_seqaij_C",MatMatMultNumeric_SeqDense_SeqAIJ);CHKERRQ(ierr);
  ierr = MatCreate_SeqAIJ_Inode(B);CHKERRQ(ierr);
  ierr = PetscObjectChangeTypeName((PetscObject)B,MATSEQAIJ);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatDuplicateNoCreate_SeqAIJ"
/*
    Given a matrix generated with MatGetFactor() duplicates all the information in A into B
*/
PetscErrorCode MatDuplicateNoCreate_SeqAIJ(Mat C,Mat A,MatDuplicateOption cpvalues,PetscBool mallocmatspace)
{
  Mat_SeqAIJ     *c,*a = (Mat_SeqAIJ*)A->data;
  PetscErrorCode ierr;
  PetscInt       i,m = A->rmap->n;

  PetscFunctionBegin;
  c = (Mat_SeqAIJ*)C->data;

  C->factortype = A->factortype;
  c->row        = 0;
  c->col        = 0;
  c->icol       = 0;
  c->reallocs   = 0;

  C->assembled = PETSC_TRUE;

  ierr = PetscLayoutReference(A->rmap,&C->rmap);CHKERRQ(ierr);
  ierr = PetscLayoutReference(A->cmap,&C->cmap);CHKERRQ(ierr);

  ierr = PetscMalloc2(m,&c->imax,m,&c->ilen);CHKERRQ(ierr);
  ierr = PetscLogObjectMemory((PetscObject)C, 2*m*sizeof(PetscInt));CHKERRQ(ierr);
  for (i=0; i<m; i++) {
    c->imax[i] = a->imax[i];
    c->ilen[i] = a->ilen[i];
  }

  /* allocate the matrix space */
  if (mallocmatspace) {
    ierr = PetscMalloc3(a->i[m],&c->a,a->i[m],&c->j,m+1,&c->i);CHKERRQ(ierr);
    ierr = PetscLogObjectMemory((PetscObject)C, a->i[m]*(sizeof(PetscScalar)+sizeof(PetscInt))+(m+1)*sizeof(PetscInt));CHKERRQ(ierr);

    c->singlemalloc = PETSC_TRUE;

    ierr = PetscMemcpy(c->i,a->i,(m+1)*sizeof(PetscInt));CHKERRQ(ierr);
    if (m > 0) {
      ierr = PetscMemcpy(c->j,a->j,(a->i[m])*sizeof(PetscInt));CHKERRQ(ierr);
      if (cpvalues == MAT_COPY_VALUES) {
        ierr = PetscMemcpy(c->a,a->a,(a->i[m])*sizeof(PetscScalar));CHKERRQ(ierr);
      } else {
        ierr = PetscMemzero(c->a,(a->i[m])*sizeof(PetscScalar));CHKERRQ(ierr);
      }
    }
  }

  c->ignorezeroentries = a->ignorezeroentries;
  c->roworiented       = a->roworiented;
  c->nonew             = a->nonew;
  if (a->diag) {
    ierr = PetscMalloc1(m+1,&c->diag);CHKERRQ(ierr);
    ierr = PetscLogObjectMemory((PetscObject)C,(m+1)*sizeof(PetscInt));CHKERRQ(ierr);
    for (i=0; i<m; i++) {
      c->diag[i] = a->diag[i];
    }
  } else c->diag = 0;

  c->solve_work         = 0;
  c->saved_values       = 0;
  c->idiag              = 0;
  c->ssor_work          = 0;
  c->keepnonzeropattern = a->keepnonzeropattern;
  c->free_a             = PETSC_TRUE;
  c->free_ij            = PETSC_TRUE;

  c->rmax         = a->rmax;
  c->nz           = a->nz;
  c->maxnz        = a->nz;       /* Since we allocate exactly the right amount */
  C->preallocated = PETSC_TRUE;

  c->compressedrow.use   = a->compressedrow.use;
  c->compressedrow.nrows = a->compressedrow.nrows;
  if (a->compressedrow.use) {
    i    = a->compressedrow.nrows;
    ierr = PetscMalloc2(i+1,&c->compressedrow.i,i,&c->compressedrow.rindex);CHKERRQ(ierr);
    ierr = PetscMemcpy(c->compressedrow.i,a->compressedrow.i,(i+1)*sizeof(PetscInt));CHKERRQ(ierr);
    ierr = PetscMemcpy(c->compressedrow.rindex,a->compressedrow.rindex,i*sizeof(PetscInt));CHKERRQ(ierr);
  } else {
    c->compressedrow.use    = PETSC_FALSE;
    c->compressedrow.i      = NULL;
    c->compressedrow.rindex = NULL;
  }
  c->nonzerorowcnt = a->nonzerorowcnt;
  C->nonzerostate  = A->nonzerostate;

  ierr = MatDuplicate_SeqAIJ_Inode(A,cpvalues,&C);CHKERRQ(ierr);
  ierr = PetscFunctionListDuplicate(((PetscObject)A)->qlist,&((PetscObject)C)->qlist);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatDuplicate_SeqAIJ"
PetscErrorCode MatDuplicate_SeqAIJ(Mat A,MatDuplicateOption cpvalues,Mat *B)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = MatCreate(PetscObjectComm((PetscObject)A),B);CHKERRQ(ierr);
  ierr = MatSetSizes(*B,A->rmap->n,A->cmap->n,A->rmap->n,A->cmap->n);CHKERRQ(ierr);
  if (!(A->rmap->n % A->rmap->bs) && !(A->cmap->n % A->cmap->bs)) {
    ierr = MatSetBlockSizesFromMats(*B,A,A);CHKERRQ(ierr);
  }
  ierr = MatSetType(*B,((PetscObject)A)->type_name);CHKERRQ(ierr);
  ierr = MatDuplicateNoCreate_SeqAIJ(*B,A,cpvalues,PETSC_TRUE);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatLoad_SeqAIJ"
PetscErrorCode MatLoad_SeqAIJ(Mat newMat, PetscViewer viewer)
{
  Mat_SeqAIJ     *a;
  PetscErrorCode ierr;
  PetscInt       i,sum,nz,header[4],*rowlengths = 0,M,N,rows,cols;
  int            fd;
  PetscMPIInt    size;
  MPI_Comm       comm;
  PetscInt       bs = newMat->rmap->bs;

  PetscFunctionBegin;
  /* force binary viewer to load .info file if it has not yet done so */
  ierr = PetscViewerSetUp(viewer);CHKERRQ(ierr);
  ierr = PetscObjectGetComm((PetscObject)viewer,&comm);CHKERRQ(ierr);
  ierr = MPI_Comm_size(comm,&size);CHKERRQ(ierr);
  if (size > 1) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_SIZ,"view must have one processor");

  ierr = PetscOptionsBegin(comm,NULL,"Options for loading SEQAIJ matrix","Mat");CHKERRQ(ierr);
  ierr = PetscOptionsInt("-matload_block_size","Set the blocksize used to store the matrix","MatLoad",bs,&bs,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsEnd();CHKERRQ(ierr);
  if (bs < 0) bs = 1;
  ierr = MatSetBlockSize(newMat,bs);CHKERRQ(ierr);

  ierr = PetscViewerBinaryGetDescriptor(viewer,&fd);CHKERRQ(ierr);
  ierr = PetscBinaryRead(fd,header,4,PETSC_INT);CHKERRQ(ierr);
  if (header[0] != MAT_FILE_CLASSID) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_FILE_UNEXPECTED,"not matrix object in file");
  M = header[1]; N = header[2]; nz = header[3];

  if (nz < 0) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_FILE_UNEXPECTED,"Matrix stored in special format on disk,cannot load as SeqAIJ");

  /* read in row lengths */
  ierr = PetscMalloc1(M,&rowlengths);CHKERRQ(ierr);
  ierr = PetscBinaryRead(fd,rowlengths,M,PETSC_INT);CHKERRQ(ierr);

  /* check if sum of rowlengths is same as nz */
  for (i=0,sum=0; i< M; i++) sum +=rowlengths[i];
  if (sum != nz) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_FILE_READ,"Inconsistant matrix data in file. no-nonzeros = %dD, sum-row-lengths = %D\n",nz,sum);

  /* set global size if not set already*/
  if (newMat->rmap->n < 0 && newMat->rmap->N < 0 && newMat->cmap->n < 0 && newMat->cmap->N < 0) {
    ierr = MatSetSizes(newMat,PETSC_DECIDE,PETSC_DECIDE,M,N);CHKERRQ(ierr);
  } else {
    /* if sizes and type are already set, check if the matrix  global sizes are correct */
    ierr = MatGetSize(newMat,&rows,&cols);CHKERRQ(ierr);
    if (rows < 0 && cols < 0) { /* user might provide local size instead of global size */
      ierr = MatGetLocalSize(newMat,&rows,&cols);CHKERRQ(ierr);
    }
    if (M != rows ||  N != cols) SETERRQ4(PETSC_COMM_SELF,PETSC_ERR_FILE_UNEXPECTED, "Matrix in file of different length (%D, %D) than the input matrix (%D, %D)",M,N,rows,cols);
  }
  ierr = MatSeqAIJSetPreallocation_SeqAIJ(newMat,0,rowlengths);CHKERRQ(ierr);
  a    = (Mat_SeqAIJ*)newMat->data;

  ierr = PetscBinaryRead(fd,a->j,nz,PETSC_INT);CHKERRQ(ierr);

  /* read in nonzero values */
  ierr = PetscBinaryRead(fd,a->a,nz,PETSC_SCALAR);CHKERRQ(ierr);

  /* set matrix "i" values */
  a->i[0] = 0;
  for (i=1; i<= M; i++) {
    a->i[i]      = a->i[i-1] + rowlengths[i-1];
    a->ilen[i-1] = rowlengths[i-1];
  }
  ierr = PetscFree(rowlengths);CHKERRQ(ierr);

  ierr = MatAssemblyBegin(newMat,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(newMat,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatEqual_SeqAIJ"
PetscErrorCode MatEqual_SeqAIJ(Mat A,Mat B,PetscBool * flg)
{
  Mat_SeqAIJ     *a = (Mat_SeqAIJ*)A->data,*b = (Mat_SeqAIJ*)B->data;
  PetscErrorCode ierr;
#if defined(PETSC_USE_COMPLEX)
  PetscInt k;
#endif

  PetscFunctionBegin;
  /* If the  matrix dimensions are not equal,or no of nonzeros */
  if ((A->rmap->n != B->rmap->n) || (A->cmap->n != B->cmap->n) ||(a->nz != b->nz)) {
    *flg = PETSC_FALSE;
    PetscFunctionReturn(0);
  }

  /* if the a->i are the same */
  ierr = PetscMemcmp(a->i,b->i,(A->rmap->n+1)*sizeof(PetscInt),flg);CHKERRQ(ierr);
  if (!*flg) PetscFunctionReturn(0);

  /* if a->j are the same */
  ierr = PetscMemcmp(a->j,b->j,(a->nz)*sizeof(PetscInt),flg);CHKERRQ(ierr);
  if (!*flg) PetscFunctionReturn(0);

  /* if a->a are the same */
#if defined(PETSC_USE_COMPLEX)
  for (k=0; k<a->nz; k++) {
    if (PetscRealPart(a->a[k]) != PetscRealPart(b->a[k]) || PetscImaginaryPart(a->a[k]) != PetscImaginaryPart(b->a[k])) {
      *flg = PETSC_FALSE;
      PetscFunctionReturn(0);
    }
  }
#else
  ierr = PetscMemcmp(a->a,b->a,(a->nz)*sizeof(PetscScalar),flg);CHKERRQ(ierr);
#endif
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatCreateSeqAIJWithArrays"
/*@
     MatCreateSeqAIJWithArrays - Creates an sequential AIJ matrix using matrix elements (in CSR format)
              provided by the user.

      Collective on MPI_Comm

   Input Parameters:
+   comm - must be an MPI communicator of size 1
.   m - number of rows
.   n - number of columns
.   i - row indices
.   j - column indices
-   a - matrix values

   Output Parameter:
.   mat - the matrix

   Level: intermediate

   Notes:
       The i, j, and a arrays are not copied by this routine, the user must free these arrays
    once the matrix is destroyed and not before

       You cannot set new nonzero locations into this matrix, that will generate an error.

       The i and j indices are 0 based

       The format which is used for the sparse matrix input, is equivalent to a
    row-major ordering.. i.e for the following matrix, the input data expected is
    as shown

$        1 0 0
$        2 0 3
$        4 5 6
$
$        i =  {0,1,3,6}  [size = nrow+1  = 3+1]
$        j =  {0,0,2,0,1,2}  [size = 6]; values must be sorted for each row
$        v =  {1,2,3,4,5,6}  [size = 6]


.seealso: MatCreate(), MatCreateAIJ(), MatCreateSeqAIJ(), MatCreateMPIAIJWithArrays(), MatMPIAIJSetPreallocationCSR()

@*/
PetscErrorCode  MatCreateSeqAIJWithArrays(MPI_Comm comm,PetscInt m,PetscInt n,PetscInt *i,PetscInt *j,PetscScalar *a,Mat *mat)
{
  PetscErrorCode ierr;
  PetscInt       ii;
  Mat_SeqAIJ     *aij;
#if defined(PETSC_USE_DEBUG)
  PetscInt jj;
#endif

  PetscFunctionBegin;
  if (i[0]) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"i (row indices) must start with 0");
  ierr = MatCreate(comm,mat);CHKERRQ(ierr);
  ierr = MatSetSizes(*mat,m,n,m,n);CHKERRQ(ierr);
  /* ierr = MatSetBlockSizes(*mat,,);CHKERRQ(ierr); */
  ierr = MatSetType(*mat,MATSEQAIJ);CHKERRQ(ierr);
  ierr = MatSeqAIJSetPreallocation_SeqAIJ(*mat,MAT_SKIP_ALLOCATION,0);CHKERRQ(ierr);
  aij  = (Mat_SeqAIJ*)(*mat)->data;
  ierr = PetscMalloc2(m,&aij->imax,m,&aij->ilen);CHKERRQ(ierr);

  aij->i            = i;
  aij->j            = j;
  aij->a            = a;
  aij->singlemalloc = PETSC_FALSE;
  aij->nonew        = -1;             /*this indicates that inserting a new value in the matrix that generates a new nonzero is an error*/
  aij->free_a       = PETSC_FALSE;
  aij->free_ij      = PETSC_FALSE;

  for (ii=0; ii<m; ii++) {
    aij->ilen[ii] = aij->imax[ii] = i[ii+1] - i[ii];
#if defined(PETSC_USE_DEBUG)
    if (i[ii+1] - i[ii] < 0) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Negative row length in i (row indices) row = %D length = %D",ii,i[ii+1] - i[ii]);
    for (jj=i[ii]+1; jj<i[ii+1]; jj++) {
      if (j[jj] < j[jj-1]) SETERRQ3(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Column entry number %D (actual colum %D) in row %D is not sorted",jj-i[ii],j[jj],ii);
      if (j[jj] == j[jj]-1) SETERRQ3(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Column entry number %D (actual colum %D) in row %D is identical to previous entry",jj-i[ii],j[jj],ii);
    }
#endif
  }
#if defined(PETSC_USE_DEBUG)
  for (ii=0; ii<aij->i[m]; ii++) {
    if (j[ii] < 0) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Negative column index at location = %D index = %D",ii,j[ii]);
    if (j[ii] > n - 1) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Column index to large at location = %D index = %D",ii,j[ii]);
  }
#endif

  ierr = MatAssemblyBegin(*mat,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(*mat,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}
#undef __FUNCT__
#define __FUNCT__ "MatCreateSeqAIJFromTriple"
/*@C
     MatCreateSeqAIJFromTriple - Creates an sequential AIJ matrix using matrix elements (in COO format)
              provided by the user.

      Collective on MPI_Comm

   Input Parameters:
+   comm - must be an MPI communicator of size 1
.   m   - number of rows
.   n   - number of columns
.   i   - row indices
.   j   - column indices
.   a   - matrix values
.   nz  - number of nonzeros
-   idx - 0 or 1 based

   Output Parameter:
.   mat - the matrix

   Level: intermediate

   Notes:
       The i and j indices are 0 based

       The format which is used for the sparse matrix input, is equivalent to a
    row-major ordering.. i.e for the following matrix, the input data expected is
    as shown:

        1 0 0
        2 0 3
        4 5 6

        i =  {0,1,1,2,2,2}
        j =  {0,0,2,0,1,2}
        v =  {1,2,3,4,5,6}


.seealso: MatCreate(), MatCreateAIJ(), MatCreateSeqAIJ(), MatCreateSeqAIJWithArrays(), MatMPIAIJSetPreallocationCSR()

@*/
PetscErrorCode  MatCreateSeqAIJFromTriple(MPI_Comm comm,PetscInt m,PetscInt n,PetscInt *i,PetscInt *j,PetscScalar *a,Mat *mat,PetscInt nz,PetscBool idx)
{
  PetscErrorCode ierr;
  PetscInt       ii, *nnz, one = 1,row,col;


  PetscFunctionBegin;
  ierr = PetscCalloc1(m,&nnz);CHKERRQ(ierr);
  for (ii = 0; ii < nz; ii++) {
    nnz[i[ii] - !!idx] += 1;
  }
  ierr = MatCreate(comm,mat);CHKERRQ(ierr);
  ierr = MatSetSizes(*mat,m,n,m,n);CHKERRQ(ierr);
  ierr = MatSetType(*mat,MATSEQAIJ);CHKERRQ(ierr);
  ierr = MatSeqAIJSetPreallocation_SeqAIJ(*mat,0,nnz);CHKERRQ(ierr);
  for (ii = 0; ii < nz; ii++) {
    if (idx) {
      row = i[ii] - 1;
      col = j[ii] - 1;
    } else {
      row = i[ii];
      col = j[ii];
    }
    ierr = MatSetValues(*mat,one,&row,one,&col,&a[ii],ADD_VALUES);CHKERRQ(ierr);
  }
  ierr = MatAssemblyBegin(*mat,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(*mat,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = PetscFree(nnz);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatSetColoring_SeqAIJ"
PetscErrorCode MatSetColoring_SeqAIJ(Mat A,ISColoring coloring)
{
  PetscErrorCode ierr;
  Mat_SeqAIJ     *a = (Mat_SeqAIJ*)A->data;

  PetscFunctionBegin;
  if (coloring->ctype == IS_COLORING_GLOBAL) {
    ierr        = ISColoringReference(coloring);CHKERRQ(ierr);
    a->coloring = coloring;
  } else if (coloring->ctype == IS_COLORING_GHOSTED) {
    PetscInt        i,*larray;
    ISColoring      ocoloring;
    ISColoringValue *colors;

    /* set coloring for diagonal portion */
    ierr = PetscMalloc1(A->cmap->n,&larray);CHKERRQ(ierr);
    for (i=0; i<A->cmap->n; i++) larray[i] = i;
    ierr = ISGlobalToLocalMappingApply(A->cmap->mapping,IS_GTOLM_MASK,A->cmap->n,larray,NULL,larray);CHKERRQ(ierr);
    ierr = PetscMalloc1(A->cmap->n,&colors);CHKERRQ(ierr);
    for (i=0; i<A->cmap->n; i++) colors[i] = coloring->colors[larray[i]];
    ierr        = PetscFree(larray);CHKERRQ(ierr);
    ierr        = ISColoringCreate(PETSC_COMM_SELF,coloring->n,A->cmap->n,colors,PETSC_OWN_POINTER,&ocoloring);CHKERRQ(ierr);
    a->coloring = ocoloring;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatSetValuesAdifor_SeqAIJ"
PetscErrorCode MatSetValuesAdifor_SeqAIJ(Mat A,PetscInt nl,void *advalues)
{
  Mat_SeqAIJ      *a      = (Mat_SeqAIJ*)A->data;
  PetscInt        m       = A->rmap->n,*ii = a->i,*jj = a->j,nz,i,j;
  MatScalar       *v      = a->a;
  PetscScalar     *values = (PetscScalar*)advalues;
  ISColoringValue *color;

  PetscFunctionBegin;
  if (!a->coloring) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONGSTATE,"Coloring not set for matrix");
  color = a->coloring->colors;
  /* loop over rows */
  for (i=0; i<m; i++) {
    nz = ii[i+1] - ii[i];
    /* loop over columns putting computed value into matrix */
    for (j=0; j<nz; j++) *v++ = values[color[*jj++]];
    values += nl; /* jump to next row of derivatives */
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatSeqAIJInvalidateDiagonal"
PetscErrorCode MatSeqAIJInvalidateDiagonal(Mat A)
{
  Mat_SeqAIJ     *a=(Mat_SeqAIJ*)A->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  a->idiagvalid  = PETSC_FALSE;
  a->ibdiagvalid = PETSC_FALSE;

  ierr = MatSeqAIJInvalidateDiagonal_Inode(A);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatCreateMPIMatConcatenateSeqMat_SeqAIJ"
PetscErrorCode MatCreateMPIMatConcatenateSeqMat_SeqAIJ(MPI_Comm comm,Mat inmat,PetscInt n,MatReuse scall,Mat *outmat)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = MatCreateMPIMatConcatenateSeqMat_MPIAIJ(comm,inmat,n,scall,outmat);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/*
 Permute A into C's *local* index space using rowemb,colemb.
 The embedding are supposed to be injections and the above implies that the range of rowemb is a subset
 of [0,m), colemb is in [0,n).
 If pattern == DIFFERENT_NONZERO_PATTERN, C is preallocated according to A.
 */
#undef __FUNCT__
#define __FUNCT__ "MatSetSeqMat_SeqAIJ"
PetscErrorCode MatSetSeqMat_SeqAIJ(Mat C,IS rowemb,IS colemb,MatStructure pattern,Mat B)
{
  /* If making this function public, change the error returned in this function away from _PLIB. */
  PetscErrorCode ierr;
  Mat_SeqAIJ     *Baij;
  PetscBool      seqaij;
  PetscInt       m,n,*nz,i,j,count;
  PetscScalar    v;
  const PetscInt *rowindices,*colindices;

  PetscFunctionBegin;
  if (!B) PetscFunctionReturn(0);
  /* Check to make sure the target matrix (and embeddings) are compatible with C and each other. */
  ierr = PetscObjectTypeCompare((PetscObject)B,MATSEQAIJ,&seqaij);CHKERRQ(ierr);
  if (!seqaij) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_PLIB,"Input matrix is of wrong type");
  if (rowemb) {
    ierr = ISGetLocalSize(rowemb,&m);CHKERRQ(ierr);
    if (m != B->rmap->n) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_PLIB,"Row IS of size %D is incompatible with matrix row size %D",m,B->rmap->n);
  } else {
    if (C->rmap->n != B->rmap->n) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_PLIB,"Input matrix is row-incompatible with the target matrix");
  }
  if (colemb) {
    ierr = ISGetLocalSize(colemb,&n);CHKERRQ(ierr);
    if (n != B->cmap->n) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_PLIB,"Diag col IS of size %D is incompatible with input matrix col size %D",n,B->cmap->n);
  } else {
    if (C->cmap->n != B->cmap->n) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_PLIB,"Input matrix is col-incompatible with the target matrix");
  }

  Baij = (Mat_SeqAIJ*)(B->data);
  if (pattern == DIFFERENT_NONZERO_PATTERN) {
    ierr = PetscMalloc1(B->rmap->n,&nz);CHKERRQ(ierr);
    for (i=0; i<B->rmap->n; i++) {
      nz[i] = Baij->i[i+1] - Baij->i[i];
    }
    ierr = MatSeqAIJSetPreallocation(C,0,nz);CHKERRQ(ierr);
    ierr = PetscFree(nz);CHKERRQ(ierr);
  }
  if (pattern == SUBSET_NONZERO_PATTERN) {
    ierr = MatZeroEntries(C);CHKERRQ(ierr);
  }
  count = 0;
  rowindices = NULL;
  colindices = NULL;
  if (rowemb) {
    ierr = ISGetIndices(rowemb,&rowindices);CHKERRQ(ierr);
  }
  if (colemb) {
    ierr = ISGetIndices(colemb,&colindices);CHKERRQ(ierr);
  }
  for (i=0; i<B->rmap->n; i++) {
    PetscInt row;
    row = i;
    if (rowindices) row = rowindices[i];
    for (j=Baij->i[i]; j<Baij->i[i+1]; j++) {
      PetscInt col;
      col  = Baij->j[count];
      if (colindices) col = colindices[col];
      v    = Baij->a[count];
      ierr = MatSetValues(C,1,&row,1,&col,&v,INSERT_VALUES);CHKERRQ(ierr);
      ++count;
    }
  }
  /* FIXME: set C's nonzerostate correctly. */
  /* Assembly for C is necessary. */
  C->preallocated = PETSC_TRUE;
  C->assembled     = PETSC_TRUE;
  C->was_assembled = PETSC_FALSE;
  PetscFunctionReturn(0);
}


/*
    Special version for direct calls from Fortran
*/
#include <petsc/private/fortranimpl.h>
#if defined(PETSC_HAVE_FORTRAN_CAPS)
#define matsetvaluesseqaij_ MATSETVALUESSEQAIJ
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE)
#define matsetvaluesseqaij_ matsetvaluesseqaij
#endif

/* Change these macros so can be used in void function */
#undef CHKERRQ
#define CHKERRQ(ierr) CHKERRABORT(PetscObjectComm((PetscObject)A),ierr)
#undef SETERRQ2
#define SETERRQ2(comm,ierr,b,c,d) CHKERRABORT(comm,ierr)
#undef SETERRQ3
#define SETERRQ3(comm,ierr,b,c,d,e) CHKERRABORT(comm,ierr)

#undef __FUNCT__
#define __FUNCT__ "matsetvaluesseqaij_"
PETSC_EXTERN void PETSC_STDCALL matsetvaluesseqaij_(Mat *AA,PetscInt *mm,const PetscInt im[],PetscInt *nn,const PetscInt in[],const PetscScalar v[],InsertMode *isis, PetscErrorCode *_ierr)
{
  Mat            A  = *AA;
  PetscInt       m  = *mm, n = *nn;
  InsertMode     is = *isis;
  Mat_SeqAIJ     *a = (Mat_SeqAIJ*)A->data;
  PetscInt       *rp,k,low,high,t,ii,row,nrow,i,col,l,rmax,N;
  PetscInt       *imax,*ai,*ailen;
  PetscErrorCode ierr;
  PetscInt       *aj,nonew = a->nonew,lastcol = -1;
  MatScalar      *ap,value,*aa;
  PetscBool      ignorezeroentries = a->ignorezeroentries;
  PetscBool      roworiented       = a->roworiented;

  PetscFunctionBegin;
  MatCheckPreallocated(A,1);
  imax  = a->imax;
  ai    = a->i;
  ailen = a->ilen;
  aj    = a->j;
  aa    = a->a;

  for (k=0; k<m; k++) { /* loop over added rows */
    row = im[k];
    if (row < 0) continue;
#if defined(PETSC_USE_DEBUG)
    if (row >= A->rmap->n) SETERRABORT(PetscObjectComm((PetscObject)A),PETSC_ERR_ARG_OUTOFRANGE,"Row too large");
#endif
    rp   = aj + ai[row]; ap = aa + ai[row];
    rmax = imax[row]; nrow = ailen[row];
    low  = 0;
    high = nrow;
    for (l=0; l<n; l++) { /* loop over added columns */
      if (in[l] < 0) continue;
#if defined(PETSC_USE_DEBUG)
      if (in[l] >= A->cmap->n) SETERRABORT(PetscObjectComm((PetscObject)A),PETSC_ERR_ARG_OUTOFRANGE,"Column too large");
#endif
      col = in[l];
      if (roworiented) value = v[l + k*n];
      else value = v[k + l*m];

      if (value == 0.0 && ignorezeroentries && (is == ADD_VALUES)) continue;

      if (col <= lastcol) low = 0;
      else high = nrow;
      lastcol = col;
      while (high-low > 5) {
        t = (low+high)/2;
        if (rp[t] > col) high = t;
        else             low  = t;
      }
      for (i=low; i<high; i++) {
        if (rp[i] > col) break;
        if (rp[i] == col) {
          if (is == ADD_VALUES) ap[i] += value;
          else                  ap[i] = value;
          goto noinsert;
        }
      }
      if (value == 0.0 && ignorezeroentries) goto noinsert;
      if (nonew == 1) goto noinsert;
      if (nonew == -1) SETERRABORT(PetscObjectComm((PetscObject)A),PETSC_ERR_ARG_OUTOFRANGE,"Inserting a new nonzero in the matrix");
      MatSeqXAIJReallocateAIJ(A,A->rmap->n,1,nrow,row,col,rmax,aa,ai,aj,rp,ap,imax,nonew,MatScalar);
      N = nrow++ - 1; a->nz++; high++;
      /* shift up all the later entries in this row */
      for (ii=N; ii>=i; ii--) {
        rp[ii+1] = rp[ii];
        ap[ii+1] = ap[ii];
      }
      rp[i] = col;
      ap[i] = value;
      A->nonzerostate++;
noinsert:;
      low = i + 1;
    }
    ailen[row] = nrow;
  }
  PetscFunctionReturnVoid();
}

