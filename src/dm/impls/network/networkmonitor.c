#include <petscdmnetwork.h> /*I  "petscdmnetwork.h"  I*/
#include <petscdraw.h>

#undef __FUNCT__
#define __FUNCT__ "DMNetworkMonitorCreate"
/*@
  DMNetworkMonitorCreate - Creates a network monitor context

  Collective on MPI_Comm

  Input Parameters:
. network - network to monitor

  Output Parameters:
. Monitorptr - Location to put network monitor context

  Level: intermediate

.seealso: DMNetworkMonitorDestroy(), DMNetworkMonitorAdd()
@*/
PetscErrorCode DMNetworkMonitorCreate(DM network,DMNetworkMonitor *monitorptr)
{
  PetscErrorCode   ierr;
  DMNetworkMonitor monitor;
  MPI_Comm         comm;
  PetscMPIInt      size;

  PetscFunctionBegin;
  ierr = PetscObjectGetComm((PetscObject)network,&comm);CHKERRQ(ierr);
  ierr = MPI_Comm_size(comm, &size);CHKERRQ(ierr);
  if (size > 1) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_SUP,"Parallel DMNetworkMonitor is not supported yet");

  ierr = PetscMalloc1(1,&monitor);CHKERRQ(ierr);
  monitor->comm      = comm;
  monitor->network   = network;
  monitor->firstnode = PETSC_NULL;

  *monitorptr = monitor;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMNetworkMonitorDestroy"
/*@
  DMNetworkMonitorDestroy - Destroys a network monitor and all associated viewers

  Collective on DMNetworkMonitor

  Input Parameters:
. monitor - monitor to destroy

  Level: intermediate

.seealso: DMNetworkMonitorCreate, DMNetworkMonitorAdd
@*/
PetscErrorCode DMNetworkMonitorDestroy(DMNetworkMonitor *monitor)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  while ((*monitor)->firstnode) {
    ierr = DMNetworkMonitorPop(*monitor);CHKERRQ(ierr);
  }

  ierr = PetscFree(*monitor);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMNetworkMonitorPop"
/*@
  DMNetworkMonitorPop - Removes the most recently added viewer

  Collective on DMNetworkMonitor

  Input Parameters:
. monitor - the monitor

  Level: intermediate

.seealso: DMNetworkMonitorCreate(), DMNetworkMonitorDestroy()
@*/
PetscErrorCode DMNetworkMonitorPop(DMNetworkMonitor monitor)
{
  PetscErrorCode       ierr;
  DMNetworkMonitorList node;

  PetscFunctionBegin;
  if (monitor->firstnode) {
    /* Update links */
    node = monitor->firstnode;
    monitor->firstnode = node->next;

    /* Free list node */
    ierr = PetscViewerDestroy(&(node->viewer));CHKERRQ(ierr);
    ierr = VecDestroy(&(node->v));CHKERRQ(ierr);
    ierr = PetscFree(node);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMNetworkMonitorAdd"
/*@
  DMNetworkMonitorAdd - Adds a new viewer to monitor

  Collective on DMNetworkMonitor

  Input Parameters:
+ monitor - the monitor
. name - name of viewer
. element - vertex / edge number
. nodes - number of nodes
. start - variable starting offset
. blocksize - variable blocksize
. ymin - ymin for viewer
. ymax - ymax for viewer
- hold - determines if plot limits should be held

  Level: intermediate

  Notes:
  This is written to be independent of the semantics associated to the variables
  at a given network vertex / edge.

  Precisely, the parameters nodes, start and blocksize allow you to select a general
  strided subarray of the variables to monitor.

.seealso: DMNetworkMonitorCreate(), DMNetworkMonitorDestroy() 
@*/
PetscErrorCode DMNetworkMonitorAdd(DMNetworkMonitor monitor,const char *name,PetscInt element,PetscInt nodes,PetscInt start,PetscInt blocksize,PetscReal ymin,PetscReal ymax,PetscBool hold)
{
  PetscErrorCode       ierr;
  PetscDrawLG          drawlg;
  PetscDrawAxis        axis;
  PetscMPIInt          rank, size;
  DMNetworkMonitorList node; 
  char                 titleBuffer[64];
  PetscInt             vStart,vEnd,eStart,eEnd;

  PetscFunctionBegin;
  ierr = MPI_Comm_rank(monitor->comm, &rank);CHKERRQ(ierr);
  ierr = MPI_Comm_size(monitor->comm, &size);CHKERRQ(ierr);

  ierr = DMNetworkGetVertexRange(monitor->network, &vStart, &vEnd);
  ierr = DMNetworkGetEdgeRange(monitor->network, &eStart, &eEnd);

  /* Make window title */
  if (vStart <= element && element < vEnd) {
    ierr = PetscSNPrintf(titleBuffer, 64, "%s @ vertex %d [%d / %d]", name, element - vStart, rank, size-1);CHKERRQ(ierr);
  } else if (eStart <= element && element < eEnd) {
    ierr = PetscSNPrintf(titleBuffer, 64, "%s @ edge %d [%d / %d]", name, element - eStart, rank, size-1);CHKERRQ(ierr);
  } else {
    /* vertex / edge is not on local machine, so skip! */
    PetscFunctionReturn(0); 
  }

  ierr = PetscMalloc1(1, &node);CHKERRQ(ierr);

  /* Setup viewer. */
  ierr = PetscViewerDrawOpen(monitor->comm, PETSC_NULL, titleBuffer, PETSC_DECIDE, PETSC_DECIDE, PETSC_DRAW_QUARTER_SIZE, PETSC_DRAW_QUARTER_SIZE, &(node->viewer));CHKERRQ(ierr);
  ierr = PetscViewerPushFormat(node->viewer, PETSC_VIEWER_DRAW_LG);CHKERRQ(ierr);
  ierr = PetscViewerDrawGetDrawLG(node->viewer, 0, &drawlg);CHKERRQ(ierr);
  ierr = PetscDrawLGGetAxis(drawlg, &axis);CHKERRQ(ierr);
  ierr = PetscDrawAxisSetLimits(axis, 0, nodes-1, ymin, ymax);CHKERRQ(ierr);
  ierr = PetscDrawAxisSetHoldLimits(axis, hold);CHKERRQ(ierr);

  /* Setup vector storage for drawing. */
  ierr = VecCreateSeq(PETSC_COMM_SELF, nodes, &(node->v));CHKERRQ(ierr);

  node->element   = element;
  node->nodes     = nodes;
  node->start     = start;
  node->blocksize = blocksize;

  node->next         = monitor->firstnode;
  monitor->firstnode = node;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMNetworkMonitorView"
/*@
  DMNetworkMonitorView - Monitor function for TSMonitorSet.

  Collectiveon DMNetworkMonitor

  Input Parameters:
+ monitor - DMNetworkMonitor object
- x - TS solution vector

  Level: intermediate

.seealso: DMNetworkMonitorCreate(), DMNetworkMonitorDestroy(), DMNetworkMonitorAdd()
@*/
PetscErrorCode DMNetworkMonitorView(DMNetworkMonitor monitor,Vec x)
{
  PetscErrorCode      ierr;
  PetscInt            varoffset,i,start;
  const PetscScalar   *xx;
  PetscScalar         *vv;
  DMNetworkMonitorList node;

  PetscFunctionBegin;
  ierr = VecGetArrayRead(x, &xx);CHKERRQ(ierr);
  for (node = monitor->firstnode; node; node = node->next) {
    ierr = DMNetworkGetVariableGlobalOffset(monitor->network, node->element, &varoffset);CHKERRQ(ierr);
    ierr = VecGetArray(node->v, &vv);CHKERRQ(ierr);
    start = varoffset + node->start;
    for (i = 0; i < node->nodes; i++) {
      vv[i] = xx[start+i*node->blocksize];
    }
    ierr = VecRestoreArray(node->v, &vv);CHKERRQ(ierr);
    ierr = VecView(node->v, node->viewer);CHKERRQ(ierr);
  }
  ierr = VecRestoreArrayRead(x, &xx);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}
