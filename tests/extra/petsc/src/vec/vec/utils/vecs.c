
#include <petscvec.h>

#undef __FUNCT__
#define __FUNCT__ "VecsDestroy"
PetscErrorCode VecsDestroy(Vecs x)
{
  PetscErrorCode ierr;
  PetscFunctionBegin;
  ierr = VecDestroy(&(x)->v);CHKERRQ(ierr);
  ierr = PetscFree(x);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecsCreateSeq"
PetscErrorCode VecsCreateSeq(MPI_Comm comm,PetscInt p,PetscInt m,Vecs *x)
{
  PetscErrorCode ierr;
  PetscFunctionBegin;
  ierr = PetscNew(x);CHKERRQ(ierr);
  ierr = VecCreateSeq(comm,p*m,&(*x)->v);CHKERRQ(ierr);
  (*x)->n = m;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecsCreateSeqWithArray"
PetscErrorCode VecsCreateSeqWithArray(MPI_Comm comm,PetscInt p,PetscInt m,PetscScalar *a,Vecs *x)
{
  PetscErrorCode ierr;
  PetscFunctionBegin;
  ierr = PetscNew(x);CHKERRQ(ierr);
  ierr = VecCreateSeqWithArray(comm,1,p*m,a,&(*x)->v);CHKERRQ(ierr);
  (*x)->n = m;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecsDuplicate"
PetscErrorCode VecsDuplicate(Vecs x,Vecs *y)
{
  PetscErrorCode ierr;
  PetscFunctionBegin;
  ierr = PetscNew(y);CHKERRQ(ierr);
  ierr = VecDuplicate(x->v,&(*y)->v);CHKERRQ(ierr);
  (*y)->n = x->n;
  PetscFunctionReturn(0);
}


