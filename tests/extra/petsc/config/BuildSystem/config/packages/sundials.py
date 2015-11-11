import config.package

class Configure(config.package.GNUPackage):
  def __init__(self, framework):
    config.package.GNUPackage.__init__(self, framework)
    self.download     = ['http://ftp.mcs.anl.gov/pub/petsc/externalpackages/sundials-2.5.0p1.tar.gz']
    self.functions    = ['CVSpgmr']
    self.includes     = ['sundials/sundials_nvector.h']
    self.liblist      = [['libsundials_cvode.a','libsundials_nvecserial.a','libsundials_nvecparallel.a']] #currently only support CVODE
    self.license      = 'http://www.llnl.gov/CASC/sundials/download/download.html'
    self.needsMath    = 1
    self.parallelMake = 0  # uses recursive make so better be safe and not use make -j np
    self.hastests     = 1

  def setupDependencies(self, framework):
    config.package.GNUPackage.setupDependencies(self, framework)
    self.blasLapack = framework.require('config.packages.BlasLapack',self)
    self.mpi        = framework.require('config.packages.MPI',self)
    self.deps       = [self.mpi,self.blasLapack]

  def formGNUConfigureArgs(self):
    import os
    args = config.package.GNUPackage.formGNUConfigureArgs(self)

    self.framework.pushLanguage('C')
    # use --with-mpi-root if we know it works
    if self.mpi.directory and (os.path.realpath(self.framework.getCompiler())).find(os.path.realpath(self.mpi.directory)) >=0:
      self.log.write('Sundials configure: using --with-mpi-root='+self.mpi.directory+'\n')
      args.append('--with-mpi-root="'+self.mpi.directory+'"')
    # else provide everything!
    else:
      #print a message if the previous check failed
      if self.mpi.directory:
        self.log.write('Sundials configure: --with-mpi-dir specified - but could not use it\n')
        self.log.write(str(os.path.realpath(self.framework.getCompiler()))+' '+str(os.path.realpath(self.mpi.directory))+'\n')

      if self.mpi.include:
        args.append('--with-mpi-incdir="'+self.mpi.include[0]+'"')
      else:
        args.append('--with-mpi-incdir="/usr/include"')  # dummy case

      if self.mpi.lib:
        args.append('--with-mpi-libdir="'+os.path.dirname(self.mpi.lib[0])+'"')
        libs = []
        for l in self.mpi.lib:
          ll = os.path.basename(l)
          if ll.endswith('.a'): libs.append(ll[3:-2])
          elif ll.endswith('.so'): libs.append(ll[3:-3])
          elif ll.endswith('.dylib'): libs.append(ll[3:-6])
          libs.append(ll[3:-2])
        libs = '-l' + ' -l'.join(libs)
        args.append('--with-mpi-libs="'+libs+'"')
      else:
        args.append('--with-mpi-libdir="/usr/lib"')  # dummy case
        args.append('--with-mpi-libs="-lc"')

    self.framework.popLanguage()
    args.append('--without-mpif77')
    args.append('--disable-examples')
    args.append('--disable-cvodes')
    args.append('--disable-ida')
    args.append('--disable-idas')
    args.append('--disable-cpodes')
    args.append('--disable-fcmix')
    args.append('--disable-kinsol')

    args.append('--disable-f77') #does not work? Use 'F77=no' instead
    args.append('F77=no')
    args.append('--disable-libtool-lock')
    return [arg for arg in args if not arg in ['--enable-shared']]
