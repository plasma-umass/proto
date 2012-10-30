from datetime import datetime
import fcntl
from hashlib import md5
import os
import platform
from select import select
import subprocess
import sys
import tarfile
import urllib.request, urllib.parse, urllib.error
import urllib.request, urllib.error, urllib.parse

class TestFailure(Exception):
  pass

class UtilFailure(TestFailure):
  pass

class IntegrationTestFailure(TestFailure):
  def __init__(self, test, args, output):
    self.test = test
    self.args = args
    self.output = output

  def __str__(self):
    return "Test %s failed (`%s`):\n%s\n" % (self.test.name(),
                                             self.args,
                                             self.output)

def lib_dir():
  return sys.environ['PROTO_LIB_DIR']

ARCH_X86_64 = 'x86-64'
ARCH_X86 = 'x86'
MD5_CHUNK_SIZE = 128

canon_arches = set((ARCH_X86_64, ARCH_X86))

# Converts all the myriad arches to the canonical one.
arch_map = {
    'x86-64' : ARCH_X86_64,
    'x86_64' : ARCH_X86_64,
    'x64'    : ARCH_X86_64,
    'amd64'  : ARCH_X86_64,

    'i386' : ARCH_X86,
    'i486' : ARCH_X86,
    'i586' : ARCH_X86,
    'i686' : ARCH_X86,
    'x86'  : ARCH_X86,
}

# maps canonical arches to bits.
arch_bits = {
    ARCH_X86_64 : 64,
    ARCH_X86    : 32,
}

def bits_for_arch(arch):
  return arch_bits[arch]

def normalize_arch(arch):
  if not arch in arch_map:
    raise Exception('Invalid or unknown architecture: ' + arch)
  norm = arch_map[arch]
  assert norm in canon_arches
  return norm

def get_arches():
  if 'PROTO_ARCH' in os.environ:
    arch_str = os.environ['PROTO_ARCH']
    if arch_str:
      return set(normalize_arch(arch) for arch in arch_str.split(':'))
  # We have to guess the host arch.
  if platform.system() == 'Windows':
    arch = platform.machine()
    if arch:
      return set([normalize_arch(arch)])
    return set([ARCH_X86])
  else: # assume uname
    machine = subprocess.check_output(['uname', '-m'])
    return set([normalize_arch(machine.decode().strip())])

def mkdir_p(directory):
  if os.path.exists(directory):
    return
  os.mkdir(directory)

def download_to(url, local, md5hash, verbose=True):
  # if it's already downloaded, don't do so again.
  if os.path.exists(local):
    if md5hash == file_hash(local):
      if verbose:
        sys.stdout.write('Already downloaded and verified %s from %s\n' % (
                         local,
                         url))
      return
    if verbose:
      sys.stdout.write('File %s appears to be invalid, re-downloading:\n' %
                       local)

  if verbose:
    sys.stdout.write('Fetching %s...' % url)
    sys.stdout.flush() # make sure buffer flushes before download blocks.
  req = urllib.request.urlopen(urllib.request.Request(url))
  with open(local, 'wb') as f:
    f.write(req.read())
  req.close()
  if verbose:
    sys.stdout.write('ok.\n')

def untar_to(tar_gz_file, dest_dir, verbose=True):
  if verbose:
    sys.stdout.write('Untarring %s to %s...' % (tar_gz_file, dest_dir))
    sys.stdout.flush()
  mkdir_p(dest_dir)
  with tarfile.open(tar_gz_file, 'r') as tar:
    tar.extractall(path=dest_dir, members=None)
  if verbose:
    sys.stdout.write('ok.\n')

# Because zip files (and python's treatment of them) is really bad,
# e.g. will happily extract '../../../bin/ls', we just use the system
# unzip command.
def unzip_to(zip_file, dest_dir, verbose=True):
  if verbose:
    sys.stdout.write('Unzipping %s to %s...' % (zip_file, dest_dir))
    sys.stdout.flush()
  zip_args = ['unzip', zip_file, '-d', dest_dir]
  prog = subprocess.Popen(zip_args,
                          stdout=subprocess.PIPE,
                          stderr=subprocess.STDOUT,
                          stdin=None)
  (stdout, stderr) = prog.communicate(input='')
  if prog.returncode != 0:
    raise UtilFailure('unzip returned code %d (%s). Output:\n%s\n' % (proc.returncode, ' '.join(zip_args), stdout))
  if verbose:
    sys.stdout.write('ok.\n')

# patch -p1 -d pfscan_test <pfscan.patch
# diff -runP dir1 dir2
def patch(patch_file, working_dir, patch_level, verbose=True):
  args = ['patch', '-p%d' % patch_level, '-d', working_dir]
  if verbose:
    sys.stdout.write('Patching %s...(%s)\n' % (working_dir,
                                                   ' '.join(args)))
  proc = subprocess.Popen(args,
                          stdout=subprocess.PIPE,
                          stderr=subprocess.STDOUT,
                          stdin=subprocess.PIPE)
  (stdout, stderr) = proc.communicate(input=patch_file)
  if proc.returncode != 0:
    raise UtilFailure('patch returned code %d (%s). Output:\n%s\n' % (proc.returncode, ' '.join(args), stdout.decode('utf-8')))
  if proc.returncode == 0 and verbose:
    sys.stdout.write('ok.\n')

def bin_links_to(bin_file, lib_path, verbose=True):
# assumes existence of ldd
  args = ['ldd', bin_file]
  proc = subprocess.Popen(args,
                          stdout=subprocess.PIPE,
                          stderr=subprocess.STDOUT,
                          stdin=None)
  (stdout, stderr) = proc.communicate()
  if proc.returncode != 0:
    raise UtilFailure('ldd returned error code %d: (%s)\n' % (proc.returncode,
                                                              ' '.join(args)))
  for line in stdout.decode('ascii').split('\n'):
    if line.strip().find(lib_path) == 0:
      return True
  return False

def cmp_run(test_args, stdin='', verbose=True):
  prog_a = control_args
  prog_b = test_args
  arg_str_a = ' '.join(prog_a)
  arg_str_b = ' '.join(prog_b)
  if verbose:
    sys.stdout.write('Comparing (%s) to (%s)...' % (arg_str_a, arg_str_b))
    sys.stdout.flush()

  proca = subprocess.Popen(prog_a,
                           stdout=subprocess.PIPE,
                           stderr=subprocess.STDOUT,
                           stdin=subprocess.PIPE)
  procb = subprocess.Popen(prog_b,
                           stdout=subprocess.PIPE,
                           stderr=subprocess.STDOUT,
                           stdin=subprocess.PIPE)
  (stdouta, stderra) = proca.communicate(input=stdin)
  (stdoutb, stderrb) = procb.communicate(input=stdin)
  if stdouta != stdoutb:
    raise UtilFailure('Differing outputs for program comparison: (%s) vs (%s)\n'
                      % (' '.join(prog_a), ' '.join(prog_b)))
  if proca.returncode != procb.returncode:
    raise UtilFailure("""Differing return outputs for program comparison:
    (%s) returning %d
    (%s) returning %d""" % (' '.join(prog_a),
                            proca.returncode,
                            ' '.join(prog_b),
                            procb.returncode))
  if verbose:
    sys.stdout.write('ok.\n')
  return True

def chunkify(file_like, chunk_size=MD5_CHUNK_SIZE):
  while True:
    chunk = file_like.read(chunk_size)
    if not chunk:
      return
    yield chunk

def gen_hash(chunk_generator):
  digest = md5()
  for chunk in chunk_generator:
    digest.update(chunk)
  return digest.hexdigest()

def file_hash(file_path):
  with open(file_path, 'rb') as file:
    return gen_hash(chunkify(file))

def make(make_dir, *extra_args):
  args = ['make', '-C', make_dir] + list(extra_args)
  proc = subprocess.Popen(args,
                          stdout=subprocess.PIPE,
                          stderr=subprocess.STDOUT,
                          stdin=None)
  (stdout, stderr) = proc.communicate()
  if proc.returncode != 0:
    raise UtilFailure('make (%s) returned %d with output:\n%s\n' % (
                      ' '.join(args),
                      proc.returncode,
                      stdout.decode()))

def make_fd_async(fd):
  fl = fcntl.fcntl(fd, fcntl.F_GETFL)
  fcntl.fcntl(fd, fcntl.F_SETFL, fl | os.O_NONBLOCK)

# a timeout of 0 indicates no timeout, i.e. potentially
# an infinite loop. Do not use with a blocking func.
# func should take zero arguments and return true if it succeeded.
# returns true if func succeeded before <timeout>.
def loop_with_timeout(func, timeout):
  time_a = datetime.now()
  elapsed = 0.0
  status = None
  while func() == None:
    elapsed = (datetime.now() - time_a).total_seconds()
    if elapsed > timeout:
      return elapsed
  return elapsed

def expect_proc_close(proc, args, timeout):
  elapsed = loop_with_timeout(lambda: proc.poll(), timeout)
  if elapsed >= timeout:
    raise UtilFailure('Child program (%s) failed to exit %.2f seconds after' \
                      ' ceasing output.' % (' '.join(args), elapsed))

def separate_hash_run(args, timeout, stdin=None, verbose=True):
# args is an array of strings
# stdin is either None for no input or a bytestring object.
#   (i.e. caller is responsible for encoding.)
# timeout is a nonnegative integer or float where timeout=0 implies
#   no timeout.
# may still block if program is not reading from stdin and <input>
#   is suitably large enough.
# returns: (returncode, stdout_md5, stderr_md5)
#
# TODO: generalize async code to automatically call coroutines on each of n inputs.
  proc = subprocess.Popen(args,
                          stdout=subprocess.PIPE,
                          stderr=subprocess.PIPE,
                          stdin=subprocess.PIPE)
  inputs = [proc.stdout, proc.stderr]
  digests = [md5() for input in inputs]
  index = {input.fileno() : idx for (idx, input) in enumerate(inputs)}
  open_inputs = list(inputs)
  for input in inputs:
    make_fd_async(input.fileno())
  if stdin:
    proc.stdin.write(stdin)
  proc.stdin.close()
  cull = set()
  while open_inputs:
    # TODO: check for xlist
    (rlist, wlist, xlist) = select(open_inputs, [], [], timeout)
    if not rlist:
      # timeout
      proc.kill()
      raise UtilFailure('Child process (%s) failed to respond after a timeout' \
                        ' of %f seconds.' % (' '.join(args), timeout))
    # if we hit an EOF, we store the fd here to be culled after this loop.
    for pipe in rlist:
      fd = pipe.fileno()
      idx = index[fd]
      while True:
# WARNING: async behavior REQUIRES python >3.0
# returns None if blocking or "" if EOF
        result = pipe.read(MD5_CHUNK_SIZE)
        if result == None:
# wait until there's either MD5_CHUNK_SIZE bytes available
# or EOF.
          break
        if result == b'':
          cull.add(fd)
          break
        # we read the chunk just fine!
        digests[idx].update(result)
# now we need to remove EOF'd inputs out of list of inputs we're watching
    if cull:
      open_inputs = [input for input in inputs if input.fileno() not in cull]
  # We've run out of inputs, so everything should be closed and dandy.
  # Program might still hang, so we poll for <timeout> seconds.
  expect_proc_close(proc, args, timeout)
  return (proc.returncode, digests[0].hexdigest(), digests[1].hexdigest())

def combined_hash_run(args, timeout, stdin=None, verbose=True):
  proc = subprocess.Popen(args,
                          stdout=subprocess.PIPE,
                          stderr=subprocess.STDIN,
                          stdin=subprocess.PIPE)
  if stdin:
    proc.stdin.write(stdin)
  proc.stdin.close()
 
  digest = md5()
  while True:
    result = proc.stdout.read(MD5_BLOCK_SIZE)
    if result == b'':
      break
      # EOF.
    digest.update(result)
  expect_proc_close(proc, args, timeout)
  return (proc.returncode, digest.hexdigest())

def hash_cmp_run(control_args,
                 test_args,
                 stdin='',
                 verbose=True,
                 timeout=10):
  a_args = control_args
  b_args = test_args
  a_args_str = ' '.join(a_args)
  b_args_str = ' '.join(b_args)
  a_return = 0
  b_return = 0
  if verbose:
    sys.stdout.write('Comparing (%s) to (%s)... ' % (a_args_str, b_args_str))
    sys.stdout.flush()
  if platform.system() == 'Windows':
    (a_return, a_md5) = combined_hash_run(a_args, timeout, stdin, verbose)
    (b_return, b_md5) = combined_hash_run(b_args, timeout, stdin, verbose)
    if a_md5 != b_md5:
      raise UtilFailure('Differing combined outputs for program comparison: (%s) vs (%s)\n'
                        % (' '.join(prog_a), ' '.join(prog_b)))
  else:
    (a_return, a_md5_out, a_md5_err) = separate_hash_run(a_args, timeout, stdin, verbose)
    (b_return, b_md5_out, b_md5_err) = separate_hash_run(b_args, timeout,  stdin, verbose)
    if a_md5_out != b_md5_out:
      raise UtilFailure('Differing stdout for program comparison: (%s) vs (%s)\n'
                        % (a_args_str, b_args_str))
    if a_md5_err != b_md5_err:
      raise UtilFailure('Differing stderr for program comparison: (%s) vs (%s)\n'
                        % (a_args_str, b_args_str))
  if a_return != b_return:
    raise UtilFailure("""Differing return outputs for program comparison:
                         \t(%s) returning %d
                         \t(%s) returning %d""" % (a_args_str, b_args_str))
  if verbose:
    sys.stdout.write('ok.\n')

