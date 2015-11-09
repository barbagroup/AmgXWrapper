/*****************************************************************************/
/* Context for using preconditioned conjugate gradient method to minimized a */
/* quadratic function subject to a trust region constraint.  If the matrix   */
/* is indefinite, a direction of negative curvature may be encountered.  If  */
/* a direction of negative curvature is found, then we follow it to the      */
/* boundary of the trust region.                                             */
/*                                                                           */
/* This method is described in:                                              */
/*   T. Steihaug, "The Conjugate Gradient Method and Trust Regions in Large  */
/*     Scale Optimization", SIAM Journal on Numerical Analysis, 20,          */
/*     pages 626-637, 1983.                                                  */
/*****************************************************************************/

#if !defined(__STCG)
#define __STCG

typedef struct {
  PetscReal radius;
  PetscReal norm_d;
  PetscReal o_fcn;
  PetscInt  dtype;
} KSP_STCG;

#endif

