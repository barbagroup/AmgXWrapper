
#include <petsc/private/pcimpl.h>
#include <petscksp.h>           /*I "petscksp.h" I*/
#include <petscdm.h>
#include <petscdmda.h>

#include "telescope.h"

#undef __FUNCT__
#define __FUNCT__ "_DMDADetermineRankFromGlobalIJK"
PetscErrorCode _DMDADetermineRankFromGlobalIJK(PetscInt dim,PetscInt i,PetscInt j,PetscInt k,
                                               PetscInt Mp,PetscInt Np,PetscInt Pp,
                                               PetscInt start_i[],PetscInt start_j[],PetscInt start_k[],
                                               PetscInt span_i[],PetscInt span_j[],PetscInt span_k[],
                                               PetscMPIInt *_pi,PetscMPIInt *_pj,PetscMPIInt *_pk,PetscMPIInt *rank_re)
{
  PetscInt pi,pj,pk,n;

  PetscFunctionBegin;
  pi = pj = pk = -1;
  if (_pi) {
    for (n=0; n<Mp; n++) {
      if ( (i >= start_i[n]) && (i < start_i[n]+span_i[n]) ) {
        pi = n;
        break;
      }
    }
    if (pi == -1) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_USER,"[dmda-ijk] pi cannot be determined : range %D, val %D",Mp,i);
    *_pi = pi;
  }

  if (_pj) {
    for (n=0; n<Np; n++) {
      if ( (j >= start_j[n]) && (j < start_j[n]+span_j[n]) ) {
        pj = n;
        break;
      }
    }
    if (pj == -1) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_USER,"[dmda-ijk] pj cannot be determined : range %D, val %D",Np,j);
    *_pj = pj;
  }

  if (_pk) {
    for (n=0; n<Pp; n++) {
      if ( (k >= start_k[n]) && (k < start_k[n]+span_k[n]) ) {
        pk = n;
        break;
      }
    }
    if (pk == -1) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_USER,"[dmda-ijk] pk cannot be determined : range %D, val %D",Pp,k);
    *_pk = pk;
  }

  switch (dim) {
    case 1:
      *rank_re = pi;
      break;
    case 2:
      *rank_re = pi + pj * Mp;
      break;
    case 3:
      *rank_re = pi + pj * Mp + pk * (Mp*Np);
      break;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "_DMDADetermineGlobalS0"
PetscErrorCode _DMDADetermineGlobalS0(PetscInt dim,PetscMPIInt rank_re,PetscInt Mp_re,PetscInt Np_re,PetscInt Pp_re,
                                      PetscInt range_i_re[],PetscInt range_j_re[],PetscInt range_k_re[],PetscInt *s0)
{
  PetscInt i,j,k,start_IJK = 0;
  PetscInt rank_ijk;

  PetscFunctionBegin;
  switch (dim) {
    case 1:
      for (i=0; i<Mp_re; i++) {
        rank_ijk = i;
        if (rank_ijk < rank_re) {
          start_IJK += range_i_re[i];
        }
      }
      break;
    case 2:
      for (j=0; j<Np_re; j++) {
        for (i=0; i<Mp_re; i++) {
          rank_ijk = i + j*Mp_re;
          if (rank_ijk < rank_re) {
            start_IJK += range_i_re[i]*range_j_re[j];
          }
        }
      }
      break;
    case 3:
      for (k=0; k<Pp_re; k++) {
        for (j=0; j<Np_re; j++) {
          for (i=0; i<Mp_re; i++) {
            rank_ijk = i + j*Mp_re + k*Mp_re*Np_re;
            if (rank_ijk < rank_re) {
              start_IJK += range_i_re[i]*range_j_re[j]*range_k_re[k];
            }
          }
        }
      }
      break;
  }
  *s0 = start_IJK;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PCTelescopeSetUp_dmda_repart_coors2d"
PetscErrorCode PCTelescopeSetUp_dmda_repart_coors2d(PetscSubcomm psubcomm,DM dm,DM subdm)
{
  PetscErrorCode ierr;
  DM             cdm;
  Vec            coor,coor_natural,perm_coors;
  PetscInt       i,j,si,sj,ni,nj,M,N,Ml,Nl,c,nidx;
  PetscInt       *fine_indices;
  IS             is_fine,is_local;
  VecScatter     sctx;

  PetscFunctionBegin;
  ierr = DMGetCoordinates(dm,&coor);CHKERRQ(ierr);
  if (!coor) return(0);
  if (isActiveRank(psubcomm)) {
    ierr = DMDASetUniformCoordinates(subdm,0.0,1.0,0.0,1.0,0.0,1.0);CHKERRQ(ierr);
  }
  /* Get the coordinate vector from the distributed array */
  ierr = DMGetCoordinateDM(dm,&cdm);CHKERRQ(ierr);
  ierr = DMDACreateNaturalVector(cdm,&coor_natural);CHKERRQ(ierr);

  ierr = DMDAGlobalToNaturalBegin(cdm,coor,INSERT_VALUES,coor_natural);CHKERRQ(ierr);
  ierr = DMDAGlobalToNaturalEnd(cdm,coor,INSERT_VALUES,coor_natural);CHKERRQ(ierr);

  /* get indices of the guys I want to grab */
  ierr = DMDAGetInfo(dm,NULL,&M,&N,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);CHKERRQ(ierr);
  if (isActiveRank(psubcomm)) {
    ierr = DMDAGetCorners(subdm,&si,&sj,NULL,&ni,&nj,NULL);CHKERRQ(ierr);
    Ml = ni;
    Nl = nj;
  } else {
    Ml = Nl = 0;
  }

  ierr = PetscMalloc1(Ml*Nl*2,&fine_indices);CHKERRQ(ierr);
  c = 0;
  if (isActiveRank(psubcomm)) {
    for (j=sj; j<sj+nj; j++) {
      for (i=si; i<si+ni; i++) {
        nidx = (i) + (j)*M;
        fine_indices[c  ] = 2 * nidx     ;
        fine_indices[c+1] = 2 * nidx + 1 ;
        c = c + 2;
      }
    }
    if (c != Ml*Nl*2) SETERRQ3(PETSC_COMM_SELF,PETSC_ERR_PLIB,"c %D should equal 2 * Ml %D * Nl %D",c,Ml,Nl);
  }

  /* generate scatter */
  ierr = ISCreateGeneral(PetscObjectComm((PetscObject)dm),Ml*Nl*2,fine_indices,PETSC_USE_POINTER,&is_fine);CHKERRQ(ierr);
  ierr = ISCreateStride(PETSC_COMM_SELF,Ml*Nl*2,0,1,&is_local);CHKERRQ(ierr);

  /* scatter */
  ierr = VecCreate(PETSC_COMM_SELF,&perm_coors);CHKERRQ(ierr);
  ierr = VecSetSizes(perm_coors,PETSC_DECIDE,Ml*Nl*2);CHKERRQ(ierr);
  ierr = VecSetType(perm_coors,VECSEQ);CHKERRQ(ierr);

  ierr = VecScatterCreate(coor_natural,is_fine,perm_coors,is_local,&sctx);CHKERRQ(ierr);
  ierr = VecScatterBegin(sctx,coor_natural,perm_coors,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
  ierr = VecScatterEnd(  sctx,coor_natural,perm_coors,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
  /* access */
  if (isActiveRank(psubcomm)) {
    Vec               _coors;
    const PetscScalar *LA_perm;
    PetscScalar       *LA_coors;

    ierr = DMGetCoordinates(subdm,&_coors);CHKERRQ(ierr);
    ierr = VecGetArrayRead(perm_coors,&LA_perm);CHKERRQ(ierr);
    ierr = VecGetArray(_coors,&LA_coors);CHKERRQ(ierr);
    for (i=0; i<Ml*Nl*2; i++) {
      LA_coors[i] = LA_perm[i];
    }
    ierr = VecRestoreArray(_coors,&LA_coors);CHKERRQ(ierr);
    ierr = VecRestoreArrayRead(perm_coors,&LA_perm);CHKERRQ(ierr);
  }

  /* update local coords */
  if (isActiveRank(psubcomm)) {
    DM  _dmc;
    Vec _coors,_coors_local;
    ierr = DMGetCoordinateDM(subdm,&_dmc);CHKERRQ(ierr);
    ierr = DMGetCoordinates(subdm,&_coors);CHKERRQ(ierr);
    ierr = DMGetCoordinatesLocal(subdm,&_coors_local);CHKERRQ(ierr);
    ierr = DMGlobalToLocalBegin(_dmc,_coors,INSERT_VALUES,_coors_local);CHKERRQ(ierr);
    ierr = DMGlobalToLocalEnd(_dmc,_coors,INSERT_VALUES,_coors_local);CHKERRQ(ierr);
  }
  ierr = VecScatterDestroy(&sctx);CHKERRQ(ierr);
  ierr = ISDestroy(&is_fine);CHKERRQ(ierr);
  ierr = PetscFree(fine_indices);CHKERRQ(ierr);
  ierr = ISDestroy(&is_local);CHKERRQ(ierr);
  ierr = VecDestroy(&perm_coors);CHKERRQ(ierr);
  ierr = VecDestroy(&coor_natural);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PCTelescopeSetUp_dmda_repart_coors3d"
PetscErrorCode PCTelescopeSetUp_dmda_repart_coors3d(PetscSubcomm psubcomm,DM dm,DM subdm)
{
  PetscErrorCode ierr;
  DM             cdm;
  Vec            coor,coor_natural,perm_coors;
  PetscInt       i,j,k,si,sj,sk,ni,nj,nk,M,N,P,Ml,Nl,Pl,c,nidx;
  PetscInt       *fine_indices;
  IS             is_fine,is_local;
  VecScatter     sctx;

  PetscFunctionBegin;
  ierr = DMGetCoordinates(dm,&coor);CHKERRQ(ierr);
  if (!coor) PetscFunctionReturn(0);

  if (isActiveRank(psubcomm)) {
    ierr = DMDASetUniformCoordinates(subdm,0.0,1.0,0.0,1.0,0.0,1.0);CHKERRQ(ierr);
  }

  /* Get the coordinate vector from the distributed array */
  ierr = DMGetCoordinateDM(dm,&cdm);CHKERRQ(ierr);
  ierr = DMDACreateNaturalVector(cdm,&coor_natural);CHKERRQ(ierr);
  ierr = DMDAGlobalToNaturalBegin(cdm,coor,INSERT_VALUES,coor_natural);CHKERRQ(ierr);
  ierr = DMDAGlobalToNaturalEnd(cdm,coor,INSERT_VALUES,coor_natural);CHKERRQ(ierr);

  /* get indices of the guys I want to grab */
  ierr = DMDAGetInfo(dm,NULL,&M,&N,&P,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);CHKERRQ(ierr);

  if (isActiveRank(psubcomm)) {
    ierr = DMDAGetCorners(subdm,&si,&sj,&sk,&ni,&nj,&nk);CHKERRQ(ierr);
    Ml = ni;
    Nl = nj;
    Pl = nk;
  } else {
    Ml = Nl = Pl = 0;
  }

  ierr = PetscMalloc1(Ml*Nl*Pl*3,&fine_indices);CHKERRQ(ierr);

  c = 0;
  if (isActiveRank(psubcomm)) {
    for (k=sk; k<sk+nk; k++) {
      for (j=sj; j<sj+nj; j++) {
        for (i=si; i<si+ni; i++) {
          nidx = (i) + (j)*M + (k)*M*N;
          fine_indices[c  ] = 3 * nidx     ;
          fine_indices[c+1] = 3 * nidx + 1 ;
          fine_indices[c+2] = 3 * nidx + 2 ;
          c = c + 3;
        }
      }
    }
  }

  /* generate scatter */
  ierr = ISCreateGeneral(PetscObjectComm((PetscObject)dm),Ml*Nl*Pl*3,fine_indices,PETSC_USE_POINTER,&is_fine);CHKERRQ(ierr);
  ierr = ISCreateStride(PETSC_COMM_SELF,Ml*Nl*Pl*3,0,1,&is_local);CHKERRQ(ierr);

  /* scatter */
  ierr = VecCreate(PETSC_COMM_SELF,&perm_coors);CHKERRQ(ierr);
  ierr = VecSetSizes(perm_coors,PETSC_DECIDE,Ml*Nl*Pl*3);CHKERRQ(ierr);
  ierr = VecSetType(perm_coors,VECSEQ);CHKERRQ(ierr);
  ierr = VecScatterCreate(coor_natural,is_fine,perm_coors,is_local,&sctx);CHKERRQ(ierr);
  ierr = VecScatterBegin(sctx,coor_natural,perm_coors,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
  ierr = VecScatterEnd(  sctx,coor_natural,perm_coors,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);

  /* access */
  if (isActiveRank(psubcomm)) {
    Vec               _coors;
    const PetscScalar *LA_perm;
    PetscScalar       *LA_coors;

    ierr = DMGetCoordinates(subdm,&_coors);CHKERRQ(ierr);
    ierr = VecGetArrayRead(perm_coors,&LA_perm);CHKERRQ(ierr);
    ierr = VecGetArray(_coors,&LA_coors);CHKERRQ(ierr);
    for (i=0; i<Ml*Nl*Pl*3; i++) {
      LA_coors[i] = LA_perm[i];
    }
    ierr = VecRestoreArray(_coors,&LA_coors);CHKERRQ(ierr);
    ierr = VecRestoreArrayRead(perm_coors,&LA_perm);CHKERRQ(ierr);
  }

  /* update local coords */
  if (isActiveRank(psubcomm)) {
    DM  _dmc;
    Vec _coors,_coors_local;

    ierr = DMGetCoordinateDM(subdm,&_dmc);CHKERRQ(ierr);
    ierr = DMGetCoordinates(subdm,&_coors);CHKERRQ(ierr);
    ierr = DMGetCoordinatesLocal(subdm,&_coors_local);CHKERRQ(ierr);
    ierr = DMGlobalToLocalBegin(_dmc,_coors,INSERT_VALUES,_coors_local);CHKERRQ(ierr);
    ierr = DMGlobalToLocalEnd(_dmc,_coors,INSERT_VALUES,_coors_local);CHKERRQ(ierr);
  }

  ierr = VecScatterDestroy(&sctx);CHKERRQ(ierr);
  ierr = ISDestroy(&is_fine);CHKERRQ(ierr);
  ierr = PetscFree(fine_indices);CHKERRQ(ierr);
  ierr = ISDestroy(&is_local);CHKERRQ(ierr);
  ierr = VecDestroy(&perm_coors);CHKERRQ(ierr);
  ierr = VecDestroy(&coor_natural);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PCTelescopeSetUp_dmda_repart_coors"
PetscErrorCode PCTelescopeSetUp_dmda_repart_coors(PC pc,PC_Telescope sred,PC_Telescope_DMDACtx *ctx)
{
  PetscInt       dim;
  DM             dm,subdm;
  PetscSubcomm   psubcomm;
  MPI_Comm       comm;
  Vec            coor;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PCGetDM(pc,&dm);CHKERRQ(ierr);
  ierr = DMGetCoordinates(dm,&coor);CHKERRQ(ierr);
  if (!coor) PetscFunctionReturn(0);
  psubcomm = sred->psubcomm;
  comm = PetscSubcommParent(psubcomm);
  subdm = ctx->dmrepart;


  ierr = PetscInfo(pc,"PCTelescope: setting up the coordinates (DMDA)\n");CHKERRQ(ierr);
  ierr = DMDAGetInfo(dm,&dim,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);CHKERRQ(ierr);
  switch (dim) {
  case 1: SETERRQ(comm,PETSC_ERR_SUP,"Telescope: DMDA (1D) repartitioning not provided");
    break;
  case 2: PCTelescopeSetUp_dmda_repart_coors2d(psubcomm,dm,subdm);
    break;
  case 3: PCTelescopeSetUp_dmda_repart_coors3d(psubcomm,dm,subdm);
    break;
  }
  PetscFunctionReturn(0);
}

/* setup repartitioned dm */
#undef __FUNCT__
#define __FUNCT__ "PCTelescopeSetUp_dmda_repart"
PetscErrorCode PCTelescopeSetUp_dmda_repart(PC pc,PC_Telescope sred,PC_Telescope_DMDACtx *ctx)
{
  PetscErrorCode  ierr;
  DM              dm;
  PetscInt        dim,nx,ny,nz,ndof,nsw,sum,k;
  DMBoundaryType  bx,by,bz;
  DMDAStencilType stencil;
  const PetscInt  *_range_i_re;
  const PetscInt  *_range_j_re;
  const PetscInt  *_range_k_re;
  DMDAInterpolationType itype;
  PetscInt              refine_x,refine_y,refine_z;
  MPI_Comm              comm,subcomm;
  const char            *prefix;

  PetscFunctionBegin;
  comm = PetscSubcommParent(sred->psubcomm);
  subcomm = PetscSubcommChild(sred->psubcomm);
  ierr = PCGetDM(pc,&dm);CHKERRQ(ierr);

  ierr = DMDAGetInfo(dm,&dim,&nx,&ny,&nz,NULL,NULL,NULL,&ndof,&nsw,&bx,&by,&bz,&stencil);CHKERRQ(ierr);
  ierr = DMDAGetInterpolationType(dm,&itype);CHKERRQ(ierr);
  ierr = DMDAGetRefinementFactor(dm,&refine_x,&refine_y,&refine_z);CHKERRQ(ierr);

  ctx->dmrepart = NULL;
  _range_i_re = _range_j_re = _range_k_re = NULL;
  /* Create DMDA on the child communicator */
  if (isActiveRank(sred->psubcomm)) {
    switch (dim) {
    case 1:
      ierr = PetscInfo(pc,"PCTelescope: setting up the DMDA on comm subset (1D)\n");CHKERRQ(ierr);
      /*ierr = DMDACreate1d(subcomm,bx,nx,ndof,nsw,NULL,&ctx->dmrepart);CHKERRQ(ierr);*/
      ny = nz = 1;
      by = bz = DM_BOUNDARY_NONE;
      break;
    case 2:
      ierr = PetscInfo(pc,"PCTelescope: setting up the DMDA on comm subset (2D)\n");CHKERRQ(ierr);
      /*ierr = DMDACreate2d(subcomm,bx,by,stencil,nx,ny, PETSC_DECIDE,PETSC_DECIDE, ndof,nsw, NULL,NULL,&ctx->dmrepart);CHKERRQ(ierr);*/
      nz = 1;
      bz = DM_BOUNDARY_NONE;
      break;
    case 3:
      ierr = PetscInfo(pc,"PCTelescope: setting up the DMDA on comm subset (3D)\n");CHKERRQ(ierr);
      /*ierr = DMDACreate3d(subcomm,bx,by,bz,stencil,nx,ny,nz, PETSC_DECIDE,PETSC_DECIDE,PETSC_DECIDE, ndof,nsw, NULL,NULL,NULL,&ctx->dmrepart);CHKERRQ(ierr);*/
      break;
    }
    /*
     The API DMDACreate1d(), DMDACreate2d(), DMDACreate3d() does not allow us to set/append
     a unique option prefix for the DM, thus I prefer to expose the contents of these API's here.
     This allows users to control the partitioning of the subDM.
    */
    ierr = DMDACreate(subcomm,&ctx->dmrepart);CHKERRQ(ierr);
    /* Set unique option prefix name */
    ierr = DMGetOptionsPrefix(dm,&prefix);CHKERRQ(ierr);
    ierr = DMSetOptionsPrefix(ctx->dmrepart,prefix);CHKERRQ(ierr);
    ierr = DMAppendOptionsPrefix(ctx->dmrepart,"repart_");CHKERRQ(ierr);
    /* standard setup from DMDACreate{1,2,3}d() */
    ierr = DMSetDimension(ctx->dmrepart,dim);CHKERRQ(ierr);
    ierr = DMDASetSizes(ctx->dmrepart,nx,ny,nz);CHKERRQ(ierr);
    ierr = DMDASetNumProcs(ctx->dmrepart,PETSC_DECIDE,PETSC_DECIDE,PETSC_DECIDE);CHKERRQ(ierr);
    ierr = DMDASetBoundaryType(ctx->dmrepart,bx,by,bz);CHKERRQ(ierr);
    ierr = DMDASetDof(ctx->dmrepart,ndof);CHKERRQ(ierr);
    ierr = DMDASetStencilType(ctx->dmrepart,stencil);CHKERRQ(ierr);
    ierr = DMDASetStencilWidth(ctx->dmrepart,nsw);CHKERRQ(ierr);
    ierr = DMDASetOwnershipRanges(ctx->dmrepart,NULL,NULL,NULL);CHKERRQ(ierr);
    ierr = DMSetFromOptions(ctx->dmrepart);CHKERRQ(ierr);
    ierr = DMSetUp(ctx->dmrepart);CHKERRQ(ierr);
    /* Set refinement factors and interpolation type from the partent */
    ierr = DMDASetRefinementFactor(ctx->dmrepart,refine_x,refine_y,refine_z);CHKERRQ(ierr);
    ierr = DMDASetInterpolationType(ctx->dmrepart,itype);CHKERRQ(ierr);

    ierr = DMDAGetInfo(ctx->dmrepart,NULL,NULL,NULL,NULL,&ctx->Mp_re,&ctx->Np_re,&ctx->Pp_re,NULL,NULL,NULL,NULL,NULL,NULL);CHKERRQ(ierr);
    ierr = DMDAGetOwnershipRanges(ctx->dmrepart,&_range_i_re,&_range_j_re,&_range_k_re);CHKERRQ(ierr);
  }

  /* generate ranges for repartitioned dm */
  /* note - assume rank 0 always participates */
  ierr = MPI_Bcast(&ctx->Mp_re,1,MPIU_INT,0,comm);CHKERRQ(ierr);
  ierr = MPI_Bcast(&ctx->Np_re,1,MPIU_INT,0,comm);CHKERRQ(ierr);
  ierr = MPI_Bcast(&ctx->Pp_re,1,MPIU_INT,0,comm);CHKERRQ(ierr);

  ierr = PetscCalloc1(ctx->Mp_re,&ctx->range_i_re);CHKERRQ(ierr);
  ierr = PetscCalloc1(ctx->Np_re,&ctx->range_j_re);CHKERRQ(ierr);
  ierr = PetscCalloc1(ctx->Pp_re,&ctx->range_k_re);CHKERRQ(ierr);

  if (_range_i_re) {ierr = PetscMemcpy(ctx->range_i_re,_range_i_re,sizeof(PetscInt)*ctx->Mp_re);CHKERRQ(ierr);}
  if (_range_j_re) {ierr = PetscMemcpy(ctx->range_j_re,_range_j_re,sizeof(PetscInt)*ctx->Np_re);CHKERRQ(ierr);}
  if (_range_k_re) {ierr = PetscMemcpy(ctx->range_k_re,_range_k_re,sizeof(PetscInt)*ctx->Pp_re);CHKERRQ(ierr);}

  ierr = MPI_Bcast(ctx->range_i_re,ctx->Mp_re,MPIU_INT,0,comm);CHKERRQ(ierr);
  ierr = MPI_Bcast(ctx->range_j_re,ctx->Np_re,MPIU_INT,0,comm);CHKERRQ(ierr);
  ierr = MPI_Bcast(ctx->range_k_re,ctx->Pp_re,MPIU_INT,0,comm);CHKERRQ(ierr);

  ierr = PetscMalloc1(ctx->Mp_re,&ctx->start_i_re);CHKERRQ(ierr);
  ierr = PetscMalloc1(ctx->Np_re,&ctx->start_j_re);CHKERRQ(ierr);
  ierr = PetscMalloc1(ctx->Pp_re,&ctx->start_k_re);CHKERRQ(ierr);

  sum = 0;
  for (k=0; k<ctx->Mp_re; k++) {
    ctx->start_i_re[k] = sum;
    sum += ctx->range_i_re[k];
  }

  sum = 0;
  for (k=0; k<ctx->Np_re; k++) {
    ctx->start_j_re[k] = sum;
    sum += ctx->range_j_re[k];
  }

  sum = 0;
  for (k=0; k<ctx->Pp_re; k++) {
    ctx->start_k_re[k] = sum;
    sum += ctx->range_k_re[k];
  }

  /* attach dm to ksp on sub communicator */
  if (isActiveRank(sred->psubcomm)) {
    ierr = KSPSetDM(sred->ksp,ctx->dmrepart);CHKERRQ(ierr);
    ierr = KSPSetDMActive(sred->ksp,PETSC_FALSE);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PCTelescopeSetUp_dmda_permutation_3d"
PetscErrorCode PCTelescopeSetUp_dmda_permutation_3d(PC pc,PC_Telescope sred,PC_Telescope_DMDACtx *ctx)
{
  PetscErrorCode ierr;
  DM             dm;
  MPI_Comm       comm;
  Mat            Pscalar,P;
  PetscInt       ndof;
  PetscInt       i,j,k,location,startI[3],endI[3],lenI[3],nx,ny,nz;
  PetscInt       sr,er,Mr;
  Vec            V;

  PetscFunctionBegin;
  ierr = PetscInfo(pc,"PCTelescope: setting up the permutation matrix (DMDA-3D)\n");CHKERRQ(ierr);
  ierr = PetscObjectGetComm((PetscObject)pc,&comm);CHKERRQ(ierr);

  ierr = PCGetDM(pc,&dm);CHKERRQ(ierr);
  ierr = DMDAGetInfo(dm,NULL,&nx,&ny,&nz,NULL,NULL,NULL,&ndof,NULL,NULL,NULL,NULL,NULL);CHKERRQ(ierr);

  ierr = DMGetGlobalVector(dm,&V);CHKERRQ(ierr);
  ierr = VecGetSize(V,&Mr);CHKERRQ(ierr);
  ierr = VecGetOwnershipRange(V,&sr,&er);CHKERRQ(ierr);
  ierr = DMRestoreGlobalVector(dm,&V);CHKERRQ(ierr);
  sr = sr / ndof;
  er = er / ndof;
  Mr = Mr / ndof;

  ierr = MatCreate(comm,&Pscalar);CHKERRQ(ierr);
  ierr = MatSetSizes(Pscalar,(er-sr),(er-sr),Mr,Mr);CHKERRQ(ierr);
  ierr = MatSetType(Pscalar,MATAIJ);CHKERRQ(ierr);
  ierr = MatSeqAIJSetPreallocation(Pscalar,2,NULL);CHKERRQ(ierr);
  ierr = MatMPIAIJSetPreallocation(Pscalar,2,NULL,2,NULL);CHKERRQ(ierr);

  ierr = DMDAGetCorners(dm,NULL,NULL,NULL,&lenI[0],&lenI[1],&lenI[2]);CHKERRQ(ierr);
  ierr = DMDAGetCorners(dm,&startI[0],&startI[1],&startI[2],&endI[0],&endI[1],&endI[2]);CHKERRQ(ierr);
  endI[0] += startI[0];
  endI[1] += startI[1];
  endI[2] += startI[2];

  for (k=startI[2]; k<endI[2]; k++) {
    for (j=startI[1]; j<endI[1]; j++) {
      for (i=startI[0]; i<endI[0]; i++) {
        PetscMPIInt rank_ijk_re,rank_reI[3];
        PetscInt    s0_re;
        PetscInt    ii,jj,kk,local_ijk_re,mapped_ijk;
        PetscInt    lenI_re[3];

        location = (i - startI[0]) + (j - startI[1])*lenI[0] + (k - startI[2])*lenI[0]*lenI[1];
        ierr = _DMDADetermineRankFromGlobalIJK(3,i,j,k,   ctx->Mp_re,ctx->Np_re,ctx->Pp_re,
                                               ctx->start_i_re,ctx->start_j_re,ctx->start_k_re,
                                               ctx->range_i_re,ctx->range_j_re,ctx->range_k_re,
                                               &rank_reI[0],&rank_reI[1],&rank_reI[2],&rank_ijk_re);CHKERRQ(ierr);
        ierr = _DMDADetermineGlobalS0(3,rank_ijk_re, ctx->Mp_re,ctx->Np_re,ctx->Pp_re, ctx->range_i_re,ctx->range_j_re,ctx->range_k_re, &s0_re);CHKERRQ(ierr);
        ii = i - ctx->start_i_re[ rank_reI[0] ];
        if (ii < 0) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_USER,"[dmdarepart-perm3d] index error ii");
        jj = j - ctx->start_j_re[ rank_reI[1] ];
        if (jj < 0) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_USER,"[dmdarepart-perm3d] index error jj");
        kk = k - ctx->start_k_re[ rank_reI[2] ];
        if (kk < 0) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_USER,"[dmdarepart-perm3d] index error kk");
        lenI_re[0] = ctx->range_i_re[ rank_reI[0] ];
        lenI_re[1] = ctx->range_j_re[ rank_reI[1] ];
        lenI_re[2] = ctx->range_k_re[ rank_reI[2] ];
        local_ijk_re = ii + jj * lenI_re[0] + kk * lenI_re[0] * lenI_re[1];
        mapped_ijk = s0_re + local_ijk_re;
        ierr = MatSetValue(Pscalar,sr+location,mapped_ijk,1.0,INSERT_VALUES);CHKERRQ(ierr);
      }
    }
  }
  ierr = MatAssemblyBegin(Pscalar,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(Pscalar,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatCreateMAIJ(Pscalar,ndof,&P);CHKERRQ(ierr);
  ierr = MatDestroy(&Pscalar);CHKERRQ(ierr);
  ctx->permutation = P;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PCTelescopeSetUp_dmda_permutation_2d"
PetscErrorCode PCTelescopeSetUp_dmda_permutation_2d(PC pc,PC_Telescope sred,PC_Telescope_DMDACtx *ctx)
{
  PetscErrorCode ierr;
  DM             dm;
  MPI_Comm       comm;
  Mat            Pscalar,P;
  PetscInt       ndof;
  PetscInt       i,j,location,startI[2],endI[2],lenI[2],nx,ny,nz;
  PetscInt       sr,er,Mr;
  Vec            V;

  PetscFunctionBegin;
  ierr = PetscInfo(pc,"PCTelescope: setting up the permutation matrix (DMDA-2D)\n");CHKERRQ(ierr);
  ierr = PetscObjectGetComm((PetscObject)pc,&comm);CHKERRQ(ierr);
  ierr = PCGetDM(pc,&dm);CHKERRQ(ierr);
  ierr = DMDAGetInfo(dm,NULL,&nx,&ny,&nz,NULL,NULL,NULL,&ndof,NULL,NULL,NULL,NULL,NULL);CHKERRQ(ierr);
  ierr = DMGetGlobalVector(dm,&V);CHKERRQ(ierr);
  ierr = VecGetSize(V,&Mr);CHKERRQ(ierr);
  ierr = VecGetOwnershipRange(V,&sr,&er);CHKERRQ(ierr);
  ierr = DMRestoreGlobalVector(dm,&V);CHKERRQ(ierr);
  sr = sr / ndof;
  er = er / ndof;
  Mr = Mr / ndof;

  ierr = MatCreate(comm,&Pscalar);CHKERRQ(ierr);
  ierr = MatSetSizes(Pscalar,(er-sr),(er-sr),Mr,Mr);CHKERRQ(ierr);
  ierr = MatSetType(Pscalar,MATAIJ);CHKERRQ(ierr);
  ierr = MatSeqAIJSetPreallocation(Pscalar,2,NULL);CHKERRQ(ierr);
  ierr = MatMPIAIJSetPreallocation(Pscalar,2,NULL,2,NULL);CHKERRQ(ierr);

  ierr = DMDAGetCorners(dm,NULL,NULL,NULL,&lenI[0],&lenI[1],NULL);CHKERRQ(ierr);
  ierr = DMDAGetCorners(dm,&startI[0],&startI[1],NULL,&endI[0],&endI[1],NULL);CHKERRQ(ierr);
  endI[0] += startI[0];
  endI[1] += startI[1];

  for (j=startI[1]; j<endI[1]; j++) {
    for (i=startI[0]; i<endI[0]; i++) {
      PetscMPIInt rank_ijk_re,rank_reI[3];
      PetscInt    s0_re;
      PetscInt    ii,jj,local_ijk_re,mapped_ijk;
      PetscInt    lenI_re[3];

      location = (i - startI[0]) + (j - startI[1])*lenI[0];
      ierr = _DMDADetermineRankFromGlobalIJK(2,i,j,0,   ctx->Mp_re,ctx->Np_re,ctx->Pp_re,
                                             ctx->start_i_re,ctx->start_j_re,ctx->start_k_re,
                                             ctx->range_i_re,ctx->range_j_re,ctx->range_k_re,
                                             &rank_reI[0],&rank_reI[1],NULL,&rank_ijk_re);CHKERRQ(ierr);

      ierr = _DMDADetermineGlobalS0(2,rank_ijk_re, ctx->Mp_re,ctx->Np_re,ctx->Pp_re, ctx->range_i_re,ctx->range_j_re,ctx->range_k_re, &s0_re);CHKERRQ(ierr);

      ii = i - ctx->start_i_re[ rank_reI[0] ];
      if (ii < 0) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_USER,"[dmdarepart-perm2d] index error ii");
      jj = j - ctx->start_j_re[ rank_reI[1] ];
      if (jj < 0) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_USER,"[dmdarepart-perm2d] index error jj");

      lenI_re[0] = ctx->range_i_re[ rank_reI[0] ];
      lenI_re[1] = ctx->range_j_re[ rank_reI[1] ];
      local_ijk_re = ii + jj * lenI_re[0];
      mapped_ijk = s0_re + local_ijk_re;
      ierr = MatSetValue(Pscalar,sr+location,mapped_ijk,1.0,INSERT_VALUES);CHKERRQ(ierr);
    }
  }
  ierr = MatAssemblyBegin(Pscalar,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(Pscalar,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatCreateMAIJ(Pscalar,ndof,&P);CHKERRQ(ierr);
  ierr = MatDestroy(&Pscalar);CHKERRQ(ierr);
  ctx->permutation = P;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PCTelescopeSetUp_dmda_scatters"
PetscErrorCode PCTelescopeSetUp_dmda_scatters(PC pc,PC_Telescope sred,PC_Telescope_DMDACtx *ctx)
{
  PetscErrorCode ierr;
  Vec            xred,yred,xtmp,x,xp;
  VecScatter     scatter;
  IS             isin;
  Mat            B;
  PetscInt       m,bs,st,ed;
  MPI_Comm       comm;

  PetscFunctionBegin;
  ierr = PetscObjectGetComm((PetscObject)pc,&comm);CHKERRQ(ierr);
  ierr = PCGetOperators(pc,NULL,&B);CHKERRQ(ierr);
  ierr = MatCreateVecs(B,&x,NULL);CHKERRQ(ierr);
  ierr = MatGetBlockSize(B,&bs);CHKERRQ(ierr);
  ierr = VecDuplicate(x,&xp);CHKERRQ(ierr);
  m = 0;
  xred = NULL;
  yred = NULL;
  if (isActiveRank(sred->psubcomm)) {
    ierr = DMCreateGlobalVector(ctx->dmrepart,&xred);CHKERRQ(ierr);
    ierr = VecDuplicate(xred,&yred);CHKERRQ(ierr);
    ierr = VecGetOwnershipRange(xred,&st,&ed);CHKERRQ(ierr);
    ierr = ISCreateStride(comm,ed-st,st,1,&isin);CHKERRQ(ierr);
    ierr = VecGetLocalSize(xred,&m);
  } else {
    ierr = VecGetOwnershipRange(x,&st,&ed);CHKERRQ(ierr);
    ierr = ISCreateStride(comm,0,st,1,&isin);CHKERRQ(ierr);
  }
  ierr = ISSetBlockSize(isin,bs);CHKERRQ(ierr);
  ierr = VecCreate(comm,&xtmp);CHKERRQ(ierr);
  ierr = VecSetSizes(xtmp,m,PETSC_DECIDE);CHKERRQ(ierr);
  ierr = VecSetBlockSize(xtmp,bs);CHKERRQ(ierr);
  ierr = VecSetType(xtmp,((PetscObject)x)->type_name);CHKERRQ(ierr);
  ierr = VecScatterCreate(x,isin,xtmp,NULL,&scatter);CHKERRQ(ierr);
  sred->xred    = xred;
  sred->yred    = yred;
  sred->isin    = isin;
  sred->scatter = scatter;
  sred->xtmp    = xtmp;

  ctx->xp       = xp;
  ierr = VecDestroy(&x);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PCTelescopeSetUp_dmda"
PetscErrorCode PCTelescopeSetUp_dmda(PC pc,PC_Telescope sred)
{
  PC_Telescope_DMDACtx *ctx;
  PetscInt                 dim;
  DM                       dm;
  MPI_Comm                 comm;
  PetscErrorCode           ierr;

  PetscFunctionBegin;
  ierr = PetscInfo(pc,"PCTelescope: setup (DMDA)\n");CHKERRQ(ierr);
  PetscMalloc1(1,&ctx);
  PetscMemzero(ctx,sizeof(PC_Telescope_DMDACtx));
  sred->dm_ctx = (void*)ctx;

  ierr = PetscObjectGetComm((PetscObject)pc,&comm);CHKERRQ(ierr);
  ierr = PCGetDM(pc,&dm);CHKERRQ(ierr);
  ierr = DMDAGetInfo(dm,&dim,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);CHKERRQ(ierr);

  PCTelescopeSetUp_dmda_repart(pc,sred,ctx);
  PCTelescopeSetUp_dmda_repart_coors(pc,sred,ctx);
  switch (dim) {
  case 1: SETERRQ(comm,PETSC_ERR_SUP,"Telescope: DMDA (1D) repartitioning not provided");
    break;
  case 2: ierr = PCTelescopeSetUp_dmda_permutation_2d(pc,sred,ctx);CHKERRQ(ierr);
    break;
  case 3: ierr = PCTelescopeSetUp_dmda_permutation_3d(pc,sred,ctx);CHKERRQ(ierr);
    break;
  }
  ierr = PCTelescopeSetUp_dmda_scatters(pc,sred,ctx);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PCTelescopeMatCreate_dmda"
PetscErrorCode PCTelescopeMatCreate_dmda(PC pc,PC_Telescope sred,MatReuse reuse,Mat *A)
{
  PetscErrorCode ierr;
  PC_Telescope_DMDACtx *ctx;
  MPI_Comm       comm,subcomm;
  Mat            Bperm,Bred,B,P;
  PetscInt       nr,nc;
  IS             isrow,iscol;
  Mat            Blocal,*_Blocal;

  PetscFunctionBegin;
  ierr = PetscInfo(pc,"PCTelescope: updating the redundant preconditioned operator (DMDA)\n");CHKERRQ(ierr);
  ierr = PetscObjectGetComm((PetscObject)pc,&comm);CHKERRQ(ierr);
    subcomm = PetscSubcommChild(sred->psubcomm);
  ctx = (PC_Telescope_DMDACtx*)sred->dm_ctx;

  ierr = PCGetOperators(pc,NULL,&B);CHKERRQ(ierr);
  ierr = MatGetSize(B,&nr,&nc);CHKERRQ(ierr);

  P = ctx->permutation;
  ierr = MatPtAP(B,P,MAT_INITIAL_MATRIX,1.1,&Bperm);CHKERRQ(ierr);

  /* Get submatrices */
  isrow = sred->isin;
  ierr = ISCreateStride(comm,nc,0,1,&iscol);CHKERRQ(ierr);

  ierr = MatGetSubMatrices(Bperm,1,&isrow,&iscol,MAT_INITIAL_MATRIX,&_Blocal);CHKERRQ(ierr);
  Blocal = *_Blocal;
  Bred = NULL;
  if (isActiveRank(sred->psubcomm)) {
    PetscInt mm;

    if (reuse != MAT_INITIAL_MATRIX) {Bred = *A;}
    ierr = MatGetSize(Blocal,&mm,NULL);CHKERRQ(ierr);
    /* ierr = MatCreateMPIMatConcatenateSeqMat(subcomm,Blocal,PETSC_DECIDE,reuse,&Bred);CHKERRQ(ierr); */
    ierr = MatCreateMPIMatConcatenateSeqMat(subcomm,Blocal,mm,reuse,&Bred);CHKERRQ(ierr);
  }
  *A = Bred;

  ierr = ISDestroy(&iscol);CHKERRQ(ierr);
  ierr = MatDestroy(&Bperm);CHKERRQ(ierr);
  ierr = MatDestroyMatrices(1,&_Blocal);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PCTelescopeMatNullSpaceCreate_dmda"
PetscErrorCode PCTelescopeMatNullSpaceCreate_dmda(PC pc,PC_Telescope sred,Mat sub_mat)
{
  PetscErrorCode   ierr;
  MatNullSpace     nullspace,sub_nullspace;
  Mat              A,B;
  PetscBool        has_const;
  PetscInt         i,k,n;
  const Vec        *vecs;
  Vec              *sub_vecs;
  MPI_Comm         subcomm;
  PC_Telescope_DMDACtx *ctx;

  PetscFunctionBegin;
  ierr = PCGetOperators(pc,&A,&B);CHKERRQ(ierr);
  ierr = MatGetNullSpace(B,&nullspace);CHKERRQ(ierr);
  if (!nullspace) return(0);

  ierr = PetscInfo(pc,"PCTelescope: generating nullspace (DMDA)\n");CHKERRQ(ierr);
  ctx = (PC_Telescope_DMDACtx*)sred->dm_ctx;
  subcomm = PetscSubcommChild(sred->psubcomm);
  ierr = MatNullSpaceGetVecs(nullspace,&has_const,&n,&vecs);CHKERRQ(ierr);

  if (isActiveRank(sred->psubcomm)) {
    /* create new vectors */
    if (n) {
      ierr = VecDuplicateVecs(sred->xred,n,&sub_vecs);CHKERRQ(ierr);
    }
  }

  /* copy entries */
  for (k=0; k<n; k++) {
    const PetscScalar *x_array;
    PetscScalar       *LA_sub_vec;
    PetscInt          st,ed,bs;

    /* permute vector into ordering associated with re-partitioned dmda */
    ierr = MatMultTranspose(ctx->permutation,vecs[k],ctx->xp);CHKERRQ(ierr);

    /* pull in vector x->xtmp */
    ierr = VecScatterBegin(sred->scatter,ctx->xp,sred->xtmp,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
    ierr = VecScatterEnd(sred->scatter,ctx->xp,sred->xtmp,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);

    /* copy vector entires into xred */
    ierr = VecGetBlockSize(sred->xtmp,&bs);CHKERRQ(ierr);
    ierr = VecGetArrayRead(sred->xtmp,&x_array);CHKERRQ(ierr);
    if (sub_vecs[k]) {
      ierr = VecGetOwnershipRange(sub_vecs[k],&st,&ed);CHKERRQ(ierr);
      ierr = VecGetArray(sub_vecs[k],&LA_sub_vec);CHKERRQ(ierr);
      for (i=0; i<ed-st; i++) {
        LA_sub_vec[i] = x_array[i];
      }
      ierr = VecRestoreArray(sub_vecs[k],&LA_sub_vec);CHKERRQ(ierr);
    }
    ierr = VecRestoreArrayRead(sred->xtmp,&x_array);CHKERRQ(ierr);
  }

  if (isActiveRank(sred->psubcomm)) {
    /* create new nullspace for redundant object */
    ierr = MatNullSpaceCreate(subcomm,has_const,n,sub_vecs,&sub_nullspace);CHKERRQ(ierr);

    /* attach redundant nullspace to Bred */
    ierr = MatSetNullSpace(sub_mat,sub_nullspace);CHKERRQ(ierr);
    ierr = VecDestroyVecs(n,&sub_vecs);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PCApply_Telescope_dmda"
PetscErrorCode PCApply_Telescope_dmda(PC pc,Vec x,Vec y)
{
  PC_Telescope      sred = (PC_Telescope)pc->data;
  PetscErrorCode    ierr;
  Mat               perm;
  Vec               xtmp,xp,xred,yred;
  PetscInt          i,st,ed,bs;
  VecScatter        scatter;
  PetscScalar       *array;
  const PetscScalar *x_array;
  PC_Telescope_DMDACtx *ctx;

  ctx = (PC_Telescope_DMDACtx*)sred->dm_ctx;
  xtmp    = sred->xtmp;
  scatter = sred->scatter;
  xred    = sred->xred;
  yred    = sred->yred;
  perm  = ctx->permutation;
  xp    = ctx->xp;

  PetscFunctionBegin;
  /* permute vector into ordering associated with re-partitioned dmda */
  ierr = MatMultTranspose(perm,x,xp);CHKERRQ(ierr);

  /* pull in vector x->xtmp */
  ierr = VecScatterBegin(scatter,xp,xtmp,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
  ierr = VecScatterEnd(scatter,xp,xtmp,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);

  /* copy vector entires into xred */
  ierr = VecGetBlockSize(xtmp,&bs);CHKERRQ(ierr);
  ierr = VecGetArrayRead(xtmp,&x_array);CHKERRQ(ierr);
  if (xred) {
    PetscScalar *LA_xred;
    ierr = VecGetOwnershipRange(xred,&st,&ed);CHKERRQ(ierr);

    ierr = VecGetArray(xred,&LA_xred);CHKERRQ(ierr);
    for (i=0; i<ed-st; i++) {
      LA_xred[i] = x_array[i];
    }
    ierr = VecRestoreArray(xred,&LA_xred);CHKERRQ(ierr);
  }
  ierr = VecRestoreArrayRead(xtmp,&x_array);CHKERRQ(ierr);

  /* solve */
  if (isActiveRank(sred->psubcomm)) {
    ierr = KSPSolve(sred->ksp,xred,yred);CHKERRQ(ierr);
  }

  /* return vector */
  ierr = VecGetBlockSize(xtmp,&bs);CHKERRQ(ierr);
  ierr = VecGetArray(xtmp,&array);CHKERRQ(ierr);
  if (yred) {
    const PetscScalar *LA_yred;
    ierr = VecGetOwnershipRange(yred,&st,&ed);CHKERRQ(ierr);
    ierr = VecGetArrayRead(yred,&LA_yred);CHKERRQ(ierr);
    for (i=0; i<ed-st; i++) {
      array[i] = LA_yred[i];
    }
    ierr = VecRestoreArrayRead(yred,&LA_yred);CHKERRQ(ierr);
  }
  ierr = VecRestoreArray(xtmp,&array);CHKERRQ(ierr);
  ierr = VecScatterBegin(scatter,xtmp,xp,INSERT_VALUES,SCATTER_REVERSE);CHKERRQ(ierr);
  ierr = VecScatterEnd(scatter,xtmp,xp,INSERT_VALUES,SCATTER_REVERSE);CHKERRQ(ierr);
  ierr = MatMult(perm,xp,y);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PCReset_Telescope_dmda"
PetscErrorCode PCReset_Telescope_dmda(PC pc)
{
  PetscErrorCode       ierr;
  PC_Telescope         sred = (PC_Telescope)pc->data;
  PC_Telescope_DMDACtx *ctx;

  PetscFunctionBegin;
  ctx = (PC_Telescope_DMDACtx*)sred->dm_ctx;
  ierr = VecDestroy(&ctx->xp);CHKERRQ(ierr);
  ierr = MatDestroy(&ctx->permutation);CHKERRQ(ierr);
  ierr = DMDestroy(&ctx->dmrepart);CHKERRQ(ierr);
  ierr = PetscFree(ctx->range_i_re);CHKERRQ(ierr);
  ierr = PetscFree(ctx->range_j_re);CHKERRQ(ierr);
  ierr = PetscFree(ctx->range_k_re);CHKERRQ(ierr);
  ierr = PetscFree(ctx->start_i_re);CHKERRQ(ierr);
  ierr = PetscFree(ctx->start_j_re);CHKERRQ(ierr);
  ierr = PetscFree(ctx->start_k_re);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMView_DMDAShort_3d"
PetscErrorCode DMView_DMDAShort_3d(DM dm,PetscViewer v)
{
  PetscInt       M,N,P,m,n,p,ndof,nsw;
  MPI_Comm       comm;
  PetscMPIInt    size;
  const char*    prefix;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscObjectGetComm((PetscObject)dm,&comm);CHKERRQ(ierr);
  ierr = MPI_Comm_size(comm,&size);CHKERRQ(ierr);
  ierr = DMGetOptionsPrefix(dm,&prefix);CHKERRQ(ierr);
  ierr = DMDAGetInfo(dm,NULL,&M,&N,&P,&m,&n,&p,&ndof,&nsw,NULL,NULL,NULL,NULL);CHKERRQ(ierr);
  if (prefix) {ierr = PetscViewerASCIIPrintf(v,"DMDA Object:    (%s)    %d MPI processes\n",prefix,size);CHKERRQ(ierr);}
  else {ierr = PetscViewerASCIIPrintf(v,"DMDA Object:    %d MPI processes\n",size);CHKERRQ(ierr);}
  ierr = PetscViewerASCIIPrintf(v,"  M %D N %D P %D m %D n %D p %D dof %D overlap %D\n",M,N,P,m,n,p,ndof,nsw);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMView_DMDAShort_2d"
PetscErrorCode DMView_DMDAShort_2d(DM dm,PetscViewer v)
{
  PetscInt       M,N,m,n,ndof,nsw;
  MPI_Comm       comm;
  PetscMPIInt    size;
  const char*    prefix;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscObjectGetComm((PetscObject)dm,&comm);CHKERRQ(ierr);
  ierr = MPI_Comm_size(comm,&size);CHKERRQ(ierr);
  ierr = DMGetOptionsPrefix(dm,&prefix);CHKERRQ(ierr);
  ierr = DMDAGetInfo(dm,NULL,&M,&N,NULL,&m,&n,NULL,&ndof,&nsw,NULL,NULL,NULL,NULL);CHKERRQ(ierr);
  if (prefix) {PetscViewerASCIIPrintf(v,"DMDA Object:    (%s)    %d MPI processes\n",prefix,size);CHKERRQ(ierr);}
  else {ierr = PetscViewerASCIIPrintf(v,"DMDA Object:    %d MPI processes\n",size);CHKERRQ(ierr);} 
  ierr = PetscViewerASCIIPrintf(v,"  M %D N %D m %D n %D dof %D overlap %D\n",M,N,m,n,ndof,nsw);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMView_DMDAShort"
PetscErrorCode DMView_DMDAShort(DM dm,PetscViewer v)
{
  PetscErrorCode ierr;
  PetscInt       dim;

  PetscFunctionBegin;
  ierr = DMDAGetInfo(dm,&dim,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);CHKERRQ(ierr);
  switch (dim) {
  case 2: ierr = DMView_DMDAShort_2d(dm,v);CHKERRQ(ierr);
    break;
  case 3: ierr = DMView_DMDAShort_3d(dm,v);CHKERRQ(ierr);
    break;
  }
  PetscFunctionReturn(0);
}

