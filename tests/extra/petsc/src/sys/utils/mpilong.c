
#include <petscsys.h>         /*I  "petscsys.h"  I*/

/*
    Allows sending/receiving larger messages then 2 gigabytes in a single call
*/

PetscErrorCode MPIULong_Send(void *mess,PetscInt cnt, MPI_Datatype type,PetscMPIInt to, PetscMPIInt tag, MPI_Comm comm)
{
  PetscErrorCode  ierr;
  static PetscInt CHUNKSIZE = 250000000; /* 250,000,000 */
  PetscInt        i,numchunks;
  PetscMPIInt     icnt;

  PetscFunctionBegin;
  numchunks = cnt/CHUNKSIZE + 1;
  for (i=0; i<numchunks; i++) {
    ierr = PetscMPIIntCast((i < numchunks-1) ? CHUNKSIZE : cnt - (numchunks-1)*CHUNKSIZE,&icnt);CHKERRQ(ierr);
    ierr = MPI_Send(mess,icnt,type,to,tag,comm);CHKERRQ(ierr);
    if (type == MPIU_INT)         mess = (void*) (((PetscInt*)mess) + CHUNKSIZE);
    else if (type == MPIU_SCALAR) mess = (void*) (((PetscScalar*)mess) + CHUNKSIZE);
    else SETERRQ(comm,PETSC_ERR_SUP,"No support for this datatype");
  }
  PetscFunctionReturn(0);
}

PetscErrorCode MPIULong_Recv(void *mess,PetscInt cnt, MPI_Datatype type,PetscMPIInt from, PetscMPIInt tag, MPI_Comm comm)
{
  int             ierr;
  static PetscInt CHUNKSIZE = 250000000; /* 250,000,000 */
  MPI_Status      status;
  PetscInt        i,numchunks;
  PetscMPIInt     icnt;

  PetscFunctionBegin;
  numchunks = cnt/CHUNKSIZE + 1;
  for (i=0; i<numchunks; i++) {
    ierr = PetscMPIIntCast((i < numchunks-1) ? CHUNKSIZE : cnt - (numchunks-1)*CHUNKSIZE,&icnt);CHKERRQ(ierr);
    ierr = MPI_Recv(mess,icnt,type,from,tag,comm,&status);CHKERRQ(ierr);
    if (type == MPIU_INT)         mess = (void*) (((PetscInt*)mess) + CHUNKSIZE);
    else if (type == MPIU_SCALAR) mess = (void*) (((PetscScalar*)mess) + CHUNKSIZE);
    else SETERRQ(comm,PETSC_ERR_SUP,"No support for this datatype");
  }
  PetscFunctionReturn(0);
}
