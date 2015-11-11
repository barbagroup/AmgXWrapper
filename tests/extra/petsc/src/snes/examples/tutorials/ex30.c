static const char help[] = "Steady-state 2D subduction flow, pressure and temperature solver.\n\
       The flow is driven by the subducting slab.\n\
---------------------------------ex30 help---------------------------------\n\
  -OPTION <DEFAULT> = (UNITS) DESCRIPTION.\n\n\
  -width <320> = (km) width of domain.\n\
  -depth <300> = (km) depth of domain.\n\
  -slab_dip <45> = (degrees) dip angle of the slab (determines the grid aspect ratio).\n\
  -lid_depth <35> = (km) depth of the static conductive lid.\n\
  -fault_depth <35> = (km) depth of slab-wedge mechanical coupling\n\
     (fault dept >= lid depth).\n\
\n\
  -ni <82> = grid cells in x-direction. (nj adjusts to accommodate\n\
      the slab dip & depth). DO NOT USE -da_grid_x option!!!\n\
  -ivisc <3> = rheology option.\n\
      0 --- constant viscosity.\n\
      1 --- olivine diffusion creep rheology (T&P-dependent, newtonian).\n\
      2 --- olivine dislocation creep rheology (T&P-dependent, non-newtonian).\n\
      3 --- Full mantle rheology, combination of 1 & 2.\n\
\n\
  -slab_velocity <5> = (cm/year) convergence rate of slab into subduction zone.\n\
  -slab_age <50> = (million yrs) age of slab for thermal profile boundary condition.\n\
  -lid_age <50> = (million yrs) age of lid for thermal profile boundary condition.\n\
\n\
  FOR OTHER PARAMETER OPTIONS AND THEIR DEFAULT VALUES, see SetParams() in ex30.c.\n\
---------------------------------ex30 help---------------------------------\n";


/*F-----------------------------------------------------------------------

    This PETSc 2.2.0 example by Richard F. Katz
    http://www.ldeo.columbia.edu/~katz/

    The problem is modeled by the partial differential equation system

\begin{eqnarray}
         -\nabla P + \nabla \cdot [\eta (\nabla v + \nabla v^T)] & = & 0  \\
                                           \nabla \cdot v & = & 0   \\
                    dT/dt + \nabla \cdot (vT) - 1/Pe \triangle^2(T) & = & 0  \\
\end{eqnarray}

 \begin{eqnarray}
        \eta(T,Eps\_dot) &  = & \hbox{constant                        }    \hbox{if ivisc} ==0  \\
                      &  = & \hbox{diffusion creep (T,P-dependent)    }     \hbox{if ivisc} ==1  \\
                      &  = & \hbox{dislocation creep (T,P,v-dependent)}  \hbox{if ivisc} ==2  \\
                      &  = & \hbox{mantle viscosity (difn and disl)   }  \hbox{if ivisc} ==3
\end{eqnarray}

    which is uniformly discretized on a staggered mesh:
                      -------$w_{ij}$------
                  $u_{i-1j}$    $P,T_{ij}$   $u_{ij}$
                      ------$w_{ij-1}$-----

  ------------------------------------------------------------------------F*/

#include <petscsnes.h>
#include <petscdm.h>
#include <petscdmda.h>

#define VISC_CONST   0
#define VISC_DIFN    1
#define VISC_DISL    2
#define VISC_FULL    3
#define CELL_CENTER  0
#define CELL_CORNER  1
#define BC_ANALYTIC  0
#define BC_NOSTRESS  1
#define BC_EXPERMNT  2
#define ADVECT_FV    0
#define ADVECT_FROMM 1
#define PLATE_SLAB   0
#define PLATE_LID    1
#define EPS_ZERO     0.00000001

typedef struct { /* holds the variables to be solved for */
  PetscScalar u,w,p,T;
} Field;

typedef struct { /* parameters needed to compute viscosity */
  PetscReal A,n,Estar,Vstar;
} ViscParam;

typedef struct { /* physical and miscelaneous parameters */
  PetscReal width, depth, scaled_width, scaled_depth, peclet, potentialT;
  PetscReal slab_dip, slab_age, slab_velocity, kappa, z_scale;
  PetscReal c, d, sb, cb, skt, visc_cutoff, lid_age, eta0, continuation;
  PetscReal L, V, lid_depth, fault_depth;
  ViscParam diffusion, dislocation;
  PetscInt  ivisc, adv_scheme, ibound, output_ivisc;
  PetscBool quiet, param_test, output_to_file, pv_analytic;
  PetscBool interrupted, stop_solve, toggle_kspmon, kspmon;
  char      filename[PETSC_MAX_PATH_LEN];
} Parameter;

typedef struct { /* grid parameters */
  DMBoundaryType   bx,by;
  DMDAStencilType  stencil;
  PetscInt         corner,ni,nj,jlid,jfault,inose;
  PetscInt         dof,stencil_width,mglevels;
  PetscReal        dx,dz;
} GridInfo;

typedef struct { /* application context */
  Vec       x,Xguess;
  Parameter *param;
  GridInfo  *grid;
} AppCtx;

/* Callback functions (static interface) */
extern PetscErrorCode FormFunctionLocal(DMDALocalInfo*,Field**,Field**,void*);

/* Main routines */
extern PetscErrorCode SetParams(Parameter*, GridInfo*);
extern PetscErrorCode ReportParams(Parameter*, GridInfo*);
extern PetscErrorCode Initialize(DM);
extern PetscErrorCode UpdateSolution(SNES,AppCtx*, PetscInt*);
extern PetscErrorCode DoOutput(SNES,PetscInt);

/* Post-processing & misc */
extern PetscErrorCode ViscosityField(DM,Vec,Vec);
extern PetscErrorCode StressField(DM);
extern PetscErrorCode SNESConverged_Interactive(SNES, PetscInt, PetscReal, PetscReal, PetscReal, SNESConvergedReason*, void*);
extern PetscErrorCode InteractiveHandler(int, void*);
extern PetscBool  OptionsHasName(const char pre[],const char name[]);

/*-----------------------------------------------------------------------*/
#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc,char **argv)
/*-----------------------------------------------------------------------*/
{
  SNES           snes;
  AppCtx         *user;               /* user-defined work context */
  Parameter      param;
  GridInfo       grid;
  PetscInt       nits;
  PetscErrorCode ierr;
  MPI_Comm       comm;
  DM             da;

  PetscInitialize(&argc,&argv,(char*)0,help);
  PetscOptionsSetValue(NULL,"-file","ex30_output");
  PetscOptionsSetValue(NULL,"-snes_monitor_short",NULL);
  PetscOptionsSetValue(NULL,"-snes_max_it","20");
  PetscOptionsSetValue(NULL,"-ksp_max_it","1500");
  PetscOptionsSetValue(NULL,"-ksp_gmres_restart","300");
  PetscOptionsInsert(NULL,&argc,&argv,NULL);

  comm = PETSC_COMM_WORLD;

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Set up the problem parameters.
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  ierr = SetParams(&param,&grid);CHKERRQ(ierr);
  ierr = ReportParams(&param,&grid);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  ierr = SNESCreate(comm,&snes);CHKERRQ(ierr);
  ierr = DMDACreate2d(comm,grid.bx,grid.by,grid.stencil,grid.ni,grid.nj,PETSC_DECIDE,PETSC_DECIDE,grid.dof,grid.stencil_width,0,0,&da);CHKERRQ(ierr);
  ierr = SNESSetDM(snes,da);CHKERRQ(ierr);
  ierr = DMDASetFieldName(da,0,"x-velocity");CHKERRQ(ierr);
  ierr = DMDASetFieldName(da,1,"y-velocity");CHKERRQ(ierr);
  ierr = DMDASetFieldName(da,2,"pressure");CHKERRQ(ierr);
  ierr = DMDASetFieldName(da,3,"temperature");CHKERRQ(ierr);


  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Create user context, set problem data, create vector data structures.
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  ierr        = PetscNew(&user);CHKERRQ(ierr);
  user->param = &param;
  user->grid  = &grid;
  ierr        = DMSetApplicationContext(da,user);CHKERRQ(ierr);
  ierr        = DMCreateGlobalVector(da,&(user->Xguess));CHKERRQ(ierr);


  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Set up the SNES solver with callback functions.
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  ierr = DMDASNESSetFunctionLocal(da,INSERT_VALUES,(PetscErrorCode (*)(DMDALocalInfo*,void*,void*,void*))FormFunctionLocal,(void*)user);CHKERRQ(ierr);
  ierr = SNESSetFromOptions(snes);CHKERRQ(ierr);


  ierr = SNESSetConvergenceTest(snes,SNESConverged_Interactive,(void*)user,NULL);CHKERRQ(ierr);
  ierr = PetscPushSignalHandler(InteractiveHandler,(void*)user);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Initialize and solve the nonlinear system
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  ierr = Initialize(da);CHKERRQ(ierr);
  ierr = UpdateSolution(snes,user,&nits);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Output variables.
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  ierr = DoOutput(snes,nits);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Free work space.
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  ierr = VecDestroy(&user->Xguess);CHKERRQ(ierr);
  ierr = VecDestroy(&user->x);CHKERRQ(ierr);
  ierr = PetscFree(user);CHKERRQ(ierr);
  ierr = SNESDestroy(&snes);CHKERRQ(ierr);
  ierr = DMDestroy(&da);CHKERRQ(ierr);
  ierr = PetscPopSignalHandler();CHKERRQ(ierr);
  ierr = PetscFinalize();
  return 0;
}

/*=====================================================================
  PETSc INTERACTION FUNCTIONS (initialize & call SNESSolve)
  =====================================================================*/

/*---------------------------------------------------------------------*/
#undef __FUNCT__
#define __FUNCT__ "UpdateSolution"
/*  manages solve: adaptive continuation method  */
PetscErrorCode UpdateSolution(SNES snes, AppCtx *user, PetscInt *nits)
{
  KSP                 ksp;
  PC                  pc;
  SNESConvergedReason reason;
  Parameter           *param   = user->param;
  PetscReal           cont_incr=0.3;
  PetscInt            its;
  PetscErrorCode      ierr;
  PetscBool           q = PETSC_FALSE;
  DM                  dm;

  PetscFunctionBeginUser;
  ierr = SNESGetDM(snes,&dm);CHKERRQ(ierr);
  ierr = DMCreateGlobalVector(dm,&user->x);CHKERRQ(ierr);
  ierr = SNESGetKSP(snes,&ksp);CHKERRQ(ierr);
  ierr = KSPGetPC(ksp, &pc);CHKERRQ(ierr);
  ierr = KSPSetComputeSingularValues(ksp, PETSC_TRUE);CHKERRQ(ierr);

  *nits=0;

  /* Isoviscous solve */
  if (param->ivisc == VISC_CONST && !param->stop_solve) {
    param->ivisc = VISC_CONST;

    ierr   = SNESSolve(snes,0,user->x);CHKERRQ(ierr);
    ierr   = SNESGetConvergedReason(snes,&reason);CHKERRQ(ierr);
    ierr   = SNESGetIterationNumber(snes,&its);CHKERRQ(ierr);
    *nits += its;
    ierr   = VecCopy(user->x,user->Xguess);CHKERRQ(ierr);
    if (param->stop_solve) goto done;
  }

  /* Olivine diffusion creep */
  if (param->ivisc >= VISC_DIFN && !param->stop_solve) {
    if (!q) PetscPrintf(PETSC_COMM_WORLD,"Computing Variable Viscosity Solution\n");

    /* continuation method on viscosity cutoff */
    for (param->continuation=0.0;; param->continuation+=cont_incr) {
      if (!q) PetscPrintf(PETSC_COMM_WORLD," Continuation parameter = %g\n", (double)param->continuation);

      /* solve the non-linear system */
      ierr   = VecCopy(user->Xguess,user->x);CHKERRQ(ierr);
      ierr   = SNESSolve(snes,0,user->x);CHKERRQ(ierr);
      ierr   = SNESGetConvergedReason(snes,&reason);CHKERRQ(ierr);
      ierr   = SNESGetIterationNumber(snes,&its);CHKERRQ(ierr);
      *nits += its;
      if (!q) PetscPrintf(PETSC_COMM_WORLD," SNES iterations: %D, Cumulative: %D\n", its, *nits);
      if (param->stop_solve) goto done;

      if (reason<0) {
        /* NOT converged */
        cont_incr = -fabs(cont_incr)/2.0;
        if (fabs(cont_incr)<0.01) goto done;

      } else {
        /* converged */
        ierr = VecCopy(user->x,user->Xguess);CHKERRQ(ierr);
        if (param->continuation >= 1.0) goto done;
        if (its<=3)      cont_incr = 0.30001;
        else if (its<=8) cont_incr = 0.15001;
        else             cont_incr = 0.10001;

        if (param->continuation+cont_incr > 1.0) cont_incr = 1.0 - param->continuation;
      } /* endif reason<0 */
    }
  }
done:
  if (param->stop_solve && !q) PetscPrintf(PETSC_COMM_WORLD,"USER SIGNAL: stopping solve.\n");
  if (reason<0 && !q) PetscPrintf(PETSC_COMM_WORLD,"FAILED TO CONVERGE: stopping solve.\n");
  PetscFunctionReturn(0);
}


/*=====================================================================
  PHYSICS FUNCTIONS (compute the discrete residual)
  =====================================================================*/


/*---------------------------------------------------------------------*/
#undef __FUNCT__
#define __FUNCT__ "UInterp"
PETSC_STATIC_INLINE PetscScalar UInterp(Field **x, PetscInt i, PetscInt j)
/*---------------------------------------------------------------------*/
{
  return 0.25*(x[j][i].u+x[j+1][i].u+x[j][i+1].u+x[j+1][i+1].u);
}

/*---------------------------------------------------------------------*/
#undef __FUNCT__
#define __FUNCT__ "WInterp"
PETSC_STATIC_INLINE PetscScalar WInterp(Field **x, PetscInt i, PetscInt j)
/*---------------------------------------------------------------------*/
{
  return 0.25*(x[j][i].w+x[j+1][i].w+x[j][i+1].w+x[j+1][i+1].w);
}

/*---------------------------------------------------------------------*/
#undef __FUNCT__
#define __FUNCT__ "PInterp"
PETSC_STATIC_INLINE PetscScalar PInterp(Field **x, PetscInt i, PetscInt j)
/*---------------------------------------------------------------------*/
{
  return 0.25*(x[j][i].p+x[j+1][i].p+x[j][i+1].p+x[j+1][i+1].p);
}

/*---------------------------------------------------------------------*/
#undef __FUNCT__
#define __FUNCT__ "TInterp"
PETSC_STATIC_INLINE PetscScalar TInterp(Field **x, PetscInt i, PetscInt j)
/*---------------------------------------------------------------------*/
{
  return 0.25*(x[j][i].T+x[j+1][i].T+x[j][i+1].T+x[j+1][i+1].T);
}

/*---------------------------------------------------------------------*/
#undef __FUNCT__
#define __FUNCT__ "HorizVelocity"
/*  isoviscous analytic solution for IC */
PETSC_STATIC_INLINE PassiveScalar HorizVelocity(PetscInt i, PetscInt j, AppCtx *user)
/*---------------------------------------------------------------------*/
{
  Parameter   *param = user->param;
  GridInfo    *grid  = user->grid;
  PetscScalar st, ct, th, c=param->c, d=param->d;
  PetscReal   x, z,r;

  x  = (i - grid->jlid)*grid->dx;  z = (j - grid->jlid - 0.5)*grid->dz;
  r  = PetscSqrtReal(x*x+z*z);
  st = z/r;
  ct = x/r;
  th = atan(z/x);
  return ct*(c*th*st+d*(st+th*ct)) + st*(c*(st-th*ct)+d*th*st);
}

/*---------------------------------------------------------------------*/
#undef __FUNCT__
#define __FUNCT__ "VertVelocity"
/*  isoviscous analytic solution for IC */
PETSC_STATIC_INLINE PetscScalar VertVelocity(PetscInt i, PetscInt j, AppCtx *user)
/*---------------------------------------------------------------------*/
{
  Parameter   *param = user->param;
  GridInfo    *grid  = user->grid;
  PetscScalar st, ct, th, c=param->c, d=param->d;
  PetscReal   x, z, r;

  x = (i - grid->jlid - 0.5)*grid->dx;  z = (j - grid->jlid)*grid->dz;
  r = PetscSqrtReal(x*x+z*z); st = z/r;  ct = x/r;  th = atan(z/x);
  return st*(c*th*st+d*(st+th*ct)) - ct*(c*(st-th*ct)+d*th*st);
}

/*---------------------------------------------------------------------*/
#undef __FUNCT__
#define __FUNCT__ "Pressure"
/*  isoviscous analytic solution for IC */
PETSC_STATIC_INLINE PetscScalar Pressure(PetscInt i, PetscInt j, AppCtx *user)
/*---------------------------------------------------------------------*/
{
  Parameter   *param = user->param;
  GridInfo    *grid  = user->grid;
  PetscScalar x, z, r, st, ct, c=param->c, d=param->d;

  x = (i - grid->jlid - 0.5)*grid->dx;  z = (j - grid->jlid - 0.5)*grid->dz;
  r = PetscSqrtReal(x*x+z*z);  st = z/r;  ct = x/r;
  return (-2.0*(c*ct-d*st)/r);
}

#undef __FUNCT__
#define __FUNCT__ "CalcSecInv"
/*  computes the second invariant of the strain rate tensor */
PETSC_STATIC_INLINE PetscScalar CalcSecInv(Field **x, PetscInt i, PetscInt j, PetscInt ipos, AppCtx *user)
/*---------------------------------------------------------------------*/
{
  Parameter   *param = user->param;
  GridInfo    *grid  = user->grid;
  PetscInt    ilim   =grid->ni-1, jlim=grid->nj-1;
  PetscScalar uN,uS,uE,uW,wN,wS,wE,wW;
  PetscScalar eps11, eps12, eps22;

  if (i<j) return EPS_ZERO;
  if (i==ilim) i--;
  if (j==jlim) j--;

  if (ipos==CELL_CENTER) { /* on cell center */
    if (j<=grid->jlid) return EPS_ZERO;

    uE = x[j][i].u; uW = x[j][i-1].u;
    wN = x[j][i].w; wS = x[j-1][i].w;
    wE = WInterp(x,i,j-1);
    if (i==j) {
      uN = param->cb; wW = param->sb; 
    } else {
      uN = UInterp(x,i-1,j); wW = WInterp(x,i-1,j-1);
    }

    if (j==grid->jlid+1) uS = 0.0;
    else                 uS = UInterp(x,i-1,j-1);

  } else {       /* on CELL_CORNER */
    if (j<grid->jlid) return EPS_ZERO;

    uN = x[j+1][i].u;  uS = x[j][i].u;
    wE = x[j][i+1].w;  wW = x[j][i].w;
    if (i==j) {
      wN = param->sb;
      uW = param->cb;
    } else {
      wN = WInterp(x,i,j);
      uW = UInterp(x,i-1,j);
    }

    if (j==grid->jlid) {
      uE = 0.0;  uW = 0.0;
      uS = -uN;
      wS = -wN;
    } else {
      uE = UInterp(x,i,j);
      wS = WInterp(x,i,j-1);
    }
  }

  eps11 = (uE-uW)/grid->dx;  eps22 = (wN-wS)/grid->dz;
  eps12 = 0.5*((uN-uS)/grid->dz + (wE-wW)/grid->dx);

  return PetscSqrtReal(0.5*(eps11*eps11 + 2.0*eps12*eps12 + eps22*eps22));
}

/*---------------------------------------------------------------------*/
#undef __FUNCT__
#define __FUNCT__ "Viscosity"
/*  computes the shear viscosity */
PETSC_STATIC_INLINE PetscScalar Viscosity(PetscScalar T, PetscScalar eps, PassiveScalar z, Parameter *param)
/*---------------------------------------------------------------------*/
{
  PetscReal   result   =0.0;
  ViscParam   difn     =param->diffusion, disl=param->dislocation;
  PetscInt    iVisc    =param->ivisc;
  PetscScalar eps_scale=param->V/(param->L*1000.0);
  PetscScalar strain_power, v1, v2, P;
  PetscScalar rho_g = 32340.0, R=8.3144;

  P = rho_g*(z*param->L*1000.0); /* Pa */

  if (iVisc==VISC_CONST) {
    /* constant viscosity */
    return 1.0;
  } else if (iVisc==VISC_DIFN) {
    /* diffusion creep rheology */
    result = PetscRealPart((difn.A*PetscExpScalar((difn.Estar + P*difn.Vstar)/R/(T+273.0))/param->eta0));
  } else if (iVisc==VISC_DISL) {
    /* dislocation creep rheology */
    strain_power = PetscPowScalar(eps*eps_scale, (1.0-disl.n)/disl.n);

    result = PetscRealPart(disl.A*PetscExpScalar((disl.Estar + P*disl.Vstar)/disl.n/R/(T+273.0))*strain_power/param->eta0);
  } else if (iVisc==VISC_FULL) {
    /* dislocation/diffusion creep rheology */
    strain_power = PetscPowScalar(eps*eps_scale, (1.0-disl.n)/disl.n);

    v1 = difn.A*PetscExpScalar((difn.Estar + P*difn.Vstar)/R/(T+273.0))/param->eta0;
    v2 = disl.A*PetscExpScalar((disl.Estar + P*disl.Vstar)/disl.n/R/(T+273.0))*strain_power/param->eta0;

    result = PetscRealPart(1.0/(1.0/v1 + 1.0/v2));
  }

  /* max viscosity is param->eta0 */
  result = PetscMin(result, 1.0);
  /* min viscosity is param->visc_cutoff */
  result = PetscMax(result, param->visc_cutoff);
  /* continuation method */
  result = PetscPowReal(result,param->continuation);
  return result;
}

/*---------------------------------------------------------------------*/
#undef __FUNCT__
#define __FUNCT__ "XMomentumResidual"
/*  computes the residual of the x-component of eqn (1) above */
PETSC_STATIC_INLINE PetscScalar XMomentumResidual(Field **x, PetscInt i, PetscInt j, AppCtx *user)
/*---------------------------------------------------------------------*/
{
  Parameter   *param=user->param;
  GridInfo    *grid =user->grid;
  PetscScalar dx    = grid->dx, dz=grid->dz;
  PetscScalar etaN,etaS,etaE,etaW,epsN=0.0,epsS=0.0,epsE=0.0,epsW=0.0;
  PetscScalar TE=0.0,TN=0.0,TS=0.0,TW=0.0, dPdx, residual, z_scale;
  PetscScalar dudxW,dudxE,dudzN,dudzS,dwdxN,dwdxS;
  PetscInt    jlim = grid->nj-1;

  z_scale = param->z_scale;

  if (param->ivisc==VISC_DIFN || param->ivisc>=VISC_DISL) { /* viscosity is T-dependent */
    TS = param->potentialT * TInterp(x,i,j-1) * PetscExpScalar((j-1.0)*dz*z_scale);
    if (j==jlim) TN = TS;
    else         TN = param->potentialT * TInterp(x,i,j) * PetscExpScalar(j*dz*z_scale);
    TW = param->potentialT * x[j][i].T        * PetscExpScalar((j-0.5)*dz*z_scale);
    TE = param->potentialT * x[j][i+1].T      * PetscExpScalar((j-0.5)*dz*z_scale);
    if (param->ivisc>=VISC_DISL) { /* olivine dislocation creep */
      epsN = CalcSecInv(x,i,j,  CELL_CORNER,user);
      epsS = CalcSecInv(x,i,j-1,CELL_CORNER,user);
      epsE = CalcSecInv(x,i+1,j,CELL_CENTER,user);
      epsW = CalcSecInv(x,i,j,  CELL_CENTER,user);
    }
  }
  etaN = Viscosity(TN,epsN,dz*(j+0.5),param);
  etaS = Viscosity(TS,epsS,dz*(j-0.5),param);
  etaW = Viscosity(TW,epsW,dz*j,param);
  etaE = Viscosity(TE,epsE,dz*j,param);

  dPdx = (x[j][i+1].p - x[j][i].p)/dx;
  if (j==jlim) dudzN = etaN * (x[j][i].w   - x[j][i+1].w)/dx;
  else         dudzN = etaN * (x[j+1][i].u - x[j][i].u)  /dz;
  dudzS = etaS * (x[j][i].u    - x[j-1][i].u)/dz;
  dudxE = etaE * (x[j][i+1].u  - x[j][i].u)  /dx;
  dudxW = etaW * (x[j][i].u    - x[j][i-1].u)/dx;

  residual = -dPdx                          /* X-MOMENTUM EQUATION*/
             +(dudxE - dudxW)/dx
             +(dudzN - dudzS)/dz;

  if (param->ivisc!=VISC_CONST) {
    dwdxN = etaN * (x[j][i+1].w   - x[j][i].w)  /dx;
    dwdxS = etaS * (x[j-1][i+1].w - x[j-1][i].w)/dx;

    residual += (dudxE - dudxW)/dx + (dwdxN - dwdxS)/dz;
  }

  return residual;
}

/*---------------------------------------------------------------------*/
#undef __FUNCT__
#define __FUNCT__ "ZMomentumResidual"
/*  computes the residual of the z-component of eqn (1) above */
PETSC_STATIC_INLINE PetscScalar ZMomentumResidual(Field **x, PetscInt i, PetscInt j, AppCtx *user)
/*---------------------------------------------------------------------*/
{
  Parameter   *param=user->param;
  GridInfo    *grid =user->grid;
  PetscScalar dx    = grid->dx, dz=grid->dz;
  PetscScalar etaN  =0.0,etaS=0.0,etaE=0.0,etaW=0.0,epsN=0.0,epsS=0.0,epsE=0.0,epsW=0.0;
  PetscScalar TE    =0.0,TN=0.0,TS=0.0,TW=0.0, dPdz, residual,z_scale;
  PetscScalar dudzE,dudzW,dwdxW,dwdxE,dwdzN,dwdzS;
  PetscInt    ilim = grid->ni-1;

  /* geometric and other parameters */
  z_scale = param->z_scale;

  /* viscosity */
  if (param->ivisc==VISC_DIFN || param->ivisc>=VISC_DISL) { /* viscosity is T-dependent */
    TN = param->potentialT * x[j+1][i].T      * PetscExpScalar((j+0.5)*dz*z_scale);
    TS = param->potentialT * x[j][i].T        * PetscExpScalar((j-0.5)*dz*z_scale);
    TW = param->potentialT * TInterp(x,i-1,j) * PetscExpScalar(j*dz*z_scale);
    if (i==ilim) TE = TW;
    else         TE = param->potentialT * TInterp(x,i,j) * PetscExpScalar(j*dz*z_scale);
    if (param->ivisc>=VISC_DISL) { /* olivine dislocation creep */
      epsN = CalcSecInv(x,i,j+1,CELL_CENTER,user);
      epsS = CalcSecInv(x,i,j,  CELL_CENTER,user);
      epsE = CalcSecInv(x,i,j,  CELL_CORNER,user);
      epsW = CalcSecInv(x,i-1,j,CELL_CORNER,user);
    }
  }
  etaN = Viscosity(TN,epsN,dz*(j+1.0),param);
  etaS = Viscosity(TS,epsS,dz*(j+0.0),param);
  etaW = Viscosity(TW,epsW,dz*(j+0.5),param);
  etaE = Viscosity(TE,epsE,dz*(j+0.5),param);

  dPdz  = (x[j+1][i].p - x[j][i].p)/dz;
  dwdzN = etaN * (x[j+1][i].w - x[j][i].w)/dz;
  dwdzS = etaS * (x[j][i].w - x[j-1][i].w)/dz;
  if (i==ilim) dwdxE = etaE * (x[j][i].u   - x[j+1][i].u)/dz;
  else         dwdxE = etaE * (x[j][i+1].w - x[j][i].w)  /dx;
  dwdxW = 2.0*etaW * (x[j][i].w - x[j][i-1].w)/dx;

  /* Z-MOMENTUM */
  residual = -dPdz                 /* constant viscosity terms */
             +(dwdzN - dwdzS)/dz
             +(dwdxE - dwdxW)/dx;

  if (param->ivisc!=VISC_CONST) {
    dudzE = etaE * (x[j+1][i].u - x[j][i].u)/dz;
    dudzW = etaW * (x[j+1][i-1].u - x[j][i-1].u)/dz;

    residual += (dwdzN - dwdzS)/dz + (dudzE - dudzW)/dx;
  }

  return residual;
}

/*---------------------------------------------------------------------*/
#undef __FUNCT__
#define __FUNCT__ "ContinuityResidual"
/*  computes the residual of eqn (2) above */
PETSC_STATIC_INLINE PetscScalar ContinuityResidual(Field **x, PetscInt i, PetscInt j, AppCtx *user)
/*---------------------------------------------------------------------*/
{
  GridInfo    *grid =user->grid;
  PetscScalar uE,uW,wN,wS,dudx,dwdz;

  uW = x[j][i-1].u; uE = x[j][i].u; dudx = (uE - uW)/grid->dx;
  wS = x[j-1][i].w; wN = x[j][i].w; dwdz = (wN - wS)/grid->dz;

  return dudx + dwdz;
}

/*---------------------------------------------------------------------*/
#undef __FUNCT__
#define __FUNCT__ "EnergyResidual"
/*  computes the residual of eqn (3) above */
PETSC_STATIC_INLINE PetscScalar EnergyResidual(Field **x, PetscInt i, PetscInt j, AppCtx *user)
/*---------------------------------------------------------------------*/
{
  Parameter   *param=user->param;
  GridInfo    *grid =user->grid;
  PetscScalar dx    = grid->dx, dz=grid->dz;
  PetscInt    ilim  =grid->ni-1, jlim=grid->nj-1, jlid=grid->jlid;
  PetscScalar TE, TN, TS, TW, residual;
  PetscScalar uE,uW,wN,wS;
  PetscScalar fN,fS,fE,fW,dTdxW,dTdxE,dTdzN,dTdzS;

  dTdzN = (x[j+1][i].T - x[j][i].T)  /dz;
  dTdzS = (x[j][i].T   - x[j-1][i].T)/dz;
  dTdxE = (x[j][i+1].T - x[j][i].T)  /dx;
  dTdxW = (x[j][i].T   - x[j][i-1].T)/dx;

  residual = ((dTdzN - dTdzS)/dz + /* diffusion term */
              (dTdxE - dTdxW)/dx)*dx*dz/param->peclet;

  if (j<=jlid && i>=j) {
    /* don't advect in the lid */
    return residual;
  } else if (i<j) {
    /* beneath the slab sfc */
    uW = uE = param->cb;
    wS = wN = param->sb;
  } else {
    /* advect in the slab and wedge */
    uW = x[j][i-1].u; uE = x[j][i].u;
    wS = x[j-1][i].w; wN = x[j][i].w;
  }

  if (param->adv_scheme==ADVECT_FV || i==ilim-1 || j==jlim-1 || i==1 || j==1) {
    /* finite volume advection */
    TS = (x[j][i].T + x[j-1][i].T)/2.0;
    TN = (x[j][i].T + x[j+1][i].T)/2.0;
    TE = (x[j][i].T + x[j][i+1].T)/2.0;
    TW = (x[j][i].T + x[j][i-1].T)/2.0;
    fN = wN*TN*dx; fS = wS*TS*dx;
    fE = uE*TE*dz; fW = uW*TW*dz;

  } else {
    /* Fromm advection scheme */
    fE =     (uE *(-x[j][i+2].T + 5.0*(x[j][i+1].T+x[j][i].T)-x[j][i-1].T)/8.0
              - PetscAbsScalar(uE)*(-x[j][i+2].T + 3.0*(x[j][i+1].T-x[j][i].T)+x[j][i-1].T)/8.0)*dz;
    fW =     (uW *(-x[j][i+1].T + 5.0*(x[j][i].T+x[j][i-1].T)-x[j][i-2].T)/8.0
              - PetscAbsScalar(uW)*(-x[j][i+1].T + 3.0*(x[j][i].T-x[j][i-1].T)+x[j][i-2].T)/8.0)*dz;
    fN =     (wN *(-x[j+2][i].T + 5.0*(x[j+1][i].T+x[j][i].T)-x[j-1][i].T)/8.0
              - PetscAbsScalar(wN)*(-x[j+2][i].T + 3.0*(x[j+1][i].T-x[j][i].T)+x[j-1][i].T)/8.0)*dx;
    fS =     (wS *(-x[j+1][i].T + 5.0*(x[j][i].T+x[j-1][i].T)-x[j-2][i].T)/8.0
              - PetscAbsScalar(wS)*(-x[j+1][i].T + 3.0*(x[j][i].T-x[j-1][i].T)+x[j-2][i].T)/8.0)*dx;
  }

  residual -= (fE - fW + fN - fS);

  return residual;
}

/*---------------------------------------------------------------------*/
#undef __FUNCT__
#define __FUNCT__ "ShearStress"
/*  computes the shear stress---used on the boundaries */
PETSC_STATIC_INLINE PetscScalar ShearStress(Field **x, PetscInt i, PetscInt j, PetscInt ipos, AppCtx *user)
/*---------------------------------------------------------------------*/
{
  Parameter   *param=user->param;
  GridInfo    *grid =user->grid;
  PetscInt    ilim  =grid->ni-1, jlim=grid->nj-1;
  PetscScalar uN, uS, wE, wW;

  if (j<=grid->jlid || i<j || i==ilim || j==jlim) return EPS_ZERO;

  if (ipos==CELL_CENTER) { /* on cell center */

    wE = WInterp(x,i,j-1);
    if (i==j) {
      wW = param->sb;
      uN = param->cb;
    } else {
      wW = WInterp(x,i-1,j-1);
      uN = UInterp(x,i-1,j);
    }
    if (j==grid->jlid+1) uS = 0.0;
    else                 uS = UInterp(x,i-1,j-1);

  } else { /* on cell corner */

    uN = x[j+1][i].u;         uS = x[j][i].u;
    wW = x[j][i].w;           wE = x[j][i+1].w;

  }

  return (uN-uS)/grid->dz + (wE-wW)/grid->dx;
}

/*---------------------------------------------------------------------*/
#undef __FUNCT__
#define __FUNCT__ "XNormalStress"
/*  computes the normal stress---used on the boundaries */
PETSC_STATIC_INLINE PetscScalar XNormalStress(Field **x, PetscInt i, PetscInt j, PetscInt ipos, AppCtx *user)
/*---------------------------------------------------------------------*/
{
  Parameter   *param=user->param;
  GridInfo    *grid =user->grid;
  PetscScalar dx    = grid->dx, dz=grid->dz;
  PetscInt    ilim  =grid->ni-1, jlim=grid->nj-1, ivisc;
  PetscScalar epsC  =0.0, etaC, TC, uE, uW, pC, z_scale;
  if (i<j || j<=grid->jlid) return EPS_ZERO;

  ivisc=param->ivisc;  z_scale = param->z_scale;

  if (ipos==CELL_CENTER) { /* on cell center */

    TC = param->potentialT * x[j][i].T * PetscExpScalar((j-0.5)*dz*z_scale);
    if (ivisc>=VISC_DISL) epsC = CalcSecInv(x,i,j,CELL_CENTER,user);
    etaC = Viscosity(TC,epsC,dz*j,param);

    uW = x[j][i-1].u;   uE = x[j][i].u;
    pC = x[j][i].p;

  } else { /* on cell corner */
    if (i==ilim || j==jlim) return EPS_ZERO;

    TC = param->potentialT * TInterp(x,i,j) * PetscExpScalar(j*dz*z_scale);
    if (ivisc>=VISC_DISL) epsC = CalcSecInv(x,i,j,CELL_CORNER,user);
    etaC = Viscosity(TC,epsC,dz*(j+0.5),param);

    if (i==j) uW = param->sb;
    else      uW = UInterp(x,i-1,j);
    uE = UInterp(x,i,j); pC = PInterp(x,i,j);
  }

  return 2.0*etaC*(uE-uW)/dx - pC;
}

/*---------------------------------------------------------------------*/
#undef __FUNCT__
#define __FUNCT__ "ZNormalStress"
/*  computes the normal stress---used on the boundaries */
PETSC_STATIC_INLINE PetscScalar ZNormalStress(Field **x, PetscInt i, PetscInt j, PetscInt ipos, AppCtx *user)
/*---------------------------------------------------------------------*/
{
  Parameter   *param=user->param;
  GridInfo    *grid =user->grid;
  PetscScalar dz    =grid->dz;
  PetscInt    ilim  =grid->ni-1, jlim=grid->nj-1, ivisc;
  PetscScalar epsC  =0.0, etaC, TC;
  PetscScalar pC, wN, wS, z_scale;
  if (i<j || j<=grid->jlid) return EPS_ZERO;

  ivisc=param->ivisc;  z_scale = param->z_scale;

  if (ipos==CELL_CENTER) { /* on cell center */

    TC = param->potentialT * x[j][i].T * PetscExpScalar((j-0.5)*dz*z_scale);
    if (ivisc>=VISC_DISL) epsC = CalcSecInv(x,i,j,CELL_CENTER,user);
    etaC = Viscosity(TC,epsC,dz*j,param);
    wN   = x[j][i].w; wS = x[j-1][i].w; pC = x[j][i].p;

  } else { /* on cell corner */
    if ((i==ilim) || (j==jlim)) return EPS_ZERO;

    TC = param->potentialT * TInterp(x,i,j) * PetscExpScalar(j*dz*z_scale);
    if (ivisc>=VISC_DISL) epsC = CalcSecInv(x,i,j,CELL_CORNER,user);
    etaC = Viscosity(TC,epsC,dz*(j+0.5),param);
    if (i==j) wN = param->sb;
    else      wN = WInterp(x,i,j);
    wS = WInterp(x,i,j-1); pC = PInterp(x,i,j);
  }

  return 2.0*etaC*(wN-wS)/dz - pC;
}

/*---------------------------------------------------------------------*/

/*=====================================================================
  INITIALIZATION, POST-PROCESSING AND OUTPUT FUNCTIONS
  =====================================================================*/

/*---------------------------------------------------------------------*/
#undef __FUNCT__
#define __FUNCT__ "SetParams"
/* initializes the problem parameters and checks for
   command line changes */
PetscErrorCode SetParams(Parameter *param, GridInfo *grid)
/*---------------------------------------------------------------------*/
{
  PetscErrorCode ierr, ierr_out=0;
  PetscReal      SEC_PER_YR                    = 3600.00*24.00*365.2500;
  PetscReal      alpha_g_on_cp_units_inverse_km=4.0e-5*9.8;

  /* domain geometry */
  param->slab_dip    = 45.0;
  param->width       = 320.0;                                              /* km */
  param->depth       = 300.0;                                              /* km */
  param->lid_depth   = 35.0;                                               /* km */
  param->fault_depth = 35.0;                                               /* km */

  ierr = PetscOptionsGetReal(NULL,NULL,"-slab_dip",&(param->slab_dip),NULL);CHKERRQ(ierr);
  ierr = PetscOptionsGetReal(NULL,NULL,"-width",&(param->width),NULL);CHKERRQ(ierr);
  ierr = PetscOptionsGetReal(NULL,NULL,"-depth",&(param->depth),NULL);CHKERRQ(ierr);
  ierr = PetscOptionsGetReal(NULL,NULL,"-lid_depth",&(param->lid_depth),NULL);CHKERRQ(ierr);
  ierr = PetscOptionsGetReal(NULL,NULL,"-fault_depth",&(param->fault_depth),NULL);CHKERRQ(ierr);

  param->slab_dip = param->slab_dip*PETSC_PI/180.0;                    /* radians */

  /* grid information */
  ierr     = PetscOptionsGetInt(NULL,NULL, "-jfault",&(grid->jfault),NULL);CHKERRQ(ierr);
  grid->ni = 82;
  ierr     = PetscOptionsGetInt(NULL,NULL, "-ni",&(grid->ni),NULL);CHKERRQ(ierr);

  grid->dx            = param->width/((PetscReal)(grid->ni-2));               /* km */
  grid->dz            = grid->dx*tan(param->slab_dip);                     /* km */
  grid->nj            = (PetscInt)(param->depth/grid->dz + 3.0);         /* gridpoints*/
  param->depth        = grid->dz*(grid->nj-2);                             /* km */
  grid->inose         = 0;                                          /* gridpoints*/
  ierr                = PetscOptionsGetInt(NULL,NULL,"-inose",&(grid->inose),NULL);CHKERRQ(ierr);
  grid->bx            = DM_BOUNDARY_NONE;
  grid->by            = DM_BOUNDARY_NONE;
  grid->stencil       = DMDA_STENCIL_BOX;
  grid->dof           = 4;
  grid->stencil_width = 2;
  grid->mglevels      = 1;

  /* boundary conditions */
  param->pv_analytic = PETSC_FALSE;
  param->ibound      = BC_NOSTRESS;
  ierr               = PetscOptionsGetInt(NULL,NULL,"-ibound",&(param->ibound),NULL);CHKERRQ(ierr);

  /* physical constants */
  param->slab_velocity = 5.0;               /* cm/yr */
  param->slab_age      = 50.0;              /* Ma */
  param->lid_age       = 50.0;              /* Ma */
  param->kappa         = 0.7272e-6;         /* m^2/sec */
  param->potentialT    = 1300.0;            /* degrees C */

  ierr = PetscOptionsGetReal(NULL,NULL,"-slab_velocity",&(param->slab_velocity),NULL);CHKERRQ(ierr);
  ierr = PetscOptionsGetReal(NULL,NULL,"-slab_age",&(param->slab_age),NULL);CHKERRQ(ierr);
  ierr = PetscOptionsGetReal(NULL,NULL,"-lid_age",&(param->lid_age),NULL);CHKERRQ(ierr);
  ierr = PetscOptionsGetReal(NULL,NULL,"-kappa",&(param->kappa),NULL);CHKERRQ(ierr);
  ierr = PetscOptionsGetReal(NULL,NULL,"-potentialT",&(param->potentialT),NULL);CHKERRQ(ierr);

  /* viscosity */
  param->ivisc        = 3;                  /* 0=isovisc, 1=difn creep, 2=disl creep, 3=full */
  param->eta0         = 1e24;               /* Pa-s */
  param->visc_cutoff  = 0.0;                /* factor of eta_0 */
  param->continuation = 1.0;

  /* constants for diffusion creep */
  param->diffusion.A     = 1.8e7;             /* Pa-s */
  param->diffusion.n     = 1.0;               /* dim'less */
  param->diffusion.Estar = 375e3;             /* J/mol */
  param->diffusion.Vstar = 5e-6;              /* m^3/mol */

  /* constants for param->dislocationocation creep */
  param->dislocation.A     = 2.8969e4;        /* Pa-s */
  param->dislocation.n     = 3.5;             /* dim'less */
  param->dislocation.Estar = 530e3;           /* J/mol */
  param->dislocation.Vstar = 14e-6;           /* m^3/mol */

  ierr = PetscOptionsGetInt(NULL,NULL, "-ivisc",&(param->ivisc),NULL);CHKERRQ(ierr);
  ierr = PetscOptionsGetReal(NULL,NULL,"-visc_cutoff",&(param->visc_cutoff),NULL);CHKERRQ(ierr);

  param->output_ivisc = param->ivisc;

  ierr = PetscOptionsGetInt(NULL,NULL,"-output_ivisc",&(param->output_ivisc),NULL);CHKERRQ(ierr);
  ierr = PetscOptionsGetReal(NULL,NULL,"-vstar",&(param->dislocation.Vstar),NULL);CHKERRQ(ierr);

  /* output options */
  param->quiet      = PETSC_FALSE;
  param->param_test = PETSC_FALSE;

  ierr = PetscOptionsHasName(NULL,NULL,"-quiet",&(param->quiet));CHKERRQ(ierr);
  ierr = PetscOptionsHasName(NULL,NULL,"-test",&(param->param_test));CHKERRQ(ierr);
  ierr = PetscOptionsGetString(NULL,NULL,"-file",param->filename,PETSC_MAX_PATH_LEN,&(param->output_to_file));

  /* advection */
  param->adv_scheme = ADVECT_FROMM;       /* advection scheme: 0=finite vol, 1=Fromm */

  ierr = PetscOptionsGetInt(NULL,NULL,"-adv_scheme",&(param->adv_scheme),NULL);CHKERRQ(ierr);

  /* misc. flags */
  param->stop_solve    = PETSC_FALSE;
  param->interrupted   = PETSC_FALSE;
  param->kspmon        = PETSC_FALSE;
  param->toggle_kspmon = PETSC_FALSE;

  /* derived parameters for slab angle */
  param->sb = PetscSinReal(param->slab_dip);
  param->cb = PetscCosReal(param->slab_dip);
  param->c  =  param->slab_dip*param->sb/(param->slab_dip*param->slab_dip-param->sb*param->sb);
  param->d  = (param->slab_dip*param->cb-param->sb)/(param->slab_dip*param->slab_dip-param->sb*param->sb);

  /* length, velocity and time scale for non-dimensionalization */
  param->L = PetscMin(param->width,param->depth);               /* km */
  param->V = param->slab_velocity/100.0/SEC_PER_YR;             /* m/sec */

  /* other unit conversions and derived parameters */
  param->scaled_width = param->width/param->L;                  /* dim'less */
  param->scaled_depth = param->depth/param->L;                  /* dim'less */
  param->lid_depth    = param->lid_depth/param->L;              /* dim'less */
  param->fault_depth  = param->fault_depth/param->L;            /* dim'less */
  grid->dx            = grid->dx/param->L;                      /* dim'less */
  grid->dz            = grid->dz/param->L;                      /* dim'less */
  grid->jlid          = (PetscInt)(param->lid_depth/grid->dz);       /* gridcells */
  grid->jfault        = (PetscInt)(param->fault_depth/grid->dz);     /* gridcells */
  param->lid_depth    = grid->jlid*grid->dz;                    /* dim'less */
  param->fault_depth  = grid->jfault*grid->dz;                  /* dim'less */
  grid->corner        = grid->jlid+1;                           /* gridcells */
  param->peclet       = param->V                                /* m/sec */
                        * param->L*1000.0                       /* m */
                        / param->kappa;                         /* m^2/sec */
  param->z_scale = param->L * alpha_g_on_cp_units_inverse_km;
  param->skt     = PetscSqrtReal(param->kappa*param->slab_age*SEC_PER_YR);
  ierr           = PetscOptionsGetReal(NULL,NULL,"-peclet",&(param->peclet),NULL);CHKERRQ(ierr);

  return ierr_out;
}

/*---------------------------------------------------------------------*/
#undef __FUNCT__
#define __FUNCT__ "ReportParams"
/*  prints a report of the problem parameters to stdout */
PetscErrorCode ReportParams(Parameter *param, GridInfo *grid)
/*---------------------------------------------------------------------*/
{
  PetscErrorCode ierr, ierr_out=0;
  char           date[30];

  ierr = PetscGetDate(date,30);CHKERRQ(ierr);

  if (!(param->quiet)) {
    PetscPrintf(PETSC_COMM_WORLD,"---------------------BEGIN ex30 PARAM REPORT-------------------\n");
    /* PetscPrintf(PETSC_COMM_WORLD,"                   %s\n",&(date[0]));*/

    PetscPrintf(PETSC_COMM_WORLD,"Domain: \n");
    ierr = PetscPrintf(PETSC_COMM_WORLD,"  Width = %g km,         Depth = %g km\n",(double)param->width,(double)param->depth);CHKERRQ(ierr);
    ierr = PetscPrintf(PETSC_COMM_WORLD,"  Slab dip = %g degrees,  Slab velocity = %g cm/yr\n",(double)(param->slab_dip*180.0/PETSC_PI),(double)param->slab_velocity);CHKERRQ(ierr);
    ierr = PetscPrintf(PETSC_COMM_WORLD,"  Lid depth = %5.2f km,   Fault depth = %5.2f km\n",param->lid_depth*param->L,param->fault_depth*param->L);CHKERRQ(ierr);

    PetscPrintf(PETSC_COMM_WORLD,"\nGrid: \n");
    ierr = PetscPrintf(PETSC_COMM_WORLD,"  [ni,nj] = %D, %D       [dx,dz] = %g, %g km\n",grid->ni,grid->nj,(double)grid->dx*param->L,(double)(grid->dz*param->L));CHKERRQ(ierr);
    ierr = PetscPrintf(PETSC_COMM_WORLD,"  jlid = %3D              jfault = %3D \n",grid->jlid,grid->jfault);CHKERRQ(ierr);
    ierr = PetscPrintf(PETSC_COMM_WORLD,"  Pe = %g\n",(double)param->peclet);CHKERRQ(ierr);

    PetscPrintf(PETSC_COMM_WORLD,"\nRheology:");
    if (param->ivisc==VISC_CONST) {
      ierr = PetscPrintf(PETSC_COMM_WORLD,"                 Isoviscous \n");CHKERRQ(ierr);
      if (param->pv_analytic) {
        ierr = PetscPrintf(PETSC_COMM_WORLD,"                          Pressure and Velocity prescribed! \n");CHKERRQ(ierr);
      }
    } else if (param->ivisc==VISC_DIFN) {
      ierr = PetscPrintf(PETSC_COMM_WORLD,"                 Diffusion Creep (T-Dependent Newtonian) \n");CHKERRQ(ierr);
      ierr = PetscPrintf(PETSC_COMM_WORLD,"                          Viscosity range: %g--%g Pa-sec \n",(double)param->eta0,(double)(param->visc_cutoff*param->eta0));CHKERRQ(ierr);
    } else if (param->ivisc==VISC_DISL) {
      ierr = PetscPrintf(PETSC_COMM_WORLD,"                 Dislocation Creep (T-Dependent Non-Newtonian) \n");CHKERRQ(ierr);
      ierr = PetscPrintf(PETSC_COMM_WORLD,"                          Viscosity range: %g--%g Pa-sec \n",(double)param->eta0,(double)(param->visc_cutoff*param->eta0));CHKERRQ(ierr);
    } else if (param->ivisc==VISC_FULL) {
      ierr = PetscPrintf(PETSC_COMM_WORLD,"                 Full Rheology \n");CHKERRQ(ierr);
      ierr = PetscPrintf(PETSC_COMM_WORLD,"                          Viscosity range: %g--%g Pa-sec \n",(double)param->eta0,(double)(param->visc_cutoff*param->eta0));CHKERRQ(ierr);
    } else {
      ierr     = PetscPrintf(PETSC_COMM_WORLD,"                 Invalid! \n");CHKERRQ(ierr);
      ierr_out = 1;
    }

    PetscPrintf(PETSC_COMM_WORLD,"Boundary condition:");
    if (param->ibound==BC_ANALYTIC) {
      ierr = PetscPrintf(PETSC_COMM_WORLD,"       Isoviscous Analytic Dirichlet \n");CHKERRQ(ierr);
    } else if (param->ibound==BC_NOSTRESS) {
      ierr = PetscPrintf(PETSC_COMM_WORLD,"       Stress-Free (normal & shear stress)\n");CHKERRQ(ierr);
    } else if (param->ibound==BC_EXPERMNT) {
      ierr = PetscPrintf(PETSC_COMM_WORLD,"       Experimental boundary condition \n");CHKERRQ(ierr);
    } else {
      ierr     = PetscPrintf(PETSC_COMM_WORLD,"       Invalid! \n");CHKERRQ(ierr);
      ierr_out = 1;
    }

    if (param->output_to_file)
#if defined(PETSC_HAVE_MATLAB_ENGINE)
      PetscPrintf(PETSC_COMM_WORLD,"Output Destination:       Mat file \"%s\"\n",param->filename);
#else
      PetscPrintf(PETSC_COMM_WORLD,"Output Destination:       PETSc binary file \"%s\"\n",param->filename);
#endif
    if (param->output_ivisc != param->ivisc) {
      PetscPrintf(PETSC_COMM_WORLD,"                          Output viscosity: -ivisc %D\n",param->output_ivisc);
    }

    ierr = PetscPrintf(PETSC_COMM_WORLD,"---------------------END ex30 PARAM REPORT---------------------\n");CHKERRQ(ierr);
  }
  if (param->param_test) PetscEnd();
  return ierr_out;
}

/* ------------------------------------------------------------------- */
#undef __FUNCT__
#define __FUNCT__ "Initialize"
/*  generates an inital guess using the analytic solution for isoviscous
    corner flow */
PetscErrorCode Initialize(DM da)
/* ------------------------------------------------------------------- */
{
  AppCtx         *user;
  Parameter      *param;
  GridInfo       *grid;
  PetscInt       i,j,is,js,im,jm;
  PetscErrorCode ierr;
  Field          **x;
  Vec            Xguess;

  /* Get the fine grid */
  ierr   = DMGetApplicationContext(da,&user);CHKERRQ(ierr);
  Xguess = user->Xguess;
  param  = user->param;
  grid   = user->grid;
  ierr   = DMDAGetCorners(da,&is,&js,NULL,&im,&jm,NULL);CHKERRQ(ierr);
  ierr   = DMDAVecGetArray(da,Xguess,(void**)&x);CHKERRQ(ierr);

  /* Compute initial guess */
  for (j=js; j<js+jm; j++) {
    for (i=is; i<is+im; i++) {
      if (i<j)                x[j][i].u = param->cb;
      else if (j<=grid->jlid) x[j][i].u = 0.0;
      else                    x[j][i].u = HorizVelocity(i,j,user);

      if (i<=j)               x[j][i].w = param->sb;
      else if (j<=grid->jlid) x[j][i].w = 0.0;
      else                    x[j][i].w = VertVelocity(i,j,user);

      if (i<j || j<=grid->jlid) x[j][i].p = 0.0;
      else                      x[j][i].p = Pressure(i,j,user);

      x[j][i].T = PetscMin(grid->dz*(j-0.5),1.0);
    }
  }

  /* Restore x to Xguess */
  ierr = DMDAVecRestoreArray(da,Xguess,(void**)&x);CHKERRQ(ierr);

  return 0;
}

/*---------------------------------------------------------------------*/
#undef __FUNCT__
#define __FUNCT__ "DoOutput"
/*  controls output to a file */
PetscErrorCode DoOutput(SNES snes, PetscInt its)
/*---------------------------------------------------------------------*/
{
  AppCtx         *user;
  Parameter      *param;
  GridInfo       *grid;
  PetscInt       ivt;
  PetscErrorCode ierr;
  PetscMPIInt    rank;
  PetscViewer    viewer;
  Vec            res, pars;
  MPI_Comm       comm;
  DM             da;

  ierr  = SNESGetDM(snes,&da);CHKERRQ(ierr);
  ierr  = DMGetApplicationContext(da,&user);CHKERRQ(ierr);
  param = user->param;
  grid  = user->grid;
  ivt   = param->ivisc;

  param->ivisc = param->output_ivisc;

  /* compute final residual and final viscosity/strain rate fields */
  ierr = SNESGetFunction(snes, &res, NULL, NULL);CHKERRQ(ierr);
  ierr = ViscosityField(da, user->x, user->Xguess);CHKERRQ(ierr);

  /* get the communicator and the rank of the processor */
  ierr = PetscObjectGetComm((PetscObject)snes, &comm);CHKERRQ(ierr);
  ierr = MPI_Comm_rank(comm, &rank);CHKERRQ(ierr);

  if (param->output_to_file) { /* send output to binary file */
    ierr = VecCreate(comm, &pars);CHKERRQ(ierr);
    if (!rank) { /* on processor 0 */
      ierr = VecSetSizes(pars, 20, PETSC_DETERMINE);CHKERRQ(ierr);
      ierr = VecSetFromOptions(pars);CHKERRQ(ierr);
      ierr = VecSetValue(pars,0, (PetscScalar)(grid->ni),INSERT_VALUES);
      ierr = VecSetValue(pars,1, (PetscScalar)(grid->nj),INSERT_VALUES);
      ierr = VecSetValue(pars,2, (PetscScalar)(grid->dx),INSERT_VALUES);
      ierr = VecSetValue(pars,3, (PetscScalar)(grid->dz),INSERT_VALUES);
      ierr = VecSetValue(pars,4, (PetscScalar)(param->L),INSERT_VALUES);
      ierr = VecSetValue(pars,5, (PetscScalar)(param->V),INSERT_VALUES);
      /* skipped 6 intentionally */
      ierr = VecSetValue(pars,7, (PetscScalar)(param->slab_dip),INSERT_VALUES);
      ierr = VecSetValue(pars,8, (PetscScalar)(grid->jlid),INSERT_VALUES);
      ierr = VecSetValue(pars,9, (PetscScalar)(param->lid_depth),INSERT_VALUES);
      ierr = VecSetValue(pars,10,(PetscScalar)(grid->jfault),INSERT_VALUES);
      ierr = VecSetValue(pars,11,(PetscScalar)(param->fault_depth),INSERT_VALUES);
      ierr = VecSetValue(pars,12,(PetscScalar)(param->potentialT),INSERT_VALUES);
      ierr = VecSetValue(pars,13,(PetscScalar)(param->ivisc),INSERT_VALUES);
      ierr = VecSetValue(pars,14,(PetscScalar)(param->visc_cutoff),INSERT_VALUES);
      ierr = VecSetValue(pars,15,(PetscScalar)(param->ibound),INSERT_VALUES);
      ierr = VecSetValue(pars,16,(PetscScalar)(its),INSERT_VALUES);
    } else { /* on some other processor */
      ierr = VecSetSizes(pars, 0, PETSC_DETERMINE);CHKERRQ(ierr);
      ierr = VecSetFromOptions(pars);CHKERRQ(ierr);
    }
    ierr = VecAssemblyBegin(pars);CHKERRQ(ierr); ierr = VecAssemblyEnd(pars);CHKERRQ(ierr);

    /* create viewer */
#if defined(PETSC_HAVE_MATLAB_ENGINE)
    ierr = PetscViewerMatlabOpen(PETSC_COMM_WORLD,param->filename,FILE_MODE_WRITE,&viewer);CHKERRQ(ierr);
#else
    ierr = PetscViewerBinaryOpen(PETSC_COMM_WORLD,param->filename,FILE_MODE_WRITE,&viewer);CHKERRQ(ierr);
#endif

    /* send vectors to viewer */
    ierr = PetscObjectSetName((PetscObject)res,"res");
    ierr = VecView(res,viewer);CHKERRQ(ierr);
    ierr = PetscObjectSetName((PetscObject)user->x,"out");
    ierr = VecView(user->x, viewer);CHKERRQ(ierr);
    ierr = PetscObjectSetName((PetscObject)(user->Xguess),"aux");
    ierr = VecView(user->Xguess, viewer);CHKERRQ(ierr);
    ierr = StressField(da);CHKERRQ(ierr); /* compute stress fields */
    ierr = PetscObjectSetName((PetscObject)(user->Xguess),"str");
    ierr = VecView(user->Xguess, viewer);CHKERRQ(ierr);
    ierr = PetscObjectSetName((PetscObject)pars,"par");
    ierr = VecView(pars, viewer);CHKERRQ(ierr);

    /* destroy viewer and vector */
    ierr = PetscViewerDestroy(&viewer);CHKERRQ(ierr);
    ierr = VecDestroy(&pars);CHKERRQ(ierr);
  }

  param->ivisc = ivt;
  return 0;
}

/* ------------------------------------------------------------------- */
#undef __FUNCT__
#define __FUNCT__ "ViscosityField"
/* Compute both the second invariant of the strain rate tensor and the viscosity, at both cell centers and cell corners */
PetscErrorCode ViscosityField(DM da, Vec X, Vec V)
/* ------------------------------------------------------------------- */
{
  AppCtx         *user;
  Parameter      *param;
  GridInfo       *grid;
  Vec            localX;
  Field          **v, **x;
  PassiveReal    eps, /* dx,*/ dz, T, epsC, TC;
  PetscInt       i,j,is,js,im,jm,ilim,jlim,ivt;
  PetscErrorCode ierr;

  PetscFunctionBeginUser;
  ierr         = DMGetApplicationContext(da,&user);CHKERRQ(ierr);
  param        = user->param;
  grid         = user->grid;
  ivt          = param->ivisc;
  param->ivisc = param->output_ivisc;

  ierr = DMGetLocalVector(da, &localX);CHKERRQ(ierr);
  ierr = DMGlobalToLocalBegin(da, X, INSERT_VALUES, localX);CHKERRQ(ierr);
  ierr = DMGlobalToLocalEnd(da, X, INSERT_VALUES, localX);CHKERRQ(ierr);
  ierr = DMDAVecGetArray(da,localX,(void**)&x);CHKERRQ(ierr);
  ierr = DMDAVecGetArray(da,V,(void**)&v);CHKERRQ(ierr);

  /* Parameters */
  /* dx = grid->dx; */ dz = grid->dz;

  ilim = grid->ni-1; jlim = grid->nj-1;

  /* Compute real temperature, strain rate and viscosity */
  ierr = DMDAGetCorners(da,&is,&js,NULL,&im,&jm,NULL);CHKERRQ(ierr);
  for (j=js; j<js+jm; j++) {
    for (i=is; i<is+im; i++) {
      T = PetscRealPart(param->potentialT * x[j][i].T * PetscExpScalar((j-0.5)*dz*param->z_scale));
      if (i<ilim && j<jlim) {
        TC = PetscRealPart(param->potentialT * TInterp(x,i,j) * PetscExpScalar(j*dz*param->z_scale));
      } else {
        TC = T;
      }
      eps  = PetscRealPart((CalcSecInv(x,i,j,CELL_CENTER,user)));
      epsC = PetscRealPart(CalcSecInv(x,i,j,CELL_CORNER,user));

      v[j][i].u = eps;
      v[j][i].w = epsC;
      v[j][i].p = Viscosity(T,eps,dz*(j-0.5),param);
      v[j][i].T = Viscosity(TC,epsC,dz*j,param);
    }
  }
  ierr = DMDAVecRestoreArray(da,V,(void**)&v);CHKERRQ(ierr);
  ierr = DMDAVecRestoreArray(da,localX,(void**)&x);CHKERRQ(ierr);
  ierr = DMRestoreLocalVector(da, &localX);CHKERRQ(ierr);

  param->ivisc = ivt;
  PetscFunctionReturn(0);
}

/* ------------------------------------------------------------------- */
#undef __FUNCT__
#define __FUNCT__ "StressField"
/* post-processing: compute stress everywhere */
PetscErrorCode StressField(DM da)
/* ------------------------------------------------------------------- */
{
  AppCtx         *user;
  PetscInt       i,j,is,js,im,jm;
  PetscErrorCode ierr;
  Vec            locVec;
  Field          **x, **y;

  ierr = DMGetApplicationContext(da,&user);CHKERRQ(ierr);

  /* Get the fine grid of Xguess and X */
  ierr = DMDAGetCorners(da,&is,&js,NULL,&im,&jm,NULL);CHKERRQ(ierr);
  ierr = DMDAVecGetArray(da,user->Xguess,(void**)&x);CHKERRQ(ierr);

  ierr = DMGetLocalVector(da, &locVec);CHKERRQ(ierr);
  ierr = DMGlobalToLocalBegin(da, user->x, INSERT_VALUES, locVec);CHKERRQ(ierr);
  ierr = DMGlobalToLocalEnd(da, user->x, INSERT_VALUES, locVec);CHKERRQ(ierr);
  ierr = DMDAVecGetArray(da,locVec,(void**)&y);CHKERRQ(ierr);

  /* Compute stress on the corner points */
  for (j=js; j<js+jm; j++) {
    for (i=is; i<is+im; i++) {
      x[j][i].u = ShearStress(y,i,j,CELL_CENTER,user);
      x[j][i].w = ShearStress(y,i,j,CELL_CORNER,user);
      x[j][i].p = XNormalStress(y,i,j,CELL_CENTER,user);
      x[j][i].T = ZNormalStress(y,i,j,CELL_CENTER,user);
    }
  }

  /* Restore the fine grid of Xguess and X */
  ierr = DMDAVecRestoreArray(da,user->Xguess,(void**)&x);CHKERRQ(ierr);
  ierr = DMDAVecRestoreArray(da,locVec,(void**)&y);CHKERRQ(ierr);
  ierr = DMRestoreLocalVector(da, &locVec);CHKERRQ(ierr);
  return 0;
}

/*=====================================================================
  UTILITY FUNCTIONS
  =====================================================================*/

/*---------------------------------------------------------------------*/
#undef __FUNCT__
#define __FUNCT__ "SlabVel"
/* returns the velocity of the subducting slab and handles fault nodes
   for BC */
PETSC_STATIC_INLINE PassiveScalar SlabVel(char c, PetscInt i, PetscInt j, AppCtx *user)
/*---------------------------------------------------------------------*/
{
  Parameter *param = user->param;
  GridInfo  *grid  = user->grid;

  if (c=='U' || c=='u') {
    if (i<j-1) return param->cb;
    else if (j<=grid->jfault) return 0.0;
    else return param->cb;

  } else {
    if (i<j) return param->sb;
    else if (j<=grid->jfault) return 0.0;
    else return param->sb;
  }
}

/*---------------------------------------------------------------------*/
#undef __FUNCT__
#define __FUNCT__ "PlateModel"
/*  solution to diffusive half-space cooling model for BC */
PETSC_STATIC_INLINE PassiveScalar PlateModel(PetscInt j, PetscInt plate, AppCtx *user)
/*---------------------------------------------------------------------*/
{
  Parameter     *param = user->param;
  PassiveScalar z;
  if (plate==PLATE_LID) z = (j-0.5)*user->grid->dz;
  else z = (j-0.5)*user->grid->dz*param->cb;  /* PLATE_SLAB */
#if defined(PETSC_HAVE_ERF)
  return (erf(PetscRealPart(z*param->L/2.0/param->skt)));
#else
  SETERRQ(PETSC_COMM_SELF,1,"erf() not available on this machine");
#endif
}


/* ------------------------------------------------------------------- */
#undef __FUNCT__
#define __FUNCT__ "OptionsHasName"
/*  utility function */
PetscBool  OptionsHasName(const char pre[],const char name[])
/* ------------------------------------------------------------------- */
{
  PetscBool      retval;
  PetscErrorCode ierr;
  ierr = PetscOptionsHasName(NULL,pre,name,&retval);CHKERRABORT(PETSC_COMM_WORLD,ierr);
  return retval;
}

/*=====================================================================
  INTERACTIVE SIGNAL HANDLING
  =====================================================================*/

/* ------------------------------------------------------------------- */
#undef __FUNCT__
#define __FUNCT__ "SNESConverged_Interactive"
PetscErrorCode SNESConverged_Interactive(SNES snes, PetscInt it,PetscReal xnorm, PetscReal snorm, PetscReal fnorm, SNESConvergedReason *reason, void *ctx)
/* ------------------------------------------------------------------- */
{
  AppCtx         *user  = (AppCtx*) ctx;
  Parameter      *param = user->param;
  KSP            ksp;
  PetscErrorCode ierr;

  PetscFunctionBeginUser;
  if (param->interrupted) {
    param->interrupted = PETSC_FALSE;
    PetscPrintf(PETSC_COMM_WORLD,"USER SIGNAL: exiting SNES solve. \n");
    *reason = SNES_CONVERGED_FNORM_ABS;
    PetscFunctionReturn(0);
  } else if (param->toggle_kspmon) {
    param->toggle_kspmon = PETSC_FALSE;

    ierr = SNESGetKSP(snes, &ksp);CHKERRQ(ierr);

    if (param->kspmon) {
      ierr = KSPMonitorCancel(ksp);CHKERRQ(ierr);

      param->kspmon = PETSC_FALSE;
      PetscPrintf(PETSC_COMM_WORLD,"USER SIGNAL: deactivating ksp singular value monitor. \n");
    } else {
      ierr = KSPMonitorSet(ksp,KSPMonitorSingularValue,NULL,NULL);CHKERRQ(ierr);

      param->kspmon = PETSC_TRUE;
      PetscPrintf(PETSC_COMM_WORLD,"USER SIGNAL: activating ksp singular value monitor. \n");
    }
  }
  PetscFunctionReturn(SNESConvergedDefault(snes,it,xnorm,snorm,fnorm,reason,ctx));
}

/* ------------------------------------------------------------------- */
#include <signal.h>
#undef __FUNCT__
#define __FUNCT__ "InteractiveHandler"
PetscErrorCode InteractiveHandler(int signum, void *ctx)
/* ------------------------------------------------------------------- */
{
  AppCtx    *user  = (AppCtx*) ctx;
  Parameter *param = user->param;

  if (signum == SIGILL) {
    param->toggle_kspmon = PETSC_TRUE;
#if !defined(PETSC_MISSING_SIGCONT)
  } else if (signum == SIGCONT) {
    param->interrupted = PETSC_TRUE;
#endif
#if !defined(PETSC_MISSING_SIGURG)
  } else if (signum == SIGURG) {
    param->stop_solve = PETSC_TRUE;
#endif
  }
  return 0;
}

/*---------------------------------------------------------------------*/
#undef __FUNCT__
#define __FUNCT__ "FormFunctionLocal"
/*  main call-back function that computes the processor-local piece
    of the residual */
PetscErrorCode FormFunctionLocal(DMDALocalInfo *info,Field **x,Field **f,void *ptr)
/*---------------------------------------------------------------------*/
{
  AppCtx      *user  = (AppCtx*)ptr;
  Parameter   *param = user->param;
  GridInfo    *grid  = user->grid;
  PetscScalar mag_w, mag_u;
  PetscInt    i,j,mx,mz,ilim,jlim;
  PetscInt    is,ie,js,je,ibound;    /* ,ivisc */

  PetscFunctionBeginUser;
  /* Define global and local grid parameters */
  mx   = info->mx;     mz   = info->my;
  ilim = mx-1;         jlim = mz-1;
  is   = info->xs;     ie   = info->xs+info->xm;
  js   = info->ys;     je   = info->ys+info->ym;

  /* Define geometric and numeric parameters */
  /* ivisc = param->ivisc; */ ibound = param->ibound;

  for (j=js; j<je; j++) {
    for (i=is; i<ie; i++) {

      /************* X-MOMENTUM/VELOCITY *************/
      if (i<j) f[j][i].u = x[j][i].u - SlabVel('U',i,j,user);
      else if (j<=grid->jlid || (j<grid->corner+grid->inose && i<grid->corner+grid->inose)) {
        /* in the lithospheric lid */
        f[j][i].u = x[j][i].u - 0.0;
      } else if (i==ilim) {
        /* on the right side boundary */
        if (ibound==BC_ANALYTIC) {
          f[j][i].u = x[j][i].u - HorizVelocity(i,j,user);
        } else {
          f[j][i].u = XNormalStress(x,i,j,CELL_CENTER,user) - EPS_ZERO;
        }

      } else if (j==jlim) {
        /* on the bottom boundary */
        if (ibound==BC_ANALYTIC) {
          f[j][i].u = x[j][i].u - HorizVelocity(i,j,user);
        } else if (ibound==BC_NOSTRESS) {
          f[j][i].u = XMomentumResidual(x,i,j,user);
        } else {
          /* experimental boundary condition */
        }

      } else {
        /* in the mantle wedge */
        f[j][i].u = XMomentumResidual(x,i,j,user);
      }

      /************* Z-MOMENTUM/VELOCITY *************/
      if (i<=j) {
        f[j][i].w = x[j][i].w - SlabVel('W',i,j,user);

      } else if (j<=grid->jlid || (j<grid->corner+grid->inose && i<grid->corner+grid->inose)) {
        /* in the lithospheric lid */
        f[j][i].w = x[j][i].w - 0.0;

      } else if (j==jlim) {
        /* on the bottom boundary */
        if (ibound==BC_ANALYTIC) {
          f[j][i].w = x[j][i].w - VertVelocity(i,j,user);
        } else {
          f[j][i].w = ZNormalStress(x,i,j,CELL_CENTER,user) - EPS_ZERO;
        }

      } else if (i==ilim) {
        /* on the right side boundary */
        if (ibound==BC_ANALYTIC) {
          f[j][i].w = x[j][i].w - VertVelocity(i,j,user);
        } else if (ibound==BC_NOSTRESS) {
          f[j][i].w = ZMomentumResidual(x,i,j,user);
        } else {
          /* experimental boundary condition */
        }

      } else {
        /* in the mantle wedge */
        f[j][i].w =  ZMomentumResidual(x,i,j,user);
      }

      /************* CONTINUITY/PRESSURE *************/
      if (i<j || j<=grid->jlid || (j<grid->corner+grid->inose && i<grid->corner+grid->inose)) {
        /* in the lid or slab */
        f[j][i].p = x[j][i].p;

      } else if ((i==ilim || j==jlim) && ibound==BC_ANALYTIC) {
        /* on an analytic boundary */
        f[j][i].p = x[j][i].p - Pressure(i,j,user);

      } else {
        /* in the mantle wedge */
        f[j][i].p = ContinuityResidual(x,i,j,user);
      }

      /************* TEMPERATURE *************/
      if (j==0) {
        /* on the surface */
        f[j][i].T = x[j][i].T + x[j+1][i].T + PetscMax(PetscRealPart(x[j][i].T),0.0);

      } else if (i==0) {
        /* slab inflow boundary */
        f[j][i].T = x[j][i].T - PlateModel(j,PLATE_SLAB,user);

      } else if (i==ilim) {
        /* right side boundary */
        mag_u = 1.0 - PetscPowRealInt((1.0-PetscMax(PetscMin(PetscRealPart(x[j][i-1].u)/param->cb,1.0),0.0)), 5);
        f[j][i].T = x[j][i].T - mag_u*x[j-1][i-1].T - (1.0-mag_u)*PlateModel(j,PLATE_LID,user);

      } else if (j==jlim) {
        /* bottom boundary */
        mag_w = 1.0 - PetscPowRealInt((1.0-PetscMax(PetscMin(PetscRealPart(x[j-1][i].w)/param->sb,1.0),0.0)), 5);
        f[j][i].T = x[j][i].T - mag_w*x[j-1][i-1].T - (1.0-mag_w);

      } else {
        /* in the mantle wedge */
        f[j][i].T = EnergyResidual(x,i,j,user);
      }
    }
  }
  PetscFunctionReturn(0);
}
