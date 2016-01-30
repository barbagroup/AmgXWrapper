
#include <../src/mat/impls/aij/mpi/mpiaij.h>   /*I "petscmat.h" I*/
#include <petsc/private/vecimpl.h>
#include <petsc/private/isimpl.h>
#include <petscblaslapack.h>
#include <petscsf.h>

/*MC
   MATAIJ - MATAIJ = "aij" - A matrix type to be used for sparse matrices.

   This matrix type is identical to MATSEQAIJ when constructed with a single process communicator,
   and MATMPIAIJ otherwise.  As a result, for single process communicators,
  MatSeqAIJSetPreallocation is supported, and similarly MatMPIAIJSetPreallocation is supported
  for communicators controlling multiple processes.  It is recommended that you call both of
  the above preallocation routines for simplicity.

   Options Database Keys:
. -mat_type aij - sets the matrix type to "aij" during a call to MatSetFromOptions()

  Developer Notes: Subclasses include MATAIJCUSP, MATAIJCUSPARSE, MATAIJPERM, MATAIJCRL, and also automatically switches over to use inodes when
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

#undef __FUNCT__
#define __FUNCT__ "MatFindNonzeroRows_MPIAIJ"
PetscErrorCode MatFindNonzeroRows_MPIAIJ(Mat M,IS *keptrows)
{
  PetscErrorCode  ierr;
  Mat_MPIAIJ      *mat = (Mat_MPIAIJ*)M->data;
  Mat_SeqAIJ      *a   = (Mat_SeqAIJ*)mat->A->data;
  Mat_SeqAIJ      *b   = (Mat_SeqAIJ*)mat->B->data;
  const PetscInt  *ia,*ib;
  const MatScalar *aa,*bb;
  PetscInt        na,nb,i,j,*rows,cnt=0,n0rows;
  PetscInt        m = M->rmap->n,rstart = M->rmap->rstart;

  PetscFunctionBegin;
  *keptrows = 0;
  ia        = a->i;
  ib        = b->i;
  for (i=0; i<m; i++) {
    na = ia[i+1] - ia[i];
    nb = ib[i+1] - ib[i];
    if (!na && !nb) {
      cnt++;
      goto ok1;
    }
    aa = a->a + ia[i];
    for (j=0; j<na; j++) {
      if (aa[j] != 0.0) goto ok1;
    }
    bb = b->a + ib[i];
    for (j=0; j <nb; j++) {
      if (bb[j] != 0.0) goto ok1;
    }
    cnt++;
ok1:;
  }
  ierr = MPIU_Allreduce(&cnt,&n0rows,1,MPIU_INT,MPI_SUM,PetscObjectComm((PetscObject)M));CHKERRQ(ierr);
  if (!n0rows) PetscFunctionReturn(0);
  ierr = PetscMalloc1(M->rmap->n-cnt,&rows);CHKERRQ(ierr);
  cnt  = 0;
  for (i=0; i<m; i++) {
    na = ia[i+1] - ia[i];
    nb = ib[i+1] - ib[i];
    if (!na && !nb) continue;
    aa = a->a + ia[i];
    for (j=0; j<na;j++) {
      if (aa[j] != 0.0) {
        rows[cnt++] = rstart + i;
        goto ok2;
      }
    }
    bb = b->a + ib[i];
    for (j=0; j<nb; j++) {
      if (bb[j] != 0.0) {
        rows[cnt++] = rstart + i;
        goto ok2;
      }
    }
ok2:;
  }
  ierr = ISCreateGeneral(PetscObjectComm((PetscObject)M),cnt,rows,PETSC_OWN_POINTER,keptrows);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatDiagonalSet_MPIAIJ"
PetscErrorCode  MatDiagonalSet_MPIAIJ(Mat Y,Vec D,InsertMode is)
{
  PetscErrorCode    ierr;
  Mat_MPIAIJ        *aij = (Mat_MPIAIJ*) Y->data;

  PetscFunctionBegin;
  if (Y->assembled && Y->rmap->rstart == Y->cmap->rstart && Y->rmap->rend == Y->cmap->rend) {
    ierr = MatDiagonalSet(aij->A,D,is);CHKERRQ(ierr);
  } else {
    ierr = MatDiagonalSet_Default(Y,D,is);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "MatFindZeroDiagonals_MPIAIJ"
PetscErrorCode MatFindZeroDiagonals_MPIAIJ(Mat M,IS *zrows)
{
  Mat_MPIAIJ     *aij = (Mat_MPIAIJ*)M->data;
  PetscErrorCode ierr;
  PetscInt       i,rstart,nrows,*rows;

  PetscFunctionBegin;
  *zrows = NULL;
  ierr   = MatFindZeroDiagonals_SeqAIJ_Private(aij->A,&nrows,&rows);CHKERRQ(ierr);
  ierr   = MatGetOwnershipRange(M,&rstart,NULL);CHKERRQ(ierr);
  for (i=0; i<nrows; i++) rows[i] += rstart;
  ierr = ISCreateGeneral(PetscObjectComm((PetscObject)M),nrows,rows,PETSC_OWN_POINTER,zrows);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatGetColumnNorms_MPIAIJ"
PetscErrorCode MatGetColumnNorms_MPIAIJ(Mat A,NormType type,PetscReal *norms)
{
  PetscErrorCode ierr;
  Mat_MPIAIJ     *aij = (Mat_MPIAIJ*)A->data;
  PetscInt       i,n,*garray = aij->garray;
  Mat_SeqAIJ     *a_aij = (Mat_SeqAIJ*) aij->A->data;
  Mat_SeqAIJ     *b_aij = (Mat_SeqAIJ*) aij->B->data;
  PetscReal      *work;

  PetscFunctionBegin;
  ierr = MatGetSize(A,NULL,&n);CHKERRQ(ierr);
  ierr = PetscCalloc1(n,&work);CHKERRQ(ierr);
  if (type == NORM_2) {
    for (i=0; i<a_aij->i[aij->A->rmap->n]; i++) {
      work[A->cmap->rstart + a_aij->j[i]] += PetscAbsScalar(a_aij->a[i]*a_aij->a[i]);
    }
    for (i=0; i<b_aij->i[aij->B->rmap->n]; i++) {
      work[garray[b_aij->j[i]]] += PetscAbsScalar(b_aij->a[i]*b_aij->a[i]);
    }
  } else if (type == NORM_1) {
    for (i=0; i<a_aij->i[aij->A->rmap->n]; i++) {
      work[A->cmap->rstart + a_aij->j[i]] += PetscAbsScalar(a_aij->a[i]);
    }
    for (i=0; i<b_aij->i[aij->B->rmap->n]; i++) {
      work[garray[b_aij->j[i]]] += PetscAbsScalar(b_aij->a[i]);
    }
  } else if (type == NORM_INFINITY) {
    for (i=0; i<a_aij->i[aij->A->rmap->n]; i++) {
      work[A->cmap->rstart + a_aij->j[i]] = PetscMax(PetscAbsScalar(a_aij->a[i]), work[A->cmap->rstart + a_aij->j[i]]);
    }
    for (i=0; i<b_aij->i[aij->B->rmap->n]; i++) {
      work[garray[b_aij->j[i]]] = PetscMax(PetscAbsScalar(b_aij->a[i]),work[garray[b_aij->j[i]]]);
    }

  } else SETERRQ(PetscObjectComm((PetscObject)A),PETSC_ERR_ARG_WRONG,"Unknown NormType");
  if (type == NORM_INFINITY) {
    ierr = MPIU_Allreduce(work,norms,n,MPIU_REAL,MPIU_MAX,PetscObjectComm((PetscObject)A));CHKERRQ(ierr);
  } else {
    ierr = MPIU_Allreduce(work,norms,n,MPIU_REAL,MPIU_SUM,PetscObjectComm((PetscObject)A));CHKERRQ(ierr);
  }
  ierr = PetscFree(work);CHKERRQ(ierr);
  if (type == NORM_2) {
    for (i=0; i<n; i++) norms[i] = PetscSqrtReal(norms[i]);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatFindOffBlockDiagonalEntries_MPIAIJ"
PetscErrorCode MatFindOffBlockDiagonalEntries_MPIAIJ(Mat A,IS *is)
{
  Mat_MPIAIJ      *a  = (Mat_MPIAIJ*)A->data;
  IS              sis,gis;
  PetscErrorCode  ierr;
  const PetscInt  *isis,*igis;
  PetscInt        n,*iis,nsis,ngis,rstart,i;

  PetscFunctionBegin;
  ierr = MatFindOffBlockDiagonalEntries(a->A,&sis);CHKERRQ(ierr);
  ierr = MatFindNonzeroRows(a->B,&gis);CHKERRQ(ierr);
  ierr = ISGetSize(gis,&ngis);CHKERRQ(ierr);
  ierr = ISGetSize(sis,&nsis);CHKERRQ(ierr);
  ierr = ISGetIndices(sis,&isis);CHKERRQ(ierr);
  ierr = ISGetIndices(gis,&igis);CHKERRQ(ierr);

  ierr = PetscMalloc1(ngis+nsis,&iis);CHKERRQ(ierr);
  ierr = PetscMemcpy(iis,igis,ngis*sizeof(PetscInt));CHKERRQ(ierr);
  ierr = PetscMemcpy(iis+ngis,isis,nsis*sizeof(PetscInt));CHKERRQ(ierr);
  n    = ngis + nsis;
  ierr = PetscSortRemoveDupsInt(&n,iis);CHKERRQ(ierr);
  ierr = MatGetOwnershipRange(A,&rstart,NULL);CHKERRQ(ierr);
  for (i=0; i<n; i++) iis[i] += rstart;
  ierr = ISCreateGeneral(PetscObjectComm((PetscObject)A),n,iis,PETSC_OWN_POINTER,is);CHKERRQ(ierr);

  ierr = ISRestoreIndices(sis,&isis);CHKERRQ(ierr);
  ierr = ISRestoreIndices(gis,&igis);CHKERRQ(ierr);
  ierr = ISDestroy(&sis);CHKERRQ(ierr);
  ierr = ISDestroy(&gis);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatDistribute_MPIAIJ"
/*
    Distributes a SeqAIJ matrix across a set of processes. Code stolen from
    MatLoad_MPIAIJ(). Horrible lack of reuse. Should be a routine for each matrix type.

    Only for square matrices

    Used by a preconditioner, hence PETSC_EXTERN
*/
PETSC_EXTERN PetscErrorCode MatDistribute_MPIAIJ(MPI_Comm comm,Mat gmat,PetscInt m,MatReuse reuse,Mat *inmat)
{
  PetscMPIInt    rank,size;
  PetscInt       *rowners,*dlens,*olens,i,rstart,rend,j,jj,nz = 0,*gmataj,cnt,row,*ld,bses[2];
  PetscErrorCode ierr;
  Mat            mat;
  Mat_SeqAIJ     *gmata;
  PetscMPIInt    tag;
  MPI_Status     status;
  PetscBool      aij;
  MatScalar      *gmataa,*ao,*ad,*gmataarestore=0;

  PetscFunctionBegin;
  ierr = MPI_Comm_rank(comm,&rank);CHKERRQ(ierr);
  ierr = MPI_Comm_size(comm,&size);CHKERRQ(ierr);
  if (!rank) {
    ierr = PetscObjectTypeCompare((PetscObject)gmat,MATSEQAIJ,&aij);CHKERRQ(ierr);
    if (!aij) SETERRQ1(PetscObjectComm((PetscObject)gmat),PETSC_ERR_SUP,"Currently no support for input matrix of type %s\n",((PetscObject)gmat)->type_name);
  }
  if (reuse == MAT_INITIAL_MATRIX) {
    ierr = MatCreate(comm,&mat);CHKERRQ(ierr);
    ierr = MatSetSizes(mat,m,m,PETSC_DETERMINE,PETSC_DETERMINE);CHKERRQ(ierr);
    ierr = MatGetBlockSizes(gmat,&bses[0],&bses[1]);CHKERRQ(ierr);
    ierr = MPI_Bcast(bses,2,MPIU_INT,0,comm);CHKERRQ(ierr);
    ierr = MatSetBlockSizes(mat,bses[0],bses[1]);CHKERRQ(ierr);
    ierr = MatSetType(mat,MATAIJ);CHKERRQ(ierr);
    ierr = PetscMalloc1(size+1,&rowners);CHKERRQ(ierr);
    ierr = PetscMalloc2(m,&dlens,m,&olens);CHKERRQ(ierr);
    ierr = MPI_Allgather(&m,1,MPIU_INT,rowners+1,1,MPIU_INT,comm);CHKERRQ(ierr);

    rowners[0] = 0;
    for (i=2; i<=size; i++) rowners[i] += rowners[i-1];
    rstart = rowners[rank];
    rend   = rowners[rank+1];
    ierr   = PetscObjectGetNewTag((PetscObject)mat,&tag);CHKERRQ(ierr);
    if (!rank) {
      gmata = (Mat_SeqAIJ*) gmat->data;
      /* send row lengths to all processors */
      for (i=0; i<m; i++) dlens[i] = gmata->ilen[i];
      for (i=1; i<size; i++) {
        ierr = MPI_Send(gmata->ilen + rowners[i],rowners[i+1]-rowners[i],MPIU_INT,i,tag,comm);CHKERRQ(ierr);
      }
      /* determine number diagonal and off-diagonal counts */
      ierr = PetscMemzero(olens,m*sizeof(PetscInt));CHKERRQ(ierr);
      ierr = PetscCalloc1(m,&ld);CHKERRQ(ierr);
      jj   = 0;
      for (i=0; i<m; i++) {
        for (j=0; j<dlens[i]; j++) {
          if (gmata->j[jj] < rstart) ld[i]++;
          if (gmata->j[jj] < rstart || gmata->j[jj] >= rend) olens[i]++;
          jj++;
        }
      }
      /* send column indices to other processes */
      for (i=1; i<size; i++) {
        nz   = gmata->i[rowners[i+1]]-gmata->i[rowners[i]];
        ierr = MPI_Send(&nz,1,MPIU_INT,i,tag,comm);CHKERRQ(ierr);
        ierr = MPI_Send(gmata->j + gmata->i[rowners[i]],nz,MPIU_INT,i,tag,comm);CHKERRQ(ierr);
      }

      /* send numerical values to other processes */
      for (i=1; i<size; i++) {
        nz   = gmata->i[rowners[i+1]]-gmata->i[rowners[i]];
        ierr = MPI_Send(gmata->a + gmata->i[rowners[i]],nz,MPIU_SCALAR,i,tag,comm);CHKERRQ(ierr);
      }
      gmataa = gmata->a;
      gmataj = gmata->j;

    } else {
      /* receive row lengths */
      ierr = MPI_Recv(dlens,m,MPIU_INT,0,tag,comm,&status);CHKERRQ(ierr);
      /* receive column indices */
      ierr = MPI_Recv(&nz,1,MPIU_INT,0,tag,comm,&status);CHKERRQ(ierr);
      ierr = PetscMalloc2(nz,&gmataa,nz,&gmataj);CHKERRQ(ierr);
      ierr = MPI_Recv(gmataj,nz,MPIU_INT,0,tag,comm,&status);CHKERRQ(ierr);
      /* determine number diagonal and off-diagonal counts */
      ierr = PetscMemzero(olens,m*sizeof(PetscInt));CHKERRQ(ierr);
      ierr = PetscCalloc1(m,&ld);CHKERRQ(ierr);
      jj   = 0;
      for (i=0; i<m; i++) {
        for (j=0; j<dlens[i]; j++) {
          if (gmataj[jj] < rstart) ld[i]++;
          if (gmataj[jj] < rstart || gmataj[jj] >= rend) olens[i]++;
          jj++;
        }
      }
      /* receive numerical values */
      ierr = PetscMemzero(gmataa,nz*sizeof(PetscScalar));CHKERRQ(ierr);
      ierr = MPI_Recv(gmataa,nz,MPIU_SCALAR,0,tag,comm,&status);CHKERRQ(ierr);
    }
    /* set preallocation */
    for (i=0; i<m; i++) {
      dlens[i] -= olens[i];
    }
    ierr = MatSeqAIJSetPreallocation(mat,0,dlens);CHKERRQ(ierr);
    ierr = MatMPIAIJSetPreallocation(mat,0,dlens,0,olens);CHKERRQ(ierr);

    for (i=0; i<m; i++) {
      dlens[i] += olens[i];
    }
    cnt = 0;
    for (i=0; i<m; i++) {
      row  = rstart + i;
      ierr = MatSetValues(mat,1,&row,dlens[i],gmataj+cnt,gmataa+cnt,INSERT_VALUES);CHKERRQ(ierr);
      cnt += dlens[i];
    }
    if (rank) {
      ierr = PetscFree2(gmataa,gmataj);CHKERRQ(ierr);
    }
    ierr = PetscFree2(dlens,olens);CHKERRQ(ierr);
    ierr = PetscFree(rowners);CHKERRQ(ierr);

    ((Mat_MPIAIJ*)(mat->data))->ld = ld;

    *inmat = mat;
  } else {   /* column indices are already set; only need to move over numerical values from process 0 */
    Mat_SeqAIJ *Ad = (Mat_SeqAIJ*)((Mat_MPIAIJ*)((*inmat)->data))->A->data;
    Mat_SeqAIJ *Ao = (Mat_SeqAIJ*)((Mat_MPIAIJ*)((*inmat)->data))->B->data;
    mat  = *inmat;
    ierr = PetscObjectGetNewTag((PetscObject)mat,&tag);CHKERRQ(ierr);
    if (!rank) {
      /* send numerical values to other processes */
      gmata  = (Mat_SeqAIJ*) gmat->data;
      ierr   = MatGetOwnershipRanges(mat,(const PetscInt**)&rowners);CHKERRQ(ierr);
      gmataa = gmata->a;
      for (i=1; i<size; i++) {
        nz   = gmata->i[rowners[i+1]]-gmata->i[rowners[i]];
        ierr = MPI_Send(gmataa + gmata->i[rowners[i]],nz,MPIU_SCALAR,i,tag,comm);CHKERRQ(ierr);
      }
      nz = gmata->i[rowners[1]]-gmata->i[rowners[0]];
    } else {
      /* receive numerical values from process 0*/
      nz   = Ad->nz + Ao->nz;
      ierr = PetscMalloc1(nz,&gmataa);CHKERRQ(ierr); gmataarestore = gmataa;
      ierr = MPI_Recv(gmataa,nz,MPIU_SCALAR,0,tag,comm,&status);CHKERRQ(ierr);
    }
    /* transfer numerical values into the diagonal A and off diagonal B parts of mat */
    ld = ((Mat_MPIAIJ*)(mat->data))->ld;
    ad = Ad->a;
    ao = Ao->a;
    if (mat->rmap->n) {
      i  = 0;
      nz = ld[i];                                   ierr = PetscMemcpy(ao,gmataa,nz*sizeof(PetscScalar));CHKERRQ(ierr); ao += nz; gmataa += nz;
      nz = Ad->i[i+1] - Ad->i[i];                   ierr = PetscMemcpy(ad,gmataa,nz*sizeof(PetscScalar));CHKERRQ(ierr); ad += nz; gmataa += nz;
    }
    for (i=1; i<mat->rmap->n; i++) {
      nz = Ao->i[i] - Ao->i[i-1] - ld[i-1] + ld[i]; ierr = PetscMemcpy(ao,gmataa,nz*sizeof(PetscScalar));CHKERRQ(ierr); ao += nz; gmataa += nz;
      nz = Ad->i[i+1] - Ad->i[i];                   ierr = PetscMemcpy(ad,gmataa,nz*sizeof(PetscScalar));CHKERRQ(ierr); ad += nz; gmataa += nz;
    }
    i--;
    if (mat->rmap->n) {
      nz = Ao->i[i+1] - Ao->i[i] - ld[i];           ierr = PetscMemcpy(ao,gmataa,nz*sizeof(PetscScalar));CHKERRQ(ierr);
    }
    if (rank) {
      ierr = PetscFree(gmataarestore);CHKERRQ(ierr);
    }
  }
  ierr = MatAssemblyBegin(mat,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(mat,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/*
  Local utility routine that creates a mapping from the global column
number to the local number in the off-diagonal part of the local
storage of the matrix.  When PETSC_USE_CTABLE is used this is scalable at
a slightly higher hash table cost; without it it is not scalable (each processor
has an order N integer array but is fast to acess.
*/
#undef __FUNCT__
#define __FUNCT__ "MatCreateColmap_MPIAIJ_Private"
PetscErrorCode MatCreateColmap_MPIAIJ_Private(Mat mat)
{
  Mat_MPIAIJ     *aij = (Mat_MPIAIJ*)mat->data;
  PetscErrorCode ierr;
  PetscInt       n = aij->B->cmap->n,i;

  PetscFunctionBegin;
  if (!aij->garray) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_PLIB,"MPIAIJ Matrix was assembled but is missing garray");
#if defined(PETSC_USE_CTABLE)
  ierr = PetscTableCreate(n,mat->cmap->N+1,&aij->colmap);CHKERRQ(ierr);
  for (i=0; i<n; i++) {
    ierr = PetscTableAdd(aij->colmap,aij->garray[i]+1,i+1,INSERT_VALUES);CHKERRQ(ierr);
  }
#else
  ierr = PetscCalloc1(mat->cmap->N+1,&aij->colmap);CHKERRQ(ierr);
  ierr = PetscLogObjectMemory((PetscObject)mat,(mat->cmap->N+1)*sizeof(PetscInt));CHKERRQ(ierr);
  for (i=0; i<n; i++) aij->colmap[aij->garray[i]] = i+1;
#endif
  PetscFunctionReturn(0);
}

#define MatSetValues_SeqAIJ_A_Private(row,col,value,addv,orow,ocol)     \
{ \
    if (col <= lastcol1)  low1 = 0;     \
    else                 high1 = nrow1; \
    lastcol1 = col;\
    while (high1-low1 > 5) { \
      t = (low1+high1)/2; \
      if (rp1[t] > col) high1 = t; \
      else              low1  = t; \
    } \
      for (_i=low1; _i<high1; _i++) { \
        if (rp1[_i] > col) break; \
        if (rp1[_i] == col) { \
          if (addv == ADD_VALUES) ap1[_i] += value;   \
          else                    ap1[_i] = value; \
          goto a_noinsert; \
        } \
      }  \
      if (value == 0.0 && ignorezeroentries) {low1 = 0; high1 = nrow1;goto a_noinsert;} \
      if (nonew == 1) {low1 = 0; high1 = nrow1; goto a_noinsert;}                \
      if (nonew == -1) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Inserting a new nonzero at global row/column (%D, %D) into matrix", orow, ocol); \
      MatSeqXAIJReallocateAIJ(A,am,1,nrow1,row,col,rmax1,aa,ai,aj,rp1,ap1,aimax,nonew,MatScalar); \
      N = nrow1++ - 1; a->nz++; high1++; \
      /* shift up all the later entries in this row */ \
      for (ii=N; ii>=_i; ii--) { \
        rp1[ii+1] = rp1[ii]; \
        ap1[ii+1] = ap1[ii]; \
      } \
      rp1[_i] = col;  \
      ap1[_i] = value;  \
      A->nonzerostate++;\
      a_noinsert: ; \
      ailen[row] = nrow1; \
}


#define MatSetValues_SeqAIJ_B_Private(row,col,value,addv,orow,ocol) \
  { \
    if (col <= lastcol2) low2 = 0;                        \
    else high2 = nrow2;                                   \
    lastcol2 = col;                                       \
    while (high2-low2 > 5) {                              \
      t = (low2+high2)/2;                                 \
      if (rp2[t] > col) high2 = t;                        \
      else             low2  = t;                         \
    }                                                     \
    for (_i=low2; _i<high2; _i++) {                       \
      if (rp2[_i] > col) break;                           \
      if (rp2[_i] == col) {                               \
        if (addv == ADD_VALUES) ap2[_i] += value;         \
        else                    ap2[_i] = value;          \
        goto b_noinsert;                                  \
      }                                                   \
    }                                                     \
    if (value == 0.0 && ignorezeroentries) {low2 = 0; high2 = nrow2; goto b_noinsert;} \
    if (nonew == 1) {low2 = 0; high2 = nrow2; goto b_noinsert;}                        \
    if (nonew == -1) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Inserting a new nonzero at global row/column (%D, %D) into matrix", orow, ocol); \
    MatSeqXAIJReallocateAIJ(B,bm,1,nrow2,row,col,rmax2,ba,bi,bj,rp2,ap2,bimax,nonew,MatScalar); \
    N = nrow2++ - 1; b->nz++; high2++;                    \
    /* shift up all the later entries in this row */      \
    for (ii=N; ii>=_i; ii--) {                            \
      rp2[ii+1] = rp2[ii];                                \
      ap2[ii+1] = ap2[ii];                                \
    }                                                     \
    rp2[_i] = col;                                        \
    ap2[_i] = value;                                      \
    B->nonzerostate++;                                    \
    b_noinsert: ;                                         \
    bilen[row] = nrow2;                                   \
  }

#undef __FUNCT__
#define __FUNCT__ "MatSetValuesRow_MPIAIJ"
PetscErrorCode MatSetValuesRow_MPIAIJ(Mat A,PetscInt row,const PetscScalar v[])
{
  Mat_MPIAIJ     *mat = (Mat_MPIAIJ*)A->data;
  Mat_SeqAIJ     *a   = (Mat_SeqAIJ*)mat->A->data,*b = (Mat_SeqAIJ*)mat->B->data;
  PetscErrorCode ierr;
  PetscInt       l,*garray = mat->garray,diag;

  PetscFunctionBegin;
  /* code only works for square matrices A */

  /* find size of row to the left of the diagonal part */
  ierr = MatGetOwnershipRange(A,&diag,0);CHKERRQ(ierr);
  row  = row - diag;
  for (l=0; l<b->i[row+1]-b->i[row]; l++) {
    if (garray[b->j[b->i[row]+l]] > diag) break;
  }
  ierr = PetscMemcpy(b->a+b->i[row],v,l*sizeof(PetscScalar));CHKERRQ(ierr);

  /* diagonal part */
  ierr = PetscMemcpy(a->a+a->i[row],v+l,(a->i[row+1]-a->i[row])*sizeof(PetscScalar));CHKERRQ(ierr);

  /* right of diagonal part */
  ierr = PetscMemcpy(b->a+b->i[row]+l,v+l+a->i[row+1]-a->i[row],(b->i[row+1]-b->i[row]-l)*sizeof(PetscScalar));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatSetValues_MPIAIJ"
PetscErrorCode MatSetValues_MPIAIJ(Mat mat,PetscInt m,const PetscInt im[],PetscInt n,const PetscInt in[],const PetscScalar v[],InsertMode addv)
{
  Mat_MPIAIJ     *aij = (Mat_MPIAIJ*)mat->data;
  PetscScalar    value;
  PetscErrorCode ierr;
  PetscInt       i,j,rstart  = mat->rmap->rstart,rend = mat->rmap->rend;
  PetscInt       cstart      = mat->cmap->rstart,cend = mat->cmap->rend,row,col;
  PetscBool      roworiented = aij->roworiented;

  /* Some Variables required in the macro */
  Mat        A                 = aij->A;
  Mat_SeqAIJ *a                = (Mat_SeqAIJ*)A->data;
  PetscInt   *aimax            = a->imax,*ai = a->i,*ailen = a->ilen,*aj = a->j;
  MatScalar  *aa               = a->a;
  PetscBool  ignorezeroentries = a->ignorezeroentries;
  Mat        B                 = aij->B;
  Mat_SeqAIJ *b                = (Mat_SeqAIJ*)B->data;
  PetscInt   *bimax            = b->imax,*bi = b->i,*bilen = b->ilen,*bj = b->j,bm = aij->B->rmap->n,am = aij->A->rmap->n;
  MatScalar  *ba               = b->a;

  PetscInt  *rp1,*rp2,ii,nrow1,nrow2,_i,rmax1,rmax2,N,low1,high1,low2,high2,t,lastcol1,lastcol2;
  PetscInt  nonew;
  MatScalar *ap1,*ap2;

  PetscFunctionBegin;
  for (i=0; i<m; i++) {
    if (im[i] < 0) continue;
#if defined(PETSC_USE_DEBUG)
    if (im[i] >= mat->rmap->N) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Row too large: row %D max %D",im[i],mat->rmap->N-1);
#endif
    if (im[i] >= rstart && im[i] < rend) {
      row      = im[i] - rstart;
      lastcol1 = -1;
      rp1      = aj + ai[row];
      ap1      = aa + ai[row];
      rmax1    = aimax[row];
      nrow1    = ailen[row];
      low1     = 0;
      high1    = nrow1;
      lastcol2 = -1;
      rp2      = bj + bi[row];
      ap2      = ba + bi[row];
      rmax2    = bimax[row];
      nrow2    = bilen[row];
      low2     = 0;
      high2    = nrow2;

      for (j=0; j<n; j++) {
        if (roworiented) value = v[i*n+j];
        else             value = v[i+j*m];
        if (ignorezeroentries && value == 0.0 && (addv == ADD_VALUES)) continue;
        if (in[j] >= cstart && in[j] < cend) {
          col   = in[j] - cstart;
          nonew = a->nonew;
          MatSetValues_SeqAIJ_A_Private(row,col,value,addv,im[i],in[j]);
        } else if (in[j] < 0) continue;
#if defined(PETSC_USE_DEBUG)
        else if (in[j] >= mat->cmap->N) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Column too large: col %D max %D",in[j],mat->cmap->N-1);
#endif
        else {
          if (mat->was_assembled) {
            if (!aij->colmap) {
              ierr = MatCreateColmap_MPIAIJ_Private(mat);CHKERRQ(ierr);
            }
#if defined(PETSC_USE_CTABLE)
            ierr = PetscTableFind(aij->colmap,in[j]+1,&col);CHKERRQ(ierr);
            col--;
#else
            col = aij->colmap[in[j]] - 1;
#endif
            if (col < 0 && !((Mat_SeqAIJ*)(aij->B->data))->nonew) {
              ierr = MatDisAssemble_MPIAIJ(mat);CHKERRQ(ierr);
              col  =  in[j];
              /* Reinitialize the variables required by MatSetValues_SeqAIJ_B_Private() */
              B     = aij->B;
              b     = (Mat_SeqAIJ*)B->data;
              bimax = b->imax; bi = b->i; bilen = b->ilen; bj = b->j; ba = b->a;
              rp2   = bj + bi[row];
              ap2   = ba + bi[row];
              rmax2 = bimax[row];
              nrow2 = bilen[row];
              low2  = 0;
              high2 = nrow2;
              bm    = aij->B->rmap->n;
              ba    = b->a;
            } else if (col < 0) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Inserting a new nonzero at global row/column (%D, %D) into matrix", im[i], in[j]);
          } else col = in[j];
          nonew = b->nonew;
          MatSetValues_SeqAIJ_B_Private(row,col,value,addv,im[i],in[j]);
        }
      }
    } else {
      if (mat->nooffprocentries) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONG,"Setting off process row %D even though MatSetOption(,MAT_NO_OFF_PROC_ENTRIES,PETSC_TRUE) was set",im[i]);
      if (!aij->donotstash) {
        mat->assembled = PETSC_FALSE;
        if (roworiented) {
          ierr = MatStashValuesRow_Private(&mat->stash,im[i],n,in,v+i*n,(PetscBool)(ignorezeroentries && (addv == ADD_VALUES)));CHKERRQ(ierr);
        } else {
          ierr = MatStashValuesCol_Private(&mat->stash,im[i],n,in,v+i,m,(PetscBool)(ignorezeroentries && (addv == ADD_VALUES)));CHKERRQ(ierr);
        }
      }
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatGetValues_MPIAIJ"
PetscErrorCode MatGetValues_MPIAIJ(Mat mat,PetscInt m,const PetscInt idxm[],PetscInt n,const PetscInt idxn[],PetscScalar v[])
{
  Mat_MPIAIJ     *aij = (Mat_MPIAIJ*)mat->data;
  PetscErrorCode ierr;
  PetscInt       i,j,rstart = mat->rmap->rstart,rend = mat->rmap->rend;
  PetscInt       cstart = mat->cmap->rstart,cend = mat->cmap->rend,row,col;

  PetscFunctionBegin;
  for (i=0; i<m; i++) {
    if (idxm[i] < 0) continue; /* SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Negative row: %D",idxm[i]);*/
    if (idxm[i] >= mat->rmap->N) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Row too large: row %D max %D",idxm[i],mat->rmap->N-1);
    if (idxm[i] >= rstart && idxm[i] < rend) {
      row = idxm[i] - rstart;
      for (j=0; j<n; j++) {
        if (idxn[j] < 0) continue; /* SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Negative column: %D",idxn[j]); */
        if (idxn[j] >= mat->cmap->N) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Column too large: col %D max %D",idxn[j],mat->cmap->N-1);
        if (idxn[j] >= cstart && idxn[j] < cend) {
          col  = idxn[j] - cstart;
          ierr = MatGetValues(aij->A,1,&row,1,&col,v+i*n+j);CHKERRQ(ierr);
        } else {
          if (!aij->colmap) {
            ierr = MatCreateColmap_MPIAIJ_Private(mat);CHKERRQ(ierr);
          }
#if defined(PETSC_USE_CTABLE)
          ierr = PetscTableFind(aij->colmap,idxn[j]+1,&col);CHKERRQ(ierr);
          col--;
#else
          col = aij->colmap[idxn[j]] - 1;
#endif
          if ((col < 0) || (aij->garray[col] != idxn[j])) *(v+i*n+j) = 0.0;
          else {
            ierr = MatGetValues(aij->B,1,&row,1,&col,v+i*n+j);CHKERRQ(ierr);
          }
        }
      }
    } else SETERRQ(PETSC_COMM_SELF,PETSC_ERR_SUP,"Only local values currently supported");
  }
  PetscFunctionReturn(0);
}

extern PetscErrorCode MatMultDiagonalBlock_MPIAIJ(Mat,Vec,Vec);

#undef __FUNCT__
#define __FUNCT__ "MatAssemblyBegin_MPIAIJ"
PetscErrorCode MatAssemblyBegin_MPIAIJ(Mat mat,MatAssemblyType mode)
{
  Mat_MPIAIJ     *aij = (Mat_MPIAIJ*)mat->data;
  PetscErrorCode ierr;
  PetscInt       nstash,reallocs;

  PetscFunctionBegin;
  if (aij->donotstash || mat->nooffprocentries) PetscFunctionReturn(0);

  ierr = MatStashScatterBegin_Private(mat,&mat->stash,mat->rmap->range);CHKERRQ(ierr);
  ierr = MatStashGetInfo_Private(&mat->stash,&nstash,&reallocs);CHKERRQ(ierr);
  ierr = PetscInfo2(aij->A,"Stash has %D entries, uses %D mallocs.\n",nstash,reallocs);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatAssemblyEnd_MPIAIJ"
PetscErrorCode MatAssemblyEnd_MPIAIJ(Mat mat,MatAssemblyType mode)
{
  Mat_MPIAIJ     *aij = (Mat_MPIAIJ*)mat->data;
  Mat_SeqAIJ     *a   = (Mat_SeqAIJ*)aij->A->data;
  PetscErrorCode ierr;
  PetscMPIInt    n;
  PetscInt       i,j,rstart,ncols,flg;
  PetscInt       *row,*col;
  PetscBool      other_disassembled;
  PetscScalar    *val;

  /* do not use 'b = (Mat_SeqAIJ*)aij->B->data' as B can be reset in disassembly */

  PetscFunctionBegin;
  if (!aij->donotstash && !mat->nooffprocentries) {
    while (1) {
      ierr = MatStashScatterGetMesg_Private(&mat->stash,&n,&row,&col,&val,&flg);CHKERRQ(ierr);
      if (!flg) break;

      for (i=0; i<n; ) {
        /* Now identify the consecutive vals belonging to the same row */
        for (j=i,rstart=row[j]; j<n; j++) {
          if (row[j] != rstart) break;
        }
        if (j < n) ncols = j-i;
        else       ncols = n-i;
        /* Now assemble all these values with a single function call */
        ierr = MatSetValues_MPIAIJ(mat,1,row+i,ncols,col+i,val+i,mat->insertmode);CHKERRQ(ierr);

        i = j;
      }
    }
    ierr = MatStashScatterEnd_Private(&mat->stash);CHKERRQ(ierr);
  }
  ierr = MatAssemblyBegin(aij->A,mode);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(aij->A,mode);CHKERRQ(ierr);

  /* determine if any processor has disassembled, if so we must
     also disassemble ourselfs, in order that we may reassemble. */
  /*
     if nonzero structure of submatrix B cannot change then we know that
     no processor disassembled thus we can skip this stuff
  */
  if (!((Mat_SeqAIJ*)aij->B->data)->nonew) {
    ierr = MPIU_Allreduce(&mat->was_assembled,&other_disassembled,1,MPIU_BOOL,MPI_PROD,PetscObjectComm((PetscObject)mat));CHKERRQ(ierr);
    if (mat->was_assembled && !other_disassembled) {
      ierr = MatDisAssemble_MPIAIJ(mat);CHKERRQ(ierr);
    }
  }
  if (!mat->was_assembled && mode == MAT_FINAL_ASSEMBLY) {
    ierr = MatSetUpMultiply_MPIAIJ(mat);CHKERRQ(ierr);
  }
  ierr = MatSetOption(aij->B,MAT_USE_INODES,PETSC_FALSE);CHKERRQ(ierr);
  ierr = MatAssemblyBegin(aij->B,mode);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(aij->B,mode);CHKERRQ(ierr);

  ierr = PetscFree2(aij->rowvalues,aij->rowindices);CHKERRQ(ierr);

  aij->rowvalues = 0;

  ierr = VecDestroy(&aij->diag);CHKERRQ(ierr);
  if (a->inode.size) mat->ops->multdiagonalblock = MatMultDiagonalBlock_MPIAIJ;

  /* if no new nonzero locations are allowed in matrix then only set the matrix state the first time through */
  if ((!mat->was_assembled && mode == MAT_FINAL_ASSEMBLY) || !((Mat_SeqAIJ*)(aij->A->data))->nonew) {
    PetscObjectState state = aij->A->nonzerostate + aij->B->nonzerostate;
    ierr = MPIU_Allreduce(&state,&mat->nonzerostate,1,MPIU_INT64,MPI_SUM,PetscObjectComm((PetscObject)mat));CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatZeroEntries_MPIAIJ"
PetscErrorCode MatZeroEntries_MPIAIJ(Mat A)
{
  Mat_MPIAIJ     *l = (Mat_MPIAIJ*)A->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = MatZeroEntries(l->A);CHKERRQ(ierr);
  ierr = MatZeroEntries(l->B);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatZeroRows_MPIAIJ"
PetscErrorCode MatZeroRows_MPIAIJ(Mat A,PetscInt N,const PetscInt rows[],PetscScalar diag,Vec x,Vec b)
{
  Mat_MPIAIJ    *mat    = (Mat_MPIAIJ *) A->data;
  PetscInt      *owners = A->rmap->range;
  PetscInt       n      = A->rmap->n;
  PetscSF        sf;
  PetscInt      *lrows;
  PetscSFNode   *rrows;
  PetscInt       r, p = 0, len = 0;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  /* Create SF where leaves are input rows and roots are owned rows */
  ierr = PetscMalloc1(n, &lrows);CHKERRQ(ierr);
  for (r = 0; r < n; ++r) lrows[r] = -1;
  if (!A->nooffproczerorows) {ierr = PetscMalloc1(N, &rrows);CHKERRQ(ierr);}
  for (r = 0; r < N; ++r) {
    const PetscInt idx   = rows[r];
    if (idx < 0 || A->rmap->N <= idx) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Row %D out of range [0,%D)",idx,A->rmap->N);
    if (idx < owners[p] || owners[p+1] <= idx) { /* short-circuit the search if the last p owns this row too */
      ierr = PetscLayoutFindOwner(A->rmap,idx,&p);CHKERRQ(ierr);
    }
    if (A->nooffproczerorows) {
      if (p != mat->rank) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"MAT_NO_OFF_PROC_ZERO_ROWS set, but row %D is not owned by rank %d",idx,mat->rank);
      lrows[len++] = idx - owners[p];
    } else {
      rrows[r].rank = p;
      rrows[r].index = rows[r] - owners[p];
    }
  }
  if (!A->nooffproczerorows) {
    ierr = PetscSFCreate(PetscObjectComm((PetscObject) A), &sf);CHKERRQ(ierr);
    ierr = PetscSFSetGraph(sf, n, N, NULL, PETSC_OWN_POINTER, rrows, PETSC_OWN_POINTER);CHKERRQ(ierr);
    /* Collect flags for rows to be zeroed */
    ierr = PetscSFReduceBegin(sf, MPIU_INT, (PetscInt*)rows, lrows, MPI_LOR);CHKERRQ(ierr);
    ierr = PetscSFReduceEnd(sf, MPIU_INT, (PetscInt*)rows, lrows, MPI_LOR);CHKERRQ(ierr);
    ierr = PetscSFDestroy(&sf);CHKERRQ(ierr);
    /* Compress and put in row numbers */
    for (r = 0; r < n; ++r) if (lrows[r] >= 0) lrows[len++] = r;
  }
  /* fix right hand side if needed */
  if (x && b) {
    const PetscScalar *xx;
    PetscScalar       *bb;

    ierr = VecGetArrayRead(x, &xx);CHKERRQ(ierr);
    ierr = VecGetArray(b, &bb);CHKERRQ(ierr);
    for (r = 0; r < len; ++r) bb[lrows[r]] = diag*xx[lrows[r]];
    ierr = VecRestoreArrayRead(x, &xx);CHKERRQ(ierr);
    ierr = VecRestoreArray(b, &bb);CHKERRQ(ierr);
  }
  /* Must zero l->B before l->A because the (diag) case below may put values into l->B*/
  ierr = MatZeroRows(mat->B, len, lrows, 0.0, NULL, NULL);CHKERRQ(ierr);
  if ((diag != 0.0) && (mat->A->rmap->N == mat->A->cmap->N)) {
    ierr = MatZeroRows(mat->A, len, lrows, diag, NULL, NULL);CHKERRQ(ierr);
  } else if (diag != 0.0) {
    ierr = MatZeroRows(mat->A, len, lrows, 0.0, NULL, NULL);CHKERRQ(ierr);
    if (((Mat_SeqAIJ *) mat->A->data)->nonew) SETERRQ(PETSC_COMM_SELF, PETSC_ERR_SUP, "MatZeroRows() on rectangular matrices cannot be used with the Mat options\nMAT_NEW_NONZERO_LOCATIONS,MAT_NEW_NONZERO_LOCATION_ERR,MAT_NEW_NONZERO_ALLOCATION_ERR");
    for (r = 0; r < len; ++r) {
      const PetscInt row = lrows[r] + A->rmap->rstart;
      ierr = MatSetValues(A, 1, &row, 1, &row, &diag, INSERT_VALUES);CHKERRQ(ierr);
    }
    ierr = MatAssemblyBegin(A, MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
    ierr = MatAssemblyEnd(A, MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  } else {
    ierr = MatZeroRows(mat->A, len, lrows, 0.0, NULL, NULL);CHKERRQ(ierr);
  }
  ierr = PetscFree(lrows);CHKERRQ(ierr);

  /* only change matrix nonzero state if pattern was allowed to be changed */
  if (!((Mat_SeqAIJ*)(mat->A->data))->keepnonzeropattern) {
    PetscObjectState state = mat->A->nonzerostate + mat->B->nonzerostate;
    ierr = MPIU_Allreduce(&state,&A->nonzerostate,1,MPIU_INT64,MPI_SUM,PetscObjectComm((PetscObject)A));CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatZeroRowsColumns_MPIAIJ"
PetscErrorCode MatZeroRowsColumns_MPIAIJ(Mat A,PetscInt N,const PetscInt rows[],PetscScalar diag,Vec x,Vec b)
{
  Mat_MPIAIJ        *l = (Mat_MPIAIJ*)A->data;
  PetscErrorCode    ierr;
  PetscMPIInt       n = A->rmap->n;
  PetscInt          i,j,r,m,p = 0,len = 0;
  PetscInt          *lrows,*owners = A->rmap->range;
  PetscSFNode       *rrows;
  PetscSF           sf;
  const PetscScalar *xx;
  PetscScalar       *bb,*mask;
  Vec               xmask,lmask;
  Mat_SeqAIJ        *aij = (Mat_SeqAIJ*)l->B->data;
  const PetscInt    *aj, *ii,*ridx;
  PetscScalar       *aa;

  PetscFunctionBegin;
  /* Create SF where leaves are input rows and roots are owned rows */
  ierr = PetscMalloc1(n, &lrows);CHKERRQ(ierr);
  for (r = 0; r < n; ++r) lrows[r] = -1;
  ierr = PetscMalloc1(N, &rrows);CHKERRQ(ierr);
  for (r = 0; r < N; ++r) {
    const PetscInt idx   = rows[r];
    if (idx < 0 || A->rmap->N <= idx) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Row %D out of range [0,%D)",idx,A->rmap->N);
    if (idx < owners[p] || owners[p+1] <= idx) { /* short-circuit the search if the last p owns this row too */
      ierr = PetscLayoutFindOwner(A->rmap,idx,&p);CHKERRQ(ierr);
    }
    rrows[r].rank  = p;
    rrows[r].index = rows[r] - owners[p];
  }
  ierr = PetscSFCreate(PetscObjectComm((PetscObject) A), &sf);CHKERRQ(ierr);
  ierr = PetscSFSetGraph(sf, n, N, NULL, PETSC_OWN_POINTER, rrows, PETSC_OWN_POINTER);CHKERRQ(ierr);
  /* Collect flags for rows to be zeroed */
  ierr = PetscSFReduceBegin(sf, MPIU_INT, (PetscInt *) rows, lrows, MPI_LOR);CHKERRQ(ierr);
  ierr = PetscSFReduceEnd(sf, MPIU_INT, (PetscInt *) rows, lrows, MPI_LOR);CHKERRQ(ierr);
  ierr = PetscSFDestroy(&sf);CHKERRQ(ierr);
  /* Compress and put in row numbers */
  for (r = 0; r < n; ++r) if (lrows[r] >= 0) lrows[len++] = r;
  /* zero diagonal part of matrix */
  ierr = MatZeroRowsColumns(l->A,len,lrows,diag,x,b);CHKERRQ(ierr);
  /* handle off diagonal part of matrix */
  ierr = MatCreateVecs(A,&xmask,NULL);CHKERRQ(ierr);
  ierr = VecDuplicate(l->lvec,&lmask);CHKERRQ(ierr);
  ierr = VecGetArray(xmask,&bb);CHKERRQ(ierr);
  for (i=0; i<len; i++) bb[lrows[i]] = 1;
  ierr = VecRestoreArray(xmask,&bb);CHKERRQ(ierr);
  ierr = VecScatterBegin(l->Mvctx,xmask,lmask,ADD_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
  ierr = VecScatterEnd(l->Mvctx,xmask,lmask,ADD_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
  ierr = VecDestroy(&xmask);CHKERRQ(ierr);
  if (x) {
    ierr = VecScatterBegin(l->Mvctx,x,l->lvec,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
    ierr = VecScatterEnd(l->Mvctx,x,l->lvec,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
    ierr = VecGetArrayRead(l->lvec,&xx);CHKERRQ(ierr);
    ierr = VecGetArray(b,&bb);CHKERRQ(ierr);
  }
  ierr = VecGetArray(lmask,&mask);CHKERRQ(ierr);
  /* remove zeroed rows of off diagonal matrix */
  ii = aij->i;
  for (i=0; i<len; i++) {
    ierr = PetscMemzero(aij->a + ii[lrows[i]],(ii[lrows[i]+1] - ii[lrows[i]])*sizeof(PetscScalar));CHKERRQ(ierr);
  }
  /* loop over all elements of off process part of matrix zeroing removed columns*/
  if (aij->compressedrow.use) {
    m    = aij->compressedrow.nrows;
    ii   = aij->compressedrow.i;
    ridx = aij->compressedrow.rindex;
    for (i=0; i<m; i++) {
      n  = ii[i+1] - ii[i];
      aj = aij->j + ii[i];
      aa = aij->a + ii[i];

      for (j=0; j<n; j++) {
        if (PetscAbsScalar(mask[*aj])) {
          if (b) bb[*ridx] -= *aa*xx[*aj];
          *aa = 0.0;
        }
        aa++;
        aj++;
      }
      ridx++;
    }
  } else { /* do not use compressed row format */
    m = l->B->rmap->n;
    for (i=0; i<m; i++) {
      n  = ii[i+1] - ii[i];
      aj = aij->j + ii[i];
      aa = aij->a + ii[i];
      for (j=0; j<n; j++) {
        if (PetscAbsScalar(mask[*aj])) {
          if (b) bb[i] -= *aa*xx[*aj];
          *aa = 0.0;
        }
        aa++;
        aj++;
      }
    }
  }
  if (x) {
    ierr = VecRestoreArray(b,&bb);CHKERRQ(ierr);
    ierr = VecRestoreArrayRead(l->lvec,&xx);CHKERRQ(ierr);
  }
  ierr = VecRestoreArray(lmask,&mask);CHKERRQ(ierr);
  ierr = VecDestroy(&lmask);CHKERRQ(ierr);
  ierr = PetscFree(lrows);CHKERRQ(ierr);

  /* only change matrix nonzero state if pattern was allowed to be changed */
  if (!((Mat_SeqAIJ*)(l->A->data))->keepnonzeropattern) {
    PetscObjectState state = l->A->nonzerostate + l->B->nonzerostate;
    ierr = MPIU_Allreduce(&state,&A->nonzerostate,1,MPIU_INT64,MPI_SUM,PetscObjectComm((PetscObject)A));CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatMult_MPIAIJ"
PetscErrorCode MatMult_MPIAIJ(Mat A,Vec xx,Vec yy)
{
  Mat_MPIAIJ     *a = (Mat_MPIAIJ*)A->data;
  PetscErrorCode ierr;
  PetscInt       nt;

  PetscFunctionBegin;
  ierr = VecGetLocalSize(xx,&nt);CHKERRQ(ierr);
  if (nt != A->cmap->n) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_SIZ,"Incompatible partition of A (%D) and xx (%D)",A->cmap->n,nt);
  ierr = VecScatterBegin(a->Mvctx,xx,a->lvec,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
  ierr = (*a->A->ops->mult)(a->A,xx,yy);CHKERRQ(ierr);
  ierr = VecScatterEnd(a->Mvctx,xx,a->lvec,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
  ierr = (*a->B->ops->multadd)(a->B,a->lvec,yy,yy);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatMultDiagonalBlock_MPIAIJ"
PetscErrorCode MatMultDiagonalBlock_MPIAIJ(Mat A,Vec bb,Vec xx)
{
  Mat_MPIAIJ     *a = (Mat_MPIAIJ*)A->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = MatMultDiagonalBlock(a->A,bb,xx);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatMultAdd_MPIAIJ"
PetscErrorCode MatMultAdd_MPIAIJ(Mat A,Vec xx,Vec yy,Vec zz)
{
  Mat_MPIAIJ     *a = (Mat_MPIAIJ*)A->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = VecScatterBegin(a->Mvctx,xx,a->lvec,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
  ierr = (*a->A->ops->multadd)(a->A,xx,yy,zz);CHKERRQ(ierr);
  ierr = VecScatterEnd(a->Mvctx,xx,a->lvec,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
  ierr = (*a->B->ops->multadd)(a->B,a->lvec,zz,zz);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatMultTranspose_MPIAIJ"
PetscErrorCode MatMultTranspose_MPIAIJ(Mat A,Vec xx,Vec yy)
{
  Mat_MPIAIJ     *a = (Mat_MPIAIJ*)A->data;
  PetscErrorCode ierr;
  PetscBool      merged;

  PetscFunctionBegin;
  ierr = VecScatterGetMerged(a->Mvctx,&merged);CHKERRQ(ierr);
  /* do nondiagonal part */
  ierr = (*a->B->ops->multtranspose)(a->B,xx,a->lvec);CHKERRQ(ierr);
  if (!merged) {
    /* send it on its way */
    ierr = VecScatterBegin(a->Mvctx,a->lvec,yy,ADD_VALUES,SCATTER_REVERSE);CHKERRQ(ierr);
    /* do local part */
    ierr = (*a->A->ops->multtranspose)(a->A,xx,yy);CHKERRQ(ierr);
    /* receive remote parts: note this assumes the values are not actually */
    /* added in yy until the next line, */
    ierr = VecScatterEnd(a->Mvctx,a->lvec,yy,ADD_VALUES,SCATTER_REVERSE);CHKERRQ(ierr);
  } else {
    /* do local part */
    ierr = (*a->A->ops->multtranspose)(a->A,xx,yy);CHKERRQ(ierr);
    /* send it on its way */
    ierr = VecScatterBegin(a->Mvctx,a->lvec,yy,ADD_VALUES,SCATTER_REVERSE);CHKERRQ(ierr);
    /* values actually were received in the Begin() but we need to call this nop */
    ierr = VecScatterEnd(a->Mvctx,a->lvec,yy,ADD_VALUES,SCATTER_REVERSE);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatIsTranspose_MPIAIJ"
PetscErrorCode  MatIsTranspose_MPIAIJ(Mat Amat,Mat Bmat,PetscReal tol,PetscBool  *f)
{
  MPI_Comm       comm;
  Mat_MPIAIJ     *Aij = (Mat_MPIAIJ*) Amat->data, *Bij;
  Mat            Adia = Aij->A, Bdia, Aoff,Boff,*Aoffs,*Boffs;
  IS             Me,Notme;
  PetscErrorCode ierr;
  PetscInt       M,N,first,last,*notme,i;
  PetscMPIInt    size;

  PetscFunctionBegin;
  /* Easy test: symmetric diagonal block */
  Bij  = (Mat_MPIAIJ*) Bmat->data; Bdia = Bij->A;
  ierr = MatIsTranspose(Adia,Bdia,tol,f);CHKERRQ(ierr);
  if (!*f) PetscFunctionReturn(0);
  ierr = PetscObjectGetComm((PetscObject)Amat,&comm);CHKERRQ(ierr);
  ierr = MPI_Comm_size(comm,&size);CHKERRQ(ierr);
  if (size == 1) PetscFunctionReturn(0);

  /* Hard test: off-diagonal block. This takes a MatGetSubMatrix. */
  ierr = MatGetSize(Amat,&M,&N);CHKERRQ(ierr);
  ierr = MatGetOwnershipRange(Amat,&first,&last);CHKERRQ(ierr);
  ierr = PetscMalloc1(N-last+first,&notme);CHKERRQ(ierr);
  for (i=0; i<first; i++) notme[i] = i;
  for (i=last; i<M; i++) notme[i-last+first] = i;
  ierr = ISCreateGeneral(MPI_COMM_SELF,N-last+first,notme,PETSC_COPY_VALUES,&Notme);CHKERRQ(ierr);
  ierr = ISCreateStride(MPI_COMM_SELF,last-first,first,1,&Me);CHKERRQ(ierr);
  ierr = MatGetSubMatrices(Amat,1,&Me,&Notme,MAT_INITIAL_MATRIX,&Aoffs);CHKERRQ(ierr);
  Aoff = Aoffs[0];
  ierr = MatGetSubMatrices(Bmat,1,&Notme,&Me,MAT_INITIAL_MATRIX,&Boffs);CHKERRQ(ierr);
  Boff = Boffs[0];
  ierr = MatIsTranspose(Aoff,Boff,tol,f);CHKERRQ(ierr);
  ierr = MatDestroyMatrices(1,&Aoffs);CHKERRQ(ierr);
  ierr = MatDestroyMatrices(1,&Boffs);CHKERRQ(ierr);
  ierr = ISDestroy(&Me);CHKERRQ(ierr);
  ierr = ISDestroy(&Notme);CHKERRQ(ierr);
  ierr = PetscFree(notme);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatMultTransposeAdd_MPIAIJ"
PetscErrorCode MatMultTransposeAdd_MPIAIJ(Mat A,Vec xx,Vec yy,Vec zz)
{
  Mat_MPIAIJ     *a = (Mat_MPIAIJ*)A->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  /* do nondiagonal part */
  ierr = (*a->B->ops->multtranspose)(a->B,xx,a->lvec);CHKERRQ(ierr);
  /* send it on its way */
  ierr = VecScatterBegin(a->Mvctx,a->lvec,zz,ADD_VALUES,SCATTER_REVERSE);CHKERRQ(ierr);
  /* do local part */
  ierr = (*a->A->ops->multtransposeadd)(a->A,xx,yy,zz);CHKERRQ(ierr);
  /* receive remote parts */
  ierr = VecScatterEnd(a->Mvctx,a->lvec,zz,ADD_VALUES,SCATTER_REVERSE);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/*
  This only works correctly for square matrices where the subblock A->A is the
   diagonal block
*/
#undef __FUNCT__
#define __FUNCT__ "MatGetDiagonal_MPIAIJ"
PetscErrorCode MatGetDiagonal_MPIAIJ(Mat A,Vec v)
{
  PetscErrorCode ierr;
  Mat_MPIAIJ     *a = (Mat_MPIAIJ*)A->data;

  PetscFunctionBegin;
  if (A->rmap->N != A->cmap->N) SETERRQ(PetscObjectComm((PetscObject)A),PETSC_ERR_SUP,"Supports only square matrix where A->A is diag block");
  if (A->rmap->rstart != A->cmap->rstart || A->rmap->rend != A->cmap->rend) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_SIZ,"row partition must equal col partition");
  ierr = MatGetDiagonal(a->A,v);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatScale_MPIAIJ"
PetscErrorCode MatScale_MPIAIJ(Mat A,PetscScalar aa)
{
  Mat_MPIAIJ     *a = (Mat_MPIAIJ*)A->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = MatScale(a->A,aa);CHKERRQ(ierr);
  ierr = MatScale(a->B,aa);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatDestroy_MPIAIJ"
PetscErrorCode MatDestroy_MPIAIJ(Mat mat)
{
  Mat_MPIAIJ     *aij = (Mat_MPIAIJ*)mat->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
#if defined(PETSC_USE_LOG)
  PetscLogObjectState((PetscObject)mat,"Rows=%D, Cols=%D",mat->rmap->N,mat->cmap->N);
#endif
  ierr = MatStashDestroy_Private(&mat->stash);CHKERRQ(ierr);
  ierr = VecDestroy(&aij->diag);CHKERRQ(ierr);
  ierr = MatDestroy(&aij->A);CHKERRQ(ierr);
  ierr = MatDestroy(&aij->B);CHKERRQ(ierr);
#if defined(PETSC_USE_CTABLE)
  ierr = PetscTableDestroy(&aij->colmap);CHKERRQ(ierr);
#else
  ierr = PetscFree(aij->colmap);CHKERRQ(ierr);
#endif
  ierr = PetscFree(aij->garray);CHKERRQ(ierr);
  ierr = VecDestroy(&aij->lvec);CHKERRQ(ierr);
  ierr = VecScatterDestroy(&aij->Mvctx);CHKERRQ(ierr);
  ierr = PetscFree2(aij->rowvalues,aij->rowindices);CHKERRQ(ierr);
  ierr = PetscFree(aij->ld);CHKERRQ(ierr);
  ierr = PetscFree(mat->data);CHKERRQ(ierr);

  ierr = PetscObjectChangeTypeName((PetscObject)mat,0);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)mat,"MatStoreValues_C",NULL);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)mat,"MatRetrieveValues_C",NULL);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)mat,"MatGetDiagonalBlock_C",NULL);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)mat,"MatIsTranspose_C",NULL);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)mat,"MatMPIAIJSetPreallocation_C",NULL);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)mat,"MatMPIAIJSetPreallocationCSR_C",NULL);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)mat,"MatDiagonalScaleLocal_C",NULL);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)mat,"MatConvert_mpiaij_mpisbaij_C",NULL);CHKERRQ(ierr);
#if defined(PETSC_HAVE_ELEMENTAL)
  ierr = PetscObjectComposeFunction((PetscObject)mat,"MatConvert_mpiaij_elemental_C",NULL);CHKERRQ(ierr);
#endif
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatView_MPIAIJ_Binary"
PetscErrorCode MatView_MPIAIJ_Binary(Mat mat,PetscViewer viewer)
{
  Mat_MPIAIJ     *aij = (Mat_MPIAIJ*)mat->data;
  Mat_SeqAIJ     *A   = (Mat_SeqAIJ*)aij->A->data;
  Mat_SeqAIJ     *B   = (Mat_SeqAIJ*)aij->B->data;
  PetscErrorCode ierr;
  PetscMPIInt    rank,size,tag = ((PetscObject)viewer)->tag;
  int            fd;
  PetscInt       nz,header[4],*row_lengths,*range=0,rlen,i;
  PetscInt       nzmax,*column_indices,j,k,col,*garray = aij->garray,cnt,cstart = mat->cmap->rstart,rnz = 0;
  PetscScalar    *column_values;
  PetscInt       message_count,flowcontrolcount;
  FILE           *file;

  PetscFunctionBegin;
  ierr = MPI_Comm_rank(PetscObjectComm((PetscObject)mat),&rank);CHKERRQ(ierr);
  ierr = MPI_Comm_size(PetscObjectComm((PetscObject)mat),&size);CHKERRQ(ierr);
  nz   = A->nz + B->nz;
  ierr = PetscViewerBinaryGetDescriptor(viewer,&fd);CHKERRQ(ierr);
  if (!rank) {
    header[0] = MAT_FILE_CLASSID;
    header[1] = mat->rmap->N;
    header[2] = mat->cmap->N;

    ierr = MPI_Reduce(&nz,&header[3],1,MPIU_INT,MPI_SUM,0,PetscObjectComm((PetscObject)mat));CHKERRQ(ierr);
    ierr = PetscBinaryWrite(fd,header,4,PETSC_INT,PETSC_TRUE);CHKERRQ(ierr);
    /* get largest number of rows any processor has */
    rlen  = mat->rmap->n;
    range = mat->rmap->range;
    for (i=1; i<size; i++) rlen = PetscMax(rlen,range[i+1] - range[i]);
  } else {
    ierr = MPI_Reduce(&nz,0,1,MPIU_INT,MPI_SUM,0,PetscObjectComm((PetscObject)mat));CHKERRQ(ierr);
    rlen = mat->rmap->n;
  }

  /* load up the local row counts */
  ierr = PetscMalloc1(rlen+1,&row_lengths);CHKERRQ(ierr);
  for (i=0; i<mat->rmap->n; i++) row_lengths[i] = A->i[i+1] - A->i[i] + B->i[i+1] - B->i[i];

  /* store the row lengths to the file */
  ierr = PetscViewerFlowControlStart(viewer,&message_count,&flowcontrolcount);CHKERRQ(ierr);
  if (!rank) {
    ierr = PetscBinaryWrite(fd,row_lengths,mat->rmap->n,PETSC_INT,PETSC_TRUE);CHKERRQ(ierr);
    for (i=1; i<size; i++) {
      ierr = PetscViewerFlowControlStepMaster(viewer,i,&message_count,flowcontrolcount);CHKERRQ(ierr);
      rlen = range[i+1] - range[i];
      ierr = MPIULong_Recv(row_lengths,rlen,MPIU_INT,i,tag,PetscObjectComm((PetscObject)mat));CHKERRQ(ierr);
      ierr = PetscBinaryWrite(fd,row_lengths,rlen,PETSC_INT,PETSC_TRUE);CHKERRQ(ierr);
    }
    ierr = PetscViewerFlowControlEndMaster(viewer,&message_count);CHKERRQ(ierr);
  } else {
    ierr = PetscViewerFlowControlStepWorker(viewer,rank,&message_count);CHKERRQ(ierr);
    ierr = MPIULong_Send(row_lengths,mat->rmap->n,MPIU_INT,0,tag,PetscObjectComm((PetscObject)mat));CHKERRQ(ierr);
    ierr = PetscViewerFlowControlEndWorker(viewer,&message_count);CHKERRQ(ierr);
  }
  ierr = PetscFree(row_lengths);CHKERRQ(ierr);

  /* load up the local column indices */
  nzmax = nz; /* th processor needs space a largest processor needs */
  ierr  = MPI_Reduce(&nz,&nzmax,1,MPIU_INT,MPI_MAX,0,PetscObjectComm((PetscObject)mat));CHKERRQ(ierr);
  ierr  = PetscMalloc1(nzmax+1,&column_indices);CHKERRQ(ierr);
  cnt   = 0;
  for (i=0; i<mat->rmap->n; i++) {
    for (j=B->i[i]; j<B->i[i+1]; j++) {
      if ((col = garray[B->j[j]]) > cstart) break;
      column_indices[cnt++] = col;
    }
    for (k=A->i[i]; k<A->i[i+1]; k++) column_indices[cnt++] = A->j[k] + cstart;
    for (; j<B->i[i+1]; j++) column_indices[cnt++] = garray[B->j[j]];
  }
  if (cnt != A->nz + B->nz) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_LIB,"Internal PETSc error: cnt = %D nz = %D",cnt,A->nz+B->nz);

  /* store the column indices to the file */
  ierr = PetscViewerFlowControlStart(viewer,&message_count,&flowcontrolcount);CHKERRQ(ierr);
  if (!rank) {
    MPI_Status status;
    ierr = PetscBinaryWrite(fd,column_indices,nz,PETSC_INT,PETSC_TRUE);CHKERRQ(ierr);
    for (i=1; i<size; i++) {
      ierr = PetscViewerFlowControlStepMaster(viewer,i,&message_count,flowcontrolcount);CHKERRQ(ierr);
      ierr = MPI_Recv(&rnz,1,MPIU_INT,i,tag,PetscObjectComm((PetscObject)mat),&status);CHKERRQ(ierr);
      if (rnz > nzmax) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_LIB,"Internal PETSc error: nz = %D nzmax = %D",nz,nzmax);
      ierr = MPIULong_Recv(column_indices,rnz,MPIU_INT,i,tag,PetscObjectComm((PetscObject)mat));CHKERRQ(ierr);
      ierr = PetscBinaryWrite(fd,column_indices,rnz,PETSC_INT,PETSC_TRUE);CHKERRQ(ierr);
    }
    ierr = PetscViewerFlowControlEndMaster(viewer,&message_count);CHKERRQ(ierr);
  } else {
    ierr = PetscViewerFlowControlStepWorker(viewer,rank,&message_count);CHKERRQ(ierr);
    ierr = MPI_Send(&nz,1,MPIU_INT,0,tag,PetscObjectComm((PetscObject)mat));CHKERRQ(ierr);
    ierr = MPIULong_Send(column_indices,nz,MPIU_INT,0,tag,PetscObjectComm((PetscObject)mat));CHKERRQ(ierr);
    ierr = PetscViewerFlowControlEndWorker(viewer,&message_count);CHKERRQ(ierr);
  }
  ierr = PetscFree(column_indices);CHKERRQ(ierr);

  /* load up the local column values */
  ierr = PetscMalloc1(nzmax+1,&column_values);CHKERRQ(ierr);
  cnt  = 0;
  for (i=0; i<mat->rmap->n; i++) {
    for (j=B->i[i]; j<B->i[i+1]; j++) {
      if (garray[B->j[j]] > cstart) break;
      column_values[cnt++] = B->a[j];
    }
    for (k=A->i[i]; k<A->i[i+1]; k++) column_values[cnt++] = A->a[k];
    for (; j<B->i[i+1]; j++) column_values[cnt++] = B->a[j];
  }
  if (cnt != A->nz + B->nz) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_PLIB,"Internal PETSc error: cnt = %D nz = %D",cnt,A->nz+B->nz);

  /* store the column values to the file */
  ierr = PetscViewerFlowControlStart(viewer,&message_count,&flowcontrolcount);CHKERRQ(ierr);
  if (!rank) {
    MPI_Status status;
    ierr = PetscBinaryWrite(fd,column_values,nz,PETSC_SCALAR,PETSC_TRUE);CHKERRQ(ierr);
    for (i=1; i<size; i++) {
      ierr = PetscViewerFlowControlStepMaster(viewer,i,&message_count,flowcontrolcount);CHKERRQ(ierr);
      ierr = MPI_Recv(&rnz,1,MPIU_INT,i,tag,PetscObjectComm((PetscObject)mat),&status);CHKERRQ(ierr);
      if (rnz > nzmax) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_LIB,"Internal PETSc error: nz = %D nzmax = %D",nz,nzmax);
      ierr = MPIULong_Recv(column_values,rnz,MPIU_SCALAR,i,tag,PetscObjectComm((PetscObject)mat));CHKERRQ(ierr);
      ierr = PetscBinaryWrite(fd,column_values,rnz,PETSC_SCALAR,PETSC_TRUE);CHKERRQ(ierr);
    }
    ierr = PetscViewerFlowControlEndMaster(viewer,&message_count);CHKERRQ(ierr);
  } else {
    ierr = PetscViewerFlowControlStepWorker(viewer,rank,&message_count);CHKERRQ(ierr);
    ierr = MPI_Send(&nz,1,MPIU_INT,0,tag,PetscObjectComm((PetscObject)mat));CHKERRQ(ierr);
    ierr = MPIULong_Send(column_values,nz,MPIU_SCALAR,0,tag,PetscObjectComm((PetscObject)mat));CHKERRQ(ierr);
    ierr = PetscViewerFlowControlEndWorker(viewer,&message_count);CHKERRQ(ierr);
  }
  ierr = PetscFree(column_values);CHKERRQ(ierr);

  ierr = PetscViewerBinaryGetInfoPointer(viewer,&file);CHKERRQ(ierr);
  if (file) fprintf(file,"-matload_block_size %d\n",(int)PetscAbs(mat->rmap->bs));
  PetscFunctionReturn(0);
}

#include <petscdraw.h>
#undef __FUNCT__
#define __FUNCT__ "MatView_MPIAIJ_ASCIIorDraworSocket"
PetscErrorCode MatView_MPIAIJ_ASCIIorDraworSocket(Mat mat,PetscViewer viewer)
{
  Mat_MPIAIJ        *aij = (Mat_MPIAIJ*)mat->data;
  PetscErrorCode    ierr;
  PetscMPIInt       rank = aij->rank,size = aij->size;
  PetscBool         isdraw,iascii,isbinary;
  PetscViewer       sviewer;
  PetscViewerFormat format;

  PetscFunctionBegin;
  ierr = PetscObjectTypeCompare((PetscObject)viewer,PETSCVIEWERDRAW,&isdraw);CHKERRQ(ierr);
  ierr = PetscObjectTypeCompare((PetscObject)viewer,PETSCVIEWERASCII,&iascii);CHKERRQ(ierr);
  ierr = PetscObjectTypeCompare((PetscObject)viewer,PETSCVIEWERBINARY,&isbinary);CHKERRQ(ierr);
  if (iascii) {
    ierr = PetscViewerGetFormat(viewer,&format);CHKERRQ(ierr);
    if (format == PETSC_VIEWER_ASCII_INFO_DETAIL) {
      MatInfo   info;
      PetscBool inodes;

      ierr = MPI_Comm_rank(PetscObjectComm((PetscObject)mat),&rank);CHKERRQ(ierr);
      ierr = MatGetInfo(mat,MAT_LOCAL,&info);CHKERRQ(ierr);
      ierr = MatInodeGetInodeSizes(aij->A,NULL,(PetscInt**)&inodes,NULL);CHKERRQ(ierr);
      ierr = PetscViewerASCIIPushSynchronized(viewer);CHKERRQ(ierr);
      if (!inodes) {
        ierr = PetscViewerASCIISynchronizedPrintf(viewer,"[%d] Local rows %D nz %D nz alloced %D mem %D, not using I-node routines\n",
                                                  rank,mat->rmap->n,(PetscInt)info.nz_used,(PetscInt)info.nz_allocated,(PetscInt)info.memory);CHKERRQ(ierr);
      } else {
        ierr = PetscViewerASCIISynchronizedPrintf(viewer,"[%d] Local rows %D nz %D nz alloced %D mem %D, using I-node routines\n",
                                                  rank,mat->rmap->n,(PetscInt)info.nz_used,(PetscInt)info.nz_allocated,(PetscInt)info.memory);CHKERRQ(ierr);
      }
      ierr = MatGetInfo(aij->A,MAT_LOCAL,&info);CHKERRQ(ierr);
      ierr = PetscViewerASCIISynchronizedPrintf(viewer,"[%d] on-diagonal part: nz %D \n",rank,(PetscInt)info.nz_used);CHKERRQ(ierr);
      ierr = MatGetInfo(aij->B,MAT_LOCAL,&info);CHKERRQ(ierr);
      ierr = PetscViewerASCIISynchronizedPrintf(viewer,"[%d] off-diagonal part: nz %D \n",rank,(PetscInt)info.nz_used);CHKERRQ(ierr);
      ierr = PetscViewerFlush(viewer);CHKERRQ(ierr);
      ierr = PetscViewerASCIIPopSynchronized(viewer);CHKERRQ(ierr);
      ierr = PetscViewerASCIIPrintf(viewer,"Information on VecScatter used in matrix-vector product: \n");CHKERRQ(ierr);
      ierr = VecScatterView(aij->Mvctx,viewer);CHKERRQ(ierr);
      PetscFunctionReturn(0);
    } else if (format == PETSC_VIEWER_ASCII_INFO) {
      PetscInt inodecount,inodelimit,*inodes;
      ierr = MatInodeGetInodeSizes(aij->A,&inodecount,&inodes,&inodelimit);CHKERRQ(ierr);
      if (inodes) {
        ierr = PetscViewerASCIIPrintf(viewer,"using I-node (on process 0) routines: found %D nodes, limit used is %D\n",inodecount,inodelimit);CHKERRQ(ierr);
      } else {
        ierr = PetscViewerASCIIPrintf(viewer,"not using I-node (on process 0) routines\n");CHKERRQ(ierr);
      }
      PetscFunctionReturn(0);
    } else if (format == PETSC_VIEWER_ASCII_FACTOR_INFO) {
      PetscFunctionReturn(0);
    }
  } else if (isbinary) {
    if (size == 1) {
      ierr = PetscObjectSetName((PetscObject)aij->A,((PetscObject)mat)->name);CHKERRQ(ierr);
      ierr = MatView(aij->A,viewer);CHKERRQ(ierr);
    } else {
      ierr = MatView_MPIAIJ_Binary(mat,viewer);CHKERRQ(ierr);
    }
    PetscFunctionReturn(0);
  } else if (isdraw) {
    PetscDraw draw;
    PetscBool isnull;
    ierr = PetscViewerDrawGetDraw(viewer,0,&draw);CHKERRQ(ierr);
    ierr = PetscDrawIsNull(draw,&isnull);CHKERRQ(ierr); if (isnull) PetscFunctionReturn(0);
  }

  {
    /* assemble the entire matrix onto first processor. */
    Mat        A;
    Mat_SeqAIJ *Aloc;
    PetscInt   M = mat->rmap->N,N = mat->cmap->N,m,*ai,*aj,row,*cols,i,*ct;
    MatScalar  *a;

    ierr = MatCreate(PetscObjectComm((PetscObject)mat),&A);CHKERRQ(ierr);
    if (!rank) {
      ierr = MatSetSizes(A,M,N,M,N);CHKERRQ(ierr);
    } else {
      ierr = MatSetSizes(A,0,0,M,N);CHKERRQ(ierr);
    }
    /* This is just a temporary matrix, so explicitly using MATMPIAIJ is probably best */
    ierr = MatSetType(A,MATMPIAIJ);CHKERRQ(ierr);
    ierr = MatMPIAIJSetPreallocation(A,0,NULL,0,NULL);CHKERRQ(ierr);
    ierr = MatSetOption(A,MAT_NEW_NONZERO_LOCATION_ERR,PETSC_FALSE);CHKERRQ(ierr);
    ierr = PetscLogObjectParent((PetscObject)mat,(PetscObject)A);CHKERRQ(ierr);

    /* copy over the A part */
    Aloc = (Mat_SeqAIJ*)aij->A->data;
    m    = aij->A->rmap->n; ai = Aloc->i; aj = Aloc->j; a = Aloc->a;
    row  = mat->rmap->rstart;
    for (i=0; i<ai[m]; i++) aj[i] += mat->cmap->rstart;
    for (i=0; i<m; i++) {
      ierr = MatSetValues(A,1,&row,ai[i+1]-ai[i],aj,a,INSERT_VALUES);CHKERRQ(ierr);
      row++;
      a += ai[i+1]-ai[i]; aj += ai[i+1]-ai[i];
    }
    aj = Aloc->j;
    for (i=0; i<ai[m]; i++) aj[i] -= mat->cmap->rstart;

    /* copy over the B part */
    Aloc = (Mat_SeqAIJ*)aij->B->data;
    m    = aij->B->rmap->n;  ai = Aloc->i; aj = Aloc->j; a = Aloc->a;
    row  = mat->rmap->rstart;
    ierr = PetscMalloc1(ai[m]+1,&cols);CHKERRQ(ierr);
    ct   = cols;
    for (i=0; i<ai[m]; i++) cols[i] = aij->garray[aj[i]];
    for (i=0; i<m; i++) {
      ierr = MatSetValues(A,1,&row,ai[i+1]-ai[i],cols,a,INSERT_VALUES);CHKERRQ(ierr);
      row++;
      a += ai[i+1]-ai[i]; cols += ai[i+1]-ai[i];
    }
    ierr = PetscFree(ct);CHKERRQ(ierr);
    ierr = MatAssemblyBegin(A,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
    ierr = MatAssemblyEnd(A,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
    /*
       Everyone has to call to draw the matrix since the graphics waits are
       synchronized across all processors that share the PetscDraw object
    */
    ierr = PetscViewerGetSubViewer(viewer,PETSC_COMM_SELF,&sviewer);CHKERRQ(ierr);
    if (!rank) {
      ierr = PetscObjectSetName((PetscObject)((Mat_MPIAIJ*)(A->data))->A,((PetscObject)mat)->name);CHKERRQ(ierr);
      ierr = MatView_SeqAIJ(((Mat_MPIAIJ*)(A->data))->A,sviewer);CHKERRQ(ierr);
    }
    ierr = PetscViewerRestoreSubViewer(viewer,PETSC_COMM_SELF,&sviewer);CHKERRQ(ierr);
    ierr = PetscViewerFlush(viewer);CHKERRQ(ierr);
    ierr = MatDestroy(&A);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatView_MPIAIJ"
PetscErrorCode MatView_MPIAIJ(Mat mat,PetscViewer viewer)
{
  PetscErrorCode ierr;
  PetscBool      iascii,isdraw,issocket,isbinary;

  PetscFunctionBegin;
  ierr = PetscObjectTypeCompare((PetscObject)viewer,PETSCVIEWERASCII,&iascii);CHKERRQ(ierr);
  ierr = PetscObjectTypeCompare((PetscObject)viewer,PETSCVIEWERDRAW,&isdraw);CHKERRQ(ierr);
  ierr = PetscObjectTypeCompare((PetscObject)viewer,PETSCVIEWERBINARY,&isbinary);CHKERRQ(ierr);
  ierr = PetscObjectTypeCompare((PetscObject)viewer,PETSCVIEWERSOCKET,&issocket);CHKERRQ(ierr);
  if (iascii || isdraw || isbinary || issocket) {
    ierr = MatView_MPIAIJ_ASCIIorDraworSocket(mat,viewer);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatSOR_MPIAIJ"
PetscErrorCode MatSOR_MPIAIJ(Mat matin,Vec bb,PetscReal omega,MatSORType flag,PetscReal fshift,PetscInt its,PetscInt lits,Vec xx)
{
  Mat_MPIAIJ     *mat = (Mat_MPIAIJ*)matin->data;
  PetscErrorCode ierr;
  Vec            bb1 = 0;
  PetscBool      hasop;

  PetscFunctionBegin;
  if (flag == SOR_APPLY_UPPER) {
    ierr = (*mat->A->ops->sor)(mat->A,bb,omega,flag,fshift,lits,1,xx);CHKERRQ(ierr);
    PetscFunctionReturn(0);
  }

  if (its > 1 || ~flag & SOR_ZERO_INITIAL_GUESS || flag & SOR_EISENSTAT) {
    ierr = VecDuplicate(bb,&bb1);CHKERRQ(ierr);
  }

  if ((flag & SOR_LOCAL_SYMMETRIC_SWEEP) == SOR_LOCAL_SYMMETRIC_SWEEP) {
    if (flag & SOR_ZERO_INITIAL_GUESS) {
      ierr = (*mat->A->ops->sor)(mat->A,bb,omega,flag,fshift,lits,1,xx);CHKERRQ(ierr);
      its--;
    }

    while (its--) {
      ierr = VecScatterBegin(mat->Mvctx,xx,mat->lvec,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
      ierr = VecScatterEnd(mat->Mvctx,xx,mat->lvec,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);

      /* update rhs: bb1 = bb - B*x */
      ierr = VecScale(mat->lvec,-1.0);CHKERRQ(ierr);
      ierr = (*mat->B->ops->multadd)(mat->B,mat->lvec,bb,bb1);CHKERRQ(ierr);

      /* local sweep */
      ierr = (*mat->A->ops->sor)(mat->A,bb1,omega,SOR_SYMMETRIC_SWEEP,fshift,lits,1,xx);CHKERRQ(ierr);
    }
  } else if (flag & SOR_LOCAL_FORWARD_SWEEP) {
    if (flag & SOR_ZERO_INITIAL_GUESS) {
      ierr = (*mat->A->ops->sor)(mat->A,bb,omega,flag,fshift,lits,1,xx);CHKERRQ(ierr);
      its--;
    }
    while (its--) {
      ierr = VecScatterBegin(mat->Mvctx,xx,mat->lvec,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
      ierr = VecScatterEnd(mat->Mvctx,xx,mat->lvec,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);

      /* update rhs: bb1 = bb - B*x */
      ierr = VecScale(mat->lvec,-1.0);CHKERRQ(ierr);
      ierr = (*mat->B->ops->multadd)(mat->B,mat->lvec,bb,bb1);CHKERRQ(ierr);

      /* local sweep */
      ierr = (*mat->A->ops->sor)(mat->A,bb1,omega,SOR_FORWARD_SWEEP,fshift,lits,1,xx);CHKERRQ(ierr);
    }
  } else if (flag & SOR_LOCAL_BACKWARD_SWEEP) {
    if (flag & SOR_ZERO_INITIAL_GUESS) {
      ierr = (*mat->A->ops->sor)(mat->A,bb,omega,flag,fshift,lits,1,xx);CHKERRQ(ierr);
      its--;
    }
    while (its--) {
      ierr = VecScatterBegin(mat->Mvctx,xx,mat->lvec,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
      ierr = VecScatterEnd(mat->Mvctx,xx,mat->lvec,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);

      /* update rhs: bb1 = bb - B*x */
      ierr = VecScale(mat->lvec,-1.0);CHKERRQ(ierr);
      ierr = (*mat->B->ops->multadd)(mat->B,mat->lvec,bb,bb1);CHKERRQ(ierr);

      /* local sweep */
      ierr = (*mat->A->ops->sor)(mat->A,bb1,omega,SOR_BACKWARD_SWEEP,fshift,lits,1,xx);CHKERRQ(ierr);
    }
  } else if (flag & SOR_EISENSTAT) {
    Vec xx1;

    ierr = VecDuplicate(bb,&xx1);CHKERRQ(ierr);
    ierr = (*mat->A->ops->sor)(mat->A,bb,omega,(MatSORType)(SOR_ZERO_INITIAL_GUESS | SOR_LOCAL_BACKWARD_SWEEP),fshift,lits,1,xx);CHKERRQ(ierr);

    ierr = VecScatterBegin(mat->Mvctx,xx,mat->lvec,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
    ierr = VecScatterEnd(mat->Mvctx,xx,mat->lvec,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
    if (!mat->diag) {
      ierr = MatCreateVecs(matin,&mat->diag,NULL);CHKERRQ(ierr);
      ierr = MatGetDiagonal(matin,mat->diag);CHKERRQ(ierr);
    }
    ierr = MatHasOperation(matin,MATOP_MULT_DIAGONAL_BLOCK,&hasop);CHKERRQ(ierr);
    if (hasop) {
      ierr = MatMultDiagonalBlock(matin,xx,bb1);CHKERRQ(ierr);
    } else {
      ierr = VecPointwiseMult(bb1,mat->diag,xx);CHKERRQ(ierr);
    }
    ierr = VecAYPX(bb1,(omega-2.0)/omega,bb);CHKERRQ(ierr);

    ierr = MatMultAdd(mat->B,mat->lvec,bb1,bb1);CHKERRQ(ierr);

    /* local sweep */
    ierr = (*mat->A->ops->sor)(mat->A,bb1,omega,(MatSORType)(SOR_ZERO_INITIAL_GUESS | SOR_LOCAL_FORWARD_SWEEP),fshift,lits,1,xx1);CHKERRQ(ierr);
    ierr = VecAXPY(xx,1.0,xx1);CHKERRQ(ierr);
    ierr = VecDestroy(&xx1);CHKERRQ(ierr);
  } else SETERRQ(PetscObjectComm((PetscObject)matin),PETSC_ERR_SUP,"Parallel SOR not supported");

  ierr = VecDestroy(&bb1);CHKERRQ(ierr);

  matin->errortype = mat->A->errortype;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatPermute_MPIAIJ"
PetscErrorCode MatPermute_MPIAIJ(Mat A,IS rowp,IS colp,Mat *B)
{
  Mat            aA,aB,Aperm;
  const PetscInt *rwant,*cwant,*gcols,*ai,*bi,*aj,*bj;
  PetscScalar    *aa,*ba;
  PetscInt       i,j,m,n,ng,anz,bnz,*dnnz,*onnz,*tdnnz,*tonnz,*rdest,*cdest,*work,*gcdest;
  PetscSF        rowsf,sf;
  IS             parcolp = NULL;
  PetscBool      done;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = MatGetLocalSize(A,&m,&n);CHKERRQ(ierr);
  ierr = ISGetIndices(rowp,&rwant);CHKERRQ(ierr);
  ierr = ISGetIndices(colp,&cwant);CHKERRQ(ierr);
  ierr = PetscMalloc3(PetscMax(m,n),&work,m,&rdest,n,&cdest);CHKERRQ(ierr);

  /* Invert row permutation to find out where my rows should go */
  ierr = PetscSFCreate(PetscObjectComm((PetscObject)A),&rowsf);CHKERRQ(ierr);
  ierr = PetscSFSetGraphLayout(rowsf,A->rmap,A->rmap->n,NULL,PETSC_OWN_POINTER,rwant);CHKERRQ(ierr);
  ierr = PetscSFSetFromOptions(rowsf);CHKERRQ(ierr);
  for (i=0; i<m; i++) work[i] = A->rmap->rstart + i;
  ierr = PetscSFReduceBegin(rowsf,MPIU_INT,work,rdest,MPIU_REPLACE);CHKERRQ(ierr);
  ierr = PetscSFReduceEnd(rowsf,MPIU_INT,work,rdest,MPIU_REPLACE);CHKERRQ(ierr);

  /* Invert column permutation to find out where my columns should go */
  ierr = PetscSFCreate(PetscObjectComm((PetscObject)A),&sf);CHKERRQ(ierr);
  ierr = PetscSFSetGraphLayout(sf,A->cmap,A->cmap->n,NULL,PETSC_OWN_POINTER,cwant);CHKERRQ(ierr);
  ierr = PetscSFSetFromOptions(sf);CHKERRQ(ierr);
  for (i=0; i<n; i++) work[i] = A->cmap->rstart + i;
  ierr = PetscSFReduceBegin(sf,MPIU_INT,work,cdest,MPIU_REPLACE);CHKERRQ(ierr);
  ierr = PetscSFReduceEnd(sf,MPIU_INT,work,cdest,MPIU_REPLACE);CHKERRQ(ierr);
  ierr = PetscSFDestroy(&sf);CHKERRQ(ierr);

  ierr = ISRestoreIndices(rowp,&rwant);CHKERRQ(ierr);
  ierr = ISRestoreIndices(colp,&cwant);CHKERRQ(ierr);
  ierr = MatMPIAIJGetSeqAIJ(A,&aA,&aB,&gcols);CHKERRQ(ierr);

  /* Find out where my gcols should go */
  ierr = MatGetSize(aB,NULL,&ng);CHKERRQ(ierr);
  ierr = PetscMalloc1(ng,&gcdest);CHKERRQ(ierr);
  ierr = PetscSFCreate(PetscObjectComm((PetscObject)A),&sf);CHKERRQ(ierr);
  ierr = PetscSFSetGraphLayout(sf,A->cmap,ng,NULL,PETSC_OWN_POINTER,gcols);CHKERRQ(ierr);
  ierr = PetscSFSetFromOptions(sf);CHKERRQ(ierr);
  ierr = PetscSFBcastBegin(sf,MPIU_INT,cdest,gcdest);CHKERRQ(ierr);
  ierr = PetscSFBcastEnd(sf,MPIU_INT,cdest,gcdest);CHKERRQ(ierr);
  ierr = PetscSFDestroy(&sf);CHKERRQ(ierr);

  ierr = PetscCalloc4(m,&dnnz,m,&onnz,m,&tdnnz,m,&tonnz);CHKERRQ(ierr);
  ierr = MatGetRowIJ(aA,0,PETSC_FALSE,PETSC_FALSE,&anz,&ai,&aj,&done);CHKERRQ(ierr);
  ierr = MatGetRowIJ(aB,0,PETSC_FALSE,PETSC_FALSE,&bnz,&bi,&bj,&done);CHKERRQ(ierr);
  for (i=0; i<m; i++) {
    PetscInt row = rdest[i],rowner;
    ierr = PetscLayoutFindOwner(A->rmap,row,&rowner);CHKERRQ(ierr);
    for (j=ai[i]; j<ai[i+1]; j++) {
      PetscInt cowner,col = cdest[aj[j]];
      ierr = PetscLayoutFindOwner(A->cmap,col,&cowner);CHKERRQ(ierr); /* Could build an index for the columns to eliminate this search */
      if (rowner == cowner) dnnz[i]++;
      else onnz[i]++;
    }
    for (j=bi[i]; j<bi[i+1]; j++) {
      PetscInt cowner,col = gcdest[bj[j]];
      ierr = PetscLayoutFindOwner(A->cmap,col,&cowner);CHKERRQ(ierr);
      if (rowner == cowner) dnnz[i]++;
      else onnz[i]++;
    }
  }
  ierr = PetscSFBcastBegin(rowsf,MPIU_INT,dnnz,tdnnz);CHKERRQ(ierr);
  ierr = PetscSFBcastEnd(rowsf,MPIU_INT,dnnz,tdnnz);CHKERRQ(ierr);
  ierr = PetscSFBcastBegin(rowsf,MPIU_INT,onnz,tonnz);CHKERRQ(ierr);
  ierr = PetscSFBcastEnd(rowsf,MPIU_INT,onnz,tonnz);CHKERRQ(ierr);
  ierr = PetscSFDestroy(&rowsf);CHKERRQ(ierr);

  ierr = MatCreateAIJ(PetscObjectComm((PetscObject)A),A->rmap->n,A->cmap->n,A->rmap->N,A->cmap->N,0,tdnnz,0,tonnz,&Aperm);CHKERRQ(ierr);
  ierr = MatSeqAIJGetArray(aA,&aa);CHKERRQ(ierr);
  ierr = MatSeqAIJGetArray(aB,&ba);CHKERRQ(ierr);
  for (i=0; i<m; i++) {
    PetscInt *acols = dnnz,*bcols = onnz; /* Repurpose now-unneeded arrays */
    PetscInt j0,rowlen;
    rowlen = ai[i+1] - ai[i];
    for (j0=j=0; j<rowlen; j0=j) { /* rowlen could be larger than number of rows m, so sum in batches */
      for ( ; j<PetscMin(rowlen,j0+m); j++) acols[j-j0] = cdest[aj[ai[i]+j]];
      ierr = MatSetValues(Aperm,1,&rdest[i],j-j0,acols,aa+ai[i]+j0,INSERT_VALUES);CHKERRQ(ierr);
    }
    rowlen = bi[i+1] - bi[i];
    for (j0=j=0; j<rowlen; j0=j) {
      for ( ; j<PetscMin(rowlen,j0+m); j++) bcols[j-j0] = gcdest[bj[bi[i]+j]];
      ierr = MatSetValues(Aperm,1,&rdest[i],j-j0,bcols,ba+bi[i]+j0,INSERT_VALUES);CHKERRQ(ierr);
    }
  }
  ierr = MatAssemblyBegin(Aperm,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(Aperm,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatRestoreRowIJ(aA,0,PETSC_FALSE,PETSC_FALSE,&anz,&ai,&aj,&done);CHKERRQ(ierr);
  ierr = MatRestoreRowIJ(aB,0,PETSC_FALSE,PETSC_FALSE,&bnz,&bi,&bj,&done);CHKERRQ(ierr);
  ierr = MatSeqAIJRestoreArray(aA,&aa);CHKERRQ(ierr);
  ierr = MatSeqAIJRestoreArray(aB,&ba);CHKERRQ(ierr);
  ierr = PetscFree4(dnnz,onnz,tdnnz,tonnz);CHKERRQ(ierr);
  ierr = PetscFree3(work,rdest,cdest);CHKERRQ(ierr);
  ierr = PetscFree(gcdest);CHKERRQ(ierr);
  if (parcolp) {ierr = ISDestroy(&colp);CHKERRQ(ierr);}
  *B = Aperm;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatGetGhosts_MPIAIJ"
PetscErrorCode  MatGetGhosts_MPIAIJ(Mat mat,PetscInt *nghosts,const PetscInt *ghosts[])
{
  Mat_MPIAIJ *aij = (Mat_MPIAIJ*)mat->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = MatGetSize(aij->B,NULL,nghosts);CHKERRQ(ierr);
  if (ghosts) *ghosts = aij->garray;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatGetInfo_MPIAIJ"
PetscErrorCode MatGetInfo_MPIAIJ(Mat matin,MatInfoType flag,MatInfo *info)
{
  Mat_MPIAIJ     *mat = (Mat_MPIAIJ*)matin->data;
  Mat            A    = mat->A,B = mat->B;
  PetscErrorCode ierr;
  PetscReal      isend[5],irecv[5];

  PetscFunctionBegin;
  info->block_size = 1.0;
  ierr             = MatGetInfo(A,MAT_LOCAL,info);CHKERRQ(ierr);

  isend[0] = info->nz_used; isend[1] = info->nz_allocated; isend[2] = info->nz_unneeded;
  isend[3] = info->memory;  isend[4] = info->mallocs;

  ierr = MatGetInfo(B,MAT_LOCAL,info);CHKERRQ(ierr);

  isend[0] += info->nz_used; isend[1] += info->nz_allocated; isend[2] += info->nz_unneeded;
  isend[3] += info->memory;  isend[4] += info->mallocs;
  if (flag == MAT_LOCAL) {
    info->nz_used      = isend[0];
    info->nz_allocated = isend[1];
    info->nz_unneeded  = isend[2];
    info->memory       = isend[3];
    info->mallocs      = isend[4];
  } else if (flag == MAT_GLOBAL_MAX) {
    ierr = MPIU_Allreduce(isend,irecv,5,MPIU_REAL,MPIU_MAX,PetscObjectComm((PetscObject)matin));CHKERRQ(ierr);

    info->nz_used      = irecv[0];
    info->nz_allocated = irecv[1];
    info->nz_unneeded  = irecv[2];
    info->memory       = irecv[3];
    info->mallocs      = irecv[4];
  } else if (flag == MAT_GLOBAL_SUM) {
    ierr = MPIU_Allreduce(isend,irecv,5,MPIU_REAL,MPIU_SUM,PetscObjectComm((PetscObject)matin));CHKERRQ(ierr);

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
#define __FUNCT__ "MatSetOption_MPIAIJ"
PetscErrorCode MatSetOption_MPIAIJ(Mat A,MatOption op,PetscBool flg)
{
  Mat_MPIAIJ     *a = (Mat_MPIAIJ*)A->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  switch (op) {
  case MAT_NEW_NONZERO_LOCATIONS:
  case MAT_NEW_NONZERO_ALLOCATION_ERR:
  case MAT_UNUSED_NONZERO_LOCATION_ERR:
  case MAT_KEEP_NONZERO_PATTERN:
  case MAT_NEW_NONZERO_LOCATION_ERR:
  case MAT_USE_INODES:
  case MAT_IGNORE_ZERO_ENTRIES:
    MatCheckPreallocated(A,1);
    ierr = MatSetOption(a->A,op,flg);CHKERRQ(ierr);
    ierr = MatSetOption(a->B,op,flg);CHKERRQ(ierr);
    break;
  case MAT_ROW_ORIENTED:
    a->roworiented = flg;

    ierr = MatSetOption(a->A,op,flg);CHKERRQ(ierr);
    ierr = MatSetOption(a->B,op,flg);CHKERRQ(ierr);
    break;
  case MAT_NEW_DIAGONALS:
    ierr = PetscInfo1(A,"Option %s ignored\n",MatOptions[op]);CHKERRQ(ierr);
    break;
  case MAT_IGNORE_OFF_PROC_ENTRIES:
    a->donotstash = flg;
    break;
  case MAT_SPD:
    A->spd_set = PETSC_TRUE;
    A->spd     = flg;
    if (flg) {
      A->symmetric                  = PETSC_TRUE;
      A->structurally_symmetric     = PETSC_TRUE;
      A->symmetric_set              = PETSC_TRUE;
      A->structurally_symmetric_set = PETSC_TRUE;
    }
    break;
  case MAT_SYMMETRIC:
    ierr = MatSetOption(a->A,op,flg);CHKERRQ(ierr);
    break;
  case MAT_STRUCTURALLY_SYMMETRIC:
    ierr = MatSetOption(a->A,op,flg);CHKERRQ(ierr);
    break;
  case MAT_HERMITIAN:
    ierr = MatSetOption(a->A,op,flg);CHKERRQ(ierr);
    break;
  case MAT_SYMMETRY_ETERNAL:
    ierr = MatSetOption(a->A,op,flg);CHKERRQ(ierr);
    break;
  default:
    SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_SUP,"unknown option %d",op);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatGetRow_MPIAIJ"
PetscErrorCode MatGetRow_MPIAIJ(Mat matin,PetscInt row,PetscInt *nz,PetscInt **idx,PetscScalar **v)
{
  Mat_MPIAIJ     *mat = (Mat_MPIAIJ*)matin->data;
  PetscScalar    *vworkA,*vworkB,**pvA,**pvB,*v_p;
  PetscErrorCode ierr;
  PetscInt       i,*cworkA,*cworkB,**pcA,**pcB,cstart = matin->cmap->rstart;
  PetscInt       nztot,nzA,nzB,lrow,rstart = matin->rmap->rstart,rend = matin->rmap->rend;
  PetscInt       *cmap,*idx_p;

  PetscFunctionBegin;
  if (mat->getrowactive) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONGSTATE,"Already active");
  mat->getrowactive = PETSC_TRUE;

  if (!mat->rowvalues && (idx || v)) {
    /*
        allocate enough space to hold information from the longest row.
    */
    Mat_SeqAIJ *Aa = (Mat_SeqAIJ*)mat->A->data,*Ba = (Mat_SeqAIJ*)mat->B->data;
    PetscInt   max = 1,tmp;
    for (i=0; i<matin->rmap->n; i++) {
      tmp = Aa->i[i+1] - Aa->i[i] + Ba->i[i+1] - Ba->i[i];
      if (max < tmp) max = tmp;
    }
    ierr = PetscMalloc2(max,&mat->rowvalues,max,&mat->rowindices);CHKERRQ(ierr);
  }

  if (row < rstart || row >= rend) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Only local rows");
  lrow = row - rstart;

  pvA = &vworkA; pcA = &cworkA; pvB = &vworkB; pcB = &cworkB;
  if (!v)   {pvA = 0; pvB = 0;}
  if (!idx) {pcA = 0; if (!v) pcB = 0;}
  ierr  = (*mat->A->ops->getrow)(mat->A,lrow,&nzA,pcA,pvA);CHKERRQ(ierr);
  ierr  = (*mat->B->ops->getrow)(mat->B,lrow,&nzB,pcB,pvB);CHKERRQ(ierr);
  nztot = nzA + nzB;

  cmap = mat->garray;
  if (v  || idx) {
    if (nztot) {
      /* Sort by increasing column numbers, assuming A and B already sorted */
      PetscInt imark = -1;
      if (v) {
        *v = v_p = mat->rowvalues;
        for (i=0; i<nzB; i++) {
          if (cmap[cworkB[i]] < cstart) v_p[i] = vworkB[i];
          else break;
        }
        imark = i;
        for (i=0; i<nzA; i++)     v_p[imark+i] = vworkA[i];
        for (i=imark; i<nzB; i++) v_p[nzA+i]   = vworkB[i];
      }
      if (idx) {
        *idx = idx_p = mat->rowindices;
        if (imark > -1) {
          for (i=0; i<imark; i++) {
            idx_p[i] = cmap[cworkB[i]];
          }
        } else {
          for (i=0; i<nzB; i++) {
            if (cmap[cworkB[i]] < cstart) idx_p[i] = cmap[cworkB[i]];
            else break;
          }
          imark = i;
        }
        for (i=0; i<nzA; i++)     idx_p[imark+i] = cstart + cworkA[i];
        for (i=imark; i<nzB; i++) idx_p[nzA+i]   = cmap[cworkB[i]];
      }
    } else {
      if (idx) *idx = 0;
      if (v)   *v   = 0;
    }
  }
  *nz  = nztot;
  ierr = (*mat->A->ops->restorerow)(mat->A,lrow,&nzA,pcA,pvA);CHKERRQ(ierr);
  ierr = (*mat->B->ops->restorerow)(mat->B,lrow,&nzB,pcB,pvB);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatRestoreRow_MPIAIJ"
PetscErrorCode MatRestoreRow_MPIAIJ(Mat mat,PetscInt row,PetscInt *nz,PetscInt **idx,PetscScalar **v)
{
  Mat_MPIAIJ *aij = (Mat_MPIAIJ*)mat->data;

  PetscFunctionBegin;
  if (!aij->getrowactive) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONGSTATE,"MatGetRow() must be called first");
  aij->getrowactive = PETSC_FALSE;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatNorm_MPIAIJ"
PetscErrorCode MatNorm_MPIAIJ(Mat mat,NormType type,PetscReal *norm)
{
  Mat_MPIAIJ     *aij  = (Mat_MPIAIJ*)mat->data;
  Mat_SeqAIJ     *amat = (Mat_SeqAIJ*)aij->A->data,*bmat = (Mat_SeqAIJ*)aij->B->data;
  PetscErrorCode ierr;
  PetscInt       i,j,cstart = mat->cmap->rstart;
  PetscReal      sum = 0.0;
  MatScalar      *v;

  PetscFunctionBegin;
  if (aij->size == 1) {
    ierr =  MatNorm(aij->A,type,norm);CHKERRQ(ierr);
  } else {
    if (type == NORM_FROBENIUS) {
      v = amat->a;
      for (i=0; i<amat->nz; i++) {
        sum += PetscRealPart(PetscConj(*v)*(*v)); v++;
      }
      v = bmat->a;
      for (i=0; i<bmat->nz; i++) {
        sum += PetscRealPart(PetscConj(*v)*(*v)); v++;
      }
      ierr  = MPIU_Allreduce(&sum,norm,1,MPIU_REAL,MPIU_SUM,PetscObjectComm((PetscObject)mat));CHKERRQ(ierr);
      *norm = PetscSqrtReal(*norm);
    } else if (type == NORM_1) { /* max column norm */
      PetscReal *tmp,*tmp2;
      PetscInt  *jj,*garray = aij->garray;
      ierr  = PetscCalloc1(mat->cmap->N+1,&tmp);CHKERRQ(ierr);
      ierr  = PetscMalloc1(mat->cmap->N+1,&tmp2);CHKERRQ(ierr);
      *norm = 0.0;
      v     = amat->a; jj = amat->j;
      for (j=0; j<amat->nz; j++) {
        tmp[cstart + *jj++] += PetscAbsScalar(*v);  v++;
      }
      v = bmat->a; jj = bmat->j;
      for (j=0; j<bmat->nz; j++) {
        tmp[garray[*jj++]] += PetscAbsScalar(*v); v++;
      }
      ierr = MPIU_Allreduce(tmp,tmp2,mat->cmap->N,MPIU_REAL,MPIU_SUM,PetscObjectComm((PetscObject)mat));CHKERRQ(ierr);
      for (j=0; j<mat->cmap->N; j++) {
        if (tmp2[j] > *norm) *norm = tmp2[j];
      }
      ierr = PetscFree(tmp);CHKERRQ(ierr);
      ierr = PetscFree(tmp2);CHKERRQ(ierr);
    } else if (type == NORM_INFINITY) { /* max row norm */
      PetscReal ntemp = 0.0;
      for (j=0; j<aij->A->rmap->n; j++) {
        v   = amat->a + amat->i[j];
        sum = 0.0;
        for (i=0; i<amat->i[j+1]-amat->i[j]; i++) {
          sum += PetscAbsScalar(*v); v++;
        }
        v = bmat->a + bmat->i[j];
        for (i=0; i<bmat->i[j+1]-bmat->i[j]; i++) {
          sum += PetscAbsScalar(*v); v++;
        }
        if (sum > ntemp) ntemp = sum;
      }
      ierr = MPIU_Allreduce(&ntemp,norm,1,MPIU_REAL,MPIU_MAX,PetscObjectComm((PetscObject)mat));CHKERRQ(ierr);
    } else SETERRQ(PetscObjectComm((PetscObject)mat),PETSC_ERR_SUP,"No support for two norm");
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatTranspose_MPIAIJ"
PetscErrorCode MatTranspose_MPIAIJ(Mat A,MatReuse reuse,Mat *matout)
{
  Mat_MPIAIJ     *a   = (Mat_MPIAIJ*)A->data;
  Mat_SeqAIJ     *Aloc=(Mat_SeqAIJ*)a->A->data,*Bloc=(Mat_SeqAIJ*)a->B->data;
  PetscErrorCode ierr;
  PetscInt       M      = A->rmap->N,N = A->cmap->N,ma,na,mb,nb,*ai,*aj,*bi,*bj,row,*cols,*cols_tmp,i;
  PetscInt       cstart = A->cmap->rstart,ncol;
  Mat            B;
  MatScalar      *array;

  PetscFunctionBegin;
  if (reuse == MAT_REUSE_MATRIX && A == *matout && M != N) SETERRQ(PetscObjectComm((PetscObject)A),PETSC_ERR_ARG_SIZ,"Square matrix only for in-place");

  ma = A->rmap->n; na = A->cmap->n; mb = a->B->rmap->n; nb = a->B->cmap->n;
  ai = Aloc->i; aj = Aloc->j;
  bi = Bloc->i; bj = Bloc->j;
  if (reuse == MAT_INITIAL_MATRIX || *matout == A) {
    PetscInt             *d_nnz,*g_nnz,*o_nnz;
    PetscSFNode          *oloc;
    PETSC_UNUSED PetscSF sf;

    ierr = PetscMalloc4(na,&d_nnz,na,&o_nnz,nb,&g_nnz,nb,&oloc);CHKERRQ(ierr);
    /* compute d_nnz for preallocation */
    ierr = PetscMemzero(d_nnz,na*sizeof(PetscInt));CHKERRQ(ierr);
    for (i=0; i<ai[ma]; i++) {
      d_nnz[aj[i]]++;
      aj[i] += cstart; /* global col index to be used by MatSetValues() */
    }
    /* compute local off-diagonal contributions */
    ierr = PetscMemzero(g_nnz,nb*sizeof(PetscInt));CHKERRQ(ierr);
    for (i=0; i<bi[ma]; i++) g_nnz[bj[i]]++;
    /* map those to global */
    ierr = PetscSFCreate(PetscObjectComm((PetscObject)A),&sf);CHKERRQ(ierr);
    ierr = PetscSFSetGraphLayout(sf,A->cmap,nb,NULL,PETSC_USE_POINTER,a->garray);CHKERRQ(ierr);
    ierr = PetscSFSetFromOptions(sf);CHKERRQ(ierr);
    ierr = PetscMemzero(o_nnz,na*sizeof(PetscInt));CHKERRQ(ierr);
    ierr = PetscSFReduceBegin(sf,MPIU_INT,g_nnz,o_nnz,MPIU_SUM);CHKERRQ(ierr);
    ierr = PetscSFReduceEnd(sf,MPIU_INT,g_nnz,o_nnz,MPIU_SUM);CHKERRQ(ierr);
    ierr = PetscSFDestroy(&sf);CHKERRQ(ierr);

    ierr = MatCreate(PetscObjectComm((PetscObject)A),&B);CHKERRQ(ierr);
    ierr = MatSetSizes(B,A->cmap->n,A->rmap->n,N,M);CHKERRQ(ierr);
    ierr = MatSetBlockSizes(B,PetscAbs(A->cmap->bs),PetscAbs(A->rmap->bs));CHKERRQ(ierr);
    ierr = MatSetType(B,((PetscObject)A)->type_name);CHKERRQ(ierr);
    ierr = MatMPIAIJSetPreallocation(B,0,d_nnz,0,o_nnz);CHKERRQ(ierr);
    ierr = PetscFree4(d_nnz,o_nnz,g_nnz,oloc);CHKERRQ(ierr);
  } else {
    B    = *matout;
    ierr = MatSetOption(B,MAT_NEW_NONZERO_ALLOCATION_ERR,PETSC_TRUE);CHKERRQ(ierr);
    for (i=0; i<ai[ma]; i++) aj[i] += cstart; /* global col index to be used by MatSetValues() */
  }

  /* copy over the A part */
  array = Aloc->a;
  row   = A->rmap->rstart;
  for (i=0; i<ma; i++) {
    ncol = ai[i+1]-ai[i];
    ierr = MatSetValues(B,ncol,aj,1,&row,array,INSERT_VALUES);CHKERRQ(ierr);
    row++;
    array += ncol; aj += ncol;
  }
  aj = Aloc->j;
  for (i=0; i<ai[ma]; i++) aj[i] -= cstart; /* resume local col index */

  /* copy over the B part */
  ierr  = PetscCalloc1(bi[mb],&cols);CHKERRQ(ierr);
  array = Bloc->a;
  row   = A->rmap->rstart;
  for (i=0; i<bi[mb]; i++) cols[i] = a->garray[bj[i]];
  cols_tmp = cols;
  for (i=0; i<mb; i++) {
    ncol = bi[i+1]-bi[i];
    ierr = MatSetValues(B,ncol,cols_tmp,1,&row,array,INSERT_VALUES);CHKERRQ(ierr);
    row++;
    array += ncol; cols_tmp += ncol;
  }
  ierr = PetscFree(cols);CHKERRQ(ierr);

  ierr = MatAssemblyBegin(B,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(B,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  if (reuse == MAT_INITIAL_MATRIX || *matout != A) {
    *matout = B;
  } else {
    ierr = MatHeaderMerge(A,&B);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatDiagonalScale_MPIAIJ"
PetscErrorCode MatDiagonalScale_MPIAIJ(Mat mat,Vec ll,Vec rr)
{
  Mat_MPIAIJ     *aij = (Mat_MPIAIJ*)mat->data;
  Mat            a    = aij->A,b = aij->B;
  PetscErrorCode ierr;
  PetscInt       s1,s2,s3;

  PetscFunctionBegin;
  ierr = MatGetLocalSize(mat,&s2,&s3);CHKERRQ(ierr);
  if (rr) {
    ierr = VecGetLocalSize(rr,&s1);CHKERRQ(ierr);
    if (s1!=s3) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_SIZ,"right vector non-conforming local size");
    /* Overlap communication with computation. */
    ierr = VecScatterBegin(aij->Mvctx,rr,aij->lvec,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
  }
  if (ll) {
    ierr = VecGetLocalSize(ll,&s1);CHKERRQ(ierr);
    if (s1!=s2) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_SIZ,"left vector non-conforming local size");
    ierr = (*b->ops->diagonalscale)(b,ll,0);CHKERRQ(ierr);
  }
  /* scale  the diagonal block */
  ierr = (*a->ops->diagonalscale)(a,ll,rr);CHKERRQ(ierr);

  if (rr) {
    /* Do a scatter end and then right scale the off-diagonal block */
    ierr = VecScatterEnd(aij->Mvctx,rr,aij->lvec,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
    ierr = (*b->ops->diagonalscale)(b,0,aij->lvec);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatSetUnfactored_MPIAIJ"
PetscErrorCode MatSetUnfactored_MPIAIJ(Mat A)
{
  Mat_MPIAIJ     *a = (Mat_MPIAIJ*)A->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = MatSetUnfactored(a->A);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatEqual_MPIAIJ"
PetscErrorCode MatEqual_MPIAIJ(Mat A,Mat B,PetscBool  *flag)
{
  Mat_MPIAIJ     *matB = (Mat_MPIAIJ*)B->data,*matA = (Mat_MPIAIJ*)A->data;
  Mat            a,b,c,d;
  PetscBool      flg;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  a = matA->A; b = matA->B;
  c = matB->A; d = matB->B;

  ierr = MatEqual(a,c,&flg);CHKERRQ(ierr);
  if (flg) {
    ierr = MatEqual(b,d,&flg);CHKERRQ(ierr);
  }
  ierr = MPIU_Allreduce(&flg,flag,1,MPIU_BOOL,MPI_LAND,PetscObjectComm((PetscObject)A));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatCopy_MPIAIJ"
PetscErrorCode MatCopy_MPIAIJ(Mat A,Mat B,MatStructure str)
{
  PetscErrorCode ierr;
  Mat_MPIAIJ     *a = (Mat_MPIAIJ*)A->data;
  Mat_MPIAIJ     *b = (Mat_MPIAIJ*)B->data;

  PetscFunctionBegin;
  /* If the two matrices don't have the same copy implementation, they aren't compatible for fast copy. */
  if ((str != SAME_NONZERO_PATTERN) || (A->ops->copy != B->ops->copy)) {
    /* because of the column compression in the off-processor part of the matrix a->B,
       the number of columns in a->B and b->B may be different, hence we cannot call
       the MatCopy() directly on the two parts. If need be, we can provide a more
       efficient copy than the MatCopy_Basic() by first uncompressing the a->B matrices
       then copying the submatrices */
    ierr = MatCopy_Basic(A,B,str);CHKERRQ(ierr);
  } else {
    ierr = MatCopy(a->A,b->A,str);CHKERRQ(ierr);
    ierr = MatCopy(a->B,b->B,str);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatSetUp_MPIAIJ"
PetscErrorCode MatSetUp_MPIAIJ(Mat A)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr =  MatMPIAIJSetPreallocation(A,PETSC_DEFAULT,0,PETSC_DEFAULT,0);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/*
   Computes the number of nonzeros per row needed for preallocation when X and Y
   have different nonzero structure.
*/
#undef __FUNCT__
#define __FUNCT__ "MatAXPYGetPreallocation_MPIX_private"
PetscErrorCode MatAXPYGetPreallocation_MPIX_private(PetscInt m,const PetscInt *xi,const PetscInt *xj,const PetscInt *xltog,const PetscInt *yi,const PetscInt *yj,const PetscInt *yltog,PetscInt *nnz)
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
      for (; k<nzy && yltog[yjj[k]]<xltog[xjj[j]]; k++) nnz[i]++; /* Catch up to X */
      if (k<nzy && yltog[yjj[k]]==xltog[xjj[j]]) k++;             /* Skip duplicate */
      nnz[i]++;
    }
    for (; k<nzy; k++) nnz[i]++;
  }
  PetscFunctionReturn(0);
}

/* This is the same as MatAXPYGetPreallocation_SeqAIJ, except that the local-to-global map is provided */
#undef __FUNCT__
#define __FUNCT__ "MatAXPYGetPreallocation_MPIAIJ"
static PetscErrorCode MatAXPYGetPreallocation_MPIAIJ(Mat Y,const PetscInt *yltog,Mat X,const PetscInt *xltog,PetscInt *nnz)
{
  PetscErrorCode ierr;
  PetscInt       m = Y->rmap->N;
  Mat_SeqAIJ     *x = (Mat_SeqAIJ*)X->data;
  Mat_SeqAIJ     *y = (Mat_SeqAIJ*)Y->data;

  PetscFunctionBegin;
  ierr = MatAXPYGetPreallocation_MPIX_private(m,x->i,x->j,xltog,y->i,y->j,yltog,nnz);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatAXPY_MPIAIJ"
PetscErrorCode MatAXPY_MPIAIJ(Mat Y,PetscScalar a,Mat X,MatStructure str)
{
  PetscErrorCode ierr;
  Mat_MPIAIJ     *xx = (Mat_MPIAIJ*)X->data,*yy = (Mat_MPIAIJ*)Y->data;
  PetscBLASInt   bnz,one=1;
  Mat_SeqAIJ     *x,*y;

  PetscFunctionBegin;
  if (str == SAME_NONZERO_PATTERN) {
    PetscScalar alpha = a;
    x    = (Mat_SeqAIJ*)xx->A->data;
    ierr = PetscBLASIntCast(x->nz,&bnz);CHKERRQ(ierr);
    y    = (Mat_SeqAIJ*)yy->A->data;
    PetscStackCallBLAS("BLASaxpy",BLASaxpy_(&bnz,&alpha,x->a,&one,y->a,&one));
    x    = (Mat_SeqAIJ*)xx->B->data;
    y    = (Mat_SeqAIJ*)yy->B->data;
    ierr = PetscBLASIntCast(x->nz,&bnz);CHKERRQ(ierr);
    PetscStackCallBLAS("BLASaxpy",BLASaxpy_(&bnz,&alpha,x->a,&one,y->a,&one));
    ierr = PetscObjectStateIncrease((PetscObject)Y);CHKERRQ(ierr);
  } else if (str == SUBSET_NONZERO_PATTERN) { /* nonzeros of X is a subset of Y's */
    ierr = MatAXPY_Basic(Y,a,X,str);CHKERRQ(ierr);
  } else {
    Mat      B;
    PetscInt *nnz_d,*nnz_o;
    ierr = PetscMalloc1(yy->A->rmap->N,&nnz_d);CHKERRQ(ierr);
    ierr = PetscMalloc1(yy->B->rmap->N,&nnz_o);CHKERRQ(ierr);
    ierr = MatCreate(PetscObjectComm((PetscObject)Y),&B);CHKERRQ(ierr);
    ierr = PetscObjectSetName((PetscObject)B,((PetscObject)Y)->name);CHKERRQ(ierr);
    ierr = MatSetSizes(B,Y->rmap->n,Y->cmap->n,Y->rmap->N,Y->cmap->N);CHKERRQ(ierr);
    ierr = MatSetBlockSizesFromMats(B,Y,Y);CHKERRQ(ierr);
    ierr = MatSetType(B,MATMPIAIJ);CHKERRQ(ierr);
    ierr = MatAXPYGetPreallocation_SeqAIJ(yy->A,xx->A,nnz_d);CHKERRQ(ierr);
    ierr = MatAXPYGetPreallocation_MPIAIJ(yy->B,yy->garray,xx->B,xx->garray,nnz_o);CHKERRQ(ierr);
    ierr = MatMPIAIJSetPreallocation(B,0,nnz_d,0,nnz_o);CHKERRQ(ierr);
    ierr = MatAXPY_BasicWithPreallocation(B,Y,a,X,str);CHKERRQ(ierr);
    ierr = MatHeaderReplace(Y,&B);CHKERRQ(ierr);
    ierr = PetscFree(nnz_d);CHKERRQ(ierr);
    ierr = PetscFree(nnz_o);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

extern PetscErrorCode  MatConjugate_SeqAIJ(Mat);

#undef __FUNCT__
#define __FUNCT__ "MatConjugate_MPIAIJ"
PetscErrorCode  MatConjugate_MPIAIJ(Mat mat)
{
#if defined(PETSC_USE_COMPLEX)
  PetscErrorCode ierr;
  Mat_MPIAIJ     *aij = (Mat_MPIAIJ*)mat->data;

  PetscFunctionBegin;
  ierr = MatConjugate_SeqAIJ(aij->A);CHKERRQ(ierr);
  ierr = MatConjugate_SeqAIJ(aij->B);CHKERRQ(ierr);
#else
  PetscFunctionBegin;
#endif
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatRealPart_MPIAIJ"
PetscErrorCode MatRealPart_MPIAIJ(Mat A)
{
  Mat_MPIAIJ     *a = (Mat_MPIAIJ*)A->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = MatRealPart(a->A);CHKERRQ(ierr);
  ierr = MatRealPart(a->B);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatImaginaryPart_MPIAIJ"
PetscErrorCode MatImaginaryPart_MPIAIJ(Mat A)
{
  Mat_MPIAIJ     *a = (Mat_MPIAIJ*)A->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = MatImaginaryPart(a->A);CHKERRQ(ierr);
  ierr = MatImaginaryPart(a->B);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatGetRowMaxAbs_MPIAIJ"
PetscErrorCode MatGetRowMaxAbs_MPIAIJ(Mat A, Vec v, PetscInt idx[])
{
  Mat_MPIAIJ     *a = (Mat_MPIAIJ*)A->data;
  PetscErrorCode ierr;
  PetscInt       i,*idxb = 0;
  PetscScalar    *va,*vb;
  Vec            vtmp;

  PetscFunctionBegin;
  ierr = MatGetRowMaxAbs(a->A,v,idx);CHKERRQ(ierr);
  ierr = VecGetArray(v,&va);CHKERRQ(ierr);
  if (idx) {
    for (i=0; i<A->rmap->n; i++) {
      if (PetscAbsScalar(va[i])) idx[i] += A->cmap->rstart;
    }
  }

  ierr = VecCreateSeq(PETSC_COMM_SELF,A->rmap->n,&vtmp);CHKERRQ(ierr);
  if (idx) {
    ierr = PetscMalloc1(A->rmap->n,&idxb);CHKERRQ(ierr);
  }
  ierr = MatGetRowMaxAbs(a->B,vtmp,idxb);CHKERRQ(ierr);
  ierr = VecGetArray(vtmp,&vb);CHKERRQ(ierr);

  for (i=0; i<A->rmap->n; i++) {
    if (PetscAbsScalar(va[i]) < PetscAbsScalar(vb[i])) {
      va[i] = vb[i];
      if (idx) idx[i] = a->garray[idxb[i]];
    }
  }

  ierr = VecRestoreArray(v,&va);CHKERRQ(ierr);
  ierr = VecRestoreArray(vtmp,&vb);CHKERRQ(ierr);
  ierr = PetscFree(idxb);CHKERRQ(ierr);
  ierr = VecDestroy(&vtmp);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatGetRowMinAbs_MPIAIJ"
PetscErrorCode MatGetRowMinAbs_MPIAIJ(Mat A, Vec v, PetscInt idx[])
{
  Mat_MPIAIJ     *a = (Mat_MPIAIJ*)A->data;
  PetscErrorCode ierr;
  PetscInt       i,*idxb = 0;
  PetscScalar    *va,*vb;
  Vec            vtmp;

  PetscFunctionBegin;
  ierr = MatGetRowMinAbs(a->A,v,idx);CHKERRQ(ierr);
  ierr = VecGetArray(v,&va);CHKERRQ(ierr);
  if (idx) {
    for (i=0; i<A->cmap->n; i++) {
      if (PetscAbsScalar(va[i])) idx[i] += A->cmap->rstart;
    }
  }

  ierr = VecCreateSeq(PETSC_COMM_SELF,A->rmap->n,&vtmp);CHKERRQ(ierr);
  if (idx) {
    ierr = PetscMalloc1(A->rmap->n,&idxb);CHKERRQ(ierr);
  }
  ierr = MatGetRowMinAbs(a->B,vtmp,idxb);CHKERRQ(ierr);
  ierr = VecGetArray(vtmp,&vb);CHKERRQ(ierr);

  for (i=0; i<A->rmap->n; i++) {
    if (PetscAbsScalar(va[i]) > PetscAbsScalar(vb[i])) {
      va[i] = vb[i];
      if (idx) idx[i] = a->garray[idxb[i]];
    }
  }

  ierr = VecRestoreArray(v,&va);CHKERRQ(ierr);
  ierr = VecRestoreArray(vtmp,&vb);CHKERRQ(ierr);
  ierr = PetscFree(idxb);CHKERRQ(ierr);
  ierr = VecDestroy(&vtmp);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatGetRowMin_MPIAIJ"
PetscErrorCode MatGetRowMin_MPIAIJ(Mat A, Vec v, PetscInt idx[])
{
  Mat_MPIAIJ     *mat   = (Mat_MPIAIJ*) A->data;
  PetscInt       n      = A->rmap->n;
  PetscInt       cstart = A->cmap->rstart;
  PetscInt       *cmap  = mat->garray;
  PetscInt       *diagIdx, *offdiagIdx;
  Vec            diagV, offdiagV;
  PetscScalar    *a, *diagA, *offdiagA;
  PetscInt       r;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscMalloc2(n,&diagIdx,n,&offdiagIdx);CHKERRQ(ierr);
  ierr = VecCreateSeq(PetscObjectComm((PetscObject)A), n, &diagV);CHKERRQ(ierr);
  ierr = VecCreateSeq(PetscObjectComm((PetscObject)A), n, &offdiagV);CHKERRQ(ierr);
  ierr = MatGetRowMin(mat->A, diagV,    diagIdx);CHKERRQ(ierr);
  ierr = MatGetRowMin(mat->B, offdiagV, offdiagIdx);CHKERRQ(ierr);
  ierr = VecGetArray(v,        &a);CHKERRQ(ierr);
  ierr = VecGetArray(diagV,    &diagA);CHKERRQ(ierr);
  ierr = VecGetArray(offdiagV, &offdiagA);CHKERRQ(ierr);
  for (r = 0; r < n; ++r) {
    if (PetscAbsScalar(diagA[r]) <= PetscAbsScalar(offdiagA[r])) {
      a[r]   = diagA[r];
      idx[r] = cstart + diagIdx[r];
    } else {
      a[r]   = offdiagA[r];
      idx[r] = cmap[offdiagIdx[r]];
    }
  }
  ierr = VecRestoreArray(v,        &a);CHKERRQ(ierr);
  ierr = VecRestoreArray(diagV,    &diagA);CHKERRQ(ierr);
  ierr = VecRestoreArray(offdiagV, &offdiagA);CHKERRQ(ierr);
  ierr = VecDestroy(&diagV);CHKERRQ(ierr);
  ierr = VecDestroy(&offdiagV);CHKERRQ(ierr);
  ierr = PetscFree2(diagIdx, offdiagIdx);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatGetRowMax_MPIAIJ"
PetscErrorCode MatGetRowMax_MPIAIJ(Mat A, Vec v, PetscInt idx[])
{
  Mat_MPIAIJ     *mat   = (Mat_MPIAIJ*) A->data;
  PetscInt       n      = A->rmap->n;
  PetscInt       cstart = A->cmap->rstart;
  PetscInt       *cmap  = mat->garray;
  PetscInt       *diagIdx, *offdiagIdx;
  Vec            diagV, offdiagV;
  PetscScalar    *a, *diagA, *offdiagA;
  PetscInt       r;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscMalloc2(n,&diagIdx,n,&offdiagIdx);CHKERRQ(ierr);
  ierr = VecCreateSeq(PETSC_COMM_SELF, n, &diagV);CHKERRQ(ierr);
  ierr = VecCreateSeq(PETSC_COMM_SELF, n, &offdiagV);CHKERRQ(ierr);
  ierr = MatGetRowMax(mat->A, diagV,    diagIdx);CHKERRQ(ierr);
  ierr = MatGetRowMax(mat->B, offdiagV, offdiagIdx);CHKERRQ(ierr);
  ierr = VecGetArray(v,        &a);CHKERRQ(ierr);
  ierr = VecGetArray(diagV,    &diagA);CHKERRQ(ierr);
  ierr = VecGetArray(offdiagV, &offdiagA);CHKERRQ(ierr);
  for (r = 0; r < n; ++r) {
    if (PetscAbsScalar(diagA[r]) >= PetscAbsScalar(offdiagA[r])) {
      a[r]   = diagA[r];
      idx[r] = cstart + diagIdx[r];
    } else {
      a[r]   = offdiagA[r];
      idx[r] = cmap[offdiagIdx[r]];
    }
  }
  ierr = VecRestoreArray(v,        &a);CHKERRQ(ierr);
  ierr = VecRestoreArray(diagV,    &diagA);CHKERRQ(ierr);
  ierr = VecRestoreArray(offdiagV, &offdiagA);CHKERRQ(ierr);
  ierr = VecDestroy(&diagV);CHKERRQ(ierr);
  ierr = VecDestroy(&offdiagV);CHKERRQ(ierr);
  ierr = PetscFree2(diagIdx, offdiagIdx);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatGetSeqNonzeroStructure_MPIAIJ"
PetscErrorCode MatGetSeqNonzeroStructure_MPIAIJ(Mat mat,Mat *newmat)
{
  PetscErrorCode ierr;
  Mat            *dummy;

  PetscFunctionBegin;
  ierr    = MatGetSubMatrix_MPIAIJ_All(mat,MAT_DO_NOT_GET_VALUES,MAT_INITIAL_MATRIX,&dummy);CHKERRQ(ierr);
  *newmat = *dummy;
  ierr    = PetscFree(dummy);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatInvertBlockDiagonal_MPIAIJ"
PetscErrorCode  MatInvertBlockDiagonal_MPIAIJ(Mat A,const PetscScalar **values)
{
  Mat_MPIAIJ     *a = (Mat_MPIAIJ*) A->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = MatInvertBlockDiagonal(a->A,values);CHKERRQ(ierr);
  A->errortype = a->A->errortype;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatSetRandom_MPIAIJ"
static PetscErrorCode  MatSetRandom_MPIAIJ(Mat x,PetscRandom rctx)
{
  PetscErrorCode ierr;
  Mat_MPIAIJ     *aij = (Mat_MPIAIJ*)x->data;

  PetscFunctionBegin;
  ierr = MatSetRandom(aij->A,rctx);CHKERRQ(ierr);
  ierr = MatSetRandom(aij->B,rctx);CHKERRQ(ierr);
  ierr = MatAssemblyBegin(x,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(x,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatMPIAIJSetUseScalableIncreaseOverlap_MPIAIJ"
PetscErrorCode MatMPIAIJSetUseScalableIncreaseOverlap_MPIAIJ(Mat A,PetscBool sc)
{
  PetscFunctionBegin;
  if (sc) A->ops->increaseoverlap = MatIncreaseOverlap_MPIAIJ_Scalable;
  else A->ops->increaseoverlap    = MatIncreaseOverlap_MPIAIJ;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatMPIAIJSetUseScalableIncreaseOverlap"
/*@
   MatMPIAIJSetUseScalableIncreaseOverlap - Determine if the matrix uses a scalable algorithm to compute the overlap

   Collective on Mat

   Input Parameters:
+    A - the matrix
-    sc - PETSC_TRUE indicates use the scalable algorithm (default is not to use the scalable algorithm)

 Level: advanced

@*/
PetscErrorCode MatMPIAIJSetUseScalableIncreaseOverlap(Mat A,PetscBool sc)
{
  PetscErrorCode       ierr;

  PetscFunctionBegin;
  ierr = PetscTryMethod(A,"MatMPIAIJSetUseScalableIncreaseOverlap_C",(Mat,PetscBool),(A,sc));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatSetFromOptions_MPIAIJ"
PetscErrorCode MatSetFromOptions_MPIAIJ(PetscOptionItems *PetscOptionsObject,Mat A)
{
  PetscErrorCode       ierr;
  PetscBool            sc = PETSC_FALSE,flg;

  PetscFunctionBegin;
  ierr = PetscOptionsHead(PetscOptionsObject,"MPIAIJ options");CHKERRQ(ierr);
  ierr = PetscObjectOptionsBegin((PetscObject)A);
    if (A->ops->increaseoverlap == MatIncreaseOverlap_MPIAIJ_Scalable) sc = PETSC_TRUE;
    ierr = PetscOptionsBool("-mat_increase_overlap_scalable","Use a scalable algorithm to compute the overlap","MatIncreaseOverlap",sc,&sc,&flg);CHKERRQ(ierr);
    if (flg) {
      ierr = MatMPIAIJSetUseScalableIncreaseOverlap(A,sc);CHKERRQ(ierr);
    }
  ierr = PetscOptionsEnd();CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatShift_MPIAIJ"
PetscErrorCode MatShift_MPIAIJ(Mat Y,PetscScalar a)
{
  PetscErrorCode ierr;
  Mat_MPIAIJ     *maij = (Mat_MPIAIJ*)Y->data;
  Mat_SeqAIJ     *aij = (Mat_SeqAIJ*)maij->A->data;

  PetscFunctionBegin;
  if (!Y->preallocated) {
    ierr = MatMPIAIJSetPreallocation(Y,1,NULL,0,NULL);CHKERRQ(ierr);
  } else if (!aij->nz) {
    PetscInt nonew = aij->nonew;
    ierr = MatSeqAIJSetPreallocation(maij->A,1,NULL);CHKERRQ(ierr);
    aij->nonew = nonew;
  }
  ierr = MatShift_Basic(Y,a);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatMissingDiagonal_MPIAIJ"
PetscErrorCode MatMissingDiagonal_MPIAIJ(Mat A,PetscBool  *missing,PetscInt *d)
{
  Mat_MPIAIJ     *a = (Mat_MPIAIJ*)A->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (A->rmap->n != A->cmap->n) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_SUP,"Only works for square matrices");
  ierr = MatMissingDiagonal(a->A,missing,d);CHKERRQ(ierr);
  if (d) {
    PetscInt rstart;
    ierr = MatGetOwnershipRange(A,&rstart,NULL);CHKERRQ(ierr);
    *d += rstart;

  }
  PetscFunctionReturn(0);
}


/* -------------------------------------------------------------------*/
static struct _MatOps MatOps_Values = {MatSetValues_MPIAIJ,
                                       MatGetRow_MPIAIJ,
                                       MatRestoreRow_MPIAIJ,
                                       MatMult_MPIAIJ,
                                /* 4*/ MatMultAdd_MPIAIJ,
                                       MatMultTranspose_MPIAIJ,
                                       MatMultTransposeAdd_MPIAIJ,
                                       0,
                                       0,
                                       0,
                                /*10*/ 0,
                                       0,
                                       0,
                                       MatSOR_MPIAIJ,
                                       MatTranspose_MPIAIJ,
                                /*15*/ MatGetInfo_MPIAIJ,
                                       MatEqual_MPIAIJ,
                                       MatGetDiagonal_MPIAIJ,
                                       MatDiagonalScale_MPIAIJ,
                                       MatNorm_MPIAIJ,
                                /*20*/ MatAssemblyBegin_MPIAIJ,
                                       MatAssemblyEnd_MPIAIJ,
                                       MatSetOption_MPIAIJ,
                                       MatZeroEntries_MPIAIJ,
                                /*24*/ MatZeroRows_MPIAIJ,
                                       0,
                                       0,
                                       0,
                                       0,
                                /*29*/ MatSetUp_MPIAIJ,
                                       0,
                                       0,
                                       0,
                                       0,
                                /*34*/ MatDuplicate_MPIAIJ,
                                       0,
                                       0,
                                       0,
                                       0,
                                /*39*/ MatAXPY_MPIAIJ,
                                       MatGetSubMatrices_MPIAIJ,
                                       MatIncreaseOverlap_MPIAIJ,
                                       MatGetValues_MPIAIJ,
                                       MatCopy_MPIAIJ,
                                /*44*/ MatGetRowMax_MPIAIJ,
                                       MatScale_MPIAIJ,
                                       MatShift_MPIAIJ,
                                       MatDiagonalSet_MPIAIJ,
                                       MatZeroRowsColumns_MPIAIJ,
                                /*49*/ MatSetRandom_MPIAIJ,
                                       0,
                                       0,
                                       0,
                                       0,
                                /*54*/ MatFDColoringCreate_MPIXAIJ,
                                       0,
                                       MatSetUnfactored_MPIAIJ,
                                       MatPermute_MPIAIJ,
                                       0,
                                /*59*/ MatGetSubMatrix_MPIAIJ,
                                       MatDestroy_MPIAIJ,
                                       MatView_MPIAIJ,
                                       0,
                                       MatMatMatMult_MPIAIJ_MPIAIJ_MPIAIJ,
                                /*64*/ MatMatMatMultSymbolic_MPIAIJ_MPIAIJ_MPIAIJ,
                                       MatMatMatMultNumeric_MPIAIJ_MPIAIJ_MPIAIJ,
                                       0,
                                       0,
                                       0,
                                /*69*/ MatGetRowMaxAbs_MPIAIJ,
                                       MatGetRowMinAbs_MPIAIJ,
                                       0,
                                       MatSetColoring_MPIAIJ,
                                       0,
                                       MatSetValuesAdifor_MPIAIJ,
                                /*75*/ MatFDColoringApply_AIJ,
                                       MatSetFromOptions_MPIAIJ,
                                       0,
                                       0,
                                       MatFindZeroDiagonals_MPIAIJ,
                                /*80*/ 0,
                                       0,
                                       0,
                                /*83*/ MatLoad_MPIAIJ,
                                       0,
                                       0,
                                       0,
                                       0,
                                       0,
                                /*89*/ MatMatMult_MPIAIJ_MPIAIJ,
                                       MatMatMultSymbolic_MPIAIJ_MPIAIJ,
                                       MatMatMultNumeric_MPIAIJ_MPIAIJ,
                                       MatPtAP_MPIAIJ_MPIAIJ,
                                       MatPtAPSymbolic_MPIAIJ_MPIAIJ,
                                /*94*/ MatPtAPNumeric_MPIAIJ_MPIAIJ,
                                       0,
                                       0,
                                       0,
                                       0,
                                /*99*/ 0,
                                       0,
                                       0,
                                       MatConjugate_MPIAIJ,
                                       0,
                                /*104*/MatSetValuesRow_MPIAIJ,
                                       MatRealPart_MPIAIJ,
                                       MatImaginaryPart_MPIAIJ,
                                       0,
                                       0,
                                /*109*/0,
                                       0,
                                       MatGetRowMin_MPIAIJ,
                                       0,
                                       MatMissingDiagonal_MPIAIJ,
                                /*114*/MatGetSeqNonzeroStructure_MPIAIJ,
                                       0,
                                       MatGetGhosts_MPIAIJ,
                                       0,
                                       0,
                                /*119*/0,
                                       0,
                                       0,
                                       0,
                                       MatGetMultiProcBlock_MPIAIJ,
                                /*124*/MatFindNonzeroRows_MPIAIJ,
                                       MatGetColumnNorms_MPIAIJ,
                                       MatInvertBlockDiagonal_MPIAIJ,
                                       0,
                                       MatGetSubMatricesMPI_MPIAIJ,
                                /*129*/0,
                                       MatTransposeMatMult_MPIAIJ_MPIAIJ,
                                       MatTransposeMatMultSymbolic_MPIAIJ_MPIAIJ,
                                       MatTransposeMatMultNumeric_MPIAIJ_MPIAIJ,
                                       0,
                                /*134*/0,
                                       0,
                                       0,
                                       0,
                                       0,
                                /*139*/0,
                                       0,
                                       0,
                                       MatFDColoringSetUp_MPIXAIJ,
                                       MatFindOffBlockDiagonalEntries_MPIAIJ,
                                /*144*/MatCreateMPIMatConcatenateSeqMat_MPIAIJ
};

/* ----------------------------------------------------------------------------------------*/

#undef __FUNCT__
#define __FUNCT__ "MatStoreValues_MPIAIJ"
PetscErrorCode  MatStoreValues_MPIAIJ(Mat mat)
{
  Mat_MPIAIJ     *aij = (Mat_MPIAIJ*)mat->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = MatStoreValues(aij->A);CHKERRQ(ierr);
  ierr = MatStoreValues(aij->B);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatRetrieveValues_MPIAIJ"
PetscErrorCode  MatRetrieveValues_MPIAIJ(Mat mat)
{
  Mat_MPIAIJ     *aij = (Mat_MPIAIJ*)mat->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = MatRetrieveValues(aij->A);CHKERRQ(ierr);
  ierr = MatRetrieveValues(aij->B);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatMPIAIJSetPreallocation_MPIAIJ"
PetscErrorCode  MatMPIAIJSetPreallocation_MPIAIJ(Mat B,PetscInt d_nz,const PetscInt d_nnz[],PetscInt o_nz,const PetscInt o_nnz[])
{
  Mat_MPIAIJ     *b;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscLayoutSetUp(B->rmap);CHKERRQ(ierr);
  ierr = PetscLayoutSetUp(B->cmap);CHKERRQ(ierr);
  b = (Mat_MPIAIJ*)B->data;

  if (!B->preallocated) {
    /* Explicitly create 2 MATSEQAIJ matrices. */
    ierr = MatCreate(PETSC_COMM_SELF,&b->A);CHKERRQ(ierr);
    ierr = MatSetSizes(b->A,B->rmap->n,B->cmap->n,B->rmap->n,B->cmap->n);CHKERRQ(ierr);
    ierr = MatSetBlockSizesFromMats(b->A,B,B);CHKERRQ(ierr);
    ierr = MatSetType(b->A,MATSEQAIJ);CHKERRQ(ierr);
    ierr = PetscLogObjectParent((PetscObject)B,(PetscObject)b->A);CHKERRQ(ierr);
    ierr = MatCreate(PETSC_COMM_SELF,&b->B);CHKERRQ(ierr);
    ierr = MatSetSizes(b->B,B->rmap->n,B->cmap->N,B->rmap->n,B->cmap->N);CHKERRQ(ierr);
    ierr = MatSetBlockSizesFromMats(b->B,B,B);CHKERRQ(ierr);
    ierr = MatSetType(b->B,MATSEQAIJ);CHKERRQ(ierr);
    ierr = PetscLogObjectParent((PetscObject)B,(PetscObject)b->B);CHKERRQ(ierr);
  }

  ierr = MatSeqAIJSetPreallocation(b->A,d_nz,d_nnz);CHKERRQ(ierr);
  ierr = MatSeqAIJSetPreallocation(b->B,o_nz,o_nnz);CHKERRQ(ierr);
  B->preallocated = PETSC_TRUE;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatDuplicate_MPIAIJ"
PetscErrorCode MatDuplicate_MPIAIJ(Mat matin,MatDuplicateOption cpvalues,Mat *newmat)
{
  Mat            mat;
  Mat_MPIAIJ     *a,*oldmat = (Mat_MPIAIJ*)matin->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  *newmat = 0;
  ierr    = MatCreate(PetscObjectComm((PetscObject)matin),&mat);CHKERRQ(ierr);
  ierr    = MatSetSizes(mat,matin->rmap->n,matin->cmap->n,matin->rmap->N,matin->cmap->N);CHKERRQ(ierr);
  ierr    = MatSetBlockSizesFromMats(mat,matin,matin);CHKERRQ(ierr);
  ierr    = MatSetType(mat,((PetscObject)matin)->type_name);CHKERRQ(ierr);
  ierr    = PetscMemcpy(mat->ops,matin->ops,sizeof(struct _MatOps));CHKERRQ(ierr);
  a       = (Mat_MPIAIJ*)mat->data;

  mat->factortype   = matin->factortype;
  mat->assembled    = PETSC_TRUE;
  mat->insertmode   = NOT_SET_VALUES;
  mat->preallocated = PETSC_TRUE;

  a->size         = oldmat->size;
  a->rank         = oldmat->rank;
  a->donotstash   = oldmat->donotstash;
  a->roworiented  = oldmat->roworiented;
  a->rowindices   = 0;
  a->rowvalues    = 0;
  a->getrowactive = PETSC_FALSE;

  ierr = PetscLayoutReference(matin->rmap,&mat->rmap);CHKERRQ(ierr);
  ierr = PetscLayoutReference(matin->cmap,&mat->cmap);CHKERRQ(ierr);

  if (oldmat->colmap) {
#if defined(PETSC_USE_CTABLE)
    ierr = PetscTableCreateCopy(oldmat->colmap,&a->colmap);CHKERRQ(ierr);
#else
    ierr = PetscMalloc1(mat->cmap->N,&a->colmap);CHKERRQ(ierr);
    ierr = PetscLogObjectMemory((PetscObject)mat,(mat->cmap->N)*sizeof(PetscInt));CHKERRQ(ierr);
    ierr = PetscMemcpy(a->colmap,oldmat->colmap,(mat->cmap->N)*sizeof(PetscInt));CHKERRQ(ierr);
#endif
  } else a->colmap = 0;
  if (oldmat->garray) {
    PetscInt len;
    len  = oldmat->B->cmap->n;
    ierr = PetscMalloc1(len+1,&a->garray);CHKERRQ(ierr);
    ierr = PetscLogObjectMemory((PetscObject)mat,len*sizeof(PetscInt));CHKERRQ(ierr);
    if (len) { ierr = PetscMemcpy(a->garray,oldmat->garray,len*sizeof(PetscInt));CHKERRQ(ierr); }
  } else a->garray = 0;

  ierr    = VecDuplicate(oldmat->lvec,&a->lvec);CHKERRQ(ierr);
  ierr    = PetscLogObjectParent((PetscObject)mat,(PetscObject)a->lvec);CHKERRQ(ierr);
  ierr    = VecScatterCopy(oldmat->Mvctx,&a->Mvctx);CHKERRQ(ierr);
  ierr    = PetscLogObjectParent((PetscObject)mat,(PetscObject)a->Mvctx);CHKERRQ(ierr);
  ierr    = MatDuplicate(oldmat->A,cpvalues,&a->A);CHKERRQ(ierr);
  ierr    = PetscLogObjectParent((PetscObject)mat,(PetscObject)a->A);CHKERRQ(ierr);
  ierr    = MatDuplicate(oldmat->B,cpvalues,&a->B);CHKERRQ(ierr);
  ierr    = PetscLogObjectParent((PetscObject)mat,(PetscObject)a->B);CHKERRQ(ierr);
  ierr    = PetscFunctionListDuplicate(((PetscObject)matin)->qlist,&((PetscObject)mat)->qlist);CHKERRQ(ierr);
  *newmat = mat;
  PetscFunctionReturn(0);
}



#undef __FUNCT__
#define __FUNCT__ "MatLoad_MPIAIJ"
PetscErrorCode MatLoad_MPIAIJ(Mat newMat, PetscViewer viewer)
{
  PetscScalar    *vals,*svals;
  MPI_Comm       comm;
  PetscErrorCode ierr;
  PetscMPIInt    rank,size,tag = ((PetscObject)viewer)->tag;
  PetscInt       i,nz,j,rstart,rend,mmax,maxnz = 0;
  PetscInt       header[4],*rowlengths = 0,M,N,m,*cols;
  PetscInt       *ourlens = NULL,*procsnz = NULL,*offlens = NULL,jj,*mycols,*smycols;
  PetscInt       cend,cstart,n,*rowners;
  int            fd;
  PetscInt       bs = newMat->rmap->bs;

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

  ierr = PetscOptionsBegin(comm,NULL,"Options for loading MPIAIJ matrix","Mat");CHKERRQ(ierr);
  ierr = PetscOptionsInt("-matload_block_size","Set the blocksize used to store the matrix","MatLoad",bs,&bs,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsEnd();CHKERRQ(ierr);
  if (bs < 0) bs = 1;

  ierr = MPI_Bcast(header+1,3,MPIU_INT,0,comm);CHKERRQ(ierr);
  M    = header[1]; N = header[2];

  /* If global sizes are set, check if they are consistent with that given in the file */
  if (newMat->rmap->N >= 0 && newMat->rmap->N != M) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_FILE_UNEXPECTED,"Inconsistent # of rows:Matrix in file has (%D) and input matrix has (%D)",newMat->rmap->N,M);
  if (newMat->cmap->N >=0 && newMat->cmap->N != N) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_FILE_UNEXPECTED,"Inconsistent # of cols:Matrix in file has (%D) and input matrix has (%D)",newMat->cmap->N,N);

  /* determine ownership of all (block) rows */
  if (M%bs) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_FILE_UNEXPECTED, "Inconsistent # of rows (%d) and block size (%d)",M,bs);
  if (newMat->rmap->n < 0) m = bs*((M/bs)/size + (((M/bs) % size) > rank));    /* PETSC_DECIDE */
  else m = newMat->rmap->n; /* Set by user */

  ierr = PetscMalloc1(size+1,&rowners);CHKERRQ(ierr);
  ierr = MPI_Allgather(&m,1,MPIU_INT,rowners+1,1,MPIU_INT,comm);CHKERRQ(ierr);

  /* First process needs enough room for process with most rows */
  if (!rank) {
    mmax = rowners[1];
    for (i=2; i<=size; i++) {
      mmax = PetscMax(mmax, rowners[i]);
    }
  } else mmax = -1;             /* unused, but compilers complain */

  rowners[0] = 0;
  for (i=2; i<=size; i++) {
    rowners[i] += rowners[i-1];
  }
  rstart = rowners[rank];
  rend   = rowners[rank+1];

  /* distribute row lengths to all processors */
  ierr = PetscMalloc2(m,&ourlens,m,&offlens);CHKERRQ(ierr);
  if (!rank) {
    ierr = PetscBinaryRead(fd,ourlens,m,PETSC_INT);CHKERRQ(ierr);
    ierr = PetscMalloc1(mmax,&rowlengths);CHKERRQ(ierr);
    ierr = PetscCalloc1(size,&procsnz);CHKERRQ(ierr);
    for (j=0; j<m; j++) {
      procsnz[0] += ourlens[j];
    }
    for (i=1; i<size; i++) {
      ierr = PetscBinaryRead(fd,rowlengths,rowners[i+1]-rowners[i],PETSC_INT);CHKERRQ(ierr);
      /* calculate the number of nonzeros on each processor */
      for (j=0; j<rowners[i+1]-rowners[i]; j++) {
        procsnz[i] += rowlengths[j];
      }
      ierr = MPIULong_Send(rowlengths,rowners[i+1]-rowners[i],MPIU_INT,i,tag,comm);CHKERRQ(ierr);
    }
    ierr = PetscFree(rowlengths);CHKERRQ(ierr);
  } else {
    ierr = MPIULong_Recv(ourlens,m,MPIU_INT,0,tag,comm);CHKERRQ(ierr);
  }

  if (!rank) {
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
      ierr = MPIULong_Send(cols,nz,MPIU_INT,i,tag,comm);CHKERRQ(ierr);
    }
    ierr = PetscFree(cols);CHKERRQ(ierr);
  } else {
    /* determine buffer space needed for message */
    nz = 0;
    for (i=0; i<m; i++) {
      nz += ourlens[i];
    }
    ierr = PetscMalloc1(nz,&mycols);CHKERRQ(ierr);

    /* receive message of column indices*/
    ierr = MPIULong_Recv(mycols,nz,MPIU_INT,0,tag,comm);CHKERRQ(ierr);
  }

  /* determine column ownership if matrix is not square */
  if (N != M) {
    if (newMat->cmap->n < 0) n = N/size + ((N % size) > rank);
    else n = newMat->cmap->n;
    ierr   = MPI_Scan(&n,&cend,1,MPIU_INT,MPI_SUM,comm);CHKERRQ(ierr);
    cstart = cend - n;
  } else {
    cstart = rstart;
    cend   = rend;
    n      = cend - cstart;
  }

  /* loop over local rows, determining number of off diagonal entries */
  ierr = PetscMemzero(offlens,m*sizeof(PetscInt));CHKERRQ(ierr);
  jj   = 0;
  for (i=0; i<m; i++) {
    for (j=0; j<ourlens[i]; j++) {
      if (mycols[jj] < cstart || mycols[jj] >= cend) offlens[i]++;
      jj++;
    }
  }

  for (i=0; i<m; i++) {
    ourlens[i] -= offlens[i];
  }
  ierr = MatSetSizes(newMat,m,n,M,N);CHKERRQ(ierr);

  if (bs > 1) {ierr = MatSetBlockSize(newMat,bs);CHKERRQ(ierr);}

  ierr = MatMPIAIJSetPreallocation(newMat,0,ourlens,0,offlens);CHKERRQ(ierr);

  for (i=0; i<m; i++) {
    ourlens[i] += offlens[i];
  }

  if (!rank) {
    ierr = PetscMalloc1(maxnz+1,&vals);CHKERRQ(ierr);

    /* read in my part of the matrix numerical values  */
    nz   = procsnz[0];
    ierr = PetscBinaryRead(fd,vals,nz,PETSC_SCALAR);CHKERRQ(ierr);

    /* insert into matrix */
    jj      = rstart;
    smycols = mycols;
    svals   = vals;
    for (i=0; i<m; i++) {
      ierr     = MatSetValues_MPIAIJ(newMat,1,&jj,ourlens[i],smycols,svals,INSERT_VALUES);CHKERRQ(ierr);
      smycols += ourlens[i];
      svals   += ourlens[i];
      jj++;
    }

    /* read in other processors and ship out */
    for (i=1; i<size; i++) {
      nz   = procsnz[i];
      ierr = PetscBinaryRead(fd,vals,nz,PETSC_SCALAR);CHKERRQ(ierr);
      ierr = MPIULong_Send(vals,nz,MPIU_SCALAR,i,((PetscObject)newMat)->tag,comm);CHKERRQ(ierr);
    }
    ierr = PetscFree(procsnz);CHKERRQ(ierr);
  } else {
    /* receive numeric values */
    ierr = PetscMalloc1(nz+1,&vals);CHKERRQ(ierr);

    /* receive message of values*/
    ierr = MPIULong_Recv(vals,nz,MPIU_SCALAR,0,((PetscObject)newMat)->tag,comm);CHKERRQ(ierr);

    /* insert into matrix */
    jj      = rstart;
    smycols = mycols;
    svals   = vals;
    for (i=0; i<m; i++) {
      ierr     = MatSetValues_MPIAIJ(newMat,1,&jj,ourlens[i],smycols,svals,INSERT_VALUES);CHKERRQ(ierr);
      smycols += ourlens[i];
      svals   += ourlens[i];
      jj++;
    }
  }
  ierr = PetscFree2(ourlens,offlens);CHKERRQ(ierr);
  ierr = PetscFree(vals);CHKERRQ(ierr);
  ierr = PetscFree(mycols);CHKERRQ(ierr);
  ierr = PetscFree(rowners);CHKERRQ(ierr);
  ierr = MatAssemblyBegin(newMat,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(newMat,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatGetSubMatrix_MPIAIJ"
/* TODO: Not scalable because of ISAllGather() unless getting all columns. */
PetscErrorCode MatGetSubMatrix_MPIAIJ(Mat mat,IS isrow,IS iscol,MatReuse call,Mat *newmat)
{
  PetscErrorCode ierr;
  IS             iscol_local;
  PetscInt       csize;

  PetscFunctionBegin;
  ierr = ISGetLocalSize(iscol,&csize);CHKERRQ(ierr);
  if (call == MAT_REUSE_MATRIX) {
    ierr = PetscObjectQuery((PetscObject)*newmat,"ISAllGather",(PetscObject*)&iscol_local);CHKERRQ(ierr);
    if (!iscol_local) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONGSTATE,"Submatrix passed in was not used before, cannot reuse");
  } else {
    /* check if we are grabbing all columns*/
    PetscBool    isstride;
    PetscMPIInt  lisstride = 0,gisstride;
    ierr = PetscObjectTypeCompare((PetscObject)iscol,ISSTRIDE,&isstride);CHKERRQ(ierr);
    if (isstride) {
      PetscInt  start,len,mstart,mlen;
      ierr = ISStrideGetInfo(iscol,&start,NULL);CHKERRQ(ierr);
      ierr = ISGetLocalSize(iscol,&len);CHKERRQ(ierr);
      ierr = MatGetOwnershipRangeColumn(mat,&mstart,&mlen);CHKERRQ(ierr);
      if (mstart == start && mlen-mstart == len) lisstride = 1;
    }
    ierr = MPIU_Allreduce(&lisstride,&gisstride,1,MPI_INT,MPI_MIN,PetscObjectComm((PetscObject)mat));CHKERRQ(ierr);
    if (gisstride) {
      PetscInt N;
      ierr = MatGetSize(mat,NULL,&N);CHKERRQ(ierr);
      ierr = ISCreateStride(PetscObjectComm((PetscObject)mat),N,0,1,&iscol_local);CHKERRQ(ierr);
      ierr = ISSetIdentity(iscol_local);CHKERRQ(ierr);
      ierr = PetscInfo(mat,"Optimizing for obtaining all columns of the matrix; skipping ISAllGather()\n");CHKERRQ(ierr);
    } else {
      PetscInt cbs;
      ierr = ISGetBlockSize(iscol,&cbs);CHKERRQ(ierr);
      ierr = ISAllGather(iscol,&iscol_local);CHKERRQ(ierr);
      ierr = ISSetBlockSize(iscol_local,cbs);CHKERRQ(ierr);
    }
  }
  ierr = MatGetSubMatrix_MPIAIJ_Private(mat,isrow,iscol_local,csize,call,newmat);CHKERRQ(ierr);
  if (call == MAT_INITIAL_MATRIX) {
    ierr = PetscObjectCompose((PetscObject)*newmat,"ISAllGather",(PetscObject)iscol_local);CHKERRQ(ierr);
    ierr = ISDestroy(&iscol_local);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

extern PetscErrorCode MatGetSubMatrices_MPIAIJ_Local(Mat,PetscInt,const IS[],const IS[],MatReuse,PetscBool*,Mat*);
#undef __FUNCT__
#define __FUNCT__ "MatGetSubMatrix_MPIAIJ_Private"
/*
    Not great since it makes two copies of the submatrix, first an SeqAIJ
  in local and then by concatenating the local matrices the end result.
  Writing it directly would be much like MatGetSubMatrices_MPIAIJ()

  Note: This requires a sequential iscol with all indices.
*/
PetscErrorCode MatGetSubMatrix_MPIAIJ_Private(Mat mat,IS isrow,IS iscol,PetscInt csize,MatReuse call,Mat *newmat)
{
  PetscErrorCode ierr;
  PetscMPIInt    rank,size;
  PetscInt       i,m,n,rstart,row,rend,nz,*cwork,j,bs,cbs;
  PetscInt       *ii,*jj,nlocal,*dlens,*olens,dlen,olen,jend,mglobal,ncol;
  PetscBool      allcolumns, colflag;
  Mat            M,Mreuse;
  MatScalar      *vwork,*aa;
  MPI_Comm       comm;
  Mat_SeqAIJ     *aij;

  PetscFunctionBegin;
  ierr = PetscObjectGetComm((PetscObject)mat,&comm);CHKERRQ(ierr);
  ierr = MPI_Comm_rank(comm,&rank);CHKERRQ(ierr);
  ierr = MPI_Comm_size(comm,&size);CHKERRQ(ierr);

  ierr = ISIdentity(iscol,&colflag);CHKERRQ(ierr);
  ierr = ISGetLocalSize(iscol,&ncol);CHKERRQ(ierr);
  if (colflag && ncol == mat->cmap->N) {
    allcolumns = PETSC_TRUE;
    ierr = PetscInfo(mat,"Optimizing for obtaining all columns of the matrix\n");CHKERRQ(ierr);
  } else {
    allcolumns = PETSC_FALSE;
  }
  if (call ==  MAT_REUSE_MATRIX) {
    ierr = PetscObjectQuery((PetscObject)*newmat,"SubMatrix",(PetscObject*)&Mreuse);CHKERRQ(ierr);
    if (!Mreuse) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONGSTATE,"Submatrix passed in was not used before, cannot reuse");
    ierr = MatGetSubMatrices_MPIAIJ_Local(mat,1,&isrow,&iscol,MAT_REUSE_MATRIX,&allcolumns,&Mreuse);CHKERRQ(ierr);
  } else {
    ierr = MatGetSubMatrices_MPIAIJ_Local(mat,1,&isrow,&iscol,MAT_INITIAL_MATRIX,&allcolumns,&Mreuse);CHKERRQ(ierr);
  }

  /*
      m - number of local rows
      n - number of columns (same on all processors)
      rstart - first row in new global matrix generated
  */
  ierr = MatGetSize(Mreuse,&m,&n);CHKERRQ(ierr);
  ierr = MatGetBlockSizes(Mreuse,&bs,&cbs);CHKERRQ(ierr);
  if (call == MAT_INITIAL_MATRIX) {
    aij = (Mat_SeqAIJ*)(Mreuse)->data;
    ii  = aij->i;
    jj  = aij->j;

    /*
        Determine the number of non-zeros in the diagonal and off-diagonal
        portions of the matrix in order to do correct preallocation
    */

    /* first get start and end of "diagonal" columns */
    if (csize == PETSC_DECIDE) {
      ierr = ISGetSize(isrow,&mglobal);CHKERRQ(ierr);
      if (mglobal == n) { /* square matrix */
        nlocal = m;
      } else {
        nlocal = n/size + ((n % size) > rank);
      }
    } else {
      nlocal = csize;
    }
    ierr   = MPI_Scan(&nlocal,&rend,1,MPIU_INT,MPI_SUM,comm);CHKERRQ(ierr);
    rstart = rend - nlocal;
    if (rank == size - 1 && rend != n) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_SIZ,"Local column sizes %D do not add up to total number of columns %D",rend,n);

    /* next, compute all the lengths */
    ierr  = PetscMalloc1(2*m+1,&dlens);CHKERRQ(ierr);
    olens = dlens + m;
    for (i=0; i<m; i++) {
      jend = ii[i+1] - ii[i];
      olen = 0;
      dlen = 0;
      for (j=0; j<jend; j++) {
        if (*jj < rstart || *jj >= rend) olen++;
        else dlen++;
        jj++;
      }
      olens[i] = olen;
      dlens[i] = dlen;
    }
    ierr = MatCreate(comm,&M);CHKERRQ(ierr);
    ierr = MatSetSizes(M,m,nlocal,PETSC_DECIDE,n);CHKERRQ(ierr);
    ierr = MatSetBlockSizes(M,bs,cbs);CHKERRQ(ierr);
    ierr = MatSetType(M,((PetscObject)mat)->type_name);CHKERRQ(ierr);
    ierr = MatMPIAIJSetPreallocation(M,0,dlens,0,olens);CHKERRQ(ierr);
    ierr = PetscFree(dlens);CHKERRQ(ierr);
  } else {
    PetscInt ml,nl;

    M    = *newmat;
    ierr = MatGetLocalSize(M,&ml,&nl);CHKERRQ(ierr);
    if (ml != m) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_SIZ,"Previous matrix must be same size/layout as request");
    ierr = MatZeroEntries(M);CHKERRQ(ierr);
    /*
         The next two lines are needed so we may call MatSetValues_MPIAIJ() below directly,
       rather than the slower MatSetValues().
    */
    M->was_assembled = PETSC_TRUE;
    M->assembled     = PETSC_FALSE;
  }
  ierr = MatGetOwnershipRange(M,&rstart,&rend);CHKERRQ(ierr);
  aij  = (Mat_SeqAIJ*)(Mreuse)->data;
  ii   = aij->i;
  jj   = aij->j;
  aa   = aij->a;
  for (i=0; i<m; i++) {
    row   = rstart + i;
    nz    = ii[i+1] - ii[i];
    cwork = jj;     jj += nz;
    vwork = aa;     aa += nz;
    ierr  = MatSetValues_MPIAIJ(M,1,&row,nz,cwork,vwork,INSERT_VALUES);CHKERRQ(ierr);
  }

  ierr    = MatAssemblyBegin(M,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr    = MatAssemblyEnd(M,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  *newmat = M;

  /* save submatrix used in processor for next request */
  if (call ==  MAT_INITIAL_MATRIX) {
    ierr = PetscObjectCompose((PetscObject)M,"SubMatrix",(PetscObject)Mreuse);CHKERRQ(ierr);
    ierr = MatDestroy(&Mreuse);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatMPIAIJSetPreallocationCSR_MPIAIJ"
PetscErrorCode  MatMPIAIJSetPreallocationCSR_MPIAIJ(Mat B,const PetscInt Ii[],const PetscInt J[],const PetscScalar v[])
{
  PetscInt       m,cstart, cend,j,nnz,i,d;
  PetscInt       *d_nnz,*o_nnz,nnz_max = 0,rstart,ii;
  const PetscInt *JJ;
  PetscScalar    *values;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (Ii[0]) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Ii[0] must be 0 it is %D",Ii[0]);

  ierr   = PetscLayoutSetUp(B->rmap);CHKERRQ(ierr);
  ierr   = PetscLayoutSetUp(B->cmap);CHKERRQ(ierr);
  m      = B->rmap->n;
  cstart = B->cmap->rstart;
  cend   = B->cmap->rend;
  rstart = B->rmap->rstart;

  ierr = PetscMalloc2(m,&d_nnz,m,&o_nnz);CHKERRQ(ierr);

#if defined(PETSC_USE_DEBUGGING)
  for (i=0; i<m; i++) {
    nnz = Ii[i+1]- Ii[i];
    JJ  = J + Ii[i];
    if (nnz < 0) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Local row %D has a negative %D number of columns",i,nnz);
    if (nnz && (JJ[0] < 0)) SETERRRQ1(PETSC_ERR_ARG_WRONGSTATE,"Row %D starts with negative column index",i,j);
    if (nnz && (JJ[nnz-1] >= B->cmap->N) SETERRRQ3(PETSC_ERR_ARG_WRONGSTATE,"Row %D ends with too large a column index %D (max allowed %D)",i,JJ[nnz-1],B->cmap->N);
  }
#endif

  for (i=0; i<m; i++) {
    nnz     = Ii[i+1]- Ii[i];
    JJ      = J + Ii[i];
    nnz_max = PetscMax(nnz_max,nnz);
    d       = 0;
    for (j=0; j<nnz; j++) {
      if (cstart <= JJ[j] && JJ[j] < cend) d++;
    }
    d_nnz[i] = d;
    o_nnz[i] = nnz - d;
  }
  ierr = MatMPIAIJSetPreallocation(B,0,d_nnz,0,o_nnz);CHKERRQ(ierr);
  ierr = PetscFree2(d_nnz,o_nnz);CHKERRQ(ierr);

  if (v) values = (PetscScalar*)v;
  else {
    ierr = PetscCalloc1(nnz_max+1,&values);CHKERRQ(ierr);
  }

  for (i=0; i<m; i++) {
    ii   = i + rstart;
    nnz  = Ii[i+1]- Ii[i];
    ierr = MatSetValues_MPIAIJ(B,1,&ii,nnz,J+Ii[i],values+(v ? Ii[i] : 0),INSERT_VALUES);CHKERRQ(ierr);
  }
  ierr = MatAssemblyBegin(B,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(B,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);

  if (!v) {
    ierr = PetscFree(values);CHKERRQ(ierr);
  }
  ierr = MatSetOption(B,MAT_NEW_NONZERO_LOCATION_ERR,PETSC_TRUE);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatMPIAIJSetPreallocationCSR"
/*@
   MatMPIAIJSetPreallocationCSR - Allocates memory for a sparse parallel matrix in AIJ format
   (the default parallel PETSc format).

   Collective on MPI_Comm

   Input Parameters:
+  B - the matrix
.  i - the indices into j for the start of each local row (starts with zero)
.  j - the column indices for each local row (starts with zero)
-  v - optional values in the matrix

   Level: developer

   Notes:
       The i, j, and a arrays ARE copied by this routine into the internal format used by PETSc;
     thus you CANNOT change the matrix entries by changing the values of a[] after you have
     called this routine. Use MatCreateMPIAIJWithSplitArrays() to avoid needing to copy the arrays.

       The i and j indices are 0 based, and i indices are indices corresponding to the local j array.

       The format which is used for the sparse matrix input, is equivalent to a
    row-major ordering.. i.e for the following matrix, the input data expected is
    as shown

$        1 0 0
$        2 0 3     P0
$       -------
$        4 5 6     P1
$
$     Process0 [P0]: rows_owned=[0,1]
$        i =  {0,1,3}  [size = nrow+1  = 2+1]
$        j =  {0,0,2}  [size = 3]
$        v =  {1,2,3}  [size = 3]
$
$     Process1 [P1]: rows_owned=[2]
$        i =  {0,3}    [size = nrow+1  = 1+1]
$        j =  {0,1,2}  [size = 3]
$        v =  {4,5,6}  [size = 3]

.keywords: matrix, aij, compressed row, sparse, parallel

.seealso: MatCreate(), MatCreateSeqAIJ(), MatSetValues(), MatMPIAIJSetPreallocation(), MatCreateAIJ(), MPIAIJ,
          MatCreateSeqAIJWithArrays(), MatCreateMPIAIJWithSplitArrays()
@*/
PetscErrorCode  MatMPIAIJSetPreallocationCSR(Mat B,const PetscInt i[],const PetscInt j[], const PetscScalar v[])
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscTryMethod(B,"MatMPIAIJSetPreallocationCSR_C",(Mat,const PetscInt[],const PetscInt[],const PetscScalar[]),(B,i,j,v));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatMPIAIJSetPreallocation"
/*@C
   MatMPIAIJSetPreallocation - Preallocates memory for a sparse parallel matrix in AIJ format
   (the default parallel PETSc format).  For good matrix assembly performance
   the user should preallocate the matrix storage by setting the parameters
   d_nz (or d_nnz) and o_nz (or o_nnz).  By setting these parameters accurately,
   performance can be increased by more than a factor of 50.

   Collective on MPI_Comm

   Input Parameters:
+  B - the matrix
.  d_nz  - number of nonzeros per row in DIAGONAL portion of local submatrix
           (same value is used for all local rows)
.  d_nnz - array containing the number of nonzeros in the various rows of the
           DIAGONAL portion of the local submatrix (possibly different for each row)
           or NULL (PETSC_NULL_INTEGER in Fortran), if d_nz is used to specify the nonzero structure.
           The size of this array is equal to the number of local rows, i.e 'm'.
           For matrices that will be factored, you must leave room for (and set)
           the diagonal entry even if it is zero.
.  o_nz  - number of nonzeros per row in the OFF-DIAGONAL portion of local
           submatrix (same value is used for all local rows).
-  o_nnz - array containing the number of nonzeros in the various rows of the
           OFF-DIAGONAL portion of the local submatrix (possibly different for
           each row) or NULL (PETSC_NULL_INTEGER in Fortran), if o_nz is used to specify the nonzero
           structure. The size of this array is equal to the number
           of local rows, i.e 'm'.

   If the *_nnz parameter is given then the *_nz parameter is ignored

   The AIJ format (also called the Yale sparse matrix format or
   compressed row storage (CSR)), is fully compatible with standard Fortran 77
   storage.  The stored row and column indices begin with zero.
   See Users-Manual: ch_mat for details.

   The parallel matrix is partitioned such that the first m0 rows belong to
   process 0, the next m1 rows belong to process 1, the next m2 rows belong
   to process 2 etc.. where m0,m1,m2... are the input parameter 'm'.

   The DIAGONAL portion of the local submatrix of a processor can be defined
   as the submatrix which is obtained by extraction the part corresponding to
   the rows r1-r2 and columns c1-c2 of the global matrix, where r1 is the
   first row that belongs to the processor, r2 is the last row belonging to
   the this processor, and c1-c2 is range of indices of the local part of a
   vector suitable for applying the matrix to.  This is an mxn matrix.  In the
   common case of a square matrix, the row and column ranges are the same and
   the DIAGONAL part is also square. The remaining portion of the local
   submatrix (mxN) constitute the OFF-DIAGONAL portion.

   If o_nnz, d_nnz are specified, then o_nz, and d_nz are ignored.

   You can call MatGetInfo() to get information on how effective the preallocation was;
   for example the fields mallocs,nz_allocated,nz_used,nz_unneeded;
   You can also run with the option -info and look for messages with the string
   malloc in them to see if additional memory allocation was needed.

   Example usage:

   Consider the following 8x8 matrix with 34 non-zero values, that is
   assembled across 3 processors. Lets assume that proc0 owns 3 rows,
   proc1 owns 3 rows, proc2 owns 2 rows. This division can be shown
   as follows:

.vb
            1  2  0  |  0  3  0  |  0  4
    Proc0   0  5  6  |  7  0  0  |  8  0
            9  0 10  | 11  0  0  | 12  0
    -------------------------------------
           13  0 14  | 15 16 17  |  0  0
    Proc1   0 18  0  | 19 20 21  |  0  0
            0  0  0  | 22 23  0  | 24  0
    -------------------------------------
    Proc2  25 26 27  |  0  0 28  | 29  0
           30  0  0  | 31 32 33  |  0 34
.ve

   This can be represented as a collection of submatrices as:

.vb
      A B C
      D E F
      G H I
.ve

   Where the submatrices A,B,C are owned by proc0, D,E,F are
   owned by proc1, G,H,I are owned by proc2.

   The 'm' parameters for proc0,proc1,proc2 are 3,3,2 respectively.
   The 'n' parameters for proc0,proc1,proc2 are 3,3,2 respectively.
   The 'M','N' parameters are 8,8, and have the same values on all procs.

   The DIAGONAL submatrices corresponding to proc0,proc1,proc2 are
   submatrices [A], [E], [I] respectively. The OFF-DIAGONAL submatrices
   corresponding to proc0,proc1,proc2 are [BC], [DF], [GH] respectively.
   Internally, each processor stores the DIAGONAL part, and the OFF-DIAGONAL
   part as SeqAIJ matrices. for eg: proc1 will store [E] as a SeqAIJ
   matrix, ans [DF] as another SeqAIJ matrix.

   When d_nz, o_nz parameters are specified, d_nz storage elements are
   allocated for every row of the local diagonal submatrix, and o_nz
   storage locations are allocated for every row of the OFF-DIAGONAL submat.
   One way to choose d_nz and o_nz is to use the max nonzerors per local
   rows for each of the local DIAGONAL, and the OFF-DIAGONAL submatrices.
   In this case, the values of d_nz,o_nz are:
.vb
     proc0 : dnz = 2, o_nz = 2
     proc1 : dnz = 3, o_nz = 2
     proc2 : dnz = 1, o_nz = 4
.ve
   We are allocating m*(d_nz+o_nz) storage locations for every proc. This
   translates to 3*(2+2)=12 for proc0, 3*(3+2)=15 for proc1, 2*(1+4)=10
   for proc3. i.e we are using 12+15+10=37 storage locations to store
   34 values.

   When d_nnz, o_nnz parameters are specified, the storage is specified
   for every row, coresponding to both DIAGONAL and OFF-DIAGONAL submatrices.
   In the above case the values for d_nnz,o_nnz are:
.vb
     proc0: d_nnz = [2,2,2] and o_nnz = [2,2,2]
     proc1: d_nnz = [3,3,2] and o_nnz = [2,1,1]
     proc2: d_nnz = [1,1]   and o_nnz = [4,4]
.ve
   Here the space allocated is sum of all the above values i.e 34, and
   hence pre-allocation is perfect.

   Level: intermediate

.keywords: matrix, aij, compressed row, sparse, parallel

.seealso: MatCreate(), MatCreateSeqAIJ(), MatSetValues(), MatCreateAIJ(), MatMPIAIJSetPreallocationCSR(),
          MPIAIJ, MatGetInfo(), PetscSplitOwnership()
@*/
PetscErrorCode  MatMPIAIJSetPreallocation(Mat B,PetscInt d_nz,const PetscInt d_nnz[],PetscInt o_nz,const PetscInt o_nnz[])
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(B,MAT_CLASSID,1);
  PetscValidType(B,1);
  ierr = PetscTryMethod(B,"MatMPIAIJSetPreallocation_C",(Mat,PetscInt,const PetscInt[],PetscInt,const PetscInt[]),(B,d_nz,d_nnz,o_nz,o_nnz));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatCreateMPIAIJWithArrays"
/*@
     MatCreateMPIAIJWithArrays - creates a MPI AIJ matrix using arrays that contain in standard
         CSR format the local rows.

   Collective on MPI_Comm

   Input Parameters:
+  comm - MPI communicator
.  m - number of local rows (Cannot be PETSC_DECIDE)
.  n - This value should be the same as the local size used in creating the
       x vector for the matrix-vector product y = Ax. (or PETSC_DECIDE to have
       calculated if N is given) For square matrices n is almost always m.
.  M - number of global rows (or PETSC_DETERMINE to have calculated if m is given)
.  N - number of global columns (or PETSC_DETERMINE to have calculated if n is given)
.   i - row indices
.   j - column indices
-   a - matrix values

   Output Parameter:
.   mat - the matrix

   Level: intermediate

   Notes:
       The i, j, and a arrays ARE copied by this routine into the internal format used by PETSc;
     thus you CANNOT change the matrix entries by changing the values of a[] after you have
     called this routine. Use MatCreateMPIAIJWithSplitArrays() to avoid needing to copy the arrays.

       The i and j indices are 0 based, and i indices are indices corresponding to the local j array.

       The format which is used for the sparse matrix input, is equivalent to a
    row-major ordering.. i.e for the following matrix, the input data expected is
    as shown

$        1 0 0
$        2 0 3     P0
$       -------
$        4 5 6     P1
$
$     Process0 [P0]: rows_owned=[0,1]
$        i =  {0,1,3}  [size = nrow+1  = 2+1]
$        j =  {0,0,2}  [size = 3]
$        v =  {1,2,3}  [size = 3]
$
$     Process1 [P1]: rows_owned=[2]
$        i =  {0,3}    [size = nrow+1  = 1+1]
$        j =  {0,1,2}  [size = 3]
$        v =  {4,5,6}  [size = 3]

.keywords: matrix, aij, compressed row, sparse, parallel

.seealso: MatCreate(), MatCreateSeqAIJ(), MatSetValues(), MatMPIAIJSetPreallocation(), MatMPIAIJSetPreallocationCSR(),
          MPIAIJ, MatCreateAIJ(), MatCreateMPIAIJWithSplitArrays()
@*/
PetscErrorCode  MatCreateMPIAIJWithArrays(MPI_Comm comm,PetscInt m,PetscInt n,PetscInt M,PetscInt N,const PetscInt i[],const PetscInt j[],const PetscScalar a[],Mat *mat)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (i[0]) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"i (row indices) must start with 0");
  if (m < 0) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"local number of rows (m) cannot be PETSC_DECIDE, or negative");
  ierr = MatCreate(comm,mat);CHKERRQ(ierr);
  ierr = MatSetSizes(*mat,m,n,M,N);CHKERRQ(ierr);
  /* ierr = MatSetBlockSizes(M,bs,cbs);CHKERRQ(ierr); */
  ierr = MatSetType(*mat,MATMPIAIJ);CHKERRQ(ierr);
  ierr = MatMPIAIJSetPreallocationCSR(*mat,i,j,a);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatCreateAIJ"
/*@C
   MatCreateAIJ - Creates a sparse parallel matrix in AIJ format
   (the default parallel PETSc format).  For good matrix assembly performance
   the user should preallocate the matrix storage by setting the parameters
   d_nz (or d_nnz) and o_nz (or o_nnz).  By setting these parameters accurately,
   performance can be increased by more than a factor of 50.

   Collective on MPI_Comm

   Input Parameters:
+  comm - MPI communicator
.  m - number of local rows (or PETSC_DECIDE to have calculated if M is given)
           This value should be the same as the local size used in creating the
           y vector for the matrix-vector product y = Ax.
.  n - This value should be the same as the local size used in creating the
       x vector for the matrix-vector product y = Ax. (or PETSC_DECIDE to have
       calculated if N is given) For square matrices n is almost always m.
.  M - number of global rows (or PETSC_DETERMINE to have calculated if m is given)
.  N - number of global columns (or PETSC_DETERMINE to have calculated if n is given)
.  d_nz  - number of nonzeros per row in DIAGONAL portion of local submatrix
           (same value is used for all local rows)
.  d_nnz - array containing the number of nonzeros in the various rows of the
           DIAGONAL portion of the local submatrix (possibly different for each row)
           or NULL, if d_nz is used to specify the nonzero structure.
           The size of this array is equal to the number of local rows, i.e 'm'.
.  o_nz  - number of nonzeros per row in the OFF-DIAGONAL portion of local
           submatrix (same value is used for all local rows).
-  o_nnz - array containing the number of nonzeros in the various rows of the
           OFF-DIAGONAL portion of the local submatrix (possibly different for
           each row) or NULL, if o_nz is used to specify the nonzero
           structure. The size of this array is equal to the number
           of local rows, i.e 'm'.

   Output Parameter:
.  A - the matrix

   It is recommended that one use the MatCreate(), MatSetType() and/or MatSetFromOptions(),
   MatXXXXSetPreallocation() paradgm instead of this routine directly.
   [MatXXXXSetPreallocation() is, for example, MatSeqAIJSetPreallocation]

   Notes:
   If the *_nnz parameter is given then the *_nz parameter is ignored

   m,n,M,N parameters specify the size of the matrix, and its partitioning across
   processors, while d_nz,d_nnz,o_nz,o_nnz parameters specify the approximate
   storage requirements for this matrix.

   If PETSC_DECIDE or  PETSC_DETERMINE is used for a particular argument on one
   processor than it must be used on all processors that share the object for
   that argument.

   The user MUST specify either the local or global matrix dimensions
   (possibly both).

   The parallel matrix is partitioned across processors such that the
   first m0 rows belong to process 0, the next m1 rows belong to
   process 1, the next m2 rows belong to process 2 etc.. where
   m0,m1,m2,.. are the input parameter 'm'. i.e each processor stores
   values corresponding to [m x N] submatrix.

   The columns are logically partitioned with the n0 columns belonging
   to 0th partition, the next n1 columns belonging to the next
   partition etc.. where n0,n1,n2... are the input parameter 'n'.

   The DIAGONAL portion of the local submatrix on any given processor
   is the submatrix corresponding to the rows and columns m,n
   corresponding to the given processor. i.e diagonal matrix on
   process 0 is [m0 x n0], diagonal matrix on process 1 is [m1 x n1]
   etc. The remaining portion of the local submatrix [m x (N-n)]
   constitute the OFF-DIAGONAL portion. The example below better
   illustrates this concept.

   For a square global matrix we define each processor's diagonal portion
   to be its local rows and the corresponding columns (a square submatrix);
   each processor's off-diagonal portion encompasses the remainder of the
   local matrix (a rectangular submatrix).

   If o_nnz, d_nnz are specified, then o_nz, and d_nz are ignored.

   When calling this routine with a single process communicator, a matrix of
   type SEQAIJ is returned.  If a matrix of type MPIAIJ is desired for this
   type of communicator, use the construction mechanism:
     MatCreate(...,&A); MatSetType(A,MATMPIAIJ); MatSetSizes(A, m,n,M,N); MatMPIAIJSetPreallocation(A,...);

   By default, this format uses inodes (identical nodes) when possible.
   We search for consecutive rows with the same nonzero structure, thereby
   reusing matrix information to achieve increased efficiency.

   Options Database Keys:
+  -mat_no_inode  - Do not use inodes
.  -mat_inode_limit <limit> - Sets inode limit (max limit=5)
-  -mat_aij_oneindex - Internally use indexing starting at 1
        rather than 0.  Note that when calling MatSetValues(),
        the user still MUST index entries starting at 0!


   Example usage:

   Consider the following 8x8 matrix with 34 non-zero values, that is
   assembled across 3 processors. Lets assume that proc0 owns 3 rows,
   proc1 owns 3 rows, proc2 owns 2 rows. This division can be shown
   as follows:

.vb
            1  2  0  |  0  3  0  |  0  4
    Proc0   0  5  6  |  7  0  0  |  8  0
            9  0 10  | 11  0  0  | 12  0
    -------------------------------------
           13  0 14  | 15 16 17  |  0  0
    Proc1   0 18  0  | 19 20 21  |  0  0
            0  0  0  | 22 23  0  | 24  0
    -------------------------------------
    Proc2  25 26 27  |  0  0 28  | 29  0
           30  0  0  | 31 32 33  |  0 34
.ve

   This can be represented as a collection of submatrices as:

.vb
      A B C
      D E F
      G H I
.ve

   Where the submatrices A,B,C are owned by proc0, D,E,F are
   owned by proc1, G,H,I are owned by proc2.

   The 'm' parameters for proc0,proc1,proc2 are 3,3,2 respectively.
   The 'n' parameters for proc0,proc1,proc2 are 3,3,2 respectively.
   The 'M','N' parameters are 8,8, and have the same values on all procs.

   The DIAGONAL submatrices corresponding to proc0,proc1,proc2 are
   submatrices [A], [E], [I] respectively. The OFF-DIAGONAL submatrices
   corresponding to proc0,proc1,proc2 are [BC], [DF], [GH] respectively.
   Internally, each processor stores the DIAGONAL part, and the OFF-DIAGONAL
   part as SeqAIJ matrices. for eg: proc1 will store [E] as a SeqAIJ
   matrix, ans [DF] as another SeqAIJ matrix.

   When d_nz, o_nz parameters are specified, d_nz storage elements are
   allocated for every row of the local diagonal submatrix, and o_nz
   storage locations are allocated for every row of the OFF-DIAGONAL submat.
   One way to choose d_nz and o_nz is to use the max nonzerors per local
   rows for each of the local DIAGONAL, and the OFF-DIAGONAL submatrices.
   In this case, the values of d_nz,o_nz are:
.vb
     proc0 : dnz = 2, o_nz = 2
     proc1 : dnz = 3, o_nz = 2
     proc2 : dnz = 1, o_nz = 4
.ve
   We are allocating m*(d_nz+o_nz) storage locations for every proc. This
   translates to 3*(2+2)=12 for proc0, 3*(3+2)=15 for proc1, 2*(1+4)=10
   for proc3. i.e we are using 12+15+10=37 storage locations to store
   34 values.

   When d_nnz, o_nnz parameters are specified, the storage is specified
   for every row, coresponding to both DIAGONAL and OFF-DIAGONAL submatrices.
   In the above case the values for d_nnz,o_nnz are:
.vb
     proc0: d_nnz = [2,2,2] and o_nnz = [2,2,2]
     proc1: d_nnz = [3,3,2] and o_nnz = [2,1,1]
     proc2: d_nnz = [1,1]   and o_nnz = [4,4]
.ve
   Here the space allocated is sum of all the above values i.e 34, and
   hence pre-allocation is perfect.

   Level: intermediate

.keywords: matrix, aij, compressed row, sparse, parallel

.seealso: MatCreate(), MatCreateSeqAIJ(), MatSetValues(), MatMPIAIJSetPreallocation(), MatMPIAIJSetPreallocationCSR(),
          MPIAIJ, MatCreateMPIAIJWithArrays()
@*/
PetscErrorCode  MatCreateAIJ(MPI_Comm comm,PetscInt m,PetscInt n,PetscInt M,PetscInt N,PetscInt d_nz,const PetscInt d_nnz[],PetscInt o_nz,const PetscInt o_nnz[],Mat *A)
{
  PetscErrorCode ierr;
  PetscMPIInt    size;

  PetscFunctionBegin;
  ierr = MatCreate(comm,A);CHKERRQ(ierr);
  ierr = MatSetSizes(*A,m,n,M,N);CHKERRQ(ierr);
  ierr = MPI_Comm_size(comm,&size);CHKERRQ(ierr);
  if (size > 1) {
    ierr = MatSetType(*A,MATMPIAIJ);CHKERRQ(ierr);
    ierr = MatMPIAIJSetPreallocation(*A,d_nz,d_nnz,o_nz,o_nnz);CHKERRQ(ierr);
  } else {
    ierr = MatSetType(*A,MATSEQAIJ);CHKERRQ(ierr);
    ierr = MatSeqAIJSetPreallocation(*A,d_nz,d_nnz);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatMPIAIJGetSeqAIJ"
PetscErrorCode  MatMPIAIJGetSeqAIJ(Mat A,Mat *Ad,Mat *Ao,const PetscInt *colmap[])
{
  Mat_MPIAIJ *a = (Mat_MPIAIJ*)A->data;

  PetscFunctionBegin;
  if (Ad)     *Ad     = a->A;
  if (Ao)     *Ao     = a->B;
  if (colmap) *colmap = a->garray;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatSetColoring_MPIAIJ"
PetscErrorCode MatSetColoring_MPIAIJ(Mat A,ISColoring coloring)
{
  PetscErrorCode ierr;
  PetscInt       i;
  Mat_MPIAIJ     *a = (Mat_MPIAIJ*)A->data;

  PetscFunctionBegin;
  if (coloring->ctype == IS_COLORING_GLOBAL) {
    ISColoringValue *allcolors,*colors;
    ISColoring      ocoloring;

    /* set coloring for diagonal portion */
    ierr = MatSetColoring_SeqAIJ(a->A,coloring);CHKERRQ(ierr);

    /* set coloring for off-diagonal portion */
    ierr = ISAllGatherColors(PetscObjectComm((PetscObject)A),coloring->n,coloring->colors,NULL,&allcolors);CHKERRQ(ierr);
    ierr = PetscMalloc1(a->B->cmap->n+1,&colors);CHKERRQ(ierr);
    for (i=0; i<a->B->cmap->n; i++) {
      colors[i] = allcolors[a->garray[i]];
    }
    ierr = PetscFree(allcolors);CHKERRQ(ierr);
    ierr = ISColoringCreate(MPI_COMM_SELF,coloring->n,a->B->cmap->n,colors,PETSC_OWN_POINTER,&ocoloring);CHKERRQ(ierr);
    ierr = MatSetColoring_SeqAIJ(a->B,ocoloring);CHKERRQ(ierr);
    ierr = ISColoringDestroy(&ocoloring);CHKERRQ(ierr);
  } else if (coloring->ctype == IS_COLORING_GHOSTED) {
    ISColoringValue *colors;
    PetscInt        *larray;
    ISColoring      ocoloring;

    /* set coloring for diagonal portion */
    ierr = PetscMalloc1(a->A->cmap->n+1,&larray);CHKERRQ(ierr);
    for (i=0; i<a->A->cmap->n; i++) {
      larray[i] = i + A->cmap->rstart;
    }
    ierr = ISGlobalToLocalMappingApply(A->cmap->mapping,IS_GTOLM_MASK,a->A->cmap->n,larray,NULL,larray);CHKERRQ(ierr);
    ierr = PetscMalloc1(a->A->cmap->n+1,&colors);CHKERRQ(ierr);
    for (i=0; i<a->A->cmap->n; i++) {
      colors[i] = coloring->colors[larray[i]];
    }
    ierr = PetscFree(larray);CHKERRQ(ierr);
    ierr = ISColoringCreate(PETSC_COMM_SELF,coloring->n,a->A->cmap->n,colors,PETSC_OWN_POINTER,&ocoloring);CHKERRQ(ierr);
    ierr = MatSetColoring_SeqAIJ(a->A,ocoloring);CHKERRQ(ierr);
    ierr = ISColoringDestroy(&ocoloring);CHKERRQ(ierr);

    /* set coloring for off-diagonal portion */
    ierr = PetscMalloc1(a->B->cmap->n+1,&larray);CHKERRQ(ierr);
    ierr = ISGlobalToLocalMappingApply(A->cmap->mapping,IS_GTOLM_MASK,a->B->cmap->n,a->garray,NULL,larray);CHKERRQ(ierr);
    ierr = PetscMalloc1(a->B->cmap->n+1,&colors);CHKERRQ(ierr);
    for (i=0; i<a->B->cmap->n; i++) {
      colors[i] = coloring->colors[larray[i]];
    }
    ierr = PetscFree(larray);CHKERRQ(ierr);
    ierr = ISColoringCreate(MPI_COMM_SELF,coloring->n,a->B->cmap->n,colors,PETSC_OWN_POINTER,&ocoloring);CHKERRQ(ierr);
    ierr = MatSetColoring_SeqAIJ(a->B,ocoloring);CHKERRQ(ierr);
    ierr = ISColoringDestroy(&ocoloring);CHKERRQ(ierr);
  } else SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_SUP,"No support ISColoringType %d",(int)coloring->ctype);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatSetValuesAdifor_MPIAIJ"
PetscErrorCode MatSetValuesAdifor_MPIAIJ(Mat A,PetscInt nl,void *advalues)
{
  Mat_MPIAIJ     *a = (Mat_MPIAIJ*)A->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = MatSetValuesAdifor_SeqAIJ(a->A,nl,advalues);CHKERRQ(ierr);
  ierr = MatSetValuesAdifor_SeqAIJ(a->B,nl,advalues);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatCreateMPIMatConcatenateSeqMat_MPIAIJ"
PetscErrorCode MatCreateMPIMatConcatenateSeqMat_MPIAIJ(MPI_Comm comm,Mat inmat,PetscInt n,MatReuse scall,Mat *outmat)
{
  PetscErrorCode ierr;
  PetscInt       m,N,i,rstart,nnz,Ii;
  PetscInt       *indx;
  PetscScalar    *values;

  PetscFunctionBegin;
  ierr = MatGetSize(inmat,&m,&N);CHKERRQ(ierr);
  if (scall == MAT_INITIAL_MATRIX) { /* symbolic phase */
    PetscInt       *dnz,*onz,sum,bs,cbs;

    if (n == PETSC_DECIDE) {
      ierr = PetscSplitOwnership(comm,&n,&N);CHKERRQ(ierr);
    }
    /* Check sum(n) = N */
    ierr = MPIU_Allreduce(&n,&sum,1,MPIU_INT,MPI_SUM,comm);CHKERRQ(ierr);
    if (sum != N) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_ARG_INCOMP,"Sum of local columns != global columns %d",N);

    ierr    = MPI_Scan(&m, &rstart,1,MPIU_INT,MPI_SUM,comm);CHKERRQ(ierr);
    rstart -= m;

    ierr = MatPreallocateInitialize(comm,m,n,dnz,onz);CHKERRQ(ierr);
    for (i=0; i<m; i++) {
      ierr = MatGetRow_SeqAIJ(inmat,i,&nnz,&indx,NULL);CHKERRQ(ierr);
      ierr = MatPreallocateSet(i+rstart,nnz,indx,dnz,onz);CHKERRQ(ierr);
      ierr = MatRestoreRow_SeqAIJ(inmat,i,&nnz,&indx,NULL);CHKERRQ(ierr);
    }

    ierr = MatCreate(comm,outmat);CHKERRQ(ierr);
    ierr = MatSetSizes(*outmat,m,n,PETSC_DETERMINE,PETSC_DETERMINE);CHKERRQ(ierr);
    ierr = MatGetBlockSizes(inmat,&bs,&cbs);CHKERRQ(ierr);
    ierr = MatSetBlockSizes(*outmat,bs,cbs);CHKERRQ(ierr);
    ierr = MatSetType(*outmat,MATMPIAIJ);CHKERRQ(ierr);
    ierr = MatMPIAIJSetPreallocation(*outmat,0,dnz,0,onz);CHKERRQ(ierr);
    ierr = MatPreallocateFinalize(dnz,onz);CHKERRQ(ierr);
  }

  /* numeric phase */
  ierr = MatGetOwnershipRange(*outmat,&rstart,NULL);CHKERRQ(ierr);
  for (i=0; i<m; i++) {
    ierr = MatGetRow_SeqAIJ(inmat,i,&nnz,&indx,&values);CHKERRQ(ierr);
    Ii   = i + rstart;
    ierr = MatSetValues(*outmat,1,&Ii,nnz,indx,values,INSERT_VALUES);CHKERRQ(ierr);
    ierr = MatRestoreRow_SeqAIJ(inmat,i,&nnz,&indx,&values);CHKERRQ(ierr);
  }
  ierr = MatAssemblyBegin(*outmat,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(*outmat,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatFileSplit"
PetscErrorCode MatFileSplit(Mat A,char *outfile)
{
  PetscErrorCode    ierr;
  PetscMPIInt       rank;
  PetscInt          m,N,i,rstart,nnz;
  size_t            len;
  const PetscInt    *indx;
  PetscViewer       out;
  char              *name;
  Mat               B;
  const PetscScalar *values;

  PetscFunctionBegin;
  ierr = MatGetLocalSize(A,&m,0);CHKERRQ(ierr);
  ierr = MatGetSize(A,0,&N);CHKERRQ(ierr);
  /* Should this be the type of the diagonal block of A? */
  ierr = MatCreate(PETSC_COMM_SELF,&B);CHKERRQ(ierr);
  ierr = MatSetSizes(B,m,N,m,N);CHKERRQ(ierr);
  ierr = MatSetBlockSizesFromMats(B,A,A);CHKERRQ(ierr);
  ierr = MatSetType(B,MATSEQAIJ);CHKERRQ(ierr);
  ierr = MatSeqAIJSetPreallocation(B,0,NULL);CHKERRQ(ierr);
  ierr = MatGetOwnershipRange(A,&rstart,0);CHKERRQ(ierr);
  for (i=0; i<m; i++) {
    ierr = MatGetRow(A,i+rstart,&nnz,&indx,&values);CHKERRQ(ierr);
    ierr = MatSetValues(B,1,&i,nnz,indx,values,INSERT_VALUES);CHKERRQ(ierr);
    ierr = MatRestoreRow(A,i+rstart,&nnz,&indx,&values);CHKERRQ(ierr);
  }
  ierr = MatAssemblyBegin(B,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(B,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);

  ierr = MPI_Comm_rank(PetscObjectComm((PetscObject)A),&rank);CHKERRQ(ierr);
  ierr = PetscStrlen(outfile,&len);CHKERRQ(ierr);
  ierr = PetscMalloc1(len+5,&name);CHKERRQ(ierr);
  sprintf(name,"%s.%d",outfile,rank);
  ierr = PetscViewerBinaryOpen(PETSC_COMM_SELF,name,FILE_MODE_APPEND,&out);CHKERRQ(ierr);
  ierr = PetscFree(name);CHKERRQ(ierr);
  ierr = MatView(B,out);CHKERRQ(ierr);
  ierr = PetscViewerDestroy(&out);CHKERRQ(ierr);
  ierr = MatDestroy(&B);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

extern PetscErrorCode MatDestroy_MPIAIJ(Mat);
#undef __FUNCT__
#define __FUNCT__ "MatDestroy_MPIAIJ_SeqsToMPI"
PetscErrorCode  MatDestroy_MPIAIJ_SeqsToMPI(Mat A)
{
  PetscErrorCode      ierr;
  Mat_Merge_SeqsToMPI *merge;
  PetscContainer      container;

  PetscFunctionBegin;
  ierr = PetscObjectQuery((PetscObject)A,"MatMergeSeqsToMPI",(PetscObject*)&container);CHKERRQ(ierr);
  if (container) {
    ierr = PetscContainerGetPointer(container,(void**)&merge);CHKERRQ(ierr);
    ierr = PetscFree(merge->id_r);CHKERRQ(ierr);
    ierr = PetscFree(merge->len_s);CHKERRQ(ierr);
    ierr = PetscFree(merge->len_r);CHKERRQ(ierr);
    ierr = PetscFree(merge->bi);CHKERRQ(ierr);
    ierr = PetscFree(merge->bj);CHKERRQ(ierr);
    ierr = PetscFree(merge->buf_ri[0]);CHKERRQ(ierr);
    ierr = PetscFree(merge->buf_ri);CHKERRQ(ierr);
    ierr = PetscFree(merge->buf_rj[0]);CHKERRQ(ierr);
    ierr = PetscFree(merge->buf_rj);CHKERRQ(ierr);
    ierr = PetscFree(merge->coi);CHKERRQ(ierr);
    ierr = PetscFree(merge->coj);CHKERRQ(ierr);
    ierr = PetscFree(merge->owners_co);CHKERRQ(ierr);
    ierr = PetscLayoutDestroy(&merge->rowmap);CHKERRQ(ierr);
    ierr = PetscFree(merge);CHKERRQ(ierr);
    ierr = PetscObjectCompose((PetscObject)A,"MatMergeSeqsToMPI",0);CHKERRQ(ierr);
  }
  ierr = MatDestroy_MPIAIJ(A);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#include <../src/mat/utils/freespace.h>
#include <petscbt.h>

#undef __FUNCT__
#define __FUNCT__ "MatCreateMPIAIJSumSeqAIJNumeric"
PetscErrorCode  MatCreateMPIAIJSumSeqAIJNumeric(Mat seqmat,Mat mpimat)
{
  PetscErrorCode      ierr;
  MPI_Comm            comm;
  Mat_SeqAIJ          *a  =(Mat_SeqAIJ*)seqmat->data;
  PetscMPIInt         size,rank,taga,*len_s;
  PetscInt            N=mpimat->cmap->N,i,j,*owners,*ai=a->i,*aj;
  PetscInt            proc,m;
  PetscInt            **buf_ri,**buf_rj;
  PetscInt            k,anzi,*bj_i,*bi,*bj,arow,bnzi,nextaj;
  PetscInt            nrows,**buf_ri_k,**nextrow,**nextai;
  MPI_Request         *s_waits,*r_waits;
  MPI_Status          *status;
  MatScalar           *aa=a->a;
  MatScalar           **abuf_r,*ba_i;
  Mat_Merge_SeqsToMPI *merge;
  PetscContainer      container;

  PetscFunctionBegin;
  ierr = PetscObjectGetComm((PetscObject)mpimat,&comm);CHKERRQ(ierr);
  ierr = PetscLogEventBegin(MAT_Seqstompinum,seqmat,0,0,0);CHKERRQ(ierr);

  ierr = MPI_Comm_size(comm,&size);CHKERRQ(ierr);
  ierr = MPI_Comm_rank(comm,&rank);CHKERRQ(ierr);

  ierr = PetscObjectQuery((PetscObject)mpimat,"MatMergeSeqsToMPI",(PetscObject*)&container);CHKERRQ(ierr);
  ierr = PetscContainerGetPointer(container,(void**)&merge);CHKERRQ(ierr);

  bi     = merge->bi;
  bj     = merge->bj;
  buf_ri = merge->buf_ri;
  buf_rj = merge->buf_rj;

  ierr   = PetscMalloc1(size,&status);CHKERRQ(ierr);
  owners = merge->rowmap->range;
  len_s  = merge->len_s;

  /* send and recv matrix values */
  /*-----------------------------*/
  ierr = PetscObjectGetNewTag((PetscObject)mpimat,&taga);CHKERRQ(ierr);
  ierr = PetscPostIrecvScalar(comm,taga,merge->nrecv,merge->id_r,merge->len_r,&abuf_r,&r_waits);CHKERRQ(ierr);

  ierr = PetscMalloc1(merge->nsend+1,&s_waits);CHKERRQ(ierr);
  for (proc=0,k=0; proc<size; proc++) {
    if (!len_s[proc]) continue;
    i    = owners[proc];
    ierr = MPI_Isend(aa+ai[i],len_s[proc],MPIU_MATSCALAR,proc,taga,comm,s_waits+k);CHKERRQ(ierr);
    k++;
  }

  if (merge->nrecv) {ierr = MPI_Waitall(merge->nrecv,r_waits,status);CHKERRQ(ierr);}
  if (merge->nsend) {ierr = MPI_Waitall(merge->nsend,s_waits,status);CHKERRQ(ierr);}
  ierr = PetscFree(status);CHKERRQ(ierr);

  ierr = PetscFree(s_waits);CHKERRQ(ierr);
  ierr = PetscFree(r_waits);CHKERRQ(ierr);

  /* insert mat values of mpimat */
  /*----------------------------*/
  ierr = PetscMalloc1(N,&ba_i);CHKERRQ(ierr);
  ierr = PetscMalloc3(merge->nrecv,&buf_ri_k,merge->nrecv,&nextrow,merge->nrecv,&nextai);CHKERRQ(ierr);

  for (k=0; k<merge->nrecv; k++) {
    buf_ri_k[k] = buf_ri[k]; /* beginning of k-th recved i-structure */
    nrows       = *(buf_ri_k[k]);
    nextrow[k]  = buf_ri_k[k]+1;  /* next row number of k-th recved i-structure */
    nextai[k]   = buf_ri_k[k] + (nrows + 1); /* poins to the next i-structure of k-th recved i-structure  */
  }

  /* set values of ba */
  m = merge->rowmap->n;
  for (i=0; i<m; i++) {
    arow = owners[rank] + i;
    bj_i = bj+bi[i];  /* col indices of the i-th row of mpimat */
    bnzi = bi[i+1] - bi[i];
    ierr = PetscMemzero(ba_i,bnzi*sizeof(PetscScalar));CHKERRQ(ierr);

    /* add local non-zero vals of this proc's seqmat into ba */
    anzi   = ai[arow+1] - ai[arow];
    aj     = a->j + ai[arow];
    aa     = a->a + ai[arow];
    nextaj = 0;
    for (j=0; nextaj<anzi; j++) {
      if (*(bj_i + j) == aj[nextaj]) { /* bcol == acol */
        ba_i[j] += aa[nextaj++];
      }
    }

    /* add received vals into ba */
    for (k=0; k<merge->nrecv; k++) { /* k-th received message */
      /* i-th row */
      if (i == *nextrow[k]) {
        anzi   = *(nextai[k]+1) - *nextai[k];
        aj     = buf_rj[k] + *(nextai[k]);
        aa     = abuf_r[k] + *(nextai[k]);
        nextaj = 0;
        for (j=0; nextaj<anzi; j++) {
          if (*(bj_i + j) == aj[nextaj]) { /* bcol == acol */
            ba_i[j] += aa[nextaj++];
          }
        }
        nextrow[k]++; nextai[k]++;
      }
    }
    ierr = MatSetValues(mpimat,1,&arow,bnzi,bj_i,ba_i,INSERT_VALUES);CHKERRQ(ierr);
  }
  ierr = MatAssemblyBegin(mpimat,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(mpimat,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);

  ierr = PetscFree(abuf_r[0]);CHKERRQ(ierr);
  ierr = PetscFree(abuf_r);CHKERRQ(ierr);
  ierr = PetscFree(ba_i);CHKERRQ(ierr);
  ierr = PetscFree3(buf_ri_k,nextrow,nextai);CHKERRQ(ierr);
  ierr = PetscLogEventEnd(MAT_Seqstompinum,seqmat,0,0,0);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

extern PetscErrorCode  MatDestroy_MPIAIJ_SeqsToMPI(Mat);

#undef __FUNCT__
#define __FUNCT__ "MatCreateMPIAIJSumSeqAIJSymbolic"
PetscErrorCode  MatCreateMPIAIJSumSeqAIJSymbolic(MPI_Comm comm,Mat seqmat,PetscInt m,PetscInt n,Mat *mpimat)
{
  PetscErrorCode      ierr;
  Mat                 B_mpi;
  Mat_SeqAIJ          *a=(Mat_SeqAIJ*)seqmat->data;
  PetscMPIInt         size,rank,tagi,tagj,*len_s,*len_si,*len_ri;
  PetscInt            **buf_rj,**buf_ri,**buf_ri_k;
  PetscInt            M=seqmat->rmap->n,N=seqmat->cmap->n,i,*owners,*ai=a->i,*aj=a->j;
  PetscInt            len,proc,*dnz,*onz,bs,cbs;
  PetscInt            k,anzi,*bi,*bj,*lnk,nlnk,arow,bnzi,nspacedouble=0;
  PetscInt            nrows,*buf_s,*buf_si,*buf_si_i,**nextrow,**nextai;
  MPI_Request         *si_waits,*sj_waits,*ri_waits,*rj_waits;
  MPI_Status          *status;
  PetscFreeSpaceList  free_space=NULL,current_space=NULL;
  PetscBT             lnkbt;
  Mat_Merge_SeqsToMPI *merge;
  PetscContainer      container;

  PetscFunctionBegin;
  ierr = PetscLogEventBegin(MAT_Seqstompisym,seqmat,0,0,0);CHKERRQ(ierr);

  /* make sure it is a PETSc comm */
  ierr = PetscCommDuplicate(comm,&comm,NULL);CHKERRQ(ierr);
  ierr = MPI_Comm_size(comm,&size);CHKERRQ(ierr);
  ierr = MPI_Comm_rank(comm,&rank);CHKERRQ(ierr);

  ierr = PetscNew(&merge);CHKERRQ(ierr);
  ierr = PetscMalloc1(size,&status);CHKERRQ(ierr);

  /* determine row ownership */
  /*---------------------------------------------------------*/
  ierr = PetscLayoutCreate(comm,&merge->rowmap);CHKERRQ(ierr);
  ierr = PetscLayoutSetLocalSize(merge->rowmap,m);CHKERRQ(ierr);
  ierr = PetscLayoutSetSize(merge->rowmap,M);CHKERRQ(ierr);
  ierr = PetscLayoutSetBlockSize(merge->rowmap,1);CHKERRQ(ierr);
  ierr = PetscLayoutSetUp(merge->rowmap);CHKERRQ(ierr);
  ierr = PetscMalloc1(size,&len_si);CHKERRQ(ierr);
  ierr = PetscMalloc1(size,&merge->len_s);CHKERRQ(ierr);

  m      = merge->rowmap->n;
  owners = merge->rowmap->range;

  /* determine the number of messages to send, their lengths */
  /*---------------------------------------------------------*/
  len_s = merge->len_s;

  len          = 0; /* length of buf_si[] */
  merge->nsend = 0;
  for (proc=0; proc<size; proc++) {
    len_si[proc] = 0;
    if (proc == rank) {
      len_s[proc] = 0;
    } else {
      len_si[proc] = owners[proc+1] - owners[proc] + 1;
      len_s[proc]  = ai[owners[proc+1]] - ai[owners[proc]]; /* num of rows to be sent to [proc] */
    }
    if (len_s[proc]) {
      merge->nsend++;
      nrows = 0;
      for (i=owners[proc]; i<owners[proc+1]; i++) {
        if (ai[i+1] > ai[i]) nrows++;
      }
      len_si[proc] = 2*(nrows+1);
      len         += len_si[proc];
    }
  }

  /* determine the number and length of messages to receive for ij-structure */
  /*-------------------------------------------------------------------------*/
  ierr = PetscGatherNumberOfMessages(comm,NULL,len_s,&merge->nrecv);CHKERRQ(ierr);
  ierr = PetscGatherMessageLengths2(comm,merge->nsend,merge->nrecv,len_s,len_si,&merge->id_r,&merge->len_r,&len_ri);CHKERRQ(ierr);

  /* post the Irecv of j-structure */
  /*-------------------------------*/
  ierr = PetscCommGetNewTag(comm,&tagj);CHKERRQ(ierr);
  ierr = PetscPostIrecvInt(comm,tagj,merge->nrecv,merge->id_r,merge->len_r,&buf_rj,&rj_waits);CHKERRQ(ierr);

  /* post the Isend of j-structure */
  /*--------------------------------*/
  ierr = PetscMalloc2(merge->nsend,&si_waits,merge->nsend,&sj_waits);CHKERRQ(ierr);

  for (proc=0, k=0; proc<size; proc++) {
    if (!len_s[proc]) continue;
    i    = owners[proc];
    ierr = MPI_Isend(aj+ai[i],len_s[proc],MPIU_INT,proc,tagj,comm,sj_waits+k);CHKERRQ(ierr);
    k++;
  }

  /* receives and sends of j-structure are complete */
  /*------------------------------------------------*/
  if (merge->nrecv) {ierr = MPI_Waitall(merge->nrecv,rj_waits,status);CHKERRQ(ierr);}
  if (merge->nsend) {ierr = MPI_Waitall(merge->nsend,sj_waits,status);CHKERRQ(ierr);}

  /* send and recv i-structure */
  /*---------------------------*/
  ierr = PetscCommGetNewTag(comm,&tagi);CHKERRQ(ierr);
  ierr = PetscPostIrecvInt(comm,tagi,merge->nrecv,merge->id_r,len_ri,&buf_ri,&ri_waits);CHKERRQ(ierr);

  ierr   = PetscMalloc1(len+1,&buf_s);CHKERRQ(ierr);
  buf_si = buf_s;  /* points to the beginning of k-th msg to be sent */
  for (proc=0,k=0; proc<size; proc++) {
    if (!len_s[proc]) continue;
    /* form outgoing message for i-structure:
         buf_si[0]:                 nrows to be sent
               [1:nrows]:           row index (global)
               [nrows+1:2*nrows+1]: i-structure index
    */
    /*-------------------------------------------*/
    nrows       = len_si[proc]/2 - 1;
    buf_si_i    = buf_si + nrows+1;
    buf_si[0]   = nrows;
    buf_si_i[0] = 0;
    nrows       = 0;
    for (i=owners[proc]; i<owners[proc+1]; i++) {
      anzi = ai[i+1] - ai[i];
      if (anzi) {
        buf_si_i[nrows+1] = buf_si_i[nrows] + anzi; /* i-structure */
        buf_si[nrows+1]   = i-owners[proc]; /* local row index */
        nrows++;
      }
    }
    ierr = MPI_Isend(buf_si,len_si[proc],MPIU_INT,proc,tagi,comm,si_waits+k);CHKERRQ(ierr);
    k++;
    buf_si += len_si[proc];
  }

  if (merge->nrecv) {ierr = MPI_Waitall(merge->nrecv,ri_waits,status);CHKERRQ(ierr);}
  if (merge->nsend) {ierr = MPI_Waitall(merge->nsend,si_waits,status);CHKERRQ(ierr);}

  ierr = PetscInfo2(seqmat,"nsend: %D, nrecv: %D\n",merge->nsend,merge->nrecv);CHKERRQ(ierr);
  for (i=0; i<merge->nrecv; i++) {
    ierr = PetscInfo3(seqmat,"recv len_ri=%D, len_rj=%D from [%D]\n",len_ri[i],merge->len_r[i],merge->id_r[i]);CHKERRQ(ierr);
  }

  ierr = PetscFree(len_si);CHKERRQ(ierr);
  ierr = PetscFree(len_ri);CHKERRQ(ierr);
  ierr = PetscFree(rj_waits);CHKERRQ(ierr);
  ierr = PetscFree2(si_waits,sj_waits);CHKERRQ(ierr);
  ierr = PetscFree(ri_waits);CHKERRQ(ierr);
  ierr = PetscFree(buf_s);CHKERRQ(ierr);
  ierr = PetscFree(status);CHKERRQ(ierr);

  /* compute a local seq matrix in each processor */
  /*----------------------------------------------*/
  /* allocate bi array and free space for accumulating nonzero column info */
  ierr  = PetscMalloc1(m+1,&bi);CHKERRQ(ierr);
  bi[0] = 0;

  /* create and initialize a linked list */
  nlnk = N+1;
  ierr = PetscLLCreate(N,N,nlnk,lnk,lnkbt);CHKERRQ(ierr);

  /* initial FreeSpace size is 2*(num of local nnz(seqmat)) */
  len  = ai[owners[rank+1]] - ai[owners[rank]];
  ierr = PetscFreeSpaceGet(PetscIntMultTruncate(2,len)+1,&free_space);CHKERRQ(ierr);

  current_space = free_space;

  /* determine symbolic info for each local row */
  ierr = PetscMalloc3(merge->nrecv,&buf_ri_k,merge->nrecv,&nextrow,merge->nrecv,&nextai);CHKERRQ(ierr);

  for (k=0; k<merge->nrecv; k++) {
    buf_ri_k[k] = buf_ri[k]; /* beginning of k-th recved i-structure */
    nrows       = *buf_ri_k[k];
    nextrow[k]  = buf_ri_k[k] + 1;  /* next row number of k-th recved i-structure */
    nextai[k]   = buf_ri_k[k] + (nrows + 1); /* poins to the next i-structure of k-th recved i-structure  */
  }

  ierr = MatPreallocateInitialize(comm,m,n,dnz,onz);CHKERRQ(ierr);
  len  = 0;
  for (i=0; i<m; i++) {
    bnzi = 0;
    /* add local non-zero cols of this proc's seqmat into lnk */
    arow  = owners[rank] + i;
    anzi  = ai[arow+1] - ai[arow];
    aj    = a->j + ai[arow];
    ierr  = PetscLLAddSorted(anzi,aj,N,nlnk,lnk,lnkbt);CHKERRQ(ierr);
    bnzi += nlnk;
    /* add received col data into lnk */
    for (k=0; k<merge->nrecv; k++) { /* k-th received message */
      if (i == *nextrow[k]) { /* i-th row */
        anzi  = *(nextai[k]+1) - *nextai[k];
        aj    = buf_rj[k] + *nextai[k];
        ierr  = PetscLLAddSorted(anzi,aj,N,nlnk,lnk,lnkbt);CHKERRQ(ierr);
        bnzi += nlnk;
        nextrow[k]++; nextai[k]++;
      }
    }
    if (len < bnzi) len = bnzi;  /* =max(bnzi) */

    /* if free space is not available, make more free space */
    if (current_space->local_remaining<bnzi) {
      ierr = PetscFreeSpaceGet(PetscIntSumTruncate(bnzi,current_space->total_array_size),&current_space);CHKERRQ(ierr);
      nspacedouble++;
    }
    /* copy data into free space, then initialize lnk */
    ierr = PetscLLClean(N,N,bnzi,lnk,current_space->array,lnkbt);CHKERRQ(ierr);
    ierr = MatPreallocateSet(i+owners[rank],bnzi,current_space->array,dnz,onz);CHKERRQ(ierr);

    current_space->array           += bnzi;
    current_space->local_used      += bnzi;
    current_space->local_remaining -= bnzi;

    bi[i+1] = bi[i] + bnzi;
  }

  ierr = PetscFree3(buf_ri_k,nextrow,nextai);CHKERRQ(ierr);

  ierr = PetscMalloc1(bi[m]+1,&bj);CHKERRQ(ierr);
  ierr = PetscFreeSpaceContiguous(&free_space,bj);CHKERRQ(ierr);
  ierr = PetscLLDestroy(lnk,lnkbt);CHKERRQ(ierr);

  /* create symbolic parallel matrix B_mpi */
  /*---------------------------------------*/
  ierr = MatGetBlockSizes(seqmat,&bs,&cbs);CHKERRQ(ierr);
  ierr = MatCreate(comm,&B_mpi);CHKERRQ(ierr);
  if (n==PETSC_DECIDE) {
    ierr = MatSetSizes(B_mpi,m,n,PETSC_DETERMINE,N);CHKERRQ(ierr);
  } else {
    ierr = MatSetSizes(B_mpi,m,n,PETSC_DETERMINE,PETSC_DETERMINE);CHKERRQ(ierr);
  }
  ierr = MatSetBlockSizes(B_mpi,bs,cbs);CHKERRQ(ierr);
  ierr = MatSetType(B_mpi,MATMPIAIJ);CHKERRQ(ierr);
  ierr = MatMPIAIJSetPreallocation(B_mpi,0,dnz,0,onz);CHKERRQ(ierr);
  ierr = MatPreallocateFinalize(dnz,onz);CHKERRQ(ierr);
  ierr = MatSetOption(B_mpi,MAT_NEW_NONZERO_ALLOCATION_ERR,PETSC_FALSE);CHKERRQ(ierr);

  /* B_mpi is not ready for use - assembly will be done by MatCreateMPIAIJSumSeqAIJNumeric() */
  B_mpi->assembled    = PETSC_FALSE;
  B_mpi->ops->destroy = MatDestroy_MPIAIJ_SeqsToMPI;
  merge->bi           = bi;
  merge->bj           = bj;
  merge->buf_ri       = buf_ri;
  merge->buf_rj       = buf_rj;
  merge->coi          = NULL;
  merge->coj          = NULL;
  merge->owners_co    = NULL;

  ierr = PetscCommDestroy(&comm);CHKERRQ(ierr);

  /* attach the supporting struct to B_mpi for reuse */
  ierr    = PetscContainerCreate(PETSC_COMM_SELF,&container);CHKERRQ(ierr);
  ierr    = PetscContainerSetPointer(container,merge);CHKERRQ(ierr);
  ierr    = PetscObjectCompose((PetscObject)B_mpi,"MatMergeSeqsToMPI",(PetscObject)container);CHKERRQ(ierr);
  ierr    = PetscContainerDestroy(&container);CHKERRQ(ierr);
  *mpimat = B_mpi;

  ierr = PetscLogEventEnd(MAT_Seqstompisym,seqmat,0,0,0);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatCreateMPIAIJSumSeqAIJ"
/*@C
      MatCreateMPIAIJSumSeqAIJ - Creates a MPIAIJ matrix by adding sequential
                 matrices from each processor

    Collective on MPI_Comm

   Input Parameters:
+    comm - the communicators the parallel matrix will live on
.    seqmat - the input sequential matrices
.    m - number of local rows (or PETSC_DECIDE)
.    n - number of local columns (or PETSC_DECIDE)
-    scall - either MAT_INITIAL_MATRIX or MAT_REUSE_MATRIX

   Output Parameter:
.    mpimat - the parallel matrix generated

    Level: advanced

   Notes:
     The dimensions of the sequential matrix in each processor MUST be the same.
     The input seqmat is included into the container "Mat_Merge_SeqsToMPI", and will be
     destroyed when mpimat is destroyed. Call PetscObjectQuery() to access seqmat.
@*/
PetscErrorCode  MatCreateMPIAIJSumSeqAIJ(MPI_Comm comm,Mat seqmat,PetscInt m,PetscInt n,MatReuse scall,Mat *mpimat)
{
  PetscErrorCode ierr;
  PetscMPIInt    size;

  PetscFunctionBegin;
  ierr = MPI_Comm_size(comm,&size);CHKERRQ(ierr);
  if (size == 1) {
    ierr = PetscLogEventBegin(MAT_Seqstompi,seqmat,0,0,0);CHKERRQ(ierr);
    if (scall == MAT_INITIAL_MATRIX) {
      ierr = MatDuplicate(seqmat,MAT_COPY_VALUES,mpimat);CHKERRQ(ierr);
    } else {
      ierr = MatCopy(seqmat,*mpimat,SAME_NONZERO_PATTERN);CHKERRQ(ierr);
    }
    ierr = PetscLogEventEnd(MAT_Seqstompi,seqmat,0,0,0);CHKERRQ(ierr);
    PetscFunctionReturn(0);
  }
  ierr = PetscLogEventBegin(MAT_Seqstompi,seqmat,0,0,0);CHKERRQ(ierr);
  if (scall == MAT_INITIAL_MATRIX) {
    ierr = MatCreateMPIAIJSumSeqAIJSymbolic(comm,seqmat,m,n,mpimat);CHKERRQ(ierr);
  }
  ierr = MatCreateMPIAIJSumSeqAIJNumeric(seqmat,*mpimat);CHKERRQ(ierr);
  ierr = PetscLogEventEnd(MAT_Seqstompi,seqmat,0,0,0);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatMPIAIJGetLocalMat"
/*@
     MatMPIAIJGetLocalMat - Creates a SeqAIJ from a MPIAIJ matrix by taking all its local rows and putting them into a sequential vector with
          mlocal rows and n columns. Where mlocal is the row count obtained with MatGetLocalSize() and n is the global column count obtained
          with MatGetSize()

    Not Collective

   Input Parameters:
+    A - the matrix
.    scall - either MAT_INITIAL_MATRIX or MAT_REUSE_MATRIX

   Output Parameter:
.    A_loc - the local sequential matrix generated

    Level: developer

.seealso: MatGetOwnerShipRange(), MatMPIAIJGetLocalMatCondensed()

@*/
PetscErrorCode  MatMPIAIJGetLocalMat(Mat A,MatReuse scall,Mat *A_loc)
{
  PetscErrorCode ierr;
  Mat_MPIAIJ     *mpimat=(Mat_MPIAIJ*)A->data;
  Mat_SeqAIJ     *mat,*a,*b;
  PetscInt       *ai,*aj,*bi,*bj,*cmap=mpimat->garray;
  MatScalar      *aa,*ba,*cam;
  PetscScalar    *ca;
  PetscInt       am=A->rmap->n,i,j,k,cstart=A->cmap->rstart;
  PetscInt       *ci,*cj,col,ncols_d,ncols_o,jo;
  PetscBool      match;
  MPI_Comm       comm;
  PetscMPIInt    size;

  PetscFunctionBegin;
  ierr = PetscObjectTypeCompare((PetscObject)A,MATMPIAIJ,&match);CHKERRQ(ierr);
  if (!match) SETERRQ(PetscObjectComm((PetscObject)A), PETSC_ERR_SUP,"Requires MPIAIJ matrix as input");
  ierr = PetscObjectGetComm((PetscObject)A,&comm);CHKERRQ(ierr);
  ierr = MPI_Comm_size(comm,&size);CHKERRQ(ierr);
  if (size == 1 && scall == MAT_REUSE_MATRIX) PetscFunctionReturn(0);

  ierr = PetscLogEventBegin(MAT_Getlocalmat,A,0,0,0);CHKERRQ(ierr);
  a = (Mat_SeqAIJ*)(mpimat->A)->data;
  b = (Mat_SeqAIJ*)(mpimat->B)->data;
  ai = a->i; aj = a->j; bi = b->i; bj = b->j;
  aa = a->a; ba = b->a;
  if (scall == MAT_INITIAL_MATRIX) {
    if (size == 1) {
      ierr = MatCreateSeqAIJWithArrays(PETSC_COMM_SELF,am,A->cmap->N,ai,aj,aa,A_loc);CHKERRQ(ierr);
      PetscFunctionReturn(0);
    }

    ierr  = PetscMalloc1(1+am,&ci);CHKERRQ(ierr);
    ci[0] = 0;
    for (i=0; i<am; i++) {
      ci[i+1] = ci[i] + (ai[i+1] - ai[i]) + (bi[i+1] - bi[i]);
    }
    ierr = PetscMalloc1(1+ci[am],&cj);CHKERRQ(ierr);
    ierr = PetscMalloc1(1+ci[am],&ca);CHKERRQ(ierr);
    k    = 0;
    for (i=0; i<am; i++) {
      ncols_o = bi[i+1] - bi[i];
      ncols_d = ai[i+1] - ai[i];
      /* off-diagonal portion of A */
      for (jo=0; jo<ncols_o; jo++) {
        col = cmap[*bj];
        if (col >= cstart) break;
        cj[k]   = col; bj++;
        ca[k++] = *ba++;
      }
      /* diagonal portion of A */
      for (j=0; j<ncols_d; j++) {
        cj[k]   = cstart + *aj++;
        ca[k++] = *aa++;
      }
      /* off-diagonal portion of A */
      for (j=jo; j<ncols_o; j++) {
        cj[k]   = cmap[*bj++];
        ca[k++] = *ba++;
      }
    }
    /* put together the new matrix */
    ierr = MatCreateSeqAIJWithArrays(PETSC_COMM_SELF,am,A->cmap->N,ci,cj,ca,A_loc);CHKERRQ(ierr);
    /* MatCreateSeqAIJWithArrays flags matrix so PETSc doesn't free the user's arrays. */
    /* Since these are PETSc arrays, change flags to free them as necessary. */
    mat          = (Mat_SeqAIJ*)(*A_loc)->data;
    mat->free_a  = PETSC_TRUE;
    mat->free_ij = PETSC_TRUE;
    mat->nonew   = 0;
  } else if (scall == MAT_REUSE_MATRIX) {
    mat=(Mat_SeqAIJ*)(*A_loc)->data;
    ci = mat->i; cj = mat->j; cam = mat->a;
    for (i=0; i<am; i++) {
      /* off-diagonal portion of A */
      ncols_o = bi[i+1] - bi[i];
      for (jo=0; jo<ncols_o; jo++) {
        col = cmap[*bj];
        if (col >= cstart) break;
        *cam++ = *ba++; bj++;
      }
      /* diagonal portion of A */
      ncols_d = ai[i+1] - ai[i];
      for (j=0; j<ncols_d; j++) *cam++ = *aa++;
      /* off-diagonal portion of A */
      for (j=jo; j<ncols_o; j++) {
        *cam++ = *ba++; bj++;
      }
    }
  } else SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONG,"Invalid MatReuse %d",(int)scall);
  ierr = PetscLogEventEnd(MAT_Getlocalmat,A,0,0,0);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatMPIAIJGetLocalMatCondensed"
/*@C
     MatMPIAIJGetLocalMatCondensed - Creates a SeqAIJ matrix from an MPIAIJ matrix by taking all its local rows and NON-ZERO columns

    Not Collective

   Input Parameters:
+    A - the matrix
.    scall - either MAT_INITIAL_MATRIX or MAT_REUSE_MATRIX
-    row, col - index sets of rows and columns to extract (or NULL)

   Output Parameter:
.    A_loc - the local sequential matrix generated

    Level: developer

.seealso: MatGetOwnershipRange(), MatMPIAIJGetLocalMat()

@*/
PetscErrorCode  MatMPIAIJGetLocalMatCondensed(Mat A,MatReuse scall,IS *row,IS *col,Mat *A_loc)
{
  Mat_MPIAIJ     *a=(Mat_MPIAIJ*)A->data;
  PetscErrorCode ierr;
  PetscInt       i,start,end,ncols,nzA,nzB,*cmap,imark,*idx;
  IS             isrowa,iscola;
  Mat            *aloc;
  PetscBool      match;

  PetscFunctionBegin;
  ierr = PetscObjectTypeCompare((PetscObject)A,MATMPIAIJ,&match);CHKERRQ(ierr);
  if (!match) SETERRQ(PetscObjectComm((PetscObject)A), PETSC_ERR_SUP,"Requires MPIAIJ matrix as input");
  ierr = PetscLogEventBegin(MAT_Getlocalmatcondensed,A,0,0,0);CHKERRQ(ierr);
  if (!row) {
    start = A->rmap->rstart; end = A->rmap->rend;
    ierr  = ISCreateStride(PETSC_COMM_SELF,end-start,start,1,&isrowa);CHKERRQ(ierr);
  } else {
    isrowa = *row;
  }
  if (!col) {
    start = A->cmap->rstart;
    cmap  = a->garray;
    nzA   = a->A->cmap->n;
    nzB   = a->B->cmap->n;
    ierr  = PetscMalloc1(nzA+nzB, &idx);CHKERRQ(ierr);
    ncols = 0;
    for (i=0; i<nzB; i++) {
      if (cmap[i] < start) idx[ncols++] = cmap[i];
      else break;
    }
    imark = i;
    for (i=0; i<nzA; i++) idx[ncols++] = start + i;
    for (i=imark; i<nzB; i++) idx[ncols++] = cmap[i];
    ierr = ISCreateGeneral(PETSC_COMM_SELF,ncols,idx,PETSC_OWN_POINTER,&iscola);CHKERRQ(ierr);
  } else {
    iscola = *col;
  }
  if (scall != MAT_INITIAL_MATRIX) {
    ierr    = PetscMalloc1(1,&aloc);CHKERRQ(ierr);
    aloc[0] = *A_loc;
  }
  ierr   = MatGetSubMatrices(A,1,&isrowa,&iscola,scall,&aloc);CHKERRQ(ierr);
  *A_loc = aloc[0];
  ierr   = PetscFree(aloc);CHKERRQ(ierr);
  if (!row) {
    ierr = ISDestroy(&isrowa);CHKERRQ(ierr);
  }
  if (!col) {
    ierr = ISDestroy(&iscola);CHKERRQ(ierr);
  }
  ierr = PetscLogEventEnd(MAT_Getlocalmatcondensed,A,0,0,0);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatGetBrowsOfAcols"
/*@C
    MatGetBrowsOfAcols - Creates a SeqAIJ matrix by taking rows of B that equal to nonzero columns of local A

    Collective on Mat

   Input Parameters:
+    A,B - the matrices in mpiaij format
.    scall - either MAT_INITIAL_MATRIX or MAT_REUSE_MATRIX
-    rowb, colb - index sets of rows and columns of B to extract (or NULL)

   Output Parameter:
+    rowb, colb - index sets of rows and columns of B to extract
-    B_seq - the sequential matrix generated

    Level: developer

@*/
PetscErrorCode  MatGetBrowsOfAcols(Mat A,Mat B,MatReuse scall,IS *rowb,IS *colb,Mat *B_seq)
{
  Mat_MPIAIJ     *a=(Mat_MPIAIJ*)A->data;
  PetscErrorCode ierr;
  PetscInt       *idx,i,start,ncols,nzA,nzB,*cmap,imark;
  IS             isrowb,iscolb;
  Mat            *bseq=NULL;

  PetscFunctionBegin;
  if (A->cmap->rstart != B->rmap->rstart || A->cmap->rend != B->rmap->rend) {
    SETERRQ4(PETSC_COMM_SELF,PETSC_ERR_ARG_SIZ,"Matrix local dimensions are incompatible, (%D, %D) != (%D,%D)",A->cmap->rstart,A->cmap->rend,B->rmap->rstart,B->rmap->rend);
  }
  ierr = PetscLogEventBegin(MAT_GetBrowsOfAcols,A,B,0,0);CHKERRQ(ierr);

  if (scall == MAT_INITIAL_MATRIX) {
    start = A->cmap->rstart;
    cmap  = a->garray;
    nzA   = a->A->cmap->n;
    nzB   = a->B->cmap->n;
    ierr  = PetscMalloc1(nzA+nzB, &idx);CHKERRQ(ierr);
    ncols = 0;
    for (i=0; i<nzB; i++) {  /* row < local row index */
      if (cmap[i] < start) idx[ncols++] = cmap[i];
      else break;
    }
    imark = i;
    for (i=0; i<nzA; i++) idx[ncols++] = start + i;  /* local rows */
    for (i=imark; i<nzB; i++) idx[ncols++] = cmap[i]; /* row > local row index */
    ierr = ISCreateGeneral(PETSC_COMM_SELF,ncols,idx,PETSC_OWN_POINTER,&isrowb);CHKERRQ(ierr);
    ierr = ISCreateStride(PETSC_COMM_SELF,B->cmap->N,0,1,&iscolb);CHKERRQ(ierr);
  } else {
    if (!rowb || !colb) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_SUP,"IS rowb and colb must be provided for MAT_REUSE_MATRIX");
    isrowb  = *rowb; iscolb = *colb;
    ierr    = PetscMalloc1(1,&bseq);CHKERRQ(ierr);
    bseq[0] = *B_seq;
  }
  ierr   = MatGetSubMatrices(B,1,&isrowb,&iscolb,scall,&bseq);CHKERRQ(ierr);
  *B_seq = bseq[0];
  ierr   = PetscFree(bseq);CHKERRQ(ierr);
  if (!rowb) {
    ierr = ISDestroy(&isrowb);CHKERRQ(ierr);
  } else {
    *rowb = isrowb;
  }
  if (!colb) {
    ierr = ISDestroy(&iscolb);CHKERRQ(ierr);
  } else {
    *colb = iscolb;
  }
  ierr = PetscLogEventEnd(MAT_GetBrowsOfAcols,A,B,0,0);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatGetBrowsOfAoCols_MPIAIJ"
/*
    MatGetBrowsOfAoCols_MPIAIJ - Creates a SeqAIJ matrix by taking rows of B that equal to nonzero columns
    of the OFF-DIAGONAL portion of local A

    Collective on Mat

   Input Parameters:
+    A,B - the matrices in mpiaij format
-    scall - either MAT_INITIAL_MATRIX or MAT_REUSE_MATRIX

   Output Parameter:
+    startsj_s - starting point in B's sending j-arrays, saved for MAT_REUSE (or NULL)
.    startsj_r - starting point in B's receiving j-arrays, saved for MAT_REUSE (or NULL)
.    bufa_ptr - array for sending matrix values, saved for MAT_REUSE (or NULL)
-    B_oth - the sequential matrix generated with size aBn=a->B->cmap->n by B->cmap->N

    Level: developer

*/
PetscErrorCode  MatGetBrowsOfAoCols_MPIAIJ(Mat A,Mat B,MatReuse scall,PetscInt **startsj_s,PetscInt **startsj_r,MatScalar **bufa_ptr,Mat *B_oth)
{
  VecScatter_MPI_General *gen_to,*gen_from;
  PetscErrorCode         ierr;
  Mat_MPIAIJ             *a=(Mat_MPIAIJ*)A->data;
  Mat_SeqAIJ             *b_oth;
  VecScatter             ctx =a->Mvctx;
  MPI_Comm               comm;
  PetscMPIInt            *rprocs,*sprocs,tag=((PetscObject)ctx)->tag,rank;
  PetscInt               *rowlen,*bufj,*bufJ,ncols,aBn=a->B->cmap->n,row,*b_othi,*b_othj;
  PetscScalar            *rvalues,*svalues;
  MatScalar              *b_otha,*bufa,*bufA;
  PetscInt               i,j,k,l,ll,nrecvs,nsends,nrows,*srow,*rstarts,*rstartsj = 0,*sstarts,*sstartsj,len;
  MPI_Request            *rwaits = NULL,*swaits = NULL;
  MPI_Status             *sstatus,rstatus;
  PetscMPIInt            jj,size;
  PetscInt               *cols,sbs,rbs;
  PetscScalar            *vals;

  PetscFunctionBegin;
  ierr = PetscObjectGetComm((PetscObject)A,&comm);CHKERRQ(ierr);
  ierr = MPI_Comm_size(comm,&size);CHKERRQ(ierr);

  if (A->cmap->rstart != B->rmap->rstart || A->cmap->rend != B->rmap->rend) {
    SETERRQ4(PETSC_COMM_SELF,PETSC_ERR_ARG_SIZ,"Matrix local dimensions are incompatible, (%d, %d) != (%d,%d)",A->cmap->rstart,A->cmap->rend,B->rmap->rstart,B->rmap->rend);
  }
  ierr = PetscLogEventBegin(MAT_GetBrowsOfAocols,A,B,0,0);CHKERRQ(ierr);
  ierr = MPI_Comm_rank(comm,&rank);CHKERRQ(ierr);

  gen_to   = (VecScatter_MPI_General*)ctx->todata;
  gen_from = (VecScatter_MPI_General*)ctx->fromdata;
  rvalues  = gen_from->values; /* holds the length of receiving row */
  svalues  = gen_to->values;   /* holds the length of sending row */
  nrecvs   = gen_from->n;
  nsends   = gen_to->n;

  ierr    = PetscMalloc2(nrecvs,&rwaits,nsends,&swaits);CHKERRQ(ierr);
  srow    = gen_to->indices;    /* local row index to be sent */
  sstarts = gen_to->starts;
  sprocs  = gen_to->procs;
  sstatus = gen_to->sstatus;
  sbs     = gen_to->bs;
  rstarts = gen_from->starts;
  rprocs  = gen_from->procs;
  rbs     = gen_from->bs;

  if (!startsj_s || !bufa_ptr) scall = MAT_INITIAL_MATRIX;
  if (scall == MAT_INITIAL_MATRIX) {
    /* i-array */
    /*---------*/
    /*  post receives */
    for (i=0; i<nrecvs; i++) {
      rowlen = (PetscInt*)rvalues + rstarts[i]*rbs;
      nrows  = (rstarts[i+1]-rstarts[i])*rbs; /* num of indices to be received */
      ierr   = MPI_Irecv(rowlen,nrows,MPIU_INT,rprocs[i],tag,comm,rwaits+i);CHKERRQ(ierr);
    }

    /* pack the outgoing message */
    ierr = PetscMalloc2(nsends+1,&sstartsj,nrecvs+1,&rstartsj);CHKERRQ(ierr);

    sstartsj[0] = 0;
    rstartsj[0] = 0;
    len         = 0; /* total length of j or a array to be sent */
    k           = 0;
    for (i=0; i<nsends; i++) {
      rowlen = (PetscInt*)svalues + sstarts[i]*sbs;
      nrows  = sstarts[i+1]-sstarts[i]; /* num of block rows */
      for (j=0; j<nrows; j++) {
        row = srow[k] + B->rmap->range[rank]; /* global row idx */
        for (l=0; l<sbs; l++) {
          ierr = MatGetRow_MPIAIJ(B,row+l,&ncols,NULL,NULL);CHKERRQ(ierr); /* rowlength */

          rowlen[j*sbs+l] = ncols;

          len += ncols;
          ierr = MatRestoreRow_MPIAIJ(B,row+l,&ncols,NULL,NULL);CHKERRQ(ierr);
        }
        k++;
      }
      ierr = MPI_Isend(rowlen,nrows*sbs,MPIU_INT,sprocs[i],tag,comm,swaits+i);CHKERRQ(ierr);

      sstartsj[i+1] = len;  /* starting point of (i+1)-th outgoing msg in bufj and bufa */
    }
    /* recvs and sends of i-array are completed */
    i = nrecvs;
    while (i--) {
      ierr = MPI_Waitany(nrecvs,rwaits,&jj,&rstatus);CHKERRQ(ierr);
    }
    if (nsends) {ierr = MPI_Waitall(nsends,swaits,sstatus);CHKERRQ(ierr);}

    /* allocate buffers for sending j and a arrays */
    ierr = PetscMalloc1(len+1,&bufj);CHKERRQ(ierr);
    ierr = PetscMalloc1(len+1,&bufa);CHKERRQ(ierr);

    /* create i-array of B_oth */
    ierr = PetscMalloc1(aBn+2,&b_othi);CHKERRQ(ierr);

    b_othi[0] = 0;
    len       = 0; /* total length of j or a array to be received */
    k         = 0;
    for (i=0; i<nrecvs; i++) {
      rowlen = (PetscInt*)rvalues + rstarts[i]*rbs;
      nrows  = rbs*(rstarts[i+1]-rstarts[i]); /* num of rows to be received */
      for (j=0; j<nrows; j++) {
        b_othi[k+1] = b_othi[k] + rowlen[j];
        ierr = PetscIntSumError(rowlen[j],len,&len);
        k++;
      }
      rstartsj[i+1] = len; /* starting point of (i+1)-th incoming msg in bufj and bufa */
    }

    /* allocate space for j and a arrrays of B_oth */
    ierr = PetscMalloc1(b_othi[aBn]+1,&b_othj);CHKERRQ(ierr);
    ierr = PetscMalloc1(b_othi[aBn]+1,&b_otha);CHKERRQ(ierr);

    /* j-array */
    /*---------*/
    /*  post receives of j-array */
    for (i=0; i<nrecvs; i++) {
      nrows = rstartsj[i+1]-rstartsj[i]; /* length of the msg received */
      ierr  = MPI_Irecv(b_othj+rstartsj[i],nrows,MPIU_INT,rprocs[i],tag,comm,rwaits+i);CHKERRQ(ierr);
    }

    /* pack the outgoing message j-array */
    k = 0;
    for (i=0; i<nsends; i++) {
      nrows = sstarts[i+1]-sstarts[i]; /* num of block rows */
      bufJ  = bufj+sstartsj[i];
      for (j=0; j<nrows; j++) {
        row = srow[k++] + B->rmap->range[rank];  /* global row idx */
        for (ll=0; ll<sbs; ll++) {
          ierr = MatGetRow_MPIAIJ(B,row+ll,&ncols,&cols,NULL);CHKERRQ(ierr);
          for (l=0; l<ncols; l++) {
            *bufJ++ = cols[l];
          }
          ierr = MatRestoreRow_MPIAIJ(B,row+ll,&ncols,&cols,NULL);CHKERRQ(ierr);
        }
      }
      ierr = MPI_Isend(bufj+sstartsj[i],sstartsj[i+1]-sstartsj[i],MPIU_INT,sprocs[i],tag,comm,swaits+i);CHKERRQ(ierr);
    }

    /* recvs and sends of j-array are completed */
    i = nrecvs;
    while (i--) {
      ierr = MPI_Waitany(nrecvs,rwaits,&jj,&rstatus);CHKERRQ(ierr);
    }
    if (nsends) {ierr = MPI_Waitall(nsends,swaits,sstatus);CHKERRQ(ierr);}
  } else if (scall == MAT_REUSE_MATRIX) {
    sstartsj = *startsj_s;
    rstartsj = *startsj_r;
    bufa     = *bufa_ptr;
    b_oth    = (Mat_SeqAIJ*)(*B_oth)->data;
    b_otha   = b_oth->a;
  } else SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONGSTATE, "Matrix P does not posses an object container");

  /* a-array */
  /*---------*/
  /*  post receives of a-array */
  for (i=0; i<nrecvs; i++) {
    nrows = rstartsj[i+1]-rstartsj[i]; /* length of the msg received */
    ierr  = MPI_Irecv(b_otha+rstartsj[i],nrows,MPIU_SCALAR,rprocs[i],tag,comm,rwaits+i);CHKERRQ(ierr);
  }

  /* pack the outgoing message a-array */
  k = 0;
  for (i=0; i<nsends; i++) {
    nrows = sstarts[i+1]-sstarts[i]; /* num of block rows */
    bufA  = bufa+sstartsj[i];
    for (j=0; j<nrows; j++) {
      row = srow[k++] + B->rmap->range[rank];  /* global row idx */
      for (ll=0; ll<sbs; ll++) {
        ierr = MatGetRow_MPIAIJ(B,row+ll,&ncols,NULL,&vals);CHKERRQ(ierr);
        for (l=0; l<ncols; l++) {
          *bufA++ = vals[l];
        }
        ierr = MatRestoreRow_MPIAIJ(B,row+ll,&ncols,NULL,&vals);CHKERRQ(ierr);
      }
    }
    ierr = MPI_Isend(bufa+sstartsj[i],sstartsj[i+1]-sstartsj[i],MPIU_SCALAR,sprocs[i],tag,comm,swaits+i);CHKERRQ(ierr);
  }
  /* recvs and sends of a-array are completed */
  i = nrecvs;
  while (i--) {
    ierr = MPI_Waitany(nrecvs,rwaits,&jj,&rstatus);CHKERRQ(ierr);
  }
  if (nsends) {ierr = MPI_Waitall(nsends,swaits,sstatus);CHKERRQ(ierr);}
  ierr = PetscFree2(rwaits,swaits);CHKERRQ(ierr);

  if (scall == MAT_INITIAL_MATRIX) {
    /* put together the new matrix */
    ierr = MatCreateSeqAIJWithArrays(PETSC_COMM_SELF,aBn,B->cmap->N,b_othi,b_othj,b_otha,B_oth);CHKERRQ(ierr);

    /* MatCreateSeqAIJWithArrays flags matrix so PETSc doesn't free the user's arrays. */
    /* Since these are PETSc arrays, change flags to free them as necessary. */
    b_oth          = (Mat_SeqAIJ*)(*B_oth)->data;
    b_oth->free_a  = PETSC_TRUE;
    b_oth->free_ij = PETSC_TRUE;
    b_oth->nonew   = 0;

    ierr = PetscFree(bufj);CHKERRQ(ierr);
    if (!startsj_s || !bufa_ptr) {
      ierr = PetscFree2(sstartsj,rstartsj);CHKERRQ(ierr);
      ierr = PetscFree(bufa_ptr);CHKERRQ(ierr);
    } else {
      *startsj_s = sstartsj;
      *startsj_r = rstartsj;
      *bufa_ptr  = bufa;
    }
  }
  ierr = PetscLogEventEnd(MAT_GetBrowsOfAocols,A,B,0,0);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatGetCommunicationStructs"
/*@C
  MatGetCommunicationStructs - Provides access to the communication structures used in matrix-vector multiplication.

  Not Collective

  Input Parameters:
. A - The matrix in mpiaij format

  Output Parameter:
+ lvec - The local vector holding off-process values from the argument to a matrix-vector product
. colmap - A map from global column index to local index into lvec
- multScatter - A scatter from the argument of a matrix-vector product to lvec

  Level: developer

@*/
#if defined(PETSC_USE_CTABLE)
PetscErrorCode  MatGetCommunicationStructs(Mat A, Vec *lvec, PetscTable *colmap, VecScatter *multScatter)
#else
PetscErrorCode  MatGetCommunicationStructs(Mat A, Vec *lvec, PetscInt *colmap[], VecScatter *multScatter)
#endif
{
  Mat_MPIAIJ *a;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(A, MAT_CLASSID, 1);
  PetscValidPointer(lvec, 2);
  PetscValidPointer(colmap, 3);
  PetscValidPointer(multScatter, 4);
  a = (Mat_MPIAIJ*) A->data;
  if (lvec) *lvec = a->lvec;
  if (colmap) *colmap = a->colmap;
  if (multScatter) *multScatter = a->Mvctx;
  PetscFunctionReturn(0);
}

PETSC_EXTERN PetscErrorCode MatConvert_MPIAIJ_MPIAIJCRL(Mat,MatType,MatReuse,Mat*);
PETSC_EXTERN PetscErrorCode MatConvert_MPIAIJ_MPIAIJPERM(Mat,MatType,MatReuse,Mat*);
PETSC_EXTERN PetscErrorCode MatConvert_MPIAIJ_MPISBAIJ(Mat,MatType,MatReuse,Mat*);
#if defined(PETSC_HAVE_ELEMENTAL)
PETSC_EXTERN PetscErrorCode MatConvert_MPIAIJ_Elemental(Mat,MatType,MatReuse,Mat*);
#endif

#undef __FUNCT__
#define __FUNCT__ "MatMatMultNumeric_MPIDense_MPIAIJ"
/*
    Computes (B'*A')' since computing B*A directly is untenable

               n                       p                          p
        (              )       (              )         (                  )
      m (      A       )  *  n (       B      )   =   m (         C        )
        (              )       (              )         (                  )

*/
PetscErrorCode MatMatMultNumeric_MPIDense_MPIAIJ(Mat A,Mat B,Mat C)
{
  PetscErrorCode ierr;
  Mat            At,Bt,Ct;

  PetscFunctionBegin;
  ierr = MatTranspose(A,MAT_INITIAL_MATRIX,&At);CHKERRQ(ierr);
  ierr = MatTranspose(B,MAT_INITIAL_MATRIX,&Bt);CHKERRQ(ierr);
  ierr = MatMatMult(Bt,At,MAT_INITIAL_MATRIX,1.0,&Ct);CHKERRQ(ierr);
  ierr = MatDestroy(&At);CHKERRQ(ierr);
  ierr = MatDestroy(&Bt);CHKERRQ(ierr);
  ierr = MatTranspose(Ct,MAT_REUSE_MATRIX,&C);CHKERRQ(ierr);
  ierr = MatDestroy(&Ct);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatMatMultSymbolic_MPIDense_MPIAIJ"
PetscErrorCode MatMatMultSymbolic_MPIDense_MPIAIJ(Mat A,Mat B,PetscReal fill,Mat *C)
{
  PetscErrorCode ierr;
  PetscInt       m=A->rmap->n,n=B->cmap->n;
  Mat            Cmat;

  PetscFunctionBegin;
  if (A->cmap->n != B->rmap->n) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_SIZ,"A->cmap->n %d != B->rmap->n %d\n",A->cmap->n,B->rmap->n);
  ierr = MatCreate(PetscObjectComm((PetscObject)A),&Cmat);CHKERRQ(ierr);
  ierr = MatSetSizes(Cmat,m,n,PETSC_DETERMINE,PETSC_DETERMINE);CHKERRQ(ierr);
  ierr = MatSetBlockSizesFromMats(Cmat,A,B);CHKERRQ(ierr);
  ierr = MatSetType(Cmat,MATMPIDENSE);CHKERRQ(ierr);
  ierr = MatMPIDenseSetPreallocation(Cmat,NULL);CHKERRQ(ierr);
  ierr = MatAssemblyBegin(Cmat,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(Cmat,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);

  Cmat->ops->matmultnumeric = MatMatMultNumeric_MPIDense_MPIAIJ;

  *C = Cmat;
  PetscFunctionReturn(0);
}

/* ----------------------------------------------------------------*/
#undef __FUNCT__
#define __FUNCT__ "MatMatMult_MPIDense_MPIAIJ"
PetscErrorCode MatMatMult_MPIDense_MPIAIJ(Mat A,Mat B,MatReuse scall,PetscReal fill,Mat *C)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (scall == MAT_INITIAL_MATRIX) {
    ierr = PetscLogEventBegin(MAT_MatMultSymbolic,A,B,0,0);CHKERRQ(ierr);
    ierr = MatMatMultSymbolic_MPIDense_MPIAIJ(A,B,fill,C);CHKERRQ(ierr);
    ierr = PetscLogEventEnd(MAT_MatMultSymbolic,A,B,0,0);CHKERRQ(ierr);
  }
  ierr = PetscLogEventBegin(MAT_MatMultNumeric,A,B,0,0);CHKERRQ(ierr);
  ierr = MatMatMultNumeric_MPIDense_MPIAIJ(A,B,*C);CHKERRQ(ierr);
  ierr = PetscLogEventEnd(MAT_MatMultNumeric,A,B,0,0);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/*MC
   MATMPIAIJ - MATMPIAIJ = "mpiaij" - A matrix type to be used for parallel sparse matrices.

   Options Database Keys:
. -mat_type mpiaij - sets the matrix type to "mpiaij" during a call to MatSetFromOptions()

  Level: beginner

.seealso: MatCreateAIJ()
M*/

#undef __FUNCT__
#define __FUNCT__ "MatCreate_MPIAIJ"
PETSC_EXTERN PetscErrorCode MatCreate_MPIAIJ(Mat B)
{
  Mat_MPIAIJ     *b;
  PetscErrorCode ierr;
  PetscMPIInt    size;

  PetscFunctionBegin;
  ierr = MPI_Comm_size(PetscObjectComm((PetscObject)B),&size);CHKERRQ(ierr);

  ierr          = PetscNewLog(B,&b);CHKERRQ(ierr);
  B->data       = (void*)b;
  ierr          = PetscMemcpy(B->ops,&MatOps_Values,sizeof(struct _MatOps));CHKERRQ(ierr);
  B->assembled  = PETSC_FALSE;
  B->insertmode = NOT_SET_VALUES;
  b->size       = size;

  ierr = MPI_Comm_rank(PetscObjectComm((PetscObject)B),&b->rank);CHKERRQ(ierr);

  /* build cache for off array entries formed */
  ierr = MatStashCreate_Private(PetscObjectComm((PetscObject)B),1,&B->stash);CHKERRQ(ierr);

  b->donotstash  = PETSC_FALSE;
  b->colmap      = 0;
  b->garray      = 0;
  b->roworiented = PETSC_TRUE;

  /* stuff used for matrix vector multiply */
  b->lvec  = NULL;
  b->Mvctx = NULL;

  /* stuff for MatGetRow() */
  b->rowindices   = 0;
  b->rowvalues    = 0;
  b->getrowactive = PETSC_FALSE;

  /* flexible pointer used in CUSP/CUSPARSE classes */
  b->spptr = NULL;

  ierr = PetscObjectComposeFunction((PetscObject)B,"MatMPIAIJSetUseScalableIncreaseOverlap_C",MatMPIAIJSetUseScalableIncreaseOverlap_MPIAIJ);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)B,"MatStoreValues_C",MatStoreValues_MPIAIJ);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)B,"MatRetrieveValues_C",MatRetrieveValues_MPIAIJ);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)B,"MatGetDiagonalBlock_C",MatGetDiagonalBlock_MPIAIJ);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)B,"MatIsTranspose_C",MatIsTranspose_MPIAIJ);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)B,"MatMPIAIJSetPreallocation_C",MatMPIAIJSetPreallocation_MPIAIJ);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)B,"MatMPIAIJSetPreallocationCSR_C",MatMPIAIJSetPreallocationCSR_MPIAIJ);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)B,"MatDiagonalScaleLocal_C",MatDiagonalScaleLocal_MPIAIJ);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)B,"MatConvert_mpiaij_mpiaijperm_C",MatConvert_MPIAIJ_MPIAIJPERM);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)B,"MatConvert_mpiaij_mpiaijcrl_C",MatConvert_MPIAIJ_MPIAIJCRL);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)B,"MatConvert_mpiaij_mpisbaij_C",MatConvert_MPIAIJ_MPISBAIJ);CHKERRQ(ierr);
#if defined(PETSC_HAVE_ELEMENTAL)
  ierr = PetscObjectComposeFunction((PetscObject)B,"MatConvert_mpiaij_elemental_C",MatConvert_MPIAIJ_Elemental);CHKERRQ(ierr);
#endif
  ierr = PetscObjectComposeFunction((PetscObject)B,"MatMatMult_mpidense_mpiaij_C",MatMatMult_MPIDense_MPIAIJ);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)B,"MatMatMultSymbolic_mpidense_mpiaij_C",MatMatMultSymbolic_MPIDense_MPIAIJ);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)B,"MatMatMultNumeric_mpidense_mpiaij_C",MatMatMultNumeric_MPIDense_MPIAIJ);CHKERRQ(ierr);
  ierr = PetscObjectChangeTypeName((PetscObject)B,MATMPIAIJ);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatCreateMPIAIJWithSplitArrays"
/*@C
     MatCreateMPIAIJWithSplitArrays - creates a MPI AIJ matrix using arrays that contain the "diagonal"
         and "off-diagonal" part of the matrix in CSR format.

   Collective on MPI_Comm

   Input Parameters:
+  comm - MPI communicator
.  m - number of local rows (Cannot be PETSC_DECIDE)
.  n - This value should be the same as the local size used in creating the
       x vector for the matrix-vector product y = Ax. (or PETSC_DECIDE to have
       calculated if N is given) For square matrices n is almost always m.
.  M - number of global rows (or PETSC_DETERMINE to have calculated if m is given)
.  N - number of global columns (or PETSC_DETERMINE to have calculated if n is given)
.   i - row indices for "diagonal" portion of matrix
.   j - column indices
.   a - matrix values
.   oi - row indices for "off-diagonal" portion of matrix
.   oj - column indices
-   oa - matrix values

   Output Parameter:
.   mat - the matrix

   Level: advanced

   Notes:
       The i, j, and a arrays ARE NOT copied by this routine into the internal format used by PETSc. The user
       must free the arrays once the matrix has been destroyed and not before.

       The i and j indices are 0 based

       See MatCreateAIJ() for the definition of "diagonal" and "off-diagonal" portion of the matrix

       This sets local rows and cannot be used to set off-processor values.

       Use of this routine is discouraged because it is inflexible and cumbersome to use. It is extremely rare that a
       legacy application natively assembles into exactly this split format. The code to do so is nontrivial and does
       not easily support in-place reassembly. It is recommended to use MatSetValues() (or a variant thereof) because
       the resulting assembly is easier to implement, will work with any matrix format, and the user does not have to
       keep track of the underlying array. Use MatSetOption(A,MAT_IGNORE_OFF_PROC_ENTRIES,PETSC_TRUE) to disable all
       communication if it is known that only local entries will be set.

.keywords: matrix, aij, compressed row, sparse, parallel

.seealso: MatCreate(), MatCreateSeqAIJ(), MatSetValues(), MatMPIAIJSetPreallocation(), MatMPIAIJSetPreallocationCSR(),
          MPIAIJ, MatCreateAIJ(), MatCreateMPIAIJWithArrays()
@*/
PetscErrorCode  MatCreateMPIAIJWithSplitArrays(MPI_Comm comm,PetscInt m,PetscInt n,PetscInt M,PetscInt N,PetscInt i[],PetscInt j[],PetscScalar a[],PetscInt oi[], PetscInt oj[],PetscScalar oa[],Mat *mat)
{
  PetscErrorCode ierr;
  Mat_MPIAIJ     *maij;

  PetscFunctionBegin;
  if (m < 0) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"local number of rows (m) cannot be PETSC_DECIDE, or negative");
  if (i[0]) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"i (row indices) must start with 0");
  if (oi[0]) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"oi (row indices) must start with 0");
  ierr = MatCreate(comm,mat);CHKERRQ(ierr);
  ierr = MatSetSizes(*mat,m,n,M,N);CHKERRQ(ierr);
  ierr = MatSetType(*mat,MATMPIAIJ);CHKERRQ(ierr);
  maij = (Mat_MPIAIJ*) (*mat)->data;

  (*mat)->preallocated = PETSC_TRUE;

  ierr = PetscLayoutSetUp((*mat)->rmap);CHKERRQ(ierr);
  ierr = PetscLayoutSetUp((*mat)->cmap);CHKERRQ(ierr);

  ierr = MatCreateSeqAIJWithArrays(PETSC_COMM_SELF,m,n,i,j,a,&maij->A);CHKERRQ(ierr);
  ierr = MatCreateSeqAIJWithArrays(PETSC_COMM_SELF,m,(*mat)->cmap->N,oi,oj,oa,&maij->B);CHKERRQ(ierr);

  ierr = MatAssemblyBegin(maij->A,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(maij->A,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyBegin(maij->B,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(maij->B,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);

  ierr = MatAssemblyBegin(*mat,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(*mat,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatSetOption(*mat,MAT_NEW_NONZERO_LOCATION_ERR,PETSC_TRUE);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/*
    Special version for direct calls from Fortran
*/
#include <petsc/private/fortranimpl.h>

#if defined(PETSC_HAVE_FORTRAN_CAPS)
#define matsetvaluesmpiaij_ MATSETVALUESMPIAIJ
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE)
#define matsetvaluesmpiaij_ matsetvaluesmpiaij
#endif

/* Change these macros so can be used in void function */
#undef CHKERRQ
#define CHKERRQ(ierr) CHKERRABORT(PETSC_COMM_WORLD,ierr)
#undef SETERRQ2
#define SETERRQ2(comm,ierr,b,c,d) CHKERRABORT(comm,ierr)
#undef SETERRQ3
#define SETERRQ3(comm,ierr,b,c,d,e) CHKERRABORT(comm,ierr)
#undef SETERRQ
#define SETERRQ(c,ierr,b) CHKERRABORT(c,ierr)

#undef __FUNCT__
#define __FUNCT__ "matsetvaluesmpiaij_"
PETSC_EXTERN void PETSC_STDCALL matsetvaluesmpiaij_(Mat *mmat,PetscInt *mm,const PetscInt im[],PetscInt *mn,const PetscInt in[],const PetscScalar v[],InsertMode *maddv,PetscErrorCode *_ierr)
{
  Mat            mat  = *mmat;
  PetscInt       m    = *mm, n = *mn;
  InsertMode     addv = *maddv;
  Mat_MPIAIJ     *aij = (Mat_MPIAIJ*)mat->data;
  PetscScalar    value;
  PetscErrorCode ierr;

  MatCheckPreallocated(mat,1);
  if (mat->insertmode == NOT_SET_VALUES) mat->insertmode = addv;

#if defined(PETSC_USE_DEBUG)
  else if (mat->insertmode != addv) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONGSTATE,"Cannot mix add values and insert values");
#endif
  {
    PetscInt  i,j,rstart  = mat->rmap->rstart,rend = mat->rmap->rend;
    PetscInt  cstart      = mat->cmap->rstart,cend = mat->cmap->rend,row,col;
    PetscBool roworiented = aij->roworiented;

    /* Some Variables required in the macro */
    Mat        A                 = aij->A;
    Mat_SeqAIJ *a                = (Mat_SeqAIJ*)A->data;
    PetscInt   *aimax            = a->imax,*ai = a->i,*ailen = a->ilen,*aj = a->j;
    MatScalar  *aa               = a->a;
    PetscBool  ignorezeroentries = (((a->ignorezeroentries)&&(addv==ADD_VALUES)) ? PETSC_TRUE : PETSC_FALSE);
    Mat        B                 = aij->B;
    Mat_SeqAIJ *b                = (Mat_SeqAIJ*)B->data;
    PetscInt   *bimax            = b->imax,*bi = b->i,*bilen = b->ilen,*bj = b->j,bm = aij->B->rmap->n,am = aij->A->rmap->n;
    MatScalar  *ba               = b->a;

    PetscInt  *rp1,*rp2,ii,nrow1,nrow2,_i,rmax1,rmax2,N,low1,high1,low2,high2,t,lastcol1,lastcol2;
    PetscInt  nonew = a->nonew;
    MatScalar *ap1,*ap2;

    PetscFunctionBegin;
    for (i=0; i<m; i++) {
      if (im[i] < 0) continue;
#if defined(PETSC_USE_DEBUG)
      if (im[i] >= mat->rmap->N) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Row too large: row %D max %D",im[i],mat->rmap->N-1);
#endif
      if (im[i] >= rstart && im[i] < rend) {
        row      = im[i] - rstart;
        lastcol1 = -1;
        rp1      = aj + ai[row];
        ap1      = aa + ai[row];
        rmax1    = aimax[row];
        nrow1    = ailen[row];
        low1     = 0;
        high1    = nrow1;
        lastcol2 = -1;
        rp2      = bj + bi[row];
        ap2      = ba + bi[row];
        rmax2    = bimax[row];
        nrow2    = bilen[row];
        low2     = 0;
        high2    = nrow2;

        for (j=0; j<n; j++) {
          if (roworiented) value = v[i*n+j];
          else value = v[i+j*m];
          if (ignorezeroentries && value == 0.0 && (addv == ADD_VALUES)) continue;
          if (in[j] >= cstart && in[j] < cend) {
            col = in[j] - cstart;
            MatSetValues_SeqAIJ_A_Private(row,col,value,addv,im[i],in[j]);
          } else if (in[j] < 0) continue;
#if defined(PETSC_USE_DEBUG)
          else if (in[j] >= mat->cmap->N) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Column too large: col %D max %D",in[j],mat->cmap->N-1);
#endif
          else {
            if (mat->was_assembled) {
              if (!aij->colmap) {
                ierr = MatCreateColmap_MPIAIJ_Private(mat);CHKERRQ(ierr);
              }
#if defined(PETSC_USE_CTABLE)
              ierr = PetscTableFind(aij->colmap,in[j]+1,&col);CHKERRQ(ierr);
              col--;
#else
              col = aij->colmap[in[j]] - 1;
#endif
              if (col < 0 && !((Mat_SeqAIJ*)(aij->A->data))->nonew) {
                ierr = MatDisAssemble_MPIAIJ(mat);CHKERRQ(ierr);
                col  =  in[j];
                /* Reinitialize the variables required by MatSetValues_SeqAIJ_B_Private() */
                B     = aij->B;
                b     = (Mat_SeqAIJ*)B->data;
                bimax = b->imax; bi = b->i; bilen = b->ilen; bj = b->j;
                rp2   = bj + bi[row];
                ap2   = ba + bi[row];
                rmax2 = bimax[row];
                nrow2 = bilen[row];
                low2  = 0;
                high2 = nrow2;
                bm    = aij->B->rmap->n;
                ba    = b->a;
              }
            } else col = in[j];
            MatSetValues_SeqAIJ_B_Private(row,col,value,addv,im[i],in[j]);
          }
        }
      } else if (!aij->donotstash) {
        if (roworiented) {
          ierr = MatStashValuesRow_Private(&mat->stash,im[i],n,in,v+i*n,(PetscBool)(ignorezeroentries && (addv == ADD_VALUES)));CHKERRQ(ierr);
        } else {
          ierr = MatStashValuesCol_Private(&mat->stash,im[i],n,in,v+i,m,(PetscBool)(ignorezeroentries && (addv == ADD_VALUES)));CHKERRQ(ierr);
        }
      }
    }
  }
  PetscFunctionReturnVoid();
}

