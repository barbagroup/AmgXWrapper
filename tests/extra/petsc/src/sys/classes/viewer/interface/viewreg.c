
#include <petsc/private/viewerimpl.h>  /*I "petscviewer.h" I*/
#if defined(PETSC_HAVE_SAWS)
#include <petscviewersaws.h>
#endif

PetscFunctionList PetscViewerList = 0;

#undef __FUNCT__
#define __FUNCT__ "PetscOptionsGetViewer"
/*@C
   PetscOptionsGetViewer - Gets a viewer appropriate for the type indicated by the user

   Collective on MPI_Comm

   Input Parameters:
+  comm - the communicator to own the viewer
.  pre - the string to prepend to the name or NULL
-  name - the option one is seeking

   Output Parameter:
+  viewer - the viewer, pass NULL if not needed
.  format - the PetscViewerFormat requested by the user, pass NULL if not needed
-  set - PETSC_TRUE if found, else PETSC_FALSE

   Level: intermediate

   Notes: If no value is provided ascii:stdout is used
$       ascii[:[filename][:[format][:append]]]    defaults to stdout - format can be one of ascii_info, ascii_info_detail, or ascii_matlab, 
                                                  for example ascii::ascii_info prints just the information about the object not all details
                                                  unless :append is given filename opens in write mode, overwriting what was already there
$       binary[:[filename][:[format][:append]]]   defaults to the file binaryoutput
$       draw[:drawtype]                           for example, draw:tikz  or draw:x
$       socket[:port]                             defaults to the standard output port
$       saws[:communicatorname]                    publishes object to the Scientific Application Webserver (SAWs)

   Use PetscViewerDestroy() after using the viewer, otherwise a memory leak will occur

.seealso: PetscOptionsGetReal(), PetscOptionsHasName(), PetscOptionsGetString(),
          PetscOptionsGetIntArray(), PetscOptionsGetRealArray(), PetscOptionsBool()
          PetscOptionsInt(), PetscOptionsString(), PetscOptionsReal(), PetscOptionsBool(),
          PetscOptionsName(), PetscOptionsBegin(), PetscOptionsEnd(), PetscOptionsHead(),
          PetscOptionsStringArray(),PetscOptionsRealArray(), PetscOptionsScalar(),
          PetscOptionsBoolGroupBegin(), PetscOptionsBoolGroup(), PetscOptionsBoolGroupEnd(),
          PetscOptionsFList(), PetscOptionsEList()
@*/
PetscErrorCode  PetscOptionsGetViewer(MPI_Comm comm,const char pre[],const char name[],PetscViewer *viewer,PetscViewerFormat *format,PetscBool  *set)
{
  char           *value;
  PetscErrorCode ierr;
  PetscBool      flag,hashelp;

  PetscFunctionBegin;
  PetscValidCharPointer(name,3);

  ierr = PetscOptionsHasName(NULL,NULL,"-help",&hashelp);CHKERRQ(ierr);
  if (hashelp) {
    ierr = (*PetscHelpPrintf)(comm,"  -%s%s ascii[:[filename][:[format][:append]]]: %s (%s)\n",pre ? pre : "",name+1,"Triggers display of a PETSc object to screen or ASCII file","PetscOptionsGetViewer");CHKERRQ(ierr);
    ierr = (*PetscHelpPrintf)(comm,"  -%s%s binary[:[filename][:[format][:append]]]: %s (%s)\n",pre ? pre : "",name+1,"Triggers saving of a PETSc object to a binary file","PetscOptionsGetViewer");CHKERRQ(ierr);
    ierr = (*PetscHelpPrintf)(comm,"  -%s%s draw[:drawtype]: %s (%s)\n",pre ? pre : "",name+1,"Triggers drawing of a PETSc object","PetscOptionsGetViewer");CHKERRQ(ierr);
    ierr = (*PetscHelpPrintf)(comm,"  -%s%s socket[:port]: %s (%s)\n",pre ? pre : "",name+1,"Triggers push of a PETSc object to a Unix socket","PetscOptionsGetViewer");CHKERRQ(ierr);
    ierr = (*PetscHelpPrintf)(comm,"  -%s%s saws[:communicatorname]: %s (%s)\n",pre ? pre : "",name+1,"Triggers publishing of a PETSc object to SAWs","PetscOptionsGetViewer");CHKERRQ(ierr);
  }

  if (format) *format = PETSC_VIEWER_DEFAULT;
  if (set) *set = PETSC_FALSE;
  ierr = PetscOptionsFindPair_Private(NULL,pre,name,&value,&flag);CHKERRQ(ierr);
  if (flag) {
    if (set) *set = PETSC_TRUE;
    if (!value) {
      if (viewer) {
        ierr = PetscViewerASCIIGetStdout(comm,viewer);CHKERRQ(ierr);
        ierr = PetscObjectReference((PetscObject)*viewer);CHKERRQ(ierr);
      }
    } else {
      char       *loc0_vtype,*loc1_fname,*loc2_fmt = NULL,*loc3_fmode = NULL;
      PetscInt   cnt;
      const char *viewers[] = {PETSCVIEWERASCII,PETSCVIEWERBINARY,PETSCVIEWERDRAW,PETSCVIEWERSOCKET,PETSCVIEWERMATLAB,PETSCVIEWERSAWS,PETSCVIEWERVTK,PETSCVIEWERHDF5,0};

      ierr = PetscStrallocpy(value,&loc0_vtype);CHKERRQ(ierr);
      ierr = PetscStrchr(loc0_vtype,':',&loc1_fname);CHKERRQ(ierr);
      if (loc1_fname) {
        *loc1_fname++ = 0;
        ierr = PetscStrchr(loc1_fname,':',&loc2_fmt);CHKERRQ(ierr);
      }
      if (loc2_fmt) {
        *loc2_fmt++ = 0;
        ierr = PetscStrchr(loc2_fmt,':',&loc3_fmode);CHKERRQ(ierr);
      }
      if (loc3_fmode) *loc3_fmode++ = 0;
      ierr = PetscStrendswithwhich(*loc0_vtype ? loc0_vtype : "ascii",viewers,&cnt);CHKERRQ(ierr);
      if (cnt > (PetscInt) sizeof(viewers)-1) SETERRQ1(comm,PETSC_ERR_ARG_OUTOFRANGE,"Unknown viewer type: %s",loc0_vtype);
      if (viewer) {
        if (!loc1_fname) {
          switch (cnt) {
          case 0:
            ierr = PetscViewerASCIIGetStdout(comm,viewer);CHKERRQ(ierr);
            break;
          case 1:
            if (!(*viewer = PETSC_VIEWER_BINARY_(comm))) CHKERRQ(PETSC_ERR_PLIB);
            break;
          case 2:
            if (!(*viewer = PETSC_VIEWER_DRAW_(comm))) CHKERRQ(PETSC_ERR_PLIB);
            break;
#if defined(PETSC_USE_SOCKET_VIEWER)
          case 3:
            if (!(*viewer = PETSC_VIEWER_SOCKET_(comm))) CHKERRQ(PETSC_ERR_PLIB);
            break;
#endif
#if defined(PETSC_HAVE_MATLAB_ENGINE)
          case 4:
            if (!(*viewer = PETSC_VIEWER_MATLAB_(comm))) CHKERRQ(PETSC_ERR_PLIB);
            break;
#endif
#if defined(PETSC_HAVE_SAWS)
          case 5:
            if (!(*viewer = PETSC_VIEWER_SAWS_(comm))) CHKERRQ(PETSC_ERR_PLIB);
            break;
#endif
#if defined(PETSC_HAVE_HDF5)
          case 7:
            if (!(*viewer = PETSC_VIEWER_HDF5_(comm))) CHKERRQ(PETSC_ERR_PLIB);
            break;
#endif
          default: SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_SUP,"Unsupported viewer %s",loc0_vtype);
          }
          ierr = PetscObjectReference((PetscObject)*viewer);CHKERRQ(ierr);
        } else {
          if (loc2_fmt && !*loc1_fname && (cnt == 0)) { /* ASCII format without file name */
            ierr = PetscViewerASCIIGetStdout(comm,viewer);CHKERRQ(ierr);
            ierr = PetscObjectReference((PetscObject)*viewer);CHKERRQ(ierr);
          } else {
            PetscFileMode fmode;
            ierr = PetscViewerCreate(comm,viewer);CHKERRQ(ierr);
            ierr = PetscViewerSetType(*viewer,*loc0_vtype ? loc0_vtype : "ascii");CHKERRQ(ierr);
            fmode = FILE_MODE_WRITE;
            if (loc3_fmode && *loc3_fmode) { /* Has non-empty file mode ("write" or "append") */
              ierr = PetscEnumFind(PetscFileModes,loc3_fmode,(PetscEnum*)&fmode,&flag);CHKERRQ(ierr);
              if (!flag) SETERRQ1(comm,PETSC_ERR_ARG_UNKNOWN_TYPE,"Unknown file mode: %s",loc3_fmode);
            }
            ierr = PetscViewerFileSetMode(*viewer,flag?fmode:FILE_MODE_WRITE);CHKERRQ(ierr);
            ierr = PetscViewerFileSetName(*viewer,loc1_fname);CHKERRQ(ierr);
            ierr = PetscViewerDrawSetDrawType(*viewer,loc1_fname);CHKERRQ(ierr);
          }
        }
      }
      if (viewer) {
        ierr = PetscViewerSetUp(*viewer);CHKERRQ(ierr);
      }
      if (loc2_fmt && *loc2_fmt) {
        ierr = PetscEnumFind(PetscViewerFormats,loc2_fmt,(PetscEnum*)format,&flag);CHKERRQ(ierr);
        if (!flag) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_SUP,"Unknown viewer format %s",loc2_fmt);CHKERRQ(ierr);
      }
      ierr = PetscFree(loc0_vtype);CHKERRQ(ierr);
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscViewerCreate"
/*@
   PetscViewerCreate - Creates a viewing context

   Collective on MPI_Comm

   Input Parameter:
.  comm - MPI communicator

   Output Parameter:
.  inviewer - location to put the PetscViewer context

   Level: advanced

   Concepts: graphics^creating PetscViewer
   Concepts: file input/output^creating PetscViewer
   Concepts: sockets^creating PetscViewer

.seealso: PetscViewerDestroy(), PetscViewerSetType(), PetscViewerType

@*/
PetscErrorCode  PetscViewerCreate(MPI_Comm comm,PetscViewer *inviewer)
{
  PetscViewer    viewer;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  *inviewer = 0;
  ierr = PetscViewerInitializePackage();CHKERRQ(ierr);
  ierr         = PetscHeaderCreate(viewer,PETSC_VIEWER_CLASSID,"PetscViewer","PetscViewer","Viewer",comm,PetscViewerDestroy,NULL);CHKERRQ(ierr);
  *inviewer    = viewer;
  viewer->data = 0;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscViewerSetType"
/*@C
   PetscViewerSetType - Builds PetscViewer for a particular implementation.

   Collective on PetscViewer

   Input Parameter:
+  viewer      - the PetscViewer context
-  type        - for example, PETSCVIEWERASCII

   Options Database Command:
.  -draw_type  <type> - Sets the type; use -help for a list
    of available methods (for instance, ascii)

   Level: advanced

   Notes:
   See "include/petscviewer.h" for available methods (for instance,
   PETSCVIEWERSOCKET)

.seealso: PetscViewerCreate(), PetscViewerGetType(), PetscViewerType, PetscViewerPushFormat()
@*/
PetscErrorCode  PetscViewerSetType(PetscViewer viewer,PetscViewerType type)
{
  PetscErrorCode ierr,(*r)(PetscViewer);
  PetscBool      match;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(viewer,PETSC_VIEWER_CLASSID,1);
  PetscValidCharPointer(type,2);
  ierr = PetscObjectTypeCompare((PetscObject)viewer,type,&match);CHKERRQ(ierr);
  if (match) PetscFunctionReturn(0);

  /* cleanup any old type that may be there */
  if (viewer->data) {
    ierr         = (*viewer->ops->destroy)(viewer);CHKERRQ(ierr);

    viewer->ops->destroy = NULL;
    viewer->data         = 0;
  }
  ierr = PetscMemzero(viewer->ops,sizeof(struct _PetscViewerOps));CHKERRQ(ierr);

  ierr =  PetscFunctionListFind(PetscViewerList,type,&r);CHKERRQ(ierr);
  if (!r) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_ARG_UNKNOWN_TYPE,"Unknown PetscViewer type given: %s",type);

  ierr = PetscObjectChangeTypeName((PetscObject)viewer,type);CHKERRQ(ierr);
  ierr = (*r)(viewer);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscViewerRegister"
/*@C
   PetscViewerRegister - Adds a viewer

   Not Collective

   Input Parameters:
+  name_solver - name of a new user-defined viewer
-  routine_create - routine to create method context

   Level: developer
   Notes:
   PetscViewerRegister() may be called multiple times to add several user-defined viewers.

   Sample usage:
.vb
   PetscViewerRegister("my_viewer_type",MyViewerCreate);
.ve

   Then, your solver can be chosen with the procedural interface via
$     PetscViewerSetType(viewer,"my_viewer_type")
   or at runtime via the option
$     -viewer_type my_viewer_type

  Concepts: registering^Viewers

.seealso: PetscViewerRegisterAll(), PetscViewerRegisterDestroy()
 @*/
PetscErrorCode  PetscViewerRegister(const char *sname,PetscErrorCode (*function)(PetscViewer))
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscFunctionListAdd(&PetscViewerList,sname,function);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscViewerSetFromOptions"
/*@C
   PetscViewerSetFromOptions - Sets the graphics type from the options database.
      Defaults to a PETSc X windows graphics.

   Collective on PetscViewer

   Input Parameter:
.     PetscViewer - the graphics context

   Level: intermediate

   Notes:
    Must be called after PetscViewerCreate() before the PetscViewer is used.

  Concepts: PetscViewer^setting options

.seealso: PetscViewerCreate(), PetscViewerSetType(), PetscViewerType

@*/
PetscErrorCode  PetscViewerSetFromOptions(PetscViewer viewer)
{
  PetscErrorCode    ierr;
  char              vtype[256];
  PetscBool         flg;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(viewer,PETSC_VIEWER_CLASSID,1);

  if (!PetscViewerList) {
    ierr = PetscViewerRegisterAll();CHKERRQ(ierr);
  }
  ierr = PetscObjectOptionsBegin((PetscObject)viewer);CHKERRQ(ierr);
  ierr = PetscOptionsFList("-viewer_type","Type of PetscViewer","None",PetscViewerList,(char*)(((PetscObject)viewer)->type_name ? ((PetscObject)viewer)->type_name : PETSCVIEWERASCII),vtype,256,&flg);CHKERRQ(ierr);
  if (flg) {
    ierr = PetscViewerSetType(viewer,vtype);CHKERRQ(ierr);
  }
  /* type has not been set? */
  if (!((PetscObject)viewer)->type_name) {
    ierr = PetscViewerSetType(viewer,PETSCVIEWERASCII);CHKERRQ(ierr);
  }
  if (viewer->ops->setfromoptions) {
    ierr = (*viewer->ops->setfromoptions)(PetscOptionsObject,viewer);CHKERRQ(ierr);
  }

  /* process any options handlers added with PetscObjectAddOptionsHandler() */
  ierr = PetscObjectProcessOptionsHandlers(PetscOptionsObject,(PetscObject)viewer);CHKERRQ(ierr);
  ierr = PetscViewerViewFromOptions(viewer,NULL,"-viewer_view");CHKERRQ(ierr);
  ierr = PetscOptionsEnd();CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscViewerFlowControlStart"
PetscErrorCode PetscViewerFlowControlStart(PetscViewer viewer,PetscInt *mcnt,PetscInt *cnt)
{
  PetscErrorCode ierr;
  PetscFunctionBegin;
  ierr = PetscViewerBinaryGetFlowControl(viewer,mcnt);CHKERRQ(ierr);
  ierr = PetscViewerBinaryGetFlowControl(viewer,cnt);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscViewerFlowControlStepMaster"
PetscErrorCode PetscViewerFlowControlStepMaster(PetscViewer viewer,PetscInt i,PetscInt *mcnt,PetscInt cnt)
{
  PetscErrorCode ierr;
  MPI_Comm       comm;

  PetscFunctionBegin;
  ierr = PetscObjectGetComm((PetscObject)viewer,&comm);CHKERRQ(ierr);
  if (i >= *mcnt) {
    *mcnt += cnt;
    ierr = MPI_Bcast(mcnt,1,MPIU_INT,0,comm);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscViewerFlowControlEndMaster"
PetscErrorCode PetscViewerFlowControlEndMaster(PetscViewer viewer,PetscInt *mcnt)
{
  PetscErrorCode ierr;
  MPI_Comm       comm;
  PetscFunctionBegin;
  ierr = PetscObjectGetComm((PetscObject)viewer,&comm);CHKERRQ(ierr);
  *mcnt = 0;
  ierr = MPI_Bcast(mcnt,1,MPIU_INT,0,comm);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscViewerFlowControlStepWorker"
PetscErrorCode PetscViewerFlowControlStepWorker(PetscViewer viewer,PetscMPIInt rank,PetscInt *mcnt)
{
  PetscErrorCode ierr;
  MPI_Comm       comm;
  PetscFunctionBegin;
  ierr = PetscObjectGetComm((PetscObject)viewer,&comm);CHKERRQ(ierr);
  while (PETSC_TRUE) { 
    if (rank < *mcnt) break;
    ierr = MPI_Bcast(mcnt,1,MPIU_INT,0,comm);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscViewerFlowControlEndWorker"
PetscErrorCode PetscViewerFlowControlEndWorker(PetscViewer viewer,PetscInt *mcnt)
{
  PetscErrorCode ierr;
  MPI_Comm       comm;
  PetscFunctionBegin;
  ierr = PetscObjectGetComm((PetscObject)viewer,&comm);CHKERRQ(ierr);
  while (PETSC_TRUE) {
    ierr = MPI_Bcast(mcnt,1,MPIU_INT,0,comm);CHKERRQ(ierr);
    if (!*mcnt) break;
  }
  PetscFunctionReturn(0);
}
