
/*
     Provides utility routines for split MPI communicator.
*/
#include <petscsys.h>    /*I   "petscsys.h"    I*/
#include <petscviewer.h>

const char *const PetscSubcommTypes[] = {"GENERAL","CONTIGUOUS","INTERLACED","PetscSubcommType","PETSC_SUBCOMM_",0};

extern PetscErrorCode PetscSubcommCreate_contiguous(PetscSubcomm);
extern PetscErrorCode PetscSubcommCreate_interlaced(PetscSubcomm);
#undef __FUNCT__
#define __FUNCT__ "PetscSubcommSetFromOptions"
PetscErrorCode PetscSubcommSetFromOptions(PetscSubcomm psubcomm)
{
  PetscErrorCode ierr;
  PetscSubcommType type;
  PetscBool      flg;

  PetscFunctionBegin;
  if (!psubcomm) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_NULL,"Must call PetscSubcommCreate firt");
  type = psubcomm->type;
  ierr = PetscOptionsGetEnum(NULL,NULL,"-psubcomm_type",PetscSubcommTypes,(PetscEnum*)&type,&flg);CHKERRQ(ierr);
  if (flg && psubcomm->type != type) {
    /* free old structures */
    ierr = PetscCommDestroy(&(psubcomm)->dupparent);CHKERRQ(ierr);
    ierr = PetscCommDestroy(&(psubcomm)->child);CHKERRQ(ierr);
    ierr = PetscFree((psubcomm)->subsize);CHKERRQ(ierr);
    switch (type) {
    case PETSC_SUBCOMM_GENERAL:
      SETERRQ(PETSC_COMM_SELF,PETSC_ERR_SUP,"Runtime option PETSC_SUBCOMM_GENERAL is not supported, use PetscSubcommSetTypeGeneral()");
    case PETSC_SUBCOMM_CONTIGUOUS:
      ierr = PetscSubcommCreate_contiguous(psubcomm);CHKERRQ(ierr);
      break;
    case PETSC_SUBCOMM_INTERLACED:
      ierr = PetscSubcommCreate_interlaced(psubcomm);CHKERRQ(ierr);
      break;
    default:
      SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_SUP,"PetscSubcommType %s is not supported yet",PetscSubcommTypes[type]);
    }
  }

  ierr = PetscOptionsHasName(NULL,NULL, "-psubcomm_view", &flg);CHKERRQ(ierr);
  if (flg) {
    ierr = PetscSubcommView(psubcomm,PETSC_VIEWER_STDOUT_WORLD);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscSubcommView"
PetscErrorCode PetscSubcommView(PetscSubcomm psubcomm,PetscViewer viewer)
{
  PetscErrorCode    ierr;
  PetscBool         iascii;
  PetscViewerFormat format;

  PetscFunctionBegin;
  ierr = PetscObjectTypeCompare((PetscObject)viewer,PETSCVIEWERASCII,&iascii);CHKERRQ(ierr);
  if (iascii) {
    ierr = PetscViewerGetFormat(viewer,&format);CHKERRQ(ierr);
    if (format == PETSC_VIEWER_DEFAULT) {
      MPI_Comm    comm=psubcomm->parent;
      PetscMPIInt rank,size,subsize,subrank,duprank;

      ierr = MPI_Comm_size(comm,&size);CHKERRQ(ierr);
      ierr = PetscViewerASCIIPrintf(viewer,"PetscSubcomm type %s with total %d MPI processes:\n",PetscSubcommTypes[psubcomm->type],size);CHKERRQ(ierr);
      ierr = MPI_Comm_rank(comm,&rank);CHKERRQ(ierr);
      ierr = MPI_Comm_size(psubcomm->child,&subsize);CHKERRQ(ierr);
      ierr = MPI_Comm_rank(psubcomm->child,&subrank);CHKERRQ(ierr);
      ierr = MPI_Comm_rank(psubcomm->dupparent,&duprank);CHKERRQ(ierr);
      ierr = PetscSynchronizedPrintf(comm,"  [%d], color %d, sub-size %d, sub-rank %d, duprank %d\n",rank,psubcomm->color,subsize,subrank,duprank);CHKERRQ(ierr);
      ierr = PetscSynchronizedFlush(comm,PETSC_STDOUT);CHKERRQ(ierr);
    }
  } else SETERRQ(PETSC_COMM_SELF,PETSC_ERR_SUP,"Not supported yet");
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscSubcommSetNumber"
/*@C
  PetscSubcommSetNumber - Set total number of subcommunicators.

   Collective on MPI_Comm

   Input Parameter:
+  psubcomm - PetscSubcomm context
-  nsubcomm - the total number of subcommunicators in psubcomm

   Level: advanced

.keywords: communicator

.seealso: PetscSubcommCreate(),PetscSubcommDestroy(),PetscSubcommSetType(),PetscSubcommSetTypeGeneral()
@*/
PetscErrorCode  PetscSubcommSetNumber(PetscSubcomm psubcomm,PetscInt nsubcomm)
{
  PetscErrorCode ierr;
  MPI_Comm       comm=psubcomm->parent;
  PetscMPIInt    msub,size;

  PetscFunctionBegin;
  if (!psubcomm) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_NULL,"PetscSubcomm is not created. Call PetscSubcommCreate() first");
  ierr = MPI_Comm_size(comm,&size);CHKERRQ(ierr);
  ierr = PetscMPIIntCast(nsubcomm,&msub);CHKERRQ(ierr);
  if (msub < 1 || msub > size) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE, "Num of subcommunicators %d cannot be < 1 or > input comm size %d",msub,size);

  psubcomm->n = msub;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscSubcommSetType"
/*@C
  PetscSubcommSetType - Set type of subcommunicators.

   Collective on MPI_Comm

   Input Parameter:
+  psubcomm - PetscSubcomm context
-  subcommtype - subcommunicator type, PETSC_SUBCOMM_CONTIGUOUS,PETSC_SUBCOMM_INTERLACED

   Level: advanced

.keywords: communicator

.seealso: PetscSubcommCreate(),PetscSubcommDestroy(),PetscSubcommSetNumber(),PetscSubcommSetTypeGeneral()
@*/
PetscErrorCode  PetscSubcommSetType(PetscSubcomm psubcomm,PetscSubcommType subcommtype)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (!psubcomm) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_NULL,"PetscSubcomm is not created. Call PetscSubcommCreate()");
  if (psubcomm->n < 1) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONG,"number of subcommunicators %d is incorrect. Call PetscSubcommSetNumber()",psubcomm->n);

  if (subcommtype == PETSC_SUBCOMM_CONTIGUOUS) {
    ierr = PetscSubcommCreate_contiguous(psubcomm);CHKERRQ(ierr);
  } else if (subcommtype == PETSC_SUBCOMM_INTERLACED) {
    ierr = PetscSubcommCreate_interlaced(psubcomm);CHKERRQ(ierr);
  } else SETERRQ1(psubcomm->parent,PETSC_ERR_SUP,"PetscSubcommType %s is not supported yet",PetscSubcommTypes[subcommtype]);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscSubcommSetTypeGeneral"
/*@C
  PetscSubcommSetTypeGeneral - Set a PetscSubcomm from user's specifications

   Collective on MPI_Comm

   Input Parameter:
+  psubcomm - PetscSubcomm context
.  color   - control of subset assignment (nonnegative integer). Processes with the same color are in the same subcommunicator.
-  subrank - rank in the subcommunicator

   Level: advanced

.keywords: communicator, create

.seealso: PetscSubcommCreate(),PetscSubcommDestroy(),PetscSubcommSetNumber(),PetscSubcommSetType()
@*/
PetscErrorCode PetscSubcommSetTypeGeneral(PetscSubcomm psubcomm,PetscMPIInt color,PetscMPIInt subrank)
{
  PetscErrorCode ierr;
  MPI_Comm       subcomm=0,dupcomm=0,comm=psubcomm->parent;
  PetscMPIInt    size,icolor,duprank,*recvbuf,sendbuf[3],mysubsize,rank,*subsize;
  PetscMPIInt    i,nsubcomm=psubcomm->n;

  PetscFunctionBegin;
  if (!psubcomm) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_NULL,"PetscSubcomm is not created. Call PetscSubcommCreate()");
  if (nsubcomm < 1) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONG,"number of subcommunicators %d is incorrect. Call PetscSubcommSetNumber()",nsubcomm);

  ierr = MPI_Comm_split(comm,color,subrank,&subcomm);CHKERRQ(ierr);

  /* create dupcomm with same size as comm, but its rank, duprank, maps subcomm's contiguously into dupcomm */
  /* TODO: this can be done in an ostensibly scalale way (i.e., without allocating an array of size 'size') as is done in PetscObjectsCreateGlobalOrdering(). */
  ierr = MPI_Comm_size(comm,&size);CHKERRQ(ierr);
  ierr = PetscMalloc1(2*size,&recvbuf);CHKERRQ(ierr);

  ierr = MPI_Comm_rank(comm,&rank);CHKERRQ(ierr);
  ierr = MPI_Comm_size(subcomm,&mysubsize);CHKERRQ(ierr);

  sendbuf[0] = color;
  sendbuf[1] = mysubsize;
  ierr = MPI_Allgather(sendbuf,2,MPI_INT,recvbuf,2,MPI_INT,comm);CHKERRQ(ierr);

  ierr = PetscCalloc1(nsubcomm,&subsize);CHKERRQ(ierr);
  for (i=0; i<2*size; i+=2) {
    subsize[recvbuf[i]] = recvbuf[i+1];
  }
  ierr = PetscFree(recvbuf);CHKERRQ(ierr);

  duprank = 0;
  for (icolor=0; icolor<nsubcomm; icolor++) {
    if (icolor != color) { /* not color of this process */
      duprank += subsize[icolor];
    } else {
      duprank += subrank;
      break;
    }
  }
  ierr = MPI_Comm_split(comm,0,duprank,&dupcomm);CHKERRQ(ierr);

  ierr = PetscCommDuplicate(dupcomm,&psubcomm->dupparent,NULL);CHKERRQ(ierr);
  ierr = PetscCommDuplicate(subcomm,&psubcomm->child,NULL);CHKERRQ(ierr);
  ierr = MPI_Comm_free(&dupcomm);CHKERRQ(ierr);
  ierr = MPI_Comm_free(&subcomm);CHKERRQ(ierr);

  psubcomm->color   = color;
  psubcomm->subsize = subsize;
  psubcomm->type    = PETSC_SUBCOMM_GENERAL;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscSubcommDestroy"
PetscErrorCode  PetscSubcommDestroy(PetscSubcomm *psubcomm)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (!*psubcomm) PetscFunctionReturn(0);
  ierr = PetscCommDestroy(&(*psubcomm)->dupparent);CHKERRQ(ierr);
  ierr = PetscCommDestroy(&(*psubcomm)->child);CHKERRQ(ierr);
  ierr = PetscFree((*psubcomm)->subsize);CHKERRQ(ierr);
  ierr = PetscFree((*psubcomm));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscSubcommCreate"
/*@C
  PetscSubcommCreate - Create a PetscSubcomm context.

   Collective on MPI_Comm

   Input Parameter:
.  comm - MPI communicator

   Output Parameter:
.  psubcomm - location to store the PetscSubcomm context

   Level: advanced

.keywords: communicator, create

.seealso: PetscSubcommDestroy()
@*/
PetscErrorCode  PetscSubcommCreate(MPI_Comm comm,PetscSubcomm *psubcomm)
{
  PetscErrorCode ierr;
  PetscMPIInt    rank,size;

  PetscFunctionBegin;
  ierr = PetscNew(psubcomm);CHKERRQ(ierr);

  /* set defaults */
  ierr = MPI_Comm_rank(comm,&rank);CHKERRQ(ierr);
  ierr = MPI_Comm_size(comm,&size);CHKERRQ(ierr);

  (*psubcomm)->parent    = comm;
  (*psubcomm)->dupparent = comm;
  (*psubcomm)->child     = PETSC_COMM_SELF;
  (*psubcomm)->n         = size;
  (*psubcomm)->color     = rank;
  (*psubcomm)->subsize   = NULL;
  (*psubcomm)->type      = PETSC_SUBCOMM_INTERLACED;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscSubcommCreate_contiguous"
PetscErrorCode PetscSubcommCreate_contiguous(PetscSubcomm psubcomm)
{
  PetscErrorCode ierr;
  PetscMPIInt    rank,size,*subsize,duprank=-1,subrank=-1;
  PetscMPIInt    np_subcomm,nleftover,i,color=-1,rankstart,nsubcomm=psubcomm->n;
  MPI_Comm       subcomm=0,dupcomm=0,comm=psubcomm->parent;

  PetscFunctionBegin;
  ierr = MPI_Comm_rank(comm,&rank);CHKERRQ(ierr);
  ierr = MPI_Comm_size(comm,&size);CHKERRQ(ierr);

  /* get size of each subcommunicator */
  ierr = PetscMalloc1(1+nsubcomm,&subsize);CHKERRQ(ierr);

  np_subcomm = size/nsubcomm;
  nleftover  = size - nsubcomm*np_subcomm;
  for (i=0; i<nsubcomm; i++) {
    subsize[i] = np_subcomm;
    if (i<nleftover) subsize[i]++;
  }

  /* get color and subrank of this proc */
  rankstart = 0;
  for (i=0; i<nsubcomm; i++) {
    if (rank >= rankstart && rank < rankstart+subsize[i]) {
      color   = i;
      subrank = rank - rankstart;
      duprank = rank;
      break;
    } else rankstart += subsize[i];
  }

  ierr = MPI_Comm_split(comm,color,subrank,&subcomm);CHKERRQ(ierr);

  /* create dupcomm with same size as comm, but its rank, duprank, maps subcomm's contiguously into dupcomm */
  ierr = MPI_Comm_split(comm,0,duprank,&dupcomm);CHKERRQ(ierr);
  ierr = PetscCommDuplicate(dupcomm,&psubcomm->dupparent,NULL);CHKERRQ(ierr);
  ierr = PetscCommDuplicate(subcomm,&psubcomm->child,NULL);CHKERRQ(ierr);
  ierr = MPI_Comm_free(&dupcomm);CHKERRQ(ierr);
  ierr = MPI_Comm_free(&subcomm);CHKERRQ(ierr);

  psubcomm->color   = color;
  psubcomm->subsize = subsize;
  psubcomm->type    = PETSC_SUBCOMM_CONTIGUOUS;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscSubcommCreate_interlaced"
/*
   Note:
   In PCREDUNDANT, to avoid data scattering from subcomm back to original comm, we create subcommunicators
   by iteratively taking a process into a subcommunicator.
   Example: size=4, nsubcomm=(*psubcomm)->n=3
     comm=(*psubcomm)->parent:
      rank:     [0]  [1]  [2]  [3]
      color:     0    1    2    0

     subcomm=(*psubcomm)->comm:
      subrank:  [0]  [0]  [0]  [1]

     dupcomm=(*psubcomm)->dupparent:
      duprank:  [0]  [2]  [3]  [1]

     Here, subcomm[color = 0] has subsize=2, owns process [0] and [3]
           subcomm[color = 1] has subsize=1, owns process [1]
           subcomm[color = 2] has subsize=1, owns process [2]
           dupcomm has same number of processes as comm, and its duprank maps
           processes in subcomm contiguously into a 1d array:
            duprank: [0] [1]      [2]         [3]
            rank:    [0] [3]      [1]         [2]
                    subcomm[0] subcomm[1]  subcomm[2]
*/

PetscErrorCode PetscSubcommCreate_interlaced(PetscSubcomm psubcomm)
{
  PetscErrorCode ierr;
  PetscMPIInt    rank,size,*subsize,duprank,subrank;
  PetscMPIInt    np_subcomm,nleftover,i,j,color,nsubcomm=psubcomm->n;
  MPI_Comm       subcomm=0,dupcomm=0,comm=psubcomm->parent;

  PetscFunctionBegin;
  ierr = MPI_Comm_rank(comm,&rank);CHKERRQ(ierr);
  ierr = MPI_Comm_size(comm,&size);CHKERRQ(ierr);

  /* get size of each subcommunicator */
  ierr = PetscMalloc1(1+nsubcomm,&subsize);CHKERRQ(ierr);

  np_subcomm = size/nsubcomm;
  nleftover  = size - nsubcomm*np_subcomm;
  for (i=0; i<nsubcomm; i++) {
    subsize[i] = np_subcomm;
    if (i<nleftover) subsize[i]++;
  }

  /* find color for this proc */
  color   = rank%nsubcomm;
  subrank = rank/nsubcomm;

  ierr = MPI_Comm_split(comm,color,subrank,&subcomm);CHKERRQ(ierr);

  j = 0; duprank = 0;
  for (i=0; i<nsubcomm; i++) {
    if (j == color) {
      duprank += subrank;
      break;
    }
    duprank += subsize[i]; j++;
  }

  /* create dupcomm with same size as comm, but its rank, duprank, maps subcomm's contiguously into dupcomm */
  ierr = MPI_Comm_split(comm,0,duprank,&dupcomm);CHKERRQ(ierr);
  ierr = PetscCommDuplicate(dupcomm,&psubcomm->dupparent,NULL);CHKERRQ(ierr);
  ierr = PetscCommDuplicate(subcomm,&psubcomm->child,NULL);CHKERRQ(ierr);
  ierr = MPI_Comm_free(&dupcomm);CHKERRQ(ierr);
  ierr = MPI_Comm_free(&subcomm);CHKERRQ(ierr);

  psubcomm->color   = color;
  psubcomm->subsize = subsize;
  psubcomm->type    = PETSC_SUBCOMM_INTERLACED;
  PetscFunctionReturn(0);
}


