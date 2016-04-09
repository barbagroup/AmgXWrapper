#include <petsctao.h>

static char  help[] =
"This example demonstrates use of the TAO package to\n\
solve an unconstrained system of equations.  This example is based on a\n\
problem from the MINPACK-2 test suite.  Given a rectangular 2-D domain and\n\
boundary values along the edges of the domain, the objective is to find the\n\
surface with the minimal area that satisfies the boundary conditions.\n\
This application solves this problem using complimentarity -- We are actually\n\
solving the system  (grad f)_i >= 0, if x_i == l_i \n\
                    (grad f)_i = 0, if l_i < x_i < u_i \n\
                    (grad f)_i <= 0, if x_i == u_i  \n\
where f is the function to be minimized. \n\
\n\
The command line options are:\n\
  -mx <xg>, where <xg> = number of grid points in the 1st coordinate direction\n\
  -my <yg>, where <yg> = number of grid points in the 2nd coordinate direction\n\
  -start <st>, where <st> =0 for zero vector, and an average of the boundary conditions otherwise \n\n";

/*T
   Concepts: TAO^Solving a complementarity problem
   Routines: TaoCreate(); TaoDestroy();

   Processors: 1
T*/


/*
   User-defined application context - contains data needed by the
   application-provided call-back routines, FormFunctionGradient(),
   FormHessian().
*/
typedef struct {
  PetscInt  mx, my;
  PetscReal *bottom, *top, *left, *right;
} AppCtx;


/* -------- User-defined Routines --------- */

static PetscErrorCode MSA_BoundaryConditions(AppCtx *);
static PetscErrorCode MSA_InitialPoint(AppCtx *, Vec);
PetscErrorCode FormConstraints(Tao, Vec, Vec, void *);
PetscErrorCode FormJacobian(Tao, Vec, Mat, Mat, void *);

#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc, char **argv)
{
  PetscErrorCode ierr;              /* used to check for functions returning nonzeros */
  Vec            x;                 /* solution vector */
  Vec            c;                 /* Constraints function vector */
  Vec            xl,xu;             /* Bounds on the variables */
  PetscBool      flg;               /* A return variable when checking for user options */
  Tao            tao;               /* TAO solver context */
  Mat            J;                 /* Jacobian matrix */
  PetscInt       N;                 /* Number of elements in vector */
  PetscScalar    lb =  PETSC_NINFINITY;      /* lower bound constant */
  PetscScalar    ub =  PETSC_INFINITY;      /* upper bound constant */
  AppCtx         user;                    /* user-defined work context */

  /* Initialize PETSc, TAO */
  PetscInitialize(&argc, &argv, (char *)0, help );

  /* Specify default dimension of the problem */
  user.mx = 4; user.my = 4;

  /* Check for any command line arguments that override defaults */
  ierr = PetscOptionsGetInt(NULL,NULL, "-mx", &user.mx, &flg);CHKERRQ(ierr);
  ierr = PetscOptionsGetInt(NULL,NULL, "-my", &user.my, &flg);CHKERRQ(ierr);

  /* Calculate any derived values from parameters */
  N = user.mx*user.my;

  ierr = PetscPrintf(PETSC_COMM_SELF,"\n---- Minimum Surface Area Problem -----\n");CHKERRQ(ierr);
  ierr = PetscPrintf(PETSC_COMM_SELF,"mx:%D, my:%D\n", user.mx,user.my);CHKERRQ(ierr);

  /* Create appropriate vectors and matrices */
  ierr = VecCreateSeq(MPI_COMM_SELF, N, &x);
  ierr = VecDuplicate(x, &c);CHKERRQ(ierr);
  ierr = MatCreateSeqAIJ(MPI_COMM_SELF, N, N, 7, NULL, &J);CHKERRQ(ierr);

  /* The TAO code begins here */

  /* Create TAO solver and set desired solution method */
  ierr = TaoCreate(PETSC_COMM_SELF,&tao);CHKERRQ(ierr);
  ierr = TaoSetType(tao,TAOSSILS);CHKERRQ(ierr);

  /* Set data structure */
  ierr = TaoSetInitialVector(tao, x);CHKERRQ(ierr);

  /*  Set routines for constraints function and Jacobian evaluation */
  ierr = TaoSetConstraintsRoutine(tao, c, FormConstraints, (void *)&user);CHKERRQ(ierr);
  ierr = TaoSetJacobianRoutine(tao, J, J, FormJacobian, (void *)&user);CHKERRQ(ierr);

  /* Set the variable bounds */
  ierr = MSA_BoundaryConditions(&user);CHKERRQ(ierr);

  /* Set initial solution guess */
  ierr = MSA_InitialPoint(&user, x);CHKERRQ(ierr);

  /* Set Bounds on variables */
  ierr = VecDuplicate(x, &xl);CHKERRQ(ierr);
  ierr = VecDuplicate(x, &xu);CHKERRQ(ierr);
  ierr = VecSet(xl, lb);CHKERRQ(ierr);
  ierr = VecSet(xu, ub);CHKERRQ(ierr);
  ierr = TaoSetVariableBounds(tao,xl,xu);CHKERRQ(ierr);

  /* Check for any tao command line options */
  ierr = TaoSetFromOptions(tao);CHKERRQ(ierr);

  /* Solve the application */
  ierr = TaoSolve(tao); CHKERRQ(ierr);

  /* Free Tao data structures */
  ierr = TaoDestroy(&tao);CHKERRQ(ierr);

  /* Free PETSc data structures */
  ierr = VecDestroy(&x);CHKERRQ(ierr);
  ierr = VecDestroy(&xl);CHKERRQ(ierr);
  ierr = VecDestroy(&xu);CHKERRQ(ierr);
  ierr = VecDestroy(&c);CHKERRQ(ierr);
  ierr = MatDestroy(&J);CHKERRQ(ierr);

  /* Free user-created data structures */
  ierr = PetscFree(user.bottom);CHKERRQ(ierr);
  ierr = PetscFree(user.top);CHKERRQ(ierr);
  ierr = PetscFree(user.left);CHKERRQ(ierr);
  ierr = PetscFree(user.right);CHKERRQ(ierr);

  PetscFinalize();
  return 0;
}

/* -------------------------------------------------------------------- */
#undef __FUNCT__
#define __FUNCT__ "FormConstraints"

/*  FormConstraints - Evaluates gradient of f.

    Input Parameters:
.   tao  - the TAO_APPLICATION context
.   X    - input vector
.   ptr  - optional user-defined context, as set by TaoSetConstraintsRoutine()

    Output Parameters:
.   G - vector containing the newly evaluated gradient
*/
PetscErrorCode FormConstraints(Tao tao, Vec X, Vec G, void *ptr)
{
  AppCtx         *user = (AppCtx *) ptr;
  PetscErrorCode ierr;
  PetscInt       i,j,row;
  PetscInt       mx=user->mx, my=user->my;
  PetscReal      hx=1.0/(mx+1),hy=1.0/(my+1), hydhx=hy/hx, hxdhy=hx/hy;
  PetscReal      f1,f2,f3,f4,f5,f6,d1,d2,d3,d4,d5,d6,d7,d8,xc,xl,xr,xt,xb,xlt,xrb;
  PetscReal      df1dxc,df2dxc,df3dxc,df4dxc,df5dxc,df6dxc;
  PetscScalar    zero=0.0;
  PetscScalar    *g, *x;

  PetscFunctionBegin;
  /* Initialize vector to zero */
  ierr = VecSet(G, zero);CHKERRQ(ierr);

  /* Get pointers to vector data */
  ierr = VecGetArray(X, &x);CHKERRQ(ierr);
  ierr = VecGetArray(G, &g);CHKERRQ(ierr);

  /* Compute function over the locally owned part of the mesh */
  for (j=0; j<my; j++){
    for (i=0; i< mx; i++){
      row= j*mx + i;

      xc = x[row];
      xlt=xrb=xl=xr=xb=xt=xc;

      if (i==0){ /* left side */
        xl= user->left[j+1];
        xlt = user->left[j+2];
      } else {
        xl = x[row-1];
      }

      if (j==0){ /* bottom side */
        xb=user->bottom[i+1];
        xrb = user->bottom[i+2];
      } else {
        xb = x[row-mx];
      }

      if (i+1 == mx){ /* right side */
        xr=user->right[j+1];
        xrb = user->right[j];
      } else {
        xr = x[row+1];
      }

      if (j+1==0+my){ /* top side */
        xt=user->top[i+1];
        xlt = user->top[i];
      }else {
        xt = x[row+mx];
      }

      if (i>0 && j+1<my){
        xlt = x[row-1+mx];
      }
      if (j>0 && i+1<mx){
        xrb = x[row+1-mx];
      }

      d1 = (xc-xl);
      d2 = (xc-xr);
      d3 = (xc-xt);
      d4 = (xc-xb);
      d5 = (xr-xrb);
      d6 = (xrb-xb);
      d7 = (xlt-xl);
      d8 = (xt-xlt);

      df1dxc = d1*hydhx;
      df2dxc = ( d1*hydhx + d4*hxdhy );
      df3dxc = d3*hxdhy;
      df4dxc = ( d2*hydhx + d3*hxdhy );
      df5dxc = d2*hydhx;
      df6dxc = d4*hxdhy;

      d1 /= hx;
      d2 /= hx;
      d3 /= hy;
      d4 /= hy;
      d5 /= hy;
      d6 /= hx;
      d7 /= hy;
      d8 /= hx;

      f1 = PetscSqrtScalar( 1.0 + d1*d1 + d7*d7);
      f2 = PetscSqrtScalar( 1.0 + d1*d1 + d4*d4);
      f3 = PetscSqrtScalar( 1.0 + d3*d3 + d8*d8);
      f4 = PetscSqrtScalar( 1.0 + d3*d3 + d2*d2);
      f5 = PetscSqrtScalar( 1.0 + d2*d2 + d5*d5);
      f6 = PetscSqrtScalar( 1.0 + d4*d4 + d6*d6);

      df1dxc /= f1;
      df2dxc /= f2;
      df3dxc /= f3;
      df4dxc /= f4;
      df5dxc /= f5;
      df6dxc /= f6;

      g[row] = (df1dxc+df2dxc+df3dxc+df4dxc+df5dxc+df6dxc )/2.0;
    }
  }

  /* Restore vectors */
  ierr = VecRestoreArray(X, &x);CHKERRQ(ierr);
  ierr = VecRestoreArray(G, &g);CHKERRQ(ierr);
  ierr = PetscLogFlops(67*mx*my);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/* ------------------------------------------------------------------- */
#undef __FUNCT__
#define __FUNCT__ "FormJacobian"
/*
   FormJacobian - Evaluates Jacobian matrix.

   Input Parameters:
.  tao  - the TAO_APPLICATION context
.  X    - input vector
.  ptr  - optional user-defined context, as set by TaoSetJacobian()

   Output Parameters:
.  tH    - Jacobian matrix

*/
PetscErrorCode FormJacobian(Tao tao, Vec X, Mat H, Mat tHPre, void *ptr)
{
  AppCtx         *user = (AppCtx *) ptr;
  PetscErrorCode ierr;
  PetscInt       i,j,k,row;
  PetscInt       mx=user->mx, my=user->my;
  PetscInt       col[7];
  PetscReal      hx=1.0/(mx+1), hy=1.0/(my+1), hydhx=hy/hx, hxdhy=hx/hy;
  PetscReal      f1,f2,f3,f4,f5,f6,d1,d2,d3,d4,d5,d6,d7,d8,xc,xl,xr,xt,xb,xlt,xrb;
  PetscReal      hl,hr,ht,hb,hc,htl,hbr;
  PetscScalar    *x, v[7];
  PetscBool      assembled;

  /* Set various matrix options */
  ierr = MatSetOption(H,MAT_IGNORE_OFF_PROC_ENTRIES,PETSC_TRUE);CHKERRQ(ierr);
  ierr = MatAssembled(H,&assembled);CHKERRQ(ierr);
  if (assembled){ierr = MatZeroEntries(H); CHKERRQ(ierr);}

  /* Get pointers to vector data */
  ierr = VecGetArray(X, &x);CHKERRQ(ierr);

  /* Compute Jacobian over the locally owned part of the mesh */
  for (i=0; i< mx; i++){
    for (j=0; j<my; j++){
      row= j*mx + i;

      xc = x[row];
      xlt=xrb=xl=xr=xb=xt=xc;

      /* Left side */
      if (i==0){
        xl= user->left[j+1];
        xlt = user->left[j+2];
      } else {
        xl = x[row-1];
      }

      if (j==0){
        xb=user->bottom[i+1];
        xrb = user->bottom[i+2];
      } else {
        xb = x[row-mx];
      }

      if (i+1 == mx){
        xr=user->right[j+1];
        xrb = user->right[j];
      } else {
        xr = x[row+1];
      }

      if (j+1==my){
        xt=user->top[i+1];
        xlt = user->top[i];
      }else {
        xt = x[row+mx];
      }

      if (i>0 && j+1<my){
        xlt = x[row-1+mx];
      }
      if (j>0 && i+1<mx){
        xrb = x[row+1-mx];
      }


      d1 = (xc-xl)/hx;
      d2 = (xc-xr)/hx;
      d3 = (xc-xt)/hy;
      d4 = (xc-xb)/hy;
      d5 = (xrb-xr)/hy;
      d6 = (xrb-xb)/hx;
      d7 = (xlt-xl)/hy;
      d8 = (xlt-xt)/hx;

      f1 = PetscSqrtScalar( 1.0 + d1*d1 + d7*d7);
      f2 = PetscSqrtScalar( 1.0 + d1*d1 + d4*d4);
      f3 = PetscSqrtScalar( 1.0 + d3*d3 + d8*d8);
      f4 = PetscSqrtScalar( 1.0 + d3*d3 + d2*d2);
      f5 = PetscSqrtScalar( 1.0 + d2*d2 + d5*d5);
      f6 = PetscSqrtScalar( 1.0 + d4*d4 + d6*d6);


      hl = (-hydhx*(1.0+d7*d7)+d1*d7)/(f1*f1*f1)+(-hydhx*(1.0+d4*d4)+d1*d4)/(f2*f2*f2);
      hr = (-hydhx*(1.0+d5*d5)+d2*d5)/(f5*f5*f5)+(-hydhx*(1.0+d3*d3)+d2*d3)/(f4*f4*f4);
      ht = (-hxdhy*(1.0+d8*d8)+d3*d8)/(f3*f3*f3)+(-hxdhy*(1.0+d2*d2)+d2*d3)/(f4*f4*f4);
      hb = (-hxdhy*(1.0+d6*d6)+d4*d6)/(f6*f6*f6)+(-hxdhy*(1.0+d1*d1)+d1*d4)/(f2*f2*f2);

      hbr = -d2*d5/(f5*f5*f5) - d4*d6/(f6*f6*f6);
      htl = -d1*d7/(f1*f1*f1) - d3*d8/(f3*f3*f3);

      hc = hydhx*(1.0+d7*d7)/(f1*f1*f1) + hxdhy*(1.0+d8*d8)/(f3*f3*f3) + hydhx*(1.0+d5*d5)/(f5*f5*f5) + hxdhy*(1.0+d6*d6)/(f6*f6*f6) +
           (hxdhy*(1.0+d1*d1)+hydhx*(1.0+d4*d4)-2*d1*d4)/(f2*f2*f2) + (hxdhy*(1.0+d2*d2)+hydhx*(1.0+d3*d3)-2*d2*d3)/(f4*f4*f4);

      hl/=2.0; hr/=2.0; ht/=2.0; hb/=2.0; hbr/=2.0; htl/=2.0;  hc/=2.0;

      k=0;
      if (j>0){
        v[k]=hb; col[k]=row - mx; k++;
      }

      if (j>0 && i < mx -1){
        v[k]=hbr; col[k]=row - mx+1; k++;
      }

      if (i>0){
        v[k]= hl; col[k]=row - 1; k++;
      }

      v[k]= hc; col[k]=row; k++;

      if (i < mx-1 ){
        v[k]= hr; col[k]=row+1; k++;
      }

      if (i>0 && j < my-1 ){
        v[k]= htl; col[k] = row+mx-1; k++;
      }

      if (j < my-1 ){
        v[k]= ht; col[k] = row+mx; k++;
      }

      /*
         Set matrix values using local numbering, which was defined
         earlier, in the main routine.
      */
      ierr = MatSetValues(H,1,&row,k,col,v,INSERT_VALUES);CHKERRQ(ierr);
    }
  }

  /* Restore vectors */
  ierr = VecRestoreArray(X,&x);CHKERRQ(ierr);

  /* Assemble the matrix */
  ierr = MatAssemblyBegin(H,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(H,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = PetscLogFlops(199*mx*my);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/* ------------------------------------------------------------------- */
#undef __FUNCT__
#define __FUNCT__ "MSA_BoundaryConditions"
/*
   MSA_BoundaryConditions -  Calculates the boundary conditions for
   the region.

   Input Parameter:
.  user - user-defined application context

   Output Parameter:
.  user - user-defined application context
*/
static PetscErrorCode MSA_BoundaryConditions(AppCtx * user)
{
  PetscErrorCode  ierr;
  PetscInt        i,j,k,limit=0,maxits=5;
  PetscInt        mx=user->mx,my=user->my;
  PetscInt        bsize=0, lsize=0, tsize=0, rsize=0;
  PetscReal       one=1.0, two=2.0, three=3.0, tol=1e-10;
  PetscReal       fnorm,det,hx,hy,xt=0,yt=0;
  PetscReal       u1,u2,nf1,nf2,njac11,njac12,njac21,njac22;
  PetscReal       b=-0.5, t=0.5, l=-0.5, r=0.5;
  PetscReal       *boundary;

  PetscFunctionBegin;
  bsize=mx+2; lsize=my+2; rsize=my+2; tsize=mx+2;

  ierr = PetscMalloc1(bsize, &user->bottom);CHKERRQ(ierr);
  ierr = PetscMalloc1(tsize, &user->top);CHKERRQ(ierr);
  ierr = PetscMalloc1(lsize, &user->left);CHKERRQ(ierr);
  ierr = PetscMalloc1(rsize, &user->right);CHKERRQ(ierr);

  hx= (r-l)/(mx+1); hy=(t-b)/(my+1);

  for (j=0; j<4; j++){
    if (j==0){
      yt=b;
      xt=l;
      limit=bsize;
      boundary=user->bottom;
    } else if (j==1){
      yt=t;
      xt=l;
      limit=tsize;
      boundary=user->top;
    } else if (j==2){
      yt=b;
      xt=l;
      limit=lsize;
      boundary=user->left;
    } else { /* if  (j==3) */
      yt=b;
      xt=r;
      limit=rsize;
      boundary=user->right;
    }

    for (i=0; i<limit; i++){
      u1=xt;
      u2=-yt;
      for (k=0; k<maxits; k++){
        nf1=u1 + u1*u2*u2 - u1*u1*u1/three-xt;
        nf2=-u2 - u1*u1*u2 + u2*u2*u2/three-yt;
        fnorm=PetscSqrtScalar(nf1*nf1+nf2*nf2);
        if (fnorm <= tol) break;
        njac11=one+u2*u2-u1*u1;
        njac12=two*u1*u2;
        njac21=-two*u1*u2;
        njac22=-one - u1*u1 + u2*u2;
        det = njac11*njac22-njac21*njac12;
        u1 = u1-(njac22*nf1-njac12*nf2)/det;
        u2 = u2-(njac11*nf2-njac21*nf1)/det;
      }

      boundary[i]=u1*u1-u2*u2;
      if (j==0 || j==1) {
        xt=xt+hx;
      } else { /* if (j==2 || j==3) */
        yt=yt+hy;
      }
    }
  }
  PetscFunctionReturn(0);
}

/* ------------------------------------------------------------------- */
#undef __FUNCT__
#define __FUNCT__ "MSA_InitialPoint"
/*
   MSA_InitialPoint - Calculates the initial guess in one of three ways.

   Input Parameters:
.  user - user-defined application context
.  X - vector for initial guess

   Output Parameters:
.  X - newly computed initial guess
*/
static PetscErrorCode MSA_InitialPoint(AppCtx * user, Vec X)
{
  PetscErrorCode ierr;
  PetscInt       start=-1,i,j;
  PetscScalar    zero=0.0;
  PetscBool      flg;

  PetscFunctionBegin;
  ierr = PetscOptionsGetInt(NULL,NULL,"-start",&start,&flg);CHKERRQ(ierr);

  if (flg && start==0){ /* The zero vector is reasonable */
    ierr = VecSet(X, zero);CHKERRQ(ierr);
  } else { /* Take an average of the boundary conditions */
    PetscInt    row;
    PetscInt    mx=user->mx,my=user->my;
    PetscScalar *x;

    /* Get pointers to vector data */
    ierr = VecGetArray(X,&x);CHKERRQ(ierr);

    /* Perform local computations */
    for (j=0; j<my; j++){
      for (i=0; i< mx; i++){
        row=(j)*mx + (i);
        x[row] = ( ((j+1)*user->bottom[i+1]+(my-j+1)*user->top[i+1])/(my+2)+ ((i+1)*user->left[j+1]+(mx-i+1)*user->right[j+1])/(mx+2))/2.0;
      }
    }

    /* Restore vectors */
    ierr = VecRestoreArray(X,&x);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}
