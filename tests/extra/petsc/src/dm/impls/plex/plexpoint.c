#include <petsc/private/dmpleximpl.h>   /*I      "petscdmplex.h"   I*/

#undef __FUNCT__
#define __FUNCT__ "DMPlexGetPointLocal"
/*@
   DMPlexGetPointLocal - get location of point data in local Vec

   Not Collective

   Input Arguments:
+  dm - DM defining the topological space
-  point - topological point

   Output Arguments:
+  start - start of point data
-  end - end of point data

   Note: This is a half open interval [start, end)

   Level: intermediate

.seealso: DMPlexGetPointLocalField(), DMGetDefaultSection(), PetscSectionGetOffset(), PetscSectionGetDof(), DMPlexPointLocalRead(), DMPlexPointLocalRead(), DMPlexPointLocalRef()
@*/
PetscErrorCode DMPlexGetPointLocal(DM dm, PetscInt point, PetscInt *start, PetscInt *end)
{
  PetscInt       s, e;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  if (start) PetscValidPointer(start, 3);
  if (end)   PetscValidPointer(end,   4);
  ierr = DMPlexGetLocalOffset_Private(dm, point, &s, &e);CHKERRQ(ierr);
  if (start) *start = s;
  if (end)   *end   = e;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMPlexPointLocalRead"
/*@
   DMPlexPointLocalRead - return read access to a point in local array

   Not Collective

   Input Arguments:
+  dm - DM defining topological space
.  point - topological point
-  array - array to index into

   Output Arguments:
.  ptr - address of read reference to point data, type generic so user can place in structure

   Level: intermediate

   Note:
   A common usage when data sizes are known statically:

$  const struct { PetscScalar foo,bar,baz; } *ptr;
$  DMPlexPointLocalRead(dm,point,array,&ptr);
$  x = 2*ptr->foo + 3*ptr->bar + 5*ptr->baz;

.seealso: DMGetDefaultSection(), PetscSectionGetOffset(), PetscSectionGetDof(), DMPlexGetPointLocal(), DMPlexPointGlobalRead()
@*/
PetscErrorCode DMPlexPointLocalRead(DM dm,PetscInt point,const PetscScalar *array,const void *ptr)
{
  PetscErrorCode ierr;
  PetscInt       start, end;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  PetscValidScalarPointer(array,3);
  PetscValidPointer(ptr,4);
  ierr                      = DMPlexGetLocalOffset_Private(dm,point,&start,&end);CHKERRQ(ierr);
  *(const PetscScalar**)ptr = (start < end) ? array + start : NULL;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMPlexPointLocalRef"
/*@
   DMPlexPointLocalRef - return read/write access to a point in local array

   Not Collective

   Input Arguments:
+  dm - DM defining topological space
.  point - topological point
-  array - array to index into

   Output Arguments:
.  ptr - address of reference to point data, type generic so user can place in structure

   Level: intermediate

   Note:
   A common usage when data sizes are known statically:

$  struct { PetscScalar foo,bar,baz; } *ptr;
$  DMPlexPointLocalRef(dm,point,array,&ptr);
$  ptr->foo = 2; ptr->bar = 3; ptr->baz = 5;

.seealso: DMGetDefaultSection(), PetscSectionGetOffset(), PetscSectionGetDof(), DMPlexGetPointLocal(), DMPlexPointGlobalRef()
@*/
PetscErrorCode DMPlexPointLocalRef(DM dm,PetscInt point,PetscScalar *array,void *ptr)
{
  PetscErrorCode ierr;
  PetscInt       start, end;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  PetscValidScalarPointer(array,3);
  PetscValidPointer(ptr,4);
  ierr                = DMPlexGetLocalOffset_Private(dm,point,&start,&end);CHKERRQ(ierr);
  *(PetscScalar**)ptr = (start < end) ? array + start : NULL;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMPlexGetPointLocalField"
/*@
  DMPlexGetPointLocalField - get location of point field data in local Vec

  Not Collective

  Input Arguments:
+ dm - DM defining the topological space
. point - topological point
- field - the field number

  Output Arguments:
+ start - start of point data
- end - end of point data

  Note: This is a half open interval [start, end)

  Level: intermediate

.seealso: DMPlexGetPointLocal(), DMGetDefaultSection(), PetscSectionGetOffset(), PetscSectionGetDof(), DMPlexPointLocalRead(), DMPlexPointLocalRead(), DMPlexPointLocalRef()
@*/
PetscErrorCode DMPlexGetPointLocalField(DM dm, PetscInt point, PetscInt field, PetscInt *start, PetscInt *end)
{
  PetscInt       s, e;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  if (start) PetscValidPointer(start, 4);
  if (end)   PetscValidPointer(end,   5);
  ierr = DMPlexGetLocalFieldOffset_Private(dm, point, field, &s, &e);CHKERRQ(ierr);
  if (start) *start = s;
  if (end)   *end   = e;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMPlexPointLocalFieldRead"
/*@
   DMPlexPointLocalFieldRead - return read access to a field on a point in local array

   Not Collective

   Input Arguments:
+  dm - DM defining topological space
.  point - topological point
.  field - field number
-  array - array to index into

   Output Arguments:
.  ptr - address of read reference to point data, type generic so user can place in structure

   Level: intermediate

.seealso: DMGetDefaultSection(), PetscSectionGetOffset(), PetscSectionGetDof(), DMPlexGetPointLocal(), DMPlexPointGlobalRef()
@*/
PetscErrorCode DMPlexPointLocalFieldRead(DM dm, PetscInt point,PetscInt field,const PetscScalar *array,const void *ptr)
{
  PetscErrorCode ierr;
  PetscInt       start, end;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  PetscValidScalarPointer(array,3);
  PetscValidPointer(ptr,4);
  ierr                      = DMPlexGetLocalFieldOffset_Private(dm, point, field, &start, &end);CHKERRQ(ierr);
  *(const PetscScalar**)ptr = array + start;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMPlexPointLocalFieldRef"
/*@
   DMPlexPointLocalFieldRef - return read/write access to a field on a point in local array

   Not Collective

   Input Arguments:
+  dm - DM defining topological space
.  point - topological point
.  field - field number
-  array - array to index into

   Output Arguments:
.  ptr - address of reference to point data, type generic so user can place in structure

   Level: intermediate

.seealso: DMGetDefaultSection(), PetscSectionGetOffset(), PetscSectionGetDof(), DMPlexGetPointLocal(), DMPlexPointGlobalRef()
@*/
PetscErrorCode DMPlexPointLocalFieldRef(DM dm,PetscInt point,PetscInt field,PetscScalar *array,void *ptr)
{
  PetscErrorCode ierr;
  PetscInt       start, end;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  PetscValidScalarPointer(array,3);
  PetscValidPointer(ptr,4);
  ierr                = DMPlexGetLocalFieldOffset_Private(dm, point, field, &start, &end);CHKERRQ(ierr);
  *(PetscScalar**)ptr = array + start;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMPlexGetPointGlobal"
/*@
  DMPlexGetPointGlobal - get location of point data in global Vec

  Not Collective

  Input Arguments:
+ dm - DM defining the topological space
- point - topological point

  Output Arguments:
+ start - start of point data; returns -(globalStart+1) if point is not owned
- end - end of point data; returns -(globalEnd+1) if point is not owned

  Note: This is a half open interval [start, end)

  Level: intermediate

.seealso: DMPlexGetPointGlobalField(), DMGetDefaultSection(), PetscSectionGetOffset(), PetscSectionGetDof(), DMPlexPointGlobalRead(), DMPlexGetPointLocal(), DMPlexPointGlobalRead(), DMPlexPointGlobalRef()
@*/
PetscErrorCode DMPlexGetPointGlobal(DM dm, PetscInt point, PetscInt *start, PetscInt *end)
{
  PetscInt       s, e;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm, DM_CLASSID, 1);
  if (start) PetscValidPointer(start, 3);
  if (end)   PetscValidPointer(end,   4);
  ierr = DMPlexGetGlobalOffset_Private(dm, point, &s, &e);CHKERRQ(ierr);
  if (start) *start = s;
  if (end)   *end   = e;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMPlexPointGlobalRead"
/*@
   DMPlexPointGlobalRead - return read access to a point in global array

   Not Collective

   Input Arguments:
+  dm - DM defining topological space
.  point - topological point
-  array - array to index into

   Output Arguments:
.  ptr - address of read reference to point data, type generic so user can place in structure; returns NULL if global point is not owned

   Level: intermediate

   Note:
   A common usage when data sizes are known statically:

$  const struct { PetscScalar foo,bar,baz; } *ptr;
$  DMPlexPointGlobalRead(dm,point,array,&ptr);
$  x = 2*ptr->foo + 3*ptr->bar + 5*ptr->baz;

.seealso: DMGetDefaultSection(), PetscSectionGetOffset(), PetscSectionGetDof(), DMPlexGetPointGlobal(), DMPlexPointLocalRead(), DMPlexPointGlobalRef()
@*/
PetscErrorCode DMPlexPointGlobalRead(DM dm,PetscInt point,const PetscScalar *array,const void *ptr)
{
  PetscInt       start, end;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm, DM_CLASSID, 1);
  PetscValidScalarPointer(array, 3);
  PetscValidPointer(ptr, 4);
  ierr = DMPlexGetGlobalOffset_Private(dm, point, &start, &end);CHKERRQ(ierr);
  *(const PetscScalar**) ptr = (start < end) ? array + start - dm->map->rstart : NULL;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMPlexPointGlobalRef"
/*@
   DMPlexPointGlobalRef - return read/write access to a point in global array

   Not Collective

   Input Arguments:
+  dm - DM defining topological space
.  point - topological point
-  array - array to index into

   Output Arguments:
.  ptr - address of reference to point data, type generic so user can place in structure; returns NULL if global point is not owned

   Level: intermediate

   Note:
   A common usage when data sizes are known statically:

$  struct { PetscScalar foo,bar,baz; } *ptr;
$  DMPlexPointGlobalRef(dm,point,array,&ptr);
$  ptr->foo = 2; ptr->bar = 3; ptr->baz = 5;

.seealso: DMGetDefaultSection(), PetscSectionGetOffset(), PetscSectionGetDof(), DMPlexGetPointGlobal(), DMPlexPointLocalRef(), DMPlexPointGlobalRead()
@*/
PetscErrorCode DMPlexPointGlobalRef(DM dm,PetscInt point,PetscScalar *array,void *ptr)
{
  PetscInt       start, end;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm, DM_CLASSID, 1);
  PetscValidScalarPointer(array, 3);
  PetscValidPointer(ptr, 4);
  ierr = DMPlexGetGlobalOffset_Private(dm, point, &start, &end);CHKERRQ(ierr);
  *(PetscScalar**) ptr = (start < end) ? array + start - dm->map->rstart : NULL;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMPlexGetPointGlobalField"
/*@
  DMPlexGetPointGlobalField - get location of point field data in global Vec

  Not Collective

  Input Arguments:
+ dm - DM defining the topological space
. point - topological point
- field - the field number

  Output Arguments:
+ start - start of point data; returns -(globalStart+1) if point is not owned
- end - end of point data; returns -(globalEnd+1) if point is not owned

  Note: This is a half open interval [start, end)

  Level: intermediate

.seealso: DMPlexGetPointGlobal(), DMGetDefaultSection(), PetscSectionGetOffset(), PetscSectionGetDof(), DMPlexPointGlobalRead(), DMPlexGetPointLocal(), DMPlexPointGlobalRead(), DMPlexPointGlobalRef()
@*/
PetscErrorCode DMPlexGetPointGlobalField(DM dm, PetscInt point, PetscInt field, PetscInt *start, PetscInt *end)
{
  PetscInt       s, e;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm, DM_CLASSID, 1);
  if (start) PetscValidPointer(start, 4);
  if (end)   PetscValidPointer(end,   5);
  ierr = DMPlexGetGlobalFieldOffset_Private(dm, point, field, &s, &e);CHKERRQ(ierr);
  if (start) *start = s;
  if (end)   *end   = e;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMPlexPointGlobalFieldRead"
/*@
   DMPlexPointGlobalFieldRead - return read access to a field on a point in global array

   Not Collective

   Input Arguments:
+  dm - DM defining topological space
.  point - topological point
.  field - field number
-  array - array to index into

   Output Arguments:
.  ptr - address of read reference to point data, type generic so user can place in structure; returns NULL if global point is not owned

   Level: intermediate

.seealso: DMGetDefaultSection(), PetscSectionGetOffset(), PetscSectionGetDof(), DMPlexGetPointGlobal(), DMPlexPointLocalRead(), DMPlexPointGlobalRef()
@*/
PetscErrorCode DMPlexPointGlobalFieldRead(DM dm,PetscInt point,PetscInt field,const PetscScalar *array,const void *ptr)
{
  PetscInt       start, end;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm, DM_CLASSID, 1);
  PetscValidScalarPointer(array, 3);
  PetscValidPointer(ptr, 4);
  ierr = DMPlexGetGlobalFieldOffset_Private(dm, point, field, &start, &end);CHKERRQ(ierr);
  *(const PetscScalar**) ptr = (start < end) ? array + start - dm->map->rstart : NULL;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMPlexPointGlobalFieldRef"
/*@
   DMPlexPointGlobalFieldRef - return read/write access to a field on a point in global array

   Not Collective

   Input Arguments:
+  dm - DM defining topological space
.  point - topological point
.  field - field number
-  array - array to index into

   Output Arguments:
.  ptr - address of reference to point data, type generic so user can place in structure; returns NULL if global point is not owned

   Level: intermediate

.seealso: DMGetDefaultSection(), PetscSectionGetOffset(), PetscSectionGetDof(), DMPlexGetPointGlobal(), DMPlexPointLocalRef(), DMPlexPointGlobalRead()
@*/
PetscErrorCode DMPlexPointGlobalFieldRef(DM dm,PetscInt point,PetscInt field,PetscScalar *array,void *ptr)
{
  PetscInt       start, end;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm, DM_CLASSID, 1);
  PetscValidScalarPointer(array, 3);
  PetscValidPointer(ptr, 4);
  ierr = DMPlexGetGlobalFieldOffset_Private(dm, point, field, &start, &end);CHKERRQ(ierr);
  *(PetscScalar**) ptr = (start < end) ? array + start - dm->map->rstart : NULL;
  PetscFunctionReturn(0);
}
