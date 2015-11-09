
static char help[] = "Tests DMDAGetElements() and VecView() contour plotting for 2d DMDAs.\n\n";

#include <petscdm.h>
#include <petscdmda.h>
#include <petscdraw.h>

#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc,char **argv)
{
  PetscInt         M = 10,N = 8,m = PETSC_DECIDE,n = PETSC_DECIDE,ne,nc,i;
  const PetscInt   *e;
  PetscErrorCode   ierr;
  PetscBool        flg = PETSC_FALSE;
  DM               da;
  PetscViewer      viewer;
  Vec              local,global;
  PetscScalar      value;
  DMBoundaryType   bx    = DM_BOUNDARY_NONE,by = DM_BOUNDARY_NONE;
  DMDAStencilType  stype = DMDA_STENCIL_BOX;
  PetscScalar      *lv;

  ierr = PetscInitialize(&argc,&argv,(char*)0,help);CHKERRQ(ierr);
  ierr = PetscViewerDrawOpen(PETSC_COMM_WORLD,0,"",300,0,300,300,&viewer);CHKERRQ(ierr);

  /* Read options */
  ierr = PetscOptionsGetInt(NULL,NULL,"-M",&M,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsGetInt(NULL,NULL,"-N",&N,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsGetInt(NULL,NULL,"-m",&m,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsGetInt(NULL,NULL,"-n",&n,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsGetBool(NULL,NULL,"-star_stencil",&flg,NULL);CHKERRQ(ierr);
  if (flg) stype = DMDA_STENCIL_STAR;

  /* Create distributed array and get vectors */
  ierr = DMDACreate2d(PETSC_COMM_WORLD,bx,by,stype,M,N,m,n,1,1,NULL,NULL,&da);CHKERRQ(ierr);
  ierr = DMCreateGlobalVector(da,&global);CHKERRQ(ierr);
  ierr = DMCreateLocalVector(da,&local);CHKERRQ(ierr);

  value = -3.0;
  ierr  = VecSet(global,value);CHKERRQ(ierr);
  ierr  = DMGlobalToLocalBegin(da,global,INSERT_VALUES,local);CHKERRQ(ierr);
  ierr  = DMGlobalToLocalEnd(da,global,INSERT_VALUES,local);CHKERRQ(ierr);

  ierr = DMDASetElementType(da,DMDA_ELEMENT_P1);CHKERRQ(ierr);
  ierr = DMDAGetElements(da,&ne,&nc,&e);CHKERRQ(ierr);
  ierr = VecGetArray(local,&lv);CHKERRQ(ierr);
  for (i=0; i<ne; i++) {
    ierr       = PetscPrintf(PETSC_COMM_WORLD,"i %D e[3*i] %D %D %D\n",i,e[3*i],e[3*i+1],e[3*i+2]);
    lv[e[3*i]] = i;
  }
  ierr = VecRestoreArray(local,&lv);CHKERRQ(ierr);
  ierr = DMDARestoreElements(da,&ne,&nc,&e);CHKERRQ(ierr);

  ierr = DMLocalToGlobalBegin(da,local,ADD_VALUES,global);CHKERRQ(ierr);
  ierr = DMLocalToGlobalEnd(da,local,ADD_VALUES,global);CHKERRQ(ierr);

  ierr = DMView(da,viewer);CHKERRQ(ierr);
  ierr = VecView(global,viewer);CHKERRQ(ierr);

  /* Free memory */
  ierr = PetscViewerDestroy(&viewer);CHKERRQ(ierr);
  ierr = VecDestroy(&local);CHKERRQ(ierr);
  ierr = VecDestroy(&global);CHKERRQ(ierr);
  ierr = DMDestroy(&da);CHKERRQ(ierr);
  ierr = PetscFinalize();
  return 0;
}

