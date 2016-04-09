
/*
    Creates hypre ijmatrix from PETSc matrix
*/
#include <petscsys.h>
#include <petsc/private/matimpl.h>
#include <petscdmda.h>                /*I "petscdmda.h" I*/
#include <../src/dm/impls/da/hypre/mhyp.h>

#undef __FUNCT__
#define __FUNCT__ "MatHYPRE_IJMatrixPreallocate"
PetscErrorCode MatHYPRE_IJMatrixPreallocate(Mat A_d, Mat A_o,HYPRE_IJMatrix ij)
{
  PetscErrorCode ierr;
  PetscInt       i,n_d,n_o;
  const PetscInt *ia_d,*ia_o;
  PetscBool      done_d=PETSC_FALSE,done_o=PETSC_FALSE;
  PetscInt       *nnz_d=NULL,*nnz_o=NULL;

  PetscFunctionBegin;
  if (A_d) { /* determine number of nonzero entries in local diagonal part */
    ierr = MatGetRowIJ(A_d,0,PETSC_FALSE,PETSC_FALSE,&n_d,&ia_d,NULL,&done_d);CHKERRQ(ierr);
    if (done_d) {
      ierr = PetscMalloc1(n_d,&nnz_d);CHKERRQ(ierr);
      for (i=0; i<n_d; i++) {
        nnz_d[i] = ia_d[i+1] - ia_d[i];
      }
    }
    ierr = MatRestoreRowIJ(A_d,0,PETSC_FALSE,PETSC_FALSE,NULL,&ia_d,NULL,&done_d);CHKERRQ(ierr);
  }
  if (A_o) { /* determine number of nonzero entries in local off-diagonal part */
    ierr = MatGetRowIJ(A_o,0,PETSC_FALSE,PETSC_FALSE,&n_o,&ia_o,NULL,&done_o);CHKERRQ(ierr);
    if (done_o) {
      ierr = PetscMalloc1(n_o,&nnz_o);CHKERRQ(ierr);
      for (i=0; i<n_o; i++) {
        nnz_o[i] = ia_o[i+1] - ia_o[i];
      }
    }
    ierr = MatRestoreRowIJ(A_o,0,PETSC_FALSE,PETSC_FALSE,&n_o,&ia_o,NULL,&done_o);CHKERRQ(ierr);
  }
  if (done_d) {    /* set number of nonzeros in HYPRE IJ matrix */
    if (!done_o) { /* only diagonal part */
      ierr = PetscMalloc1(n_d,&nnz_o);CHKERRQ(ierr);
      for (i=0; i<n_d; i++) {
        nnz_o[i] = 0;
      }
    }
    PetscStackCallStandard(HYPRE_IJMatrixSetDiagOffdSizes,(ij,(HYPRE_Int *)nnz_d,(HYPRE_Int *)nnz_o));
    ierr = PetscFree(nnz_d);CHKERRQ(ierr);
    ierr = PetscFree(nnz_o);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatHYPRE_IJMatrixCreate"
PetscErrorCode MatHYPRE_IJMatrixCreate(Mat A,HYPRE_IJMatrix *ij)
{
  PetscErrorCode ierr;
  PetscInt       rstart,rend,cstart,cend;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(A,MAT_CLASSID,1);
  PetscValidType(A,1);
  PetscValidPointer(ij,2);
  ierr   = MatSetUp(A);CHKERRQ(ierr);
  rstart = A->rmap->rstart;
  rend   = A->rmap->rend;
  cstart = A->cmap->rstart;
  cend   = A->cmap->rend;
  PetscStackCallStandard(HYPRE_IJMatrixCreate,(PetscObjectComm((PetscObject)A),rstart,rend-1,cstart,cend-1,ij));
  PetscStackCallStandard(HYPRE_IJMatrixSetObjectType,(*ij,HYPRE_PARCSR));
  {
    PetscBool      same;
    Mat            A_d,A_o;
    const PetscInt *colmap;
    ierr = PetscObjectTypeCompare((PetscObject)A,MATMPIAIJ,&same);CHKERRQ(ierr);
    if (same) {
      ierr = MatMPIAIJGetSeqAIJ(A,&A_d,&A_o,&colmap);CHKERRQ(ierr);
      ierr = MatHYPRE_IJMatrixPreallocate(A_d,A_o,*ij);CHKERRQ(ierr);
      PetscFunctionReturn(0);
    }
    ierr = PetscObjectTypeCompare((PetscObject)A,MATMPIBAIJ,&same);CHKERRQ(ierr);
    if (same) {
      ierr = MatMPIBAIJGetSeqBAIJ(A,&A_d,&A_o,&colmap);CHKERRQ(ierr);
      ierr = MatHYPRE_IJMatrixPreallocate(A_d,A_o,*ij);CHKERRQ(ierr);
      PetscFunctionReturn(0);
    }
    ierr = PetscObjectTypeCompare((PetscObject)A,MATSEQAIJ,&same);CHKERRQ(ierr);
    if (same) {
      ierr = MatHYPRE_IJMatrixPreallocate(A,NULL,*ij);CHKERRQ(ierr);
      PetscFunctionReturn(0);
    }
    ierr = PetscObjectTypeCompare((PetscObject)A,MATSEQBAIJ,&same);CHKERRQ(ierr);
    if (same) {
      ierr = MatHYPRE_IJMatrixPreallocate(A,NULL,*ij);CHKERRQ(ierr);
      PetscFunctionReturn(0);
    }
  }
  PetscFunctionReturn(0);
}

extern PetscErrorCode MatHYPRE_IJMatrixFastCopy_MPIAIJ(Mat,HYPRE_IJMatrix);
extern PetscErrorCode MatHYPRE_IJMatrixFastCopy_SeqAIJ(Mat,HYPRE_IJMatrix);
/*
    Copies the data over (column indices, numerical values) to hypre matrix
*/

#undef __FUNCT__
#define __FUNCT__ "MatHYPRE_IJMatrixCopy"
PetscErrorCode MatHYPRE_IJMatrixCopy(Mat A,HYPRE_IJMatrix ij)
{
  PetscErrorCode    ierr;
  PetscInt          i,rstart,rend,ncols,nr,nc;
  const PetscScalar *values;
  const PetscInt    *cols;
  PetscBool         flg;

  PetscFunctionBegin;
  ierr = PetscObjectTypeCompare((PetscObject)A,MATMPIAIJ,&flg);CHKERRQ(ierr);
  ierr = MatGetSize(A,&nr,&nc);CHKERRQ(ierr);
  if (flg && nr == nc) {
    ierr = MatHYPRE_IJMatrixFastCopy_MPIAIJ(A,ij);CHKERRQ(ierr);
    PetscFunctionReturn(0);
  }
  ierr = PetscObjectTypeCompare((PetscObject)A,MATSEQAIJ,&flg);CHKERRQ(ierr);
  if (flg) {
    ierr = MatHYPRE_IJMatrixFastCopy_SeqAIJ(A,ij);CHKERRQ(ierr);
    PetscFunctionReturn(0);
  }

  ierr = PetscLogEventBegin(MAT_Convert,A,0,0,0);CHKERRQ(ierr);
  PetscStackCallStandard(HYPRE_IJMatrixInitialize,(ij));
  ierr = MatGetOwnershipRange(A,&rstart,&rend);CHKERRQ(ierr);
  for (i=rstart; i<rend; i++) {
    ierr = MatGetRow(A,i,&ncols,&cols,&values);CHKERRQ(ierr);
    PetscStackCallStandard(HYPRE_IJMatrixSetValues,(ij,1,(HYPRE_Int *)&ncols,(HYPRE_Int *)&i,(HYPRE_Int *)cols,values));
    ierr = MatRestoreRow(A,i,&ncols,&cols,&values);CHKERRQ(ierr);
  }
  PetscStackCallStandard(HYPRE_IJMatrixAssemble,(ij));
  ierr = PetscLogEventEnd(MAT_Convert,A,0,0,0);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/*
    This copies the CSR format directly from the PETSc data structure to the hypre
    data structure without calls to MatGetRow() or hypre's set values.

*/
#include <_hypre_IJ_mv.h>
#include <HYPRE_IJ_mv.h>
#include <../src/mat/impls/aij/mpi/mpiaij.h>

#undef __FUNCT__
#define __FUNCT__ "MatHYPRE_IJMatrixFastCopy_SeqAIJ"
PetscErrorCode MatHYPRE_IJMatrixFastCopy_SeqAIJ(Mat A,HYPRE_IJMatrix ij)
{
  PetscErrorCode ierr;
  Mat_SeqAIJ     *pdiag = (Mat_SeqAIJ*)A->data;

  hypre_ParCSRMatrix    *par_matrix;
  hypre_AuxParCSRMatrix *aux_matrix;
  hypre_CSRMatrix       *hdiag;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(A,MAT_CLASSID,1);
  PetscValidType(A,1);
  PetscValidPointer(ij,2);

  ierr = PetscLogEventBegin(MAT_Convert,A,0,0,0);CHKERRQ(ierr);
  PetscStackCallStandard(HYPRE_IJMatrixInitialize,(ij));
  par_matrix = (hypre_ParCSRMatrix*)hypre_IJMatrixObject(ij);
  aux_matrix = (hypre_AuxParCSRMatrix*)hypre_IJMatrixTranslator(ij);
  hdiag      = hypre_ParCSRMatrixDiag(par_matrix);

  /*
       this is the Hack part where we monkey directly with the hypre datastructures
  */
  ierr = PetscMemcpy(hdiag->i,pdiag->i,(A->rmap->n + 1)*sizeof(PetscInt));
  ierr = PetscMemcpy(hdiag->j,pdiag->j,pdiag->nz*sizeof(PetscInt));
  ierr = PetscMemcpy(hdiag->data,pdiag->a,pdiag->nz*sizeof(PetscScalar));

  hypre_AuxParCSRMatrixNeedAux(aux_matrix) = 0;
  PetscStackCallStandard(HYPRE_IJMatrixAssemble,(ij));
  ierr = PetscLogEventEnd(MAT_Convert,A,0,0,0);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatHYPRE_IJMatrixFastCopy_MPIAIJ"
PetscErrorCode MatHYPRE_IJMatrixFastCopy_MPIAIJ(Mat A,HYPRE_IJMatrix ij)
{
  PetscErrorCode ierr;
  Mat_MPIAIJ     *pA = (Mat_MPIAIJ*)A->data;
  Mat_SeqAIJ     *pdiag,*poffd;
  PetscInt       i,*garray = pA->garray,*jj,cstart,*pjj;

  hypre_ParCSRMatrix    *par_matrix;
  hypre_AuxParCSRMatrix *aux_matrix;
  hypre_CSRMatrix       *hdiag,*hoffd;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(A,MAT_CLASSID,1);
  PetscValidType(A,1);
  PetscValidPointer(ij,2);
  pdiag = (Mat_SeqAIJ*) pA->A->data;
  poffd = (Mat_SeqAIJ*) pA->B->data;
  /* cstart is only valid for square MPIAIJ layed out in the usual way */
  ierr = MatGetOwnershipRange(A,&cstart,NULL);CHKERRQ(ierr);

  ierr = PetscLogEventBegin(MAT_Convert,A,0,0,0);CHKERRQ(ierr);
  PetscStackCallStandard(HYPRE_IJMatrixInitialize,(ij));
  par_matrix = (hypre_ParCSRMatrix*)hypre_IJMatrixObject(ij);
  aux_matrix = (hypre_AuxParCSRMatrix*)hypre_IJMatrixTranslator(ij);
  hdiag      = hypre_ParCSRMatrixDiag(par_matrix);
  hoffd      = hypre_ParCSRMatrixOffd(par_matrix);

  /*
       this is the Hack part where we monkey directly with the hypre datastructures
  */
  ierr = PetscMemcpy(hdiag->i,pdiag->i,(pA->A->rmap->n + 1)*sizeof(PetscInt));
  /* need to shift the diag column indices (hdiag->j) back to global numbering since hypre is expecting this */
  jj  = (PetscInt*)hdiag->j;
  pjj = (PetscInt*)pdiag->j;
  for (i=0; i<pdiag->nz; i++) jj[i] = cstart + pjj[i];
  ierr = PetscMemcpy(hdiag->data,pdiag->a,pdiag->nz*sizeof(PetscScalar));
  ierr = PetscMemcpy(hoffd->i,poffd->i,(pA->A->rmap->n + 1)*sizeof(PetscInt));
  /* need to move the offd column indices (hoffd->j) back to global numbering since hypre is expecting this
     If we hacked a hypre a bit more we might be able to avoid this step */
  jj  = (PetscInt*) hoffd->j;
  pjj = (PetscInt*) poffd->j;
  for (i=0; i<poffd->nz; i++) jj[i] = garray[pjj[i]];
  ierr = PetscMemcpy(hoffd->data,poffd->a,poffd->nz*sizeof(PetscScalar));

  hypre_AuxParCSRMatrixNeedAux(aux_matrix) = 0;
  PetscStackCallStandard(HYPRE_IJMatrixAssemble,(ij));
  ierr = PetscLogEventEnd(MAT_Convert,A,0,0,0);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/*
    Does NOT copy the data over, instead uses DIRECTLY the pointers from the PETSc MPIAIJ format

    This is UNFINISHED and does NOT work! The problem is that hypre puts the diagonal entry first
    which will corrupt the PETSc data structure if we did this. Need a work around to this problem.
*/
#include <_hypre_IJ_mv.h>
#include <HYPRE_IJ_mv.h>

#undef __FUNCT__
#define __FUNCT__ "MatHYPRE_IJMatrixLink"
PetscErrorCode MatHYPRE_IJMatrixLink(Mat A,HYPRE_IJMatrix *ij)
{
  PetscErrorCode        ierr;
  PetscInt              rstart,rend,cstart,cend;
  PetscBool             flg;
  hypre_AuxParCSRMatrix *aux_matrix;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(A,MAT_CLASSID,1);
  PetscValidType(A,1);
  PetscValidPointer(ij,2);
  ierr = PetscObjectTypeCompare((PetscObject)A,MATMPIAIJ,&flg);CHKERRQ(ierr);
  if (!flg) SETERRQ(PetscObjectComm((PetscObject)A),PETSC_ERR_SUP,"Can only use with PETSc MPIAIJ matrices");
  ierr = MatSetUp(A);CHKERRQ(ierr);

  ierr   = PetscLogEventBegin(MAT_Convert,A,0,0,0);CHKERRQ(ierr);
  rstart = A->rmap->rstart;
  rend   = A->rmap->rend;
  cstart = A->cmap->rstart;
  cend   = A->cmap->rend;
  PetscStackCallStandard(HYPRE_IJMatrixCreate,(PetscObjectComm((PetscObject)A),rstart,rend-1,cstart,cend-1,ij));
  PetscStackCallStandard(HYPRE_IJMatrixSetObjectType,(*ij,HYPRE_PARCSR));

  PetscStackCallStandard(HYPRE_IJMatrixInitialize,(*ij));
  PetscStackCall("hypre_IJMatrixTranslator",aux_matrix = (hypre_AuxParCSRMatrix*)hypre_IJMatrixTranslator(*ij));

  hypre_AuxParCSRMatrixNeedAux(aux_matrix) = 0;

  /* this is the Hack part where we monkey directly with the hypre datastructures */

  PetscStackCallStandard(HYPRE_IJMatrixAssemble,(*ij));
  ierr = PetscLogEventEnd(MAT_Convert,A,0,0,0);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/* -----------------------------------------------------------------------------------------------------------------*/

/*MC
   MATHYPRESTRUCT - MATHYPRESTRUCT = "hyprestruct" - A matrix type to be used for parallel sparse matrices
          based on the hypre HYPRE_StructMatrix.

   Level: intermediate

   Notes: Unlike the more general support for blocks in hypre this allows only one block per process and requires the block
          be defined by a DMDA.

          The matrix needs a DMDA associated with it by either a call to MatSetupDM() or if the matrix is obtained from DMCreateMatrix()

.seealso: MatCreate(), PCPFMG, MatSetupDM(), DMCreateMatrix()
M*/


#undef __FUNCT__
#define __FUNCT__ "MatSetValuesLocal_HYPREStruct_3d"
PetscErrorCode  MatSetValuesLocal_HYPREStruct_3d(Mat mat,PetscInt nrow,const PetscInt irow[],PetscInt ncol,const PetscInt icol[],const PetscScalar y[],InsertMode addv)
{
  PetscErrorCode    ierr;
  PetscInt          i,j,stencil,index[3],row,entries[7];
  const PetscScalar *values = y;
  Mat_HYPREStruct   *ex     = (Mat_HYPREStruct*) mat->data;

  PetscFunctionBegin;
  if (PetscUnlikely(ncol >= 7)) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"ncol %D >= 7 too large",ncol);
  for (i=0; i<nrow; i++) {
    for (j=0; j<ncol; j++) {
      stencil = icol[j] - irow[i];
      if (!stencil) {
        entries[j] = 3;
      } else if (stencil == -1) {
        entries[j] = 2;
      } else if (stencil == 1) {
        entries[j] = 4;
      } else if (stencil == -ex->gnx) {
        entries[j] = 1;
      } else if (stencil == ex->gnx) {
        entries[j] = 5;
      } else if (stencil == -ex->gnxgny) {
        entries[j] = 0;
      } else if (stencil == ex->gnxgny) {
        entries[j] = 6;
      } else SETERRQ3(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONG,"Local row %D local column %D have bad stencil %D",irow[i],icol[j],stencil);
    }
    row      = ex->gindices[irow[i]] - ex->rstart;
    index[0] = ex->xs + (row % ex->nx);
    index[1] = ex->ys + ((row/ex->nx) % ex->ny);
    index[2] = ex->zs + (row/(ex->nxny));
    if (addv == ADD_VALUES) {
      PetscStackCallStandard(HYPRE_StructMatrixAddToValues,(ex->hmat,(HYPRE_Int *)index,ncol,(HYPRE_Int *)entries,(PetscScalar*)values));
    } else {
      PetscStackCallStandard(HYPRE_StructMatrixSetValues,(ex->hmat,(HYPRE_Int *)index,ncol,(HYPRE_Int *)entries,(PetscScalar*)values));
    }
    values += ncol;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatZeroRowsLocal_HYPREStruct_3d"
PetscErrorCode  MatZeroRowsLocal_HYPREStruct_3d(Mat mat,PetscInt nrow,const PetscInt irow[],PetscScalar d,Vec x,Vec b)
{
  PetscErrorCode  ierr;
  PetscInt        i,index[3],row,entries[7] = {0,1,2,3,4,5,6};
  PetscScalar     values[7];
  Mat_HYPREStruct *ex = (Mat_HYPREStruct*) mat->data;

  PetscFunctionBegin;
  if (x && b) SETERRQ(PetscObjectComm((PetscObject)mat),PETSC_ERR_SUP,"No support");
  ierr      = PetscMemzero(values,7*sizeof(PetscScalar));CHKERRQ(ierr);
  values[3] = d;
  for (i=0; i<nrow; i++) {
    row      = ex->gindices[irow[i]] - ex->rstart;
    index[0] = ex->xs + (row % ex->nx);
    index[1] = ex->ys + ((row/ex->nx) % ex->ny);
    index[2] = ex->zs + (row/(ex->nxny));
    PetscStackCallStandard(HYPRE_StructMatrixSetValues,(ex->hmat,(HYPRE_Int *)index,7,(HYPRE_Int *)entries,values));
  }
  PetscStackCallStandard(HYPRE_StructMatrixAssemble,(ex->hmat));
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatZeroEntries_HYPREStruct_3d"
PetscErrorCode MatZeroEntries_HYPREStruct_3d(Mat mat)
{
  PetscErrorCode  ierr;
  PetscInt        indices[7] = {0,1,2,3,4,5,6};
  Mat_HYPREStruct *ex        = (Mat_HYPREStruct*) mat->data;

  PetscFunctionBegin;
  /* hypre has no public interface to do this */
  PetscStackCallStandard(hypre_StructMatrixClearBoxValues,(ex->hmat,&ex->hbox,7,(HYPRE_Int *)indices,0,1));
  PetscStackCallStandard(HYPRE_StructMatrixAssemble,(ex->hmat));
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatSetupDM_HYPREStruct"
static PetscErrorCode  MatSetupDM_HYPREStruct(Mat mat,DM da)
{
  PetscErrorCode         ierr;
  Mat_HYPREStruct        *ex = (Mat_HYPREStruct*) mat->data;
  PetscInt               dim,dof,sw[3],nx,ny,nz,ilower[3],iupper[3],ssize,i;
  DMBoundaryType         px,py,pz;
  DMDAStencilType        st;
  ISLocalToGlobalMapping ltog;

  PetscFunctionBegin;
  ex->da = da;
  ierr   = PetscObjectReference((PetscObject)da);CHKERRQ(ierr);

  ierr       = DMDAGetInfo(ex->da,&dim,0,0,0,0,0,0,&dof,&sw[0],&px,&py,&pz,&st);CHKERRQ(ierr);
  ierr       = DMDAGetCorners(ex->da,&ilower[0],&ilower[1],&ilower[2],&iupper[0],&iupper[1],&iupper[2]);CHKERRQ(ierr);
  iupper[0] += ilower[0] - 1;
  iupper[1] += ilower[1] - 1;
  iupper[2] += ilower[2] - 1;

  /* the hypre_Box is used to zero out the matrix entries in MatZeroValues() */
  ex->hbox.imin[0] = ilower[0];
  ex->hbox.imin[1] = ilower[1];
  ex->hbox.imin[2] = ilower[2];
  ex->hbox.imax[0] = iupper[0];
  ex->hbox.imax[1] = iupper[1];
  ex->hbox.imax[2] = iupper[2];

  /* create the hypre grid object and set its information */
  if (dof > 1) SETERRQ(PetscObjectComm((PetscObject)da),PETSC_ERR_SUP,"Currently only support for scalar problems");
  if (px || py || pz) SETERRQ(PetscObjectComm((PetscObject)da),PETSC_ERR_SUP,"Ask us to add periodic support by calling HYPRE_StructGridSetPeriodic()");
  PetscStackCallStandard(HYPRE_StructGridCreate,(ex->hcomm,dim,&ex->hgrid));
  PetscStackCallStandard(HYPRE_StructGridSetExtents,(ex->hgrid,(HYPRE_Int *)ilower,(HYPRE_Int *)iupper));
  PetscStackCallStandard(HYPRE_StructGridAssemble,(ex->hgrid));

  sw[1] = sw[0];
  sw[2] = sw[1];
  PetscStackCallStandard(HYPRE_StructGridSetNumGhost,(ex->hgrid,(HYPRE_Int *)sw));

  /* create the hypre stencil object and set its information */
  if (sw[0] > 1) SETERRQ(PetscObjectComm((PetscObject)da),PETSC_ERR_SUP,"Ask us to add support for wider stencils");
  if (st == DMDA_STENCIL_BOX) SETERRQ(PetscObjectComm((PetscObject)da),PETSC_ERR_SUP,"Ask us to add support for box stencils");
  if (dim == 1) {
    PetscInt offsets[3][1] = {{-1},{0},{1}};
    ssize = 3;
    PetscStackCallStandard(HYPRE_StructStencilCreate,(dim,ssize,&ex->hstencil));
    for (i=0; i<ssize; i++) {
      PetscStackCallStandard(HYPRE_StructStencilSetElement,(ex->hstencil,i,(HYPRE_Int *)offsets[i]));
    }
  } else if (dim == 2) {
    PetscInt offsets[5][2] = {{0,-1},{-1,0},{0,0},{1,0},{0,1}};
    ssize = 5;
    PetscStackCallStandard(HYPRE_StructStencilCreate,(dim,ssize,&ex->hstencil));
    for (i=0; i<ssize; i++) {
      PetscStackCallStandard(HYPRE_StructStencilSetElement,(ex->hstencil,i,(HYPRE_Int *)offsets[i]));
    }
  } else if (dim == 3) {
    PetscInt offsets[7][3] = {{0,0,-1},{0,-1,0},{-1,0,0},{0,0,0},{1,0,0},{0,1,0},{0,0,1}};
    ssize = 7;
    PetscStackCallStandard(HYPRE_StructStencilCreate,(dim,ssize,&ex->hstencil));
    for (i=0; i<ssize; i++) {
      PetscStackCallStandard(HYPRE_StructStencilSetElement,(ex->hstencil,i,(HYPRE_Int *)offsets[i]));
    }
  }

  /* create the HYPRE vector for rhs and solution */
  PetscStackCallStandard(HYPRE_StructVectorCreate,(ex->hcomm,ex->hgrid,&ex->hb));
  PetscStackCallStandard(HYPRE_StructVectorCreate,(ex->hcomm,ex->hgrid,&ex->hx));
  PetscStackCallStandard(HYPRE_StructVectorInitialize,(ex->hb));
  PetscStackCallStandard(HYPRE_StructVectorInitialize,(ex->hx));
  PetscStackCallStandard(HYPRE_StructVectorAssemble,(ex->hb));
  PetscStackCallStandard(HYPRE_StructVectorAssemble,(ex->hx));

  /* create the hypre matrix object and set its information */
  PetscStackCallStandard(HYPRE_StructMatrixCreate,(ex->hcomm,ex->hgrid,ex->hstencil,&ex->hmat));
  PetscStackCallStandard(HYPRE_StructGridDestroy,(ex->hgrid));
  PetscStackCallStandard(HYPRE_StructStencilDestroy,(ex->hstencil));
  if (ex->needsinitialization) {
    PetscStackCallStandard(HYPRE_StructMatrixInitialize,(ex->hmat));
    ex->needsinitialization = PETSC_FALSE;
  }

  /* set the global and local sizes of the matrix */
  ierr = DMDAGetCorners(da,0,0,0,&nx,&ny,&nz);CHKERRQ(ierr);
  ierr = MatSetSizes(mat,dof*nx*ny*nz,dof*nx*ny*nz,PETSC_DECIDE,PETSC_DECIDE);CHKERRQ(ierr);
  ierr = PetscLayoutSetBlockSize(mat->rmap,1);CHKERRQ(ierr);
  ierr = PetscLayoutSetBlockSize(mat->cmap,1);CHKERRQ(ierr);
  ierr = PetscLayoutSetUp(mat->rmap);CHKERRQ(ierr);
  ierr = PetscLayoutSetUp(mat->cmap);CHKERRQ(ierr);

  if (dim == 3) {
    mat->ops->setvalueslocal = MatSetValuesLocal_HYPREStruct_3d;
    mat->ops->zerorowslocal  = MatZeroRowsLocal_HYPREStruct_3d;
    mat->ops->zeroentries    = MatZeroEntries_HYPREStruct_3d;

    ierr = MatZeroEntries_HYPREStruct_3d(mat);CHKERRQ(ierr);
  } else SETERRQ(PetscObjectComm((PetscObject)da),PETSC_ERR_SUP,"Only support for 3d DMDA currently");

  /* get values that will be used repeatedly in MatSetValuesLocal() and MatZeroRowsLocal() repeatedly */
  ierr        = MatGetOwnershipRange(mat,&ex->rstart,NULL);CHKERRQ(ierr);
  ierr        = DMGetLocalToGlobalMapping(ex->da,&ltog);CHKERRQ(ierr);
  ierr        = ISLocalToGlobalMappingGetIndices(ltog, (const PetscInt **) &ex->gindices);CHKERRQ(ierr);
  ierr        = DMDAGetGhostCorners(ex->da,0,0,0,&ex->gnx,&ex->gnxgny,0);CHKERRQ(ierr);
  ex->gnxgny *= ex->gnx;
  ierr        = DMDAGetCorners(ex->da,&ex->xs,&ex->ys,&ex->zs,&ex->nx,&ex->ny,0);CHKERRQ(ierr);
  ex->nxny    = ex->nx*ex->ny;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatMult_HYPREStruct"
PetscErrorCode MatMult_HYPREStruct(Mat A,Vec x,Vec y)
{
  PetscErrorCode  ierr;
  PetscScalar     *xx,*yy;
  PetscInt        ilower[3],iupper[3];
  Mat_HYPREStruct *mx = (Mat_HYPREStruct*)(A->data);

  PetscFunctionBegin;
  ierr = DMDAGetCorners(mx->da,&ilower[0],&ilower[1],&ilower[2],&iupper[0],&iupper[1],&iupper[2]);CHKERRQ(ierr);

  iupper[0] += ilower[0] - 1;
  iupper[1] += ilower[1] - 1;
  iupper[2] += ilower[2] - 1;

  /* copy x values over to hypre */
  PetscStackCallStandard(HYPRE_StructVectorSetConstantValues,(mx->hb,0.0));
  ierr = VecGetArray(x,&xx);CHKERRQ(ierr);
  PetscStackCallStandard(HYPRE_StructVectorSetBoxValues,(mx->hb,(HYPRE_Int *)ilower,(HYPRE_Int *)iupper,xx));
  ierr = VecRestoreArray(x,&xx);CHKERRQ(ierr);
  PetscStackCallStandard(HYPRE_StructVectorAssemble,(mx->hb));
  PetscStackCallStandard(HYPRE_StructMatrixMatvec,(1.0,mx->hmat,mx->hb,0.0,mx->hx));

  /* copy solution values back to PETSc */
  ierr = VecGetArray(y,&yy);CHKERRQ(ierr);
  PetscStackCallStandard(HYPRE_StructVectorGetBoxValues,(mx->hx,(HYPRE_Int *)ilower,(HYPRE_Int *)iupper,yy));
  ierr = VecRestoreArray(y,&yy);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatAssemblyEnd_HYPREStruct"
PetscErrorCode MatAssemblyEnd_HYPREStruct(Mat mat,MatAssemblyType mode)
{
  Mat_HYPREStruct *ex = (Mat_HYPREStruct*) mat->data;
  PetscErrorCode  ierr;

  PetscFunctionBegin;
  PetscStackCallStandard(HYPRE_StructMatrixAssemble,(ex->hmat));
  /* PetscStackCallStandard(HYPRE_StructMatrixPrint,("dummy",ex->hmat,0)); */
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatZeroEntries_HYPREStruct"
PetscErrorCode MatZeroEntries_HYPREStruct(Mat mat)
{
  PetscFunctionBegin;
  /* before the DMDA is set to the matrix the zero doesn't need to do anything */
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "MatDestroy_HYPREStruct"
PetscErrorCode MatDestroy_HYPREStruct(Mat mat)
{
  Mat_HYPREStruct *ex = (Mat_HYPREStruct*) mat->data;
  PetscErrorCode  ierr;

  PetscFunctionBegin;
  PetscStackCallStandard(HYPRE_StructMatrixDestroy,(ex->hmat));
  PetscStackCallStandard(HYPRE_StructVectorDestroy,(ex->hx));
  PetscStackCallStandard(HYPRE_StructVectorDestroy,(ex->hb));
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "MatCreate_HYPREStruct"
PETSC_EXTERN PetscErrorCode MatCreate_HYPREStruct(Mat B)
{
  Mat_HYPREStruct *ex;
  PetscErrorCode  ierr;

  PetscFunctionBegin;
  ierr         = PetscNewLog(B,&ex);CHKERRQ(ierr);
  B->data      = (void*)ex;
  B->rmap->bs  = 1;
  B->assembled = PETSC_FALSE;

  B->insertmode = NOT_SET_VALUES;

  B->ops->assemblyend = MatAssemblyEnd_HYPREStruct;
  B->ops->mult        = MatMult_HYPREStruct;
  B->ops->zeroentries = MatZeroEntries_HYPREStruct;
  B->ops->destroy     = MatDestroy_HYPREStruct;

  ex->needsinitialization = PETSC_TRUE;

  ierr = MPI_Comm_dup(PetscObjectComm((PetscObject)B),&(ex->hcomm));CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)B,"MatSetupDM_C",MatSetupDM_HYPREStruct);CHKERRQ(ierr);
  ierr = PetscObjectChangeTypeName((PetscObject)B,MATHYPRESTRUCT);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/*MC
   MATHYPRESSTRUCT - MATHYPRESSTRUCT = "hypresstruct" - A matrix type to be used for parallel sparse matrices
          based on the hypre HYPRE_SStructMatrix.


   Level: intermediate

   Notes: Unlike hypre's general semi-struct object consisting of a collection of structured-grid objects and unstructured
          grid objects, we will restrict the semi-struct objects to consist of only structured-grid components.

          Unlike the more general support for parts and blocks in hypre this allows only one part, and one block per process and requires the block
          be defined by a DMDA.

          The matrix needs a DMDA associated with it by either a call to MatSetupDM() or if the matrix is obtained from DMCreateMatrix()

M*/

#undef __FUNCT__
#define __FUNCT__ "MatSetValuesLocal_HYPRESStruct_3d"
PetscErrorCode  MatSetValuesLocal_HYPRESStruct_3d(Mat mat,PetscInt nrow,const PetscInt irow[],PetscInt ncol,const PetscInt icol[],const PetscScalar y[],InsertMode addv)
{
  PetscErrorCode    ierr;
  PetscInt          i,j,stencil,index[3];
  const PetscScalar *values = y;
  Mat_HYPRESStruct  *ex     = (Mat_HYPRESStruct*) mat->data;

  PetscInt part = 0;          /* Petsc sstruct interface only allows 1 part */
  PetscInt ordering;
  PetscInt grid_rank, to_grid_rank;
  PetscInt var_type, to_var_type;
  PetscInt to_var_entry = 0;

  PetscInt nvars= ex->nvars;
  PetscInt row,*entries;

  PetscFunctionBegin;
  ierr    = PetscMalloc1(7*nvars,&entries);CHKERRQ(ierr);
  ordering= ex->dofs_order;  /* ordering= 0   nodal ordering
                                          1   variable ordering */
  /* stencil entries are orderer by variables: var0_stencil0, var0_stencil1, ..., var0_stencil6, var1_stencil0, var1_stencil1, ...  */

  if (!ordering) {
    for (i=0; i<nrow; i++) {
      grid_rank= irow[i]/nvars;
      var_type = (irow[i] % nvars);

      for (j=0; j<ncol; j++) {
        to_grid_rank= icol[j]/nvars;
        to_var_type = (icol[j] % nvars);

        to_var_entry= to_var_entry*7;
        entries[j]  = to_var_entry;

        stencil = to_grid_rank-grid_rank;
        if (!stencil) {
          entries[j] += 3;
        } else if (stencil == -1) {
          entries[j] += 2;
        } else if (stencil == 1) {
          entries[j] += 4;
        } else if (stencil == -ex->gnx) {
          entries[j] += 1;
        } else if (stencil == ex->gnx) {
          entries[j] += 5;
        } else if (stencil == -ex->gnxgny) {
          entries[j] += 0;
        } else if (stencil == ex->gnxgny) {
          entries[j] += 6;
        } else SETERRQ3(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONG,"Local row %D local column %D have bad stencil %D",irow[i],icol[j],stencil);
      }

      row      = ex->gindices[grid_rank] - ex->rstart;
      index[0] = ex->xs + (row % ex->nx);
      index[1] = ex->ys + ((row/ex->nx) % ex->ny);
      index[2] = ex->zs + (row/(ex->nxny));

      if (addv == ADD_VALUES) {
        PetscStackCallStandard(HYPRE_SStructMatrixAddToValues,(ex->ss_mat,part,(HYPRE_Int *)index,var_type,ncol,(HYPRE_Int *)entries,(PetscScalar*)values));
      } else {
        PetscStackCallStandard(HYPRE_SStructMatrixSetValues,(ex->ss_mat,part,(HYPRE_Int *)index,var_type,ncol,(HYPRE_Int *)entries,(PetscScalar*)values));
      }
      values += ncol;
    }
  } else {
    for (i=0; i<nrow; i++) {
      var_type = irow[i]/(ex->gnxgnygnz);
      grid_rank= irow[i] - var_type*(ex->gnxgnygnz);

      for (j=0; j<ncol; j++) {
        to_var_type = icol[j]/(ex->gnxgnygnz);
        to_grid_rank= icol[j] - to_var_type*(ex->gnxgnygnz);

        to_var_entry= to_var_entry*7;
        entries[j]  = to_var_entry;

        stencil = to_grid_rank-grid_rank;
        if (!stencil) {
          entries[j] += 3;
        } else if (stencil == -1) {
          entries[j] += 2;
        } else if (stencil == 1) {
          entries[j] += 4;
        } else if (stencil == -ex->gnx) {
          entries[j] += 1;
        } else if (stencil == ex->gnx) {
          entries[j] += 5;
        } else if (stencil == -ex->gnxgny) {
          entries[j] += 0;
        } else if (stencil == ex->gnxgny) {
          entries[j] += 6;
        } else SETERRQ3(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONG,"Local row %D local column %D have bad stencil %D",irow[i],icol[j],stencil);
      }

      row      = ex->gindices[grid_rank] - ex->rstart;
      index[0] = ex->xs + (row % ex->nx);
      index[1] = ex->ys + ((row/ex->nx) % ex->ny);
      index[2] = ex->zs + (row/(ex->nxny));

      if (addv == ADD_VALUES) {
        PetscStackCallStandard(HYPRE_SStructMatrixAddToValues,(ex->ss_mat,part,(HYPRE_Int *)index,var_type,ncol,(HYPRE_Int *)entries,(PetscScalar*)values));
      } else {
        PetscStackCallStandard(HYPRE_SStructMatrixSetValues,(ex->ss_mat,part,(HYPRE_Int *)index,var_type,ncol,(HYPRE_Int *)entries,(PetscScalar*)values));
      }
      values += ncol;
    }
  }
  ierr = PetscFree(entries);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatZeroRowsLocal_HYPRESStruct_3d"
PetscErrorCode  MatZeroRowsLocal_HYPRESStruct_3d(Mat mat,PetscInt nrow,const PetscInt irow[],PetscScalar d,Vec x,Vec b)
{
  PetscErrorCode   ierr;
  PetscInt         i,index[3];
  PetscScalar      **values;
  Mat_HYPRESStruct *ex = (Mat_HYPRESStruct*) mat->data;

  PetscInt part     = 0;      /* Petsc sstruct interface only allows 1 part */
  PetscInt ordering = ex->dofs_order;
  PetscInt grid_rank;
  PetscInt var_type;
  PetscInt nvars= ex->nvars;
  PetscInt row,*entries;

  PetscFunctionBegin;
  if (x && b) SETERRQ(PetscObjectComm((PetscObject)mat),PETSC_ERR_SUP,"No support");
  ierr = PetscMalloc1(7*nvars,&entries);CHKERRQ(ierr);

  ierr = PetscMalloc1(nvars,&values);CHKERRQ(ierr);
  ierr = PetscMalloc1(7*nvars*nvars,&values[0]);CHKERRQ(ierr);
  for (i=1; i<nvars; i++) {
    values[i] = values[i-1] + nvars*7;
  }

  for (i=0; i< nvars; i++) {
    ierr           = PetscMemzero(values[i],nvars*7*sizeof(PetscScalar));CHKERRQ(ierr);
    *(values[i]+3) = d;
  }

  for (i= 0; i< nvars*7; i++) entries[i] = i;

  if (!ordering) {
    for (i=0; i<nrow; i++) {
      grid_rank = irow[i] / nvars;
      var_type  =(irow[i] % nvars);

      row      = ex->gindices[grid_rank] - ex->rstart;
      index[0] = ex->xs + (row % ex->nx);
      index[1] = ex->ys + ((row/ex->nx) % ex->ny);
      index[2] = ex->zs + (row/(ex->nxny));
      PetscStackCallStandard(HYPRE_SStructMatrixSetValues,(ex->ss_mat,part,(HYPRE_Int *)index,var_type,7*nvars,(HYPRE_Int *)entries,values[var_type]));
    }
  } else {
    for (i=0; i<nrow; i++) {
      var_type = irow[i]/(ex->gnxgnygnz);
      grid_rank= irow[i] - var_type*(ex->gnxgnygnz);

      row      = ex->gindices[grid_rank] - ex->rstart;
      index[0] = ex->xs + (row % ex->nx);
      index[1] = ex->ys + ((row/ex->nx) % ex->ny);
      index[2] = ex->zs + (row/(ex->nxny));
      PetscStackCallStandard(HYPRE_SStructMatrixSetValues,(ex->ss_mat,part,(HYPRE_Int *)index,var_type,7*nvars,(HYPRE_Int *)entries,values[var_type]));
    }
  }
  PetscStackCallStandard(HYPRE_SStructMatrixAssemble,(ex->ss_mat));
  ierr = PetscFree(values[0]);CHKERRQ(ierr);
  ierr = PetscFree(values);CHKERRQ(ierr);
  ierr = PetscFree(entries);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatZeroEntries_HYPRESStruct_3d"
PetscErrorCode MatZeroEntries_HYPRESStruct_3d(Mat mat)
{
  PetscErrorCode   ierr;
  Mat_HYPRESStruct *ex  = (Mat_HYPRESStruct*) mat->data;
  PetscInt         nvars= ex->nvars;
  PetscInt         size;
  PetscInt         part= 0;   /* only one part */

  PetscFunctionBegin;
  size = ((ex->hbox.imax[0])-(ex->hbox.imin[0])+1)*((ex->hbox.imax[1])-(ex->hbox.imin[1])+1)*((ex->hbox.imax[2])-(ex->hbox.imin[2])+1);
  {
    PetscInt    i,*entries,iupper[3],ilower[3];
    PetscScalar *values;


    for (i= 0; i< 3; i++) {
      ilower[i]= ex->hbox.imin[i];
      iupper[i]= ex->hbox.imax[i];
    }

    ierr = PetscMalloc2(nvars*7,&entries,nvars*7*size,&values);CHKERRQ(ierr);
    for (i= 0; i< nvars*7; i++) entries[i]= i;
    ierr = PetscMemzero(values,nvars*7*size*sizeof(PetscScalar));CHKERRQ(ierr);

    for (i= 0; i< nvars; i++) {
      PetscStackCallStandard(HYPRE_SStructMatrixSetBoxValues,(ex->ss_mat,part,(HYPRE_Int *)ilower,(HYPRE_Int *)iupper,i,nvars*7,(HYPRE_Int *)entries,values));
    }
    ierr = PetscFree2(entries,values);CHKERRQ(ierr);
  }
  PetscStackCallStandard(HYPRE_SStructMatrixAssemble,(ex->ss_mat));
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "MatSetupDM_HYPRESStruct"
static PetscErrorCode  MatSetupDM_HYPRESStruct(Mat mat,DM da)
{
  PetscErrorCode         ierr;
  Mat_HYPRESStruct       *ex = (Mat_HYPRESStruct*) mat->data;
  PetscInt               dim,dof,sw[3],nx,ny,nz;
  PetscInt               ilower[3],iupper[3],ssize,i;
  DMBoundaryType         px,py,pz;
  DMDAStencilType        st;
  PetscInt               nparts= 1;  /* assuming only one part */
  PetscInt               part  = 0;
  ISLocalToGlobalMapping ltog;
  PetscFunctionBegin;
  ex->da = da;
  ierr   = PetscObjectReference((PetscObject)da);CHKERRQ(ierr);

  ierr       = DMDAGetInfo(ex->da,&dim,0,0,0,0,0,0,&dof,&sw[0],&px,&py,&pz,&st);CHKERRQ(ierr);
  ierr       = DMDAGetCorners(ex->da,&ilower[0],&ilower[1],&ilower[2],&iupper[0],&iupper[1],&iupper[2]);CHKERRQ(ierr);
  iupper[0] += ilower[0] - 1;
  iupper[1] += ilower[1] - 1;
  iupper[2] += ilower[2] - 1;
  /* the hypre_Box is used to zero out the matrix entries in MatZeroValues() */
  ex->hbox.imin[0] = ilower[0];
  ex->hbox.imin[1] = ilower[1];
  ex->hbox.imin[2] = ilower[2];
  ex->hbox.imax[0] = iupper[0];
  ex->hbox.imax[1] = iupper[1];
  ex->hbox.imax[2] = iupper[2];

  ex->dofs_order = 0;

  /* assuming that the same number of dofs on each gridpoint. Also assume all cell-centred based */
  ex->nvars= dof;

  /* create the hypre grid object and set its information */
  if (px || py || pz) SETERRQ(PetscObjectComm((PetscObject)da),PETSC_ERR_SUP,"Ask us to add periodic support by calling HYPRE_SStructGridSetPeriodic()");
  PetscStackCallStandard(HYPRE_SStructGridCreate,(ex->hcomm,dim,nparts,&ex->ss_grid));

  PetscStackCallStandard(HYPRE_SStructGridSetExtents,(ex->ss_grid,part,ex->hbox.imin,ex->hbox.imax));

  {
    HYPRE_SStructVariable *vartypes;
    ierr = PetscMalloc1(ex->nvars,&vartypes);CHKERRQ(ierr);
    for (i= 0; i< ex->nvars; i++) vartypes[i]= HYPRE_SSTRUCT_VARIABLE_CELL;
    PetscStackCallStandard(HYPRE_SStructGridSetVariables,(ex->ss_grid, part, ex->nvars,vartypes));
    ierr = PetscFree(vartypes);CHKERRQ(ierr);
  }
  PetscStackCallStandard(HYPRE_SStructGridAssemble,(ex->ss_grid));

  sw[1] = sw[0];
  sw[2] = sw[1];
  /* PetscStackCallStandard(HYPRE_SStructGridSetNumGhost,(ex->ss_grid,sw)); */

  /* create the hypre stencil object and set its information */
  if (sw[0] > 1) SETERRQ(PetscObjectComm((PetscObject)da),PETSC_ERR_SUP,"Ask us to add support for wider stencils");
  if (st == DMDA_STENCIL_BOX) SETERRQ(PetscObjectComm((PetscObject)da),PETSC_ERR_SUP,"Ask us to add support for box stencils");

  if (dim == 1) {
    PetscInt offsets[3][1] = {{-1},{0},{1}};
    PetscInt j, cnt;

    ssize = 3*(ex->nvars);
    PetscStackCallStandard(HYPRE_SStructStencilCreate,(dim,ssize,&ex->ss_stencil));
    cnt= 0;
    for (i = 0; i < (ex->nvars); i++) {
      for (j = 0; j < 3; j++) {
        PetscStackCallStandard(HYPRE_SStructStencilSetEntry,(ex->ss_stencil, cnt, (HYPRE_Int *)offsets[j], i));
        cnt++;
      }
    }
  } else if (dim == 2) {
    PetscInt offsets[5][2] = {{0,-1},{-1,0},{0,0},{1,0},{0,1}};
    PetscInt j, cnt;

    ssize = 5*(ex->nvars);
    PetscStackCallStandard(HYPRE_SStructStencilCreate,(dim,ssize,&ex->ss_stencil));
    cnt= 0;
    for (i= 0; i< (ex->nvars); i++) {
      for (j= 0; j< 5; j++) {
        PetscStackCallStandard(HYPRE_SStructStencilSetEntry,(ex->ss_stencil, cnt, (HYPRE_Int *)offsets[j], i));
        cnt++;
      }
    }
  } else if (dim == 3) {
    PetscInt offsets[7][3] = {{0,0,-1},{0,-1,0},{-1,0,0},{0,0,0},{1,0,0},{0,1,0},{0,0,1}};
    PetscInt j, cnt;

    ssize = 7*(ex->nvars);
    PetscStackCallStandard(HYPRE_SStructStencilCreate,(dim,ssize,&ex->ss_stencil));
    cnt= 0;
    for (i= 0; i< (ex->nvars); i++) {
      for (j= 0; j< 7; j++) {
        PetscStackCallStandard(HYPRE_SStructStencilSetEntry,(ex->ss_stencil, cnt, (HYPRE_Int *)offsets[j], i));
        cnt++;
      }
    }
  }

  /* create the HYPRE graph */
  PetscStackCallStandard(HYPRE_SStructGraphCreate,(ex->hcomm, ex->ss_grid, &(ex->ss_graph)));

  /* set the stencil graph. Note that each variable has the same graph. This means that each
     variable couples to all the other variable and with the same stencil pattern. */
  for (i= 0; i< (ex->nvars); i++) {
    PetscStackCallStandard(HYPRE_SStructGraphSetStencil,(ex->ss_graph,part,i,ex->ss_stencil));
  }
  PetscStackCallStandard(HYPRE_SStructGraphAssemble,(ex->ss_graph));

  /* create the HYPRE sstruct vectors for rhs and solution */
  PetscStackCallStandard(HYPRE_SStructVectorCreate,(ex->hcomm,ex->ss_grid,&ex->ss_b));
  PetscStackCallStandard(HYPRE_SStructVectorCreate,(ex->hcomm,ex->ss_grid,&ex->ss_x));
  PetscStackCallStandard(HYPRE_SStructVectorInitialize,(ex->ss_b));
  PetscStackCallStandard(HYPRE_SStructVectorInitialize,(ex->ss_x));
  PetscStackCallStandard(HYPRE_SStructVectorAssemble,(ex->ss_b));
  PetscStackCallStandard(HYPRE_SStructVectorAssemble,(ex->ss_x));

  /* create the hypre matrix object and set its information */
  PetscStackCallStandard(HYPRE_SStructMatrixCreate,(ex->hcomm,ex->ss_graph,&ex->ss_mat));
  PetscStackCallStandard(HYPRE_SStructGridDestroy,(ex->ss_grid));
  PetscStackCallStandard(HYPRE_SStructStencilDestroy,(ex->ss_stencil));
  if (ex->needsinitialization) {
    PetscStackCallStandard(HYPRE_SStructMatrixInitialize,(ex->ss_mat));
    ex->needsinitialization = PETSC_FALSE;
  }

  /* set the global and local sizes of the matrix */
  ierr = DMDAGetCorners(da,0,0,0,&nx,&ny,&nz);CHKERRQ(ierr);
  ierr = MatSetSizes(mat,dof*nx*ny*nz,dof*nx*ny*nz,PETSC_DECIDE,PETSC_DECIDE);CHKERRQ(ierr);
  ierr = PetscLayoutSetBlockSize(mat->rmap,1);CHKERRQ(ierr);
  ierr = PetscLayoutSetBlockSize(mat->cmap,1);CHKERRQ(ierr);
  ierr = PetscLayoutSetUp(mat->rmap);CHKERRQ(ierr);
  ierr = PetscLayoutSetUp(mat->cmap);CHKERRQ(ierr);

  if (dim == 3) {
    mat->ops->setvalueslocal = MatSetValuesLocal_HYPRESStruct_3d;
    mat->ops->zerorowslocal  = MatZeroRowsLocal_HYPRESStruct_3d;
    mat->ops->zeroentries    = MatZeroEntries_HYPRESStruct_3d;

    ierr = MatZeroEntries_HYPRESStruct_3d(mat);CHKERRQ(ierr);
  } else SETERRQ(PetscObjectComm((PetscObject)da),PETSC_ERR_SUP,"Only support for 3d DMDA currently");

  /* get values that will be used repeatedly in MatSetValuesLocal() and MatZeroRowsLocal() repeatedly */
  ierr = MatGetOwnershipRange(mat,&ex->rstart,NULL);CHKERRQ(ierr);
  ierr = DMGetLocalToGlobalMapping(ex->da,&ltog);CHKERRQ(ierr);
  ierr = ISLocalToGlobalMappingGetIndices(ltog, (const PetscInt **) &ex->gindices);CHKERRQ(ierr);
  ierr = DMDAGetGhostCorners(ex->da,0,0,0,&ex->gnx,&ex->gnxgny,&ex->gnxgnygnz);CHKERRQ(ierr);

  ex->gnxgny    *= ex->gnx;
  ex->gnxgnygnz *= ex->gnxgny;

  ierr = DMDAGetCorners(ex->da,&ex->xs,&ex->ys,&ex->zs,&ex->nx,&ex->ny,&ex->nz);CHKERRQ(ierr);

  ex->nxny   = ex->nx*ex->ny;
  ex->nxnynz = ex->nz*ex->nxny;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatMult_HYPRESStruct"
PetscErrorCode MatMult_HYPRESStruct(Mat A,Vec x,Vec y)
{
  PetscErrorCode   ierr;
  PetscScalar      *xx,*yy;
  PetscInt         ilower[3],iupper[3];
  Mat_HYPRESStruct *mx     = (Mat_HYPRESStruct*)(A->data);
  PetscInt         ordering= mx->dofs_order;
  PetscInt         nvars   = mx->nvars;
  PetscInt         part    = 0;
  PetscInt         size;
  PetscInt         i;

  PetscFunctionBegin;
  ierr       = DMDAGetCorners(mx->da,&ilower[0],&ilower[1],&ilower[2],&iupper[0],&iupper[1],&iupper[2]);CHKERRQ(ierr);
  iupper[0] += ilower[0] - 1;
  iupper[1] += ilower[1] - 1;
  iupper[2] += ilower[2] - 1;

  size= 1;
  for (i = 0; i < 3; i++) size *= (iupper[i]-ilower[i]+1);

  /* copy x values over to hypre for variable ordering */
  if (ordering) {
    PetscStackCallStandard(HYPRE_SStructVectorSetConstantValues,(mx->ss_b,0.0));
    ierr = VecGetArray(x,&xx);CHKERRQ(ierr);
    for (i= 0; i< nvars; i++) {
      PetscStackCallStandard(HYPRE_SStructVectorSetBoxValues,(mx->ss_b,part,(HYPRE_Int *)ilower,(HYPRE_Int *)iupper,i,xx+(size*i)));
    }
    ierr = VecRestoreArray(x,&xx);CHKERRQ(ierr);
    PetscStackCallStandard(HYPRE_SStructVectorAssemble,(mx->ss_b));
    PetscStackCallStandard(HYPRE_SStructMatrixMatvec,(1.0,mx->ss_mat,mx->ss_b,0.0,mx->ss_x));

    /* copy solution values back to PETSc */
    ierr = VecGetArray(y,&yy);CHKERRQ(ierr);
    for (i= 0; i< nvars; i++) {
      PetscStackCallStandard(HYPRE_SStructVectorGetBoxValues,(mx->ss_x,part,(HYPRE_Int *)ilower,(HYPRE_Int *)iupper,i,yy+(size*i)));
    }
    ierr = VecRestoreArray(y,&yy);CHKERRQ(ierr);
  } else {      /* nodal ordering must be mapped to variable ordering for sys_pfmg */
    PetscScalar *z;
    PetscInt    j, k;

    ierr = PetscMalloc1(nvars*size,&z);CHKERRQ(ierr);
    PetscStackCallStandard(HYPRE_SStructVectorSetConstantValues,(mx->ss_b,0.0));
    ierr = VecGetArray(x,&xx);CHKERRQ(ierr);

    /* transform nodal to hypre's variable ordering for sys_pfmg */
    for (i= 0; i< size; i++) {
      k= i*nvars;
      for (j= 0; j< nvars; j++) z[j*size+i]= xx[k+j];
    }
    for (i= 0; i< nvars; i++) {
      PetscStackCallStandard(HYPRE_SStructVectorSetBoxValues,(mx->ss_b,part,(HYPRE_Int *)ilower,(HYPRE_Int *)iupper,i,z+(size*i)));
    }
    ierr = VecRestoreArray(x,&xx);CHKERRQ(ierr);
    PetscStackCallStandard(HYPRE_SStructVectorAssemble,(mx->ss_b));
    PetscStackCallStandard(HYPRE_SStructMatrixMatvec,(1.0,mx->ss_mat,mx->ss_b,0.0,mx->ss_x));

    /* copy solution values back to PETSc */
    ierr = VecGetArray(y,&yy);CHKERRQ(ierr);
    for (i= 0; i< nvars; i++) {
      PetscStackCallStandard(HYPRE_SStructVectorGetBoxValues,(mx->ss_x,part,(HYPRE_Int *)ilower,(HYPRE_Int *)iupper,i,z+(size*i)));
    }
    /* transform hypre's variable ordering for sys_pfmg to nodal ordering */
    for (i= 0; i< size; i++) {
      k= i*nvars;
      for (j= 0; j< nvars; j++) yy[k+j]= z[j*size+i];
    }
    ierr = VecRestoreArray(y,&yy);CHKERRQ(ierr);
    ierr = PetscFree(z);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatAssemblyEnd_HYPRESStruct"
PetscErrorCode MatAssemblyEnd_HYPRESStruct(Mat mat,MatAssemblyType mode)
{
  Mat_HYPRESStruct *ex = (Mat_HYPRESStruct*) mat->data;
  PetscErrorCode   ierr;

  PetscFunctionBegin;
  PetscStackCallStandard(HYPRE_SStructMatrixAssemble,(ex->ss_mat));
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatZeroEntries_HYPRESStruct"
PetscErrorCode MatZeroEntries_HYPRESStruct(Mat mat)
{
  PetscFunctionBegin;
  /* before the DMDA is set to the matrix the zero doesn't need to do anything */
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "MatDestroy_HYPRESStruct"
PetscErrorCode MatDestroy_HYPRESStruct(Mat mat)
{
  Mat_HYPRESStruct *ex = (Mat_HYPRESStruct*) mat->data;
  PetscErrorCode   ierr;

  PetscFunctionBegin;
  PetscStackCallStandard(HYPRE_SStructGraphDestroy,(ex->ss_graph));
  PetscStackCallStandard(HYPRE_SStructMatrixDestroy,(ex->ss_mat));
  PetscStackCallStandard(HYPRE_SStructVectorDestroy,(ex->ss_x));
  PetscStackCallStandard(HYPRE_SStructVectorDestroy,(ex->ss_b));
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatCreate_HYPRESStruct"
PETSC_EXTERN PetscErrorCode MatCreate_HYPRESStruct(Mat B)
{
  Mat_HYPRESStruct *ex;
  PetscErrorCode   ierr;

  PetscFunctionBegin;
  ierr         = PetscNewLog(B,&ex);CHKERRQ(ierr);
  B->data      = (void*)ex;
  B->rmap->bs  = 1;
  B->assembled = PETSC_FALSE;

  B->insertmode = NOT_SET_VALUES;

  B->ops->assemblyend = MatAssemblyEnd_HYPRESStruct;
  B->ops->mult        = MatMult_HYPRESStruct;
  B->ops->zeroentries = MatZeroEntries_HYPRESStruct;
  B->ops->destroy     = MatDestroy_HYPRESStruct;

  ex->needsinitialization = PETSC_TRUE;

  ierr = MPI_Comm_dup(PetscObjectComm((PetscObject)B),&(ex->hcomm));CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)B,"MatSetupDM_C",MatSetupDM_HYPRESStruct);CHKERRQ(ierr);
  ierr = PetscObjectChangeTypeName((PetscObject)B,MATHYPRESSTRUCT);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}



