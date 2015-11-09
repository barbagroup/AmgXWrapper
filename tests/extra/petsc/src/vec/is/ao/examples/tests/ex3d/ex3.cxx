
static char help[] = "AO test contributed by Sebastian Steiger <steiger@purdue.edu>, March 2011\n\n";

/*
  Example of usage:
    mpiexec -n 12 ./ex3
    mpiexec -n 30 ./ex3 -ao_type basic
*/

#include <iostream>
#include <fstream>
#include <vector>
#include <assert.h>
#include <petsc.h>

using namespace std;

int main(int argc, char** argv)
{
  PetscErrorCode ierr;
  AO ao;
  IS isapp;
  char infile[PETSC_MAX_PATH_LEN],datafiles[PETSC_MAX_PATH_LEN];
  PetscBool flg;

  PetscInitialize(&argc, &argv, (char*)0, help);
  int size=-1;   MPI_Comm_size(PETSC_COMM_WORLD, &size);
  int myrank=-1; MPI_Comm_rank(PETSC_COMM_WORLD, &myrank);

  ierr = PetscOptionsGetString(NULL,NULL,"-datafiles",datafiles,sizeof(datafiles),&flg);CHKERRQ(ierr);
  if (!flg) SETERRQ(PETSC_COMM_WORLD,PETSC_ERR_USER,"Must specify -datafiles ${DATAFILESPATH}/ao");

  // read in application indices
  ierr = PetscSNPrintf(infile,sizeof(infile),"%s/AO%dCPUs/ao_p%d_appindices.txt",datafiles,size,myrank);CHKERRQ(ierr);
  //cout << infile << endl;
  ifstream fin(infile);
  if (!fin) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_FILE_OPEN,"File not found: %s",infile);
  vector<int>  myapp;
  int tmp=-1;
  while (!fin.eof()) {
    tmp=-1;
    fin >> tmp;
    if (tmp==-1) break;
    myapp.push_back(tmp);
  }
  ierr = PetscSynchronizedPrintf(PETSC_COMM_WORLD,"[%d] has %D indices.\n",
          myrank,myapp.size());CHKERRQ(ierr);
  ierr = PetscSynchronizedFlush(PETSC_COMM_WORLD,PETSC_STDOUT);CHKERRQ(ierr);

  ierr = ISCreateGeneral(PETSC_COMM_WORLD, myapp.size(), &(myapp[0]), PETSC_USE_POINTER, &isapp);CHKERRQ(ierr);
  //ierr = ISView(isapp,PETSC_VIEWER_STDOUT_WORLD);CHKERRQ(ierr);

  ierr = AOCreate(PETSC_COMM_WORLD, &ao);CHKERRQ(ierr);
  ierr = AOSetIS(ao, isapp, NULL);CHKERRQ(ierr);
  ierr = AOSetType(ao, AOMEMORYSCALABLE);CHKERRQ(ierr);
  ierr = AOSetFromOptions(ao);CHKERRQ(ierr);

  if (myrank==0) cout << "AO has been set up." << endl;

  ierr = AODestroy(&ao);CHKERRQ(ierr);
  ierr = ISDestroy(&isapp);CHKERRQ(ierr);

  if (myrank==0) cout << "AO is done." << endl;

  PetscFinalize();
  return 0;
}
