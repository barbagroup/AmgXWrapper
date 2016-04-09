
#include <../src/mat/impls/aij/seq/aij.h>
#include <../src/mat/impls/baij/seq/baij.h>
#include <petsc/private/isimpl.h>

/*
    This routine is shared by SeqAIJ and SeqBAIJ matrices,
    since it operators only on the nonzero structure of the elements or blocks.
*/
#undef __FUNCT__
#define __FUNCT__ "MatFDColoringCreate_SeqXAIJ"
PetscErrorCode MatFDColoringCreate_SeqXAIJ(Mat mat,ISColoring iscoloring,MatFDColoring c)
{
  PetscErrorCode ierr;
  PetscInt       bs,nis=iscoloring->n,m=mat->rmap->n;
  PetscBool      isBAIJ;

  PetscFunctionBegin;
  /* set default brows and bcols for speedup inserting the dense matrix into sparse Jacobian */
  ierr = MatGetBlockSize(mat,&bs);CHKERRQ(ierr);
  ierr = PetscObjectTypeCompare((PetscObject)mat,MATSEQBAIJ,&isBAIJ);CHKERRQ(ierr);
  if (isBAIJ) {
    c->brows = m;
    c->bcols = 1;
  } else { /* seqaij matrix */
    /* bcols is chosen s.t. dy-array takes 50% of memory space as mat */
    Mat_SeqAIJ *spA = (Mat_SeqAIJ*)mat->data;
    PetscReal  mem;
    PetscInt   nz,brows,bcols;

    bs    = 1; /* only bs=1 is supported for SeqAIJ matrix */

    nz    = spA->nz;
    mem   = nz*(sizeof(PetscScalar) + sizeof(PetscInt)) + 3*m*sizeof(PetscInt);
    bcols = (PetscInt)(0.5*mem /(m*sizeof(PetscScalar)));
    brows = 1000/bcols;
    if (bcols > nis) bcols = nis;
    if (brows == 0 || brows > m) brows = m;
    c->brows = brows;
    c->bcols = bcols;
  }

  c->M       = mat->rmap->N/bs;   /* set total rows, columns and local rows */
  c->N       = mat->cmap->N/bs;
  c->m       = mat->rmap->N/bs;
  c->rstart  = 0;
  c->ncolors = nis;
  c->ctype   = IS_COLORING_GHOSTED;
  PetscFunctionReturn(0);
}

/*
 Reorder Jentry such that blocked brows*bols of entries from dense matrix are inserted into Jacobian for improved cache performance
   Input Parameters:
+  mat - the matrix containing the nonzero structure of the Jacobian
.  color - the coloring context
-  nz - number of local non-zeros in mat
*/
#undef __FUNCT__
#define __FUNCT__ "MatFDColoringSetUpBlocked_AIJ_Private"
PetscErrorCode MatFDColoringSetUpBlocked_AIJ_Private(Mat mat,MatFDColoring c,PetscInt nz)
{
  PetscErrorCode ierr;
  PetscInt       i,j,nrows,nbcols,brows=c->brows,bcols=c->bcols,mbs=c->m,nis=c->ncolors;
  PetscInt       *color_start,*row_start,*nrows_new,nz_new,row_end;

  PetscFunctionBegin;
  if (brows < 1 || brows > mbs) brows = mbs;
  ierr = PetscMalloc2(bcols+1,&color_start,bcols,&row_start);CHKERRQ(ierr);
  ierr = PetscMalloc1(nis,&nrows_new);CHKERRQ(ierr);
  ierr = PetscMalloc1(bcols*mat->rmap->n,&c->dy);CHKERRQ(ierr);
  ierr = PetscLogObjectMemory((PetscObject)c,bcols*mat->rmap->n*sizeof(PetscScalar));CHKERRQ(ierr);

  nz_new = 0;
  nbcols = 0;
  color_start[bcols] = 0;

  if (c->htype[0] == 'd') { /* ----  c->htype == 'ds', use MatEntry --------*/
    MatEntry       *Jentry_new,*Jentry=c->matentry;
    ierr = PetscMalloc1(nz,&Jentry_new);CHKERRQ(ierr);
    for (i=0; i<nis; i+=bcols) { /* loop over colors */
      if (i + bcols > nis) {
        color_start[nis - i] = color_start[bcols];
        bcols                = nis - i;
      }

      color_start[0] = color_start[bcols];
      for (j=0; j<bcols; j++) {
        color_start[j+1] = c->nrows[i+j] + color_start[j];
        row_start[j]     = 0;
      }

      row_end = brows;
      if (row_end > mbs) row_end = mbs;

      while (row_end <= mbs) {   /* loop over block rows */
        for (j=0; j<bcols; j++) {       /* loop over block columns */
          nrows = c->nrows[i+j];
          nz    = color_start[j];
          while (row_start[j] < nrows) {
            if (Jentry[nz].row >= row_end) {
              color_start[j] = nz;
              break;
            } else { /* copy Jentry[nz] to Jentry_new[nz_new] */
              Jentry_new[nz_new].row     = Jentry[nz].row + j*mbs; /* index in dy-array */
              Jentry_new[nz_new].col     = Jentry[nz].col;
              Jentry_new[nz_new].valaddr = Jentry[nz].valaddr;
              nz_new++; nz++; row_start[j]++;
            }
          }
        }
        if (row_end == mbs) break;
        row_end += brows;
        if (row_end > mbs) row_end = mbs;
      }
      nrows_new[nbcols++] = nz_new;
    }
    ierr = PetscFree(Jentry);CHKERRQ(ierr);
    c->matentry = Jentry_new;
  } else { /* ---------  c->htype == 'wp', use MatEntry2 ------------------*/
    MatEntry2      *Jentry2_new,*Jentry2=c->matentry2;
    ierr = PetscMalloc1(nz,&Jentry2_new);CHKERRQ(ierr);
    for (i=0; i<nis; i+=bcols) { /* loop over colors */
      if (i + bcols > nis) {
        color_start[nis - i] = color_start[bcols];
        bcols                = nis - i;
      }

      color_start[0] = color_start[bcols];
      for (j=0; j<bcols; j++) {
        color_start[j+1] = c->nrows[i+j] + color_start[j];
        row_start[j]     = 0;
      }

      row_end = brows;
      if (row_end > mbs) row_end = mbs;

      while (row_end <= mbs) {   /* loop over block rows */
        for (j=0; j<bcols; j++) {       /* loop over block columns */
          nrows = c->nrows[i+j];
          nz    = color_start[j];
          while (row_start[j] < nrows) {
            if (Jentry2[nz].row >= row_end) {
              color_start[j] = nz;
              break;
            } else { /* copy Jentry2[nz] to Jentry2_new[nz_new] */
              Jentry2_new[nz_new].row     = Jentry2[nz].row + j*mbs; /* index in dy-array */
              Jentry2_new[nz_new].valaddr = Jentry2[nz].valaddr;
              nz_new++; nz++; row_start[j]++;
            }
          }
        }
        if (row_end == mbs) break;
        row_end += brows;
        if (row_end > mbs) row_end = mbs;
      }
      nrows_new[nbcols++] = nz_new;
    }
    ierr = PetscFree(Jentry2);CHKERRQ(ierr);
    c->matentry2 = Jentry2_new;
  } /* ---------------------------------------------*/

  ierr = PetscFree2(color_start,row_start);CHKERRQ(ierr);

  for (i=nbcols-1; i>0; i--) nrows_new[i] -= nrows_new[i-1];
  ierr = PetscFree(c->nrows);CHKERRQ(ierr);
  c->nrows = nrows_new;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatFDColoringSetUp_SeqXAIJ"
PetscErrorCode MatFDColoringSetUp_SeqXAIJ(Mat mat,ISColoring iscoloring,MatFDColoring c)
{
  PetscErrorCode ierr;
  PetscInt       i,n,nrows,mbs=c->m,j,k,m,ncols,col,nis=iscoloring->n,*rowhit,bs,bs2,*spidx,nz;
  const PetscInt *is,*row,*ci,*cj;
  IS             *isa;
  PetscBool      isBAIJ;
  PetscScalar    *A_val,**valaddrhit;
  MatEntry       *Jentry;
  MatEntry2      *Jentry2;

  PetscFunctionBegin;
  ierr = ISColoringGetIS(iscoloring,PETSC_IGNORE,&isa);CHKERRQ(ierr);

  ierr = MatGetBlockSize(mat,&bs);CHKERRQ(ierr);
  ierr = PetscObjectTypeCompare((PetscObject)mat,MATSEQBAIJ,&isBAIJ);CHKERRQ(ierr);
  if (isBAIJ) {
    Mat_SeqBAIJ *spA = (Mat_SeqBAIJ*)mat->data;
    A_val = spA->a;
    nz    = spA->nz;
  } else {
    Mat_SeqAIJ  *spA = (Mat_SeqAIJ*)mat->data;
    A_val = spA->a;
    nz    = spA->nz;
    bs    = 1; /* only bs=1 is supported for SeqAIJ matrix */
  }

  ierr       = PetscMalloc1(nis,&c->ncolumns);CHKERRQ(ierr);
  ierr       = PetscMalloc1(nis,&c->columns);CHKERRQ(ierr);
  ierr       = PetscMalloc1(nis,&c->nrows);CHKERRQ(ierr);
  ierr       = PetscLogObjectMemory((PetscObject)c,3*nis*sizeof(PetscInt));CHKERRQ(ierr);

  if (c->htype[0] == 'd') {
    ierr       = PetscMalloc1(nz,&Jentry);CHKERRQ(ierr);
    ierr       = PetscLogObjectMemory((PetscObject)c,nz*sizeof(MatEntry));CHKERRQ(ierr);
    c->matentry = Jentry;
  } else if (c->htype[0] == 'w') {
    ierr       = PetscMalloc1(nz,&Jentry2);CHKERRQ(ierr);
    ierr       = PetscLogObjectMemory((PetscObject)c,nz*sizeof(MatEntry2));CHKERRQ(ierr);
    c->matentry2 = Jentry2;
  } else SETERRQ(PetscObjectComm((PetscObject)mat),PETSC_ERR_SUP,"htype is not supported");

  if (isBAIJ) {
    ierr = MatGetColumnIJ_SeqBAIJ_Color(mat,0,PETSC_FALSE,PETSC_FALSE,&ncols,&ci,&cj,&spidx,NULL);CHKERRQ(ierr);
  } else {
    ierr = MatGetColumnIJ_SeqAIJ_Color(mat,0,PETSC_FALSE,PETSC_FALSE,&ncols,&ci,&cj,&spidx,NULL);CHKERRQ(ierr);
  }

  ierr = PetscMalloc2(c->m,&rowhit,c->m,&valaddrhit);CHKERRQ(ierr);
  ierr = PetscMemzero(rowhit,c->m*sizeof(PetscInt));CHKERRQ(ierr);

  nz = 0;
  for (i=0; i<nis; i++) { /* loop over colors */
    ierr = ISGetLocalSize(isa[i],&n);CHKERRQ(ierr);
    ierr = ISGetIndices(isa[i],&is);CHKERRQ(ierr);

    c->ncolumns[i] = n;
    if (n) {
      ierr = PetscMalloc1(n,&c->columns[i]);CHKERRQ(ierr);
      ierr = PetscLogObjectMemory((PetscObject)c,n*sizeof(PetscInt));CHKERRQ(ierr);
      ierr = PetscMemcpy(c->columns[i],is,n*sizeof(PetscInt));CHKERRQ(ierr);
    } else {
      c->columns[i] = 0;
    }

    /* fast, crude version requires O(N*N) work */
    bs2   = bs*bs;
    nrows = 0;
    for (j=0; j<n; j++) {  /* loop over columns */
      col    = is[j];
      row    = cj + ci[col];
      m      = ci[col+1] - ci[col];
      nrows += m;
      for (k=0; k<m; k++) {  /* loop over columns marking them in rowhit */
        rowhit[*row]       = col + 1;
        valaddrhit[*row++] = &A_val[bs2*spidx[ci[col] + k]];
      }
    }
    c->nrows[i] = nrows; /* total num of rows for this color */

    if (c->htype[0] == 'd') {
      for (j=0; j<mbs; j++) { /* loop over rows */
        if (rowhit[j]) {
          Jentry[nz].row     = j;              /* local row index */
          Jentry[nz].col     = rowhit[j] - 1;  /* local column index */
          Jentry[nz].valaddr = valaddrhit[j];  /* address of mat value for this entry */
          nz++;
          rowhit[j] = 0.0;                     /* zero rowhit for reuse */
        }
      }
    }  else { /* c->htype == 'wp' */
      for (j=0; j<mbs; j++) { /* loop over rows */
        if (rowhit[j]) {
          Jentry2[nz].row     = j;              /* local row index */
          Jentry2[nz].valaddr = valaddrhit[j];  /* address of mat value for this entry */
          nz++;
          rowhit[j] = 0.0;                     /* zero rowhit for reuse */
        }
      }
    }
    ierr = ISRestoreIndices(isa[i],&is);CHKERRQ(ierr);
  }

  if (c->bcols > 1) {  /* reorder Jentry for faster MatFDColoringApply() */
    ierr = MatFDColoringSetUpBlocked_AIJ_Private(mat,c,nz);CHKERRQ(ierr);
  }

  if (isBAIJ) {
    ierr = MatRestoreColumnIJ_SeqBAIJ_Color(mat,0,PETSC_FALSE,PETSC_FALSE,&ncols,&ci,&cj,&spidx,NULL);CHKERRQ(ierr);
    ierr = PetscMalloc1(bs*mat->rmap->n,&c->dy);CHKERRQ(ierr);
    ierr = PetscLogObjectMemory((PetscObject)c,bs*mat->rmap->n*sizeof(PetscScalar));CHKERRQ(ierr);
  } else {
    ierr = MatRestoreColumnIJ_SeqAIJ_Color(mat,0,PETSC_FALSE,PETSC_FALSE,&ncols,&ci,&cj,&spidx,NULL);CHKERRQ(ierr);
  }
  ierr = PetscFree2(rowhit,valaddrhit);CHKERRQ(ierr);
  ierr = ISColoringRestoreIS(iscoloring,&isa);CHKERRQ(ierr);

  ierr = VecCreateGhost(PetscObjectComm((PetscObject)mat),mat->rmap->n,PETSC_DETERMINE,0,NULL,&c->vscale);CHKERRQ(ierr);
  ierr = PetscInfo3(c,"ncolors %D, brows %D and bcols %D are used.\n",c->ncolors,c->brows,c->bcols);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}
