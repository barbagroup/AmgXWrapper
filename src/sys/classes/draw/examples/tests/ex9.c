
static char help[] = "Makes a simple histogram.\n";

#include <petscsys.h>
#include <petscdraw.h>

#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc,char **argv)
{
  PetscDraw      draw;
  PetscDrawHG    hist;
  PetscDrawAxis  axis;
  PetscErrorCode ierr;
  int            n     = 20,i,x = 0,y = 0,width = 300,height = 300,bins = 8;
  PetscInt       w     = 300,h = 300,nn = 20,b = 8,c = PETSC_DRAW_GREEN;
  int            color = PETSC_DRAW_GREEN;
  const char     *xlabel,*ylabel,*toplabel;
  PetscReal      xd;
  PetscBool      flg;

  xlabel = "X-axis Label";toplabel = "Top Label";ylabel = "Y-axis Label";

  ierr  = PetscInitialize(&argc,&argv,(char*)0,help);CHKERRQ(ierr);
  ierr  = PetscOptionsGetInt(NULL,NULL,"-width",&w,NULL);CHKERRQ(ierr);
  ierr  = PetscOptionsGetInt(NULL,NULL,"-height",&h,NULL);CHKERRQ(ierr);
  ierr  = PetscOptionsGetInt(NULL,NULL,"-n",&nn,NULL);CHKERRQ(ierr);
  ierr  = PetscOptionsGetInt(NULL,NULL,"-bins",&b,NULL);CHKERRQ(ierr);
  ierr  = PetscOptionsGetInt(NULL,NULL,"-color",&c,NULL);CHKERRQ(ierr);
  ierr  = PetscOptionsHasName(NULL,NULL,"-nolabels",&flg);CHKERRQ(ierr);
  width = (int) w; height = (int)h; n = (int)nn; bins = (int) b; color = (int) c;
  if (flg) {
    xlabel = (char*)0; toplabel = (char*)0;
  }
  ierr = PetscDrawCreate(PETSC_COMM_WORLD,0,"Title",x,y,width,height,&draw);CHKERRQ(ierr);
  ierr = PetscDrawSetFromOptions(draw);CHKERRQ(ierr);
  ierr = PetscDrawHGCreate(draw,bins,&hist);CHKERRQ(ierr);
  ierr = PetscDrawHGGetAxis(hist,&axis);CHKERRQ(ierr);
  ierr = PetscDrawAxisSetColors(axis,PETSC_DRAW_BLACK,PETSC_DRAW_RED,PETSC_DRAW_BLUE);CHKERRQ(ierr);
  ierr = PetscDrawAxisSetLabels(axis,toplabel,xlabel,ylabel);CHKERRQ(ierr);

  for (i=0; i<n; i++) {
    xd   = (PetscReal)(i - 5);
    ierr = PetscDrawHGAddValue(hist,xd*xd);CHKERRQ(ierr);
  }
  ierr = PetscDrawHGSetColor(hist,color);CHKERRQ(ierr);
  ierr = PetscDrawHGDraw(hist);CHKERRQ(ierr);

  ierr = PetscDrawHGDestroy(&hist);CHKERRQ(ierr);
  ierr = PetscDrawDestroy(&draw);CHKERRQ(ierr);
  ierr = PetscFinalize();
  return 0;
}

