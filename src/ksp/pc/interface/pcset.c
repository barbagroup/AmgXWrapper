
/*
    Routines to set PC methods and options.
*/

#include <petsc/private/pcimpl.h>      /*I "petscpc.h" I*/
#include <petscdm.h>

PetscBool PCRegisterAllCalled = PETSC_FALSE;
/*
   Contains the list of registered KSP routines
*/
PetscFunctionList PCList = 0;

#undef __FUNCT__
#define __FUNCT__ "PCSetType"
/*@C
   PCSetType - Builds PC for a particular preconditioner type

   Collective on PC

   Input Parameter:
+  pc - the preconditioner context.
-  type - a known method

   Options Database Key:
.  -pc_type <type> - Sets PC type

   Use -help for a list of available methods (for instance,
   jacobi or bjacobi)

  Notes:
  See "petsc/include/petscpc.h" for available methods (for instance,
  PCJACOBI, PCILU, or PCBJACOBI).

  Normally, it is best to use the KSPSetFromOptions() command and
  then set the PC type from the options database rather than by using
  this routine.  Using the options database provides the user with
  maximum flexibility in evaluating the many different preconditioners.
  The PCSetType() routine is provided for those situations where it
  is necessary to set the preconditioner independently of the command
  line or options database.  This might be the case, for example, when
  the choice of preconditioner changes during the execution of the
  program, and the user's application is taking responsibility for
  choosing the appropriate preconditioner.  In other words, this
  routine is not for beginners.

  Level: intermediate

  Developer Note: PCRegister() is used to add preconditioner types to PCList from which they
  are accessed by PCSetType().

.keywords: PC, set, method, type

.seealso: KSPSetType(), PCType, PCRegister(), PCCreate(), KSPGetPC()

@*/
PetscErrorCode  PCSetType(PC pc,PCType type)
{
  PetscErrorCode ierr,(*r)(PC);
  PetscBool      match;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(pc,PC_CLASSID,1);
  PetscValidCharPointer(type,2);

  ierr = PetscObjectTypeCompare((PetscObject)pc,type,&match);CHKERRQ(ierr);
  if (match) PetscFunctionReturn(0);

  ierr =  PetscFunctionListFind(PCList,type,&r);CHKERRQ(ierr);
  if (!r) SETERRQ1(PetscObjectComm((PetscObject)pc),PETSC_ERR_ARG_UNKNOWN_TYPE,"Unable to find requested PC type %s",type);
  /* Destroy the previous private PC context */
  if (pc->ops->destroy) {
    ierr             =  (*pc->ops->destroy)(pc);CHKERRQ(ierr);
    pc->ops->destroy = NULL;
    pc->data         = 0;
  }
  ierr = PetscFunctionListDestroy(&((PetscObject)pc)->qlist);CHKERRQ(ierr);
  /* Reinitialize function pointers in PCOps structure */
  ierr = PetscMemzero(pc->ops,sizeof(struct _PCOps));CHKERRQ(ierr);
  /* XXX Is this OK?? */
  pc->modifysubmatrices  = 0;
  pc->modifysubmatricesP = 0;
  /* Call the PCCreate_XXX routine for this particular preconditioner */
  pc->setupcalled = 0;

  ierr = PetscObjectChangeTypeName((PetscObject)pc,type);CHKERRQ(ierr);
  ierr = (*r)(pc);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PCGetType"
/*@C
   PCGetType - Gets the PC method type and name (as a string) from the PC
   context.

   Not Collective

   Input Parameter:
.  pc - the preconditioner context

   Output Parameter:
.  type - name of preconditioner method

   Level: intermediate

.keywords: PC, get, method, name, type

.seealso: PCSetType()

@*/
PetscErrorCode  PCGetType(PC pc,PCType *type)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(pc,PC_CLASSID,1);
  PetscValidPointer(type,2);
  *type = ((PetscObject)pc)->type_name;
  PetscFunctionReturn(0);
}

extern PetscErrorCode PCGetDefaultType_Private(PC,const char*[]);

#undef __FUNCT__
#define __FUNCT__ "PCSetFromOptions"
/*@
   PCSetFromOptions - Sets PC options from the options database.
   This routine must be called before PCSetUp() if the user is to be
   allowed to set the preconditioner method.

   Collective on PC

   Input Parameter:
.  pc - the preconditioner context

   Options Database:
.   -pc_use_amat true,false see PCSetUseAmat()

   Level: developer

.keywords: PC, set, from, options, database

.seealso: PCSetUseAmat()

@*/
PetscErrorCode  PCSetFromOptions(PC pc)
{
  PetscErrorCode ierr;
  char           type[256];
  const char     *def;
  PetscBool      flg;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(pc,PC_CLASSID,1);

  ierr = PCRegisterAll();CHKERRQ(ierr);
  ierr = PetscObjectOptionsBegin((PetscObject)pc);CHKERRQ(ierr);
  if (!((PetscObject)pc)->type_name) {
    ierr = PCGetDefaultType_Private(pc,&def);CHKERRQ(ierr);
  } else {
    def = ((PetscObject)pc)->type_name;
  }

  ierr = PetscOptionsFList("-pc_type","Preconditioner","PCSetType",PCList,def,type,256,&flg);CHKERRQ(ierr);
  if (flg) {
    ierr = PCSetType(pc,type);CHKERRQ(ierr);
  } else if (!((PetscObject)pc)->type_name) {
    ierr = PCSetType(pc,def);CHKERRQ(ierr);
  }

  ierr = PetscObjectTypeCompare((PetscObject)pc,PCNONE,&flg);CHKERRQ(ierr);
  if (flg) goto skipoptions;

  ierr = PetscOptionsBool("-pc_use_amat","use Amat (instead of Pmat) to define preconditioner in nested inner solves","PCSetUseAmat",pc->useAmat,&pc->useAmat,NULL);CHKERRQ(ierr);

  if (pc->ops->setfromoptions) {
    ierr = (*pc->ops->setfromoptions)(PetscOptionsObject,pc);CHKERRQ(ierr);
  }

  skipoptions:
  /* process any options handlers added with PetscObjectAddOptionsHandler() */
  ierr = PetscObjectProcessOptionsHandlers(PetscOptionsObject,(PetscObject)pc);CHKERRQ(ierr);
  ierr = PetscOptionsEnd();CHKERRQ(ierr);
  pc->setfromoptionscalled++;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PCSetDM"
/*@
   PCSetDM - Sets the DM that may be used by some preconditioners

   Logically Collective on PC

   Input Parameters:
+  pc - the preconditioner context
-  dm - the dm

   Level: intermediate


.seealso: PCGetDM(), KSPSetDM(), KSPGetDM()
@*/
PetscErrorCode  PCSetDM(PC pc,DM dm)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(pc,PC_CLASSID,1);
  if (dm) {ierr = PetscObjectReference((PetscObject)dm);CHKERRQ(ierr);}
  ierr   = DMDestroy(&pc->dm);CHKERRQ(ierr);
  pc->dm = dm;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PCGetDM"
/*@
   PCGetDM - Gets the DM that may be used by some preconditioners

   Not Collective

   Input Parameter:
. pc - the preconditioner context

   Output Parameter:
.  dm - the dm

   Level: intermediate


.seealso: PCSetDM(), KSPSetDM(), KSPGetDM()
@*/
PetscErrorCode  PCGetDM(PC pc,DM *dm)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(pc,PC_CLASSID,1);
  *dm = pc->dm;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PCSetApplicationContext"
/*@
   PCSetApplicationContext - Sets the optional user-defined context for the linear solver.

   Logically Collective on PC

   Input Parameters:
+  pc - the PC context
-  usrP - optional user context

   Level: intermediate

.keywords: PC, set, application, context

.seealso: PCGetApplicationContext()
@*/
PetscErrorCode  PCSetApplicationContext(PC pc,void *usrP)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(pc,PC_CLASSID,1);
  pc->user = usrP;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PCGetApplicationContext"
/*@
   PCGetApplicationContext - Gets the user-defined context for the linear solver.

   Not Collective

   Input Parameter:
.  pc - PC context

   Output Parameter:
.  usrP - user context

   Level: intermediate

.keywords: PC, get, application, context

.seealso: PCSetApplicationContext()
@*/
PetscErrorCode  PCGetApplicationContext(PC pc,void *usrP)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(pc,PC_CLASSID,1);
  *(void**)usrP = pc->user;
  PetscFunctionReturn(0);
}

