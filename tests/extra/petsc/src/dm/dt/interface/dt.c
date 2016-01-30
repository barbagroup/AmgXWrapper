/* Discretization tools */

#include <petscconf.h>
#if defined(PETSC_HAVE_MATHIMF_H)
#include <mathimf.h>           /* this needs to be included before math.h */
#endif
#ifdef PETSC_HAVE_MPFR
#include <mpfr.h>
#endif

#include <petscdt.h>            /*I "petscdt.h" I*/
#include <petscblaslapack.h>
#include <petsc/private/petscimpl.h>
#include <petsc/private/dtimpl.h>
#include <petscviewer.h>
#include <petscdmplex.h>
#include <petscdmshell.h>

static PetscBool GaussCite       = PETSC_FALSE;
const char       GaussCitation[] = "@article{GolubWelsch1969,\n"
                                   "  author  = {Golub and Welsch},\n"
                                   "  title   = {Calculation of Quadrature Rules},\n"
                                   "  journal = {Math. Comp.},\n"
                                   "  volume  = {23},\n"
                                   "  number  = {106},\n"
                                   "  pages   = {221--230},\n"
                                   "  year    = {1969}\n}\n";

#undef __FUNCT__
#define __FUNCT__ "PetscQuadratureCreate"
/*@
  PetscQuadratureCreate - Create a PetscQuadrature object

  Collective on MPI_Comm

  Input Parameter:
. comm - The communicator for the PetscQuadrature object

  Output Parameter:
. q  - The PetscQuadrature object

  Level: beginner

.keywords: PetscQuadrature, quadrature, create
.seealso: PetscQuadratureDestroy(), PetscQuadratureGetData()
@*/
PetscErrorCode PetscQuadratureCreate(MPI_Comm comm, PetscQuadrature *q)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidPointer(q, 2);
  ierr = DMInitializePackage();CHKERRQ(ierr);
  ierr = PetscHeaderCreate(*q,PETSC_OBJECT_CLASSID,"PetscQuadrature","Quadrature","DT",comm,PetscQuadratureDestroy,PetscQuadratureView);CHKERRQ(ierr);
  (*q)->dim       = -1;
  (*q)->order     = -1;
  (*q)->numPoints = 0;
  (*q)->points    = NULL;
  (*q)->weights   = NULL;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscQuadratureDuplicate"
/*@
  PetscQuadratureDuplicate - Create a deep copy of the PetscQuadrature object

  Collective on PetscQuadrature

  Input Parameter:
. q  - The PetscQuadrature object

  Output Parameter:
. r  - The new PetscQuadrature object

  Level: beginner

.keywords: PetscQuadrature, quadrature, clone
.seealso: PetscQuadratureCreate(), PetscQuadratureDestroy(), PetscQuadratureGetData()
@*/
PetscErrorCode PetscQuadratureDuplicate(PetscQuadrature q, PetscQuadrature *r)
{
  PetscInt         order, dim, Nq;
  const PetscReal *points, *weights;
  PetscReal       *p, *w;
  PetscErrorCode   ierr;

  PetscFunctionBegin;
  PetscValidPointer(q, 2);
  ierr = PetscQuadratureCreate(PetscObjectComm((PetscObject) q), r);CHKERRQ(ierr);
  ierr = PetscQuadratureGetOrder(q, &order);CHKERRQ(ierr);
  ierr = PetscQuadratureSetOrder(*r, order);CHKERRQ(ierr);
  ierr = PetscQuadratureGetData(q, &dim, &Nq, &points, &weights);CHKERRQ(ierr);
  ierr = PetscMalloc1(Nq*dim, &p);CHKERRQ(ierr);
  ierr = PetscMalloc1(Nq, &w);CHKERRQ(ierr);
  ierr = PetscMemcpy(p, points, Nq*dim * sizeof(PetscReal));CHKERRQ(ierr);
  ierr = PetscMemcpy(w, weights, Nq * sizeof(PetscReal));CHKERRQ(ierr);
  ierr = PetscQuadratureSetData(*r, dim, Nq, p, w);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscQuadratureDestroy"
/*@
  PetscQuadratureDestroy - Destroys a PetscQuadrature object

  Collective on PetscQuadrature

  Input Parameter:
. q  - The PetscQuadrature object

  Level: beginner

.keywords: PetscQuadrature, quadrature, destroy
.seealso: PetscQuadratureCreate(), PetscQuadratureGetData()
@*/
PetscErrorCode PetscQuadratureDestroy(PetscQuadrature *q)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (!*q) PetscFunctionReturn(0);
  PetscValidHeaderSpecific((*q),PETSC_OBJECT_CLASSID,1);
  if (--((PetscObject)(*q))->refct > 0) {
    *q = NULL;
    PetscFunctionReturn(0);
  }
  ierr = PetscFree((*q)->points);CHKERRQ(ierr);
  ierr = PetscFree((*q)->weights);CHKERRQ(ierr);
  ierr = PetscHeaderDestroy(q);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscQuadratureGetOrder"
/*@
  PetscQuadratureGetOrder - Return the quadrature information

  Not collective

  Input Parameter:
. q - The PetscQuadrature object

  Output Parameter:
. order - The order of the quadrature, i.e. the highest degree polynomial that is exactly integrated

  Output Parameter:

  Level: intermediate

.seealso: PetscQuadratureSetOrder(), PetscQuadratureGetData(), PetscQuadratureSetData()
@*/
PetscErrorCode PetscQuadratureGetOrder(PetscQuadrature q, PetscInt *order)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(q, PETSC_OBJECT_CLASSID, 1);
  PetscValidPointer(order, 2);
  *order = q->order;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscQuadratureSetOrder"
/*@
  PetscQuadratureSetOrder - Return the quadrature information

  Not collective

  Input Parameters:
+ q - The PetscQuadrature object
- order - The order of the quadrature, i.e. the highest degree polynomial that is exactly integrated

  Level: intermediate

.seealso: PetscQuadratureGetOrder(), PetscQuadratureGetData(), PetscQuadratureSetData()
@*/
PetscErrorCode PetscQuadratureSetOrder(PetscQuadrature q, PetscInt order)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(q, PETSC_OBJECT_CLASSID, 1);
  q->order = order;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscQuadratureGetData"
/*@C
  PetscQuadratureGetData - Returns the data defining the quadrature

  Not collective

  Input Parameter:
. q  - The PetscQuadrature object

  Output Parameters:
+ dim - The spatial dimension
. npoints - The number of quadrature points
. points - The coordinates of each quadrature point
- weights - The weight of each quadrature point

  Level: intermediate

.keywords: PetscQuadrature, quadrature
.seealso: PetscQuadratureCreate(), PetscQuadratureSetData()
@*/
PetscErrorCode PetscQuadratureGetData(PetscQuadrature q, PetscInt *dim, PetscInt *npoints, const PetscReal *points[], const PetscReal *weights[])
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(q, PETSC_OBJECT_CLASSID, 1);
  if (dim) {
    PetscValidPointer(dim, 2);
    *dim = q->dim;
  }
  if (npoints) {
    PetscValidPointer(npoints, 3);
    *npoints = q->numPoints;
  }
  if (points) {
    PetscValidPointer(points, 4);
    *points = q->points;
  }
  if (weights) {
    PetscValidPointer(weights, 5);
    *weights = q->weights;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscQuadratureSetData"
/*@C
  PetscQuadratureSetData - Sets the data defining the quadrature

  Not collective

  Input Parameters:
+ q  - The PetscQuadrature object
. dim - The spatial dimension
. npoints - The number of quadrature points
. points - The coordinates of each quadrature point
- weights - The weight of each quadrature point

  Level: intermediate

.keywords: PetscQuadrature, quadrature
.seealso: PetscQuadratureCreate(), PetscQuadratureGetData()
@*/
PetscErrorCode PetscQuadratureSetData(PetscQuadrature q, PetscInt dim, PetscInt npoints, const PetscReal points[], const PetscReal weights[])
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(q, PETSC_OBJECT_CLASSID, 1);
  if (dim >= 0)     q->dim       = dim;
  if (npoints >= 0) q->numPoints = npoints;
  if (points) {
    PetscValidPointer(points, 4);
    q->points = points;
  }
  if (weights) {
    PetscValidPointer(weights, 5);
    q->weights = weights;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscQuadratureView"
/*@C
  PetscQuadratureView - Views a PetscQuadrature object

  Collective on PetscQuadrature

  Input Parameters:
+ q  - The PetscQuadrature object
- viewer - The PetscViewer object

  Level: beginner

.keywords: PetscQuadrature, quadrature, view
.seealso: PetscQuadratureCreate(), PetscQuadratureGetData()
@*/
PetscErrorCode PetscQuadratureView(PetscQuadrature quad, PetscViewer viewer)
{
  PetscInt       q, d;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscObjectPrintClassNamePrefixType((PetscObject)quad,viewer);CHKERRQ(ierr);
  ierr = PetscViewerASCIIPrintf(viewer, "Quadrature on %d points\n  (", quad->numPoints);CHKERRQ(ierr);
  for (q = 0; q < quad->numPoints; ++q) {
    for (d = 0; d < quad->dim; ++d) {
      if (d) ierr = PetscViewerASCIIPrintf(viewer, ", ");CHKERRQ(ierr);
      ierr = PetscViewerASCIIPrintf(viewer, "%g\n", (double)quad->points[q*quad->dim+d]);CHKERRQ(ierr);
    }
    ierr = PetscViewerASCIIPrintf(viewer, ") %g\n", (double)quad->weights[q]);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscQuadratureExpandComposite"
/*@C
  PetscQuadratureExpandComposite - Return a quadrature over the composite element, which has the original quadrature in each subelement

  Not collective

  Input Parameter:
+ q - The original PetscQuadrature
. numSubelements - The number of subelements the original element is divided into
. v0 - An array of the initial points for each subelement
- jac - An array of the Jacobian mappings from the reference to each subelement

  Output Parameters:
. dim - The dimension

  Note: Together v0 and jac define an affine mapping from the original reference element to each subelement

  Level: intermediate

.seealso: PetscFECreate(), PetscSpaceGetDimension(), PetscDualSpaceGetDimension()
@*/
PetscErrorCode PetscQuadratureExpandComposite(PetscQuadrature q, PetscInt numSubelements, const PetscReal v0[], const PetscReal jac[], PetscQuadrature *qref)
{
  const PetscReal *points,    *weights;
  PetscReal       *pointsRef, *weightsRef;
  PetscInt         dim, order, npoints, npointsRef, c, p, d, e;
  PetscErrorCode   ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(q, PETSC_OBJECT_CLASSID, 1);
  PetscValidPointer(v0, 3);
  PetscValidPointer(jac, 4);
  PetscValidPointer(qref, 5);
  ierr = PetscQuadratureCreate(PETSC_COMM_SELF, qref);CHKERRQ(ierr);
  ierr = PetscQuadratureGetOrder(q, &order);CHKERRQ(ierr);
  ierr = PetscQuadratureGetData(q, &dim, &npoints, &points, &weights);CHKERRQ(ierr);
  npointsRef = npoints*numSubelements;
  ierr = PetscMalloc1(npointsRef*dim,&pointsRef);CHKERRQ(ierr);
  ierr = PetscMalloc1(npointsRef,&weightsRef);CHKERRQ(ierr);
  for (c = 0; c < numSubelements; ++c) {
    for (p = 0; p < npoints; ++p) {
      for (d = 0; d < dim; ++d) {
        pointsRef[(c*npoints + p)*dim+d] = v0[c*dim+d];
        for (e = 0; e < dim; ++e) {
          pointsRef[(c*npoints + p)*dim+d] += jac[(c*dim + d)*dim+e]*(points[p*dim+e] + 1.0);
        }
      }
      /* Could also use detJ here */
      weightsRef[c*npoints+p] = weights[p]/numSubelements;
    }
  }
  ierr = PetscQuadratureSetOrder(*qref, order);CHKERRQ(ierr);
  ierr = PetscQuadratureSetData(*qref, dim, npointsRef, pointsRef, weightsRef);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDTLegendreEval"
/*@
   PetscDTLegendreEval - evaluate Legendre polynomial at points

   Not Collective

   Input Arguments:
+  npoints - number of spatial points to evaluate at
.  points - array of locations to evaluate at
.  ndegree - number of basis degrees to evaluate
-  degrees - sorted array of degrees to evaluate

   Output Arguments:
+  B - row-oriented basis evaluation matrix B[point*ndegree + degree] (dimension npoints*ndegrees, allocated by caller) (or NULL)
.  D - row-oriented derivative evaluation matrix (or NULL)
-  D2 - row-oriented second derivative evaluation matrix (or NULL)

   Level: intermediate

.seealso: PetscDTGaussQuadrature()
@*/
PetscErrorCode PetscDTLegendreEval(PetscInt npoints,const PetscReal *points,PetscInt ndegree,const PetscInt *degrees,PetscReal *B,PetscReal *D,PetscReal *D2)
{
  PetscInt i,maxdegree;

  PetscFunctionBegin;
  if (!npoints || !ndegree) PetscFunctionReturn(0);
  maxdegree = degrees[ndegree-1];
  for (i=0; i<npoints; i++) {
    PetscReal pm1,pm2,pd1,pd2,pdd1,pdd2,x;
    PetscInt  j,k;
    x    = points[i];
    pm2  = 0;
    pm1  = 1;
    pd2  = 0;
    pd1  = 0;
    pdd2 = 0;
    pdd1 = 0;
    k    = 0;
    if (degrees[k] == 0) {
      if (B) B[i*ndegree+k] = pm1;
      if (D) D[i*ndegree+k] = pd1;
      if (D2) D2[i*ndegree+k] = pdd1;
      k++;
    }
    for (j=1; j<=maxdegree; j++,k++) {
      PetscReal p,d,dd;
      p    = ((2*j-1)*x*pm1 - (j-1)*pm2)/j;
      d    = pd2 + (2*j-1)*pm1;
      dd   = pdd2 + (2*j-1)*pd1;
      pm2  = pm1;
      pm1  = p;
      pd2  = pd1;
      pd1  = d;
      pdd2 = pdd1;
      pdd1 = dd;
      if (degrees[k] == j) {
        if (B) B[i*ndegree+k] = p;
        if (D) D[i*ndegree+k] = d;
        if (D2) D2[i*ndegree+k] = dd;
      }
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDTGaussQuadrature"
/*@
   PetscDTGaussQuadrature - create Gauss quadrature

   Not Collective

   Input Arguments:
+  npoints - number of points
.  a - left end of interval (often-1)
-  b - right end of interval (often +1)

   Output Arguments:
+  x - quadrature points
-  w - quadrature weights

   Level: intermediate

   References:
.   1. - Golub and Welsch, Calculation of Quadrature Rules, Math. Comp. 23(106), 1969.

.seealso: PetscDTLegendreEval()
@*/
PetscErrorCode PetscDTGaussQuadrature(PetscInt npoints,PetscReal a,PetscReal b,PetscReal *x,PetscReal *w)
{
  PetscErrorCode ierr;
  PetscInt       i;
  PetscReal      *work;
  PetscScalar    *Z;
  PetscBLASInt   N,LDZ,info;

  PetscFunctionBegin;
  ierr = PetscCitationsRegister(GaussCitation, &GaussCite);CHKERRQ(ierr);
  /* Set up the Golub-Welsch system */
  for (i=0; i<npoints; i++) {
    x[i] = 0;                   /* diagonal is 0 */
    if (i) w[i-1] = 0.5 / PetscSqrtReal(1 - 1./PetscSqr(2*i));
  }
  ierr = PetscMalloc2(npoints*npoints,&Z,PetscMax(1,2*npoints-2),&work);CHKERRQ(ierr);
  ierr = PetscBLASIntCast(npoints,&N);CHKERRQ(ierr);
  LDZ  = N;
  ierr = PetscFPTrapPush(PETSC_FP_TRAP_OFF);CHKERRQ(ierr);
  PetscStackCallBLAS("LAPACKsteqr",LAPACKsteqr_("I",&N,x,w,Z,&LDZ,work,&info));
  ierr = PetscFPTrapPop();CHKERRQ(ierr);
  if (info) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_PLIB,"xSTEQR error");

  for (i=0; i<(npoints+1)/2; i++) {
    PetscReal y = 0.5 * (-x[i] + x[npoints-i-1]); /* enforces symmetry */
    x[i]           = (a+b)/2 - y*(b-a)/2;
    if (x[i] == -0.0) x[i] = 0.0;
    x[npoints-i-1] = (a+b)/2 + y*(b-a)/2;

    w[i] = w[npoints-1-i] = 0.5*(b-a)*(PetscSqr(PetscAbsScalar(Z[i*npoints])) + PetscSqr(PetscAbsScalar(Z[(npoints-i-1)*npoints])));
  }
  ierr = PetscFree2(Z,work);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDTGaussTensorQuadrature"
/*@
  PetscDTGaussTensorQuadrature - creates a tensor-product Gauss quadrature

  Not Collective

  Input Arguments:
+ dim     - The spatial dimension
. npoints - number of points in one dimension
. a       - left end of interval (often-1)
- b       - right end of interval (often +1)

  Output Argument:
. q - A PetscQuadrature object

  Level: intermediate

.seealso: PetscDTGaussQuadrature(), PetscDTLegendreEval()
@*/
PetscErrorCode PetscDTGaussTensorQuadrature(PetscInt dim, PetscInt npoints, PetscReal a, PetscReal b, PetscQuadrature *q)
{
  PetscInt       totpoints = dim > 1 ? dim > 2 ? npoints*PetscSqr(npoints) : PetscSqr(npoints) : npoints, i, j, k;
  PetscReal     *x, *w, *xw, *ww;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscMalloc1(totpoints*dim,&x);CHKERRQ(ierr);
  ierr = PetscMalloc1(totpoints,&w);CHKERRQ(ierr);
  /* Set up the Golub-Welsch system */
  switch (dim) {
  case 0:
    ierr = PetscFree(x);CHKERRQ(ierr);
    ierr = PetscFree(w);CHKERRQ(ierr);
    ierr = PetscMalloc1(1, &x);CHKERRQ(ierr);
    ierr = PetscMalloc1(1, &w);CHKERRQ(ierr);
    x[0] = 0.0;
    w[0] = 1.0;
    break;
  case 1:
    ierr = PetscDTGaussQuadrature(npoints, a, b, x, w);CHKERRQ(ierr);
    break;
  case 2:
    ierr = PetscMalloc2(npoints,&xw,npoints,&ww);CHKERRQ(ierr);
    ierr = PetscDTGaussQuadrature(npoints, a, b, xw, ww);CHKERRQ(ierr);
    for (i = 0; i < npoints; ++i) {
      for (j = 0; j < npoints; ++j) {
        x[(i*npoints+j)*dim+0] = xw[i];
        x[(i*npoints+j)*dim+1] = xw[j];
        w[i*npoints+j]         = ww[i] * ww[j];
      }
    }
    ierr = PetscFree2(xw,ww);CHKERRQ(ierr);
    break;
  case 3:
    ierr = PetscMalloc2(npoints,&xw,npoints,&ww);CHKERRQ(ierr);
    ierr = PetscDTGaussQuadrature(npoints, a, b, xw, ww);CHKERRQ(ierr);
    for (i = 0; i < npoints; ++i) {
      for (j = 0; j < npoints; ++j) {
        for (k = 0; k < npoints; ++k) {
          x[((i*npoints+j)*npoints+k)*dim+0] = xw[i];
          x[((i*npoints+j)*npoints+k)*dim+1] = xw[j];
          x[((i*npoints+j)*npoints+k)*dim+2] = xw[k];
          w[(i*npoints+j)*npoints+k]         = ww[i] * ww[j] * ww[k];
        }
      }
    }
    ierr = PetscFree2(xw,ww);CHKERRQ(ierr);
    break;
  default:
    SETERRQ1(PETSC_COMM_SELF, PETSC_ERR_ARG_OUTOFRANGE, "Cannot construct quadrature rule for dimension %d", dim);
  }
  ierr = PetscQuadratureCreate(PETSC_COMM_SELF, q);CHKERRQ(ierr);
  ierr = PetscQuadratureSetOrder(*q, npoints);CHKERRQ(ierr);
  ierr = PetscQuadratureSetData(*q, dim, totpoints, x, w);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDTFactorial_Internal"
/* Evaluates the nth jacobi polynomial with weight parameters a,b at a point x.
   Recurrence relations implemented from the pseudocode given in Karniadakis and Sherwin, Appendix B */
PETSC_STATIC_INLINE PetscErrorCode PetscDTFactorial_Internal(PetscInt n, PetscReal *factorial)
{
  PetscReal f = 1.0;
  PetscInt  i;

  PetscFunctionBegin;
  for (i = 1; i < n+1; ++i) f *= i;
  *factorial = f;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDTComputeJacobi"
/* Evaluates the nth jacobi polynomial with weight parameters a,b at a point x.
   Recurrence relations implemented from the pseudocode given in Karniadakis and Sherwin, Appendix B */
PETSC_STATIC_INLINE PetscErrorCode PetscDTComputeJacobi(PetscReal a, PetscReal b, PetscInt n, PetscReal x, PetscReal *P)
{
  PetscReal apb, pn1, pn2;
  PetscInt  k;

  PetscFunctionBegin;
  if (!n) {*P = 1.0; PetscFunctionReturn(0);}
  if (n == 1) {*P = 0.5 * (a - b + (a + b + 2.0) * x); PetscFunctionReturn(0);}
  apb = a + b;
  pn2 = 1.0;
  pn1 = 0.5 * (a - b + (apb + 2.0) * x);
  *P  = 0.0;
  for (k = 2; k < n+1; ++k) {
    PetscReal a1 = 2.0 * k * (k + apb) * (2.0*k + apb - 2.0);
    PetscReal a2 = (2.0 * k + apb - 1.0) * (a*a - b*b);
    PetscReal a3 = (2.0 * k + apb - 2.0) * (2.0 * k + apb - 1.0) * (2.0 * k + apb);
    PetscReal a4 = 2.0 * (k + a - 1.0) * (k + b - 1.0) * (2.0 * k + apb);

    a2  = a2 / a1;
    a3  = a3 / a1;
    a4  = a4 / a1;
    *P  = (a2 + a3 * x) * pn1 - a4 * pn2;
    pn2 = pn1;
    pn1 = *P;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDTComputeJacobiDerivative"
/* Evaluates the first derivative of P_{n}^{a,b} at a point x. */
PETSC_STATIC_INLINE PetscErrorCode PetscDTComputeJacobiDerivative(PetscReal a, PetscReal b, PetscInt n, PetscReal x, PetscReal *P)
{
  PetscReal      nP;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (!n) {*P = 0.0; PetscFunctionReturn(0);}
  ierr = PetscDTComputeJacobi(a+1, b+1, n-1, x, &nP);CHKERRQ(ierr);
  *P   = 0.5 * (a + b + n + 1) * nP;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDTMapSquareToTriangle_Internal"
/* Maps from [-1,1]^2 to the (-1,1) reference triangle */
PETSC_STATIC_INLINE PetscErrorCode PetscDTMapSquareToTriangle_Internal(PetscReal x, PetscReal y, PetscReal *xi, PetscReal *eta)
{
  PetscFunctionBegin;
  *xi  = 0.5 * (1.0 + x) * (1.0 - y) - 1.0;
  *eta = y;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDTMapCubeToTetrahedron_Internal"
/* Maps from [-1,1]^2 to the (-1,1) reference triangle */
PETSC_STATIC_INLINE PetscErrorCode PetscDTMapCubeToTetrahedron_Internal(PetscReal x, PetscReal y, PetscReal z, PetscReal *xi, PetscReal *eta, PetscReal *zeta)
{
  PetscFunctionBegin;
  *xi   = 0.25 * (1.0 + x) * (1.0 - y) * (1.0 - z) - 1.0;
  *eta  = 0.5  * (1.0 + y) * (1.0 - z) - 1.0;
  *zeta = z;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDTGaussJacobiQuadrature1D_Internal"
static PetscErrorCode PetscDTGaussJacobiQuadrature1D_Internal(PetscInt npoints, PetscReal a, PetscReal b, PetscReal *x, PetscReal *w)
{
  PetscInt       maxIter = 100;
  PetscReal      eps     = 1.0e-8;
  PetscReal      a1, a2, a3, a4, a5, a6;
  PetscInt       k;
  PetscErrorCode ierr;

  PetscFunctionBegin;

  a1      = PetscPowReal(2.0, a+b+1);
#if defined(PETSC_HAVE_TGAMMA)
  a2      = PetscTGamma(a + npoints + 1);
  a3      = PetscTGamma(b + npoints + 1);
  a4      = PetscTGamma(a + b + npoints + 1);
#else
  SETERRQ(PETSC_COMM_SELF,PETSC_ERR_SUP,"tgamma() - math routine is unavailable.");
#endif

  ierr = PetscDTFactorial_Internal(npoints, &a5);CHKERRQ(ierr);
  a6   = a1 * a2 * a3 / a4 / a5;
  /* Computes the m roots of P_{m}^{a,b} on [-1,1] by Newton's method with Chebyshev points as initial guesses.
   Algorithm implemented from the pseudocode given by Karniadakis and Sherwin and Python in FIAT */
  for (k = 0; k < npoints; ++k) {
    PetscReal r = -PetscCosReal((2.0*k + 1.0) * PETSC_PI / (2.0 * npoints)), dP;
    PetscInt  j;

    if (k > 0) r = 0.5 * (r + x[k-1]);
    for (j = 0; j < maxIter; ++j) {
      PetscReal s = 0.0, delta, f, fp;
      PetscInt  i;

      for (i = 0; i < k; ++i) s = s + 1.0 / (r - x[i]);
      ierr = PetscDTComputeJacobi(a, b, npoints, r, &f);CHKERRQ(ierr);
      ierr = PetscDTComputeJacobiDerivative(a, b, npoints, r, &fp);CHKERRQ(ierr);
      delta = f / (fp - f * s);
      r     = r - delta;
      if (PetscAbsReal(delta) < eps) break;
    }
    x[k] = r;
    ierr = PetscDTComputeJacobiDerivative(a, b, npoints, x[k], &dP);CHKERRQ(ierr);
    w[k] = a6 / (1.0 - PetscSqr(x[k])) / PetscSqr(dP);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDTGaussJacobiQuadrature"
/*@C
  PetscDTGaussJacobiQuadrature - create Gauss-Jacobi quadrature for a simplex

  Not Collective

  Input Arguments:
+ dim   - The simplex dimension
. order - The number of points in one dimension
. a     - left end of interval (often-1)
- b     - right end of interval (often +1)

  Output Argument:
. q - A PetscQuadrature object

  Level: intermediate

  References:
.  1. - Karniadakis and Sherwin.  FIAT

.seealso: PetscDTGaussTensorQuadrature(), PetscDTGaussQuadrature()
@*/
PetscErrorCode PetscDTGaussJacobiQuadrature(PetscInt dim, PetscInt order, PetscReal a, PetscReal b, PetscQuadrature *q)
{
  PetscInt       npoints = dim > 1 ? dim > 2 ? order*PetscSqr(order) : PetscSqr(order) : order;
  PetscReal     *px, *wx, *py, *wy, *pz, *wz, *x, *w;
  PetscInt       i, j, k;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if ((a != -1.0) || (b != 1.0)) SETERRQ(PETSC_COMM_SELF, PETSC_ERR_ARG_OUTOFRANGE, "Must use default internal right now");
  ierr = PetscMalloc1(npoints*dim, &x);CHKERRQ(ierr);
  ierr = PetscMalloc1(npoints, &w);CHKERRQ(ierr);
  switch (dim) {
  case 0:
    ierr = PetscFree(x);CHKERRQ(ierr);
    ierr = PetscFree(w);CHKERRQ(ierr);
    ierr = PetscMalloc1(1, &x);CHKERRQ(ierr);
    ierr = PetscMalloc1(1, &w);CHKERRQ(ierr);
    x[0] = 0.0;
    w[0] = 1.0;
    break;
  case 1:
    ierr = PetscDTGaussJacobiQuadrature1D_Internal(order, 0.0, 0.0, x, w);CHKERRQ(ierr);
    break;
  case 2:
    ierr = PetscMalloc4(order,&px,order,&wx,order,&py,order,&wy);CHKERRQ(ierr);
    ierr = PetscDTGaussJacobiQuadrature1D_Internal(order, 0.0, 0.0, px, wx);CHKERRQ(ierr);
    ierr = PetscDTGaussJacobiQuadrature1D_Internal(order, 1.0, 0.0, py, wy);CHKERRQ(ierr);
    for (i = 0; i < order; ++i) {
      for (j = 0; j < order; ++j) {
        ierr = PetscDTMapSquareToTriangle_Internal(px[i], py[j], &x[(i*order+j)*2+0], &x[(i*order+j)*2+1]);CHKERRQ(ierr);
        w[i*order+j] = 0.5 * wx[i] * wy[j];
      }
    }
    ierr = PetscFree4(px,wx,py,wy);CHKERRQ(ierr);
    break;
  case 3:
    ierr = PetscMalloc6(order,&px,order,&wx,order,&py,order,&wy,order,&pz,order,&wz);CHKERRQ(ierr);
    ierr = PetscDTGaussJacobiQuadrature1D_Internal(order, 0.0, 0.0, px, wx);CHKERRQ(ierr);
    ierr = PetscDTGaussJacobiQuadrature1D_Internal(order, 1.0, 0.0, py, wy);CHKERRQ(ierr);
    ierr = PetscDTGaussJacobiQuadrature1D_Internal(order, 2.0, 0.0, pz, wz);CHKERRQ(ierr);
    for (i = 0; i < order; ++i) {
      for (j = 0; j < order; ++j) {
        for (k = 0; k < order; ++k) {
          ierr = PetscDTMapCubeToTetrahedron_Internal(px[i], py[j], pz[k], &x[((i*order+j)*order+k)*3+0], &x[((i*order+j)*order+k)*3+1], &x[((i*order+j)*order+k)*3+2]);CHKERRQ(ierr);
          w[(i*order+j)*order+k] = 0.125 * wx[i] * wy[j] * wz[k];
        }
      }
    }
    ierr = PetscFree6(px,wx,py,wy,pz,wz);CHKERRQ(ierr);
    break;
  default:
    SETERRQ1(PETSC_COMM_SELF, PETSC_ERR_ARG_OUTOFRANGE, "Cannot construct quadrature rule for dimension %d", dim);
  }
  ierr = PetscQuadratureCreate(PETSC_COMM_SELF, q);CHKERRQ(ierr);
  ierr = PetscQuadratureSetOrder(*q, order);CHKERRQ(ierr);
  ierr = PetscQuadratureSetData(*q, dim, npoints, x, w);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDTTanhSinhTensorQuadrature"
/*@C
  PetscDTTanhSinhTensorQuadrature - create tanh-sinh quadrature for a tensor product cell

  Not Collective

  Input Arguments:
+ dim   - The cell dimension
. level - The number of points in one dimension, 2^l
. a     - left end of interval (often-1)
- b     - right end of interval (often +1)

  Output Argument:
. q - A PetscQuadrature object

  Level: intermediate

.seealso: PetscDTGaussTensorQuadrature()
@*/
PetscErrorCode PetscDTTanhSinhTensorQuadrature(PetscInt dim, PetscInt level, PetscReal a, PetscReal b, PetscQuadrature *q)
{
  const PetscInt  p     = 16;                        /* Digits of precision in the evaluation */
  const PetscReal alpha = (b-a)/2.;                  /* Half-width of the integration interval */
  const PetscReal beta  = (b+a)/2.;                  /* Center of the integration interval */
  const PetscReal h     = PetscPowReal(2.0, -level); /* Step size, length between x_k */
  PetscReal       xk;                                /* Quadrature point x_k on reference domain [-1, 1] */
  PetscReal       wk    = 0.5*PETSC_PI;              /* Quadrature weight at x_k */
  PetscReal      *x, *w;
  PetscInt        K, k, npoints;
  PetscErrorCode  ierr;

  PetscFunctionBegin;
  if (dim > 1) SETERRQ1(PETSC_COMM_SELF, PETSC_ERR_SUP, "Dimension %d not yet implemented", dim);
  if (!level) SETERRQ(PETSC_COMM_SELF, PETSC_ERR_ARG_OUTOFRANGE, "Must give a number of significant digits");
  /* Find K such that the weights are < 32 digits of precision */
  for (K = 1; PetscAbsReal(PetscLog10Real(wk)) < 2*p; ++K) {
    wk = 0.5*h*PETSC_PI*PetscCoshReal(K*h)/PetscSqr(PetscCoshReal(0.5*PETSC_PI*PetscSinhReal(K*h)));
  }
  ierr = PetscQuadratureCreate(PETSC_COMM_SELF, q);CHKERRQ(ierr);
  ierr = PetscQuadratureSetOrder(*q, 2*K+1);CHKERRQ(ierr);
  npoints = 2*K-1;
  ierr = PetscMalloc1(npoints*dim, &x);CHKERRQ(ierr);
  ierr = PetscMalloc1(npoints, &w);CHKERRQ(ierr);
  /* Center term */
  x[0] = beta;
  w[0] = 0.5*alpha*PETSC_PI;
  for (k = 1; k < K; ++k) {
    wk = 0.5*alpha*h*PETSC_PI*PetscCoshReal(k*h)/PetscSqr(PetscCoshReal(0.5*PETSC_PI*PetscSinhReal(k*h)));
    xk = tanh(0.5*PETSC_PI*PetscSinhReal(k*h));
    x[2*k-1] = -alpha*xk+beta;
    w[2*k-1] = wk;
    x[2*k+0] =  alpha*xk+beta;
    w[2*k+0] = wk;
  }
  ierr = PetscQuadratureSetData(*q, dim, npoints, x, w);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDTTanhSinhIntegrate"
PetscErrorCode PetscDTTanhSinhIntegrate(void (*func)(PetscReal, PetscReal *), PetscReal a, PetscReal b, PetscInt digits, PetscReal *sol)
{
  const PetscInt  p     = 16;        /* Digits of precision in the evaluation */
  const PetscReal alpha = (b-a)/2.;  /* Half-width of the integration interval */
  const PetscReal beta  = (b+a)/2.;  /* Center of the integration interval */
  PetscReal       h     = 1.0;       /* Step size, length between x_k */
  PetscInt        l     = 0;         /* Level of refinement, h = 2^{-l} */
  PetscReal       osum  = 0.0;       /* Integral on last level */
  PetscReal       psum  = 0.0;       /* Integral on the level before the last level */
  PetscReal       sum;               /* Integral on current level */
  PetscReal       yk;                /* Quadrature point 1 - x_k on reference domain [-1, 1] */
  PetscReal       lx, rx;            /* Quadrature points to the left and right of 0 on the real domain [a, b] */
  PetscReal       wk;                /* Quadrature weight at x_k */
  PetscReal       lval, rval;        /* Terms in the quadature sum to the left and right of 0 */
  PetscInt        d;                 /* Digits of precision in the integral */

  PetscFunctionBegin;
  if (digits <= 0) SETERRQ(PETSC_COMM_SELF, PETSC_ERR_ARG_OUTOFRANGE, "Must give a positive number of significant digits");
  /* Center term */
  func(beta, &lval);
  sum = 0.5*alpha*PETSC_PI*lval;
  /* */
  do {
    PetscReal lterm, rterm, maxTerm = 0.0, d1, d2, d3, d4;
    PetscInt  k = 1;

    ++l;
    /* PetscPrintf(PETSC_COMM_SELF, "LEVEL %D sum: %15.15f\n", l, sum); */
    /* At each level of refinement, h --> h/2 and sum --> sum/2 */
    psum = osum;
    osum = sum;
    h   *= 0.5;
    sum *= 0.5;
    do {
      wk = 0.5*h*PETSC_PI*PetscCoshReal(k*h)/PetscSqr(PetscCoshReal(0.5*PETSC_PI*PetscSinhReal(k*h)));
      yk = 1.0/(PetscExpReal(0.5*PETSC_PI*PetscSinhReal(k*h)) * PetscCoshReal(0.5*PETSC_PI*PetscSinhReal(k*h)));
      lx = -alpha*(1.0 - yk)+beta;
      rx =  alpha*(1.0 - yk)+beta;
      func(lx, &lval);
      func(rx, &rval);
      lterm   = alpha*wk*lval;
      maxTerm = PetscMax(PetscAbsReal(lterm), maxTerm);
      sum    += lterm;
      rterm   = alpha*wk*rval;
      maxTerm = PetscMax(PetscAbsReal(rterm), maxTerm);
      sum    += rterm;
      /* if (l == 1) printf("k is %d and sum is %15.15f and wk is %15.15f\n", k, sum, wk); */
      ++k;
      /* Only need to evaluate every other point on refined levels */
      if (l != 1) ++k;
    } while (PetscAbsReal(PetscLog10Real(wk)) < p); /* Only need to evaluate sum until weights are < 32 digits of precision */

    d1 = PetscLog10Real(PetscAbsReal(sum - osum));
    d2 = PetscLog10Real(PetscAbsReal(sum - psum));
    d3 = PetscLog10Real(maxTerm) - p;
    if (PetscMax(PetscAbsReal(lterm), PetscAbsReal(rterm)) == 0.0) d4 = 0.0;
    else d4 = PetscLog10Real(PetscMax(PetscAbsReal(lterm), PetscAbsReal(rterm)));
    d  = PetscAbsInt(PetscMin(0, PetscMax(PetscMax(PetscMax(PetscSqr(d1)/d2, 2*d1), d3), d4)));
  } while (d < digits && l < 12);
  *sol = sum;

  PetscFunctionReturn(0);
}

#ifdef PETSC_HAVE_MPFR
#undef __FUNCT__
#define __FUNCT__ "PetscDTTanhSinhIntegrateMPFR"
PetscErrorCode PetscDTTanhSinhIntegrateMPFR(void (*func)(PetscReal, PetscReal *), PetscReal a, PetscReal b, PetscInt digits, PetscReal *sol)
{
  const PetscInt  safetyFactor = 2;  /* Calculate abcissa until 2*p digits */
  PetscInt        l            = 0;  /* Level of refinement, h = 2^{-l} */
  mpfr_t          alpha;             /* Half-width of the integration interval */
  mpfr_t          beta;              /* Center of the integration interval */
  mpfr_t          h;                 /* Step size, length between x_k */
  mpfr_t          osum;              /* Integral on last level */
  mpfr_t          psum;              /* Integral on the level before the last level */
  mpfr_t          sum;               /* Integral on current level */
  mpfr_t          yk;                /* Quadrature point 1 - x_k on reference domain [-1, 1] */
  mpfr_t          lx, rx;            /* Quadrature points to the left and right of 0 on the real domain [a, b] */
  mpfr_t          wk;                /* Quadrature weight at x_k */
  PetscReal       lval, rval;        /* Terms in the quadature sum to the left and right of 0 */
  PetscInt        d;                 /* Digits of precision in the integral */
  mpfr_t          pi2, kh, msinh, mcosh, maxTerm, curTerm, tmp;

  PetscFunctionBegin;
  if (digits <= 0) SETERRQ(PETSC_COMM_SELF, PETSC_ERR_ARG_OUTOFRANGE, "Must give a positive number of significant digits");
  /* Create high precision storage */
  mpfr_inits2(PetscCeilReal(safetyFactor*digits*PetscLogReal(10.)/PetscLogReal(2.)), alpha, beta, h, sum, osum, psum, yk, wk, lx, rx, tmp, maxTerm, curTerm, pi2, kh, msinh, mcosh, NULL);
  /* Initialization */
  mpfr_set_d(alpha, 0.5*(b-a), MPFR_RNDN);
  mpfr_set_d(beta,  0.5*(b+a), MPFR_RNDN);
  mpfr_set_d(osum,  0.0,       MPFR_RNDN);
  mpfr_set_d(psum,  0.0,       MPFR_RNDN);
  mpfr_set_d(h,     1.0,       MPFR_RNDN);
  mpfr_const_pi(pi2, MPFR_RNDN);
  mpfr_mul_d(pi2, pi2, 0.5, MPFR_RNDN);
  /* Center term */
  func(0.5*(b+a), &lval);
  mpfr_set(sum, pi2, MPFR_RNDN);
  mpfr_mul(sum, sum, alpha, MPFR_RNDN);
  mpfr_mul_d(sum, sum, lval, MPFR_RNDN);
  /* */
  do {
    PetscReal d1, d2, d3, d4;
    PetscInt  k = 1;

    ++l;
    mpfr_set_d(maxTerm, 0.0, MPFR_RNDN);
    /* PetscPrintf(PETSC_COMM_SELF, "LEVEL %D sum: %15.15f\n", l, sum); */
    /* At each level of refinement, h --> h/2 and sum --> sum/2 */
    mpfr_set(psum, osum, MPFR_RNDN);
    mpfr_set(osum,  sum, MPFR_RNDN);
    mpfr_mul_d(h,   h,   0.5, MPFR_RNDN);
    mpfr_mul_d(sum, sum, 0.5, MPFR_RNDN);
    do {
      mpfr_set_si(kh, k, MPFR_RNDN);
      mpfr_mul(kh, kh, h, MPFR_RNDN);
      /* Weight */
      mpfr_set(wk, h, MPFR_RNDN);
      mpfr_sinh_cosh(msinh, mcosh, kh, MPFR_RNDN);
      mpfr_mul(msinh, msinh, pi2, MPFR_RNDN);
      mpfr_mul(mcosh, mcosh, pi2, MPFR_RNDN);
      mpfr_cosh(tmp, msinh, MPFR_RNDN);
      mpfr_sqr(tmp, tmp, MPFR_RNDN);
      mpfr_mul(wk, wk, mcosh, MPFR_RNDN);
      mpfr_div(wk, wk, tmp, MPFR_RNDN);
      /* Abscissa */
      mpfr_set_d(yk, 1.0, MPFR_RNDZ);
      mpfr_cosh(tmp, msinh, MPFR_RNDN);
      mpfr_div(yk, yk, tmp, MPFR_RNDZ);
      mpfr_exp(tmp, msinh, MPFR_RNDN);
      mpfr_div(yk, yk, tmp, MPFR_RNDZ);
      /* Quadrature points */
      mpfr_sub_d(lx, yk, 1.0, MPFR_RNDZ);
      mpfr_mul(lx, lx, alpha, MPFR_RNDU);
      mpfr_add(lx, lx, beta, MPFR_RNDU);
      mpfr_d_sub(rx, 1.0, yk, MPFR_RNDZ);
      mpfr_mul(rx, rx, alpha, MPFR_RNDD);
      mpfr_add(rx, rx, beta, MPFR_RNDD);
      /* Evaluation */
      func(mpfr_get_d(lx, MPFR_RNDU), &lval);
      func(mpfr_get_d(rx, MPFR_RNDD), &rval);
      /* Update */
      mpfr_mul(tmp, wk, alpha, MPFR_RNDN);
      mpfr_mul_d(tmp, tmp, lval, MPFR_RNDN);
      mpfr_add(sum, sum, tmp, MPFR_RNDN);
      mpfr_abs(tmp, tmp, MPFR_RNDN);
      mpfr_max(maxTerm, maxTerm, tmp, MPFR_RNDN);
      mpfr_set(curTerm, tmp, MPFR_RNDN);
      mpfr_mul(tmp, wk, alpha, MPFR_RNDN);
      mpfr_mul_d(tmp, tmp, rval, MPFR_RNDN);
      mpfr_add(sum, sum, tmp, MPFR_RNDN);
      mpfr_abs(tmp, tmp, MPFR_RNDN);
      mpfr_max(maxTerm, maxTerm, tmp, MPFR_RNDN);
      mpfr_max(curTerm, curTerm, tmp, MPFR_RNDN);
      /* if (l == 1) printf("k is %d and sum is %15.15f and wk is %15.15f\n", k, sum, wk); */
      ++k;
      /* Only need to evaluate every other point on refined levels */
      if (l != 1) ++k;
      mpfr_log10(tmp, wk, MPFR_RNDN);
      mpfr_abs(tmp, tmp, MPFR_RNDN);
    } while (mpfr_get_d(tmp, MPFR_RNDN) < safetyFactor*digits); /* Only need to evaluate sum until weights are < 32 digits of precision */
    mpfr_sub(tmp, sum, osum, MPFR_RNDN);
    mpfr_abs(tmp, tmp, MPFR_RNDN);
    mpfr_log10(tmp, tmp, MPFR_RNDN);
    d1 = mpfr_get_d(tmp, MPFR_RNDN);
    mpfr_sub(tmp, sum, psum, MPFR_RNDN);
    mpfr_abs(tmp, tmp, MPFR_RNDN);
    mpfr_log10(tmp, tmp, MPFR_RNDN);
    d2 = mpfr_get_d(tmp, MPFR_RNDN);
    mpfr_log10(tmp, maxTerm, MPFR_RNDN);
    d3 = mpfr_get_d(tmp, MPFR_RNDN) - digits;
    mpfr_log10(tmp, curTerm, MPFR_RNDN);
    d4 = mpfr_get_d(tmp, MPFR_RNDN);
    d  = PetscAbsInt(PetscMin(0, PetscMax(PetscMax(PetscMax(PetscSqr(d1)/d2, 2*d1), d3), d4)));
  } while (d < digits && l < 8);
  *sol = mpfr_get_d(sum, MPFR_RNDN);
  /* Cleanup */
  mpfr_clears(alpha, beta, h, sum, osum, psum, yk, wk, lx, rx, tmp, maxTerm, curTerm, pi2, kh, msinh, mcosh, NULL);
  PetscFunctionReturn(0);
}
#else
#undef __FUNCT__
#define __FUNCT__ "PetscDTTanhSinhIntegrateMPFR"
PetscErrorCode PetscDTTanhSinhIntegrateMPFR(void (*func)(PetscReal, PetscReal *), PetscReal a, PetscReal b, PetscInt digits, PetscReal *sol)
{
  SETERRQ(PETSC_COMM_SELF, PETSC_ERR_SUP, "This method will not work without MPFR. Reconfigure using --download-mpfr --download-gmp");
}
#endif

#undef __FUNCT__
#define __FUNCT__ "PetscDTPseudoInverseQR"
/* Overwrites A. Can only handle full-rank problems with m>=n
 * A in column-major format
 * Ainv in row-major format
 * tau has length m
 * worksize must be >= max(1,n)
 */
static PetscErrorCode PetscDTPseudoInverseQR(PetscInt m,PetscInt mstride,PetscInt n,PetscReal *A_in,PetscReal *Ainv_out,PetscScalar *tau,PetscInt worksize,PetscScalar *work)
{
  PetscErrorCode ierr;
  PetscBLASInt   M,N,K,lda,ldb,ldwork,info;
  PetscScalar    *A,*Ainv,*R,*Q,Alpha;

  PetscFunctionBegin;
#if defined(PETSC_USE_COMPLEX)
  {
    PetscInt i,j;
    ierr = PetscMalloc2(m*n,&A,m*n,&Ainv);CHKERRQ(ierr);
    for (j=0; j<n; j++) {
      for (i=0; i<m; i++) A[i+m*j] = A_in[i+mstride*j];
    }
    mstride = m;
  }
#else
  A = A_in;
  Ainv = Ainv_out;
#endif

  ierr = PetscBLASIntCast(m,&M);CHKERRQ(ierr);
  ierr = PetscBLASIntCast(n,&N);CHKERRQ(ierr);
  ierr = PetscBLASIntCast(mstride,&lda);CHKERRQ(ierr);
  ierr = PetscBLASIntCast(worksize,&ldwork);CHKERRQ(ierr);
  ierr = PetscFPTrapPush(PETSC_FP_TRAP_OFF);CHKERRQ(ierr);
  PetscStackCallBLAS("LAPACKgeqrf",LAPACKgeqrf_(&M,&N,A,&lda,tau,work,&ldwork,&info));
  ierr = PetscFPTrapPop();CHKERRQ(ierr);
  if (info) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_LIB,"xGEQRF error");
  R = A; /* Upper triangular part of A now contains R, the rest contains the elementary reflectors */

  /* Extract an explicit representation of Q */
  Q = Ainv;
  ierr = PetscMemcpy(Q,A,mstride*n*sizeof(PetscScalar));CHKERRQ(ierr);
  K = N;                        /* full rank */
  PetscStackCallBLAS("LAPACKungqr",LAPACKungqr_(&M,&N,&K,Q,&lda,tau,work,&ldwork,&info));
  if (info) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_LIB,"xORGQR/xUNGQR error");

  /* Compute A^{-T} = (R^{-1} Q^T)^T = Q R^{-T} */
  Alpha = 1.0;
  ldb = lda;
  PetscStackCallBLAS("BLAStrsm",BLAStrsm_("Right","Upper","ConjugateTranspose","NotUnitTriangular",&M,&N,&Alpha,R,&lda,Q,&ldb));
  /* Ainv is Q, overwritten with inverse */

#if defined(PETSC_USE_COMPLEX)
  {
    PetscInt i;
    for (i=0; i<m*n; i++) Ainv_out[i] = PetscRealPart(Ainv[i]);
    ierr = PetscFree2(A,Ainv);CHKERRQ(ierr);
  }
#endif
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDTLegendreIntegrate"
/* Computes integral of L_p' over intervals {(x0,x1),(x1,x2),...} */
static PetscErrorCode PetscDTLegendreIntegrate(PetscInt ninterval,const PetscReal *x,PetscInt ndegree,const PetscInt *degrees,PetscBool Transpose,PetscReal *B)
{
  PetscErrorCode ierr;
  PetscReal      *Bv;
  PetscInt       i,j;

  PetscFunctionBegin;
  ierr = PetscMalloc1((ninterval+1)*ndegree,&Bv);CHKERRQ(ierr);
  /* Point evaluation of L_p on all the source vertices */
  ierr = PetscDTLegendreEval(ninterval+1,x,ndegree,degrees,Bv,NULL,NULL);CHKERRQ(ierr);
  /* Integral over each interval: \int_a^b L_p' = L_p(b)-L_p(a) */
  for (i=0; i<ninterval; i++) {
    for (j=0; j<ndegree; j++) {
      if (Transpose) B[i+ninterval*j] = Bv[(i+1)*ndegree+j] - Bv[i*ndegree+j];
      else           B[i*ndegree+j]   = Bv[(i+1)*ndegree+j] - Bv[i*ndegree+j];
    }
  }
  ierr = PetscFree(Bv);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDTReconstructPoly"
/*@
   PetscDTReconstructPoly - create matrix representing polynomial reconstruction using cell intervals and evaluation at target intervals

   Not Collective

   Input Arguments:
+  degree - degree of reconstruction polynomial
.  nsource - number of source intervals
.  sourcex - sorted coordinates of source cell boundaries (length nsource+1)
.  ntarget - number of target intervals
-  targetx - sorted coordinates of target cell boundaries (length ntarget+1)

   Output Arguments:
.  R - reconstruction matrix, utarget = sum_s R[t*nsource+s] * usource[s]

   Level: advanced

.seealso: PetscDTLegendreEval()
@*/
PetscErrorCode PetscDTReconstructPoly(PetscInt degree,PetscInt nsource,const PetscReal *sourcex,PetscInt ntarget,const PetscReal *targetx,PetscReal *R)
{
  PetscErrorCode ierr;
  PetscInt       i,j,k,*bdegrees,worksize;
  PetscReal      xmin,xmax,center,hscale,*sourcey,*targety,*Bsource,*Bsinv,*Btarget;
  PetscScalar    *tau,*work;

  PetscFunctionBegin;
  PetscValidRealPointer(sourcex,3);
  PetscValidRealPointer(targetx,5);
  PetscValidRealPointer(R,6);
  if (degree >= nsource) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_INCOMP,"Reconstruction degree %D must be less than number of source intervals %D",degree,nsource);
#if defined(PETSC_USE_DEBUG)
  for (i=0; i<nsource; i++) {
    if (sourcex[i] >= sourcex[i+1]) SETERRQ3(PETSC_COMM_SELF,PETSC_ERR_ARG_CORRUPT,"Source interval %D has negative orientation (%g,%g)",i,(double)sourcex[i],(double)sourcex[i+1]);
  }
  for (i=0; i<ntarget; i++) {
    if (targetx[i] >= targetx[i+1]) SETERRQ3(PETSC_COMM_SELF,PETSC_ERR_ARG_CORRUPT,"Target interval %D has negative orientation (%g,%g)",i,(double)targetx[i],(double)targetx[i+1]);
  }
#endif
  xmin = PetscMin(sourcex[0],targetx[0]);
  xmax = PetscMax(sourcex[nsource],targetx[ntarget]);
  center = (xmin + xmax)/2;
  hscale = (xmax - xmin)/2;
  worksize = nsource;
  ierr = PetscMalloc4(degree+1,&bdegrees,nsource+1,&sourcey,nsource*(degree+1),&Bsource,worksize,&work);CHKERRQ(ierr);
  ierr = PetscMalloc4(nsource,&tau,nsource*(degree+1),&Bsinv,ntarget+1,&targety,ntarget*(degree+1),&Btarget);CHKERRQ(ierr);
  for (i=0; i<=nsource; i++) sourcey[i] = (sourcex[i]-center)/hscale;
  for (i=0; i<=degree; i++) bdegrees[i] = i+1;
  ierr = PetscDTLegendreIntegrate(nsource,sourcey,degree+1,bdegrees,PETSC_TRUE,Bsource);CHKERRQ(ierr);
  ierr = PetscDTPseudoInverseQR(nsource,nsource,degree+1,Bsource,Bsinv,tau,nsource,work);CHKERRQ(ierr);
  for (i=0; i<=ntarget; i++) targety[i] = (targetx[i]-center)/hscale;
  ierr = PetscDTLegendreIntegrate(ntarget,targety,degree+1,bdegrees,PETSC_FALSE,Btarget);CHKERRQ(ierr);
  for (i=0; i<ntarget; i++) {
    PetscReal rowsum = 0;
    for (j=0; j<nsource; j++) {
      PetscReal sum = 0;
      for (k=0; k<degree+1; k++) {
        sum += Btarget[i*(degree+1)+k] * Bsinv[k*nsource+j];
      }
      R[i*nsource+j] = sum;
      rowsum += sum;
    }
    for (j=0; j<nsource; j++) R[i*nsource+j] /= rowsum; /* normalize each row */
  }
  ierr = PetscFree4(bdegrees,sourcey,Bsource,work);CHKERRQ(ierr);
  ierr = PetscFree4(tau,Bsinv,targety,Btarget);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}
