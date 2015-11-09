#include <petsc/private/dmmbimpl.h> /*I  "petscdmmoab.h"   I*/

#include <petscdmmoab.h>
#include <MBTagConventions.hpp>
#include <moab/Skinner.hpp>


/*MC
  DMMOAB = "moab" - A DM object that encapsulates an unstructured mesh described by the MOAB mesh database.
                    Direct access to the MOAB Interface and other mesh manipulation related objects are available
                    through public API. Ability to create global and local representation of Vecs containing all
                    unknowns in the interior and shared boundary via a transparent tag-data wrapper is provided
                    along with utility functions to traverse the mesh and assemble a discrete system via
                    field-based/blocked Vec(Get/Set) methods. Input from and output to different formats are
                    available.

  Reference: http://www.mcs.anl.gov/~fathom/moab-docs/html/contents.html

  Level: intermediate

.seealso: DMType, DMMoabCreate(), DMCreate(), DMSetType(), DMMoabCreateMoab()
M*/


#undef __FUNCT__
#define __FUNCT__ "DMMoabCreate"
/*@
  DMMoabCreate - Creates a DMMoab object, which encapsulates a moab instance

  Collective on MPI_Comm

  Input Parameter:
. comm - The communicator for the DMMoab object

  Output Parameter:
. dmb  - The DMMoab object

  Level: beginner

.keywords: DMMoab, create
@*/
PetscErrorCode DMMoabCreate(MPI_Comm comm, DM *dmb)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidPointer(dmb,2);
  ierr = DMCreate(comm, dmb);CHKERRQ(ierr);
  ierr = DMSetType(*dmb, DMMOAB);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMMoabCreateMoab"
/*@
  DMMoabCreate - Creates a DMMoab object, optionally from an instance and other data

  Collective on MPI_Comm

  Input Parameter:
. comm - The communicator for the DMMoab object
. mbiface - (ptr to) the MOAB Instance; if passed in NULL, MOAB instance is created inside PETSc, and destroyed
         along with the DMMoab
. pcomm - (ptr to) a ParallelComm; if NULL, creates one internally for the whole communicator
. ltog_tag - A tag to use to retrieve global id for an entity; if 0, will use GLOBAL_ID_TAG_NAME/tag
. range - If non-NULL, contains range of entities to which DOFs will be assigned

  Output Parameter:
. dmb  - The DMMoab object

  Level: intermediate

.keywords: DMMoab, create
@*/
PetscErrorCode DMMoabCreateMoab(MPI_Comm comm, moab::Interface *mbiface, moab::ParallelComm *pcomm, moab::Tag *ltog_tag, moab::Range *range, DM *dmb)
{
  PetscErrorCode ierr;
  moab::ErrorCode merr;
  moab::EntityHandle partnset;
  PetscInt rank, nprocs;
  DM             dmmb;
  DM_Moab        *dmmoab;

  PetscFunctionBegin;
  PetscValidPointer(dmb,6);

  ierr = DMMoabCreate(comm, &dmmb);CHKERRQ(ierr);
  dmmoab = (DM_Moab*)(dmmb)->data;

  if (!mbiface) {
    dmmoab->mbiface = new moab::Core();
    dmmoab->icreatedinstance = PETSC_TRUE;
  }
  else {
    dmmoab->mbiface = mbiface;
    dmmoab->icreatedinstance = PETSC_FALSE;
  }

  /* by default the fileset = root set. This set stores the hierarchy of entities belonging to current DM */
  dmmoab->fileset=0;

  if (!pcomm) {
    ierr = MPI_Comm_rank(comm, &rank);CHKERRQ(ierr);
    ierr = MPI_Comm_size(comm, &nprocs);CHKERRQ(ierr);

    /* Create root sets for each mesh.  Then pass these
       to the load_file functions to be populated. */
    merr = dmmoab->mbiface->create_meshset(moab::MESHSET_SET, partnset);MBERR("Creating partition set failed", merr);

    /* Create the parallel communicator object with the partition handle associated with MOAB */
    dmmoab->pcomm = moab::ParallelComm::get_pcomm(dmmoab->mbiface, partnset, &comm);
  }
  else {
    ierr = DMMoabSetParallelComm(dmmb, pcomm);CHKERRQ(ierr);
  }

  /* do the remaining initializations for DMMoab */
  dmmoab->bs = 1;
  dmmoab->numFields = 1;
  dmmoab->rw_dbglevel = 0;
  dmmoab->partition_by_rank = PETSC_FALSE;
  dmmoab->extra_read_options[0] = '\0';
  dmmoab->extra_write_options[0] = '\0';
  dmmoab->read_mode = READ_PART;
  dmmoab->write_mode = WRITE_PART;

  /* set global ID tag handle */
  if (ltog_tag && *ltog_tag) {
    ierr = DMMoabSetLocalToGlobalTag(dmmb, *ltog_tag);CHKERRQ(ierr);
  }
  else {
    merr = dmmoab->mbiface->tag_get_handle(GLOBAL_ID_TAG_NAME, dmmoab->ltog_tag);MBERRNM(merr);
    if (ltog_tag) *ltog_tag = dmmoab->ltog_tag;
  }

  merr = dmmoab->mbiface->tag_get_handle(MATERIAL_SET_TAG_NAME, dmmoab->material_tag);MBERRNM(merr);

  /* set the local range of entities (vertices) of interest */
  if (range) {
    ierr = DMMoabSetLocalVertices(dmmb, range);CHKERRQ(ierr);
  }
  *dmb=dmmb;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMMoabSetParallelComm"
/*@
  DMMoabSetParallelComm - Set the ParallelComm used with this DMMoab

  Collective on MPI_Comm

  Input Parameter:
. dm    - The DMMoab object being set
. pcomm - The ParallelComm being set on the DMMoab

  Level: beginner

.keywords: DMMoab, create
@*/
PetscErrorCode DMMoabSetParallelComm(DM dm,moab::ParallelComm *pcomm)
{
  DM_Moab        *dmmoab = (DM_Moab*)(dm)->data;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  PetscValidPointer(pcomm,2);
  dmmoab->pcomm = pcomm;
  dmmoab->mbiface = pcomm->get_moab();
  dmmoab->icreatedinstance = PETSC_FALSE;
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "DMMoabGetParallelComm"
/*@
  DMMoabGetParallelComm - Get the ParallelComm used with this DMMoab

  Collective on MPI_Comm

  Input Parameter:
. dm    - The DMMoab object being set

  Output Parameter:
. pcomm - The ParallelComm for the DMMoab

  Level: beginner

.keywords: DMMoab, create
@*/
PetscErrorCode DMMoabGetParallelComm(DM dm,moab::ParallelComm **pcomm)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  *pcomm = ((DM_Moab*)(dm)->data)->pcomm;
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "DMMoabSetInterface"
/*@
  DMMoabSetInterface - Set the MOAB instance used with this DMMoab

  Collective on MPI_Comm

  Input Parameter:
. dm      - The DMMoab object being set
. mbiface - The MOAB instance being set on this DMMoab

  Level: beginner

.keywords: DMMoab, create
@*/
PetscErrorCode DMMoabSetInterface(DM dm,moab::Interface *mbiface)
{
  DM_Moab        *dmmoab = (DM_Moab*)(dm)->data;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  PetscValidPointer(mbiface,2);
  dmmoab->pcomm = NULL;
  dmmoab->mbiface = mbiface;
  dmmoab->icreatedinstance = PETSC_FALSE;
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "DMMoabGetInterface"
/*@
  DMMoabGetInterface - Get the MOAB instance used with this DMMoab

  Collective on MPI_Comm

  Input Parameter:
. dm      - The DMMoab object being set

  Output Parameter:
. mbiface - The MOAB instance set on this DMMoab

  Level: beginner

.keywords: DMMoab, create
@*/
PetscErrorCode DMMoabGetInterface(DM dm,moab::Interface **mbiface)
{
  PetscErrorCode   ierr;
  static PetscBool cite = PETSC_FALSE;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  ierr = PetscCitationsRegister("@techreport{tautges_moab:_2004,\n  type = {{SAND2004-1592}},\n  title = {{MOAB:} A Mesh-Oriented Database},  institution = {Sandia National Laboratories},\n  author = {Tautges, T. J. and Meyers, R. and Merkley, K. and Stimpson, C. and Ernst, C.},\n  year = {2004},  note = {Report}\n}\n",&cite);CHKERRQ(ierr);
  *mbiface = ((DM_Moab*)dm->data)->mbiface;
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "DMMoabSetLocalVertices"
/*@
  DMMoabSetLocalVertices - Set the entities having DOFs on this DMMoab

  Collective on MPI_Comm

  Input Parameter:
. dm    - The DMMoab object being set
. range - The entities treated by this DMMoab

  Level: beginner

.keywords: DMMoab, create
@*/
PetscErrorCode DMMoabSetLocalVertices(DM dm,moab::Range *range)
{
  moab::ErrorCode merr;
  PetscErrorCode  ierr;
  moab::Range     tmpvtxs;
  DM_Moab        *dmmoab = (DM_Moab*)(dm)->data;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  dmmoab->vlocal->clear();
  dmmoab->vowned->clear();

  dmmoab->vlocal->insert(range->begin(), range->end());

  /* filter based on parallel status */
  merr = dmmoab->pcomm->filter_pstatus(*dmmoab->vlocal,PSTATUS_NOT_OWNED,PSTATUS_NOT,-1,dmmoab->vowned);MBERRNM(merr);

  /* filter all the non-owned and shared entities out of the list */
  tmpvtxs = moab::subtract(*dmmoab->vlocal, *dmmoab->vowned);
  merr = dmmoab->pcomm->filter_pstatus(tmpvtxs,PSTATUS_INTERFACE,PSTATUS_OR,-1,dmmoab->vghost);MBERRNM(merr);
  tmpvtxs = moab::subtract(tmpvtxs, *dmmoab->vghost);
  *dmmoab->vlocal = moab::subtract(*dmmoab->vlocal, tmpvtxs);

  /* compute and cache the sizes of local and ghosted entities */
  dmmoab->nloc = dmmoab->vowned->size();
  dmmoab->nghost = dmmoab->vghost->size();
  ierr = MPIU_Allreduce(&dmmoab->nloc, &dmmoab->n, 1, MPI_INTEGER, MPI_SUM, ((PetscObject)dm)->comm);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "DMMoabGetAllVertices"
/*@
  DMMoabGetAllVertices - Get the entities having DOFs on this DMMoab

  Collective on MPI_Comm

  Input Parameter:
. dm    - The DMMoab object being set

  Output Parameter:
. owned - The local vertex entities in this DMMoab = (owned+ghosted)

  Level: beginner

.keywords: DMMoab, create
@*/
PetscErrorCode DMMoabGetAllVertices(DM dm,moab::Range *local)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  if (local) *local = *((DM_Moab*)dm->data)->vlocal;
  PetscFunctionReturn(0);
}



#undef __FUNCT__
#define __FUNCT__ "DMMoabGetLocalVertices"
/*@
  DMMoabGetLocalVertices - Get the entities having DOFs on this DMMoab

  Collective on MPI_Comm

  Input Parameter:
. dm    - The DMMoab object being set

  Output Parameter:
. owned - The owned vertex entities in this DMMoab
. ghost - The ghosted entities (non-owned) stored locally in this partition

  Level: beginner

.keywords: DMMoab, create
@*/
PetscErrorCode DMMoabGetLocalVertices(DM dm,const moab::Range **owned,const moab::Range **ghost)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  if (owned) *owned = ((DM_Moab*)dm->data)->vowned;
  if (ghost) *ghost = ((DM_Moab*)dm->data)->vghost;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMMoabGetLocalElements"
/*@
  DMMoabGetLocalElements - Get the higher-dimensional entities that are locally owned

  Collective on MPI_Comm

  Input Parameter:
. dm    - The DMMoab object being set

  Output Parameter:
. range - The entities owned locally

  Level: beginner

.keywords: DMMoab, create
@*/
PetscErrorCode DMMoabGetLocalElements(DM dm,const moab::Range **range)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  if (range) *range = ((DM_Moab*)dm->data)->elocal;
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "DMMoabSetLocalElements"
/*@
  DMMoabSetLocalElements - Set the entities having DOFs on this DMMoab

  Collective on MPI_Comm

  Input Parameter:
. dm    - The DMMoab object being set
. range - The entities treated by this DMMoab

  Level: beginner

.keywords: DMMoab, create
@*/
PetscErrorCode DMMoabSetLocalElements(DM dm,moab::Range *range)
{
  moab::ErrorCode merr;
  PetscErrorCode  ierr;
  DM_Moab        *dmmoab = (DM_Moab*)(dm)->data;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  dmmoab->elocal->clear();
  dmmoab->eghost->clear();
  dmmoab->elocal->insert(range->begin(), range->end());
  merr = dmmoab->pcomm->filter_pstatus(*dmmoab->elocal,PSTATUS_NOT_OWNED,PSTATUS_NOT);MBERRNM(merr);
  *dmmoab->eghost = moab::subtract(*range, *dmmoab->elocal);
  dmmoab->neleloc=dmmoab->elocal->size();
  dmmoab->neleghost=dmmoab->eghost->size();
  ierr = MPIU_Allreduce(&dmmoab->nele, &dmmoab->neleloc, 1, MPI_INTEGER, MPI_SUM, ((PetscObject)dm)->comm);CHKERRQ(ierr);
  PetscInfo2(dm, "Created %D local and %D global elements.\n", dmmoab->neleloc, dmmoab->nele);
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "DMMoabSetLocalToGlobalTag"
/*@
  DMMoabSetLocalToGlobalTag - Set the tag used for local to global numbering

  Collective on MPI_Comm

  Input Parameter:
. dm      - The DMMoab object being set
. ltogtag - The MOAB tag used for local to global ids

  Level: beginner

.keywords: DMMoab, create
@*/
PetscErrorCode DMMoabSetLocalToGlobalTag(DM dm,moab::Tag ltogtag)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  ((DM_Moab*)dm->data)->ltog_tag = ltogtag;
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "DMMoabGetLocalToGlobalTag"
/*@
  DMMoabGetLocalToGlobalTag - Get the tag used for local to global numbering

  Collective on MPI_Comm

  Input Parameter:
. dm      - The DMMoab object being set

  Output Parameter:
. ltogtag - The MOAB tag used for local to global ids

  Level: beginner

.keywords: DMMoab, create
@*/
PetscErrorCode DMMoabGetLocalToGlobalTag(DM dm,moab::Tag *ltog_tag)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  *ltog_tag = ((DM_Moab*)dm->data)->ltog_tag;
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "DMMoabSetBlockSize"
/*@
  DMMoabSetBlockSize - Set the block size used with this DMMoab

  Collective on MPI_Comm

  Input Parameter:
. dm - The DMMoab object being set
. bs - The block size used with this DMMoab

  Level: beginner

.keywords: DMMoab, create
@*/
PetscErrorCode DMMoabSetBlockSize(DM dm,PetscInt bs)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  ((DM_Moab*)dm->data)->bs = bs;
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "DMMoabGetBlockSize"
/*@
  DMMoabGetBlockSize - Get the block size used with this DMMoab

  Collective on MPI_Comm

  Input Parameter:
. dm - The DMMoab object being set

  Output Parameter:
. bs - The block size used with this DMMoab

  Level: beginner

.keywords: DMMoab, create
@*/
PetscErrorCode DMMoabGetBlockSize(DM dm,PetscInt *bs)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  *bs = ((DM_Moab*)dm->data)->bs;
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "DMMoabGetSize"
/*@
  DMMoabGetSize - Get the global vertex size used with this DMMoab

  Collective on DM

  Input Parameter:
. dm - The DMMoab object being set

  Output Parameter:
. neg - The number of global elements in the DMMoab instance
. nvg - The number of global vertices in the DMMoab instance

  Level: beginner

.keywords: DMMoab, create
@*/
PetscErrorCode DMMoabGetSize(DM dm,PetscInt *neg,PetscInt *nvg)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  if(neg) *neg = ((DM_Moab*)dm->data)->nele;
  if(nvg) *nvg = ((DM_Moab*)dm->data)->n;
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "DMMoabGetLocalSize"
/*@
  DMMoabGetLocalSize - Get the local and ghosted vertex size used with this DMMoab

  Collective on DM

  Input Parameter:
. dm - The DMMoab object being set

  Output Parameter:
+ nel - The number of owned elements in this processor
. neg - The number of ghosted elements in this processor
. nvl - The number of owned vertices in this processor
. nvg - The number of ghosted vertices in this processor

  Level: beginner

.keywords: DMMoab, create
@*/
PetscErrorCode DMMoabGetLocalSize(DM dm,PetscInt *nel,PetscInt *neg,PetscInt *nvl,PetscInt *nvg)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  if(nel) *nel = ((DM_Moab*)dm->data)->neleloc;
  if(neg) *neg = ((DM_Moab*)dm->data)->neleghost;
  if(nvl) *nvl = ((DM_Moab*)dm->data)->nloc;
  if(nvg) *nvg = ((DM_Moab*)dm->data)->nghost;
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "DMMoabGetOffset"
/*@
  DMMoabGetOffset - Get the local offset for the global vector

  Collective on MPI_Comm

  Input Parameter:
. dm - The DMMoab object being set

  Output Parameter:
. offset - The local offset for the global vector

  Level: beginner

.keywords: DMMoab, create
@*/
PetscErrorCode DMMoabGetOffset(DM dm,PetscInt *offset)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  *offset = ((DM_Moab*)dm->data)->vstart;
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "DMMoabGetDimension"
/*@
  DMMoabGetDimension - Get the dimension of the DM Mesh

  Collective on MPI_Comm

  Input Parameter:
. dm - The DMMoab object

  Output Parameter:
. dim - The dimension of DM

  Level: beginner

.keywords: DMMoab, create
@*/
PetscErrorCode DMMoabGetDimension(DM dm,PetscInt *dim)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  *dim = ((DM_Moab*)dm->data)->dim;
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "DMMoabGetMaterialBlock"
/*@
  DMMoabGetMaterialBlock - Get the material ID corresponding to the current entity of the DM Mesh

  Collective on MPI_Comm

  Input Parameter:
. dm - The DMMoab object
. ehandle - The element entity handle

  Output Parameter:
. mat - The material ID for the current entity

  Level: beginner

.keywords: DMMoab, create
@*/
PetscErrorCode DMMoabGetMaterialBlock(DM dm,const moab::EntityHandle ehandle, PetscInt *mat)
{
  DM_Moab         *dmmoab;
  moab::ErrorCode merr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  if (*mat) {
    dmmoab = (DM_Moab*)(dm)->data;
    merr=dmmoab->mbiface->tag_get_data(dmmoab->material_tag, &ehandle, 1, mat);MBERRNM(merr);
  }
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "DMMoabGetVertexCoordinates"
/*@
  DMMoabGetVertexCoordinates - Get the coordinates corresponding to the requested vertex entities

  Collective on MPI_Comm

  Input Parameter:
. dm - The DMMoab object
. nconn - Number of entities whose coordinates are needed
. conn - The vertex entity handles

  Output Parameter:
. vpos - The coordinates of the requested vertex entities

  Level: beginner

.seealso: DMMoabGetVertexConnectivity()
@*/
PetscErrorCode DMMoabGetVertexCoordinates(DM dm,PetscInt nconn,const moab::EntityHandle *conn,PetscReal *vpos)
{
  DM_Moab         *dmmoab;
  PetscErrorCode  ierr;
  moab::ErrorCode merr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  PetscValidPointer(conn,3);
  dmmoab = (DM_Moab*)(dm)->data;

  if (!vpos) {
    ierr = PetscMalloc1(nconn*3, &vpos);CHKERRQ(ierr);
  }

  /* Get connectivity information in MOAB canonical ordering */
  merr = dmmoab->mbiface->get_coords(conn, nconn, vpos);MBERRNM(merr);
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "DMMoabGetVertexConnectivity"
/*@
  DMMoabGetVertexConnectivity - Get the vertex adjacency for the given entity

  Collective on MPI_Comm

  Input Parameter:
. dm - The DMMoab object
. vhandle - Vertex entity handle

  Output Parameter:
. nconn - Number of entities whose coordinates are needed
. conn - The vertex entity handles

  Level: beginner

.seealso: DMMoabGetVertexCoordinates(), DMMoabRestoreVertexConnectivity()
@*/
PetscErrorCode DMMoabGetVertexConnectivity(DM dm,moab::EntityHandle vhandle,PetscInt* nconn, moab::EntityHandle **conn)
{
  DM_Moab        *dmmoab;
  std::vector<moab::EntityHandle> adj_entities,connect;
  PetscErrorCode  ierr;
  moab::ErrorCode merr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  PetscValidPointer(conn,4);
  dmmoab = (DM_Moab*)(dm)->data;

  /* Get connectivity information in MOAB canonical ordering */
  merr = dmmoab->mbiface->get_adjacencies(&vhandle, 1, 1, true, adj_entities, moab::Interface::UNION);MBERRNM(merr);
  merr = dmmoab->mbiface->get_connectivity(&adj_entities[0],adj_entities.size(),connect);MBERRNM(merr);

  if (conn) {
    ierr = PetscMalloc(sizeof(moab::EntityHandle)*connect.size(), conn);CHKERRQ(ierr);
    ierr = PetscMemcpy(*conn, &connect[0], sizeof(moab::EntityHandle)*connect.size());CHKERRQ(ierr);
  }
  if (nconn) *nconn=connect.size();
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "DMMoabRestoreVertexConnectivity"
/*@
  DMMoabRestoreVertexConnectivity - Restore the vertex connectivity for the given entity

  Collective on MPI_Comm

  Input Parameter:
. dm - The DMMoab object
. vhandle - Vertex entity handle
. nconn - Number of entities whose coordinates are needed
. conn - The vertex entity handles

  Level: beginner

.seealso: DMMoabGetVertexCoordinates(), DMMoabGetVertexConnectivity()
@*/
PetscErrorCode DMMoabRestoreVertexConnectivity(DM dm,moab::EntityHandle ehandle,PetscInt* nconn, moab::EntityHandle **conn)
{
  PetscErrorCode  ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  PetscValidPointer(conn,4);

  if (conn) {
    ierr = PetscFree(*conn);CHKERRQ(ierr);
  }
  if (nconn) *nconn=0;
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "DMMoabGetElementConnectivity"
/*@
  DMMoabGetElementConnectivity - Get the vertex adjacency for the given entity

  Collective on MPI_Comm

  Input Parameter:
. dm - The DMMoab object
. ehandle - Vertex entity handle

  Output Parameter:
. nconn - Number of entities whose coordinates are needed
. conn - The vertex entity handles

  Level: beginner

.seealso: DMMoabGetVertexCoordinates(), DMMoabGetVertexConnectivity(), DMMoabRestoreVertexConnectivity()
@*/
PetscErrorCode DMMoabGetElementConnectivity(DM dm,moab::EntityHandle ehandle,PetscInt* nconn,const moab::EntityHandle **conn)
{
  DM_Moab        *dmmoab;
  const moab::EntityHandle *connect;
  moab::ErrorCode merr;
  PetscInt nnodes;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  PetscValidPointer(conn,4);
  dmmoab = (DM_Moab*)(dm)->data;

  /* Get connectivity information in MOAB canonical ordering */
  merr = dmmoab->mbiface->get_connectivity(ehandle, connect, nnodes);MBERRNM(merr);
  if (conn) *conn=connect;
  if (nconn) *nconn=nnodes;
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "DMMoabIsEntityOnBoundary"
/*@
  DMMoabIsEntityOnBoundary - Check whether a given entity is on the boundary (vertex, edge, face, element)

  Collective on MPI_Comm

  Input Parameter:
. dm - The DMMoab object
. ent - Entity handle

  Output Parameter:
. ent_on_boundary - PETSC_TRUE if entity on boundary; PETSC_FALSE otherwise

  Level: beginner

.seealso: DMMoabCheckBoundaryVertices()
@*/
PetscErrorCode DMMoabIsEntityOnBoundary(DM dm,const moab::EntityHandle ent,PetscBool* ent_on_boundary)
{
  moab::EntityType etype;
  DM_Moab         *dmmoab;
  PetscInt         edim;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  PetscValidPointer(ent_on_boundary,3);
  dmmoab = (DM_Moab*)(dm)->data;

  /* get the entity type and handle accordingly */
  etype=dmmoab->mbiface->type_from_handle(ent);
  if(etype >= moab::MBPOLYHEDRON) SETERRQ1(PETSC_COMM_WORLD, PETSC_ERR_ARG_OUTOFRANGE, "Entity type on the boundary skin is invalid. EntityType = %D\n",etype);

  /* get the entity dimension */
  edim=dmmoab->mbiface->dimension_from_handle(ent);

  *ent_on_boundary=PETSC_FALSE;
  if(etype == moab::MBVERTEX && edim == 0) {
    if (dmmoab->bndyvtx->index(ent) >= 0) *ent_on_boundary=PETSC_TRUE;
  }
  else {
    if (edim == dmmoab->dim) {  /* check the higher-dimensional elements first */
      if (dmmoab->bndyelems->index(ent) >= 0) *ent_on_boundary=PETSC_TRUE;
    }
    else {                      /* next check the lower-dimensional faces */
      if (dmmoab->bndyfaces->index(ent) >= 0) *ent_on_boundary=PETSC_TRUE;
    }
  }
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "DMMoabCheckBoundaryVertices"
/*@
  DMMoabIsEntityOnBoundary - Check whether a given entity is on the boundary (vertex, edge, face, element)

  Input Parameter:
. dm - The DMMoab object
. nconn - Number of handles
. cnt - Array of entity handles

  Output Parameter:
. isbdvtx - Array of boundary markers - PETSC_TRUE if entity on boundary; PETSC_FALSE otherwise

  Level: beginner

.seealso: DMMoabIsEntityOnBoundary()
@*/
PetscErrorCode DMMoabCheckBoundaryVertices(DM dm,PetscInt nconn,const moab::EntityHandle *cnt,PetscBool* isbdvtx)
{
  DM_Moab        *dmmoab;
  PetscInt       i;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  PetscValidPointer(cnt,3);
  PetscValidPointer(isbdvtx,4);
  dmmoab = (DM_Moab*)(dm)->data;

  for (i=0; i < nconn; ++i) {
    isbdvtx[i]=(dmmoab->bndyvtx->index(cnt[i]) >= 0 ? PETSC_TRUE:PETSC_FALSE);
  }
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "DMMoabGetBoundaryMarkers"
/*@
  DMMoabGetBoundaryMarkers - Return references to the vertices, faces, elements on the boundary

  Input Parameter:
. dm - The DMMoab object

  Output Parameter:
. bdvtx - Boundary vertices
. bdelems - Boundary elements
. bdfaces - Boundary faces

  Level: beginner

.seealso: DMMoabCheckBoundaryVertices(), DMMoabIsEntityOnBoundary()
@*/
PetscErrorCode DMMoabGetBoundaryMarkers(DM dm,const moab::Range **bdvtx,const moab::Range** bdelems,const moab::Range** bdfaces)
{
  DM_Moab        *dmmoab;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  dmmoab = (DM_Moab*)(dm)->data;

  if (bdvtx)  *bdvtx = dmmoab->bndyvtx;
  if (bdfaces)  *bdfaces = dmmoab->bndyfaces;
  if (bdelems)  *bdfaces = dmmoab->bndyelems;
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "DMDestroy_Moab"
PETSC_EXTERN PetscErrorCode DMDestroy_Moab(DM dm)
{
  PetscErrorCode ierr;
  PetscInt       i;
  DM_Moab        *dmmoab = (DM_Moab*)dm->data;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  if (dmmoab->icreatedinstance) {
    delete dmmoab->mbiface;
  }
  dmmoab->mbiface = NULL;
  dmmoab->pcomm = NULL;
  delete dmmoab->vlocal;
  delete dmmoab->vowned;
  delete dmmoab->vghost;
  delete dmmoab->elocal;
  delete dmmoab->eghost;
  delete dmmoab->bndyvtx;
  delete dmmoab->bndyfaces;
  delete dmmoab->bndyelems;

  ierr = PetscFree(dmmoab->gsindices);CHKERRQ(ierr);
  ierr = PetscFree2(dmmoab->gidmap,dmmoab->lidmap);CHKERRQ(ierr);
  ierr = PetscFree2(dmmoab->lgmap,dmmoab->llmap);CHKERRQ(ierr);
  ierr = PetscFree(dmmoab->dfill);CHKERRQ(ierr);
  ierr = PetscFree(dmmoab->ofill);CHKERRQ(ierr);
  if (dmmoab->fieldNames) {
    for(i=0; i<dmmoab->numFields; i++) {
      ierr = PetscFree(dmmoab->fieldNames[i]);CHKERRQ(ierr);
    }
    ierr = PetscFree(dmmoab->fieldNames);CHKERRQ(ierr);
  }
  ierr = VecScatterDestroy(&dmmoab->ltog_sendrecv);CHKERRQ(ierr);
  ierr = ISLocalToGlobalMappingDestroy(&dmmoab->ltog_map);CHKERRQ(ierr);
  ierr = PetscFree(dm->data);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "DMSetFromOptions_Moab"
PETSC_EXTERN PetscErrorCode DMSetFromOptions_Moab(PetscOptionItems *PetscOptionsObject,DM dm)
{
  PetscErrorCode ierr;
  DM_Moab        *dmmoab = (DM_Moab*)dm->data;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  ierr = PetscOptionsHead(PetscOptionsObject,"DMMoab Options");CHKERRQ(ierr);
  ierr  = PetscOptionsInt("-dm_moab_rw_dbg", "The verbosity level for reading and writing MOAB meshes", "DMView", dmmoab->rw_dbglevel, &dmmoab->rw_dbglevel, NULL);CHKERRQ(ierr);
  ierr  = PetscOptionsBool("-dm_moab_partiton_by_rank", "Use partition by rank when reading MOAB meshes from file", "DMView", dmmoab->partition_by_rank, &dmmoab->partition_by_rank, NULL);CHKERRQ(ierr);
  /* TODO: typically, the read options are needed before a DM is completely created and available in which case, the options wont be available ?? */
  ierr  = PetscOptionsString("-dm_moab_read_opts", "Extra options to enable MOAB reader to load DM from file", "DMView", dmmoab->extra_read_options, dmmoab->extra_read_options, PETSC_MAX_PATH_LEN, NULL);CHKERRQ(ierr);
  ierr  = PetscOptionsString("-dm_moab_write_opts", "Extra options to enable MOAB writer to serialize DM to file", "DMView", dmmoab->extra_write_options, dmmoab->extra_write_options, PETSC_MAX_PATH_LEN, NULL);CHKERRQ(ierr);
  ierr  = PetscOptionsEnum("-dm_moab_read_mode", "MOAB parallel read mode", "DMView", MoabReadModes, (PetscEnum)dmmoab->read_mode, (PetscEnum*)&dmmoab->read_mode, NULL);CHKERRQ(ierr);
  ierr  = PetscOptionsEnum("-dm_moab_write_mode", "MOAB parallel write mode", "DMView", MoabWriteModes, (PetscEnum)dmmoab->write_mode, (PetscEnum*)&dmmoab->write_mode, NULL);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "DMSetUp_Moab"
PETSC_EXTERN PetscErrorCode DMSetUp_Moab(DM dm)
{
  PetscErrorCode          ierr;
  moab::ErrorCode         merr;
  Vec                     local, global;
  IS                      from,to;
  moab::Range::iterator   iter;
  PetscInt                i,j,f,bs,gmin,lmin,lmax,vent,totsize;
  DM_Moab                *dmmoab = (DM_Moab*)dm->data;
  moab::Range             adjs;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  /* Get the local and shared vertices and cache it */
  if (dmmoab->mbiface == PETSC_NULL || dmmoab->pcomm == PETSC_NULL) SETERRQ(PETSC_COMM_WORLD, PETSC_ERR_ORDER, "Set the MOAB Interface and ParallelComm objects before calling SetUp.");

  /* Get the entities recursively in the current part of the mesh, if user did not set the local vertices explicitly */
  if (dmmoab->vlocal->empty())
  {
    merr = dmmoab->mbiface->get_entities_by_type(dmmoab->fileset,moab::MBVERTEX,*dmmoab->vlocal,true);MBERRNM(merr);

    /* filter based on parallel status */
    merr = dmmoab->pcomm->filter_pstatus(*dmmoab->vlocal,PSTATUS_NOT_OWNED,PSTATUS_NOT,-1,dmmoab->vowned);MBERRNM(merr);

    /* filter all the non-owned and shared entities out of the list */
    adjs = moab::subtract(*dmmoab->vlocal, *dmmoab->vowned);
    merr = dmmoab->pcomm->filter_pstatus(adjs,PSTATUS_INTERFACE,PSTATUS_OR,-1,dmmoab->vghost);MBERRNM(merr);
    adjs = moab::subtract(adjs, *dmmoab->vghost);
    *dmmoab->vlocal = moab::subtract(*dmmoab->vlocal, adjs);

    /* compute and cache the sizes of local and ghosted entities */
    dmmoab->nloc = dmmoab->vowned->size();
    dmmoab->nghost = dmmoab->vghost->size();
    ierr = MPIU_Allreduce(&dmmoab->nloc, &dmmoab->n, 1, MPI_INTEGER, MPI_SUM, ((PetscObject)dm)->comm);CHKERRQ(ierr);
  }

  {
    /* get the information about the local elements in the mesh */
    dmmoab->eghost->clear();

    /* first decipher the leading dimension */
    for (i=3;i>0;i--) {
      dmmoab->elocal->clear();
      merr = dmmoab->mbiface->get_entities_by_dimension(dmmoab->fileset, i, *dmmoab->elocal, true);CHKERRQ(merr);

      /* store the current mesh dimension */
      if (dmmoab->elocal->size()) {
        dmmoab->dim=i;
        break;
      }
    }

    /* filter the ghosted and owned element list */
    *dmmoab->eghost = *dmmoab->elocal;
    merr = dmmoab->pcomm->filter_pstatus(*dmmoab->elocal,PSTATUS_NOT_OWNED,PSTATUS_NOT);MBERRNM(merr);
    *dmmoab->eghost = moab::subtract(*dmmoab->eghost, *dmmoab->elocal);

    dmmoab->neleloc = dmmoab->elocal->size();
    dmmoab->neleghost = dmmoab->eghost->size();
    ierr = MPIU_Allreduce(&dmmoab->neleloc, &dmmoab->nele, 1, MPI_INTEGER, MPI_SUM, ((PetscObject)dm)->comm);CHKERRQ(ierr);
  }

  bs = dmmoab->bs;
  if (!dmmoab->ltog_tag) {
    /* Get the global ID tag. The global ID tag is applied to each
       vertex. It acts as an global identifier which MOAB uses to
       assemble the individual pieces of the mesh */
    merr = dmmoab->mbiface->tag_get_handle(GLOBAL_ID_TAG_NAME, dmmoab->ltog_tag);MBERRNM(merr);
  }

  totsize=dmmoab->vlocal->size();
  if (totsize != dmmoab->nloc+dmmoab->nghost) SETERRQ2(PETSC_COMM_WORLD, PETSC_ERR_ARG_OUTOFRANGE, "Mismatch between local and owned+ghost vertices. %D != %D.",totsize,dmmoab->nloc+dmmoab->nghost);
  ierr = PetscMalloc1(totsize,&dmmoab->gsindices);CHKERRQ(ierr);
  {
    /* first get the local indices */
    merr = dmmoab->mbiface->tag_get_data(dmmoab->ltog_tag,*dmmoab->vowned,&dmmoab->gsindices[0]);MBERRNM(merr);
    /* next get the ghosted indices */
    if (dmmoab->nghost) {
      merr = dmmoab->mbiface->tag_get_data(dmmoab->ltog_tag,*dmmoab->vghost,&dmmoab->gsindices[dmmoab->nloc]);MBERRNM(merr);
    }

    /* find out the local and global minima of GLOBAL_ID */
    lmin=lmax=dmmoab->gsindices[0];
    for (i=0; i<totsize; ++i) {
      if(lmin>dmmoab->gsindices[i]) lmin=dmmoab->gsindices[i];
      if(lmax<dmmoab->gsindices[i]) lmax=dmmoab->gsindices[i];
    }

    ierr = MPIU_Allreduce(&lmin, &gmin, 1, MPI_INT, MPI_MIN, ((PetscObject)dm)->comm);CHKERRQ(ierr);

    /* set the GID map */
    for (i=0; i<totsize; ++i) {
      dmmoab->gsindices[i]-=gmin;   /* zero based index needed for IS */
    }
    lmin-=gmin;
    lmax-=gmin;

    PetscInfo3(NULL, "GLOBAL_ID: Local minima - %D, Local maxima - %D, Global minima - %D.\n", lmin, lmax, gmin);
  }
  if(!(dmmoab->bs==dmmoab->numFields || dmmoab->bs == 1)) SETERRQ3(PETSC_COMM_SELF, PETSC_ERR_ARG_WRONG, "Mismatch between block size and number of component fields. %D != 1 OR %D != %D.",dmmoab->bs,dmmoab->bs,dmmoab->numFields);

  {
    i=(PetscInt)dmmoab->vlocal->back()+1;
    //i=(PetscInt)(dmmoab->vlocal->back()-dmmoab->vlocal->front())+1;
    j=totsize*dmmoab->numFields;
    ierr = PetscMalloc2(i,&dmmoab->gidmap,i,&dmmoab->lidmap);CHKERRQ(ierr);
    ierr = PetscMalloc2(j,&dmmoab->lgmap,j,&dmmoab->llmap);CHKERRQ(ierr);

    i=j=0;
    /* set the owned vertex data first */
    for(moab::Range::iterator iter = dmmoab->vowned->begin(); iter != dmmoab->vowned->end(); iter++,i++) {
      vent=(PetscInt)(*iter);
      dmmoab->gidmap[vent]=dmmoab->gsindices[i];
      dmmoab->lidmap[vent]=i;
      if (bs > 1) {
        for (f=0;f<dmmoab->numFields;f++,j++) {
          dmmoab->lgmap[j]=dmmoab->gsindices[i]*dmmoab->numFields+f;
          dmmoab->llmap[j]=i*dmmoab->numFields+f;
        }
      }
      else {
        for (f=0;f<dmmoab->numFields;f++,j++) {
          dmmoab->lgmap[j]=totsize*f+dmmoab->gsindices[i];
          dmmoab->llmap[j]=totsize*f+i;
        }
      }
    }
    /* next arrange all the ghosted data information */
    for(moab::Range::iterator iter = dmmoab->vghost->begin(); iter != dmmoab->vghost->end(); iter++,i++) {
      vent=(PetscInt)(*iter);
      dmmoab->gidmap[vent]=dmmoab->gsindices[i];
      dmmoab->lidmap[vent]=i;
      if (bs > 1) {
        for (f=0;f<dmmoab->numFields;f++,j++) {
          dmmoab->lgmap[j]=dmmoab->gsindices[i]*dmmoab->numFields+f;
          dmmoab->llmap[j]=i*dmmoab->numFields+f;
        }
      }
      else {
        for (f=0;f<dmmoab->numFields;f++,j++) {
          dmmoab->lgmap[j]=totsize*f+dmmoab->gsindices[i];
          dmmoab->llmap[j]=totsize*f+i;
        }
      }
    }

    /* We need to create the Global to Local Vector Scatter Contexts
       1) First create a local and global vector
       2) Create a local and global IS
       3) Create VecScatter and LtoGMapping objects
       4) Cleanup the IS and Vec objects
    */
    ierr = DMCreateGlobalVector(dm, &global);CHKERRQ(ierr);
    ierr = DMCreateLocalVector(dm, &local);CHKERRQ(ierr);

    ierr = VecGetOwnershipRange(global, &dmmoab->vstart, &dmmoab->vend);CHKERRQ(ierr);
    PetscInfo3(NULL, "Total-size = %D\t Owned = %D, Ghosted = %D.\n", totsize, dmmoab->nloc, dmmoab->nghost);

    /* global to local must retrieve ghost points */
    ierr = ISCreateStride(((PetscObject)dm)->comm,dmmoab->nloc*dmmoab->numFields,dmmoab->vstart,1,&from);CHKERRQ(ierr);
    ierr = ISSetBlockSize(from,bs);CHKERRQ(ierr);

    ierr = ISCreateGeneral(((PetscObject)dm)->comm,dmmoab->nloc*dmmoab->numFields,&dmmoab->lgmap[0],PETSC_COPY_VALUES,&to);CHKERRQ(ierr);
    ierr = ISSetBlockSize(to,bs);CHKERRQ(ierr);

    if (!dmmoab->ltog_map) {
      /* create to the local to global mapping for vectors in order to use VecSetValuesLocal */
      ierr = ISLocalToGlobalMappingCreate(((PetscObject)dm)->comm,dmmoab->bs,totsize*dmmoab->numFields,dmmoab->lgmap,
                                          PETSC_COPY_VALUES,&dmmoab->ltog_map);CHKERRQ(ierr);
    }

    /* now create the scatter object from local to global vector */
    ierr = VecScatterCreate(local,from,global,to,&dmmoab->ltog_sendrecv);CHKERRQ(ierr);

    /* clean up IS, Vec */
    ierr = ISDestroy(&from);CHKERRQ(ierr);
    ierr = ISDestroy(&to);CHKERRQ(ierr);
    ierr = VecDestroy(&local);CHKERRQ(ierr);
    ierr = VecDestroy(&global);CHKERRQ(ierr);
  }

  /* skin the boundary and store nodes */
  {
    /* get the skin vertices of boundary faces for the current partition and then filter
       the local, boundary faces, vertices and elements alone via PSTATUS flags;
       this should not give us any ghosted boundary, but if user needs such a functionality
       it would be easy to add it based on the find_skin query below */
    moab::Skinner skinner(dmmoab->mbiface);

    dmmoab->bndyvtx = new moab::Range();
    dmmoab->bndyfaces = new moab::Range();
    dmmoab->bndyelems = new moab::Range();

    /* get the entities on the skin - only the faces */
    merr = skinner.find_skin(dmmoab->fileset, *dmmoab->elocal, false, *dmmoab->bndyfaces, NULL, false, true, false);MBERRNM(merr); // 'false' param indicates we want faces back, not vertices

    /* filter all the non-owned and shared entities out of the list */
    merr = dmmoab->pcomm->filter_pstatus(*dmmoab->bndyfaces,PSTATUS_NOT_OWNED,PSTATUS_NOT);MBERRNM(merr);
    merr = dmmoab->pcomm->filter_pstatus(*dmmoab->bndyfaces,PSTATUS_SHARED,PSTATUS_NOT);MBERRNM(merr);

    /* get all the nodes via connectivity and the parent elements via adjacency information */
    merr = dmmoab->mbiface->get_connectivity(*dmmoab->bndyfaces, *dmmoab->bndyvtx, false);MBERRNM(ierr);
    merr = dmmoab->mbiface->get_adjacencies(*dmmoab->bndyfaces, dmmoab->dim, false, *dmmoab->bndyelems, moab::Interface::UNION);MBERRNM(ierr);
    PetscInfo3(NULL, "Found %D boundary vertices, %D boundary faces and %D boundary elements.\n", dmmoab->bndyvtx->size(), dmmoab->bndyvtx->size(), dmmoab->bndyelems->size());
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMCreate_Moab"
PETSC_EXTERN PetscErrorCode DMCreate_Moab(DM dm)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  ierr = PetscNewLog(dm,(DM_Moab**)&dm->data);CHKERRQ(ierr);

  ((DM_Moab*)dm->data)->bs = 1;
  ((DM_Moab*)dm->data)->numFields = 1;
  ((DM_Moab*)dm->data)->n = 0;
  ((DM_Moab*)dm->data)->nloc = 0;
  ((DM_Moab*)dm->data)->nghost = 0;
  ((DM_Moab*)dm->data)->nele = 0;
  ((DM_Moab*)dm->data)->neleloc = 0;
  ((DM_Moab*)dm->data)->neleghost = 0;
  ((DM_Moab*)dm->data)->ltog_map = PETSC_NULL;
  ((DM_Moab*)dm->data)->ltog_sendrecv = PETSC_NULL;

  ((DM_Moab*)dm->data)->vlocal = new moab::Range();
  ((DM_Moab*)dm->data)->vowned = new moab::Range();
  ((DM_Moab*)dm->data)->vghost = new moab::Range();
  ((DM_Moab*)dm->data)->elocal = new moab::Range();
  ((DM_Moab*)dm->data)->eghost = new moab::Range();

  dm->ops->createglobalvector       = DMCreateGlobalVector_Moab;
  dm->ops->createlocalvector        = DMCreateLocalVector_Moab;
  dm->ops->creatematrix             = DMCreateMatrix_Moab;
  dm->ops->setup                    = DMSetUp_Moab;
  dm->ops->destroy                  = DMDestroy_Moab;
  dm->ops->setfromoptions           = DMSetFromOptions_Moab;
  dm->ops->globaltolocalbegin       = DMGlobalToLocalBegin_Moab;
  dm->ops->globaltolocalend         = DMGlobalToLocalEnd_Moab;
  dm->ops->localtoglobalbegin       = DMLocalToGlobalBegin_Moab;
  dm->ops->localtoglobalend         = DMLocalToGlobalEnd_Moab;
  PetscFunctionReturn(0);
}

