!
!
!  Base include file for Fortran use of the PETSc package.
!
#include "petscconf.h"
#include "petscversion.h"
#include "petsc/finclude/petscsysdef.h"

#if !defined(PETSC_AVOID_MPIF_H)
#include "mpif.h"
#endif

! ------------------------------------------------------------------------
!     Non Common block Stuff declared first
!
!     Flags
!
      PetscBool  PETSC_TRUE
      PetscBool  PETSC_FALSE
#if defined(PETSC_FORTRAN_PETSCTRUTH_INT)
      parameter (PETSC_TRUE = 1,PETSC_FALSE = 0)
#else
      parameter (PETSC_TRUE = .true.,PETSC_FALSE = .false.)
#endif
      PetscInt   PETSC_DECIDE,PETSC_DETERMINE
      parameter (PETSC_DECIDE=-1,PETSC_DETERMINE=-1)

      PetscInt  PETSC_DEFAULT_INTEGER
      parameter (PETSC_DEFAULT_INTEGER = -2)

      PetscReal PETSC_DEFAULT_REAL
      parameter (PETSC_DEFAULT_REAL=-2.0d0)

      PetscEnum PETSC_FP_TRAP_OFF
      PetscEnum PETSC_FP_TRAP_ON
      parameter (PETSC_FP_TRAP_OFF = 0,PETSC_FP_TRAP_ON = 1)



!
!     Default PetscViewers.
!
      PetscFortranAddr PETSC_VIEWER_DRAW_WORLD
      PetscFortranAddr PETSC_VIEWER_DRAW_SELF
      PetscFortranAddr PETSC_VIEWER_SOCKET_WORLD
      PetscFortranAddr PETSC_VIEWER_SOCKET_SELF
      PetscFortranAddr PETSC_VIEWER_STDOUT_WORLD
      PetscFortranAddr PETSC_VIEWER_STDOUT_SELF
      PetscFortranAddr PETSC_VIEWER_STDERR_WORLD
      PetscFortranAddr PETSC_VIEWER_STDERR_SELF
      PetscFortranAddr PETSC_VIEWER_BINARY_WORLD
      PetscFortranAddr PETSC_VIEWER_BINARY_SELF
      PetscFortranAddr PETSC_VIEWER_MATLAB_WORLD
      PetscFortranAddr PETSC_VIEWER_MATLAB_SELF

!
!     The numbers used below should match those in
!     petsc/private/fortranimpl.h
!
      parameter (PETSC_VIEWER_DRAW_WORLD   = 4)
      parameter (PETSC_VIEWER_DRAW_SELF    = 5)
      parameter (PETSC_VIEWER_SOCKET_WORLD = 6)
      parameter (PETSC_VIEWER_SOCKET_SELF  = 7)
      parameter (PETSC_VIEWER_STDOUT_WORLD = 8)
      parameter (PETSC_VIEWER_STDOUT_SELF  = 9)
      parameter (PETSC_VIEWER_STDERR_WORLD = 10)
      parameter (PETSC_VIEWER_STDERR_SELF  = 11)
      parameter (PETSC_VIEWER_BINARY_WORLD = 12)
      parameter (PETSC_VIEWER_BINARY_SELF  = 13)
      parameter (PETSC_VIEWER_MATLAB_WORLD = 14)
      parameter (PETSC_VIEWER_MATLAB_SELF  = 15)
!
!     PETSc DataTypes
!
      PetscEnum PETSC_INT
      PetscEnum PETSC_DOUBLE
      PetscEnum PETSC_COMPLEX
      PetscEnum PETSC_LONG
      PetscEnum PETSC_SHORT
      PetscEnum PETSC_FLOAT
      PetscEnum PETSC_CHAR
      PetscEnum PETSC_BIT_LOGICAL
      PetscEnum PETSC_ENUM
      PetscEnum PETSC_BOOL
      PetscEnum PETSC___FLOAT128

#if defined(PETSC_USE_REAL_SINGLE)
#define PETSC_REAL PETSC_FLOAT
#elif defined(PETSC_USE_REAL___FLOAT128)
#define PETSC_REAL PETSC___FLOAT128
#else
#define PETSC_REAL PETSC_DOUBLE
#endif
#define PETSC_FORTRANADDR PETSC_LONG

      parameter (PETSC_INT=0,PETSC_DOUBLE=1,PETSC_COMPLEX=2)
      parameter (PETSC_LONG=3,PETSC_SHORT=4,PETSC_FLOAT=5)
      parameter (PETSC_CHAR=6,PETSC_BIT_LOGICAL=7,PETSC_ENUM=8)
      parameter (PETSC_BOOL=9,PETSC___FLOAT128=10)
!
!
!
      PetscEnum PETSC_COPY_VALUES
      PetscEnum PETSC_OWN_POINTER
      PetscEnum PETSC_USE_POINTER

      parameter (PETSC_COPY_VALUES = 0)
      parameter (PETSC_OWN_POINTER = 1)
      parameter (PETSC_USE_POINTER = 2)
!
! ------------------------------------------------------------------------
!     PETSc mathematics include file. Defines certain basic mathematical
!    constants and functions for working with single and double precision
!    floating point numbers as well as complex and integers.
!
!     Representation of complex i
!
      PetscFortranComplex PETSC_i
#if defined(PETSC_USE_REAL_SINGLE)
      parameter (PETSC_i = (0.0e0,1.0e0))
#else
      parameter (PETSC_i = (0.0d0,1.0d0))
#endif

!
! ----------------------------------------------------------------------------
!    BEGIN PETSc aliases for MPI_ constants
!
!   These values for __float128 are handled in the common block (below)
!     and transmitted from the C code
!
#if !defined(PETSC_USE_REAL___FLOAT128)
      integer MPIU_REAL
#if defined (PETSC_USE_REAL_SINGLE)
      parameter (MPIU_REAL = MPI_REAL)
#else
      parameter(MPIU_REAL = MPI_DOUBLE_PRECISION)
#endif

      integer MPIU_SUM
      parameter (MPIU_SUM = MPI_SUM)

      integer MPIU_SCALAR
#if defined(PETSC_USE_COMPLEX)
#if defined (PETSC_USE_REAL_SINGLE)
      parameter(MPIU_SCALAR = MPI_COMPLEX)
#else
      parameter(MPIU_SCALAR = MPI_DOUBLE_COMPLEX)
#endif
#else
#if defined (PETSC_USE_REAL_SINGLE)
      parameter (MPIU_SCALAR = MPI_REAL)
#else
      parameter(MPIU_SCALAR = MPI_DOUBLE_PRECISION)
#endif
#endif
#endif

      integer MPIU_INTEGER
#if defined(PETSC_USE_64BIT_INDICES)
      parameter(MPIU_INTEGER = MPI_INTEGER8)
#else
      parameter(MPIU_INTEGER = MPI_INTEGER)
#endif

!
! ----------------------------------------------------------------------------
!    BEGIN COMMON-BLOCK VARIABLES
!
!
!     PETSc world communicator
!
      MPI_Comm PETSC_COMM_WORLD
      MPI_Comm PETSC_COMM_SELF
!
!     Fortran Null
!
      PetscChar(80)       PETSC_NULL_CHARACTER
      PetscInt            PETSC_NULL_INTEGER
      PetscFortranDouble  PETSC_NULL_DOUBLE
      PetscObject         PETSC_NULL_OBJECT
!
!      A PETSC_NULL_FUNCTION pointer
!
      external PETSC_NULL_FUNCTION
      PetscScalar   PETSC_NULL_SCALAR
      PetscReal     PETSC_NULL_REAL
      PetscBool     PETSC_NULL_BOOL
!
#if defined(PETSC_USE_REAL___FLOAT128)
      integer MPIU_REAL
      integer MPIU_SCALAR
      integer MPIU_SUM
#endif
!
!
!
!     Basic math constants
!
      PetscReal PETSC_PI
      PetscReal PETSC_MAX_REAL
      PetscReal PETSC_MIN_REAL
      PetscReal PETSC_MACHINE_EPSILON
      PetscReal PETSC_SQRT_MACHINE_EPSILON
      PetscReal PETSC_SMALL
      PetscReal PETSC_INFINITY
      PetscReal PETSC_NINFINITY

!
!     Common Block to store some of the PETSc constants.
!     which can be set - only at runtime.
!
      common /petscfortran1/ PETSC_NULL_CHARACTER
      common /petscfortran2/ PETSC_NULL_INTEGER
      common /petscfortran4/ PETSC_NULL_SCALAR
      common /petscfortran5/ PETSC_NULL_DOUBLE
      common /petscfortran6/ PETSC_NULL_REAL
      common /petscfortran7/ PETSC_NULL_BOOL
      common /petscfortran8/ PETSC_NULL_OBJECT
      common /petscfortran9/ PETSC_COMM_WORLD
      common /petscfortran10/ PETSC_COMM_SELF
#if defined(PETSC_USE_REAL___FLOAT128)
      common /petscfortran11/ MPIU_REAL
      common /petscfortran12/ MPIU_SCALAR
      common /petscfortran13/ MPIU_SUM
#endif
      common /petscfortran14/ PETSC_PI
      common /petscfortran15/ PETSC_MAX_REAL
      common /petscfortran16/ PETSC_MIN_REAL
      common /petscfortran17/ PETSC_MACHINE_EPSILON
      common /petscfortran18/ PETSC_SQRT_MACHINE_EPSILON
      common /petscfortran19/ PETSC_SMALL
      common /petscfortran20/ PETSC_INFINITY
      common /petscfortran21/ PETSC_NINFINITY

!
!     Possible arguments to PetscPushErrorHandler()
!
      external PETSCTRACEBACKERRORHANDLER
      external PETSCABORTERRORHANDLER
      external PETSCEMACSCLIENTERRORHANDLER
      external PETSCATTACHDEBUGGERERRORHANDLER
      external PETSCIGNOREERRORHANDLER
!
      external  PetscIsInfOrNanScalar
      external  PetscIsInfOrNanReal
      PetscBool PetscIsInfOrNanScalar
      PetscBool PetscIsInfOrNanReal


!    END COMMON-BLOCK VARIABLES
! ----------------------------------------------------------------------------
!
!
!     Random numbers
!
#define PETSCRAND 'rand'
#define PETSCRAND48 'rand48'
#define PETSCSPRNG 'sprng'
!
!
!
      PetscEnum PETSC_BINARY_INT_SIZE
      PetscEnum PETSC_BINARY_FLOAT_SIZE
      PetscEnum PETSC_BINARY_CHAR_SIZE
      PetscEnum PETSC_BINARY_SHORT_SIZE
      PetscEnum PETSC_BINARY_DOUBLE_SIZE
      PetscEnum PETSC_BINARY_SCALAR_SIZE

      parameter (PETSC_BINARY_INT_SIZE = 4)
      parameter (PETSC_BINARY_FLOAT_SIZE = 4)
      parameter (PETSC_BINARY_CHAR_SIZE = 1)
      parameter (PETSC_BINARY_SHORT_SIZE = 2)
      parameter (PETSC_BINARY_DOUBLE_SIZE = 8)
#if defined(PETSC_USE_COMPLEX)
      parameter (PETSC_BINARY_SCALAR_SIZE = 16)
#else
      parameter (PETSC_BINARY_SCALAR_SIZE = 8)
#endif

      PetscEnum PETSC_BINARY_SEEK_SET
      PetscEnum PETSC_BINARY_SEEK_CUR
      PetscEnum PETSC_BINARY_SEEK_END

      parameter (PETSC_BINARY_SEEK_SET = 0,PETSC_BINARY_SEEK_CUR = 1)
      parameter (PETSC_BINARY_SEEK_END = 2)

      PetscEnum PETSC_BUILDTWOSIDED_ALLREDUCE
      PetscEnum PETSC_BUILDTWOSIDED_IBARRIER
      PetscEnum PETSC_BUILDTWOSIDED_REDSCATTER
      parameter (PETSC_BUILDTWOSIDED_ALLREDUCE = 0)
      parameter (PETSC_BUILDTWOSIDED_IBARRIER = 1)
      parameter (PETSC_BUILDTWOSIDED_REDSCATTER = 2)
!
