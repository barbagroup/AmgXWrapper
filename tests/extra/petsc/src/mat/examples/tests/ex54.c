
static char help[] = "Tests MatIncreaseOverlap(), MatGetSubMatrices() for parallel MatBAIJ format.\n";

#include <petscmat.h>

#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc,char **args)
{
  Mat            A,B,*submatA,*submatB;
  PetscInt       bs=1,m=11,ov=1,i,j,k,*rows,*cols,nd=5,*idx,rstart,rend,sz,mm,nn,M,N,Mbs;
  PetscErrorCode ierr;
  PetscMPIInt    size,rank;
  PetscScalar    *vals,rval;
  IS             *is1,*is2;
  PetscRandom    rdm;
  Vec            xx,s1,s2;
  PetscReal      s1norm,s2norm,rnorm,tol = PETSC_SQRT_MACHINE_EPSILON;
  PetscBool      flg;

  PetscInitialize(&argc,&args,(char*)0,help);
  ierr = MPI_Comm_size(PETSC_COMM_WORLD,&size);CHKERRQ(ierr);
  ierr = MPI_Comm_rank(PETSC_COMM_WORLD,&rank);CHKERRQ(ierr);

  ierr = PetscOptionsGetInt(NULL,NULL,"-mat_block_size",&bs,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsGetInt(NULL,NULL,"-mat_size",&m,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsGetInt(NULL,NULL,"-ov",&ov,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsGetInt(NULL,NULL,"-nd",&nd,NULL);CHKERRQ(ierr);

  /* ierr = MatCreateBAIJ(PETSC_COMM_WORLD,bs,m*bs,m*bs,PETSC_DECIDE,PETSC_DECIDE,PETSC_DEFAULT,NULL,PETSC_DEFAULT,NULL,&A);CHKERRQ(ierr); */
  ierr = MatCreate(PETSC_COMM_WORLD,&A);CHKERRQ(ierr);
  ierr = MatSetSizes(A,m*bs,m*bs,PETSC_DECIDE,PETSC_DECIDE);CHKERRQ(ierr);
  ierr = MatSetType(A,MATBAIJ);CHKERRQ(ierr);
  ierr = MatSeqBAIJSetPreallocation(A,bs,PETSC_DEFAULT,NULL);CHKERRQ(ierr);
  ierr = MatMPIBAIJSetPreallocation(A,bs,PETSC_DEFAULT,NULL,PETSC_DEFAULT,NULL);CHKERRQ(ierr);
  ierr = MatSetFromOptions(A);CHKERRQ(ierr);
  ierr = MatSetOption(A,MAT_NEW_NONZERO_ALLOCATION_ERR,PETSC_FALSE);CHKERRQ(ierr);

  ierr = MatCreateAIJ(PETSC_COMM_WORLD,m*bs,m*bs,PETSC_DECIDE,PETSC_DECIDE,PETSC_DEFAULT,NULL,PETSC_DEFAULT,NULL,&B);CHKERRQ(ierr);
  ierr = MatSetOption(B,MAT_NEW_NONZERO_ALLOCATION_ERR,PETSC_FALSE);CHKERRQ(ierr);
  ierr = PetscRandomCreate(PETSC_COMM_WORLD,&rdm);CHKERRQ(ierr);
  ierr = PetscRandomSetFromOptions(rdm);CHKERRQ(ierr);

  ierr = MatGetOwnershipRange(A,&rstart,&rend);CHKERRQ(ierr);
  ierr = MatGetSize(A,&M,&N);
  Mbs  = M/bs;

  ierr = PetscMalloc1(bs,&rows);CHKERRQ(ierr);
  ierr = PetscMalloc1(bs,&cols);CHKERRQ(ierr);
  ierr = PetscMalloc1(bs*bs,&vals);CHKERRQ(ierr);
  ierr = PetscMalloc1(M,&idx);CHKERRQ(ierr);

  /* Now set blocks of values */
  for (i=0; i<40*bs; i++) {
    ierr    = PetscRandomGetValue(rdm,&rval);CHKERRQ(ierr);
    cols[0] = bs*(int)(PetscRealPart(rval)*Mbs);
    ierr    = PetscRandomGetValue(rdm,&rval);CHKERRQ(ierr);
    rows[0] = rstart + bs*(int)(PetscRealPart(rval)*m);
    for (j=1; j<bs; j++) {
      rows[j] = rows[j-1]+1;
      cols[j] = cols[j-1]+1;
    }

    for (j=0; j<bs*bs; j++) {
      ierr    = PetscRandomGetValue(rdm,&rval);CHKERRQ(ierr);
      vals[j] = rval;
    }
    ierr = MatSetValues(A,bs,rows,bs,cols,vals,ADD_VALUES);CHKERRQ(ierr);
    ierr = MatSetValues(B,bs,rows,bs,cols,vals,ADD_VALUES);CHKERRQ(ierr);
  }

  ierr = MatAssemblyBegin(A,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(A,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyBegin(B,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(B,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);

  /* Test MatIncreaseOverlap() */
  ierr = PetscMalloc1(nd,&is1);CHKERRQ(ierr);
  ierr = PetscMalloc1(nd,&is2);CHKERRQ(ierr);


  for (i=0; i<nd; i++) {
    ierr = PetscRandomGetValue(rdm,&rval);CHKERRQ(ierr);
    sz   = (int)(PetscRealPart(rval)*m);
    for (j=0; j<sz; j++) {
      ierr      = PetscRandomGetValue(rdm,&rval);CHKERRQ(ierr);
      idx[j*bs] = bs*(int)(PetscRealPart(rval)*Mbs);
      for (k=1; k<bs; k++) idx[j*bs+k] = idx[j*bs]+k;
    }
    ierr = ISCreateGeneral(PETSC_COMM_SELF,sz*bs,idx,PETSC_COPY_VALUES,is1+i);CHKERRQ(ierr);
    ierr = ISCreateGeneral(PETSC_COMM_SELF,sz*bs,idx,PETSC_COPY_VALUES,is2+i);CHKERRQ(ierr);
  }
  ierr = MatIncreaseOverlap(A,nd,is1,ov);CHKERRQ(ierr);
  ierr = MatIncreaseOverlap(B,nd,is2,ov);CHKERRQ(ierr);

  for (i=0; i<nd; ++i) {
    ierr = ISEqual(is1[i],is2[i],&flg);CHKERRQ(ierr);

    if (!flg) {
      ierr = PetscPrintf(PETSC_COMM_SELF,"i=%D, flg=%d :bs=%D m=%D ov=%D nd=%D np=%D\n",i,flg,bs,m,ov,nd,size);CHKERRQ(ierr);
    }
  }

  for (i=0; i<nd; ++i) {
    ierr = ISSort(is1[i]);CHKERRQ(ierr);
    ierr = ISSort(is2[i]);CHKERRQ(ierr);
  }

  ierr = MatGetSubMatrices(B,nd,is2,is2,MAT_INITIAL_MATRIX,&submatB);CHKERRQ(ierr);
  ierr = MatGetSubMatrices(A,nd,is1,is1,MAT_INITIAL_MATRIX,&submatA);CHKERRQ(ierr);


  /* Test MatMult() */
  for (i=0; i<nd; i++) {
    ierr = MatGetSize(submatA[i],&mm,&nn);CHKERRQ(ierr);
    ierr = VecCreateSeq(PETSC_COMM_SELF,mm,&xx);CHKERRQ(ierr);
    ierr = VecDuplicate(xx,&s1);CHKERRQ(ierr);
    ierr = VecDuplicate(xx,&s2);CHKERRQ(ierr);
    for (j=0; j<3; j++) {
      ierr  = VecSetRandom(xx,rdm);CHKERRQ(ierr);
      ierr  = MatMult(submatA[i],xx,s1);CHKERRQ(ierr);
      ierr  = MatMult(submatB[i],xx,s2);CHKERRQ(ierr);
      ierr  = VecNorm(s1,NORM_2,&s1norm);CHKERRQ(ierr);
      ierr  = VecNorm(s2,NORM_2,&s2norm);CHKERRQ(ierr);
      rnorm = s2norm-s1norm;
      if (rnorm<-tol || rnorm>tol) {
        ierr = PetscPrintf(PETSC_COMM_SELF,"[%d]Error:MatMult - Norm1=%16.14e Norm2=%16.14e\n",rank,s1norm,s2norm);CHKERRQ(ierr);
      }
    }
    ierr = VecDestroy(&xx);CHKERRQ(ierr);
    ierr = VecDestroy(&s1);CHKERRQ(ierr);
    ierr = VecDestroy(&s2);CHKERRQ(ierr);
  }

  /* Now test MatGetSubmatrices with MAT_REUSE_MATRIX option */

  ierr = MatGetSubMatrices(A,nd,is1,is1,MAT_REUSE_MATRIX,&submatA);CHKERRQ(ierr);
  ierr = MatGetSubMatrices(B,nd,is2,is2,MAT_REUSE_MATRIX,&submatB);CHKERRQ(ierr);

  /* Test MatMult() */
  for (i=0; i<nd; i++) {
    ierr = MatGetSize(submatA[i],&mm,&nn);CHKERRQ(ierr);
    ierr = VecCreateSeq(PETSC_COMM_SELF,mm,&xx);CHKERRQ(ierr);
    ierr = VecDuplicate(xx,&s1);CHKERRQ(ierr);
    ierr = VecDuplicate(xx,&s2);CHKERRQ(ierr);
    for (j=0; j<3; j++) {
      ierr  = VecSetRandom(xx,rdm);CHKERRQ(ierr);
      ierr  = MatMult(submatA[i],xx,s1);CHKERRQ(ierr);
      ierr  = MatMult(submatB[i],xx,s2);CHKERRQ(ierr);
      ierr  = VecNorm(s1,NORM_2,&s1norm);CHKERRQ(ierr);
      ierr  = VecNorm(s2,NORM_2,&s2norm);CHKERRQ(ierr);
      rnorm = s2norm-s1norm;
      if (rnorm<-tol || rnorm>tol) {
        ierr = PetscPrintf(PETSC_COMM_SELF,"[%d]Error:MatMult - Norm1=%16.14e Norm2=%16.14e\n",rank,s1norm,s2norm);CHKERRQ(ierr);
      }
    }
    ierr = VecDestroy(&xx);CHKERRQ(ierr);
    ierr = VecDestroy(&s1);CHKERRQ(ierr);
    ierr = VecDestroy(&s2);CHKERRQ(ierr);
  }

  /* Free allocated memory */
  for (i=0; i<nd; ++i) {
    ierr = ISDestroy(&is1[i]);CHKERRQ(ierr);
    ierr = ISDestroy(&is2[i]);CHKERRQ(ierr);
    ierr = MatDestroy(&submatA[i]);CHKERRQ(ierr);
    ierr = MatDestroy(&submatB[i]);CHKERRQ(ierr);
  }
  ierr = PetscFree(is1);CHKERRQ(ierr);
  ierr = PetscFree(is2);CHKERRQ(ierr);
  ierr = PetscFree(idx);CHKERRQ(ierr);
  ierr = PetscFree(rows);CHKERRQ(ierr);
  ierr = PetscFree(cols);CHKERRQ(ierr);
  ierr = PetscFree(vals);CHKERRQ(ierr);
  ierr = MatDestroy(&A);CHKERRQ(ierr);
  ierr = MatDestroy(&B);CHKERRQ(ierr);
  ierr = PetscFree(submatA);CHKERRQ(ierr);
  ierr = PetscFree(submatB);CHKERRQ(ierr);
  ierr = PetscRandomDestroy(&rdm);CHKERRQ(ierr);
  ierr = PetscFinalize();
  return 0;
}
