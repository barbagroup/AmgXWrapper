
#include <petscvec.h>    /*I "petscvec.h" I*/
#include <petsc/private/petscimpl.h>

#include <engine.h>   /* MATLAB include file */
#include <mex.h>      /* MATLAB include file */

#undef __FUNCT__
#define __FUNCT__ "VecMatlabEnginePut_Default"
PETSC_EXTERN PetscErrorCode  VecMatlabEnginePut_Default(PetscObject obj,void *mengine)
{
  PetscErrorCode ierr;
  PetscInt       n;
  Vec            vec = (Vec)obj;
  PetscScalar    *array;
  mxArray        *mat;

  PetscFunctionBegin;
  ierr = VecGetArray(vec,&array);CHKERRQ(ierr);
  ierr = VecGetLocalSize(vec,&n);CHKERRQ(ierr);
#if !defined(PETSC_USE_COMPLEX)
  mat  = mxCreateDoubleMatrix(n,1,mxREAL);
#else
  mat  = mxCreateDoubleMatrix(n,1,mxCOMPLEX);
#endif
  ierr = PetscMemcpy(mxGetPr(mat),array,n*sizeof(PetscScalar));CHKERRQ(ierr);
  ierr = PetscObjectName(obj);CHKERRQ(ierr);
  engPutVariable((Engine*)mengine,obj->name,mat);

  ierr = VecRestoreArray(vec,&array);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecMatlabEngineGet_Default"
PETSC_EXTERN PetscErrorCode  VecMatlabEngineGet_Default(PetscObject obj,void *mengine)
{
  PetscErrorCode ierr;
  PetscInt       n;
  Vec            vec = (Vec)obj;
  PetscScalar    *array;
  mxArray        *mat;

  PetscFunctionBegin;
  ierr = VecGetArray(vec,&array);CHKERRQ(ierr);
  ierr = VecGetLocalSize(vec,&n);CHKERRQ(ierr);
  mat  = engGetVariable((Engine*)mengine,obj->name);
  if (!mat) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"Unable to get object %s from matlab",obj->name);
  ierr = PetscMemcpy(array,mxGetPr(mat),n*sizeof(PetscScalar));CHKERRQ(ierr);
  ierr = VecRestoreArray(vec,&array);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}



