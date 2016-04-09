!
!  Include file for Fortran use of the TS (timestepping) package in PETSc
!
#if !defined (__PETSCTSDEF_H)
#define __PETSCTSDEF_H

#include "petsc/finclude/petscsnesdef.h"

#if !defined(PETSC_USE_FORTRAN_DATATYPES)
#define TS PetscFortranAddr
#define TSAdapt PetscFortranAddr
#define TSTrajectory PetscFortranAddr
#endif
#define TSType character*(80)
#define TSAdaptType character*(80)
#define TSTrajectoryType character*(80)
#define TSEquationType PetscEnum
#define TSConvergedReason PetscEnum
#define TSExactFinalTimeOption PetscEnum
#define TSSundialsType PetscEnum
#define TSProblemType PetscEnum
#define TSSundialsGramSchmidtType PetscEnum
#define TSSundialsLmmType PetscEnum

#define TSEULER           'euler'
#define TSBEULER          'beuler'
#define TSPSEUDO          'pseudo'
#define TSCN              'cn'
#define TSSUNDIALS        'sundials'
#define TSRK              'rk'
#define TSPYTHON          'python'
#define TSTHETA           'theta'
#define TSALPHA           'alpha'
#define TSGL              'gl'
#define TSSSP             'ssp'
#define TSARKIMEX         'arkimex'
#define TSROSW            'rosw'
#define TSEIMEX           'eimex'

#define TSTRAJECTORYBASIC 'basic'

#define TSSSPType character*(80)
#define TSSSPRKS2  'rks2'
#define TSSSPRKS3  'rks3'
#define TSSSPRK104 'rk104'

#define TSGLAdaptType character*(80)
#define TSGLADAPT_NONE 'none'
#define TSGLADAPT_SIZE 'size'
#define TSGLADAPT_BOTH 'both'

#define TSAdaptType character*(80)
#define TSADAPTBASIC 'basic'
#define TSADAPTNONE  'none'
#define TSADAPTCFL   'cfl'

#define TSRKType character*(80)
#define TSRK1FE   '1fe'
#define TSRK2A    '2a'
#define TSRK3     '3'
#define TSRK3BS   '3bs'
#define TSRK4     '4'
#define TSRK5F    '5f'
#define TSRK5DP   '5dp'

#define TSARKIMEXType character*(80)
#define TSARKIMEX1BEE   '1bee'
#define TSARKIMEXA2     'a2'
#define TSARKIMEXL2     'l2'
#define TSARKIMEXARS122 'ars122'
#define TSARKIMEX2C     '2c'
#define TSARKIMEX2D     '2d'
#define TSARKIMEX2E     '2e'
#define TSARKIMEXPRSSP2 'prssp2'
#define TSARKIMEX3      '3'
#define TSARKIMEXBPR3   'bpr3'
#define TSARKIMEXARS443 'ars443'
#define TSARKIMEX4      '4'
#define TSARKIMEX5      '5'

#define TSROSWType character*(80)
#define TSROSW2M          '2m'
#define TSROSW2P          '2p'
#define TSROSWRA3PW       'ra3pw'
#define TSROSWRA34PW2     'ra34pw2'
#define TSROSWRODAS3      'rodas3'
#define TSROSWSANDU3      'sandu3'
#define TSROSWASSP3P3S1C  'assp3p3s1c'
#define TSROSWLASSP3P4S2C 'lassp3p4s2c'
#define TSROSWLLSSP3P3S2C 'llssp3p3s2c'
#define TSROSWARK3        'ark3'
#define TSROSWTHETA1      'theta1'
#define TSROSWTHETA2      'theta2'
#define TSROSWGRK4T       'grk4t'
#define TSROSWSHAMP4      'shamp4'
#define TSROSWVELDD4      'veldd4'
#define TSROSW4L          '4l'

#define TSEIMEXType character*(80)
#define TSEIMEXS2     's2'
#define TSEIMEXS3     's3'
#define TSEIMEXS4     's4'

#endif
