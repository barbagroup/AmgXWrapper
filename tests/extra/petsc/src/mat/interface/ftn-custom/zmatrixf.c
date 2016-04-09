#include <petsc/private/fortranimpl.h>
#include <petscmat.h>
#include <petscviewer.h>

#if defined(PETSC_HAVE_FORTRAN_CAPS)
#define matgetrowmin_                    MATGETROWMIN
#define matgetrowminabs_                 MATGETROWMINABS
#define matgetrowmax_                    MATGETROWMAX
#define matgetrowmaxabs_                 MATGETROWMAXABS
#define matdestroymatrices_              MATDESTROYMATRICES
#define matgetfactor_                    MATGETFACTOR
#define matfactorgetsolverpackage_       MATFACTORGETSOLVERPACKAGE
#define matgetrowij_                     MATGETROWIJ
#define matrestorerowij_                 MATRESTOREROWIJ
#define matgetrow_                       MATGETROW
#define matrestorerow_                   MATRESTOREROW
#define matload_                         MATLOAD
#define matview_                         MATVIEW
#define matseqaijgetarray_               MATSEQAIJGETARRAY
#define matseqaijrestorearray_           MATSEQAIJRESTOREARRAY
#define matdensegetarray_                MATDENSEGETARRAY
#define matdenserestorearray_            MATDENSERESTOREARRAY
#define matconvert_                      MATCONVERT
#define matgetsubmatrices_               MATGETSUBMATRICES
#define matzerorowscolumns_              MATZEROROWSCOLUMNS
#define matzerorowscolumnsis_            MATZEROROWSCOLUMNSIS
#define matzerorowsstencil_              MATZEROROWSSTENCIL
#define matzerorowscolumnsstencil_       MATZEROROWSCOLUMNSSTENCIL
#define matzerorows_                     MATZEROROWS
#define matzerorowsis_                   MATZEROROWSIS
#define matzerorowslocal_                MATZEROROWSLOCAL
#define matzerorowslocalis_              MATZEROROWSLOCALIS
#define matzerorowscolumnslocal_         MATZEROROWSCOLUMNSLOCAL
#define matzerorowscolumnslocalis_       MATZEROROWSCOLUMNSLOCALIS
#define matsetoptionsprefix_             MATSETOPTIONSPREFIX
#define matcreatevecs_                   MATCREATEVECS
#define matnullspaceremove_              MATNULLSPACEREMOVE
#define matgetinfo_                      MATGETINFO
#define matlufactor_                     MATLUFACTOR
#define matilufactor_                    MATILUFACTOR
#define matlufactorsymbolic_             MATLUFACTORSYMBOLIC
#define matlufactornumeric_              MATLUFACTORNUMERIC
#define matcholeskyfactor_               MATCHOLESKYFACTOR
#define matcholeskyfactorsymbolic_       MATCHOLESKYFACTORSYMBOLIC
#define matcholeskyfactornumeric_        MATCHOLESKYFACTORNUMERIC
#define matilufactorsymbolic_            MATILUFACTORSYMBOLIC
#define maticcfactorsymbolic_            MATICCFACTORSYMBOLIC
#define maticcfactor_                    MATICCFACTOR
#define matfactorinfoinitialize_         MATFACTORINFOINITIALIZE
#define matnullspacesetfunction_         MATNULLSPACESETFUNCTION
#define matfindnonzerorows_              MATFINDNONZEROROWS
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE)
#define matgetrowmin_                    matgetrowmin
#define matgetrowminabs_                 matgetrowminabs
#define matgetrowmax_                    matgetrowmax
#define matgetrowmaxabs_                 matgetrowmaxabs
#define matdestroymatrices_              matdestroymatrices
#define matgetfactor_                    matgetfactor
#define matfactorgetsolverpackage_       matfactorgetsolverpackage
#define matcreatevecs_                   matcreatevecs
#define matgetrowij_                     matgetrowij
#define matrestorerowij_                 matrestorerowij
#define matgetrow_                       matgetrow
#define matrestorerow_                   matrestorerow
#define matview_                         matview
#define matload_                         matload
#define matseqaijgetarray_               matseqaijgetarray
#define matseqaijrestorearray_           matseqaijrestorearray
#define matdensegetarray_                matdensegetarray
#define matdenserestorearray_            matdenserestorearray
#define matconvert_                      matconvert
#define matgetsubmatrices_               matgetsubmatrices
#define matzerorowscolumns_              matzerorowscolumns
#define matzerorowscolumnsis_            matzerorowscolumnsis
#define matzerorowsstencil_              matzerorowsstencil
#define matzerorowscolumnsstencil_       matzerorowscolumnsstencil
#define matzerorows_                     matzerorows
#define matzerorowsis_                   matzerorowsis
#define matzerorowslocal_                matzerorowslocal
#define matzerorowslocalis_              matzerorowslocalis
#define matzerorowscolumnslocal_         matzerorowscolumnslocal
#define matzerorowscolumnslocalis_       matzerorowscolumnslocalis
#define matsetoptionsprefix_             matsetoptionsprefix
#define matnullspaceremove_              matnullspaceremove
#define matgetinfo_                      matgetinfo
#define matlufactor_                     matlufactor
#define matilufactor_                    matilufactor
#define matlufactorsymbolic_             matlufactorsymbolic
#define matlufactornumeric_              matlufactornumeric
#define matcholeskyfactor_               matcholeskyfactor
#define matcholeskyfactorsymbolic_       matcholeskyfactorsymbolic
#define matcholeskyfactornumeric_        matcholeskyfactornumeric
#define matilufactorsymbolic_            matilufactorsymbolic
#define maticcfactorsymbolic_            maticcfactorsymbolic
#define maticcfactor_                    maticcfactor
#define matfactorinfoinitialize_         matfactorinfoinitialize
#define matnullspacesetfunction_         matnullspacesetfunction
#define matfindnonzerorows_              matfindnonzerorows
#endif

PETSC_EXTERN void PETSC_STDCALL  matgetrowmin_(Mat *mat,Vec *v,PetscInt idx[], int *ierr )
{
  CHKFORTRANNULLINTEGER(idx);
  *ierr = MatGetRowMin(*mat,*v,idx);
}
PETSC_EXTERN void PETSC_STDCALL  matgetrowminabs_(Mat *mat,Vec *v,PetscInt idx[], int *ierr )
{
  CHKFORTRANNULLINTEGER(idx);
  *ierr = MatGetRowMinAbs(*mat,*v,idx);
}

PETSC_EXTERN void PETSC_STDCALL  matgetrowmax_(Mat *mat,Vec *v,PetscInt idx[], int *ierr )
{
  CHKFORTRANNULLINTEGER(idx);
  *ierr = MatGetRowMax(*mat,*v,idx);
}

PETSC_EXTERN void PETSC_STDCALL  matgetrowmaxabs_(Mat *mat,Vec *v,PetscInt idx[], int *ierr )
{
  CHKFORTRANNULLINTEGER(idx);
  *ierr = MatGetRowMaxAbs(*mat,*v,idx);
}

static PetscErrorCode ournullfunction(MatNullSpace sp,Vec x,void *ctx)
{
  PetscErrorCode ierr = 0;
  (*(void (PETSC_STDCALL *)(MatNullSpace*,Vec*,void*,PetscErrorCode*))(((PetscObject)sp)->fortran_func_pointers[0]))(&sp,&x,ctx,&ierr);CHKERRQ(ierr);
  return 0;
}

PETSC_EXTERN void PETSC_STDCALL matnullspacesetfunction_(MatNullSpace *sp, PetscErrorCode (*rem)(MatNullSpace,Vec,void*),void *ctx,PetscErrorCode *ierr)
{
  PetscObjectAllocateFortranPointers(*sp,1);
  ((PetscObject)*sp)->fortran_func_pointers[0] = (PetscVoidFunction)rem;

  *ierr = MatNullSpaceSetFunction(*sp,ournullfunction,ctx);
}

PETSC_EXTERN void PETSC_STDCALL matcreatevecs_(Mat *mat,Vec *right,Vec *left, int *ierr)
{
  CHKFORTRANNULLOBJECT(right);
  CHKFORTRANNULLOBJECT(left);
  *ierr = MatCreateVecs(*mat,right,left);
}

PETSC_EXTERN void PETSC_STDCALL matgetrowij_(Mat *B,PetscInt *shift,PetscBool *sym,PetscBool *blockcompressed,PetscInt *n,PetscInt *ia,size_t *iia,
                                PetscInt *ja,size_t *jja,PetscBool  *done,PetscErrorCode *ierr)
{
  const PetscInt *IA,*JA;
  *ierr = MatGetRowIJ(*B,*shift,*sym,*blockcompressed,n,&IA,&JA,done);if (*ierr) return;
  *iia  = PetscIntAddressToFortran(ia,(PetscInt*)IA);
  *jja  = PetscIntAddressToFortran(ja,(PetscInt*)JA);
}

PETSC_EXTERN void PETSC_STDCALL matrestorerowij_(Mat *B,PetscInt *shift,PetscBool *sym,PetscBool *blockcompressed, PetscInt *n,PetscInt *ia,size_t *iia,
                                    PetscInt *ja,size_t *jja,PetscBool  *done,PetscErrorCode *ierr)
{
  const PetscInt *IA = PetscIntAddressFromFortran(ia,*iia),*JA = PetscIntAddressFromFortran(ja,*jja);
  *ierr = MatRestoreRowIJ(*B,*shift,*sym,*blockcompressed,n,&IA,&JA,done);
}

/*
   This is a poor way of storing the column and value pointers
  generated by MatGetRow() to be returned with MatRestoreRow()
  but there is not natural,good place else to store them. Hence
  Fortran programmers can only have one outstanding MatGetRows()
  at a time.
*/
static PetscErrorCode    matgetrowactive = 0;
static const PetscInt    *my_ocols       = 0;
static const PetscScalar *my_ovals       = 0;

PETSC_EXTERN void PETSC_STDCALL matgetrow_(Mat *mat,PetscInt *row,PetscInt *ncols,PetscInt *cols,PetscScalar *vals,PetscErrorCode *ierr)
{
  const PetscInt    **oocols = &my_ocols;
  const PetscScalar **oovals = &my_ovals;

  if (matgetrowactive) {
    PetscError(PETSC_COMM_SELF,__LINE__,"MatGetRow_Fortran",__FILE__,PETSC_ERR_ARG_WRONGSTATE,PETSC_ERROR_INITIAL,
               "Cannot have two MatGetRow() active simultaneously\n\
               call MatRestoreRow() before calling MatGetRow() a second time");
    *ierr = 1;
    return;
  }

  CHKFORTRANNULLINTEGER(cols); if (!cols) oocols = NULL;
  CHKFORTRANNULLSCALAR(vals);  if (!vals) oovals = NULL;

  *ierr = MatGetRow(*mat,*row,ncols,oocols,oovals);
  if (*ierr) return;

  if (oocols) { *ierr = PetscMemcpy(cols,my_ocols,(*ncols)*sizeof(PetscInt)); if (*ierr) return;}
  if (oovals) { *ierr = PetscMemcpy(vals,my_ovals,(*ncols)*sizeof(PetscScalar)); if (*ierr) return;}
  matgetrowactive = 1;
}

PETSC_EXTERN void PETSC_STDCALL matrestorerow_(Mat *mat,PetscInt *row,PetscInt *ncols,PetscInt *cols,PetscScalar *vals,PetscErrorCode *ierr)
{
  const PetscInt    **oocols = &my_ocols;
  const PetscScalar **oovals = &my_ovals;
  if (!matgetrowactive) {
    PetscError(PETSC_COMM_SELF,__LINE__,"MatRestoreRow_Fortran",__FILE__,PETSC_ERR_ARG_WRONGSTATE,PETSC_ERROR_INITIAL,
               "Must call MatGetRow() first");
    *ierr = 1;
    return;
  }
  CHKFORTRANNULLINTEGER(cols); if (!cols) oocols = NULL;
  CHKFORTRANNULLSCALAR(vals);  if (!vals) oovals = NULL;

  *ierr           = MatRestoreRow(*mat,*row,ncols,oocols,oovals);
  matgetrowactive = 0;
}

PETSC_EXTERN void PETSC_STDCALL matview_(Mat *mat,PetscViewer *vin,PetscErrorCode *ierr)
{
  PetscViewer v;
  PetscPatchDefaultViewers_Fortran(vin,v);
  *ierr = MatView(*mat,v);
}

PETSC_EXTERN void PETSC_STDCALL matload_(Mat *mat,PetscViewer *vin,PetscErrorCode *ierr)
{
  PetscViewer v;
  PetscPatchDefaultViewers_Fortran(vin,v);
  *ierr = MatLoad(*mat,v);
}

PETSC_EXTERN void PETSC_STDCALL matseqaijgetarray_(Mat *mat,PetscScalar *fa,size_t *ia,PetscErrorCode *ierr)
{
  PetscScalar *mm;
  PetscInt    m,n;

  *ierr = MatSeqAIJGetArray(*mat,&mm); if (*ierr) return;
  *ierr = MatGetSize(*mat,&m,&n);  if (*ierr) return;
  *ierr = PetscScalarAddressToFortran((PetscObject)*mat,1,fa,mm,m*n,ia); if (*ierr) return;
}

PETSC_EXTERN void PETSC_STDCALL matseqaijrestorearray_(Mat *mat,PetscScalar *fa,size_t *ia,PetscErrorCode *ierr)
{
  PetscScalar *lx;
  PetscInt    m,n;

  *ierr = MatGetSize(*mat,&m,&n); if (*ierr) return;
  *ierr = PetscScalarAddressFromFortran((PetscObject)*mat,fa,*ia,m*n,&lx);if (*ierr) return;
  *ierr = MatSeqAIJRestoreArray(*mat,&lx);if (*ierr) return;
}

PETSC_EXTERN void PETSC_STDCALL matdensegetarray_(Mat *mat,PetscScalar *fa,size_t *ia,PetscErrorCode *ierr)
{
  PetscScalar *mm;
  PetscInt    m,n;

  *ierr = MatDenseGetArray(*mat,&mm); if (*ierr) return;
  *ierr = MatGetSize(*mat,&m,&n);  if (*ierr) return;
  *ierr = PetscScalarAddressToFortran((PetscObject)*mat,1,fa,mm,m*n,ia); if (*ierr) return;
}

PETSC_EXTERN void PETSC_STDCALL matdenserestorearray_(Mat *mat,PetscScalar *fa,size_t *ia,PetscErrorCode *ierr)
{
  PetscScalar *lx;
  PetscInt    m,n;

  *ierr = MatGetSize(*mat,&m,&n); if (*ierr) return;
  *ierr = PetscScalarAddressFromFortran((PetscObject)*mat,fa,*ia,m*n,&lx);if (*ierr) return;
  *ierr = MatDenseRestoreArray(*mat,&lx);if (*ierr) return;
}

PETSC_EXTERN void PETSC_STDCALL matfactorgetsolverpackage_(Mat *mat,CHAR name PETSC_MIXED_LEN(len),PetscErrorCode *ierr PETSC_END_LEN(len))
{
  const char *tname;

  *ierr = MatFactorGetSolverPackage(*mat,&tname);if (*ierr) return;
  if (name != PETSC_NULL_CHARACTER_Fortran) {
    *ierr = PetscStrncpy(name,tname,len);if (*ierr) return;
  }
  FIXRETURNCHAR(PETSC_TRUE,name,len);
}

PETSC_EXTERN void PETSC_STDCALL matgetfactor_(Mat *mat,CHAR outtype PETSC_MIXED_LEN(len),MatFactorType *ftype,Mat *M,PetscErrorCode *ierr PETSC_END_LEN(len))
{
  char *t;
  FIXCHAR(outtype,len,t);
  *ierr = MatGetFactor(*mat,t,*ftype,M);
  FREECHAR(outtype,t);
}

PETSC_EXTERN void PETSC_STDCALL matconvert_(Mat *mat,CHAR outtype PETSC_MIXED_LEN(len),MatReuse *reuse,Mat *M,PetscErrorCode *ierr PETSC_END_LEN(len))
{
  char *t;
  FIXCHAR(outtype,len,t);
  *ierr = MatConvert(*mat,t,*reuse,M);
  FREECHAR(outtype,t);
}

/*
    MatGetSubmatrices() is slightly different from C since the
    Fortran provides the array to hold the submatrix objects,while in C that
    array is allocated by the MatGetSubmatrices()
*/
PETSC_EXTERN void PETSC_STDCALL matgetsubmatrices_(Mat *mat,PetscInt *n,IS *isrow,IS *iscol,MatReuse *scall,Mat *smat,PetscErrorCode *ierr)
{
  Mat      *lsmat;
  PetscInt i;

  if (*scall == MAT_INITIAL_MATRIX) {
    *ierr = MatGetSubMatrices(*mat,*n,isrow,iscol,*scall,&lsmat);
    for (i=0; i<*n; i++) {
      smat[i] = lsmat[i];
    }
    *ierr = PetscFree(lsmat);
  } else {
    *ierr = MatGetSubMatrices(*mat,*n,isrow,iscol,*scall,&smat);
  }
}

/*
    MatDestroyMatrices() is slightly different from C since the
    Fortran provides the array to hold the submatrix objects,while in C that
    array is allocated by the MatGetSubmatrices()
*/
PETSC_EXTERN void PETSC_STDCALL matdestroymatrices_(Mat *mat,PetscInt *n,Mat *smat,PetscErrorCode *ierr)
{
  PetscInt i;

  for (i=0; i<*n; i++) {
    *ierr = MatDestroy(&smat[i]);if (*ierr) return;
  }
}

PETSC_EXTERN void PETSC_STDCALL matzerorowscolumns_(Mat *mat,PetscInt *numRows,PetscInt *rows,PetscScalar *diag,Vec *x,Vec *b,PetscErrorCode *ierr)
{
  CHKFORTRANNULLOBJECTDEREFERENCE(x);
  CHKFORTRANNULLOBJECTDEREFERENCE(b);
  *ierr = MatZeroRowsColumns(*mat,*numRows,rows,*diag,*x,*b);
}

PETSC_EXTERN void PETSC_STDCALL matzerorowscolumnsis_(Mat *mat,IS *is,PetscScalar *diag,Vec *x,Vec *b,PetscErrorCode *ierr)
{
  CHKFORTRANNULLOBJECTDEREFERENCE(x);
  CHKFORTRANNULLOBJECTDEREFERENCE(b);
  *ierr = MatZeroRowsColumnsIS(*mat,*is,*diag,*x,*b);
}

PETSC_EXTERN void PETSC_STDCALL matzerorowsstencil_(Mat *mat,PetscInt *numRows,MatStencil *rows,PetscScalar *diag,Vec *x,Vec *b,PetscErrorCode *ierr)
{
  CHKFORTRANNULLOBJECTDEREFERENCE(x);
  CHKFORTRANNULLOBJECTDEREFERENCE(b);
  *ierr = MatZeroRowsStencil(*mat,*numRows,rows,*diag,*x,*b);
}

PETSC_EXTERN void PETSC_STDCALL matzerorowscolumnsstencil_(Mat *mat,PetscInt *numRows,MatStencil *rows,PetscScalar *diag,Vec *x,Vec *b,PetscErrorCode *ierr)
{
  CHKFORTRANNULLOBJECTDEREFERENCE(x);
  CHKFORTRANNULLOBJECTDEREFERENCE(b);
  *ierr = MatZeroRowsColumnsStencil(*mat,*numRows,rows,*diag,*x,*b);
}

PETSC_EXTERN void PETSC_STDCALL matzerorows_(Mat *mat,PetscInt *numRows,PetscInt *rows,PetscScalar *diag,Vec *x,Vec *b,PetscErrorCode *ierr)
{
  CHKFORTRANNULLOBJECTDEREFERENCE(x);
  CHKFORTRANNULLOBJECTDEREFERENCE(b);
  *ierr = MatZeroRows(*mat,*numRows,rows,*diag,*x,*b);
}

PETSC_EXTERN void PETSC_STDCALL matzerorowsis_(Mat *mat,IS *is,PetscScalar *diag,Vec *x,Vec *b,PetscErrorCode *ierr)
{
  CHKFORTRANNULLOBJECTDEREFERENCE(x);
  CHKFORTRANNULLOBJECTDEREFERENCE(b);
  *ierr = MatZeroRowsIS(*mat,*is,*diag,*x,*b);
}

PETSC_EXTERN void PETSC_STDCALL matzerorowslocal_(Mat *mat,PetscInt *numRows,PetscInt *rows,PetscScalar *diag,Vec *x,Vec *b,PetscErrorCode *ierr)
{
  CHKFORTRANNULLOBJECTDEREFERENCE(x);
  CHKFORTRANNULLOBJECTDEREFERENCE(b);
  *ierr = MatZeroRowsLocal(*mat,*numRows,rows,*diag,*x,*b);
}

PETSC_EXTERN void PETSC_STDCALL matzerorowslocalis_(Mat *mat,IS *is,PetscScalar *diag,Vec *x,Vec *b,PetscErrorCode *ierr)
{
  CHKFORTRANNULLOBJECTDEREFERENCE(x);
  CHKFORTRANNULLOBJECTDEREFERENCE(b);
  *ierr = MatZeroRowsLocalIS(*mat,*is,*diag,*x,*b);
}

PETSC_EXTERN void PETSC_STDCALL matzerorowscolumnslocal_(Mat *mat,PetscInt *numRows,PetscInt *rows,PetscScalar *diag,Vec *x,Vec *b,PetscErrorCode *ierr)
{
  CHKFORTRANNULLOBJECTDEREFERENCE(x);
  CHKFORTRANNULLOBJECTDEREFERENCE(b);
  *ierr = MatZeroRowsColumnsLocal(*mat,*numRows,rows,*diag,*x,*b);
}

PETSC_EXTERN void PETSC_STDCALL matzerorowscolumnslocalis_(Mat *mat,IS *is,PetscScalar *diag,Vec *x,Vec *b,PetscErrorCode *ierr)
{
  CHKFORTRANNULLOBJECTDEREFERENCE(x);
  CHKFORTRANNULLOBJECTDEREFERENCE(b);
  *ierr = MatZeroRowsColumnsLocalIS(*mat,*is,*diag,*x,*b);
}

PETSC_EXTERN void PETSC_STDCALL matsetoptionsprefix_(Mat *mat,CHAR prefix PETSC_MIXED_LEN(len),PetscErrorCode *ierr PETSC_END_LEN(len))
{
  char *t;

  FIXCHAR(prefix,len,t);
  *ierr = MatSetOptionsPrefix(*mat,t);
  FREECHAR(prefix,t);
}

PETSC_EXTERN void PETSC_STDCALL matnullspaceremove_(MatNullSpace *sp,Vec *vec,PetscErrorCode *ierr)
{
  *ierr = MatNullSpaceRemove(*sp,*vec);
}

PETSC_EXTERN void PETSC_STDCALL matgetinfo_(Mat *mat,MatInfoType *flag,MatInfo *info, int *__ierr)
{
  *__ierr = MatGetInfo(*mat,*flag,info);
}

PETSC_EXTERN void PETSC_STDCALL matlufactor_(Mat *mat,IS *row,IS *col,const MatFactorInfo *info, int *__ierr)
{
  *__ierr = MatLUFactor(*mat,*row,*col,info);
}

PETSC_EXTERN void PETSC_STDCALL matilufactor_(Mat *mat,IS *row,IS *col,const MatFactorInfo *info, int *__ierr)
{
  *__ierr = MatILUFactor(*mat,*row,*col,info);
}

PETSC_EXTERN void PETSC_STDCALL matlufactorsymbolic_(Mat *fact,Mat *mat,IS *row,IS *col,const MatFactorInfo *info, int *__ierr)
{
  *__ierr = MatLUFactorSymbolic(*fact,*mat,*row,*col,info);
}

PETSC_EXTERN void PETSC_STDCALL matlufactornumeric_(Mat *fact,Mat *mat,const MatFactorInfo *info, int *__ierr)
{
  *__ierr = MatLUFactorNumeric(*fact,*mat,info);
}

PETSC_EXTERN void PETSC_STDCALL matcholeskyfactor_(Mat *mat,IS *perm,const MatFactorInfo *info, int *__ierr)
{
  *__ierr = MatCholeskyFactor(*mat,*perm,info);
}

PETSC_EXTERN void PETSC_STDCALL matcholeskyfactorsymbolic_(Mat *fact,Mat *mat,IS *perm,const MatFactorInfo *info, int *__ierr)
{
  *__ierr = MatCholeskyFactorSymbolic(*fact,*mat,*perm,info);
}

PETSC_EXTERN void PETSC_STDCALL matcholeskyfactornumeric_(Mat *fact,Mat *mat,const MatFactorInfo *info, int *__ierr)
{
  *__ierr = MatCholeskyFactorNumeric(*fact,*mat,info);
}

PETSC_EXTERN void PETSC_STDCALL matilufactorsymbolic_(Mat *fact,Mat *mat,IS *row,IS *col,const MatFactorInfo *info, int *__ierr)
{
  *__ierr = MatILUFactorSymbolic(*fact,*mat,*row,*col,info);
}

PETSC_EXTERN void PETSC_STDCALL maticcfactorsymbolic_(Mat *fact,Mat *mat,IS *perm,const MatFactorInfo *info, int *__ierr)
{
  *__ierr = MatICCFactorSymbolic(*fact,*mat,*perm,info);
}

PETSC_EXTERN void PETSC_STDCALL maticcfactor_(Mat *mat,IS *row,const MatFactorInfo *info, int *__ierr)
{
  *__ierr = MatICCFactor(*mat,*row,info);
}

PETSC_EXTERN void PETSC_STDCALL matfactorinfoinitialize_(MatFactorInfo *info, int *__ierr)
{
  *__ierr = MatFactorInfoInitialize(info);
}
