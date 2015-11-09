#include <../src/sys/f90-src/f90impl.h>
#include <petsc/private/fortranimpl.h>
#include <petscbag.h>
#include <petsc/private/bagimpl.h>
#include <petscviewer.h>

#if defined(PETSC_HAVE_FORTRAN_CAPS)
#define petscbagdestroy_ PETSCBAGDESTROY
#define petscbagview_ PETSCBAGVIEW
#define petscbagload_ PETSCBAGLOAD
#define petscbaggetdata_ PETSCBAGGETDATA
#define petscbagregisterint_ PETSCBAGREGISTERINT
#define petscbagregister64bitint_ PETSCBAGREGISTER64BITINT
#define petscbagregisterintarray_ PETSCBAGREGISTERINTARRAY
#define petscbagregisterscalar_ PETSCBAGREGISTERSCALAR
#define petscbagregisterstring_ PETSCBAGREGISTERSTRING
#define petscbagregisterreal_ PETSCBAGREGISTERREAL
#define petscbagregisterrealarray_ PETSCBAGREGISTERREALARRAY
#define petscbagregisterbool_ PETSCBAGREGISTERBOOL
#define petscbagregisterboolarray_ PETSCBAGREGISTERBOOLARRAY
#define petscbagsetname_ PETSCBAGSETNAME
#define petscbagsetoptionsprefix_ PETSCBAGSETOPTIONSPREFIX
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE)
#define petscbagdestroy_ petscbagdestroy
#define petscbagview_ petscbagview
#define petscbagload_ petscbagload
#define petscbaggetdata_ petscbaggetdata
#define petscbagregisterint_ petscbagregisterint
#define petscbagregister64bitint_ petscbagregister64bitint
#define petscbagregisterintarray_ petscbagregisterintarray
#define petscbagregisterscalar_ petscbagregisterscalar
#define petscbagregisterstring_ petscbagregisterstring
#define petscbagregisterreal_ petscbagregisterreal
#define petscbagregisterrealarray_ petscbagregisterrealarray
#define petscbagregisterbool_ petscbagregisterbool
#define petscbagregisterboolarray_ petscbagregisterboolarray
#define petscbagsetname_ petscbagsetname
#define petscbagsetoptionsprefix_ petscbagsetoptionsprefix
#endif

PETSC_EXTERN void PETSC_STDCALL petscbagdestroy_(PetscBag *bag,PetscErrorCode *ierr)
{
  *ierr = PetscBagDestroy(bag);
}

PETSC_EXTERN void PETSC_STDCALL petscbagview_(PetscBag *bag,PetscViewer *viewer,PetscErrorCode *ierr)
{
  PetscViewer v;
  PetscPatchDefaultViewers_Fortran(viewer,v);
  *ierr = PetscBagView(*bag,v);
}

PETSC_EXTERN void PETSC_STDCALL petscbagload_(PetscViewer *viewer,PetscBag *bag,PetscErrorCode *ierr)
{
  PetscViewer v;
  PetscPatchDefaultViewers_Fortran(viewer,v);
  *ierr = PetscBagLoad(v,*bag);
}

PETSC_EXTERN void PETSC_STDCALL petscbagregisterint_(PetscBag *bag,void *ptr,PetscInt *def,CHAR s1 PETSC_MIXED_LEN(l1),
                                        CHAR s2 PETSC_MIXED_LEN(l2),PetscErrorCode *ierr PETSC_END_LEN(l1) PETSC_END_LEN(l2))
{
  char *t1,*t2;
  FIXCHAR(s1,l1,t1);
  FIXCHAR(s2,l2,t2);
  *ierr = PetscBagRegisterInt(*bag,ptr,*def,t1,t2);
  FREECHAR(s1,t1);
  FREECHAR(s2,t2);
}

PETSC_EXTERN void PETSC_STDCALL petscbagregister64bitint_(PetscBag *bag,void *ptr,Petsc64bitInt *def,CHAR s1 PETSC_MIXED_LEN(l1),
                                        CHAR s2 PETSC_MIXED_LEN(l2),PetscErrorCode *ierr PETSC_END_LEN(l1) PETSC_END_LEN(l2))
{
  char *t1,*t2;
  FIXCHAR(s1,l1,t1);
  FIXCHAR(s2,l2,t2);
  *ierr = PetscBagRegister64bitInt(*bag,ptr,*def,t1,t2);
  FREECHAR(s1,t1);
  FREECHAR(s2,t2);
}

PETSC_EXTERN void PETSC_STDCALL petscbagregisterintarray_(PetscBag *bag,void *ptr,PetscInt *msize,CHAR s1 PETSC_MIXED_LEN(l1),
                                             CHAR s2 PETSC_MIXED_LEN(l2),PetscErrorCode *ierr PETSC_END_LEN(l1) PETSC_END_LEN(l2))
{
  char *t1,*t2;
  FIXCHAR(s1,l1,t1);
  FIXCHAR(s2,l2,t2);
  *ierr = PetscBagRegisterIntArray(*bag,ptr,*msize,t1,t2);
  FREECHAR(s1,t1);
  FREECHAR(s2,t2);
}

PETSC_EXTERN void PETSC_STDCALL petscbagregisterscalar_(PetscBag *bag,void *ptr,PetscScalar *def,CHAR s1 PETSC_MIXED_LEN(l1),
                                           CHAR s2 PETSC_MIXED_LEN(l2),PetscErrorCode *ierr PETSC_END_LEN(l1) PETSC_END_LEN(l2))
{
  char *t1,*t2;
  FIXCHAR(s1,l1,t1);
  FIXCHAR(s2,l2,t2);
  *ierr = PetscBagRegisterScalar(*bag,ptr,*def,t1,t2);
  FREECHAR(s1,t1);
  FREECHAR(s2,t2);
}

PETSC_EXTERN void PETSC_STDCALL petscbagregisterreal_(PetscBag *bag,void *ptr,PetscReal *def,CHAR s1 PETSC_MIXED_LEN(l1),
                                         CHAR s2 PETSC_MIXED_LEN(l2),PetscErrorCode *ierr PETSC_END_LEN(l1) PETSC_END_LEN(l2))
{
  char *t1,*t2;
  FIXCHAR(s1,l1,t1);
  FIXCHAR(s2,l2,t2);
  *ierr = PetscBagRegisterReal(*bag,ptr,*def,t1,t2);
  FREECHAR(s1,t1);
  FREECHAR(s2,t2);
}

PETSC_EXTERN void PETSC_STDCALL petscbagregisterrealarray_(PetscBag *bag,void *ptr,PetscInt *msize,CHAR s1 PETSC_MIXED_LEN(l1),
                                              CHAR s2 PETSC_MIXED_LEN(l2),PetscErrorCode *ierr PETSC_END_LEN(l1) PETSC_END_LEN(l2))
{
  char *t1,*t2;
  FIXCHAR(s1,l1,t1);
  FIXCHAR(s2,l2,t2);
  *ierr = PetscBagRegisterRealArray(*bag,ptr,*msize,t1,t2);
  FREECHAR(s1,t1);
  FREECHAR(s2,t2);
}

PETSC_EXTERN void PETSC_STDCALL petscbagregisterbool_(PetscBag *bag,void *ptr,PetscBool *def,CHAR s1 PETSC_MIXED_LEN(l1),
                                         CHAR s2 PETSC_MIXED_LEN(l2),PetscErrorCode *ierr PETSC_END_LEN(l1) PETSC_END_LEN(l2))
{
  char       *t1,*t2;
  PetscBool  flg = PETSC_FALSE;

  /* some Fortran compilers use -1 as boolean */
  if (*def) flg = PETSC_TRUE;
  FIXCHAR(s1,l1,t1);
  FIXCHAR(s2,l2,t2);
  *ierr = PetscBagRegisterBool(*bag,ptr,flg,t1,t2);
  FREECHAR(s1,t1);
  FREECHAR(s2,t2);
}

PETSC_EXTERN void PETSC_STDCALL petscbagregisterboolarray_(PetscBag *bag,void *ptr,PetscInt *msize,CHAR s1 PETSC_MIXED_LEN(l1),
                                              CHAR s2 PETSC_MIXED_LEN(l2),PetscErrorCode *ierr PETSC_END_LEN(l1) PETSC_END_LEN(l2))
{
  char       *t1,*t2;

  /* some Fortran compilers use -1 as boolean */
  FIXCHAR(s1,l1,t1);
  FIXCHAR(s2,l2,t2);
  *ierr = PetscBagRegisterBoolArray(*bag,ptr,*msize,t1,t2);
  FREECHAR(s1,t1);
  FREECHAR(s2,t2);
}

PETSC_EXTERN void PETSC_STDCALL petscbagregisterstring_(PetscBag *bag,CHAR p PETSC_MIXED_LEN(pl),CHAR cs1 PETSC_MIXED_LEN(cl1),CHAR s1 PETSC_MIXED_LEN(l1),
                                                        CHAR s2 PETSC_MIXED_LEN(l2),PetscErrorCode *ierr PETSC_END_LEN(pl) PETSC_END_LEN(cl1) PETSC_END_LEN(l1) PETSC_END_LEN(l2))
{
  char *t1,*t2,*ct1;
  FIXCHAR(s1,l1,t1);
  FIXCHAR(cs1,cl1,ct1);
  FIXCHAR(s2,l2,t2);
  *ierr = PetscBagRegisterString(*bag,p,pl,ct1,t1,t2);
  FREECHAR(cs1,ct1);
  FREECHAR(s1,t1);
  FREECHAR(s2,t2);
}


PETSC_EXTERN void PETSC_STDCALL petscbaggetdata_(PetscBag *bag,void **data,PetscErrorCode *ierr)
{
  *ierr = PetscBagGetData(*bag,data);
}

PETSC_EXTERN void PETSC_STDCALL petscbagsetname_(PetscBag *bag,CHAR ns PETSC_MIXED_LEN(nl),CHAR hs PETSC_MIXED_LEN(hl), PetscErrorCode *ierr PETSC_END_LEN(nl) PETSC_END_LEN(hl))
{
  char *nt,*ht;
  FIXCHAR(ns,nl,nt);
  FIXCHAR(hs,hl,ht);
  *ierr = PetscBagSetName(*bag,nt,ht);
  FREECHAR(ns,nt);
  FREECHAR(hs,ht);
}

PETSC_EXTERN void PETSC_STDCALL petscbagsetoptionsprefix_(PetscBag *bag,CHAR pre PETSC_MIXED_LEN(len), PetscErrorCode *ierr PETSC_END_LEN(len))
{
  char *t;
  FIXCHAR(pre,len,t);
  *ierr = PetscBagSetOptionsPrefix(*bag,t);
  FREECHAR(pre,t);
}
