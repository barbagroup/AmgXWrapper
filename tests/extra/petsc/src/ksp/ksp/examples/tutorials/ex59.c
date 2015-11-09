static char help[] =  "This example illustrates the use of PCBDDC/FETI-DP and its customization.\n\n\
Discrete system: 1D, 2D or 3D laplacian, discretized with spectral elements.\n\
Spectral degree can be specified by passing values to -p option.\n\
Global problem either with dirichlet boundary conditions on one side or in the pure neumann case (depending on runtime parameters).\n\
Domain is [-nex,nex]x[-ney,ney]x[-nez,nez]: ne_ number of elements in _ direction.\n\
Exaple usage: \n\
1D: mpiexec -n 4 ex59 -nex 7\n\
2D: mpiexec -n 4 ex59 -npx 2 -npy 2 -nex 2 -ney 2\n\
3D: mpiexec -n 4 ex59 -npx 2 -npy 2 -npz 1 -nex 2 -ney 2 -nez 1\n\
Subdomain decomposition can be specified with -np_ parameters.\n\
Dirichlet boundaries on one side by default:\n\
it does not iterate on dirichlet nodes by default: if -usezerorows is passed in, it also iterates on Dirichlet nodes.\n\
Pure Neumann case can be requested by passing in -pureneumann.\n\
In the latter case, in order to avoid runtime errors during factorization, please specify also -coarse_redundant_pc_factor_zeropivot 0\n\n";

#include <petscksp.h>
#include <petscpc.h>
#include <petscdm.h>
#include <petscdmda.h>
#include <petscblaslapack.h>
#define DEBUG 0

/* structure holding domain data */
typedef struct {
  /* communicator */
  MPI_Comm gcomm;
  /* space dimension */
  PetscInt dim;
  /* spectral degree */
  PetscInt p;
  /* subdomains per dimension */
  PetscInt npx,npy,npz;
  /* subdomain index in cartesian dimensions */
  PetscInt ipx,ipy,ipz;
  /* elements per dimension */
  PetscInt nex,ney,nez;
  /* local elements per dimension */
  PetscInt nex_l,ney_l,nez_l;
  /* global number of dofs per dimension */
  PetscInt xm,ym,zm;
  /* local number of dofs per dimension */
  PetscInt xm_l,ym_l,zm_l;
  /* starting global indexes for subdomain in lexicographic ordering */
  PetscInt startx,starty,startz;
  /* Is is a pure Neumann problem? */
  PetscBool pure_neumann;
  /* Dirichlet BC implementation */
  PetscBool DBC_zerorows;
  /* Scaling factor for subdomain */
  PetscScalar scalingfactor;
} DomainData;

/* structure holding GLL data */
typedef struct {
  /* GLL nodes */
  PetscReal   *zGL;
  /* GLL weights */
  PetscScalar *rhoGL;
  /* aux_mat */
  PetscScalar **A;
  /* Element matrix */
  Mat elem_mat;
} GLLData;


#undef __FUNCT__
#define __FUNCT__ "BuildCSRGraph"
static PetscErrorCode BuildCSRGraph(DomainData dd, PetscInt **xadj, PetscInt **adjncy)
{
  PetscErrorCode ierr;
  PetscInt       *xadj_temp,*adjncy_temp;
  PetscInt       i,j,k,ii,jj,kk,iindex,count_adj;
  PetscInt       istart_csr,iend_csr,jstart_csr,jend_csr,kstart_csr,kend_csr;
  PetscBool      internal_node;

  /* first count dimension of adjncy */
  count_adj=0;
  for (k=0; k<dd.zm_l; k++) {
    internal_node = PETSC_TRUE;
    kstart_csr    =1;
    kend_csr      =dd.zm_l-1;
    if (k == 0 || k == dd.zm_l-1) {
      internal_node = PETSC_FALSE;
      kstart_csr    = k;
      kend_csr      = k+1;
    }
    for (j=0; j<dd.ym_l; j++) {
      jstart_csr=1;
      jend_csr  =dd.ym_l-1;
      if (j == 0 || j == dd.ym_l-1) {
        internal_node = PETSC_FALSE;
        jstart_csr    = j;
        jend_csr      = j+1;
      }
      for (i=0; i<dd.xm_l; i++) {
        istart_csr = 1;
        iend_csr   = dd.xm_l-1;
        if (i == 0 || i == dd.xm_l-1) {
          internal_node = PETSC_FALSE;
          istart_csr    = i;
          iend_csr      = i+1;
        }
        iindex=k*dd.xm_l*dd.ym_l+j*dd.xm_l+i;
        if (internal_node) {
          istart_csr = i;
          iend_csr   = i+1;
          jstart_csr = j;
          jend_csr   = j+1;
          kstart_csr = k;
          kend_csr   = k+1;
        }
        for (kk=kstart_csr;kk<kend_csr;kk++) {
          for (jj=jstart_csr;jj<jend_csr;jj++) {
            for (ii=istart_csr;ii<iend_csr;ii++) {
              count_adj=count_adj+1;
            }
          }
        }
      }
    }
  }
  ierr = PetscMalloc1(dd.xm_l*dd.ym_l*dd.zm_l+1,&xadj_temp);CHKERRQ(ierr);
  ierr = PetscMalloc1(count_adj,&adjncy_temp);CHKERRQ(ierr);

  /* now fill CSR data structure */
  count_adj=0;
  for (k=0; k<dd.zm_l; k++) {
    internal_node = PETSC_TRUE;
    kstart_csr    = 1;
    kend_csr      = dd.zm_l-1;
    if (k == 0 || k == dd.zm_l-1) {
      internal_node = PETSC_FALSE;
      kstart_csr    = k;
      kend_csr      = k+1;
    }
    for (j=0; j<dd.ym_l; j++) {
      jstart_csr = 1;
      jend_csr   = dd.ym_l-1;
      if (j == 0 || j == dd.ym_l-1) {
        internal_node = PETSC_FALSE;
        jstart_csr    = j;
        jend_csr      = j+1;
      }
      for (i=0; i<dd.xm_l; i++) {
        istart_csr = 1;
        iend_csr   = dd.xm_l-1;
        if (i == 0 || i == dd.xm_l-1) {
          internal_node = PETSC_FALSE;
          istart_csr    = i;
          iend_csr      = i+1;
        }
        iindex            = k*dd.xm_l*dd.ym_l+j*dd.xm_l+i;
        xadj_temp[iindex] = count_adj;
        if (internal_node) {
          istart_csr = i;
          iend_csr   = i+1;
          jstart_csr = j;
          jend_csr   = j+1;
          kstart_csr = k;
          kend_csr   = k+1;
        }
        for (kk=kstart_csr; kk<kend_csr; kk++) {
          for (jj=jstart_csr; jj<jend_csr; jj++) {
            for (ii=istart_csr; ii<iend_csr; ii++) {
              iindex = kk*dd.xm_l*dd.ym_l+jj*dd.xm_l+ii;

              adjncy_temp[count_adj] = iindex;
              count_adj              = count_adj+1;
            }
          }
        }
      }
    }
  }
  xadj_temp[dd.xm_l*dd.ym_l*dd.zm_l] = count_adj;

  *xadj   = xadj_temp;
  *adjncy = adjncy_temp;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "ComputeSpecialBoundaryIndices"
static PetscErrorCode ComputeSpecialBoundaryIndices(DomainData dd,IS *dirichlet,IS *neumann)
{
  PetscErrorCode ierr;
  IS             temp_dirichlet=0,temp_neumann=0;
  PetscInt       localsize,i,j,k,*indices;
  PetscBool      *touched;

  PetscFunctionBeginUser;
  localsize = dd.xm_l*dd.ym_l*dd.zm_l;

  ierr = PetscMalloc1(localsize,&indices);CHKERRQ(ierr);
  ierr = PetscMalloc1(localsize,&touched);CHKERRQ(ierr);
  for (i=0; i<localsize; i++) touched[i] = PETSC_FALSE;

  if (dirichlet) {
    i = 0;
    /* west boundary */
    if (dd.ipx == 0) {
      for (k=0;k<dd.zm_l;k++) {
        for (j=0;j<dd.ym_l;j++) {
          indices[i]=k*dd.ym_l*dd.xm_l+j*dd.xm_l;
          touched[indices[i]]=PETSC_TRUE;
          i++;
        }
      }
    }
    ierr = ISCreateGeneral(dd.gcomm,i,indices,PETSC_COPY_VALUES,&temp_dirichlet);CHKERRQ(ierr);
  }
  if (neumann) {
    i = 0;
    /* west boundary */
    if (dd.ipx == 0) {
      for (k=0;k<dd.zm_l;k++) {
        for (j=0;j<dd.ym_l;j++) {
          indices[i]=k*dd.ym_l*dd.xm_l+j*dd.xm_l;
          if (!touched[indices[i]]) {
            touched[indices[i]]=PETSC_TRUE;
            i++;
          }
        }
      }
    }
    /* east boundary */
    if (dd.ipx == dd.npx-1) {
      for (k=0;k<dd.zm_l;k++) {
        for (j=0;j<dd.ym_l;j++) {
          indices[i]=k*dd.ym_l*dd.xm_l+j*dd.xm_l+dd.xm_l-1;
          if (!touched[indices[i]]) {
            touched[indices[i]]=PETSC_TRUE;
            i++;
          }
        }
      }
    }
    /* south boundary */
    if (dd.ipy == 0 && dd.dim > 1) {
      for (k=0;k<dd.zm_l;k++) {
        for (j=0;j<dd.xm_l;j++) {
          indices[i]=k*dd.ym_l*dd.xm_l+j;
          if (!touched[indices[i]]) {
            touched[indices[i]]=PETSC_TRUE;
            i++;
          }
        }
      }
    }
    /* north boundary */
    if (dd.ipy == dd.npy-1 && dd.dim > 1) {
      for (k=0;k<dd.zm_l;k++) {
        for (j=0;j<dd.xm_l;j++) {
          indices[i]=k*dd.ym_l*dd.xm_l+(dd.ym_l-1)*dd.xm_l+j;
          if (!touched[indices[i]]) {
            touched[indices[i]]=PETSC_TRUE;
            i++;
          }
        }
      }
    }
    /* bottom boundary */
    if (dd.ipz == 0 && dd.dim > 2) {
      for (k=0;k<dd.ym_l;k++) {
        for (j=0;j<dd.xm_l;j++) {
          indices[i]=k*dd.xm_l+j;
          if (!touched[indices[i]]) {
            touched[indices[i]]=PETSC_TRUE;
            i++;
          }
        }
      }
    }
    /* top boundary */
    if (dd.ipz == dd.npz-1 && dd.dim > 2) {
      for (k=0;k<dd.ym_l;k++) {
        for (j=0;j<dd.xm_l;j++) {
          indices[i]=(dd.zm_l-1)*dd.ym_l*dd.xm_l+k*dd.xm_l+j;
          if (!touched[indices[i]]) {
            touched[indices[i]]=PETSC_TRUE;
            i++;
          }
        }
      }
    }
    ierr = ISCreateGeneral(dd.gcomm,i,indices,PETSC_COPY_VALUES,&temp_neumann);
  }
  if (dirichlet) *dirichlet = temp_dirichlet;
  if (neumann) *neumann = temp_neumann;
  ierr = PetscFree(indices);CHKERRQ(ierr);
  ierr = PetscFree(touched);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "ComputeMapping"
static PetscErrorCode ComputeMapping(DomainData dd,ISLocalToGlobalMapping *isg2lmap)
{
  PetscErrorCode         ierr;
  DM                     da;
  AO                     ao;
  DMBoundaryType         bx = DM_BOUNDARY_NONE,by = DM_BOUNDARY_NONE, bz = DM_BOUNDARY_NONE;
  DMDAStencilType        stype = DMDA_STENCIL_BOX;
  ISLocalToGlobalMapping temp_isg2lmap;
  PetscInt               i,j,k,ig,jg,kg,lindex,gindex,localsize;
  PetscInt               *global_indices;

  PetscFunctionBeginUser;
  /* Not an efficient mapping: this function computes a very simple lexicographic mapping
     just to illustrate the creation of a MATIS object */
  localsize = dd.xm_l*dd.ym_l*dd.zm_l;
  ierr      = PetscMalloc1(localsize,&global_indices);CHKERRQ(ierr);
  for (k=0; k<dd.zm_l; k++) {
    kg=dd.startz+k;
    for (j=0; j<dd.ym_l; j++) {
      jg=dd.starty+j;
      for (i=0; i<dd.xm_l; i++) {
        ig                    =dd.startx+i;
        lindex                =k*dd.xm_l*dd.ym_l+j*dd.xm_l+i;
        gindex                =kg*dd.xm*dd.ym+jg*dd.xm+ig;
        global_indices[lindex]=gindex;
      }
    }
  }
  if (dd.dim==3) {
    ierr = DMDACreate3d(dd.gcomm,bx,by,bz,stype,dd.xm,dd.ym,dd.zm,PETSC_DECIDE,PETSC_DECIDE,PETSC_DECIDE,1,1,NULL,NULL,NULL,&da);CHKERRQ(ierr);
  } else if (dd.dim==2) {
    ierr = DMDACreate2d(dd.gcomm,bx,by,stype,dd.xm,dd.ym,PETSC_DECIDE,PETSC_DECIDE,1,1,NULL,NULL,&da);CHKERRQ(ierr);
  } else {
    ierr = DMDACreate1d(dd.gcomm,bx,dd.xm,1,1,NULL,&da);CHKERRQ(ierr);
  }
  ierr = DMDASetAOType(da,AOMEMORYSCALABLE);CHKERRQ(ierr);
  ierr = DMDAGetAO(da,&ao);CHKERRQ(ierr);
  ierr = AOApplicationToPetsc(ao,dd.xm_l*dd.ym_l*dd.zm_l,global_indices);CHKERRQ(ierr);
  ierr = ISLocalToGlobalMappingCreate(dd.gcomm,1,localsize,global_indices,PETSC_OWN_POINTER,&temp_isg2lmap);
  ierr = DMDestroy(&da);CHKERRQ(ierr);
  *isg2lmap = temp_isg2lmap;
  PetscFunctionReturn(0);
}
#undef __FUNCT__
#define __FUNCT__ "ComputeSubdomainMatrix"
static PetscErrorCode ComputeSubdomainMatrix(DomainData dd, GLLData glldata, Mat *local_mat)
{
  PetscErrorCode ierr;
  PetscInt       localsize,zloc,yloc,xloc,auxnex,auxney,auxnez;
  PetscInt       ie,je,ke,i,j,k,ig,jg,kg,ii,ming;
  PetscInt       *indexg,*cols,*colsg;
  PetscScalar    *vals;
  Mat            temp_local_mat,elem_mat_DBC=0,*usedmat;
  IS             submatIS;

  PetscFunctionBeginUser;
  ierr = MatGetSize(glldata.elem_mat,&i,&j);CHKERRQ(ierr);
  ierr = PetscMalloc1(i,&indexg);CHKERRQ(ierr);
  ierr = PetscMalloc1(i,&colsg);CHKERRQ(ierr);
  /* get submatrix of elem_mat without dirichlet nodes */
  if (!dd.pure_neumann && !dd.DBC_zerorows && !dd.ipx) {
    xloc = dd.p+1;
    yloc = 1;
    zloc = 1;
    if (dd.dim>1) yloc = dd.p+1;
    if (dd.dim>2) zloc = dd.p+1;
    ii = 0;
    for (k=0;k<zloc;k++) {
      for (j=0;j<yloc;j++) {
        for (i=1;i<xloc;i++) {
          indexg[ii]=k*xloc*yloc+j*xloc+i;
          ii++;
        }
      }
    }
    ierr = ISCreateGeneral(PETSC_COMM_SELF,ii,indexg,PETSC_COPY_VALUES,&submatIS);CHKERRQ(ierr);
    ierr = MatGetSubMatrix(glldata.elem_mat,submatIS,submatIS,MAT_INITIAL_MATRIX,&elem_mat_DBC);CHKERRQ(ierr);
    ierr = ISDestroy(&submatIS);CHKERRQ(ierr);
  }

  /* Assemble subdomain matrix */
  localsize = dd.xm_l*dd.ym_l*dd.zm_l;
  ierr      = MatCreate(PETSC_COMM_SELF,&temp_local_mat);CHKERRQ(ierr);
  ierr      = MatSetSizes(temp_local_mat,localsize,localsize,localsize,localsize);CHKERRQ(ierr);
  /* set local matrices type: here we use SEQSBAIJ primarily for testing purpose */
  /* in order to avoid conversions inside the BDDC code, use SeqAIJ if possible */
  if (dd.DBC_zerorows && !dd.ipx) { /* in this case, we need to zero out some of the rows, so use seqaij */
    ierr      = MatSetType(temp_local_mat,MATSEQAIJ);CHKERRQ(ierr);
  } else {
    ierr      = MatSetType(temp_local_mat,MATSEQSBAIJ);CHKERRQ(ierr);
  }

  i = PetscPowRealInt(3.0*(dd.p+1.0),dd.dim);

  ierr = MatSeqAIJSetPreallocation(temp_local_mat,i,NULL);CHKERRQ(ierr);      /* very overestimated */
  ierr = MatSeqSBAIJSetPreallocation(temp_local_mat,1,i,NULL);CHKERRQ(ierr);      /* very overestimated */
  ierr = MatSetOption(temp_local_mat,MAT_KEEP_NONZERO_PATTERN,PETSC_TRUE);CHKERRQ(ierr);

  yloc = dd.p+1;
  zloc = dd.p+1;
  if (dd.dim < 3) zloc = 1;
  if (dd.dim < 2) yloc = 1;

  auxnez = dd.nez_l;
  auxney = dd.ney_l;
  auxnex = dd.nex_l;
  if (dd.dim < 3) auxnez = 1;
  if (dd.dim < 2) auxney = 1;

  for (ke=0; ke<auxnez; ke++) {
    for (je=0; je<auxney; je++) {
      for (ie=0; ie<auxnex; ie++) {
        /* customize element accounting for BC */
        xloc    = dd.p+1;
        ming    = 0;
        usedmat = &glldata.elem_mat;
        if (!dd.pure_neumann && !dd.DBC_zerorows && !dd.ipx) {
          if (ie == 0) {
            xloc    = dd.p;
            usedmat = &elem_mat_DBC;
          } else {
            ming    = -1;
            usedmat = &glldata.elem_mat;
          }
        }
        /* local to the element/global to the subdomain indexing */
        for (k=0; k<zloc; k++) {
          kg = ke*dd.p+k;
          for (j=0; j<yloc; j++) {
            jg = je*dd.p+j;
            for (i=0; i<xloc; i++) {
              ig         = ie*dd.p+i+ming;
              ii         = k*xloc*yloc+j*xloc+i;
              indexg[ii] = kg*dd.xm_l*dd.ym_l+jg*dd.xm_l+ig;
            }
          }
        }
        /* Set values */
        for (i=0; i<xloc*yloc*zloc; i++) {
          ierr = MatGetRow(*usedmat,i,&j,(const PetscInt**)&cols,(const PetscScalar**)&vals);CHKERRQ(ierr);
          for (k=0; k<j; k++) colsg[k] = indexg[cols[k]];
          ierr = MatSetValues(temp_local_mat,1,&indexg[i],j,colsg,vals,ADD_VALUES);CHKERRQ(ierr);
          ierr = MatRestoreRow(*usedmat,i,&j,(const PetscInt**)&cols,(const PetscScalar**)&vals);CHKERRQ(ierr);
        }
      }
    }
  }
  ierr = PetscFree(indexg);CHKERRQ(ierr);
  ierr = PetscFree(colsg);CHKERRQ(ierr);
  ierr = MatAssemblyBegin(temp_local_mat,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd  (temp_local_mat,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
#if DEBUG
  {
    Vec       lvec,rvec;
    PetscReal norm;
    ierr = MatCreateVecs(temp_local_mat,&lvec,&rvec);CHKERRQ(ierr);
    ierr = VecSet(lvec,1.0);CHKERRQ(ierr);
    ierr = MatMult(temp_local_mat,lvec,rvec);CHKERRQ(ierr);
    ierr = VecNorm(rvec,NORM_INFINITY,&norm);CHKERRQ(ierr);
    printf("Test null space of local mat % 1.14e\n",norm);
    ierr = VecDestroy(&lvec);CHKERRQ(ierr);
    ierr = VecDestroy(&rvec);CHKERRQ(ierr);
  }
#endif
  *local_mat = temp_local_mat;
  ierr       = MatDestroy(&elem_mat_DBC);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "GLLStuffs"
static PetscErrorCode GLLStuffs(DomainData dd, GLLData *glldata)
{
  PetscErrorCode ierr;
  PetscReal      *M,si;
  PetscScalar    x,z0,z1,z2,Lpj,Lpr,rhoGLj,rhoGLk;
  PetscBLASInt   pm1,lierr;
  PetscInt       i,j,n,k,s,r,q,ii,jj,p=dd.p;
  PetscInt       xloc,yloc,zloc,xyloc,xyzloc;

  PetscFunctionBeginUser;
  /* Gauss-Lobatto-Legendre nodes zGL on [-1,1] */
  ierr = PetscMalloc1(p+1,&glldata->zGL);CHKERRQ(ierr);
  ierr = PetscMemzero(glldata->zGL,(p+1)*sizeof(*glldata->zGL));CHKERRQ(ierr);

  glldata->zGL[0]=-1.0;
  glldata->zGL[p]= 1.0;
  if (p > 1) {
    if (p == 2) glldata->zGL[1]=0.0;
    else {
      ierr = PetscMalloc1(p-1,&M);CHKERRQ(ierr);
      for (i=0; i<p-1; i++) {
        si  = (PetscReal)(i+1.0);
        M[i]=0.5*PetscSqrtReal(si*(si+2.0)/((si+0.5)*(si+1.5)));
      }
      pm1  = p-1;
      ierr = PetscFPTrapPush(PETSC_FP_TRAP_OFF);CHKERRQ(ierr);
      PetscStackCallBLAS("LAPACKsteqr",LAPACKsteqr_("N",&pm1,&glldata->zGL[1],M,&x,&pm1,M,&lierr));
      if (lierr) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"Error in STERF Lapack routine %d",(int)lierr);
      ierr = PetscFPTrapPop();CHKERRQ(ierr);
      ierr = PetscFree(M);CHKERRQ(ierr);
    }
  }

  /* Weights for 1D quadrature */
  ierr = PetscMalloc1(p+1,&glldata->rhoGL);CHKERRQ(ierr);

  glldata->rhoGL[0]=2.0/(PetscScalar)(p*(p+1.0));
  glldata->rhoGL[p]=glldata->rhoGL[0];
  z2 = -1;                      /* Dummy value to avoid -Wmaybe-initialized */
  for (i=1; i<p; i++) {
    x  = glldata->zGL[i];
    z0 = 1.0;
    z1 = x;
    for (n=1; n<p; n++) {
      z2 = x*z1*(2.0*n+1.0)/(n+1.0)-z0*(PetscScalar)(n/(n+1.0));
      z0 = z1;
      z1 = z2;
    }
    glldata->rhoGL[i]=2.0/(p*(p+1.0)*z2*z2);
  }

  /* Auxiliary mat for laplacian */
  ierr = PetscMalloc1(p+1,&glldata->A);CHKERRQ(ierr);
  ierr = PetscMalloc1((p+1)*(p+1),&glldata->A[0]);CHKERRQ(ierr);
  for (i=1; i<p+1; i++) glldata->A[i]=glldata->A[i-1]+p+1;

  for (j=1; j<p; j++) {
    x =glldata->zGL[j];
    z0=1.0;
    z1=x;
    for (n=1; n<p; n++) {
      z2=x*z1*(2.0*n+1.0)/(n+1.0)-z0*(PetscScalar)(n/(n+1.0));
      z0=z1;
      z1=z2;
    }
    Lpj=z2;
    for (r=1; r<p; r++) {
      if (r == j) {
        glldata->A[j][j]=2.0/(3.0*(1.0-glldata->zGL[j]*glldata->zGL[j])*Lpj*Lpj);
      } else {
        x  = glldata->zGL[r];
        z0 = 1.0;
        z1 = x;
        for (n=1; n<p; n++) {
          z2=x*z1*(2.0*n+1.0)/(n+1.0)-z0*(PetscScalar)(n/(n+1.0));
          z0=z1;
          z1=z2;
        }
        Lpr             = z2;
        glldata->A[r][j]=4.0/(p*(p+1.0)*Lpj*Lpr*(glldata->zGL[j]-glldata->zGL[r])*(glldata->zGL[j]-glldata->zGL[r]));
      }
    }
  }
  for (j=1; j<p+1; j++) {
    x  = glldata->zGL[j];
    z0 = 1.0;
    z1 = x;
    for (n=1; n<p; n++) {
      z2=x*z1*(2.0*n+1.0)/(n+1.0)-z0*(PetscScalar)(n/(n+1.0));
      z0=z1;
      z1=z2;
    }
    Lpj             = z2;
    glldata->A[j][0]=4.0*PetscPowRealInt(-1.0,p)/(p*(p+1.0)*Lpj*(1.0+glldata->zGL[j])*(1.0+glldata->zGL[j]));
    glldata->A[0][j]=glldata->A[j][0];
  }
  for (j=0; j<p; j++) {
    x  = glldata->zGL[j];
    z0 = 1.0;
    z1 = x;
    for (n=1; n<p; n++) {
      z2=x*z1*(2.0*n+1.0)/(n+1.0)-z0*(PetscScalar)(n/(n+1.0));
      z0=z1;
      z1=z2;
    }
    Lpj=z2;

    glldata->A[p][j]=4.0/(p*(p+1.0)*Lpj*(1.0-glldata->zGL[j])*(1.0-glldata->zGL[j]));
    glldata->A[j][p]=glldata->A[p][j];
  }
  glldata->A[0][0]=0.5+(p*(p+1.0)-2.0)/6.0;
  glldata->A[p][p]=glldata->A[0][0];

  /* compute element matrix */
  xloc = p+1;
  yloc = p+1;
  zloc = p+1;
  if (dd.dim<2) yloc=1;
  if (dd.dim<3) zloc=1;
  xyloc  = xloc*yloc;
  xyzloc = xloc*yloc*zloc;

  ierr = MatCreate(PETSC_COMM_SELF,&glldata->elem_mat);
  ierr = MatSetSizes(glldata->elem_mat,xyzloc,xyzloc,xyzloc,xyzloc);CHKERRQ(ierr);
  ierr = MatSetType(glldata->elem_mat,MATSEQAIJ);CHKERRQ(ierr);
  ierr = MatSeqAIJSetPreallocation(glldata->elem_mat,xyzloc,NULL);CHKERRQ(ierr); /* overestimated */
  ierr = MatZeroEntries(glldata->elem_mat);CHKERRQ(ierr);
  ierr = MatSetOption(glldata->elem_mat,MAT_IGNORE_ZERO_ENTRIES,PETSC_TRUE);CHKERRQ(ierr);

  for (k=0; k<zloc; k++) {
    if (dd.dim>2) rhoGLk=glldata->rhoGL[k];
    else rhoGLk=1.0;

    for (j=0; j<yloc; j++) {
      if (dd.dim>1) rhoGLj=glldata->rhoGL[j];
      else rhoGLj=1.0;

      for (i=0; i<xloc; i++) {
        ii = k*xyloc+j*xloc+i;
        s  = k;
        r  = j;
        for (q=0; q<xloc; q++) {
          jj   = s*xyloc+r*xloc+q;
          ierr = MatSetValue(glldata->elem_mat,jj,ii,glldata->A[i][q]*rhoGLj*rhoGLk,ADD_VALUES);CHKERRQ(ierr);
        }
        if (dd.dim>1) {
          s=k;
          q=i;
          for (r=0; r<yloc; r++) {
            jj   = s*xyloc+r*xloc+q;
            ierr = MatSetValue(glldata->elem_mat,jj,ii,glldata->A[j][r]*glldata->rhoGL[i]*rhoGLk,ADD_VALUES);CHKERRQ(ierr);
          }
        }
        if (dd.dim>2) {
          r=j;
          q=i;
          for (s=0; s<zloc; s++) {
            jj   = s*xyloc+r*xloc+q;
            ierr = MatSetValue(glldata->elem_mat,jj,ii,glldata->A[k][s]*rhoGLj*glldata->rhoGL[i],ADD_VALUES);CHKERRQ(ierr);
          }
        }
      }
    }
  }
  ierr = MatAssemblyBegin(glldata->elem_mat,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd  (glldata->elem_mat,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
#if DEBUG
  {
    Vec       lvec,rvec;
    PetscReal norm;
    ierr = MatCreateVecs(glldata->elem_mat,&lvec,&rvec);CHKERRQ(ierr);
    ierr = VecSet(lvec,1.0);CHKERRQ(ierr);
    ierr = MatMult(glldata->elem_mat,lvec,rvec);CHKERRQ(ierr);
    ierr = VecNorm(rvec,NORM_INFINITY,&norm);CHKERRQ(ierr);
    printf("Test null space of elem mat % 1.14e\n",norm);
    ierr = VecDestroy(&lvec);CHKERRQ(ierr);
    ierr = VecDestroy(&rvec);CHKERRQ(ierr);
  }
#endif
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DomainDecomposition"
static PetscErrorCode DomainDecomposition(DomainData *dd)
{
  PetscMPIInt rank;
  PetscInt    i,j,k;

  PetscFunctionBeginUser;
  /* Subdomain index in cartesian coordinates */
  MPI_Comm_rank(dd->gcomm,&rank);
  dd->ipx = rank%dd->npx;
  if (dd->dim>1) dd->ipz = rank/(dd->npx*dd->npy);
  else dd->ipz = 0;

  dd->ipy = rank/dd->npx-dd->ipz*dd->npy;
  /* number of local elements */
  dd->nex_l = dd->nex/dd->npx;
  if (dd->ipx < dd->nex%dd->npx) dd->nex_l++;
  if (dd->dim>1) {
    dd->ney_l = dd->ney/dd->npy;
    if (dd->ipy < dd->ney%dd->npy) dd->ney_l++;
  } else {
    dd->ney_l = 0;
  }
  if (dd->dim>2) {
    dd->nez_l = dd->nez/dd->npz;
    if (dd->ipz < dd->nez%dd->npz) dd->nez_l++;
  } else {
    dd->nez_l = 0;
  }
  /* local and global number of dofs */
  dd->xm_l = dd->nex_l*dd->p+1;
  dd->xm   = dd->nex*dd->p+1;
  dd->ym_l = dd->ney_l*dd->p+1;
  dd->ym   = dd->ney*dd->p+1;
  dd->zm_l = dd->nez_l*dd->p+1;
  dd->zm   = dd->nez*dd->p+1;
  if (!dd->pure_neumann) {
    if (!dd->DBC_zerorows && !dd->ipx) dd->xm_l--;
    if (!dd->DBC_zerorows) dd->xm--;
  }

  /* starting global index for local dofs (simple lexicographic order) */
  dd->startx = 0;
  j          = dd->nex/dd->npx;
  for (i=0; i<dd->ipx; i++) {
    k = j;
    if (i<dd->nex%dd->npx) k++;
    dd->startx = dd->startx+k*dd->p;
  }
  if (!dd->pure_neumann && !dd->DBC_zerorows && dd->ipx) dd->startx--;

  dd->starty = 0;
  if (dd->dim > 1) {
    j = dd->ney/dd->npy;
    for (i=0; i<dd->ipy; i++) {
      k = j;
      if (i<dd->ney%dd->npy) k++;
      dd->starty = dd->starty+k*dd->p;
    }
  }
  dd->startz = 0;
  if (dd->dim > 2) {
    j = dd->nez/dd->npz;
    for (i=0; i<dd->ipz; i++) {
      k = j;
      if (i<dd->nez%dd->npz) k++;
      dd->startz = dd->startz+k*dd->p;
    }
  }
  PetscFunctionReturn(0);
}
#undef __FUNCT__
#define __FUNCT__ "ComputeMatrix"
static PetscErrorCode ComputeMatrix(DomainData dd, Mat *A)
{
  PetscErrorCode         ierr;
  GLLData                gll;
  Mat                    local_mat  =0,temp_A=0;
  ISLocalToGlobalMapping matis_map  =0;
  IS                     dirichletIS=0;

  PetscFunctionBeginUser;
  /* Compute some stuff of Gauss-Legendre-Lobatto quadrature rule */
  ierr = GLLStuffs(dd,&gll);CHKERRQ(ierr);
  /* Compute matrix of subdomain Neumann problem */
  ierr = ComputeSubdomainMatrix(dd,gll,&local_mat);CHKERRQ(ierr);
  /* Compute global mapping of local dofs */
  ierr = ComputeMapping(dd,&matis_map);CHKERRQ(ierr);
  /* Create MATIS object needed by BDDC */
  ierr = MatCreateIS(dd.gcomm,1,PETSC_DECIDE,PETSC_DECIDE,dd.xm*dd.ym*dd.zm,dd.xm*dd.ym*dd.zm,matis_map,NULL,&temp_A);CHKERRQ(ierr);
  /* Set local subdomain matrices into MATIS object */
  ierr = MatScale(local_mat,dd.scalingfactor);CHKERRQ(ierr);
  ierr = MatISSetLocalMat(temp_A,local_mat);CHKERRQ(ierr);
  /* Call assembly functions */
  ierr = MatAssemblyBegin(temp_A,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(temp_A,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);

  if (dd.DBC_zerorows) {
    PetscInt dirsize;

    ierr = ComputeSpecialBoundaryIndices(dd,&dirichletIS,NULL);CHKERRQ(ierr);
    ierr = MatSetOption(local_mat,MAT_KEEP_NONZERO_PATTERN,PETSC_TRUE);CHKERRQ(ierr);
    ierr = MatZeroRowsLocalIS(temp_A,dirichletIS,1.0,NULL,NULL);CHKERRQ(ierr);
    ierr = ISGetLocalSize(dirichletIS,&dirsize);CHKERRQ(ierr);
    /* giving hints to local and global matrices could be useful for the BDDC */
    if (!dirsize) {
      ierr = MatSetOption(local_mat,MAT_SYMMETRIC,PETSC_TRUE);CHKERRQ(ierr);
      ierr = MatSetOption(local_mat,MAT_SPD,PETSC_TRUE);CHKERRQ(ierr);
    } else {
      ierr = MatSetOption(local_mat,MAT_SYMMETRIC,PETSC_FALSE);CHKERRQ(ierr);
      ierr = MatSetOption(local_mat,MAT_SPD,PETSC_FALSE);CHKERRQ(ierr);
    }
    ierr = ISDestroy(&dirichletIS);CHKERRQ(ierr);
  } else { /* safe to set the options for the global matrices (they will be communicated to the matis local matrices) */
    ierr = MatSetOption(temp_A,MAT_SYMMETRIC,PETSC_TRUE);CHKERRQ(ierr);
    ierr = MatSetOption(temp_A,MAT_SPD,PETSC_TRUE);CHKERRQ(ierr);
  }
#if DEBUG
  {
    Vec       lvec,rvec;
    PetscReal norm;
    ierr = MatCreateVecs(temp_A,&lvec,&rvec);CHKERRQ(ierr);
    ierr = VecSet(lvec,1.0);CHKERRQ(ierr);
    ierr = MatMult(temp_A,lvec,rvec);CHKERRQ(ierr);
    ierr = VecNorm(rvec,NORM_INFINITY,&norm);CHKERRQ(ierr);
    printf("Test null space of global mat % 1.14e\n",norm);
    ierr = VecDestroy(&lvec);CHKERRQ(ierr);
    ierr = VecDestroy(&rvec);CHKERRQ(ierr);
  }
#endif
  /* free allocated workspace */
  ierr = PetscFree(gll.zGL);CHKERRQ(ierr);
  ierr = PetscFree(gll.rhoGL);CHKERRQ(ierr);
  ierr = PetscFree(gll.A[0]);CHKERRQ(ierr);
  ierr = PetscFree(gll.A);CHKERRQ(ierr);
  ierr = MatDestroy(&gll.elem_mat);CHKERRQ(ierr);
  ierr = MatDestroy(&local_mat);CHKERRQ(ierr);
  ierr = ISLocalToGlobalMappingDestroy(&matis_map);CHKERRQ(ierr);
  /* give back the pointer to te MATIS object */
  *A = temp_A;
  PetscFunctionReturn(0);
}
#undef __FUNCT__
#define __FUNCT__ "ComputeKSPFETIDP"
static PetscErrorCode ComputeKSPFETIDP(DomainData dd, KSP ksp_bddc, KSP *ksp_fetidp)
{
  PetscErrorCode ierr;
  KSP            temp_ksp;
  PC             pc,D;
  Mat            F;

  PetscFunctionBeginUser;
  ierr        = KSPGetPC(ksp_bddc,&pc);CHKERRQ(ierr);
  ierr        = PCBDDCCreateFETIDPOperators(pc,&F,&D);CHKERRQ(ierr);
  ierr        = KSPCreate(PetscObjectComm((PetscObject)F),&temp_ksp);CHKERRQ(ierr);
  ierr        = KSPSetOperators(temp_ksp,F,F);CHKERRQ(ierr);
  ierr        = KSPSetType(temp_ksp,KSPCG);CHKERRQ(ierr);
  ierr        = KSPSetPC(temp_ksp,D);CHKERRQ(ierr);
  ierr        = KSPSetComputeSingularValues(temp_ksp,PETSC_TRUE);CHKERRQ(ierr);
  ierr        = KSPSetFromOptions(temp_ksp);CHKERRQ(ierr);
  ierr        = KSPSetUp(temp_ksp);CHKERRQ(ierr);
  *ksp_fetidp = temp_ksp;
  ierr        = MatDestroy(&F);CHKERRQ(ierr);
  ierr        = PCDestroy(&D);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "ComputeKSPBDDC"
static PetscErrorCode ComputeKSPBDDC(DomainData dd,Mat A,KSP *ksp)
{
  PetscErrorCode ierr;
  KSP            temp_ksp;
  PC             pc;
  IS             dirichletIS=0,neumannIS=0,*bddc_dofs_splitting;
  PetscInt       localsize,*xadj=NULL,*adjncy=NULL;
  MatNullSpace   near_null_space;

  PetscFunctionBeginUser;
  ierr = KSPCreate(dd.gcomm,&temp_ksp);CHKERRQ(ierr);
  ierr = KSPSetOperators(temp_ksp,A,A);CHKERRQ(ierr);
  ierr = KSPSetType(temp_ksp,KSPCG);CHKERRQ(ierr);
  ierr = KSPGetPC(temp_ksp,&pc);CHKERRQ(ierr);
  ierr = PCSetType(pc,PCBDDC);CHKERRQ(ierr);

  localsize = dd.xm_l*dd.ym_l*dd.zm_l;

  /* BDDC customization */

  /* jumping coefficients case */
  ierr = PCISSetSubdomainScalingFactor(pc,dd.scalingfactor);CHKERRQ(ierr);

  /* Dofs splitting
     Simple stride-1 IS
     It is not needed since, by default, PCBDDC assumes a stride-1 split */
  ierr = PetscMalloc1(1,&bddc_dofs_splitting);CHKERRQ(ierr);
#if 1
  ierr = ISCreateStride(PETSC_COMM_WORLD,localsize,0,1,&bddc_dofs_splitting[0]);CHKERRQ(ierr);
  ierr = PCBDDCSetDofsSplittingLocal(pc,1,bddc_dofs_splitting);CHKERRQ(ierr);
#else
  /* examples for global ordering */

  /* each process lists the nodes it owns */
  PetscInt sr,er;
  ierr = MatGetOwnershipRange(A,&sr,&er);CHKERRQ(ierr);
  ierr = ISCreateStride(PETSC_COMM_WORLD,er-sr,sr,1,&bddc_dofs_splitting[0]);CHKERRQ(ierr);
  ierr = PCBDDCSetDofsSplitting(pc,1,bddc_dofs_splitting);CHKERRQ(ierr);
  /* Split can be passed in a more general way since any process can list any node */
#endif
  ierr = ISDestroy(&bddc_dofs_splitting[0]);CHKERRQ(ierr);
  ierr = PetscFree(bddc_dofs_splitting);CHKERRQ(ierr);

  /* Primal constraints implemented by using a near null space attached to A -> now it passes in only the constants
    (which in practice is not needed since, by default, PCBDDC build the primal space using constants for quadrature formulas */
#if 0
  Vec vecs[2];
  PetscRandom rctx;
  ierr = MatCreateVecs(A,&vecs[0],&vecs[1]);CHKERRQ(ierr);
  ierr = PetscRandomCreate(dd.gcomm,&rctx);CHKERRQ(ierr);
  ierr = VecSetRandom(vecs[0],rctx);CHKERRQ(ierr);
  ierr = VecSetRandom(vecs[1],rctx);CHKERRQ(ierr);
  ierr = MatNullSpaceCreate(dd.gcomm,PETSC_TRUE,2,vecs,&near_null_space);CHKERRQ(ierr);
  ierr = VecDestroy(&vecs[0]);CHKERRQ(ierr);
  ierr = VecDestroy(&vecs[1]);CHKERRQ(ierr);
  ierr = PetscRandomDestroy(&rctx);CHKERRQ(ierr);
#else
  ierr = MatNullSpaceCreate(dd.gcomm,PETSC_TRUE,0,NULL,&near_null_space);CHKERRQ(ierr);
#endif
  ierr = MatSetNearNullSpace(A,near_null_space);CHKERRQ(ierr);
  ierr = MatNullSpaceDestroy(&near_null_space);CHKERRQ(ierr);

  /* CSR graph of subdomain dofs */
  ierr = BuildCSRGraph(dd,&xadj,&adjncy);CHKERRQ(ierr);
  ierr = PCBDDCSetLocalAdjacencyGraph(pc,localsize,xadj,adjncy,PETSC_OWN_POINTER);CHKERRQ(ierr);

  /* Neumann/Dirichlet indices on the global boundary */
  if (dd.DBC_zerorows) {
    /* Only in case you eliminate some rows matrix with zerorows function, you need to set dirichlet indices into PCBDDC data */
    ierr = ComputeSpecialBoundaryIndices(dd,&dirichletIS,&neumannIS);CHKERRQ(ierr);
    ierr = PCBDDCSetNeumannBoundariesLocal(pc,neumannIS);CHKERRQ(ierr);
    ierr = PCBDDCSetDirichletBoundariesLocal(pc,dirichletIS);CHKERRQ(ierr);
  } else {
    if (dd.pure_neumann) {
      /* In such a case, all interface nodes lying on the global boundary are neumann nodes */
      ierr = ComputeSpecialBoundaryIndices(dd,NULL,&neumannIS);CHKERRQ(ierr);
      ierr = PCBDDCSetNeumannBoundariesLocal(pc,neumannIS);CHKERRQ(ierr);
    } else {
      /* It is wrong setting dirichlet indices without having zeroed the corresponding rows in the global matrix */
      /* But we can still compute them since nodes near the dirichlet boundaries does not need to be defined as neumann nodes */
      ierr = ComputeSpecialBoundaryIndices(dd,&dirichletIS,&neumannIS);CHKERRQ(ierr);
      ierr = PCBDDCSetNeumannBoundariesLocal(pc,neumannIS);CHKERRQ(ierr);
    }
  }

  /* Pass null space information to BDDC (don't pass it via MatSetNullSpace!) */
  if (dd.pure_neumann) {
    MatNullSpace nsp;
    ierr = MatNullSpaceCreate(dd.gcomm,PETSC_TRUE,0,NULL,&nsp);CHKERRQ(ierr);
    ierr = PCBDDCSetNullSpace(pc,nsp);CHKERRQ(ierr);
    ierr = MatNullSpaceDestroy(&nsp);CHKERRQ(ierr);
  }
  ierr = KSPSetComputeSingularValues(temp_ksp,PETSC_TRUE);CHKERRQ(ierr);
  ierr = KSPSetFromOptions(temp_ksp);CHKERRQ(ierr);
  ierr = KSPSetUp(temp_ksp);CHKERRQ(ierr);
  *ksp = temp_ksp;
  ierr = ISDestroy(&dirichletIS);CHKERRQ(ierr);
  ierr = ISDestroy(&neumannIS);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "InitializeDomainData"
static PetscErrorCode InitializeDomainData(DomainData *dd)
{
  PetscErrorCode ierr;
  PetscMPIInt    sizes,rank;
  PetscInt       factor;

  PetscFunctionBeginUser;
  dd->gcomm = PETSC_COMM_WORLD;
  ierr      = MPI_Comm_size(dd->gcomm,&sizes);
  ierr      = MPI_Comm_rank(dd->gcomm,&rank);
  /* test data passed in */
  if (sizes<2) SETERRQ(dd->gcomm,PETSC_ERR_USER,"This is not a uniprocessor test");
  /* Get informations from command line */
  /* Processors/subdomains per dimension */
  /* Default is 1d problem */
  dd->npx = sizes;
  dd->npy = 0;
  dd->npz = 0;
  dd->dim = 1;
  ierr    = PetscOptionsGetInt(NULL,NULL,"-npx",&dd->npx,NULL);CHKERRQ(ierr);
  ierr    = PetscOptionsGetInt(NULL,NULL,"-npy",&dd->npy,NULL);CHKERRQ(ierr);
  ierr    = PetscOptionsGetInt(NULL,NULL,"-npz",&dd->npz,NULL);CHKERRQ(ierr);
  if (dd->npy) dd->dim++;
  if (dd->npz) dd->dim++;
  /* Number of elements per dimension */
  /* Default is one element per subdomain */
  dd->nex = dd->npx;
  dd->ney = dd->npy;
  dd->nez = dd->npz;
  ierr    = PetscOptionsGetInt(NULL,NULL,"-nex",&dd->nex,NULL);CHKERRQ(ierr);
  ierr    = PetscOptionsGetInt(NULL,NULL,"-ney",&dd->ney,NULL);CHKERRQ(ierr);
  ierr    = PetscOptionsGetInt(NULL,NULL,"-nez",&dd->nez,NULL);CHKERRQ(ierr);
  if (!dd->npy) {
    dd->ney=0;
    dd->nez=0;
  }
  if (!dd->npz) dd->nez=0;
  /* Spectral degree */
  dd->p = 3;
  ierr  = PetscOptionsGetInt(NULL,NULL,"-p",&dd->p,NULL);CHKERRQ(ierr);
  /* pure neumann problem? */
  dd->pure_neumann = PETSC_FALSE;
  ierr             = PetscOptionsGetBool(NULL,NULL,"-pureneumann",&dd->pure_neumann,NULL);CHKERRQ(ierr);

  /* How to enforce dirichlet boundary conditions (in case pureneumann has not been requested explicitly) */
  dd->DBC_zerorows = PETSC_FALSE;

  ierr = PetscOptionsGetBool(NULL,NULL,"-usezerorows",&dd->DBC_zerorows,NULL);CHKERRQ(ierr);
  if (dd->pure_neumann) dd->DBC_zerorows = PETSC_FALSE;
  dd->scalingfactor = 1.0;

  factor = 0.0;
  ierr   = PetscOptionsGetInt(NULL,NULL,"-jump",&factor,NULL);CHKERRQ(ierr);
  /* checkerboard pattern */
  dd->scalingfactor = PetscPowScalar(10.0,(PetscScalar)factor*PetscPowScalar(-1.0,(PetscScalar)rank));
  /* test data passed in */
  if (dd->dim==1) {
    if (sizes!=dd->npx) SETERRQ(dd->gcomm,PETSC_ERR_USER,"Number of mpi procs in 1D must be equal to npx");
    if (dd->nex<dd->npx) SETERRQ(dd->gcomm,PETSC_ERR_USER,"Number of elements per dim must be greater/equal than number of procs per dim");
  } else if (dd->dim==2) {
    if (sizes!=dd->npx*dd->npy) SETERRQ(dd->gcomm,PETSC_ERR_USER,"Number of mpi procs in 2D must be equal to npx*npy");
    if (dd->nex<dd->npx || dd->ney<dd->npy) SETERRQ(dd->gcomm,PETSC_ERR_USER,"Number of elements per dim must be greater/equal than number of procs per dim");
  } else {
    if (sizes!=dd->npx*dd->npy*dd->npz) SETERRQ(dd->gcomm,PETSC_ERR_USER,"Number of mpi procs in 3D must be equal to npx*npy*npz");
    if (dd->nex<dd->npx || dd->ney<dd->npy || dd->nez<dd->npz) SETERRQ(dd->gcomm,PETSC_ERR_USER,"Number of elements per dim must be greater/equal than number of procs per dim");
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc,char **args)
{
  PetscErrorCode ierr;
  DomainData     dd;
  PetscReal      norm,maxeig,mineig;
  PetscScalar    scalar_value;
  PetscInt       ndofs,its;
  Mat            A                  =0,F=0;
  KSP            KSPwithBDDC        =0,KSPwithFETIDP=0;
  Vec            fetidp_solution_all=0,bddc_solution=0,bddc_rhs=0;
  Vec            exact_solution     =0,fetidp_solution=0,fetidp_rhs=0;

  /* Init PETSc */
  PetscInitialize(&argc,&args,(char*)0,help);
  /* Initialize DomainData */
  ierr = InitializeDomainData(&dd);CHKERRQ(ierr);
  /* Decompose domain */
  ierr = DomainDecomposition(&dd);CHKERRQ(ierr);
#if DEBUG
  printf("Subdomain data\n");
  printf("IPS   : %d %d %d\n",dd.ipx,dd.ipy,dd.ipz);
  printf("NEG   : %d %d %d\n",dd.nex,dd.ney,dd.nez);
  printf("NEL   : %d %d %d\n",dd.nex_l,dd.ney_l,dd.nez_l);
  printf("LDO   : %d %d %d\n",dd.xm_l,dd.ym_l,dd.zm_l);
  printf("SIZES : %d %d %d\n",dd.xm,dd.ym,dd.zm);
  printf("STARTS: %d %d %d\n",dd.startx,dd.starty,dd.startz);
#endif
  /* assemble global matrix */
  ierr = ComputeMatrix(dd,&A);CHKERRQ(ierr);
  /* get work vectors */
  ierr = MatCreateVecs(A,&bddc_solution,NULL);CHKERRQ(ierr);
  ierr = VecDuplicate(bddc_solution,&bddc_rhs);CHKERRQ(ierr);
  ierr = VecDuplicate(bddc_solution,&fetidp_solution_all);CHKERRQ(ierr);
  ierr = VecDuplicate(bddc_solution,&exact_solution);CHKERRQ(ierr);
  /* create and customize KSP/PC for BDDC */
  ierr = ComputeKSPBDDC(dd,A,&KSPwithBDDC);CHKERRQ(ierr);
  /* create KSP/PC for FETIDP */
  ierr = ComputeKSPFETIDP(dd,KSPwithBDDC,&KSPwithFETIDP);CHKERRQ(ierr);
  /* create random exact solution */
  ierr = VecSetRandom(exact_solution,NULL);CHKERRQ(ierr);
  ierr = VecShift(exact_solution,-0.5);CHKERRQ(ierr);
  ierr = VecScale(exact_solution,100.0);CHKERRQ(ierr);
  ierr = VecGetSize(exact_solution,&ndofs);
  if (dd.pure_neumann) {
    ierr = VecSum(exact_solution,&scalar_value);CHKERRQ(ierr);
    scalar_value = -scalar_value/(PetscScalar)ndofs;
    ierr = VecShift(exact_solution,scalar_value);CHKERRQ(ierr);
  }
  /* assemble BDDC rhs */
  ierr = MatMult(A,exact_solution,bddc_rhs);CHKERRQ(ierr);
  /* test ksp with BDDC */
  ierr = KSPSolve(KSPwithBDDC,bddc_rhs,bddc_solution);CHKERRQ(ierr);
  ierr = KSPGetIterationNumber(KSPwithBDDC,&its);CHKERRQ(ierr);
  ierr = KSPComputeExtremeSingularValues(KSPwithBDDC,&maxeig,&mineig);CHKERRQ(ierr);
  if (dd.pure_neumann) {
    ierr = VecSum(bddc_solution,&scalar_value);CHKERRQ(ierr);
    scalar_value = -scalar_value/(PetscScalar)ndofs;
    ierr = VecShift(bddc_solution,scalar_value);CHKERRQ(ierr);
  }
  /* check exact_solution and BDDC solultion */
  ierr = VecAXPY(bddc_solution,-1.0,exact_solution);CHKERRQ(ierr);
  ierr = VecNorm(bddc_solution,NORM_INFINITY,&norm);CHKERRQ(ierr);
  ierr = PetscPrintf(dd.gcomm,"---------------------BDDC stats-------------------------------\n");CHKERRQ(ierr);
  ierr = PetscPrintf(dd.gcomm,"Number of degrees of freedom               : %8D \n",ndofs);CHKERRQ(ierr);
  ierr = PetscPrintf(dd.gcomm,"Number of iterations                       : %8D \n",its);CHKERRQ(ierr);
  ierr = PetscPrintf(dd.gcomm,"Eigenvalues preconditioned operator        : %1.2e %1.2e\n",(double)mineig,(double)maxeig);CHKERRQ(ierr);
  ierr = PetscPrintf(dd.gcomm,"Error betweeen exact and computed solution : %1.2e\n",(double)norm);CHKERRQ(ierr);
  ierr = PetscPrintf(dd.gcomm,"--------------------------------------------------------------\n");CHKERRQ(ierr);
  /* assemble fetidp rhs on the space of Lagrange multipliers */
  ierr = KSPGetOperators(KSPwithFETIDP,&F,NULL);CHKERRQ(ierr);
  ierr = MatCreateVecs(F,&fetidp_solution,&fetidp_rhs);CHKERRQ(ierr);
  ierr = PCBDDCMatFETIDPGetRHS(F,bddc_rhs,fetidp_rhs);CHKERRQ(ierr);
  ierr = VecSet(fetidp_solution,0.0);CHKERRQ(ierr);
  /* test ksp with FETIDP */
  ierr = KSPSolve(KSPwithFETIDP,fetidp_rhs,fetidp_solution);CHKERRQ(ierr);
  ierr = KSPGetIterationNumber(KSPwithFETIDP,&its);CHKERRQ(ierr);
  ierr = KSPComputeExtremeSingularValues(KSPwithFETIDP,&maxeig,&mineig);CHKERRQ(ierr);
  /* assemble fetidp solution on physical domain */
  ierr = PCBDDCMatFETIDPGetSolution(F,fetidp_solution,fetidp_solution_all);CHKERRQ(ierr);
  /* check FETIDP sol */
  if (dd.pure_neumann) {
    ierr = VecSum(fetidp_solution_all,&scalar_value);CHKERRQ(ierr);
    scalar_value = -scalar_value/(PetscScalar)ndofs;
    ierr = VecShift(fetidp_solution_all,scalar_value);CHKERRQ(ierr);
  }
  ierr = VecAXPY(fetidp_solution_all,-1.0,exact_solution);CHKERRQ(ierr);
  ierr = VecNorm(fetidp_solution_all,NORM_INFINITY,&norm);CHKERRQ(ierr);
  ierr = VecGetSize(fetidp_solution,&ndofs);
  ierr = PetscPrintf(dd.gcomm,"------------------FETI-DP stats-------------------------------\n");CHKERRQ(ierr);
  ierr = PetscPrintf(dd.gcomm,"Number of degrees of freedom               : %8D \n",ndofs);CHKERRQ(ierr);
  ierr = PetscPrintf(dd.gcomm,"Number of iterations                       : %8D \n",its);CHKERRQ(ierr);
  ierr = PetscPrintf(dd.gcomm,"Eigenvalues preconditioned operator        : %1.2e %1.2e\n",(double)mineig,(double)maxeig);CHKERRQ(ierr);
  ierr = PetscPrintf(dd.gcomm,"Error betweeen exact and computed solution : %1.2e\n",(double)norm);CHKERRQ(ierr);
  ierr = PetscPrintf(dd.gcomm,"--------------------------------------------------------------\n");CHKERRQ(ierr);
  /* Free workspace */
  ierr = VecDestroy(&exact_solution);CHKERRQ(ierr);
  ierr = VecDestroy(&bddc_solution);CHKERRQ(ierr);
  ierr = VecDestroy(&fetidp_solution);CHKERRQ(ierr);
  ierr = VecDestroy(&fetidp_solution_all);CHKERRQ(ierr);
  ierr = VecDestroy(&bddc_rhs);CHKERRQ(ierr);
  ierr = VecDestroy(&fetidp_rhs);CHKERRQ(ierr);
  ierr = MatDestroy(&A);CHKERRQ(ierr);
  ierr = KSPDestroy(&KSPwithBDDC);CHKERRQ(ierr);
  ierr = KSPDestroy(&KSPwithFETIDP);CHKERRQ(ierr);
  /* Quit PETSc */
  ierr = PetscFinalize();CHKERRQ(ierr);

  return 0;
}

