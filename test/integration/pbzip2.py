import os
import subprocess
import shutil
import sys

import test
import test.util as util



class pbzip2(test.TarballTest):
  def __init__(self, arches, verbose):
    self.tar_url = 'http://compression.ca/pbzip2/pbzip2-1.1.6.tar.gz'
    self.local_tar = 'pbzip2-1.1.6.tar.gz'
    self.make_dir = 'pbzip2-1.1.6'
    self.bin_name = 'pbzip2'
    self.tar_md5 = '26cc5a0d882198f106e75101ff0544a3'
    self.make_args = []
    super(pbzip2, self).__init__(arches, verbose)

  def patch(self, arch, opt, debug, proto):
    bits = util.bits_for_arch(arch)
    diff = """diff -ruN pbzip2_x86_ref/pbzip2-1.1.6/Makefile pbzip2_x86_proto/pbzip2-1.1.6/Makefile
              --- pbzip2_x86_ref/pbzip2-1.1.6/Makefile  2011-10-30 15:22:47.000000000 -0400
              +++ pbzip2_x86_proto/pbzip2-1.1.6/Makefile  2012-10-29 22:49:36.881509837 -0400
              @@ -3,7 +3,7 @@
               
               # Compiler to use
               CC = g++
              -CFLAGS = -O2
              +CFLAGS = -O%d%s
               #CFLAGS += -g -Wall
               #CFLAGS += -ansi
               #CFLAGS += -pedantic
              @@ -34,11 +34,11 @@
               #CFLAGS += -DIGNORE_TRAILING_GARBAGE=1
               
               # On some compilers -pthreads
              -CFLAGS += -pthread
              +CFLAGS += -m%d%s
              
               # External libraries
               LDFLAGS = -lbz2
              -LDFLAGS += -lpthread
              +LDFLAGS += -m%d%s
              
               # Where you want pbzip2 installed when you do 'make install'
               PREFIX = /usr
              """ % (opt,
                     ' -g' if debug else '',
                     bits,
                     ' -pthread' if not proto else '',
                     bits,
                     ' -pthread' if not proto else ' -rdynamic ${PROTO_LIB_DIR}/libproto.so -dl')
    return diff.encode('ascii')

  def name(self):
    return 'pbzip2'

  def run(self, control=False):
    for arch in self.arches:
      for opt,debug in self.opts:
        a_bin = self.ref_dir(arch, opt, self.make_dir, self.bin_name)
        b_bin = a_bin if control else self.proto_dir(arch, opt, self.make_dir, self.bin_name)
        args = ['-zcqk', self.cwd('local_files.tar')]
        util.hash_cmp_run([a_bin] + args,
                          [b_bin] + args,
                          verbose=self.verbose)
