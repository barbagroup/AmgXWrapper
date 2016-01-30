#include <petsc/private/petscdsimpl.h> /*I "petscds.h" I*/

PetscClassId PETSCDS_CLASSID = 0;

PetscFunctionList PetscDSList              = NULL;
PetscBool         PetscDSRegisterAllCalled = PETSC_FALSE;

#undef __FUNCT__
#define __FUNCT__ "PetscDSRegister"
/*@C
  PetscDSRegister - Adds a new PetscDS implementation

  Not Collective

  Input Parameters:
+ name        - The name of a new user-defined creation routine
- create_func - The creation routine itself

  Notes:
  PetscDSRegister() may be called multiple times to add several user-defined PetscDSs

  Sample usage:
.vb
    PetscDSRegister("my_ds", MyPetscDSCreate);
.ve

  Then, your PetscDS type can be chosen with the procedural interface via
.vb
    PetscDSCreate(MPI_Comm, PetscDS *);
    PetscDSSetType(PetscDS, "my_ds");
.ve
   or at runtime via the option
.vb
    -petscds_type my_ds
.ve

  Level: advanced

.keywords: PetscDS, register
.seealso: PetscDSRegisterAll(), PetscDSRegisterDestroy()

@*/
PetscErrorCode PetscDSRegister(const char sname[], PetscErrorCode (*function)(PetscDS))
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscFunctionListAdd(&PetscDSList, sname, function);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSSetType"
/*@C
  PetscDSSetType - Builds a particular PetscDS

  Collective on PetscDS

  Input Parameters:
+ prob - The PetscDS object
- name - The kind of system

  Options Database Key:
. -petscds_type <type> - Sets the PetscDS type; use -help for a list of available types

  Level: intermediate

.keywords: PetscDS, set, type
.seealso: PetscDSGetType(), PetscDSCreate()
@*/
PetscErrorCode PetscDSSetType(PetscDS prob, PetscDSType name)
{
  PetscErrorCode (*r)(PetscDS);
  PetscBool      match;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  ierr = PetscObjectTypeCompare((PetscObject) prob, name, &match);CHKERRQ(ierr);
  if (match) PetscFunctionReturn(0);

  ierr = PetscDSRegisterAll();CHKERRQ(ierr);
  ierr = PetscFunctionListFind(PetscDSList, name, &r);CHKERRQ(ierr);
  if (!r) SETERRQ1(PetscObjectComm((PetscObject) prob), PETSC_ERR_ARG_UNKNOWN_TYPE, "Unknown PetscDS type: %s", name);

  if (prob->ops->destroy) {
    ierr             = (*prob->ops->destroy)(prob);CHKERRQ(ierr);
    prob->ops->destroy = NULL;
  }
  ierr = (*r)(prob);CHKERRQ(ierr);
  ierr = PetscObjectChangeTypeName((PetscObject) prob, name);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSGetType"
/*@C
  PetscDSGetType - Gets the PetscDS type name (as a string) from the object.

  Not Collective

  Input Parameter:
. prob  - The PetscDS

  Output Parameter:
. name - The PetscDS type name

  Level: intermediate

.keywords: PetscDS, get, type, name
.seealso: PetscDSSetType(), PetscDSCreate()
@*/
PetscErrorCode PetscDSGetType(PetscDS prob, PetscDSType *name)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  PetscValidPointer(name, 2);
  ierr = PetscDSRegisterAll();CHKERRQ(ierr);
  *name = ((PetscObject) prob)->type_name;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSView_Ascii"
static PetscErrorCode PetscDSView_Ascii(PetscDS prob, PetscViewer viewer)
{
  PetscViewerFormat format;
  PetscInt          f;
  PetscErrorCode    ierr;

  PetscFunctionBegin;
  ierr = PetscViewerGetFormat(viewer, &format);CHKERRQ(ierr);
  ierr = PetscViewerASCIIPrintf(viewer, "Discrete System with %d fields\n", prob->Nf);CHKERRQ(ierr);
  ierr = PetscViewerASCIIPushTab(viewer);CHKERRQ(ierr);
  for (f = 0; f < prob->Nf; ++f) {
    PetscObject  obj;
    PetscClassId id;
    const char  *name;
    PetscInt     Nc;

    ierr = PetscDSGetDiscretization(prob, f, &obj);CHKERRQ(ierr);
    ierr = PetscObjectGetClassId(obj, &id);CHKERRQ(ierr);
    ierr = PetscObjectGetName(obj, &name);CHKERRQ(ierr);
    ierr = PetscViewerASCIIPrintf(viewer, "Field %s", name ? name : "<unknown>");CHKERRQ(ierr);
    if (id == PETSCFE_CLASSID)      {
      ierr = PetscFEGetNumComponents((PetscFE) obj, &Nc);CHKERRQ(ierr);
      ierr = PetscViewerASCIIPrintf(viewer, " FEM");CHKERRQ(ierr);
    } else if (id == PETSCFV_CLASSID) {
      ierr = PetscFVGetNumComponents((PetscFV) obj, &Nc);CHKERRQ(ierr);
      ierr = PetscViewerASCIIPrintf(viewer, " FVM");CHKERRQ(ierr);
    }
    else SETERRQ1(PetscObjectComm((PetscObject) prob), PETSC_ERR_ARG_WRONG, "Unknown discretization type for field %d", f);
    if (Nc > 1) {ierr = PetscViewerASCIIPrintf(viewer, "%d components", Nc);CHKERRQ(ierr);}
    else        {ierr = PetscViewerASCIIPrintf(viewer, "%d component ", Nc);CHKERRQ(ierr);}
    if (prob->implicit[f]) {ierr = PetscViewerASCIIPrintf(viewer, " (implicit)");CHKERRQ(ierr);}
    else                   {ierr = PetscViewerASCIIPrintf(viewer, " (explicit)");CHKERRQ(ierr);}
    if (prob->adjacency[f*2+0]) {
      if (prob->adjacency[f*2+1]) {ierr = PetscViewerASCIIPrintf(viewer, " (adj FVM++)");CHKERRQ(ierr);}
      else                        {ierr = PetscViewerASCIIPrintf(viewer, " (adj FVM)");CHKERRQ(ierr);}
    } else {
      if (prob->adjacency[f*2+1]) {ierr = PetscViewerASCIIPrintf(viewer, " (adj FEM)");CHKERRQ(ierr);}
      else                        {ierr = PetscViewerASCIIPrintf(viewer, " (adj FUNKY)");CHKERRQ(ierr);}
    }
    ierr = PetscViewerASCIIPrintf(viewer, "\n");CHKERRQ(ierr);
    if (format == PETSC_VIEWER_ASCII_INFO_DETAIL) {
      if (id == PETSCFE_CLASSID)      {ierr = PetscFEView((PetscFE) obj, viewer);CHKERRQ(ierr);}
      else if (id == PETSCFV_CLASSID) {ierr = PetscFVView((PetscFV) obj, viewer);CHKERRQ(ierr);}
    }
  }
  ierr = PetscViewerASCIIPopTab(viewer);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSView"
/*@C
  PetscDSView - Views a PetscDS

  Collective on PetscDS

  Input Parameter:
+ prob - the PetscDS object to view
- v  - the viewer

  Level: developer

.seealso PetscDSDestroy()
@*/
PetscErrorCode PetscDSView(PetscDS prob, PetscViewer v)
{
  PetscBool      iascii;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  if (!v) {ierr = PetscViewerASCIIGetStdout(PetscObjectComm((PetscObject) prob), &v);CHKERRQ(ierr);}
  else    {PetscValidHeaderSpecific(v, PETSC_VIEWER_CLASSID, 2);}
  ierr = PetscObjectTypeCompare((PetscObject) v, PETSCVIEWERASCII, &iascii);CHKERRQ(ierr);
  if (iascii) {ierr = PetscDSView_Ascii(prob, v);CHKERRQ(ierr);}
  if (prob->ops->view) {ierr = (*prob->ops->view)(prob, v);CHKERRQ(ierr);}
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSSetFromOptions"
/*@
  PetscDSSetFromOptions - sets parameters in a PetscDS from the options database

  Collective on PetscDS

  Input Parameter:
. prob - the PetscDS object to set options for

  Options Database:

  Level: developer

.seealso PetscDSView()
@*/
PetscErrorCode PetscDSSetFromOptions(PetscDS prob)
{
  const char    *defaultType;
  char           name[256];
  PetscBool      flg;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  if (!((PetscObject) prob)->type_name) {
    defaultType = PETSCDSBASIC;
  } else {
    defaultType = ((PetscObject) prob)->type_name;
  }
  ierr = PetscDSRegisterAll();CHKERRQ(ierr);

  ierr = PetscObjectOptionsBegin((PetscObject) prob);CHKERRQ(ierr);
  ierr = PetscOptionsFList("-petscds_type", "Discrete System", "PetscDSSetType", PetscDSList, defaultType, name, 256, &flg);CHKERRQ(ierr);
  if (flg) {
    ierr = PetscDSSetType(prob, name);CHKERRQ(ierr);
  } else if (!((PetscObject) prob)->type_name) {
    ierr = PetscDSSetType(prob, defaultType);CHKERRQ(ierr);
  }
  if (prob->ops->setfromoptions) {ierr = (*prob->ops->setfromoptions)(prob);CHKERRQ(ierr);}
  /* process any options handlers added with PetscObjectAddOptionsHandler() */
  ierr = PetscObjectProcessOptionsHandlers(PetscOptionsObject,(PetscObject) prob);CHKERRQ(ierr);
  ierr = PetscOptionsEnd();CHKERRQ(ierr);
  ierr = PetscDSViewFromOptions(prob, NULL, "-petscds_view");CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSSetUp"
/*@C
  PetscDSSetUp - Construct data structures for the PetscDS

  Collective on PetscDS

  Input Parameter:
. prob - the PetscDS object to setup

  Level: developer

.seealso PetscDSView(), PetscDSDestroy()
@*/
PetscErrorCode PetscDSSetUp(PetscDS prob)
{
  const PetscInt Nf = prob->Nf;
  PetscInt       dim, work, NcMax = 0, NqMax = 0, f;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  if (prob->setup) PetscFunctionReturn(0);
  /* Calculate sizes */
  ierr = PetscDSGetSpatialDimension(prob, &dim);CHKERRQ(ierr);
  prob->totDim = prob->totDimBd = prob->totComp = 0;
  ierr = PetscCalloc4(Nf+1,&prob->off,Nf+1,&prob->offDer,Nf+1,&prob->offBd,Nf+1,&prob->offDerBd);CHKERRQ(ierr);
  ierr = PetscMalloc4(Nf,&prob->basis,Nf,&prob->basisDer,Nf,&prob->basisBd,Nf,&prob->basisDerBd);CHKERRQ(ierr);
  for (f = 0; f < Nf; ++f) {
    PetscFE         feBd = (PetscFE) prob->discBd[f];
    PetscObject     obj;
    PetscClassId    id;
    PetscQuadrature q;
    PetscInt        Nq = 0, Nb, Nc;

    ierr = PetscDSGetDiscretization(prob, f, &obj);CHKERRQ(ierr);
    ierr = PetscObjectGetClassId(obj, &id);CHKERRQ(ierr);
    if (id == PETSCFE_CLASSID)      {
      PetscFE fe = (PetscFE) obj;

      ierr = PetscFEGetQuadrature(fe, &q);CHKERRQ(ierr);
      ierr = PetscFEGetDimension(fe, &Nb);CHKERRQ(ierr);
      ierr = PetscFEGetNumComponents(fe, &Nc);CHKERRQ(ierr);
      ierr = PetscFEGetDefaultTabulation(fe, &prob->basis[f], &prob->basisDer[f], NULL);CHKERRQ(ierr);
    } else if (id == PETSCFV_CLASSID) {
      PetscFV fv = (PetscFV) obj;

      ierr = PetscFVGetQuadrature(fv, &q);CHKERRQ(ierr);
      Nb   = 1;
      ierr = PetscFVGetNumComponents(fv, &Nc);CHKERRQ(ierr);
      ierr = PetscFVGetDefaultTabulation(fv, &prob->basis[f], &prob->basisDer[f], NULL);CHKERRQ(ierr);
    } else SETERRQ1(PetscObjectComm((PetscObject) prob), PETSC_ERR_ARG_WRONG, "Unknown discretization type for field %d", f);
    prob->off[f+1]    = Nc     + prob->off[f];
    prob->offDer[f+1] = Nc*dim + prob->offDer[f];
    if (q) {ierr = PetscQuadratureGetData(q, NULL, &Nq, NULL, NULL);CHKERRQ(ierr);}
    NqMax          = PetscMax(NqMax, Nq);
    NcMax          = PetscMax(NcMax, Nc);
    prob->totDim  += Nb*Nc;
    prob->totComp += Nc;
    if (feBd) {
      ierr = PetscFEGetDimension(feBd, &Nb);CHKERRQ(ierr);
      ierr = PetscFEGetNumComponents(feBd, &Nc);CHKERRQ(ierr);
      ierr = PetscFEGetDefaultTabulation(feBd, &prob->basisBd[f], &prob->basisDerBd[f], NULL);CHKERRQ(ierr);
      prob->totDimBd += Nb*Nc;
      prob->offBd[f+1]    = Nc     + prob->offBd[f];
      prob->offDerBd[f+1] = Nc*dim + prob->offDerBd[f];
    }
  }
  work = PetscMax(prob->totComp*dim, PetscSqr(NcMax*dim));
  /* Allocate works space */
  ierr = PetscMalloc5(prob->totComp,&prob->u,prob->totComp,&prob->u_t,prob->totComp*dim,&prob->u_x,dim,&prob->x,work,&prob->refSpaceDer);CHKERRQ(ierr);
  ierr = PetscMalloc6(NqMax*NcMax,&prob->f0,NqMax*NcMax*dim,&prob->f1,NqMax*NcMax*NcMax,&prob->g0,NqMax*NcMax*NcMax*dim,&prob->g1,NqMax*NcMax*NcMax*dim,&prob->g2,NqMax*NcMax*NcMax*dim*dim,&prob->g3);CHKERRQ(ierr);
  if (prob->ops->setup) {ierr = (*prob->ops->setup)(prob);CHKERRQ(ierr);}
  prob->setup = PETSC_TRUE;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSDestroyStructs_Static"
static PetscErrorCode PetscDSDestroyStructs_Static(PetscDS prob)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscFree4(prob->off,prob->offDer,prob->offBd,prob->offDerBd);CHKERRQ(ierr);
  ierr = PetscFree4(prob->basis,prob->basisDer,prob->basisBd,prob->basisDerBd);CHKERRQ(ierr);
  ierr = PetscFree5(prob->u,prob->u_t,prob->u_x,prob->x,prob->refSpaceDer);CHKERRQ(ierr);
  ierr = PetscFree6(prob->f0,prob->f1,prob->g0,prob->g1,prob->g2,prob->g3);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSEnlarge_Static"
static PetscErrorCode PetscDSEnlarge_Static(PetscDS prob, PetscInt NfNew)
{
  PetscObject      *tmpd, *tmpdbd;
  PetscBool        *tmpi, *tmpa;
  PetscPointFunc   *tmpobj, *tmpf;
  PetscPointJac    *tmpg, *tmpgp;
  PetscBdPointFunc *tmpfbd;
  PetscBdPointJac  *tmpgbd;
  PetscRiemannFunc *tmpr;
  void            **tmpctx;
  PetscInt          Nf = prob->Nf, f, i;
  PetscErrorCode    ierr;

  PetscFunctionBegin;
  if (Nf >= NfNew) PetscFunctionReturn(0);
  prob->setup = PETSC_FALSE;
  ierr = PetscDSDestroyStructs_Static(prob);CHKERRQ(ierr);
  ierr = PetscMalloc4(NfNew, &tmpd, NfNew, &tmpdbd, NfNew, &tmpi, NfNew*2, &tmpa);CHKERRQ(ierr);
  for (f = 0; f < Nf; ++f) {tmpd[f] = prob->disc[f]; tmpdbd[f] = prob->discBd[f]; tmpi[f] = prob->implicit[f]; for (i = 0; i < 2; ++i) tmpa[f*2+i] = prob->adjacency[f*2+i];}
  for (f = Nf; f < NfNew; ++f) {ierr = PetscContainerCreate(PetscObjectComm((PetscObject) prob), (PetscContainer *) &tmpd[f]);CHKERRQ(ierr);
    tmpdbd[f] = NULL; tmpi[f] = PETSC_TRUE; tmpa[f*2+0] = PETSC_FALSE; tmpa[f*2+1] = PETSC_TRUE;}
  ierr = PetscFree4(prob->disc, prob->discBd, prob->implicit, prob->adjacency);CHKERRQ(ierr);
  prob->Nf        = NfNew;
  prob->disc      = tmpd;
  prob->discBd    = tmpdbd;
  prob->implicit  = tmpi;
  prob->adjacency = tmpa;
  ierr = PetscCalloc6(NfNew, &tmpobj, NfNew*2, &tmpf, NfNew*NfNew*4, &tmpg, NfNew*NfNew*4, &tmpgp, NfNew, &tmpr, NfNew, &tmpctx);CHKERRQ(ierr);
  for (f = 0; f < Nf; ++f) tmpobj[f] = prob->obj[f];
  for (f = 0; f < Nf*2; ++f) tmpf[f] = prob->f[f];
  for (f = 0; f < Nf*Nf*4; ++f) tmpg[f] = prob->g[f];
  for (f = 0; f < Nf*Nf*4; ++f) tmpgp[f] = prob->gp[f];
  for (f = 0; f < Nf; ++f) tmpr[f] = prob->r[f];
  for (f = 0; f < Nf; ++f) tmpctx[f] = prob->ctx[f];
  for (f = Nf; f < NfNew; ++f) tmpobj[f] = NULL;
  for (f = Nf*2; f < NfNew*2; ++f) tmpf[f] = NULL;
  for (f = Nf*Nf*4; f < NfNew*NfNew*4; ++f) tmpg[f] = NULL;
  for (f = Nf*Nf*4; f < NfNew*NfNew*4; ++f) tmpgp[f] = NULL;
  for (f = Nf; f < NfNew; ++f) tmpr[f] = NULL;
  for (f = Nf; f < NfNew; ++f) tmpctx[f] = NULL;
  ierr = PetscFree6(prob->obj, prob->f, prob->g, prob->gp, prob->r, prob->ctx);CHKERRQ(ierr);
  prob->obj = tmpobj;
  prob->f   = tmpf;
  prob->g   = tmpg;
  prob->gp  = tmpgp;
  prob->r   = tmpr;
  prob->ctx = tmpctx;
  ierr = PetscCalloc2(NfNew*2, &tmpfbd, NfNew*NfNew*4, &tmpgbd);CHKERRQ(ierr);
  for (f = 0; f < Nf*2; ++f) tmpfbd[f] = prob->fBd[f];
  for (f = 0; f < Nf*Nf*4; ++f) tmpgbd[f] = prob->gBd[f];
  for (f = Nf*2; f < NfNew*2; ++f) tmpfbd[f] = NULL;
  for (f = Nf*Nf*4; f < NfNew*NfNew*4; ++f) tmpgbd[f] = NULL;
  ierr = PetscFree2(prob->fBd, prob->gBd);CHKERRQ(ierr);
  prob->fBd = tmpfbd;
  prob->gBd = tmpgbd;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSDestroy"
/*@
  PetscDSDestroy - Destroys a PetscDS object

  Collective on PetscDS

  Input Parameter:
. prob - the PetscDS object to destroy

  Level: developer

.seealso PetscDSView()
@*/
PetscErrorCode PetscDSDestroy(PetscDS *prob)
{
  PetscInt       f;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (!*prob) PetscFunctionReturn(0);
  PetscValidHeaderSpecific((*prob), PETSCDS_CLASSID, 1);

  if (--((PetscObject)(*prob))->refct > 0) {*prob = 0; PetscFunctionReturn(0);}
  ((PetscObject) (*prob))->refct = 0;
  ierr = PetscDSDestroyStructs_Static(*prob);CHKERRQ(ierr);
  for (f = 0; f < (*prob)->Nf; ++f) {
    ierr = PetscObjectDereference((*prob)->disc[f]);CHKERRQ(ierr);
    ierr = PetscObjectDereference((*prob)->discBd[f]);CHKERRQ(ierr);
  }
  ierr = PetscFree4((*prob)->disc, (*prob)->discBd, (*prob)->implicit, (*prob)->adjacency);CHKERRQ(ierr);
  ierr = PetscFree6((*prob)->obj,(*prob)->f,(*prob)->g,(*prob)->gp,(*prob)->r,(*prob)->ctx);CHKERRQ(ierr);
  ierr = PetscFree2((*prob)->fBd,(*prob)->gBd);CHKERRQ(ierr);
  if ((*prob)->ops->destroy) {ierr = (*(*prob)->ops->destroy)(*prob);CHKERRQ(ierr);}
  ierr = PetscHeaderDestroy(prob);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSCreate"
/*@
  PetscDSCreate - Creates an empty PetscDS object. The type can then be set with PetscDSSetType().

  Collective on MPI_Comm

  Input Parameter:
. comm - The communicator for the PetscDS object

  Output Parameter:
. prob - The PetscDS object

  Level: beginner

.seealso: PetscDSSetType(), PETSCDSBASIC
@*/
PetscErrorCode PetscDSCreate(MPI_Comm comm, PetscDS *prob)
{
  PetscDS   p;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidPointer(prob, 2);
  *prob  = NULL;
  ierr = PetscDSInitializePackage();CHKERRQ(ierr);

  ierr = PetscHeaderCreate(p, PETSCDS_CLASSID, "PetscDS", "Discrete System", "PetscDS", comm, PetscDSDestroy, PetscDSView);CHKERRQ(ierr);

  p->Nf    = 0;
  p->setup = PETSC_FALSE;

  *prob = p;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSGetNumFields"
/*@
  PetscDSGetNumFields - Returns the number of fields in the DS

  Not collective

  Input Parameter:
. prob - The PetscDS object

  Output Parameter:
. Nf - The number of fields

  Level: beginner

.seealso: PetscDSGetSpatialDimension(), PetscDSCreate()
@*/
PetscErrorCode PetscDSGetNumFields(PetscDS prob, PetscInt *Nf)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  PetscValidPointer(Nf, 2);
  *Nf = prob->Nf;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSGetSpatialDimension"
/*@
  PetscDSGetSpatialDimension - Returns the spatial dimension of the DS

  Not collective

  Input Parameter:
. prob - The PetscDS object

  Output Parameter:
. dim - The spatial dimension

  Level: beginner

.seealso: PetscDSGetNumFields(), PetscDSCreate()
@*/
PetscErrorCode PetscDSGetSpatialDimension(PetscDS prob, PetscInt *dim)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  PetscValidPointer(dim, 2);
  *dim = 0;
  if (prob->Nf) {
    PetscObject  obj;
    PetscClassId id;

    ierr = PetscDSGetDiscretization(prob, 0, &obj);CHKERRQ(ierr);
    ierr = PetscObjectGetClassId(obj, &id);CHKERRQ(ierr);
    if (id == PETSCFE_CLASSID)      {ierr = PetscFEGetSpatialDimension((PetscFE) obj, dim);CHKERRQ(ierr);}
    else if (id == PETSCFV_CLASSID) {ierr = PetscFVGetSpatialDimension((PetscFV) obj, dim);CHKERRQ(ierr);}
    else SETERRQ1(PetscObjectComm((PetscObject) prob), PETSC_ERR_ARG_WRONG, "Unknown discretization type for field %d", 0);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSGetTotalDimension"
/*@
  PetscDSGetTotalDimension - Returns the total size of the approximation space for this system

  Not collective

  Input Parameter:
. prob - The PetscDS object

  Output Parameter:
. dim - The total problem dimension

  Level: beginner

.seealso: PetscDSGetNumFields(), PetscDSCreate()
@*/
PetscErrorCode PetscDSGetTotalDimension(PetscDS prob, PetscInt *dim)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  ierr = PetscDSSetUp(prob);CHKERRQ(ierr);
  PetscValidPointer(dim, 2);
  *dim = prob->totDim;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSGetTotalBdDimension"
/*@
  PetscDSGetTotalBdDimension - Returns the total size of the boundary approximation space for this system

  Not collective

  Input Parameter:
. prob - The PetscDS object

  Output Parameter:
. dim - The total boundary problem dimension

  Level: beginner

.seealso: PetscDSGetNumFields(), PetscDSCreate()
@*/
PetscErrorCode PetscDSGetTotalBdDimension(PetscDS prob, PetscInt *dim)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  ierr = PetscDSSetUp(prob);CHKERRQ(ierr);
  PetscValidPointer(dim, 2);
  *dim = prob->totDimBd;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSGetTotalComponents"
/*@
  PetscDSGetTotalComponents - Returns the total number of components in this system

  Not collective

  Input Parameter:
. prob - The PetscDS object

  Output Parameter:
. dim - The total number of components

  Level: beginner

.seealso: PetscDSGetNumFields(), PetscDSCreate()
@*/
PetscErrorCode PetscDSGetTotalComponents(PetscDS prob, PetscInt *Nc)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  ierr = PetscDSSetUp(prob);CHKERRQ(ierr);
  PetscValidPointer(Nc, 2);
  *Nc = prob->totComp;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSGetDiscretization"
/*@
  PetscDSGetDiscretization - Returns the discretization object for the given field

  Not collective

  Input Parameters:
+ prob - The PetscDS object
- f - The field number

  Output Parameter:
. disc - The discretization object

  Level: beginner

.seealso: PetscDSSetDiscretization(), PetscDSAddDiscretization(), PetscDSGetBdDiscretization(), PetscDSGetNumFields(), PetscDSCreate()
@*/
PetscErrorCode PetscDSGetDiscretization(PetscDS prob, PetscInt f, PetscObject *disc)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  PetscValidPointer(disc, 3);
  if ((f < 0) || (f >= prob->Nf)) SETERRQ2(PETSC_COMM_SELF, PETSC_ERR_ARG_OUTOFRANGE, "Field number %d must be in [0, %d)", f, prob->Nf);
  *disc = prob->disc[f];
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSGetBdDiscretization"
/*@
  PetscDSGetBdDiscretization - Returns the boundary discretization object for the given field

  Not collective

  Input Parameters:
+ prob - The PetscDS object
- f - The field number

  Output Parameter:
. disc - The boundary discretization object

  Level: beginner

.seealso: PetscDSSetBdDiscretization(), PetscDSAddBdDiscretization(), PetscDSGetDiscretization(), PetscDSGetNumFields(), PetscDSCreate()
@*/
PetscErrorCode PetscDSGetBdDiscretization(PetscDS prob, PetscInt f, PetscObject *disc)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  PetscValidPointer(disc, 3);
  if ((f < 0) || (f >= prob->Nf)) SETERRQ2(PETSC_COMM_SELF, PETSC_ERR_ARG_OUTOFRANGE, "Field number %d must be in [0, %d)", f, prob->Nf);
  *disc = prob->discBd[f];
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSSetDiscretization"
/*@
  PetscDSSetDiscretization - Sets the discretization object for the given field

  Not collective

  Input Parameters:
+ prob - The PetscDS object
. f - The field number
- disc - The discretization object

  Level: beginner

.seealso: PetscDSGetDiscretization(), PetscDSAddDiscretization(), PetscDSGetNumFields(), PetscDSCreate()
@*/
PetscErrorCode PetscDSSetDiscretization(PetscDS prob, PetscInt f, PetscObject disc)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  PetscValidPointer(disc, 3);
  if (f < 0) SETERRQ1(PETSC_COMM_SELF, PETSC_ERR_ARG_OUTOFRANGE, "Field number %d must be non-negative", f);
  ierr = PetscDSEnlarge_Static(prob, f+1);CHKERRQ(ierr);
  if (prob->disc[f]) {ierr = PetscObjectDereference(prob->disc[f]);CHKERRQ(ierr);}
  prob->disc[f] = disc;
  ierr = PetscObjectReference(disc);CHKERRQ(ierr);
  {
    PetscClassId id;

    ierr = PetscObjectGetClassId(disc, &id);CHKERRQ(ierr);
    if (id == PETSCFV_CLASSID) {
      prob->implicit[f]      = PETSC_FALSE;
      prob->adjacency[f*2+0] = PETSC_TRUE;
      prob->adjacency[f*2+1] = PETSC_FALSE;
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSSetBdDiscretization"
/*@
  PetscDSSetBdDiscretization - Sets the boundary discretization object for the given field

  Not collective

  Input Parameters:
+ prob - The PetscDS object
. f - The field number
- disc - The boundary discretization object

  Level: beginner

.seealso: PetscDSGetBdDiscretization(), PetscDSAddBdDiscretization(), PetscDSGetNumFields(), PetscDSCreate()
@*/
PetscErrorCode PetscDSSetBdDiscretization(PetscDS prob, PetscInt f, PetscObject disc)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  if (disc) PetscValidPointer(disc, 3);
  if (f < 0) SETERRQ1(PETSC_COMM_SELF, PETSC_ERR_ARG_OUTOFRANGE, "Field number %d must be non-negative", f);
  ierr = PetscDSEnlarge_Static(prob, f+1);CHKERRQ(ierr);
  if (prob->discBd[f]) {ierr = PetscObjectDereference(prob->discBd[f]);CHKERRQ(ierr);}
  prob->discBd[f] = disc;
  ierr = PetscObjectReference(disc);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSAddDiscretization"
/*@
  PetscDSAddDiscretization - Adds a discretization object

  Not collective

  Input Parameters:
+ prob - The PetscDS object
- disc - The boundary discretization object

  Level: beginner

.seealso: PetscDSGetDiscretization(), PetscDSSetDiscretization(), PetscDSGetNumFields(), PetscDSCreate()
@*/
PetscErrorCode PetscDSAddDiscretization(PetscDS prob, PetscObject disc)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscDSSetDiscretization(prob, prob->Nf, disc);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSAddBdDiscretization"
/*@
  PetscDSAddBdDiscretization - Adds a boundary discretization object

  Not collective

  Input Parameters:
+ prob - The PetscDS object
- disc - The boundary discretization object

  Level: beginner

.seealso: PetscDSGetBdDiscretization(), PetscDSSetBdDiscretization(), PetscDSGetNumFields(), PetscDSCreate()
@*/
PetscErrorCode PetscDSAddBdDiscretization(PetscDS prob, PetscObject disc)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscDSSetBdDiscretization(prob, prob->Nf, disc);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSGetImplicit"
/*@
  PetscDSGetImplicit - Returns the flag for implicit solve for this field. This is just a guide for IMEX

  Not collective

  Input Parameters:
+ prob - The PetscDS object
- f - The field number

  Output Parameter:
. implicit - The flag indicating what kind of solve to use for this field

  Level: developer

.seealso: PetscDSSetImplicit(), PetscDSSetDiscretization(), PetscDSAddDiscretization(), PetscDSGetBdDiscretization(), PetscDSGetNumFields(), PetscDSCreate()
@*/
PetscErrorCode PetscDSGetImplicit(PetscDS prob, PetscInt f, PetscBool *implicit)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  PetscValidPointer(implicit, 3);
  if ((f < 0) || (f >= prob->Nf)) SETERRQ2(PETSC_COMM_SELF, PETSC_ERR_ARG_OUTOFRANGE, "Field number %d must be in [0, %d)", f, prob->Nf);
  *implicit = prob->implicit[f];
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSSetImplicit"
/*@
  PetscDSSetImplicit - Set the flag for implicit solve for this field. This is just a guide for IMEX

  Not collective

  Input Parameters:
+ prob - The PetscDS object
. f - The field number
- implicit - The flag indicating what kind of solve to use for this field

  Level: developer

.seealso: PetscDSGetImplicit(), PetscDSSetDiscretization(), PetscDSAddDiscretization(), PetscDSGetBdDiscretization(), PetscDSGetNumFields(), PetscDSCreate()
@*/
PetscErrorCode PetscDSSetImplicit(PetscDS prob, PetscInt f, PetscBool implicit)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  if ((f < 0) || (f >= prob->Nf)) SETERRQ2(PETSC_COMM_SELF, PETSC_ERR_ARG_OUTOFRANGE, "Field number %d must be in [0, %d)", f, prob->Nf);
  prob->implicit[f] = implicit;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSGetAdjacency"
/*@
  PetscDSGetAdjacency - Returns the flags for determining variable influence

  Not collective

  Input Parameters:
+ prob - The PetscDS object
- f - The field number

  Output Parameter:
+ useCone    - Flag for variable influence starting with the cone operation
- useClosure - Flag for variable influence using transitive closure

  Note: See the discussion in DMPlexGetAdjacencyUseCone() and DMPlexGetAdjacencyUseClosure()

  Level: developer

.seealso: PetscDSSetAdjacency(), DMPlexGetAdjacencyUseCone(), DMPlexGetAdjacencyUseClosure(), PetscDSSetDiscretization(), PetscDSAddDiscretization(), PetscDSGetBdDiscretization(), PetscDSGetNumFields(), PetscDSCreate()
@*/
PetscErrorCode PetscDSGetAdjacency(PetscDS prob, PetscInt f, PetscBool *useCone, PetscBool *useClosure)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  PetscValidPointer(useCone, 3);
  PetscValidPointer(useClosure, 4);
  if ((f < 0) || (f >= prob->Nf)) SETERRQ2(PETSC_COMM_SELF, PETSC_ERR_ARG_OUTOFRANGE, "Field number %d must be in [0, %d)", f, prob->Nf);
  *useCone    = prob->adjacency[f*2+0];
  *useClosure = prob->adjacency[f*2+1];
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSSetAdjacency"
/*@
  PetscDSSetAdjacency - Set the flags for determining variable influence

  Not collective

  Input Parameters:
+ prob - The PetscDS object
. f - The field number
. useCone    - Flag for variable influence starting with the cone operation
- useClosure - Flag for variable influence using transitive closure

  Note: See the discussion in DMPlexGetAdjacencyUseCone() and DMPlexGetAdjacencyUseClosure()

  Level: developer

.seealso: PetscDSGetAdjacency(), DMPlexGetAdjacencyUseCone(), DMPlexGetAdjacencyUseClosure(), PetscDSSetDiscretization(), PetscDSAddDiscretization(), PetscDSGetBdDiscretization(), PetscDSGetNumFields(), PetscDSCreate()
@*/
PetscErrorCode PetscDSSetAdjacency(PetscDS prob, PetscInt f, PetscBool useCone, PetscBool useClosure)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  if ((f < 0) || (f >= prob->Nf)) SETERRQ2(PETSC_COMM_SELF, PETSC_ERR_ARG_OUTOFRANGE, "Field number %d must be in [0, %d)", f, prob->Nf);
  prob->adjacency[f*2+0] = useCone;
  prob->adjacency[f*2+1] = useClosure;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSGetObjective"
PetscErrorCode PetscDSGetObjective(PetscDS prob, PetscInt f,
                                   void (**obj)(PetscInt dim, PetscInt Nf, PetscInt NfAux,
                                                const PetscInt uOff[], const PetscInt uOff_x[], const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[],
                                                const PetscInt aOff[], const PetscInt aOff_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[],
                                                PetscReal t, const PetscReal x[], PetscScalar obj[]))
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  PetscValidPointer(obj, 2);
  if ((f < 0) || (f >= prob->Nf)) SETERRQ2(PETSC_COMM_SELF, PETSC_ERR_ARG_OUTOFRANGE, "Field number %d must be in [0, %d)", f, prob->Nf);
  *obj = prob->obj[f];
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSSetObjective"
PetscErrorCode PetscDSSetObjective(PetscDS prob, PetscInt f,
                                   void (*obj)(PetscInt dim, PetscInt Nf, PetscInt NfAux,
                                               const PetscInt uOff[], const PetscInt uOff_x[], const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[],
                                               const PetscInt aOff[], const PetscInt aOff_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[],
                                               PetscReal t, const PetscReal x[], PetscScalar obj[]))
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  PetscValidFunction(obj, 2);
  if (f < 0) SETERRQ1(PETSC_COMM_SELF, PETSC_ERR_ARG_OUTOFRANGE, "Field number %d must be non-negative", f);
  ierr = PetscDSEnlarge_Static(prob, f+1);CHKERRQ(ierr);
  prob->obj[f] = obj;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSGetResidual"
/*@C
  PetscDSGetResidual - Get the pointwise residual function for a given test field

  Not collective

  Input Parameters:
+ prob - The PetscDS
- f    - The test field number

  Output Parameters:
+ f0 - integrand for the test function term
- f1 - integrand for the test function gradient term

  Note: We are using a first order FEM model for the weak form:

  \int_\Omega \phi f_0(u, u_t, \nabla u, x, t) + \nabla\phi \cdot {\vec f}_1(u, u_t, \nabla u, x, t)

The calling sequence for the callbacks f0 and f1 is given by:

$ f0(PetscInt dim, PetscInt Nf, PetscInt NfAux,
$    const PetscInt uOff[], const PetscInt uOff_x[], const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[],
$    const PetscInt aOff[], const PetscInt aOff_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[],
$    PetscReal t, const PetscReal x[], PetscScalar f0[])

+ dim - the spatial dimension
. Nf - the number of fields
. uOff - the offset into u[] and u_t[] for each field
. uOff_x - the offset into u_x[] for each field
. u - each field evaluated at the current point
. u_t - the time derivative of each field evaluated at the current point
. u_x - the gradient of each field evaluated at the current point
. aOff - the offset into a[] and a_t[] for each auxiliary field
. aOff_x - the offset into a_x[] for each auxiliary field
. a - each auxiliary field evaluated at the current point
. a_t - the time derivative of each auxiliary field evaluated at the current point
. a_x - the gradient of auxiliary each field evaluated at the current point
. t - current time
. x - coordinates of the current point
- f0 - output values at the current point

  Level: intermediate

.seealso: PetscDSSetResidual()
@*/
PetscErrorCode PetscDSGetResidual(PetscDS prob, PetscInt f,
                                  void (**f0)(PetscInt dim, PetscInt Nf, PetscInt NfAux,
                                              const PetscInt uOff[], const PetscInt uOff_x[], const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[],
                                              const PetscInt aOff[], const PetscInt aOff_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[],
                                              PetscReal t, const PetscReal x[], PetscScalar f0[]),
                                  void (**f1)(PetscInt dim, PetscInt Nf, PetscInt NfAux,
                                              const PetscInt uOff[], const PetscInt uOff_x[], const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[],
                                              const PetscInt aOff[], const PetscInt aOff_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[],
                                              PetscReal t, const PetscReal x[], PetscScalar f1[]))
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  if ((f < 0) || (f >= prob->Nf)) SETERRQ2(PETSC_COMM_SELF, PETSC_ERR_ARG_OUTOFRANGE, "Field number %d must be in [0, %d)", f, prob->Nf);
  if (f0) {PetscValidPointer(f0, 3); *f0 = prob->f[f*2+0];}
  if (f1) {PetscValidPointer(f1, 4); *f1 = prob->f[f*2+1];}
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSSetResidual"
/*@C
  PetscDSSetResidual - Set the pointwise residual function for a given test field

  Not collective

  Input Parameters:
+ prob - The PetscDS
. f    - The test field number
. f0 - integrand for the test function term
- f1 - integrand for the test function gradient term

  Note: We are using a first order FEM model for the weak form:

  \int_\Omega \phi f_0(u, u_t, \nabla u, x, t) + \nabla\phi \cdot {\vec f}_1(u, u_t, \nabla u, x, t)

The calling sequence for the callbacks f0 and f1 is given by:

$ f0(PetscInt dim, PetscInt Nf, PetscInt NfAux,
$    const PetscInt uOff[], const PetscInt uOff_x[], const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[],
$    const PetscInt aOff[], const PetscInt aOff_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[],
$    PetscReal t, const PetscReal x[], PetscScalar f0[])

+ dim - the spatial dimension
. Nf - the number of fields
. uOff - the offset into u[] and u_t[] for each field
. uOff_x - the offset into u_x[] for each field
. u - each field evaluated at the current point
. u_t - the time derivative of each field evaluated at the current point
. u_x - the gradient of each field evaluated at the current point
. aOff - the offset into a[] and a_t[] for each auxiliary field
. aOff_x - the offset into a_x[] for each auxiliary field
. a - each auxiliary field evaluated at the current point
. a_t - the time derivative of each auxiliary field evaluated at the current point
. a_x - the gradient of auxiliary each field evaluated at the current point
. t - current time
. x - coordinates of the current point
- f0 - output values at the current point

  Level: intermediate

.seealso: PetscDSGetResidual()
@*/
PetscErrorCode PetscDSSetResidual(PetscDS prob, PetscInt f,
                                  void (*f0)(PetscInt dim, PetscInt Nf, PetscInt NfAux,
                                             const PetscInt uOff[], const PetscInt uOff_x[], const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[],
                                             const PetscInt aOff[], const PetscInt aOff_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[],
                                             PetscReal t, const PetscReal x[], PetscScalar f0[]),
                                  void (*f1)(PetscInt dim, PetscInt Nf, PetscInt NfAux,
                                             const PetscInt uOff[], const PetscInt uOff_x[], const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[],
                                             const PetscInt aOff[], const PetscInt aOff_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[],
                                             PetscReal t, const PetscReal x[], PetscScalar f1[]))
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  if (f0) PetscValidFunction(f0, 3);
  if (f1) PetscValidFunction(f1, 4);
  if (f < 0) SETERRQ1(PETSC_COMM_SELF, PETSC_ERR_ARG_OUTOFRANGE, "Field number %d must be non-negative", f);
  ierr = PetscDSEnlarge_Static(prob, f+1);CHKERRQ(ierr);
  prob->f[f*2+0] = f0;
  prob->f[f*2+1] = f1;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSGetJacobian"
/*@C
  PetscDSGetJacobian - Get the pointwise Jacobian function for given test and basis field

  Not collective

  Input Parameters:
+ prob - The PetscDS
. f    - The test field number
- g    - The field number

  Output Parameters:
+ g0 - integrand for the test and basis function term
. g1 - integrand for the test function and basis function gradient term
. g2 - integrand for the test function gradient and basis function term
- g3 - integrand for the test function gradient and basis function gradient term

  Note: We are using a first order FEM model for the weak form:

  \int_\Omega \phi g_0(u, u_t, \nabla u, x, t) \psi + \phi {\vec g}_1(u, u_t, \nabla u, x, t) \nabla \psi + \nabla\phi \cdot {\vec g}_2(u, u_t, \nabla u, x, t) \psi + \nabla\phi \cdot {\overleftrightarrow g}_3(u, u_t, \nabla u, x, t) \cdot \nabla \psi

The calling sequence for the callbacks g0, g1, g2 and g3 is given by:

$ g0(PetscInt dim, PetscInt Nf, PetscInt NfAux,
$    const PetscInt uOff[], const PetscInt uOff_x[], const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[],
$    const PetscInt aOff[], const PetscInt aOff_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[],
$    PetscReal t, const PetscReal u_tShift, const PetscReal x[], PetscScalar g0[])

+ dim - the spatial dimension
. Nf - the number of fields
. uOff - the offset into u[] and u_t[] for each field
. uOff_x - the offset into u_x[] for each field
. u - each field evaluated at the current point
. u_t - the time derivative of each field evaluated at the current point
. u_x - the gradient of each field evaluated at the current point
. aOff - the offset into a[] and a_t[] for each auxiliary field
. aOff_x - the offset into a_x[] for each auxiliary field
. a - each auxiliary field evaluated at the current point
. a_t - the time derivative of each auxiliary field evaluated at the current point
. a_x - the gradient of auxiliary each field evaluated at the current point
. t - current time
. u_tShift - the multiplier a for dF/dU_t
. x - coordinates of the current point
- g0 - output values at the current point

  Level: intermediate

.seealso: PetscDSSetJacobian()
@*/
PetscErrorCode PetscDSGetJacobian(PetscDS prob, PetscInt f, PetscInt g,
                                  void (**g0)(PetscInt dim, PetscInt Nf, PetscInt NfAux,
                                              const PetscInt uOff[], const PetscInt uOff_x[], const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[],
                                              const PetscInt aOff[], const PetscInt aOff_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[],
                                              PetscReal t, PetscReal u_tShift, const PetscReal x[], PetscScalar g0[]),
                                  void (**g1)(PetscInt dim, PetscInt Nf, PetscInt NfAux,
                                              const PetscInt uOff[], const PetscInt uOff_x[], const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[],
                                              const PetscInt aOff[], const PetscInt aOff_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[],
                                              PetscReal t, PetscReal u_tShift, const PetscReal x[], PetscScalar g1[]),
                                  void (**g2)(PetscInt dim, PetscInt Nf, PetscInt NfAux,
                                              const PetscInt uOff[], const PetscInt uOff_x[], const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[],
                                              const PetscInt aOff[], const PetscInt aOff_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[],
                                              PetscReal t, PetscReal u_tShift, const PetscReal x[], PetscScalar g2[]),
                                  void (**g3)(PetscInt dim, PetscInt Nf, PetscInt NfAux,
                                              const PetscInt uOff[], const PetscInt uOff_x[], const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[],
                                              const PetscInt aOff[], const PetscInt aOff_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[],
                                              PetscReal t, PetscReal u_tShift, const PetscReal x[], PetscScalar g3[]))
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  if ((f < 0) || (f >= prob->Nf)) SETERRQ2(PETSC_COMM_SELF, PETSC_ERR_ARG_OUTOFRANGE, "Field number %d must be in [0, %d)", f, prob->Nf);
  if ((g < 0) || (g >= prob->Nf)) SETERRQ2(PETSC_COMM_SELF, PETSC_ERR_ARG_OUTOFRANGE, "Field number %d must be in [0, %d)", g, prob->Nf);
  if (g0) {PetscValidPointer(g0, 4); *g0 = prob->g[(f*prob->Nf + g)*4+0];}
  if (g1) {PetscValidPointer(g1, 5); *g1 = prob->g[(f*prob->Nf + g)*4+1];}
  if (g2) {PetscValidPointer(g2, 6); *g2 = prob->g[(f*prob->Nf + g)*4+2];}
  if (g3) {PetscValidPointer(g3, 7); *g3 = prob->g[(f*prob->Nf + g)*4+3];}
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSSetJacobian"
/*@C
  PetscDSSetJacobian - Set the pointwise Jacobian function for given test and basis fields

  Not collective

  Input Parameters:
+ prob - The PetscDS
. f    - The test field number
. g    - The field number
. g0 - integrand for the test and basis function term
. g1 - integrand for the test function and basis function gradient term
. g2 - integrand for the test function gradient and basis function term
- g3 - integrand for the test function gradient and basis function gradient term

  Note: We are using a first order FEM model for the weak form:

  \int_\Omega \phi g_0(u, u_t, \nabla u, x, t) \psi + \phi {\vec g}_1(u, u_t, \nabla u, x, t) \nabla \psi + \nabla\phi \cdot {\vec g}_2(u, u_t, \nabla u, x, t) \psi + \nabla\phi \cdot {\overleftrightarrow g}_3(u, u_t, \nabla u, x, t) \cdot \nabla \psi

The calling sequence for the callbacks g0, g1, g2 and g3 is given by:

$ g0(PetscInt dim, PetscInt Nf, PetscInt NfAux,
$    const PetscInt uOff[], const PetscInt uOff_x[], const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[],
$    const PetscInt aOff[], const PetscInt aOff_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[],
$    PetscReal t, const PetscReal x[], PetscScalar g0[])

+ dim - the spatial dimension
. Nf - the number of fields
. uOff - the offset into u[] and u_t[] for each field
. uOff_x - the offset into u_x[] for each field
. u - each field evaluated at the current point
. u_t - the time derivative of each field evaluated at the current point
. u_x - the gradient of each field evaluated at the current point
. aOff - the offset into a[] and a_t[] for each auxiliary field
. aOff_x - the offset into a_x[] for each auxiliary field
. a - each auxiliary field evaluated at the current point
. a_t - the time derivative of each auxiliary field evaluated at the current point
. a_x - the gradient of auxiliary each field evaluated at the current point
. t - current time
. u_tShift - the multiplier a for dF/dU_t
. x - coordinates of the current point
- g0 - output values at the current point

  Level: intermediate

.seealso: PetscDSGetJacobian()
@*/
PetscErrorCode PetscDSSetJacobian(PetscDS prob, PetscInt f, PetscInt g,
                                  void (*g0)(PetscInt dim, PetscInt Nf, PetscInt NfAux,
                                             const PetscInt uOff[], const PetscInt uOff_x[], const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[],
                                             const PetscInt aOff[], const PetscInt aOff_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[],
                                             PetscReal t, PetscReal u_tShift, const PetscReal x[], PetscScalar g0[]),
                                  void (*g1)(PetscInt dim, PetscInt Nf, PetscInt NfAux,
                                             const PetscInt uOff[], const PetscInt uOff_x[], const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[],
                                             const PetscInt aOff[], const PetscInt aOff_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[],
                                             PetscReal t, PetscReal u_tShift, const PetscReal x[], PetscScalar g1[]),
                                  void (*g2)(PetscInt dim, PetscInt Nf, PetscInt NfAux,
                                             const PetscInt uOff[], const PetscInt uOff_x[], const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[],
                                             const PetscInt aOff[], const PetscInt aOff_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[],
                                             PetscReal t, PetscReal u_tShift, const PetscReal x[], PetscScalar g2[]),
                                  void (*g3)(PetscInt dim, PetscInt Nf, PetscInt NfAux,
                                             const PetscInt uOff[], const PetscInt uOff_x[], const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[],
                                             const PetscInt aOff[], const PetscInt aOff_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[],
                                             PetscReal t, PetscReal u_tShift, const PetscReal x[], PetscScalar g3[]))
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  if (g0) PetscValidFunction(g0, 4);
  if (g1) PetscValidFunction(g1, 5);
  if (g2) PetscValidFunction(g2, 6);
  if (g3) PetscValidFunction(g3, 7);
  if (f < 0) SETERRQ1(PETSC_COMM_SELF, PETSC_ERR_ARG_OUTOFRANGE, "Field number %d must be non-negative", f);
  if (g < 0) SETERRQ1(PETSC_COMM_SELF, PETSC_ERR_ARG_OUTOFRANGE, "Field number %d must be non-negative", g);
  ierr = PetscDSEnlarge_Static(prob, PetscMax(f, g)+1);CHKERRQ(ierr);
  prob->g[(f*prob->Nf + g)*4+0] = g0;
  prob->g[(f*prob->Nf + g)*4+1] = g1;
  prob->g[(f*prob->Nf + g)*4+2] = g2;
  prob->g[(f*prob->Nf + g)*4+3] = g3;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSHasJacobianPreconditioner"
/*@C
  PetscDSHasJacobianPreconditioner - Signals that a Jacobian preconditioner matrix has been set

  Not collective

  Input Parameter:
. prob - The PetscDS

  Output Parameter:
. hasJacPre - flag that pointwise function for Jacobian preconditioner matrix has been set

  Level: intermediate

.seealso: PetscDSGetJacobianPreconditioner(), PetscDSSetJacobianPreconditioner(), PetscDSGetJacobian()
@*/
PetscErrorCode PetscDSHasJacobianPreconditioner(PetscDS prob, PetscBool *hasJacPre)
{
  PetscInt f, g, h;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  *hasJacPre = PETSC_FALSE;
  for (f = 0; f < prob->Nf; ++f) {
    for (g = 0; g < prob->Nf; ++g) {
      for (h = 0; h < 4; ++h) {
        if (prob->gp[(f*prob->Nf + g)*4+h]) *hasJacPre = PETSC_TRUE;
      }
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSGetJacobianPreconditioner"
/*@C
  PetscDSGetJacobianPreconditioner - Get the pointwise Jacobian preconditioner function for given test and basis field. If this is missing, the system matrix is used to build the preconditioner.

  Not collective

  Input Parameters:
+ prob - The PetscDS
. f    - The test field number
- g    - The field number

  Output Parameters:
+ g0 - integrand for the test and basis function term
. g1 - integrand for the test function and basis function gradient term
. g2 - integrand for the test function gradient and basis function term
- g3 - integrand for the test function gradient and basis function gradient term

  Note: We are using a first order FEM model for the weak form:

  \int_\Omega \phi g_0(u, u_t, \nabla u, x, t) \psi + \phi {\vec g}_1(u, u_t, \nabla u, x, t) \nabla \psi + \nabla\phi \cdot {\vec g}_2(u, u_t, \nabla u, x, t) \psi + \nabla\phi \cdot {\overleftrightarrow g}_3(u, u_t, \nabla u, x, t) \cdot \nabla \psi

The calling sequence for the callbacks g0, g1, g2 and g3 is given by:

$ g0(PetscInt dim, PetscInt Nf, PetscInt NfAux,
$    const PetscInt uOff[], const PetscInt uOff_x[], const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[],
$    const PetscInt aOff[], const PetscInt aOff_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[],
$    PetscReal t, const PetscReal u_tShift, const PetscReal x[], PetscScalar g0[])

+ dim - the spatial dimension
. Nf - the number of fields
. uOff - the offset into u[] and u_t[] for each field
. uOff_x - the offset into u_x[] for each field
. u - each field evaluated at the current point
. u_t - the time derivative of each field evaluated at the current point
. u_x - the gradient of each field evaluated at the current point
. aOff - the offset into a[] and a_t[] for each auxiliary field
. aOff_x - the offset into a_x[] for each auxiliary field
. a - each auxiliary field evaluated at the current point
. a_t - the time derivative of each auxiliary field evaluated at the current point
. a_x - the gradient of auxiliary each field evaluated at the current point
. t - current time
. u_tShift - the multiplier a for dF/dU_t
. x - coordinates of the current point
- g0 - output values at the current point

  Level: intermediate

.seealso: PetscDSSetJacobianPreconditioner(), PetscDSGetJacobian()
@*/
PetscErrorCode PetscDSGetJacobianPreconditioner(PetscDS prob, PetscInt f, PetscInt g,
                                  void (**g0)(PetscInt dim, PetscInt Nf, PetscInt NfAux,
                                              const PetscInt uOff[], const PetscInt uOff_x[], const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[],
                                              const PetscInt aOff[], const PetscInt aOff_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[],
                                              PetscReal t, PetscReal u_tShift, const PetscReal x[], PetscScalar g0[]),
                                  void (**g1)(PetscInt dim, PetscInt Nf, PetscInt NfAux,
                                              const PetscInt uOff[], const PetscInt uOff_x[], const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[],
                                              const PetscInt aOff[], const PetscInt aOff_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[],
                                              PetscReal t, PetscReal u_tShift, const PetscReal x[], PetscScalar g1[]),
                                  void (**g2)(PetscInt dim, PetscInt Nf, PetscInt NfAux,
                                              const PetscInt uOff[], const PetscInt uOff_x[], const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[],
                                              const PetscInt aOff[], const PetscInt aOff_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[],
                                              PetscReal t, PetscReal u_tShift, const PetscReal x[], PetscScalar g2[]),
                                  void (**g3)(PetscInt dim, PetscInt Nf, PetscInt NfAux,
                                              const PetscInt uOff[], const PetscInt uOff_x[], const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[],
                                              const PetscInt aOff[], const PetscInt aOff_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[],
                                              PetscReal t, PetscReal u_tShift, const PetscReal x[], PetscScalar g3[]))
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  if ((f < 0) || (f >= prob->Nf)) SETERRQ2(PETSC_COMM_SELF, PETSC_ERR_ARG_OUTOFRANGE, "Field number %d must be in [0, %d)", f, prob->Nf);
  if ((g < 0) || (g >= prob->Nf)) SETERRQ2(PETSC_COMM_SELF, PETSC_ERR_ARG_OUTOFRANGE, "Field number %d must be in [0, %d)", g, prob->Nf);
  if (g0) {PetscValidPointer(g0, 4); *g0 = prob->gp[(f*prob->Nf + g)*4+0];}
  if (g1) {PetscValidPointer(g1, 5); *g1 = prob->gp[(f*prob->Nf + g)*4+1];}
  if (g2) {PetscValidPointer(g2, 6); *g2 = prob->gp[(f*prob->Nf + g)*4+2];}
  if (g3) {PetscValidPointer(g3, 7); *g3 = prob->gp[(f*prob->Nf + g)*4+3];}
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSSetJacobianPreconditioner"
/*@C
  PetscDSSetJacobianPreconditioner - Set the pointwise Jacobian preconditioner function for given test and basis fields. If this is missing, the system matrix is used to build the preconditioner.

  Not collective

  Input Parameters:
+ prob - The PetscDS
. f    - The test field number
. g    - The field number
. g0 - integrand for the test and basis function term
. g1 - integrand for the test function and basis function gradient term
. g2 - integrand for the test function gradient and basis function term
- g3 - integrand for the test function gradient and basis function gradient term

  Note: We are using a first order FEM model for the weak form:

  \int_\Omega \phi g_0(u, u_t, \nabla u, x, t) \psi + \phi {\vec g}_1(u, u_t, \nabla u, x, t) \nabla \psi + \nabla\phi \cdot {\vec g}_2(u, u_t, \nabla u, x, t) \psi + \nabla\phi \cdot {\overleftrightarrow g}_3(u, u_t, \nabla u, x, t) \cdot \nabla \psi

The calling sequence for the callbacks g0, g1, g2 and g3 is given by:

$ g0(PetscInt dim, PetscInt Nf, PetscInt NfAux,
$    const PetscInt uOff[], const PetscInt uOff_x[], const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[],
$    const PetscInt aOff[], const PetscInt aOff_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[],
$    PetscReal t, const PetscReal x[], PetscScalar g0[])

+ dim - the spatial dimension
. Nf - the number of fields
. uOff - the offset into u[] and u_t[] for each field
. uOff_x - the offset into u_x[] for each field
. u - each field evaluated at the current point
. u_t - the time derivative of each field evaluated at the current point
. u_x - the gradient of each field evaluated at the current point
. aOff - the offset into a[] and a_t[] for each auxiliary field
. aOff_x - the offset into a_x[] for each auxiliary field
. a - each auxiliary field evaluated at the current point
. a_t - the time derivative of each auxiliary field evaluated at the current point
. a_x - the gradient of auxiliary each field evaluated at the current point
. t - current time
. u_tShift - the multiplier a for dF/dU_t
. x - coordinates of the current point
- g0 - output values at the current point

  Level: intermediate

.seealso: PetscDSGetJacobianPreconditioner(), PetscDSSetJacobian()
@*/
PetscErrorCode PetscDSSetJacobianPreconditioner(PetscDS prob, PetscInt f, PetscInt g,
                                  void (*g0)(PetscInt dim, PetscInt Nf, PetscInt NfAux,
                                             const PetscInt uOff[], const PetscInt uOff_x[], const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[],
                                             const PetscInt aOff[], const PetscInt aOff_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[],
                                             PetscReal t, PetscReal u_tShift, const PetscReal x[], PetscScalar g0[]),
                                  void (*g1)(PetscInt dim, PetscInt Nf, PetscInt NfAux,
                                             const PetscInt uOff[], const PetscInt uOff_x[], const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[],
                                             const PetscInt aOff[], const PetscInt aOff_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[],
                                             PetscReal t, PetscReal u_tShift, const PetscReal x[], PetscScalar g1[]),
                                  void (*g2)(PetscInt dim, PetscInt Nf, PetscInt NfAux,
                                             const PetscInt uOff[], const PetscInt uOff_x[], const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[],
                                             const PetscInt aOff[], const PetscInt aOff_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[],
                                             PetscReal t, PetscReal u_tShift, const PetscReal x[], PetscScalar g2[]),
                                  void (*g3)(PetscInt dim, PetscInt Nf, PetscInt NfAux,
                                             const PetscInt uOff[], const PetscInt uOff_x[], const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[],
                                             const PetscInt aOff[], const PetscInt aOff_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[],
                                             PetscReal t, PetscReal u_tShift, const PetscReal x[], PetscScalar g3[]))
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  if (g0) PetscValidFunction(g0, 4);
  if (g1) PetscValidFunction(g1, 5);
  if (g2) PetscValidFunction(g2, 6);
  if (g3) PetscValidFunction(g3, 7);
  if (f < 0) SETERRQ1(PETSC_COMM_SELF, PETSC_ERR_ARG_OUTOFRANGE, "Field number %d must be non-negative", f);
  if (g < 0) SETERRQ1(PETSC_COMM_SELF, PETSC_ERR_ARG_OUTOFRANGE, "Field number %d must be non-negative", g);
  ierr = PetscDSEnlarge_Static(prob, PetscMax(f, g)+1);CHKERRQ(ierr);
  prob->gp[(f*prob->Nf + g)*4+0] = g0;
  prob->gp[(f*prob->Nf + g)*4+1] = g1;
  prob->gp[(f*prob->Nf + g)*4+2] = g2;
  prob->gp[(f*prob->Nf + g)*4+3] = g3;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSGetRiemannSolver"
/*@C
  PetscDSGetRiemannSolver - Returns the Riemann solver for the given field

  Not collective

  Input Arguments:
+ prob - The PetscDS object
- f    - The field number

  Output Argument:
. r    - Riemann solver

  Calling sequence for r:

$ r(PetscInt dim, PetscInt Nf, const PetscReal x[], const PetscReal n[], const PetscScalar uL[], const PetscScalar uR[], PetscScalar flux[], void *ctx)

+ dim  - The spatial dimension
. Nf   - The number of fields
. x    - The coordinates at a point on the interface
. n    - The normal vector to the interface
. uL   - The state vector to the left of the interface
. uR   - The state vector to the right of the interface
. flux - output array of flux through the interface
- ctx  - optional user context

  Level: intermediate

.seealso: PetscDSSetRiemannSolver()
@*/
PetscErrorCode PetscDSGetRiemannSolver(PetscDS prob, PetscInt f,
                                       void (**r)(PetscInt dim, PetscInt Nf, const PetscReal x[], const PetscReal n[], const PetscScalar uL[], const PetscScalar uR[], PetscScalar flux[], void *ctx))
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  if ((f < 0) || (f >= prob->Nf)) SETERRQ2(PETSC_COMM_SELF, PETSC_ERR_ARG_OUTOFRANGE, "Field number %d must be in [0, %d)", f, prob->Nf);
  PetscValidPointer(r, 3);
  *r = prob->r[f];
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSSetRiemannSolver"
/*@C
  PetscDSSetRiemannSolver - Sets the Riemann solver for the given field

  Not collective

  Input Arguments:
+ prob - The PetscDS object
. f    - The field number
- r    - Riemann solver

  Calling sequence for r:

$ r(PetscInt dim, PetscInt Nf, const PetscReal x[], const PetscReal n[], const PetscScalar uL[], const PetscScalar uR[], PetscScalar flux[], void *ctx)

+ dim  - The spatial dimension
. Nf   - The number of fields
. x    - The coordinates at a point on the interface
. n    - The normal vector to the interface
. uL   - The state vector to the left of the interface
. uR   - The state vector to the right of the interface
. flux - output array of flux through the interface
- ctx  - optional user context

  Level: intermediate

.seealso: PetscDSGetRiemannSolver()
@*/
PetscErrorCode PetscDSSetRiemannSolver(PetscDS prob, PetscInt f,
                                       void (*r)(PetscInt dim, PetscInt Nf, const PetscReal x[], const PetscReal n[], const PetscScalar uL[], const PetscScalar uR[], PetscScalar flux[], void *ctx))
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  PetscValidFunction(r, 3);
  if (f < 0) SETERRQ1(PETSC_COMM_SELF, PETSC_ERR_ARG_OUTOFRANGE, "Field number %d must be non-negative", f);
  ierr = PetscDSEnlarge_Static(prob, f+1);CHKERRQ(ierr);
  prob->r[f] = r;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSGetContext"
PetscErrorCode PetscDSGetContext(PetscDS prob, PetscInt f, void **ctx)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  if ((f < 0) || (f >= prob->Nf)) SETERRQ2(PETSC_COMM_SELF, PETSC_ERR_ARG_OUTOFRANGE, "Field number %d must be in [0, %d)", f, prob->Nf);
  PetscValidPointer(ctx, 3);
  *ctx = prob->ctx[f];
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSSetContext"
PetscErrorCode PetscDSSetContext(PetscDS prob, PetscInt f, void *ctx)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  if (f < 0) SETERRQ1(PETSC_COMM_SELF, PETSC_ERR_ARG_OUTOFRANGE, "Field number %d must be non-negative", f);
  ierr = PetscDSEnlarge_Static(prob, f+1);CHKERRQ(ierr);
  prob->ctx[f] = ctx;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSGetBdResidual"
/*@C
  PetscDSGetBdResidual - Get the pointwise boundary residual function for a given test field

  Not collective

  Input Parameters:
+ prob - The PetscDS
- f    - The test field number

  Output Parameters:
+ f0 - boundary integrand for the test function term
- f1 - boundary integrand for the test function gradient term

  Note: We are using a first order FEM model for the weak form:

  \int_\Gamma \phi {\vec f}_0(u, u_t, \nabla u, x, t) \cdot \hat n + \nabla\phi \cdot {\overleftrightarrow f}_1(u, u_t, \nabla u, x, t) \cdot \hat n

The calling sequence for the callbacks f0 and f1 is given by:

$ f0(PetscInt dim, PetscInt Nf, PetscInt NfAux,
$    const PetscInt uOff[], const PetscInt uOff_x[], const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[],
$    const PetscInt aOff[], const PetscInt aOff_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[],
$    PetscReal t, const PetscReal x[], const PetscReal n[], PetscScalar f0[])

+ dim - the spatial dimension
. Nf - the number of fields
. uOff - the offset into u[] and u_t[] for each field
. uOff_x - the offset into u_x[] for each field
. u - each field evaluated at the current point
. u_t - the time derivative of each field evaluated at the current point
. u_x - the gradient of each field evaluated at the current point
. aOff - the offset into a[] and a_t[] for each auxiliary field
. aOff_x - the offset into a_x[] for each auxiliary field
. a - each auxiliary field evaluated at the current point
. a_t - the time derivative of each auxiliary field evaluated at the current point
. a_x - the gradient of auxiliary each field evaluated at the current point
. t - current time
. x - coordinates of the current point
. n - unit normal at the current point
- f0 - output values at the current point

  Level: intermediate

.seealso: PetscDSSetBdResidual()
@*/
PetscErrorCode PetscDSGetBdResidual(PetscDS prob, PetscInt f,
                                    void (**f0)(PetscInt dim, PetscInt Nf, PetscInt NfAux,
                                                const PetscInt uOff[], const PetscInt uOff_x[], const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[],
                                                const PetscInt aOff[], const PetscInt aOff_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[],
                                                PetscReal t, const PetscReal x[], const PetscReal n[], PetscScalar f0[]),
                                    void (**f1)(PetscInt dim, PetscInt Nf, PetscInt NfAux,
                                                const PetscInt uOff[], const PetscInt uOff_x[], const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[],
                                                const PetscInt aOff[], const PetscInt aOff_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[],
                                                PetscReal t, const PetscReal x[], const PetscReal n[], PetscScalar f1[]))
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  if ((f < 0) || (f >= prob->Nf)) SETERRQ2(PETSC_COMM_SELF, PETSC_ERR_ARG_OUTOFRANGE, "Field number %d must be in [0, %d)", f, prob->Nf);
  if (f0) {PetscValidPointer(f0, 3); *f0 = prob->fBd[f*2+0];}
  if (f1) {PetscValidPointer(f1, 4); *f1 = prob->fBd[f*2+1];}
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSSetBdResidual"
/*@C
  PetscDSSetBdResidual - Get the pointwise boundary residual function for a given test field

  Not collective

  Input Parameters:
+ prob - The PetscDS
. f    - The test field number
. f0 - boundary integrand for the test function term
- f1 - boundary integrand for the test function gradient term

  Note: We are using a first order FEM model for the weak form:

  \int_\Gamma \phi {\vec f}_0(u, u_t, \nabla u, x, t) \cdot \hat n + \nabla\phi \cdot {\overleftrightarrow f}_1(u, u_t, \nabla u, x, t) \cdot \hat n

The calling sequence for the callbacks f0 and f1 is given by:

$ f0(PetscInt dim, PetscInt Nf, PetscInt NfAux,
$    const PetscInt uOff[], const PetscInt uOff_x[], const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[],
$    const PetscInt aOff[], const PetscInt aOff_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[],
$    PetscReal t, const PetscReal x[], const PetscReal n[], PetscScalar f0[])

+ dim - the spatial dimension
. Nf - the number of fields
. uOff - the offset into u[] and u_t[] for each field
. uOff_x - the offset into u_x[] for each field
. u - each field evaluated at the current point
. u_t - the time derivative of each field evaluated at the current point
. u_x - the gradient of each field evaluated at the current point
. aOff - the offset into a[] and a_t[] for each auxiliary field
. aOff_x - the offset into a_x[] for each auxiliary field
. a - each auxiliary field evaluated at the current point
. a_t - the time derivative of each auxiliary field evaluated at the current point
. a_x - the gradient of auxiliary each field evaluated at the current point
. t - current time
. x - coordinates of the current point
. n - unit normal at the current point
- f0 - output values at the current point

  Level: intermediate

.seealso: PetscDSGetBdResidual()
@*/
PetscErrorCode PetscDSSetBdResidual(PetscDS prob, PetscInt f,
                                    void (*f0)(PetscInt dim, PetscInt Nf, PetscInt NfAux,
                                               const PetscInt uOff[], const PetscInt uOff_x[], const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[],
                                               const PetscInt aOff[], const PetscInt aOff_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[],
                                               PetscReal t, const PetscReal x[], const PetscReal n[], PetscScalar f0[]),
                                    void (*f1)(PetscInt dim, PetscInt Nf, PetscInt NfAux,
                                               const PetscInt uOff[], const PetscInt uOff_x[], const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[],
                                               const PetscInt aOff[], const PetscInt aOff_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[],
                                               PetscReal t, const PetscReal x[], const PetscReal n[], PetscScalar f1[]))
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  if (f < 0) SETERRQ1(PETSC_COMM_SELF, PETSC_ERR_ARG_OUTOFRANGE, "Field number %d must be non-negative", f);
  ierr = PetscDSEnlarge_Static(prob, f+1);CHKERRQ(ierr);
  if (f0) {PetscValidFunction(f0, 3); prob->fBd[f*2+0] = f0;}
  if (f1) {PetscValidFunction(f1, 4); prob->fBd[f*2+1] = f1;}
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSGetBdJacobian"
/*@C
  PetscDSGetBdJacobian - Get the pointwise boundary Jacobian function for given test and basis field

  Not collective

  Input Parameters:
+ prob - The PetscDS
. f    - The test field number
- g    - The field number

  Output Parameters:
+ g0 - integrand for the test and basis function term
. g1 - integrand for the test function and basis function gradient term
. g2 - integrand for the test function gradient and basis function term
- g3 - integrand for the test function gradient and basis function gradient term

  Note: We are using a first order FEM model for the weak form:

  \int_\Gamma \phi {\vec g}_0(u, u_t, \nabla u, x, t) \cdot \hat n \psi + \phi {\vec g}_1(u, u_t, \nabla u, x, t) \cdot \hat n \nabla \psi + \nabla\phi \cdot {\vec g}_2(u, u_t, \nabla u, x, t) \cdot \hat n \psi + \nabla\phi \cdot {\overleftrightarrow g}_3(u, u_t, \nabla u, x, t) \cdot \hat n \cdot \nabla \psi

The calling sequence for the callbacks g0, g1, g2 and g3 is given by:

$ g0(PetscInt dim, PetscInt Nf, PetscInt NfAux,
$    const PetscInt uOff[], const PetscInt uOff_x[], const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[],
$    const PetscInt aOff[], const PetscInt aOff_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[],
$    PetscReal t, const PetscReal x[], const PetscReal n[], PetscScalar g0[])

+ dim - the spatial dimension
. Nf - the number of fields
. uOff - the offset into u[] and u_t[] for each field
. uOff_x - the offset into u_x[] for each field
. u - each field evaluated at the current point
. u_t - the time derivative of each field evaluated at the current point
. u_x - the gradient of each field evaluated at the current point
. aOff - the offset into a[] and a_t[] for each auxiliary field
. aOff_x - the offset into a_x[] for each auxiliary field
. a - each auxiliary field evaluated at the current point
. a_t - the time derivative of each auxiliary field evaluated at the current point
. a_x - the gradient of auxiliary each field evaluated at the current point
. t - current time
. u_tShift - the multiplier a for dF/dU_t
. x - coordinates of the current point
. n - normal at the current point
- g0 - output values at the current point

  Level: intermediate

.seealso: PetscDSSetBdJacobian()
@*/
PetscErrorCode PetscDSGetBdJacobian(PetscDS prob, PetscInt f, PetscInt g,
                                    void (**g0)(PetscInt dim, PetscInt Nf, PetscInt NfAux,
                                                const PetscInt uOff[], const PetscInt uOff_x[], const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[],
                                                const PetscInt aOff[], const PetscInt aOff_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[],
                                                PetscReal t, PetscReal u_tShift, const PetscReal x[], const PetscReal n[], PetscScalar g0[]),
                                    void (**g1)(PetscInt dim, PetscInt Nf, PetscInt NfAux,
                                                const PetscInt uOff[], const PetscInt uOff_x[], const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[],
                                                const PetscInt aOff[], const PetscInt aOff_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[],
                                                PetscReal t, PetscReal u_tShift, const PetscReal x[], const PetscReal n[], PetscScalar g1[]),
                                    void (**g2)(PetscInt dim, PetscInt Nf, PetscInt NfAux,
                                                const PetscInt uOff[], const PetscInt uOff_x[], const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[],
                                                const PetscInt aOff[], const PetscInt aOff_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[],
                                                PetscReal t, PetscReal u_tShift, const PetscReal x[], const PetscReal n[], PetscScalar g2[]),
                                    void (**g3)(PetscInt dim, PetscInt Nf, PetscInt NfAux,
                                                const PetscInt uOff[], const PetscInt uOff_x[], const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[],
                                                const PetscInt aOff[], const PetscInt aOff_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[],
                                                PetscReal t, PetscReal u_tShift, const PetscReal x[], const PetscReal n[], PetscScalar g3[]))
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  if ((f < 0) || (f >= prob->Nf)) SETERRQ2(PETSC_COMM_SELF, PETSC_ERR_ARG_OUTOFRANGE, "Field number %d must be in [0, %d)", f, prob->Nf);
  if ((g < 0) || (g >= prob->Nf)) SETERRQ2(PETSC_COMM_SELF, PETSC_ERR_ARG_OUTOFRANGE, "Field number %d must be in [0, %d)", g, prob->Nf);
  if (g0) {PetscValidPointer(g0, 4); *g0 = prob->gBd[(f*prob->Nf + g)*4+0];}
  if (g1) {PetscValidPointer(g1, 5); *g1 = prob->gBd[(f*prob->Nf + g)*4+1];}
  if (g2) {PetscValidPointer(g2, 6); *g2 = prob->gBd[(f*prob->Nf + g)*4+2];}
  if (g3) {PetscValidPointer(g3, 7); *g3 = prob->gBd[(f*prob->Nf + g)*4+3];}
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSSetBdJacobian"
/*@C
  PetscDSSetBdJacobian - Set the pointwise boundary Jacobian function for given test and basis field

  Not collective

  Input Parameters:
+ prob - The PetscDS
. f    - The test field number
. g    - The field number
. g0 - integrand for the test and basis function term
. g1 - integrand for the test function and basis function gradient term
. g2 - integrand for the test function gradient and basis function term
- g3 - integrand for the test function gradient and basis function gradient term

  Note: We are using a first order FEM model for the weak form:

  \int_\Gamma \phi {\vec g}_0(u, u_t, \nabla u, x, t) \cdot \hat n \psi + \phi {\vec g}_1(u, u_t, \nabla u, x, t) \cdot \hat n \nabla \psi + \nabla\phi \cdot {\vec g}_2(u, u_t, \nabla u, x, t) \cdot \hat n \psi + \nabla\phi \cdot {\overleftrightarrow g}_3(u, u_t, \nabla u, x, t) \cdot \hat n \cdot \nabla \psi

The calling sequence for the callbacks g0, g1, g2 and g3 is given by:

$ g0(PetscInt dim, PetscInt Nf, PetscInt NfAux,
$    const PetscInt uOff[], const PetscInt uOff_x[], const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[],
$    const PetscInt aOff[], const PetscInt aOff_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[],
$    PetscReal t, const PetscReal x[], const PetscReal n[], PetscScalar g0[])

+ dim - the spatial dimension
. Nf - the number of fields
. uOff - the offset into u[] and u_t[] for each field
. uOff_x - the offset into u_x[] for each field
. u - each field evaluated at the current point
. u_t - the time derivative of each field evaluated at the current point
. u_x - the gradient of each field evaluated at the current point
. aOff - the offset into a[] and a_t[] for each auxiliary field
. aOff_x - the offset into a_x[] for each auxiliary field
. a - each auxiliary field evaluated at the current point
. a_t - the time derivative of each auxiliary field evaluated at the current point
. a_x - the gradient of auxiliary each field evaluated at the current point
. t - current time
. u_tShift - the multiplier a for dF/dU_t
. x - coordinates of the current point
. n - normal at the current point
- g0 - output values at the current point

  Level: intermediate

.seealso: PetscDSGetBdJacobian()
@*/
PetscErrorCode PetscDSSetBdJacobian(PetscDS prob, PetscInt f, PetscInt g,
                                    void (*g0)(PetscInt dim, PetscInt Nf, PetscInt NfAux,
                                               const PetscInt uOff[], const PetscInt uOff_x[], const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[],
                                               const PetscInt aOff[], const PetscInt aOff_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[],
                                               PetscReal t, PetscReal u_tShift, const PetscReal x[], const PetscReal n[], PetscScalar g0[]),
                                    void (*g1)(PetscInt dim, PetscInt Nf, PetscInt NfAux,
                                               const PetscInt uOff[], const PetscInt uOff_x[], const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[],
                                               const PetscInt aOff[], const PetscInt aOff_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[],
                                               PetscReal t, PetscReal u_tShift, const PetscReal x[], const PetscReal n[], PetscScalar g1[]),
                                    void (*g2)(PetscInt dim, PetscInt Nf, PetscInt NfAux,
                                               const PetscInt uOff[], const PetscInt uOff_x[], const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[],
                                               const PetscInt aOff[], const PetscInt aOff_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[],
                                               PetscReal t, PetscReal u_tShift, const PetscReal x[], const PetscReal n[], PetscScalar g2[]),
                                    void (*g3)(PetscInt dim, PetscInt Nf, PetscInt NfAux,
                                               const PetscInt uOff[], const PetscInt uOff_x[], const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[],
                                               const PetscInt aOff[], const PetscInt aOff_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[],
                                               PetscReal t, PetscReal u_tShift, const PetscReal x[], const PetscReal n[], PetscScalar g3[]))
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  if (g0) PetscValidFunction(g0, 4);
  if (g1) PetscValidFunction(g1, 5);
  if (g2) PetscValidFunction(g2, 6);
  if (g3) PetscValidFunction(g3, 7);
  if (f < 0) SETERRQ1(PETSC_COMM_SELF, PETSC_ERR_ARG_OUTOFRANGE, "Field number %d must be non-negative", f);
  if (g < 0) SETERRQ1(PETSC_COMM_SELF, PETSC_ERR_ARG_OUTOFRANGE, "Field number %d must be non-negative", g);
  ierr = PetscDSEnlarge_Static(prob, PetscMax(f, g)+1);CHKERRQ(ierr);
  prob->gBd[(f*prob->Nf + g)*4+0] = g0;
  prob->gBd[(f*prob->Nf + g)*4+1] = g1;
  prob->gBd[(f*prob->Nf + g)*4+2] = g2;
  prob->gBd[(f*prob->Nf + g)*4+3] = g3;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSGetFieldOffset"
/*@
  PetscDSGetFieldOffset - Returns the offset of the given field in the full space basis

  Not collective

  Input Parameters:
+ prob - The PetscDS object
- f - The field number

  Output Parameter:
. off - The offset

  Level: beginner

.seealso: PetscDSGetBdFieldOffset(), PetscDSGetNumFields(), PetscDSCreate()
@*/
PetscErrorCode PetscDSGetFieldOffset(PetscDS prob, PetscInt f, PetscInt *off)
{
  PetscInt       g;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  PetscValidPointer(off, 3);
  if ((f < 0) || (f >= prob->Nf)) SETERRQ2(PETSC_COMM_SELF, PETSC_ERR_ARG_OUTOFRANGE, "Field number %d must be in [0, %d)", f, prob->Nf);
  *off = 0;
  for (g = 0; g < f; ++g) {
    PetscFE  fe = (PetscFE) prob->disc[g];
    PetscInt Nb, Nc;

    ierr = PetscFEGetDimension(fe, &Nb);CHKERRQ(ierr);
    ierr = PetscFEGetNumComponents(fe, &Nc);CHKERRQ(ierr);
    *off += Nb*Nc;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSGetBdFieldOffset"
/*@
  PetscDSGetBdFieldOffset - Returns the offset of the given field in the full space boundary basis

  Not collective

  Input Parameters:
+ prob - The PetscDS object
- f - The field number

  Output Parameter:
. off - The boundary offset

  Level: beginner

.seealso: PetscDSGetFieldOffset(), PetscDSGetNumFields(), PetscDSCreate()
@*/
PetscErrorCode PetscDSGetBdFieldOffset(PetscDS prob, PetscInt f, PetscInt *off)
{
  PetscInt       g;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  PetscValidPointer(off, 3);
  if ((f < 0) || (f >= prob->Nf)) SETERRQ2(PETSC_COMM_SELF, PETSC_ERR_ARG_OUTOFRANGE, "Field number %d must be in [0, %d)", f, prob->Nf);
  *off = 0;
  for (g = 0; g < f; ++g) {
    PetscFE  fe = (PetscFE) prob->discBd[g];
    PetscInt Nb, Nc;

    ierr = PetscFEGetDimension(fe, &Nb);CHKERRQ(ierr);
    ierr = PetscFEGetNumComponents(fe, &Nc);CHKERRQ(ierr);
    *off += Nb*Nc;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSGetComponentOffset"
/*@
  PetscDSGetComponentOffset - Returns the offset of the given field on an evaluation point

  Not collective

  Input Parameters:
+ prob - The PetscDS object
- f - The field number

  Output Parameter:
. off - The offset

  Level: beginner

.seealso: PetscDSGetBdFieldOffset(), PetscDSGetNumFields(), PetscDSCreate()
@*/
PetscErrorCode PetscDSGetComponentOffset(PetscDS prob, PetscInt f, PetscInt *off)
{
  PetscInt       g;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  PetscValidPointer(off, 3);
  if ((f < 0) || (f >= prob->Nf)) SETERRQ2(PETSC_COMM_SELF, PETSC_ERR_ARG_OUTOFRANGE, "Field number %d must be in [0, %d)", f, prob->Nf);
  *off = 0;
  for (g = 0; g < f; ++g) {
    PetscFE  fe = (PetscFE) prob->disc[g];
    PetscInt Nc;

    ierr = PetscFEGetNumComponents(fe, &Nc);CHKERRQ(ierr);
    *off += Nc;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSGetComponentOffsets"
/*@
  PetscDSGetComponentOffsets - Returns the offset of each field on an evaluation point

  Not collective

  Input Parameter:
. prob - The PetscDS object

  Output Parameter:
. offsets - The offsets

  Level: beginner

.seealso: PetscDSGetBdFieldOffset(), PetscDSGetNumFields(), PetscDSCreate()
@*/
PetscErrorCode PetscDSGetComponentOffsets(PetscDS prob, PetscInt *offsets[])
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  PetscValidPointer(offsets, 2);
  *offsets = prob->off;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSGetComponentDerivativeOffsets"
/*@
  PetscDSGetComponentDerivativeOffsets - Returns the offset of each field derivative on an evaluation point

  Not collective

  Input Parameter:
. prob - The PetscDS object

  Output Parameter:
. offsets - The offsets

  Level: beginner

.seealso: PetscDSGetBdFieldOffset(), PetscDSGetNumFields(), PetscDSCreate()
@*/
PetscErrorCode PetscDSGetComponentDerivativeOffsets(PetscDS prob, PetscInt *offsets[])
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  PetscValidPointer(offsets, 2);
  *offsets = prob->offDer;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSGetComponentBdOffsets"
/*@
  PetscDSGetComponentBdOffsets - Returns the offset of each field on a boundary evaluation point

  Not collective

  Input Parameter:
. prob - The PetscDS object

  Output Parameter:
. offsets - The offsets

  Level: beginner

.seealso: PetscDSGetBdFieldOffset(), PetscDSGetNumFields(), PetscDSCreate()
@*/
PetscErrorCode PetscDSGetComponentBdOffsets(PetscDS prob, PetscInt *offsets[])
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  PetscValidPointer(offsets, 2);
  *offsets = prob->offBd;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSGetComponentBdDerivativeOffsets"
/*@
  PetscDSGetComponentBdDerivativeOffsets - Returns the offset of each field derivative on a boundary evaluation point

  Not collective

  Input Parameter:
. prob - The PetscDS object

  Output Parameter:
. offsets - The offsets

  Level: beginner

.seealso: PetscDSGetBdFieldOffset(), PetscDSGetNumFields(), PetscDSCreate()
@*/
PetscErrorCode PetscDSGetComponentBdDerivativeOffsets(PetscDS prob, PetscInt *offsets[])
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  PetscValidPointer(offsets, 2);
  *offsets = prob->offDerBd;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSGetTabulation"
/*@C
  PetscDSGetTabulation - Return the basis tabulation at quadrature points for the volume discretization

  Not collective

  Input Parameter:
. prob - The PetscDS object

  Output Parameters:
+ basis - The basis function tabulation at quadrature points
- basisDer - The basis function derivative tabulation at quadrature points

  Level: intermediate

.seealso: PetscDSGetBdTabulation(), PetscDSCreate()
@*/
PetscErrorCode PetscDSGetTabulation(PetscDS prob, PetscReal ***basis, PetscReal ***basisDer)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  ierr = PetscDSSetUp(prob);CHKERRQ(ierr);
  if (basis)    {PetscValidPointer(basis, 2);    *basis    = prob->basis;}
  if (basisDer) {PetscValidPointer(basisDer, 3); *basisDer = prob->basisDer;}
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSGetBdTabulation"
/*@C
  PetscDSGetBdTabulation - Return the basis tabulation at quadrature points for the boundary discretization

  Not collective

  Input Parameter:
. prob - The PetscDS object

  Output Parameters:
+ basis - The basis function tabulation at quadrature points
- basisDer - The basis function derivative tabulation at quadrature points

  Level: intermediate

.seealso: PetscDSGetTabulation(), PetscDSCreate()
@*/
PetscErrorCode PetscDSGetBdTabulation(PetscDS prob, PetscReal ***basis, PetscReal ***basisDer)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  ierr = PetscDSSetUp(prob);CHKERRQ(ierr);
  if (basis)    {PetscValidPointer(basis, 2);    *basis    = prob->basisBd;}
  if (basisDer) {PetscValidPointer(basisDer, 3); *basisDer = prob->basisDerBd;}
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSGetEvaluationArrays"
PetscErrorCode PetscDSGetEvaluationArrays(PetscDS prob, PetscScalar **u, PetscScalar **u_t, PetscScalar **u_x)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  ierr = PetscDSSetUp(prob);CHKERRQ(ierr);
  if (u)   {PetscValidPointer(u, 2);   *u   = prob->u;}
  if (u_t) {PetscValidPointer(u_t, 3); *u_t = prob->u_t;}
  if (u_x) {PetscValidPointer(u_x, 4); *u_x = prob->u_x;}
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSGetWeakFormArrays"
PetscErrorCode PetscDSGetWeakFormArrays(PetscDS prob, PetscScalar **f0, PetscScalar **f1, PetscScalar **g0, PetscScalar **g1, PetscScalar **g2, PetscScalar **g3)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  ierr = PetscDSSetUp(prob);CHKERRQ(ierr);
  if (f0) {PetscValidPointer(f0, 2); *f0 = prob->f0;}
  if (f1) {PetscValidPointer(f1, 3); *f1 = prob->f1;}
  if (g0) {PetscValidPointer(g0, 4); *g0 = prob->g0;}
  if (g1) {PetscValidPointer(g1, 5); *g1 = prob->g1;}
  if (g2) {PetscValidPointer(g2, 6); *g2 = prob->g2;}
  if (g3) {PetscValidPointer(g3, 7); *g3 = prob->g3;}
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSGetRefCoordArrays"
PetscErrorCode PetscDSGetRefCoordArrays(PetscDS prob, PetscReal **x, PetscScalar **refSpaceDer)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  ierr = PetscDSSetUp(prob);CHKERRQ(ierr);
  if (x)           {PetscValidPointer(x, 2);           *x           = prob->x;}
  if (refSpaceDer) {PetscValidPointer(refSpaceDer, 3); *refSpaceDer = prob->refSpaceDer;}
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSCopyEquations"
/*@
  PetscDSCopyEquations - Copy all pointwise function pointers to the new problem

  Not collective

  Input Parameter:
. prob - The PetscDS object

  Output Parameter:
. newprob - The PetscDS copy

  Level: intermediate

.seealso: PetscDSSetResidual(), PetscDSSetJacobian(), PetscDSSetRiemannSolver(), PetscDSSetBdResidual(), PetscDSSetBdJacobian(), PetscDSCreate()
@*/
PetscErrorCode PetscDSCopyEquations(PetscDS prob, PetscDS newprob)
{
  PetscInt       Nf, Ng, f, g;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  PetscValidHeaderSpecific(newprob, PETSCDS_CLASSID, 2);
  ierr = PetscDSGetNumFields(prob, &Nf);CHKERRQ(ierr);
  ierr = PetscDSGetNumFields(newprob, &Ng);CHKERRQ(ierr);
  if (Nf != Ng) SETERRQ2(PetscObjectComm((PetscObject) prob), PETSC_ERR_ARG_SIZ, "Number of fields must match %D != %D", Nf, Ng);CHKERRQ(ierr);
  for (f = 0; f < Nf; ++f) {
    PetscPointFunc   obj;
    PetscPointFunc   f0, f1;
    PetscPointJac    g0, g1, g2, g3;
    PetscBdPointFunc f0Bd, f1Bd;
    PetscBdPointJac  g0Bd, g1Bd, g2Bd, g3Bd;
    PetscRiemannFunc r;

    ierr = PetscDSGetObjective(prob, f, &obj);CHKERRQ(ierr);
    ierr = PetscDSGetResidual(prob, f, &f0, &f1);CHKERRQ(ierr);
    ierr = PetscDSGetBdResidual(prob, f, &f0Bd, &f1Bd);CHKERRQ(ierr);
    ierr = PetscDSGetRiemannSolver(prob, f, &r);CHKERRQ(ierr);
    ierr = PetscDSSetObjective(newprob, f, obj);CHKERRQ(ierr);
    ierr = PetscDSSetResidual(newprob, f, f0, f1);CHKERRQ(ierr);
    ierr = PetscDSSetBdResidual(newprob, f, f0Bd, f1Bd);CHKERRQ(ierr);
    ierr = PetscDSSetRiemannSolver(newprob, f, r);CHKERRQ(ierr);
    for (g = 0; g < Nf; ++g) {
      ierr = PetscDSGetJacobian(prob, f, g, &g0, &g1, &g2, &g3);CHKERRQ(ierr);
      ierr = PetscDSGetBdJacobian(prob, f, g, &g0Bd, &g1Bd, &g2Bd, &g3Bd);CHKERRQ(ierr);
      ierr = PetscDSSetJacobian(newprob, f, g, g0, g1, g2, g3);CHKERRQ(ierr);
      ierr = PetscDSSetBdJacobian(newprob, f, g, g0Bd, g1Bd, g2Bd, g3Bd);CHKERRQ(ierr);
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSDestroy_Basic"
static PetscErrorCode PetscDSDestroy_Basic(PetscDS prob)
{
  PetscFunctionBegin;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSInitialize_Basic"
static PetscErrorCode PetscDSInitialize_Basic(PetscDS prob)
{
  PetscFunctionBegin;
  prob->ops->setfromoptions = NULL;
  prob->ops->setup          = NULL;
  prob->ops->view           = NULL;
  prob->ops->destroy        = PetscDSDestroy_Basic;
  PetscFunctionReturn(0);
}

/*MC
  PETSCDSBASIC = "basic" - A discrete system with pointwise residual and boundary residual functions

  Level: intermediate

.seealso: PetscDSType, PetscDSCreate(), PetscDSSetType()
M*/

#undef __FUNCT__
#define __FUNCT__ "PetscDSCreate_Basic"
PETSC_EXTERN PetscErrorCode PetscDSCreate_Basic(PetscDS prob)
{
  PetscDS_Basic *b;
  PetscErrorCode      ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCSPACE_CLASSID, 1);
  ierr       = PetscNewLog(prob, &b);CHKERRQ(ierr);
  prob->data = b;

  ierr = PetscDSInitialize_Basic(prob);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}
