
/*
      Code for manipulating files.
*/
#include <petscsys.h>
#if defined(PETSC_HAVE_WINDOWS_H)
#include <windows.h>
#endif

#if defined(PETSC_HAVE_GET_USER_NAME)
#undef __FUNCT__
#define __FUNCT__ "PetscGetUserName"
PetscErrorCode  PetscGetUserName(char name[],size_t nlen)
{
  PetscFunctionBegin;
  GetUserName((LPTSTR)name,(LPDWORD)(&nlen));
  PetscFunctionReturn(0);
}

#else
#undef __FUNCT__
#define __FUNCT__ "PetscGetUserName"
/*@C
    PetscGetUserName - Returns the name of the user.

    Not Collective

    Input Parameter:
    nlen - length of name

    Output Parameter:
.   name - contains user name.  Must be long enough to hold the name

    Level: developer

    Concepts: user name

.seealso: PetscGetHostName()
@*/
PetscErrorCode  PetscGetUserName(char name[],size_t nlen)
{
  const char     *user;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  user = getenv("USER");
  if (!user) user = "Unknown";
  ierr = PetscStrncpy(name,user,nlen);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}
#endif
