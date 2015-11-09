import logger

import os
import urllib
import urlparse
import config.base
import socket

# Fix parsing for nonstandard schemes
urlparse.uses_netloc.extend(['bk', 'ssh', 'svn'])

class Retriever(logger.Logger):
  def __init__(self, sourceControl, clArgs = None, argDB = None):
    logger.Logger.__init__(self, clArgs, argDB)
    self.sourceControl = sourceControl
    self.stamp = None
    return

  def getAuthorizedUrl(self, url):
    '''This returns a tuple of the unauthorized and authorized URLs for the given URL, and a flag indicating which was input'''
    (scheme, location, path, parameters, query, fragment) = urlparse.urlparse(url)
    if not location:
      url     = urlparse.urlunparse(('', '', path, parameters, query, fragment))
      authUrl = None
      wasAuth = 0
    else:
      index = location.find('@')
      if index >= 0:
        login   = location[0:index]
        authUrl = url
        url     = urlparse.urlunparse((scheme, location[index+1:], path, parameters, query, fragment))
        wasAuth = 1
      else:
        login   = location.split('.')[0]
        authUrl = urlparse.urlunparse((scheme, login+'@'+location, path, parameters, query, fragment))
        wasAuth = 0
    return (url, authUrl, wasAuth)

  def testAuthorizedUrl(self, authUrl):
    '''Raise an exception if the URL cannot receive an SSH login without a password'''
    if not authUrl:
      raise RuntimeError('Url is empty')
    (scheme, location, path, parameters, query, fragment) = urlparse.urlparse(authUrl)
    return self.executeShellCommand('echo "quit" | ssh -oBatchMode=yes '+location)

  def genericRetrieve(self, url, root, package):
    '''Fetch the gzipped tarfile indicated by url and expand it into root
       - All the logic for removing old versions, updating etc. must move'''

    # copy a directory
    if url.startswith('dir://'):
      import shutil
      dir = url[6:]
      if not os.path.isdir(dir): raise RuntimeError('Url begins with dir:// but is not a directory')

      if os.path.isdir(os.path.join(root,os.path.basename(dir))): shutil.rmtree(os.path.join(root,os.path.basename(dir)))
      if os.path.isfile(os.path.join(root,os.path.basename(dir))): os.unlink(os.path.join(root,os.path.basename(dir)))

      shutil.copytree(dir,os.path.join(root,os.path.basename(dir)))
      return

    if url.startswith('git://'):
      if not hasattr(self.sourceControl, 'git'): return
      import shutil
      dir = url[6:]
      if os.path.isdir(dir):
        if not os.path.isdir(os.path.join(dir,'.git')): raise RuntimeError('Url begins with git:// and is a directory but but does not have a .git subdirectory')

      newgitrepo = os.path.join(root,'git.'+package)
      if os.path.isdir(newgitrepo): shutil.rmtree(newgitrepo)
      if os.path.isfile(newgitrepo): os.unlink(newgitrepo)

      config.base.Configure.executeShellCommand(self.sourceControl.git+' clone '+dir+' '+newgitrepo)
      return
    # get the tarball file name from the URL
    filename = os.path.basename(urlparse.urlparse(url)[2])
    localFile = os.path.join(root,'_d_'+filename)
    ext =  os.path.splitext(localFile)[1]
    if ext not in ['.bz2','.tbz','.gz','.tgz','.zip','.ZIP']:
      raise RuntimeError('Unknown compression type in URL: '+ url)
    self.logPrint('Downloading '+url+' to '+localFile)
    if os.path.exists(localFile):
      os.unlink(localFile)

    try:
      sav_timeout = socket.getdefaulttimeout()
      socket.setdefaulttimeout(30)
      urllib.urlretrieve(url, localFile)
      socket.setdefaulttimeout(sav_timeout)
    except Exception, e:
      socket.setdefaulttimeout(sav_timeout)
      failureMessage = '''\
Unable to download package %s from: %s
* If URL specified manually - perhaps there is a typo?
* If your network is disconnected - please reconnect and rerun ./configure
* Or perhaps you have a firewall blocking the download
* Alternatively, you can download the above URL manually, to /yourselectedlocation/%s
  and use the configure option:
  --download-%s=/yourselectedlocation/%s
''' % (package.upper(), url, filename, package, filename)
      raise RuntimeError(failureMessage)

    self.logPrint('Extracting '+localFile)
    if ext in ['.zip','.ZIP']:
      config.base.Configure.executeShellCommand('cd '+root+'; unzip '+localFile, log = self.log)
      output = config.base.Configure.executeShellCommand('cd '+root+'; zipinfo -1 '+localFile+' | head -n 1', log = self.log)
      dirname = os.path.normpath(output[0].strip())
    else:
      failureMessage = '''\
Downloaded package %s from: %s is not a tarball.
[or installed python cannot process compressed files]
* If you are behind a firewall - please fix your proxy and rerun ./configure
  For example at LANL you may need to set the environmental variable http_proxy (or HTTP_PROXY?) to  http://proxyout.lanl.gov
* Alternatively, you can download the above URL manually, to /yourselectedlocation/%s
  and use the configure option:
  --download-%s=/yourselectedlocation/%s
''' % (package.upper(), url, filename, package, filename)
      import tarfile
      try:
        tf  = tarfile.open(os.path.join(root, localFile))
      except tarfile.ReadError, e:
        raise RuntimeError(str(e)+'\n'+failureMessage)
      if not tf: raise RuntimeError(failureMessage)
      #git puts 'pax_global_header' as the first entry and some tar utils process this as a file
      firstname = tf.getnames()[0]
      if firstname == 'pax_global_header':
        firstmember = tf.getmembers()[1]
      else:
        firstmember = tf.getmembers()[0]
      # some tarfiles list packagename/ but some list packagename/filename in the first entry
      if firstmember.isdir():
        dirname = firstmember.name
      else:
        dirname = os.path.dirname(firstmember.name)
      if hasattr(tf,'extractall'): #python 2.5+
        tf.extractall(root)
      else:
        for tfile in tf.getmembers():
          tf.extract(tfile,root)
      tf.close()

    # fix file permissions for the untared tarballs.
    try:
      # check if 'dirname' is set'
      if dirname:
        config.base.Configure.executeShellCommand('cd '+root+'; chmod -R a+r '+dirname+';find  '+dirname + ' -type d -name "*" -exec chmod a+rx {} \;', log = self.log)
      else:
        self.logPrintBox('WARNING: Could not determine dirname extracted by '+localFile+' to fix file permissions')
    except RuntimeError, e:
      raise RuntimeError('Error changing permissions for '+dirname+' obtained from '+localFile+ ' : '+str(e))
    os.unlink(localFile)
    return

  def ftpRetrieve(self, url, root, name,force):
    self.logPrint('Retrieving '+url+' --> '+os.path.join(root, name)+' via ftp', 3, 'install')
    return self.genericRetrieve(url, root, name)

  def httpRetrieve(self, url, root, name,force):
    self.logPrint('Retrieving '+url+' --> '+os.path.join(root, name)+' via http', 3, 'install')
    return self.genericRetrieve(url, root, name)

  def fileRetrieve(self, url, root, name,force):
    self.logPrint('Retrieving '+url+' --> '+os.path.join(root, name)+' via cp', 3, 'install')
    return self.genericRetrieve(url, root, name)

  def svnRetrieve(self, url, root, name,force):
    if not hasattr(self.sourceControl, 'svn'):
      raise RuntimeError('Cannot retrieve a SVN repository since svn was not found')
    self.logPrint('Retrieving '+url+' --> '+os.path.join(root, name)+' via svn', 3, 'install')
    try:
      config.base.Configure.executeShellCommand(self.sourceControl.svn+' checkout http'+url[3:]+' '+os.path.join(root, name))
    except RuntimeError:
      pass


