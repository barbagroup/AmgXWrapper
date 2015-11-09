#ifndef __PETSCCTABLE_H
#define __PETSCCTABLE_H
#include <petscsys.h>

struct _n_PetscTable {
  PetscInt *keytable;
  PetscInt *table;
  PetscInt count;
  PetscInt tablesize;
  PetscInt head;
  PetscInt maxkey;   /* largest key allowed */
};

typedef struct _n_PetscTable* PetscTable;
typedef PetscInt* PetscTablePosition;

#undef __FUNCT__
#define __FUNCT__ "PetscHash"
PETSC_STATIC_INLINE unsigned long PetscHash(PetscTable ta,unsigned long x)
{
#define PETSC_HASH_FACT 79943
  PetscFunctionBegin;
  PetscFunctionReturn((PETSC_HASH_FACT*x)%(unsigned long)ta->tablesize);
}

PETSC_EXTERN PetscErrorCode PetscTableCreate(const PetscInt,PetscInt,PetscTable*);
PETSC_EXTERN PetscErrorCode PetscTableCreateCopy(const PetscTable,PetscTable*);
PETSC_EXTERN PetscErrorCode PetscTableDestroy(PetscTable*);
PETSC_EXTERN PetscErrorCode PetscTableGetCount(const PetscTable,PetscInt*);
PETSC_EXTERN PetscErrorCode PetscTableIsEmpty(const PetscTable,PetscInt*);
PETSC_EXTERN PetscErrorCode PetscTableAddExpand(PetscTable,PetscInt,PetscInt,InsertMode);
PETSC_EXTERN PetscErrorCode PetscTableAddCountExpand(PetscTable,PetscInt);
PETSC_EXTERN PetscErrorCode PetscTableGetHeadPosition(PetscTable,PetscTablePosition*);
PETSC_EXTERN PetscErrorCode PetscTableGetNext(PetscTable,PetscTablePosition*,PetscInt*,PetscInt*);
PETSC_EXTERN PetscErrorCode PetscTableRemoveAll(PetscTable);

#undef __FUNCT__
#define __FUNCT__ "PetscTableAdd"
PETSC_STATIC_INLINE PetscErrorCode PetscTableAdd(PetscTable ta,PetscInt key,PetscInt data,InsertMode imode)
{
  PetscErrorCode ierr;
  PetscInt       i,hash = (PetscInt)PetscHash(ta,(unsigned long)key);

  PetscFunctionBegin;
  if (key <= 0) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"key <= 0");
  if (key > ta->maxkey) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"key %D is greater than largest key allowed %D",key,ta->maxkey);
  if (!data) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Null data");

  for (i=0; i<ta->tablesize; i++) {
    if (ta->keytable[hash] == key) {
      switch (imode) {
      case INSERT_VALUES:
        ta->table[hash] = data; /* over write */
        break;
      case ADD_VALUES:
        ta->table[hash] += data;
        break;
      case MAX_VALUES:
        ta->table[hash] = PetscMax(ta->table[hash],data);
        break;
      case NOT_SET_VALUES:
      case INSERT_ALL_VALUES:
      case ADD_ALL_VALUES:
      case INSERT_BC_VALUES:
      case ADD_BC_VALUES:
        SETERRQ(PETSC_COMM_SELF,PETSC_ERR_SUP,"Unsupported InsertMode");
      }
      PetscFunctionReturn(0);
    } else if (!ta->keytable[hash]) {
      if (ta->count < 5*(ta->tablesize/6) - 1) {
        ta->count++; /* add */
        ta->keytable[hash] = key;
        ta->table[hash] = data;
      } else {
        ierr = PetscTableAddExpand(ta,key,data,imode);CHKERRQ(ierr);
      }
      PetscFunctionReturn(0);
    }
    hash = (hash == (ta->tablesize-1)) ? 0 : hash+1;
  }
  SETERRQ(PETSC_COMM_SELF,PETSC_ERR_COR,"Full table");
  /* PetscFunctionReturn(0); */
}

#undef __FUNCT__
#define __FUNCT__ "PetscTableAddCount"
PETSC_STATIC_INLINE PetscErrorCode  PetscTableAddCount(PetscTable ta,PetscInt key)
{
  PetscErrorCode ierr;
  PetscInt       i,hash = (PetscInt)PetscHash(ta,(unsigned long)key);

  PetscFunctionBegin;
  if (key <= 0) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"key <= 0");
  if (key > ta->maxkey) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"key %D is greater than largest key allowed %D",key,ta->maxkey);

  for (i=0; i<ta->tablesize; i++) {
    if (ta->keytable[hash] == key) {
      PetscFunctionReturn(0);
    } else if (!ta->keytable[hash]) {
      if (ta->count < 5*(ta->tablesize/6) - 1) {
        ta->count++; /* add */
        ta->keytable[hash] = key;
        ta->table[hash] = ta->count;
      } else {
        ierr = PetscTableAddCountExpand(ta,key);CHKERRQ(ierr);
      }
      PetscFunctionReturn(0);
    }
    hash = (hash == (ta->tablesize-1)) ? 0 : hash+1;
  }
  SETERRQ(PETSC_COMM_SELF,PETSC_ERR_COR,"Full table");
  /* PetscFunctionReturn(0); */
}


#undef __FUNCT__
#define __FUNCT__ "PetscTableFind"
/*
    PetscTableFind - checks if a key is in the table

    If data==0, then no table entry exists.

*/
PETSC_STATIC_INLINE PetscErrorCode  PetscTableFind(PetscTable ta,PetscInt key,PetscInt *data)
{
  PetscInt hash,ii = 0;

  PetscFunctionBegin;
  *data = 0;
  if (key <= 0) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Key <= 0");
  if (key > ta->maxkey) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"key %D is greater than largest key allowed %D",key,ta->maxkey);

  hash  = (PetscInt)PetscHash(ta,(unsigned long)key);
  while (ii++ < ta->tablesize) {
    if (!ta->keytable[hash]) break;
    else if (ta->keytable[hash] == key) {
      *data = ta->table[hash];
      break;
    }
    hash = (hash == (ta->tablesize-1)) ? 0 : hash+1;
  }
  PetscFunctionReturn(0);
}

/* Reset __FUNCT__ in case the user does not define it themselves */
#undef __FUNCT__
#define __FUNCT__ "User provided function"

#endif
