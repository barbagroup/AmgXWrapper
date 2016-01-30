#define TAO_DLL

#include <petsc/private/taoimpl.h> /*I "petsctao.h" I*/

PetscBool TaoRegisterAllCalled = PETSC_FALSE;
PetscFunctionList TaoList = NULL;

PetscClassId TAO_CLASSID;
PetscLogEvent Tao_Solve, Tao_ObjectiveEval, Tao_GradientEval, Tao_ObjGradientEval, Tao_HessianEval, Tao_ConstraintsEval, Tao_JacobianEval;

const char *TaoSubSetTypes[] = {  "subvec","mask","matrixfree","TaoSubSetType","TAO_SUBSET_",0};

#undef __FUNCT__
#define __FUNCT__ "TaoCreate"
/*@
  TaoCreate - Creates a TAO solver

  Collective on MPI_Comm

  Input Parameter:
. comm - MPI communicator

  Output Parameter:
. newtao - the new Tao context

  Available methods include:
+    nls - Newton's method with line search for unconstrained minimization
.    ntr - Newton's method with trust region for unconstrained minimization
.    ntl - Newton's method with trust region, line search for unconstrained minimization
.    lmvm - Limited memory variable metric method for unconstrained minimization
.    cg - Nonlinear conjugate gradient method for unconstrained minimization
.    nm - Nelder-Mead algorithm for derivate-free unconstrained minimization
.    tron - Newton Trust Region method for bound constrained minimization
.    gpcg - Newton Trust Region method for quadratic bound constrained minimization
.    blmvm - Limited memory variable metric method for bound constrained minimization
.    lcl - Linearly constrained Lagrangian method for pde-constrained minimization
-    pounders - Model-based algorithm for nonlinear least squares

   Options Database Keys:
.   -tao_type - select which method TAO should use

   Level: beginner

.seealso: TaoSolve(), TaoDestroy()
@*/
PetscErrorCode TaoCreate(MPI_Comm comm, Tao *newtao)
{
  PetscErrorCode ierr;
  Tao            tao;

  PetscFunctionBegin;
  PetscValidPointer(newtao,2);
  *newtao = NULL;

  ierr = TaoInitializePackage();CHKERRQ(ierr);
  ierr = TaoLineSearchInitializePackage();CHKERRQ(ierr);

  ierr = PetscHeaderCreate(tao,TAO_CLASSID,"Tao","Optimization solver","Tao",comm,TaoDestroy,TaoView);CHKERRQ(ierr);
  tao->ops->computeobjective=0;
  tao->ops->computeobjectiveandgradient=0;
  tao->ops->computegradient=0;
  tao->ops->computehessian=0;
  tao->ops->computeseparableobjective=0;
  tao->ops->computeconstraints=0;
  tao->ops->computejacobian=0;
  tao->ops->computejacobianequality=0;
  tao->ops->computejacobianinequality=0;
  tao->ops->computeequalityconstraints=0;
  tao->ops->computeinequalityconstraints=0;
  tao->ops->convergencetest=TaoDefaultConvergenceTest;
  tao->ops->convergencedestroy=0;
  tao->ops->computedual=0;
  tao->ops->setup=0;
  tao->ops->solve=0;
  tao->ops->view=0;
  tao->ops->setfromoptions=0;
  tao->ops->destroy=0;

  tao->solution=NULL;
  tao->gradient=NULL;
  tao->sep_objective = NULL;
  tao->constraints=NULL;
  tao->constraints_equality=NULL;
  tao->constraints_inequality=NULL;
  tao->stepdirection=NULL;
  tao->niter=0;
  tao->ntotalits=0;
  tao->XL = NULL;
  tao->XU = NULL;
  tao->IL = NULL;
  tao->IU = NULL;
  tao->DI = NULL;
  tao->DE = NULL;
  tao->gradient_norm = NULL;
  tao->gradient_norm_tmp = NULL;
  tao->hessian = NULL;
  tao->hessian_pre = NULL;
  tao->jacobian = NULL;
  tao->jacobian_pre = NULL;
  tao->jacobian_state = NULL;
  tao->jacobian_state_pre = NULL;
  tao->jacobian_state_inv = NULL;
  tao->jacobian_design = NULL;
  tao->jacobian_design_pre = NULL;
  tao->jacobian_equality = NULL;
  tao->jacobian_equality_pre = NULL;
  tao->jacobian_inequality = NULL;
  tao->jacobian_inequality_pre = NULL;
  tao->state_is = NULL;
  tao->design_is = NULL;

  tao->max_it     = 10000;
  tao->max_funcs   = 10000;
#if defined(PETSC_USE_REAL_SINGLE)
  tao->gatol       = 1e-5;
  tao->grtol       = 1e-5;
#else
  tao->gatol       = 1e-8;
  tao->grtol       = 1e-8;
#endif
  tao->crtol       = 0.0;
  tao->catol       = 0.0;
  tao->steptol     = 0.0;
  tao->gttol       = 0.0;
  tao->trust0      = PETSC_INFINITY;
  tao->fmin        = PETSC_NINFINITY;
  tao->hist_malloc = PETSC_FALSE;
  tao->hist_reset = PETSC_TRUE;
  tao->hist_max = 0;
  tao->hist_len = 0;
  tao->hist_obj = NULL;
  tao->hist_resid = NULL;
  tao->hist_cnorm = NULL;
  tao->hist_lits = NULL;

  tao->numbermonitors=0;
  tao->viewsolution=PETSC_FALSE;
  tao->viewhessian=PETSC_FALSE;
  tao->viewgradient=PETSC_FALSE;
  tao->viewjacobian=PETSC_FALSE;
  tao->viewconstraints = PETSC_FALSE;

  /* These flags prevents algorithms from overriding user options */
  tao->max_it_changed   =PETSC_FALSE;
  tao->max_funcs_changed=PETSC_FALSE;
  tao->gatol_changed    =PETSC_FALSE;
  tao->grtol_changed    =PETSC_FALSE;
  tao->gttol_changed    =PETSC_FALSE;
  tao->steptol_changed  =PETSC_FALSE;
  tao->trust0_changed   =PETSC_FALSE;
  tao->fmin_changed     =PETSC_FALSE;
  tao->catol_changed    =PETSC_FALSE;
  tao->crtol_changed    =PETSC_FALSE;
  ierr = TaoResetStatistics(tao);CHKERRQ(ierr);
  *newtao = tao;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoSolve"
/*@
  TaoSolve - Solves an optimization problem min F(x) s.t. l <= x <= u

  Collective on Tao

  Input Parameters:
. tao - the Tao context

  Notes:
  The user must set up the Tao with calls to TaoSetInitialVector(),
  TaoSetObjectiveRoutine(),
  TaoSetGradientRoutine(), and (if using 2nd order method) TaoSetHessianRoutine().

  Level: beginner

.seealso: TaoCreate(), TaoSetObjectiveRoutine(), TaoSetGradientRoutine(), TaoSetHessianRoutine()
 @*/
PetscErrorCode TaoSolve(Tao tao)
{
  PetscErrorCode   ierr;
  static PetscBool set = PETSC_FALSE;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(tao,TAO_CLASSID,1);
  ierr = PetscCitationsRegister("@TechReport{tao-user-ref,\n"
                                "title   = {Toolkit for Advanced Optimization (TAO) Users Manual},\n"
                                "author  = {Todd Munson and Jason Sarich and Stefan Wild and Steve Benson and Lois Curfman McInnes},\n"
                                "Institution = {Argonne National Laboratory},\n"
                                "Year   = 2014,\n"
                                "Number = {ANL/MCS-TM-322 - Revision 3.5},\n"
                                "url    = {http://www.mcs.anl.gov/tao}\n}\n",&set);CHKERRQ(ierr);

  ierr = TaoSetUp(tao);CHKERRQ(ierr);
  ierr = TaoResetStatistics(tao);CHKERRQ(ierr);
  if (tao->linesearch) {
    ierr = TaoLineSearchReset(tao->linesearch);CHKERRQ(ierr);
  }

  ierr = PetscLogEventBegin(Tao_Solve,tao,0,0,0);CHKERRQ(ierr);
  if (tao->ops->solve){ ierr = (*tao->ops->solve)(tao);CHKERRQ(ierr); }
  ierr = PetscLogEventEnd(Tao_Solve,tao,0,0,0);CHKERRQ(ierr);

  tao->ntotalits += tao->niter;
  ierr = TaoViewFromOptions(tao,NULL,"-tao_view");CHKERRQ(ierr);

  if (tao->printreason) {
    if (tao->reason > 0) {
      ierr = PetscPrintf(((PetscObject)tao)->comm,"TAO solve converged due to %s iterations %D\n",TaoConvergedReasons[tao->reason],tao->niter);CHKERRQ(ierr);
    } else {
      ierr = PetscPrintf(((PetscObject)tao)->comm,"TAO solve did not converge due to %s iteration %D\n",TaoConvergedReasons[tao->reason],tao->niter);CHKERRQ(ierr);
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoSetUp"
/*@
  TaoSetUp - Sets up the internal data structures for the later use
  of a Tao solver

  Collective on tao

  Input Parameters:
. tao - the TAO context

  Notes:
  The user will not need to explicitly call TaoSetUp(), as it will
  automatically be called in TaoSolve().  However, if the user
  desires to call it explicitly, it should come after TaoCreate()
  and any TaoSetSomething() routines, but before TaoSolve().

  Level: advanced

.seealso: TaoCreate(), TaoSolve()
@*/
PetscErrorCode TaoSetUp(Tao tao)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(tao, TAO_CLASSID,1);
  if (tao->setupcalled) PetscFunctionReturn(0);

  if (!tao->solution) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONGSTATE,"Must call TaoSetInitialVector");
  if (tao->ops->setup) {
    ierr = (*tao->ops->setup)(tao);CHKERRQ(ierr);
  }
  tao->setupcalled = PETSC_TRUE;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoDestroy"
/*@
  TaoDestroy - Destroys the TAO context that was created with
  TaoCreate()

  Collective on Tao

  Input Parameter:
. tao - the Tao context

  Level: beginner

.seealso: TaoCreate(), TaoSolve()
@*/
PetscErrorCode TaoDestroy(Tao *tao)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (!*tao) PetscFunctionReturn(0);
  PetscValidHeaderSpecific(*tao,TAO_CLASSID,1);
  if (--((PetscObject)*tao)->refct > 0) {*tao=0;PetscFunctionReturn(0);}

  if ((*tao)->ops->destroy) {
    ierr = (*((*tao))->ops->destroy)(*tao);CHKERRQ(ierr);
  }
  ierr = KSPDestroy(&(*tao)->ksp);CHKERRQ(ierr);
  ierr = TaoLineSearchDestroy(&(*tao)->linesearch);CHKERRQ(ierr);

  if ((*tao)->ops->convergencedestroy) {
    ierr = (*(*tao)->ops->convergencedestroy)((*tao)->cnvP);CHKERRQ(ierr);
    if ((*tao)->jacobian_state_inv) {
      ierr = MatDestroy(&(*tao)->jacobian_state_inv);CHKERRQ(ierr);
    }
  }
  ierr = VecDestroy(&(*tao)->solution);CHKERRQ(ierr);
  ierr = VecDestroy(&(*tao)->gradient);CHKERRQ(ierr);

  if ((*tao)->gradient_norm) {
    ierr = PetscObjectDereference((PetscObject)(*tao)->gradient_norm);CHKERRQ(ierr);
    ierr = VecDestroy(&(*tao)->gradient_norm_tmp);CHKERRQ(ierr);
  }

  ierr = VecDestroy(&(*tao)->XL);CHKERRQ(ierr);
  ierr = VecDestroy(&(*tao)->XU);CHKERRQ(ierr);
  ierr = VecDestroy(&(*tao)->IL);CHKERRQ(ierr);
  ierr = VecDestroy(&(*tao)->IU);CHKERRQ(ierr);
  ierr = VecDestroy(&(*tao)->DE);CHKERRQ(ierr);
  ierr = VecDestroy(&(*tao)->DI);CHKERRQ(ierr);
  ierr = VecDestroy(&(*tao)->constraints_equality);CHKERRQ(ierr);
  ierr = VecDestroy(&(*tao)->constraints_inequality);CHKERRQ(ierr);
  ierr = VecDestroy(&(*tao)->stepdirection);CHKERRQ(ierr);
  ierr = MatDestroy(&(*tao)->hessian_pre);CHKERRQ(ierr);
  ierr = MatDestroy(&(*tao)->hessian);CHKERRQ(ierr);
  ierr = MatDestroy(&(*tao)->jacobian_pre);CHKERRQ(ierr);
  ierr = MatDestroy(&(*tao)->jacobian);CHKERRQ(ierr);
  ierr = MatDestroy(&(*tao)->jacobian_state_pre);CHKERRQ(ierr);
  ierr = MatDestroy(&(*tao)->jacobian_state);CHKERRQ(ierr);
  ierr = MatDestroy(&(*tao)->jacobian_state_inv);CHKERRQ(ierr);
  ierr = MatDestroy(&(*tao)->jacobian_design);CHKERRQ(ierr);
  ierr = MatDestroy(&(*tao)->jacobian_equality);CHKERRQ(ierr);
  ierr = MatDestroy(&(*tao)->jacobian_equality_pre);CHKERRQ(ierr);
  ierr = MatDestroy(&(*tao)->jacobian_inequality);CHKERRQ(ierr);
  ierr = MatDestroy(&(*tao)->jacobian_inequality_pre);CHKERRQ(ierr);
  ierr = ISDestroy(&(*tao)->state_is);CHKERRQ(ierr);
  ierr = ISDestroy(&(*tao)->design_is);CHKERRQ(ierr);
  ierr = TaoCancelMonitors(*tao);CHKERRQ(ierr);
  if ((*tao)->hist_malloc) {
    ierr = PetscFree((*tao)->hist_obj);CHKERRQ(ierr);
    ierr = PetscFree((*tao)->hist_resid);CHKERRQ(ierr);
    ierr = PetscFree((*tao)->hist_cnorm);CHKERRQ(ierr);
    ierr = PetscFree((*tao)->hist_lits);CHKERRQ(ierr);
  }
  ierr = PetscHeaderDestroy(tao);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoSetFromOptions"
/*@
  TaoSetFromOptions - Sets various Tao parameters from user
  options.

  Collective on Tao

  Input Paremeter:
. tao - the Tao solver context

  options Database Keys:
+ -tao_type <type> - The algorithm that TAO uses (lmvm, nls, etc.)
. -tao_gatol <gatol> - absolute error tolerance for ||gradient||
. -tao_grtol <grtol> - relative error tolerance for ||gradient||
. -tao_gttol <gttol> - reduction of ||gradient|| relative to initial gradient
. -tao_max_it <max> - sets maximum number of iterations
. -tao_max_funcs <max> - sets maximum number of function evaluations
. -tao_fmin <fmin> - stop if function value reaches fmin
. -tao_steptol <tol> - stop if trust region radius less than <tol>
. -tao_trust0 <t> - initial trust region radius
. -tao_monitor - prints function value and residual at each iteration
. -tao_smonitor - same as tao_monitor, but truncates very small values
. -tao_cmonitor - prints function value, residual, and constraint norm at each iteration
. -tao_view_solution - prints solution vector at each iteration
. -tao_view_separableobjective - prints separable objective vector at each iteration
. -tao_view_step - prints step direction vector at each iteration
. -tao_view_gradient - prints gradient vector at each iteration
. -tao_draw_solution - graphically view solution vector at each iteration
. -tao_draw_step - graphically view step vector at each iteration
. -tao_draw_gradient - graphically view gradient at each iteration
. -tao_fd_gradient - use gradient computed with finite differences
. -tao_cancelmonitors - cancels all monitors (except those set with command line)
. -tao_view - prints information about the Tao after solving
- -tao_converged_reason - prints the reason TAO stopped iterating

  Notes:
  To see all options, run your program with the -help option or consult the
  user's manual. Should be called after TaoCreate() but before TaoSolve()

  Level: beginner
@*/
PetscErrorCode TaoSetFromOptions(Tao tao)
{
  PetscErrorCode ierr;
  const TaoType  default_type = TAOLMVM;
  char           type[256], monfilename[PETSC_MAX_PATH_LEN];
  PetscViewer    monviewer;
  PetscBool      flg;
  MPI_Comm       comm;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(tao,TAO_CLASSID,1);
  ierr = PetscObjectGetComm((PetscObject)tao,&comm);CHKERRQ(ierr);
  /* So no warnings are given about unused options */
  ierr = PetscOptionsHasName(((PetscObject)tao)->options,((PetscObject)tao)->prefix,"-tao_ls_type",&flg);CHKERRQ(ierr);

  ierr = PetscObjectOptionsBegin((PetscObject)tao);CHKERRQ(ierr);
  {
    ierr = TaoRegisterAll();CHKERRQ(ierr);
    if (((PetscObject)tao)->type_name) {
      default_type = ((PetscObject)tao)->type_name;
    }
    /* Check for type from options */
    ierr = PetscOptionsFList("-tao_type","Tao Solver type","TaoSetType",TaoList,default_type,type,256,&flg);CHKERRQ(ierr);
    if (flg) {
      ierr = TaoSetType(tao,type);CHKERRQ(ierr);
    } else if (!((PetscObject)tao)->type_name) {
      ierr = TaoSetType(tao,default_type);CHKERRQ(ierr);
    }

    ierr = PetscOptionsReal("-tao_catol","Stop if constraints violations within","TaoSetConstraintTolerances",tao->catol,&tao->catol,&flg);CHKERRQ(ierr);
    if (flg) tao->catol_changed=PETSC_TRUE;
    ierr = PetscOptionsReal("-tao_crtol","Stop if relative contraint violations within","TaoSetConstraintTolerances",tao->crtol,&tao->crtol,&flg);CHKERRQ(ierr);
    if (flg) tao->crtol_changed=PETSC_TRUE;
    ierr = PetscOptionsReal("-tao_gatol","Stop if norm of gradient less than","TaoSetTolerances",tao->gatol,&tao->gatol,&flg);CHKERRQ(ierr);
    if (flg) tao->gatol_changed=PETSC_TRUE;
    ierr = PetscOptionsReal("-tao_grtol","Stop if norm of gradient divided by the function value is less than","TaoSetTolerances",tao->grtol,&tao->grtol,&flg);CHKERRQ(ierr);
    if (flg) tao->grtol_changed=PETSC_TRUE;
    ierr = PetscOptionsReal("-tao_gttol","Stop if the norm of the gradient is less than the norm of the initial gradient times tol","TaoSetTolerances",tao->gttol,&tao->gttol,&flg);CHKERRQ(ierr);
    if (flg) tao->gttol_changed=PETSC_TRUE;
    ierr = PetscOptionsInt("-tao_max_it","Stop if iteration number exceeds","TaoSetMaximumIterations",tao->max_it,&tao->max_it,&flg);CHKERRQ(ierr);
    if (flg) tao->max_it_changed=PETSC_TRUE;
    ierr = PetscOptionsInt("-tao_max_funcs","Stop if number of function evaluations exceeds","TaoSetMaximumFunctionEvaluations",tao->max_funcs,&tao->max_funcs,&flg);CHKERRQ(ierr);
    if (flg) tao->max_funcs_changed=PETSC_TRUE;
    ierr = PetscOptionsReal("-tao_fmin","Stop if function less than","TaoSetFunctionLowerBound",tao->fmin,&tao->fmin,&flg);CHKERRQ(ierr);
    if (flg) tao->fmin_changed=PETSC_TRUE;
    ierr = PetscOptionsReal("-tao_steptol","Stop if step size or trust region radius less than","",tao->steptol,&tao->steptol,&flg);CHKERRQ(ierr);
    if (flg) tao->steptol_changed=PETSC_TRUE;
    ierr = PetscOptionsReal("-tao_trust0","Initial trust region radius","TaoSetTrustRegionRadius",tao->trust0,&tao->trust0,&flg);CHKERRQ(ierr);
    if (flg) tao->trust0_changed=PETSC_TRUE;
    ierr = PetscOptionsString("-tao_view_solution","view solution vector after each evaluation","TaoSetMonitor","stdout",monfilename,PETSC_MAX_PATH_LEN,&flg);CHKERRQ(ierr);
    if (flg) {
      ierr = PetscViewerASCIIOpen(comm,monfilename,&monviewer);CHKERRQ(ierr);
      ierr = TaoSetMonitor(tao,TaoSolutionMonitor,monviewer,(PetscErrorCode (*)(void**))PetscViewerDestroy);CHKERRQ(ierr);
    }

    ierr = PetscOptionsBool("-tao_converged_reason","Print reason for TAO converged","TaoSolve",tao->printreason,&tao->printreason,NULL);CHKERRQ(ierr);
    ierr = PetscOptionsString("-tao_view_gradient","view gradient vector after each evaluation","TaoSetMonitor","stdout",monfilename,PETSC_MAX_PATH_LEN,&flg);CHKERRQ(ierr);
    if (flg) {
      ierr = PetscViewerASCIIOpen(comm,monfilename,&monviewer);CHKERRQ(ierr);
      ierr = TaoSetMonitor(tao,TaoGradientMonitor,monviewer,(PetscErrorCode (*)(void**))PetscViewerDestroy);CHKERRQ(ierr);
    }

    ierr = PetscOptionsString("-tao_view_stepdirection","view step direction vector after each iteration","TaoSetMonitor","stdout",monfilename,PETSC_MAX_PATH_LEN,&flg);CHKERRQ(ierr);
    if (flg) {
      ierr = PetscViewerASCIIOpen(comm,monfilename,&monviewer);CHKERRQ(ierr);
      ierr = TaoSetMonitor(tao,TaoStepDirectionMonitor,monviewer,(PetscErrorCode (*)(void**))PetscViewerDestroy);CHKERRQ(ierr);
    }

    ierr = PetscOptionsString("-tao_view_separableobjective","view separable objective vector after each evaluation","TaoSetMonitor","stdout",monfilename,PETSC_MAX_PATH_LEN,&flg);CHKERRQ(ierr);
    if (flg) {
      ierr = PetscViewerASCIIOpen(comm,monfilename,&monviewer);CHKERRQ(ierr);
      ierr = TaoSetMonitor(tao,TaoSeparableObjectiveMonitor,monviewer,(PetscErrorCode (*)(void**))PetscViewerDestroy);CHKERRQ(ierr);
    }

    ierr = PetscOptionsString("-tao_monitor","Use the default convergence monitor","TaoSetMonitor","stdout",monfilename,PETSC_MAX_PATH_LEN,&flg);CHKERRQ(ierr);
    if (flg) {
      ierr = PetscViewerASCIIOpen(comm,monfilename,&monviewer);CHKERRQ(ierr);
      ierr = TaoSetMonitor(tao,TaoDefaultMonitor,monviewer,(PetscErrorCode (*)(void**))PetscViewerDestroy);CHKERRQ(ierr);
    }

    ierr = PetscOptionsString("-tao_smonitor","Use the short convergence monitor","TaoSetMonitor","stdout",monfilename,PETSC_MAX_PATH_LEN,&flg);CHKERRQ(ierr);
    if (flg) {
      ierr = PetscViewerASCIIOpen(comm,monfilename,&monviewer);CHKERRQ(ierr);
      ierr = TaoSetMonitor(tao,TaoDefaultSMonitor,monviewer,(PetscErrorCode (*)(void**))PetscViewerDestroy);CHKERRQ(ierr);
    }

    ierr = PetscOptionsString("-tao_cmonitor","Use the default convergence monitor with constraint norm","TaoSetMonitor","stdout",monfilename,PETSC_MAX_PATH_LEN,&flg);CHKERRQ(ierr);
    if (flg) {
      ierr = PetscViewerASCIIOpen(comm,monfilename,&monviewer);CHKERRQ(ierr);
      ierr = TaoSetMonitor(tao,TaoDefaultCMonitor,monviewer,(PetscErrorCode (*)(void**))PetscViewerDestroy);CHKERRQ(ierr);
    }


    flg = PETSC_FALSE;
    ierr = PetscOptionsBool("-tao_cancelmonitors","cancel all monitors and call any registered destroy routines","TaoCancelMonitors",flg,&flg,NULL);CHKERRQ(ierr);
    if (flg) {ierr = TaoCancelMonitors(tao);CHKERRQ(ierr);}

    flg = PETSC_FALSE;
    ierr = PetscOptionsBool("-tao_draw_solution","Plot solution vector at each iteration","TaoSetMonitor",flg,&flg,NULL);CHKERRQ(ierr);
    if (flg) {
      ierr = TaoSetMonitor(tao,TaoDrawSolutionMonitor,NULL,NULL);CHKERRQ(ierr);
    }

    flg = PETSC_FALSE;
    ierr = PetscOptionsBool("-tao_draw_step","plots step direction at each iteration","TaoSetMonitor",flg,&flg,NULL);CHKERRQ(ierr);
    if (flg) {
      ierr = TaoSetMonitor(tao,TaoDrawStepMonitor,NULL,NULL);CHKERRQ(ierr);
    }

    flg = PETSC_FALSE;
    ierr = PetscOptionsBool("-tao_draw_gradient","plots gradient at each iteration","TaoSetMonitor",flg,&flg,NULL);CHKERRQ(ierr);
    if (flg) {
      ierr = TaoSetMonitor(tao,TaoDrawGradientMonitor,NULL,NULL);CHKERRQ(ierr);
    }
    flg = PETSC_FALSE;
    ierr = PetscOptionsBool("-tao_fd_gradient","compute gradient using finite differences","TaoDefaultComputeGradient",flg,&flg,NULL);CHKERRQ(ierr);
    if (flg) {
      ierr = TaoSetGradientRoutine(tao,TaoDefaultComputeGradient,NULL);CHKERRQ(ierr);
    }
    ierr = PetscOptionsEnum("-tao_subset_type","subset type", "", TaoSubSetTypes,(PetscEnum)tao->subset_type, (PetscEnum*)&tao->subset_type, 0);CHKERRQ(ierr);

    if (tao->ops->setfromoptions) {
      ierr = (*tao->ops->setfromoptions)(PetscOptionsObject,tao);CHKERRQ(ierr);
    }
  }
  ierr = PetscOptionsEnd();CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoView"
/*@C
  TaoView - Prints information about the Tao

  Collective on Tao

  InputParameters:
+ tao - the Tao context
- viewer - visualization context

  Options Database Key:
. -tao_view - Calls TaoView() at the end of TaoSolve()

  Notes:
  The available visualization contexts include
+     PETSC_VIEWER_STDOUT_SELF - standard output (default)
-     PETSC_VIEWER_STDOUT_WORLD - synchronized standard
         output where only the first processor opens
         the file.  All other processors send their
         data to the first processor to print.

  Level: beginner

.seealso: PetscViewerASCIIOpen()
@*/
PetscErrorCode TaoView(Tao tao, PetscViewer viewer)
{
  PetscErrorCode      ierr;
  PetscBool           isascii,isstring;
  const TaoType type;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(tao,TAO_CLASSID,1);
  if (!viewer) {
    ierr = PetscViewerASCIIGetStdout(((PetscObject)tao)->comm,&viewer);CHKERRQ(ierr);
  }
  PetscValidHeaderSpecific(viewer,PETSC_VIEWER_CLASSID,2);
  PetscCheckSameComm(tao,1,viewer,2);

  ierr = PetscObjectTypeCompare((PetscObject)viewer,PETSCVIEWERASCII,&isascii);CHKERRQ(ierr);
  ierr = PetscObjectTypeCompare((PetscObject)viewer,PETSCVIEWERSTRING,&isstring);CHKERRQ(ierr);
  if (isascii) {
    ierr = PetscObjectPrintClassNamePrefixType((PetscObject)tao,viewer);CHKERRQ(ierr);
    ierr = PetscViewerASCIIPushTab(viewer);CHKERRQ(ierr);

    if (tao->ops->view) {
      ierr = PetscViewerASCIIPushTab(viewer);CHKERRQ(ierr);
      ierr = (*tao->ops->view)(tao,viewer);CHKERRQ(ierr);
      ierr = PetscViewerASCIIPopTab(viewer);CHKERRQ(ierr);
    }
    if (tao->linesearch) {
      ierr = PetscObjectPrintClassNamePrefixType((PetscObject)(tao->linesearch),viewer);CHKERRQ(ierr);
    }
    if (tao->ksp) {
      ierr = PetscObjectPrintClassNamePrefixType((PetscObject)(tao->ksp),viewer);CHKERRQ(ierr);
      ierr = PetscViewerASCIIPrintf(viewer,"total KSP iterations: %D\n",tao->ksp_tot_its);CHKERRQ(ierr);
    }
    if (tao->XL || tao->XU) {
      ierr = PetscViewerASCIIPrintf(viewer,"Active Set subset type: %s\n",TaoSubSetTypes[tao->subset_type]);CHKERRQ(ierr);
    }

    ierr=PetscViewerASCIIPrintf(viewer,"convergence tolerances: gatol=%g,",(double)tao->gatol);CHKERRQ(ierr);
    ierr=PetscViewerASCIIPrintf(viewer," steptol=%g,",(double)tao->steptol);CHKERRQ(ierr);
    ierr=PetscViewerASCIIPrintf(viewer," gttol=%g\n",(double)tao->gttol);CHKERRQ(ierr);

    ierr = PetscViewerASCIIPrintf(viewer,"Residual in Function/Gradient:=%g\n",(double)tao->residual);CHKERRQ(ierr);

    if (tao->cnorm>0 || tao->catol>0 || tao->crtol>0){
      ierr=PetscViewerASCIIPrintf(viewer,"convergence tolerances:");CHKERRQ(ierr);
      ierr=PetscViewerASCIIPrintf(viewer," catol=%g,",(double)tao->catol);CHKERRQ(ierr);
      ierr=PetscViewerASCIIPrintf(viewer," crtol=%g\n",(double)tao->crtol);CHKERRQ(ierr);
      ierr = PetscViewerASCIIPrintf(viewer,"Residual in Constraints:=%g\n",(double)tao->cnorm);CHKERRQ(ierr);
    }

    if (tao->trust < tao->steptol){
      ierr=PetscViewerASCIIPrintf(viewer,"convergence tolerances: steptol=%g\n",(double)tao->steptol);CHKERRQ(ierr);
      ierr=PetscViewerASCIIPrintf(viewer,"Final trust region radius:=%g\n",(double)tao->trust);CHKERRQ(ierr);
    }

    if (tao->fmin>-1.e25){
      ierr=PetscViewerASCIIPrintf(viewer,"convergence tolerances: function minimum=%g\n",(double)tao->fmin);CHKERRQ(ierr);
    }
    ierr = PetscViewerASCIIPrintf(viewer,"Objective value=%g\n",(double)tao->fc);CHKERRQ(ierr);

    ierr = PetscViewerASCIIPrintf(viewer,"total number of iterations=%D,          ",tao->niter);CHKERRQ(ierr);
    ierr = PetscViewerASCIIPrintf(viewer,"              (max: %D)\n",tao->max_it);CHKERRQ(ierr);

    if (tao->nfuncs>0){
      ierr = PetscViewerASCIIPrintf(viewer,"total number of function evaluations=%D,",tao->nfuncs);CHKERRQ(ierr);
      ierr = PetscViewerASCIIPrintf(viewer,"                max: %D\n",tao->max_funcs);CHKERRQ(ierr);
    }
    if (tao->ngrads>0){
      ierr = PetscViewerASCIIPrintf(viewer,"total number of gradient evaluations=%D,",tao->ngrads);CHKERRQ(ierr);
      ierr = PetscViewerASCIIPrintf(viewer,"                max: %D\n",tao->max_funcs);CHKERRQ(ierr);
    }
    if (tao->nfuncgrads>0){
      ierr = PetscViewerASCIIPrintf(viewer,"total number of function/gradient evaluations=%D,",tao->nfuncgrads);CHKERRQ(ierr);
      ierr = PetscViewerASCIIPrintf(viewer,"    (max: %D)\n",tao->max_funcs);CHKERRQ(ierr);
    }
    if (tao->nhess>0){
      ierr = PetscViewerASCIIPrintf(viewer,"total number of Hessian evaluations=%D\n",tao->nhess);CHKERRQ(ierr);
    }
    /*  if (tao->linear_its>0){
     ierr = PetscViewerASCIIPrintf(viewer,"  total Krylov method iterations=%D\n",tao->linear_its);CHKERRQ(ierr);
     }*/
    if (tao->nconstraints>0){
      ierr = PetscViewerASCIIPrintf(viewer,"total number of constraint function evaluations=%D\n",tao->nconstraints);CHKERRQ(ierr);
    }
    if (tao->njac>0){
      ierr = PetscViewerASCIIPrintf(viewer,"total number of Jacobian evaluations=%D\n",tao->njac);CHKERRQ(ierr);
    }

    if (tao->reason>0){
      ierr = PetscViewerASCIIPrintf(viewer,    "Solution converged: ");CHKERRQ(ierr);
      switch (tao->reason) {
      case TAO_CONVERGED_GATOL:
        ierr = PetscViewerASCIIPrintf(viewer," ||g(X)|| <= gatol\n");CHKERRQ(ierr);
        break;
      case TAO_CONVERGED_GRTOL:
        ierr = PetscViewerASCIIPrintf(viewer," ||g(X)||/|f(X)| <= grtol\n");CHKERRQ(ierr);
        break;
      case TAO_CONVERGED_GTTOL:
        ierr = PetscViewerASCIIPrintf(viewer," ||g(X)||/||g(X0)|| <= gttol\n");CHKERRQ(ierr);
        break;
      case TAO_CONVERGED_STEPTOL:
        ierr = PetscViewerASCIIPrintf(viewer," Steptol -- step size small\n");CHKERRQ(ierr);
        break;
      case TAO_CONVERGED_MINF:
        ierr = PetscViewerASCIIPrintf(viewer," Minf --  f < fmin\n");CHKERRQ(ierr);
        break;
      case TAO_CONVERGED_USER:
        ierr = PetscViewerASCIIPrintf(viewer," User Terminated\n");CHKERRQ(ierr);
        break;
      default:
        ierr = PetscViewerASCIIPrintf(viewer,"\n");CHKERRQ(ierr);
        break;
      }

    } else {
      ierr = PetscViewerASCIIPrintf(viewer,"Solver terminated: %d",tao->reason);CHKERRQ(ierr);
      switch (tao->reason) {
      case TAO_DIVERGED_MAXITS:
        ierr = PetscViewerASCIIPrintf(viewer," Maximum Iterations\n");CHKERRQ(ierr);
        break;
      case TAO_DIVERGED_NAN:
        ierr = PetscViewerASCIIPrintf(viewer," NAN or Inf encountered\n");CHKERRQ(ierr);
        break;
      case TAO_DIVERGED_MAXFCN:
        ierr = PetscViewerASCIIPrintf(viewer," Maximum Function Evaluations\n");CHKERRQ(ierr);
        break;
      case TAO_DIVERGED_LS_FAILURE:
        ierr = PetscViewerASCIIPrintf(viewer," Line Search Failure\n");CHKERRQ(ierr);
        break;
      case TAO_DIVERGED_TR_REDUCTION:
        ierr = PetscViewerASCIIPrintf(viewer," Trust Region too small\n");CHKERRQ(ierr);
        break;
      case TAO_DIVERGED_USER:
        ierr = PetscViewerASCIIPrintf(viewer," User Terminated\n");CHKERRQ(ierr);
        break;
      default:
        ierr = PetscViewerASCIIPrintf(viewer,"\n");CHKERRQ(ierr);
        break;
      }
    }
    ierr = PetscViewerASCIIPopTab(viewer);CHKERRQ(ierr);
  } else if (isstring) {
    ierr = TaoGetType(tao,&type);CHKERRQ(ierr);
    ierr = PetscViewerStringSPrintf(viewer," %-3.3s",type);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoSetTolerances"
/*@
  TaoSetTolerances - Sets parameters used in TAO convergence tests

  Logically collective on Tao

  Input Parameters:
+ tao - the Tao context
. gatol - stop if norm of gradient is less than this
. grtol - stop if relative norm of gradient is less than this
- gttol - stop if norm of gradient is reduced by this factor

  Options Database Keys:
+ -tao_gatol <gatol> - Sets gatol
. -tao_grtol <grtol> - Sets grtol
- -tao_gttol <gttol> - Sets gttol

  Stopping Criteria:
$ ||g(X)||                            <= gatol
$ ||g(X)|| / |f(X)|                   <= grtol
$ ||g(X)|| / ||g(X0)||                <= gttol

  Notes:
  Use PETSC_DEFAULT to leave one or more tolerances unchanged.

  Level: beginner

.seealso: TaoGetTolerances()

@*/
PetscErrorCode TaoSetTolerances(Tao tao, PetscReal gatol, PetscReal grtol, PetscReal gttol)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(tao,TAO_CLASSID,1);

  if (gatol != PETSC_DEFAULT) {
    if (gatol<0) {
      ierr = PetscInfo(tao,"Tried to set negative gatol -- ignored.\n");CHKERRQ(ierr);
    } else {
      tao->gatol = PetscMax(0,gatol);
      tao->gatol_changed=PETSC_TRUE;
    }
  }

  if (grtol != PETSC_DEFAULT) {
    if (grtol<0) {
      ierr = PetscInfo(tao,"Tried to set negative grtol -- ignored.\n");CHKERRQ(ierr);
    } else {
      tao->grtol = PetscMax(0,grtol);
      tao->grtol_changed=PETSC_TRUE;
    }
  }

  if (gttol != PETSC_DEFAULT) {
    if (gttol<0) {
      ierr = PetscInfo(tao,"Tried to set negative gttol -- ignored.\n");CHKERRQ(ierr);
    } else {
      tao->gttol = PetscMax(0,gttol);
      tao->gttol_changed=PETSC_TRUE;
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoSetConstraintTolerances"
/*@
  TaoSetConstraintTolerances - Sets constraint tolerance parameters used in TAO  convergence tests

  Logically collective on Tao

  Input Parameters:
+ tao - the Tao context
. catol - absolute constraint tolerance, constraint norm must be less than catol for used for gatol convergence criteria
- crtol - relative contraint tolerance, constraint norm must be less than crtol for used for gatol, gttol convergence criteria

  Options Database Keys:
+ -tao_catol <catol> - Sets catol
- -tao_crtol <crtol> - Sets crtol

  Notes:
  Use PETSC_DEFAULT to leave any tolerance unchanged.

  Level: intermediate

.seealso: TaoGetTolerances(), TaoGetConstraintTolerances(), TaoSetTolerances()

@*/
PetscErrorCode TaoSetConstraintTolerances(Tao tao, PetscReal catol, PetscReal crtol)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(tao,TAO_CLASSID,1);

  if (catol != PETSC_DEFAULT) {
    if (catol<0) {
      ierr = PetscInfo(tao,"Tried to set negative catol -- ignored.\n");CHKERRQ(ierr);
    } else {
      tao->catol = PetscMax(0,catol);
      tao->catol_changed=PETSC_TRUE;
    }
  }

  if (crtol != PETSC_DEFAULT) {
    if (crtol<0) {
      ierr = PetscInfo(tao,"Tried to set negative crtol -- ignored.\n");CHKERRQ(ierr);
    } else {
      tao->crtol = PetscMax(0,crtol);
      tao->crtol_changed=PETSC_TRUE;
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoGetConstraintTolerances"
/*@
  TaoGetConstraintTolerances - Gets constraint tolerance parameters used in TAO  convergence tests

  Not ollective

  Input Parameter:
. tao - the Tao context

  Output Parameter:
+ catol - absolute constraint tolerance, constraint norm must be less than catol for used for gatol convergence criteria
- crtol - relative contraint tolerance, constraint norm must be less than crtol for used for gatol, gttol convergence criteria

  Level: intermediate

.seealso: TaoGetTolerances(), TaoSetTolerances(), TaoSetConstraintTolerances()

@*/
PetscErrorCode TaoGetConstraintTolerances(Tao tao, PetscReal *catol, PetscReal *crtol)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(tao,TAO_CLASSID,1);
  if (catol) *catol = tao->catol;
  if (crtol) *crtol = tao->crtol;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoSetFunctionLowerBound"
/*@
   TaoSetFunctionLowerBound - Sets a bound on the solution objective value.
   When an approximate solution with an objective value below this number
   has been found, the solver will terminate.

   Logically Collective on Tao

   Input Parameters:
+  tao - the Tao solver context
-  fmin - the tolerance

   Options Database Keys:
.    -tao_fmin <fmin> - sets the minimum function value

   Level: intermediate

.seealso: TaoSetTolerances()
@*/
PetscErrorCode TaoSetFunctionLowerBound(Tao tao,PetscReal fmin)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(tao,TAO_CLASSID,1);
  tao->fmin = fmin;
  tao->fmin_changed=PETSC_TRUE;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoGetFunctionLowerBound"
/*@
   TaoGetFunctionLowerBound - Gets the bound on the solution objective value.
   When an approximate solution with an objective value below this number
   has been found, the solver will terminate.

   Not collective on Tao

   Input Parameters:
.  tao - the Tao solver context

   OutputParameters:
.  fmin - the minimum function value

   Level: intermediate

.seealso: TaoSetFunctionLowerBound()
@*/
PetscErrorCode TaoGetFunctionLowerBound(Tao tao,PetscReal *fmin)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(tao,TAO_CLASSID,1);
  *fmin = tao->fmin;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoSetMaximumFunctionEvaluations"
/*@
   TaoSetMaximumFunctionEvaluations - Sets a maximum number of
   function evaluations.

   Logically Collective on Tao

   Input Parameters:
+  tao - the Tao solver context
-  nfcn - the maximum number of function evaluations (>=0)

   Options Database Keys:
.    -tao_max_funcs <nfcn> - sets the maximum number of function evaluations

   Level: intermediate

.seealso: TaoSetTolerances(), TaoSetMaximumIterations()
@*/

PetscErrorCode TaoSetMaximumFunctionEvaluations(Tao tao,PetscInt nfcn)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(tao,TAO_CLASSID,1);
  tao->max_funcs = PetscMax(0,nfcn);
  tao->max_funcs_changed=PETSC_TRUE;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoGetMaximumFunctionEvaluations"
/*@
   TaoGetMaximumFunctionEvaluations - Sets a maximum number of
   function evaluations.

   Not Collective

   Input Parameters:
.  tao - the Tao solver context

   Output Parameters:
.  nfcn - the maximum number of function evaluations

   Level: intermediate

.seealso: TaoSetMaximumFunctionEvaluations(), TaoGetMaximumIterations()
@*/

PetscErrorCode TaoGetMaximumFunctionEvaluations(Tao tao,PetscInt *nfcn)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(tao,TAO_CLASSID,1);
  *nfcn = tao->max_funcs;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoGetCurrentFunctionEvaluations"
/*@
   TaoGetCurrentFunctionEvaluations - Get current number of
   function evaluations.

   Not Collective

   Input Parameters:
.  tao - the Tao solver context

   Output Parameters:
.  nfuncs - the current number of function evaluations

   Level: intermediate

.seealso: TaoSetMaximumFunctionEvaluations(), TaoGetMaximumFunctionEvaluations(), TaoGetMaximumIterations()
@*/

PetscErrorCode TaoGetCurrentFunctionEvaluations(Tao tao,PetscInt *nfuncs)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(tao,TAO_CLASSID,1);
  *nfuncs=PetscMax(tao->nfuncs,tao->nfuncgrads);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoSetMaximumIterations"
/*@
   TaoSetMaximumIterations - Sets a maximum number of iterates.

   Logically Collective on Tao

   Input Parameters:
+  tao - the Tao solver context
-  maxits - the maximum number of iterates (>=0)

   Options Database Keys:
.    -tao_max_it <its> - sets the maximum number of iterations

   Level: intermediate

.seealso: TaoSetTolerances(), TaoSetMaximumFunctionEvaluations()
@*/
PetscErrorCode TaoSetMaximumIterations(Tao tao,PetscInt maxits)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(tao,TAO_CLASSID,1);
  tao->max_it = PetscMax(0,maxits);
  tao->max_it_changed=PETSC_TRUE;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoGetMaximumIterations"
/*@
   TaoGetMaximumIterations - Sets a maximum number of iterates.

   Not Collective

   Input Parameters:
.  tao - the Tao solver context

   Output Parameters:
.  maxits - the maximum number of iterates

   Level: intermediate

.seealso: TaoSetMaximumIterations(), TaoGetMaximumFunctionEvaluations()
@*/
PetscErrorCode TaoGetMaximumIterations(Tao tao,PetscInt *maxits)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(tao,TAO_CLASSID,1);
  *maxits = tao->max_it;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoSetInitialTrustRegionRadius"
/*@
   TaoSetInitialTrustRegionRadius - Sets the initial trust region radius.

   Logically collective on Tao

   Input Parameter:
+  tao - a TAO optimization solver
-  radius - the trust region radius

   Level: intermediate

   Options Database Key:
.  -tao_trust0 <t0> - sets initial trust region radius

.seealso: TaoGetTrustRegionRadius(), TaoSetTrustRegionTolerance()
@*/
PetscErrorCode TaoSetInitialTrustRegionRadius(Tao tao, PetscReal radius)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(tao,TAO_CLASSID,1);
  tao->trust0 = PetscMax(0.0,radius);
  tao->trust0_changed=PETSC_TRUE;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoGetInitialTrustRegionRadius"
/*@
   TaoGetInitialTrustRegionRadius - Sets the initial trust region radius.

   Not Collective

   Input Parameter:
.  tao - a TAO optimization solver

   Output Parameter:
.  radius - the trust region radius

   Level: intermediate

.seealso: TaoSetInitialTrustRegionRadius(), TaoGetCurrentTrustRegionRadius()
@*/
PetscErrorCode TaoGetInitialTrustRegionRadius(Tao tao, PetscReal *radius)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(tao,TAO_CLASSID,1);
  *radius = tao->trust0;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoGetCurrentTrustRegionRadius"
/*@
   TaoGetCurrentTrustRegionRadius - Gets the current trust region radius.

   Not Collective

   Input Parameter:
.  tao - a TAO optimization solver

   Output Parameter:
.  radius - the trust region radius

   Level: intermediate

.seealso: TaoSetInitialTrustRegionRadius(), TaoGetInitialTrustRegionRadius()
@*/
PetscErrorCode TaoGetCurrentTrustRegionRadius(Tao tao, PetscReal *radius)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(tao,TAO_CLASSID,1);
  *radius = tao->trust;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoGetTolerances"
/*@
  TaoGetTolerances - gets the current values of tolerances

  Not Collective

  Input Parameters:
. tao - the Tao context

  Output Parameters:
+ gatol - stop if norm of gradient is less than this
. grtol - stop if relative norm of gradient is less than this
- gttol - stop if norm of gradient is reduced by a this factor

  Note: NULL can be used as an argument if not all tolerances values are needed

.seealso TaoSetTolerances()

  Level: intermediate
@*/
PetscErrorCode TaoGetTolerances(Tao tao, PetscReal *gatol, PetscReal *grtol, PetscReal *gttol)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(tao,TAO_CLASSID,1);
  if (gatol) *gatol=tao->gatol;
  if (grtol) *grtol=tao->grtol;
  if (gttol) *gttol=tao->gttol;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoGetKSP"
/*@
  TaoGetKSP - Gets the linear solver used by the optimization solver.
  Application writers should use TaoGetKSP if they need direct access
  to the PETSc KSP object.

  Not Collective

   Input Parameters:
.  tao - the TAO solver

   Output Parameters:
.  ksp - the KSP linear solver used in the optimization solver

   Level: intermediate

@*/
PetscErrorCode TaoGetKSP(Tao tao, KSP *ksp)
{
  PetscFunctionBegin;
  *ksp = tao->ksp;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoGetLinearSolveIterations"
/*@
   TaoGetLinearSolveIterations - Gets the total number of linear iterations
   used by the TAO solver

   Not Collective

   Input Parameter:
.  tao - TAO context

   Output Parameter:
.  lits - number of linear iterations

   Notes:
   This counter is reset to zero for each successive call to TaoSolve()

   Level: intermediate

.keywords: TAO

.seealso:  TaoGetKSP()
@*/
PetscErrorCode  TaoGetLinearSolveIterations(Tao tao,PetscInt *lits)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(tao,TAO_CLASSID,1);
  PetscValidIntPointer(lits,2);
  *lits = tao->ksp_tot_its;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoGetLineSearch"
/*@
  TaoGetLineSearch - Gets the line search used by the optimization solver.
  Application writers should use TaoGetLineSearch if they need direct access
  to the TaoLineSearch object.

  Not Collective

   Input Parameters:
.  tao - the TAO solver

   Output Parameters:
.  ls - the line search used in the optimization solver

   Level: intermediate

@*/
PetscErrorCode TaoGetLineSearch(Tao tao, TaoLineSearch *ls)
{
  PetscFunctionBegin;
  *ls = tao->linesearch;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoAddLineSearchCounts"
/*@
  TaoAddLineSearchCounts - Adds the number of function evaluations spent
  in the line search to the running total.

   Input Parameters:
+  tao - the TAO solver
-  ls - the line search used in the optimization solver

   Level: developer

.seealso: TaoLineSearchApply()
@*/
PetscErrorCode TaoAddLineSearchCounts(Tao tao)
{
  PetscErrorCode ierr;
  PetscBool      flg;
  PetscInt       nfeval,ngeval,nfgeval;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(tao,TAO_CLASSID,1);
  if (tao->linesearch) {
    ierr = TaoLineSearchIsUsingTaoRoutines(tao->linesearch,&flg);CHKERRQ(ierr);
    if (!flg) {
      ierr = TaoLineSearchGetNumberFunctionEvaluations(tao->linesearch,&nfeval,&ngeval,&nfgeval);CHKERRQ(ierr);
      tao->nfuncs+=nfeval;
      tao->ngrads+=ngeval;
      tao->nfuncgrads+=nfgeval;
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoGetSolutionVector"
/*@
  TaoGetSolutionVector - Returns the vector with the current TAO solution

  Not Collective

  Input Parameter:
. tao - the Tao context

  Output Parameter:
. X - the current solution

  Level: intermediate

  Note:  The returned vector will be the same object that was passed into TaoSetInitialVector()
@*/
PetscErrorCode TaoGetSolutionVector(Tao tao, Vec *X)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(tao,TAO_CLASSID,1);
  *X = tao->solution;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoGetGradientVector"
/*@
  TaoGetGradientVector - Returns the vector with the current TAO gradient

  Not Collective

  Input Parameter:
. tao - the Tao context

  Output Parameter:
. G - the current solution

  Level: intermediate
@*/
PetscErrorCode TaoGetGradientVector(Tao tao, Vec *G)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(tao,TAO_CLASSID,1);
  *G = tao->gradient;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoResetStatistics"
/*@
   TaoResetStatistics - Initialize the statistics used by TAO for all of the solvers.
   These statistics include the iteration number, residual norms, and convergence status.
   This routine gets called before solving each optimization problem.

   Collective on Tao

   Input Parameters:
.  solver - the Tao context

   Level: developer

.seealso: TaoCreate(), TaoSolve()
@*/
PetscErrorCode TaoResetStatistics(Tao tao)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(tao,TAO_CLASSID,1);
  tao->niter        = 0;
  tao->nfuncs       = 0;
  tao->nfuncgrads   = 0;
  tao->ngrads       = 0;
  tao->nhess        = 0;
  tao->njac         = 0;
  tao->nconstraints = 0;
  tao->ksp_its      = 0;
  tao->ksp_tot_its      = 0;
  tao->reason       = TAO_CONTINUE_ITERATING;
  tao->residual     = 0.0;
  tao->cnorm        = 0.0;
  tao->step         = 0.0;
  tao->lsflag       = PETSC_FALSE;
  if (tao->hist_reset) tao->hist_len=0;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoSetConvergenceTest"
/*@C
  TaoSetConvergenceTest - Sets the function that is to be used to test
  for convergence o fthe iterative minimization solution.  The new convergence
  testing routine will replace TAO's default convergence test.

  Logically Collective on Tao

  Input Parameters:
+ tao - the Tao object
. conv - the routine to test for convergence
- ctx - [optional] context for private data for the convergence routine
        (may be NULL)

  Calling sequence of conv:
$   PetscErrorCode conv(Tao tao, void *ctx)

+ tao - the Tao object
- ctx - [optional] convergence context

  Note: The new convergence testing routine should call TaoSetConvergedReason().

  Level: advanced

.seealso: TaoSetConvergedReason(), TaoGetSolutionStatus(), TaoGetTolerances(), TaoSetMonitor

@*/
PetscErrorCode TaoSetConvergenceTest(Tao tao, PetscErrorCode (*conv)(Tao,void*), void *ctx)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(tao,TAO_CLASSID,1);
  (tao)->ops->convergencetest = conv;
  (tao)->cnvP = ctx;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoSetMonitor"
/*@C
   TaoSetMonitor - Sets an ADDITIONAL function that is to be used at every
   iteration of the solver to display the iteration's
   progress.

   Logically Collective on Tao

   Input Parameters:
+  tao - the Tao solver context
.  mymonitor - monitoring routine
-  mctx - [optional] user-defined context for private data for the
          monitor routine (may be NULL)

   Calling sequence of mymonitor:
$     int mymonitor(Tao tao,void *mctx)

+    tao - the Tao solver context
-    mctx - [optional] monitoring context


   Options Database Keys:
+    -tao_monitor        - sets TaoDefaultMonitor()
.    -tao_smonitor       - sets short monitor
.    -tao_cmonitor       - same as smonitor plus constraint norm
.    -tao_view_solution   - view solution at each iteration
.    -tao_view_gradient   - view gradient at each iteration
.    -tao_view_separableobjective - view separable objective function at each iteration
-    -tao_cancelmonitors - cancels all monitors that have been hardwired into a code by calls to TaoSetMonitor(), but does not cancel those set via the options database.


   Notes:
   Several different monitoring routines may be set by calling
   TaoSetMonitor() multiple times; all will be called in the
   order in which they were set.

   Fortran Notes: Only one monitor function may be set

   Level: intermediate

.seealso: TaoDefaultMonitor(), TaoCancelMonitors(),  TaoSetDestroyRoutine()
@*/
PetscErrorCode TaoSetMonitor(Tao tao, PetscErrorCode (*func)(Tao, void*), void *ctx,PetscErrorCode (*dest)(void**))
{
  PetscErrorCode ierr;
  PetscInt       i;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(tao,TAO_CLASSID,1);
  if (tao->numbermonitors >= MAXTAOMONITORS) SETERRQ1(PETSC_COMM_SELF,1,"Cannot attach another monitor -- max=",MAXTAOMONITORS);

  for (i=0; i<tao->numbermonitors;i++) {
    if (func == tao->monitor[i] && dest == tao->monitordestroy[i] && ctx == tao->monitorcontext[i]) {
      if (dest) {
        ierr = (*dest)(&ctx);CHKERRQ(ierr);
      }
      PetscFunctionReturn(0);
    }
  }
  tao->monitor[tao->numbermonitors] = func;
  tao->monitorcontext[tao->numbermonitors] = ctx;
  tao->monitordestroy[tao->numbermonitors] = dest;
  ++tao->numbermonitors;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoCancelMonitors"
/*@
   TaoCancelMonitors - Clears all the monitor functions for a Tao object.

   Logically Collective on Tao

   Input Parameters:
.  tao - the Tao solver context

   Options Database:
.  -tao_cancelmonitors - cancels all monitors that have been hardwired
    into a code by calls to TaoSetMonitor(), but does not cancel those
    set via the options database

   Notes:
   There is no way to clear one specific monitor from a Tao object.

   Level: advanced

.seealso: TaoDefaultMonitor(), TaoSetMonitor()
@*/
PetscErrorCode TaoCancelMonitors(Tao tao)
{
  PetscInt       i;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(tao,TAO_CLASSID,1);
  for (i=0;i<tao->numbermonitors;i++) {
    if (tao->monitordestroy[i]) {
      ierr = (*tao->monitordestroy[i])(&tao->monitorcontext[i]);CHKERRQ(ierr);
    }
  }
  tao->numbermonitors=0;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoDefaultMonitor"
/*@
   TaoDefaultMonitor - Default routine for monitoring progress of the
   Tao solvers (default).  This monitor prints the function value and gradient
   norm at each iteration.  It can be turned on from the command line using the
   -tao_monitor option

   Collective on Tao

   Input Parameters:
+  tao - the Tao context
-  ctx - PetscViewer context or NULL

   Options Database Keys:
.  -tao_monitor

   Level: advanced

.seealso: TaoDefaultSMonitor(), TaoSetMonitor()
@*/
PetscErrorCode TaoDefaultMonitor(Tao tao, void *ctx)
{
  PetscErrorCode ierr;
  PetscInt       its;
  PetscReal      fct,gnorm;
  PetscViewer    viewer = (PetscViewer)ctx;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(viewer,PETSC_VIEWER_CLASSID,2);
  its=tao->niter;
  fct=tao->fc;
  gnorm=tao->residual;
  ierr=PetscViewerASCIIPrintf(viewer,"iter = %3D,",its);CHKERRQ(ierr);
  ierr=PetscViewerASCIIPrintf(viewer," Function value: %g,",(double)fct);CHKERRQ(ierr);
  if (gnorm >= PETSC_INFINITY) {
    ierr=PetscViewerASCIIPrintf(viewer,"  Residual: Inf \n");CHKERRQ(ierr);
  } else {
    ierr=PetscViewerASCIIPrintf(viewer,"  Residual: %g \n",(double)gnorm);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoDefaultSMonitor"
/*@
   TaoDefaultSMonitor - Default routine for monitoring progress of the
   solver. Same as TaoDefaultMonitor() except
   it prints fewer digits of the residual as the residual gets smaller.
   This is because the later digits are meaningless and are often
   different on different machines; by using this routine different
   machines will usually generate the same output. It can be turned on
   by using the -tao_smonitor option

   Collective on Tao

   Input Parameters:
+  tao - the Tao context
-  ctx - PetscViewer context of type ASCII

   Options Database Keys:
.  -tao_smonitor

   Level: advanced

.seealso: TaoDefaultMonitor(), TaoSetMonitor()
@*/
PetscErrorCode TaoDefaultSMonitor(Tao tao, void *ctx)
{
  PetscErrorCode ierr;
  PetscInt       its;
  PetscReal      fct,gnorm;
  PetscViewer    viewer = (PetscViewer)ctx;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(viewer,PETSC_VIEWER_CLASSID,2);
  its=tao->niter;
  fct=tao->fc;
  gnorm=tao->residual;
  ierr=PetscViewerASCIIPrintf(viewer,"iter = %3D,",its);CHKERRQ(ierr);
  ierr=PetscViewerASCIIPrintf(viewer," Function value %g,",(double)fct);CHKERRQ(ierr);
  if (gnorm >= PETSC_INFINITY) {
    ierr=PetscViewerASCIIPrintf(viewer," Residual: Inf \n");CHKERRQ(ierr);
  } else if (gnorm > 1.e-6) {
    ierr=PetscViewerASCIIPrintf(viewer," Residual: %g \n",(double)gnorm);CHKERRQ(ierr);
  } else if (gnorm > 1.e-11) {
    ierr=PetscViewerASCIIPrintf(viewer," Residual: < 1.0e-6 \n");CHKERRQ(ierr);
  } else {
    ierr=PetscViewerASCIIPrintf(viewer," Residual: < 1.0e-11 \n");CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoDefaultCMonitor"
/*@
   TaoDefaultCMonitor - same as TaoDefaultMonitor() except
   it prints the norm of the constraints function. It can be turned on
   from the command line using the -tao_cmonitor option

   Collective on Tao

   Input Parameters:
+  tao - the Tao context
-  ctx - PetscViewer context or NULL

   Options Database Keys:
.  -tao_cmonitor

   Level: advanced

.seealso: TaoDefaultMonitor(), TaoSetMonitor()
@*/
PetscErrorCode TaoDefaultCMonitor(Tao tao, void *ctx)
{
  PetscErrorCode ierr;
  PetscInt       its;
  PetscReal      fct,gnorm;
  PetscViewer    viewer = (PetscViewer)ctx;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(viewer,PETSC_VIEWER_CLASSID,2);
  its=tao->niter;
  fct=tao->fc;
  gnorm=tao->residual;
  ierr=PetscViewerASCIIPrintf(viewer,"iter = %D,",its);CHKERRQ(ierr);
  ierr=PetscViewerASCIIPrintf(viewer," Function value: %g,",(double)fct);CHKERRQ(ierr);
  ierr=PetscViewerASCIIPrintf(viewer,"  Residual: %g ",(double)gnorm);CHKERRQ(ierr);
  ierr = PetscViewerASCIIPrintf(viewer,"  Constraint: %g \n",(double)tao->cnorm);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoSolutionMonitor"
/*@C
   TaoSolutionMonitor - Views the solution at each iteration
   It can be turned on from the command line using the
   -tao_view_solution option

   Collective on Tao

   Input Parameters:
+  tao - the Tao context
-  ctx - PetscViewer context or NULL

   Options Database Keys:
.  -tao_view_solution

   Level: advanced

.seealso: TaoDefaultSMonitor(), TaoSetMonitor()
@*/
PetscErrorCode TaoSolutionMonitor(Tao tao, void *ctx)
{
  PetscErrorCode ierr;
  PetscViewer    viewer  = (PetscViewer)ctx;;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(viewer,PETSC_VIEWER_CLASSID,2);
  ierr = VecView(tao->solution, viewer);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoGradientMonitor"
/*@C
   TaoGradientMonitor - Views the gradient at each iteration
   It can be turned on from the command line using the
   -tao_view_gradient option

   Collective on Tao

   Input Parameters:
+  tao - the Tao context
-  ctx - PetscViewer context or NULL

   Options Database Keys:
.  -tao_view_gradient

   Level: advanced

.seealso: TaoDefaultSMonitor(), TaoSetMonitor()
@*/
PetscErrorCode TaoGradientMonitor(Tao tao, void *ctx)
{
  PetscErrorCode ierr;
  PetscViewer    viewer = (PetscViewer)ctx;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(viewer,PETSC_VIEWER_CLASSID,2);
  ierr = VecView(tao->gradient, viewer);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoStepDirectionMonitor"
/*@C
   TaoStepDirectionMonitor - Views the gradient at each iteration
   It can be turned on from the command line using the
   -tao_view_gradient option

   Collective on Tao

   Input Parameters:
+  tao - the Tao context
-  ctx - PetscViewer context or NULL

   Options Database Keys:
.  -tao_view_gradient

   Level: advanced

.seealso: TaoDefaultSMonitor(), TaoSetMonitor()
@*/
PetscErrorCode TaoStepDirectionMonitor(Tao tao, void *ctx)
{
  PetscErrorCode ierr;
  PetscViewer    viewer = (PetscViewer)ctx;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(viewer,PETSC_VIEWER_CLASSID,2);
  ierr = VecView(tao->stepdirection, viewer);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoDrawSolutionMonitor"
/*@C
   TaoDrawSolutionMonitor - Plots the solution at each iteration
   It can be turned on from the command line using the
   -tao_draw_solution option

   Collective on Tao

   Input Parameters:
+  tao - the Tao context
-  ctx - PetscViewer context

   Options Database Keys:
.  -tao_draw_solution

   Level: advanced

.seealso: TaoSolutionMonitor(), TaoSetMonitor(), TaoDrawGradientMonitor
@*/
PetscErrorCode TaoDrawSolutionMonitor(Tao tao, void *ctx)
{
  PetscErrorCode ierr;
  PetscViewer    viewer = (PetscViewer) ctx;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(viewer,PETSC_VIEWER_CLASSID,2);
  ierr = VecView(tao->solution, viewer);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoDrawGradientMonitor"
/*@C
   TaoDrawGradientMonitor - Plots the gradient at each iteration
   It can be turned on from the command line using the
   -tao_draw_gradient option

   Collective on Tao

   Input Parameters:
+  tao - the Tao context
-  ctx - PetscViewer context

   Options Database Keys:
.  -tao_draw_gradient

   Level: advanced

.seealso: TaoGradientMonitor(), TaoSetMonitor(), TaoDrawSolutionMonitor
@*/
PetscErrorCode TaoDrawGradientMonitor(Tao tao, void *ctx)
{
  PetscErrorCode ierr;
  PetscViewer    viewer = (PetscViewer)ctx;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(viewer,PETSC_VIEWER_CLASSID,2);
  ierr = VecView(tao->gradient, viewer);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoDrawStepMonitor"
/*@C
   TaoDrawStepMonitor - Plots the step direction at each iteration
   It can be turned on from the command line using the
   -tao_draw_step option

   Collective on Tao

   Input Parameters:
+  tao - the Tao context
-  ctx - PetscViewer context

   Options Database Keys:
.  -tao_draw_step

   Level: advanced

.seealso: TaoSetMonitor(), TaoDrawSolutionMonitor
@*/
PetscErrorCode TaoDrawStepMonitor(Tao tao, void *ctx)
{
  PetscErrorCode ierr;
  PetscViewer    viewer = (PetscViewer)(ctx);

  PetscFunctionBegin;
  ierr = VecView(tao->stepdirection, viewer);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoSeparableObjectiveMonitor"
/*@C
   TaoSeparableObjectiveMonitor - Views the separable objective function at each iteration
   It can be turned on from the command line using the
   -tao_view_separableobjective option

   Collective on Tao

   Input Parameters:
+  tao - the Tao context
-  ctx - PetscViewer context or NULL

   Options Database Keys:
.  -tao_view_separableobjective

   Level: advanced

.seealso: TaoDefaultSMonitor(), TaoSetMonitor()
@*/
PetscErrorCode TaoSeparableObjectiveMonitor(Tao tao, void *ctx)
{
  PetscErrorCode ierr;
  PetscViewer    viewer  = (PetscViewer)ctx;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(viewer,PETSC_VIEWER_CLASSID,2);
  ierr = VecView(tao->sep_objective,viewer);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoDefaultConvergenceTest"
/*@
   TaoDefaultConvergenceTest - Determines whether the solver should continue iterating
   or terminate.

   Collective on Tao

   Input Parameters:
+  tao - the Tao context
-  dummy - unused dummy context

   Output Parameter:
.  reason - for terminating

   Notes:
   This routine checks the residual in the optimality conditions, the
   relative residual in the optimity conditions, the number of function
   evaluations, and the function value to test convergence.  Some
   solvers may use different convergence routines.

   Level: developer

.seealso: TaoSetTolerances(),TaoGetConvergedReason(),TaoSetConvergedReason()
@*/

PetscErrorCode TaoDefaultConvergenceTest(Tao tao,void *dummy)
{
  PetscInt           niter=tao->niter, nfuncs=PetscMax(tao->nfuncs,tao->nfuncgrads);
  PetscInt           max_funcs=tao->max_funcs;
  PetscReal          gnorm=tao->residual, gnorm0=tao->gnorm0;
  PetscReal          f=tao->fc, steptol=tao->steptol,trradius=tao->step;
  PetscReal          gatol=tao->gatol,grtol=tao->grtol,gttol=tao->gttol;
  PetscReal          catol=tao->catol,crtol=tao->crtol;
  PetscReal          fmin=tao->fmin, cnorm=tao->cnorm;
  TaoConvergedReason reason=tao->reason;
  PetscErrorCode     ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(tao, TAO_CLASSID,1);
  if (reason != TAO_CONTINUE_ITERATING) {
    PetscFunctionReturn(0);
  }

  if (PetscIsInfOrNanReal(f)) {
    ierr = PetscInfo(tao,"Failed to converged, function value is Inf or NaN\n");CHKERRQ(ierr);
    reason = TAO_DIVERGED_NAN;
  } else if (f <= fmin && cnorm <=catol) {
    ierr = PetscInfo2(tao,"Converged due to function value %g < minimum function value %g\n", (double)f,(double)fmin);CHKERRQ(ierr);
    reason = TAO_CONVERGED_MINF;
  } else if (gnorm<= gatol && cnorm <=catol) {
    ierr = PetscInfo2(tao,"Converged due to residual norm ||g(X)||=%g < %g\n",(double)gnorm,(double)gatol);CHKERRQ(ierr);
    reason = TAO_CONVERGED_GATOL;
  } else if ( f!=0 && PetscAbsReal(gnorm/f) <= grtol && cnorm <= crtol) {
    ierr = PetscInfo2(tao,"Converged due to residual ||g(X)||/|f(X)| =%g < %g\n",(double)(gnorm/f),(double)grtol);CHKERRQ(ierr);
    reason = TAO_CONVERGED_GRTOL;
  } else if (gnorm0 != 0 && ((gttol == 0 && gnorm == 0) || gnorm/gnorm0 < gttol) && cnorm <= crtol) {
    ierr = PetscInfo2(tao,"Converged due to relative residual norm ||g(X)||/||g(X0)|| = %g < %g\n",(double)(gnorm/gnorm0),(double)gttol);CHKERRQ(ierr);
    reason = TAO_CONVERGED_GTTOL;
  } else if (nfuncs > max_funcs){
    ierr = PetscInfo2(tao,"Exceeded maximum number of function evaluations: %D > %D\n", nfuncs,max_funcs);CHKERRQ(ierr);
    reason = TAO_DIVERGED_MAXFCN;
  } else if ( tao->lsflag != 0 ){
    ierr = PetscInfo(tao,"Tao Line Search failure.\n");CHKERRQ(ierr);
    reason = TAO_DIVERGED_LS_FAILURE;
  } else if (trradius < steptol && niter > 0){
    ierr = PetscInfo2(tao,"Trust region/step size too small: %g < %g\n", (double)trradius,(double)steptol);CHKERRQ(ierr);
    reason = TAO_CONVERGED_STEPTOL;
  } else if (niter > tao->max_it) {
    ierr = PetscInfo2(tao,"Exceeded maximum number of iterations: %D > %D\n",niter,tao->max_it);CHKERRQ(ierr);
    reason = TAO_DIVERGED_MAXITS;
  } else {
    reason = TAO_CONTINUE_ITERATING;
  }
  tao->reason = reason;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoSetOptionsPrefix"
/*@C
   TaoSetOptionsPrefix - Sets the prefix used for searching for all
   TAO options in the database.


   Logically Collective on Tao

   Input Parameters:
+  tao - the Tao context
-  prefix - the prefix string to prepend to all TAO option requests

   Notes:
   A hyphen (-) must NOT be given at the beginning of the prefix name.
   The first character of all runtime options is AUTOMATICALLY the hyphen.

   For example, to distinguish between the runtime options for two
   different TAO solvers, one could call
.vb
      TaoSetOptionsPrefix(tao1,"sys1_")
      TaoSetOptionsPrefix(tao2,"sys2_")
.ve

   This would enable use of different options for each system, such as
.vb
      -sys1_tao_method blmvm -sys1_tao_gtol 1.e-3
      -sys2_tao_method lmvm  -sys2_tao_gtol 1.e-4
.ve


   Level: advanced

.seealso: TaoAppendOptionsPrefix(), TaoGetOptionsPrefix()
@*/

PetscErrorCode TaoSetOptionsPrefix(Tao tao, const char p[])
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscObjectSetOptionsPrefix((PetscObject)tao,p);CHKERRQ(ierr);
  if (tao->linesearch) {
    ierr = TaoLineSearchSetOptionsPrefix(tao->linesearch,p);CHKERRQ(ierr);
  }
  if (tao->ksp) {
    ierr = KSPSetOptionsPrefix(tao->ksp,p);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoAppendOptionsPrefix"
/*@C
   TaoAppendOptionsPrefix - Appends to the prefix used for searching for all
   TAO options in the database.


   Logically Collective on Tao

   Input Parameters:
+  tao - the Tao solver context
-  prefix - the prefix string to prepend to all TAO option requests

   Notes:
   A hyphen (-) must NOT be given at the beginning of the prefix name.
   The first character of all runtime options is AUTOMATICALLY the hyphen.


   Level: advanced

.seealso: TaoSetOptionsPrefix(), TaoGetOptionsPrefix()
@*/
PetscErrorCode TaoAppendOptionsPrefix(Tao tao, const char p[])
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscObjectAppendOptionsPrefix((PetscObject)tao,p);CHKERRQ(ierr);
  if (tao->linesearch) {
    ierr = TaoLineSearchSetOptionsPrefix(tao->linesearch,p);CHKERRQ(ierr);
  }
  if (tao->ksp) {
    ierr = KSPSetOptionsPrefix(tao->ksp,p);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoGetOptionsPrefix"
/*@C
  TaoGetOptionsPrefix - Gets the prefix used for searching for all
  TAO options in the database

  Not Collective

  Input Parameters:
. tao - the Tao context

  Output Parameters:
. prefix - pointer to the prefix string used is returned

  Notes: On the fortran side, the user should pass in a string 'prefix' of
  sufficient length to hold the prefix.

  Level: advanced

.seealso: TaoSetOptionsPrefix(), TaoAppendOptionsPrefix()
@*/
PetscErrorCode TaoGetOptionsPrefix(Tao tao, const char *p[])
{
   return PetscObjectGetOptionsPrefix((PetscObject)tao,p);
}

#undef __FUNCT__
#define __FUNCT__ "TaoSetType"
/*@C
   TaoSetType - Sets the method for the unconstrained minimization solver.

   Collective on Tao

   Input Parameters:
+  solver - the Tao solver context
-  type - a known method

   Options Database Key:
.  -tao_type <type> - Sets the method; use -help for a list
   of available methods (for instance, "-tao_type lmvm" or "-tao_type tron")

   Available methods include:
+    nls - Newton's method with line search for unconstrained minimization
.    ntr - Newton's method with trust region for unconstrained minimization
.    ntl - Newton's method with trust region, line search for unconstrained minimization
.    lmvm - Limited memory variable metric method for unconstrained minimization
.    cg - Nonlinear conjugate gradient method for unconstrained minimization
.    nm - Nelder-Mead algorithm for derivate-free unconstrained minimization
.    tron - Newton Trust Region method for bound constrained minimization
.    gpcg - Newton Trust Region method for quadratic bound constrained minimization
.    blmvm - Limited memory variable metric method for bound constrained minimization
-    pounders - Model-based algorithm pounder extended for nonlinear least squares

  Level: intermediate

.seealso: TaoCreate(), TaoGetType(), TaoType

@*/
PetscErrorCode TaoSetType(Tao tao, const TaoType type)
{
  PetscErrorCode ierr;
  PetscErrorCode (*create_xxx)(Tao);
  PetscBool      issame;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(tao,TAO_CLASSID,1);

  ierr = PetscObjectTypeCompare((PetscObject)tao,type,&issame);CHKERRQ(ierr);
  if (issame) PetscFunctionReturn(0);

  ierr = PetscFunctionListFind(TaoList, type, (void(**)(void))&create_xxx);CHKERRQ(ierr);
  if (!create_xxx) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_ARG_UNKNOWN_TYPE,"Unable to find requested Tao type %s",type);

  /* Destroy the existing solver information */
  if (tao->ops->destroy) {
    ierr = (*tao->ops->destroy)(tao);CHKERRQ(ierr);
  }
  ierr = KSPDestroy(&tao->ksp);CHKERRQ(ierr);
  ierr = TaoLineSearchDestroy(&tao->linesearch);CHKERRQ(ierr);
  ierr = VecDestroy(&tao->gradient);CHKERRQ(ierr);
  ierr = VecDestroy(&tao->stepdirection);CHKERRQ(ierr);

  tao->ops->setup = 0;
  tao->ops->solve = 0;
  tao->ops->view  = 0;
  tao->ops->setfromoptions = 0;
  tao->ops->destroy = 0;

  tao->setupcalled = PETSC_FALSE;

  ierr = (*create_xxx)(tao);CHKERRQ(ierr);
  ierr = PetscObjectChangeTypeName((PetscObject)tao,type);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoRegister"
/*MC
   TaoRegister - Adds a method to the TAO package for unconstrained minimization.

   Synopsis:
   TaoRegister(char *name_solver,char *path,char *name_Create,int (*routine_Create)(Tao))

   Not collective

   Input Parameters:
+  sname - name of a new user-defined solver
-  func - routine to Create method context

   Notes:
   TaoRegister() may be called multiple times to add several user-defined solvers.

   Sample usage:
.vb
   TaoRegister("my_solver",MySolverCreate);
.ve

   Then, your solver can be chosen with the procedural interface via
$     TaoSetType(tao,"my_solver")
   or at runtime via the option
$     -tao_type my_solver

   Level: advanced

.seealso: TaoRegisterAll(), TaoRegisterDestroy()
M*/
PetscErrorCode TaoRegister(const char sname[], PetscErrorCode (*func)(Tao))
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscFunctionListAdd(&TaoList,sname, (void (*)(void))func);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoRegisterDestroy"
/*@C
   TaoRegisterDestroy - Frees the list of minimization solvers that were
   registered by TaoRegisterDynamic().

   Not Collective

   Level: advanced

.seealso: TaoRegisterAll(), TaoRegister()
@*/
PetscErrorCode TaoRegisterDestroy(void)
{
  PetscErrorCode ierr;
  PetscFunctionBegin;
  ierr = PetscFunctionListDestroy(&TaoList);CHKERRQ(ierr);
  TaoRegisterAllCalled = PETSC_FALSE;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoGetIterationNumber"
/*@
   TaoGetIterationNumber - Gets the number of Tao iterations completed
   at this time.

   Not Collective

   Input Parameter:
.  tao - Tao context

   Output Parameter:
.  iter - iteration number

   Notes:
   For example, during the computation of iteration 2 this would return 1.


   Level: intermediate

.keywords: Tao, nonlinear, get, iteration, number,

.seealso:   TaoGetLinearSolveIterations()
@*/
PetscErrorCode  TaoGetIterationNumber(Tao tao,PetscInt *iter)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(tao,TAO_CLASSID,1);
  PetscValidIntPointer(iter,2);
  *iter = tao->niter;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoSetIterationNumber"
/*@
   TaoSetIterationNumber - Sets the current iteration number.

   Not Collective

   Input Parameter:
.  tao - Tao context
.  iter - iteration number

   Level: developer

.keywords: Tao, nonlinear, set, iteration, number,

.seealso:   TaoGetLinearSolveIterations()
@*/
PetscErrorCode  TaoSetIterationNumber(Tao tao,PetscInt iter)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(tao,TAO_CLASSID,1);
  ierr       = PetscObjectSAWsTakeAccess((PetscObject)tao);CHKERRQ(ierr);
  tao->niter = iter;
  ierr       = PetscObjectSAWsGrantAccess((PetscObject)tao);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoGetTotalIterationNumber"
/*@
   TaoGetTotalIterationNumber - Gets the total number of Tao iterations
   completed. This number keeps accumulating if multiple solves
   are called with the Tao object.

   Not Collective

   Input Parameter:
.  tao - Tao context

   Output Parameter:
.  iter - iteration number

   Notes:
   The total iteration count is updated after each solve, if there is a current
   TaoSolve() in progress then those iterations are not yet counted.

   Level: intermediate

.keywords: Tao, nonlinear, get, iteration, number,

.seealso:   TaoGetLinearSolveIterations()
@*/
PetscErrorCode  TaoGetTotalIterationNumber(Tao tao,PetscInt *iter)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(tao,TAO_CLASSID,1);
  PetscValidIntPointer(iter,2);
  *iter = tao->ntotalits;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoSetTotalIterationNumber"
/*@
   TaoSetTotalIterationNumber - Sets the current total iteration number.

   Not Collective

   Input Parameter:
.  tao - Tao context
.  iter - iteration number

   Level: developer

.keywords: Tao, nonlinear, set, iteration, number,

.seealso:   TaoGetLinearSolveIterations()
@*/
PetscErrorCode  TaoSetTotalIterationNumber(Tao tao,PetscInt iter)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(tao,TAO_CLASSID,1);
  ierr       = PetscObjectSAWsTakeAccess((PetscObject)tao);CHKERRQ(ierr);
  tao->ntotalits = iter;
  ierr       = PetscObjectSAWsGrantAccess((PetscObject)tao);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoSetConvergedReason"
/*@
  TaoSetConvergedReason - Sets the termination flag on a Tao object

  Logically Collective on Tao

  Input Parameters:
+ tao - the Tao context
- reason - one of
$     TAO_CONVERGED_ATOL (2),
$     TAO_CONVERGED_RTOL (3),
$     TAO_CONVERGED_STEPTOL (4),
$     TAO_CONVERGED_MINF (5),
$     TAO_CONVERGED_USER (6),
$     TAO_DIVERGED_MAXITS (-2),
$     TAO_DIVERGED_NAN (-4),
$     TAO_DIVERGED_MAXFCN (-5),
$     TAO_DIVERGED_LS_FAILURE (-6),
$     TAO_DIVERGED_TR_REDUCTION (-7),
$     TAO_DIVERGED_USER (-8),
$     TAO_CONTINUE_ITERATING (0)

   Level: intermediate

@*/
PetscErrorCode TaoSetConvergedReason(Tao tao, TaoConvergedReason reason)
{
  PetscValidHeaderSpecific(tao,TAO_CLASSID,1);
  PetscFunctionBegin;
  tao->reason = reason;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoGetConvergedReason"
/*@
   TaoGetConvergedReason - Gets the reason the Tao iteration was stopped.

   Not Collective

   Input Parameter:
.  tao - the Tao solver context

   Output Parameter:
.  reason - one of
$  TAO_CONVERGED_GATOL (3)           ||g(X)|| < gatol
$  TAO_CONVERGED_GRTOL (4)           ||g(X)|| / f(X)  < grtol
$  TAO_CONVERGED_GTTOL (5)           ||g(X)|| / ||g(X0)|| < gttol
$  TAO_CONVERGED_STEPTOL (6)         step size small
$  TAO_CONVERGED_MINF (7)            F < F_min
$  TAO_CONVERGED_USER (8)            User defined
$  TAO_DIVERGED_MAXITS (-2)          its > maxits
$  TAO_DIVERGED_NAN (-4)             Numerical problems
$  TAO_DIVERGED_MAXFCN (-5)          fevals > max_funcsals
$  TAO_DIVERGED_LS_FAILURE (-6)      line search failure
$  TAO_DIVERGED_TR_REDUCTION (-7)    trust region failure
$  TAO_DIVERGED_USER(-8)             (user defined)
 $  TAO_CONTINUE_ITERATING (0)

   where
+  X - current solution
.  X0 - initial guess
.  f(X) - current function value
.  f(X*) - true solution (estimated)
.  g(X) - current gradient
.  its - current iterate number
.  maxits - maximum number of iterates
.  fevals - number of function evaluations
-  max_funcsals - maximum number of function evaluations

   Level: intermediate

.seealso: TaoSetConvergenceTest(), TaoSetTolerances()

@*/
PetscErrorCode TaoGetConvergedReason(Tao tao, TaoConvergedReason *reason)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(tao,TAO_CLASSID,1);
  PetscValidPointer(reason,2);
  *reason = tao->reason;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoGetSolutionStatus"
/*@
  TaoGetSolutionStatus - Get the current iterate, objective value,
  residual, infeasibility, and termination

  Not Collective

   Input Parameters:
.  tao - the Tao context

   Output Parameters:
+  iterate - the current iterate number (>=0)
.  f - the current function value
.  gnorm - the square of the gradient norm, duality gap, or other measure indicating distance from optimality.
.  cnorm - the infeasibility of the current solution with regard to the constraints.
.  xdiff - the step length or trust region radius of the most recent iterate.
-  reason - The termination reason, which can equal TAO_CONTINUE_ITERATING

   Level: intermediate

   Note:
   TAO returns the values set by the solvers in the routine TaoMonitor().

   Note:
   If any of the output arguments are set to NULL, no corresponding value will be returned.

.seealso: TaoMonitor(), TaoGetConvergedReason()
@*/
PetscErrorCode TaoGetSolutionStatus(Tao tao, PetscInt *its, PetscReal *f, PetscReal *gnorm, PetscReal *cnorm, PetscReal *xdiff, TaoConvergedReason *reason)
{
  PetscFunctionBegin;
  if (its) *its=tao->niter;
  if (f) *f=tao->fc;
  if (gnorm) *gnorm=tao->residual;
  if (cnorm) *cnorm=tao->cnorm;
  if (reason) *reason=tao->reason;
  if (xdiff) *xdiff=tao->step;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoGetType"
/*@C
   TaoGetType - Gets the current Tao algorithm.

   Not Collective

   Input Parameter:
.  tao - the Tao solver context

   Output Parameter:
.  type - Tao method

   Level: intermediate

@*/
PetscErrorCode TaoGetType(Tao tao, const TaoType *type)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(tao,TAO_CLASSID,1);
  PetscValidPointer(type,2);
  *type=((PetscObject)tao)->type_name;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoMonitor"
/*@C
  TaoMonitor - Monitor the solver and the current solution.  This
  routine will record the iteration number and residual statistics,
  call any monitors specified by the user, and calls the convergence-check routine.

   Input Parameters:
+  tao - the Tao context
.  its - the current iterate number (>=0)
.  f - the current objective function value
.  res - the gradient norm, square root of the duality gap, or other measure indicating distince from optimality.  This measure will be recorded and
          used for some termination tests.
.  cnorm - the infeasibility of the current solution with regard to the constraints.
-  steplength - multiple of the step direction added to the previous iterate.

   Output Parameters:
.  reason - The termination reason, which can equal TAO_CONTINUE_ITERATING

   Options Database Key:
.  -tao_monitor - Use the default monitor, which prints statistics to standard output

.seealso TaoGetConvergedReason(), TaoDefaultMonitor(), TaoSetMonitor()

   Level: developer

@*/
PetscErrorCode TaoMonitor(Tao tao, PetscInt its, PetscReal f, PetscReal res, PetscReal cnorm, PetscReal steplength, TaoConvergedReason *reason)
{
  PetscErrorCode ierr;
  PetscInt       i;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(tao,TAO_CLASSID,1);
  tao->fc = f;
  tao->residual = res;
  tao->cnorm = cnorm;
  tao->step = steplength;
  if (!its) {
    tao->cnorm0 = cnorm; tao->gnorm0 = res;
  }
  TaoLogConvergenceHistory(tao,f,res,cnorm,tao->ksp_its);
  if (PetscIsInfOrNanReal(f) || PetscIsInfOrNanReal(res)) SETERRQ(PETSC_COMM_SELF,1, "User provided compute function generated Inf or NaN");
  if (tao->ops->convergencetest) {
    ierr = (*tao->ops->convergencetest)(tao,tao->cnvP);CHKERRQ(ierr);
  }
  for (i=0;i<tao->numbermonitors;i++) {
    ierr = (*tao->monitor[i])(tao,tao->monitorcontext[i]);CHKERRQ(ierr);
  }
  *reason = tao->reason;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoSetConvergenceHistory"
/*@
   TaoSetConvergenceHistory - Sets the array used to hold the convergence history.

   Logically Collective on Tao

   Input Parameters:
+  tao - the Tao solver context
.  obj   - array to hold objective value history
.  resid - array to hold residual history
.  cnorm - array to hold constraint violation history
.  lits - integer array holds the number of linear iterations for each Tao iteration
.  na  - size of obj, resid, and cnorm
-  reset - PetscTrue indicates each new minimization resets the history counter to zero,
           else it continues storing new values for new minimizations after the old ones

   Notes:
   If set, TAO will fill the given arrays with the indicated
   information at each iteration.  If 'obj','resid','cnorm','lits' are
   *all* NULL then space (using size na, or 1000 if na is PETSC_DECIDE or
   PETSC_DEFAULT) is allocated for the history.
   If not all are NULL, then only the non-NULL information categories
   will be stored, the others will be ignored.

   Any convergence information after iteration number 'na' will not be stored.

   This routine is useful, e.g., when running a code for purposes
   of accurate performance monitoring, when no I/O should be done
   during the section of code that is being timed.

   Level: intermediate

.seealso: TaoGetConvergenceHistory()

@*/
PetscErrorCode TaoSetConvergenceHistory(Tao tao, PetscReal *obj, PetscReal *resid, PetscReal *cnorm, PetscInt *lits, PetscInt na,PetscBool reset)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(tao,TAO_CLASSID,1);
  if (obj) PetscValidScalarPointer(obj,2);
  if (resid) PetscValidScalarPointer(resid,3);
  if (cnorm) PetscValidScalarPointer(cnorm,4);
  if (lits) PetscValidIntPointer(lits,5);

  if (na == PETSC_DECIDE || na == PETSC_DEFAULT) na = 1000;
  if (!obj && !resid && !cnorm && !lits) {
    ierr = PetscCalloc1(na,&obj);CHKERRQ(ierr);
    ierr = PetscCalloc1(na,&resid);CHKERRQ(ierr);
    ierr = PetscCalloc1(na,&cnorm);CHKERRQ(ierr);
    ierr = PetscCalloc1(na,&lits);CHKERRQ(ierr);
    tao->hist_malloc=PETSC_TRUE;
  }

  tao->hist_obj = obj;
  tao->hist_resid = resid;
  tao->hist_cnorm = cnorm;
  tao->hist_lits = lits;
  tao->hist_max   = na;
  tao->hist_reset = reset;
  tao->hist_len = 0;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoGetConvergenceHistory"
/*@C
   TaoGetConvergenceHistory - Gets the arrays used to hold the convergence history.

   Collective on Tao

   Input Parameter:
.  tao - the Tao context

   Output Parameters:
+  obj   - array used to hold objective value history
.  resid - array used to hold residual history
.  cnorm - array used to hold constraint violation history
.  lits  - integer array used to hold linear solver iteration count
-  nhist  - size of obj, resid, cnorm, and lits (will be less than or equal to na given in TaoSetHistory)

   Notes:
    This routine must be preceded by calls to TaoSetConvergenceHistory()
    and TaoSolve(), otherwise it returns useless information.

    The calling sequence for this routine in Fortran is
$   call TaoGetConvergenceHistory(Tao tao, PetscInt nhist, PetscErrorCode ierr)

   This routine is useful, e.g., when running a code for purposes
   of accurate performance monitoring, when no I/O should be done
   during the section of code that is being timed.

   Level: advanced

.seealso: TaoSetConvergenceHistory()

@*/
PetscErrorCode TaoGetConvergenceHistory(Tao tao, PetscReal **obj, PetscReal **resid, PetscReal **cnorm, PetscInt **lits, PetscInt *nhist)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(tao,TAO_CLASSID,1);
  if (obj)   *obj   = tao->hist_obj;
  if (cnorm) *cnorm = tao->hist_cnorm;
  if (resid) *resid = tao->hist_resid;
  if (nhist) *nhist   = tao->hist_len;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoSetApplicationContext"
/*@
   TaoSetApplicationContext - Sets the optional user-defined context for
   a solver.

   Logically Collective on Tao

   Input Parameters:
+  tao  - the Tao context
-  usrP - optional user context

   Level: intermediate

.seealso: TaoGetApplicationContext(), TaoSetApplicationContext()
@*/
PetscErrorCode  TaoSetApplicationContext(Tao tao,void *usrP)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(tao,TAO_CLASSID,1);
  tao->user = usrP;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoGetApplicationContext"
/*@
   TaoGetApplicationContext - Gets the user-defined context for a
   TAO solvers.

   Not Collective

   Input Parameter:
.  tao  - Tao context

   Output Parameter:
.  usrP - user context

   Level: intermediate

.seealso: TaoSetApplicationContext()
@*/
PetscErrorCode  TaoGetApplicationContext(Tao tao,void *usrP)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(tao,TAO_CLASSID,1);
  *(void**)usrP = tao->user;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoSetGradientNorm"
/*@
   TaoSetGradientNorm - Sets the matrix used to define the inner product that measures the size of the gradient.

   Collective on tao

   Input Parameters:
+  tao  - the Tao context
-  M    - gradient norm

   Level: beginner

.seealso: TaoGetGradientNorm(), TaoGradientNorm()
@*/
PetscErrorCode  TaoSetGradientNorm(Tao tao, Mat M)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(tao,TAO_CLASSID,1);

  if (tao->gradient_norm) {
    ierr = PetscObjectDereference((PetscObject)tao->gradient_norm);CHKERRQ(ierr);
    ierr = VecDestroy(&tao->gradient_norm_tmp);CHKERRQ(ierr);
  }

  ierr = PetscObjectReference((PetscObject)M);CHKERRQ(ierr);
  tao->gradient_norm = M;
  ierr = MatCreateVecs(M, NULL, &tao->gradient_norm_tmp);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoGetGradientNorm"
/*@
   TaoGetGradientNorm - Returns the matrix used to define the inner product for measuring the size of the gradient.

   Not Collective

   Input Parameter:
.  tao  - Tao context

   Output Parameter:
.  M - gradient norm

   Level: beginner

.seealso: TaoSetGradientNorm(), TaoGradientNorm()
@*/
PetscErrorCode  TaoGetGradientNorm(Tao tao, Mat *M)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(tao,TAO_CLASSID,1);
  *M = tao->gradient_norm;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoGradientNorm"
/*c
   TaoGradientNorm - Compute the norm with respect to the inner product the user has set.

   Collective on tao

   Input Parameter:
.  tao      - the Tao context
.  gradient - the gradient to be computed
.  norm     - the norm type

   Output Parameter:
.  gnorm    - the gradient norm

   Level: developer

.seealso: TaoSetGradientNorm(), TaoGetGradientNorm()
@*/
PetscErrorCode  TaoGradientNorm(Tao tao, Vec gradient, NormType type, PetscReal *gnorm)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(gradient,VEC_CLASSID,1);

  if (tao->gradient_norm) {
    PetscScalar gnorms;

    if (type != NORM_2) SETERRQ(PetscObjectComm((PetscObject)gradient), PETSC_ERR_ARG_WRONGSTATE, "Norm type must be NORM_2 if an inner product for the gradient norm is set.");
    ierr = MatMult(tao->gradient_norm, gradient, tao->gradient_norm_tmp);CHKERRQ(ierr);
    ierr = VecDot(gradient, tao->gradient_norm_tmp, &gnorms);CHKERRQ(ierr);
    *gnorm = PetscRealPart(PetscSqrtScalar(gnorms));
  } else {
    ierr = VecNorm(gradient, type, gnorm);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}


