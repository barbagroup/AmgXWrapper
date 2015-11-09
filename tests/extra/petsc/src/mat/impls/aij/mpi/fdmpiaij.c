
#include <../src/mat/impls/aij/mpi/mpiaij.h>
#include <../src/mat/impls/baij/mpi/mpibaij.h>
#include <petsc/private/isimpl.h>

#undef __FUNCT__
#define __FUNCT__ "MatFDColoringApply_BAIJ"
PetscErrorCode  MatFDColoringApply_BAIJ(Mat J,MatFDColoring coloring,Vec x1,void *sctx)
{
  PetscErrorCode    (*f)(void*,Vec,Vec,void*) = (PetscErrorCode (*)(void*,Vec,Vec,void*))coloring->f;
  PetscErrorCode    ierr;
  PetscInt          k,cstart,cend,l,row,col,nz,spidx,i,j;
  PetscScalar       dx=0.0,*w3_array,*dy_i,*dy=coloring->dy;
  PetscScalar       *vscale_array;
  const PetscScalar *xx;
  PetscReal         epsilon=coloring->error_rel,umin=coloring->umin,unorm;
  Vec               w1=coloring->w1,w2=coloring->w2,w3,vscale=coloring->vscale;
  void              *fctx=coloring->fctx;
  PetscInt          ctype=coloring->ctype,nxloc,nrows_k;
  PetscScalar       *valaddr;
  MatEntry          *Jentry=coloring->matentry;
  MatEntry2         *Jentry2=coloring->matentry2;
  const PetscInt    ncolors=coloring->ncolors,*ncolumns=coloring->ncolumns,*nrows=coloring->nrows;
  PetscInt          bs=J->rmap->bs;

  PetscFunctionBegin;
  /* (1) Set w1 = F(x1) */
  if (!coloring->fset) {
    ierr = PetscLogEventBegin(MAT_FDColoringFunction,0,0,0,0);CHKERRQ(ierr);
    ierr = (*f)(sctx,x1,w1,fctx);CHKERRQ(ierr);
    ierr = PetscLogEventEnd(MAT_FDColoringFunction,0,0,0,0);CHKERRQ(ierr);
  } else {
    coloring->fset = PETSC_FALSE;
  }

  /* (2) Compute vscale = 1./dx - the local scale factors, including ghost points */
  ierr = VecGetLocalSize(x1,&nxloc);CHKERRQ(ierr);
  if (coloring->htype[0] == 'w') {
    /* vscale = dx is a constant scalar */
    ierr = VecNorm(x1,NORM_2,&unorm);CHKERRQ(ierr);
    dx = 1.0/(PetscSqrtReal(1.0 + unorm)*epsilon);
  } else {
    ierr = VecGetArrayRead(x1,&xx);CHKERRQ(ierr);
    ierr = VecGetArray(vscale,&vscale_array);CHKERRQ(ierr);
    for (col=0; col<nxloc; col++) {
      dx = xx[col];
      if (PetscAbsScalar(dx) < umin) {
        if (PetscRealPart(dx) >= 0.0)      dx = umin;
        else if (PetscRealPart(dx) < 0.0 ) dx = -umin;
      }
      dx               *= epsilon;
      vscale_array[col] = 1.0/dx;
    }
    ierr = VecRestoreArrayRead(x1,&xx);CHKERRQ(ierr);
    ierr = VecRestoreArray(vscale,&vscale_array);CHKERRQ(ierr);
  }
  if (ctype == IS_COLORING_GLOBAL && coloring->htype[0] == 'd') {
    ierr = VecGhostUpdateBegin(vscale,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
    ierr = VecGhostUpdateEnd(vscale,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
  }

  /* (3) Loop over each color */
  if (!coloring->w3) {
    ierr = VecDuplicate(x1,&coloring->w3);CHKERRQ(ierr);
    ierr = PetscLogObjectParent((PetscObject)coloring,(PetscObject)coloring->w3);CHKERRQ(ierr);
  }
  w3 = coloring->w3;

  ierr = VecGetOwnershipRange(x1,&cstart,&cend);CHKERRQ(ierr); /* used by ghosted vscale */
  if (vscale) {
    ierr = VecGetArray(vscale,&vscale_array);CHKERRQ(ierr);
  }
  nz   = 0;
  for (k=0; k<ncolors; k++) {
    coloring->currentcolor = k;

    /*
      (3-1) Loop over each column associated with color
      adding the perturbation to the vector w3 = x1 + dx.
    */
    ierr = VecCopy(x1,w3);CHKERRQ(ierr);
    dy_i = dy;
    for (i=0; i<bs; i++) {     /* Loop over a block of columns */
      ierr = VecGetArray(w3,&w3_array);CHKERRQ(ierr);
      if (ctype == IS_COLORING_GLOBAL) w3_array -= cstart; /* shift pointer so global index can be used */
      if (coloring->htype[0] == 'w') {
        for (l=0; l<ncolumns[k]; l++) {
          col            = i + bs*coloring->columns[k][l];  /* local column (in global index!) of the matrix we are probing for */
          w3_array[col] += 1.0/dx;
          if (i) w3_array[col-1] -= 1.0/dx; /* resume original w3[col-1] */
        }
      } else { /* htype == 'ds' */
        vscale_array -= cstart; /* shift pointer so global index can be used */
        for (l=0; l<ncolumns[k]; l++) {
          col = i + bs*coloring->columns[k][l]; /* local column (in global index!) of the matrix we are probing for */
          w3_array[col] += 1.0/vscale_array[col];
          if (i) w3_array[col-1] -=  1.0/vscale_array[col-1]; /* resume original w3[col-1] */
        }
        vscale_array += cstart;
      }
      if (ctype == IS_COLORING_GLOBAL) w3_array += cstart;
      ierr = VecRestoreArray(w3,&w3_array);CHKERRQ(ierr);

      /*
       (3-2) Evaluate function at w3 = x1 + dx (here dx is a vector of perturbations)
                           w2 = F(x1 + dx) - F(x1)
       */
      ierr = PetscLogEventBegin(MAT_FDColoringFunction,0,0,0,0);CHKERRQ(ierr);
      ierr = VecPlaceArray(w2,dy_i);CHKERRQ(ierr); /* place w2 to the array dy_i */
      ierr = (*f)(sctx,w3,w2,fctx);CHKERRQ(ierr);
      ierr = PetscLogEventEnd(MAT_FDColoringFunction,0,0,0,0);CHKERRQ(ierr);
      ierr = VecAXPY(w2,-1.0,w1);CHKERRQ(ierr);
      ierr = VecResetArray(w2);CHKERRQ(ierr);
      dy_i += nxloc; /* points to dy+i*nxloc */
    }

    /*
     (3-3) Loop over rows of vector, putting results into Jacobian matrix
    */
    nrows_k = nrows[k];
    if (coloring->htype[0] == 'w') {
      for (l=0; l<nrows_k; l++) {
        row     = bs*Jentry2[nz].row;   /* local row index */
        valaddr = Jentry2[nz++].valaddr;
        spidx   = 0;
        dy_i    = dy;
        for (i=0; i<bs; i++) {   /* column of the block */
          for (j=0; j<bs; j++) { /* row of the block */
            valaddr[spidx++] = dy_i[row+j]*dx;
          }
          dy_i += nxloc; /* points to dy+i*nxloc */
        }
      }
    } else { /* htype == 'ds' */
      for (l=0; l<nrows_k; l++) {
        row     = bs*Jentry[nz].row;   /* local row index */
        col     = bs*Jentry[nz].col;   /* local column index */
        valaddr = Jentry[nz++].valaddr;
        spidx   = 0;
        dy_i    = dy;
        for (i=0; i<bs; i++) {   /* column of the block */
          for (j=0; j<bs; j++) { /* row of the block */
            valaddr[spidx++] = dy_i[row+j]*vscale_array[col+i];
          }
          dy_i += nxloc; /* points to dy+i*nxloc */
        }
      }
    }
  }
  ierr = MatAssemblyBegin(J,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(J,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  if (vscale) {
    ierr = VecRestoreArray(vscale,&vscale_array);CHKERRQ(ierr);
  }

  coloring->currentcolor = -1;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatFDColoringApply_AIJ"
PetscErrorCode  MatFDColoringApply_AIJ(Mat J,MatFDColoring coloring,Vec x1,void *sctx)
{
  PetscErrorCode    (*f)(void*,Vec,Vec,void*) = (PetscErrorCode (*)(void*,Vec,Vec,void*))coloring->f;
  PetscErrorCode    ierr;
  PetscInt          k,cstart,cend,l,row,col,nz;
  PetscScalar       dx=0.0,*y,*w3_array;
  const PetscScalar *xx;
  PetscScalar       *vscale_array;
  PetscReal         epsilon=coloring->error_rel,umin=coloring->umin,unorm;
  Vec               w1=coloring->w1,w2=coloring->w2,w3,vscale=coloring->vscale;
  void              *fctx=coloring->fctx;
  PetscInt          ctype=coloring->ctype,nxloc,nrows_k;
  MatEntry          *Jentry=coloring->matentry;
  MatEntry2         *Jentry2=coloring->matentry2;
  const PetscInt    ncolors=coloring->ncolors,*ncolumns=coloring->ncolumns,*nrows=coloring->nrows;

  PetscFunctionBegin;
  /* (1) Set w1 = F(x1) */
  if (!coloring->fset) {
    ierr = PetscLogEventBegin(MAT_FDColoringFunction,0,0,0,0);CHKERRQ(ierr);
    ierr = (*f)(sctx,x1,w1,fctx);CHKERRQ(ierr);
    ierr = PetscLogEventEnd(MAT_FDColoringFunction,0,0,0,0);CHKERRQ(ierr);
  } else {
    coloring->fset = PETSC_FALSE;
  }

  /* (2) Compute vscale = 1./dx - the local scale factors, including ghost points */
  if (coloring->htype[0] == 'w') {
    /* vscale = 1./dx is a constant scalar */
    ierr = VecNorm(x1,NORM_2,&unorm);CHKERRQ(ierr);
    dx = 1.0/(PetscSqrtReal(1.0 + unorm)*epsilon);
  } else {
    ierr = VecGetLocalSize(x1,&nxloc);CHKERRQ(ierr);
    ierr = VecGetArrayRead(x1,&xx);CHKERRQ(ierr);
    ierr = VecGetArray(vscale,&vscale_array);CHKERRQ(ierr);
    for (col=0; col<nxloc; col++) {
      dx = xx[col];
      if (PetscAbsScalar(dx) < umin) {
        if (PetscRealPart(dx) >= 0.0)      dx = umin;
        else if (PetscRealPart(dx) < 0.0 ) dx = -umin;
      }
      dx               *= epsilon;
      vscale_array[col] = 1.0/dx;
    }
    ierr = VecRestoreArrayRead(x1,&xx);CHKERRQ(ierr);
    ierr = VecRestoreArray(vscale,&vscale_array);CHKERRQ(ierr);
  }
  if (ctype == IS_COLORING_GLOBAL && coloring->htype[0] == 'd') {
    ierr = VecGhostUpdateBegin(vscale,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
    ierr = VecGhostUpdateEnd(vscale,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
  }

  /* (3) Loop over each color */
  if (!coloring->w3) {
    ierr = VecDuplicate(x1,&coloring->w3);CHKERRQ(ierr);
    ierr = PetscLogObjectParent((PetscObject)coloring,(PetscObject)coloring->w3);CHKERRQ(ierr);
  }
  w3 = coloring->w3;

  ierr = VecGetOwnershipRange(x1,&cstart,&cend);CHKERRQ(ierr); /* used by ghosted vscale */
  if (vscale) {
    ierr = VecGetArray(vscale,&vscale_array);CHKERRQ(ierr);
  }
  nz   = 0;

  if (coloring->bcols > 1) { /* use blocked insertion of Jentry */
    PetscInt    i,m=J->rmap->n,nbcols,bcols=coloring->bcols;
    PetscScalar *dy=coloring->dy,*dy_k;

    nbcols = 0;
    for (k=0; k<ncolors; k+=bcols) {
      coloring->currentcolor = k;

      /*
       (3-1) Loop over each column associated with color
       adding the perturbation to the vector w3 = x1 + dx.
       */

      dy_k = dy;
      if (k + bcols > ncolors) bcols = ncolors - k;
      for (i=0; i<bcols; i++) {

        ierr = VecCopy(x1,w3);CHKERRQ(ierr);
        ierr = VecGetArray(w3,&w3_array);CHKERRQ(ierr);
        if (ctype == IS_COLORING_GLOBAL) w3_array -= cstart; /* shift pointer so global index can be used */
        if (coloring->htype[0] == 'w') {
          for (l=0; l<ncolumns[k+i]; l++) {
            col = coloring->columns[k+i][l]; /* local column (in global index!) of the matrix we are probing for */
            w3_array[col] += 1.0/dx;
          }
        } else { /* htype == 'ds' */
          vscale_array -= cstart; /* shift pointer so global index can be used */
          for (l=0; l<ncolumns[k+i]; l++) {
            col = coloring->columns[k+i][l]; /* local column (in global index!) of the matrix we are probing for */
            w3_array[col] += 1.0/vscale_array[col];
          }
          vscale_array += cstart;
        }
        if (ctype == IS_COLORING_GLOBAL) w3_array += cstart;
        ierr = VecRestoreArray(w3,&w3_array);CHKERRQ(ierr);

        /*
         (3-2) Evaluate function at w3 = x1 + dx (here dx is a vector of perturbations)
                           w2 = F(x1 + dx) - F(x1)
         */
        ierr = PetscLogEventBegin(MAT_FDColoringFunction,0,0,0,0);CHKERRQ(ierr);
        ierr = VecPlaceArray(w2,dy_k);CHKERRQ(ierr); /* place w2 to the array dy_i */
        ierr = (*f)(sctx,w3,w2,fctx);CHKERRQ(ierr);
        ierr = PetscLogEventEnd(MAT_FDColoringFunction,0,0,0,0);CHKERRQ(ierr);
        ierr = VecAXPY(w2,-1.0,w1);CHKERRQ(ierr);
        ierr = VecResetArray(w2);CHKERRQ(ierr);
        dy_k += m; /* points to dy+i*nxloc */
      }

      /*
       (3-3) Loop over block rows of vector, putting results into Jacobian matrix
       */
      nrows_k = nrows[nbcols++];
      ierr = VecGetArray(w2,&y);CHKERRQ(ierr);

      if (coloring->htype[0] == 'w') {
        for (l=0; l<nrows_k; l++) {
          row                      = Jentry2[nz].row;   /* local row index */
          *(Jentry2[nz++].valaddr) = dy[row]*dx;
        }
      } else { /* htype == 'ds' */
        for (l=0; l<nrows_k; l++) {
          row                   = Jentry[nz].row;   /* local row index */
          *(Jentry[nz].valaddr) = dy[row]*vscale_array[Jentry[nz].col];
          nz++;
        }
      }
      ierr = VecRestoreArray(w2,&y);CHKERRQ(ierr);
    }
  } else { /* bcols == 1 */
    for (k=0; k<ncolors; k++) {
      coloring->currentcolor = k;

      /*
       (3-1) Loop over each column associated with color
       adding the perturbation to the vector w3 = x1 + dx.
       */
      ierr = VecCopy(x1,w3);CHKERRQ(ierr);
      ierr = VecGetArray(w3,&w3_array);CHKERRQ(ierr);
      if (ctype == IS_COLORING_GLOBAL) w3_array -= cstart; /* shift pointer so global index can be used */
      if (coloring->htype[0] == 'w') {
        for (l=0; l<ncolumns[k]; l++) {
          col = coloring->columns[k][l]; /* local column (in global index!) of the matrix we are probing for */
          w3_array[col] += 1.0/dx;
        }
      } else { /* htype == 'ds' */
        vscale_array -= cstart; /* shift pointer so global index can be used */
        for (l=0; l<ncolumns[k]; l++) {
          col = coloring->columns[k][l]; /* local column (in global index!) of the matrix we are probing for */
          w3_array[col] += 1.0/vscale_array[col];
        }
        vscale_array += cstart;
      }
      if (ctype == IS_COLORING_GLOBAL) w3_array += cstart;
      ierr = VecRestoreArray(w3,&w3_array);CHKERRQ(ierr);

      /*
       (3-2) Evaluate function at w3 = x1 + dx (here dx is a vector of perturbations)
                           w2 = F(x1 + dx) - F(x1)
       */
      ierr = PetscLogEventBegin(MAT_FDColoringFunction,0,0,0,0);CHKERRQ(ierr);
      ierr = (*f)(sctx,w3,w2,fctx);CHKERRQ(ierr);
      ierr = PetscLogEventEnd(MAT_FDColoringFunction,0,0,0,0);CHKERRQ(ierr);
      ierr = VecAXPY(w2,-1.0,w1);CHKERRQ(ierr);

      /*
       (3-3) Loop over rows of vector, putting results into Jacobian matrix
       */
      nrows_k = nrows[k];
      ierr = VecGetArray(w2,&y);CHKERRQ(ierr);
      if (coloring->htype[0] == 'w') {
        for (l=0; l<nrows_k; l++) {
          row                      = Jentry2[nz].row;   /* local row index */
          *(Jentry2[nz++].valaddr) = y[row]*dx;
        }
      } else { /* htype == 'ds' */
        for (l=0; l<nrows_k; l++) {
          row                   = Jentry[nz].row;   /* local row index */
          *(Jentry[nz].valaddr) = y[row]*vscale_array[Jentry[nz].col];
          nz++;
        }
      }
      ierr = VecRestoreArray(w2,&y);CHKERRQ(ierr);
    }
  }

  ierr = MatAssemblyBegin(J,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(J,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  if (vscale) {
    ierr = VecRestoreArray(vscale,&vscale_array);CHKERRQ(ierr);
  }
  coloring->currentcolor = -1;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatFDColoringSetUp_MPIXAIJ"
PetscErrorCode MatFDColoringSetUp_MPIXAIJ(Mat mat,ISColoring iscoloring,MatFDColoring c)
{
  PetscErrorCode         ierr;
  PetscMPIInt            size,*ncolsonproc,*disp,nn;
  PetscInt               i,n,nrows,nrows_i,j,k,m,ncols,col,*rowhit,cstart,cend,colb;
  const PetscInt         *is,*A_ci,*A_cj,*B_ci,*B_cj,*row=NULL,*ltog=NULL;
  PetscInt               nis=iscoloring->n,nctot,*cols;
  IS                     *isa;
  ISLocalToGlobalMapping map=mat->cmap->mapping;
  PetscInt               ctype=c->ctype,*spidxA,*spidxB,nz,bs,bs2,spidx;
  Mat                    A,B;
  PetscScalar            *A_val,*B_val,**valaddrhit;
  MatEntry               *Jentry;
  MatEntry2              *Jentry2;
  PetscBool              isBAIJ;
  PetscInt               bcols=c->bcols;
#if defined(PETSC_USE_CTABLE)
  PetscTable             colmap=NULL;
#else
  PetscInt               *colmap=NULL;     /* local col number of off-diag col */
#endif

  PetscFunctionBegin;
  if (ctype == IS_COLORING_GHOSTED) {
    if (!map) SETERRQ(PetscObjectComm((PetscObject)mat),PETSC_ERR_ARG_INCOMP,"When using ghosted differencing matrix must have local to global mapping provided with MatSetLocalToGlobalMapping");
    ierr = ISLocalToGlobalMappingGetIndices(map,&ltog);CHKERRQ(ierr);
  }

  ierr = MatGetBlockSize(mat,&bs);CHKERRQ(ierr);
  ierr = PetscObjectTypeCompare((PetscObject)mat,MATMPIBAIJ,&isBAIJ);CHKERRQ(ierr);
  if (isBAIJ) {
    Mat_MPIBAIJ *baij=(Mat_MPIBAIJ*)mat->data;
    Mat_SeqBAIJ *spA,*spB;
    A = baij->A;  spA = (Mat_SeqBAIJ*)A->data; A_val = spA->a;
    B = baij->B;  spB = (Mat_SeqBAIJ*)B->data; B_val = spB->a;
    nz = spA->nz + spB->nz; /* total nonzero entries of mat */
    if (!baij->colmap) {
      ierr = MatCreateColmap_MPIBAIJ_Private(mat);CHKERRQ(ierr);
      colmap = baij->colmap;
    }
    ierr = MatGetColumnIJ_SeqBAIJ_Color(A,0,PETSC_FALSE,PETSC_FALSE,&ncols,&A_ci,&A_cj,&spidxA,NULL);CHKERRQ(ierr);
    ierr = MatGetColumnIJ_SeqBAIJ_Color(B,0,PETSC_FALSE,PETSC_FALSE,&ncols,&B_ci,&B_cj,&spidxB,NULL);CHKERRQ(ierr);

    if (ctype == IS_COLORING_GLOBAL && c->htype[0] == 'd') {  /* create vscale for storing dx */
      PetscInt    *garray;
      ierr = PetscMalloc1(B->cmap->n,&garray);CHKERRQ(ierr);
      for (i=0; i<baij->B->cmap->n/bs; i++) {
        for (j=0; j<bs; j++) {
          garray[i*bs+j] = bs*baij->garray[i]+j;
        }
      }
      ierr = VecCreateGhost(PetscObjectComm((PetscObject)mat),mat->cmap->n,PETSC_DETERMINE,B->cmap->n,garray,&c->vscale);CHKERRQ(ierr);
      ierr = PetscFree(garray);CHKERRQ(ierr);
    }
  } else {
    Mat_MPIAIJ *aij=(Mat_MPIAIJ*)mat->data;
    Mat_SeqAIJ *spA,*spB;
    A = aij->A;  spA = (Mat_SeqAIJ*)A->data; A_val = spA->a;
    B = aij->B;  spB = (Mat_SeqAIJ*)B->data; B_val = spB->a;
    nz = spA->nz + spB->nz; /* total nonzero entries of mat */
    if (!aij->colmap) {
      /* Allow access to data structures of local part of matrix
       - creates aij->colmap which maps global column number to local number in part B */
      ierr = MatCreateColmap_MPIAIJ_Private(mat);CHKERRQ(ierr);
      colmap = aij->colmap;
    }
    ierr = MatGetColumnIJ_SeqAIJ_Color(A,0,PETSC_FALSE,PETSC_FALSE,&ncols,&A_ci,&A_cj,&spidxA,NULL);CHKERRQ(ierr);
    ierr = MatGetColumnIJ_SeqAIJ_Color(B,0,PETSC_FALSE,PETSC_FALSE,&ncols,&B_ci,&B_cj,&spidxB,NULL);CHKERRQ(ierr);

    bs = 1; /* only bs=1 is supported for non MPIBAIJ matrix */

    if (ctype == IS_COLORING_GLOBAL && c->htype[0] == 'd') { /* create vscale for storing dx */
      ierr = VecCreateGhost(PetscObjectComm((PetscObject)mat),mat->cmap->n,PETSC_DETERMINE,B->cmap->n,aij->garray,&c->vscale);CHKERRQ(ierr);
    }
  }

  m         = mat->rmap->n/bs;
  cstart    = mat->cmap->rstart/bs;
  cend      = mat->cmap->rend/bs;

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

  ierr = PetscMalloc2(m+1,&rowhit,m+1,&valaddrhit);CHKERRQ(ierr);
  nz = 0;
  ierr = ISColoringGetIS(iscoloring,PETSC_IGNORE,&isa);CHKERRQ(ierr);
  for (i=0; i<nis; i++) { /* for each local color */
    ierr = ISGetLocalSize(isa[i],&n);CHKERRQ(ierr);
    ierr = ISGetIndices(isa[i],&is);CHKERRQ(ierr);

    c->ncolumns[i] = n; /* local number of columns of this color on this process */
    if (n) {
      ierr = PetscMalloc1(n,&c->columns[i]);CHKERRQ(ierr);
      ierr = PetscLogObjectMemory((PetscObject)c,n*sizeof(PetscInt));CHKERRQ(ierr);
      ierr = PetscMemcpy(c->columns[i],is,n*sizeof(PetscInt));CHKERRQ(ierr);
    } else {
      c->columns[i] = 0;
    }

    if (ctype == IS_COLORING_GLOBAL) {
      /* Determine nctot, the total (parallel) number of columns of this color */
      ierr = MPI_Comm_size(PetscObjectComm((PetscObject)mat),&size);CHKERRQ(ierr);
      ierr = PetscMalloc2(size,&ncolsonproc,size,&disp);CHKERRQ(ierr);

      /* ncolsonproc[j]: local ncolumns on proc[j] of this color */
      ierr  = PetscMPIIntCast(n,&nn);CHKERRQ(ierr);
      ierr  = MPI_Allgather(&nn,1,MPI_INT,ncolsonproc,1,MPI_INT,PetscObjectComm((PetscObject)mat));CHKERRQ(ierr);
      nctot = 0; for (j=0; j<size; j++) nctot += ncolsonproc[j];
      if (!nctot) {
        ierr = PetscInfo(mat,"Coloring of matrix has some unneeded colors with no corresponding rows\n");CHKERRQ(ierr);
      }

      disp[0] = 0;
      for (j=1; j<size; j++) {
        disp[j] = disp[j-1] + ncolsonproc[j-1];
      }

      /* Get cols, the complete list of columns for this color on each process */
      ierr = PetscMalloc1(nctot+1,&cols);CHKERRQ(ierr);
      ierr = MPI_Allgatherv((void*)is,n,MPIU_INT,cols,ncolsonproc,disp,MPIU_INT,PetscObjectComm((PetscObject)mat));CHKERRQ(ierr);
      ierr = PetscFree2(ncolsonproc,disp);CHKERRQ(ierr);
    } else if (ctype == IS_COLORING_GHOSTED) {
      /* Determine local number of columns of this color on this process, including ghost points */
      nctot = n;
      ierr  = PetscMalloc1(nctot+1,&cols);CHKERRQ(ierr);
      ierr  = PetscMemcpy(cols,is,n*sizeof(PetscInt));CHKERRQ(ierr);
    } else SETERRQ(PETSC_COMM_SELF,PETSC_ERR_SUP,"Not provided for this MatFDColoring type");

    /* Mark all rows affect by these columns */
    ierr    = PetscMemzero(rowhit,m*sizeof(PetscInt));CHKERRQ(ierr);
    bs2     = bs*bs;
    nrows_i = 0;
    for (j=0; j<nctot; j++) { /* loop over columns*/
      if (ctype == IS_COLORING_GHOSTED) {
        col = ltog[cols[j]];
      } else {
        col = cols[j];
      }
      if (col >= cstart && col < cend) { /* column is in A, diagonal block of mat */
        row      = A_cj + A_ci[col-cstart];
        nrows    = A_ci[col-cstart+1] - A_ci[col-cstart];
        nrows_i += nrows;
        /* loop over columns of A marking them in rowhit */
        for (k=0; k<nrows; k++) {
          /* set valaddrhit for part A */
          spidx            = bs2*spidxA[A_ci[col-cstart] + k];
          valaddrhit[*row] = &A_val[spidx];
          rowhit[*row++]   = col - cstart + 1; /* local column index */
        }
      } else { /* column is in B, off-diagonal block of mat */
#if defined(PETSC_USE_CTABLE)
        ierr = PetscTableFind(colmap,col+1,&colb);CHKERRQ(ierr);
        colb--;
#else
        colb = colmap[col] - 1; /* local column index */
#endif
        if (colb == -1) {
          nrows = 0;
        } else {
          colb  = colb/bs;
          row   = B_cj + B_ci[colb];
          nrows = B_ci[colb+1] - B_ci[colb];
        }
        nrows_i += nrows;
        /* loop over columns of B marking them in rowhit */
        for (k=0; k<nrows; k++) {
          /* set valaddrhit for part B */
          spidx            = bs2*spidxB[B_ci[colb] + k];
          valaddrhit[*row] = &B_val[spidx];
          rowhit[*row++]   = colb + 1 + cend - cstart; /* local column index */
        }
      }
    }
    c->nrows[i] = nrows_i;

    if (c->htype[0] == 'd') {
      for (j=0; j<m; j++) {
        if (rowhit[j]) {
          Jentry[nz].row     = j;              /* local row index */
          Jentry[nz].col     = rowhit[j] - 1;  /* local column index */
          Jentry[nz].valaddr = valaddrhit[j];  /* address of mat value for this entry */
          nz++;
        }
      }
    } else { /* c->htype == 'wp' */
      for (j=0; j<m; j++) {
        if (rowhit[j]) {
          Jentry2[nz].row     = j;              /* local row index */
          Jentry2[nz].valaddr = valaddrhit[j];  /* address of mat value for this entry */
          nz++;
        }
      }
    }
    ierr = PetscFree(cols);CHKERRQ(ierr);
  }

  if (bcols > 1) { /* reorder Jentry for faster MatFDColoringApply() */
    ierr = MatFDColoringSetUpBlocked_AIJ_Private(mat,c,nz);CHKERRQ(ierr);
  }

  if (isBAIJ) {
    ierr = MatRestoreColumnIJ_SeqBAIJ_Color(A,0,PETSC_FALSE,PETSC_FALSE,&ncols,&A_ci,&A_cj,&spidxA,NULL);CHKERRQ(ierr);
    ierr = MatRestoreColumnIJ_SeqBAIJ_Color(B,0,PETSC_FALSE,PETSC_FALSE,&ncols,&B_ci,&B_cj,&spidxB,NULL);CHKERRQ(ierr);
    ierr = PetscMalloc1(bs*mat->rmap->n,&c->dy);CHKERRQ(ierr);
  } else {
    ierr = MatRestoreColumnIJ_SeqAIJ_Color(A,0,PETSC_FALSE,PETSC_FALSE,&ncols,&A_ci,&A_cj,&spidxA,NULL);CHKERRQ(ierr);
    ierr = MatRestoreColumnIJ_SeqAIJ_Color(B,0,PETSC_FALSE,PETSC_FALSE,&ncols,&B_ci,&B_cj,&spidxB,NULL);CHKERRQ(ierr);
  }

  ierr = ISColoringRestoreIS(iscoloring,&isa);CHKERRQ(ierr);
  ierr = PetscFree2(rowhit,valaddrhit);CHKERRQ(ierr);

  if (ctype == IS_COLORING_GHOSTED) {
    ierr = ISLocalToGlobalMappingRestoreIndices(map,&ltog);CHKERRQ(ierr);
  }
  ierr = PetscInfo3(c,"ncolors %D, brows %D and bcols %D are used.\n",c->ncolors,c->brows,c->bcols);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatFDColoringCreate_MPIXAIJ"
PetscErrorCode MatFDColoringCreate_MPIXAIJ(Mat mat,ISColoring iscoloring,MatFDColoring c)
{
  PetscErrorCode ierr;
  PetscInt       bs,nis=iscoloring->n,m=mat->rmap->n;
  PetscBool      isBAIJ;

  PetscFunctionBegin;
  /* set default brows and bcols for speedup inserting the dense matrix into sparse Jacobian;
   bcols is chosen s.t. dy-array takes 50% of memory space as mat */
  ierr = MatGetBlockSize(mat,&bs);CHKERRQ(ierr);
  ierr = PetscObjectTypeCompare((PetscObject)mat,MATMPIBAIJ,&isBAIJ);CHKERRQ(ierr);
  if (isBAIJ || m == 0) {
    c->brows = m;
    c->bcols = 1;
  } else { /* mpiaij matrix */
    /* bcols is chosen s.t. dy-array takes 50% of local memory space as mat */
    Mat_MPIAIJ *aij=(Mat_MPIAIJ*)mat->data;
    Mat_SeqAIJ *spA,*spB;
    Mat        A,B;
    PetscInt   nz,brows,bcols;
    PetscReal  mem;

    bs    = 1; /* only bs=1 is supported for MPIAIJ matrix */

    A = aij->A;  spA = (Mat_SeqAIJ*)A->data;
    B = aij->B;  spB = (Mat_SeqAIJ*)B->data;
    nz = spA->nz + spB->nz; /* total local nonzero entries of mat */
    mem = nz*(sizeof(PetscScalar) + sizeof(PetscInt)) + 3*m*sizeof(PetscInt);
    bcols = (PetscInt)(0.5*mem /(m*sizeof(PetscScalar)));
    brows = 1000/bcols;
    if (bcols > nis) bcols = nis;
    if (brows == 0 || brows > m) brows = m;
    c->brows = brows;
    c->bcols = bcols;
  }

  c->M       = mat->rmap->N/bs;         /* set the global rows and columns and local rows */
  c->N       = mat->cmap->N/bs;
  c->m       = mat->rmap->n/bs;
  c->rstart  = mat->rmap->rstart/bs;
  c->ncolors = nis;
  PetscFunctionReturn(0);
}
