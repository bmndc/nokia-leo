""" GDB Python customization auto-loader for js shell """

import os.path
sys.path[0:0] = [os.path.join('/work/fp_daily/workspace/LEO_Official_Build/gecko/js/src', 'gdb')]

import mozilla.autoload
mozilla.autoload.register(gdb.current_objfile())

import mozilla.asmjs
mozilla.asmjs.install()
