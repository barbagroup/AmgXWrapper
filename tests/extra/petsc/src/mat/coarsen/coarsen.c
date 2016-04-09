
#include <petsc/private/matimpl.h>               /*I "petscmat.h" I*/

/* Logging support */
PetscClassId MAT_COARSEN_CLASSID;

PetscFunctionList MatCoarsenList              = 0;
PetscBool         MatCoarsenRegisterAllCalled = PETSC_FALSE;

#undef __FUNCT__
#define __FUNCT__ "MatCoarsenRegister"
/*@C
   MatCoarsenRegister - Adds a new sparse matrix coarser to the  matrix package.

   Logically Collective

   Input Parameters:
+  sname - name of coarsen (for example MATCOARSENMIS)
-  function - function pointer that creates the coarsen type

   Level: developer

   Sample usage:
.vb
   MatCoarsenRegister("my_agg",MyAggCreate);
.ve

   Then, your aggregator can be chosen with the procedural interface via
$     MatCoarsenSetType(agg,"my_agg")
   or at runtime via the option
$     -mat_coarsen_type my_agg

.keywords: matrix, coarsen, register

.seealso: MatCoarsenRegisterDestroy(), MatCoarsenRegisterAll()
@*/
PetscErrorCode  MatCoarsenRegister(const char sname[],PetscErrorCode (*function)(MatCoarsen))
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscFunctionListAdd(&MatCoarsenList,sname,function);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatCoarsenGetType"
/*@C
   MatCoarsenGetType - Gets the Coarsen method type and name (as a string)
        from the coarsen context.

   Not collective

   Input Parameter:
.  coarsen - the coarsen context

   Output Parameter:
.  type - coarsener type

   Level: intermediate

   Not Collective

.keywords: Coarsen, get, method, name, type
@*/
PetscErrorCode  MatCoarsenGetType(MatCoarsen coarsen,MatCoarsenType *type)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(coarsen,MAT_COARSEN_CLASSID,1);
  PetscValidPointer(type,2);
  *type = ((PetscObject)coarsen)->type_name;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatCoarsenApply"
/*@
   MatCoarsenApply - Gets a coarsen for a matrix.

   Collective on MatCoarsen

   Input Parameter:
.   coarsen - the coarsen

   Options Database Keys:
   To specify the coarsen through the options database, use one of
   the following
$    -mat_coarsen_type mis
   To see the coarsen result
$    -mat_coarsen_view

   Level: beginner

   Notes: Use MatCoarsenGetData() to access the results of the coarsening

   The user can define additional coarsens; see MatCoarsenRegister().

.keywords: matrix, get, coarsen

.seealso:  MatCoarsenRegister(), MatCoarsenCreate(),
           MatCoarsenDestroy(), MatCoarsenSetAdjacency(), ISCoarsenToNumbering(),
           ISCoarsenCount(), MatCoarsenGetData()
@*/
PetscErrorCode  MatCoarsenApply(MatCoarsen coarser)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(coarser,MAT_COARSEN_CLASSID,1);
  PetscValidPointer(coarser,2);
  if (!coarser->graph->assembled) SETERRQ(PetscObjectComm((PetscObject)coarser),PETSC_ERR_ARG_WRONGSTATE,"Not for unassembled matrix");
  if (coarser->graph->factortype) SETERRQ(PetscObjectComm((PetscObject)coarser),PETSC_ERR_ARG_WRONGSTATE,"Not for factored matrix");
  if (!coarser->ops->apply) SETERRQ(PetscObjectComm((PetscObject)coarser),PETSC_ERR_ARG_WRONGSTATE,"Must set type with MatCoarsenSetFromOptions() or MatCoarsenSetType()");
  ierr = PetscLogEventBegin(MAT_Coarsen,coarser,0,0,0);CHKERRQ(ierr);
  ierr = (*coarser->ops->apply)(coarser);CHKERRQ(ierr);
  ierr = PetscLogEventEnd(MAT_Coarsen,coarser,0,0,0);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatCoarsenSetAdjacency"
/*@
   MatCoarsenSetAdjacency - Sets the adjacency graph (matrix) of the thing to be
      partitioned.

   Collective on MatCoarsen and Mat

   Input Parameters:
+  agg - the coarsen context
-  adj - the adjacency matrix

   Level: beginner

.keywords: Coarsen, adjacency

.seealso: MatCoarsenCreate()
@*/
PetscErrorCode  MatCoarsenSetAdjacency(MatCoarsen agg, Mat adj)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(agg,MAT_COARSEN_CLASSID,1);
  PetscValidHeaderSpecific(adj,MAT_CLASSID,2);
  agg->graph = adj;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatCoarsenSetStrictAggs"
/*@
   MatCoarsenSetStrictAggs -

   Logically Collective on MatCoarsen

   Input Parameters:
+  agg - the coarsen context
-  str - the adjacency matrix

   Level: beginner

.keywords: Coarsen, adjacency

.seealso: MatCoarsenCreate()
@*/
PetscErrorCode MatCoarsenSetStrictAggs(MatCoarsen agg, PetscBool str)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(agg,MAT_COARSEN_CLASSID,1);
  agg->strict_aggs = str;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatCoarsenDestroy"
/*@
   MatCoarsenDestroy - Destroys the coarsen context.

   Collective on MatCoarsen

   Input Parameters:
.  agg - the coarsen context

   Level: beginner

.keywords: Coarsen, destroy, context

.seealso: MatCoarsenCreate()
@*/
PetscErrorCode  MatCoarsenDestroy(MatCoarsen *agg)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (!*agg) PetscFunctionReturn(0);
  PetscValidHeaderSpecific((*agg),MAT_COARSEN_CLASSID,1);
  if (--((PetscObject)(*agg))->refct > 0) {*agg = 0; PetscFunctionReturn(0);}

  if ((*agg)->ops->destroy) {
    ierr = (*(*agg)->ops->destroy)((*agg));CHKERRQ(ierr);
  }

  if ((*agg)->agg_lists) {
    ierr = PetscCDDestroy((*agg)->agg_lists);CHKERRQ(ierr);
  }

  ierr = PetscHeaderDestroy(agg);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatCoarsenCreate"
/*@
   MatCoarsenCreate - Creates a coarsen context.

   Collective on MPI_Comm

   Input Parameter:
.   comm - MPI communicator

   Output Parameter:
.  newcrs - location to put the context

   Level: beginner

.keywords: Coarsen, create, context

.seealso: MatCoarsenSetType(), MatCoarsenApply(), MatCoarsenDestroy(),
          MatCoarsenSetAdjacency()

@*/
PetscErrorCode  MatCoarsenCreate(MPI_Comm comm, MatCoarsen *newcrs)
{
  MatCoarsen     agg;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  *newcrs = 0;

  ierr = MatInitializePackage();CHKERRQ(ierr);
  ierr = PetscHeaderCreate(agg, MAT_COARSEN_CLASSID,"MatCoarsen","Matrix/graph coarsen", "MatCoarsen", comm, MatCoarsenDestroy, MatCoarsenView);CHKERRQ(ierr);

  *newcrs = agg;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatCoarsenView"
/*@C
   MatCoarsenView - Prints the coarsen data structure.

   Collective on MatCoarsen

   Input Parameters:
.  agg - the coarsen context
.  viewer - optional visualization context

   Level: intermediate

   Note:
   The available visualization contexts include
+     PETSC_VIEWER_STDOUT_SELF - standard output (default)
-     PETSC_VIEWER_STDOUT_WORLD - synchronized standard
         output where only the first processor opens
         the file.  All other processors send their
         data to the first processor to print.

   The user can open alternative visualization contexts with
.     PetscViewerASCIIOpen() - output to a specified file

.keywords: Coarsen, view

.seealso: PetscViewerASCIIOpen()
@*/
PetscErrorCode  MatCoarsenView(MatCoarsen agg,PetscViewer viewer)
{
  PetscErrorCode ierr;
  PetscBool      iascii;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(agg,MAT_COARSEN_CLASSID,1);
  if (!viewer) {
    ierr = PetscViewerASCIIGetStdout(PetscObjectComm((PetscObject)agg),&viewer);CHKERRQ(ierr);
  }
  PetscValidHeaderSpecific(viewer,PETSC_VIEWER_CLASSID,2);
  PetscCheckSameComm(agg,1,viewer,2);

  ierr = PetscObjectTypeCompare((PetscObject)viewer,PETSCVIEWERASCII,&iascii);CHKERRQ(ierr);
  ierr = PetscObjectPrintClassNamePrefixType((PetscObject)agg,viewer);CHKERRQ(ierr);
  if (agg->ops->view) {
    ierr = PetscViewerASCIIPushTab(viewer);CHKERRQ(ierr);
    ierr = (*agg->ops->view)(agg,viewer);CHKERRQ(ierr);
    ierr = PetscViewerASCIIPopTab(viewer);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatCoarsenSetType"
/*@C
   MatCoarsenSetType - Sets the type of aggregator to use

   Collective on MatCoarsen

   Input Parameter:
+  coarser - the coarsen context.
-  type - a known coarsening method

   Options Database Command:
$  -mat_coarsen_type  <type>
$      Use -help for a list of available methods
$      (for instance, mis)

   Level: intermediate

.keywords: coarsen, set, method, type

.seealso: MatCoarsenCreate(), MatCoarsenApply(), MatCoarsenType, MatCoarsenGetType()

@*/
PetscErrorCode  MatCoarsenSetType(MatCoarsen coarser, MatCoarsenType type)
{
  PetscErrorCode ierr,(*r)(MatCoarsen);
  PetscBool      match;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(coarser,MAT_COARSEN_CLASSID,1);
  PetscValidCharPointer(type,2);

  ierr = PetscObjectTypeCompare((PetscObject)coarser,type,&match);CHKERRQ(ierr);
  if (match) PetscFunctionReturn(0);

  if (coarser->setupcalled) {
    ierr =  (*coarser->ops->destroy)(coarser);CHKERRQ(ierr);

    coarser->ops->destroy = NULL;
    coarser->subctx       = 0;
    coarser->setupcalled  = 0;
  }

  ierr =  PetscFunctionListFind(MatCoarsenList,type,&r);CHKERRQ(ierr);

  if (!r) SETERRQ1(PetscObjectComm((PetscObject)coarser),PETSC_ERR_ARG_UNKNOWN_TYPE,"Unknown coarsen type %s",type);

  coarser->ops->destroy = (PetscErrorCode (*)(MatCoarsen)) 0;
  coarser->ops->view    = (PetscErrorCode (*)(MatCoarsen,PetscViewer)) 0;

  ierr = (*r)(coarser);CHKERRQ(ierr);

  ierr = PetscFree(((PetscObject)coarser)->type_name);CHKERRQ(ierr);
  ierr = PetscStrallocpy(type,&((PetscObject)coarser)->type_name);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatCoarsenSetGreedyOrdering"
/*@C
   MatCoarsenSetGreedyOrdering - Sets the ordering of the vertices to use with a greedy coarsening method

   Logically Collective on Coarsen

   Input Parameters:
+  coarser - the coarsen context
-  perm - vertex ordering of (greedy) algorithm

   Level: beginner

   Notes:
      The IS weights is freed by PETSc, so user has given this to us

.keywords: Coarsen

.seealso: MatCoarsenCreate(), MatCoarsenSetType()
@*/
PetscErrorCode MatCoarsenSetGreedyOrdering(MatCoarsen coarser, const IS perm)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(coarser,MAT_COARSEN_CLASSID,1);
  coarser->perm = perm;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatCoarsenGetData"
/*@C
   MatCoarsenGetData - Gets the weights for vertices for a coarsen.

   Logically Collective on Coarsen

   Input Parameter:
.  coarser - the coarsen context

   Output Parameter:
.  llist - linked list of aggregates

   Level: advanced

.keywords: Coarsen

.seealso: MatCoarsenCreate(), MatCoarsenSetType()
@*/
PetscErrorCode MatCoarsenGetData(MatCoarsen coarser, PetscCoarsenData **llist)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(coarser,MAT_COARSEN_CLASSID,1);
  if (!coarser->agg_lists) SETERRQ(PetscObjectComm((PetscObject)coarser),PETSC_ERR_ARG_WRONGSTATE,"No linked list - generate it or call ApplyCoarsen");
  *llist             = coarser->agg_lists;
  coarser->agg_lists = 0; /* giving up ownership */
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatCoarsenSetFromOptions"
/*@
   MatCoarsenSetFromOptions - Sets various coarsen options from the
        options database.

   Collective on MatCoarsen

   Input Parameter:
.  coarser - the coarsen context.

   Options Database Command:
$  -mat_coarsen_type  <type>
$      Use -help for a list of available methods
$      (for instance, mis)

   Level: beginner

.keywords: coarsen, set, method, type
@*/
PetscErrorCode MatCoarsenSetFromOptions(MatCoarsen coarser)
{
  PetscErrorCode ierr;
  PetscBool      flag;
  char           type[256];
  const char     *def;

  PetscFunctionBegin;
  ierr = PetscObjectOptionsBegin((PetscObject)coarser);CHKERRQ(ierr);
  if (!((PetscObject)coarser)->type_name) {
    def = MATCOARSENMIS;
  } else {
    def = ((PetscObject)coarser)->type_name;
  }

  ierr = PetscOptionsFList("-mat_coarsen_type","Type of aggregator","MatCoarsenSetType",MatCoarsenList,def,type,256,&flag);CHKERRQ(ierr);
  if (flag) {
    ierr = MatCoarsenSetType(coarser,type);CHKERRQ(ierr);
  }
  /*
   Set the type if it was never set.
   */
  if (!((PetscObject)coarser)->type_name) {
    ierr = MatCoarsenSetType(coarser,def);CHKERRQ(ierr);
  }

  if (coarser->ops->setfromoptions) {
    ierr = (*coarser->ops->setfromoptions)(PetscOptionsObject,coarser);CHKERRQ(ierr);
  }
  ierr = PetscOptionsEnd();CHKERRQ(ierr);
  ierr = MatCoarsenViewFromOptions(coarser,NULL,"-mat_coarsen_view");CHKERRQ(ierr);
  PetscFunctionReturn(0);
}






