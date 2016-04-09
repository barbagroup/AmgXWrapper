
static char help[] = "FUN3D - 3-D, Unstructured Compressible Euler Solver\n\
originally written by W. K. Anderson of NASA Langley, \n\
and ported into PETSc framework by D. K. Kaushik, ODU and ICASE.\n\n";

#include <petscsnes.h>
#include <petsctime.h>
#include "user.h"

typedef struct {
  PetscViewer viewer;
} MonitorCtx;

#define ICALLOC(size,y) ierr = PetscMalloc((PetscMax(size,1))*sizeof(PetscInt),y);CHKERRQ(ierr);
#define FCALLOC(size,y) ierr = PetscMalloc((PetscMax(size,1))*sizeof(PetscScalar),y);CHKERRQ(ierr);

typedef struct {
  Vec      qnew, qold, func;
  double   fnorm_ini, dt_ini, cfl_ini;
  double   ptime;
  double   cfl_max, max_time;
  double   fnorm, dt, cfl;
  double   fnorm_fo_rtol,fnorm_rtol, fnorm_atol;
  PetscInt ires, iramp;
  PetscInt max_steps, print_freq;
} TstepCtx;

typedef struct {                               /*============================*/
  GRID      *grid;                             /* Pointer to Grid info       */
  TstepCtx  *tsCtx;                            /* Pointer to Time Stepping Context */
  PetscBool PreLoading;
} AppCtx;                                      /*============================*/

PetscErrorCode FormJacobian(SNES,Vec,Mat,Mat,void*),
    FormFunction(SNES,Vec,Vec,void*),
    FormInitialGuess(SNES, GRID*),
    Monitor(SNES,PetscInt,double,void*),
    Update(SNES, void*),
    ComputeTimeStep(SNES, PetscInt, void*),
    GetLocalOrdering(GRID*),
    SetPetscDS(GRID*, TstepCtx*),
    FieldOutput(GRID*, PetscInt),
    WriteRestartFile(GRID*, PetscInt),
    ReadRestartFile(GRID*);


/* Global Variables */

                                               /*============================*/
CINFO    *c_info;                              /* Pointer to COMMON INFO     */
CRUNGE   *c_runge;                             /* Pointer to COMMON RUNGE    */
CGMCOM   *c_gmcom;                             /* Pointer to COMMON GMCOM    */
CREFGEOM *c_refgeom;                           /* Pointer to COMMON REFGEOM  */
                                               /*============================*/
PetscMPIInt   rank, CommSize;
PetscInt      rstart = 0, SecondOrder = 0;
off_t         solidBndPos = 0;
REAL          memSize     = 0.0, grad_time = 0.0;

#if defined(PARCH_IRIX64) && defined(USE_HW_COUNTERS)
int         event0, event1;
PetscScalar time_counters;
long long   counter0, counter1;
#endif
#if defined(PARCH_t3d)
int int_size = sizeof(short);
#else
int int_size = sizeof(int);
#endif
int  ntran[max_nbtran];       /* transition stuff put here to make global */
REAL dxtran[max_nbtran];

/* ======================== MAIN ROUTINE =================================== */
/*                                                                           */
/* Finite volume flux split solver for general polygons                      */
/*                                                                           */
/*===========================================================================*/

int main(int argc,char **args)
{
  AppCtx         user;
  GRID           f_pntr;
  TstepCtx       tsCtx;
  SNES           snes;                    /* SNES context */
  Mat            Jpc;                     /* Jacobian and Preconditioner matrices */
  PetscScalar    *qnode;
  PetscErrorCode ierr;
  int            ileast;
  PetscBool      flg;
  MPI_Comm       comm;

  ierr = PetscInitialize(&argc,&args,NULL,help);CHKERRQ(ierr);
  ierr = PetscInitializeFortran();CHKERRQ(ierr);
  comm = PETSC_COMM_WORLD;
  f77FORLINK();                               /* Link FORTRAN and C COMMONS */
  ierr = MPI_Comm_rank(MPI_COMM_WORLD,&rank);CHKERRQ(ierr);
  ierr = MPI_Comm_size(MPI_COMM_WORLD,&CommSize);CHKERRQ(ierr);

  /*PetscPrintf(MPI_COMM_WORLD, " Program name is %s\n",
                OptionsGetProgramName());*/
  PetscInitializeFortran();

  /*======================================================================*/
  /* Initilize stuff related to time stepping */
  /*======================================================================*/
  tsCtx.fnorm_ini  = 0.0;  tsCtx.cfl_ini       = 100.0;   tsCtx.cfl_max    = 1.0e+05;
  tsCtx.max_steps  = 50;   tsCtx.max_time      = 1.0e+12; tsCtx.iramp      = -50;
  tsCtx.dt         = -5.0; tsCtx.fnorm_fo_rtol = 1.0e-02; tsCtx.fnorm_rtol = 1.0e-10;
  tsCtx.fnorm_atol = 1.0e-14;
  ierr             = PetscOptionsGetInt(NULL,NULL,"-max_st",&tsCtx.max_steps,NULL);CHKERRQ(ierr);
  ierr             = PetscOptionsGetReal(NULL,"-ts_fo_rtol",&tsCtx.fnorm_fo_rtol,NULL);CHKERRQ(ierr);
  ierr             = PetscOptionsGetReal(NULL,"-ts_so_rtol",&tsCtx.fnorm_rtol,NULL);CHKERRQ(ierr);
  ierr             = PetscOptionsGetReal(NULL,"-ts_atol",&tsCtx.fnorm_atol,NULL);CHKERRQ(ierr);
  ierr             = PetscOptionsGetReal(NULL,"-cfl_ini",&tsCtx.cfl_ini,NULL);CHKERRQ(ierr);
  ierr             = PetscOptionsGetReal(NULL,"-cfl_max",&tsCtx.cfl_max,NULL);CHKERRQ(ierr);
  tsCtx.print_freq = tsCtx.max_steps;
  ierr             = PetscOptionsGetInt(NULL,NULL,"-print_freq",&tsCtx.print_freq,&flg);CHKERRQ(ierr);
  /*======================================================================*/

  f77FORLINK();                               /* Link FORTRAN and C COMMONS */

  f77OPENM(&rank);                            /* Open files for I/O         */

/*  Read input */

  f77READR1(&ileast, &rank);
  f_pntr.jvisc  = c_info->ivisc;
  f_pntr.ileast = ileast;
  c_gmcom->ilu0 = 1; c_gmcom->nsrch = 10;

  c_runge->nitfo = 0;
  ierr           = PetscOptionsGetInt(NULL,NULL,"-first_order_it",&c_runge->nitfo,&flg);CHKERRQ(ierr);


/* Read & process the grid */
/*  f77RDGPAR(&f_pntr.nnodes,  &f_pntr.ncell,   &f_pntr.nedge,
           &f_pntr.nccolor, &f_pntr.ncolor,
           &f_pntr.nnbound, &f_pntr.nvbound, &f_pntr.nfbound,
           &f_pntr.nnfacet, &f_pntr.nvfacet, &f_pntr.nffacet,
           &f_pntr.nsnode,  &f_pntr.nvnode,  &f_pntr.nfnode,
           &f_pntr.ntte,
           &f_pntr.nsface,  &f_pntr.nvface,  &f_pntr.nfface,
           &rank);*/


/* Read the grid information */

  /*f77README(&f_pntr.nnodes,  &f_pntr.ncell,   &f_pntr.nedge,
            &f_pntr.ncolor,  &f_pntr.nccolor,
            &f_pntr.nnbound, &f_pntr.nvbound, &f_pntr.nfbound,
            &f_pntr.nnfacet, &f_pntr.nvfacet, &f_pntr.nffacet,
            &f_pntr.nsnode,  &f_pntr.nvnode,  &f_pntr.nfnode,
            &f_pntr.ntte,
             f_pntr.eptr,
             f_pntr.x,        f_pntr.y,        f_pntr.z,
             f_pntr.area,     f_pntr.c2n,      f_pntr.c2e,
             f_pntr.xn,       f_pntr.yn,       f_pntr.zn,
             f_pntr.rl,
             f_pntr.nntet,    f_pntr.nnpts,    f_pntr.nvtet,
             f_pntr.nvpts,    f_pntr.nftet,    f_pntr.nfpts,
             f_pntr.f2ntn,    f_pntr.f2ntv,    f_pntr.f2ntf,
             f_pntr.isnode,   f_pntr.sxn,      f_pntr.syn,
             f_pntr.szn,      f_pntr.ivnode,   f_pntr.vxn,
             f_pntr.vyn,      f_pntr.vzn,      f_pntr.ifnode,
             f_pntr.fxn,      f_pntr.fyn,      f_pntr.fzn,
             f_pntr.slen,
            &rank);*/

/* Get the grid information into local ordering */

  ierr = GetLocalOrdering(&f_pntr);CHKERRQ(ierr);

/* Allocate Memory for Some Other Grid Arrays */
  set_up_grid(&f_pntr);

/* Now set up PETSc datastructure */
/* ierr = SetPetscDS(&f_pntr, &tsCtx);CHKERRQ(ierr);*/

/* If using least squares for the gradients, calculate the r's */
  if (f_pntr.ileast == 4) {
    f77SUMGS(&f_pntr.nnodesLoc, &f_pntr.nedgeLoc, f_pntr.eptr,
              f_pntr.x,       f_pntr.y,      f_pntr.z,
              f_pntr.rxy,
              &rank, &f_pntr.nvertices);
  }

   /*write_fine_grid(&f_pntr);*/

  user.grid  = &f_pntr;
  user.tsCtx = &tsCtx;



  /*
   Preload the executable to get accurate timings. This runs the following chunk of
   code twice, first to get the executable pages into memory and the second time for
   accurate timings.
  */
  PetscPreLoadBegin(PETSC_TRUE,"Time integration");
  user.PreLoading = PetscPreLoading;
  /* Create nonlinear solver */
  ierr = SetPetscDS(&f_pntr, &tsCtx);CHKERRQ(ierr);
  ierr = SNESCreate(comm,&snes);CHKERRQ(ierr);
  ierr = SNESSetType(snes,"ls");CHKERRQ(ierr);

  /* Set various routines and options */
  ierr = SNESSetFunction(snes,user.grid->res,FormFunction,&user);CHKERRQ(ierr);
  ierr = PetscOptionsHasName(NULL,"-matrix_free",&flg);CHKERRQ(ierr);
  if (flg) {
    /* Use matrix-free Jacobian to define Newton system; use explicit (approx)
       Jacobian for preconditioner */
    /*ierr = SNESDefaultMatrixFreeMatCreate(snes,user.grid->qnode,&Jpc);*/
    ierr = MatCreateSNESMF(snes,&Jpc);CHKERRQ(ierr);
    ierr = SNESSetJacobian(snes,Jpc,user.grid->A,FormJacobian,&user);CHKERRQ(ierr);
    /*ierr = SNESSetJacobian(snes,Jpc,user.grid->A,0,&user);*/
  } else {
    /* Use explicit (approx) Jacobian to define Newton system and
       preconditioner */
    ierr = SNESSetJacobian(snes,user.grid->A,user.grid->A,FormJacobian,&user);CHKERRQ(ierr);
  }

  /*ierr = SNESMonitorSet(snes,Monitor,(void*)&monP);CHKERRQ(ierr);*/
  ierr = SNESSetFromOptions(snes);CHKERRQ(ierr);


/* Initialize the flowfield */
  ierr = FormInitialGuess(snes,user.grid);CHKERRQ(ierr);

  /* Solve nonlinear system */

  ierr = Update(snes,&user);CHKERRQ(ierr);


/* Write restart file */
  ierr = VecGetArray(user.grid->qnode, &qnode);CHKERRQ(ierr);
  /*f77WREST(&user.grid->nnodes,qnode,user.grid->turbre,
            user.grid->amut);*/

/* Write Tecplot solution file */
#if 0
  if (!rank)
    f77TECFLO(&user.grid->nnodes,
              &user.grid->nnbound, &user.grid->nvbound, &user.grid->nfbound,
              &user.grid->nnfacet, &user.grid->nvfacet, &user.grid->nffacet,
              &user.grid->nsnode,  &user.grid->nvnode,  &user.grid->nfnode,
              c_info->title,
              user.grid->x,        user.grid->y,        user.grid->z,
              qnode,
              user.grid->nnpts,    user.grid->nntet,    user.grid->nvpts,
              user.grid->nvtet,    user.grid->nfpts,    user.grid->nftet,
              user.grid->f2ntn,    user.grid->f2ntv,    user.grid->f2ntf,
              user.grid->isnode,   user.grid->ivnode,   user.grid->ifnode,
              &rank, &nvertices);
#endif

  /*f77FASFLO(&user.grid->nnodes, &user.grid->nsnode, &user.grid->nnfacet,
             user.grid->isnode,  user.grid->f2ntn,
             user.grid->x,       user.grid->y,       user.grid->z,
             qnode);*/

/* Write residual, lift, drag, and moment history file */
/*
   if (!rank)
      f77PLLAN(&user.grid->nnodes, &rank);
*/

  ierr = VecRestoreArray(user.grid->qnode, &qnode);CHKERRQ(ierr);
  ierr = PetscOptionsHasName(NULL,"-mem_use",&flg);CHKERRQ(ierr);
  if (flg) {
    ierr = PetscMemoryView(PETSC_VIEWER_STDOUT_WORLD,"Memory usage before destroying\n");CHKERRQ(ierr);
  }

  ierr = VecDestroy(&user.grid->qnode);CHKERRQ(ierr);
  ierr = VecDestroy(&user.grid->qnodeLoc);CHKERRQ(ierr);
  ierr = VecDestroy(&user.tsCtx->qold);CHKERRQ(ierr);
  ierr = VecDestroy(&user.tsCtx->func);CHKERRQ(ierr);
  ierr = VecDestroy(&user.grid->res);CHKERRQ(ierr);
  ierr = VecDestroy(&user.grid->grad);CHKERRQ(ierr);
  ierr = VecDestroy(&user.grid->gradLoc);CHKERRQ(ierr);
  ierr = MatDestroy(&user.grid->A);CHKERRQ(ierr);
  ierr = PetscOptionsHasName(NULL,"-matrix_free",&flg);CHKERRQ(ierr);
  if (flg) { ierr = MatDestroy(&Jpc);CHKERRQ(ierr);}
  ierr = SNESDestroy(&snes);CHKERRQ(ierr);
  ierr = VecScatterDestroy(&user.grid->scatter);CHKERRQ(ierr);
  ierr = VecScatterDestroy(&user.grid->gradScatter);CHKERRQ(ierr);
  ierr = PetscOptionsHasName(NULL,"-mem_use",&flg);CHKERRQ(ierr);
  if (flg) {
    ierr = PetscMemoryView(PETSC_VIEWER_STDOUT_WORLD,"Memory usage after destroying\n");CHKERRQ(ierr);
  }
  PetscPreLoadEnd();
  ierr = AODestroy(&user.grid->ao);CHKERRQ(ierr);
  PetscPrintf(MPI_COMM_WORLD, "Time taken in gradient calculation is %g sec.\n",grad_time);
  PetscFinalize();
  return 0;
}

/*---------------------------------------------------------------------*/
/* ---------------------  Form initial approximation ----------------- */
int FormInitialGuess(SNES snes, GRID *grid)
/*---------------------------------------------------------------------*/
{
  PetscErrorCode ierr;
  PetscScalar    *qnode;
  PetscBool      flg;

  ierr = VecGetArray(grid->qnode,&qnode);CHKERRQ(ierr);
  f77INIT(&grid->nnodesLoc, qnode, grid->turbre,
          grid->amut, &grid->nvnodeLoc, grid->ivnode, &rank);
  ierr = VecRestoreArray(grid->qnode,&qnode);CHKERRQ(ierr);
  ierr = PetscOptionsGetBool(0,"-restart",&flg,NULL);CHKERRQ(ierr);
  if (flg) {
    ierr = ReadRestartFile(grid);CHKERRQ(ierr);
    PetscPrintf(MPI_COMM_WORLD,"Restarting from file restart.bin\n");
  }
  return 0;
}

/*---------------------------------------------------------------------*/
/* ---------------------  Evaluate Function F(x) --------------------- */
int FormFunction(SNES snes,Vec x,Vec f,void *dummy)
/*---------------------------------------------------------------------*/
{
  AppCtx      *user  = (AppCtx*) dummy;
  GRID        *grid  = user->grid;
  TstepCtx    *tsCtx = user->tsCtx;
  PetscScalar *qnode, *res, *qold;
  PetscScalar *grad;
  PetscScalar temp;
  VecScatter  scatter     = grid->scatter;
  VecScatter  gradScatter = grid->gradScatter;
  Vec         localX      = grid->qnodeLoc;
  Vec         localGrad   = grid->gradLoc;
  PetscInt    i,j,in,ierr;
  PetscInt    bs = 5;
  PetscInt    nbface, ires;
  PetscScalar time_ini, time_fin;

  ierr = VecScatterBegin(scatter,x,localX,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
  ierr = VecScatterEnd(scatter,x,localX,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
  /*{
    PetscScalar qNorm = 0.0, qNorm1 = 0.0;
    ierr = VecNorm(localX,NORM_2,&qNorm1);CHKERRQ(ierr);
    ierr = VecNorm(x,NORM_2,&qNorm);CHKERRQ(ierr);
    printf("Local qNorm is %g and global qNorm is %g\n",
          qNorm1, qNorm);
  }*/
  ires = tsCtx->ires;
  ierr = VecGetArray(f,&res);CHKERRQ(ierr);
  ierr = VecGetArray(localX,&qnode);CHKERRQ(ierr);

  if (SecondOrder) {
    ierr = VecGetArray(grid->grad,&grad);CHKERRQ(ierr);
    ierr = PetscTime(&time_ini);CHKERRQ(ierr);
    f77LSTGS(&grid->nnodesLoc,&grid->nedgeLoc,grid->eptr,
             qnode,grad,grid->x,grid->y,grid->z,
             grid->rxy,
             &rank,&grid->nvertices);
    ierr       = PetscTime(&time_fin);CHKERRQ(ierr);
    grad_time += time_fin - time_ini;
    ierr       = VecRestoreArray(grid->grad,&grad);CHKERRQ(ierr);
    ierr       = VecScatterBegin(gradScatter,grid->grad,localGrad,INSERT_VALUES,
                                 SCATTER_FORWARD);CHKERRQ(ierr);
    ierr = VecScatterEnd(gradScatter,grid->grad,localGrad,INSERT_VALUES,
                         SCATTER_FORWARD);CHKERRQ(ierr);
  }
  ierr = VecGetArray(localGrad,&grad);CHKERRQ(ierr);
  if ((!SecondOrder) && (c_info->ntt == 1)) {
    ierr = PetscMemzero(grad,3*bs*grid->nvertices*sizeof(REAL));CHKERRQ(ierr);
  }
  nbface = grid->nsface + grid->nvface + grid->nfface;
  f77GETRES(&grid->nnodesLoc,&grid->ncell,&grid->nedgeLoc,&grid->nsface,
            &grid->nvface,&grid->nfface,&nbface,
            &grid->nsnodeLoc,&grid->nvnodeLoc,&grid->nfnodeLoc,
            grid->isface,grid->ivface,grid->ifface,&grid->ileast,
            grid->isnode,grid->ivnode,grid->ifnode,
            &grid->nnfacetLoc,grid->f2ntn,&grid->nnbound,
            &grid->nvfacetLoc,grid->f2ntv,&grid->nvbound,
            &grid->nffacetLoc,grid->f2ntf,&grid->nfbound,grid->eptr,
            grid->sxn,grid->syn,grid->szn,
            grid->vxn,grid->vyn,grid->vzn,
            grid->fxn,grid->fyn,grid->fzn,
            grid->xn,grid->yn,grid->zn,grid->rl,
            qnode,grid->cdt,grid->x,grid->y,grid->z,grid->area,
            grad,res,grid->turbre,grid->slen,grid->c2n,grid->c2e,
            grid->us,grid->vs,grid->as,grid->phi,grid->amut,
            &ires,&rank,&grid->nvertices);

/* Add the contribution due to time stepping */
  if (ires == 1) {
    /*ierr = VecGetLocalSize(tsCtx->qold, &vecSize);CHKERRQ(ierr);
    printf("Size of local vector tsCtx->qold is %d\n", vecSize);*/
    ierr = VecGetArray(tsCtx->qold,&qold);CHKERRQ(ierr);
#if defined(INTERLACING)
    for (i = 0; i < grid->nnodesLoc; i++) {
      temp = grid->area[i]/(tsCtx->cfl*grid->cdt[i]);
      for (j = 0; j < bs; j++) {
        in       = bs*i + j;
        res[in] += temp*(qnode[in] - qold[in]);
      }
    }
#else
    for (j = 0; j < bs; j++) {
      for (i = 0; i < grid->nnodesLoc; i++) {
        temp     = grid->area[i]/(tsCtx->cfl*grid->cdt[i]);
        in       = grid->nnodesLoc*j + i;
        res[in] += temp*(qnode[in] - qold[in]);
      }
    }
#endif
    ierr = VecRestoreArray(tsCtx->qold,&qold);CHKERRQ(ierr);
  }

  ierr = VecRestoreArray(localX,&qnode);CHKERRQ(ierr);
  /*ierr = VecRestoreArray(grid->dq,&dq);CHKERRQ(ierr);*/
  ierr = VecRestoreArray(f,&res);CHKERRQ(ierr);
  ierr = VecRestoreArray(localGrad,&grad);CHKERRQ(ierr);
  /* if (ires == 1) {
    PetscScalar resNorm = 0.0, qNorm = 0.0, timeNorm = 0.0;
    for (i = 0; i < grid->nnodesLoc; i++)
      timeNorm+= grid->cdt[i]*grid->cdt[i];
    ierr = VecNorm(localX,NORM_2,&qNorm);CHKERRQ(ierr);
    ierr = VecNorm(f,NORM_2,&resNorm);CHKERRQ(ierr);
    printf("In FormFunction residual norm after TS is %g\n", resNorm);
    printf("qNorm and timeNorm are %g %g\n", qNorm, sqrt(timeNorm));
    printf("cfl is %g\n", tsCtx->cfl);
    }*/

  return 0;
}

/*---------------------------------------------------------------------*/
/* --------------------  Evaluate Jacobian F'(x) -------------------- */

int FormJacobian(SNES snes, Vec x, Mat Jac, Mat jac,void *dummy)
/*---------------------------------------------------------------------*/
{
  AppCtx         *user  = (AppCtx*) dummy;
  GRID           *grid  = user->grid;
  TstepCtx       *tsCtx = user->tsCtx;
  Vec            localX = grid->qnodeLoc;
  PetscScalar    *qnode;
  PetscErrorCode ierr;

  /*
  ierr = VecScatterBegin(scatter,x,localX,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
  ierr = VecScatterEnd(scatter,x,localX,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
  */
  ierr   = MatSetUnfactored(jac);CHKERRQ(ierr);
  ierr   = VecGetArray(localX,&qnode);CHKERRQ(ierr);
  /*ierr = MatZeroEntries(jac);CHKERRQ(ierr);*/

  f77FILLA(&grid->nnodesLoc,&grid->nedgeLoc,grid->eptr,&grid->nsface,
            grid->isface,grid->fxn,grid->fyn,grid->fzn,
            grid->sxn,grid->syn,grid->szn,&grid->nsnodeLoc,&grid->nvnodeLoc,
            &grid->nfnodeLoc,grid->isnode,grid->ivnode,grid->ifnode,
            qnode,&jac,grid->cdt,grid->rl,grid->area,grid->xn,grid->yn,grid->zn,
            &tsCtx->cfl,&rank,&grid->nvertices);

  /*ierr = PetscFortranObjectToCObject(ijac, &jac);CHKERRQ(ierr);*/
  /*ierr = MatView(jac,VIEWER_STDOUT_SELF);CHKERRQ(ierr);*/
  ierr  = VecRestoreArray(localX,&qnode);CHKERRQ(ierr);
  ierr  = MatAssemblyBegin(Jac,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr  = MatAssemblyEnd(Jac,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  return 0;
}

/*---------------------------------------------------------------------*/
int Update(SNES snes, void *ctx)
/*---------------------------------------------------------------------*/
{
  AppCtx      *user   = (AppCtx*) ctx;
  GRID        *grid   = user->grid;
  TstepCtx    *tsCtx  = user->tsCtx;
  VecScatter  scatter = grid->scatter;
  Vec         localX  = grid->qnodeLoc;
  PetscScalar *qnode, *res;
  PetscScalar clift = 0.0, cdrag = 0.0, cmom = 0.0;
  PetscErrorCode ierr;
  PetscInt         i, its;
  PetscScalar fratio;
  PetscScalar time1, time2, cpuloc, cpuglo;
  PetscInt         max_steps;
  FILE        *fptr;
  static PetscInt  PreLoadFlag    = 1;
  PetscInt         Converged      = 0;
  PetscInt         nfailsCum      = 0, nfails = 0;
  PetscScalar cfl_damp_ratio = 1.0e-02, cfl_damp_power = 0.75;
  PetscBool   print_flag     = PETSC_FALSE,cfl_damp_flag = PETSC_FALSE,flg;

  ierr = PetscOptionsGetBool(NULL,NULL,"-print", &print_flag,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsGetBool(NULL,NULL,"-cfl_damp", &cfl_damp_flag,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsGetReal(NULL,"-cfl_damp_ratio",&cfl_damp_ratio,&flg);CHKERRQ(ierr);
  ierr = PetscOptionsGetReal(NULL,"-cfl_damp_power",&cfl_damp_power,&flg);CHKERRQ(ierr);
  /*
  if (PreLoadFlag) {
    ierr = VecCopy(grid->qnode, tsCtx->qold);CHKERRQ(ierr);
    ierr = ComputeTimeStep(snes,i,user);CHKERRQ(ierr);
    ierr = SNESSolve(snes,grid->qnode,&its);CHKERRQ(ierr);
    tsCtx->fnorm_ini = 0.0;
    PetscPrintf(MPI_COMM_WORLD, "Preloading done ...\n");
    PreLoadFlag = 0;
  }
  */
  if ((!rank) && (print_flag)) {
    fptr = fopen("history.out", "w");
    fprintf(fptr, "VARIABLES = iter,cfl,fnorm,clift,cdrag,cmom,cpu\n");
  }
  if (PreLoadFlag) max_steps = 1;
  else max_steps = tsCtx->max_steps;
  fratio       = 1.0;
  tsCtx->ptime = 0.0;
  /*ierr = VecDuplicate(grid->qnode,&tsCtx->qold);CHKERRQ(ierr);
  ierr = VecGetLocalSize(tsCtx->qold, &vecSize);CHKERRQ(ierr);
  printf("Size of local vector tsCtx->qold is %d\n", vecSize);*/
  ierr = VecCopy(grid->qnode, tsCtx->qold);CHKERRQ(ierr);
  ierr = PetscTime(&time1);
  #if defined(PARCH_IRIX64) && defined(USE_HW_COUNTERS)
  /* if (!PreLoadFlag) {
    ierr = PetscOptionsGetInt(NULL,NULL,"-e0",&event0,&flg);CHKERRQ(ierr);
    ierr = PetscOptionsGetInt(NULL,NULL,"-e1",&event1,&flg);CHKERRQ(ierr);
    ierr = PetscTime(&time_start_counters);CHKERRQ(ierr);
    if ((gen_start = start_counters(event0,event1)) < 0)
    SETERRQ(PETSC_COMM_SELF,PETSC_ERR_PLIB,PETSC_ERROR_INITIAL,"Error in start_counters\n");
  } */
  #endif
  /*cpu_ini = PetscGetCPUTime();*/
  for (i = 0; ((i < max_steps) && ((!Converged) || (!SecondOrder))); i++) {
    /* For history file */
    c_info->ntt = i+1;
    if (c_runge->nitfo == 0) SecondOrder = 1;
    if (((c_info->ntt > c_runge->nitfo) || (Converged)) && (!SecondOrder)) {
      PetscPrintf(MPI_COMM_WORLD, "Switching to second order at time step %d ...\n",
                  i);
      SecondOrder      = 1;
      Converged        = 0;
      c_runge->nitfo   = c_info->ntt - 1;
      tsCtx->cfl_ini   = 1.25*tsCtx->cfl_ini;
      tsCtx->fnorm_ini = 0.0;
      tsCtx->cfl       = tsCtx->cfl_ini;
      fratio           = 1.0;
      /* Write Tecplot solution file */
      /* if (print_flag) {
        FieldOutput(grid, i);
        WriteRestartFile(grid,i);
      } */
    }
    ierr = ComputeTimeStep(snes,i,user);CHKERRQ(ierr);
    if (cfl_damp_flag) {
      if ((SecondOrder) && (fratio > cfl_damp_ratio)) {
        /*PetscScalar cfl_ramp = 3*tsCtx->cfl_ini;
        PetscPrintf(MPI_COMM_WORLD, "In Update : cfl_ramp, cfl = %g\t%g\n", cfl_ramp, tsCtx->cfl);
        tsCtx->cfl = PetscMin(tsCtx->cfl,cfl_ramp);*/

        PetscScalar cfl_ramp = PetscPowScalar(tsCtx->cfl,cfl_damp_power);
        tsCtx->cfl = PetscMax(tsCtx->cfl_ini,cfl_ramp);
        /*PetscPrintf(MPI_COMM_WORLD, "In Update : cfl_ramp, cfl = %g\t%g\n",
          cfl_ramp, tsCtx->cfl);*/
      }
    }
    /*tsCtx->ptime +=  tsCtx->dt;*/

    ierr = SNESSolve(snes,NULL,grid->qnode);CHKERRQ(ierr);
    ierr = SNESGetIterationNumber(snes,&its);CHKERRQ(ierr);
    ierr = SNESGetNonlinearStepFailures(snes, &nfails);CHKERRQ(ierr);
    nfailsCum += nfails; nfails = 0;
    if (nfailsCum >= 2) SETERRQ(PETSC_COMM_SELF,1,"Unable to find a Newton Step");
    if (print_flag)
      PetscPrintf(MPI_COMM_WORLD,"At Time Step %d cfl = %g and fnorm = %g\n", i,
                  tsCtx->cfl, tsCtx->fnorm);
    ierr = VecCopy(grid->qnode,tsCtx->qold);CHKERRQ(ierr);

    /* Timing Info */
    ierr = PetscTime(&time2);
    /*cpu_fin = PetscGetCPUTime();*/
    cpuloc = time2-time1;
    cpuglo = 0.0;
    ierr   = MPI_Allreduce(&cpuloc,&cpuglo,1,MPI_DOUBLE,MPI_MAX,MPI_COMM_WORLD);CHKERRQ(ierr);

    /*cpu_fin = PetscGetCPUTime();
    cpuloc = cpu_fin - cpu_ini;
    cpu_time = 0.0;
    ierr = MPI_Allreduce(&cpuloc,&cpu_time,1,MPI_DOUBLE,
                        MPI_MAX,MPI_COMM_WORLD);  */
    c_info->tot = cpuglo;    /* Total CPU time used upto this time step */

    /* Calculate Aerodynamic coeeficients at the current time step */
    ierr = VecScatterBegin(scatter,grid->qnode,localX,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
    ierr = VecScatterEnd(scatter,grid->qnode,localX,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);

    ierr = VecGetArray(grid->res, &res);CHKERRQ(ierr);
    ierr = VecGetArray(localX,&qnode);CHKERRQ(ierr);
    /*f77L2NORM(res, &grid->nnodesLoc, &grid->nnodes, grid->x,
              grid->y,    grid->z,
              grid->area, &rank);*/
    f77FORCE(&grid->nnodesLoc,&grid->nedgeLoc,
              grid->isnode,grid->ivnode,
             &grid->nnfacetLoc,grid->f2ntn,&grid->nnbound,
             &grid->nvfacetLoc,grid->f2ntv,&grid->nvbound,
              grid->eptr,qnode,grid->x,grid->y,grid->z,
             &grid->nvnodeLoc,grid->c2n,&grid->ncell,
              grid->amut,grid->sface_bit,grid->vface_bit,
              &clift,&cdrag,&cmom,&rank,&grid->nvertices);
    ierr = VecRestoreArray(localX, &qnode);CHKERRQ(ierr);
    ierr = VecRestoreArray(grid->res, &res);CHKERRQ(ierr);
    /*PetscPrintf(MPI_COMM_WORLD,"%d\t%15.8e\t%15.8e\t%15.8e\t%15.8e\t%15.8e\t%15.8e\n",
            i, tsCtx->cfl, tsCtx->fnorm, clift, cdrag, cmom, cpuglo);*/
    if (print_flag) {
      PetscPrintf(MPI_COMM_WORLD, "%d\t%g\t%g\t%g\t%g\t%g\n", i,
                  tsCtx->cfl, tsCtx->fnorm, clift, cdrag, cmom);
      PetscPrintf(MPI_COMM_WORLD,"Wall clock time needed %g seconds for %d time steps\n",
                  cpuglo, i);
      if (!rank)
        fprintf(fptr, "%d\t%g\t%g\t%g\t%g\t%g\t%g\n",
                i, tsCtx->cfl, tsCtx->fnorm, clift, cdrag, cmom, cpuglo);
      /* Write Tecplot solution file */
      if ((i != 0) && ((i % tsCtx->print_freq) == 0)) {
        FieldOutput(grid, i);
        WriteRestartFile(grid,i);
      }
    }
    fratio = tsCtx->fnorm/tsCtx->fnorm_ini;
    if (((!SecondOrder) && (fratio <= tsCtx->fnorm_fo_rtol)) ||
        ((fratio <= tsCtx->fnorm_rtol) || (fratio <= tsCtx->fnorm_atol))) Converged = 1;
    ierr = MPI_Barrier(MPI_COMM_WORLD);
  } /* End of time step loop */
  #if defined(PARCH_IRIX64) && defined(USE_HW_COUNTERS)
  if (!PreLoadFlag) {
    int  eve0, eve1;
    FILE *cfp0, *cfp1;
    char str[256];
    /* if ((gen_read = read_counters(event0,&counter0,event1,&counter1)) < 0)
    SETERRQ(PETSC_COMM_SELF,1,"Error in read_counter\n");
    ierr = PetscTime(&time_read_counters);CHKERRQ(ierr);
    if (gen_read != gen_start) {
    SETERRQ(PETSC_COMM_SELF,1,"Lost Counters!! Aborting ...\n");
    }*/
    /*sprintf(str,"counters%d_and_%d",event0,event1);
    cfp0 = fopen(str,"a");*/
    /*ierr = print_counters(event0,counter0,event1,counter1);*/
    /*fprintf(cfp0,"%lld %lld %g\n",counter0,counter1,
                  time_counters);
    fclose(cfp0);*/
  }
  #endif
  PetscPrintf(MPI_COMM_WORLD,"Total wall clock time needed %g seconds for %d time steps\n",
              cpuglo, i);
  PetscPrintf(MPI_COMM_WORLD, "cfl = %g fnorm = %g\n",
              tsCtx->cfl, tsCtx->fnorm);
  PetscPrintf(MPI_COMM_WORLD, "clift = %g cdrag = %g cmom = %g\n",
              clift, cdrag, cmom);
  /*PetscPrintf(MPI_COMM_WORLD, "Total cpu time needed %g seconds\n", cpu_time);*/
  /* if (!PreLoadFlag) {
      FieldOutput(grid, i);
      WriteRestartFile(grid,i);
  }*/
  if ((!rank) && (print_flag)) fclose(fptr);
  if (PreLoadFlag) {
    tsCtx->fnorm_ini = 0.0;
    PetscPrintf(MPI_COMM_WORLD, "Preloading done ...\n");
    PreLoadFlag = 0;
  }

  return 0;
}

/*---------------------------------------------------------------------*/
int ComputeTimeStep(SNES snes, int iter, void *ctx)
/*---------------------------------------------------------------------*/
{
  AppCtx *user = (AppCtx*) ctx;
  /*GRID *grid = user->grid;*/
  TstepCtx *tsCtx = user->tsCtx;
  Vec      func   = tsCtx->func;
  double   inc    = 1.1;
  double   newcfl, fnorm;
  PetscErrorCode      ierr;
  /*int     iramp = tsCtx->iramp;*/


  /*VecDuplicate(tsCtx->qold,&func);*/
  /* Set the flag to calculate the local time step in GETRES */
  tsCtx->ires = 0;
  ierr        = FormFunction(snes,tsCtx->qold,func,user);CHKERRQ(ierr);
/*
 *ierr = VecGetArray(func, &res);CHKERRQ(ierr);
 *f77L2NORM(res, &grid->nnodesLoc, &grid->nnodes, grid->x,
 *          grid->y,    grid->z,
 *          grid->area, &rank);
 *ierr = VecRestoreArray(func, &res);CHKERRQ(ierr);
 */
  tsCtx->ires = 1;
  ierr        = VecNorm(func,NORM_2,&tsCtx->fnorm);CHKERRQ(ierr);
  fnorm       = tsCtx->fnorm;
  if (!SecondOrder) inc = 2.0;
  /* if (iter == 0) {*/
  if (tsCtx->fnorm_ini == 0.0) {
    /* first time through so compute initial function norm */
    tsCtx->fnorm_ini = fnorm;
    tsCtx->cfl       = tsCtx->cfl_ini;
  } else {
    newcfl     = inc*tsCtx->cfl_ini*tsCtx->fnorm_ini/fnorm;
    tsCtx->cfl = PetscMin(newcfl, tsCtx->cfl_max);
  }

  /* if (iramp < 0) {
   newcfl = inc*tsCtx->cfl_ini*tsCtx->fnorm_ini/fnorm;
  } else {
   if (tsCtx->dt < 0 && iramp > 0)
    if (iter > iramp) newcfl = tsCtx->cfl_max;
    else newcfl = tsCtx->cfl_ini + (tsCtx->cfl_max - tsCtx->cfl_ini)*
                                (double) iter/(double) iramp;
  }
  tsCtx->cfl = PetscMin(newcfl, tsCtx->cfl_max);*/
  /*PetscPrintf(MPI_COMM_WORLD,"In ComputeTimeStep - fnorm, and cfl are  %g %g\n",
                fnorm,tsCtx->cfl);*/
  /*ierr = VecDestroy(&func);CHKERRQ(ierr);*/
  return 0;
}

/*---------------------------------------------------------------------*/
int GetLocalOrdering(GRID *grid)
/*---------------------------------------------------------------------*/
{
  PetscErrorCode   ierr;
  PetscInt         i, j, k, inode, isurf, nno, nte, nb, node1, node2, node3;
  PetscInt         nnodes, nedge, nnz, jstart, jend;
  PetscInt         nnodesLoc, nvertices, nedgeLoc,nnodesLocEst;
  PetscInt         nedgeLocEst, remEdges, readEdges, remNodes, readNodes;
  PetscInt         nnfacet, nvfacet, nffacet;
  PetscInt         nnfacetLoc, nvfacetLoc, nffacetLoc;
  PetscInt         nsnode, nvnode, nfnode;
  PetscInt         nsnodeLoc, nvnodeLoc, nfnodeLoc;
  PetscInt         nnbound, nvbound, nfbound;
  PetscInt         bs = 5;
  PetscInt         fdes = 0;
  off_t       currentPos  = 0, newPos = 0;
  PetscInt         grid_param  = 13;
  /* PetscInt         cross_edges = 0;*/
  PetscInt         *edge_bit, *pordering;
  PetscInt         *l2p, *l2a, *p2l, *a2l, *v2p, *eperm;
  PetscInt         *tmp, *tmp1, *tmp2;
  PetscScalar time_ini, time_fin;
  PetscScalar *ftmp;
  char        mesh_file[PETSC_MAX_PATH_LEN];
  PetscBool   flg;
  FILE        *fptr = NULL, *fptr1 = NULL;
  MPI_Comm    comm = PETSC_COMM_WORLD;
  /*AO        ao;*/

  /* Read the integer grid parameters */
  ICALLOC(grid_param,&tmp);
  if (!rank) {
    PetscBool exists;
    ierr = PetscOptionsGetString(NULL,NULL,"-mesh",mesh_file,256,&flg);CHKERRQ(ierr);
    ierr = PetscTestFile(mesh_file,'r',&exists);CHKERRQ(ierr);
    if (!exists) { /* try uns3d.msh as the file name */
      ierr = PetscStrcpy(mesh_file,"uns3d.msh");CHKERRQ(ierr);
    }
    ierr = PetscBinaryOpen(mesh_file,FILE_MODE_READ,&fdes);CHKERRQ(ierr);
  }
  ierr          = PetscBinarySynchronizedRead(comm,fdes,tmp,grid_param,PETSC_INT);CHKERRQ(ierr);
  grid->ncell   = tmp[0];
  grid->nnodes  = tmp[1];
  grid->nedge   = tmp[2];
  grid->nnbound = tmp[3];
  grid->nvbound = tmp[4];
  grid->nfbound = tmp[5];
  grid->nnfacet = tmp[6];
  grid->nvfacet = tmp[7];
  grid->nffacet = tmp[8];
  grid->nsnode  = tmp[9];
  grid->nvnode  = tmp[10];
  grid->nfnode  = tmp[11];
  grid->ntte    = tmp[12];
  grid->nsface  = 0;
  grid->nvface  = 0;
  grid->nfface  = 0;
  ierr          = PetscFree(tmp);CHKERRQ(ierr);
  PetscPrintf(PETSC_COMM_WORLD, "nnodes = %d, nedge = %d, nnfacet = %d, nsnode = %d, nfnode = %d\n",
              grid->nnodes,grid->nedge,grid->nnfacet,grid->nsnode,grid->nfnode);
/* Until the above line, the equivalent I/O and other initialization
  work of RDGPAR has been done */
  nnodes  = grid->nnodes;
  nedge   = grid->nedge;
  nnfacet = grid->nnfacet;
  nvfacet = grid->nvfacet;
  nffacet = grid->nffacet;
  nnbound = grid->nnbound;
  nvbound = grid->nvbound;
  nfbound = grid->nfbound;
  nsnode  = grid->nsnode;
  nvnode  = grid->nvnode;
  nfnode  = grid->nfnode;

  /* Read the partitioning vector generated by MeTiS */
  ICALLOC(nnodes, &l2a);
  ICALLOC(nnodes, &v2p);
  ICALLOC(nnodes, &a2l);
  nnodesLoc = 0;

  for (i = 0; i < nnodes; i++) a2l[i] = -1;
  ierr = PetscTime(&time_ini);CHKERRQ(ierr);

  if (!rank) {
    if (CommSize == 1) {
      ierr = PetscMemzero(v2p,nnodes*sizeof(int));CHKERRQ(ierr);
    } else {
      char      spart_file[PETSC_MAX_PATH_LEN],part_file[PETSC_MAX_PATH_LEN];
      PetscBool exists;

      ierr = PetscOptionsGetString(NULL,NULL,"-partition",spart_file,PETSC_MAX_PATH_LEN,&flg);CHKERRQ(ierr);
      ierr = PetscTestFile(spart_file,'r',&exists);CHKERRQ(ierr);
      if (!exists) { /* try appending the number of processors */
        sprintf(part_file,"part_vec.part.%d",CommSize);
        ierr = PetscStrcpy(spart_file,part_file);CHKERRQ(ierr);
      }
      fptr = fopen(spart_file,"r");
      if (!fptr) SETERRQ1(PETSC_COMM_SELF,1,"Cannot open file %s\n",part_file);
      for (inode = 0; inode < nnodes; inode++) {
        fscanf(fptr,"%d\n",&node1);
        v2p[inode] = node1;
      }
      fclose(fptr);
    }
  }
  ierr = MPI_Bcast(v2p,nnodes,MPI_INT,0,comm);CHKERRQ(ierr);
  for (inode = 0; inode < nnodes; inode++) {
    if (v2p[inode] == rank) {
      l2a[nnodesLoc] = inode ;
      a2l[inode]     = nnodesLoc ;
      nnodesLoc++;
    }
  }

  ierr      = PetscTime(&time_fin);CHKERRQ(ierr);
  time_fin -= time_ini;
  ierr      = PetscPrintf(comm,"Partition Vector read successfully\n");CHKERRQ(ierr);
  ierr      = PetscPrintf(comm,"Time taken in this phase was %g\n",time_fin);CHKERRQ(ierr);
  ierr      = MPI_Scan(&nnodesLoc,&rstart,1,MPI_INT,MPI_SUM,MPI_COMM_WORLD);CHKERRQ(ierr);
  rstart   -= nnodesLoc;
  ICALLOC(nnodesLoc, &pordering);
  for (i=0; i < nnodesLoc; i++) pordering[i] = rstart + i;
  ierr = AOCreateBasic(MPI_COMM_WORLD,nnodesLoc,l2a,pordering,&grid->ao);CHKERRQ(ierr);
  ierr = PetscFree(pordering);CHKERRQ(ierr);

  /* Now count the local number of edges - including edges with
   ghost nodes but edges between ghost nodes are NOT counted */
  nedgeLoc  = 0;
  nvertices = nnodesLoc;
  /* Choose an estimated number of local edges. The choice
   nedgeLocEst = 1000000 looks reasonable as it will read
   the edge and edge normal arrays in 8 MB chunks */
  /*nedgeLocEst = nedge/size;*/
  nedgeLocEst = PetscMin(nedge,1000000);
  remEdges    = nedge;
  ICALLOC(2*nedgeLocEst,&tmp);
  ierr = PetscBinarySynchronizedSeek(comm,fdes,0,PETSC_BINARY_SEEK_CUR,&currentPos);CHKERRQ(ierr);
  ierr = PetscTime(&time_ini);CHKERRQ(ierr);
  while (remEdges > 0) {
    readEdges = PetscMin(remEdges,nedgeLocEst);
    /*time_ini = PetscTime();*/
    ierr = PetscBinarySynchronizedRead(comm,fdes,tmp,readEdges,PETSC_INT);CHKERRQ(ierr);
    ierr = PetscBinarySynchronizedSeek(comm,fdes,(nedge-readEdges)*PETSC_BINARY_INT_SIZE,PETSC_BINARY_SEEK_CUR,&newPos);CHKERRQ(ierr);
    ierr = PetscBinarySynchronizedRead(comm,fdes,tmp+readEdges,readEdges,PETSC_INT);CHKERRQ(ierr);
    ierr = PetscBinarySynchronizedSeek(comm,fdes,-nedge*PETSC_BINARY_INT_SIZE,PETSC_BINARY_SEEK_CUR,&newPos);CHKERRQ(ierr);
    /*time_fin += PetscTime()-time_ini;*/
    for (j = 0; j < readEdges; j++) {
      node1 = tmp[j]-1;
      node2 = tmp[j+readEdges]-1;
      if ((v2p[node1] == rank) || (v2p[node2] == rank)) {
        nedgeLoc++;
        if (a2l[node1] == -1) {
          l2a[nvertices] = node1;
          a2l[node1]     = nvertices;
          nvertices++;
        }
        if (a2l[node2] == -1) {
          l2a[nvertices] = node2;
          a2l[node2]     = nvertices;
          nvertices++;
        }
      }
    }
    remEdges = remEdges - readEdges;
    ierr     = MPI_Barrier(comm);
  }
  ierr      = PetscTime(&time_fin);CHKERRQ(ierr);
  time_fin -= time_ini;
  ierr      = PetscPrintf(comm,"Local edges counted with MPI_Bcast %d\n",nedgeLoc);CHKERRQ(ierr);
  ierr      = PetscPrintf(comm,"Local vertices counted %d\n",nvertices);CHKERRQ(ierr);
  ierr      = PetscPrintf(comm,"Time taken in this phase was %g\n",time_fin);CHKERRQ(ierr);

  /* Now store the local edges */
  ICALLOC(2*nedgeLoc,&grid->eptr);
  ICALLOC(nedgeLoc,&edge_bit);
  ICALLOC(nedgeLoc,&eperm);
  i          = 0; j = 0; k = 0;
  remEdges   = nedge;
  ierr       = PetscBinarySynchronizedSeek(comm,fdes,currentPos,PETSC_BINARY_SEEK_SET,&newPos);CHKERRQ(ierr);
  currentPos = newPos;

  ierr = PetscTime(&time_ini);CHKERRQ(ierr);
  while (remEdges > 0) {
    readEdges = PetscMin(remEdges,nedgeLocEst);
    ierr      = PetscBinarySynchronizedRead(comm,fdes,tmp,readEdges,PETSC_INT);CHKERRQ(ierr);
    ierr      = PetscBinarySynchronizedSeek(comm,fdes,(nedge-readEdges)*PETSC_BINARY_INT_SIZE,PETSC_BINARY_SEEK_CUR,&newPos);CHKERRQ(ierr);
    ierr      = PetscBinarySynchronizedRead(comm,fdes,tmp+readEdges,readEdges,PETSC_INT);CHKERRQ(ierr);
    ierr      = PetscBinarySynchronizedSeek(comm,fdes,-nedge*PETSC_BINARY_INT_SIZE,PETSC_BINARY_SEEK_CUR,&newPos);CHKERRQ(ierr);
    for (j = 0; j < readEdges; j++) {
      node1 = tmp[j]-1;
      node2 = tmp[j+readEdges]-1;
      if ((v2p[node1] == rank) || (v2p[node2] == rank)) {
        grid->eptr[k]          = a2l[node1];
        grid->eptr[k+nedgeLoc] = a2l[node2];
        edge_bit[k]            = i;
        eperm[k]               = k;
        k++;
      }
      i++;
    }
    remEdges = remEdges - readEdges;
    ierr     = MPI_Barrier(comm);
  }
  ierr = PetscBinarySynchronizedSeek(comm,fdes,currentPos+2*nedge*PETSC_BINARY_INT_SIZE,PETSC_BINARY_SEEK_SET,&newPos);CHKERRQ(ierr);
  ierr = PetscTime(&time_fin);CHKERRQ(ierr);
  time_fin -= time_ini;
  ierr = PetscPrintf(comm,"Local edges stored\n");CHKERRQ(ierr);
  ierr = PetscPrintf(comm,"Time taken in this phase was %g\n",time_fin);CHKERRQ(ierr);

  ierr = PetscFree(tmp);CHKERRQ(ierr);
  ICALLOC(2*nedgeLoc, &tmp);
  /* Now reorder the edges for better cache locality */
  /*
  tmp[0]=7;tmp[1]=6;tmp[2]=3;tmp[3]=9;tmp[4]=2;tmp[5]=0;
  ierr = PetscSortIntWithPermutation(6, tmp, eperm);
  for (i=0; i<6; i++)
   printf("%d %d %d\n", i, tmp[i], eperm[i]);
  */
  ierr = PetscMemcpy(tmp,grid->eptr,2*nedgeLoc*sizeof(int));CHKERRQ(ierr);
  flg  = PETSC_FALSE;
  ierr = PetscOptionsGetBool(0,"-no_edge_reordering",&flg,NULL);CHKERRQ(ierr);
  if (!flg) {
    ierr = PetscSortIntWithPermutation(nedgeLoc,tmp,eperm);CHKERRQ(ierr);
  }
  ierr = PetscMallocValidate(__LINE__,__FUNCT__,__FILE__);CHKERRQ(ierr);
  k = 0;
  for (i = 0; i < nedgeLoc; i++) {
#if defined(INTERLACING)
    /*int cross_node=nnodesLoc/2;*/
    grid->eptr[k++] = tmp[eperm[i]] + 1;
    grid->eptr[k++] = tmp[nedgeLoc+eperm[i]] + 1;
#else
    grid->eptr[i]          = tmp[eperm[i]] + 1;
    grid->eptr[nedgeLoc+i] = tmp[nedgeLoc+eperm[i]] + 1;
#endif
    /* if (node1 > node2)
     printf("On processor %d, for edge %d node1 = %d, node2 = %d\n",
            rank,i,node1,node2);CHKERRQ(ierr);
    if ((node1 <= cross_node) && (node2 > cross_node)) cross_edges++;*/
  }
  /*ierr = PetscPrintf(comm,"Number of cross edges %d\n", cross_edges);CHKERRQ(ierr);*/
  ierr = PetscFree(tmp);CHKERRQ(ierr);

  /* Now make the local 'ia' and 'ja' arrays */
  ICALLOC(nnodesLoc+1, &grid->ia);
  /* Use tmp for a work array */
  ICALLOC(nnodesLoc, &tmp);
  f77GETIA(&nnodesLoc, &nedgeLoc, grid->eptr,
           grid->ia, tmp, &rank);
  nnz = grid->ia[nnodesLoc] - 1;

#if defined(BLOCKING)
  ierr = PetscPrintf(comm,"The Jacobian has %d non-zero blocks with block size = %d\n",nnz,bs);CHKERRQ(ierr);
#else
  ierr = PetscPrintf(comm,"The Jacobian has %d non-zeros\n",nnz);CHKERRQ(ierr);
#endif
  ICALLOC(nnz, &grid->ja);
  f77GETJA(&nnodesLoc, &nedgeLoc, grid->eptr,
           grid->ia,      grid->ja,    tmp, &rank);

  ierr = PetscFree(tmp);CHKERRQ(ierr);

  ICALLOC(nvertices, &grid->loc2glo);
  ierr = PetscMemcpy(grid->loc2glo,l2a,nvertices*sizeof(int));CHKERRQ(ierr);
  ierr = PetscFree(l2a);CHKERRQ(ierr);
  l2a  = grid->loc2glo;
  ICALLOC(nvertices, &grid->loc2pet);
  l2p  = grid->loc2pet;
  ierr = PetscMemcpy(l2p,l2a,nvertices*sizeof(int));CHKERRQ(ierr);CHKERRQ(ierr);
  ierr = AOApplicationToPetsc(grid->ao,nvertices,l2p);CHKERRQ(ierr);

/* Map the 'ja' array in petsc ordering */
  nnz = grid->ia[nnodesLoc] - 1;
  for (i = 0; i < nnz; i++) grid->ja[i] = l2a[grid->ja[i] - 1];
  ierr = AOApplicationToPetsc(grid->ao,nnz,grid->ja);CHKERRQ(ierr);
  /*ierr = AODestroy(&ao);CHKERRQ(ierr);*/

  /* Renumber unit normals of dual face (from node1 to node2)
      and the area of the dual mesh face */

  /* Do the x-component */
  FCALLOC(nedgeLocEst, &ftmp);
  FCALLOC(nedgeLoc, &grid->xn);
  i = 0; k = 0;
  remEdges = nedge;
  ierr     = PetscTime(&time_ini);CHKERRQ(ierr);
  while (remEdges > 0) {
    readEdges = PetscMin(remEdges,nedgeLocEst);
    ierr = PetscBinarySynchronizedRead(comm,fdes,ftmp,readEdges,PETSC_SCALAR);CHKERRQ(ierr);
    for (j = 0; j < readEdges; j++) {
      if (edge_bit[k] == (i+j)) {
        grid->xn[k] = ftmp[j];
        k++;
      }
    }
    i+= readEdges;
    remEdges = remEdges - readEdges;
    ierr     = MPI_Barrier(MPI_COMM_WORLD);CHKERRQ(ierr);
  }
  ierr = PetscFree(ftmp);CHKERRQ(ierr);
  FCALLOC(nedgeLoc, &ftmp);
  ierr = PetscMemcpy(ftmp,grid->xn,nedgeLoc*sizeof(REAL));CHKERRQ(ierr);
  for (i = 0; i < nedgeLoc; i++) grid->xn[i] = ftmp[eperm[i]];
  ierr = PetscFree(ftmp);CHKERRQ(ierr);

  /* Do the y-component */
  FCALLOC(nedgeLocEst, &ftmp);
  FCALLOC(nedgeLoc, &grid->yn);
  i = 0; k = 0;
  remEdges = nedge;
  while (remEdges > 0) {
    readEdges = PetscMin(remEdges,nedgeLocEst);
    ierr = PetscBinarySynchronizedRead(comm,fdes,ftmp,readEdges,PETSC_SCALAR);CHKERRQ(ierr);
    for (j = 0; j < readEdges; j++) {
      if (edge_bit[k] == (i+j)) {
        grid->yn[k] = ftmp[j];
        k++;
      }
    }
    i+= readEdges;
    remEdges = remEdges - readEdges;
    ierr     = MPI_Barrier(MPI_COMM_WORLD);CHKERRQ(ierr);
  }
  ierr = PetscFree(ftmp);CHKERRQ(ierr);
  FCALLOC(nedgeLoc, &ftmp);
  ierr = PetscMemcpy(ftmp,grid->yn,nedgeLoc*sizeof(REAL));CHKERRQ(ierr);
  for (i = 0; i < nedgeLoc; i++) grid->yn[i] = ftmp[eperm[i]];
  ierr = PetscFree(ftmp);CHKERRQ(ierr);

  /* Do the z-component */
  FCALLOC(nedgeLocEst, &ftmp);
  FCALLOC(nedgeLoc, &grid->zn);
  i = 0; k = 0;
  remEdges = nedge;
  while (remEdges > 0) {
    readEdges = PetscMin(remEdges,nedgeLocEst);
    ierr = PetscBinarySynchronizedRead(comm,fdes,ftmp,readEdges,PETSC_SCALAR);CHKERRQ(ierr);
    for (j = 0; j < readEdges; j++) {
      if (edge_bit[k] == (i+j)) {
        grid->zn[k] = ftmp[j];
        k++;
      }
    }
    i+= readEdges;
    remEdges = remEdges - readEdges;
    ierr     = MPI_Barrier(MPI_COMM_WORLD);CHKERRQ(ierr);
  }
  ierr = PetscFree(ftmp);CHKERRQ(ierr);
  FCALLOC(nedgeLoc, &ftmp);
  ierr = PetscMemcpy(ftmp,grid->zn,nedgeLoc*sizeof(REAL));CHKERRQ(ierr);
  for (i = 0; i < nedgeLoc; i++) grid->zn[i] = ftmp[eperm[i]];
  ierr = PetscFree(ftmp);CHKERRQ(ierr);

  /* Do the length */
  FCALLOC(nedgeLocEst, &ftmp);
  FCALLOC(nedgeLoc, &grid->rl);
  i = 0; k = 0;
  remEdges = nedge;
  while (remEdges > 0) {
    readEdges = PetscMin(remEdges,nedgeLocEst);
    ierr = PetscBinarySynchronizedRead(comm,fdes,ftmp,readEdges,PETSC_SCALAR);CHKERRQ(ierr);
    for (j = 0; j < readEdges; j++) {
      if (edge_bit[k] == (i+j)) {
        grid->rl[k] = ftmp[j];
        k++;
      }
    }
    i+= readEdges;
    remEdges = remEdges - readEdges;
    ierr     = MPI_Barrier(MPI_COMM_WORLD);CHKERRQ(ierr);
  }
  ierr = PetscFree(ftmp);CHKERRQ(ierr);
  FCALLOC(nedgeLoc, &ftmp);
  ierr = PetscMemcpy(ftmp,grid->rl,nedgeLoc*sizeof(REAL));CHKERRQ(ierr);
  for (i = 0; i < nedgeLoc; i++) grid->rl[i] = ftmp[eperm[i]];

  ierr = PetscFree(edge_bit);CHKERRQ(ierr);
  ierr = PetscFree(eperm);CHKERRQ(ierr);
  ierr = PetscFree(ftmp);CHKERRQ(ierr);
  ierr = PetscTime(&time_fin);
  time_fin -= time_ini;
  ierr = PetscPrintf(comm,"Edge normals partitioned\n");CHKERRQ(ierr);
  ierr = PetscPrintf(comm,"Time taken in this phase was %g\n",time_fin);CHKERRQ(ierr);

  /* Remap coordinates */
  nnodesLocEst = nnodes/CommSize;
  FCALLOC(nnodesLocEst, &ftmp);
  FCALLOC(nvertices, &grid->x);
  remNodes = nnodes;
  i = 0;
  ierr = PetscTime(&time_ini);CHKERRQ(ierr);
  while (remNodes > 0) {
    readNodes = PetscMin(remNodes, nnodesLocEst);
    ierr = PetscBinarySynchronizedRead(comm,fdes,ftmp,readNodes,PETSC_SCALAR);CHKERRQ(ierr);
    for (j = 0; j < readNodes; j++) {
      if (a2l[i+j] >= 0) {
        grid->x[a2l[i+j]] = ftmp[j];
      }
    }
    i+= nnodesLocEst;
    remNodes -= nnodesLocEst;
    ierr = MPI_Barrier(MPI_COMM_WORLD);CHKERRQ(ierr);
  }

  FCALLOC(nvertices, &grid->y);
  remNodes = nnodes;
  i = 0;
  while (remNodes > 0) {
    readNodes = PetscMin(remNodes,nnodesLocEst);
    ierr = PetscBinarySynchronizedRead(comm,fdes, ftmp, readNodes, PETSC_SCALAR);CHKERRQ(ierr);
    for (j = 0; j < readNodes; j++) {
      if (a2l[i+j] >= 0) {
        grid->y[a2l[i+j]] = ftmp[j];
      }
    }
    i+= nnodesLocEst;
    remNodes -= nnodesLocEst;
    ierr      = MPI_Barrier(MPI_COMM_WORLD);CHKERRQ(ierr);
  }

  FCALLOC(nvertices, &grid->z);
  remNodes = nnodes;
  i = 0;
  while (remNodes > 0) {
    readNodes = PetscMin(remNodes,nnodesLocEst);
    ierr = PetscBinarySynchronizedRead(comm,fdes,ftmp,readNodes, PETSC_SCALAR);CHKERRQ(ierr);
    for (j = 0; j < readNodes; j++) {
      if (a2l[i+j] >= 0) {
        grid->z[a2l[i+j]] = ftmp[j];
      }
    }
    i+= nnodesLocEst;
    remNodes -= nnodesLocEst;
    ierr      = MPI_Barrier(MPI_COMM_WORLD);CHKERRQ(ierr);
  }


  /* Renumber dual volume */
  FCALLOC(nvertices, &grid->area);
  remNodes = nnodes;
  i = 0;
  while (remNodes > 0) {
    readNodes = PetscMin(remNodes,nnodesLocEst);
    ierr = PetscBinarySynchronizedRead(comm,fdes, ftmp, readNodes, PETSC_SCALAR);CHKERRQ(ierr);
    for (j = 0; j < readNodes; j++) {
      if (a2l[i+j] >= 0) {
        grid->area[a2l[i+j]] = ftmp[j];
      }
    }
    i+= nnodesLocEst;
    remNodes -= nnodesLocEst;
    ierr = MPI_Barrier(MPI_COMM_WORLD);CHKERRQ(ierr);
  }

  ierr = PetscFree(ftmp);CHKERRQ(ierr);
  ierr = PetscTime(&time_fin);
  time_fin -= time_ini;
  ierr = PetscPrintf(comm,"Coordinates remapped\n");CHKERRQ(ierr);
  ierr = PetscPrintf(comm,"Time taken in this phase was %g\n",time_fin);CHKERRQ(ierr);

/* Now, handle all the solid boundaries - things to be done :
 * 1. Identify the nodes belonging to the solid
 *    boundaries and count them.
 * 2. Put proper indices into f2ntn array, after making it
 *    of suitable size.
 * 3. Remap the normals and areas of solid faces (sxn, syn, szn,
 *    and sa arrays).
 */
  ICALLOC(nnbound,   &grid->nntet);
  ICALLOC(nnbound,   &grid->nnpts);
  ICALLOC(4*nnfacet, &grid->f2ntn);
  ICALLOC(nsnode, &grid->isnode);
  FCALLOC(nsnode, &grid->sxn);
  FCALLOC(nsnode, &grid->syn);
  FCALLOC(nsnode, &grid->szn);
  FCALLOC(nsnode, &grid->sa);
  ierr = PetscBinarySynchronizedSeek(comm,fdes,0,PETSC_BINARY_SEEK_CUR,&solidBndPos);CHKERRQ(ierr);
  ierr = PetscBinarySynchronizedRead(comm,fdes,grid->nntet,nnbound,PETSC_INT);CHKERRQ(ierr);
  ierr = PetscBinarySynchronizedRead(comm,fdes,grid->nnpts,nnbound,PETSC_INT);CHKERRQ(ierr);
  ierr = PetscBinarySynchronizedRead(comm,fdes,grid->f2ntn,4*nnfacet,PETSC_INT);CHKERRQ(ierr);
  ierr = PetscBinarySynchronizedRead(comm,fdes,grid->isnode,nsnode,PETSC_INT);CHKERRQ(ierr);
  ierr = PetscBinarySynchronizedRead(comm,fdes,grid->sxn,nsnode,PETSC_SCALAR);CHKERRQ(ierr);
  ierr = PetscBinarySynchronizedRead(comm,fdes,grid->syn,nsnode,PETSC_SCALAR);CHKERRQ(ierr);
  ierr = PetscBinarySynchronizedRead(comm,fdes,grid->szn,nsnode,PETSC_SCALAR);CHKERRQ(ierr);

  ICALLOC(3*nnfacet, &tmp);
  ICALLOC(nsnode, &tmp1);
  ICALLOC(nnodes, &tmp2);
  FCALLOC(4*nsnode, &ftmp);
  ierr = PetscMemzero(tmp,3*nnfacet*sizeof(int));CHKERRQ(ierr);
  ierr = PetscMemzero(tmp1,nsnode*sizeof(int));CHKERRQ(ierr);
  ierr = PetscMemzero(tmp2,nnodes*sizeof(int));CHKERRQ(ierr);

  isurf      = 0;
  inode      = 0;
  nsnodeLoc  = 0;
  nnfacetLoc = 0;
  nb         = 0;
  nte        = 0;
  nno        = 0;
  j          = 0;
  for (i = 0; i < nnbound; i++) {
    for (k = inode; k < inode+grid->nnpts[i]; k++) {
      node1 = a2l[grid->isnode[k] - 1];
      if (node1 >= 0) {
        tmp1[nsnodeLoc] = node1;
        tmp2[node1]     = nsnodeLoc;
        ftmp[j++]       = grid->sxn[k];
        ftmp[j++]       = grid->syn[k];
        ftmp[j++]       = grid->szn[k];
        ftmp[j++]       = grid->sa[k];
        nsnodeLoc++;
        nno++;
      }
    }
    for (k = isurf; k < isurf + grid->nntet[i]; k++) {
      node1 = a2l[grid->isnode[grid->f2ntn[k] - 1] - 1];
      node2 = a2l[grid->isnode[grid->f2ntn[nnfacet + k] - 1] - 1];
      node3 = a2l[grid->isnode[grid->f2ntn[2*nnfacet + k] - 1] - 1];

      if ((node1 >= 0) && (node2 >= 0) && (node3 >= 0)) {
        nnfacetLoc++;
        nte++;
        tmp[nb++] = tmp2[node1];
        tmp[nb++] = tmp2[node2];
        tmp[nb++] = tmp2[node3];
      }
    }
    inode += grid->nnpts[i];
    isurf += grid->nntet[i];
    /*printf("grid->nntet[%d] before reordering is %d\n",i,grid->nntet[i]);*/
    grid->nnpts[i] = nno;
    grid->nntet[i] = nte;
    /*printf("grid->nntet[%d] after reordering is %d\n",i,grid->nntet[i]);*/
    nno = 0;
    nte = 0;
  }
  ierr = PetscFree(grid->f2ntn);CHKERRQ(ierr);
  ierr = PetscFree(grid->isnode);CHKERRQ(ierr);
  ierr = PetscFree(grid->sxn);CHKERRQ(ierr);
  ierr = PetscFree(grid->syn);CHKERRQ(ierr);
  ierr = PetscFree(grid->szn);CHKERRQ(ierr);
  ierr = PetscFree(grid->sa);CHKERRQ(ierr);
  ICALLOC(4*nnfacetLoc, &grid->f2ntn);
  ICALLOC(nsnodeLoc, &grid->isnode);
  FCALLOC(nsnodeLoc, &grid->sxn);
  FCALLOC(nsnodeLoc, &grid->syn);
  FCALLOC(nsnodeLoc, &grid->szn);
  FCALLOC(nsnodeLoc, &grid->sa);
  j = 0;
  for (i = 0; i < nsnodeLoc; i++) {
    grid->isnode[i] = tmp1[i] + 1;
    grid->sxn[i]    = ftmp[j++];
    grid->syn[i]    = ftmp[j++];
    grid->szn[i]    = ftmp[j++];
    grid->sa[i]     = ftmp[j++];
  }
  j = 0;
  for (i = 0; i < nnfacetLoc; i++) {
    grid->f2ntn[i]              = tmp[j++] + 1;
    grid->f2ntn[nnfacetLoc+i]   = tmp[j++] + 1;
    grid->f2ntn[2*nnfacetLoc+i] = tmp[j++] + 1;
  }
  ierr = PetscFree(tmp);CHKERRQ(ierr);
  ierr = PetscFree(tmp1);CHKERRQ(ierr);
  ierr = PetscFree(tmp2);CHKERRQ(ierr);
  ierr = PetscFree(ftmp);CHKERRQ(ierr);

/* Now identify the triangles on which the current proceesor
   would perform force calculation */
  ICALLOC(nnfacetLoc, &grid->sface_bit);
  ierr = PetscMemzero(grid->sface_bit,nnfacetLoc*sizeof(int));CHKERRQ(ierr);
  for (i = 0; i < nnfacetLoc; i++) {
    node1 = l2a[grid->isnode[grid->f2ntn[i] - 1] - 1];
    node2 = l2a[grid->isnode[grid->f2ntn[nnfacetLoc + i] - 1] - 1];
    node3 = l2a[grid->isnode[grid->f2ntn[2*nnfacetLoc + i] - 1] - 1];
    if (((v2p[node1] >= rank) && (v2p[node2] >= rank)
        && (v2p[node3] >= rank)) &&
        ((v2p[node1] == rank) || (v2p[node2] == rank)
        || (v2p[node3] == rank))) {
         grid->sface_bit[i] = 1;
    }
  }
  PetscPrintf(PETSC_COMM_WORLD, "Solid boundaries partitioned\n");

/* Now, handle all the viscous boundaries - things to be done :
 * 1. Identify the nodes belonging to the viscous
 *    boundaries and count them.
 * 2. Put proper indices into f2ntv array, after making it
 *    of suitable size
 * 3. Remap the normals and areas of viscous faces (vxn, vyn, vzn,
 *    and va arrays).
 */
  ICALLOC(nvbound,   &grid->nvtet);
  ICALLOC(nvbound,   &grid->nvpts);
  ICALLOC(4*nvfacet, &grid->f2ntv);
  ICALLOC(nvnode, &grid->ivnode);
  FCALLOC(nvnode, &grid->vxn);
  FCALLOC(nvnode, &grid->vyn);
  FCALLOC(nvnode, &grid->vzn);
  FCALLOC(nvnode, &grid->va);
  ierr = PetscBinarySynchronizedRead(comm,fdes, (void*) grid->nvtet, nvbound, PETSC_INT);CHKERRQ(ierr);
  ierr = PetscBinarySynchronizedRead(comm,fdes, (void*) grid->nvpts, nvbound, PETSC_INT);CHKERRQ(ierr);
  ierr = PetscBinarySynchronizedRead(comm,fdes, (void*) grid->f2ntv, 4*nvfacet, PETSC_INT);CHKERRQ(ierr);
  ierr = PetscBinarySynchronizedRead(comm,fdes, (void*) grid->ivnode, nvnode, PETSC_INT);CHKERRQ(ierr);
  ierr = PetscBinarySynchronizedRead(comm,fdes, (void*) grid->vxn, nvnode, PETSC_SCALAR);CHKERRQ(ierr);
  ierr = PetscBinarySynchronizedRead(comm,fdes, (void*) grid->vyn, nvnode, PETSC_SCALAR);CHKERRQ(ierr);
  ierr = PetscBinarySynchronizedRead(comm,fdes, (void*) grid->vzn, nvnode, PETSC_SCALAR);CHKERRQ(ierr);

  isurf      = 0;
  nvnodeLoc  = 0;
  nvfacetLoc = 0;
  nb         = 0;
  nte        = 0;
  ICALLOC(3*nvfacet, &tmp);
  ICALLOC(nvnode, &tmp1);
  ICALLOC(nnodes, &tmp2);
  FCALLOC(4*nvnode, &ftmp);
  ierr = PetscMemzero(tmp,3*nvfacet*sizeof(int));CHKERRQ(ierr);
  ierr = PetscMemzero(tmp1,nvnode*sizeof(int));CHKERRQ(ierr);
  ierr = PetscMemzero(tmp2,nnodes*sizeof(int));CHKERRQ(ierr);

  j = 0;
  for (i = 0; i < nvnode; i++) {
    node1 = a2l[grid->ivnode[i] - 1];
    if (node1 >= 0) {
      tmp1[nvnodeLoc] = node1;
      tmp2[node1]     = nvnodeLoc;
      ftmp[j++]       = grid->vxn[i];
      ftmp[j++]       = grid->vyn[i];
      ftmp[j++]       = grid->vzn[i];
      ftmp[j++]       = grid->va[i];
      nvnodeLoc++;
    }
  }
  for (i = 0; i < nvbound; i++) {
    for (j = isurf; j < isurf + grid->nvtet[i]; j++) {
      node1 = a2l[grid->ivnode[grid->f2ntv[j] - 1] - 1];
      node2 = a2l[grid->ivnode[grid->f2ntv[nvfacet + j] - 1] - 1];
      node3 = a2l[grid->ivnode[grid->f2ntv[2*nvfacet + j] - 1] - 1];
      if ((node1 >= 0) && (node2 >= 0) && (node3 >= 0)) {
        nvfacetLoc++;
        nte++;
        tmp[nb++] = tmp2[node1];
        tmp[nb++] = tmp2[node2];
        tmp[nb++] = tmp2[node3];
      }
    }
    isurf         += grid->nvtet[i];
    grid->nvtet[i] = nte;
    nte            = 0;
  }
  ierr = PetscFree(grid->f2ntv);CHKERRQ(ierr);
  ierr = PetscFree(grid->ivnode);CHKERRQ(ierr);
  ierr = PetscFree(grid->vxn);CHKERRQ(ierr);
  ierr = PetscFree(grid->vyn);CHKERRQ(ierr);
  ierr = PetscFree(grid->vzn);CHKERRQ(ierr);
  ierr = PetscFree(grid->va);CHKERRQ(ierr);
  ICALLOC(4*nvfacetLoc, &grid->f2ntv);
  ICALLOC(nvnodeLoc, &grid->ivnode);
  FCALLOC(nvnodeLoc, &grid->vxn);
  FCALLOC(nvnodeLoc, &grid->vyn);
  FCALLOC(nvnodeLoc, &grid->vzn);
  FCALLOC(nvnodeLoc, &grid->va);
  j = 0;
  for (i = 0; i < nvnodeLoc; i++) {
    grid->ivnode[i] = tmp1[i] + 1;
    grid->vxn[i]    = ftmp[j++];
    grid->vyn[i]    = ftmp[j++];
    grid->vzn[i]    = ftmp[j++];
    grid->va[i]     = ftmp[j++];
  }
  j = 0;
  for (i = 0; i < nvfacetLoc; i++) {
    grid->f2ntv[i]              = tmp[j++] + 1;
    grid->f2ntv[nvfacetLoc+i]   = tmp[j++] + 1;
    grid->f2ntv[2*nvfacetLoc+i] = tmp[j++] + 1;
  }
  ierr = PetscFree(tmp);CHKERRQ(ierr);
  ierr = PetscFree(tmp1);CHKERRQ(ierr);
  ierr = PetscFree(tmp2);CHKERRQ(ierr);
  ierr = PetscFree(ftmp);CHKERRQ(ierr);

/* Now identify the triangles on which the current proceesor
   would perform force calculation */
  ICALLOC(nvfacetLoc, &grid->vface_bit);
  ierr = PetscMemzero(grid->vface_bit,nvfacetLoc*sizeof(int));CHKERRQ(ierr);
  for (i = 0; i < nvfacetLoc; i++) {
    node1 = l2a[grid->ivnode[grid->f2ntv[i] - 1] - 1];
    node2 = l2a[grid->ivnode[grid->f2ntv[nvfacetLoc + i] - 1] - 1];
    node3 = l2a[grid->ivnode[grid->f2ntv[2*nvfacetLoc + i] - 1] - 1];
    if (((v2p[node1] >= rank) && (v2p[node2] >= rank)
        && (v2p[node3] >= rank)) &&
        ((v2p[node1] == rank) || (v2p[node2] == rank)
        || (v2p[node3] == rank))) {
         grid->vface_bit[i] = 1;
    }
  }
  ierr = PetscFree(v2p);CHKERRQ(ierr);
  PetscPrintf(PETSC_COMM_WORLD, "Viscous boundaries partitioned\n");

/* Now, handle all the free boundaries - things to be done :
 * 1. Identify the nodes belonging to the free
 *    boundaries and count them.
 * 2. Put proper indices into f2ntf array, after making it
 *    of suitable size
 * 3. Remap the normals and areas of free bound. faces (fxn, fyn, fzn,
 *    and fa arrays).
 */

  ICALLOC(nfbound,   &grid->nftet);
  ICALLOC(nfbound,   &grid->nfpts);
  ICALLOC(4*nffacet, &grid->f2ntf);
  ICALLOC(nfnode, &grid->ifnode);
  FCALLOC(nfnode, &grid->fxn);
  FCALLOC(nfnode, &grid->fyn);
  FCALLOC(nfnode, &grid->fzn);
  FCALLOC(nfnode, &grid->fa);
  ierr = PetscBinarySynchronizedRead(comm,fdes, (void*) grid->nftet, nfbound, PETSC_INT);CHKERRQ(ierr);
  ierr = PetscBinarySynchronizedRead(comm,fdes, (void*) grid->nfpts, nfbound, PETSC_INT);CHKERRQ(ierr);
  ierr = PetscBinarySynchronizedRead(comm,fdes, (void*) grid->f2ntf, 4*nffacet, PETSC_INT);CHKERRQ(ierr);
  ierr = PetscBinarySynchronizedRead(comm,fdes, (void*) grid->ifnode, nfnode, PETSC_INT);CHKERRQ(ierr);
  ierr = PetscBinarySynchronizedRead(comm,fdes, (void*) grid->fxn, nfnode, PETSC_SCALAR);CHKERRQ(ierr);
  ierr = PetscBinarySynchronizedRead(comm,fdes, (void*) grid->fyn, nfnode, PETSC_SCALAR);CHKERRQ(ierr);
  ierr = PetscBinarySynchronizedRead(comm,fdes, (void*) grid->fzn, nfnode, PETSC_SCALAR);CHKERRQ(ierr);
  ierr = PetscBinaryClose(fdes);CHKERRQ(ierr);

  isurf      = 0;
  nfnodeLoc  = 0;
  nffacetLoc = 0;
  nb         = 0;
  nte        = 0;
  ICALLOC(3*nffacet, &tmp);
  ICALLOC(nfnode, &tmp1);
  ICALLOC(nnodes, &tmp2);
  FCALLOC(4*nfnode, &ftmp);
  ierr = PetscMemzero(tmp,3*nffacet*sizeof(int));CHKERRQ(ierr);
  ierr = PetscMemzero(tmp1,nfnode*sizeof(int));CHKERRQ(ierr);
  ierr = PetscMemzero(tmp2,nnodes*sizeof(int));CHKERRQ(ierr);

  j = 0;
  for (i = 0; i < nfnode; i++) {
    node1 = a2l[grid->ifnode[i] - 1];
    if (node1 >= 0) {
      tmp1[nfnodeLoc] = node1;
      tmp2[node1]     = nfnodeLoc;
      ftmp[j++]       = grid->fxn[i];
      ftmp[j++]       = grid->fyn[i];
      ftmp[j++]       = grid->fzn[i];
      ftmp[j++]       = grid->fa[i];
      nfnodeLoc++;
    }
  }
  for (i = 0; i < nfbound; i++) {
    for (j = isurf; j < isurf + grid->nftet[i]; j++) {
      node1 = a2l[grid->ifnode[grid->f2ntf[j] - 1] - 1];
      node2 = a2l[grid->ifnode[grid->f2ntf[nffacet + j] - 1] - 1];
      node3 = a2l[grid->ifnode[grid->f2ntf[2*nffacet + j] - 1] - 1];
      if ((node1 >= 0) && (node2 >= 0) && (node3 >= 0)) {
        nffacetLoc++;
        nte++;
        tmp[nb++] = tmp2[node1];
        tmp[nb++] = tmp2[node2];
        tmp[nb++] = tmp2[node3];
      }
    }
    isurf         += grid->nftet[i];
    grid->nftet[i] = nte;
    nte            = 0;
  }
  ierr = PetscFree(grid->f2ntf);CHKERRQ(ierr);
  ierr = PetscFree(grid->ifnode);CHKERRQ(ierr);
  ierr = PetscFree(grid->fxn);CHKERRQ(ierr);
  ierr = PetscFree(grid->fyn);CHKERRQ(ierr);
  ierr = PetscFree(grid->fzn);CHKERRQ(ierr);
  ierr = PetscFree(grid->fa);CHKERRQ(ierr);
  ICALLOC(4*nffacetLoc, &grid->f2ntf);
  ICALLOC(nfnodeLoc, &grid->ifnode);
  FCALLOC(nfnodeLoc, &grid->fxn);
  FCALLOC(nfnodeLoc, &grid->fyn);
  FCALLOC(nfnodeLoc, &grid->fzn);
  FCALLOC(nfnodeLoc, &grid->fa);
  j = 0;
  for (i = 0; i < nfnodeLoc; i++) {
    grid->ifnode[i] = tmp1[i] + 1;
    grid->fxn[i]    = ftmp[j++];
    grid->fyn[i]    = ftmp[j++];
    grid->fzn[i]    = ftmp[j++];
    grid->fa[i]     = ftmp[j++];
  }
  j = 0;
  for (i = 0; i < nffacetLoc; i++) {
    grid->f2ntf[i]              = tmp[j++] + 1;
    grid->f2ntf[nffacetLoc+i]   = tmp[j++] + 1;
    grid->f2ntf[2*nffacetLoc+i] = tmp[j++] + 1;
  }


  ierr = PetscFree(tmp);CHKERRQ(ierr);
  ierr = PetscFree(tmp1);CHKERRQ(ierr);
  ierr = PetscFree(tmp2);CHKERRQ(ierr);
  ierr = PetscFree(ftmp);CHKERRQ(ierr);
  PetscPrintf(PETSC_COMM_WORLD, "Free boundaries partitioned\n");

  flg  = PETSC_FALSE;
  ierr = PetscOptionsGetBool(0,"-mem_use",&flg,NULL);CHKERRQ(ierr);
  if (flg) {
    ierr = PetscMemoryView(PETSC_VIEWER_STDOUT_WORLD,"Memory usage after partitioning\n");CHKERRQ(ierr);
  }
  /* Put different mappings and other info into grid */
  /* ICALLOC(nvertices, &grid->loc2pet);
   ICALLOC(nvertices, &grid->loc2glo);
   ierr = PetscMemcpy(grid->loc2pet,l2p,nvertices*sizeof(int));CHKERRQ(ierr);
   ierr = PetscMemcpy(grid->loc2glo,l2a,nvertices*sizeof(int));CHKERRQ(ierr);
   ierr = PetscFree(l2a);CHKERRQ(ierr);CHKERRQ(ierr);
   ierr = PetscFree(l2p);CHKERRQ(ierr);CHKERRQ(ierr);*/

  grid->nnodesLoc  = nnodesLoc;
  grid->nedgeLoc   = nedgeLoc;
  grid->nvertices  = nvertices;
  grid->nsnodeLoc  = nsnodeLoc;
  grid->nvnodeLoc  = nvnodeLoc;
  grid->nfnodeLoc  = nfnodeLoc;
  grid->nnfacetLoc = nnfacetLoc;
  grid->nvfacetLoc = nvfacetLoc;
  grid->nffacetLoc = nffacetLoc;
/*
 * FCALLOC(nvertices*4,  &grid->gradx);
 * FCALLOC(nvertices*4,  &grid->grady);
 * FCALLOC(nvertices*4,  &grid->gradz);
 */
  FCALLOC(nvertices,    &grid->cdt);
  FCALLOC(nvertices*bs,  &grid->phi);
/*
   FCALLOC(nvertices,    &grid->r11);
   FCALLOC(nvertices,    &grid->r12);
   FCALLOC(nvertices,    &grid->r13);
   FCALLOC(nvertices,    &grid->r22);
   FCALLOC(nvertices,    &grid->r23);
   FCALLOC(nvertices,    &grid->r33);
*/
  FCALLOC(7*nnodesLoc,    &grid->rxy);

/* Print the different mappings
 *
 */
  {
    int partLoc[7],partMax[7],partMin[7], partSum[7];
    partLoc[0] = nnodesLoc;
    partLoc[1] = nvertices;
    partLoc[2] = nedgeLoc;
    partLoc[3] = nnfacetLoc;
    partLoc[4] = nffacetLoc;
    partLoc[5] = nsnodeLoc;
    partLoc[6] = nfnodeLoc;
    for (i = 0; i < 7; i++) {
      partMin[i] = 0;
      partMax[i] = 0;
      partSum[i] = 0;
    }

    ierr = MPI_Allreduce(partLoc,partMax,7,MPI_INT,
                         MPI_MAX,MPI_COMM_WORLD);
    ierr = MPI_Allreduce(partLoc,partMin,7,MPI_INT,
                         MPI_MIN,MPI_COMM_WORLD);
    ierr = MPI_Allreduce(partLoc,partSum,7,MPI_INT,
                         MPI_SUM,MPI_COMM_WORLD);
    PetscPrintf(MPI_COMM_WORLD, "==============================\n");
    PetscPrintf(MPI_COMM_WORLD, "Partitioning quality info ....\n");
    PetscPrintf(MPI_COMM_WORLD, "==============================\n");
    PetscPrintf(MPI_COMM_WORLD, "------------------------------------------------------------\n");
    PetscPrintf(MPI_COMM_WORLD, "Item                    Min        Max    Average      Total\n");
    PetscPrintf(MPI_COMM_WORLD, "------------------------------------------------------------\n");
    PetscPrintf(MPI_COMM_WORLD, "Local Nodes       %9d  %9d  %9d  %9d\n",
                partMin[0], partMax[0], partSum[0]/CommSize, partSum[0]);
    PetscPrintf(MPI_COMM_WORLD, "Local+Ghost Nodes %9d  %9d  %9d  %9d\n",
                partMin[1], partMax[1], partSum[1]/CommSize, partSum[1]);
    PetscPrintf(MPI_COMM_WORLD, "Local Edges       %9d  %9d  %9d  %9d\n",
                partMin[2], partMax[2], partSum[2]/CommSize, partSum[2]);
    PetscPrintf(MPI_COMM_WORLD, "Local solid faces %9d  %9d  %9d  %9d\n",
                partMin[3], partMax[3], partSum[3]/CommSize, partSum[3]);
    PetscPrintf(MPI_COMM_WORLD, "Local free faces  %9d  %9d  %9d  %9d\n",
                partMin[4], partMax[4], partSum[4]/CommSize, partSum[4]);
    PetscPrintf(MPI_COMM_WORLD, "Local solid nodes %9d  %9d  %9d  %9d\n",
                partMin[5], partMax[5], partSum[5]/CommSize, partSum[5]);
    PetscPrintf(MPI_COMM_WORLD, "Local free nodes  %9d  %9d  %9d  %9d\n",
                partMin[6], partMax[6], partSum[6]/CommSize, partSum[6]);
    PetscPrintf(MPI_COMM_WORLD, "------------------------------------------------------------\n");
  }
  flg  = PETSC_FALSE;
  ierr = PetscOptionsGetBool(0,"-partition_info",&flg,NULL);CHKERRQ(ierr);
  if (flg) {
    char part_name[PETSC_MAX_PATH_LEN];
    sprintf(part_name,"output.%d",rank);
    fptr1 = fopen(part_name,"w");

    fprintf(fptr1, "---------------------------------------------\n");
    fprintf(fptr1, "Local and Global Grid Parameters are :\n");
    fprintf(fptr1, "---------------------------------------------\n");
    fprintf(fptr1, "Local\t\t\t\tGlobal\n");
    fprintf(fptr1, "nnodesLoc = %d\t\tnnodes = %d\n", nnodesLoc, nnodes);
    fprintf(fptr1, "nedgeLoc = %d\t\t\tnedge = %d\n", nedgeLoc, nedge);
    fprintf(fptr1, "nnfacetLoc = %d\t\tnnfacet = %d\n", nnfacetLoc, nnfacet);
    fprintf(fptr1, "nvfacetLoc = %d\t\t\tnvfacet = %d\n", nvfacetLoc, nvfacet);
    fprintf(fptr1, "nffacetLoc = %d\t\t\tnffacet = %d\n", nffacetLoc, nffacet);
    fprintf(fptr1, "nsnodeLoc = %d\t\t\tnsnode = %d\n", nsnodeLoc, nsnode);
    fprintf(fptr1, "nvnodeLoc = %d\t\t\tnvnode = %d\n", nvnodeLoc, nvnode);
    fprintf(fptr1, "nfnodeLoc = %d\t\t\tnfnode = %d\n", nfnodeLoc, nfnode);
    fprintf(fptr1, "\n");
    fprintf(fptr1,"nvertices = %d\n", nvertices);
    fprintf(fptr1, "nnbound = %d\n", nnbound);
    fprintf(fptr1, "nvbound = %d\n", nvbound);
    fprintf(fptr1, "nfbound = %d\n", nfbound);
    fprintf(fptr1, "\n");

    fprintf(fptr1, "---------------------------------------------\n");
    fprintf(fptr1, "Different Orderings\n");
    fprintf(fptr1, "---------------------------------------------\n");
    fprintf(fptr1, "Local\t\tPETSc\t\tGlobal\n");
    fprintf(fptr1, "---------------------------------------------\n");
    for (i = 0; i < nvertices; i++) fprintf(fptr1, "%d\t\t%d\t\t%d\n", i, grid->loc2pet[i], grid->loc2glo[i]);
    fprintf(fptr1, "\n");

    fprintf(fptr1, "---------------------------------------------\n");
    fprintf(fptr1, "Solid Boundary Nodes\n");
    fprintf(fptr1, "---------------------------------------------\n");
    fprintf(fptr1, "Local\t\tPETSc\t\tGlobal\n");
    fprintf(fptr1, "---------------------------------------------\n");
    for (i = 0; i < nsnodeLoc; i++) {
      j = grid->isnode[i]-1;
      fprintf(fptr1, "%d\t\t%d\t\t%d\n", j, grid->loc2pet[j], grid->loc2glo[j]);
    }
    fprintf(fptr1, "\n");
    fprintf(fptr1, "---------------------------------------------\n");
    fprintf(fptr1, "f2ntn array\n");
    fprintf(fptr1, "---------------------------------------------\n");
    for (i = 0; i < nnfacetLoc; i++) {
      fprintf(fptr1, "%d\t\t%d\t\t%d\t\t%d\n",i,grid->f2ntn[i],
              grid->f2ntn[nnfacetLoc+i], grid->f2ntn[2*nnfacetLoc+i]);
    }
    fprintf(fptr1, "\n");


    fprintf(fptr1, "---------------------------------------------\n");
    fprintf(fptr1, "Viscous Boundary Nodes\n");
    fprintf(fptr1, "---------------------------------------------\n");
    fprintf(fptr1, "Local\t\tPETSc\t\tGlobal\n");
    fprintf(fptr1, "---------------------------------------------\n");
    for (i = 0; i < nvnodeLoc; i++) {
      j = grid->ivnode[i]-1;
      fprintf(fptr1, "%d\t\t%d\t\t%d\n", j, grid->loc2pet[j], grid->loc2glo[j]);
    }
    fprintf(fptr1, "\n");
    fprintf(fptr1, "---------------------------------------------\n");
    fprintf(fptr1, "f2ntv array\n");
    fprintf(fptr1, "---------------------------------------------\n");
    for (i = 0; i < nvfacetLoc; i++) {
      fprintf(fptr1, "%d\t\t%d\t\t%d\t\t%d\n",i,grid->f2ntv[i],
              grid->f2ntv[nvfacetLoc+i], grid->f2ntv[2*nvfacetLoc+i]);
    }
    fprintf(fptr1, "\n");

    fprintf(fptr1, "---------------------------------------------\n");
    fprintf(fptr1, "Free Boundary Nodes\n");
    fprintf(fptr1, "---------------------------------------------\n");
    fprintf(fptr1, "Local\t\tPETSc\t\tGlobal\n");
    fprintf(fptr1, "---------------------------------------------\n");
    for (i = 0; i < nfnodeLoc; i++) {
      j = grid->ifnode[i]-1;
      fprintf(fptr1, "%d\t\t%d\t\t%d\n", j, grid->loc2pet[j], grid->loc2glo[j]);
    }
    fprintf(fptr1, "\n");
    fprintf(fptr1, "---------------------------------------------\n");
    fprintf(fptr1, "f2ntf array\n");
    fprintf(fptr1, "---------------------------------------------\n");
    for (i = 0; i < nffacetLoc; i++) {
      fprintf(fptr1, "%d\t\t%d\t\t%d\t\t%d\n",i,grid->f2ntf[i],
              grid->f2ntf[nffacetLoc+i], grid->f2ntf[2*nffacetLoc+i]);
    }
    fprintf(fptr1, "\n");

    fprintf(fptr1, "---------------------------------------------\n");
    fprintf(fptr1, "Neighborhood Info In Various Ordering\n");
    fprintf(fptr1, "---------------------------------------------\n");
    ICALLOC(nnodes, &p2l);
    for (i = 0; i < nvertices; i++) p2l[grid->loc2pet[i]] = i;
    for (i = 0; i < nnodesLoc; i++) {
      jstart = grid->ia[grid->loc2glo[i]] - 1;
      jend   = grid->ia[grid->loc2glo[i]+1] - 1;
      fprintf(fptr1, "Neighbors of Node %d in Local Ordering are :", i);
      for (j = jstart; j < jend; j++) fprintf(fptr1, "%d ", p2l[grid->ja[j]]);

      fprintf(fptr1, "\n");

      fprintf(fptr1, "Neighbors of Node %d in PETSc ordering are :", grid->loc2pet[i]);
      for (j = jstart; j < jend; j++) fprintf(fptr1, "%d ", grid->ja[j]);
      fprintf(fptr1, "\n");

      fprintf(fptr1, "Neighbors of Node %d in Global Ordering are :", grid->loc2glo[i]);
      for (j = jstart; j < jend; j++) fprintf(fptr1, "%d ", grid->loc2glo[p2l[grid->ja[j]]]);
      fprintf(fptr1, "\n");
    }
    fprintf(fptr1, "\n");
    ierr = PetscFree(p2l);CHKERRQ(ierr);
    fclose(fptr1);
  }

/* Free the temporary arrays */
  ierr = PetscFree(a2l);CHKERRQ(ierr);
  ierr = MPI_Barrier(MPI_COMM_WORLD);

  return 0;
}


/*---------------------------------------------------------------------*/
int SetPetscDS(GRID *grid, TstepCtx *tsCtx)
/*---------------------------------------------------------------------*/
{
  int                    ierr, i, j, bs = 5;
  int                    nnodes,jstart, jend, nbrs_diag, nbrs_offd;
  int                    nnodesLoc, nedgeLoc, nvertices;
  int                    *val_diag, *val_offd, *svertices, *loc2pet, *loc2glo;
  IS                     isglobal,islocal;
  ISLocalToGlobalMapping isl2g;
  PetscBool              flg;

  nnodes    = grid->nnodes;
  nnodesLoc = grid->nnodesLoc;
  nedgeLoc  = grid->nedgeLoc;
  nvertices = grid->nvertices;
  loc2pet   = grid->loc2pet;
  loc2glo   = grid->loc2glo;
/* Set up the PETSc datastructures */

  ierr = VecCreateMPI(MPI_COMM_WORLD,bs*nnodesLoc,bs*nnodes,&grid->qnode);CHKERRQ(ierr);
  ierr = VecDuplicate(grid->qnode,&grid->res);CHKERRQ(ierr);
  ierr = VecDuplicate(grid->qnode,&tsCtx->qold);CHKERRQ(ierr);
  ierr = VecDuplicate(grid->qnode,&tsCtx->func);CHKERRQ(ierr);
  ierr = VecCreateSeq(MPI_COMM_SELF,bs*nvertices,&grid->qnodeLoc);CHKERRQ(ierr);
  ierr = VecCreateMPI(MPI_COMM_WORLD,3*bs*nnodesLoc,3*bs*nnodes,&grid->grad);
  ierr = VecCreateSeq(MPI_COMM_SELF,3*bs*nvertices,&grid->gradLoc);
/* Create Scatter between the local and global vectors */
/* First create scatter for qnode */
  ierr = ISCreateStride(MPI_COMM_SELF,bs*nvertices,0,1,&islocal);CHKERRQ(ierr);
#if defined(INTERLACING)
#if defined(BLOCKING)
  ICALLOC(nvertices, &svertices);
  for (i=0; i < nvertices; i++) svertices[i] = loc2pet[i];
  ierr = ISCreateBlock(MPI_COMM_SELF,bs,nvertices,svertices,PETSC_COPY_VALUES,&isglobal);CHKERRQ(ierr);
#else
  ICALLOC(bs*nvertices, &svertices);
  for (i = 0; i < nvertices; i++)
    for (j = 0; j < bs; j++)
      svertices[j+bs*i] = j + bs*loc2pet[i];
  ierr = ISCreateGeneral(MPI_COMM_SELF,bs*nvertices,svertices,PETSC_COPY_VALUES,&isglobal);CHKERRQ(ierr);
#endif
#else
  ICALLOC(bs*nvertices, &svertices);
  for (j = 0; j < bs; j++)
    for (i = 0; i < nvertices; i++)
      svertices[j*nvertices+i] = j*nvertices + loc2pet[i];
  ierr = ISCreateGeneral(MPI_COMM_SELF,bs*nvertices,svertices,PETSC_COPY_VALUES,&isglobal);CHKERRQ(ierr);
#endif
  ierr = PetscFree(svertices);CHKERRQ(ierr);
  ierr = VecScatterCreate(grid->qnode,isglobal,grid->qnodeLoc,islocal,
                          &grid->scatter);CHKERRQ(ierr);
  ierr = ISDestroy(&isglobal);CHKERRQ(ierr);
  ierr = ISDestroy(&islocal);CHKERRQ(ierr);

/* Now create scatter for gradient vector of qnode */
  ierr = ISCreateStride(MPI_COMM_SELF,3*bs*nvertices,0,1,&islocal);CHKERRQ(ierr);
#if defined(INTERLACING)
#if defined(BLOCKING)
  ICALLOC(nvertices, &svertices);
  for (i=0; i < nvertices; i++) svertices[i] = 3*loc2pet[i];
  ierr = ISCreateBlock(MPI_COMM_SELF,3*bs,nvertices,svertices,PETSC_COPY_VALUES,&isglobal);CHKERRQ(ierr);
#else
  ICALLOC(3*bs*nvertices, &svertices);
  for (i = 0; i < nvertices; i++)
    for (j = 0; j < 3*bs; j++)
      svertices[j+3*bs*i] = j + 3*bs*loc2pet[i];
  ierr = ISCreateGeneral(MPI_COMM_SELF,3*bs*nvertices,svertices,PETSC_COPY_VALUES,&isglobal);CHKERRQ(ierr);
#endif
#else
  ICALLOC(3*bs*nvertices, &svertices);
  for (j = 0; j < 3*bs; j++)
  for (i = 0; i < nvertices; i++)
    svertices[j*nvertices+i] = j*nvertices + loc2pet[i];
  ierr = ISCreateGeneral(MPI_COMM_SELF,3*bs*nvertices,svertices,PETSC_COPY_VALUES,&isglobal);CHKERRQ(ierr);
#endif
  ierr = PetscFree(svertices);CHKERRQ(ierr);
  ierr = VecScatterCreate(grid->grad,isglobal,grid->gradLoc,islocal,
                          &grid->gradScatter);CHKERRQ(ierr);
  ierr = ISDestroy(&isglobal);CHKERRQ(ierr);
  ierr = ISDestroy(&islocal);CHKERRQ(ierr);

/* Store the number of non-zeroes per row */
#if defined(INTERLACING)
#if defined(BLOCKING)
  ICALLOC(nnodesLoc, &val_diag);
  ICALLOC(nnodesLoc, &val_offd);
  for (i = 0; i < nnodesLoc; i++) {
    jstart    = grid->ia[i] - 1;
    jend      = grid->ia[i+1] - 1;
    nbrs_diag = 0;
    nbrs_offd = 0;
    for (j = jstart; j < jend; j++) {
      if ((grid->ja[j] >= rstart) && (grid->ja[j] < (rstart+nnodesLoc))) {
        nbrs_diag++;
      } else {
        nbrs_offd++;
      }
    }
    val_diag[i] = nbrs_diag;
    val_offd[i] = nbrs_offd;
  }
  ierr = MatCreateBAIJ(MPI_COMM_WORLD,bs,bs*nnodesLoc, bs*nnodesLoc,
                       bs*nnodes,bs*nnodes,0,val_diag,
                       0,val_offd,&grid->A);CHKERRQ(ierr);
#else
  ICALLOC(nnodesLoc*bs, &val_diag);
  ICALLOC(nnodesLoc*bs, &val_offd);
  for (i = 0; i < nnodesLoc; i++) {
    jstart    = grid->ia[i] - 1;
    jend      = grid->ia[i+1] - 1;
    nbrs_diag = 0;
    nbrs_offd = 0;
    for (j = jstart; j < jend; j++) {
      if ((grid->ja[j] >= rstart) && (grid->ja[j] < (rstart+nnodesLoc))) {
        nbrs_diag++;
      } else {
        nbrs_offd++;
      }
    }
    for (j = 0; j < bs; j++) {
      row           = bs*i + j;
      val_diag[row] = nbrs_diag*bs;
      val_offd[row] = nbrs_offd*bs;
    }
  }
  ierr = MatCreateAIJ(MPI_COMM_WORLD,bs*nnodesLoc, bs*nnodesLoc,
                      bs*nnodes,bs*nnodes,NULL,val_diag,
                      NULL,val_offd,&grid->A);CHKERRQ(ierr);
#endif
  ierr = PetscFree(val_diag);CHKERRQ(ierr);
  ierr = PetscFree(val_offd);CHKERRQ(ierr);

#else
  if (CommSize > 1) SETERRQ(PETSC_COMM_SELF,1,"Parallel case not supported in non-interlaced case\n");
  ICALLOC(nnodes*bs, &val_diag);
  ICALLOC(nnodes*bs, &val_offd);
  for (j = 0; j < bs; j++) {
    for (i = 0; i < nnodes; i++) {
      row           = i + j*nnodes;
      jstart        = grid->ia[i] - 1;
      jend          = grid->ia[i+1] - 1;
      nbrs_diag     = jend - jstart;
      val_diag[row] = nbrs_diag*bs;
      val_offd[row] = 0;
    }
  }
  /* ierr = MatCreateSeqAIJ(MPI_COMM_SELF,nnodes*bs,nnodes*bs,NULL,
                        val,&grid->A);*/
  ierr = MatCreateAIJ(MPI_COMM_WORLD,bs*nnodesLoc, bs*nnodesLoc,
                      bs*nnodes,bs*nnodes,NULL,val_diag,
                      NULL,val_offd,&grid->A);CHKERRQ(ierr);
  ierr = PetscFree(val_diag);CHKERRQ(ierr);
  ierr = PetscFree(val_offd);CHKERRQ(ierr);
#endif

  flg  = PETSC_FALSE;
  ierr = PetscOptionsGetBool(0,"-mem_use",&flg,NULL);CHKERRQ(ierr);
  if (flg) {
    ierr = PetscMemoryView(PETSC_VIEWER_STDOUT_WORLD,"Memory usage after allocating PETSc data structures\n");CHKERRQ(ierr);
  }
/* Set local to global mapping for setting the matrix elements in
* local ordering : first set row by row mapping
*/
  ierr = ISLocalToGlobalMappingCreate(MPI_COMM_SELF,bs,nvertices,loc2pet,PETSC_COPY_VALUES,&isl2g);CHKERRQ(ierr);
  ierr = MatSetLocalToGlobalMapping(grid->A,isl2g,isl2g);CHKERRQ(ierr);
  ierr = ISLocalToGlobalMappingDestroy(&isl2g);CHKERRQ(ierr);

  return 0;
}

/*---------------------------------------------------------------------*/
int FieldOutput(GRID *grid, int timeStep)
/*---------------------------------------------------------------------*/
{
  int         bs = 5;
  int         fdes;
  off_t       currentPos;
  int         i,ierr;
  int         nnodes, nnodesLoc, openFile, closeFile, boundaryType;
  int         nnbound, nsnode, nnfacet;
  int         nvbound, nvnode, nvfacet;
  int         nfbound, nfnode, nffacet;
  int         *isnode, *isnodePet, *f2ntn, *nntet, *nnpts;
  int         *ivnode, *ivnodePet, *f2ntv, *nvtet, *nvpts;
  int         *ifnode, *ifnodePet, *f2ntf, *nftet, *nfpts;
  int         *svertices;
  PetscScalar *x, *y, *z, *qnode, *xyz;
  Vec         qnodeLoc, xyzLoc, xyzGlo;
  IS          islocal, isglobal;
  VecScatter  scatter;

  nnodes    = grid->nnodes;
  nnodesLoc = grid->nnodesLoc;

  /* First write the inviscid boundaries */
  if (!rank) {
    nnbound = grid->nnbound;
    nsnode  = grid->nsnode;
    nnfacet = grid->nnfacet;
    ierr    = PetscBinaryOpen("uns3d.msh",FILE_MODE_READ, &fdes);CHKERRQ(ierr);
    ierr    = PetscBinarySeek(fdes,solidBndPos,PETSC_BINARY_SEEK_SET,&currentPos);CHKERRQ(ierr);

    ICALLOC(nnbound,   &nntet);
    ICALLOC(nnbound,   &nnpts);
    ICALLOC(4*nnfacet, &f2ntn);
    ICALLOC(nsnode, &isnode);
    ierr = PetscBinaryRead(fdes, nntet, nnbound, PETSC_INT);CHKERRQ(ierr);
    ierr = PetscBinaryRead(fdes, nnpts, nnbound, PETSC_INT);CHKERRQ(ierr);
    ierr = PetscBinaryRead(fdes, f2ntn, 4*nnfacet, PETSC_INT);CHKERRQ(ierr);
    ierr = PetscBinaryRead(fdes, isnode, nsnode, PETSC_INT);CHKERRQ(ierr);
    ierr = PetscBinarySeek(fdes, 3*nsnode*PETSC_BINARY_SCALAR_SIZE, PETSC_BINARY_SEEK_CUR,&currentPos);CHKERRQ(ierr);
    ICALLOC(nsnode, &isnodePet);
    for (i = 0; i < nsnode; i++) isnodePet[i] = isnode[i] - 1;
    ierr = AOApplicationToPetsc(grid->ao,nsnode,isnodePet);CHKERRQ(ierr);
    /* Create Scatter between the local and global vectors */
    ierr = VecCreateSeq(MPI_COMM_SELF,bs*nsnode,&qnodeLoc);
    ierr = ISCreateStride(MPI_COMM_SELF,bs*nsnode,0,1,&islocal);CHKERRQ(ierr);
    ICALLOC(nsnode, &svertices);
    for (i = 0; i < nsnode; i++) svertices[i] = isnodePet[i];
    ierr = ISCreateBlock(MPI_COMM_SELF,bs,nsnode,svertices,PETSC_COPY_VALUES,&isglobal);CHKERRQ(ierr);
  } else {
    int one = 1;
    ierr = VecCreateSeq(MPI_COMM_SELF,bs,&qnodeLoc);
    ierr = ISCreateStride(MPI_COMM_SELF,bs,0,1,&islocal);CHKERRQ(ierr);
    ierr = ISCreateBlock(MPI_COMM_SELF,bs,1,&one,PETSC_COPY_VALUES,&isglobal);CHKERRQ(ierr);
  }
  ierr = VecScatterCreate(grid->qnode,isglobal,qnodeLoc,islocal,
                          &scatter);CHKERRQ(ierr);
  ierr = ISDestroy(&isglobal);CHKERRQ(ierr);
  ierr = ISDestroy(&islocal);CHKERRQ(ierr);
  ierr = VecScatterBegin(scatter,grid->qnode,qnodeLoc,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
  ierr = VecScatterEnd(scatter,grid->qnode,qnodeLoc,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
  ierr = VecScatterDestroy(&scatter);CHKERRQ(ierr);
  /* Get the coordinates */
  ierr = VecCreateMPI(MPI_COMM_WORLD,3*nnodesLoc,3*nnodes,&xyzGlo);CHKERRQ(ierr);
  ierr = VecGetArray(xyzGlo, &xyz);CHKERRQ(ierr);
  for (i = 0; i < nnodesLoc; i++) {
    int in;
    in        = 3*i;
    xyz[in]   = grid->x[i];
    xyz[in+1] = grid->y[i];
    xyz[in+2] = grid->z[i];
  }
  ierr = VecRestoreArray(xyzGlo, &xyz);CHKERRQ(ierr);
  if (!rank) {
    ierr = VecCreateSeq(MPI_COMM_SELF,3*nsnode,&xyzLoc);
    ierr = ISCreateStride(MPI_COMM_SELF,3*nsnode,0,1,&islocal);CHKERRQ(ierr);
    ICALLOC(nsnode, &svertices);
    for (i = 0; i < nsnode; i++) svertices[i] = isnodePet[i];
    ierr = ISCreateBlock(MPI_COMM_SELF,3,nsnode,svertices,PETSC_COPY_VALUES,&isglobal);CHKERRQ(ierr);
    ierr = PetscFree(isnodePet);CHKERRQ(ierr);
    ierr = PetscFree(svertices);CHKERRQ(ierr);
  } else {
    int one = 1;
    ierr = VecCreateSeq(MPI_COMM_SELF,3,&xyzLoc);
    ierr = ISCreateStride(MPI_COMM_SELF,3,0,1,&islocal);CHKERRQ(ierr);
    ierr = ISCreateBlock(MPI_COMM_SELF,3,1,&one,PETSC_COPY_VALUES,&isglobal);CHKERRQ(ierr);
  }
  ierr = VecScatterCreate(xyzGlo,isglobal,xyzLoc,islocal,
                          &scatter);CHKERRQ(ierr);
  ierr = ISDestroy(&isglobal);CHKERRQ(ierr);
  ierr = ISDestroy(&islocal);CHKERRQ(ierr);
  ierr = VecScatterBegin(scatter,xyzGlo,xyzLoc,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
  ierr = VecScatterEnd(scatter,xyzGlo,xyzLoc,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
  ierr = VecScatterDestroy(&scatter);CHKERRQ(ierr);
  if (!rank) {
    ierr = VecGetArray(xyzLoc, &xyz);CHKERRQ(ierr);
    FCALLOC(nsnode, &x);
    FCALLOC(nsnode, &y);
    FCALLOC(nsnode, &z);
    for (i = 0; i < nsnode; i++) {
      int in;
      in   = 3*i;
      x[i] = xyz[in];
      y[i] = xyz[in+1];
      z[i] = xyz[in+2];
    }
    openFile     = 1;
    closeFile    = 0;
    boundaryType = 1;
    ierr         = VecGetArray(qnodeLoc, &qnode);CHKERRQ(ierr);
    f77TECFLO(&nnodes,  &nnbound, &nnfacet,
              &nsnode,
              x, y, z,
              qnode, nnpts, nntet,
              f2ntn, isnode,
              &timeStep, &rank, &openFile, &closeFile,
              &boundaryType,c_info->title);
    ierr = VecRestoreArray(xyzLoc, &xyz);CHKERRQ(ierr);
    ierr = VecRestoreArray(qnodeLoc, &qnode);CHKERRQ(ierr);
    ierr = PetscFree(nnpts);CHKERRQ(ierr);
    ierr = PetscFree(nntet);CHKERRQ(ierr);
    ierr = PetscFree(f2ntn);CHKERRQ(ierr);
    ierr = PetscFree(isnode);CHKERRQ(ierr);
    ierr = PetscFree(x);CHKERRQ(ierr);
    ierr = PetscFree(y);CHKERRQ(ierr);
    ierr = PetscFree(z);CHKERRQ(ierr);
  }
  ierr = VecDestroy(&xyzLoc);CHKERRQ(ierr);
  ierr = VecDestroy(&qnodeLoc);CHKERRQ(ierr);
  PetscPrintf(MPI_COMM_WORLD, "Inviscid boundaries written\n");

  /* Next write the viscous boundaries */
  if (grid->nvnode > 0) {
    if (!rank) {
      nvbound = grid->nvbound;
      nvnode  = grid->nvnode;
      nvfacet = grid->nvfacet;
      ICALLOC(nvbound,   &nvtet);
      ICALLOC(nvbound,   &nvpts);
      ICALLOC(4*nvfacet, &f2ntv);
      ICALLOC(nvnode, &ivnode);
      ierr = PetscBinaryRead(fdes, nvtet, nvbound, PETSC_INT);CHKERRQ(ierr);
      ierr = PetscBinaryRead(fdes, nvpts, nvbound, PETSC_INT);CHKERRQ(ierr);
      ierr = PetscBinaryRead(fdes, f2ntv, 4*nvfacet, PETSC_INT);CHKERRQ(ierr);
      ierr = PetscBinaryRead(fdes, ivnode, nvnode, PETSC_INT);CHKERRQ(ierr);
      ierr = PetscBinarySeek(fdes, 3*nvnode*PETSC_BINARY_SCALAR_SIZE, PETSC_BINARY_SEEK_CUR,&currentPos);CHKERRQ(ierr);
      ICALLOC(nvnode, &ivnodePet);
      for (i = 0; i < nvnode; i++) ivnodePet[i] = ivnode[i] - 1;
      ierr = AOApplicationToPetsc(grid->ao,nvnode,ivnodePet);CHKERRQ(ierr);
      /* Create Scatter between the local and global vectors */
      ierr = VecCreateSeq(MPI_COMM_SELF,bs*nvnode,&qnodeLoc);
      ierr = ISCreateStride(MPI_COMM_SELF,bs*nvnode,0,1,&islocal);CHKERRQ(ierr);
      ICALLOC(nvnode, &svertices);
      for (i = 0; i < nvnode; i++) svertices[i] = ivnodePet[i];
      ierr = ISCreateBlock(MPI_COMM_SELF,bs,nvnode,svertices,PETSC_COPY_VALUES,&isglobal);CHKERRQ(ierr);
    } else {
      int one = 1;
      ierr = VecCreateSeq(MPI_COMM_SELF,bs,&qnodeLoc);
      ierr = ISCreateStride(MPI_COMM_SELF,bs,0,1,&islocal);CHKERRQ(ierr);
      ierr = ISCreateBlock(MPI_COMM_SELF,bs,1,&one,PETSC_COPY_VALUES,&isglobal);CHKERRQ(ierr);
    }
    ierr = VecScatterCreate(grid->qnode,isglobal,qnodeLoc,islocal,
                            &scatter);CHKERRQ(ierr);
    ierr = ISDestroy(&isglobal);CHKERRQ(ierr);
    ierr = ISDestroy(&islocal);CHKERRQ(ierr);
    ierr = VecScatterBegin(scatter,grid->qnode,qnodeLoc,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
    ierr = VecScatterEnd(scatter,grid->qnode,qnodeLoc,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
    ierr = VecScatterDestroy(&scatter);CHKERRQ(ierr);
    /* Get the coordinates */
    if (!rank) {
      ierr = VecCreateSeq(MPI_COMM_SELF,3*nvnode,&xyzLoc);
      ierr = ISCreateStride(MPI_COMM_SELF,3*nvnode,0,1,&islocal);CHKERRQ(ierr);
      ICALLOC(nvnode, &svertices);
      for (i = 0; i < nvnode; i++) svertices[i] = ivnodePet[i];
      ierr = ISCreateBlock(MPI_COMM_SELF,3,nvnode,svertices,PETSC_COPY_VALUES,&isglobal);CHKERRQ(ierr);
      ierr = PetscFree(ivnodePet);CHKERRQ(ierr);
      ierr = PetscFree(svertices);CHKERRQ(ierr);
    } else {
      int one = 1;
      ierr = VecCreateSeq(MPI_COMM_SELF,3,&xyzLoc);
      ierr = ISCreateStride(MPI_COMM_SELF,3,0,1,&islocal);CHKERRQ(ierr);
      ierr = ISCreateBlock(MPI_COMM_SELF,3,1,&one,PETSC_COPY_VALUES,&isglobal);CHKERRQ(ierr);
    }
    ierr = VecScatterCreate(xyzGlo,isglobal,xyzLoc,islocal,
                            &scatter);CHKERRQ(ierr);
    ierr = ISDestroy(&isglobal);CHKERRQ(ierr);
    ierr = ISDestroy(&islocal);CHKERRQ(ierr);
    ierr = VecScatterBegin(scatter,xyzGlo,xyzLoc,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
    ierr = VecScatterEnd(scatter,xyzGlo,xyzLoc,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
    ierr = VecScatterDestroy(&scatter);CHKERRQ(ierr);
    if (!rank) {
      ierr = VecGetArray(xyzLoc, &xyz);CHKERRQ(ierr);
      FCALLOC(nvnode, &x);
      FCALLOC(nvnode, &y);
      FCALLOC(nvnode, &z);
      for (i = 0; i < nvnode; i++) {
        int in;
        in   = 3*i;
        x[i] = xyz[in];
        y[i] = xyz[in+1];
        z[i] = xyz[in+2];
      }
      openFile     = 0;
      closeFile    = 0;
      boundaryType = 2;
      ierr         = VecGetArray(qnodeLoc, &qnode);CHKERRQ(ierr);
      f77TECFLO(&nnodes,  &nvbound, &nvfacet,
                &nvnode,
                x, y, z,
                qnode, nvpts, nvtet,
                f2ntv, ivnode,
                &timeStep, &rank, &openFile, &closeFile,
                &boundaryType,c_info->title);
      ierr = VecRestoreArray(xyzLoc, &xyz);CHKERRQ(ierr);
      ierr = VecRestoreArray(qnodeLoc, &qnode);CHKERRQ(ierr);
      ierr = PetscFree(nvpts);CHKERRQ(ierr);
      ierr = PetscFree(nvtet);CHKERRQ(ierr);
      ierr = PetscFree(f2ntv);CHKERRQ(ierr);
      ierr = PetscFree(ivnode);CHKERRQ(ierr);
      ierr = PetscFree(x);CHKERRQ(ierr);
      ierr = PetscFree(y);CHKERRQ(ierr);
      ierr = PetscFree(z);CHKERRQ(ierr);
    }
    ierr = VecDestroy(&xyzLoc);CHKERRQ(ierr);
    ierr = VecDestroy(&qnodeLoc);CHKERRQ(ierr);
    PetscPrintf(MPI_COMM_WORLD, "Viscous boundaries written\n");
  }

  /* Finally write the free boundaries */
  if (!rank) {
    nfbound = grid->nfbound;
    nfnode  = grid->nfnode;
    nffacet = grid->nffacet;
    ICALLOC(nfbound,   &nftet);
    ICALLOC(nfbound,   &nfpts);
    ICALLOC(4*nffacet, &f2ntf);
    ICALLOC(nfnode, &ifnode);
    ierr = PetscBinaryRead(fdes, nftet, nfbound, PETSC_INT);CHKERRQ(ierr);
    ierr = PetscBinaryRead(fdes, nfpts, nfbound, PETSC_INT);CHKERRQ(ierr);
    ierr = PetscBinaryRead(fdes, f2ntf, 4*nffacet, PETSC_INT);CHKERRQ(ierr);
    ierr = PetscBinaryRead(fdes, ifnode, nfnode, PETSC_INT);CHKERRQ(ierr);
    ierr = PetscBinaryClose(fdes);CHKERRQ(ierr);
    ICALLOC(nfnode, &ifnodePet);
    for (i = 0; i < nfnode; i++) ifnodePet[i] = ifnode[i] - 1;
    ierr = AOApplicationToPetsc(grid->ao,nfnode,ifnodePet);CHKERRQ(ierr);
    /* Create Scatter between the local and global vectors */
    ierr = VecCreateSeq(MPI_COMM_SELF,bs*nfnode,&qnodeLoc);
    ierr = ISCreateStride(MPI_COMM_SELF,bs*nfnode,0,1,&islocal);CHKERRQ(ierr);
    ICALLOC(nfnode, &svertices);
    for (i = 0; i < nfnode; i++) {
      svertices[i] = ifnodePet[i];
    }
    ierr = ISCreateBlock(MPI_COMM_SELF,bs,nfnode,svertices,PETSC_COPY_VALUES,&isglobal);CHKERRQ(ierr);
  } else {
    int one = 1;
    ierr = VecCreateSeq(MPI_COMM_SELF,bs,&qnodeLoc);
    ierr = ISCreateStride(MPI_COMM_SELF,bs,0,1,&islocal);CHKERRQ(ierr);
    ierr = ISCreateBlock(MPI_COMM_SELF,bs,1,&one,PETSC_COPY_VALUES,&isglobal);CHKERRQ(ierr);
  }
  ierr = VecScatterCreate(grid->qnode,isglobal,qnodeLoc,islocal,
                          &scatter);CHKERRQ(ierr);
  ierr = ISDestroy(&isglobal);CHKERRQ(ierr);
  ierr = ISDestroy(&islocal);CHKERRQ(ierr);
  ierr = VecScatterBegin(scatter,grid->qnode,qnodeLoc,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
  ierr = VecScatterEnd(scatter,grid->qnode,qnodeLoc,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
  ierr = VecScatterDestroy(&scatter);CHKERRQ(ierr);
  /* Get the coordinates */
  if (!rank) {
    ierr = VecCreateSeq(MPI_COMM_SELF,3*nfnode,&xyzLoc);
    ierr = ISCreateStride(MPI_COMM_SELF,3*nfnode,0,1,&islocal);CHKERRQ(ierr);
    ICALLOC(nfnode, &svertices);
    for (i = 0; i < nfnode; i++) svertices[i] = ifnodePet[i];
    ierr = ISCreateBlock(MPI_COMM_SELF,3,nfnode,svertices,PETSC_COPY_VALUES,&isglobal);CHKERRQ(ierr);
    ierr = PetscFree(ifnodePet);CHKERRQ(ierr);
    ierr = PetscFree(svertices);CHKERRQ(ierr);
  } else {
    int one = 1;
    ierr = VecCreateSeq(MPI_COMM_SELF,3,&xyzLoc);
    ierr = ISCreateStride(MPI_COMM_SELF,3,0,1,&islocal);CHKERRQ(ierr);
    ierr = ISCreateBlock(MPI_COMM_SELF,3,1,&one,PETSC_COPY_VALUES,&isglobal);CHKERRQ(ierr);
  }
  ierr = VecScatterCreate(xyzGlo,isglobal,xyzLoc,islocal,
                          &scatter);CHKERRQ(ierr);
  ierr = ISDestroy(&isglobal);CHKERRQ(ierr);
  ierr = ISDestroy(&islocal);CHKERRQ(ierr);
  ierr = VecScatterBegin(scatter,xyzGlo,xyzLoc,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
  ierr = VecScatterEnd(scatter,xyzGlo,xyzLoc,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
  ierr = VecScatterDestroy(&scatter);CHKERRQ(ierr);
  ierr = VecDestroy(&xyzGlo);CHKERRQ(ierr);
  if (!rank) {
    ierr = VecGetArray(xyzLoc, &xyz);CHKERRQ(ierr);
    FCALLOC(nfnode, &x);
    FCALLOC(nfnode, &y);
    FCALLOC(nfnode, &z);
    for (i = 0; i < nfnode; i++) {
      int in;
      in   = 3*i;
      x[i] = xyz[in];
      y[i] = xyz[in+1];
      z[i] = xyz[in+2];
    }
    openFile     = 0;
    closeFile    = 1;
    boundaryType = 3;
    ierr = VecGetArray(qnodeLoc, &qnode);CHKERRQ(ierr);
    f77TECFLO(&nnodes,  &nfbound, &nffacet,
              &nfnode,
              x, y, z,
              qnode, nfpts, nftet,
              f2ntf, ifnode,
              &timeStep, &rank, &openFile, &closeFile,
              &boundaryType,c_info->title);
    ierr = VecRestoreArray(xyzLoc, &xyz);CHKERRQ(ierr);
    ierr = VecRestoreArray(qnodeLoc, &qnode);CHKERRQ(ierr);
    ierr = PetscFree(nfpts);CHKERRQ(ierr);
    ierr = PetscFree(nftet);CHKERRQ(ierr);
    ierr = PetscFree(f2ntf);CHKERRQ(ierr);
    ierr = PetscFree(ifnode);CHKERRQ(ierr);
    ierr = PetscFree(x);CHKERRQ(ierr);
    ierr = PetscFree(y);CHKERRQ(ierr);
    ierr = PetscFree(z);CHKERRQ(ierr);
  }
  ierr = VecDestroy(&xyzLoc);CHKERRQ(ierr);
  ierr = VecDestroy(&qnodeLoc);CHKERRQ(ierr);
  PetscPrintf(MPI_COMM_WORLD, "Free boundaries written\n");

  return 0;
}

int system(const char *string);

/*---------------------------------------------------------------------*/
int WriteRestartFile(GRID *grid, int timeStep)
/*---------------------------------------------------------------------*/
{
  int         bs = 5;
  int         fdes;
  off_t       startPos;
  int         i, j,in, ierr;
  int         nnodes, nnodesLoc;
  int         *loc2pet, *svertices;
  int         status;
  char        fileName[256],command[256];
  PetscScalar pgamma = 1.4,mach;
  PetscScalar *qnode;
  Vec         qnodeLoc;
  IS          islocal, isglobal;
  VecScatter  scatter;
  FILE        *fptr,*fptr1;
  PetscBool   flgIO,flg_vtk;

  nnodes    = grid->nnodes;
  nnodesLoc = grid->nnodesLoc;

  /* Create a local solution vector in global ordering */
  ICALLOC(nnodesLoc, &loc2pet);
  for (i = 0; i < nnodesLoc; i++) loc2pet[i] = rstart+i;
  ierr = AOApplicationToPetsc(grid->ao,nnodesLoc,loc2pet);CHKERRQ(ierr);

  ierr = VecCreateSeq(MPI_COMM_SELF,bs*nnodesLoc,&qnodeLoc);
  ierr = ISCreateStride(MPI_COMM_SELF,bs*nnodesLoc,0,1,&islocal);CHKERRQ(ierr);
#if defined(INTERLACING)
#if defined(BLOCKING)
  ICALLOC(nnodesLoc, &svertices);
  for (i = 0; i < nnodesLoc; i++) svertices[i] = loc2pet[i];
  ierr = ISCreateBlock(MPI_COMM_SELF,bs,nnodesLoc,svertices,PETSC_COPY_VALUES,&isglobal);CHKERRQ(ierr);
#else
  ICALLOC(bs*nnodesLoc, &svertices);
  for (i = 0; i < nnodesLoc; i++)
    for (j = 0; j < bs; j++)
      svertices[j+bs*i] = j + bs*loc2pet[i];
  ierr = ISCreateGeneral(MPI_COMM_SELF,bs*nnodesLoc,svertices,PETSC_COPY_VALUES,&isglobal);CHKERRQ(ierr);
#endif
#else
  ICALLOC(bs*nnodesLoc, &svertices);
  for (j = 0; j < bs; j++)
    for (i = 0; i < nnodesLoc; i++)
      svertices[j*nnodesLoc+i] = j*nnodesLoc + loc2pet[i];
  ierr = ISCreateGeneral(MPI_COMM_SELF,bs*nnodesLoc,svertices,PETSC_COPY_VALUES,&isglobal);CHKERRQ(ierr);
#endif
  ierr = PetscFree(loc2pet);CHKERRQ(ierr);
  ierr = PetscFree(svertices);CHKERRQ(ierr);
  ierr = VecScatterCreate(grid->qnode,isglobal,qnodeLoc,islocal,
                          &scatter);CHKERRQ(ierr);
  ierr = ISDestroy(&isglobal);CHKERRQ(ierr);
  ierr = ISDestroy(&islocal);CHKERRQ(ierr);
  ierr = VecScatterBegin(scatter,grid->qnode,qnodeLoc,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
  ierr = VecScatterEnd(scatter,grid->qnode,qnodeLoc,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
  ierr = VecScatterDestroy(&scatter);CHKERRQ(ierr);

  ierr = PetscOptionsHasName(NULL,"-par_io",&flgIO);CHKERRQ(ierr);
  if (flgIO) {
    /* All processors write the output to the same file at appropriate positions */
    if (timeStep < 10)        sprintf(fileName,"flow000%d.bin",timeStep);
    else if (timeStep < 100)  sprintf(fileName,"flow00%d.bin",timeStep);
    else if (timeStep < 1000) sprintf(fileName,"flow0%d.bin",timeStep);
    else                      sprintf(fileName,"flow%d.bin",timeStep);
    /*printf("Restart file name is %s\n", fileName);*/
    ierr = VecGetArray(qnodeLoc, &qnode);CHKERRQ(ierr);
    printf("On Processor %d, qnode[%d] = %g\n",rank,rstart,qnode[0]);
    ierr = PetscBinaryOpen(fileName,FILE_MODE_WRITE,&fdes);CHKERRQ(ierr);
    ierr = MPI_Barrier(MPI_COMM_WORLD);
    ierr = PetscBinarySeek(fdes,bs*rstart*PETSC_BINARY_SCALAR_SIZE,PETSC_BINARY_SEEK_SET,&startPos);CHKERRQ(ierr);
    ierr = PetscBinaryWrite(fdes,qnode,bs*nnodesLoc,PETSC_SCALAR,PETSC_FALSE);CHKERRQ(ierr);
    /*write(fdes,qnode,bs*nnodesLoc*sizeof(REAL));*/
    ierr = MPI_Barrier(MPI_COMM_WORLD);
    PetscPrintf(MPI_COMM_WORLD,"Restart file written to %s\n", fileName);
    ierr = PetscBinaryClose(fdes);CHKERRQ(ierr);
    ierr = VecRestoreArray(qnodeLoc, &qnode);CHKERRQ(ierr);
  } else {
    /* Processor 0 writes the output to a file; others just send their
      pieces to it */
    if (!rank) {
      ierr = VecGetArray(qnodeLoc, &qnode);
      if (timeStep < 10)        sprintf(fileName,"flow000%d.bin",timeStep);
      else if (timeStep < 100)  sprintf(fileName,"flow00%d.bin",timeStep);
      else if (timeStep < 1000) sprintf(fileName,"flow0%d.bin",timeStep);
      else                      sprintf(fileName,"flow%d.bin",timeStep);
      printf("Restart file name is %s\n", fileName);
      ierr = PetscBinaryOpen(fileName,FILE_MODE_WRITE,&fdes);CHKERRQ(ierr);
      ierr = PetscBinaryWrite(fdes,qnode,bs*nnodesLoc,PETSC_SCALAR,PETSC_FALSE);CHKERRQ(ierr);
      /* Write the solution vector in vtk (Visualization Toolkit) format*/
      ierr = PetscOptionsHasName(NULL,"-vtk",&flg_vtk);CHKERRQ(ierr);
      if (flg_vtk) {
        if (timeStep < 10)        sprintf(fileName,"flow000%d.vtk",timeStep);
        else if (timeStep < 100)  sprintf(fileName,"flow00%d.vtk",timeStep);
        else if (timeStep < 1000) sprintf(fileName,"flow0%d.vtk",timeStep);
        else                      sprintf(fileName,"flow%d.vtk",timeStep);
        sprintf(command, "cp mesh.vtk %s", fileName);
        if ((status = system(command)) < 0)
          printf("Error in opening the file mesh.dat\n");
        fptr = fopen(fileName,"a");
        fprintf(fptr,"POINT_DATA %d\n", nnodes);
        fprintf(fptr,"SCALARS Mach double\n");
        fprintf(fptr,"LOOKUP_TABLE default\n");
        fflush(fptr);
        fptr1 = fopen("vec.vtk","w");
        fprintf(fptr1,"VECTORS Velocity double\n");
        /* Write the Mach Number */
        for (j = 0; j < nnodesLoc; j++) {
          in   = bs*j;
          mach = PetscSqrtReal((qnode[in+1]*qnode[in+1]+qnode[in+2]*qnode[in+2]+qnode[in+3]*qnode[in+3])/((pgamma*qnode[in+4])/qnode[in]));
          fprintf(fptr,"%g\n",mach);
        }
        /* Write Velocity Vector */
        for (j = 0; j < nnodesLoc; j++) {
          in = bs*j;
          fprintf(fptr1,"%g %g %g\n",qnode[in+1],qnode[in+2],qnode[in+3]);
        }
      }
      ierr = VecRestoreArray(qnodeLoc, &qnode);
    }
    for (i = 1; i < CommSize; i++) {
      if (rank == i) {
        ierr = VecGetArray(qnodeLoc, &qnode);
        ierr = MPI_Send(&nnodesLoc,1,MPI_INT,0,0,MPI_COMM_WORLD);CHKERRQ(ierr);
        ierr = MPI_Send(qnode,bs*nnodesLoc,MPI_DOUBLE,0,1,MPI_COMM_WORLD);CHKERRQ(ierr);
        ierr = VecRestoreArray(qnodeLoc, &qnode);CHKERRQ(ierr);
      }
      if (!rank) {
        int        nnodesLocIpr;
        MPI_Status mstatus;

        ierr = MPI_Recv(&nnodesLocIpr,1,MPI_INT,i,0,MPI_COMM_WORLD,&mstatus);CHKERRQ(ierr);
        FCALLOC(bs*nnodesLocIpr, &qnode);
        ierr = MPI_Recv(qnode,bs*nnodesLocIpr,MPI_DOUBLE,i,1,MPI_COMM_WORLD,&mstatus);CHKERRQ(ierr);
        ierr = PetscBinaryWrite(fdes,qnode,bs*nnodesLocIpr,PETSC_SCALAR,PETSC_FALSE);CHKERRQ(ierr);
        /* Write the solution vector in vtk (Visualization Toolkit) format*/
        if (flg_vtk != 0) {
        /* Write the Mach Number */
        for (j = 0; j < nnodesLocIpr; j++) {
          in = bs*j;
          mach = PetscSqrtReal((qnode[in+1]*qnode[in+1]+qnode[in+2]*qnode[in+2]+qnode[in+3]*qnode[in+3])/((pgamma*qnode[in+4])/qnode[in]));
          fprintf(fptr,"%g\n",mach);
        }
        /* Write Velocity Vector */
        for (j = 0; j < nnodesLocIpr; j++) {
          in = bs*j;
          fprintf(fptr1,"%g %g %g\n",qnode[in+1],qnode[in+2],qnode[in+3]);
        }
      }
      ierr = PetscFree(qnode);CHKERRQ(ierr);
      }
      ierr = MPI_Barrier(MPI_COMM_WORLD);
    }
    if (!rank) {
      ierr = PetscBinaryClose(fdes);CHKERRQ(ierr);
      printf("Restart file written to %s\n", fileName);
      if (flg_vtk) {
        fclose(fptr);
        fclose(fptr1);
        sprintf(command,"cat vec.vtk >> %s",fileName);
        if ((status = system(command)) < 0) printf("Error in appending the file vec.vtk\n");
      }
    }
  }
  ierr = VecDestroy(&qnodeLoc);CHKERRQ(ierr);
  return 0;
}

/*---------------------------------------------------------------------*/
int ReadRestartFile(GRID *grid)
/*---------------------------------------------------------------------*/
{
  int         bs = 5;
  int         fdes;
  int         i, ierr;
  int         nnodes, nnodesLoc;
  int         *loc2pet;
  PetscScalar *qnode;
  Vec         qnodeLoc;
  IS          islocal, isglobal;
  VecScatter  scatter;
  MPI_Status  status;

  nnodes    = grid->nnodes;
  nnodesLoc = grid->nnodesLoc;

  /* Processor 0 reads the input file and sends the appropriate pieces
     to others; The suitable distributed vector is constructed out of
     these pieces by doing vector scattering */
  ierr = VecCreateSeq(MPI_COMM_SELF,bs*nnodesLoc,&qnodeLoc);
  if (!rank) {
    ierr = VecGetArray(qnodeLoc, &qnode);
    ierr = PetscBinaryOpen("restart.bin",FILE_MODE_READ,&fdes);CHKERRQ(ierr);
    ierr = PetscBinaryRead(fdes,qnode,bs*nnodesLoc,PETSC_SCALAR);CHKERRQ(ierr);
    ierr = VecRestoreArray(qnodeLoc, &qnode);
  }
  for (i = 1; i < CommSize; i++) {
    if (rank == i) {
      ierr = VecGetArray(qnodeLoc, &qnode);
      ierr = MPI_Send(&nnodesLoc,1,MPI_INT,0,0,MPI_COMM_WORLD);CHKERRQ(ierr);
      ierr = MPI_Recv(qnode,bs*nnodesLoc,MPI_DOUBLE,0,1,MPI_COMM_WORLD,&status);CHKERRQ(ierr);
      ierr = VecRestoreArray(qnodeLoc, &qnode);CHKERRQ(ierr);
    }
    if (!rank) {
      int nnodesLocIpr;
      ierr = MPI_Recv(&nnodesLocIpr,1,MPI_INT,i,0,MPI_COMM_WORLD,&status);CHKERRQ(ierr);
      FCALLOC(bs*nnodesLocIpr, &qnode);
      ierr = PetscBinaryRead(fdes,qnode,bs*nnodesLocIpr,PETSC_SCALAR);CHKERRQ(ierr);
      ierr = MPI_Send(qnode,bs*nnodesLocIpr,MPI_DOUBLE,i,1,MPI_COMM_WORLD);CHKERRQ(ierr);
      ierr = PetscFree(qnode);CHKERRQ(ierr);
    }
    ierr = MPI_Barrier(MPI_COMM_WORLD);
  }
  if (!rank) ierr = PetscBinaryClose(fdes);CHKERRQ(ierr);

  /* Create a distributed vector in Petsc ordering */
  ICALLOC(nnodesLoc, &loc2pet);
  for (i = 0; i < nnodesLoc; i++) loc2pet[i] = rstart+i;
  ierr = AOApplicationToPetsc(grid->ao,nnodesLoc,loc2pet);CHKERRQ(ierr);
  ierr = ISCreateStride(MPI_COMM_SELF,bs*nnodesLoc,0,1,&islocal);CHKERRQ(ierr);
  ierr = ISCreateBlock(MPI_COMM_SELF,bs,nnodesLoc,loc2pet,PETSC_COPY_VALUES,&isglobal);CHKERRQ(ierr);
  ierr = PetscFree(loc2pet);CHKERRQ(ierr);
  ierr = VecScatterCreate(qnodeLoc,islocal,grid->qnode,isglobal,
                          &scatter);CHKERRQ(ierr);
  ierr = ISDestroy(&isglobal);CHKERRQ(ierr);
  ierr = ISDestroy(&islocal);CHKERRQ(ierr);
  ierr = VecScatterBegin(scatter,qnodeLoc,grid->qnode,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
  ierr = VecScatterEnd(scatter,qnodeLoc,grid->qnode,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
  ierr = VecScatterDestroy(&scatter);CHKERRQ(ierr);

  ierr = VecDestroy(&qnodeLoc);CHKERRQ(ierr);
  return 0;
}

/*================================= CLINK ===================================*/
/*                                                                           */
/* Used in establishing the links between FORTRAN common blocks and C        */
/*                                                                           */
/*===========================================================================*/
EXTERN_C_BEGIN
#undef __FUNCT__
#define __FUNCT__ "f77CLINK"
void f77CLINK(CINFO  *p1,CRUNGE *p2,CGMCOM *p3,CREFGEOM *p4)
{
  c_info    = p1;
  c_runge   = p2;
  c_gmcom   = p3;
  c_refgeom = p4;
}
EXTERN_C_END

/*========================== SET_UP_GRID====================================*/
/*                                                                          */
/* Allocates the memory for the fine grid                                   */
/*                                                                          */
/*==========================================================================*/
int set_up_grid(GRID *grid)
{
  int nnodes, ncell, nedge;
  int nsface, nvface, nfface, nbface, kvisc;
  int vface, tnode;
  int nsnode, nvnode, nfnode;
  int ileast, lnodes;
  int nsrch,ilu0;
  int valloc;
  int jalloc;  /* If jalloc=1 allocate space for dfp and dfm */
  int ifcn;
  int ierr;
/*
 * stuff to read in dave's grids
 */
  int nnbound,nvbound,nfbound,nnfacet,nvfacet,nffacet,ntte;
  /* end of stuff */

  nnodes = grid->nnodes;
  ncell  = grid->ncell;
  vface  = grid->nedge;
  tnode  = grid->nnodes;
  nedge  = grid->nedge;
  nsface = grid->nsface;
  nvface = grid->nvface;
  nfface = grid->nfface;
  nbface = nsface + nvface + nfface;
  nsnode = grid->nsnode;
  nvnode = grid->nvnode;
  nfnode = grid->nfnode;
  ileast = grid->ileast;
  lnodes = grid->nnodes;
  kvisc  = grid->jvisc;
  nsrch  = c_gmcom->nsrch;
  ilu0   = c_gmcom->ilu0;
  ifcn   = c_gmcom->ifcn;

  jalloc = 0;

  /*
   * stuff to read in dave's grids
   */
  nnbound = grid->nnbound;
  nvbound = grid->nvbound;
  nfbound = grid->nfbound;
  nnfacet = grid->nnfacet;
  nvfacet = grid->nvfacet;
  nffacet = grid->nffacet;
  ntte    = grid->ntte;
  /* end of stuff */


  if (ileast == 0) lnodes = 1;
  /*   printf("In set_up_grid->jvisc = %d\n",grid->jvisc); */

  if (grid->jvisc != 2 && grid->jvisc != 4 && grid->jvisc != 6) vface = 1;
  /*   printf(" vface = %d \n",vface); */
  if (grid->jvisc < 3) tnode = 1;
  valloc = 1;
  if (grid->jvisc ==  0) valloc = 0;

  /*PetscPrintf(MPI_COMM_WORLD, " nsnode= %d nvnode= %d nfnode= %d\n",nsnode,nvnode,nfnode);*/
  /*PetscPrintf(MPI_COMM_WORLD, " nsface= %d nvface= %d nfface= %d\n",nsface,nvface,nfface);
    PetscPrintf(MPI_COMM_WORLD, " nbface= %d\n", nbface);*/
  /* Now allocate memory for the other grid arrays */
  /* ICALLOC(nedge*2,   &grid->eptr); */
  ICALLOC(nsface,    &grid->isface);
  ICALLOC(nvface,    &grid->ivface);
  ICALLOC(nfface,    &grid->ifface);
  /* ICALLOC(nsnode,    &grid->isnode);
     ICALLOC(nvnode,    &grid->ivnode);
     ICALLOC(nfnode,    &grid->ifnode);*/
  /*ICALLOC(nnodes,    &grid->clist);
    ICALLOC(nnodes,    &grid->iupdate);
    ICALLOC(nsface*2,  &grid->sface);
    ICALLOC(nvface*2,  &grid->vface);
    ICALLOC(nfface*2,  &grid->fface);
    ICALLOC(lnodes,    &grid->icount);*/
  /*FCALLOC(nnodes,    &grid->x);
    FCALLOC(nnodes,    &grid->y);
    FCALLOC(nnodes,    &grid->z);
    FCALLOC(nnodes,    &grid->area);*/
  /*
   * FCALLOC(nnodes*4,  &grid->gradx);
   * FCALLOC(nnodes*4,  &grid->grady);
   * FCALLOC(nnodes*4,  &grid->gradz);
   * FCALLOC(nnodes,    &grid->cdt);
   */
  /*
   * FCALLOC(nnodes*4,  &grid->qnode);
   * FCALLOC(nnodes*4,  &grid->dq);
   * FCALLOC(nnodes*4,  &grid->res);
   * FCALLOC(jalloc*nnodes*4*4, &grid->A);
   * FCALLOC(nnodes*4,   &grid->B);
   * FCALLOC(jalloc*nedge*4*4, &grid->dfp);
   * FCALLOC(jalloc*nedge*4*4, &grid->dfm);
   */
  /*FCALLOC(nsnode,    &grid->sxn);
    FCALLOC(nsnode,    &grid->syn);
    FCALLOC(nsnode,    &grid->szn);
    FCALLOC(nsnode,    &grid->sa);
    FCALLOC(nvnode,    &grid->vxn);
    FCALLOC(nvnode,    &grid->vyn);
    FCALLOC(nvnode,    &grid->vzn);
    FCALLOC(nvnode,    &grid->va);
    FCALLOC(nfnode,    &grid->fxn);
    FCALLOC(nfnode,    &grid->fyn);
    FCALLOC(nfnode,    &grid->fzn);
    FCALLOC(nfnode,    &grid->fa);
    FCALLOC(nedge,     &grid->xn);
    FCALLOC(nedge,     &grid->yn);
    FCALLOC(nedge,     &grid->zn);
    FCALLOC(nedge,     &grid->rl);*/

  FCALLOC(nbface*15, &grid->us);
  FCALLOC(nbface*15, &grid->vs);
  FCALLOC(nbface*15, &grid->as);
  /*
   * FCALLOC(nnodes*4,  &grid->phi);
   * FCALLOC(nnodes,    &grid->r11);
   * FCALLOC(nnodes,    &grid->r12);
   * FCALLOC(nnodes,    &grid->r13);
   * FCALLOC(nnodes,    &grid->r22);
   * FCALLOC(nnodes,    &grid->r23);
   * FCALLOC(nnodes,    &grid->r33);
   */
  /*
   * Allocate memory for viscous length scale if turbulent
   */
  if (grid->jvisc >= 3) {
    FCALLOC(tnode,   &grid->slen);
    FCALLOC(nnodes,  &grid->turbre);
    FCALLOC(nnodes,  &grid->amut);
    FCALLOC(tnode,   &grid->turbres);
    FCALLOC(nedge,   &grid->dft1);
    FCALLOC(nedge,   &grid->dft2);
  }
  /*
   * Allocate memory for MG transfer
   */
  /*
    ICALLOC(mgzero*nsface,    &grid->isford);
    ICALLOC(mgzero*nvface,    &grid->ivford);
    ICALLOC(mgzero*nfface,    &grid->ifford);
    ICALLOC(mgzero*nnodes,    &grid->nflag);
    ICALLOC(mgzero*nnodes,    &grid->nnext);
    ICALLOC(mgzero*nnodes,    &grid->nneigh);
    ICALLOC(mgzero*ncell,     &grid->ctag);
    ICALLOC(mgzero*ncell,     &grid->csearch);
    ICALLOC(valloc*ncell*4,   &grid->c2n);
    ICALLOC(valloc*ncell*6,   &grid->c2e);
    grid->c2c = (int*)grid->dfp;
    ICALLOC(ncell*4,   &grid->c2c);
    ICALLOC(nnodes,    &grid->cenc);
    if (grid->iup == 1) {
       ICALLOC(mgzero*nnodes*3,  &grid->icoefup);
       FCALLOC(mgzero*nnodes*3,  &grid->rcoefup);
    }
    if (grid->idown == 1) {
       ICALLOC(mgzero*nnodes*3,  &grid->icoefdn);
       FCALLOC(mgzero*nnodes*3,  &grid->rcoefdn);
    }
    FCALLOC(nnodes*4,  &grid->ff);
    FCALLOC(tnode,     &grid->turbff);
   */
  /*
   * If using GMRES (nsrch>0) allocate memory
   */
  /* NoEq = 0;
     if (nsrch > 0)NoEq = 4*nnodes;
     if (nsrch < 0)NoEq = nnodes;
     FCALLOC(NoEq,           &grid->AP);
     FCALLOC(NoEq,           &grid->Xgm);
     FCALLOC(NoEq,           &grid->temr);
     FCALLOC((abs(nsrch)+1)*NoEq, &grid->Fgm);
   */
  /*
   * stuff to read in dave's grids
   */
  /*
    ICALLOC(nnbound,   &grid->ncolorn);
    ICALLOC(nnbound*100,&grid->countn);
    ICALLOC(nvbound,   &grid->ncolorv);
    ICALLOC(nvbound*100,&grid->countv);
    ICALLOC(nfbound,   &grid->ncolorf);
    ICALLOC(nfbound*100,&grid->countf);

    ICALLOC(nnbound,   &grid->nntet);
    ICALLOC(nnbound,   &grid->nnpts);
    ICALLOC(nvbound,   &grid->nvtet);
    ICALLOC(nvbound,   &grid->nvpts);
    ICALLOC(nfbound,   &grid->nftet);
    ICALLOC(nfbound,   &grid->nfpts);
    ICALLOC(nnfacet*4, &grid->f2ntn);
    ICALLOC(nvfacet*4, &grid->f2ntv);
    ICALLOC(nffacet*4, &grid->f2ntf);*/

  /* end of stuff */
  return 0;
}


/*========================== WRITE_FINE_GRID ================================*/
/*                                                                           */
/* Write memory locations and other information for the fine grid            */
/*                                                                           */
/*===========================================================================*/
#undef __FUNCT__
#define __FUNCT__ "write_fine_grid"
int write_fine_grid(GRID *grid)
{
  FILE *output;

  PetscFunctionBegin;
/* open file for output      */
/* call the output frame.out */

  if (!(output = fopen("frame.out","a"))) {
    SETERRQ(PETSC_COMM_SELF,1,"can't open frame.out");
  }
  fprintf(output,"information for fine grid\n");
  fprintf(output,"\n");
  fprintf(output," address of fine grid = %p\n", grid);

  fprintf(output,"grid.nnodes  = %d\n", grid->nnodes);
  fprintf(output,"grid.ncell   = %d\n", grid->ncell);
  fprintf(output,"grid.nedge   = %d\n", grid->nedge);
  fprintf(output,"grid.nsface  = %d\n", grid->nsface);
  fprintf(output,"grid.nvface  = %d\n", grid->nvface);
  fprintf(output,"grid.nfface  = %d\n", grid->nfface);
  fprintf(output,"grid.nsnode  = %d\n", grid->nsnode);
  fprintf(output,"grid.nvnode  = %d\n", grid->nvnode);
  fprintf(output,"grid.nfnode  = %d\n", grid->nfnode);

  fprintf(output,"grid.eptr    = %p\n", grid->eptr);
  fprintf(output,"grid.isface  = %p\n", grid->isface);
  fprintf(output,"grid.ivface  = %p\n", grid->ivface);
  fprintf(output,"grid.ifface  = %p\n", grid->ifface);
  fprintf(output,"grid.isnode  = %p\n", grid->isnode);
  fprintf(output,"grid.ivnode  = %p\n", grid->ivnode);
  fprintf(output,"grid.ifnode  = %p\n", grid->ifnode);
  fprintf(output,"grid.c2n     = %p\n", grid->c2n);
  fprintf(output,"grid.c2e     = %p\n", grid->c2e);
  fprintf(output,"grid.x       = %p\n", grid->x);
  fprintf(output,"grid.y       = %p\n", grid->y);
  fprintf(output,"grid.z       = %p\n", grid->z);
  fprintf(output,"grid.area    = %p\n", grid->area);
  fprintf(output,"grid.qnode   = %p\n", grid->qnode);
/*
  fprintf(output,"grid.gradx   = %p\n", grid->gradx);
  fprintf(output,"grid.grady   = %p\n", grid->grady);
  fprintf(output,"grid.gradz   = %p\n", grid->gradz);
*/
  fprintf(output,"grid.cdt     = %p\n", grid->cdt);
  fprintf(output,"grid.sxn     = %p\n", grid->sxn);
  fprintf(output,"grid.syn     = %p\n", grid->syn);
  fprintf(output,"grid.szn     = %p\n", grid->szn);
  fprintf(output,"grid.vxn     = %p\n", grid->vxn);
  fprintf(output,"grid.vyn     = %p\n", grid->vyn);
  fprintf(output,"grid.vzn     = %p\n", grid->vzn);
  fprintf(output,"grid.fxn     = %p\n", grid->fxn);
  fprintf(output,"grid.fyn     = %p\n", grid->fyn);
  fprintf(output,"grid.fzn     = %p\n", grid->fzn);
  fprintf(output,"grid.xn      = %p\n", grid->xn);
  fprintf(output,"grid.yn      = %p\n", grid->yn);
  fprintf(output,"grid.zn      = %p\n", grid->zn);
  fprintf(output,"grid.rl      = %p\n", grid->rl);
/*
* close output file
*/
  fclose(output);
  return 0;
}

#if defined(PARCH_IRIX64) && defined(USE_HW_COUNTERS)
int EventCountersBegin(int *gen_start, PetscScalar * time_start_counters)
{
  int ierr;
  if ((*gen_start = start_counters(event0,event1)) < 0) SETERRQ(PETSC_COMM_SELF,1,"Error in start_counters\n");
  ierr = PetscTime(&time_start_counters);CHKERRQ(ierr);
  return 0;
}

int EventCountersEnd(int gen_start, PetscScalar time_start_counters)
{
  int         gen_read, ierr;
  PetscScalar time_read_counters;
  long long   _counter0, _counter1;

  if ((gen_read = read_counters(event0,&_counter0,event1,&_counter1)) < 0) SETERRQ(PETSC_COMM_SELF,1,"Error in read_counter\n");
  ierr = PetscTime(&&time_read_counters);CHKERRQ(ierr);
  if (gen_read != gen_start) SETERRQ(PETSC_COMM_SELF,1,"Lost Counters!! Aborting ...\n");

  counter0      += _counter0;
  counter1      += _counter1;
  time_counters += time_read_counters-time_start_counters;
  return 0;
}
#endif
