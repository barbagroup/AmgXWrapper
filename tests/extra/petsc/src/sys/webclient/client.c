
#include <petscwebclient.h>
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#pragma gcc diagnostic ignored "-Wdeprecated-declarations"

static BIO *bio_err = NULL;

#define PASSWORD "password"

#if defined(PETSC_USE_SSL_CERTIFICATE)
static int password_cb(char *buf,int num, int rwflag,void *userdata)
{
  if (num < strlen(PASSWORD)+1) return(0);
  strcpy(buf,PASSWORD);
  return(strlen(PASSWORD));
}
#endif

static void sigpipe_handle(int x)
{
}

#undef __FUNCT__
#define __FUNCT__ "PetscSSLInitializeContext"
/*
    PetscSSLInitializeContext - Set up an SSL context suitable for initiating HTTPS requests.

    If built with PETSC_USE_SSL_CERTIFICATE requires the user have created a self-signed certificate with

$    saws/CA.pl  -newcert  (using the passphrase of password)
$    cat newkey.pem newcert.pem > sslclient.pem

    and put the resulting file in either the current directory (with the application) or in the home directory. This seems kind of
    silly but it was all I could figure out.

*/
PetscErrorCode PetscSSLInitializeContext(SSL_CTX **octx)
{
    SSL_CTX        *ctx;
#if defined(PETSC_USE_SSL_CERTIFICATE)
    char           keyfile[PETSC_MAX_PATH_LEN];
    PetscBool      exists;
    PetscErrorCode ierr;
#endif

    PetscFunctionBegin;
    if (!bio_err){
      SSL_library_init();
      SSL_load_error_strings();
      bio_err = BIO_new_fp(stderr,BIO_NOCLOSE);
    }

    /* Set up a SIGPIPE handler */
    signal(SIGPIPE,sigpipe_handle);

    ctx  = SSL_CTX_new(SSLv23_method());
    SSL_CTX_set_mode(ctx,SSL_MODE_AUTO_RETRY);

#if defined(PETSC_USE_SSL_CERTIFICATE)
    /* Locate keyfile */
    ierr = PetscStrcpy(keyfile,"sslclient.pem");CHKERRQ(ierr);
    ierr = PetscTestFile(keyfile,'r',&exists);CHKERRQ(ierr);
    if (!exists) {
      ierr = PetscGetHomeDirectory(keyfile,PETSC_MAX_PATH_LEN);CHKERRQ(ierr);
      ierr = PetscStrcat(keyfile,"/");CHKERRQ(ierr);
      ierr = PetscStrcat(keyfile,"sslclient.pem");CHKERRQ(ierr);
      ierr = PetscTestFile(keyfile,'r',&exists);CHKERRQ(ierr);
      if (!exists) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_FILE_OPEN,"Unable to locate sslclient.pem file in current directory or home directory");
    }

    /* Load our keys and certificates*/
    if (!(SSL_CTX_use_certificate_chain_file(ctx,keyfile))) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_FILE_OPEN,"Cannot read certificate file");

    SSL_CTX_set_default_passwd_cb(ctx,password_cb);
    if (!(SSL_CTX_use_PrivateKey_file(ctx,keyfile,SSL_FILETYPE_PEM))) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_FILE_OPEN,"Cannot read key file");
#endif

    *octx = ctx;
    PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscSSLDestroyContext"
PetscErrorCode PetscSSLDestroyContext(SSL_CTX *ctx)
{
  PetscFunctionBegin;
  SSL_CTX_free(ctx);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscHTTPBuildRequest"
PetscErrorCode PetscHTTPBuildRequest(const char type[],const char url[],const char header[],const char ctype[],const char body[],char **outrequest)
{
  char           *request=0;
  char           contentlength[40],contenttype[80],*path,*host;
  size_t         request_len,headlen,bodylen,contentlen,pathlen,hostlen,typelen,contenttypelen = 0;
  PetscErrorCode ierr;
  PetscBool      flg;

  PetscFunctionBegin;
  ierr = PetscStrallocpy(url,&host);CHKERRQ(ierr);
  ierr = PetscStrchr(host,'/',&path);CHKERRQ(ierr);
  if (!path) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONGSTATE,"url must contain / it is %s",url);
  *path = 0;
  ierr  = PetscStrlen(host,&hostlen);CHKERRQ(ierr);

  ierr = PetscStrchr(url,'/',&path);CHKERRQ(ierr);
  ierr = PetscStrlen(path,&pathlen);CHKERRQ(ierr);

  if (header) {
    ierr = PetscStrendswith(header,"\r\n",&flg);CHKERRQ(ierr);
    if (!flg) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONG,"header must end with \\r\\n");
  }

  ierr = PetscStrlen(type,&typelen);CHKERRQ(ierr);
  if (ctype) {
    ierr = PetscSNPrintf(contenttype,80,"Content-Type: %s\r\n",ctype);CHKERRQ(ierr);
    ierr = PetscStrlen(contenttype,&contenttypelen);CHKERRQ(ierr);
  }
  ierr = PetscStrlen(header,&headlen);CHKERRQ(ierr);
  ierr = PetscStrlen(body,&bodylen);CHKERRQ(ierr);
  ierr = PetscSNPrintf(contentlength,40,"Content-Length: %d\r\n\r\n",(int)bodylen);CHKERRQ(ierr);
  ierr = PetscStrlen(contentlength,&contentlen);CHKERRQ(ierr);

  /* Now construct our HTTP request */
  request_len = typelen + 1 + pathlen + hostlen + 100 + headlen + contenttypelen + contentlen + bodylen + 1;
  ierr = PetscMalloc1(request_len,&request);CHKERRQ(ierr);
  ierr = PetscStrcpy(request,type);CHKERRQ(ierr);
  ierr = PetscStrcat(request," ");CHKERRQ(ierr);
  ierr = PetscStrcat(request,path);CHKERRQ(ierr);
  ierr = PetscStrcat(request," HTTP/1.1\r\nHost: ");CHKERRQ(ierr);
  ierr = PetscStrcat(request,host);CHKERRQ(ierr);
  ierr = PetscFree(host);CHKERRQ(ierr);
  ierr = PetscStrcat(request,"\r\nUser-Agent:PETScClient\r\n");CHKERRQ(ierr);
  ierr = PetscStrcat(request,header);CHKERRQ(ierr);
  if (ctype) {
    ierr = PetscStrcat(request,contenttype);CHKERRQ(ierr);
  }
  ierr = PetscStrcat(request,contentlength);CHKERRQ(ierr);
  ierr = PetscStrcat(request,body);CHKERRQ(ierr);
  ierr = PetscStrlen(request,&request_len);CHKERRQ(ierr);
  ierr = PetscInfo1(NULL,"HTTPS request follows: \n%s\n",request);CHKERRQ(ierr);

  *outrequest = request;
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "PetscHTTPSRequest"
/*
     PetscHTTPSRequest - Send a request to an HTTPS server

   Input Parameters:
+   type - either "POST" or "GET"
.   url -  URL of request host/path
.   header - additional header information, may be NULL
.   ctype - data type of body, for example application/json
.   body - data to send to server
.   ssl - obtained with PetscHTTPSConnect()
-   buffsize - size of buffer

   Output Parameter:
.   buff - everything returned from server
 */
PetscErrorCode PetscHTTPSRequest(const char type[],const char url[],const char header[],const char ctype[],const char body[],SSL *ssl,char buff[],size_t buffsize)
{
  char           *request;
  int            r;
  size_t         request_len,len;
  PetscErrorCode ierr;
  PetscBool      foundbody = PETSC_FALSE;

  PetscFunctionBegin;
  ierr = PetscHTTPBuildRequest(type,url,header,ctype,body,&request);CHKERRQ(ierr);
  ierr = PetscStrlen(request,&request_len);CHKERRQ(ierr);

  r = SSL_write(ssl,request,(int)request_len);
  switch (SSL_get_error(ssl,r)){
    case SSL_ERROR_NONE:
      if (request_len != (size_t)r) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_LIB,"Incomplete write to SSL socket");
      break;
    default:
      SETERRQ(PETSC_COMM_SELF,PETSC_ERR_LIB,"SSL socket write problem");
  }

  /* Now read the server's response, globus sends it in two chunks hence must read a second time if needed */
  ierr      = PetscMemzero(buff,buffsize);CHKERRQ(ierr);
  len       = 0;
  foundbody = PETSC_FALSE;
  do {
    char   *clen;
    int    cl;
    size_t nlen;

    r = SSL_read(ssl,buff+len,(int)buffsize);
    len += r;
    switch (SSL_get_error(ssl,r)){
    case SSL_ERROR_NONE:
      break;
    case SSL_ERROR_ZERO_RETURN:
      foundbody = PETSC_TRUE;
      SSL_shutdown(ssl);
      break;
    case SSL_ERROR_SYSCALL:
      foundbody = PETSC_TRUE;
      break;
    default:
      SETERRQ(PETSC_COMM_SELF,PETSC_ERR_LIB,"SSL read problem");
    }

    ierr = PetscStrstr(buff,"Content-Length: ",&clen);CHKERRQ(ierr);
    if (clen) {
      clen += 15;
      sscanf(clen,"%d",&cl);
      if (!cl) foundbody = PETSC_TRUE;
      else {
        ierr = PetscStrstr(buff,"\r\n\r\n",&clen);CHKERRQ(ierr);
        if (clen) {
          ierr = PetscStrlen(clen,&nlen);CHKERRQ(ierr);
          if (nlen-4 == (size_t) cl) foundbody = PETSC_TRUE;
        }
      }
    } else {
      /* if no content length than must leave because you don't know if you can read again */
      foundbody = PETSC_TRUE;
    }
  } while (!foundbody);
  ierr = PetscInfo1(NULL,"HTTPS result follows: \n%s\n",buff);CHKERRQ(ierr);

  SSL_free(ssl);
  ierr = PetscFree(request);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscHTTPRequest"
/*
     PetscHTTPRequest - Send a request to an HTTP server

   Input Parameters:
+   type - either "POST" or "GET"
.   url -  URL of request host/path
.   header - additional header information, may be NULL
.   ctype - data type of body, for example application/json
.   body - data to send to server
.   sock - obtained with PetscOpenSocket()
-   buffsize - size of buffer

   Output Parameter:
.   buff - everything returned from server
 */
PetscErrorCode PetscHTTPRequest(const char type[],const char url[],const char header[],const char ctype[],const char body[],int sock,char buff[],size_t buffsize)
{
  char           *request;
  size_t         request_len;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscHTTPBuildRequest(type,url,header,ctype,body,&request);CHKERRQ(ierr);
  ierr = PetscStrlen(request,&request_len);CHKERRQ(ierr);

  ierr = PetscBinaryWrite(sock,request,request_len,PETSC_CHAR,PETSC_FALSE);CHKERRQ(ierr);
  ierr = PetscFree(request);CHKERRQ(ierr);
  PetscBinaryRead(sock,buff,buffsize,PETSC_CHAR);
  buff[buffsize-1] = 0;
  ierr = PetscInfo1(NULL,"HTTP result follows: \n%s\n",buff);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscHTTPSConnect"
PetscErrorCode PetscHTTPSConnect(const char host[],int port,SSL_CTX *ctx,int *sock,SSL **ssl)
{
  BIO            *sbio;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  /* Connect the TCP socket*/
  ierr = PetscOpenSocket(host,port,sock);CHKERRQ(ierr);

  /* Connect the SSL socket */
  *ssl = SSL_new(ctx);
  sbio = BIO_new_socket(*sock,BIO_NOCLOSE);
  SSL_set_bio(*ssl,sbio,sbio);
  if (SSL_connect(*ssl) <= 0) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_LIB,"SSL connect error");
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscPullJSONValue"
/*
     Given a JSON response containing the substring with "key" : "value"  where there may or not be spaces around the : returns the value.
*/
PetscErrorCode PetscPullJSONValue(const char buff[],const char key[],char value[],size_t valuelen,PetscBool *found)
{
  PetscErrorCode ierr;
  char           *v,*w;
  char           work[256];
  size_t         len;

  PetscFunctionBegin;
  ierr = PetscStrcpy(work,"\"");CHKERRQ(ierr);
  ierr = PetscStrncat(work,key,250);CHKERRQ(ierr);
  ierr = PetscStrcat(work,"\":");CHKERRQ(ierr);
  ierr = PetscStrstr(buff,work,&v);CHKERRQ(ierr);
  ierr = PetscStrlen(work,&len);CHKERRQ(ierr);
  if (v) {
    v += len;
  } else {
    work[len++-1] = 0;
    ierr = PetscStrcat(work," :");CHKERRQ(ierr);
    ierr = PetscStrstr(buff,work,&v);CHKERRQ(ierr);
    if (!v) {
      *found = PETSC_FALSE;
      PetscFunctionReturn(0);
    }
    v += len;
  }
  ierr = PetscStrchr(v,'\"',&v);CHKERRQ(ierr);
  if (!v) {
    *found = PETSC_FALSE;
    PetscFunctionReturn(0);
  }
  ierr = PetscStrchr(v+1,'\"',&w);CHKERRQ(ierr);
  if (!w) {
    *found = PETSC_FALSE;
    PetscFunctionReturn(0);
  }
  *found = PETSC_TRUE;
  ierr = PetscStrncpy(value,v+1,PetscMin((size_t)(w-v),valuelen));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#include <ctype.h>

#undef __FUNCT__
#define __FUNCT__ "PetscPushJSONValue"
/*
     Pushs a "key" : "value" pair onto a string

     Ignores lengths so can cause buffer overflow
*/
PetscErrorCode PetscPushJSONValue(char buff[],const char key[],const char value[],size_t bufflen)
{
  PetscErrorCode ierr;
  size_t         len;
  PetscBool      special;

  PetscFunctionBegin;
  ierr = PetscStrcmp(value,"null",&special);CHKERRQ(ierr);
  if (!special) {
    ierr = PetscStrcmp(value,"true",&special);CHKERRQ(ierr);
  }
  if (!special) {
    ierr = PetscStrcmp(value,"false",&special);CHKERRQ(ierr);
  }
  if (!special) {
    PetscInt i;

    ierr    = PetscStrlen(value,&len);CHKERRQ(ierr);
    special = PETSC_TRUE;
    for (i=0; i<(int)len; i++) {
      if (!isdigit(value[i])) {
        special = PETSC_FALSE;
        break;
      }
    }
  }

  ierr = PetscStrcat(buff,"\"");CHKERRQ(ierr);
  ierr = PetscStrcat(buff,key);CHKERRQ(ierr);
  ierr = PetscStrcat(buff,"\":");CHKERRQ(ierr);
  if (!special) {
    ierr = PetscStrcat(buff,"\"");CHKERRQ(ierr);
  }
  ierr = PetscStrcat(buff,value);CHKERRQ(ierr);
  if (!special) {
    ierr = PetscStrcat(buff,"\"");CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}
