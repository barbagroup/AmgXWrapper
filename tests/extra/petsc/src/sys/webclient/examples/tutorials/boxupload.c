
/*
    Run with -box_refresh_token XXX to allow access to Box or else it will prompt you to enter log in information for Box.

    Have not yet written the code to actually upload files

*/

#include <petscsys.h>

int main(int argc,char **argv)
{
  PetscErrorCode ierr;
  char           access_token[512],new_refresh_token[512];

  PetscInitialize(&argc,&argv,NULL,NULL);

  ierr = PetscBoxRefresh(PETSC_COMM_WORLD,NULL,access_token,new_refresh_token,sizeof(access_token));CHKERRQ(ierr);
  PetscFinalize();
  return 0;
}


