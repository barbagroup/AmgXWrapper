! - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
!             Include file for program eptorsion2f.F
! - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
!
!  This program uses CPP for preprocessing, as indicated by the use of
!  TAO include files in the directories $TAO_DIR/include/petsc/finclude and
!  $PETSC_DIR/include/petsc/finclude.  This convention enables use of the CPP
!  preprocessor, which allows the use of the #include statements that
!  define TAO objects and variables.
!
!  Since one must be very careful to include each file no more than once
!  in a Fortran routine, application programmers must explicitly list
!  each file needed for the various TAO and PETSc components within their
!  program (unlike the C/C++ interface).
!
!  See the Fortran section of the PETSc users manual for details.
!
!  The following include statements are generally used in TAO programs:
!     tao_solver.h - TAO solvers
!     petscksp.h   - Krylov subspace methods
!     petscpc.h    - preconditioners
!     petscmat.h   - matrices
!     petscvec.h   - vectors
!     petsc.h      - basic PETSc routines
!  In addition, we need the following for use of distributed arrays and
!  index sets:
!     petscdm.h    - distributed arrays (DA)
!     petscis.h    - index sets (IS)

#include "petsc/finclude/petscsys.h"
#include "petsc/finclude/petscvec.h"
#include "petsc/finclude/petscmat.h"
#include "petsc/finclude/petscksp.h"
#include "petsc/finclude/petscpc.h"
#include "petsc/finclude/petscsnes.h"
#include "petsc/finclude/petscdmda.h"
#include "petsc/finclude/petscdm.h"
#include "petsc/finclude/petscis.h"
#include "petsc/finclude/petsctao.h"

!  Common blocks:
!  In this example we use common blocks to store data needed by the
!  application-provided call-back routines, FormFunction(), FormGradient(),
!  and FormHessian().  Note that we can store (pointers to) TAO objects
!  within these common blocks.
!
!  common /params/ - contains parameters for the global application
!     mx, my     - global discretization in x- and y-directions
!     param      - nonlinearity parameter
!
!  common /pdata/ - contains some parallel data
!     localX     - local work vector (including ghost points)
!     localS     - local work vector (including ghost points)
!     dm         - distributed array
!
      Vec              localX
      DM               dm
      PetscReal      param
      PetscInt         mx, my

      common /params/ param,mx,my
      common /pdata/  dm,localX

! - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


