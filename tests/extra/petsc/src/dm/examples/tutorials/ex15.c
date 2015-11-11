
static char help[] = "Tests VecView() functionality with DMDA objects when using:"\
"(i) a PetscViewer binary with MPI-IO support; and (ii) when the binary header is skipped.\n\n";

#include <petscdm.h>
#include <petscdmda.h>

#define DMDA_I 5
#define DMDA_J 4
#define DMDA_K 6

const PetscReal dmda_i_val[] = { 1.10, 2.3006, 2.32444, 3.44006, 66.9009 };
const PetscReal dmda_j_val[] = { 0.0, 0.25, 0.5, 0.75 };
const PetscReal dmda_k_val[] = { 0.0, 1.1, 2.2, 3.3, 4.4, 5.5 };

#undef __FUNCT__
#define __FUNCT__ "MyVecDump"
PetscErrorCode MyVecDump(const char fname[],PetscBool skippheader,PetscBool usempiio,Vec x)
{
  MPI_Comm       comm;
  PetscViewer    viewer;
  PetscBool      ismpiio,isskip;
  PetscErrorCode ierr;

  PetscFunctionBeginUser;
  ierr = PetscObjectGetComm((PetscObject)x,&comm);CHKERRQ(ierr);

  ierr = PetscViewerCreate(comm,&viewer);CHKERRQ(ierr);
  ierr = PetscViewerSetType(viewer,PETSCVIEWERBINARY);CHKERRQ(ierr);
  if (skippheader) { ierr = PetscViewerBinarySetSkipHeader(viewer,PETSC_TRUE);CHKERRQ(ierr); }
  ierr = PetscViewerFileSetMode(viewer,FILE_MODE_WRITE);CHKERRQ(ierr);
  if (usempiio) { ierr = PetscViewerBinarySetUseMPIIO(viewer,PETSC_TRUE);CHKERRQ(ierr); }
  ierr = PetscViewerFileSetName(viewer,fname);CHKERRQ(ierr);

  ierr = VecView(x,viewer);CHKERRQ(ierr);

  ierr = PetscViewerBinaryGetUseMPIIO(viewer,&ismpiio);CHKERRQ(ierr);
  if (ismpiio) { ierr = PetscPrintf(comm,"*** PetscViewer[write] using MPI-IO ***\n");CHKERRQ(ierr); }
  ierr = PetscViewerBinaryGetSkipHeader(viewer,&isskip);CHKERRQ(ierr);
  if (isskip) { ierr = PetscPrintf(comm,"*** PetscViewer[write] skipping header ***\n");CHKERRQ(ierr); }

  ierr = PetscViewerDestroy(&viewer);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MyVecLoad"
PetscErrorCode MyVecLoad(const char fname[],PetscBool skippheader,PetscBool usempiio,Vec x)
{
  MPI_Comm       comm;
  PetscViewer    viewer;
  PetscBool      ismpiio,isskip;
  PetscErrorCode ierr;

  PetscFunctionBeginUser;
  ierr = PetscObjectGetComm((PetscObject)x,&comm);CHKERRQ(ierr);

  ierr = PetscViewerCreate(comm,&viewer);CHKERRQ(ierr);
  ierr = PetscViewerSetType(viewer,PETSCVIEWERBINARY);CHKERRQ(ierr);
  if (skippheader) { ierr = PetscViewerBinarySetSkipHeader(viewer,PETSC_TRUE);CHKERRQ(ierr); }
  ierr = PetscViewerFileSetMode(viewer,FILE_MODE_READ);CHKERRQ(ierr);
  if (usempiio) { ierr = PetscViewerBinarySetUseMPIIO(viewer,PETSC_TRUE);CHKERRQ(ierr); }
  ierr = PetscViewerFileSetName(viewer,fname);CHKERRQ(ierr);

  ierr = VecLoad(x,viewer);CHKERRQ(ierr);

  ierr = PetscViewerBinaryGetSkipHeader(viewer,&isskip);CHKERRQ(ierr);
  if (isskip) { ierr = PetscPrintf(comm,"*** PetscViewer[load] skipping header ***\n");CHKERRQ(ierr); }
  ierr = PetscViewerBinaryGetUseMPIIO(viewer,&ismpiio);CHKERRQ(ierr);
  if (ismpiio) { ierr = PetscPrintf(comm,"*** PetscViewer[load] using MPI-IO ***\n");CHKERRQ(ierr); }

  ierr = PetscViewerDestroy(&viewer);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMDAVecGenerateEntries"
PetscErrorCode DMDAVecGenerateEntries(DM dm,Vec a)
{
  PetscScalar    ****LA_v;
  PetscInt       i,j,k,si,sj,sk,ni,nj,nk,M,N;
  PetscErrorCode ierr;

  PetscFunctionBeginUser;
  ierr = DMDAGetInfo(dm,NULL,&M,&N,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);CHKERRQ(ierr);
  ierr = DMDAGetCorners(dm,&si,&sj,&sk,&ni,&nj,&nk);CHKERRQ(ierr);
  ierr = DMDAVecGetArrayDOF(dm,a,&LA_v);CHKERRQ(ierr);
  for (k=sk; k<sk+nk; k++) {
    for (j=sj; j<sj+nj; j++) {
      for (i=si; i<si+ni; i++) {
        PetscScalar test_value_s;
                
        test_value_s = dmda_i_val[i]*((PetscScalar)i) + dmda_j_val[j]*((PetscScalar)(i+j*M)) + dmda_k_val[k]*((PetscScalar)(i + j*M + k*M*N));
        LA_v[k][j][i][0] = 3.0 * test_value_s;
        LA_v[k][j][i][1] = 3.0 * test_value_s + 1.0;
        LA_v[k][j][i][2] = 3.0 * test_value_s + 2.0;
      }
    }
  }
  ierr = DMDAVecRestoreArrayDOF(dm,a,&LA_v);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "HeaderlessBinaryReadCheck"
PetscErrorCode HeaderlessBinaryReadCheck(DM dm,const char name[])
{
  PetscErrorCode ierr;
  int            fdes;
  PetscScalar    buffer[DMDA_I*DMDA_J*DMDA_K*3];
  PetscInt       len,d,i,j,k,M,N;
  PetscMPIInt    rank;
  PetscBool      dataverified = PETSC_TRUE;

  PetscFunctionBeginUser;
  ierr = MPI_Comm_rank(PETSC_COMM_WORLD,&rank);CHKERRQ(ierr);
  ierr = DMDAGetInfo(dm,NULL,&M,&N,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);CHKERRQ(ierr);
  len = DMDA_I*DMDA_J*DMDA_K*3;
  if (!rank) {
    ierr = PetscBinaryOpen(name,FILE_MODE_READ,&fdes);CHKERRQ(ierr);
    ierr = PetscBinaryRead(fdes,buffer,len,PETSC_SCALAR);CHKERRQ(ierr);
    ierr = PetscBinaryClose(fdes);CHKERRQ(ierr);

    for (k=0; k<DMDA_K; k++) {
      for (j=0; j<DMDA_J; j++) {
        for (i=0; i<DMDA_I; i++) {
          for (d=0; d<3; d++) {
            PetscScalar v,test_value_s,test_value;
            PetscInt    index;

            test_value_s = dmda_i_val[i]*((PetscScalar)i) + dmda_j_val[j]*((PetscScalar)(i+j*M)) + dmda_k_val[k]*((PetscScalar)(i + j*M + k*M*N));
            test_value = 3.0 * test_value_s + (PetscScalar)d;

            index = 3*(i + j*M + k*M*N) + d;
            v = PetscAbsScalar(test_value-buffer[index]);
#if defined(PETSC_USE_COMPLEX)
            if ((PetscRealPart(v) > 1.0e-10) || (PetscImaginaryPart(v) > 1.0e-10)) {
              ierr = PetscPrintf(PETSC_COMM_SELF,"ERROR: Difference > 1.0e-10 occurred (delta = (%+1.12e,%+1.12e) [loc %D,%D,%D(%D)])\n",(double)PetscRealPart(test_value),(double)PetscImaginaryPart(test_value),i,j,k,d);CHKERRQ(ierr);
              dataverified = PETSC_FALSE;
            }
#else
            if (PetscRealPart(v) > 1.0e-10) {
              ierr = PetscPrintf(PETSC_COMM_SELF,"ERROR: Difference > 1.0e-10 occurred (delta = %+1.12e [loc %D,%D,%D(%D)])\n",(double)PetscRealPart(test_value),i,j,k,d);CHKERRQ(ierr);
              dataverified = PETSC_FALSE;
            }
#endif
          }
        }
      }
    }
    if (dataverified) {
      ierr = PetscPrintf(PETSC_COMM_SELF,"Headerless read of data verified for: %s\n",name);CHKERRQ(ierr);
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecCompare"
PetscErrorCode VecCompare(Vec a,Vec b)
{
  PetscInt       locmin[2],locmax[2];
  PetscReal      min[2],max[2];
  Vec            ref;
  PetscErrorCode ierr;

  PetscFunctionBeginUser;
  ierr = VecMin(a,&locmin[0],&min[0]);CHKERRQ(ierr);
  ierr = VecMax(a,&locmax[0],&max[0]);CHKERRQ(ierr);

  ierr = VecMin(b,&locmin[1],&min[1]);CHKERRQ(ierr);
  ierr = VecMax(b,&locmax[1],&max[1]);CHKERRQ(ierr);

  ierr = PetscPrintf(PETSC_COMM_WORLD,"VecCompare\n");CHKERRQ(ierr);
  ierr = PetscPrintf(PETSC_COMM_WORLD,"  min(a)   = %+1.2e [loc %D]\n",(double)min[0],locmin[0]);CHKERRQ(ierr);
  ierr = PetscPrintf(PETSC_COMM_WORLD,"  max(a)   = %+1.2e [loc %D]\n",(double)max[0],locmax[0]);CHKERRQ(ierr);

  ierr = PetscPrintf(PETSC_COMM_WORLD,"  min(b)   = %+1.2e [loc %D]\n",(double)min[1],locmin[1]);CHKERRQ(ierr);
  ierr = PetscPrintf(PETSC_COMM_WORLD,"  max(b)   = %+1.2e [loc %D]\n",(double)max[1],locmax[1]);CHKERRQ(ierr);

  ierr = VecDuplicate(a,&ref);CHKERRQ(ierr);
  ierr = VecCopy(a,ref);CHKERRQ(ierr);
  ierr = VecAXPY(ref,-1.0,b);CHKERRQ(ierr);
  ierr = VecMin(ref,&locmin[0],&min[0]);CHKERRQ(ierr);
  if (PetscAbsReal(min[0]) > 1.0e-10) {
    ierr = PetscPrintf(PETSC_COMM_WORLD,"  ERROR: min(a-b) > 1.0e-10\n");CHKERRQ(ierr);
    ierr = PetscPrintf(PETSC_COMM_WORLD,"  min(a-b) = %+1.10e\n",(double)PetscAbsReal(min[0]));CHKERRQ(ierr);
  } else {
    ierr = PetscPrintf(PETSC_COMM_WORLD,"  min(a-b) < 1.0e-10\n");CHKERRQ(ierr);
  }
  ierr = VecDestroy(&ref);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TestDMDAVec"
PetscErrorCode TestDMDAVec(PetscBool usempiio)
{
  DM             dm;
  Vec            x_ref,x_test;
  PetscBool      skipheader = PETSC_TRUE;
  PetscErrorCode ierr;

  PetscFunctionBeginUser;
  if (!usempiio) { ierr = PetscPrintf(PETSC_COMM_WORLD,"%s\n",__FUNCT__);CHKERRQ(ierr); }
  else { ierr = PetscPrintf(PETSC_COMM_WORLD,"%s [using mpi-io]\n",__FUNCT__);CHKERRQ(ierr); }
  ierr = DMDACreate3d(PETSC_COMM_WORLD,DM_BOUNDARY_NONE,DM_BOUNDARY_NONE,DM_BOUNDARY_NONE,
                      DMDA_STENCIL_BOX,DMDA_I,DMDA_J,DMDA_K,PETSC_DECIDE,PETSC_DECIDE,PETSC_DECIDE,
                        3,2,NULL,NULL,NULL,&dm);CHKERRQ(ierr);

  ierr = DMCreateGlobalVector(dm,&x_ref);CHKERRQ(ierr);
  ierr = DMDAVecGenerateEntries(dm,x_ref);CHKERRQ(ierr);

  if (!usempiio) {
    ierr = MyVecDump("dmda.pbvec",skipheader,PETSC_FALSE,x_ref);CHKERRQ(ierr);
  } else {
    ierr = MyVecDump("dmda-mpiio.pbvec",skipheader,PETSC_TRUE,x_ref);CHKERRQ(ierr);
  }

  ierr = DMCreateGlobalVector(dm,&x_test);CHKERRQ(ierr);

  if (!usempiio) {
    ierr = MyVecLoad("dmda.pbvec",skipheader,usempiio,x_test);CHKERRQ(ierr);
  } else {
    ierr = MyVecLoad("dmda-mpiio.pbvec",skipheader,usempiio,x_test);CHKERRQ(ierr);
  }

  ierr = VecCompare(x_ref,x_test);CHKERRQ(ierr);

  if (!usempiio) {
    ierr = HeaderlessBinaryReadCheck(dm,"dmda.pbvec");CHKERRQ(ierr);
  } else {
    ierr = HeaderlessBinaryReadCheck(dm,"dmda-mpiio.pbvec");CHKERRQ(ierr);
  }
  ierr = VecDestroy(&x_ref);CHKERRQ(ierr);
  ierr = VecDestroy(&x_test);CHKERRQ(ierr);
  ierr = DMDestroy(&dm);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc,char **args)
{
  PetscErrorCode ierr;
  PetscBool      usempiio = PETSC_FALSE;

  PetscInitialize(&argc,&args,(char*)0,help);

  ierr = PetscOptionsGetBool(NULL,NULL,"-usempiio",&usempiio,NULL);CHKERRQ(ierr);
  if (!usempiio) {
    ierr = TestDMDAVec(PETSC_FALSE);CHKERRQ(ierr);
  } else {
#if defined(PETSC_HAVE_MPIIO)
    ierr = TestDMDAVec(PETSC_TRUE);CHKERRQ(ierr);
#else
    ierr = PetscPrintf(PETSC_COMM_WORLD,"Warning: Executing TestDMDAVec(PETSC_TRUE) requires a working MPI-2 implementation\n");CHKERRQ(ierr);
#endif
  }
  ierr = PetscFinalize();
  return 0;
}

