
/*
     Obtains a refresh token that you can use in the future to access Google Drive from PETSc code

     Guard the refresh token like a password.

     You can run PETSc programs with -google_refresh_token XXXX where XXX is the refresh token to access your Google Drive

*/

#include <petscsys.h>

int main(int argc,char **argv)
{
  PetscErrorCode ierr;
  char           access_token[512],refresh_token[512];

  PetscInitialize(&argc,&argv,NULL,NULL);
  ierr = PetscGoogleDriveAuthorize(PETSC_COMM_WORLD,access_token,refresh_token,sizeof(access_token));CHKERRQ(ierr);
  ierr = PetscPrintf(PETSC_COMM_WORLD,"Your Refresh token is %s\n",refresh_token);CHKERRQ(ierr);
  PetscFinalize();
  return 0;
}


