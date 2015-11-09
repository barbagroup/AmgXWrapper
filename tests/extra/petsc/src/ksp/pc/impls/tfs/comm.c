
/***********************************comm.c*************************************

Author: Henry M. Tufo III

e-mail: hmt@cs.brown.edu

snail-mail:
Division of Applied Mathematics
Brown University
Providence, RI 02912

Last Modification:
11.21.97
***********************************comm.c*************************************/
#include <../src/ksp/pc/impls/tfs/tfs.h>


/* global program control variables - explicitly exported */
PetscMPIInt PCTFS_my_id            = 0;
PetscMPIInt PCTFS_num_nodes        = 1;
PetscMPIInt PCTFS_floor_num_nodes  = 0;
PetscMPIInt PCTFS_i_log2_num_nodes = 0;

/* global program control variables */
static PetscInt p_init = 0;
static PetscInt modfl_num_nodes;
static PetscInt edge_not_pow_2;

static PetscInt edge_node[sizeof(PetscInt)*32];

/***********************************comm.c*************************************/
PetscErrorCode PCTFS_comm_init(void)
{

  if (p_init++) PetscFunctionReturn(0);

  MPI_Comm_size(MPI_COMM_WORLD,&PCTFS_num_nodes);
  MPI_Comm_rank(MPI_COMM_WORLD,&PCTFS_my_id);

  if (PCTFS_num_nodes> (INT_MAX >> 1)) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_PLIB,"Can't have more then MAX_INT/2 nodes!!!");

  PCTFS_ivec_zero((PetscInt*)edge_node,sizeof(PetscInt)*32);

  PCTFS_floor_num_nodes  = 1;
  PCTFS_i_log2_num_nodes = modfl_num_nodes = 0;
  while (PCTFS_floor_num_nodes <= PCTFS_num_nodes) {
    edge_node[PCTFS_i_log2_num_nodes] = PCTFS_my_id ^ PCTFS_floor_num_nodes;
    PCTFS_floor_num_nodes           <<= 1;
    PCTFS_i_log2_num_nodes++;
  }

  PCTFS_i_log2_num_nodes--;
  PCTFS_floor_num_nodes >>= 1;
  modfl_num_nodes         = (PCTFS_num_nodes - PCTFS_floor_num_nodes);

  if ((PCTFS_my_id > 0) && (PCTFS_my_id <= modfl_num_nodes)) edge_not_pow_2=((PCTFS_my_id|PCTFS_floor_num_nodes)-1);
  else if (PCTFS_my_id >= PCTFS_floor_num_nodes) edge_not_pow_2=((PCTFS_my_id^PCTFS_floor_num_nodes)+1);
  else edge_not_pow_2 = 0;
  PetscFunctionReturn(0);
}

/***********************************comm.c*************************************/
PetscErrorCode PCTFS_giop(PetscInt *vals, PetscInt *work, PetscInt n, PetscInt *oprs)
{
  PetscInt   mask, edge;
  PetscInt   type, dest;
  vfp        fp;
  MPI_Status status;
  PetscInt   ierr;

  PetscFunctionBegin;
  /* ok ... should have some data, work, and operator(s) */
  if (!vals||!work||!oprs) SETERRQ3(PETSC_COMM_SELF,PETSC_ERR_PLIB,"PCTFS_giop() :: vals=%D, work=%D, oprs=%D",vals,work,oprs);

  /* non-uniform should have at least two entries */
  if ((oprs[0] == NON_UNIFORM)&&(n<2)) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_PLIB,"PCTFS_giop() :: non_uniform and n=0,1?");

  /* check to make sure comm package has been initialized */
  if (!p_init) PCTFS_comm_init();

  /* if there's nothing to do return */
  if ((PCTFS_num_nodes<2)||(!n)) PetscFunctionReturn(0);

  /* a negative number if items to send ==> fatal */
  if (n<0) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_PLIB,"PCTFS_giop() :: n=%D<0?",n);

  /* advance to list of n operations for custom */
  if ((type=oprs[0])==NON_UNIFORM) oprs++;

  /* major league hack */
  if (!(fp = (vfp) PCTFS_ivec_fct_addr(type))) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_PLIB,"PCTFS_giop() :: Could not retrieve function pointer!\n");

  /* all msgs will be of the same length */
  /* if not a hypercube must colapse partial dim */
  if (edge_not_pow_2) {
    if (PCTFS_my_id >= PCTFS_floor_num_nodes) {
      ierr = MPI_Send(vals,n,MPIU_INT,edge_not_pow_2,MSGTAG0+PCTFS_my_id,MPI_COMM_WORLD);CHKERRQ(ierr);
    } else {
      ierr = MPI_Recv(work,n,MPIU_INT,MPI_ANY_SOURCE,MSGTAG0+edge_not_pow_2, MPI_COMM_WORLD,&status);CHKERRQ(ierr);
      (*fp)(vals,work,n,oprs);
    }
  }

  /* implement the mesh fan in/out exchange algorithm */
  if (PCTFS_my_id<PCTFS_floor_num_nodes) {
    for (mask=1,edge=0; edge<PCTFS_i_log2_num_nodes; edge++,mask<<=1) {
      dest = PCTFS_my_id^mask;
      if (PCTFS_my_id > dest) {
        ierr = MPI_Send(vals,n,MPIU_INT,dest,MSGTAG2+PCTFS_my_id,MPI_COMM_WORLD);CHKERRQ(ierr);
      } else {
        ierr = MPI_Recv(work,n,MPIU_INT,MPI_ANY_SOURCE,MSGTAG2+dest,MPI_COMM_WORLD, &status);CHKERRQ(ierr);
        (*fp)(vals, work, n, oprs);
      }
    }

    mask=PCTFS_floor_num_nodes>>1;
    for (edge=0; edge<PCTFS_i_log2_num_nodes; edge++,mask>>=1) {
      if (PCTFS_my_id%mask) continue;

      dest = PCTFS_my_id^mask;
      if (PCTFS_my_id < dest) {
        ierr = MPI_Send(vals,n,MPIU_INT,dest,MSGTAG4+PCTFS_my_id,MPI_COMM_WORLD);CHKERRQ(ierr);
      } else {
        ierr = MPI_Recv(vals,n,MPIU_INT,MPI_ANY_SOURCE,MSGTAG4+dest,MPI_COMM_WORLD, &status);CHKERRQ(ierr);
      }
    }
  }

  /* if not a hypercube must expand to partial dim */
  if (edge_not_pow_2) {
    if (PCTFS_my_id >= PCTFS_floor_num_nodes) {
      ierr = MPI_Recv(vals,n,MPIU_INT,MPI_ANY_SOURCE,MSGTAG5+edge_not_pow_2,MPI_COMM_WORLD,&status);CHKERRQ(ierr);
    } else {
      ierr = MPI_Send(vals,n,MPIU_INT,edge_not_pow_2,MSGTAG5+PCTFS_my_id,MPI_COMM_WORLD);CHKERRQ(ierr);
    }
  }
  PetscFunctionReturn(0);
}

/***********************************comm.c*************************************/
PetscErrorCode PCTFS_grop(PetscScalar *vals, PetscScalar *work, PetscInt n, PetscInt *oprs)
{
  PetscInt       mask, edge;
  PetscInt       type, dest;
  vfp            fp;
  MPI_Status     status;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  /* ok ... should have some data, work, and operator(s) */
  if (!vals||!work||!oprs) SETERRQ3(PETSC_COMM_SELF,PETSC_ERR_PLIB,"PCTFS_grop() :: vals=%D, work=%D, oprs=%D",vals,work,oprs);

  /* non-uniform should have at least two entries */
  if ((oprs[0] == NON_UNIFORM)&&(n<2)) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_PLIB,"PCTFS_grop() :: non_uniform and n=0,1?");

  /* check to make sure comm package has been initialized */
  if (!p_init) PCTFS_comm_init();

  /* if there's nothing to do return */
  if ((PCTFS_num_nodes<2)||(!n)) PetscFunctionReturn(0);

  /* a negative number of items to send ==> fatal */
  if (n<0) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_PLIB,"gdop() :: n=%D<0?",n);

  /* advance to list of n operations for custom */
  if ((type=oprs[0])==NON_UNIFORM) oprs++;

  if (!(fp = (vfp) PCTFS_rvec_fct_addr(type))) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_PLIB,"PCTFS_grop() :: Could not retrieve function pointer!\n");

  /* all msgs will be of the same length */
  /* if not a hypercube must colapse partial dim */
  if (edge_not_pow_2) {
    if (PCTFS_my_id >= PCTFS_floor_num_nodes) {
      ierr = MPI_Send(vals,n,MPIU_SCALAR,edge_not_pow_2,MSGTAG0+PCTFS_my_id,MPI_COMM_WORLD);CHKERRQ(ierr);
    } else {
      ierr = MPI_Recv(work,n,MPIU_SCALAR,MPI_ANY_SOURCE,MSGTAG0+edge_not_pow_2,MPI_COMM_WORLD,&status);CHKERRQ(ierr);
      (*fp)(vals,work,n,oprs);
    }
  }

  /* implement the mesh fan in/out exchange algorithm */
  if (PCTFS_my_id<PCTFS_floor_num_nodes) {
    for (mask=1,edge=0; edge<PCTFS_i_log2_num_nodes; edge++,mask<<=1) {
      dest = PCTFS_my_id^mask;
      if (PCTFS_my_id > dest) {
        ierr = MPI_Send(vals,n,MPIU_SCALAR,dest,MSGTAG2+PCTFS_my_id,MPI_COMM_WORLD);CHKERRQ(ierr);
      } else {
        ierr = MPI_Recv(work,n,MPIU_SCALAR,MPI_ANY_SOURCE,MSGTAG2+dest,MPI_COMM_WORLD, &status);CHKERRQ(ierr);
        (*fp)(vals, work, n, oprs);
      }
    }

    mask=PCTFS_floor_num_nodes>>1;
    for (edge=0; edge<PCTFS_i_log2_num_nodes; edge++,mask>>=1) {
      if (PCTFS_my_id%mask) continue;

      dest = PCTFS_my_id^mask;
      if (PCTFS_my_id < dest) {
        ierr = MPI_Send(vals,n,MPIU_SCALAR,dest,MSGTAG4+PCTFS_my_id,MPI_COMM_WORLD);CHKERRQ(ierr);
      } else {
        ierr = MPI_Recv(vals,n,MPIU_SCALAR,MPI_ANY_SOURCE,MSGTAG4+dest,MPI_COMM_WORLD, &status);CHKERRQ(ierr);
      }
    }
  }

  /* if not a hypercube must expand to partial dim */
  if (edge_not_pow_2) {
    if (PCTFS_my_id >= PCTFS_floor_num_nodes) {
      ierr = MPI_Recv(vals,n,MPIU_SCALAR,MPI_ANY_SOURCE,MSGTAG5+edge_not_pow_2, MPI_COMM_WORLD,&status);CHKERRQ(ierr);
    } else {
      ierr = MPI_Send(vals,n,MPIU_SCALAR,edge_not_pow_2,MSGTAG5+PCTFS_my_id,MPI_COMM_WORLD);CHKERRQ(ierr);
    }
  }
  PetscFunctionReturn(0);
}

/***********************************comm.c*************************************/
PetscErrorCode PCTFS_grop_hc(PetscScalar *vals, PetscScalar *work, PetscInt n, PetscInt *oprs, PetscInt dim)
{
  PetscInt       mask, edge;
  PetscInt       type, dest;
  vfp            fp;
  MPI_Status     status;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  /* ok ... should have some data, work, and operator(s) */
  if (!vals||!work||!oprs) SETERRQ3(PETSC_COMM_SELF,PETSC_ERR_PLIB,"PCTFS_grop_hc() :: vals=%D, work=%D, oprs=%D",vals,work,oprs);

  /* non-uniform should have at least two entries */
  if ((oprs[0] == NON_UNIFORM)&&(n<2)) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_PLIB,"PCTFS_grop_hc() :: non_uniform and n=0,1?");

  /* check to make sure comm package has been initialized */
  if (!p_init) PCTFS_comm_init();

  /* if there's nothing to do return */
  if ((PCTFS_num_nodes<2)||(!n)||(dim<=0)) PetscFunctionReturn(0);

  /* the error msg says it all!!! */
  if (modfl_num_nodes) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_PLIB,"PCTFS_grop_hc() :: PCTFS_num_nodes not a power of 2!?!");

  /* a negative number of items to send ==> fatal */
  if (n<0) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_PLIB,"PCTFS_grop_hc() :: n=%D<0?",n);

  /* can't do more dimensions then exist */
  dim = PetscMin(dim,PCTFS_i_log2_num_nodes);

  /* advance to list of n operations for custom */
  if ((type=oprs[0])==NON_UNIFORM) oprs++;

  if (!(fp = (vfp) PCTFS_rvec_fct_addr(type))) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_PLIB,"PCTFS_grop_hc() :: Could not retrieve function pointer!\n");

  for (mask=1,edge=0; edge<dim; edge++,mask<<=1) {
    dest = PCTFS_my_id^mask;
    if (PCTFS_my_id > dest) {
      ierr = MPI_Send(vals,n,MPIU_SCALAR,dest,MSGTAG2+PCTFS_my_id,MPI_COMM_WORLD);CHKERRQ(ierr);
    } else {
      ierr = MPI_Recv(work,n,MPIU_SCALAR,MPI_ANY_SOURCE,MSGTAG2+dest,MPI_COMM_WORLD,&status);CHKERRQ(ierr);
      (*fp)(vals, work, n, oprs);
    }
  }

  if (edge==dim) mask>>=1;
  else {
    while (++edge<dim) mask<<=1;
  }

  for (edge=0; edge<dim; edge++,mask>>=1) {
    if (PCTFS_my_id%mask) continue;

    dest = PCTFS_my_id^mask;
    if (PCTFS_my_id < dest) {
      ierr = MPI_Send(vals,n,MPIU_SCALAR,dest,MSGTAG4+PCTFS_my_id,MPI_COMM_WORLD);CHKERRQ(ierr);
    } else {
      ierr = MPI_Recv(vals,n,MPIU_SCALAR,MPI_ANY_SOURCE,MSGTAG4+dest,MPI_COMM_WORLD,&status);CHKERRQ(ierr);
    }
  }
  PetscFunctionReturn(0);
}

/******************************************************************************/
PetscErrorCode PCTFS_ssgl_radd(PetscScalar *vals,  PetscScalar *work,  PetscInt level, PetscInt *segs)
{
  PetscInt       edge, type, dest, mask;
  PetscInt       stage_n;
  MPI_Status     status;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  /* check to make sure comm package has been initialized */
  if (!p_init) PCTFS_comm_init();

  /* all msgs are *NOT* the same length */
  /* implement the mesh fan in/out exchange algorithm */
  for (mask=0, edge=0; edge<level; edge++, mask++) {
    stage_n = (segs[level] - segs[edge]);
    if (stage_n && !(PCTFS_my_id & mask)) {
      dest = edge_node[edge];
      type = MSGTAG3 + PCTFS_my_id + (PCTFS_num_nodes*edge);
      if (PCTFS_my_id>dest) {
        ierr = MPI_Send(vals+segs[edge],stage_n,MPIU_SCALAR,dest,type, MPI_COMM_WORLD);CHKERRQ(ierr);
      } else {
        type =  type - PCTFS_my_id + dest;
        ierr = MPI_Recv(work,stage_n,MPIU_SCALAR,MPI_ANY_SOURCE,type,MPI_COMM_WORLD,&status);CHKERRQ(ierr);
        PCTFS_rvec_add(vals+segs[edge], work, stage_n);
      }
    }
    mask <<= 1;
  }
  mask>>=1;
  for (edge=0; edge<level; edge++) {
    stage_n = (segs[level] - segs[level-1-edge]);
    if (stage_n && !(PCTFS_my_id & mask)) {
      dest = edge_node[level-edge-1];
      type = MSGTAG6 + PCTFS_my_id + (PCTFS_num_nodes*edge);
      if (PCTFS_my_id<dest) {
        ierr = MPI_Send(vals+segs[level-1-edge],stage_n,MPIU_SCALAR,dest,type,MPI_COMM_WORLD);CHKERRQ(ierr);
      } else {
        type =  type - PCTFS_my_id + dest;
        ierr = MPI_Recv(vals+segs[level-1-edge],stage_n,MPIU_SCALAR, MPI_ANY_SOURCE,type,MPI_COMM_WORLD,&status);CHKERRQ(ierr);
      }
    }
    mask >>= 1;
  }
  PetscFunctionReturn(0);
}

/***********************************comm.c*************************************/
PetscErrorCode PCTFS_giop_hc(PetscInt *vals, PetscInt *work, PetscInt n, PetscInt *oprs, PetscInt dim)
{
  PetscInt       mask, edge;
  PetscInt       type, dest;
  vfp            fp;
  MPI_Status     status;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  /* ok ... should have some data, work, and operator(s) */
  if (!vals||!work||!oprs) SETERRQ3(PETSC_COMM_SELF,PETSC_ERR_PLIB,"PCTFS_giop_hc() :: vals=%D, work=%D, oprs=%D",vals,work,oprs);

  /* non-uniform should have at least two entries */
  if ((oprs[0] == NON_UNIFORM)&&(n<2)) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_PLIB,"PCTFS_giop_hc() :: non_uniform and n=0,1?");

  /* check to make sure comm package has been initialized */
  if (!p_init) PCTFS_comm_init();

  /* if there's nothing to do return */
  if ((PCTFS_num_nodes<2)||(!n)||(dim<=0)) PetscFunctionReturn(0);

  /* the error msg says it all!!! */
  if (modfl_num_nodes) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_PLIB,"PCTFS_giop_hc() :: PCTFS_num_nodes not a power of 2!?!");

  /* a negative number of items to send ==> fatal */
  if (n<0) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_PLIB,"PCTFS_giop_hc() :: n=%D<0?",n);

  /* can't do more dimensions then exist */
  dim = PetscMin(dim,PCTFS_i_log2_num_nodes);

  /* advance to list of n operations for custom */
  if ((type=oprs[0])==NON_UNIFORM) oprs++;

  if (!(fp = (vfp) PCTFS_ivec_fct_addr(type))) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_PLIB,"PCTFS_giop_hc() :: Could not retrieve function pointer!\n");

  for (mask=1,edge=0; edge<dim; edge++,mask<<=1) {
    dest = PCTFS_my_id^mask;
    if (PCTFS_my_id > dest) {
      ierr = MPI_Send(vals,n,MPIU_INT,dest,MSGTAG2+PCTFS_my_id,MPI_COMM_WORLD);CHKERRQ(ierr);
    } else {
      ierr = MPI_Recv(work,n,MPIU_INT,MPI_ANY_SOURCE,MSGTAG2+dest,MPI_COMM_WORLD, &status);CHKERRQ(ierr);
      (*fp)(vals, work, n, oprs);
    }
  }

  if (edge==dim) mask>>=1;
  else {
    while (++edge<dim) mask<<=1;
  }

  for (edge=0; edge<dim; edge++,mask>>=1) {
    if (PCTFS_my_id%mask) continue;

    dest = PCTFS_my_id^mask;
    if (PCTFS_my_id < dest) {
      ierr = MPI_Send(vals,n,MPIU_INT,dest,MSGTAG4+PCTFS_my_id,MPI_COMM_WORLD);CHKERRQ(ierr);
    } else {
      ierr = MPI_Recv(vals,n,MPIU_INT,MPI_ANY_SOURCE,MSGTAG4+dest,MPI_COMM_WORLD,&status);CHKERRQ(ierr);
    }
  }
  PetscFunctionReturn(0);
}
