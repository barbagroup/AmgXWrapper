
/*
     Code for some of the parallel vector primatives.
*/
#include <../src/vec/vec/impls/mpi/pvecimpl.h>   /*I  "petscvec.h"   I*/
#include <petscviewerhdf5.h>

#undef __FUNCT__
#define __FUNCT__ "VecDestroy_MPI"
PetscErrorCode VecDestroy_MPI(Vec v)
{
  Vec_MPI        *x = (Vec_MPI*)v->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
#if defined(PETSC_USE_LOG)
  PetscLogObjectState((PetscObject)v,"Length=%D",v->map->N);
#endif
  if (!x) PetscFunctionReturn(0);
  ierr = PetscFree(x->array_allocated);CHKERRQ(ierr);

  /* Destroy local representation of vector if it exists */
  if (x->localrep) {
    ierr = VecDestroy(&x->localrep);CHKERRQ(ierr);
    ierr = VecScatterDestroy(&x->localupdate);CHKERRQ(ierr);
  }
  ierr = VecAssemblyReset_MPI(v);CHKERRQ(ierr);

  /* Destroy the stashes: note the order - so that the tags are freed properly */
  ierr = VecStashDestroy_Private(&v->bstash);CHKERRQ(ierr);
  ierr = VecStashDestroy_Private(&v->stash);CHKERRQ(ierr);
  ierr = PetscFree(v->data);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecView_MPI_ASCII"
PetscErrorCode VecView_MPI_ASCII(Vec xin,PetscViewer viewer)
{
  PetscErrorCode    ierr;
  PetscInt          i,work = xin->map->n,cnt,len,nLen;
  PetscMPIInt       j,n = 0,size,rank,tag = ((PetscObject)viewer)->tag;
  MPI_Status        status;
  PetscScalar       *values;
  const PetscScalar *xarray;
  const char        *name;
  PetscViewerFormat format;

  PetscFunctionBegin;
  ierr = VecGetArrayRead(xin,&xarray);CHKERRQ(ierr);
  /* determine maximum message to arrive */
  ierr = MPI_Comm_rank(PetscObjectComm((PetscObject)xin),&rank);CHKERRQ(ierr);
  ierr = MPI_Reduce(&work,&len,1,MPIU_INT,MPI_MAX,0,PetscObjectComm((PetscObject)xin));CHKERRQ(ierr);
  ierr = MPI_Comm_size(PetscObjectComm((PetscObject)xin),&size);CHKERRQ(ierr);

  ierr = PetscViewerGetFormat(viewer,&format);CHKERRQ(ierr);
  if (!rank) {
    ierr = PetscMalloc1(len,&values);CHKERRQ(ierr);
    /*
        MATLAB format and ASCII format are very similar except
        MATLAB uses %18.16e format while ASCII uses %g
    */
    if (format == PETSC_VIEWER_ASCII_MATLAB) {
      ierr = PetscObjectGetName((PetscObject)xin,&name);CHKERRQ(ierr);
      ierr = PetscViewerASCIIPrintf(viewer,"%s = [\n",name);CHKERRQ(ierr);
      for (i=0; i<xin->map->n; i++) {
#if defined(PETSC_USE_COMPLEX)
        if (PetscImaginaryPart(xarray[i]) > 0.0) {
          ierr = PetscViewerASCIIPrintf(viewer,"%18.16e + %18.16ei\n",(double)PetscRealPart(xarray[i]),(double)PetscImaginaryPart(xarray[i]));CHKERRQ(ierr);
        } else if (PetscImaginaryPart(xarray[i]) < 0.0) {
          ierr = PetscViewerASCIIPrintf(viewer,"%18.16e - %18.16ei\n",(double)PetscRealPart(xarray[i]),-(double)PetscImaginaryPart(xarray[i]));CHKERRQ(ierr);
        } else {
          ierr = PetscViewerASCIIPrintf(viewer,"%18.16e\n",(double)PetscRealPart(xarray[i]));CHKERRQ(ierr);
        }
#else
        ierr = PetscViewerASCIIPrintf(viewer,"%18.16e\n",(double)xarray[i]);CHKERRQ(ierr);
#endif
      }
      /* receive and print messages */
      for (j=1; j<size; j++) {
        ierr = MPI_Recv(values,(PetscMPIInt)len,MPIU_SCALAR,j,tag,PetscObjectComm((PetscObject)xin),&status);CHKERRQ(ierr);
        ierr = MPI_Get_count(&status,MPIU_SCALAR,&n);CHKERRQ(ierr);
        for (i=0; i<n; i++) {
#if defined(PETSC_USE_COMPLEX)
          if (PetscImaginaryPart(values[i]) > 0.0) {
            ierr = PetscViewerASCIIPrintf(viewer,"%18.16e + %18.16ei\n",(double)PetscRealPart(values[i]),(double)PetscImaginaryPart(values[i]));CHKERRQ(ierr);
          } else if (PetscImaginaryPart(values[i]) < 0.0) {
            ierr = PetscViewerASCIIPrintf(viewer,"%18.16e - %18.16ei\n",(double)PetscRealPart(values[i]),-(double)PetscImaginaryPart(values[i]));CHKERRQ(ierr);
          } else {
            ierr = PetscViewerASCIIPrintf(viewer,"%18.16e\n",(double)PetscRealPart(values[i]));CHKERRQ(ierr);
          }
#else
          ierr = PetscViewerASCIIPrintf(viewer,"%18.16e\n",(double)values[i]);CHKERRQ(ierr);
#endif
        }
      }
      ierr = PetscViewerASCIIPrintf(viewer,"];\n");CHKERRQ(ierr);

    } else if (format == PETSC_VIEWER_ASCII_SYMMODU) {
      for (i=0; i<xin->map->n; i++) {
#if defined(PETSC_USE_COMPLEX)
        ierr = PetscViewerASCIIPrintf(viewer,"%18.16e %18.16e\n",(double)PetscRealPart(xarray[i]),(double)PetscImaginaryPart(xarray[i]));CHKERRQ(ierr);
#else
        ierr = PetscViewerASCIIPrintf(viewer,"%18.16e\n",(double)xarray[i]);CHKERRQ(ierr);
#endif
      }
      /* receive and print messages */
      for (j=1; j<size; j++) {
        ierr = MPI_Recv(values,(PetscMPIInt)len,MPIU_SCALAR,j,tag,PetscObjectComm((PetscObject)xin),&status);CHKERRQ(ierr);
        ierr = MPI_Get_count(&status,MPIU_SCALAR,&n);CHKERRQ(ierr);
        for (i=0; i<n; i++) {
#if defined(PETSC_USE_COMPLEX)
          ierr = PetscViewerASCIIPrintf(viewer,"%18.16e %18.16e\n",(double)PetscRealPart(values[i]),(double)PetscImaginaryPart(values[i]));CHKERRQ(ierr);
#else
          ierr = PetscViewerASCIIPrintf(viewer,"%18.16e\n",(double)values[i]);CHKERRQ(ierr);
#endif
        }
      }
    } else if (format == PETSC_VIEWER_ASCII_VTK || format == PETSC_VIEWER_ASCII_VTK_CELL) {
      /*
        state 0: No header has been output
        state 1: Only POINT_DATA has been output
        state 2: Only CELL_DATA has been output
        state 3: Output both, POINT_DATA last
        state 4: Output both, CELL_DATA last
      */
      static PetscInt stateId     = -1;
      int             outputState = 0;
      int             doOutput    = 0;
      PetscBool       hasState;
      PetscInt        bs, b;

      if (stateId < 0) {
        ierr = PetscObjectComposedDataRegister(&stateId);CHKERRQ(ierr);
      }
      ierr = PetscObjectComposedDataGetInt((PetscObject) viewer, stateId, outputState, hasState);CHKERRQ(ierr);
      if (!hasState) outputState = 0;

      ierr = PetscObjectGetName((PetscObject)xin,&name);CHKERRQ(ierr);
      ierr = VecGetLocalSize(xin, &nLen);CHKERRQ(ierr);
      ierr = PetscMPIIntCast(nLen,&n);CHKERRQ(ierr);
      ierr = VecGetBlockSize(xin, &bs);CHKERRQ(ierr);
      if (format == PETSC_VIEWER_ASCII_VTK) {
        if (outputState == 0) {
          outputState = 1;
          doOutput    = 1;
        } else if (outputState == 1) doOutput = 0;
        else if (outputState == 2) {
          outputState = 3;
          doOutput    = 1;
        } else if (outputState == 3) doOutput = 0;
        else if (outputState == 4) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONGSTATE, "Tried to output POINT_DATA again after intervening CELL_DATA");

        if (doOutput) {
          ierr = PetscViewerASCIIPrintf(viewer, "POINT_DATA %d\n", xin->map->N/bs);CHKERRQ(ierr);
        }
      } else {
        if (outputState == 0) {
          outputState = 2;
          doOutput    = 1;
        } else if (outputState == 1) {
          outputState = 4;
          doOutput    = 1;
        } else if (outputState == 2) doOutput = 0;
        else if (outputState == 3) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONGSTATE, "Tried to output CELL_DATA again after intervening POINT_DATA");
        else if (outputState == 4) doOutput = 0;

        if (doOutput) {
          ierr = PetscViewerASCIIPrintf(viewer, "CELL_DATA %d\n", xin->map->N/bs);CHKERRQ(ierr);
        }
      }
      ierr = PetscObjectComposedDataSetInt((PetscObject) viewer, stateId, outputState);CHKERRQ(ierr);
      if (name) {
        if (bs == 3) {
          ierr = PetscViewerASCIIPrintf(viewer, "VECTORS %s double\n", name);CHKERRQ(ierr);
        } else {
          ierr = PetscViewerASCIIPrintf(viewer, "SCALARS %s double %d\n", name, bs);CHKERRQ(ierr);
        }
      } else {
        ierr = PetscViewerASCIIPrintf(viewer, "SCALARS scalars double %d\n", bs);CHKERRQ(ierr);
      }
      if (bs != 3) {
        ierr = PetscViewerASCIIPrintf(viewer, "LOOKUP_TABLE default\n");CHKERRQ(ierr);
      }
      for (i=0; i<n/bs; i++) {
        for (b=0; b<bs; b++) {
          if (b > 0) {
            ierr = PetscViewerASCIIPrintf(viewer," ");CHKERRQ(ierr);
          }
          ierr = PetscViewerASCIIPrintf(viewer,"%g",(double)PetscRealPart(xarray[i*bs+b]));CHKERRQ(ierr);
        }
        ierr = PetscViewerASCIIPrintf(viewer,"\n");CHKERRQ(ierr);
      }
      for (j=1; j<size; j++) {
        ierr = MPI_Recv(values,(PetscMPIInt)len,MPIU_SCALAR,j,tag,PetscObjectComm((PetscObject)xin),&status);CHKERRQ(ierr);
        ierr = MPI_Get_count(&status,MPIU_SCALAR,&n);CHKERRQ(ierr);
        for (i=0; i<n/bs; i++) {
          for (b=0; b<bs; b++) {
            if (b > 0) {
              ierr = PetscViewerASCIIPrintf(viewer," ");CHKERRQ(ierr);
            }
            ierr = PetscViewerASCIIPrintf(viewer,"%g",(double)PetscRealPart(values[i*bs+b]));CHKERRQ(ierr);
          }
          ierr = PetscViewerASCIIPrintf(viewer,"\n");CHKERRQ(ierr);
        }
      }
    } else if (format == PETSC_VIEWER_ASCII_VTK_COORDS) {
      PetscInt bs, b;

      ierr = VecGetLocalSize(xin, &nLen);CHKERRQ(ierr);
      ierr = PetscMPIIntCast(nLen,&n);CHKERRQ(ierr);
      ierr = VecGetBlockSize(xin, &bs);CHKERRQ(ierr);
      if ((bs < 1) || (bs > 3)) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONGSTATE, "VTK can only handle 3D objects, but vector dimension is %d", bs);

      for (i=0; i<n/bs; i++) {
        for (b=0; b<bs; b++) {
          if (b > 0) {
            ierr = PetscViewerASCIIPrintf(viewer," ");CHKERRQ(ierr);
          }
          ierr = PetscViewerASCIIPrintf(viewer,"%g",(double)PetscRealPart(xarray[i*bs+b]));CHKERRQ(ierr);
        }
        for (b=bs; b<3; b++) {
          ierr = PetscViewerASCIIPrintf(viewer," 0.0");CHKERRQ(ierr);
        }
        ierr = PetscViewerASCIIPrintf(viewer,"\n");CHKERRQ(ierr);
      }
      for (j=1; j<size; j++) {
        ierr = MPI_Recv(values,(PetscMPIInt)len,MPIU_SCALAR,j,tag,PetscObjectComm((PetscObject)xin),&status);CHKERRQ(ierr);
        ierr = MPI_Get_count(&status,MPIU_SCALAR,&n);CHKERRQ(ierr);
        for (i=0; i<n/bs; i++) {
          for (b=0; b<bs; b++) {
            if (b > 0) {
              ierr = PetscViewerASCIIPrintf(viewer," ");CHKERRQ(ierr);
            }
            ierr = PetscViewerASCIIPrintf(viewer,"%g",(double)PetscRealPart(values[i*bs+b]));CHKERRQ(ierr);
          }
          for (b=bs; b<3; b++) {
            ierr = PetscViewerASCIIPrintf(viewer," 0.0");CHKERRQ(ierr);
          }
          ierr = PetscViewerASCIIPrintf(viewer,"\n");CHKERRQ(ierr);
        }
      }
    } else if (format == PETSC_VIEWER_ASCII_PCICE) {
      PetscInt bs, b, vertexCount = 1;

      ierr = VecGetLocalSize(xin, &nLen);CHKERRQ(ierr);
      ierr = PetscMPIIntCast(nLen,&n);CHKERRQ(ierr);
      ierr = VecGetBlockSize(xin, &bs);CHKERRQ(ierr);
      if ((bs < 1) || (bs > 3)) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONGSTATE, "PCICE can only handle up to 3D objects, but vector dimension is %d", bs);

      ierr = PetscViewerASCIIPrintf(viewer,"%D\n", xin->map->N/bs);CHKERRQ(ierr);
      for (i=0; i<n/bs; i++) {
        ierr = PetscViewerASCIIPrintf(viewer,"%7D   ", vertexCount++);CHKERRQ(ierr);
        for (b=0; b<bs; b++) {
          if (b > 0) {
            ierr = PetscViewerASCIIPrintf(viewer," ");CHKERRQ(ierr);
          }
#if !defined(PETSC_USE_COMPLEX)
          ierr = PetscViewerASCIIPrintf(viewer,"% 12.5E",(double)xarray[i*bs+b]);CHKERRQ(ierr);
#endif
        }
        ierr = PetscViewerASCIIPrintf(viewer,"\n");CHKERRQ(ierr);
      }
      for (j=1; j<size; j++) {
        ierr = MPI_Recv(values,(PetscMPIInt)len,MPIU_SCALAR,j,tag,PetscObjectComm((PetscObject)xin),&status);CHKERRQ(ierr);
        ierr = MPI_Get_count(&status,MPIU_SCALAR,&n);CHKERRQ(ierr);
        for (i=0; i<n/bs; i++) {
          ierr = PetscViewerASCIIPrintf(viewer,"%7D   ", vertexCount++);CHKERRQ(ierr);
          for (b=0; b<bs; b++) {
            if (b > 0) {
              ierr = PetscViewerASCIIPrintf(viewer," ");CHKERRQ(ierr);
            }
#if !defined(PETSC_USE_COMPLEX)
            ierr = PetscViewerASCIIPrintf(viewer,"% 12.5E",(double)values[i*bs+b]);CHKERRQ(ierr);
#endif
          }
          ierr = PetscViewerASCIIPrintf(viewer,"\n");CHKERRQ(ierr);
        }
      }
    } else if (format == PETSC_VIEWER_ASCII_INFO || format == PETSC_VIEWER_ASCII_INFO_DETAIL) {
      /* No info */
    } else {
      if (format != PETSC_VIEWER_ASCII_COMMON) {ierr = PetscViewerASCIIPrintf(viewer,"Process [%d]\n",rank);CHKERRQ(ierr);}
      cnt = 0;
      for (i=0; i<xin->map->n; i++) {
        if (format == PETSC_VIEWER_ASCII_INDEX) {
          ierr = PetscViewerASCIIPrintf(viewer,"%D: ",cnt++);CHKERRQ(ierr);
        }
#if defined(PETSC_USE_COMPLEX)
        if (PetscImaginaryPart(xarray[i]) > 0.0) {
          ierr = PetscViewerASCIIPrintf(viewer,"%g + %g i\n",(double)PetscRealPart(xarray[i]),(double)PetscImaginaryPart(xarray[i]));CHKERRQ(ierr);
        } else if (PetscImaginaryPart(xarray[i]) < 0.0) {
          ierr = PetscViewerASCIIPrintf(viewer,"%g - %g i\n",(double)PetscRealPart(xarray[i]),-(double)PetscImaginaryPart(xarray[i]));CHKERRQ(ierr);
        } else {
          ierr = PetscViewerASCIIPrintf(viewer,"%g\n",(double)PetscRealPart(xarray[i]));CHKERRQ(ierr);
        }
#else
        ierr = PetscViewerASCIIPrintf(viewer,"%g\n",(double)xarray[i]);CHKERRQ(ierr);
#endif
      }
      /* receive and print messages */
      for (j=1; j<size; j++) {
        ierr = MPI_Recv(values,(PetscMPIInt)len,MPIU_SCALAR,j,tag,PetscObjectComm((PetscObject)xin),&status);CHKERRQ(ierr);
        ierr = MPI_Get_count(&status,MPIU_SCALAR,&n);CHKERRQ(ierr);
        if (format != PETSC_VIEWER_ASCII_COMMON) {
          ierr = PetscViewerASCIIPrintf(viewer,"Process [%d]\n",j);CHKERRQ(ierr);
        }
        for (i=0; i<n; i++) {
          if (format == PETSC_VIEWER_ASCII_INDEX) {
            ierr = PetscViewerASCIIPrintf(viewer,"%D: ",cnt++);CHKERRQ(ierr);
          }
#if defined(PETSC_USE_COMPLEX)
          if (PetscImaginaryPart(values[i]) > 0.0) {
            ierr = PetscViewerASCIIPrintf(viewer,"%g + %g i\n",(double)PetscRealPart(values[i]),(double)PetscImaginaryPart(values[i]));CHKERRQ(ierr);
          } else if (PetscImaginaryPart(values[i]) < 0.0) {
            ierr = PetscViewerASCIIPrintf(viewer,"%g - %g i\n",(double)PetscRealPart(values[i]),-(double)PetscImaginaryPart(values[i]));CHKERRQ(ierr);
          } else {
            ierr = PetscViewerASCIIPrintf(viewer,"%g\n",(double)PetscRealPart(values[i]));CHKERRQ(ierr);
          }
#else
          ierr = PetscViewerASCIIPrintf(viewer,"%g\n",(double)values[i]);CHKERRQ(ierr);
#endif
        }
      }
    }
    ierr = PetscFree(values);CHKERRQ(ierr);
  } else {
    if (format == PETSC_VIEWER_ASCII_INFO || format == PETSC_VIEWER_ASCII_INFO_DETAIL) {
      /* Rank 0 is not trying to receive anything, so don't send anything */
    } else {
      if (format == PETSC_VIEWER_ASCII_MATLAB || format == PETSC_VIEWER_ASCII_VTK || format == PETSC_VIEWER_ASCII_VTK_CELL) {
        /* this may be a collective operation so make sure everyone calls it */
        ierr = PetscObjectGetName((PetscObject)xin,&name);CHKERRQ(ierr);
      }
      ierr = MPI_Send((void*)xarray,xin->map->n,MPIU_SCALAR,0,tag,PetscObjectComm((PetscObject)xin));CHKERRQ(ierr);
    }
  }
  ierr = PetscViewerFlush(viewer);CHKERRQ(ierr);
  ierr = VecRestoreArrayRead(xin,&xarray);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecView_MPI_Binary"
PetscErrorCode VecView_MPI_Binary(Vec xin,PetscViewer viewer)
{
  PetscErrorCode    ierr;
  PetscMPIInt       rank,size,mesgsize,tag = ((PetscObject)viewer)->tag, mesglen;
  PetscInt          len,n = xin->map->n,j,tr[2];
  int               fdes;
  MPI_Status        status;
  PetscScalar       *values;
  const PetscScalar *xarray;
  FILE              *file;
#if defined(PETSC_HAVE_MPIIO)
  PetscBool         isMPIIO;
#endif
  PetscBool         skipHeader;
  PetscInt          message_count,flowcontrolcount;
  PetscViewerFormat format;

  PetscFunctionBegin;
  ierr = VecGetArrayRead(xin,&xarray);CHKERRQ(ierr);
  ierr = PetscViewerBinaryGetDescriptor(viewer,&fdes);CHKERRQ(ierr);
  ierr = PetscViewerBinaryGetSkipHeader(viewer,&skipHeader);CHKERRQ(ierr);

  /* determine maximum message to arrive */
  ierr = MPI_Comm_rank(PetscObjectComm((PetscObject)xin),&rank);CHKERRQ(ierr);
  ierr = MPI_Comm_size(PetscObjectComm((PetscObject)xin),&size);CHKERRQ(ierr);

  if (!skipHeader) {
    tr[0] = VEC_FILE_CLASSID;
    tr[1] = xin->map->N;
    ierr  = PetscViewerBinaryWrite(viewer,tr,2,PETSC_INT,PETSC_FALSE);CHKERRQ(ierr);
  }

#if defined(PETSC_HAVE_MPIIO)
  ierr = PetscViewerBinaryGetUseMPIIO(viewer,&isMPIIO);CHKERRQ(ierr);
  if (!isMPIIO) {
#endif
    ierr = PetscViewerFlowControlStart(viewer,&message_count,&flowcontrolcount);CHKERRQ(ierr);
    if (!rank) {
      ierr = PetscBinaryWrite(fdes,(void*)xarray,xin->map->n,PETSC_SCALAR,PETSC_FALSE);CHKERRQ(ierr);

      len = 0;
      for (j=1; j<size; j++) len = PetscMax(len,xin->map->range[j+1]-xin->map->range[j]);
      ierr = PetscMalloc1(len,&values);CHKERRQ(ierr);
      ierr = PetscMPIIntCast(len,&mesgsize);CHKERRQ(ierr);
      /* receive and save messages */
      for (j=1; j<size; j++) {
        ierr = PetscViewerFlowControlStepMaster(viewer,j,&message_count,flowcontrolcount);CHKERRQ(ierr);
        ierr = MPI_Recv(values,mesgsize,MPIU_SCALAR,j,tag,PetscObjectComm((PetscObject)xin),&status);CHKERRQ(ierr);
        ierr = MPI_Get_count(&status,MPIU_SCALAR,&mesglen);CHKERRQ(ierr);
        n    = (PetscInt)mesglen;
        ierr = PetscBinaryWrite(fdes,values,n,PETSC_SCALAR,PETSC_TRUE);CHKERRQ(ierr);
      }
      ierr = PetscViewerFlowControlEndMaster(viewer,&message_count);CHKERRQ(ierr);
      ierr = PetscFree(values);CHKERRQ(ierr);
    } else {
      ierr = PetscViewerFlowControlStepWorker(viewer,rank,&message_count);CHKERRQ(ierr);
      ierr = PetscMPIIntCast(xin->map->n,&mesgsize);CHKERRQ(ierr);
      ierr = MPI_Send((void*)xarray,mesgsize,MPIU_SCALAR,0,tag,PetscObjectComm((PetscObject)xin));CHKERRQ(ierr);
      ierr = PetscViewerFlowControlEndWorker(viewer,&message_count);CHKERRQ(ierr);
    }
    ierr = PetscViewerGetFormat(viewer,&format);CHKERRQ(ierr);
    if (format == PETSC_VIEWER_BINARY_MATLAB) {
      MPI_Comm   comm;
      FILE       *info;
      const char *name;

      ierr = PetscObjectGetName((PetscObject)xin,&name);CHKERRQ(ierr);
      ierr = PetscObjectGetComm((PetscObject)viewer,&comm);CHKERRQ(ierr);
      ierr = PetscViewerBinaryGetInfoPointer(viewer,&info);CHKERRQ(ierr);
      ierr = PetscFPrintf(comm,info,"#--- begin code written by PetscViewerBinary for MATLAB format ---#\n");CHKERRQ(ierr);
      ierr = PetscFPrintf(comm,info,"#$$ Set.%s = PetscBinaryRead(fd);\n",name);CHKERRQ(ierr);
      ierr = PetscFPrintf(comm,info,"#--- end code written by PetscViewerBinary for MATLAB format ---#\n\n");CHKERRQ(ierr);
    }
#if defined(PETSC_HAVE_MPIIO)
  } else {
    MPI_Offset   off;
    MPI_File     mfdes;
    PetscMPIInt  lsize;

    ierr = PetscMPIIntCast(n,&lsize);CHKERRQ(ierr);
    ierr = PetscViewerBinaryGetMPIIODescriptor(viewer,&mfdes);CHKERRQ(ierr);
    ierr = PetscViewerBinaryGetMPIIOOffset(viewer,&off);CHKERRQ(ierr);
    off += xin->map->rstart*sizeof(PetscScalar); /* off is MPI_Offset, not PetscMPIInt */
    ierr = MPI_File_set_view(mfdes,off,MPIU_SCALAR,MPIU_SCALAR,(char*)"native",MPI_INFO_NULL);CHKERRQ(ierr);
    ierr = MPIU_File_write_all(mfdes,(void*)xarray,lsize,MPIU_SCALAR,MPI_STATUS_IGNORE);CHKERRQ(ierr);
    ierr = PetscViewerBinaryAddMPIIOOffset(viewer,xin->map->N*sizeof(PetscScalar));CHKERRQ(ierr);
  }
#endif

  ierr = VecRestoreArrayRead(xin,&xarray);CHKERRQ(ierr);
  if (!rank) {
    ierr = PetscViewerBinaryGetInfoPointer(viewer,&file);CHKERRQ(ierr);
    if (file) {
      if (((PetscObject)xin)->prefix) {
        ierr = PetscFPrintf(PETSC_COMM_SELF,file,"-%svecload_block_size %D\n",((PetscObject)xin)->prefix,PetscAbs(xin->map->bs));CHKERRQ(ierr);
      } else {
        ierr = PetscFPrintf(PETSC_COMM_SELF,file,"-vecload_block_size %D\n",PetscAbs(xin->map->bs));CHKERRQ(ierr);
      }
    }
  }
  PetscFunctionReturn(0);
}

#include <petscdraw.h>
#undef __FUNCT__
#define __FUNCT__ "VecView_MPI_Draw_LG"
PetscErrorCode VecView_MPI_Draw_LG(Vec xin,PetscViewer viewer)
{
  PetscDraw      draw;
  PetscBool      isnull;
  PetscErrorCode ierr;

#if defined(PETSC_USE_64BIT_INDICES)
  PetscFunctionBegin;
  ierr = PetscViewerDrawGetDraw(viewer,0,&draw);CHKERRQ(ierr);
  ierr = PetscDrawIsNull(draw,&isnull);CHKERRQ(ierr);
  if (isnull) PetscFunctionReturn(0);
  SETERRQ(PETSC_COMM_SELF,PETSC_ERR_SUP,"Not supported with 64 bit integers");
#else
  PetscMPIInt       size,rank;
  int               i,N = xin->map->N,*lens;
  PetscReal         *xx,*yy;
  PetscDrawLG       lg;
  const PetscScalar *xarray;

  PetscFunctionBegin;
  ierr = PetscViewerDrawGetDraw(viewer,0,&draw);CHKERRQ(ierr);
  ierr = PetscDrawIsNull(draw,&isnull);CHKERRQ(ierr);
  if (isnull) PetscFunctionReturn(0);

  ierr = VecGetArrayRead(xin,&xarray);CHKERRQ(ierr);
  ierr = PetscViewerDrawGetDrawLG(viewer,0,&lg);CHKERRQ(ierr);
  ierr = PetscDrawCheckResizedWindow(draw);CHKERRQ(ierr);
  ierr = MPI_Comm_rank(PetscObjectComm((PetscObject)xin),&rank);CHKERRQ(ierr);
  ierr = MPI_Comm_size(PetscObjectComm((PetscObject)xin),&size);CHKERRQ(ierr);
  if (!rank) {
    ierr = PetscDrawLGReset(lg);CHKERRQ(ierr);
    ierr = PetscMalloc2(N,&xx,N,&yy);CHKERRQ(ierr);
    for (i=0; i<N; i++) xx[i] = (PetscReal) i;
    ierr = PetscMalloc1(size,&lens);CHKERRQ(ierr);
    for (i=0; i<size; i++) lens[i] = xin->map->range[i+1] - xin->map->range[i];

#if !defined(PETSC_USE_COMPLEX)
    ierr = MPI_Gatherv((void*)xarray,xin->map->n,MPIU_REAL,yy,lens,xin->map->range,MPIU_REAL,0,PetscObjectComm((PetscObject)xin));CHKERRQ(ierr);
#else
    {
      PetscReal *xr;
      ierr = PetscMalloc1(xin->map->n+1,&xr);CHKERRQ(ierr);
      for (i=0; i<xin->map->n; i++) xr[i] = PetscRealPart(xarray[i]);

      ierr = MPI_Gatherv(xr,xin->map->n,MPIU_REAL,yy,lens,xin->map->range,MPIU_REAL,0,PetscObjectComm((PetscObject)xin));CHKERRQ(ierr);
      ierr = PetscFree(xr);CHKERRQ(ierr);
    }
#endif
    ierr = PetscFree(lens);CHKERRQ(ierr);
    ierr = PetscDrawLGAddPoints(lg,N,&xx,&yy);CHKERRQ(ierr);
    ierr = PetscFree2(xx,yy);CHKERRQ(ierr);
  } else {
#if !defined(PETSC_USE_COMPLEX)
    ierr = MPI_Gatherv((void*)xarray,xin->map->n,MPIU_REAL,0,0,0,MPIU_REAL,0,PetscObjectComm((PetscObject)xin));CHKERRQ(ierr);
#else
    {
      PetscReal *xr;
      ierr = PetscMalloc1(xin->map->n+1,&xr);CHKERRQ(ierr);
      for (i=0; i<xin->map->n; i++) xr[i] = PetscRealPart(xarray[i]);

      ierr = MPI_Gatherv(xr,xin->map->n,MPIU_REAL,0,0,0,MPIU_REAL,0,PetscObjectComm((PetscObject)xin));CHKERRQ(ierr);
      ierr = PetscFree(xr);CHKERRQ(ierr);
    }
#endif
  }
  ierr = PetscDrawLGDraw(lg);CHKERRQ(ierr);
  ierr = PetscDrawSynchronizedFlush(draw);CHKERRQ(ierr);
  ierr = VecRestoreArrayRead(xin,&xarray);CHKERRQ(ierr);
#endif
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecView_MPI_Draw"
PetscErrorCode  VecView_MPI_Draw(Vec xin,PetscViewer viewer)
{
  PetscErrorCode    ierr;
  PetscMPIInt       rank,size,tag = ((PetscObject)viewer)->tag;
  PetscInt          i,start,end;
  MPI_Status        status;
  PetscReal         coors[4],ymin,ymax,xmin,xmax,tmp = 0.0;
  PetscDraw         draw;
  PetscBool         isnull;
  PetscDrawAxis     axis;
  const PetscScalar *xarray;

  PetscFunctionBegin;
  ierr = PetscViewerDrawGetDraw(viewer,0,&draw);CHKERRQ(ierr);
  ierr = PetscDrawIsNull(draw,&isnull);CHKERRQ(ierr); if (isnull) PetscFunctionReturn(0);

  ierr = VecGetArrayRead(xin,&xarray);CHKERRQ(ierr);
  ierr = PetscDrawCheckResizedWindow(draw);CHKERRQ(ierr);
  xmin = 1.e20; xmax = -1.e20;
  for (i=0; i<xin->map->n; i++) {
#if defined(PETSC_USE_COMPLEX)
    if (PetscRealPart(xarray[i]) < xmin) xmin = PetscRealPart(xarray[i]);
    if (PetscRealPart(xarray[i]) > xmax) xmax = PetscRealPart(xarray[i]);
#else
    if (xarray[i] < xmin) xmin = xarray[i];
    if (xarray[i] > xmax) xmax = xarray[i];
#endif
  }
  if (xmin + 1.e-10 > xmax) {
    xmin -= 1.e-5;
    xmax += 1.e-5;
  }
  ierr = MPI_Reduce(&xmin,&ymin,1,MPIU_REAL,MPIU_MIN,0,PetscObjectComm((PetscObject)xin));CHKERRQ(ierr);
  ierr = MPI_Reduce(&xmax,&ymax,1,MPIU_REAL,MPIU_MAX,0,PetscObjectComm((PetscObject)xin));CHKERRQ(ierr);
  ierr = MPI_Comm_size(PetscObjectComm((PetscObject)xin),&size);CHKERRQ(ierr);
  ierr = MPI_Comm_rank(PetscObjectComm((PetscObject)xin),&rank);CHKERRQ(ierr);
  ierr = PetscDrawAxisCreate(draw,&axis);CHKERRQ(ierr);
  ierr = PetscLogObjectParent((PetscObject)draw,(PetscObject)axis);CHKERRQ(ierr);
  if (!rank) {
    ierr = PetscDrawClear(draw);CHKERRQ(ierr);
    ierr = PetscDrawFlush(draw);CHKERRQ(ierr);
    ierr = PetscDrawAxisSetLimits(axis,0.0,(double)xin->map->N,ymin,ymax);CHKERRQ(ierr);
    ierr = PetscDrawAxisDraw(axis);CHKERRQ(ierr);
    ierr = PetscDrawGetCoordinates(draw,coors,coors+1,coors+2,coors+3);CHKERRQ(ierr);
  }
  ierr = PetscDrawAxisDestroy(&axis);CHKERRQ(ierr);
  ierr = MPI_Bcast(coors,4,MPIU_REAL,0,PetscObjectComm((PetscObject)xin));CHKERRQ(ierr);
  if (rank) {ierr = PetscDrawSetCoordinates(draw,coors[0],coors[1],coors[2],coors[3]);CHKERRQ(ierr);}
  /* draw local part of vector */
  ierr = VecGetOwnershipRange(xin,&start,&end);CHKERRQ(ierr);
  if (rank < size-1) { /*send value to right */
    ierr = MPI_Send((void*)&xarray[xin->map->n-1],1,MPIU_REAL,rank+1,tag,PetscObjectComm((PetscObject)xin));CHKERRQ(ierr);
  }
  for (i=1; i<xin->map->n; i++) {
#if !defined(PETSC_USE_COMPLEX)
    ierr = PetscDrawLine(draw,(PetscReal)(i-1+start),xarray[i-1],(PetscReal)(i+start),xarray[i],PETSC_DRAW_RED);CHKERRQ(ierr);
#else
    ierr = PetscDrawLine(draw,(PetscReal)(i-1+start),PetscRealPart(xarray[i-1]),(PetscReal)(i+start),PetscRealPart(xarray[i]),PETSC_DRAW_RED);CHKERRQ(ierr);
#endif
  }
  if (rank) { /* receive value from right */
    ierr = MPI_Recv(&tmp,1,MPIU_REAL,rank-1,tag,PetscObjectComm((PetscObject)xin),&status);CHKERRQ(ierr);
#if !defined(PETSC_USE_COMPLEX)
    ierr = PetscDrawLine(draw,(PetscReal)start-1,tmp,(PetscReal)start,xarray[0],PETSC_DRAW_RED);CHKERRQ(ierr);
#else
    ierr = PetscDrawLine(draw,(PetscReal)start-1,tmp,(PetscReal)start,PetscRealPart(xarray[0]),PETSC_DRAW_RED);CHKERRQ(ierr);
#endif
  }
  ierr = PetscDrawSynchronizedFlush(draw);CHKERRQ(ierr);
  ierr = PetscDrawPause(draw);CHKERRQ(ierr);
  ierr = VecRestoreArrayRead(xin,&xarray);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#if defined(PETSC_HAVE_MATLAB_ENGINE)
#undef __FUNCT__
#define __FUNCT__ "VecView_MPI_Matlab"
PetscErrorCode VecView_MPI_Matlab(Vec xin,PetscViewer viewer)
{
  PetscErrorCode    ierr;
  PetscMPIInt       rank,size,*lens;
  PetscInt          i,N = xin->map->N;
  const PetscScalar *xarray;
  PetscScalar       *xx;

  PetscFunctionBegin;
  ierr = VecGetArrayRead(xin,&xarray);CHKERRQ(ierr);
  ierr = MPI_Comm_rank(PetscObjectComm((PetscObject)xin),&rank);CHKERRQ(ierr);
  ierr = MPI_Comm_size(PetscObjectComm((PetscObject)xin),&size);CHKERRQ(ierr);
  if (!rank) {
    ierr = PetscMalloc1(N,&xx);CHKERRQ(ierr);
    ierr = PetscMalloc1(size,&lens);CHKERRQ(ierr);
    for (i=0; i<size; i++) lens[i] = xin->map->range[i+1] - xin->map->range[i];

    ierr = MPI_Gatherv((void*)xarray,xin->map->n,MPIU_SCALAR,xx,lens,xin->map->range,MPIU_SCALAR,0,PetscObjectComm((PetscObject)xin));CHKERRQ(ierr);
    ierr = PetscFree(lens);CHKERRQ(ierr);

    ierr = PetscObjectName((PetscObject)xin);CHKERRQ(ierr);
    ierr = PetscViewerMatlabPutArray(viewer,N,1,xx,((PetscObject)xin)->name);CHKERRQ(ierr);

    ierr = PetscFree(xx);CHKERRQ(ierr);
  } else {
    ierr = MPI_Gatherv((void*)xarray,xin->map->n,MPIU_SCALAR,0,0,0,MPIU_SCALAR,0,PetscObjectComm((PetscObject)xin));CHKERRQ(ierr);
  }
  ierr = VecRestoreArrayRead(xin,&xarray);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}
#endif

#if defined(PETSC_HAVE_HDF5)
#undef __FUNCT__
#define __FUNCT__ "VecView_MPI_HDF5"
PetscErrorCode VecView_MPI_HDF5(Vec xin, PetscViewer viewer)
{
  /* TODO: It looks like we can remove the H5Sclose(filespace) and H5Dget_space(dset_id). Why do we do this? */
  hid_t             filespace; /* file dataspace identifier */
  hid_t             chunkspace; /* chunk dataset property identifier */
  hid_t             plist_id;  /* property list identifier */
  hid_t             dset_id;   /* dataset identifier */
  hid_t             memspace;  /* memory dataspace identifier */
  hid_t             file_id;
  hid_t             group;
  hid_t             memscalartype; /* scalar type for mem (H5T_NATIVE_FLOAT or H5T_NATIVE_DOUBLE) */
  hid_t             filescalartype; /* scalar type for file (H5T_NATIVE_FLOAT or H5T_NATIVE_DOUBLE) */
  PetscInt          bs = PetscAbs(xin->map->bs);
  hsize_t           dim;
  hsize_t           maxDims[4], dims[4], chunkDims[4], count[4],offset[4];
  PetscInt          timestep;
  PetscInt          low;
  const PetscScalar *x;
  const char        *vecname;
  PetscErrorCode    ierr;
  PetscBool         dim2;
  PetscBool         spoutput;

  PetscFunctionBegin;
  ierr = PetscViewerHDF5OpenGroup(viewer, &file_id, &group);CHKERRQ(ierr);
  ierr = PetscViewerHDF5GetTimestep(viewer, &timestep);CHKERRQ(ierr);
  ierr = PetscViewerHDF5GetBaseDimension2(viewer,&dim2);CHKERRQ(ierr);
  ierr = PetscViewerHDF5GetSPOutput(viewer,&spoutput);CHKERRQ(ierr);

  /* Create the dataspace for the dataset.
   *
   * dims - holds the current dimensions of the dataset
   *
   * maxDims - holds the maximum dimensions of the dataset (unlimited
   * for the number of time steps with the current dimensions for the
   * other dimensions; so only additional time steps can be added).
   *
   * chunkDims - holds the size of a single time step (required to
   * permit extending dataset).
   */
  dim = 0;
  if (timestep >= 0) {
    dims[dim]      = timestep+1;
    maxDims[dim]   = H5S_UNLIMITED;
    chunkDims[dim] = 1;
    ++dim;
  }
  ierr = PetscHDF5IntCast(xin->map->N/bs,dims + dim);CHKERRQ(ierr);

  maxDims[dim]   = dims[dim];
  chunkDims[dim] = dims[dim];
  ++dim;
  if (bs > 1 || dim2) {
    dims[dim]      = bs;
    maxDims[dim]   = dims[dim];
    chunkDims[dim] = dims[dim];
    ++dim;
  }
#if defined(PETSC_USE_COMPLEX)
  dims[dim]      = 2;
  maxDims[dim]   = dims[dim];
  chunkDims[dim] = dims[dim];
  ++dim;
#endif
  PetscStackCallHDF5Return(filespace,H5Screate_simple,(dim, dims, maxDims));

#if defined(PETSC_USE_REAL_SINGLE)
  memscalartype = H5T_NATIVE_FLOAT;
  filescalartype = H5T_NATIVE_FLOAT;
#elif defined(PETSC_USE_REAL___FLOAT128)
#error "HDF5 output with 128 bit floats not supported."
#else
  memscalartype = H5T_NATIVE_DOUBLE;
  if (spoutput == PETSC_TRUE) filescalartype = H5T_NATIVE_FLOAT;
  else filescalartype = H5T_NATIVE_DOUBLE;
#endif

  /* Create the dataset with default properties and close filespace */
  ierr = PetscObjectGetName((PetscObject) xin, &vecname);CHKERRQ(ierr);
  if (!H5Lexists(group, vecname, H5P_DEFAULT)) {
    /* Create chunk */
    PetscStackCallHDF5Return(chunkspace,H5Pcreate,(H5P_DATASET_CREATE));
    PetscStackCallHDF5(H5Pset_chunk,(chunkspace, dim, chunkDims));

#if (H5_VERS_MAJOR * 10000 + H5_VERS_MINOR * 100 + H5_VERS_RELEASE >= 10800)
    PetscStackCallHDF5Return(dset_id,H5Dcreate2,(group, vecname, filescalartype, filespace, H5P_DEFAULT, chunkspace, H5P_DEFAULT));
#else
    PetscStackCallHDF5Return(dset_id,H5Dcreate,(group, vecname, filescalartype, filespace, H5P_DEFAULT));
#endif
    PetscStackCallHDF5(H5Pclose,(chunkspace));
  } else {
    PetscStackCallHDF5Return(dset_id,H5Dopen2,(group, vecname, H5P_DEFAULT));
    PetscStackCallHDF5(H5Dset_extent,(dset_id, dims));
  }
  PetscStackCallHDF5(H5Sclose,(filespace));

  /* Each process defines a dataset and writes it to the hyperslab in the file */
  dim = 0;
  if (timestep >= 0) {
    count[dim] = 1;
    ++dim;
  }
  ierr = PetscHDF5IntCast(xin->map->n/bs,count + dim);CHKERRQ(ierr);
  ++dim;
  if (bs > 1 || dim2) {
    count[dim] = bs;
    ++dim;
  }
#if defined(PETSC_USE_COMPLEX)
  count[dim] = 2;
  ++dim;
#endif
  if (xin->map->n > 0) {
    PetscStackCallHDF5Return(memspace,H5Screate_simple,(dim, count, NULL));
  } else {
    /* Can't create dataspace with zero for any dimension, so create null dataspace. */
    PetscStackCallHDF5Return(memspace,H5Screate,(H5S_NULL));
  }

  /* Select hyperslab in the file */
  ierr = VecGetOwnershipRange(xin, &low, NULL);CHKERRQ(ierr);
  dim  = 0;
  if (timestep >= 0) {
    offset[dim] = timestep;
    ++dim;
  }
  ierr = PetscHDF5IntCast(low/bs,offset + dim);CHKERRQ(ierr);
  ++dim;
  if (bs > 1 || dim2) {
    offset[dim] = 0;
    ++dim;
  }
#if defined(PETSC_USE_COMPLEX)
  offset[dim] = 0;
  ++dim;
#endif
  if (xin->map->n > 0) {
    PetscStackCallHDF5Return(filespace,H5Dget_space,(dset_id));
    PetscStackCallHDF5(H5Sselect_hyperslab,(filespace, H5S_SELECT_SET, offset, NULL, count, NULL));
  } else {
    /* Create null filespace to match null memspace. */
    PetscStackCallHDF5Return(filespace,H5Screate,(H5S_NULL));
  }

  /* Create property list for collective dataset write */
  PetscStackCallHDF5Return(plist_id,H5Pcreate,(H5P_DATASET_XFER));
#if defined(PETSC_HAVE_H5PSET_FAPL_MPIO)
  PetscStackCallHDF5(H5Pset_dxpl_mpio,(plist_id, H5FD_MPIO_COLLECTIVE));
#endif
  /* To write dataset independently use H5Pset_dxpl_mpio(plist_id, H5FD_MPIO_INDEPENDENT) */

  ierr   = VecGetArrayRead(xin, &x);CHKERRQ(ierr);
  PetscStackCallHDF5(H5Dwrite,(dset_id, memscalartype, memspace, filespace, plist_id, x));
  PetscStackCallHDF5(H5Fflush,(file_id, H5F_SCOPE_GLOBAL));
  ierr   = VecRestoreArrayRead(xin, &x);CHKERRQ(ierr);

  /* Close/release resources */
  if (group != file_id) {
    PetscStackCallHDF5(H5Gclose,(group));
  }
  PetscStackCallHDF5(H5Pclose,(plist_id));
  PetscStackCallHDF5(H5Sclose,(filespace));
  PetscStackCallHDF5(H5Sclose,(memspace));
  PetscStackCallHDF5(H5Dclose,(dset_id));
  ierr   = PetscInfo1(xin,"Wrote Vec object with name %s\n",vecname);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}
#endif

#undef __FUNCT__
#define __FUNCT__ "VecView_MPI"
PetscErrorCode VecView_MPI(Vec xin,PetscViewer viewer)
{
  PetscErrorCode ierr;
  PetscBool      iascii,isbinary,isdraw;
#if defined(PETSC_HAVE_MATHEMATICA)
  PetscBool      ismathematica;
#endif
#if defined(PETSC_HAVE_HDF5)
  PetscBool      ishdf5;
#endif
#if defined(PETSC_HAVE_MATLAB_ENGINE) && !defined(PETSC_USE_COMPLEX) && !defined(PETSC_USE_REAL_SINGLE) && !defined(PETSC_USE_REAL___FLOAT128)
  PetscBool      ismatlab;
#endif

  PetscFunctionBegin;
  ierr = PetscObjectTypeCompare((PetscObject)viewer,PETSCVIEWERASCII,&iascii);CHKERRQ(ierr);
  ierr = PetscObjectTypeCompare((PetscObject)viewer,PETSCVIEWERBINARY,&isbinary);CHKERRQ(ierr);
  ierr = PetscObjectTypeCompare((PetscObject)viewer,PETSCVIEWERDRAW,&isdraw);CHKERRQ(ierr);
#if defined(PETSC_HAVE_MATHEMATICA)
  ierr = PetscObjectTypeCompare((PetscObject)viewer,PETSCVIEWERMATHEMATICA,&ismathematica);CHKERRQ(ierr);
#endif
#if defined(PETSC_HAVE_HDF5)
  ierr = PetscObjectTypeCompare((PetscObject)viewer,PETSCVIEWERHDF5,&ishdf5);CHKERRQ(ierr);
#endif
#if defined(PETSC_HAVE_MATLAB_ENGINE) && !defined(PETSC_USE_COMPLEX) && !defined(PETSC_USE_REAL_SINGLE) && !defined(PETSC_USE_REAL___FLOAT128)
  ierr = PetscObjectTypeCompare((PetscObject)viewer,PETSCVIEWERMATLAB,&ismatlab);CHKERRQ(ierr);
#endif
  if (iascii) {
    ierr = VecView_MPI_ASCII(xin,viewer);CHKERRQ(ierr);
  } else if (isbinary) {
    ierr = VecView_MPI_Binary(xin,viewer);CHKERRQ(ierr);
  } else if (isdraw) {
    PetscViewerFormat format;

    ierr = PetscViewerGetFormat(viewer,&format);CHKERRQ(ierr);
    if (format == PETSC_VIEWER_DRAW_LG) {
      ierr = VecView_MPI_Draw_LG(xin,viewer);CHKERRQ(ierr);
    } else {
      ierr = VecView_MPI_Draw(xin,viewer);CHKERRQ(ierr);
    }
#if defined(PETSC_HAVE_MATHEMATICA)
  } else if (ismathematica) {
    ierr = PetscViewerMathematicaPutVector(viewer,xin);CHKERRQ(ierr);
#endif
#if defined(PETSC_HAVE_HDF5)
  } else if (ishdf5) {
    ierr = VecView_MPI_HDF5(xin,viewer);CHKERRQ(ierr);
#endif
#if defined(PETSC_HAVE_MATLAB_ENGINE) && !defined(PETSC_USE_COMPLEX) && !defined(PETSC_USE_REAL_SINGLE) && !defined(PETSC_USE_REAL___FLOAT128)
  } else if (ismatlab) {
    ierr = VecView_MPI_Matlab(xin,viewer);CHKERRQ(ierr);
#endif
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecGetSize_MPI"
PetscErrorCode VecGetSize_MPI(Vec xin,PetscInt *N)
{
  PetscFunctionBegin;
  *N = xin->map->N;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecGetValues_MPI"
PetscErrorCode VecGetValues_MPI(Vec xin,PetscInt ni,const PetscInt ix[],PetscScalar y[])
{
  const PetscScalar *xx;
  PetscInt          i,tmp,start = xin->map->range[xin->stash.rank];
  PetscErrorCode    ierr;

  PetscFunctionBegin;
  ierr = VecGetArrayRead(xin,&xx);CHKERRQ(ierr);
  for (i=0; i<ni; i++) {
    if (xin->stash.ignorenegidx && ix[i] < 0) continue;
    tmp = ix[i] - start;
#if defined(PETSC_USE_DEBUG)
    if (tmp < 0 || tmp >= xin->map->n) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Can only get local values, trying %D",ix[i]);
#endif
    y[i] = xx[tmp];
  }
  ierr = VecRestoreArrayRead(xin,&xx);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecSetValues_MPI"
PetscErrorCode VecSetValues_MPI(Vec xin,PetscInt ni,const PetscInt ix[],const PetscScalar y[],InsertMode addv)
{
  PetscErrorCode ierr;
  PetscMPIInt    rank    = xin->stash.rank;
  PetscInt       *owners = xin->map->range,start = owners[rank];
  PetscInt       end     = owners[rank+1],i,row;
  PetscScalar    *xx;

  PetscFunctionBegin;
#if defined(PETSC_USE_DEBUG)
  if (xin->stash.insertmode == INSERT_VALUES && addv == ADD_VALUES) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONGSTATE,"You have already inserted values; you cannot now add");
  else if (xin->stash.insertmode == ADD_VALUES && addv == INSERT_VALUES) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONGSTATE,"You have already added values; you cannot now insert");
#endif
  ierr = VecGetArray(xin,&xx);CHKERRQ(ierr);
  xin->stash.insertmode = addv;

  if (addv == INSERT_VALUES) {
    for (i=0; i<ni; i++) {
      if (xin->stash.ignorenegidx && ix[i] < 0) continue;
#if defined(PETSC_USE_DEBUG)
      if (ix[i] < 0) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Out of range index value %D cannot be negative",ix[i]);
#endif
      if ((row = ix[i]) >= start && row < end) {
        xx[row-start] = y[i];
      } else if (!xin->stash.donotstash) {
#if defined(PETSC_USE_DEBUG)
        if (ix[i] >= xin->map->N) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Out of range index value %D maximum %D",ix[i],xin->map->N);
#endif
        ierr = VecStashValue_Private(&xin->stash,row,y[i]);CHKERRQ(ierr);
      }
    }
  } else {
    for (i=0; i<ni; i++) {
      if (xin->stash.ignorenegidx && ix[i] < 0) continue;
#if defined(PETSC_USE_DEBUG)
      if (ix[i] < 0) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Out of range index value %D cannot be negative",ix[i]);
#endif
      if ((row = ix[i]) >= start && row < end) {
        xx[row-start] += y[i];
      } else if (!xin->stash.donotstash) {
#if defined(PETSC_USE_DEBUG)
        if (ix[i] > xin->map->N) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Out of range index value %D maximum %D",ix[i],xin->map->N);
#endif
        ierr = VecStashValue_Private(&xin->stash,row,y[i]);CHKERRQ(ierr);
      }
    }
  }
  ierr = VecRestoreArray(xin,&xx);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecSetValuesBlocked_MPI"
PetscErrorCode VecSetValuesBlocked_MPI(Vec xin,PetscInt ni,const PetscInt ix[],const PetscScalar yin[],InsertMode addv)
{
  PetscMPIInt    rank    = xin->stash.rank;
  PetscInt       *owners = xin->map->range,start = owners[rank];
  PetscErrorCode ierr;
  PetscInt       end = owners[rank+1],i,row,bs = PetscAbs(xin->map->bs),j;
  PetscScalar    *xx,*y = (PetscScalar*)yin;

  PetscFunctionBegin;
  ierr = VecGetArray(xin,&xx);CHKERRQ(ierr);
#if defined(PETSC_USE_DEBUG)
  if (xin->stash.insertmode == INSERT_VALUES && addv == ADD_VALUES) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONGSTATE,"You have already inserted values; you cannot now add");
  else if (xin->stash.insertmode == ADD_VALUES && addv == INSERT_VALUES) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONGSTATE,"You have already added values; you cannot now insert");
#endif
  xin->stash.insertmode = addv;

  if (addv == INSERT_VALUES) {
    for (i=0; i<ni; i++) {
      if ((row = bs*ix[i]) >= start && row < end) {
        for (j=0; j<bs; j++) xx[row-start+j] = y[j];
      } else if (!xin->stash.donotstash) {
        if (ix[i] < 0) continue;
#if defined(PETSC_USE_DEBUG)
        if (ix[i] >= xin->map->N) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Out of range index value %D max %D",ix[i],xin->map->N);
#endif
        ierr = VecStashValuesBlocked_Private(&xin->bstash,ix[i],y);CHKERRQ(ierr);
      }
      y += bs;
    }
  } else {
    for (i=0; i<ni; i++) {
      if ((row = bs*ix[i]) >= start && row < end) {
        for (j=0; j<bs; j++) xx[row-start+j] += y[j];
      } else if (!xin->stash.donotstash) {
        if (ix[i] < 0) continue;
#if defined(PETSC_USE_DEBUG)
        if (ix[i] > xin->map->N) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Out of range index value %D max %D",ix[i],xin->map->N);
#endif
        ierr = VecStashValuesBlocked_Private(&xin->bstash,ix[i],y);CHKERRQ(ierr);
      }
      y += bs;
    }
  }
  ierr = VecRestoreArray(xin,&xx);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/*
   Since nsends or nreceives may be zero we add 1 in certain mallocs
to make sure we never malloc an empty one.
*/
#undef __FUNCT__
#define __FUNCT__ "VecAssemblyBegin_MPI"
PetscErrorCode VecAssemblyBegin_MPI(Vec xin)
{
  PetscErrorCode ierr;
  PetscInt       *owners = xin->map->range,*bowners,i,bs,nstash,reallocs;
  PetscMPIInt    size;
  InsertMode     addv;
  MPI_Comm       comm;

  PetscFunctionBegin;
  ierr = PetscObjectGetComm((PetscObject)xin,&comm);CHKERRQ(ierr);
  if (xin->stash.donotstash) PetscFunctionReturn(0);

  ierr = MPIU_Allreduce((PetscEnum*)&xin->stash.insertmode,(PetscEnum*)&addv,1,MPIU_ENUM,MPI_BOR,comm);CHKERRQ(ierr);
  if (addv == (ADD_VALUES|INSERT_VALUES)) SETERRQ(comm,PETSC_ERR_ARG_NOTSAMETYPE,"Some processors inserted values while others added");
  xin->stash.insertmode = addv; /* in case this processor had no cache */
  xin->bstash.insertmode = addv; /* Block stash implicitly tracks InsertMode of scalar stash */

  ierr = VecGetBlockSize(xin,&bs);CHKERRQ(ierr);
  ierr = MPI_Comm_size(PetscObjectComm((PetscObject)xin),&size);CHKERRQ(ierr);
  if (!xin->bstash.bowners && xin->map->bs != -1) {
    ierr = PetscMalloc1(size+1,&bowners);CHKERRQ(ierr);
    for (i=0; i<size+1; i++) bowners[i] = owners[i]/bs;
    xin->bstash.bowners = bowners;
  } else bowners = xin->bstash.bowners;

  ierr = VecStashScatterBegin_Private(&xin->stash,owners);CHKERRQ(ierr);
  ierr = VecStashScatterBegin_Private(&xin->bstash,bowners);CHKERRQ(ierr);
  ierr = VecStashGetInfo_Private(&xin->stash,&nstash,&reallocs);CHKERRQ(ierr);
  ierr = PetscInfo2(xin,"Stash has %D entries, uses %D mallocs.\n",nstash,reallocs);CHKERRQ(ierr);
  ierr = VecStashGetInfo_Private(&xin->bstash,&nstash,&reallocs);CHKERRQ(ierr);
  ierr = PetscInfo2(xin,"Block-Stash has %D entries, uses %D mallocs.\n",nstash,reallocs);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecAssemblyEnd_MPI"
PetscErrorCode VecAssemblyEnd_MPI(Vec vec)
{
  PetscErrorCode ierr;
  PetscInt       base,i,j,*row,flg,bs;
  PetscMPIInt    n;
  PetscScalar    *val,*vv,*array,*xarray;

  PetscFunctionBegin;
  if (!vec->stash.donotstash) {
    ierr = VecGetArray(vec,&xarray);CHKERRQ(ierr);
    ierr = VecGetBlockSize(vec,&bs);CHKERRQ(ierr);
    base = vec->map->range[vec->stash.rank];

    /* Process the stash */
    while (1) {
      ierr = VecStashScatterGetMesg_Private(&vec->stash,&n,&row,&val,&flg);CHKERRQ(ierr);
      if (!flg) break;
      if (vec->stash.insertmode == ADD_VALUES) {
        for (i=0; i<n; i++) xarray[row[i] - base] += val[i];
      } else if (vec->stash.insertmode == INSERT_VALUES) {
        for (i=0; i<n; i++) xarray[row[i] - base] = val[i];
      } else SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_CORRUPT,"Insert mode is not set correctly; corrupted vector");
    }
    ierr = VecStashScatterEnd_Private(&vec->stash);CHKERRQ(ierr);

    /* now process the block-stash */
    while (1) {
      ierr = VecStashScatterGetMesg_Private(&vec->bstash,&n,&row,&val,&flg);CHKERRQ(ierr);
      if (!flg) break;
      for (i=0; i<n; i++) {
        array = xarray+row[i]*bs-base;
        vv    = val+i*bs;
        if (vec->stash.insertmode == ADD_VALUES) {
          for (j=0; j<bs; j++) array[j] += vv[j];
        } else if (vec->stash.insertmode == INSERT_VALUES) {
          for (j=0; j<bs; j++) array[j] = vv[j];
        } else SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_CORRUPT,"Insert mode is not set correctly; corrupted vector");
      }
    }
    ierr = VecStashScatterEnd_Private(&vec->bstash);CHKERRQ(ierr);
    ierr = VecRestoreArray(vec,&xarray);CHKERRQ(ierr);
  }
  vec->stash.insertmode = NOT_SET_VALUES;
  PetscFunctionReturn(0);
}

