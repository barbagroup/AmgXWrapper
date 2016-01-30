#include <petsc/private/matimpl.h>      /*I "petscmat.h"  I*/
#include <../src/mat/impls/aij/seq/aij.h>
#include <../src/mat/impls/aij/mpi/mpiaij.h>
#include <petscsf.h>

typedef struct {
  PetscSF    sf;
  PetscReal *dwts,*owts;
  PetscInt  *dmask,*omask,*cmask;
  PetscBool local;
} MC_JP;

#undef __FUNCT__
#define __FUNCT__ "MatColoringDestroy_JP"
PetscErrorCode MatColoringDestroy_JP(MatColoring mc)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscFree(mc->data);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatColoringSetFromOptions_JP"
PetscErrorCode MatColoringSetFromOptions_JP(PetscOptionItems *PetscOptionsObject,MatColoring mc)
{
  PetscErrorCode ierr;
  MC_JP          *jp = (MC_JP*)mc->data;

  PetscFunctionBegin;
  ierr = PetscOptionsHead(PetscOptionsObject,"JP options");CHKERRQ(ierr);
  ierr = PetscOptionsBool("-mat_coloring_jp_local","Do an initial coloring of local columns","",jp->local,&jp->local,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsTail();CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MCJPGreatestWeight_Private"
PetscErrorCode MCJPGreatestWeight_Private(MatColoring mc,const PetscReal *weights,PetscReal *maxweights)
{
  MC_JP          *jp = (MC_JP*)mc->data;
  PetscErrorCode ierr;
  Mat            G=mc->mat,dG,oG;
  PetscBool      isSeq,isMPI;
  Mat_MPIAIJ     *aij;
  Mat_SeqAIJ     *daij,*oaij;
  PetscInt       *di,*oi,*dj,*oj;
  PetscSF        sf=jp->sf;
  PetscLayout    layout;
  PetscInt       dn,on;
  PetscInt       i,j,l;
  PetscReal      *dwts=jp->dwts,*owts=jp->owts;
  PetscInt       ncols;
  const PetscInt *cols;

  PetscFunctionBegin;
  ierr = PetscObjectTypeCompare((PetscObject)G,MATSEQAIJ,&isSeq);CHKERRQ(ierr);
  ierr = PetscObjectTypeCompare((PetscObject)G,MATMPIAIJ,&isMPI);CHKERRQ(ierr);
  if (!isSeq && !isMPI) SETERRQ(PetscObjectComm((PetscObject)G),PETSC_ERR_ARG_WRONGSTATE,"MatColoringDegrees requires an MPI/SEQAIJ Matrix");

  /* get the inner matrix structure */
  oG = NULL;
  oi = NULL;
  oj = NULL;
  if (isMPI) {
    aij = (Mat_MPIAIJ*)G->data;
    dG = aij->A;
    oG = aij->B;
    daij = (Mat_SeqAIJ*)dG->data;
    oaij = (Mat_SeqAIJ*)oG->data;
    di = daij->i;
    dj = daij->j;
    oi = oaij->i;
    oj = oaij->j;
    ierr = MatGetSize(oG,&dn,&on);CHKERRQ(ierr);
    if (!sf) {
      ierr = PetscSFCreate(PetscObjectComm((PetscObject)mc),&sf);CHKERRQ(ierr);
      ierr = MatGetLayouts(G,&layout,NULL);CHKERRQ(ierr);
      ierr = PetscSFSetGraphLayout(sf,layout,on,NULL,PETSC_COPY_VALUES,aij->garray);CHKERRQ(ierr);
      jp->sf = sf;
    }
  } else {
    dG = G;
    ierr = MatGetSize(dG,NULL,&dn);CHKERRQ(ierr);
    daij = (Mat_SeqAIJ*)dG->data;
    di = daij->i;
    dj = daij->j;
  }
  /* set up the distance-zero weights */
  if (!dwts) {
    ierr = PetscMalloc1(dn,&dwts);CHKERRQ(ierr);
    jp->dwts = dwts;
    if (oG) {
      ierr = PetscMalloc1(on,&owts);CHKERRQ(ierr);
      jp->owts = owts;
    }
  }
  for (i=0;i<dn;i++) {
    maxweights[i] = weights[i];
    dwts[i] = maxweights[i];
  }
  /* get the off-diagonal weights */
  if (oG) {
    ierr = PetscLogEventBegin(Mat_Coloring_Comm,mc,0,0,0);CHKERRQ(ierr);
    ierr = PetscSFBcastBegin(sf,MPIU_REAL,dwts,owts);CHKERRQ(ierr);
    ierr = PetscSFBcastEnd(sf,MPIU_REAL,dwts,owts);CHKERRQ(ierr);
    ierr = PetscLogEventEnd(Mat_Coloring_Comm,mc,0,0,0);CHKERRQ(ierr);
  }
  /* check for the maximum out to the distance of the coloring */
  for (l=0;l<mc->dist;l++) {
    /* check for on-diagonal greater weights */
    for (i=0;i<dn;i++) {
      ncols = di[i+1]-di[i];
      cols = &(dj[di[i]]);
      for (j=0;j<ncols;j++) {
        if (dwts[cols[j]] > maxweights[i]) maxweights[i] = dwts[cols[j]];
      }
      /* check for off-diagonal greater weights */
      if (oG) {
        ncols = oi[i+1]-oi[i];
        cols = &(oj[oi[i]]);
        for (j=0;j<ncols;j++) {
          if (owts[cols[j]] > maxweights[i]) maxweights[i] = owts[cols[j]];
        }
      }
    }
    if (l < mc->dist-1) {
      for (i=0;i<dn;i++) {
        dwts[i] = maxweights[i];
      }
      if (oG) {
        ierr = PetscLogEventBegin(Mat_Coloring_Comm,mc,0,0,0);CHKERRQ(ierr);
        ierr = PetscSFBcastBegin(sf,MPIU_REAL,dwts,owts);CHKERRQ(ierr);
        ierr = PetscSFBcastEnd(sf,MPIU_REAL,dwts,owts);CHKERRQ(ierr);
        ierr = PetscLogEventEnd(Mat_Coloring_Comm,mc,0,0,0);CHKERRQ(ierr);
      }
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MCJPInitialLocalColor_Private"
PetscErrorCode MCJPInitialLocalColor_Private(MatColoring mc,PetscInt *lperm,ISColoringValue *colors)
{
  PetscInt       j,i,s,e,n,bidx,cidx,idx,dist,distance=mc->dist;
  Mat            G=mc->mat,dG,oG;
  PetscErrorCode ierr;
  PetscInt       *seen;
  PetscInt       *idxbuf;
  PetscBool      *boundary;
  PetscInt       *distbuf;
  PetscInt      *colormask;
  PetscInt       ncols;
  const PetscInt *cols;
  PetscBool      isSeq,isMPI;
  Mat_MPIAIJ     *aij;
  Mat_SeqAIJ     *daij,*oaij;
  PetscInt       *di,*dj,dn;
  PetscInt       *oi;

  PetscFunctionBegin;
  ierr = PetscLogEventBegin(Mat_Coloring_Local,mc,0,0,0);CHKERRQ(ierr);
  ierr = MatGetOwnershipRange(G,&s,&e);CHKERRQ(ierr);
  n=e-s;
  ierr = PetscObjectTypeCompare((PetscObject)G,MATSEQAIJ,&isSeq);CHKERRQ(ierr);
  ierr = PetscObjectTypeCompare((PetscObject)G,MATMPIAIJ,&isMPI);CHKERRQ(ierr);
  if (!isSeq && !isMPI) SETERRQ(PetscObjectComm((PetscObject)G),PETSC_ERR_ARG_WRONGSTATE,"MatColoringDegrees requires an MPI/SEQAIJ Matrix");

  /* get the inner matrix structure */
  oG = NULL;
  oi = NULL;
  if (isMPI) {
    aij = (Mat_MPIAIJ*)G->data;
    dG = aij->A;
    oG = aij->B;
    daij = (Mat_SeqAIJ*)dG->data;
    oaij = (Mat_SeqAIJ*)oG->data;
    di = daij->i;
    dj = daij->j;
    oi = oaij->i;
    ierr = MatGetSize(oG,&dn,NULL);CHKERRQ(ierr);
  } else {
    dG = G;
    ierr = MatGetSize(dG,NULL,&dn);CHKERRQ(ierr);
    daij = (Mat_SeqAIJ*)dG->data;
    di = daij->i;
    dj = daij->j;
  }
  ierr = PetscMalloc5(n,&colormask,n,&seen,n,&idxbuf,n,&distbuf,n,&boundary);CHKERRQ(ierr);
  for (i=0;i<dn;i++) {
    seen[i]=-1;
    colormask[i] = -1;
    boundary[i] = PETSC_FALSE;
  }
  /* pass one -- figure out which ones are off-boundary in the distance-n sense */
  if (oG) {
    for (i=0;i<dn;i++) {
      bidx=-1;
      /* nonempty off-diagonal, so this one is on the boundary */
      if (oi[i]!=oi[i+1]) {
        boundary[i] = PETSC_TRUE;
        continue;
      }
      ncols = di[i+1]-di[i];
      cols = &(dj[di[i]]);
      for (j=0;j<ncols;j++) {
        bidx++;
        seen[cols[j]] = i;
        distbuf[bidx] = 1;
        idxbuf[bidx] = cols[j];
      }
      while (bidx >= 0) {
        idx = idxbuf[bidx];
        dist = distbuf[bidx];
        bidx--;
        if (dist < distance) {
          if (oi[idx+1]!=oi[idx]) {
            boundary[i] = PETSC_TRUE;
            break;
          }
          ncols = di[idx+1]-di[idx];
          cols = &(dj[di[idx]]);
          for (j=0;j<ncols;j++) {
            if (seen[cols[j]] != i) {
              bidx++;
              seen[cols[j]] = i;
              idxbuf[bidx] = cols[j];
              distbuf[bidx] = dist+1;
            }
          }
        }
      }
    }
    for (i=0;i<dn;i++) {
      seen[i]=-1;
    }
  }
  /* pass two -- color it by looking at nearby vertices and building a mask */
  for (i=0;i<dn;i++) {
    cidx = lperm[i];
    if (!boundary[cidx]) {
      bidx=-1;
      ncols = di[cidx+1]-di[cidx];
      cols = &(dj[di[cidx]]);
      for (j=0;j<ncols;j++) {
        bidx++;
        seen[cols[j]] = cidx;
        distbuf[bidx] = 1;
        idxbuf[bidx] = cols[j];
      }
      while (bidx >= 0) {
        idx = idxbuf[bidx];
        dist = distbuf[bidx];
        bidx--;
        /* mask this color */
        if (colors[idx] < IS_COLORING_MAX) {
          colormask[colors[idx]] = cidx;
        }
        if (dist < distance) {
          ncols = di[idx+1]-di[idx];
          cols = &(dj[di[idx]]);
          for (j=0;j<ncols;j++) {
            if (seen[cols[j]] != cidx) {
              bidx++;
              seen[cols[j]] = cidx;
              idxbuf[bidx] = cols[j];
              distbuf[bidx] = dist+1;
            }
          }
        }
      }
      /* find the lowest untaken color */
      for (j=0;j<n;j++) {
        if (colormask[j] != cidx || j >= mc->maxcolors) {
          colors[cidx] = j;
          break;
        }
      }
    }
  }
  ierr = PetscFree5(colormask,seen,idxbuf,distbuf,boundary);CHKERRQ(ierr);
  ierr = PetscLogEventEnd(Mat_Coloring_Local,mc,0,0,0);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MCJPMinColor_Private"
PetscErrorCode MCJPMinColor_Private(MatColoring mc,ISColoringValue maxcolor,const ISColoringValue *colors,ISColoringValue *mincolors)
{
  MC_JP          *jp = (MC_JP*)mc->data;
  PetscErrorCode ierr;
  Mat            G=mc->mat,dG,oG;
  PetscBool      isSeq,isMPI;
  Mat_MPIAIJ     *aij;
  Mat_SeqAIJ     *daij,*oaij;
  PetscInt       *di,*oi,*dj,*oj;
  PetscSF        sf=jp->sf;
  PetscLayout    layout;
  PetscInt       maskrounds,maskbase,maskradix;
  PetscInt       dn,on;
  PetscInt       i,j,l,k;
  PetscInt       *dmask=jp->dmask,*omask=jp->omask,*cmask=jp->cmask,curmask;
  PetscInt       ncols;
  const PetscInt *cols;

  PetscFunctionBegin;
  maskradix = sizeof(PetscInt)*8;
  maskrounds = 1 + maxcolor / (maskradix);
  maskbase = 0;
  ierr = PetscObjectTypeCompare((PetscObject)G,MATSEQAIJ,&isSeq);CHKERRQ(ierr);
  ierr = PetscObjectTypeCompare((PetscObject)G,MATMPIAIJ,&isMPI);CHKERRQ(ierr);
  if (!isSeq && !isMPI) SETERRQ(PetscObjectComm((PetscObject)G),PETSC_ERR_ARG_WRONGSTATE,"MatColoringDegrees requires an MPI/SEQAIJ Matrix");

  /* get the inner matrix structure */
  oG = NULL;
  oi = NULL;
  oj = NULL;
  if (isMPI) {
    aij = (Mat_MPIAIJ*)G->data;
    dG = aij->A;
    oG = aij->B;
    daij = (Mat_SeqAIJ*)dG->data;
    oaij = (Mat_SeqAIJ*)oG->data;
    di = daij->i;
    dj = daij->j;
    oi = oaij->i;
    oj = oaij->j;
    ierr = MatGetSize(oG,&dn,&on);CHKERRQ(ierr);
    if (!sf) {
      ierr = PetscSFCreate(PetscObjectComm((PetscObject)mc),&sf);CHKERRQ(ierr);
      ierr = MatGetLayouts(G,&layout,NULL);CHKERRQ(ierr);
      ierr = PetscSFSetGraphLayout(sf,layout,on,NULL,PETSC_COPY_VALUES,aij->garray);CHKERRQ(ierr);
      jp->sf = sf;
    }
  } else {
    dG = G;
    ierr = MatGetSize(dG,NULL,&dn);CHKERRQ(ierr);
    daij = (Mat_SeqAIJ*)dG->data;
    di = daij->i;
    dj = daij->j;
  }
  for (i=0;i<dn;i++) {
    mincolors[i] = IS_COLORING_MAX;
  }
  /* set up the distance-zero mask */
  if (!dmask) {
    ierr = PetscMalloc1(dn,&dmask);CHKERRQ(ierr);
    ierr = PetscMalloc1(dn,&cmask);CHKERRQ(ierr);
    jp->cmask = cmask;
    jp->dmask = dmask;
    if (oG) {
      ierr = PetscMalloc1(on,&omask);CHKERRQ(ierr);
      jp->omask = omask;
    }
  }
  /* the number of colors may be more than the number of bits in a PetscInt; take multiple rounds */
  for (k=0;k<maskrounds;k++) {
    for (i=0;i<dn;i++) {
      cmask[i] = 0;
      if (colors[i] < maskbase+maskradix && colors[i] >= maskbase)
        cmask[i] = 1 << (colors[i]-maskbase);
      dmask[i] = cmask[i];
    }
    if (oG) {
      ierr = PetscLogEventBegin(Mat_Coloring_Comm,mc,0,0,0);CHKERRQ(ierr);
      ierr = PetscSFBcastBegin(sf,MPIU_INT,dmask,omask);CHKERRQ(ierr);
      ierr = PetscSFBcastEnd(sf,MPIU_INT,dmask,omask);CHKERRQ(ierr);
      ierr = PetscLogEventEnd(Mat_Coloring_Comm,mc,0,0,0);CHKERRQ(ierr);
    }
    /* fill in the mask out to the distance of the coloring */
    for (l=0;l<mc->dist;l++) {
      /* fill in the on-and-off diagonal mask */
      for (i=0;i<dn;i++) {
        ncols = di[i+1]-di[i];
        cols = &(dj[di[i]]);
        for (j=0;j<ncols;j++) {
          cmask[i] = cmask[i] | dmask[cols[j]];
        }
        if (oG) {
          ncols = oi[i+1]-oi[i];
          cols = &(oj[oi[i]]);
          for (j=0;j<ncols;j++) {
            cmask[i] = cmask[i] | omask[cols[j]];
          }
        }
      }
      for (i=0;i<dn;i++) {
        dmask[i]=cmask[i];
      }
      if (l < mc->dist-1) {
        if (oG) {
          ierr = PetscLogEventBegin(Mat_Coloring_Comm,mc,0,0,0);CHKERRQ(ierr);
          ierr = PetscSFBcastBegin(sf,MPIU_INT,dmask,omask);CHKERRQ(ierr);
          ierr = PetscSFBcastEnd(sf,MPIU_INT,dmask,omask);CHKERRQ(ierr);
          ierr = PetscLogEventEnd(Mat_Coloring_Comm,mc,0,0,0);CHKERRQ(ierr);
        }
      }
    }
    /* read through the mask to see if we've discovered an acceptable color for any vertices in this round */
    for (i=0;i<dn;i++) {
      if (mincolors[i] == IS_COLORING_MAX) {
        curmask = dmask[i];
        for (j=0;j<maskradix;j++) {
          if (curmask % 2 == 0) {
            mincolors[i] = j+maskbase;
            break;
          }
          curmask = curmask >> 1;
        }
      }
    }
    /* do the next maskradix colors */
    maskbase += maskradix;
  }
  for (i=0;i<dn;i++) {
    if (mincolors[i] == IS_COLORING_MAX) {
      mincolors[i] = maxcolor+1;
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatColoringApply_JP"
PETSC_EXTERN PetscErrorCode MatColoringApply_JP(MatColoring mc,ISColoring *iscoloring)
{
  PetscErrorCode  ierr;
  MC_JP          *jp = (MC_JP*)mc->data;
  PetscInt        i,nadded,nadded_total,nadded_total_old,ntotal,n,round;
  PetscInt        maxcolor_local=0,maxcolor_global,*lperm;
  PetscMPIInt     rank;
  PetscReal       *weights,*maxweights;
  ISColoringValue  *color,*mincolor;

  PetscFunctionBegin;
  ierr = MPI_Comm_rank(PetscObjectComm((PetscObject)mc),&rank);CHKERRQ(ierr);
  ierr = PetscLogEventBegin(Mat_Coloring_Weights,mc,0,0,0);CHKERRQ(ierr);
  ierr = MatColoringCreateWeights(mc,&weights,&lperm);CHKERRQ(ierr);
  ierr = PetscLogEventEnd(Mat_Coloring_Weights,mc,0,0,0);CHKERRQ(ierr);
  ierr = MatGetSize(mc->mat,NULL,&ntotal);CHKERRQ(ierr);
  ierr = MatGetLocalSize(mc->mat,NULL,&n);CHKERRQ(ierr);
  ierr = PetscMalloc1(n,&maxweights);CHKERRQ(ierr);
  ierr = PetscMalloc1(n,&color);CHKERRQ(ierr);
  ierr = PetscMalloc1(n,&mincolor);CHKERRQ(ierr);
  for (i=0;i<n;i++) {
    color[i] = IS_COLORING_MAX;
    mincolor[i] = 0;
  }
  nadded=0;
  nadded_total=0;
  nadded_total_old=0;
  /* compute purely local vertices */
  if (jp->local) {
    ierr = MCJPInitialLocalColor_Private(mc,lperm,color);CHKERRQ(ierr);
    for (i=0;i<n;i++) {
      if (color[i] < IS_COLORING_MAX) {
        nadded++;
        weights[i] = -1;
        if (color[i] > maxcolor_local) maxcolor_local = color[i];
      }
    }
    ierr = MPIU_Allreduce(&nadded,&nadded_total,1,MPIU_INT,MPI_SUM,PetscObjectComm((PetscObject)mc));CHKERRQ(ierr);
    maxcolor_global=0;
    ierr = MPIU_Allreduce(&maxcolor_local,&maxcolor_global,1,MPIU_INT,MPI_MAX,PetscObjectComm((PetscObject)mc));CHKERRQ(ierr);
  }
  round = 0;
  while (nadded_total < ntotal) {
    ierr = MCJPMinColor_Private(mc,(ISColoringValue)maxcolor_global,color,mincolor);CHKERRQ(ierr);
    ierr = MCJPGreatestWeight_Private(mc,weights,maxweights);CHKERRQ(ierr);
    for (i=0;i<n;i++) {
      /* choose locally maximal vertices; weights less than zero are omitted from the graph */
      if (weights[i] >= maxweights[i] && weights[i] >= 0.) {
        /* assign the minimum possible color */
        if (mc->maxcolors > mincolor[i]) {
          color[i] = mincolor[i];
        } else {
          color[i] = mc->maxcolors;
        }
        if (color[i] > maxcolor_local) maxcolor_local = color[i];
        weights[i] = -1.;
        nadded++;
      }
    }
    maxcolor_global = 0;
    ierr = MPIU_Allreduce(&maxcolor_local,&maxcolor_global,1,MPIU_INT,MPI_MAX,PetscObjectComm((PetscObject)mc));CHKERRQ(ierr);
    nadded_total=0;
    ierr = MPIU_Allreduce(&nadded,&nadded_total,1,MPIU_INT,MPI_SUM,PetscObjectComm((PetscObject)mc));CHKERRQ(ierr);
    if (nadded_total == nadded_total_old) SETERRQ(PetscObjectComm((PetscObject)mc),PETSC_ERR_NOT_CONVERGED,"JP didn't make progress");
    nadded_total_old = nadded_total;
    round++;
  }
  ierr = PetscLogEventBegin(Mat_Coloring_ISCreate,mc,0,0,0);CHKERRQ(ierr);
  ierr = ISColoringCreate(PetscObjectComm((PetscObject)mc),maxcolor_global+1,n,color,PETSC_OWN_POINTER,iscoloring);CHKERRQ(ierr);
  ierr = PetscLogEventEnd(Mat_Coloring_ISCreate,mc,0,0,0);CHKERRQ(ierr);
  ierr = PetscFree(jp->dwts);CHKERRQ(ierr);
  ierr = PetscFree(jp->dmask);CHKERRQ(ierr);
  ierr = PetscFree(jp->cmask);CHKERRQ(ierr);
  if (jp->owts) {
    ierr = PetscFree(jp->owts);CHKERRQ(ierr);
    ierr = PetscFree(jp->omask);CHKERRQ(ierr);
  }
  ierr = PetscFree(weights);CHKERRQ(ierr);
  ierr = PetscFree(lperm);CHKERRQ(ierr);
  ierr = PetscFree(maxweights);CHKERRQ(ierr);
  ierr = PetscFree(mincolor);CHKERRQ(ierr);
  if (jp->sf) {ierr = PetscSFDestroy(&jp->sf);CHKERRQ(ierr);}
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatColoringCreate_JP"
/*MC
  MATCOLORINGJP - Parallel Jones-Plassmann Coloring

   Level: beginner

   Notes: This method uses a parallel Luby-style coloring with weights to choose an independent set of processor
   boundary vertices at each stage that may be assigned colors independently.

   References:
.  1. - M. Jones and P. Plassmann, “A parallel graph coloring heuristic,” SIAM Journal on Scientific Computing, vol. 14, no. 3,
   pp. 654–669, 1993.

.seealso: MatColoringCreate(), MatColoring, MatColoringSetType()
M*/
PETSC_EXTERN PetscErrorCode MatColoringCreate_JP(MatColoring mc)
{
  MC_JP          *jp;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr                    = PetscNewLog(mc,&jp);CHKERRQ(ierr);
  jp->sf                  = NULL;
  jp->dmask               = NULL;
  jp->omask               = NULL;
  jp->cmask               = NULL;
  jp->dwts                = NULL;
  jp->owts                = NULL;
  jp->local               = PETSC_TRUE;
  mc->data                = jp;
  mc->ops->apply          = MatColoringApply_JP;
  mc->ops->view           = NULL;
  mc->ops->destroy        = MatColoringDestroy_JP;
  mc->ops->setfromoptions = MatColoringSetFromOptions_JP;
  PetscFunctionReturn(0);
}
