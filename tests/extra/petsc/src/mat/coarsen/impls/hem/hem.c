
#include <petsc/private/matimpl.h>    /*I "petscmat.h" I*/
#include <../src/mat/impls/aij/seq/aij.h>
#include <../src/mat/impls/aij/mpi/mpiaij.h>

/* linked list methods
 *
 *  PetscCDCreate
 */
#undef __FUNCT__
#define __FUNCT__ "PetscCDCreate"
PetscErrorCode PetscCDCreate(PetscInt a_size, PetscCoarsenData **a_out)
{
  PetscErrorCode   ierr;
  PetscCoarsenData *ail;
  PetscInt         ii;

  PetscFunctionBegin;
  /* alocate pool, partially */
  ierr                 = PetscNew(&ail);CHKERRQ(ierr);
  *a_out               = ail;
  ail->pool_list.next  = NULL;
  ail->pool_list.array = NULL;
  ail->chk_sz          = 0;
  /* allocate array */
  ail->size = a_size;
  ierr      = PetscMalloc1(a_size, &ail->array);CHKERRQ(ierr);
  for (ii=0; ii<a_size; ii++) ail->array[ii] = NULL;
  ail->extra_nodes = NULL;
  ail->mat         = NULL;
  /* ail->removedIS = NULL; */
  PetscFunctionReturn(0);
}

/* NPDestroy
 */
#undef __FUNCT__
#define __FUNCT__ "PetscCDDestroy"
PetscErrorCode PetscCDDestroy(PetscCoarsenData *ail)
{
  PetscErrorCode ierr;
  PetscCDArrNd   *n = &ail->pool_list;

  PetscFunctionBegin;
  n = n->next;
  while (n) {
    PetscCDArrNd *lstn = n;
    n    = n->next;
    ierr = PetscFree(lstn);CHKERRQ(ierr);
  }
  if (ail->pool_list.array) {
    ierr = PetscFree(ail->pool_list.array);CHKERRQ(ierr);
  }
  /* if (ail->removedIS) { */
  /*   ierr = ISDestroy(&ail->removedIS);CHKERRQ(ierr); */
  /* } */
  /* delete this (+array) */
  ierr = PetscFree(ail->array);CHKERRQ(ierr);
  /* delete this (+agg+pool array) */
  ierr = PetscFree(ail);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}
/* PetscCDSetChuckSize
 */
#undef __FUNCT__
#define __FUNCT__ "PetscCDSetChuckSize"
PetscErrorCode PetscCDSetChuckSize(PetscCoarsenData *ail, PetscInt a_sz)
{
  PetscFunctionBegin;
  ail->chk_sz = a_sz;
  PetscFunctionReturn(0);
}
/*  PetscCDGetNewNode
 */
#undef __FUNCT__
#define __FUNCT__ "PetscCDGetNewNode"
PetscErrorCode PetscCDGetNewNode(PetscCoarsenData *ail, PetscCDIntNd **a_out, PetscInt a_id)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (ail->extra_nodes) {
    PetscCDIntNd *node = ail->extra_nodes;
    ail->extra_nodes = node->next;
    node->gid        = a_id;
    node->next       = NULL;
    *a_out           = node;
  } else {
    if (!ail->pool_list.array) {
      if (!ail->chk_sz) ail->chk_sz = 10; /* use a chuck size of ail->size? */
      ierr                = PetscMalloc1(ail->chk_sz, &ail->pool_list.array);CHKERRQ(ierr);
      ail->new_node       = ail->pool_list.array;
      ail->new_left       = ail->chk_sz;
      ail->new_node->next = NULL;
    } else if (!ail->new_left) {
      PetscCDArrNd *node;
      ierr                = PetscMalloc(ail->chk_sz*sizeof(PetscCDIntNd) + sizeof(PetscCDArrNd), &node);CHKERRQ(ierr);
      node->array         = (PetscCDIntNd*)(node + 1);
      node->next          = ail->pool_list.next;
      ail->pool_list.next = node;
      ail->new_left       = ail->chk_sz;
      ail->new_node       = node->array;
    }
    ail->new_node->gid  = a_id;
    ail->new_node->next = NULL;
    *a_out              = ail->new_node++; ail->new_left--;
  }
  PetscFunctionReturn(0);
}

/* PetscLLNSetID
 */
#undef __FUNCT__
#define __FUNCT__ "PetscLLNSetID"
PetscErrorCode PetscLLNSetID(PetscCDIntNd *a_this, PetscInt a_id)
{
  PetscFunctionBegin;
  a_this->gid = a_id;
  PetscFunctionReturn(0);
}
/* PetscLLNGetID
 */
#undef __FUNCT__
#define __FUNCT__ "PetscLLNGetID"
PetscErrorCode PetscLLNGetID(const PetscCDIntNd *a_this, PetscInt *a_gid)
{
  PetscFunctionBegin;
  *a_gid = a_this->gid;
  PetscFunctionReturn(0);
}
/* PetscCDGetHeadPos
 */
#undef __FUNCT__
#define __FUNCT__ "PetscCDGetHeadPos"
PetscErrorCode PetscCDGetHeadPos(const PetscCoarsenData *ail, PetscInt a_idx, PetscCDPos *pos)
{
  PetscFunctionBegin;
  if (a_idx>=ail->size) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_USER,"a_idx >= ail->size: a_idx=%d.",a_idx);
  *pos = ail->array[a_idx];
  PetscFunctionReturn(0);
}
/* PetscCDGetNextPos
 */
#undef __FUNCT__
#define __FUNCT__ "PetscCDGetNextPos"
PetscErrorCode PetscCDGetNextPos(const PetscCoarsenData *ail, PetscInt l_idx, PetscCDPos *pos)
{
  PetscFunctionBegin;
  if (!(*pos)) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_USER,"NULL input position.");
  *pos = (*pos)->next;
  PetscFunctionReturn(0);
}

/* PetscCDAppendID
 */
#undef __FUNCT__
#define __FUNCT__ "PetscCDAppendID"
PetscErrorCode PetscCDAppendID(PetscCoarsenData *ail, PetscInt a_idx, PetscInt a_id)
{
  PetscErrorCode ierr;
  PetscCDIntNd   *n,*n2;

  PetscFunctionBegin;
  ierr = PetscCDGetNewNode(ail, &n, a_id);CHKERRQ(ierr);
  if (a_idx>=ail->size) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_USER,"Index %d out of range.",a_idx);
  if (!(n2=ail->array[a_idx])) ail->array[a_idx] = n;
  else {
    do {
      if (!n2->next) {
        n2->next = n;
        if (n->next) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_PLIB,"n should not have a next");
        break;
      }
      n2 = n2->next;
    } while (n2);
    if (!n2) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_PLIB,"n2 should be non-null");
  }
  PetscFunctionReturn(0);
}
/* PetscCDAppendNode
 */
#undef __FUNCT__
#define __FUNCT__ "PetscCDAppendNode"
PetscErrorCode PetscCDAppendNode(PetscCoarsenData *ail, PetscInt a_idx,  PetscCDIntNd *a_n)
{
  PetscCDIntNd *n2;

  PetscFunctionBegin;
  if (a_idx>=ail->size) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_USER,"Index %d out of range.",a_idx);
  if (!(n2=ail->array[a_idx])) ail->array[a_idx] = a_n;
  else {
    do {
      if (!n2->next) {
        n2->next  = a_n;
        a_n->next = NULL;
        break;
      }
      n2 = n2->next;
    } while (n2);
    if (!n2) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_PLIB,"n2 should be non-null");
  }
  PetscFunctionReturn(0);
}

/* PetscCDRemoveNextNode: a_last->next, this exposes single linked list structure to API
 */
#undef __FUNCT__
#define __FUNCT__ "PetscCDRemoveNextNode"
PetscErrorCode PetscCDRemoveNextNode(PetscCoarsenData *ail, PetscInt a_idx,  PetscCDIntNd *a_last)
{
  PetscCDIntNd *del;

  PetscFunctionBegin;
  if (a_idx>=ail->size) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_USER,"Index %d out of range.",a_idx);
  if (!a_last->next) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_PLIB,"a_last should have a next");
  del          = a_last->next;
  a_last->next = del->next;
  /* del->next = NULL; -- this still used in a iterator so keep it intact -- need to fix this with a double linked list */
  /* could reuse n2 but PetscCDAppendNode sometimes uses it */
  PetscFunctionReturn(0);
}

/* PetscCDPrint
 */
#undef __FUNCT__
#define __FUNCT__ "PetscCDPrint"
PetscErrorCode PetscCDPrint(const PetscCoarsenData *ail, MPI_Comm comm)
{
  PetscErrorCode ierr;
  PetscCDIntNd   *n;
  PetscInt       ii,kk;
  PetscMPIInt    rank;

  PetscFunctionBegin;
  ierr = MPI_Comm_rank(comm, &rank);CHKERRQ(ierr);
  for (ii=0; ii<ail->size; ii++) {
    kk = 0;
    n  = ail->array[ii];
    if (n) PetscPrintf(comm,"[%d]%s list %d:\n",rank,__FUNCT__,ii);
    while (n) {
      PetscPrintf(comm,"\t[%d] %d) id %d\n",rank,++kk,n->gid);
      n = n->next;
    }
  }
  PetscFunctionReturn(0);
}
/* PetscCDAppendRemove
 */
#undef __FUNCT__
#define __FUNCT__ "PetscCDAppendRemove"
PetscErrorCode PetscCDAppendRemove(PetscCoarsenData *ail, PetscInt a_destidx, PetscInt a_srcidx)
{
  PetscCDIntNd *n;

  PetscFunctionBegin;
  if (a_srcidx>=ail->size) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_USER,"Index %d out of range.",a_srcidx);
  if (a_destidx>=ail->size) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_USER,"Index %d out of range.",a_destidx);
  if (a_destidx==a_srcidx) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_USER,"a_destidx==a_srcidx %d.",a_destidx);
  n = ail->array[a_destidx];
  if (!n) ail->array[a_destidx] = ail->array[a_srcidx];
  else {
    do {
      if (!n->next) {
        n->next = ail->array[a_srcidx];
        break;
      }
      n = n->next;
    } while (1);
  }
  ail->array[a_srcidx] = NULL;
  PetscFunctionReturn(0);
}

/* PetscCDRemoveAll
 */
#undef __FUNCT__
#define __FUNCT__ "PetscCDRemoveAll"
PetscErrorCode PetscCDRemoveAll(PetscCoarsenData *ail, PetscInt a_idx)
{
  PetscCDIntNd *rem,*n1;

  PetscFunctionBegin;
  if (a_idx>=ail->size) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_USER,"Index %d out of range.",a_idx);
  rem               = ail->array[a_idx];
  ail->array[a_idx] = NULL;
  if (!(n1=ail->extra_nodes)) ail->extra_nodes = rem;
  else {
    while (n1->next) n1 = n1->next;
    n1->next = rem;
  }
  PetscFunctionReturn(0);
}

/* PetscCDSizeAt
 */
#undef __FUNCT__
#define __FUNCT__ "PetscCDSizeAt"
PetscErrorCode PetscCDSizeAt(const PetscCoarsenData *ail, PetscInt a_idx, PetscInt *a_sz)
{
  PetscCDIntNd *n1;
  PetscInt     sz = 0;

  PetscFunctionBegin;
  if (a_idx>=ail->size) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_USER,"Index %d out of range.",a_idx);
  n1 = ail->array[a_idx];
  while (n1) {
    n1 = n1->next;
    sz++;
  }
  *a_sz = sz;
  PetscFunctionReturn(0);
}

/* PetscCDEmptyAt
 */
#undef __FUNCT__
#define __FUNCT__ "PetscCDEmptyAt"
PetscErrorCode PetscCDEmptyAt(const PetscCoarsenData *ail, PetscInt a_idx, PetscBool *a_e)
{
  PetscFunctionBegin;
  if (a_idx>=ail->size) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_USER,"Index %d out of range.",a_idx);
  *a_e = (PetscBool)(ail->array[a_idx]==NULL);
  PetscFunctionReturn(0);
}

/* PetscCDGetMIS
 */
#undef __FUNCT__
#define __FUNCT__ "PetscCDGetMIS"
PetscErrorCode PetscCDGetMIS(PetscCoarsenData *ail, IS *a_mis)
{
  PetscErrorCode ierr;
  PetscCDIntNd   *n;
  PetscInt       ii,kk;
  PetscInt       *permute;

  PetscFunctionBegin;
  for (ii=kk=0; ii<ail->size; ii++) {
    n = ail->array[ii];
    if (n) kk++;
  }
  ierr = PetscMalloc1(kk, &permute);CHKERRQ(ierr);
  for (ii=kk=0; ii<ail->size; ii++) {
    n = ail->array[ii];
    if (n) permute[kk++] = ii;
  }
  ierr = ISCreateGeneral(PETSC_COMM_SELF, kk, permute, PETSC_OWN_POINTER, a_mis);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}
/* PetscCDGetMat
 */
#undef __FUNCT__
#define __FUNCT__ "PetscCDGetMat"
PetscErrorCode PetscCDGetMat(const PetscCoarsenData *ail, Mat *a_mat)
{
  PetscFunctionBegin;
  *a_mat = ail->mat;
  PetscFunctionReturn(0);
}

/* PetscCDSetMat
 */
#undef __FUNCT__
#define __FUNCT__ "PetscCDSetMat"
PetscErrorCode PetscCDSetMat(PetscCoarsenData *ail, Mat a_mat)
{
  PetscFunctionBegin;
  ail->mat = a_mat;
  PetscFunctionReturn(0);
}


/* PetscCDGetASMBlocks
 */
#undef __FUNCT__
#define __FUNCT__ "PetscCDGetASMBlocks"
PetscErrorCode PetscCDGetASMBlocks(const PetscCoarsenData *ail, const PetscInt a_bs, PetscInt *a_sz, IS **a_local_is)
{
  PetscErrorCode ierr;
  PetscCDIntNd   *n;
  PetscInt       lsz,ii,kk,*idxs,jj;
  IS             *is_loc;

  PetscFunctionBegin;
  for (ii=kk=0; ii<ail->size; ii++) {
    if (ail->array[ii]) kk++;
  }
  *a_sz = kk; /* out */

  ierr        = PetscMalloc1(kk, &is_loc);CHKERRQ(ierr);
  *a_local_is = is_loc; /* out */

  for (ii=kk=0; ii<ail->size; ii++) {
    for (lsz=0, n=ail->array[ii]; n; lsz++, n=n->next) /* void */;
    if (lsz) {
      ierr = PetscMalloc1(a_bs*lsz, &idxs);CHKERRQ(ierr);
      for (lsz = 0, n=ail->array[ii]; n; n = n->next) {
        PetscInt gid;
        ierr = PetscLLNGetID(n, &gid);CHKERRQ(ierr);
        for (jj=0; jj<a_bs; lsz++,jj++) idxs[lsz] = a_bs*gid + jj;
      }
      ierr = ISCreateGeneral(PETSC_COMM_SELF, lsz, idxs, PETSC_OWN_POINTER, &is_loc[kk++]);CHKERRQ(ierr);
    }
  }
  if (*a_sz != kk) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_PLIB,"*a_sz %D != kk %D",*a_sz,kk);
  PetscFunctionReturn(0);
}


/* PetscCDSetRemovedIS
 */
/* PetscErrorCode PetscCDSetRemovedIS(PetscCoarsenData *ail, MPI_Comm comm, const PetscInt a_sz, PetscInt a_ids[]) */
/* { */
/*   PetscErrorCode ierr; */

/*   if (ail->removedIS) { */
/*     ierr = ISDestroy(&ail->removedIS);CHKERRQ(ierr); */
/*   } */
/*   ierr = ISCreateGeneral(comm, a_sz, a_ids, PETSC_COPY_VALUES, &ail->removedIS);CHKERRQ(ierr); */
/*   PetscFunctionReturn(0); */
/* } */

/* PetscCDGetRemovedIS
 */
/* PetscErrorCode PetscCDGetRemovedIS(PetscCoarsenData *ail, IS *a_is) */
/* { */
/*   *a_is = ail->removedIS; */
/*   ail->removedIS = NULL; /\* hack to relinquish ownership *\/ */
/*   PetscFunctionReturn(0); */
/* } */

/* ********************************************************************** */
/* edge for priority queue */
typedef struct edge_tag {
  PetscReal weight;
  PetscInt  lid0,gid1,cpid1;
} Edge;

static int gamg_hem_compare(const void *a, const void *b)
{
  PetscReal va = ((Edge*)a)->weight, vb = ((Edge*)b)->weight;
  return (va < vb) ? 1 : (va == vb) ? 0 : -1; /* 0 for equal */
}

/* -------------------------------------------------------------------------- */
/*
   heavyEdgeMatchAgg - parallel heavy edge matching (HEM). MatAIJ specific!!!

   Input Parameter:
   . perm - permutation
   . a_Gmat - glabal matrix of graph (data not defined)

   Output Parameter:
   . a_locals_llist - array of list of local nodes rooted at local node
*/
#undef __FUNCT__
#define __FUNCT__ "heavyEdgeMatchAgg"
static PetscErrorCode heavyEdgeMatchAgg(IS perm,Mat a_Gmat,PetscCoarsenData **a_locals_llist)
{
  PetscErrorCode   ierr;
  PetscBool        isMPI;
  MPI_Comm         comm;
  PetscInt         sub_it,kk,n,ix,*idx,*ii,iter,Iend,my0;
  PetscMPIInt      rank,size;
  const PetscInt   nloc = a_Gmat->rmap->n,n_iter=6; /* need to figure out how to stop this */
  PetscInt         *lid_cprowID,*lid_gid;
  PetscBool        *lid_matched;
  Mat_SeqAIJ       *matA, *matB=0;
  Mat_MPIAIJ       *mpimat     =0;
  PetscScalar      one         =1.;
  PetscCoarsenData *agg_llists = NULL,*deleted_list = NULL;
  Mat              cMat,tMat,P;
  MatScalar        *ap;
  PetscMPIInt      tag1,tag2;

  PetscFunctionBegin;
  ierr = PetscObjectGetComm((PetscObject)a_Gmat,&comm);CHKERRQ(ierr);
  ierr = MPI_Comm_rank(comm, &rank);CHKERRQ(ierr);
  ierr = MPI_Comm_size(comm, &size);CHKERRQ(ierr);
  ierr = MatGetOwnershipRange(a_Gmat, &my0, &Iend);CHKERRQ(ierr);
  ierr = PetscCommGetNewTag(comm, &tag1);CHKERRQ(ierr);
  ierr = PetscCommGetNewTag(comm, &tag2);CHKERRQ(ierr);

  ierr = PetscMalloc1(nloc, &lid_gid);CHKERRQ(ierr); /* explicit array needed */
  ierr = PetscMalloc1(nloc, &lid_cprowID);CHKERRQ(ierr);
  ierr = PetscMalloc1(nloc, &lid_matched);CHKERRQ(ierr);

  ierr = PetscCDCreate(nloc, &agg_llists);CHKERRQ(ierr);
  /* ierr = PetscCDSetChuckSize(agg_llists, nloc+1);CHKERRQ(ierr); */
  *a_locals_llist = agg_llists;
  ierr            = PetscCDCreate(size, &deleted_list);CHKERRQ(ierr);
  ierr            = PetscCDSetChuckSize(deleted_list, 100);CHKERRQ(ierr);
  /* setup 'lid_gid' for scatters and add self to all lists */
  for (kk=0; kk<nloc; kk++) {
    lid_gid[kk] = kk + my0;
    ierr        = PetscCDAppendID(agg_llists, kk, my0+kk);CHKERRQ(ierr);
  }

  /* make a copy of the graph, this gets destroyed in iterates */
  ierr = MatDuplicate(a_Gmat,MAT_COPY_VALUES,&cMat);CHKERRQ(ierr);
  ierr = PetscObjectTypeCompare((PetscObject)a_Gmat, MATMPIAIJ, &isMPI);CHKERRQ(ierr);
  iter = 0;
  while (iter++ < n_iter) {
    PetscScalar    *cpcol_gid,*cpcol_max_ew,*cpcol_max_pe,*lid_max_ew;
    PetscBool      *cpcol_matched;
    PetscMPIInt    *cpcol_pe,proc;
    Vec            locMaxEdge,locMaxPE,ghostMaxEdge,ghostMaxPE;
    PetscInt       nEdges,n_nz_row,jj;
    Edge           *Edges;
    PetscInt       gid;
    const PetscInt *perm_ix, n_sub_its = 120;

    /* get submatrices of cMat */
    if (isMPI) {
      mpimat = (Mat_MPIAIJ*)cMat->data;
      matA   = (Mat_SeqAIJ*)mpimat->A->data;
      matB   = (Mat_SeqAIJ*)mpimat->B->data;
      if (!matB->compressedrow.use) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_PLIB,"matB must have compressed row usage");
    } else {
      matA = (Mat_SeqAIJ*)cMat->data;
    }

    /* set max edge on nodes */
    ierr = MatCreateVecs(cMat, &locMaxEdge, 0);CHKERRQ(ierr);
    ierr = MatCreateVecs(cMat, &locMaxPE, 0);CHKERRQ(ierr);

    /* get 'cpcol_pe' & 'cpcol_gid' & init. 'cpcol_matched' using 'mpimat->lvec' */
    if (mpimat) {
      Vec         vec;
      PetscScalar vval;

      ierr = MatCreateVecs(cMat, &vec, 0);CHKERRQ(ierr);
      /* cpcol_pe */
      vval = (PetscScalar)(rank);
      for (kk=0,gid=my0; kk<nloc; kk++,gid++) {
        ierr = VecSetValues(vec, 1, &gid, &vval, INSERT_VALUES);CHKERRQ(ierr); /* set with GID */
      }
      ierr = VecAssemblyBegin(vec);CHKERRQ(ierr);
      ierr = VecAssemblyEnd(vec);CHKERRQ(ierr);
      ierr = VecScatterBegin(mpimat->Mvctx,vec,mpimat->lvec,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
      ierr = VecScatterEnd(mpimat->Mvctx,vec,mpimat->lvec,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
      ierr = VecGetArray(mpimat->lvec, &cpcol_gid);CHKERRQ(ierr); /* get proc ID in 'cpcol_gid' */
      ierr = VecGetLocalSize(mpimat->lvec, &n);CHKERRQ(ierr);
      ierr = PetscMalloc1(n, &cpcol_pe);CHKERRQ(ierr);
      for (kk=0; kk<n; kk++) cpcol_pe[kk] = (PetscMPIInt)PetscRealPart(cpcol_gid[kk]);
      ierr = VecRestoreArray(mpimat->lvec, &cpcol_gid);CHKERRQ(ierr);

      /* cpcol_gid */
      for (kk=0,gid=my0; kk<nloc; kk++,gid++) {
        vval = (PetscScalar)(gid);
        ierr = VecSetValues(vec, 1, &gid, &vval, INSERT_VALUES);CHKERRQ(ierr); /* set with GID */
      }
      ierr = VecAssemblyBegin(vec);CHKERRQ(ierr);
      ierr = VecAssemblyEnd(vec);CHKERRQ(ierr);
      ierr = VecScatterBegin(mpimat->Mvctx,vec,mpimat->lvec,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
      ierr = VecScatterEnd(mpimat->Mvctx,vec,mpimat->lvec,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
      ierr = VecDestroy(&vec);CHKERRQ(ierr);
      ierr = VecGetArray(mpimat->lvec, &cpcol_gid);CHKERRQ(ierr); /* get proc ID in 'cpcol_gid' */

      /* cpcol_matched */
      ierr = VecGetLocalSize(mpimat->lvec, &n);CHKERRQ(ierr);
      ierr = PetscMalloc1(n, &cpcol_matched);CHKERRQ(ierr);
      for (kk=0; kk<n; kk++) cpcol_matched[kk] = PETSC_FALSE;
    }

    /* need an inverse map - locals */
    for (kk=0; kk<nloc; kk++) lid_cprowID[kk] = -1;
    /* set index into compressed row 'lid_cprowID' */
    if (matB) {
      ii = matB->compressedrow.i;
      for (ix=0; ix<matB->compressedrow.nrows; ix++) {
        lid_cprowID[matB->compressedrow.rindex[ix]] = ix;
      }
    }

    /* get removed IS, use '' */
    /* if (iter==1) { */
    /*   PetscInt *lid_rem,idx; */
    /*   ierr = PetscMalloc1(nloc, &lid_rem);CHKERRQ(ierr); */
    /*   for (kk=idx=0;kk<nloc;kk++) { */
    /*     PetscInt nn,lid=kk; */
    /*     ii = matA->i; nn = ii[lid+1] - ii[lid]; */
    /*     if ((ix=lid_cprowID[lid]) != -1) { /\* if I have any ghost neighbors *\/ */
    /*       ii = matB->compressedrow.i; */
    /*       nn += ii[ix+1] - ii[ix]; */
    /*     } */
    /*     if (nn < 2) { */
    /*       lid_rem[idx++] = kk + my0; */
    /*     } */
    /*   } */
    /*   ierr = PetscCDSetRemovedIS(agg_llists, comm, idx, lid_rem);CHKERRQ(ierr); */
    /*   ierr = PetscFree(lid_rem);CHKERRQ(ierr); */
    /* } */

    /* compute 'locMaxEdge' & 'locMaxPE', and create list of edges, count edges' */
    for (nEdges=0,kk=0,gid=my0; kk<nloc; kk++,gid++) {
      PetscReal   max_e = 0., tt;
      PetscScalar vval;
      PetscInt    lid   = kk;
      PetscMPIInt max_pe=rank,pe;
      ii = matA->i; n = ii[lid+1] - ii[lid]; idx = matA->j + ii[lid];
      ap = matA->a + ii[lid];
      for (jj=0; jj<n; jj++) {
        PetscInt lidj = idx[jj];
        if (lidj != lid && PetscRealPart(ap[jj]) > max_e) max_e = PetscRealPart(ap[jj]);
        if (lidj > lid) nEdges++;
      }
      if ((ix=lid_cprowID[lid]) != -1) { /* if I have any ghost neighbors */
        ii  = matB->compressedrow.i; n = ii[ix+1] - ii[ix];
        ap  = matB->a + ii[ix];
        idx = matB->j + ii[ix];
        for (jj=0; jj<n; jj++) {
          if ((tt=PetscRealPart(ap[jj])) > max_e) max_e = tt;
          nEdges++;
          if ((pe=cpcol_pe[idx[jj]]) > max_pe) max_pe = pe;
        }
      }
      vval = max_e;
      ierr = VecSetValues(locMaxEdge, 1, &gid, &vval, INSERT_VALUES);CHKERRQ(ierr);

      vval = (PetscScalar)max_pe;
      ierr = VecSetValues(locMaxPE, 1, &gid, &vval, INSERT_VALUES);CHKERRQ(ierr);
    }
    ierr = VecAssemblyBegin(locMaxEdge);CHKERRQ(ierr);
    ierr = VecAssemblyEnd(locMaxEdge);CHKERRQ(ierr);
    ierr = VecAssemblyBegin(locMaxPE);CHKERRQ(ierr);
    ierr = VecAssemblyEnd(locMaxPE);CHKERRQ(ierr);

    /* get 'cpcol_max_ew' & 'cpcol_max_pe' */
    if (mpimat) {
      ierr = VecDuplicate(mpimat->lvec, &ghostMaxEdge);CHKERRQ(ierr);
      ierr = VecScatterBegin(mpimat->Mvctx,locMaxEdge,ghostMaxEdge,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
      ierr = VecScatterEnd(mpimat->Mvctx,locMaxEdge,ghostMaxEdge,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
      ierr = VecGetArray(ghostMaxEdge, &cpcol_max_ew);CHKERRQ(ierr);

      ierr = VecDuplicate(mpimat->lvec, &ghostMaxPE);CHKERRQ(ierr);
      ierr = VecScatterBegin(mpimat->Mvctx,locMaxPE,ghostMaxPE,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
      ierr =   VecScatterEnd(mpimat->Mvctx,locMaxPE,ghostMaxPE,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
      ierr = VecGetArray(ghostMaxPE, &cpcol_max_pe);CHKERRQ(ierr);
    }

    /* setup sorted list of edges */
    ierr = PetscMalloc1(nEdges, &Edges);CHKERRQ(ierr);
    ierr = ISGetIndices(perm, &perm_ix);CHKERRQ(ierr);
    for (nEdges=n_nz_row=kk=0; kk<nloc; kk++) {
      PetscInt nn, lid = perm_ix[kk];
      ii = matA->i; nn = n = ii[lid+1] - ii[lid]; idx = matA->j + ii[lid];
      ap = matA->a + ii[lid];
      for (jj=0; jj<n; jj++) {
        PetscInt lidj = idx[jj];
        if (lidj > lid) {
          Edges[nEdges].lid0   = lid;
          Edges[nEdges].gid1   = lidj + my0;
          Edges[nEdges].cpid1  = -1;
          Edges[nEdges].weight = PetscRealPart(ap[jj]);
          nEdges++;
        }
      }
      if ((ix=lid_cprowID[lid]) != -1) { /* if I have any ghost neighbors */
        ii  = matB->compressedrow.i; n = ii[ix+1] - ii[ix];
        ap  = matB->a + ii[ix];
        idx = matB->j + ii[ix];
        nn += n;
        for (jj=0; jj<n; jj++) {
          Edges[nEdges].lid0   = lid;
          Edges[nEdges].gid1   = (PetscInt)PetscRealPart(cpcol_gid[idx[jj]]);
          Edges[nEdges].cpid1  = idx[jj];
          Edges[nEdges].weight = PetscRealPart(ap[jj]);
          nEdges++;
        }
      }
      if (nn > 1) n_nz_row++;
      else if (iter == 1) {
        /* should select this because it is technically in the MIS but lets not */
        ierr = PetscCDRemoveAll(agg_llists, lid);CHKERRQ(ierr);
      }
    }
    ierr = ISRestoreIndices(perm,&perm_ix);CHKERRQ(ierr);

    qsort(Edges, nEdges, sizeof(Edge), gamg_hem_compare);

    /* projection matrix */
    ierr = MatCreateAIJ(comm, nloc, nloc, PETSC_DETERMINE, PETSC_DETERMINE, 1, 0, 1, 0, &P);CHKERRQ(ierr);

    /* clear matched flags */
    for (kk=0; kk<nloc; kk++) lid_matched[kk] = PETSC_FALSE;
    /* process - communicate - process */
    for (sub_it=0; sub_it<n_sub_its; sub_it++) {
      PetscInt nactive_edges;

      ierr = VecGetArray(locMaxEdge, &lid_max_ew);CHKERRQ(ierr);
      for (kk=nactive_edges=0; kk<nEdges; kk++) {
        /* HEM */
        const Edge     *e   = &Edges[kk];
        const PetscInt lid0 =e->lid0,gid1=e->gid1,cpid1=e->cpid1,gid0=lid0+my0,lid1=gid1-my0;
        PetscBool      isOK = PETSC_TRUE;

        /* skip if either (local) vertex is done already */
        if (lid_matched[lid0] || (gid1>=my0 && gid1<Iend && lid_matched[gid1-my0])) continue;

        /* skip if ghost vertex is done */
        if (cpid1 != -1 && cpcol_matched[cpid1]) continue;

        nactive_edges++;
        /* skip if I have a bigger edge someplace (lid_max_ew gets updated) */
        if (PetscRealPart(lid_max_ew[lid0]) > e->weight + 1.e-12) continue;

        if (cpid1 == -1) {
          if (PetscRealPart(lid_max_ew[lid1]) > e->weight + 1.e-12) continue;
        } else {
          /* see if edge might get matched on other proc */
          PetscReal g_max_e = PetscRealPart(cpcol_max_ew[cpid1]);
          if (g_max_e > e->weight + 1.e-12) continue;
          else if (e->weight > g_max_e - 1.e-12 && (PetscMPIInt)PetscRealPart(cpcol_max_pe[cpid1]) > rank) {
            /* check for max_e == to this edge and larger processor that will deal with this */
            continue;
          }
        }

        /* check ghost for v0 */
        if (isOK) {
          PetscReal max_e,ew;
          if ((ix=lid_cprowID[lid0]) != -1) { /* if I have any ghost neighbors */
            ii  = matB->compressedrow.i; n = ii[ix+1] - ii[ix];
            ap  = matB->a + ii[ix];
            idx = matB->j + ii[ix];
            for (jj=0; jj<n && isOK; jj++) {
              PetscInt lidj = idx[jj];
              if (cpcol_matched[lidj]) continue;
              ew = PetscRealPart(ap[jj]); max_e = PetscRealPart(cpcol_max_ew[lidj]);
              /* check for max_e == to this edge and larger processor that will deal with this */
              if (ew > max_e - 1.e-12 && ew > PetscRealPart(lid_max_ew[lid0]) - 1.e-12 && (PetscMPIInt)PetscRealPart(cpcol_max_pe[lidj]) > rank) {
                isOK = PETSC_FALSE;
              }
            }
          }

          /* for v1 */
          if (cpid1 == -1 && isOK) {
            if ((ix=lid_cprowID[lid1]) != -1) { /* if I have any ghost neighbors */
              ii  = matB->compressedrow.i; n = ii[ix+1] - ii[ix];
              ap  = matB->a + ii[ix];
              idx = matB->j + ii[ix];
              for (jj=0; jj<n && isOK; jj++) {
                PetscInt lidj = idx[jj];
                if (cpcol_matched[lidj]) continue;
                ew = PetscRealPart(ap[jj]); max_e = PetscRealPart(cpcol_max_ew[lidj]);
                /* check for max_e == to this edge and larger processor that will deal with this */
                if (ew > max_e - 1.e-12 && ew > PetscRealPart(lid_max_ew[lid1]) - 1.e-12 && (PetscMPIInt)PetscRealPart(cpcol_max_pe[lidj]) > rank) {
                  isOK = PETSC_FALSE;
                }
              }
            }
          }
        }

        /* do it */
        if (isOK) {
          if (cpid1 == -1) {
            lid_matched[lid1] = PETSC_TRUE;  /* keep track of what we've done this round */
            ierr              = PetscCDAppendRemove(agg_llists, lid0, lid1);CHKERRQ(ierr);
          } else if (sub_it != n_sub_its-1) {
            /* add gid1 to list of ghost deleted by me -- I need their children */
            proc = cpcol_pe[cpid1];

            cpcol_matched[cpid1] = PETSC_TRUE; /* messing with VecGetArray array -- needed??? */

            ierr = PetscCDAppendID(deleted_list, proc, cpid1);CHKERRQ(ierr); /* cache to send messages */
            ierr = PetscCDAppendID(deleted_list, proc, lid0);CHKERRQ(ierr);
          } else continue;

          lid_matched[lid0] = PETSC_TRUE; /* keep track of what we've done this round */
          /* set projection */
          ierr = MatSetValues(P,1,&gid0,1,&gid0,&one,INSERT_VALUES);CHKERRQ(ierr);
          ierr = MatSetValues(P,1,&gid1,1,&gid0,&one,INSERT_VALUES);CHKERRQ(ierr);
        } /* matched */
      } /* edge loop */

      /* deal with deleted ghost on first pass */
      if (size>1 && sub_it != n_sub_its-1) {
        PetscCDPos pos;  PetscBool ise = PETSC_FALSE;
        PetscInt   nSend1, **sbuffs1,nSend2;
#define REQ_BF_SIZE 100
        MPI_Request *sreqs2[REQ_BF_SIZE],*rreqs2[REQ_BF_SIZE];
        MPI_Status status;

        /* send request */
        for (proc=0,nSend1=0; proc<size; proc++) {
          ierr = PetscCDEmptyAt(deleted_list,proc,&ise);CHKERRQ(ierr);
          if (!ise) nSend1++;
        }
        ierr = PetscMalloc1(nSend1, &sbuffs1);CHKERRQ(ierr);
        /* ierr = PetscMalloc4(nSend1, sbuffs1, nSend1, rbuffs1, nSend1, sreqs1, nSend1, rreqs1);CHKERRQ(ierr); */
        /* PetscFree4(sbuffs1,rbuffs1,sreqs1,rreqs1); */
        for (proc=0,nSend1=0; proc<size; proc++) {
          /* count ghosts */
          ierr = PetscCDSizeAt(deleted_list,proc,&n);CHKERRQ(ierr);
          if (n>0) {
#define CHUNCK_SIZE 100
            PetscInt    *sbuff,*pt;
            MPI_Request *request;
            n   /= 2;
            ierr = PetscMalloc1(2 + 2*n + n*CHUNCK_SIZE + 2, &sbuff);CHKERRQ(ierr);
            /* PetscMalloc4(2+2*n,sbuffs1[nSend1],n*CHUNCK_SIZE,rbuffs1[nSend1],1,rreqs2[nSend1],1,sreqs2[nSend1]); */
            /* save requests */
            sbuffs1[nSend1] = sbuff;
            request         = (MPI_Request*)sbuff;
            sbuff           = pt = (PetscInt*)(request+1);
            *pt++           = n; *pt++ = rank;

            ierr = PetscCDGetHeadPos(deleted_list,proc,&pos);CHKERRQ(ierr);
            while (pos) {
              PetscInt lid0, cpid, gid;
              ierr  = PetscLLNGetID(pos, &cpid);CHKERRQ(ierr);
              gid   = (PetscInt)PetscRealPart(cpcol_gid[cpid]);
              ierr  = PetscCDGetNextPos(deleted_list,proc,&pos);CHKERRQ(ierr);
              ierr  = PetscLLNGetID(pos, &lid0);CHKERRQ(ierr);
              ierr  = PetscCDGetNextPos(deleted_list,proc,&pos);CHKERRQ(ierr);
              *pt++ = gid; *pt++ = lid0;
            }
            /* send request tag1 [n, proc, n*[gid1,lid0] ] */
            ierr = MPI_Isend(sbuff, 2*n+2, MPIU_INT, proc, tag1, comm, request);CHKERRQ(ierr);
            /* post recieve */
            request        = (MPI_Request*)pt;
            rreqs2[nSend1] = request; /* cache recv request */
            pt             = (PetscInt*)(request+1);
            ierr           = MPI_Irecv(pt, n*CHUNCK_SIZE, MPIU_INT, proc, tag2, comm, request);CHKERRQ(ierr);
            /* clear list */
            ierr = PetscCDRemoveAll(deleted_list, proc);CHKERRQ(ierr);
            nSend1++;
          }
        }
        /* recieve requests, send response, clear lists */
        kk     = nactive_edges;
        ierr   = MPIU_Allreduce(&kk,&nactive_edges,1,MPIU_INT,MPI_SUM,comm);CHKERRQ(ierr); /* not correct syncronization and global */
        nSend2 = 0;
        while (1) {
#define BF_SZ 10000
          PetscMPIInt flag,count;
          PetscInt rbuff[BF_SZ],*pt,*pt2,*pt3,count2,*sbuff,count3;
          MPI_Request *request;
          ierr = MPI_Iprobe(MPI_ANY_SOURCE, tag1, comm, &flag, &status);CHKERRQ(ierr);
          if (!flag) break;
          ierr = MPI_Get_count(&status, MPIU_INT, &count);CHKERRQ(ierr);
          if (count > BF_SZ) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_SUP,"buffer too small for recieve: %d",count);
          proc = status.MPI_SOURCE;
          /* recieve request tag1 [n, proc, n*[gid1,lid0] ] */
          ierr = MPI_Recv(rbuff, count, MPIU_INT, proc, tag1, comm, &status);CHKERRQ(ierr);
          /* count sends */
          pt = rbuff; count3 = count2 = 0;
          n  = *pt++; kk = *pt++;
          while (n--) {
            PetscInt gid1=*pt++, lid1=gid1-my0; kk=*pt++;
            if (lid_matched[lid1]) {
              PetscPrintf(PETSC_COMM_SELF,"\t *** [%d]%s %d) ERROR recieved deleted gid %d, deleted by (lid) %d from proc %d\n",rank,__FUNCT__,sub_it,gid1,kk);
              PetscSleep(1);
            }
            lid_matched[lid1] = PETSC_TRUE; /* keep track of what we've done this round */
            ierr              = PetscCDSizeAt(agg_llists, lid1, &kk);CHKERRQ(ierr);
            count2           += kk + 2;
            count3++; /* number of verts requested (n) */
          }
          if (count2 > count3*CHUNCK_SIZE) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_SUP,"Irecv will be too small: %d",count2);
          /* send tag2 *[lid0, n, n*[gid] ] */
          ierr             = PetscMalloc(count2*sizeof(PetscInt) + sizeof(MPI_Request), &sbuff);CHKERRQ(ierr);
          request          = (MPI_Request*)sbuff;
          sreqs2[nSend2++] = request; /* cache request */
          if (nSend2==REQ_BF_SIZE) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_SUP,"buffer too small for requests: %d",nSend2);
          pt2 = sbuff = (PetscInt*)(request+1);
          pt  = rbuff;
          n   = *pt++; kk = *pt++;
          while (n--) {
            /* read [n, proc, n*[gid1,lid0] */
            PetscInt gid1=*pt++, lid1=gid1-my0, lid0=*pt++;
            /* write [lid0, n, n*[gid] ] */
            *pt2++ = lid0;
            pt3    = pt2++; /* save pointer for later */
            /* for (pos=PetscCDGetHeadPos(agg_llists,lid1) ; pos ; pos=PetscCDGetNextPos(agg_llists,lid1,pos)) { */
            ierr = PetscCDGetHeadPos(agg_llists,lid1,&pos);CHKERRQ(ierr);
            while (pos) {
              PetscInt gid;
              ierr   = PetscLLNGetID(pos, &gid);CHKERRQ(ierr);
              ierr   = PetscCDGetNextPos(agg_llists,lid1,&pos);CHKERRQ(ierr);
              *pt2++ = gid;
            }
            *pt3 = (pt2-pt3)-1;
            /* clear list */
            ierr = PetscCDRemoveAll(agg_llists, lid1);CHKERRQ(ierr);
          }
          /* send requested data tag2 *[lid0, n, n*[gid1] ] */
          ierr = MPI_Isend(sbuff, count2, MPIU_INT, proc, tag2, comm, request);CHKERRQ(ierr);
        }

        /* recieve tag2 *[lid0, n, n*[gid] ] */
        for (kk=0; kk<nSend1; kk++) {
          PetscMPIInt count;
          MPI_Request *request;
          PetscInt    *pt, *pt2;
          request = rreqs2[kk]; /* no need to free -- buffer is in 'sbuffs1' */
          ierr    = MPI_Wait(request, &status);CHKERRQ(ierr);
          ierr    = MPI_Get_count(&status, MPIU_INT, &count);CHKERRQ(ierr);
          pt      = pt2 = (PetscInt*)(request+1);
          while (pt-pt2 < count) {
            PetscInt lid0 = *pt++, n = *pt++;
            while (n--) {
              PetscInt gid1 = *pt++;
              ierr = PetscCDAppendID(agg_llists, lid0, gid1);CHKERRQ(ierr);
            }
          }
        }

        /* wait for tag1 isends */
        while (nSend1--) {
          MPI_Request *request;
          request = (MPI_Request*)sbuffs1[nSend1];
          ierr    = MPI_Wait(request, &status);CHKERRQ(ierr);
          ierr    = PetscFree(request);CHKERRQ(ierr);
        }
        ierr = PetscFree(sbuffs1);CHKERRQ(ierr);

        /* wait for tag2 isends */
        while (nSend2--) {
          MPI_Request *request = sreqs2[nSend2];
          ierr = MPI_Wait(request, &status);CHKERRQ(ierr);
          ierr = PetscFree(request);CHKERRQ(ierr);
        }

        ierr = VecRestoreArray(ghostMaxEdge, &cpcol_max_ew);CHKERRQ(ierr);
        ierr = VecRestoreArray(ghostMaxPE, &cpcol_max_pe);CHKERRQ(ierr);

        /* get 'cpcol_matched' - use locMaxPE, ghostMaxEdge, cpcol_max_ew */
        for (kk=0,gid=my0; kk<nloc; kk++,gid++) {
          PetscScalar vval = lid_matched[kk] ? 1.0 : 0.0;
          ierr = VecSetValues(locMaxPE, 1, &gid, &vval, INSERT_VALUES);CHKERRQ(ierr); /* set with GID */
        }
        ierr = VecAssemblyBegin(locMaxPE);CHKERRQ(ierr);
        ierr = VecAssemblyEnd(locMaxPE);CHKERRQ(ierr);
        ierr = VecScatterBegin(mpimat->Mvctx,locMaxPE,ghostMaxEdge,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
        ierr =   VecScatterEnd(mpimat->Mvctx,locMaxPE,ghostMaxEdge,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
        ierr = VecGetArray(ghostMaxEdge, &cpcol_max_ew);CHKERRQ(ierr);
        ierr = VecGetLocalSize(mpimat->lvec, &n);CHKERRQ(ierr);
        for (kk=0; kk<n; kk++) {
          cpcol_matched[kk] = (PetscBool)(PetscRealPart(cpcol_max_ew[kk]) != 0.0);
        }

        ierr = VecRestoreArray(ghostMaxEdge, &cpcol_max_ew);CHKERRQ(ierr);
      } /* size > 1 */

      /* compute 'locMaxEdge' */
      ierr = VecRestoreArray(locMaxEdge, &lid_max_ew);CHKERRQ(ierr);
      for (kk=0,gid=my0; kk<nloc; kk++,gid++) {
        PetscReal max_e = 0.,tt;
        PetscScalar vval;
        PetscInt lid = kk;
        if (lid_matched[lid]) vval = 0.;
        else {
          ii = matA->i; n = ii[lid+1] - ii[lid]; idx = matA->j + ii[lid];
          ap = matA->a + ii[lid];
          for (jj=0; jj<n; jj++) {
            PetscInt lidj = idx[jj];
            if (lid_matched[lidj]) continue; /* this is new - can change local max */
            if (lidj != lid && PetscRealPart(ap[jj]) > max_e) max_e = PetscRealPart(ap[jj]);
          }
          if (lid_cprowID && (ix=lid_cprowID[lid]) != -1) { /* if I have any ghost neighbors */
            ii  = matB->compressedrow.i; n = ii[ix+1] - ii[ix];
            ap  = matB->a + ii[ix];
            idx = matB->j + ii[ix];
            for (jj=0; jj<n; jj++) {
              PetscInt lidj = idx[jj];
              if (cpcol_matched[lidj]) continue;
              if ((tt=PetscRealPart(ap[jj])) > max_e) max_e = tt;
            }
          }
        }
        vval = (PetscScalar)max_e;
        ierr = VecSetValues(locMaxEdge, 1, &gid, &vval, INSERT_VALUES);CHKERRQ(ierr); /* set with GID */
      }
      ierr = VecAssemblyBegin(locMaxEdge);CHKERRQ(ierr);
      ierr = VecAssemblyEnd(locMaxEdge);CHKERRQ(ierr);

      if (size>1 && sub_it != n_sub_its-1) {
        /* compute 'cpcol_max_ew' */
        ierr = VecScatterBegin(mpimat->Mvctx,locMaxEdge,ghostMaxEdge,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
        ierr =   VecScatterEnd(mpimat->Mvctx,locMaxEdge,ghostMaxEdge,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
        ierr = VecGetArray(ghostMaxEdge, &cpcol_max_ew);CHKERRQ(ierr);
        ierr = VecGetArray(locMaxEdge, &lid_max_ew);CHKERRQ(ierr);

        /* compute 'cpcol_max_pe' */
        for (kk=0,gid=my0; kk<nloc; kk++,gid++) {
          PetscInt lid = kk;
          PetscReal ew,v1_max_e,v0_max_e=PetscRealPart(lid_max_ew[lid]);
          PetscScalar vval;
          PetscMPIInt max_pe=rank,pe;
          if (lid_matched[lid]) vval = (PetscScalar)rank;
          else if ((ix=lid_cprowID[lid]) != -1) { /* if I have any ghost neighbors */
            ii  = matB->compressedrow.i; n = ii[ix+1] - ii[ix];
            ap  = matB->a + ii[ix];
            idx = matB->j + ii[ix];
            for (jj=0; jj<n; jj++) {
              PetscInt lidj = idx[jj];
              if (cpcol_matched[lidj]) continue;
              ew = PetscRealPart(ap[jj]); v1_max_e = PetscRealPart(cpcol_max_ew[lidj]);
              /* get max pe that has a max_e == to this edge w */
              if ((pe=cpcol_pe[idx[jj]]) > max_pe && ew > v1_max_e - 1.e-12 && ew > v0_max_e - 1.e-12) max_pe = pe;
            }
            vval = (PetscScalar)max_pe;
          }
          ierr = VecSetValues(locMaxPE, 1, &gid, &vval, INSERT_VALUES);CHKERRQ(ierr);
        }
        ierr = VecAssemblyBegin(locMaxPE);CHKERRQ(ierr);
        ierr = VecAssemblyEnd(locMaxPE);CHKERRQ(ierr);

        ierr = VecScatterBegin(mpimat->Mvctx,locMaxPE,ghostMaxPE,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
        ierr =   VecScatterEnd(mpimat->Mvctx,locMaxPE,ghostMaxPE,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
        ierr = VecGetArray(ghostMaxPE, &cpcol_max_pe);CHKERRQ(ierr);
        ierr = VecRestoreArray(locMaxEdge, &lid_max_ew);CHKERRQ(ierr);
      } /* deal with deleted ghost */
      ierr = PetscInfo3(a_Gmat,"\t %D.%D: %D active edges.\n",iter,sub_it,nactive_edges);CHKERRQ(ierr);
      if (!nactive_edges) break;
    } /* sub_it loop */

    /* clean up iteration */
    ierr = PetscFree(Edges);CHKERRQ(ierr);
    if (mpimat) {
      ierr = VecRestoreArray(ghostMaxEdge, &cpcol_max_ew);CHKERRQ(ierr);
      ierr = VecDestroy(&ghostMaxEdge);CHKERRQ(ierr);
      ierr = VecRestoreArray(ghostMaxPE, &cpcol_max_pe);CHKERRQ(ierr);
      ierr = VecDestroy(&ghostMaxPE);CHKERRQ(ierr);
      ierr = PetscFree(cpcol_pe);CHKERRQ(ierr);
      ierr = PetscFree(cpcol_matched);CHKERRQ(ierr);
    }

    ierr = VecDestroy(&locMaxEdge);CHKERRQ(ierr);
    ierr = VecDestroy(&locMaxPE);CHKERRQ(ierr);

    if (mpimat) {
      ierr = VecRestoreArray(mpimat->lvec, &cpcol_gid);CHKERRQ(ierr);
    }

    /* create next G if needed */
    if (iter == n_iter) { /* hard wired test - need to look at full surrounded nodes or something */
      ierr = MatDestroy(&P);CHKERRQ(ierr);
      ierr = MatDestroy(&cMat);CHKERRQ(ierr);
      break;
    } else {
      Vec diag;
      /* add identity for unmatched vertices so they stay alive */
      for (kk=0,gid=my0; kk<nloc; kk++,gid++) {
        if (!lid_matched[kk]) {
          gid  = kk+my0;
          ierr = MatGetRow(cMat,gid,&n,0,0);CHKERRQ(ierr);
          if (n>1) {
            ierr = MatSetValues(P,1,&gid,1,&gid,&one,INSERT_VALUES);CHKERRQ(ierr);
          }
          ierr = MatRestoreRow(cMat,gid,&n,0,0);CHKERRQ(ierr);
        }
      }
      ierr = MatAssemblyBegin(P,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
      ierr = MatAssemblyEnd(P,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);

      /* project to make new graph with colapsed edges */
      ierr = MatPtAP(cMat,P,MAT_INITIAL_MATRIX,1.0,&tMat);CHKERRQ(ierr);
      ierr = MatDestroy(&P);CHKERRQ(ierr);
      ierr = MatDestroy(&cMat);CHKERRQ(ierr);
      cMat = tMat;
      ierr = MatCreateVecs(cMat, &diag, 0);CHKERRQ(ierr);
      ierr = MatGetDiagonal(cMat, diag);CHKERRQ(ierr); /* effectively PCJACOBI */
      ierr = VecReciprocal(diag);CHKERRQ(ierr);
      ierr = VecSqrtAbs(diag);CHKERRQ(ierr);
      ierr = MatDiagonalScale(cMat, diag, diag);CHKERRQ(ierr);
      ierr = VecDestroy(&diag);CHKERRQ(ierr);
    }
  } /* coarsen iterator */

  /* make fake matrix */
  if (size>1) {
    Mat mat;
    PetscCDPos pos;
    PetscInt gid, NN, MM, jj = 0, mxsz = 0;

    for (kk=0; kk<nloc; kk++) {
      ierr = PetscCDSizeAt(agg_llists, kk, &jj);CHKERRQ(ierr);
      if (jj > mxsz) mxsz = jj;
    }
    ierr = MatGetSize(a_Gmat, &MM, &NN);CHKERRQ(ierr);
    if (mxsz > MM-nloc) mxsz = MM-nloc;

    ierr = MatCreateAIJ(comm, nloc, nloc,PETSC_DETERMINE, PETSC_DETERMINE,0, 0, mxsz, 0, &mat);CHKERRQ(ierr);

    /* */
    for (kk=0,gid=my0; kk<nloc; kk++,gid++) {
      /* for (pos=PetscCDGetHeadPos(agg_llists,kk) ; pos ; pos=PetscCDGetNextPos(agg_llists,kk,pos)) { */
      ierr = PetscCDGetHeadPos(agg_llists,kk,&pos);CHKERRQ(ierr);
      while (pos) {
        PetscInt gid1;
        ierr = PetscLLNGetID(pos, &gid1);CHKERRQ(ierr);
        ierr = PetscCDGetNextPos(agg_llists,kk,&pos);CHKERRQ(ierr);

        if (gid1 < my0 || gid1 >= my0+nloc) {
          ierr = MatSetValues(mat,1,&gid,1,&gid1,&one,ADD_VALUES);CHKERRQ(ierr);
        }
      }
    }
    ierr = MatAssemblyBegin(mat,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
    ierr = MatAssemblyEnd(mat,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);

    ierr = PetscCDSetMat(agg_llists, mat);CHKERRQ(ierr);
  }

  ierr = PetscFree(lid_cprowID);CHKERRQ(ierr);
  ierr = PetscFree(lid_gid);CHKERRQ(ierr);
  ierr = PetscFree(lid_matched);CHKERRQ(ierr);
  ierr = PetscCDDestroy(deleted_list);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

typedef struct {
  int dummy;
} MatCoarsen_HEM;
/*
   HEM coarsen, simple greedy.
*/
#undef __FUNCT__
#define __FUNCT__ "MatCoarsenApply_HEM"
static PetscErrorCode MatCoarsenApply_HEM(MatCoarsen coarse)
{
  /* MatCoarsen_HEM *HEM = (MatCoarsen_HEM*)coarse->subctx; */
  PetscErrorCode ierr;
  Mat            mat = coarse->graph;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(coarse,MAT_COARSEN_CLASSID,1);
  if (!coarse->perm) {
    IS       perm;
    PetscInt n,m;
    
    ierr = MatGetLocalSize(mat, &m, &n);CHKERRQ(ierr);
    ierr = ISCreateStride(PetscObjectComm((PetscObject)mat), m, 0, 1, &perm);CHKERRQ(ierr);
    ierr = heavyEdgeMatchAgg(perm, mat, &coarse->agg_lists);CHKERRQ(ierr);
    ierr = ISDestroy(&perm);CHKERRQ(ierr);
  } else {
    ierr = heavyEdgeMatchAgg(coarse->perm, mat, &coarse->agg_lists);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatCoarsenView_HEM"
static PetscErrorCode MatCoarsenView_HEM(MatCoarsen coarse,PetscViewer viewer)
{
  /* MatCoarsen_HEM *HEM = (MatCoarsen_HEM*)coarse->subctx; */
  PetscErrorCode ierr;
  PetscMPIInt    rank;
  PetscBool      iascii;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(coarse,MAT_COARSEN_CLASSID,1);
  ierr = MPI_Comm_rank(PetscObjectComm((PetscObject)coarse),&rank);CHKERRQ(ierr);
  ierr = PetscObjectTypeCompare((PetscObject)viewer,PETSCVIEWERASCII,&iascii);CHKERRQ(ierr);
  if (iascii) {
    ierr = PetscViewerASCIIPushSynchronized(viewer);CHKERRQ(ierr);
    ierr = PetscViewerASCIISynchronizedPrintf(viewer,"  [%d] HEM aggregator\n",rank);CHKERRQ(ierr);
    ierr = PetscViewerFlush(viewer);CHKERRQ(ierr);
    ierr = PetscViewerASCIIPopSynchronized(viewer);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatCoarsenDestroy_HEM"
static PetscErrorCode MatCoarsenDestroy_HEM(MatCoarsen coarse)
{
  MatCoarsen_HEM *HEM = (MatCoarsen_HEM*)coarse->subctx;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(coarse,MAT_COARSEN_CLASSID,1);
  ierr = PetscFree(HEM);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/*MC
   MATCOARSENHEM - A coarsener that uses HEM a simple greedy coarsener

   Level: beginner

.keywords: Coarsen, create, context

.seealso: MatCoarsenSetType(), MatCoarsenType, MatCoarsenCreate()

M*/

#undef __FUNCT__
#define __FUNCT__ "MatCoarsenCreate_HEM"
PETSC_EXTERN PetscErrorCode MatCoarsenCreate_HEM(MatCoarsen coarse)
{
  PetscErrorCode ierr;
  MatCoarsen_HEM *HEM;

  PetscFunctionBegin;
  ierr                 = PetscNewLog(coarse,&HEM);CHKERRQ(ierr);
  coarse->subctx       = (void*)HEM;
  coarse->ops->apply   = MatCoarsenApply_HEM;
  coarse->ops->view    = MatCoarsenView_HEM;
  coarse->ops->destroy = MatCoarsenDestroy_HEM;
  /* coarse->ops->setfromoptions = MatCoarsenSetFromOptions_HEM; */
  PetscFunctionReturn(0);
}
