
#include <petsc/private/viewerimpl.h>  /*I "petscviewer.h" I*/

PetscClassId PETSC_VIEWER_CLASSID;

static PetscBool PetscViewerPackageInitialized = PETSC_FALSE;
#undef __FUNCT__
#define __FUNCT__ "PetscViewerFinalizePackage"
/*@C
  PetscViewerFinalizePackage - This function destroys everything in the Petsc interface to Mathematica. It is
  called from PetscFinalize().

  Level: developer

.keywords: Petsc, destroy, package, mathematica
.seealso: PetscFinalize()
@*/
PetscErrorCode  PetscViewerFinalizePackage(void)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscFunctionListDestroy(&PetscViewerList);CHKERRQ(ierr);
  PetscViewerPackageInitialized = PETSC_FALSE;
  PetscViewerRegisterAllCalled  = PETSC_FALSE;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscViewerInitializePackage"
/*@C
  PetscViewerInitializePackage - This function initializes everything in the main PetscViewer package.

  Level: developer

.keywords: Petsc, initialize, package
.seealso: PetscInitialize()
@*/
PetscErrorCode  PetscViewerInitializePackage(void)
{
  char           logList[256];
  char           *className;
  PetscBool      opt;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (PetscViewerPackageInitialized) PetscFunctionReturn(0);
  PetscViewerPackageInitialized = PETSC_TRUE;
  /* Register Classes */
  ierr = PetscClassIdRegister("Viewer",&PETSC_VIEWER_CLASSID);CHKERRQ(ierr);

  /* Register Constructors */
  ierr = PetscViewerRegisterAll();CHKERRQ(ierr);

  /* Process info exclusions */
  ierr = PetscOptionsGetString(NULL,NULL, "-info_exclude", logList, 256, &opt);CHKERRQ(ierr);
  if (opt) {
    ierr = PetscStrstr(logList, "viewer", &className);CHKERRQ(ierr);
    if (className) {
      ierr = PetscInfoDeactivateClass(0);CHKERRQ(ierr);
    }
  }
  /* Process summary exclusions */
  ierr = PetscOptionsGetString(NULL,NULL, "-log_summary_exclude", logList, 256, &opt);CHKERRQ(ierr);
  if (opt) {
    ierr = PetscStrstr(logList, "viewer", &className);CHKERRQ(ierr);
    if (className) {
      ierr = PetscLogEventDeactivateClass(0);CHKERRQ(ierr);
    }
  }
#if defined(PETSC_HAVE_MATHEMATICA)
  ierr = PetscViewerMathematicaInitializePackage();CHKERRQ(ierr);
#endif
  ierr = PetscRegisterFinalize(PetscViewerFinalizePackage);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscViewerDestroy"
/*@
   PetscViewerDestroy - Destroys a PetscViewer.

   Collective on PetscViewer

   Input Parameters:
.  viewer - the PetscViewer to be destroyed.

   Level: beginner

.seealso: PetscViewerSocketOpen(), PetscViewerASCIIOpen(), PetscViewerCreate(), PetscViewerDrawOpen()

@*/
PetscErrorCode  PetscViewerDestroy(PetscViewer *viewer)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (!*viewer) PetscFunctionReturn(0);
  PetscValidHeaderSpecific(*viewer,PETSC_VIEWER_CLASSID,1);

  ierr = PetscViewerFlush(*viewer);CHKERRQ(ierr);
  if (--((PetscObject)(*viewer))->refct > 0) {*viewer = 0; PetscFunctionReturn(0);}

  ierr = PetscObjectSAWsViewOff((PetscObject)*viewer);CHKERRQ(ierr);
  if ((*viewer)->ops->destroy) {
    ierr = (*(*viewer)->ops->destroy)(*viewer);CHKERRQ(ierr);
  }
  ierr = PetscHeaderDestroy(viewer);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscViewerGetType"
/*@C
   PetscViewerGetType - Returns the type of a PetscViewer.

   Not Collective

   Input Parameter:
.   viewer - the PetscViewer

   Output Parameter:
.  type - PetscViewer type (see below)

   Available Types Include:
.  PETSCVIEWERSOCKET - Socket PetscViewer
.  PETSCVIEWERASCII - ASCII PetscViewer
.  PETSCVIEWERBINARY - binary file PetscViewer
.  PETSCVIEWERSTRING - string PetscViewer
.  PETSCVIEWERDRAW - drawing PetscViewer

   Level: intermediate

   Note:
   See include/petscviewer.h for a complete list of PetscViewers.

   PetscViewerType is actually a string

.seealso: PetscViewerCreate(), PetscViewerSetType(), PetscViewerType

@*/
PetscErrorCode  PetscViewerGetType(PetscViewer viewer,PetscViewerType *type)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(viewer,PETSC_VIEWER_CLASSID,1);
  PetscValidPointer(type,2);
  *type = ((PetscObject)viewer)->type_name;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscViewerSetOptionsPrefix"
/*@C
   PetscViewerSetOptionsPrefix - Sets the prefix used for searching for all
   PetscViewer options in the database.

   Logically Collective on PetscViewer

   Input Parameter:
+  viewer - the PetscViewer context
-  prefix - the prefix to prepend to all option names

   Notes:
   A hyphen (-) must NOT be given at the beginning of the prefix name.
   The first character of all runtime options is AUTOMATICALLY the hyphen.

   Level: advanced

.keywords: PetscViewer, set, options, prefix, database

.seealso: PetscViewerSetFromOptions()
@*/
PetscErrorCode  PetscViewerSetOptionsPrefix(PetscViewer viewer,const char prefix[])
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(viewer,PETSC_VIEWER_CLASSID,1);
  ierr = PetscObjectSetOptionsPrefix((PetscObject)viewer,prefix);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscViewerAppendOptionsPrefix"
/*@C
   PetscViewerAppendOptionsPrefix - Appends to the prefix used for searching for all
   PetscViewer options in the database.

   Logically Collective on PetscViewer

   Input Parameters:
+  viewer - the PetscViewer context
-  prefix - the prefix to prepend to all option names

   Notes:
   A hyphen (-) must NOT be given at the beginning of the prefix name.
   The first character of all runtime options is AUTOMATICALLY the hyphen.

   Level: advanced

.keywords: PetscViewer, append, options, prefix, database

.seealso: PetscViewerGetOptionsPrefix()
@*/
PetscErrorCode  PetscViewerAppendOptionsPrefix(PetscViewer viewer,const char prefix[])
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(viewer,PETSC_VIEWER_CLASSID,1);
  ierr = PetscObjectAppendOptionsPrefix((PetscObject)viewer,prefix);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscViewerGetOptionsPrefix"
/*@C
   PetscViewerGetOptionsPrefix - Sets the prefix used for searching for all
   PetscViewer options in the database.

   Not Collective

   Input Parameter:
.  viewer - the PetscViewer context

   Output Parameter:
.  prefix - pointer to the prefix string used

   Notes: On the fortran side, the user should pass in a string 'prefix' of
   sufficient length to hold the prefix.

   Level: advanced

.keywords: PetscViewer, get, options, prefix, database

.seealso: PetscViewerAppendOptionsPrefix()
@*/
PetscErrorCode  PetscViewerGetOptionsPrefix(PetscViewer viewer,const char *prefix[])
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(viewer,PETSC_VIEWER_CLASSID,1);
  ierr = PetscObjectGetOptionsPrefix((PetscObject)viewer,prefix);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscViewerSetUp"
/*@
   PetscViewerSetUp - Sets up the internal viewer data structures for the later use.

   Collective on PetscViewer

   Input Parameters:
.  viewer - the PetscViewer context

   Notes:
   For basic use of the PetscViewer classes the user need not explicitly call
   PetscViewerSetUp(), since these actions will happen automatically.

   Level: advanced

.keywords: PetscViewer, setup

.seealso: PetscViewerCreate(), PetscViewerDestroy()
@*/
PetscErrorCode  PetscViewerSetUp(PetscViewer viewer)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(viewer,PETSC_VIEWER_CLASSID,1);
  if (viewer->setupcalled) PetscFunctionReturn(0);
  if (viewer->ops->setup) {
    ierr = (*viewer->ops->setup)(viewer);CHKERRQ(ierr);
  }
  viewer->setupcalled = PETSC_TRUE;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscViewerView"
/*@C
   PetscViewerView - Visualizes a viewer object.

   Collective on PetscViewer

   Input Parameters:
+  v - the viewer
-  viewer - visualization context

  Notes:
  The available visualization contexts include
+    PETSC_VIEWER_STDOUT_SELF - standard output (default)
.    PETSC_VIEWER_STDOUT_WORLD - synchronized standard
        output where only the first processor opens
        the file.  All other processors send their
        data to the first processor to print.
-     PETSC_VIEWER_DRAW_WORLD - graphical display of nonzero structure

   Level: beginner

.seealso: PetscViewerPushFormat(), PetscViewerASCIIOpen(), PetscViewerDrawOpen(),
          PetscViewerSocketOpen(), PetscViewerBinaryOpen(), PetscViewerLoad()
@*/
PetscErrorCode  PetscViewerView(PetscViewer v,PetscViewer viewer)
{
  PetscErrorCode    ierr;
  PetscBool         iascii;
  PetscViewerFormat format;
#if defined(PETSC_HAVE_SAWS)
  PetscBool         issaws;
#endif

  PetscFunctionBegin;
  PetscValidHeaderSpecific(v,PETSC_VIEWER_CLASSID,1);
  PetscValidType(v,1);
  if (!viewer) {
    ierr = PetscViewerASCIIGetStdout(PetscObjectComm((PetscObject)v),&viewer);CHKERRQ(ierr);
  }
  PetscValidHeaderSpecific(viewer,PETSC_VIEWER_CLASSID,2);
  PetscCheckSameComm(v,1,viewer,2);

  ierr = PetscObjectTypeCompare((PetscObject)viewer,PETSCVIEWERASCII,&iascii);CHKERRQ(ierr);
#if defined(PETSC_HAVE_SAWS)
  ierr = PetscObjectTypeCompare((PetscObject)viewer,PETSCVIEWERSAWS,&issaws);CHKERRQ(ierr);
#endif
  if (iascii) {
    ierr = PetscViewerGetFormat(viewer,&format);CHKERRQ(ierr);
    ierr = PetscObjectPrintClassNamePrefixType((PetscObject)v,viewer);CHKERRQ(ierr);
    if (format == PETSC_VIEWER_DEFAULT || format == PETSC_VIEWER_ASCII_INFO || format == PETSC_VIEWER_ASCII_INFO_DETAIL) {
      if (v->format) {
        ierr = PetscViewerASCIIPrintf(viewer,"  Viewer format = %s\n",PetscViewerFormats[v->format]);CHKERRQ(ierr);
      }
      ierr = PetscViewerASCIIPushTab(viewer);CHKERRQ(ierr);
      if (v->ops->view) {
        ierr = (*v->ops->view)(v,viewer);CHKERRQ(ierr);
      }
      ierr = PetscViewerASCIIPopTab(viewer);CHKERRQ(ierr);
    }
#if defined(PETSC_HAVE_SAWS)
  } else if (issaws) {
    if (!((PetscObject)v)->amsmem) {
      ierr = PetscObjectViewSAWs((PetscObject)v,viewer);CHKERRQ(ierr);
      if (v->ops->view) {
        ierr = (*v->ops->view)(v,viewer);CHKERRQ(ierr);
      }
    }
#endif
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscViewerRead"
/*@C
   PetscViewerRead - Reads data from a PetscViewer

   Collective on MPI_Comm

   Input Parameters:
+  viewer   - The viewer
.  data     - Location to write the data
.  num      - Number of items of data to read
-  datatype - Type of data to read

   Output Parameters:
.  count - number of items of data actually read, or NULL

   Level: beginner

   Concepts: binary files, ascii files

.seealso: PetscViewerASCIIOpen(), PetscViewerPushFormat(), PetscViewerDestroy(),
          VecView(), MatView(), VecLoad(), MatLoad(), PetscViewerBinaryGetDescriptor(),
          PetscViewerBinaryGetInfoPointer(), PetscFileMode, PetscViewer
@*/
PetscErrorCode  PetscViewerRead(PetscViewer viewer, void *data, PetscInt num, PetscInt *count, PetscDataType dtype)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(viewer,PETSC_VIEWER_CLASSID,1);
  if (dtype == PETSC_STRING) {
    PetscInt c, i = 0, cnt;
    char *s = (char *)data;
    for (c = 0; c < num; c++) {
      /* Skip leading whitespaces */
      do {ierr = (*viewer->ops->read)(viewer, &(s[i]), 1, &cnt, PETSC_CHAR);CHKERRQ(ierr); if (count && !cnt) break;}
      while (s[i]=='\n' || s[i]=='\t' || s[i]==' ' || s[i]=='\0' || s[i]=='\v' || s[i]=='\f' || s[i]=='\r');
      i++;
      /* Read strings one char at a time */
      do {ierr = (*viewer->ops->read)(viewer, &(s[i++]), 1, &cnt, PETSC_CHAR);CHKERRQ(ierr); if (count && !cnt) break;}
      while (s[i-1]!='\n' && s[i-1]!='\t' && s[i-1]!=' ' && s[i-1]!='\0' && s[i-1]!='\v' && s[i-1]!='\f' && s[i-1]!='\r');
      /* Terminate final string */
      if (c == num-1) s[i-1] = '\0';
    }
    if (count) *count = c;
    else if (c < num) SETERRQ2(PetscObjectComm((PetscObject) viewer), PETSC_ERR_FILE_READ, "Insufficient data, only read %D < %D strings", c, num);
  } else {
    ierr = (*viewer->ops->read)(viewer, data, num, count, dtype);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}
