
static char help[] = "Tests PetscMemmove()\n";

#include <petscsys.h>

#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc,char **argv)
{
  int i,*a,*b,ierr;
  PetscInitialize(&argc,&argv,(char*)0,help);

  ierr = PetscMalloc1(10,&a);CHKERRQ(ierr);
  ierr = PetscMalloc1(20,&b);CHKERRQ(ierr);

  /*
      Nonoverlapping regions
  */
  for (i=0; i<20; i++) b[i] = i;
  ierr = PetscMemmove(a,b,10*sizeof(int));CHKERRQ(ierr);
  PetscIntView(10,a,0);

  ierr = PetscFree(a);CHKERRQ(ierr);

  /*
     |        |                |       |
     b        a               b+15    b+20
                              a+10    a+15
  */
  a    = b + 5;
  ierr = PetscMemmove(a,b,15*sizeof(int));CHKERRQ(ierr);
  PetscIntView(15,a,0);
  ierr = PetscFree(b);CHKERRQ(ierr);

  /*
     |       |                    |       |
     a       b                   a+20   a+25
                                        b+20
  */
  ierr = PetscMalloc1(25,&a);CHKERRQ(ierr);
  b    = a + 5;
  for (i=0; i<20; i++) b[i] = i;
  ierr = PetscMemmove(a,b,20*sizeof(int));CHKERRQ(ierr);
  PetscIntView(20,a,0);
  ierr = PetscFree(a);CHKERRQ(ierr);

  ierr = PetscFinalize();
  return 0;
}

