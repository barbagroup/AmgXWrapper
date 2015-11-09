
/*
   This file contains routines to open an X window display and window
   This consists of a number of routines that set the various
   fields in the Window structure, which is passed to
   all of these routines.

   Note that if you use the default visual and colormap, then you
   can use these routines with any X toolkit that will give you the
   Window id of the window that it is managing.  Use that instead of the
   call to PetscDrawXiCreateWindow .  Similarly for the Display.
*/

#include <../src/sys/classes/draw/impls/x/ximpl.h>

extern PetscErrorCode PetscDrawXiUniformHues(PetscDraw_X*,int);
extern PetscErrorCode PetscDrawXi_wait_map(PetscDraw_X*);
extern PetscErrorCode PetscDrawXiFontFixed(PetscDraw_X*,int,int,PetscDrawXiFont**);
extern PetscErrorCode PetscDrawXiInitCmap(PetscDraw_X*);
extern PetscErrorCode PetscDrawSetColormap_X(PetscDraw_X*,char*,Colormap);

/*
  PetscDrawXiOpenDisplay - Open a display
*/
#undef __FUNCT__
#define __FUNCT__ "PetscDrawXiOpenDisplay"
PetscErrorCode PetscDrawXiOpenDisplay(PetscDraw_X *XiWin,char *display_name)
{
  PetscFunctionBegin;
  XiWin->disp = XOpenDisplay(display_name);
  if (!XiWin->disp) {
    SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"Unable to open display on %s\n.  Make sure your COMPUTE NODES are authorized to connect \n\
    to this X server and either your DISPLAY variable\n\
    is set or you use the -display name option\n",display_name);
  }
  XiWin->screen = DefaultScreen(XiWin->disp);
  PetscFunctionReturn(0);
}


/*
   PetscDrawXiSetGC - set the GC structure in the base window
*/
#undef __FUNCT__
#define __FUNCT__ "PetscDrawXiSetGC"
PetscErrorCode PetscDrawXiSetGC(PetscDraw_X *XiWin,PetscDrawXiPixVal fg)
{
  XGCValues gcvalues;             /* window graphics context values */

  PetscFunctionBegin;
  /* Set the graphics contexts */
  /* create a gc for the ROP_SET operation (writing the fg value to a pixel) */
  /* (do this with function GXcopy; GXset will automatically write 1) */
  gcvalues.function   = GXcopy;
  gcvalues.foreground = fg;
  XiWin->gc.cur_pix   = fg;
  XiWin->gc.set       = XCreateGC(XiWin->disp,RootWindow(XiWin->disp,XiWin->screen),GCFunction | GCForeground,&gcvalues);
  PetscFunctionReturn(0);
}

/*
    Actually display a window at [x,y] with sizes (w,h)
    If w and/or h are 0, use the sizes in the fields of XiWin
    (which may have been set by, for example, PetscDrawXiSetWindowSize)
*/
#undef __FUNCT__
#define __FUNCT__ "PetscDrawXiDisplayWindow"
PetscErrorCode PetscDrawXiDisplayWindow(PetscDraw_X *XiWin,char *label,int x,int y,int w,int h,PetscDrawXiPixVal backgnd_pixel)
{
  unsigned int         wavail,havail;
  XSizeHints           size_hints;
  XWindowAttributes    in_window_attributes;
  XSetWindowAttributes window_attributes;
  int                  depth,border_width;
  unsigned long        wmask;
  PetscBool            flg;
  PetscErrorCode       ierr;

  PetscFunctionBegin;
  /* get the available widths */
  wavail = DisplayWidth(XiWin->disp,XiWin->screen);
  havail = DisplayHeight(XiWin->disp,XiWin->screen);
  if (w <= 0 || h <= 0) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_LIB,"X Window display has invalid height or width");
  if ((unsigned int) w > wavail) w = wavail;
  if ((unsigned int) h > havail) h = havail;

  border_width = 0;
  if (x < 0) x = 0;
  if (y < 0) y = 0;
  x = ((unsigned int) x + w > wavail) ? wavail - w : (unsigned int)x;
  y = ((unsigned int) y + h > havail) ? havail - h : (unsigned int)y;

  /* We need XCreateWindow since we may need an visual other than the default one */
  XGetWindowAttributes(XiWin->disp,RootWindow(XiWin->disp,XiWin->screen),&in_window_attributes);
  window_attributes.background_pixmap = None;
  window_attributes.background_pixel  = backgnd_pixel;
  /* No border for now */
  window_attributes.border_pixmap     = None;
  /*
  window_attributes.border_pixel      = border_pixel;
  */
  window_attributes.bit_gravity       = in_window_attributes.bit_gravity;
  window_attributes.win_gravity       = in_window_attributes.win_gravity;
  /* Backing store is too slow in color systems */
  window_attributes.backing_store     = 0;
  window_attributes.backing_pixel     = backgnd_pixel;
  window_attributes.save_under        = 1;
  window_attributes.event_mask        = 0;
  window_attributes.do_not_propagate_mask = 0;
  window_attributes.override_redirect = 0;
  window_attributes.colormap          = XiWin->cmap;
  /* None for cursor does NOT mean none, it means cursor of Parent */
  window_attributes.cursor            = None;

  wmask = CWBackPixmap | CWBackPixel    | CWBorderPixmap  | CWBitGravity |
          CWWinGravity | CWBackingStore |CWBackingPixel   |CWOverrideRedirect |
          CWSaveUnder  | CWEventMask    | CWDontPropagate |
          CWCursor     | CWColormap;
  depth = XiWin->depth;

  XiWin->win = XCreateWindow(XiWin->disp,RootWindow(XiWin->disp,XiWin->screen),x,y,w,h,border_width,depth,InputOutput,XiWin->vis,wmask,&window_attributes);
  if (!XiWin->win) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_LIB,"Unable to open X window");

  /* set window manager hints */
  {
    XWMHints      wm_hints;
    XClassHint    class_hints;
    XTextProperty windowname,iconname;

    if (label) XStringListToTextProperty(&label,1,&windowname);
    else       XStringListToTextProperty(&label,0,&windowname);
    if (label) XStringListToTextProperty(&label,1,&iconname);
    else       XStringListToTextProperty(&label,0,&iconname);

    wm_hints.initial_state = NormalState;
    wm_hints.input         = True;
    wm_hints.flags         = StateHint|InputHint;

    /* These properties can be used by window managers to decide how to display a window */
    class_hints.res_name  = (char*)"petsc";
    class_hints.res_class = (char*)"PETSc";

    size_hints.x          = x;
    size_hints.y          = y;
    size_hints.min_width  = 4*border_width;
    size_hints.min_height = 4*border_width;
    size_hints.width      = w;
    size_hints.height     = h;
    size_hints.flags      = USPosition | USSize | PMinSize;

    XSetWMProperties(XiWin->disp,XiWin->win,&windowname,&iconname,0,0,&size_hints,&wm_hints,&class_hints);
    XFree((void*)windowname.value);
    XFree((void*)iconname.value);
  }
  /* make the window visible */
  XSelectInput(XiWin->disp,XiWin->win,ExposureMask | StructureNotifyMask);
  XMapWindow(XiWin->disp,XiWin->win);


  /* some window systems are cruel and interfere with the placement of
     windows.  We wait here for the window to be created or to die */
  if (PetscDrawXi_wait_map(XiWin)) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_LIB,"Wait for X window failed");

  flg  = PETSC_FALSE;
  ierr = PetscOptionsGetBool(NULL,NULL,"-draw_virtual",&flg,NULL);CHKERRQ(ierr);
  if (flg) {
    XiWin->drw = XCreatePixmap(XiWin->disp,XiWin->win,XiWin->w,XiWin->h,XiWin->depth);
    XDestroyWindow(XiWin->disp,XiWin->win);
    XiWin->win = 0;
    PetscFunctionReturn(0);
  }

  /* Initial values for the upper left corner */
  XiWin->x = 0;
  XiWin->y = 0;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDrawXiQuickWindow"
PetscErrorCode PetscDrawXiQuickWindow(PetscDraw_X *w,char *host,char *name,int x,int y,int nx,int ny)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscDrawXiOpenDisplay(w,host);CHKERRQ(ierr);

  w->vis   = DefaultVisual(w->disp,w->screen);
  w->depth = DefaultDepth(w->disp,w->screen);

  ierr = PetscDrawSetColormap_X(w,host,(Colormap)0);CHKERRQ(ierr);

  ierr = PetscDrawXiDisplayWindow(w,name,x,y,nx,ny,(PetscDrawXiPixVal)0);CHKERRQ(ierr);
  PetscDrawXiSetGC(w,w->cmapping[1]);
  PetscDrawXiSetPixVal(w,w->background);

  ierr = PetscDrawXiFontFixed(w,6,10,&w->font);CHKERRQ(ierr);
  if (w->win) {
    XSetWindowBackground(w->disp,w->win,w->cmapping[0]);
    XFillRectangle(w->disp,w->win,w->gc.set,0,0,nx,ny);
  }
  PetscFunctionReturn(0);
}

/*
   A version from an already defined window
*/
#undef __FUNCT__
#define __FUNCT__ "PetscDrawXiQuickWindowFromWindow"
PetscErrorCode PetscDrawXiQuickWindowFromWindow(PetscDraw_X *w,char *host,Window win)
{
  Window            root;
  PetscErrorCode    ierr;
  int               d;
  unsigned int      ud;
  XWindowAttributes attributes;

  PetscFunctionBegin;
  ierr   = PetscDrawXiOpenDisplay(w,host);CHKERRQ(ierr);
  w->win = win;
  XGetWindowAttributes(w->disp,w->win,&attributes);

  w->vis   = DefaultVisual(w->disp,w->screen);
  w->depth = DefaultDepth(w->disp,w->screen);
  ierr     = PetscDrawSetColormap_X(w,host,attributes.colormap);CHKERRQ(ierr);

  XGetGeometry(w->disp,w->win,&root,&d,&d,(unsigned int*)&w->w,(unsigned int*)&w->h,&ud,&ud);
  w->x = w->y = 0;

  PetscDrawXiSetGC(w,w->cmapping[1]);
  PetscDrawXiSetPixVal(w,w->background);
  XSetWindowBackground(w->disp,w->win,w->cmapping[0]);
  ierr = PetscDrawXiFontFixed(w,6,10,&w->font);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/*
      PetscDrawXiSetWindowLabel - Sets new label in open window.
*/
#undef __FUNCT__
#define __FUNCT__ "PetscDrawXiSetWindowLabel"
PetscErrorCode PetscDrawXiSetWindowLabel(PetscDraw_X *Xiwin,char *label)
{
  XTextProperty  prop;
  size_t         len;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  XGetWMName(Xiwin->disp,Xiwin->win,&prop);
  prop.value  = (unsigned char*)label;
  ierr        = PetscStrlen(label,&len);CHKERRQ(ierr);
  prop.nitems = (long) len;
  XSetWMName(Xiwin->disp,Xiwin->win,&prop);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDrawXiSetToBackground"
PetscErrorCode PetscDrawXiSetToBackground(PetscDraw_X *XiWin)
{
  PetscFunctionBegin;
  if (XiWin->gc.cur_pix != XiWin->background) {
    XSetForeground(XiWin->disp,XiWin->gc.set,XiWin->background);
    XiWin->gc.cur_pix = XiWin->background;
  }
  PetscFunctionReturn(0);

}

#undef __FUNCT__
#define __FUNCT__ "PetscDrawSetSave_X"
PetscErrorCode  PetscDrawSetSave_X(PetscDraw draw,const char *filename)
{
  PetscErrorCode ierr;
#if defined(PETSC_HAVE_POPEN)
  PetscMPIInt    rank;
#endif

  PetscFunctionBegin;
  PetscValidHeaderSpecific(draw,PETSC_DRAW_CLASSID,1);
#if defined(PETSC_HAVE_POPEN)
  ierr = MPI_Comm_rank(PetscObjectComm((PetscObject)draw),&rank);CHKERRQ(ierr);
  if (!rank) {
    char  command[PETSC_MAX_PATH_LEN];
    FILE  *fd;
    int   err;

    ierr = PetscMemzero(command,sizeof(command));CHKERRQ(ierr);
    ierr = PetscSNPrintf(command,PETSC_MAX_PATH_LEN,"rm -fr %s %s.m4v",draw->savefilename,draw->savefilename);CHKERRQ(ierr);
    ierr = PetscPOpen(PETSC_COMM_SELF,NULL,command,"r",&fd);CHKERRQ(ierr);
    ierr = PetscPClose(PETSC_COMM_SELF,fd,&err);CHKERRQ(ierr);
    ierr = PetscSNPrintf(command,PETSC_MAX_PATH_LEN,"mkdir %s",draw->savefilename);CHKERRQ(ierr);
    ierr = PetscPOpen(PETSC_COMM_SELF,NULL,command,"r",&fd);CHKERRQ(ierr);
    ierr = PetscPClose(PETSC_COMM_SELF,fd,&err);CHKERRQ(ierr);
  }
#endif
  PetscFunctionReturn(0);
}


#if defined(PETSC_HAVE_AFTERIMAGE)
#include <afterimage.h>

/* String names of possible Afterimage formats */
const char *PetscAfterImageFormats[] = {
        ".Xpm",
	".Xpm.Z",
	".Xpm.gz",
	".Png",
	".Jpeg",
	".Xcf", /* Gimp format */
	".Ppm",
	".Pnm",
	"MS Windows Bitmap",
	"MS Windows Icon",
	"MS Windows Cursor",
	".Gif",
	".Tiff",
	"Afterstep XMLScript",
	"Scalable Vector Graphics (SVG)",
	".Xbm",
	"Targa",
	".Pcx",
	".HTML",
	"XML",
	"Unknown"
};

#undef __FUNCT__
#define __FUNCT__ "PetscAfterimageStringToFormat"
static PetscErrorCode PetscAfterimageStringToFormat(const char *ext,ASImageFileTypes *format)
{
  PetscInt       i;
  PetscErrorCode ierr;
  PetscBool      flg;

  PetscFunctionBegin;
  ierr = PetscStrcasecmp(".Jpg",ext,&flg);CHKERRQ(ierr);
  if (flg) ext = ".Jpeg";
  for (i=0; i<sizeof(PetscAfterImageFormats)/sizeof(char**); i++) {
    ierr = PetscStrcasecmp(PetscAfterImageFormats[i],ext,&flg);CHKERRQ(ierr);
    if (flg) {
      *format = (ASImageFileTypes)i;
      PetscFunctionReturn(0);
    }
  }
  *format = ASIT_Unknown;
  PetscFunctionReturn(0);
}

#if defined(PETSC_HAVE_SAWS)
#include <petscviewersaws.h>
/*
  The PetscAfterimage object and functions are used to maintain a list of file images created by Afterimage that can
  be displayed by the SAWs webserver.
*/
typedef struct _P_PetscAfterimage *PetscAfterimage;
struct _P_PetscAfterimage {
  PetscAfterimage next;
  char            *filename;
  char            *ext;
  PetscInt        cnt;
} ;

static PetscAfterimage afterimages = 0;

#undef __FUNCT__
#define __FUNCT__ "PetscAfterimageDestroy"
static PetscErrorCode PetscAfterimageDestroy(void)
{
  PetscErrorCode ierr;
  PetscAfterimage       afterimage,oafterimage = afterimages;

  PetscFunctionBegin;
  while (oafterimage) {
    afterimage = oafterimage->next;
    ierr = PetscFree(oafterimage->filename);CHKERRQ(ierr);
    ierr = PetscFree(oafterimage->ext);CHKERRQ(ierr);
    ierr = PetscFree(oafterimage);CHKERRQ(ierr);
    oafterimage = afterimage;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscAfterimageAdd"
static PetscErrorCode PetscAfterimageAdd(const char *filename,const char *ext,PetscInt cnt)
{
  PetscErrorCode   ierr;
  PetscAfterimage  afterimage,oafterimage = afterimages;
  PetscBool        flg;

  PetscFunctionBegin;
  if (oafterimage){
    ierr = PetscStrcmp(filename,oafterimage->filename,&flg);CHKERRQ(ierr);
    if (flg) {
      oafterimage->cnt = cnt;
      PetscFunctionReturn(0);
    }
    while (oafterimage->next) {
      oafterimage = oafterimage->next;
      ierr = PetscStrcmp(filename,oafterimage->filename,&flg);CHKERRQ(ierr);
      if (flg) {
        oafterimage->cnt = cnt;
        PetscFunctionReturn(0);
      }
    }
    ierr = PetscNew(&afterimage);CHKERRQ(ierr);
    oafterimage->next = afterimage;
  } else {
    ierr = PetscNew(&afterimage);CHKERRQ(ierr);
    afterimages = afterimage;
  }
  ierr = PetscStrallocpy(filename,&afterimage->filename);CHKERRQ(ierr);
  ierr = PetscStrallocpy(ext,&afterimage->ext);CHKERRQ(ierr);
  afterimage->cnt = cnt;
  ierr = PetscRegisterFinalize(PetscAfterimageDestroy);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#endif

#undef __FUNCT__
#define __FUNCT__ "PetscDrawSave_X"
PetscErrorCode PetscDrawSave_X(PetscDraw draw)
{
  PetscDraw_X      *drawx = (PetscDraw_X*)draw->data;
  XImage           *image;
  ASImage          *asimage;
  struct  ASVisual *asv;
  char             filename[PETSC_MAX_PATH_LEN];
  PetscErrorCode   ierr;
  PetscMPIInt      rank;
  int              depth;
  ASImageFileTypes format;

  PetscFunctionBegin;
  ierr = MPI_Comm_rank(PetscObjectComm((PetscObject)draw),&rank);CHKERRQ(ierr);
  if (rank) PetscFunctionReturn(0);
  if (!draw->savefilename) PetscFunctionReturn(0);
  if (draw->savefilecount == -1) {
    /* The first PetscDrawClear() should happen before any drawing has been done, hence do not save at the first PetscDrawClear() */
    draw->savefilecount++;
    PetscFunctionReturn(0);
  }
  XSynchronize(drawx->disp, True);
  depth = DefaultDepth( drawx->disp, drawx->screen );
  asv   = create_asvisual(drawx->disp, drawx->screen, depth, NULL);if (!asv) SETERRQ(PetscObjectComm((PetscObject)draw),PETSC_ERR_PLIB,"Cannot create AfterImage ASVisual");

  image   = XGetImage(drawx->disp, drawx->drw ? drawx->drw : drawx->win, 0, 0, drawx->w, drawx->h, AllPlanes, ZPixmap);
  if (!image) SETERRQ(PetscObjectComm((PetscObject)draw),PETSC_ERR_PLIB,"Cannot XGetImage()");
  asimage = picture_ximage2asimage (asv,image,0,0);if (!asimage) SETERRQ(PetscObjectComm((PetscObject)draw),PETSC_ERR_PLIB,"Cannot create AfterImage ASImage");
  if (draw->savesinglefile) {
    ierr    = PetscSNPrintf(filename,PETSC_MAX_PATH_LEN,"%s/%s%s",draw->savefilename,draw->savefilename,draw->savefilenameext);CHKERRQ(ierr);
  } else {
    ierr    = PetscSNPrintf(filename,PETSC_MAX_PATH_LEN,"%s/%s_%d%s",draw->savefilename,draw->savefilename,draw->savefilecount++,draw->savefilenameext);CHKERRQ(ierr);
  }
  ierr = PetscAfterimageStringToFormat(draw->savefilenameext,&format);CHKERRQ(ierr);
  ASImage2file(asimage, 0, filename,format,0);
#if defined(PETSC_HAVE_SAWS)
  {
    char     body[4096];
    PetscAfterimage afterimage;
    size_t   len = 0;

    ierr = PetscAfterimageAdd(draw->savefilename,draw->savefilenameext,draw->savefilecount-1);CHKERRQ(ierr);
    afterimage  = afterimages;
    while (afterimage) {
      if (draw->savesinglefile) {
        ierr = PetscSNPrintf(body+len,4086-len,"<img src=\"%s/%s%s\" alt=\"None\">",afterimage->filename,afterimage->filename,afterimage->ext);CHKERRQ(ierr);
      } else {
        ierr = PetscSNPrintf(body+len,4086-len,"<img src=\"%s/%s_%d%s\" alt=\"None\">",afterimage->filename,afterimage->filename,afterimage->cnt,afterimage->ext);CHKERRQ(ierr);
      }
      ierr = PetscStrlen(body,&len);CHKERRQ(ierr);
      afterimage  = afterimage->next;
    }
    ierr = PetscStrcat(body,"<br>\n");CHKERRQ(ierr);
    if (draw->savefilecount > 0) PetscStackCallSAWs(SAWs_Pop_Body,("index.html",1));
    PetscStackCallSAWs(SAWs_Push_Body,("index.html",1,body));
  }
#endif

  XDestroyImage(image);
  destroy_asvisual(asv,0);
  PetscFunctionReturn(0);
}
/*
   There are routines wanted by AfterImage for PNG files
 */
void crc32(void) {;}
void inflateReset(void) {;}
void deflateReset(void) {;}
void deflateInit2(void) {;}
void deflateInit2_(void) {;}
void deflate(void) {;}
void deflateEnd(void) {;}

#endif



