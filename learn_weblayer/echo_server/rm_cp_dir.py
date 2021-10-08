#!/usr/bin/env python

import sys
import shutil
import os

def Main(src, dst):
    print('remove and copy %s %s' %(src, dst))
    if os.path.exists(dst):
        shutil.rmtree(dst)
    shutil.copytree(src, dst)


if __name__ == '__main__':
  sys.exit(Main(sys.argv[1], sys.argv[2]))
