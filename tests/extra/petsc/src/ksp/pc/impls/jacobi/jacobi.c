
/*  --------------------------------------------------------------------

     This file implements a Jacobi preconditioner in PETSc as part of PC.
     You can use this as a starting point for implementing your own
     preconditioner that is not provided with PETSc. (You might also consider
     just using PCSHELL)

     The following basic routines are required for each preconditioner.
          PCCreate_XXX()          - Creates a preconditioner context
          PCSetFromOptions_XXX()  - Sets runtime options
          PCApply_XXX()           - Applies the preconditioner
          PCDestroy_XXX()         - Destroys the preconditioner context
     where the suffix "_XXX" denotes a particular implementation, in
     this case we use _Jacobi (e.g., PCCreate_Jacobi, PCApply_Jacobi).
     These routines are actually called via the common user interface
     routines PCCreate(), PCSetFromOptions(), PCApply(), and PCDestroy(),
     so the application code interface remains identical for all
     preconditioners.

     Another key routine is:
          PCSetUp_XXX()           - Prepares for the use of a preconditioner
     by setting data structures and options.   The interface routine PCSetUp()
     is not usually called directly by the user, but instead is called by
     PCApply() if necessary.

     Additional basic routines are:
          PCView_XXX()            - Prints details of runtime options that
                                    have actually been used.
     These are called by application codes via the interface routines
     PCView().

     The various types of solvers (preconditioners, Krylov subspace methods,
     nonlinear solvers, timesteppers) are all organized similarly, so the
     above description applies to these categories also.  One exception is
     that the analogues of PCApply() for these components are KSPSolve(),
     SNESSolve(), and TSSolve().

     Additional optional functionality unique to preconditioners is left and
     right symmetric preconditioner application via PCApplySymmetricLeft()
     and PCApplySymmetricRight().  The Jacobi implementation is
     PCApplySymmetricLeftOrRight_Jacobi().

    -------------------------------------------------------------------- */

/*
   Include files needed for the Jacobi preconditioner:
     pcimpl.h - private include file intended for use by all preconditioners
*/

#include <petsc/private/pcimpl.h>   /*I "petscpc.h" I*/

const char *const PCJacobiTypes[]    = {"DIAGONAL","ROWMAX","ROWSUM","PCJacobiType","PC_JACOBI_",0};

/*
   Private context (data structure) for the Jacobi preconditioner.
*/
typedef struct {
  Vec diag;                      /* vector containing the reciprocals of the diagonal elements of the preconditioner matrix */
  Vec diagsqrt;                  /* vector containing the reciprocals of the square roots of
                                    the diagonal elements of the preconditioner matrix (used
                                    only for symmetric preconditioner application) */
  PetscBool userowmax;           /* set with PCJacobiSetType() */
  PetscBool userowsum;
  PetscBool useabs;              /* use the absolute values of the diagonal entries */
} PC_Jacobi;

#undef __FUNCT__
#define __FUNCT__ "PCJacobiSetType_Jacobi"
static PetscErrorCode  PCJacobiSetType_Jacobi(PC pc,PCJacobiType type)
{
  PC_Jacobi *j = (PC_Jacobi*)pc->data;

  PetscFunctionBegin;
  j->userowmax = PETSC_FALSE;
  j->userowsum = PETSC_FALSE;
  if (type == PC_JACOBI_ROWMAX) {
    j->userowmax = PETSC_TRUE;
  } else if (type == PC_JACOBI_ROWSUM) {
    j->userowsum = PETSC_TRUE;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PCJacobiGetType_Jacobi"
static PetscErrorCode  PCJacobiGetType_Jacobi(PC pc,PCJacobiType *type)
{
  PC_Jacobi *j = (PC_Jacobi*)pc->data;

  PetscFunctionBegin;
  if (j->userowmax) {
    *type = PC_JACOBI_ROWMAX;
  } else if (j->userowsum) {
    *type = PC_JACOBI_ROWSUM;
  } else {
    *type = PC_JACOBI_DIAGONAL;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PCJacobiSetUseAbs_Jacobi"
static PetscErrorCode  PCJacobiSetUseAbs_Jacobi(PC pc,PetscBool flg)
{
  PC_Jacobi *j = (PC_Jacobi*)pc->data;

  PetscFunctionBegin;
  j->useabs = flg;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PCJacobiGetUseAbs_Jacobi"
static PetscErrorCode  PCJacobiGetUseAbs_Jacobi(PC pc,PetscBool *flg)
{
  PC_Jacobi *j = (PC_Jacobi*)pc->data;

  PetscFunctionBegin;
  *flg = j->useabs;
  PetscFunctionReturn(0);
}

/* -------------------------------------------------------------------------- */
/*
   PCSetUp_Jacobi - Prepares for the use of the Jacobi preconditioner
                    by setting data structures and options.

   Input Parameter:
.  pc - the preconditioner context

   Application Interface Routine: PCSetUp()

   Notes:
   The interface routine PCSetUp() is not usually called directly by
   the user, but instead is called by PCApply() if necessary.
*/
#undef __FUNCT__
#define __FUNCT__ "PCSetUp_Jacobi"
static PetscErrorCode PCSetUp_Jacobi(PC pc)
{
  PC_Jacobi      *jac = (PC_Jacobi*)pc->data;
  Vec            diag,diagsqrt;
  PetscErrorCode ierr;
  PetscInt       n,i;
  PetscScalar    *x;
  PetscBool      zeroflag = PETSC_FALSE;

  PetscFunctionBegin;
  /*
       For most preconditioners the code would begin here something like

  if (pc->setupcalled == 0) { allocate space the first time this is ever called
    ierr = MatCreateVecs(pc->mat,&jac->diag);CHKERRQ(ierr);
    PetscLogObjectParent((PetscObject)pc,(PetscObject)jac->diag);
  }

    But for this preconditioner we want to support use of both the matrix' diagonal
    elements (for left or right preconditioning) and square root of diagonal elements
    (for symmetric preconditioning).  Hence we do not allocate space here, since we
    don't know at this point which will be needed (diag and/or diagsqrt) until the user
    applies the preconditioner, and we don't want to allocate BOTH unless we need
    them both.  Thus, the diag and diagsqrt are allocated in PCSetUp_Jacobi_NonSymmetric()
    and PCSetUp_Jacobi_Symmetric(), respectively.
  */

  /*
    Here we set up the preconditioner; that is, we copy the diagonal values from
    the matrix and put them into a format to make them quick to apply as a preconditioner.
  */
  diag     = jac->diag;
  diagsqrt = jac->diagsqrt;

  if (diag) {
    if (jac->userowmax) {
      ierr = MatGetRowMaxAbs(pc->pmat,diag,NULL);CHKERRQ(ierr);
    } else if (jac->userowsum) {
      ierr = MatGetRowSum(pc->pmat,diag);CHKERRQ(ierr);
    } else {
      ierr = MatGetDiagonal(pc->pmat,diag);CHKERRQ(ierr);
    }
    ierr = VecReciprocal(diag);CHKERRQ(ierr);
    ierr = VecGetLocalSize(diag,&n);CHKERRQ(ierr);
    ierr = VecGetArray(diag,&x);CHKERRQ(ierr);
    if (jac->useabs) {
      for (i=0; i<n; i++) x[i] = PetscAbsScalar(x[i]);
    }
    for (i=0; i<n; i++) {
      if (x[i] == 0.0) {
        x[i]     = 1.0;
        zeroflag = PETSC_TRUE;
      }
    }
    ierr = VecRestoreArray(diag,&x);CHKERRQ(ierr);
  }
  if (diagsqrt) {
    if (jac->userowmax) {
      ierr = MatGetRowMaxAbs(pc->pmat,diagsqrt,NULL);CHKERRQ(ierr);
    } else if (jac->userowsum) {
      ierr = MatGetRowSum(pc->pmat,diagsqrt);CHKERRQ(ierr);
    } else {
      ierr = MatGetDiagonal(pc->pmat,diagsqrt);CHKERRQ(ierr);
    }
    ierr = VecGetLocalSize(diagsqrt,&n);CHKERRQ(ierr);
    ierr = VecGetArray(diagsqrt,&x);CHKERRQ(ierr);
    for (i=0; i<n; i++) {
      if (x[i] != 0.0) x[i] = 1.0/PetscSqrtReal(PetscAbsScalar(x[i]));
      else {
        x[i]     = 1.0;
        zeroflag = PETSC_TRUE;
      }
    }
    ierr = VecRestoreArray(diagsqrt,&x);CHKERRQ(ierr);
  }
  if (zeroflag) {
    ierr = PetscInfo(pc,"Zero detected in diagonal of matrix, using 1 at those locations\n");CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}
/* -------------------------------------------------------------------------- */
/*
   PCSetUp_Jacobi_Symmetric - Allocates the vector needed to store the
   inverse of the square root of the diagonal entries of the matrix.  This
   is used for symmetric application of the Jacobi preconditioner.

   Input Parameter:
.  pc - the preconditioner context
*/
#undef __FUNCT__
#define __FUNCT__ "PCSetUp_Jacobi_Symmetric"
static PetscErrorCode PCSetUp_Jacobi_Symmetric(PC pc)
{
  PetscErrorCode ierr;
  PC_Jacobi      *jac = (PC_Jacobi*)pc->data;

  PetscFunctionBegin;
  ierr = MatCreateVecs(pc->pmat,&jac->diagsqrt,0);CHKERRQ(ierr);
  ierr = PetscLogObjectParent((PetscObject)pc,(PetscObject)jac->diagsqrt);CHKERRQ(ierr);
  ierr = PCSetUp_Jacobi(pc);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}
/* -------------------------------------------------------------------------- */
/*
   PCSetUp_Jacobi_NonSymmetric - Allocates the vector needed to store the
   inverse of the diagonal entries of the matrix.  This is used for left of
   right application of the Jacobi preconditioner.

   Input Parameter:
.  pc - the preconditioner context
*/
#undef __FUNCT__
#define __FUNCT__ "PCSetUp_Jacobi_NonSymmetric"
static PetscErrorCode PCSetUp_Jacobi_NonSymmetric(PC pc)
{
  PetscErrorCode ierr;
  PC_Jacobi      *jac = (PC_Jacobi*)pc->data;

  PetscFunctionBegin;
  ierr = MatCreateVecs(pc->pmat,&jac->diag,0);CHKERRQ(ierr);
  ierr = PetscLogObjectParent((PetscObject)pc,(PetscObject)jac->diag);CHKERRQ(ierr);
  ierr = PCSetUp_Jacobi(pc);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}
/* -------------------------------------------------------------------------- */
/*
   PCApply_Jacobi - Applies the Jacobi preconditioner to a vector.

   Input Parameters:
.  pc - the preconditioner context
.  x - input vector

   Output Parameter:
.  y - output vector

   Application Interface Routine: PCApply()
 */
#undef __FUNCT__
#define __FUNCT__ "PCApply_Jacobi"
static PetscErrorCode PCApply_Jacobi(PC pc,Vec x,Vec y)
{
  PC_Jacobi      *jac = (PC_Jacobi*)pc->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (!jac->diag) {
    ierr = PCSetUp_Jacobi_NonSymmetric(pc);CHKERRQ(ierr);
  }
  ierr = VecPointwiseMult(y,x,jac->diag);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}
/* -------------------------------------------------------------------------- */
/*
   PCApplySymmetricLeftOrRight_Jacobi - Applies the left or right part of a
   symmetric preconditioner to a vector.

   Input Parameters:
.  pc - the preconditioner context
.  x - input vector

   Output Parameter:
.  y - output vector

   Application Interface Routines: PCApplySymmetricLeft(), PCApplySymmetricRight()
*/
#undef __FUNCT__
#define __FUNCT__ "PCApplySymmetricLeftOrRight_Jacobi"
static PetscErrorCode PCApplySymmetricLeftOrRight_Jacobi(PC pc,Vec x,Vec y)
{
  PetscErrorCode ierr;
  PC_Jacobi      *jac = (PC_Jacobi*)pc->data;

  PetscFunctionBegin;
  if (!jac->diagsqrt) {
    ierr = PCSetUp_Jacobi_Symmetric(pc);CHKERRQ(ierr);
  }
  VecPointwiseMult(y,x,jac->diagsqrt);
  PetscFunctionReturn(0);
}
/* -------------------------------------------------------------------------- */
#undef __FUNCT__
#define __FUNCT__ "PCReset_Jacobi"
static PetscErrorCode PCReset_Jacobi(PC pc)
{
  PC_Jacobi      *jac = (PC_Jacobi*)pc->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = VecDestroy(&jac->diag);CHKERRQ(ierr);
  ierr = VecDestroy(&jac->diagsqrt);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/*
   PCDestroy_Jacobi - Destroys the private context for the Jacobi preconditioner
   that was created with PCCreate_Jacobi().

   Input Parameter:
.  pc - the preconditioner context

   Application Interface Routine: PCDestroy()
*/
#undef __FUNCT__
#define __FUNCT__ "PCDestroy_Jacobi"
static PetscErrorCode PCDestroy_Jacobi(PC pc)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PCReset_Jacobi(pc);CHKERRQ(ierr);

  /*
      Free the private data structure that was hanging off the PC
  */
  ierr = PetscFree(pc->data);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PCSetFromOptions_Jacobi"
static PetscErrorCode PCSetFromOptions_Jacobi(PetscOptionItems *PetscOptionsObject,PC pc)
{
  PC_Jacobi      *jac = (PC_Jacobi*)pc->data;
  PetscErrorCode ierr;
  PetscBool      flg;
  PCJacobiType   deflt,type;

  PetscFunctionBegin;
  ierr = PCJacobiGetType(pc,&deflt);CHKERRQ(ierr);
  ierr = PetscOptionsHead(PetscOptionsObject,"Jacobi options");CHKERRQ(ierr);
  ierr = PetscOptionsEnum("-pc_jacobi_type","How to construct diagonal matrix","PCJacobiSetType",PCJacobiTypes,(PetscEnum)deflt,(PetscEnum*)&type,&flg);CHKERRQ(ierr);
  if (flg) {
    ierr = PCJacobiSetType(pc,type);CHKERRQ(ierr);
  }
  ierr = PetscOptionsBool("-pc_jacobi_abs","Use absolute values of diagaonal entries","PCJacobiSetUseAbs",jac->useabs,&jac->useabs,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsTail();CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/* -------------------------------------------------------------------------- */
/*
   PCCreate_Jacobi - Creates a Jacobi preconditioner context, PC_Jacobi,
   and sets this as the private data within the generic preconditioning
   context, PC, that was created within PCCreate().

   Input Parameter:
.  pc - the preconditioner context

   Application Interface Routine: PCCreate()
*/

/*MC
     PCJACOBI - Jacobi (i.e. diagonal scaling preconditioning)

   Options Database Key:
+    -pc_jacobi_type <diagonal,rowmax,rowsum> - approach for forming the preconditioner
-    -pc_jacobi_abs - use the absolute value of the diagonal entry

   Level: beginner

  Concepts: Jacobi, diagonal scaling, preconditioners

  Notes: By using KSPSetPCSide(ksp,PC_SYMMETRIC) or -ksp_pc_side symmetric
         can scale each side of the matrix by the square root of the diagonal entries.

         Zero entries along the diagonal are replaced with the value 1.0

         See PCPBJACOBI for a point-block Jacobi preconditioner

.seealso:  PCCreate(), PCSetType(), PCType (for list of available types), PC,
           PCJacobiSetType(), PCJacobiSetUseAbs(), PCJacobiGetUseAbs(), PCPBJACOBI
M*/

#undef __FUNCT__
#define __FUNCT__ "PCCreate_Jacobi"
PETSC_EXTERN PetscErrorCode PCCreate_Jacobi(PC pc)
{
  PC_Jacobi      *jac;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  /*
     Creates the private data structure for this preconditioner and
     attach it to the PC object.
  */
  ierr     = PetscNewLog(pc,&jac);CHKERRQ(ierr);
  pc->data = (void*)jac;

  /*
     Initialize the pointers to vectors to ZERO; these will be used to store
     diagonal entries of the matrix for fast preconditioner application.
  */
  jac->diag      = 0;
  jac->diagsqrt  = 0;
  jac->userowmax = PETSC_FALSE;
  jac->userowsum = PETSC_FALSE;
  jac->useabs    = PETSC_FALSE;

  /*
      Set the pointers for the functions that are provided above.
      Now when the user-level routines (such as PCApply(), PCDestroy(), etc.)
      are called, they will automatically call these functions.  Note we
      choose not to provide a couple of these functions since they are
      not needed.
  */
  pc->ops->apply               = PCApply_Jacobi;
  pc->ops->applytranspose      = PCApply_Jacobi;
  pc->ops->setup               = PCSetUp_Jacobi;
  pc->ops->reset               = PCReset_Jacobi;
  pc->ops->destroy             = PCDestroy_Jacobi;
  pc->ops->setfromoptions      = PCSetFromOptions_Jacobi;
  pc->ops->view                = 0;
  pc->ops->applyrichardson     = 0;
  pc->ops->applysymmetricleft  = PCApplySymmetricLeftOrRight_Jacobi;
  pc->ops->applysymmetricright = PCApplySymmetricLeftOrRight_Jacobi;

  ierr = PetscObjectComposeFunction((PetscObject)pc,"PCJacobiSetType_C",PCJacobiSetType_Jacobi);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)pc,"PCJacobiGetType_C",PCJacobiGetType_Jacobi);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)pc,"PCJacobiSetUseAbs_C",PCJacobiSetUseAbs_Jacobi);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)pc,"PCJacobiGetUseAbs_C",PCJacobiGetUseAbs_Jacobi);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PCJacobiSetUseAbs"
/*@
   PCJacobiSetUseAbs - Causes the Jacobi preconditioner to use the
      absolute values of the digonal divisors in the preconditioner

   Logically Collective on PC

   Input Parameters:
+  pc - the preconditioner context
-  flg - whether to use absolute values or not

   Options Database Key:
.  -pc_jacobi_abs

   Notes: This takes affect at the next construction of the preconditioner

   Level: intermediate

   Concepts: Jacobi preconditioner

.seealso: PCJacobiaSetType(), PCJacobiGetUseAbs()

@*/
PetscErrorCode  PCJacobiSetUseAbs(PC pc,PetscBool flg)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(pc,PC_CLASSID,1);
  ierr = PetscTryMethod(pc,"PCJacobiSetUseAbs_C",(PC,PetscBool),(pc,flg));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PCJacobiGetUseAbs"
/*@
   PCJacobiGetUseAbs - Determines if the Jacobi preconditioner uses the
      absolute values of the digonal divisors in the preconditioner

   Logically Collective on PC

   Input Parameter:
.  pc - the preconditioner context

   Output Parameter:
.  flg - whether to use absolute values or not

   Options Database Key:
.  -pc_jacobi_abs

   Level: intermediate

   Concepts: Jacobi preconditioner

.seealso: PCJacobiaSetType(), PCJacobiSetUseAbs(), PCJacobiGetType()

@*/
PetscErrorCode  PCJacobiGetUseAbs(PC pc,PetscBool *flg)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(pc,PC_CLASSID,1);
  ierr = PetscUseMethod(pc,"PCJacobiGetUseAbs_C",(PC,PetscBool*),(pc,flg));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PCJacobiSetType"
/*@
   PCJacobiSetType - Causes the Jacobi preconditioner to use either the diagonal, the maximum entry in each row,
      of the sum of rows entries for the diagonal preconditioner

   Logically Collective on PC

   Input Parameters:
+  pc - the preconditioner context
-  type - PC_JACOBI_DIAGONAL, PC_JACOBI_ROWMAX, PC_JACOBI_ROWSUM

   Options Database Key:
.  -pc_jacobi_type <diagonal,rowmax,rowsum>

   Level: intermediate

   Concepts: Jacobi preconditioner

.seealso: PCJacobiaUseAbs(), PCJacobiGetType()
@*/
PetscErrorCode  PCJacobiSetType(PC pc,PCJacobiType type)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(pc,PC_CLASSID,1);
  ierr = PetscTryMethod(pc,"PCJacobiSetType_C",(PC,PCJacobiType),(pc,type));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PCJacobiGetType"
/*@
   PCJacobiGetType - Gets how the diagonal matrix is produced for the preconditioner

   Not Collective on PC

   Input Parameter:
.  pc - the preconditioner context

   Output Parameter:
.  type - PC_JACOBI_DIAGONAL, PC_JACOBI_ROWMAX, PC_JACOBI_ROWSUM

   Level: intermediate

   Concepts: Jacobi preconditioner

.seealso: PCJacobiaUseAbs(), PCJacobiSetType()
@*/
PetscErrorCode  PCJacobiGetType(PC pc,PCJacobiType *type)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(pc,PC_CLASSID,1);
  ierr = PetscUseMethod(pc,"PCJacobiGetType_C",(PC,PCJacobiType*),(pc,type));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}
