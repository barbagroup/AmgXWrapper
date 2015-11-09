#include <petsc/private/snesimpl.h>             /*I   "petscsnes.h"   I*/
#include <petscdm.h>

typedef struct {
  PetscInt   n;                   /* local subdomains */
  SNES       *subsnes;            /* nonlinear solvers for each subdomain */
  Vec        *x;                  /* solution vectors */
  Vec        *xl;                 /* solution local vectors */
  Vec        *y;                  /* step vectors */
  Vec        *b;                  /* rhs vectors */
  VecScatter *oscatter;           /* scatter from global space to the subdomain global space */
  VecScatter *iscatter;           /* scatter from global space to the nonoverlapping subdomain space */
  VecScatter *gscatter;           /* scatter from global space to the subdomain local space */
  PCASMType  type;                /* ASM type */
  PetscBool  usesdm;              /* use the DM for setting up the subproblems */
  PetscBool  finaljacobian;       /* compute the jacobian of the converged solution */
  PetscReal  damping;             /* damping parameter for updates from the blocks */
  PetscBool  same_local_solves;   /* flag to determine if the solvers have been individually modified */

  /* logging events */
  PetscLogEvent eventrestrictinterp;
  PetscLogEvent eventsubsolve;

  PetscInt      fjtype;            /* type of computed jacobian */
  Vec           xinit;             /* initial solution in case the final jacobian type is computed as first */
} SNES_NASM;

const char *const SNESNASMTypes[] = {"NONE","RESTRICT","INTERPOLATE","BASIC","PCASMType","PC_ASM_",0};
const char *const SNESNASMFJTypes[] = {"FINALOUTER","FINALINNER","INITIAL"};

#undef __FUNCT__
#define __FUNCT__ "SNESReset_NASM"
PetscErrorCode SNESReset_NASM(SNES snes)
{
  SNES_NASM      *nasm = (SNES_NASM*)snes->data;
  PetscErrorCode ierr;
  PetscInt       i;

  PetscFunctionBegin;
  for (i=0; i<nasm->n; i++) {
    if (nasm->xl) { ierr = VecDestroy(&nasm->xl[i]);CHKERRQ(ierr); }
    if (nasm->x) { ierr = VecDestroy(&nasm->x[i]);CHKERRQ(ierr); }
    if (nasm->y) { ierr = VecDestroy(&nasm->y[i]);CHKERRQ(ierr); }
    if (nasm->b) { ierr = VecDestroy(&nasm->b[i]);CHKERRQ(ierr); }

    if (nasm->subsnes) { ierr = SNESDestroy(&nasm->subsnes[i]);CHKERRQ(ierr); }
    if (nasm->oscatter) { ierr = VecScatterDestroy(&nasm->oscatter[i]);CHKERRQ(ierr); }
    if (nasm->iscatter) { ierr = VecScatterDestroy(&nasm->iscatter[i]);CHKERRQ(ierr); }
    if (nasm->gscatter) { ierr = VecScatterDestroy(&nasm->gscatter[i]);CHKERRQ(ierr); }
  }

  if (nasm->x) {ierr = PetscFree(nasm->x);CHKERRQ(ierr);}
  if (nasm->xl) {ierr = PetscFree(nasm->xl);CHKERRQ(ierr);}
  if (nasm->y) {ierr = PetscFree(nasm->y);CHKERRQ(ierr);}
  if (nasm->b) {ierr = PetscFree(nasm->b);CHKERRQ(ierr);}

  if (nasm->xinit) {ierr = VecDestroy(&nasm->xinit);CHKERRQ(ierr);}

  if (nasm->subsnes) {ierr = PetscFree(nasm->subsnes);CHKERRQ(ierr);}
  if (nasm->oscatter) {ierr = PetscFree(nasm->oscatter);CHKERRQ(ierr);}
  if (nasm->iscatter) {ierr = PetscFree(nasm->iscatter);CHKERRQ(ierr);}
  if (nasm->gscatter) {ierr = PetscFree(nasm->gscatter);CHKERRQ(ierr);}

  nasm->eventrestrictinterp = 0;
  nasm->eventsubsolve = 0;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "SNESDestroy_NASM"
PetscErrorCode SNESDestroy_NASM(SNES snes)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = SNESReset_NASM(snes);CHKERRQ(ierr);
  ierr = PetscFree(snes->data);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMGlobalToLocalSubDomainDirichletHook_Private"
PetscErrorCode DMGlobalToLocalSubDomainDirichletHook_Private(DM dm,Vec g,InsertMode mode,Vec l,void *ctx)
{
  PetscErrorCode ierr;
  Vec            bcs = (Vec)ctx;

  PetscFunctionBegin;
  ierr = VecCopy(bcs,l);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "SNESSetUp_NASM"
PetscErrorCode SNESSetUp_NASM(SNES snes)
{
  SNES_NASM      *nasm = (SNES_NASM*)snes->data;
  PetscErrorCode ierr;
  DM             dm,subdm;
  DM             *subdms;
  PetscInt       i;
  const char     *optionsprefix;
  Vec            F;
  PetscMPIInt    size;
  KSP            ksp;
  PC             pc;

  PetscFunctionBegin;
  if (!nasm->subsnes) {
    ierr = SNESGetDM(snes,&dm);CHKERRQ(ierr);
    if (dm) {
      nasm->usesdm = PETSC_TRUE;
      ierr         = DMCreateDomainDecomposition(dm,&nasm->n,NULL,NULL,NULL,&subdms);CHKERRQ(ierr);
      if (!subdms) SETERRQ(PetscObjectComm((PetscObject)dm),PETSC_ERR_ARG_WRONGSTATE,"DM has no default decomposition defined.  Set subsolves manually with SNESNASMSetSubdomains().");
      ierr = DMCreateDomainDecompositionScatters(dm,nasm->n,subdms,&nasm->iscatter,&nasm->oscatter,&nasm->gscatter);CHKERRQ(ierr);

      ierr = SNESGetOptionsPrefix(snes, &optionsprefix);CHKERRQ(ierr);
      ierr = PetscMalloc1(nasm->n,&nasm->subsnes);CHKERRQ(ierr);
      for (i=0; i<nasm->n; i++) {
        ierr = SNESCreate(PETSC_COMM_SELF,&nasm->subsnes[i]);CHKERRQ(ierr);
        ierr = SNESAppendOptionsPrefix(nasm->subsnes[i],optionsprefix);CHKERRQ(ierr);
        ierr = SNESAppendOptionsPrefix(nasm->subsnes[i],"sub_");CHKERRQ(ierr);
        ierr = SNESSetDM(nasm->subsnes[i],subdms[i]);CHKERRQ(ierr);
        ierr = MPI_Comm_size(PetscObjectComm((PetscObject)nasm->subsnes[i]),&size);CHKERRQ(ierr);
        if (size == 1) {
          ierr = SNESGetKSP(nasm->subsnes[i],&ksp);CHKERRQ(ierr);
          ierr = KSPGetPC(ksp,&pc);CHKERRQ(ierr);
          ierr = KSPSetType(ksp,KSPPREONLY);CHKERRQ(ierr);
          ierr = PCSetType(pc,PCLU);CHKERRQ(ierr);
        }
        ierr = SNESSetFromOptions(nasm->subsnes[i]);CHKERRQ(ierr);
        ierr = DMDestroy(&subdms[i]);CHKERRQ(ierr);
      }
      ierr = PetscFree(subdms);CHKERRQ(ierr);
    } else SETERRQ(PetscObjectComm((PetscObject)snes),PETSC_ERR_ARG_WRONGSTATE,"Cannot construct local problems automatically without a DM!");
  } else SETERRQ(PetscObjectComm((PetscObject)snes),PETSC_ERR_ARG_WRONGSTATE,"Must set subproblems manually if there is no DM!");
  /* allocate the global vectors */
  if (!nasm->x) {
    ierr = PetscCalloc1(nasm->n,&nasm->x);CHKERRQ(ierr);
  }
  if (!nasm->xl) {
    ierr = PetscCalloc1(nasm->n,&nasm->xl);CHKERRQ(ierr);
  }
  if (!nasm->y) {
    ierr = PetscCalloc1(nasm->n,&nasm->y);CHKERRQ(ierr);
  }
  if (!nasm->b) {
    ierr = PetscCalloc1(nasm->n,&nasm->b);CHKERRQ(ierr);
  }

  for (i=0; i<nasm->n; i++) {
    ierr = SNESGetFunction(nasm->subsnes[i],&F,NULL,NULL);CHKERRQ(ierr);
    if (!nasm->x[i]) {ierr = VecDuplicate(F,&nasm->x[i]);CHKERRQ(ierr);}
    if (!nasm->y[i]) {ierr = VecDuplicate(F,&nasm->y[i]);CHKERRQ(ierr);}
    if (!nasm->b[i]) {ierr = VecDuplicate(F,&nasm->b[i]);CHKERRQ(ierr);}
    if (!nasm->xl[i]) {
      ierr = SNESGetDM(nasm->subsnes[i],&subdm);CHKERRQ(ierr);
      ierr = DMCreateLocalVector(subdm,&nasm->xl[i]);CHKERRQ(ierr);
    }
    ierr = DMGlobalToLocalHookAdd(subdm,DMGlobalToLocalSubDomainDirichletHook_Private,NULL,nasm->xl[i]);CHKERRQ(ierr);
  }
  if (nasm->finaljacobian) {
    ierr = SNESSetUpMatrices(snes);CHKERRQ(ierr);
    if (nasm->fjtype == 2) {
      ierr = VecDuplicate(snes->vec_sol,&nasm->xinit);CHKERRQ(ierr);
    }
    for (i=0; i<nasm->n;i++) {
      ierr = SNESSetUpMatrices(nasm->subsnes[i]);CHKERRQ(ierr);
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "SNESSetFromOptions_NASM"
PetscErrorCode SNESSetFromOptions_NASM(PetscOptionItems *PetscOptionsObject,SNES snes)
{
  PetscErrorCode    ierr;
  PCASMType         asmtype;
  PetscBool         flg,monflg,subviewflg;
  SNES_NASM         *nasm = (SNES_NASM*)snes->data;

  PetscFunctionBegin;
  ierr = PetscOptionsHead(PetscOptionsObject,"Nonlinear Additive Schwartz options");CHKERRQ(ierr);
  ierr = PetscOptionsEnum("-snes_nasm_type","Type of restriction/extension","",SNESNASMTypes,(PetscEnum)nasm->type,(PetscEnum*)&asmtype,&flg);CHKERRQ(ierr);
  if (flg) {ierr = SNESNASMSetType(snes,asmtype);CHKERRQ(ierr);}
  flg    = PETSC_FALSE;
  monflg = PETSC_TRUE;
  ierr   = PetscOptionsReal("-snes_nasm_damping","Log times for subSNES solves and restriction","SNESNASMSetDamping",nasm->damping,&nasm->damping,&flg);CHKERRQ(ierr);
  if (flg) {ierr = SNESNASMSetDamping(snes,nasm->damping);CHKERRQ(ierr);}
  subviewflg = PETSC_FALSE;
  ierr   = PetscOptionsBool("-snes_nasm_sub_view","Print detailed information for every processor when using -snes_view","",subviewflg,&subviewflg,&flg);CHKERRQ(ierr);
  if (flg) {
    nasm->same_local_solves = PETSC_FALSE;
    if (!subviewflg) {
      nasm->same_local_solves = PETSC_TRUE;
    }
  }
  ierr   = PetscOptionsBool("-snes_nasm_finaljacobian","Compute the global jacobian of the final iterate (for ASPIN)","",nasm->finaljacobian,&nasm->finaljacobian,NULL);CHKERRQ(ierr);
  ierr   = PetscOptionsEList("-snes_nasm_finaljacobian_type","The type of the final jacobian computed.","",SNESNASMFJTypes,3,SNESNASMFJTypes[0],&nasm->fjtype,NULL);CHKERRQ(ierr);
  ierr   = PetscOptionsBool("-snes_nasm_log","Log times for subSNES solves and restriction","",monflg,&monflg,&flg);CHKERRQ(ierr);
  if (flg) {
    ierr = PetscLogEventRegister("SNESNASMSubSolve",((PetscObject)snes)->classid,&nasm->eventsubsolve);CHKERRQ(ierr);
    ierr = PetscLogEventRegister("SNESNASMRestrict",((PetscObject)snes)->classid,&nasm->eventrestrictinterp);CHKERRQ(ierr);
  }
  ierr = PetscOptionsTail();CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "SNESView_NASM"
PetscErrorCode SNESView_NASM(SNES snes, PetscViewer viewer)
{
  SNES_NASM      *nasm = (SNES_NASM*)snes->data;
  PetscErrorCode ierr;
  PetscMPIInt    rank,size;
  PetscInt       i,N,bsz;
  PetscBool      iascii,isstring;
  PetscViewer    sviewer;
  MPI_Comm       comm;

  PetscFunctionBegin;
  ierr = PetscObjectGetComm((PetscObject)snes,&comm);CHKERRQ(ierr);
  ierr = PetscObjectTypeCompare((PetscObject)viewer,PETSCVIEWERASCII,&iascii);CHKERRQ(ierr);
  ierr = PetscObjectTypeCompare((PetscObject)viewer,PETSCVIEWERSTRING,&isstring);CHKERRQ(ierr);
  ierr = MPI_Comm_rank(comm,&rank);CHKERRQ(ierr);
  ierr = MPI_Comm_size(comm,&size);CHKERRQ(ierr);
  ierr = MPIU_Allreduce(&nasm->n,&N,1,MPIU_INT,MPI_SUM,comm);CHKERRQ(ierr);
  if (iascii) {
    ierr = PetscViewerASCIIPrintf(viewer, "  Nonlinear Additive Schwarz: total subdomain blocks = %D\n",N);CHKERRQ(ierr);
    if (nasm->same_local_solves) {
      if (nasm->subsnes) {
        ierr = PetscViewerASCIIPrintf(viewer,"  Local solve is the same for all blocks:\n");CHKERRQ(ierr);
        ierr = PetscViewerASCIIPushTab(viewer);CHKERRQ(ierr);
        ierr = PetscViewerGetSubViewer(viewer,PETSC_COMM_SELF,&sviewer);CHKERRQ(ierr);
        if (!rank) {
          ierr = PetscViewerASCIIPushTab(viewer);CHKERRQ(ierr);
          ierr = SNESView(nasm->subsnes[0],sviewer);CHKERRQ(ierr);
          ierr = PetscViewerASCIIPopTab(viewer);CHKERRQ(ierr);
        }
        ierr = PetscViewerRestoreSubViewer(viewer,PETSC_COMM_SELF,&sviewer);CHKERRQ(ierr);
        ierr = PetscViewerASCIIPopTab(viewer);CHKERRQ(ierr);
      }
    } else {
      /* print the solver on each block */
      ierr = PetscViewerASCIIPushSynchronized(viewer);CHKERRQ(ierr);
      ierr = PetscViewerASCIISynchronizedPrintf(viewer,"  [%d] number of local blocks = %D\n",(int)rank,nasm->n);CHKERRQ(ierr);
      ierr = PetscViewerFlush(viewer);CHKERRQ(ierr);
      ierr = PetscViewerASCIIPopSynchronized(viewer);CHKERRQ(ierr);
      ierr = PetscViewerASCIIPrintf(viewer,"  Local solve info for each block is in the following SNES objects:\n");CHKERRQ(ierr);
      ierr = PetscViewerASCIIPushTab(viewer);CHKERRQ(ierr);
      ierr = PetscViewerASCIIPrintf(viewer,"- - - - - - - - - - - - - - - - - -\n");CHKERRQ(ierr);
      ierr = PetscViewerGetSubViewer(viewer,PETSC_COMM_SELF,&sviewer);CHKERRQ(ierr);
      for (i=0; i<nasm->n; i++) {
        ierr = VecGetLocalSize(nasm->x[i],&bsz);CHKERRQ(ierr);
        ierr = PetscViewerASCIIPrintf(sviewer,"[%d] local block number %D, size = %D\n",(int)rank,i,bsz);CHKERRQ(ierr);
        ierr = SNESView(nasm->subsnes[i],sviewer);CHKERRQ(ierr);
        ierr = PetscViewerASCIIPrintf(sviewer,"- - - - - - - - - - - - - - - - - -\n");CHKERRQ(ierr);
      }
      ierr = PetscViewerRestoreSubViewer(viewer,PETSC_COMM_SELF,&sviewer);CHKERRQ(ierr);
      ierr = PetscViewerFlush(viewer);CHKERRQ(ierr);
      ierr = PetscViewerASCIIPopTab(viewer);CHKERRQ(ierr);
    }
  } else if (isstring) {
    ierr = PetscViewerStringSPrintf(viewer," blocks=%D,type=%s",N,SNESNASMTypes[nasm->type]);CHKERRQ(ierr);
    ierr = PetscViewerGetSubViewer(viewer,PETSC_COMM_SELF,&sviewer);CHKERRQ(ierr);
    if (nasm->subsnes && !rank) {ierr = SNESView(nasm->subsnes[0],sviewer);CHKERRQ(ierr);}
    ierr = PetscViewerRestoreSubViewer(viewer,PETSC_COMM_SELF,&sviewer);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "SNESNASMSetType"
/*@
   SNESNASMSetType - Set the type of subdomain update used

   Logically Collective on SNES

   Input Parameters:
+  SNES - the SNES context
-  type - the type of update, PC_ASM_BASIC or PC_ASM_RESTRICT

   Level: intermediate

.keywords: SNES, NASM

.seealso: SNESNASM, SNESNASMGetType(), PCASMSetType()
@*/
PetscErrorCode SNESNASMSetType(SNES snes,PCASMType type)
{
  PetscErrorCode ierr;
  PetscErrorCode (*f)(SNES,PCASMType);

  PetscFunctionBegin;
  ierr = PetscObjectQueryFunction((PetscObject)snes,"SNESNASMSetType_C",&f);CHKERRQ(ierr);
  if (f) {ierr = (f)(snes,type);CHKERRQ(ierr);}
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "SNESNASMSetType_NASM"
PetscErrorCode SNESNASMSetType_NASM(SNES snes,PCASMType type)
{
  SNES_NASM      *nasm = (SNES_NASM*)snes->data;

  PetscFunctionBegin;
  if (type != PC_ASM_BASIC && type != PC_ASM_RESTRICT) SETERRQ(PetscObjectComm((PetscObject)snes),PETSC_ERR_ARG_OUTOFRANGE,"SNESNASM only supports basic and restrict types");
  nasm->type = type;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "SNESNASMGetType"
/*@
   SNESNASMGetType - Get the type of subdomain update used

   Logically Collective on SNES

   Input Parameters:
.  SNES - the SNES context

   Output Parameters:
.  type - the type of update

   Level: intermediate

.keywords: SNES, NASM

.seealso: SNESNASM, SNESNASMSetType(), PCASMGetType()
@*/
PetscErrorCode SNESNASMGetType(SNES snes,PCASMType *type)
{
  PetscErrorCode ierr;
  PetscErrorCode (*f)(SNES,PCASMType*);

  PetscFunctionBegin;
  ierr = PetscObjectQueryFunction((PetscObject)snes,"SNESNASMGetType_C",&f);CHKERRQ(ierr);
  if (f) {ierr = (f)(snes,type);CHKERRQ(ierr);}
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "SNESNASMGetType_NASM"
PetscErrorCode SNESNASMGetType_NASM(SNES snes,PCASMType *type)
{
  SNES_NASM      *nasm = (SNES_NASM*)snes->data;

  PetscFunctionBegin;
  *type = nasm->type;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "SNESNASMSetSubdomains"
/*@
   SNESNASMSetSubdomains - Manually Set the context required to restrict and solve subdomain problems.

   Not Collective

   Input Parameters:
+  SNES - the SNES context
.  n - the number of local subdomains
.  subsnes - solvers defined on the local subdomains
.  iscatter - scatters into the nonoverlapping portions of the local subdomains
.  oscatter - scatters into the overlapping portions of the local subdomains
-  gscatter - scatters into the (ghosted) local vector of the local subdomain

   Level: intermediate

.keywords: SNES, NASM

.seealso: SNESNASM, SNESNASMGetSubdomains()
@*/
PetscErrorCode SNESNASMSetSubdomains(SNES snes,PetscInt n,SNES subsnes[],VecScatter iscatter[],VecScatter oscatter[],VecScatter gscatter[])
{
  PetscErrorCode ierr;
  PetscErrorCode (*f)(SNES,PetscInt,SNES*,VecScatter*,VecScatter*,VecScatter*);

  PetscFunctionBegin;
  ierr = PetscObjectQueryFunction((PetscObject)snes,"SNESNASMSetSubdomains_C",&f);CHKERRQ(ierr);
  if (f) {ierr = (f)(snes,n,subsnes,iscatter,oscatter,gscatter);CHKERRQ(ierr);}
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "SNESNASMSetSubdomains_NASM"
PetscErrorCode SNESNASMSetSubdomains_NASM(SNES snes,PetscInt n,SNES subsnes[],VecScatter iscatter[],VecScatter oscatter[],VecScatter gscatter[])
{
  PetscInt       i;
  PetscErrorCode ierr;
  SNES_NASM      *nasm = (SNES_NASM*)snes->data;

  PetscFunctionBegin;
  if (snes->setupcalled) SETERRQ(PetscObjectComm((PetscObject)snes),PETSC_ERR_ARG_WRONGSTATE,"SNESNASMSetSubdomains() should be called before calling SNESSetUp().");

  /* tear down the previously set things */
  ierr = SNESReset(snes);CHKERRQ(ierr);

  nasm->n = n;
  if (oscatter) {
    for (i=0; i<n; i++) {ierr = PetscObjectReference((PetscObject)oscatter[i]);CHKERRQ(ierr);}
  }
  if (iscatter) {
    for (i=0; i<n; i++) {ierr = PetscObjectReference((PetscObject)iscatter[i]);CHKERRQ(ierr);}
  }
  if (gscatter) {
    for (i=0; i<n; i++) {ierr = PetscObjectReference((PetscObject)gscatter[i]);CHKERRQ(ierr);}
  }
  if (oscatter) {
    ierr = PetscMalloc1(n,&nasm->oscatter);CHKERRQ(ierr);
    for (i=0; i<n; i++) {
      nasm->oscatter[i] = oscatter[i];
    }
  }
  if (iscatter) {
    ierr = PetscMalloc1(n,&nasm->iscatter);CHKERRQ(ierr);
    for (i=0; i<n; i++) {
      nasm->iscatter[i] = iscatter[i];
    }
  }
  if (gscatter) {
    ierr = PetscMalloc1(n,&nasm->gscatter);CHKERRQ(ierr);
    for (i=0; i<n; i++) {
      nasm->gscatter[i] = gscatter[i];
    }
  }

  if (subsnes) {
    ierr = PetscMalloc1(n,&nasm->subsnes);CHKERRQ(ierr);
    for (i=0; i<n; i++) {
      nasm->subsnes[i] = subsnes[i];
    }
    nasm->same_local_solves = PETSC_FALSE;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "SNESNASMGetSubdomains"
/*@
   SNESNASMGetSubdomains - Get the local subdomain context.

   Not Collective

   Input Parameters:
.  SNES - the SNES context

   Output Parameters:
+  n - the number of local subdomains
.  subsnes - solvers defined on the local subdomains
.  iscatter - scatters into the nonoverlapping portions of the local subdomains
.  oscatter - scatters into the overlapping portions of the local subdomains
-  gscatter - scatters into the (ghosted) local vector of the local subdomain

   Level: intermediate

.keywords: SNES, NASM

.seealso: SNESNASM, SNESNASMSetSubdomains()
@*/
PetscErrorCode SNESNASMGetSubdomains(SNES snes,PetscInt *n,SNES *subsnes[],VecScatter *iscatter[],VecScatter *oscatter[],VecScatter *gscatter[])
{
  PetscErrorCode ierr;
  PetscErrorCode (*f)(SNES,PetscInt*,SNES**,VecScatter**,VecScatter**,VecScatter**);

  PetscFunctionBegin;
  ierr = PetscObjectQueryFunction((PetscObject)snes,"SNESNASMGetSubdomains_C",&f);CHKERRQ(ierr);
  if (f) {ierr = (f)(snes,n,subsnes,iscatter,oscatter,gscatter);CHKERRQ(ierr);}
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "SNESNASMGetSubdomains_NASM"
PetscErrorCode SNESNASMGetSubdomains_NASM(SNES snes,PetscInt *n,SNES *subsnes[],VecScatter *iscatter[],VecScatter *oscatter[],VecScatter *gscatter[])
{
  SNES_NASM      *nasm = (SNES_NASM*)snes->data;

  PetscFunctionBegin;
  if (n) *n = nasm->n;
  if (oscatter) *oscatter = nasm->oscatter;
  if (iscatter) *iscatter = nasm->iscatter;
  if (gscatter) *gscatter = nasm->gscatter;
  if (subsnes)  {
    *subsnes  = nasm->subsnes;
    nasm->same_local_solves = PETSC_FALSE;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "SNESNASMGetSubdomainVecs"
/*@
   SNESNASMGetSubdomainVecs - Get the processor-local subdomain vectors

   Not Collective

   Input Parameters:
.  SNES - the SNES context

   Output Parameters:
+  n - the number of local subdomains
.  x - The subdomain solution vector
.  y - The subdomain step vector
.  b - The subdomain RHS vector
-  xl - The subdomain local vectors (ghosted)

   Level: developer

.keywords: SNES, NASM

.seealso: SNESNASM, SNESNASMGetSubdomains()
@*/
PetscErrorCode SNESNASMGetSubdomainVecs(SNES snes,PetscInt *n,Vec **x,Vec **y,Vec **b, Vec **xl)
{
  PetscErrorCode ierr;
  PetscErrorCode (*f)(SNES,PetscInt*,Vec**,Vec**,Vec**,Vec**);

  PetscFunctionBegin;
  ierr = PetscObjectQueryFunction((PetscObject)snes,"SNESNASMGetSubdomainVecs_C",&f);CHKERRQ(ierr);
  if (f) {ierr = (f)(snes,n,x,y,b,xl);CHKERRQ(ierr);}
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "SNESNASMGetSubdomainVecs_NASM"
PetscErrorCode SNESNASMGetSubdomainVecs_NASM(SNES snes,PetscInt *n,Vec **x,Vec **y,Vec **b,Vec **xl)
{
  SNES_NASM      *nasm = (SNES_NASM*)snes->data;

  PetscFunctionBegin;
  if (n)  *n  = nasm->n;
  if (x)  *x  = nasm->x;
  if (y)  *y  = nasm->y;
  if (b)  *b  = nasm->b;
  if (xl) *xl = nasm->xl;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "SNESNASMSetComputeFinalJacobian"
/*@
   SNESNASMSetComputeFinalJacobian - Schedules the computation of the global and subdomain jacobians upon convergence

   Collective on SNES

   Input Parameters:
+  SNES - the SNES context
-  flg - indication of whether to compute the jacobians or not

   Level: developer

   Notes: This is used almost exclusively in the implementation of ASPIN, where the converged subdomain and global jacobian
   is needed at each linear iteration.

.keywords: SNES, NASM, ASPIN

.seealso: SNESNASM, SNESNASMGetSubdomains()
@*/
PetscErrorCode SNESNASMSetComputeFinalJacobian(SNES snes,PetscBool flg)
{
  PetscErrorCode (*f)(SNES,PetscBool);
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscObjectQueryFunction((PetscObject)snes,"SNESNASMSetComputeFinalJacobian_C",&f);CHKERRQ(ierr);
  if (f) {ierr = (f)(snes,flg);CHKERRQ(ierr);}
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "SNESNASMSetComputeFinalJacobian_NASM"
PetscErrorCode SNESNASMSetComputeFinalJacobian_NASM(SNES snes,PetscBool flg)
{
  SNES_NASM      *nasm = (SNES_NASM*)snes->data;

  PetscFunctionBegin;
  nasm->finaljacobian = flg;
  if (flg) snes->usesksp = PETSC_TRUE;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "SNESNASMSetDamping"
/*@
   SNESNASMSetDamping - Sets the update damping for NASM

   Logically collective on SNES

   Input Parameters:
+  SNES - the SNES context
-  dmp - damping

   Level: intermediate

.keywords: SNES, NASM, damping

.seealso: SNESNASM, SNESNASMGetDamping()
@*/
PetscErrorCode SNESNASMSetDamping(SNES snes,PetscReal dmp)
{
  PetscErrorCode (*f)(SNES,PetscReal);
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscObjectQueryFunction((PetscObject)snes,"SNESNASMSetDamping_C",(void (**)(void))&f);CHKERRQ(ierr);
  if (f) {ierr = (f)(snes,dmp);CHKERRQ(ierr);}
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "SNESNASMSetDamping_NASM"
PetscErrorCode SNESNASMSetDamping_NASM(SNES snes,PetscReal dmp)
{
  SNES_NASM      *nasm = (SNES_NASM*)snes->data;

  PetscFunctionBegin;
  nasm->damping = dmp;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "SNESNASMGetDamping"
/*@
   SNESNASMGetDamping - Gets the update damping for NASM

   Not Collective

   Input Parameters:
+  SNES - the SNES context
-  dmp - damping

   Level: intermediate

.keywords: SNES, NASM, damping

.seealso: SNESNASM, SNESNASMSetDamping()
@*/
PetscErrorCode SNESNASMGetDamping(SNES snes,PetscReal *dmp)
{
  PetscErrorCode (*f)(SNES,PetscReal*);
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscObjectQueryFunction((PetscObject)snes,"SNESNASMGetDamping_C",(void (**)(void))&f);CHKERRQ(ierr);
  if (f) {ierr = (f)(snes,dmp);CHKERRQ(ierr);}
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "SNESNASMGetDamping_NASM"
PetscErrorCode SNESNASMGetDamping_NASM(SNES snes,PetscReal *dmp)
{
  SNES_NASM      *nasm = (SNES_NASM*)snes->data;

  PetscFunctionBegin;
  *dmp = nasm->damping;
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "SNESNASMSolveLocal_Private"
/*
  Input Parameters:
+ snes - The solver
. B - The RHS vector
- X - The initial guess

  Output Parameters:
. Y - The solution update

  TODO: All scatters should be packed into one
*/
PetscErrorCode SNESNASMSolveLocal_Private(SNES snes,Vec B,Vec Y,Vec X)
{
  SNES_NASM      *nasm = (SNES_NASM*)snes->data;
  SNES           subsnes;
  PetscInt       i;
  PetscReal      dmp;
  PetscErrorCode ierr;
  Vec            Xlloc,Xl,Bl,Yl;
  VecScatter     iscat,oscat,gscat;
  DM             dm,subdm;
  PCASMType      type;

  PetscFunctionBegin;
  ierr = SNESNASMGetType(snes,&type);CHKERRQ(ierr);
  ierr = SNESGetDM(snes,&dm);CHKERRQ(ierr);
  ierr = SNESNASMGetDamping(snes,&dmp);CHKERRQ(ierr);
  ierr = VecSet(Y,0);CHKERRQ(ierr);
  if (nasm->eventrestrictinterp) {ierr = PetscLogEventBegin(nasm->eventrestrictinterp,snes,0,0,0);CHKERRQ(ierr);}
  for (i=0; i<nasm->n; i++) {
    /* scatter the solution to the local solution */
    Xlloc = nasm->xl[i];
    gscat   = nasm->gscatter[i];
    oscat   = nasm->oscatter[i];
    ierr = VecScatterBegin(gscat,X,Xlloc,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
    if (B) {
      /* scatter the RHS to the local RHS */
      Bl   = nasm->b[i];
      ierr = VecScatterBegin(oscat,B,Bl,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
    }
  }
  if (nasm->eventrestrictinterp) {ierr = PetscLogEventEnd(nasm->eventrestrictinterp,snes,0,0,0);CHKERRQ(ierr);}


  if (nasm->eventsubsolve) {ierr = PetscLogEventBegin(nasm->eventsubsolve,snes,0,0,0);CHKERRQ(ierr);}
  for (i=0; i<nasm->n; i++) {
    Xl    = nasm->x[i];
    Xlloc = nasm->xl[i];
    Yl    = nasm->y[i];
    subsnes = nasm->subsnes[i];
    ierr    = SNESGetDM(subsnes,&subdm);CHKERRQ(ierr);
    iscat   = nasm->iscatter[i];
    oscat   = nasm->oscatter[i];
    gscat   = nasm->gscatter[i];
    ierr = VecScatterEnd(gscat,X,Xlloc,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
    if (B) {
      Bl   = nasm->b[i];
      ierr = VecScatterEnd(oscat,B,Bl,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
    } else Bl = NULL;
    ierr = DMSubDomainRestrict(dm,oscat,gscat,subdm);CHKERRQ(ierr);
    /* Could scatter directly from X */
    ierr = DMLocalToGlobalBegin(subdm,Xlloc,INSERT_VALUES,Xl);CHKERRQ(ierr);
    ierr = DMLocalToGlobalEnd(subdm,Xlloc,INSERT_VALUES,Xl);CHKERRQ(ierr);
    ierr = VecCopy(Xl,Yl);CHKERRQ(ierr);
    ierr = SNESSolve(subsnes,Bl,Xl);CHKERRQ(ierr);
    ierr = VecAYPX(Yl,-1.0,Xl);CHKERRQ(ierr);
    if (type == PC_ASM_BASIC) {
      ierr = VecScatterBegin(oscat,Yl,Y,ADD_VALUES,SCATTER_REVERSE);CHKERRQ(ierr);
    } else if (type == PC_ASM_RESTRICT) {
      ierr = VecScatterBegin(iscat,Yl,Y,ADD_VALUES,SCATTER_REVERSE);CHKERRQ(ierr);
    } else SETERRQ(PetscObjectComm((PetscObject)snes),PETSC_ERR_ARG_WRONGSTATE,"Only basic and restrict types are supported for SNESNASM");
  }
  if (nasm->eventsubsolve) {ierr = PetscLogEventEnd(nasm->eventsubsolve,snes,0,0,0);CHKERRQ(ierr);}
  if (nasm->eventrestrictinterp) {ierr = PetscLogEventBegin(nasm->eventrestrictinterp,snes,0,0,0);CHKERRQ(ierr);}
  for (i=0; i<nasm->n; i++) {
    Yl    = nasm->y[i];
    iscat   = nasm->iscatter[i];
    oscat   = nasm->oscatter[i];
    if (type == PC_ASM_BASIC) {
      ierr = VecScatterEnd(oscat,Yl,Y,ADD_VALUES,SCATTER_REVERSE);CHKERRQ(ierr);
    } else if (type == PC_ASM_RESTRICT) {
      ierr = VecScatterEnd(iscat,Yl,Y,ADD_VALUES,SCATTER_REVERSE);CHKERRQ(ierr);
    } else SETERRQ(PetscObjectComm((PetscObject)snes),PETSC_ERR_ARG_WRONGSTATE,"Only basic and restrict types are supported for SNESNASM");
  }
  if (nasm->eventrestrictinterp) {ierr = PetscLogEventEnd(nasm->eventrestrictinterp,snes,0,0,0);CHKERRQ(ierr);}
  ierr = VecAXPY(X,dmp,Y);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "SNESNASMComputeFinalJacobian_Private"
PetscErrorCode SNESNASMComputeFinalJacobian_Private(SNES snes, Vec Xfinal)
{
  Vec            X = Xfinal;
  SNES_NASM      *nasm = (SNES_NASM*)snes->data;
  SNES           subsnes;
  PetscInt       i,lag = 1;
  PetscErrorCode ierr;
  Vec            Xlloc,Xl,Fl,F;
  VecScatter     oscat,gscat;
  DM             dm,subdm;

  PetscFunctionBegin;
  if (nasm->fjtype == 2) X = nasm->xinit;
  F = snes->vec_func;
  if (snes->normschedule == SNES_NORM_NONE) {ierr = SNESComputeFunction(snes,X,F);CHKERRQ(ierr);}
  ierr = SNESComputeJacobian(snes,X,snes->jacobian,snes->jacobian_pre);CHKERRQ(ierr);
  ierr = SNESGetDM(snes,&dm);CHKERRQ(ierr);
  if (nasm->eventrestrictinterp) {ierr = PetscLogEventBegin(nasm->eventrestrictinterp,snes,0,0,0);CHKERRQ(ierr);}
  if (nasm->fjtype != 1) {
    for (i=0; i<nasm->n; i++) {
      Xlloc = nasm->xl[i];
      gscat = nasm->gscatter[i];
      oscat = nasm->oscatter[i];
      ierr = VecScatterBegin(gscat,X,Xlloc,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
    }
  }
  if (nasm->eventrestrictinterp) {ierr = PetscLogEventEnd(nasm->eventrestrictinterp,snes,0,0,0);CHKERRQ(ierr);}
  for (i=0; i<nasm->n; i++) {
    Fl      = nasm->subsnes[i]->vec_func;
    Xl      = nasm->x[i];
    Xlloc   = nasm->xl[i];
    subsnes = nasm->subsnes[i];
    oscat   = nasm->oscatter[i];
    gscat   = nasm->gscatter[i];
    if (nasm->fjtype != 1) {ierr = VecScatterEnd(gscat,X,Xlloc,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);}
    ierr = SNESGetDM(subsnes,&subdm);CHKERRQ(ierr);
    ierr = DMSubDomainRestrict(dm,oscat,gscat,subdm);CHKERRQ(ierr);
    if (nasm->fjtype != 1) {
      ierr = DMLocalToGlobalBegin(subdm,Xlloc,INSERT_VALUES,Xl);CHKERRQ(ierr);
      ierr = DMLocalToGlobalEnd(subdm,Xlloc,INSERT_VALUES,Xl);CHKERRQ(ierr);
    }
    if (subsnes->lagjacobian == -1)    subsnes->lagjacobian = -2;
    else if (subsnes->lagjacobian > 1) lag = subsnes->lagjacobian;
    ierr = SNESComputeFunction(subsnes,Xl,Fl);CHKERRQ(ierr);
    ierr = SNESComputeJacobian(subsnes,Xl,subsnes->jacobian,subsnes->jacobian_pre);CHKERRQ(ierr);
    if (lag > 1) subsnes->lagjacobian = lag;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "SNESSolve_NASM"
PetscErrorCode SNESSolve_NASM(SNES snes)
{
  Vec              F;
  Vec              X;
  Vec              B;
  Vec              Y;
  PetscInt         i;
  PetscReal        fnorm = 0.0;
  PetscErrorCode   ierr;
  SNESNormSchedule normschedule;
  SNES_NASM        *nasm = (SNES_NASM*)snes->data;

  PetscFunctionBegin;

  if (snes->xl || snes->xu || snes->ops->computevariablebounds) SETERRQ1(PetscObjectComm((PetscObject)snes),PETSC_ERR_ARG_WRONGSTATE, "SNES solver %s does not support bounds", ((PetscObject)snes)->type_name);

  ierr = PetscCitationsRegister(SNESCitation,&SNEScite);CHKERRQ(ierr);
  X = snes->vec_sol;
  Y = snes->vec_sol_update;
  F = snes->vec_func;
  B = snes->vec_rhs;

  ierr         = PetscObjectSAWsTakeAccess((PetscObject)snes);CHKERRQ(ierr);
  snes->iter   = 0;
  snes->norm   = 0.;
  ierr         = PetscObjectSAWsGrantAccess((PetscObject)snes);CHKERRQ(ierr);
  snes->reason = SNES_CONVERGED_ITERATING;
  ierr         = SNESGetNormSchedule(snes, &normschedule);CHKERRQ(ierr);
  if (normschedule == SNES_NORM_ALWAYS || normschedule == SNES_NORM_INITIAL_ONLY || normschedule == SNES_NORM_INITIAL_FINAL_ONLY) {
    /* compute the initial function and preconditioned update delX */
    if (!snes->vec_func_init_set) {
      ierr = SNESComputeFunction(snes,X,F);CHKERRQ(ierr);
    } else snes->vec_func_init_set = PETSC_FALSE;

    ierr = VecNorm(F, NORM_2, &fnorm);CHKERRQ(ierr); /* fnorm <- ||F||  */
    SNESCheckFunctionNorm(snes,fnorm);
    ierr       = PetscObjectSAWsTakeAccess((PetscObject)snes);CHKERRQ(ierr);
    snes->iter = 0;
    snes->norm = fnorm;
    ierr       = PetscObjectSAWsGrantAccess((PetscObject)snes);CHKERRQ(ierr);
    ierr       = SNESLogConvergenceHistory(snes,snes->norm,0);CHKERRQ(ierr);
    ierr       = SNESMonitor(snes,0,snes->norm);CHKERRQ(ierr);

    /* test convergence */
    ierr = (*snes->ops->converged)(snes,0,0.0,0.0,fnorm,&snes->reason,snes->cnvP);CHKERRQ(ierr);
    if (snes->reason) PetscFunctionReturn(0);
  } else {
    ierr = PetscObjectSAWsGrantAccess((PetscObject)snes);CHKERRQ(ierr);
    ierr = SNESLogConvergenceHistory(snes,snes->norm,0);CHKERRQ(ierr);
    ierr = SNESMonitor(snes,0,snes->norm);CHKERRQ(ierr);
  }

  /* Call general purpose update function */
  if (snes->ops->update) {
    ierr = (*snes->ops->update)(snes, snes->iter);CHKERRQ(ierr);
  }
  /* copy the initial solution over for later */
  if (nasm->fjtype == 2) {ierr = VecCopy(X,nasm->xinit);CHKERRQ(ierr);}

  for (i = 0; i < snes->max_its; i++) {
    ierr = SNESNASMSolveLocal_Private(snes,B,Y,X);CHKERRQ(ierr);
    if (normschedule == SNES_NORM_ALWAYS || ((i == snes->max_its - 1) && (normschedule == SNES_NORM_INITIAL_FINAL_ONLY || normschedule == SNES_NORM_FINAL_ONLY))) {
      ierr = SNESComputeFunction(snes,X,F);CHKERRQ(ierr);
      ierr = VecNorm(F, NORM_2, &fnorm);CHKERRQ(ierr); /* fnorm <- ||F||  */
      SNESCheckFunctionNorm(snes,fnorm);
    }
    /* Monitor convergence */
    ierr       = PetscObjectSAWsTakeAccess((PetscObject)snes);CHKERRQ(ierr);
    snes->iter = i+1;
    snes->norm = fnorm;
    ierr       = PetscObjectSAWsGrantAccess((PetscObject)snes);CHKERRQ(ierr);
    ierr       = SNESLogConvergenceHistory(snes,snes->norm,0);CHKERRQ(ierr);
    ierr       = SNESMonitor(snes,snes->iter,snes->norm);CHKERRQ(ierr);
    /* Test for convergence */
    if (normschedule == SNES_NORM_ALWAYS) {ierr = (*snes->ops->converged)(snes,snes->iter,0.0,0.0,fnorm,&snes->reason,snes->cnvP);CHKERRQ(ierr);}
    if (snes->reason) break;
    /* Call general purpose update function */
    if (snes->ops->update) {ierr = (*snes->ops->update)(snes, snes->iter);CHKERRQ(ierr);}
  }
  if (nasm->finaljacobian) {ierr = SNESNASMComputeFinalJacobian_Private(snes,X);CHKERRQ(ierr);}
  if (normschedule == SNES_NORM_ALWAYS) {
    if (i == snes->max_its) {
      ierr = PetscInfo1(snes,"Maximum number of iterations has been reached: %D\n",snes->max_its);CHKERRQ(ierr);
      if (!snes->reason) snes->reason = SNES_DIVERGED_MAX_IT;
    }
  } else if (!snes->reason) snes->reason = SNES_CONVERGED_ITS; /* NASM is meant to be used as a preconditioner */
  PetscFunctionReturn(0);
}

/*MC
  SNESNASM - Nonlinear Additive Schwartz

   Options Database:
+  -snes_nasm_log - enable logging events for the communication and solve stages
.  -snes_nasm_type <basic,restrict> - type of subdomain update used
.  -snes_nasm_finaljacobian - compute the local and global jacobians of the final iterate
.  -snes_nasm_finaljacobian_type <finalinner,finalouter,initial> - pick state the jacobian is calculated at
.  -sub_snes_ - options prefix of the subdomain nonlinear solves
.  -sub_ksp_ - options prefix of the subdomain Krylov solver
-  -sub_pc_ - options prefix of the subdomain preconditioner

   Level: advanced

.seealso: SNESCreate(), SNES, SNESSetType(), SNESType (for list of available types)
M*/

#undef __FUNCT__
#define __FUNCT__ "SNESCreate_NASM"
PETSC_EXTERN PetscErrorCode SNESCreate_NASM(SNES snes)
{
  SNES_NASM      *nasm;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr       = PetscNewLog(snes,&nasm);CHKERRQ(ierr);
  snes->data = (void*)nasm;

  nasm->n        = PETSC_DECIDE;
  nasm->subsnes  = 0;
  nasm->x        = 0;
  nasm->xl       = 0;
  nasm->y        = 0;
  nasm->b        = 0;
  nasm->oscatter = 0;
  nasm->iscatter = 0;
  nasm->gscatter = 0;
  nasm->damping  = 1.;

  nasm->type = PC_ASM_BASIC;
  nasm->finaljacobian = PETSC_FALSE;
  nasm->same_local_solves = PETSC_TRUE;

  snes->ops->destroy        = SNESDestroy_NASM;
  snes->ops->setup          = SNESSetUp_NASM;
  snes->ops->setfromoptions = SNESSetFromOptions_NASM;
  snes->ops->view           = SNESView_NASM;
  snes->ops->solve          = SNESSolve_NASM;
  snes->ops->reset          = SNESReset_NASM;

  snes->usesksp = PETSC_FALSE;
  snes->usespc  = PETSC_FALSE;

  nasm->fjtype              = 0;
  nasm->xinit               = NULL;
  nasm->eventrestrictinterp = 0;
  nasm->eventsubsolve       = 0;

  if (!snes->tolerancesset) {
    snes->max_its   = 10000;
    snes->max_funcs = 10000;
  }

  ierr = PetscObjectComposeFunction((PetscObject)snes,"SNESNASMSetType_C",SNESNASMSetType_NASM);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)snes,"SNESNASMGetType_C",SNESNASMGetType_NASM);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)snes,"SNESNASMSetSubdomains_C",SNESNASMSetSubdomains_NASM);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)snes,"SNESNASMGetSubdomains_C",SNESNASMGetSubdomains_NASM);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)snes,"SNESNASMSetDamping_C",SNESNASMSetDamping_NASM);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)snes,"SNESNASMGetDamping_C",SNESNASMGetDamping_NASM);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)snes,"SNESNASMGetSubdomainVecs_C",SNESNASMGetSubdomainVecs_NASM);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)snes,"SNESNASMSetComputeFinalJacobian_C",SNESNASMSetComputeFinalJacobian_NASM);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

