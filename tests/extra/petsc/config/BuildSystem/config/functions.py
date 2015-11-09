import config.base
import os.path

class Configure(config.base.Configure):
  def __init__(self, framework, functions = []):
    config.base.Configure.__init__(self, framework)
    self.headerPrefix = ''
    self.substPrefix  = ''
    self.functions    = functions
    return

  def getDefineName(self, funcName):
    return 'HAVE_'+funcName.upper()

  def setupHelp(self, help):
    import nargs
    help.addArgument('Functions', '-known-memcmp-ok=<bool>', nargs.ArgBool(None, None, 'Does memcmp() work correctly?'))
    return

  def setupDependencies(self, framework):
    config.base.Configure.setupDependencies(self, framework)
    self.compilers = self.framework.require('config.compilers', self)
    self.libraries = self.framework.require('config.libraries', self) # setCompilers.LIBS is setup here
    self.headers   = self.framework.require('config.headers', self)
    return

  def haveFunction(self, function):
    return self.getDefineName(function) in self.defines

  def check(self, funcs, libraries = None, examineOutput=lambda ret,out,err:None):
    '''Checks for the function "funcName", and if found defines HAVE_"funcName"'''
    if isinstance(funcs, set): funcs = list(funcs)
    if isinstance(funcs, str): funcs = [funcs]
    self.log.write('Checking for functions ['+' '.join(funcs)+']\n')
    def genIncludes(funcName):
      return 'char %s();\n' % funcName
    def genBody(funcName):
      # The GNU C library defines __stub_* for functions that it implements
      # to always fail with ENOSYS.  Some functions are actually named
      # something starting with __ and the normal name is an alias.
      return '''
#if defined (__stub_%(func)s) || defined (__stub___%(func)s)
%(func)s_will_always_fail_with_ENOSYS();
#else
%(func)s();
#endif
''' % dict(func=funcName)

    # Don't include <ctype.h> because on OSF/1 3.0 it includes <sys/types.h>
    # which includes <sys/select.h> which contains a prototype for
    # select.  Similarly for bzero.
    includes = '''
/* System header to define __stub macros and hopefully no other prototypes since they would conflict with our 'char funcname()' declaration below. */
#include <assert.h>
/* Override any gcc2 internal prototype to avoid an error. */
#ifdef __cplusplus
extern "C" {
#endif
'''
    includes += '''
/* We use char because int might match the return type of a gcc2
builtin and then its argument prototype would still apply. */
'''
    includes += ''.join(map(genIncludes,funcs))
    includes += '''
#ifdef __cplusplus
}
#endif
'''
    body = ''.join(map(genBody,funcs))
    if libraries:
      oldLibs = self.compilers.LIBS
      if not isinstance(libraries, list):
        libraries = [libraries]
      for library in libraries:
        root,ext=os.path.splitext(library)
        if library.strip()[0] == '-' or ext == '.a' or ext == '.so' :
          self.compilers.LIBS += ' '+library
        else:
          self.compilers.LIBS += ' -l'+library
    found = self.checkLink(includes, body, examineOutput=examineOutput)
    if libraries:
      self.compilers.LIBS = oldLibs
    if found:
      for funcName in funcs:
        self.addDefine(self.getDefineName(funcName), 1)
    return found

  def checkClassify(self, funcs, libraries = None):
    '''Recursive decompose to rapidly classify functions as found or missing
    To confirm that a function is missing, we require a compile/link
    failure with only that function in a compilation unit.  In contrast,
    we can confirm that many functions are present by compiling them all
    together in a large compilation unit.  We optimistically compile
    everything together, then trim all functions that were named in the
    error message and bisect the result.  The trimming is only an
    optimization to increase the likelihood of a big-batch compile
    succeeding; we do not rely on the compiler naming missing functions.
    '''
    def functional(funcs):
      named = config.NamedInStderr(funcs)
      if self.check(funcs, libraries, named.examineStderr):
        return True
      else:
        return named.named
    import config
    found, missing = config.classify(funcs, functional)
    return found, missing

  def checkMemcmp(self):
    '''Check for 8-bit clean memcmp'''
    if 'known-memcmp-ok' in self.argDB:
      if self.argDB['known-memcmp-ok'] == 0:
        raise RuntimeError('No 8-bit clean memcmp() exists. Cannot proceed')
      else:
        return
    if not self.argDB['with-batch']:
      self.logPrint('Making executable to test memcmp()')
      if not self.checkRun('#include <string.h>\nvoid exit(int);\n\n', 'char c0 = 0x40;\nchar c1 = (char) 0x80;\nchar c2 = (char) 0x81;\nexit(memcmp(&c0, &c2, 1) < 0 && memcmp(&c1, &c2, 1) < 0 ? 0 : 1);\n'):
        raise RuntimeError('Failed to find 8-bit clean memcmp(). Cannot proceed.')
    else:
      self.framework.addBatchInclude('#include <string.h>')
      self.framework.addBatchBody(['{',
                                   '  char c0 = 0x40;',
                                   '  char c1 = (char) 0x80;',
                                   '  char c2 = (char) 0x81;',
                                   '  if (memcmp(&c0, &c2, 1) < 0 && memcmp(&c1, &c2, 1) < 0 ? 0 : 1) {',
                                   '    fprintf(output, "  \'--known-memcmp-ok=0\',\\n");',
                                   '  } else {',
                                   '    fprintf(output, "  \'--known-memcmp-ok=1\',\\n");',
                                   '  }',
                                   '}'])
    return

  def checkSysinfo(self):
    '''Check whether sysinfo takes three arguments, and if it does define HAVE_SYSINFO_3ARG'''
    self.check('sysinfo')
    if self.defines.has_key(self.getDefineName('sysinfo')):
      map(self.headers.check, ['linux/kernel.h', 'sys/sysinfo.h', 'sys/systeminfo.h'])
      includes = '''
#ifdef HAVE_LINUX_KERNEL_H
#  include <linux/kernel.h>
#  include <linux/sys.h>
#  ifdef HAVE_SYS_SYSINFO_H
#    include <sys/sysinfo.h>
#  endif
#elif defined(HAVE_SYS_SYSTEMINFO_H)
#  include <sys/systeminfo.h>
#else
#  error "Cannot check sysinfo without special headers"
#endif
'''
      body = 'char buf[10]; long count=10; sysinfo(1, buf, count);\n'
      if self.checkLink(includes, body):
        self.addDefine('HAVE_SYSINFO_3ARG', 1)
    return

  def checkVPrintf(self):
    '''Checks whether vprintf requires a char * last argument, and if it does defines HAVE_VPRINTF_CHAR'''
    self.check('vprintf')
    if not self.checkLink('#include <stdio.h>\n#include <stdarg.h>\n', 'va_list Argp;\nvprintf( "%d", Argp );\n'):
      self.addDefine('HAVE_VPRINTF_CHAR', 1)
    return

  def checkVFPrintf(self):
    '''Checks whether vfprintf requires a char * last argument, and if it does defines HAVE_VFPRINTF_CHAR'''
    self.check('vfprintf')
    if not self.checkLink('#include <stdio.h>\n#include <stdarg.h>\n', 'va_list Argp;\nvfprintf(stdout, "%d", Argp );\n'):
      self.addDefine('HAVE_VFPRINTF_CHAR', 1)
    return

  def checkVSNPrintf(self):
    '''Checks whether vsnprintf requires a char * last argument, and if it does defines HAVE_VSNPRINTF_CHAR'''
    if self.checkLink('#include <stdio.h>\n', '_vsnprintf(0,0,0,0);\n'):
      self.addDefine('HAVE__VSNPRINTF', 1)
      return
    self.check('vsnprintf')
    if not self.checkLink('#include <stdio.h>\n#include <stdarg.h>\n', 'va_list Argp;char str[6];\nvsnprintf(str,5, "%d", Argp );\n'):
      self.addDefine('HAVE_VSNPRINTF_CHAR', 1)
    return

  def checkSignalHandlerType(self):
    '''Checks the type of C++ signals handlers, and defines SIGNAL_CAST to the correct value'''
    self.pushLanguage('C++')
    if not self.checkLink('#include <signal.h>\nstatic void myhandler(int sig) {}\n', 'signal(SIGFPE,myhandler);\n'):
      self.addDefine('SIGNAL_CAST', '(void (*)(int))')
    else:
      self.addDefine('SIGNAL_CAST', ' ')
    self.popLanguage()
    return

  def checkFreeReturnType(self):
    '''Checks whether free returns void or int, and defines HAVE_FREE_RETURN_INT'''
    if self.checkLink('#include <stdlib.h>\n', 'int ierr; void *p; ierr = free(p); return 0;\n'):
      self.addDefine('HAVE_FREE_RETURN_INT', 1)
    return

  def checkVariableArgumentLists(self):
    '''Checks whether the variable argument list functionality is working'''
    if self.checkLink('#include <stdarg.h>\n', '  va_list l1, l2;\n  va_copy(l1, l2);\n  return 0;\n'):
      self.addDefine('HAVE_VA_COPY', 1)
    elif self.checkLink('#include <stdarg.h>\n', '  va_list l1, l2;\n  __va_copy(l1, l2);\n  return 0;\n'):
      self.addDefine('HAVE___VA_COPY', 1)
    return

  def checkNanosleep(self):
    '''Check for functional nanosleep() - as time.h behaves differently for different compiler flags - like -std=c89'''
    if self.checkLink('#include <time.h>','struct timespec tp;\n tp.tv_sec = 0;\n tp.tv_nsec = (long)(1e9);\n nanosleep(&tp,0);\n'):
      self.addDefine('HAVE_NANOSLEEP', 1)
    return

  def configure(self):
    self.executeTest(self.checkMemcmp)
    self.executeTest(self.checkSysinfo)
    self.executeTest(self.checkVPrintf)
    self.executeTest(self.checkVFPrintf)
    self.executeTest(self.checkVSNPrintf)
    self.executeTest(self.checkNanosleep)
    if hasattr(self.compilers, 'CXX'):
      self.executeTest(self.checkSignalHandlerType)
    self.executeTest(self.checkFreeReturnType)
    self.executeTest(self.checkVariableArgumentLists)
    self.executeTest(self.checkClassify, set(self.functions))
    return
