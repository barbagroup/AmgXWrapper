
/*
   Defines a  (S)SOR  preconditioner for any Mat implementation
*/
#include <petsc/private/pcimpl.h>               /*I "petscpc.h" I*/

typedef struct {
  PetscInt   its;         /* inner iterations, number of sweeps */
  PetscInt   lits;        /* local inner iterations, number of sweeps applied by the local matrix mat->A */
  MatSORType sym;         /* forward, reverse, symmetric etc. */
  PetscReal  omega;
  PetscReal  fshift;
} PC_SOR;

#undef __FUNCT__
#define __FUNCT__ "PCDestroy_SOR"
static PetscErrorCode PCDestroy_SOR(PC pc)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscFree(pc->data);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PCApply_SOR"
static PetscErrorCode PCApply_SOR(PC pc,Vec x,Vec y)
{
  PC_SOR         *jac = (PC_SOR*)pc->data;
  PetscErrorCode ierr;
  PetscInt       flag = jac->sym | SOR_ZERO_INITIAL_GUESS;
  PetscReal      fshift;
  
  PetscFunctionBegin;
  fshift = (jac->fshift ? jac->fshift : pc->erroriffailure ? 0.0 : -1.0);
  ierr = MatSOR(pc->pmat,x,jac->omega,(MatSORType)flag,fshift,jac->its,jac->lits,y);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PCApplyRichardson_SOR"
static PetscErrorCode PCApplyRichardson_SOR(PC pc,Vec b,Vec y,Vec w,PetscReal rtol,PetscReal abstol, PetscReal dtol,PetscInt its,PetscBool guesszero,PetscInt *outits,PCRichardsonConvergedReason *reason)
{
  PC_SOR         *jac = (PC_SOR*)pc->data;
  PetscErrorCode ierr;
  MatSORType     stype = jac->sym;
  PetscReal      fshift;

  PetscFunctionBegin;
  ierr = PetscInfo1(pc,"Warning, convergence critera ignored, using %D iterations\n",its);CHKERRQ(ierr);
  if (guesszero) stype = (MatSORType) (stype | SOR_ZERO_INITIAL_GUESS);
  fshift = (jac->fshift ? jac->fshift : pc->erroriffailure ? 0.0 : -1.0);
  ierr = MatSOR(pc->pmat,b,jac->omega,stype,fshift,its*jac->its,jac->lits,y);CHKERRQ(ierr);
  *outits = its;
  *reason = PCRICHARDSON_CONVERGED_ITS;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PCSetFromOptions_SOR"
PetscErrorCode PCSetFromOptions_SOR(PetscOptionItems *PetscOptionsObject,PC pc)
{
  PC_SOR         *jac = (PC_SOR*)pc->data;
  PetscErrorCode ierr;
  PetscBool      flg;

  PetscFunctionBegin;
  ierr = PetscOptionsHead(PetscOptionsObject,"(S)SOR options");CHKERRQ(ierr);
  ierr = PetscOptionsReal("-pc_sor_omega","relaxation factor (0 < omega < 2)","PCSORSetOmega",jac->omega,&jac->omega,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsReal("-pc_sor_diagonal_shift","Add to the diagonal entries","",jac->fshift,&jac->fshift,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsInt("-pc_sor_its","number of inner SOR iterations","PCSORSetIterations",jac->its,&jac->its,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsInt("-pc_sor_lits","number of local inner SOR iterations","PCSORSetIterations",jac->lits,&jac->lits,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsBoolGroupBegin("-pc_sor_symmetric","SSOR, not SOR","PCSORSetSymmetric",&flg);CHKERRQ(ierr);
  if (flg) {ierr = PCSORSetSymmetric(pc,SOR_SYMMETRIC_SWEEP);CHKERRQ(ierr);}
  ierr = PetscOptionsBoolGroup("-pc_sor_backward","use backward sweep instead of forward","PCSORSetSymmetric",&flg);CHKERRQ(ierr);
  if (flg) {ierr = PCSORSetSymmetric(pc,SOR_BACKWARD_SWEEP);CHKERRQ(ierr);}
  ierr = PetscOptionsBoolGroup("-pc_sor_forward","use forward sweep","PCSORSetSymmetric",&flg);CHKERRQ(ierr);
  if (flg) {ierr = PCSORSetSymmetric(pc,SOR_FORWARD_SWEEP);CHKERRQ(ierr);}
  ierr = PetscOptionsBoolGroup("-pc_sor_local_symmetric","use SSOR separately on each processor","PCSORSetSymmetric",&flg);CHKERRQ(ierr);
  if (flg) {ierr = PCSORSetSymmetric(pc,SOR_LOCAL_SYMMETRIC_SWEEP);CHKERRQ(ierr);}
  ierr = PetscOptionsBoolGroup("-pc_sor_local_backward","use backward sweep locally","PCSORSetSymmetric",&flg);CHKERRQ(ierr);
  if (flg) {ierr = PCSORSetSymmetric(pc,SOR_LOCAL_BACKWARD_SWEEP);CHKERRQ(ierr);}
  ierr = PetscOptionsBoolGroupEnd("-pc_sor_local_forward","use forward sweep locally","PCSORSetSymmetric",&flg);CHKERRQ(ierr);
  if (flg) {ierr = PCSORSetSymmetric(pc,SOR_LOCAL_FORWARD_SWEEP);CHKERRQ(ierr);}
  ierr = PetscOptionsTail();CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PCView_SOR"
PetscErrorCode PCView_SOR(PC pc,PetscViewer viewer)
{
  PC_SOR         *jac = (PC_SOR*)pc->data;
  MatSORType     sym  = jac->sym;
  const char     *sortype;
  PetscErrorCode ierr;
  PetscBool      iascii;

  PetscFunctionBegin;
  ierr = PetscObjectTypeCompare((PetscObject)viewer,PETSCVIEWERASCII,&iascii);CHKERRQ(ierr);
  if (iascii) {
    if (sym & SOR_ZERO_INITIAL_GUESS) {ierr = PetscViewerASCIIPrintf(viewer,"  SOR:  zero initial guess\n");CHKERRQ(ierr);}
    if (sym == SOR_APPLY_UPPER)                                              sortype = "apply_upper";
    else if (sym == SOR_APPLY_LOWER)                                         sortype = "apply_lower";
    else if (sym & SOR_EISENSTAT)                                            sortype = "Eisenstat";
    else if ((sym & SOR_SYMMETRIC_SWEEP) == SOR_SYMMETRIC_SWEEP)             sortype = "symmetric";
    else if (sym & SOR_BACKWARD_SWEEP)                                       sortype = "backward";
    else if (sym & SOR_FORWARD_SWEEP)                                        sortype = "forward";
    else if ((sym & SOR_LOCAL_SYMMETRIC_SWEEP) == SOR_LOCAL_SYMMETRIC_SWEEP) sortype = "local_symmetric";
    else if (sym & SOR_LOCAL_FORWARD_SWEEP)                                  sortype = "local_forward";
    else if (sym & SOR_LOCAL_BACKWARD_SWEEP)                                 sortype = "local_backward";
    else                                                                     sortype = "unknown";
    ierr = PetscViewerASCIIPrintf(viewer,"  SOR: type = %s, iterations = %D, local iterations = %D, omega = %g\n",sortype,jac->its,jac->lits,(double)jac->omega);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}


/* ------------------------------------------------------------------------------*/
#undef __FUNCT__
#define __FUNCT__ "PCSORSetSymmetric_SOR"
static PetscErrorCode  PCSORSetSymmetric_SOR(PC pc,MatSORType flag)
{
  PC_SOR *jac = (PC_SOR*)pc->data;

  PetscFunctionBegin;
  jac->sym = flag;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PCSORSetOmega_SOR"
static PetscErrorCode  PCSORSetOmega_SOR(PC pc,PetscReal omega)
{
  PC_SOR *jac = (PC_SOR*)pc->data;

  PetscFunctionBegin;
  if (omega >= 2.0 || omega <= 0.0) SETERRQ(PetscObjectComm((PetscObject)pc),PETSC_ERR_ARG_OUTOFRANGE,"Relaxation out of range");
  jac->omega = omega;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PCSORSetIterations_SOR"
static PetscErrorCode  PCSORSetIterations_SOR(PC pc,PetscInt its,PetscInt lits)
{
  PC_SOR *jac = (PC_SOR*)pc->data;

  PetscFunctionBegin;
  jac->its  = its;
  jac->lits = lits;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PCSORGetSymmetric_SOR"
static PetscErrorCode  PCSORGetSymmetric_SOR(PC pc,MatSORType *flag)
{
  PC_SOR *jac = (PC_SOR*)pc->data;

  PetscFunctionBegin;
  *flag = jac->sym;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PCSORGetOmega_SOR"
static PetscErrorCode  PCSORGetOmega_SOR(PC pc,PetscReal *omega)
{
  PC_SOR *jac = (PC_SOR*)pc->data;

  PetscFunctionBegin;
  *omega = jac->omega;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PCSORGetIterations_SOR"
static PetscErrorCode  PCSORGetIterations_SOR(PC pc,PetscInt *its,PetscInt *lits)
{
  PC_SOR *jac = (PC_SOR*)pc->data;

  PetscFunctionBegin;
  if (its)  *its = jac->its;
  if (lits) *lits = jac->lits;
  PetscFunctionReturn(0);
}

/* ------------------------------------------------------------------------------*/
#undef __FUNCT__
#define __FUNCT__ "PCSORGetSymmetric"
/*@
   PCSORGetSymmetric - Gets the form the SOR preconditioner is using;   backward, or forward relaxation.  The local variants perform SOR on
   each processor.  By default forward relaxation is used.

   Logically Collective on PC

   Input Parameter:
.  pc - the preconditioner context

   Output Parameter:
.  flag - one of the following
.vb
    SOR_FORWARD_SWEEP
    SOR_BACKWARD_SWEEP
    SOR_SYMMETRIC_SWEEP
    SOR_LOCAL_FORWARD_SWEEP
    SOR_LOCAL_BACKWARD_SWEEP
    SOR_LOCAL_SYMMETRIC_SWEEP
.ve

   Options Database Keys:
+  -pc_sor_symmetric - Activates symmetric version
.  -pc_sor_backward - Activates backward version
.  -pc_sor_local_forward - Activates local forward version
.  -pc_sor_local_symmetric - Activates local symmetric version
-  -pc_sor_local_backward - Activates local backward version

   Notes:
   To use the Eisenstat trick with SSOR, employ the PCEISENSTAT preconditioner,
   which can be chosen with the option
.  -pc_type eisenstat - Activates Eisenstat trick

   Level: intermediate

.keywords: PC, SOR, SSOR, set, relaxation, sweep, forward, backward, symmetric

.seealso: PCEisenstatSetOmega(), PCSORSetIterations(), PCSORSetOmega(), PCSORSetSymmetric()
@*/
PetscErrorCode  PCSORGetSymmetric(PC pc,MatSORType *flag)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(pc,PC_CLASSID,1);
  ierr = PetscUseMethod(pc,"PCSORGetSymmetric_C",(PC,MatSORType*),(pc,flag));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PCSORGetOmega"
/*@
   PCSORGetOmega - Gets the SOR relaxation coefficient, omega
   (where omega = 1.0 by default).

   Logically Collective on PC

   Input Parameter:
.  pc - the preconditioner context

   Output Parameter:
.  omega - relaxation coefficient (0 < omega < 2).

   Options Database Key:
.  -pc_sor_omega <omega> - Sets omega

   Level: intermediate

.keywords: PC, SOR, SSOR, set, relaxation, omega

.seealso: PCSORSetSymmetric(), PCSORSetIterations(), PCEisenstatSetOmega(), PCSORSetOmega()
@*/
PetscErrorCode  PCSORGetOmega(PC pc,PetscReal *omega)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(pc,PC_CLASSID,1);
  ierr = PetscUseMethod(pc,"PCSORGetOmega_C",(PC,PetscReal*),(pc,omega));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PCSORGetIterations"
/*@
   PCSORGetIterations - Gets the number of inner iterations to
   be used by the SOR preconditioner. The default is 1.

   Logically Collective on PC

   Input Parameter:
.  pc - the preconditioner context

   Output Parameter:
+  lits - number of local iterations, smoothings over just variables on processor
-  its - number of parallel iterations to use; each parallel iteration has lits local iterations

   Options Database Key:
+  -pc_sor_its <its> - Sets number of iterations
-  -pc_sor_lits <lits> - Sets number of local iterations

   Level: intermediate

   Notes: When run on one processor the number of smoothings is lits*its

.keywords: PC, SOR, SSOR, set, iterations

.seealso: PCSORSetOmega(), PCSORSetSymmetric(), PCSORSetIterations()
@*/
PetscErrorCode  PCSORGetIterations(PC pc,PetscInt *its,PetscInt *lits)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(pc,PC_CLASSID,1);
  ierr = PetscUseMethod(pc,"PCSORGetIterations_C",(PC,PetscInt*,PetscInt*),(pc,its,lits));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PCSORSetSymmetric"
/*@
   PCSORSetSymmetric - Sets the SOR preconditioner to use symmetric (SSOR),
   backward, or forward relaxation.  The local variants perform SOR on
   each processor.  By default forward relaxation is used.

   Logically Collective on PC

   Input Parameters:
+  pc - the preconditioner context
-  flag - one of the following
.vb
    SOR_FORWARD_SWEEP
    SOR_BACKWARD_SWEEP
    SOR_SYMMETRIC_SWEEP
    SOR_LOCAL_FORWARD_SWEEP
    SOR_LOCAL_BACKWARD_SWEEP
    SOR_LOCAL_SYMMETRIC_SWEEP
.ve

   Options Database Keys:
+  -pc_sor_symmetric - Activates symmetric version
.  -pc_sor_backward - Activates backward version
.  -pc_sor_local_forward - Activates local forward version
.  -pc_sor_local_symmetric - Activates local symmetric version
-  -pc_sor_local_backward - Activates local backward version

   Notes:
   To use the Eisenstat trick with SSOR, employ the PCEISENSTAT preconditioner,
   which can be chosen with the option
.  -pc_type eisenstat - Activates Eisenstat trick

   Level: intermediate

.keywords: PC, SOR, SSOR, set, relaxation, sweep, forward, backward, symmetric

.seealso: PCEisenstatSetOmega(), PCSORSetIterations(), PCSORSetOmega()
@*/
PetscErrorCode  PCSORSetSymmetric(PC pc,MatSORType flag)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(pc,PC_CLASSID,1);
  PetscValidLogicalCollectiveEnum(pc,flag,2);
  ierr = PetscTryMethod(pc,"PCSORSetSymmetric_C",(PC,MatSORType),(pc,flag));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PCSORSetOmega"
/*@
   PCSORSetOmega - Sets the SOR relaxation coefficient, omega
   (where omega = 1.0 by default).

   Logically Collective on PC

   Input Parameters:
+  pc - the preconditioner context
-  omega - relaxation coefficient (0 < omega < 2).

   Options Database Key:
.  -pc_sor_omega <omega> - Sets omega

   Level: intermediate

.keywords: PC, SOR, SSOR, set, relaxation, omega

.seealso: PCSORSetSymmetric(), PCSORSetIterations(), PCEisenstatSetOmega()
@*/
PetscErrorCode  PCSORSetOmega(PC pc,PetscReal omega)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(pc,PC_CLASSID,1);
  PetscValidLogicalCollectiveReal(pc,omega,2);
  ierr = PetscTryMethod(pc,"PCSORSetOmega_C",(PC,PetscReal),(pc,omega));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PCSORSetIterations"
/*@
   PCSORSetIterations - Sets the number of inner iterations to
   be used by the SOR preconditioner. The default is 1.

   Logically Collective on PC

   Input Parameters:
+  pc - the preconditioner context
.  lits - number of local iterations, smoothings over just variables on processor
-  its - number of parallel iterations to use; each parallel iteration has lits local iterations

   Options Database Key:
+  -pc_sor_its <its> - Sets number of iterations
-  -pc_sor_lits <lits> - Sets number of local iterations

   Level: intermediate

   Notes: When run on one processor the number of smoothings is lits*its

.keywords: PC, SOR, SSOR, set, iterations

.seealso: PCSORSetOmega(), PCSORSetSymmetric()
@*/
PetscErrorCode  PCSORSetIterations(PC pc,PetscInt its,PetscInt lits)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(pc,PC_CLASSID,1);
  PetscValidLogicalCollectiveInt(pc,its,2);
  ierr = PetscTryMethod(pc,"PCSORSetIterations_C",(PC,PetscInt,PetscInt),(pc,its,lits));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/*MC
     PCSOR - (S)SOR (successive over relaxation, Gauss-Seidel) preconditioning

   Options Database Keys:
+  -pc_sor_symmetric - Activates symmetric version
.  -pc_sor_backward - Activates backward version
.  -pc_sor_forward - Activates forward version
.  -pc_sor_local_forward - Activates local forward version
.  -pc_sor_local_symmetric - Activates local symmetric version  (default version)
.  -pc_sor_local_backward - Activates local backward version
.  -pc_sor_omega <omega> - Sets omega
.  -pc_sor_diagonal_shift <shift> - shift the diagonal entries; useful if the matrix has zeros on the diagonal
.  -pc_sor_its <its> - Sets number of iterations   (default 1)
-  -pc_sor_lits <lits> - Sets number of local iterations  (default 1)

   Level: beginner

  Concepts: SOR, preconditioners, Gauss-Seidel

   Notes: Only implemented for the AIJ  and SeqBAIJ matrix formats.
          Not a true parallel SOR, in parallel this implementation corresponds to block
          Jacobi with SOR on each block.

          For AIJ matrix if a diagonal entry is zero (and the diagonal shift is zero) then by default the inverse of that
          zero will be used and hence the KSPSolve() will terminate with KSP_DIVERGED_NANORIF. If the option
          KSPSetErrorIfNotConverged() or -ksp_error_if_not_converged the code will terminate as soon as it detects the 
          zero pivot.

          For SeqBAIJ matrices this implements point-block SOR, but the omega, its, lits options are not supported.

          For SeqBAIJ the diagonal blocks are inverted using dense LU with partial pivoting. If a zero pivot is detected 
          the computation is stopped with an error

  Developer Notes: We should add support for diagonal blocks that are singular to generate a Inf and thus cause KSPSolve()
           to terminate with KSP_DIVERGED_NANORIF instead of stopping the program allowing
           a nonlinear solver/ODE integrator to recover without stopping the program as currently happens.

.seealso:  PCCreate(), PCSetType(), PCType (for list of available types), PC,
           PCSORSetIterations(), PCSORSetSymmetric(), PCSORSetOmega(), PCEISENSTAT
M*/

#undef __FUNCT__
#define __FUNCT__ "PCCreate_SOR"
PETSC_EXTERN PetscErrorCode PCCreate_SOR(PC pc)
{
  PetscErrorCode ierr;
  PC_SOR         *jac;

  PetscFunctionBegin;
  ierr = PetscNewLog(pc,&jac);CHKERRQ(ierr);

  pc->ops->apply           = PCApply_SOR;
  pc->ops->applyrichardson = PCApplyRichardson_SOR;
  pc->ops->setfromoptions  = PCSetFromOptions_SOR;
  pc->ops->setup           = 0;
  pc->ops->view            = PCView_SOR;
  pc->ops->destroy         = PCDestroy_SOR;
  pc->data                 = (void*)jac;
  jac->sym                 = SOR_LOCAL_SYMMETRIC_SWEEP;
  jac->omega               = 1.0;
  jac->fshift              = 0.0;
  jac->its                 = 1;
  jac->lits                = 1;

  ierr = PetscObjectComposeFunction((PetscObject)pc,"PCSORSetSymmetric_C",PCSORSetSymmetric_SOR);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)pc,"PCSORSetOmega_C",PCSORSetOmega_SOR);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)pc,"PCSORSetIterations_C",PCSORSetIterations_SOR);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)pc,"PCSORGetSymmetric_C",PCSORGetSymmetric_SOR);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)pc,"PCSORGetOmega_C",PCSORGetOmega_SOR);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)pc,"PCSORGetIterations_C",PCSORGetIterations_SOR);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}





