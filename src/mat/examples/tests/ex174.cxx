
static char help[] = "Tests MatConvert(), MatLoad(), MatElementalHermitianGenDefEig() for MATELEMENTAL interface.\n\n";
/*
 Example:
   mpiexec -n <np> ./ex173 -fA <A_data> -fB <B_data> -vl <vl> -vu <vu> -orig_mat_type <type> -orig_mat_type <mat_type>
*/
 
#include <petscmat.h>
#include <petscmatelemental.h>

#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc,char **args)
{
  Mat            A,Ae,B,Be,X,Xe,We,C1,C2,EVAL;
  Vec            eval;
  PetscErrorCode ierr;
  PetscViewer    view;
  char           file[2][PETSC_MAX_PATH_LEN];
  PetscBool      flg,flgB,isElemental,isDense,isAij,isSbaij;
  PetscScalar    one = 1.0,*Earray;
  PetscMPIInt    rank,size;
  PetscReal      vl,vu,norm;
  PetscInt       M,N,m;

  /* Below are Elemental data types, see <elemental.hpp> */
  El::Pencil       eigtype = El::AXBX;
  El::UpperOrLower uplo    = El::UPPER;
  El::SortType     sort    = El::UNSORTED; /* UNSORTED, DESCENDING, ASCENDING */
  El::HermitianEigSubset<PetscElemScalar>       subset;
  El::HermitianEigCtrl<PetscElemScalar>   ctrl;

  PetscInitialize(&argc,&args,(char*)0,help);
#if !defined(PETSC_HAVE_ELEMENTAL)
  SETERRQ(PETSC_COMM_WORLD,1,"This example requires ELEMENTAL");
#endif
  ierr = MPI_Comm_rank(PETSC_COMM_WORLD,&rank);CHKERRQ(ierr);
  ierr = MPI_Comm_size(PETSC_COMM_WORLD,&size);CHKERRQ(ierr);

  /* Load PETSc matrices */
  ierr = PetscOptionsGetString(NULL,NULL,"-fA",file[0],PETSC_MAX_PATH_LEN,NULL);CHKERRQ(ierr);
  ierr = PetscViewerBinaryOpen(PETSC_COMM_WORLD,file[0],FILE_MODE_READ,&view);CHKERRQ(ierr);
  ierr = MatCreate(PETSC_COMM_WORLD,&A);CHKERRQ(ierr);
  ierr = MatSetOptionsPrefix(A,"orig_");CHKERRQ(ierr);
  ierr = MatSetType(A,MATAIJ);CHKERRQ(ierr); 
  ierr = MatSetFromOptions(A);CHKERRQ(ierr);
  ierr = MatLoad(A,view);CHKERRQ(ierr);
  ierr = PetscViewerDestroy(&view);CHKERRQ(ierr);

  PetscOptionsGetString(NULL,NULL,"-fB",file[1],PETSC_MAX_PATH_LEN,&flgB);
  if (flgB) {
    ierr = PetscViewerBinaryOpen(PETSC_COMM_WORLD,file[1],FILE_MODE_READ,&view);CHKERRQ(ierr);
    ierr = MatCreate(PETSC_COMM_WORLD,&B);CHKERRQ(ierr);
    ierr = MatSetOptionsPrefix(B,"orig_");CHKERRQ(ierr);
    ierr = MatSetType(B,MATAIJ);CHKERRQ(ierr);
    ierr = MatSetFromOptions(B);CHKERRQ(ierr);
    ierr = MatLoad(B,view);CHKERRQ(ierr);
    ierr = PetscViewerDestroy(&view);CHKERRQ(ierr);
  } else {
    /* Create matrix B = I */
    PetscInt rstart,rend,i;
    ierr = MatGetSize(A,&M,&N);CHKERRQ(ierr);
    ierr = MatGetOwnershipRange(A,&rstart,&rend);CHKERRQ(ierr);

    ierr = MatCreate(PETSC_COMM_WORLD,&B);CHKERRQ(ierr);
    ierr = MatSetOptionsPrefix(B,"orig_");CHKERRQ(ierr);
    ierr = MatSetSizes(B,PETSC_DECIDE,PETSC_DECIDE,M,N);CHKERRQ(ierr);
    ierr = MatSetType(B,MATAIJ);CHKERRQ(ierr);
    ierr = MatSetFromOptions(B);CHKERRQ(ierr);
    ierr = MatSetUp(B);CHKERRQ(ierr);
    for (i=rstart; i<rend; i++) {
      ierr = MatSetValues(B,1,&i,1,&i,&one,ADD_VALUES);CHKERRQ(ierr);
    }
    ierr = MatAssemblyBegin(B,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
    ierr = MatAssemblyEnd(B,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  }

  ierr = PetscObjectTypeCompare((PetscObject)A,MATELEMENTAL,&isElemental);CHKERRQ(ierr);
  if (isElemental) {
    Ae = A;
    Be = B;
    isDense = isAij = isSbaij = PETSC_FALSE;
  } else { /* Convert AIJ/DENSE/SBAIJ matrices into Elemental matrices */
    if (size == 1) {
      ierr = PetscObjectTypeCompare((PetscObject)A,MATSEQDENSE,&isDense);CHKERRQ(ierr);
      ierr = PetscObjectTypeCompare((PetscObject)A,MATSEQAIJ,&isAij);CHKERRQ(ierr);
      ierr = PetscObjectTypeCompare((PetscObject)A,MATSEQSBAIJ,&isSbaij);CHKERRQ(ierr);
    } else {
      ierr = PetscObjectTypeCompare((PetscObject)A,MATMPIDENSE,&isDense);CHKERRQ(ierr);
      ierr = PetscObjectTypeCompare((PetscObject)A,MATMPIAIJ,&isAij);CHKERRQ(ierr);
       ierr = PetscObjectTypeCompare((PetscObject)A,MATMPISBAIJ,&isSbaij);CHKERRQ(ierr);
    }

    if (!rank) {
      if (isDense) {
        printf(" Convert DENSE matrices A and B into Elemental matrix... \n");
      } else if (isAij) {
        printf(" Convert AIJ matrices A and B into Elemental matrix... \n");
      } else if (isSbaij) {
        printf(" Convert SBAIJ matrices A and B into Elemental matrix... \n");
      } else SETERRQ(PetscObjectComm((PetscObject)A),PETSC_ERR_SUP,"Not supported yet");
    }
    ierr = MatConvert(A, MATELEMENTAL, MAT_INITIAL_MATRIX, &Ae);CHKERRQ(ierr);
    ierr = MatConvert(B, MATELEMENTAL, MAT_INITIAL_MATRIX, &Be);CHKERRQ(ierr);

    /* Test accuracy */
    ierr = MatMultEqual(A,Ae,5,&flg);CHKERRQ(ierr);
    if (!flg) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_NOTSAMETYPE,"A != A_elemental.");
    ierr = MatMultEqual(B,Be,5,&flg);CHKERRQ(ierr);
    if (!flg) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_NOTSAMETYPE,"B != B_elemental.");
  }

  /* Test MatElementalHermitianGenDefEig() */
  if (!rank) printf(" Compute Ax = lambda Bx... \n");
  vl = -0.8, vu = -0.7;
  ierr = PetscOptionsGetReal(NULL,NULL,"-vl",&vl,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsGetReal(NULL,NULL,"-vu",&vu,NULL);CHKERRQ(ierr);
  subset.rangeSubset = PETSC_TRUE; 
  subset.lowerBound  = vl;
  subset.upperBound  = vu;
  ierr = MatElementalHermitianGenDefEig(eigtype,uplo,Ae,Be,&We,&Xe,sort,subset,ctrl);CHKERRQ(ierr);
  //ierr = MatView(We,PETSC_VIEWER_STDOUT_WORLD);CHKERRQ(ierr);

  /* Check || A*X - B*X*We || */
  if (isAij) {
    if (!rank) printf(" Convert Elemental matrices We and Xe into MATDENSE matrices... \n");
    ierr = MatConvert(We,MATDENSE,MAT_INITIAL_MATRIX,&EVAL);CHKERRQ(ierr); /* EVAL is a Mx1 matrix */
    ierr = MatConvert(Xe,MATDENSE,MAT_INITIAL_MATRIX,&X);CHKERRQ(ierr);
    //ierr = MatView(EVAL,PETSC_VIEWER_STDOUT_WORLD);CHKERRQ(ierr);

    ierr = MatMatMult(A,X,MAT_INITIAL_MATRIX,1.0,&C1);CHKERRQ(ierr); /* C1 = A*X */
    ierr = MatMatMult(B,X,MAT_INITIAL_MATRIX,1.0,&C2);CHKERRQ(ierr); /* C2 = B*X */
 
    /* Get vector eval from matrix EVAL for MatDiagonalScale() */
    ierr = MatGetLocalSize(EVAL,&m,NULL);CHKERRQ(ierr); 
    ierr = MatDenseGetArray(EVAL,&Earray);CHKERRQ(ierr);
    ierr = VecCreateMPIWithArray(PetscObjectComm((PetscObject)EVAL),1,m,PETSC_DECIDE,Earray,&eval);CHKERRQ(ierr);
    ierr = MatDenseRestoreArray(EVAL,&Earray);CHKERRQ(ierr);
    //ierr = VecView(eval,PETSC_VIEWER_STDOUT_WORLD);CHKERRQ(ierr);
 
    ierr = MatDiagonalScale(C2,NULL,eval);CHKERRQ(ierr); /* C2 = B*X*eval */
    ierr = MatAXPY(C1,-1.0,C2,SAME_NONZERO_PATTERN);CHKERRQ(ierr); /* C1 = - C2 + C1 */
    ierr = MatNorm(C1,NORM_FROBENIUS,&norm);CHKERRQ(ierr);
    if (norm > 1.e-14) {
      if (!rank) printf(" Warning: || A*X - B*X*We || = %g\n",norm);
    }

    ierr = MatDestroy(&C1);CHKERRQ(ierr);
    ierr = MatDestroy(&C2);CHKERRQ(ierr);
    ierr = VecDestroy(&eval);CHKERRQ(ierr);
    ierr = MatDestroy(&EVAL);CHKERRQ(ierr);
    ierr = MatDestroy(&X);CHKERRQ(ierr);

  } 
  if (!isElemental) {
    ierr = MatDestroy(&Ae);CHKERRQ(ierr);
    ierr = MatDestroy(&Be);CHKERRQ(ierr);

    /* Test MAT_REUSE_MATRIX which is only supported for inplace conversion */
    ierr = MatConvert(A, MATELEMENTAL, MAT_INPLACE_MATRIX, &A);CHKERRQ(ierr);
    //ierr = MatView(A,PETSC_VIEWER_STDOUT_WORLD);CHKERRQ(ierr);
  }

  ierr = MatDestroy(&We);CHKERRQ(ierr);
  ierr = MatDestroy(&Xe);CHKERRQ(ierr);
  ierr = MatDestroy(&A);CHKERRQ(ierr);
  ierr = MatDestroy(&B);CHKERRQ(ierr);
  ierr = PetscFinalize();
  return 0;
}
