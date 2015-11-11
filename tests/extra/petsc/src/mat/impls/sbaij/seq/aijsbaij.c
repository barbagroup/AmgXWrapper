
#include <../src/mat/impls/aij/seq/aij.h>
#include <../src/mat/impls/baij/seq/baij.h>
#include <../src/mat/impls/sbaij/seq/sbaij.h>

#undef __FUNCT__
#define __FUNCT__ "MatConvert_SeqSBAIJ_SeqAIJ"
PETSC_EXTERN PetscErrorCode MatConvert_SeqSBAIJ_SeqAIJ(Mat A, MatType newtype,MatReuse reuse,Mat *newmat)
{
  Mat            B;
  Mat_SeqSBAIJ   *a = (Mat_SeqSBAIJ*)A->data;
  Mat_SeqAIJ     *b;
  PetscErrorCode ierr;
  PetscInt       *ai=a->i,*aj=a->j,m=A->rmap->N,n=A->cmap->n,i,j,k,*bi,*bj,*rowlengths,nz,*rowstart,itmp;
  PetscInt       bs =A->rmap->bs,bs2=bs*bs,mbs=A->rmap->N/bs,diagcnt=0;
  MatScalar      *av,*bv;

  PetscFunctionBegin;
  /* compute rowlengths of newmat */
  ierr = PetscMalloc2(m,&rowlengths,m+1,&rowstart);CHKERRQ(ierr);

  for (i=0; i<mbs; i++) rowlengths[i*bs] = 0;
  aj = a->j;
  k  = 0;
  for (i=0; i<mbs; i++) {
    nz = ai[i+1] - ai[i];
    if (nz) {
      rowlengths[k] += nz;   /* no. of upper triangular blocks */
      if (*aj == i) {aj++;diagcnt++;nz--;} /* skip diagonal */
      for (j=0; j<nz; j++) { /* no. of lower triangular blocks */
        rowlengths[(*aj)*bs]++; aj++;
      }
    }
    rowlengths[k] *= bs;
    for (j=1; j<bs; j++) {
      rowlengths[k+j] = rowlengths[k];
    }
    k += bs;
    /* printf(" rowlengths[%d]: %d\n",i, rowlengths[i]); */
  }

  ierr = MatCreate(PetscObjectComm((PetscObject)A),&B);CHKERRQ(ierr);
  ierr = MatSetSizes(B,m,n,m,n);CHKERRQ(ierr);
  ierr = MatSetType(B,MATSEQAIJ);CHKERRQ(ierr);
  ierr = MatSeqAIJSetPreallocation(B,0,rowlengths);CHKERRQ(ierr);
  ierr = MatSetOption(B,MAT_ROW_ORIENTED,PETSC_FALSE);CHKERRQ(ierr);

  B->rmap->bs = A->rmap->bs;

  b  = (Mat_SeqAIJ*)(B->data);
  bi = b->i;
  bj = b->j;
  bv = b->a;

  /* set b->i */
  bi[0] = 0; rowstart[0] = 0;
  for (i=0; i<mbs; i++) {
    for (j=0; j<bs; j++) {
      b->ilen[i*bs+j]    = rowlengths[i*bs];
      rowstart[i*bs+j+1] = rowstart[i*bs+j] + rowlengths[i*bs];
    }
    bi[i+1] = bi[i] + rowlengths[i*bs]/bs;
  }
  if (bi[mbs] != 2*a->nz - diagcnt) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_PLIB,"bi[mbs]: %D != 2*a->nz-diagcnt: %D\n",bi[mbs],2*a->nz - diagcnt);

  /* set b->j and b->a */
  aj = a->j; av = a->a;
  for (i=0; i<mbs; i++) {
    nz = ai[i+1] - ai[i];
    /* diagonal block */
    if (nz && *aj == i) {
      nz--;
      for (j=0; j<bs; j++) {   /* row i*bs+j */
        itmp = i*bs+j;
        for (k=0; k<bs; k++) { /* col i*bs+k */
          *(bj + rowstart[itmp]) = (*aj)*bs+k;
          *(bv + rowstart[itmp]) = *(av+k*bs+j);
          rowstart[itmp]++;
        }
      }
      aj++; av += bs2;
    }

    while (nz--) {
      /* lower triangular blocks */
      for (j=0; j<bs; j++) {   /* row (*aj)*bs+j */
        itmp = (*aj)*bs+j;
        for (k=0; k<bs; k++) { /* col i*bs+k */
          *(bj + rowstart[itmp]) = i*bs+k;
          *(bv + rowstart[itmp]) = *(av+j*bs+k);
          rowstart[itmp]++;
        }
      }
      /* upper triangular blocks */
      for (j=0; j<bs; j++) {   /* row i*bs+j */
        itmp = i*bs+j;
        for (k=0; k<bs; k++) { /* col (*aj)*bs+k */
          *(bj + rowstart[itmp]) = (*aj)*bs+k;
          *(bv + rowstart[itmp]) = *(av+k*bs+j);
          rowstart[itmp]++;
        }
      }
      aj++; av += bs2;
    }
  }
  ierr = PetscFree2(rowlengths,rowstart);CHKERRQ(ierr);
  ierr = MatAssemblyBegin(B,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(B,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);

  if (reuse == MAT_INPLACE_MATRIX) {
    ierr = MatHeaderReplace(A,&B);CHKERRQ(ierr);
  } else {
    *newmat = B;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatConvert_SeqAIJ_SeqSBAIJ"
PETSC_EXTERN PetscErrorCode MatConvert_SeqAIJ_SeqSBAIJ(Mat A,MatType newtype,MatReuse reuse,Mat *newmat)
{
  Mat            B;
  Mat_SeqAIJ     *a = (Mat_SeqAIJ*)A->data;
  Mat_SeqSBAIJ   *b;
  PetscErrorCode ierr;
  PetscInt       *ai=a->i,*aj,m=A->rmap->N,n=A->cmap->N,i,j,*bi,*bj,*rowlengths;
  MatScalar      *av,*bv;

  PetscFunctionBegin;
  if (!A->symmetric) SETERRQ(PetscObjectComm((PetscObject)A),PETSC_ERR_USER,"Matrix must be symmetric. Call MatSetOption(mat,MAT_SYMMETRIC,PETSC_TRUE)");
  if (n != m) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONG,"Matrix must be square");

  ierr = PetscMalloc1(m,&rowlengths);CHKERRQ(ierr);
  for (i=0; i<m; i++) {
    rowlengths[i] = ai[i+1] - a->diag[i];
  }
  ierr = MatCreate(PetscObjectComm((PetscObject)A),&B);CHKERRQ(ierr);
  ierr = MatSetSizes(B,m,n,m,n);CHKERRQ(ierr);
  ierr = MatSetType(B,MATSEQSBAIJ);CHKERRQ(ierr);
  ierr = MatSeqSBAIJSetPreallocation_SeqSBAIJ(B,1,0,rowlengths);CHKERRQ(ierr);

  ierr = MatSetOption(B,MAT_ROW_ORIENTED,PETSC_TRUE);CHKERRQ(ierr);

  b  = (Mat_SeqSBAIJ*)(B->data);
  bi = b->i;
  bj = b->j;
  bv = b->a;

  bi[0] = 0;
  for (i=0; i<m; i++) {
    aj = a->j + a->diag[i];
    av = a->a + a->diag[i];
    for (j=0; j<rowlengths[i]; j++) {
      *bj = *aj; bj++; aj++;
      *bv = *av; bv++; av++;
    }
    bi[i+1]    = bi[i] + rowlengths[i];
    b->ilen[i] = rowlengths[i];
  }

  ierr = PetscFree(rowlengths);CHKERRQ(ierr);
  ierr = MatAssemblyBegin(B,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(B,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);

  if (reuse == MAT_INPLACE_MATRIX) {
    ierr = MatHeaderReplace(A,&B);CHKERRQ(ierr);
  } else {
    *newmat = B;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatConvert_SeqSBAIJ_SeqBAIJ"
PETSC_EXTERN PetscErrorCode MatConvert_SeqSBAIJ_SeqBAIJ(Mat A, MatType newtype,MatReuse reuse,Mat *newmat)
{
  Mat            B;
  Mat_SeqSBAIJ   *a = (Mat_SeqSBAIJ*)A->data;
  Mat_SeqBAIJ    *b;
  PetscErrorCode ierr;
  PetscInt       *ai=a->i,*aj=a->j,m=A->rmap->N,n=A->cmap->n,i,k,*bi,*bj,*browlengths,nz,*browstart,itmp;
  PetscInt       bs =A->rmap->bs,bs2=bs*bs,mbs=m/bs,col,row;
  MatScalar      *av,*bv;

  PetscFunctionBegin;
  /* compute browlengths of newmat */
  ierr = PetscMalloc2(mbs,&browlengths,mbs,&browstart);CHKERRQ(ierr);
  for (i=0; i<mbs; i++) browlengths[i] = 0;
  aj = a->j;
  for (i=0; i<mbs; i++) {
    nz = ai[i+1] - ai[i];
    aj++; /* skip diagonal */
    for (k=1; k<nz; k++) { /* no. of lower triangular blocks */
      browlengths[*aj]++; aj++;
    }
    browlengths[i] += nz;   /* no. of upper triangular blocks */
  }

  ierr = MatCreate(PetscObjectComm((PetscObject)A),&B);CHKERRQ(ierr);
  ierr = MatSetSizes(B,m,n,m,n);CHKERRQ(ierr);
  ierr = MatSetType(B,MATSEQBAIJ);CHKERRQ(ierr);
  ierr = MatSeqBAIJSetPreallocation(B,bs,0,browlengths);CHKERRQ(ierr);
  ierr = MatSetOption(B,MAT_ROW_ORIENTED,PETSC_TRUE);CHKERRQ(ierr);

  b  = (Mat_SeqBAIJ*)(B->data);
  bi = b->i;
  bj = b->j;
  bv = b->a;

  /* set b->i */
  bi[0] = 0;
  for (i=0; i<mbs; i++) {
    b->ilen[i]   = browlengths[i];
    bi[i+1]      = bi[i] + browlengths[i];
    browstart[i] = bi[i];
  }
  if (bi[mbs] != 2*a->nz - mbs) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_PLIB,"bi[mbs]: %D != 2*a->nz - mbs: %D\n",bi[mbs],2*a->nz - mbs);

  /* set b->j and b->a */
  aj = a->j; av = a->a;
  for (i=0; i<mbs; i++) {
    /* diagonal block */
    *(bj + browstart[i]) = *aj; aj++;

    itmp = bs2*browstart[i];
    for (k=0; k<bs2; k++) {
      *(bv + itmp + k) = *av; av++;
    }
    browstart[i]++;

    nz = ai[i+1] - ai[i] -1;
    while (nz--) {
      /* lower triangular blocks - transpose blocks of A */
      *(bj + browstart[*aj]) = i; /* block col index */

      itmp = bs2*browstart[*aj];  /* row index */
      for (col=0; col<bs; col++) {
        k = col;
        for (row=0; row<bs; row++) {
          bv[itmp + col*bs+row] = av[k]; k+=bs;
        }
      }
      browstart[*aj]++;

      /* upper triangular blocks */
      *(bj + browstart[i]) = *aj; aj++;

      itmp = bs2*browstart[i];
      for (k=0; k<bs2; k++) {
        bv[itmp + k] = av[k];
      }
      av += bs2;
      browstart[i]++;
    }
  }
  ierr = PetscFree2(browlengths,browstart);CHKERRQ(ierr);
  ierr = MatAssemblyBegin(B,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(B,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);

  if (reuse == MAT_INPLACE_MATRIX) {
    ierr = MatHeaderReplace(A,&B);CHKERRQ(ierr);
  } else {
    *newmat = B;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatConvert_SeqBAIJ_SeqSBAIJ"
PETSC_EXTERN PetscErrorCode MatConvert_SeqBAIJ_SeqSBAIJ(Mat A, MatType newtype,MatReuse reuse,Mat *newmat)
{
  Mat            B;
  Mat_SeqBAIJ    *a = (Mat_SeqBAIJ*)A->data;
  Mat_SeqSBAIJ   *b;
  PetscErrorCode ierr;
  PetscInt       *ai=a->i,*aj,m=A->rmap->N,n=A->cmap->n,i,j,k,*bi,*bj,*browlengths;
  PetscInt       bs =A->rmap->bs,bs2=bs*bs,mbs=m/bs,dd;
  MatScalar      *av,*bv;
  PetscBool      flg;

  PetscFunctionBegin;
  if (!A->symmetric) SETERRQ(PetscObjectComm((PetscObject)A),PETSC_ERR_USER,"Matrix must be symmetric. Call MatSetOption(mat,MAT_SYMMETRIC,PETSC_TRUE)");
  if (n != m) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONG,"Matrix must be square");
  ierr = MatMissingDiagonal_SeqBAIJ(A,&flg,&dd);CHKERRQ(ierr); /* check for missing diagonals, then mark diag */
  if (flg) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONGSTATE,"Matrix is missing diagonal %D",dd);

  ierr = PetscMalloc1(mbs,&browlengths);CHKERRQ(ierr);
  for (i=0; i<mbs; i++) {
    browlengths[i] = ai[i+1] - a->diag[i];
  }

  ierr = MatCreate(PetscObjectComm((PetscObject)A),&B);CHKERRQ(ierr);
  ierr = MatSetSizes(B,m,n,m,n);CHKERRQ(ierr);
  ierr = MatSetType(B,MATSEQSBAIJ);CHKERRQ(ierr);
  ierr = MatSeqSBAIJSetPreallocation_SeqSBAIJ(B,bs,0,browlengths);CHKERRQ(ierr);
  ierr = MatSetOption(B,MAT_ROW_ORIENTED,PETSC_TRUE);CHKERRQ(ierr);

  b  = (Mat_SeqSBAIJ*)(B->data);
  bi = b->i;
  bj = b->j;
  bv = b->a;

  bi[0] = 0;
  for (i=0; i<mbs; i++) {
    aj = a->j + a->diag[i];
    av = a->a + (a->diag[i])*bs2;
    for (j=0; j<browlengths[i]; j++) {
      *bj = *aj; bj++; aj++;
      for (k=0; k<bs2; k++) {
        *bv = *av; bv++; av++;
      }
    }
    bi[i+1]    = bi[i] + browlengths[i];
    b->ilen[i] = browlengths[i];
  }
  ierr = PetscFree(browlengths);CHKERRQ(ierr);
  ierr = MatAssemblyBegin(B,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(B,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);

  if (reuse == MAT_INPLACE_MATRIX) {
    ierr = MatHeaderReplace(A,&B);CHKERRQ(ierr);
  } else {
    *newmat = B;
  }
  PetscFunctionReturn(0);
}
