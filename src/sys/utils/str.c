
/*
    We define the string operations here. The reason we just do not use
  the standard string routines in the PETSc code is that on some machines
  they are broken or have the wrong prototypes.

*/
#include <petscsys.h>                   /*I  "petscsys.h"   I*/
#if defined(PETSC_HAVE_STRING_H)
#include <string.h>             /* strstr */
#endif
#if defined(PETSC_HAVE_STRINGS_H)
#  include <strings.h>          /* strcasecmp */
#endif

#undef __FUNCT__
#define __FUNCT__ "PetscStrToArray"
/*@C
   PetscStrToArray - Separates a string by a charactor (for example ' ' or '\n') and creates an array of strings

   Not Collective

   Input Parameters:
+  s - pointer to string
-  sp - separator charactor

   Output Parameter:
+   argc - the number of entries in the array
-   args - an array of the entries with a null at the end

   Level: intermediate

   Notes: this may be called before PetscInitialize() or after PetscFinalize()

   Not for use in Fortran

   Developer Notes: Using raw malloc() and does not call error handlers since this may be used before PETSc is initialized. Used
     to generate argc, args arguments passed to MPI_Init()

.seealso: PetscStrToArrayDestroy(), PetscToken, PetscTokenCreate()

@*/
PetscErrorCode  PetscStrToArray(const char s[],char sp,int *argc,char ***args)
{
  int       i,n,*lens,cnt = 0;
  PetscBool flg = PETSC_FALSE;

  if (!s) n = 0;
  else    n = strlen(s);
  *argc = 0;
  if (!n) {
    *args = 0;
    return(0);
  }
  for (i=0; i<n; i++) {
    if (s[i] != sp) break;
  }
  for (;i<n+1; i++) {
    if ((s[i] == sp || s[i] == 0) && !flg) {flg = PETSC_TRUE; (*argc)++;}
    else if (s[i] != sp) {flg = PETSC_FALSE;}
  }
  (*args) = (char**) malloc(((*argc)+1)*sizeof(char*)); if (!*args) return PETSC_ERR_MEM;
  lens    = (int*) malloc((*argc)*sizeof(int)); if (!lens) return PETSC_ERR_MEM;
  for (i=0; i<*argc; i++) lens[i] = 0;

  *argc = 0;
  for (i=0; i<n; i++) {
    if (s[i] != sp) break;
  }
  for (;i<n+1; i++) {
    if ((s[i] == sp || s[i] == 0) && !flg) {flg = PETSC_TRUE; (*argc)++;}
    else if (s[i] != sp) {lens[*argc]++;flg = PETSC_FALSE;}
  }

  for (i=0; i<*argc; i++) {
    (*args)[i] = (char*) malloc((lens[i]+1)*sizeof(char)); if (!(*args)[i]) return PETSC_ERR_MEM;
  }
  free(lens);
  (*args)[*argc] = 0;

  *argc = 0;
  for (i=0; i<n; i++) {
    if (s[i] != sp) break;
  }
  for (;i<n+1; i++) {
    if ((s[i] == sp || s[i] == 0) && !flg) {flg = PETSC_TRUE; (*args)[*argc][cnt++] = 0; (*argc)++; cnt = 0;}
    else if (s[i] != sp && s[i] != 0) {(*args)[*argc][cnt++] = s[i]; flg = PETSC_FALSE;}
  }
  return 0;
}

#undef __FUNCT__
#define __FUNCT__ "PetscStrToArrayDestroy"
/*@C
   PetscStrToArrayDestroy - Frees array created with PetscStrToArray().

   Not Collective

   Output Parameters:
+  argc - the number of arguments
-  args - the array of arguments

   Level: intermediate

   Concepts: command line arguments

   Notes: This may be called before PetscInitialize() or after PetscFinalize()

   Not for use in Fortran

.seealso: PetscStrToArray()

@*/
PetscErrorCode  PetscStrToArrayDestroy(int argc,char **args)
{
  PetscInt i;

  for (i=0; i<argc; i++) free(args[i]);
  if (args) free(args);
  return 0;
}

#undef __FUNCT__
#define __FUNCT__ "PetscStrlen"
/*@C
   PetscStrlen - Gets length of a string

   Not Collective

   Input Parameters:
.  s - pointer to string

   Output Parameter:
.  len - length in bytes

   Level: intermediate

   Note:
   This routine is analogous to strlen().

   Null string returns a length of zero

   Not for use in Fortran

  Concepts: string length

@*/
PetscErrorCode  PetscStrlen(const char s[],size_t *len)
{
  PetscFunctionBegin;
  if (!s) *len = 0;
  else    *len = strlen(s);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscStrallocpy"
/*@C
   PetscStrallocpy - Allocates space to hold a copy of a string then copies the string

   Not Collective

   Input Parameters:
.  s - pointer to string

   Output Parameter:
.  t - the copied string

   Level: intermediate

   Note:
      Null string returns a new null string

      Not for use in Fortran

  Concepts: string copy

@*/
PetscErrorCode  PetscStrallocpy(const char s[],char *t[])
{
  PetscErrorCode ierr;
  size_t         len;
  char           *tmp = 0;

  PetscFunctionBegin;
  if (s) {
    ierr = PetscStrlen(s,&len);CHKERRQ(ierr);
    ierr = PetscMalloc1(1+len,&tmp);CHKERRQ(ierr);
    ierr = PetscStrcpy(tmp,s);CHKERRQ(ierr);
  }
  *t = tmp;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscStrArrayallocpy"
/*@C
   PetscStrArrayallocpy - Allocates space to hold a copy of an array of strings then copies the strings

   Not Collective

   Input Parameters:
.  s - pointer to array of strings (final string is a null)

   Output Parameter:
.  t - the copied array string

   Level: intermediate

   Note:
      Not for use in Fortran

  Concepts: string copy

.seealso: PetscStrallocpy() PetscStrArrayDestroy()

@*/
PetscErrorCode  PetscStrArrayallocpy(const char *const *list,char ***t)
{
  PetscErrorCode ierr;
  PetscInt       i,n = 0;

  PetscFunctionBegin;
  while (list[n++]) ;
  ierr = PetscMalloc1(n+1,t);CHKERRQ(ierr);
  for (i=0; i<n; i++) {
    ierr = PetscStrallocpy(list[i],(*t)+i);CHKERRQ(ierr);
  }
  (*t)[n] = NULL;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscStrArrayDestroy"
/*@C
   PetscStrArrayDestroy - Frees array of strings created with PetscStrArrayallocpy().

   Not Collective

   Output Parameters:
.   list - array of strings

   Level: intermediate

   Concepts: command line arguments

   Notes: Not for use in Fortran

.seealso: PetscStrArrayallocpy()

@*/
PetscErrorCode PetscStrArrayDestroy(char ***list)
{
  PetscInt       n = 0;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (!*list) PetscFunctionReturn(0);
  while ((*list)[n]) {
    ierr = PetscFree((*list)[n]);CHKERRQ(ierr);
    n++;
  }
  ierr = PetscFree(*list);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscStrNArrayallocpy"
/*@C
   PetscStrNArrayallocpy - Allocates space to hold a copy of an array of strings then copies the strings

   Not Collective

   Input Parameters:
+  n - the number of string entries
-  s - pointer to array of strings

   Output Parameter:
.  t - the copied array string

   Level: intermediate

   Note:
      Not for use in Fortran

  Concepts: string copy

.seealso: PetscStrallocpy() PetscStrArrayDestroy()

@*/
PetscErrorCode  PetscStrNArrayallocpy(PetscInt n,const char *const *list,char ***t)
{
  PetscErrorCode ierr;
  PetscInt       i;

  PetscFunctionBegin;
  ierr = PetscMalloc1(n,t);CHKERRQ(ierr);
  for (i=0; i<n; i++) {
    ierr = PetscStrallocpy(list[i],(*t)+i);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscStrNArrayDestroy"
/*@C
   PetscStrNArrayDestroy - Frees array of strings created with PetscStrArrayallocpy().

   Not Collective

   Output Parameters:
+   n - number of string entries
-   list - array of strings

   Level: intermediate

   Notes: Not for use in Fortran

.seealso: PetscStrArrayallocpy()

@*/
PetscErrorCode PetscStrNArrayDestroy(PetscInt n,char ***list)
{
  PetscErrorCode ierr;
  PetscInt       i;

  PetscFunctionBegin;
  if (!*list) PetscFunctionReturn(0);
  for (i=0; i<n; i++){
    ierr = PetscFree((*list)[i]);CHKERRQ(ierr);
  }
  ierr = PetscFree(*list);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscStrcpy"
/*@C
   PetscStrcpy - Copies a string

   Not Collective

   Input Parameters:
.  t - pointer to string

   Output Parameter:
.  s - the copied string

   Level: intermediate

   Notes:
     Null string returns a string starting with zero

     Not for use in Fortran

  Concepts: string copy

.seealso: PetscStrncpy(), PetscStrcat(), PetscStrncat()

@*/

PetscErrorCode  PetscStrcpy(char s[],const char t[])
{
  PetscFunctionBegin;
  if (t && !s) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_NULL,"Trying to copy string into null pointer");
  if (t) strcpy(s,t);
  else if (s) s[0] = 0;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscStrncpy"
/*@C
   PetscStrncpy - Copies a string up to a certain length

   Not Collective

   Input Parameters:
+  t - pointer to string
-  n - the length to copy

   Output Parameter:
.  s - the copied string

   Level: intermediate

   Note:
     Null string returns a string starting with zero

     If the string that is being copied is of length n or larger then the entire string is not
     copied and the file location of s is set to NULL. This is different then the behavior of 
     strncpy() which leaves s non-terminated.

  Concepts: string copy

.seealso: PetscStrcpy(), PetscStrcat(), PetscStrncat()

@*/
PetscErrorCode  PetscStrncpy(char s[],const char t[],size_t n)
{
  PetscFunctionBegin;
  if (t && !s) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_NULL,"Trying to copy string into null pointer");
  if (t) {
    if (n > 1) {
      strncpy(s,t,n-1);
      s[n-1] = '\0';
    } else {
      s[0] = '\0';
    }
  } else if (s) s[0] = 0;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscStrcat"
/*@C
   PetscStrcat - Concatenates a string onto a given string

   Not Collective

   Input Parameters:
+  s - string to be added to
-  t - pointer to string to be added to end

   Level: intermediate

   Notes: Not for use in Fortran

  Concepts: string copy

.seealso: PetscStrcpy(), PetscStrncpy(), PetscStrncat()

@*/
PetscErrorCode  PetscStrcat(char s[],const char t[])
{
  PetscFunctionBegin;
  if (!t) PetscFunctionReturn(0);
  strcat(s,t);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscStrncat"
/*@C
   PetscStrncat - Concatenates a string onto a given string, up to a given length

   Not Collective

   Input Parameters:
+  s - pointer to string to be added to end
.  t - string to be added to
.  n - maximum length to copy

   Level: intermediate

  Notes:    Not for use in Fortran

  Concepts: string copy

.seealso: PetscStrcpy(), PetscStrncpy(), PetscStrcat()

@*/
PetscErrorCode  PetscStrncat(char s[],const char t[],size_t n)
{
  PetscFunctionBegin;
  strncat(s,t,n);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscStrcmpNoError"
/*
   Only to be used with PetscCheck__FUNCT__()!

   Will be removed once we eliminate the __FUNCT__ paradigm
*/
void  PetscStrcmpNoError(const char a[],const char b[],PetscBool  *flg)
{
  int c;

  if (!a && !b)      *flg = PETSC_TRUE;
  else if (!a || !b) *flg = PETSC_FALSE;
  else {
    c = strcmp(a,b);
    if (c) *flg = PETSC_FALSE;
    else   *flg = PETSC_TRUE;
  }
}

#undef __FUNCT__
#define __FUNCT__ "PetscStrcmp"
/*@C
   PetscStrcmp - Compares two strings,

   Not Collective

   Input Parameters:
+  a - pointer to string first string
-  b - pointer to second string

   Output Parameter:
.  flg - PETSC_TRUE if the two strings are equal

   Level: intermediate

   Notes:    Not for use in Fortran

.seealso: PetscStrgrt(), PetscStrncmp(), PetscStrcasecmp()

@*/
PetscErrorCode  PetscStrcmp(const char a[],const char b[],PetscBool  *flg)
{
  int c;

  PetscFunctionBegin;
  if (!a && !b)      *flg = PETSC_TRUE;
  else if (!a || !b) *flg = PETSC_FALSE;
  else {
    c = strcmp(a,b);
    if (c) *flg = PETSC_FALSE;
    else   *flg = PETSC_TRUE;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscStrgrt"
/*@C
   PetscStrgrt - If first string is greater than the second

   Not Collective

   Input Parameters:
+  a - pointer to first string
-  b - pointer to second string

   Output Parameter:
.  flg - if the first string is greater

   Notes:
    Null arguments are ok, a null string is considered smaller than
    all others

   Not for use in Fortran

   Level: intermediate

.seealso: PetscStrcmp(), PetscStrncmp(), PetscStrcasecmp()

@*/
PetscErrorCode  PetscStrgrt(const char a[],const char b[],PetscBool  *t)
{
  int c;

  PetscFunctionBegin;
  if (!a && !b) *t = PETSC_FALSE;
  else if (a && !b) *t = PETSC_TRUE;
  else if (!a && b) *t = PETSC_FALSE;
  else {
    c = strcmp(a,b);
    if (c > 0) *t = PETSC_TRUE;
    else       *t = PETSC_FALSE;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscStrcasecmp"
/*@C
   PetscStrcasecmp - Returns true if the two strings are the same
     except possibly for case.

   Not Collective

   Input Parameters:
+  a - pointer to first string
-  b - pointer to second string

   Output Parameter:
.  flg - if the two strings are the same

   Notes:
    Null arguments are ok

   Not for use in Fortran

   Level: intermediate

.seealso: PetscStrcmp(), PetscStrncmp(), PetscStrgrt()

@*/
PetscErrorCode  PetscStrcasecmp(const char a[],const char b[],PetscBool  *t)
{
  int c;

  PetscFunctionBegin;
  if (!a && !b) c = 0;
  else if (!a || !b) c = 1;
#if defined(PETSC_HAVE_STRCASECMP)
  else c = strcasecmp(a,b);
#elif defined(PETSC_HAVE_STRICMP)
  else c = stricmp(a,b);
#else
  else {
    char           *aa,*bb;
    PetscErrorCode ierr;
    ierr = PetscStrallocpy(a,&aa);CHKERRQ(ierr);
    ierr = PetscStrallocpy(b,&bb);CHKERRQ(ierr);
    ierr = PetscStrtolower(aa);CHKERRQ(ierr);
    ierr = PetscStrtolower(bb);CHKERRQ(ierr);
    ierr = PetscStrcmp(aa,bb,t);CHKERRQ(ierr);
    ierr = PetscFree(aa);CHKERRQ(ierr);
    ierr = PetscFree(bb);CHKERRQ(ierr);
    PetscFunctionReturn(0);
  }
#endif
  if (!c) *t = PETSC_TRUE;
  else    *t = PETSC_FALSE;
  PetscFunctionReturn(0);
}



#undef __FUNCT__
#define __FUNCT__ "PetscStrncmp"
/*@C
   PetscStrncmp - Compares two strings, up to a certain length

   Not Collective

   Input Parameters:
+  a - pointer to first string
.  b - pointer to second string
-  n - length to compare up to

   Output Parameter:
.  t - if the two strings are equal

   Level: intermediate

   Notes:    Not for use in Fortran

.seealso: PetscStrgrt(), PetscStrcmp(), PetscStrcasecmp()

@*/
PetscErrorCode  PetscStrncmp(const char a[],const char b[],size_t n,PetscBool  *t)
{
  int c;

  PetscFunctionBegin;
  c = strncmp(a,b,n);
  if (!c) *t = PETSC_TRUE;
  else    *t = PETSC_FALSE;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscStrchr"
/*@C
   PetscStrchr - Locates first occurance of a character in a string

   Not Collective

   Input Parameters:
+  a - pointer to string
-  b - character

   Output Parameter:
.  c - location of occurance, NULL if not found

   Level: intermediate

   Notes:    Not for use in Fortran

@*/
PetscErrorCode  PetscStrchr(const char a[],char b,char *c[])
{
  PetscFunctionBegin;
  *c = (char*)strchr(a,b);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscStrrchr"
/*@C
   PetscStrrchr - Locates one location past the last occurance of a character in a string,
      if the character is not found then returns entire string

   Not Collective

   Input Parameters:
+  a - pointer to string
-  b - character

   Output Parameter:
.  tmp - location of occurance, a if not found

   Level: intermediate

   Notes:    Not for use in Fortran

@*/
PetscErrorCode  PetscStrrchr(const char a[],char b,char *tmp[])
{
  PetscFunctionBegin;
  *tmp = (char*)strrchr(a,b);
  if (!*tmp) *tmp = (char*)a;
  else *tmp = *tmp + 1;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscStrtolower"
/*@C
   PetscStrtolower - Converts string to lower case

   Not Collective

   Input Parameters:
.  a - pointer to string

   Level: intermediate

   Notes:    Not for use in Fortran

@*/
PetscErrorCode  PetscStrtolower(char a[])
{
  PetscFunctionBegin;
  while (*a) {
    if (*a >= 'A' && *a <= 'Z') *a += 'a' - 'A';
    a++;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscStrtoupper"
/*@C
   PetscStrtolower - Converts string to upper case

   Not Collective

   Input Parameters:
.  a - pointer to string

   Level: intermediate

   Notes:    Not for use in Fortran

@*/
PetscErrorCode  PetscStrtoupper(char a[])
{
  PetscFunctionBegin;
  while (*a) {
    if (*a >= 'a' && *a <= 'z') *a += 'A' - 'a';
    a++;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscStrendswith"
/*@C
   PetscStrendswith - Determines if a string ends with a certain string

   Not Collective

   Input Parameters:
+  a - pointer to string
-  b - string to endwith

   Output Parameter:
.  flg - PETSC_TRUE or PETSC_FALSE

   Notes:     Not for use in Fortran

   Level: intermediate

@*/
PetscErrorCode  PetscStrendswith(const char a[],const char b[],PetscBool *flg)
{
  char           *test;
  PetscErrorCode ierr;
  size_t         na,nb;

  PetscFunctionBegin;
  *flg = PETSC_FALSE;
  ierr = PetscStrrstr(a,b,&test);CHKERRQ(ierr);
  if (test) {
    ierr = PetscStrlen(a,&na);CHKERRQ(ierr);
    ierr = PetscStrlen(b,&nb);CHKERRQ(ierr);
    if (a+na-nb == test) *flg = PETSC_TRUE;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscStrbeginswith"
/*@C
   PetscStrbeginswith - Determines if a string begins with a certain string

   Not Collective

   Input Parameters:
+  a - pointer to string
-  b - string to beginwith

   Output Parameter:
.  flg - PETSC_TRUE or PETSC_FALSE

   Notes:     Not for use in Fortran

   Level: intermediate

@*/
PetscErrorCode  PetscStrbeginswith(const char a[],const char b[],PetscBool *flg)
{
  char           *test;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  *flg = PETSC_FALSE;
  ierr = PetscStrrstr(a,b,&test);CHKERRQ(ierr);
  if (test && (test == a)) *flg = PETSC_TRUE;
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "PetscStrendswithwhich"
/*@C
   PetscStrendswithwhich - Determines if a string ends with one of several possible strings

   Not Collective

   Input Parameters:
+  a - pointer to string
-  bs - strings to endwith (last entry must be null)

   Output Parameter:
.  cnt - the index of the string it ends with or 1+the last possible index

   Notes:     Not for use in Fortran

   Level: intermediate

@*/
PetscErrorCode  PetscStrendswithwhich(const char a[],const char *const *bs,PetscInt *cnt)
{
  PetscBool      flg;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  *cnt = 0;
  while (bs[*cnt]) {
    ierr = PetscStrendswith(a,bs[*cnt],&flg);CHKERRQ(ierr);
    if (flg) PetscFunctionReturn(0);
    *cnt += 1;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscStrrstr"
/*@C
   PetscStrrstr - Locates last occurance of string in another string

   Not Collective

   Input Parameters:
+  a - pointer to string
-  b - string to find

   Output Parameter:
.  tmp - location of occurance

   Notes:     Not for use in Fortran

   Level: intermediate

@*/
PetscErrorCode  PetscStrrstr(const char a[],const char b[],char *tmp[])
{
  const char *stmp = a, *ltmp = 0;

  PetscFunctionBegin;
  while (stmp) {
    stmp = (char*)strstr(stmp,b);
    if (stmp) {ltmp = stmp;stmp++;}
  }
  *tmp = (char*)ltmp;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscStrstr"
/*@C
   PetscStrstr - Locates first occurance of string in another string

   Not Collective

   Input Parameters:
+  haystack - string to search
-  needle - string to find

   Output Parameter:
.  tmp - location of occurance, is a NULL if the string is not found

   Notes: Not for use in Fortran

   Level: intermediate

@*/
PetscErrorCode  PetscStrstr(const char haystack[],const char needle[],char *tmp[])
{
  PetscFunctionBegin;
  *tmp = (char*)strstr(haystack,needle);
  PetscFunctionReturn(0);
}

struct _p_PetscToken {char token;char *array;char *current;};

#undef __FUNCT__
#define __FUNCT__ "PetscTokenFind"
/*@C
   PetscTokenFind - Locates next "token" in a string

   Not Collective

   Input Parameters:
.  a - pointer to token

   Output Parameter:
.  result - location of occurance, NULL if not found

   Notes:

     This version is different from the system version in that
  it allows you to pass a read-only string into the function.

     This version also treats all characters etc. inside a double quote "
   as a single token.

     For example if the separator character is + and the string is xxxx+y then the first fine will return a pointer to a null terminated xxxx and the 
   second will return a null terminated y

     If the separator character is + and the string is xxxx then the first and only token found will be a pointer to a null terminated xxxx

    Not for use in Fortran

   Level: intermediate


.seealso: PetscTokenCreate(), PetscTokenDestroy()
@*/
PetscErrorCode  PetscTokenFind(PetscToken a,char *result[])
{
  char *ptr = a->current,token;

  PetscFunctionBegin;
  *result = a->current;
  if (ptr && !*ptr) {*result = 0;PetscFunctionReturn(0);}
  token = a->token;
  if (ptr && (*ptr == '"')) {token = '"';(*result)++;ptr++;}
  while (ptr) {
    if (*ptr == token) {
      *ptr++ = 0;
      while (*ptr == a->token) ptr++;
      a->current = ptr;
      break;
    }
    if (!*ptr) {
      a->current = 0;
      break;
    }
    ptr++;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscTokenCreate"
/*@C
   PetscTokenCreate - Creates a PetscToken used to find tokens in a string

   Not Collective

   Input Parameters:
+  string - the string to look in
-  b - the separator character

   Output Parameter:
.  t- the token object

   Notes:

     This version is different from the system version in that
  it allows you to pass a read-only string into the function.

    Not for use in Fortran

   Level: intermediate

.seealso: PetscTokenFind(), PetscTokenDestroy()
@*/
PetscErrorCode  PetscTokenCreate(const char a[],const char b,PetscToken *t)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscNew(t);CHKERRQ(ierr);
  ierr = PetscStrallocpy(a,&(*t)->array);CHKERRQ(ierr);

  (*t)->current = (*t)->array;
  (*t)->token   = b;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscTokenDestroy"
/*@C
   PetscTokenDestroy - Destroys a PetscToken

   Not Collective

   Input Parameters:
.  a - pointer to token

   Level: intermediate

   Notes:     Not for use in Fortran

.seealso: PetscTokenCreate(), PetscTokenFind()
@*/
PetscErrorCode  PetscTokenDestroy(PetscToken *a)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (!*a) PetscFunctionReturn(0);
  ierr = PetscFree((*a)->array);CHKERRQ(ierr);
  ierr = PetscFree(*a);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "PetscGetPetscDir"
/*@C
   PetscGetPetscDir - Gets the directory PETSc is installed in

   Not Collective

   Output Parameter:
.  dir - the directory

   Level: developer

   Notes: Not for use in Fortran

@*/
PetscErrorCode  PetscGetPetscDir(const char *dir[])
{
  PetscFunctionBegin;
  *dir = PETSC_DIR;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscStrreplace"
/*@C
   PetscStrreplace - Replaces substrings in string with other substrings

   Not Collective

   Input Parameters:
+   comm - MPI_Comm of processors that are processing the string
.   aa - the string to look in
.   b - the resulting copy of a with replaced strings (b can be the same as a)
-   len - the length of b

   Notes:
      Replaces   ${PETSC_ARCH},${PETSC_DIR},${PETSC_LIB_DIR},${DISPLAY},
      ${HOMEDIRECTORY},${WORKINGDIRECTORY},${USERNAME}, ${HOSTNAME} with appropriate values
      as well as any environmental variables.

      PETSC_LIB_DIR uses the environmental variable if it exists. PETSC_ARCH and PETSC_DIR use what
      PETSc was built with and do not use environmental variables.

      Not for use in Fortran

   Level: intermediate

@*/
PetscErrorCode  PetscStrreplace(MPI_Comm comm,const char aa[],char b[],size_t len)
{
  PetscErrorCode ierr;
  int            i = 0;
  size_t         l,l1,l2,l3;
  char           *work,*par,*epar,env[1024],*tfree,*a = (char*)aa;
  const char     *s[] = {"${PETSC_ARCH}","${PETSC_DIR}","${PETSC_LIB_DIR}","${DISPLAY}","${HOMEDIRECTORY}","${WORKINGDIRECTORY}","${USERNAME}","${HOSTNAME}",0};
  const char     *r[] = {0,0,0,0,0,0,0,0,0};
  PetscBool      flag;

  PetscFunctionBegin;
  if (!a || !b) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_NULL,"a and b strings must be nonnull");
  if (aa == b) {
    ierr = PetscStrallocpy(aa,(char**)&a);CHKERRQ(ierr);
  }
  ierr = PetscMalloc1(len,&work);CHKERRQ(ierr);

  /* get values for replaced variables */
  ierr = PetscStrallocpy(PETSC_ARCH,(char**)&r[0]);CHKERRQ(ierr);
  ierr = PetscStrallocpy(PETSC_DIR,(char**)&r[1]);CHKERRQ(ierr);
  ierr = PetscStrallocpy(PETSC_LIB_DIR,(char**)&r[2]);CHKERRQ(ierr);
  ierr = PetscMalloc1(256,&r[3]);CHKERRQ(ierr);
  ierr = PetscMalloc1(PETSC_MAX_PATH_LEN,&r[4]);CHKERRQ(ierr);
  ierr = PetscMalloc1(PETSC_MAX_PATH_LEN,&r[5]);CHKERRQ(ierr);
  ierr = PetscMalloc1(256,&r[6]);CHKERRQ(ierr);
  ierr = PetscMalloc1(256,&r[7]);CHKERRQ(ierr);
  ierr = PetscGetDisplay((char*)r[3],256);CHKERRQ(ierr);
  ierr = PetscGetHomeDirectory((char*)r[4],PETSC_MAX_PATH_LEN);CHKERRQ(ierr);
  ierr = PetscGetWorkingDirectory((char*)r[5],PETSC_MAX_PATH_LEN);CHKERRQ(ierr);
  ierr = PetscGetUserName((char*)r[6],256);CHKERRQ(ierr);
  ierr = PetscGetHostName((char*)r[7],256);CHKERRQ(ierr);

  /* replace that are in environment */
  ierr = PetscOptionsGetenv(comm,"PETSC_LIB_DIR",env,1024,&flag);CHKERRQ(ierr);
  if (flag) {
    ierr = PetscFree(r[2]);CHKERRQ(ierr);
    ierr = PetscStrallocpy(env,(char**)&r[2]);CHKERRQ(ierr);
  }

  /* replace the requested strings */
  ierr = PetscStrncpy(b,a,len);CHKERRQ(ierr);
  while (s[i]) {
    ierr = PetscStrlen(s[i],&l);CHKERRQ(ierr);
    ierr = PetscStrstr(b,s[i],&par);CHKERRQ(ierr);
    while (par) {
      *par =  0;
      par += l;

      ierr = PetscStrlen(b,&l1);CHKERRQ(ierr);
      ierr = PetscStrlen(r[i],&l2);CHKERRQ(ierr);
      ierr = PetscStrlen(par,&l3);CHKERRQ(ierr);
      if (l1 + l2 + l3 >= len) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_SIZ,"b len is not long enough to hold new values");
      ierr = PetscStrcpy(work,b);CHKERRQ(ierr);
      ierr = PetscStrcat(work,r[i]);CHKERRQ(ierr);
      ierr = PetscStrcat(work,par);CHKERRQ(ierr);
      ierr = PetscStrncpy(b,work,len);CHKERRQ(ierr);
      ierr = PetscStrstr(b,s[i],&par);CHKERRQ(ierr);
    }
    i++;
  }
  i = 0;
  while (r[i]) {
    tfree = (char*)r[i];
    ierr  = PetscFree(tfree);CHKERRQ(ierr);
    i++;
  }

  /* look for any other ${xxx} strings to replace from environmental variables */
  ierr = PetscStrstr(b,"${",&par);CHKERRQ(ierr);
  while (par) {
    *par  = 0;
    par  += 2;
    ierr  = PetscStrcpy(work,b);CHKERRQ(ierr);
    ierr  = PetscStrstr(par,"}",&epar);CHKERRQ(ierr);
    *epar = 0;
    epar += 1;
    ierr  = PetscOptionsGetenv(comm,par,env,256,&flag);CHKERRQ(ierr);
    if (!flag) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONG,"Substitution string ${%s} not found as environmental variable",par);
    ierr = PetscStrcat(work,env);CHKERRQ(ierr);
    ierr = PetscStrcat(work,epar);CHKERRQ(ierr);
    ierr = PetscStrcpy(b,work);CHKERRQ(ierr);
    ierr = PetscStrstr(b,"${",&par);CHKERRQ(ierr);
  }
  ierr = PetscFree(work);CHKERRQ(ierr);
  if (aa == b) {
    ierr = PetscFree(a);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscEListFind"
/*@C
   PetscEListFind - searches list of strings for given string, using case insensitive matching

   Not Collective

   Input Parameters:
+  n - number of strings in
.  list - list of strings to search
-  str - string to look for, empty string "" accepts default (first entry in list)

   Output Parameters:
+  value - index of matching string (if found)
-  found - boolean indicating whether string was found (can be NULL)

   Notes:
   Not for use in Fortran

   Level: advanced
@*/
PetscErrorCode PetscEListFind(PetscInt n,const char *const *list,const char *str,PetscInt *value,PetscBool *found)
{
  PetscErrorCode ierr;
  PetscBool matched;
  PetscInt i;

  PetscFunctionBegin;
  if (found) *found = PETSC_FALSE;
  for (i=0; i<n; i++) {
    ierr = PetscStrcasecmp(str,list[i],&matched);CHKERRQ(ierr);
    if (matched || !str[0]) {
      if (found) *found = PETSC_TRUE;
      *value = i;
      break;
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscEnumFind"
/*@C
   PetscEListFind - searches enum list of strings for given string, using case insensitive matching

   Not Collective

   Input Parameters:
+  enumlist - list of strings to search, followed by enum name, then enum prefix, then NUL
-  str - string to look for

   Output Parameters:
+  value - index of matching string (if found)
-  found - boolean indicating whether string was found (can be NULL)

   Notes:
   Not for use in Fortran

   Level: advanced
@*/
PetscErrorCode PetscEnumFind(const char *const *enumlist,const char *str,PetscEnum *value,PetscBool *found)
{
  PetscErrorCode ierr;
  PetscInt n = 0,evalue;
  PetscBool efound;

  PetscFunctionBegin;
  while (enumlist[n++]) if (n > 50) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONG,"List argument appears to be wrong or have more than 50 entries");
  if (n < 3) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONG,"List argument must have at least two entries: typename and type prefix");
  n -= 3; /* drop enum name, prefix, and null termination */
  ierr = PetscEListFind(n,enumlist,str,&evalue,&efound);CHKERRQ(ierr);
  if (efound) *value = (PetscEnum)evalue;
  if (found) *found = efound;
  PetscFunctionReturn(0);
}
