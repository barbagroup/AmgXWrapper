import config.package

class Configure(config.package.Package):
  def __init__(self, framework):
    config.package.Package.__init__(self, framework)
    self.download          = ['https://mpi4py.googlecode.com/files/mpi4py-1.3.1.tar.gz',
                              'http://ftp.mcs.anl.gov/pub/petsc/externalpackages/mpi4py-1.3.1.tar.gz']
    self.functions         = []
    self.includes          = []
    return

  def setupDependencies(self, framework):
    config.package.Package.setupDependencies(self, framework)
    self.numpy           = framework.require('config.packages.Numpy',self)
    self.setCompilers    = framework.require('config.setCompilers',self)
    self.sharedLibraries = framework.require('PETSc.options.sharedLibraries', self)
    self.installdir      = framework.require('PETSc.options.installDir',self)
    return

  def Install(self):
    import os
    pp = os.path.join(self.installDir,'lib','python*','site-packages')
    if self.setCompilers.isDarwin(self.log):
      apple = 'You may need to\n (csh/tcsh) setenv MACOSX_DEPLOYMENT_TARGET 10.X\n (sh/bash) MACOSX_DEPLOYMENT_TARGET=10.X; export MACOSX_DEPLOYMENT_TARGET\nbefore running make on PETSc'
    else:
      apple = ''
    self.logClearRemoveDirectory()
    self.logResetRemoveDirectory()
    archflags = ""
    if self.setCompilers.isDarwin(self.log):
      if self.types.sizes['known-sizeof-void-p'] == 32:
        archflags = "ARCHFLAGS=\'-arch i386\' "
      else:
        archflags = "ARCHFLAGS=\'-arch x86_64\' "

    self.addMakeRule('mpi4pybuild','', \
                       ['@echo "*** Building mpi4py ***"',\
                          '@(MPICC=${PCC} && export MPICC && cd '+self.packageDir+' && \\\n\
           python setup.py clean --all && \\\n\
           '+archflags+'python setup.py build ) > ${PETSC_ARCH}/lib/petsc/conf/mpi4py.log 2>&1 || \\\n\
             (echo "**************************ERROR*************************************" && \\\n\
             echo "Error building mpi4py. Check ${PETSC_ARCH}/lib/petsc/conf/mpi4py.log" && \\\n\
             echo "********************************************************************" && \\\n\
             exit 1)'])
    self.addMakeRule('mpi4pyinstall','', \
                       ['@echo "*** Installing mpi4py ***"',\
                          '@(MPICC=${PCC} && export MPICC && cd '+self.packageDir+' && \\\n\
           '+archflags+'python setup.py install --install-lib='+os.path.join(self.installDir,'lib')+') >> ${PETSC_ARCH}/lib/petsc/conf/mpi4py.log 2>&1 || \\\n\
             (echo "**************************ERROR*************************************" && \\\n\
             echo "Error building mpi4py. Check ${PETSC_ARCH}/lib/petsc/conf/mpi4py.log" && \\\n\
             echo "********************************************************************" && \\\n\
             exit 1)',\
                          '@echo "====================================="',\
                          '@echo "To use mpi4py, add '+os.path.join(self.installdir.dir,'lib')+' to PYTHONPATH"',\
                          '@echo "====================================="'])
    if self.framework.argDB['prefix']:
      self.addMakeRule('mpi4py-build','mpi4pybuild')
      self.addMakeRule('mpi4py-install','mpi4pyinstall')
    else:
      self.addMakeRule('mpi4py-build','mpi4pybuild mpi4pyinstall')
      self.addMakeRule('mpi4py-install','')

    return self.installDir

  def configureLibrary(self):
    self.checkDownload()
    if not self.sharedLibraries.useShared:
        raise RuntimeError('mpi4py requires PETSc be built with shared libraries; rerun with --with-shared-libraries')

  def alternateConfigureLibrary(self):
    self.addMakeRule('mpi4py-build','')
    self.addMakeRule('mpi4py-install','')
