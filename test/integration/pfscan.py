import os
import subprocess
import shutil
import sys
import test
import test.util as util



class pfscan(test.TarballTest):
  def __init__(self, arches, verbose):
    self.tar_url = 'ftp://ftp.lysator.liu.se/pub/unix/pfscan/pfscan-1.0.tar.gz'
    self.local_tar = 'pfscan-1.0.tar.gz'
    self.make_dir = 'pfscan-1.0'
    self.bin_name = 'pfscan'
    self.tar_md5='e6ad91e18d4ec168fef01f8eb4f063c2'
    self.make_args=['lnx']
    super(pfscan, self).__init__(arches, verbose)

  def patch(self, arch, opt, debug, proto):
    bits = util.bits_for_arch(arch)
    diff = """diff -ruN pfscan_ref/pfscan-1.0/Makefile pfscan_proto/pfscan-1.0/Makefile
              --- pfscan_ref/pfscan-1.0/Makefile  2012-10-25 22:41:35.113016000 -0400
              +++ pfscan_proto/pfscan-1.0/Makefile  2012-10-09 22:56:21.039637483 -0400
              @@ -23,9 +23,9 @@
               SOL_LIBS= -lpthread -lnsl -lsocket
                
               ## Linux 2.4 with Gcc 2.96
              -LNX_CC=gcc -Wall -g -O
              -LNX_LDOPTS=-Wl,-s 
              -LNX_LIBS=-lpthread -lnsl
              +LNX_CC=gcc -Wall -O%d -m%d%s%s
              +LNX_LDOPTS=-m%d%s
              +LNX_LIBS=%s
              
               OBJS = pfscan.o bm.o version.o pqueue.o
              """ % (opt,
                     bits,
                     ' -g' if debug else '',
                     ' -lpthread' if not proto else '',
                     bits,
                     ' -Wl,-s' if not proto else '',
                     '-rdynamic ${PROTO_LIB_DIR}/libproto.so -dl' if proto else '-lpthread -lnsl')
    return diff.encode('ascii')

  def name(self):
    return 'pfscan'

  def local_tar(self):
    return self.cwd(local_tar)

  def clean(self, level):
    super(pfscan, self).clean(level)

  # control indicates that control programs should be tested against themselves
  def run(self, control=False):
     for arch in self.arches:
       for opt,debug in self.opts:
         a_bin = self.ref_dir(arch, opt, self.make_dir, self.bin_name)
         b_bin = a_bin if control else self.proto_dir(arch, opt, self.make_dir, self.bin_name)
         pfscan_args = ['-i', '-n4', self.cwd()]
         util.hash_cmp_run([a_bin] + pfscan_args,
                           [b_bin] + pfscan_args,
                           verbose=self.verbose) 
 
