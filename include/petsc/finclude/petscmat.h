!
!
!  Include file for Fortran use of the Mat package in PETSc
!
#include "petsc/finclude/petscmatdef.h"

!
!  Flag for matrix assembly
!
      PetscEnum MAT_FLUSH_ASSEMBLY
      PetscEnum MAT_FINAL_ASSEMBLY

      parameter(MAT_FLUSH_ASSEMBLY=1,MAT_FINAL_ASSEMBLY=0)
!
!
!
      PetscEnum MAT_FACTOR_NONE
      PetscEnum MAT_FACTOR_LU
      PetscEnum MAT_FACTOR_CHOLESKY
      PetscEnum MAT_FACTOR_ILU
      PetscEnum MAT_FACTOR_ICC

      parameter(MAT_FACTOR_NONE=0,MAT_FACTOR_LU=1)
      parameter(MAT_FACTOR_CHOLESKY=2,MAT_FACTOR_ILU=3)
      parameter(MAT_FACTOR_ICC=4)

! MatGetSubMatrixOption
      PetscEnum MAT_DO_NOT_GET_VALUES
      PetscEnum MAT_GET_VALUES
      parameter(MAT_DO_NOT_GET_VALUES=0,MAT_GET_VALUES=1)

!
!  Matrix options; must match those in include/petscmat.h
!
      PetscEnum MAT_OPTION_MIN
      PetscEnum MAT_ROW_ORIENTED
      PetscEnum MAT_NEW_NONZERO_LOCATIONS
      PetscEnum MAT_SYMMETRIC
      PetscEnum MAT_STRUCTURALLY_SYMMETRIC
      PetscEnum MAT_NEW_DIAGONALS
      PetscEnum MAT_IGNORE_OFF_PROC_ENTRIES
      PetscEnum MAT_NEW_NONZERO_LOCATION_ERR
      PetscEnum MAT_NEW_NONZERO_ALLOCATION_ERR
      PetscEnum MAT_USE_HASH_TABLE
      PetscEnum MAT_KEEP_NONZERO_PATTERN
      PetscEnum MAT_IGNORE_ZERO_ENTRIES
      PetscEnum MAT_USE_INODES
      PetscEnum MAT_HERMITIAN
      PetscEnum MAT_SYMMETRY_ETERNAL
      PetscEnum MAT_DUMMY
      PetscEnum MAT_IGNORE_LOWER_TRIANGULAR
      PetscEnum MAT_ERROR_LOWER_TRIANGULAR
      PetscEnum MAT_GETROW_UPPERTRIANGULAR
      PetscEnum MAT_UNUSED_NONZERO_LOCATION_ERR
      PetscEnum MAT_SPD
      PetscEnum MAT_NO_OFF_PROC_ENTRIES
      PetscEnum MAT_SUBSET_OFF_PROC_ENTRIES
      PetscEnum MAT_NO_OFF_PROC_ZERO_ROWS
      PetscEnum MAT_DIAGBLOCK_CSR
      PetscEnum MAT_OFFDIAGBLOCK_CSR
      PetscEnum MAT_CSR
      PetscEnum MAT_DIAGBLOCK_DIA
      PetscEnum MAT_OFFDIAGBLOCK_DIA
      PetscEnum MAT_DIA
      PetscEnum MAT_DIAGBLOCK_ELL
      PetscEnum MAT_OFFDIAGBLOCK_ELL
      PetscEnum MAT_ELL
      PetscEnum MAT_DIAGBLOCK_HYB
      PetscEnum MAT_OFFDIAGBLOCK_HYB
      PetscEnum MAT_HYB
      PetscEnum MAT_OPTION_MAX

      parameter (MAT_OPTION_MIN=-3)
      parameter (MAT_UNUSED_NONZERO_LOCATION_ERR=-2)
      parameter (MAT_ROW_ORIENTED=-1)
      parameter (MAT_SYMMETRIC=1)
      parameter (MAT_STRUCTURALLY_SYMMETRIC=2)
      parameter (MAT_NEW_DIAGONALS=3)
      parameter (MAT_IGNORE_OFF_PROC_ENTRIES=4)
      parameter (MAT_USE_HASH_TABLE=5)
      parameter (MAT_KEEP_NONZERO_PATTERN=6)
      parameter (MAT_IGNORE_ZERO_ENTRIES=7)
      parameter (MAT_USE_INODES=8)
      parameter (MAT_HERMITIAN=9)
      parameter (MAT_SYMMETRY_ETERNAL=10)
      parameter (MAT_NEW_NONZERO_LOCATION_ERR=11)
      parameter (MAT_IGNORE_LOWER_TRIANGULAR=12)
      parameter (MAT_ERROR_LOWER_TRIANGULAR=13)
      parameter (MAT_GETROW_UPPERTRIANGULAR=14)
      parameter (MAT_SPD=15)
      parameter (MAT_NO_OFF_PROC_ZERO_ROWS=16)
      parameter (MAT_NO_OFF_PROC_ENTRIES=17)
      parameter (MAT_NEW_NONZERO_LOCATIONS=18)
      parameter (MAT_NEW_NONZERO_ALLOCATION_ERR=19)
      parameter (MAT_SUBSET_OFF_PROC_ENTRIES=20)
      parameter (MAT_OPTION_MAX=21)
!
!  MatFactorShiftType
!
      PetscEnum MAT_SHIFT_NONE
      PetscEnum MAT_SHIFT_NONZERO
      PetscEnum MAT_SHIFT_POSITIVE_DEFINITE
      PetscEnum MAT_SHIFT_INBLOCKS
      parameter (MAT_SHIFT_NONE=0)
      parameter (MAT_SHIFT_NONZERO=1)
      parameter (MAT_SHIFT_POSITIVE_DEFINITE=2)
      parameter (MAT_SHIFT_INBLOCKS=3)

!
!  MatDuplicateOption
!
      PetscEnum MAT_DO_NOT_COPY_VALUES
      PetscEnum MAT_COPY_VALUES
      PetscEnum MAT_SHARE_NONZERO_PATTERN
      parameter (MAT_DO_NOT_COPY_VALUES=0,MAT_COPY_VALUES=1)
      parameter (MAT_SHARE_NONZERO_PATTERN=2)
!
!  Flags for MatCopy, MatAXPY
!
      PetscEnum DIFFERENT_NONZERO_PATTERN
      PetscEnum SUBSET_NONZERO_PATTERN
      PetscEnum SAME_NONZERO_PATTERN

      parameter (DIFFERENT_NONZERO_PATTERN = 0,SUBSET_NONZERO_PATTERN=1)
      parameter (SAME_NONZERO_PATTERN = 2)

#if !(PETSC_USE_FORTRAN_DATATYPES_)
#include "petsc/finclude/petscmatinfosize.h"
#endif

      PetscEnum MAT_INFO_BLOCK_SIZE
      PetscEnum MAT_INFO_NZ_ALLOCATED
      PetscEnum MAT_INFO_NZ_USED
      PetscEnum MAT_INFO_NZ_UNNEEDED
      PetscEnum MAT_INFO_MEMORY
      PetscEnum MAT_INFO_ASSEMBLIES
      PetscEnum MAT_INFO_MALLOCS
      PetscEnum MAT_INFO_FILL_RATIO_GIVEN
      PetscEnum MAT_INFO_FILL_RATIO_NEEDED
      PetscEnum MAT_INFO_FACTOR_MALLOCS

      parameter (MAT_INFO_BLOCK_SIZE=1,MAT_INFO_NZ_ALLOCATED=2)
      parameter (MAT_INFO_NZ_USED=3,MAT_INFO_NZ_UNNEEDED=4)
      parameter (MAT_INFO_MEMORY=5,MAT_INFO_ASSEMBLIES=6)
      parameter (MAT_INFO_MALLOCS=7,MAT_INFO_FILL_RATIO_GIVEN=8)
      parameter (MAT_INFO_FILL_RATIO_NEEDED=9)
      parameter (MAT_INFO_FACTOR_MALLOCS=10)
!
!  MatReuse
!
      PetscEnum MAT_INITIAL_MATRIX
      PetscEnum MAT_REUSE_MATRIX
      PetscEnum MAT_IGNORE_MATRIX
      PetscEnum MAT_INPLACE_MATRIX

      parameter (MAT_INITIAL_MATRIX=0)
      parameter (MAT_REUSE_MATRIX=1)
      parameter (MAT_IGNORE_MATRIX=2)
      parameter (MAT_INPLACE_MATRIX=3)

!
!  MatInfoType
!
      PetscEnum MAT_LOCAL
      PetscEnum MAT_GLOBAL_MAX
      PetscEnum MAT_GLOBAL_SUM

      parameter (MAT_LOCAL=1,MAT_GLOBAL_MAX=2,MAT_GLOBAL_SUM=3)

!
!  MatCompositeType
!
      PetscEnum MAT_COMPOSITE_ADDITIVE
      PetscEnum MAT_COMPOSITE_MULTIPLICATIVE

      parameter (MAT_COMPOSITE_ADDITIVE = 0)
      parameter (MAT_COMPOSITE_MULTIPLICATIVE = 1)
#if !(PETSC_USE_FORTRAN_DATATYPES_)
#include "petsc/finclude/petscmatfactorinfosize.h"
#endif
      PetscEnum MAT_FACTORINFO_DIAGONAL_FILL
      PetscEnum MAT_FACTORINFO_USEDT
      PetscEnum MAT_FACTORINFO_DT
      PetscEnum MAT_FACTORINFO_DTCOL
      PetscEnum MAT_FACTORINFO_DTCOUNT
      PetscEnum MAT_FACTORINFO_FILL
      PetscEnum MAT_FACTORINFO_LEVELS
      PetscEnum MAT_FACTORINFO_PIVOT_IN_BLOCKS
      PetscEnum MAT_FACTORINFO_ZERO_PIVOT
      PetscEnum MAT_FACTORINFO_SHIFT_TYPE
      PetscEnum MAT_FACTORINFO_SHIFT_AMOUNT

      parameter (MAT_FACTORINFO_DIAGONAL_FILL = 1)
      parameter (MAT_FACTORINFO_USEDT = 2)
      parameter (MAT_FACTORINFO_DT = 3)
      parameter (MAT_FACTORINFO_DTCOL = 4)
      parameter (MAT_FACTORINFO_DTCOUNT = 5)
      parameter (MAT_FACTORINFO_FILL = 6)
      parameter (MAT_FACTORINFO_LEVELS = 7)
      parameter (MAT_FACTORINFO_PIVOT_IN_BLOCKS = 8)
      parameter (MAT_FACTORINFO_ZERO_PIVOT = 9)
      parameter (MAT_FACTORINFO_SHIFT_TYPE = 10)
      parameter (MAT_FACTORINFO_SHIFT_AMOUNT = 11)


!
!  Options for SOR and SSOR
!  MatSorType may be bitwise ORd together, so do not change the numbers
!
      PetscEnum SOR_FORWARD_SWEEP
      PetscEnum SOR_BACKWARD_SWEEP
      PetscEnum SOR_SYMMETRIC_SWEEP
      PetscEnum SOR_LOCAL_FORWARD_SWEEP
      PetscEnum SOR_LOCAL_BACKWARD_SWEEP
      PetscEnum SOR_LOCAL_SYMMETRIC_SWEEP
      PetscEnum SOR_ZERO_INITIAL_GUESS
      PetscEnum SOR_EISENSTAT
      PetscEnum SOR_APPLY_UPPER
      PetscEnum SOR_APPLY_LOWER

      parameter (SOR_FORWARD_SWEEP=1,SOR_BACKWARD_SWEEP=2)
      parameter (SOR_SYMMETRIC_SWEEP=3,SOR_LOCAL_FORWARD_SWEEP=4)
      parameter (SOR_LOCAL_BACKWARD_SWEEP=8)
      parameter (SOR_LOCAL_SYMMETRIC_SWEEP=12)
      parameter (SOR_ZERO_INITIAL_GUESS=16,SOR_EISENSTAT=32)
      parameter (SOR_APPLY_UPPER=64,SOR_APPLY_LOWER=128)
!
!  MatOperation
!
      PetscEnum MATOP_SET_VALUES
      PetscEnum MATOP_GET_ROW
      PetscEnum MATOP_RESTORE_ROW
      PetscEnum MATOP_MULT
      PetscEnum MATOP_MULT_ADD
      PetscEnum MATOP_MULT_TRANSPOSE
      PetscEnum MATOP_MULT_TRANSPOSE_ADD
      PetscEnum MATOP_SOLVE
      PetscEnum MATOP_SOLVE_ADD
      PetscEnum MATOP_SOLVE_TRANSPOSE
      PetscEnum MATOP_SOLVE_TRANSPOSE_ADD
      PetscEnum MATOP_LUFACTOR
      PetscEnum MATOP_CHOLESKYFACTOR
      PetscEnum MATOP_SOR
      PetscEnum MATOP_TRANSPOSE
      PetscEnum MATOP_GETINFO
      PetscEnum MATOP_EQUAL
      PetscEnum MATOP_GET_DIAGONAL
      PetscEnum MATOP_DIAGONAL_SCALE
      PetscEnum MATOP_NORM
      PetscEnum MATOP_ASSEMBLY_BEGIN
      PetscEnum MATOP_ASSEMBLY_END
      PetscEnum MATOP_SET_OPTION
      PetscEnum MATOP_ZERO_ENTRIES
      PetscEnum MATOP_ZERO_ROWS
      PetscEnum MATOP_LUFACTOR_SYMBOLIC
      PetscEnum MATOP_LUFACTOR_NUMERIC
      PetscEnum MATOP_CHOLESKY_FACTOR_SYMBOLIC
      PetscEnum MATOP_CHOLESKY_FACTOR_NUMERIC
      PetscEnum MATOP_SETUP_PREALLOCATION
      PetscEnum MATOP_ILUFACTOR_SYMBOLIC
      PetscEnum MATOP_ICCFACTOR_SYMBOLIC
!     PetscEnum MATOP_PLACEHOLDER_32
!     PetscEnum MATOP_PLACEHOLDER_33
      PetscEnum MATOP_DUPLICATE
      PetscEnum MATOP_FORWARD_SOLVE
      PetscEnum MATOP_BACKWARD_SOLVE
      PetscEnum MATOP_ILUFACTOR
      PetscEnum MATOP_ICCFACTOR
      PetscEnum MATOP_AXPY
      PetscEnum MATOP_GET_SUBMATRICES
      PetscEnum MATOP_INCREASE_OVERLAP
      PetscEnum MATOP_GET_VALUES
      PetscEnum MATOP_COPY
      PetscEnum MATOP_GET_ROW_MAX
      PetscEnum MATOP_SCALE
      PetscEnum MATOP_SHIFT
      PetscEnum MATOP_DIAGONAL_SET
      PetscEnum MATOP_ZERO_ROWS_COLUMNS
      PetscEnum MATOP_SET_RANDOM
      PetscEnum MATOP_GET_ROW_IJ
      PetscEnum MATOP_RESTORE_ROW_IJ
      PetscEnum MATOP_GET_COLUMN_IJ
      PetscEnum MATOP_RESTORE_COLUMN_IJ
      PetscEnum MATOP_FDCOLORING_CREATE
      PetscEnum MATOP_COLORING_PATCH
      PetscEnum MATOP_SET_UNFACTORED
      PetscEnum MATOP_PERMUTE
      PetscEnum MATOP_SET_VALUES_BLOCKED
      PetscEnum MATOP_GET_SUBMATRIX
      PetscEnum MATOP_DESTROY
      PetscEnum MATOP_VIEW
      PetscEnum MATOP_CONVERT_FROM
      PetscEnum MATOP_MATMAT_MULT
      PetscEnum MATOP_MATMAT_MULT_SYMBOLIC
      PetscEnum MATOP_MATMAT_MULT_NUMERIC
      PetscEnum MATOP_SET_LOCAL_TO_GLOBAL_MAP
      PetscEnum MATOP_SET_VALUES_LOCAL
      PetscEnum MATOP_ZERO_ROWS_LOCAL
      PetscEnum MATOP_GET_ROW_MAX_ABS
      PetscEnum MATOP_GET_ROW_MIN_ABS
      PetscEnum MATOP_CONVERT
      PetscEnum MATOP_SET_COLORING
!     PetscEnum MATOP_PLACEHOLDER_73
      PetscEnum MATOP_SET_VALUES_ADIFOR
      PetscEnum MATOP_FD_COLORING_APPLY
      PetscEnum MATOP_SET_FROM_OPTIONS
      PetscEnum MATOP_MULT_CONSTRAINED
      PetscEnum MATOP_MULT_TRANSPOSE_CONSTRAIN
      PetscEnum MATOP_FIND_ZERO_DIAGONALS
      PetscEnum MATOP_MULT_MULTIPLE
      PetscEnum MATOP_SOLVE_MULTIPLE
      PetscEnum MATOP_GET_INERTIA
      PetscEnum MATOP_LOAD
      PetscEnum MATOP_IS_SYMMETRIC
      PetscEnum MATOP_IS_HERMITIAN
      PetscEnum MATOP_IS_STRUCTURALLY_SYMMETRIC
      PetscEnum MATOP_SET_VALUES_BLOCKEDLOCAL
      PetscEnum MATOP_GET_VECS
      PetscEnum MATOP_MAT_MULT
      PetscEnum MATOP_MAT_MULT_SYMBOLIC
      PetscEnum MATOP_MAT_MULT_NUMERIC
      PetscEnum MATOP_PTAP
      PetscEnum MATOP_PTAP_SYMBOLIC
      PetscEnum MATOP_PTAP_NUMERIC
      PetscEnum MATOP_MAT_TRANSPOSE_MULT
      PetscEnum MATOP_MAT_TRANSPOSE_MULT_SYMBO
      PetscEnum MATOP_MAT_TRANSPOSE_MULT_NUMER
!     PetscEnum MATOP_PLACEHOLDER_98
!     PetscEnum MATOP_PLACEHOLDER_99
!     PetscEnum MATOP_PLACEHOLDER_100
!     PetscEnum MATOP_PLACEHOLDER_101
      PetscEnum MATOP_CONJUGATE
!     PetscEnum MATOP_PLACEHOLDER_103
      PetscEnum MATOP_SET_VALUES_ROW
      PetscEnum MATOP_REAL_PART
      PetscEnum MATOP_IMAGINARY_PART
      PetscEnum MATOP_GET_ROW_UPPER_TRIANGULAR
      PetscEnum MATOP_RESTORE_ROW_UPPER_TRIANG
      PetscEnum MATOP_MAT_SOLVE
      PetscEnum MATOP_GET_REDUNDANT_MATRIX
      PetscEnum MATOP_GET_ROW_MIN
      PetscEnum MATOP_GET_COLUMN_VECTOR
      PetscEnum MATOP_MISSING_DIAGONAL
      PetscEnum MATOP_GET_SEQ_NONZERO_STRUCTUR
      PetscEnum MATOP_CREATE
      PetscEnum MATOP_GET_GHOSTS
      PetscEnum MATOP_GET_LOCAL_SUB_MATRIX
      PetscEnum MATOP_RESTORE_LOCALSUB_MATRIX
      PetscEnum MATOP_MULT_DIAGONAL_BLOCK
      PetscEnum MATOP_HERMITIAN_TRANSPOSE
      PetscEnum MATOP_MULT_HERMITIAN_TRANSPOSE
      PetscEnum MATOP_MULT_HERMITIAN_TRANS_ADD
      PetscEnum MATOP_GET_MULTI_PROC_BLOCK
      PetscEnum MATOP_FIND_NONZERO_ROWS
      PetscEnum MATOP_GET_COLUMN_NORMS
      PetscEnum MATOP_INVERT_BLOCK_DIAGONAL
!     PetscEnum MATOP_PLACEHOLDER_127
      PetscEnum MATOP_GET_SUB_MATRICES_PARALLE
      PetscEnum MATOP_SET_VALUES_BATCH
      PetscEnum MATOP_TRANSPOSE_MAT_MULT
      PetscEnum MATOP_TRANSPOSE_MAT_MULT_SYMBO
      PetscEnum MATOP_TRANSPOSE_MAT_MULT_NUMER
      PetscEnum MATOP_TRANSPOSE_COLORING_CREAT
      PetscEnum MATOP_TRANS_COLORING_APPLY_SPT
      PetscEnum MATOP_TRANS_COLORING_APPLY_DEN
      PetscEnum MATOP_RART
      PetscEnum MATOP_RART_SYMBOLIC
      PetscEnum MATOP_RART_NUMERIC
      PetscEnum MATOP_SET_BLOCK_SIZES
      PetscEnum MATOP_AYPX
      PetscEnum MATOP_RESIDUAL
      PetscEnum MATOP_FDCOLORING_SETUP
      PetscEnum MATOP_MPICONCATENATESEQ

      parameter(MATOP_SET_VALUES=0)
      parameter(MATOP_GET_ROW=1)
      parameter(MATOP_RESTORE_ROW=2)
      parameter(MATOP_MULT=3)
      parameter(MATOP_MULT_ADD=4)
      parameter(MATOP_MULT_TRANSPOSE=5)
      parameter(MATOP_MULT_TRANSPOSE_ADD=6)
      parameter(MATOP_SOLVE=7)
      parameter(MATOP_SOLVE_ADD=8)
      parameter(MATOP_SOLVE_TRANSPOSE=9)
      parameter(MATOP_SOLVE_TRANSPOSE_ADD=10)
      parameter(MATOP_LUFACTOR=11)
      parameter(MATOP_CHOLESKYFACTOR=12)
      parameter(MATOP_SOR=13)
      parameter(MATOP_TRANSPOSE=14)
      parameter(MATOP_GETINFO=15)
      parameter(MATOP_EQUAL=16)
      parameter(MATOP_GET_DIAGONAL=17)
      parameter(MATOP_DIAGONAL_SCALE=18)
      parameter(MATOP_NORM=19)
      parameter(MATOP_ASSEMBLY_BEGIN=20)
      parameter(MATOP_ASSEMBLY_END=21)
      parameter(MATOP_SET_OPTION=22)
      parameter(MATOP_ZERO_ENTRIES=23)
      parameter(MATOP_ZERO_ROWS=24)
      parameter(MATOP_LUFACTOR_SYMBOLIC=25)
      parameter(MATOP_LUFACTOR_NUMERIC=26)
      parameter(MATOP_CHOLESKY_FACTOR_SYMBOLIC=27)
      parameter(MATOP_CHOLESKY_FACTOR_NUMERIC=28)
      parameter(MATOP_SETUP_PREALLOCATION=29)
      parameter(MATOP_ILUFACTOR_SYMBOLIC=30)
      parameter(MATOP_ICCFACTOR_SYMBOLIC=31)
!     parameter(MATOP_PLACEHOLDER_32=32)
!     parameter(MATOP_PLACEHOLDER_33=33)
      parameter(MATOP_DUPLICATE=34)
      parameter(MATOP_FORWARD_SOLVE=35)
      parameter(MATOP_BACKWARD_SOLVE=36)
      parameter(MATOP_ILUFACTOR=37)
      parameter(MATOP_ICCFACTOR=38)
      parameter(MATOP_AXPY=39)
      parameter(MATOP_GET_SUBMATRICES=40)
      parameter(MATOP_INCREASE_OVERLAP=41)
      parameter(MATOP_GET_VALUES=42)
      parameter(MATOP_COPY=43)
      parameter(MATOP_GET_ROW_MAX=44)
      parameter(MATOP_SCALE=45)
      parameter(MATOP_SHIFT=46)
      parameter(MATOP_DIAGONAL_SET=47)
      parameter(MATOP_ZERO_ROWS_COLUMNS=48)
      parameter(MATOP_SET_RANDOM=49)
      parameter(MATOP_GET_ROW_IJ=50)
      parameter(MATOP_RESTORE_ROW_IJ=51)
      parameter(MATOP_GET_COLUMN_IJ=52)
      parameter(MATOP_RESTORE_COLUMN_IJ=53)
      parameter(MATOP_FDCOLORING_CREATE=54)
      parameter(MATOP_COLORING_PATCH=55)
      parameter(MATOP_SET_UNFACTORED=56)
      parameter(MATOP_PERMUTE=57)
      parameter(MATOP_SET_VALUES_BLOCKED=58)
      parameter(MATOP_GET_SUBMATRIX=59)
      parameter(MATOP_DESTROY=60)
      parameter(MATOP_VIEW=61)
      parameter(MATOP_CONVERT_FROM=62)
      parameter(MATOP_MATMAT_MULT=63)
      parameter(MATOP_MATMAT_MULT_SYMBOLIC=64)
      parameter(MATOP_MATMAT_MULT_NUMERIC=65)
      parameter(MATOP_SET_LOCAL_TO_GLOBAL_MAP=66)
      parameter(MATOP_SET_VALUES_LOCAL=67)
      parameter(MATOP_ZERO_ROWS_LOCAL=68)
      parameter(MATOP_GET_ROW_MAX_ABS=69)
      parameter(MATOP_GET_ROW_MIN_ABS=70)
      parameter(MATOP_CONVERT=71)
      parameter(MATOP_SET_COLORING=72)
!     parameter(MATOP_PLACEHOLDER_73=73)
      parameter(MATOP_SET_VALUES_ADIFOR=74)
      parameter(MATOP_FD_COLORING_APPLY=75)
      parameter(MATOP_SET_FROM_OPTIONS=76)
      parameter(MATOP_MULT_CONSTRAINED=77)
      parameter(MATOP_MULT_TRANSPOSE_CONSTRAIN=78)
      parameter(MATOP_FIND_ZERO_DIAGONALS=79)
      parameter(MATOP_MULT_MULTIPLE=80)
      parameter(MATOP_SOLVE_MULTIPLE=81)
      parameter(MATOP_GET_INERTIA=82)
      parameter(MATOP_LOAD=83)
      parameter(MATOP_IS_SYMMETRIC=84)
      parameter(MATOP_IS_HERMITIAN=85)
      parameter(MATOP_IS_STRUCTURALLY_SYMMETRIC=86)
      parameter(MATOP_SET_VALUES_BLOCKEDLOCAL=87)
      parameter(MATOP_GET_VECS=88)
      parameter(MATOP_MAT_MULT=89)
      parameter(MATOP_MAT_MULT_SYMBOLIC=90)
      parameter(MATOP_MAT_MULT_NUMERIC=91)
      parameter(MATOP_PTAP=92)
      parameter(MATOP_PTAP_SYMBOLIC=93)
      parameter(MATOP_PTAP_NUMERIC=94)
      parameter(MATOP_MAT_TRANSPOSE_MULT=95)
      parameter(MATOP_MAT_TRANSPOSE_MULT_SYMBO=96)
      parameter(MATOP_MAT_TRANSPOSE_MULT_NUMER=97)
!     parameter(MATOP_PLACEHOLDER_98=98)
!     parameter(MATOP_PLACEHOLDER_99=99)
!     parameter(MATOP_PLACEHOLDER_100=100)
!     parameter(MATOP_PLACEHOLDER_101=101)
      parameter(MATOP_CONJUGATE=102)
!     parameter(MATOP_PLACEHOLDER_103=103)
      parameter(MATOP_SET_VALUES_ROW=104)
      parameter(MATOP_REAL_PART=105)
      parameter(MATOP_IMAGINARY_PART=106)
      parameter(MATOP_GET_ROW_UPPER_TRIANGULAR=107)
      parameter(MATOP_RESTORE_ROW_UPPER_TRIANG=108)
      parameter(MATOP_MAT_SOLVE=109)
      parameter(MATOP_GET_REDUNDANT_MATRIX=110)
      parameter(MATOP_GET_ROW_MIN=111)
      parameter(MATOP_GET_COLUMN_VECTOR=112)
      parameter(MATOP_MISSING_DIAGONAL=113)
      parameter(MATOP_GET_SEQ_NONZERO_STRUCTUR=114)
      parameter(MATOP_CREATE=115)
      parameter(MATOP_GET_GHOSTS=116)
      parameter(MATOP_GET_LOCAL_SUB_MATRIX=117)
      parameter(MATOP_RESTORE_LOCALSUB_MATRIX=118)
      parameter(MATOP_MULT_DIAGONAL_BLOCK=119)
      parameter(MATOP_HERMITIAN_TRANSPOSE=120)
      parameter(MATOP_MULT_HERMITIAN_TRANSPOSE=121)
      parameter(MATOP_MULT_HERMITIAN_TRANS_ADD=122)
      parameter(MATOP_GET_MULTI_PROC_BLOCK=123)
      parameter(MATOP_FIND_NONZERO_ROWS=124)
      parameter(MATOP_GET_COLUMN_NORMS=125)
      parameter(MATOP_INVERT_BLOCK_DIAGONAL=126)
!     parameter(MATOP_PLACEHOLDER_127=127)
      parameter(MATOP_GET_SUB_MATRICES_PARALLE=128)
      parameter(MATOP_SET_VALUES_BATCH=129)
      parameter(MATOP_TRANSPOSE_MAT_MULT=130)
      parameter(MATOP_TRANSPOSE_MAT_MULT_SYMBO=131)
      parameter(MATOP_TRANSPOSE_MAT_MULT_NUMER=132)
      parameter(MATOP_TRANSPOSE_COLORING_CREAT=133)
      parameter(MATOP_TRANS_COLORING_APPLY_SPT=134)
      parameter(MATOP_TRANS_COLORING_APPLY_DEN=135)
      parameter(MATOP_RART=136)
      parameter(MATOP_RART_SYMBOLIC=137)
      parameter(MATOP_RART_NUMERIC=138)
      parameter(MATOP_SET_BLOCK_SIZES=139)
      parameter(MATOP_AYPX=140)
      parameter(MATOP_RESIDUAL=141)
      parameter(MATOP_FDCOLORING_SETUP=142)
      parameter(MATOP_MPICONCATENATESEQ=144)
!
!
!
      PetscEnum MATRIX_BINARY_FORMAT_DENSE
      parameter (MATRIX_BINARY_FORMAT_DENSE=-1)
!
! MPChacoGlobalType
      PetscEnum MP_CHACO_MULTILEVEL_KL
      PetscEnum MP_CHACO_SPECTRAL
      PetscEnum MP_CHACO_LINEAR
      PetscEnum MP_CHACO_RANDOM
      PetscEnum MP_CHACO_SCATTERED
      parameter (MP_CHACO_MULTILEVEL_KL=0,MP_CHACO_SPECTRAL=1)
      parameter (MP_CHACO_LINEAR=2,MP_CHACO_RANDOM=3)
      parameter (MP_CHACO_SCATTERED=4)
!
! MPChacoLocalType
      PetscEnum MP_CHACO_KERNIGHAN_LIN
      PetscEnum MP_CHACO_NONE
      parameter (MP_CHACO_KERNIGHAN_LIN=0, MP_CHACO_NONE=1)
!
! MPChacoEigenType
      PetscEnum MP_CHACO_LANCZOS
      PetscEnum MP_CHACO_RQI_SYMMLQ
      parameter (MP_CHACO_LANCZOS=0, MP_CHACO_RQI_SYMMLQ=1)
!
! MPPTScotchStrategyType
      PetscEnum MP_PTSCOTCH_QUALITY
      PetscEnum MP_PTSCOTCH_SPEED
      PetscEnum MP_PTSCOTCH_BALANCE
      PetscEnum MP_PTSCOTCH_SAFETY
      PetscEnum MP_PTSCOTCH_SCALABILITY
      parameter (MP_PTSCOTCH_QUALITY = 0)
      parameter (MP_PTSCOTCH_SPEED = 1)
      parameter (MP_PTSCOTCH_BALANCE = 2)
      parameter (MP_PTSCOTCH_SAFETY = 3)
      parameter (MP_PTSCOTCH_SCALABILITY = 4)

! PetscScalarPrecision
      PetscEnum PETSC_SCALAR_DOUBLE
      PetscEnum PETSC_SCALAR_SINGLE
      PetscEnum PETSC_SCALAR_LONG_DOUBLE
      parameter (PETSC_SCALAR_DOUBLE=0,PETSC_SCALAR_SINGLE=1)
      parameter (PETSC_SCALAR_LONG_DOUBLE=2)


!
!     CUSP enumerated types
!
#if defined(PETSC_HAVE_CUSP)
      PetscEnum MAT_CUSP_CSR
      PetscEnum MAT_CUSP_DIA
      PetscEnum MAT_CUSP_ELL
      parameter(MAT_CUSP_CSR=0,MAT_CUSP_DIA=1)
      parameter(MAT_CUSP_ELL=2)
      PetscEnum MAT_CUSP_MULT_DIAG
      PetscEnum MAT_CUSP_MULT_OFFDIAG
      PetscEnum MAT_CUSP_MULT
      PetscEnum MAT_CUSP_ALL
      parameter(MAT_CUSP_MULT_DIAG=0)
      parameter(MAT_CUSP_MULT_OFFDIAG=1)
      parameter(MAT_CUSP_MULT=2)
      parameter(MAT_CUSP_ALL=3)
#endif


!
!     CUSPARSE enumerated types
!
#if defined(PETSC_HAVE_CUDA)
      PetscEnum MAT_CUSPARSE_CSR
      PetscEnum MAT_CUSPARSE_ELL
      PetscEnum MAT_CUSPARSE_HYB
      parameter(MAT_CUSPARSE_CSR=0,MAT_CUSPARSE_ELL=1)
      parameter(MAT_CUSPARSE_HYB=2)
      PetscEnum MAT_CUSPARSE_MULT_DIAG
      PetscEnum MAT_CUSPARSE_MULT_OFFDIAG
      PetscEnum MAT_CUSPARSE_MULT
      PetscEnum MAT_CUSPARSE_ALL
      parameter(MAT_CUSPARSE_MULT_DIAG=0)
      parameter(MAT_CUSPARSE_MULT_OFFDIAG=1)
      parameter(MAT_CUSPARSE_MULT=2)
      parameter(MAT_CUSPARSE_ALL=3)
#endif
!
!  End of Fortran include file for the Mat package in PETSc
!
