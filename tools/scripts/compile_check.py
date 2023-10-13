#-------------------------------------------------------------------------------
# Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

#------------------------------------------------------------------------------


import sys
import os
from os import walk
import warnings

def compile_check(COMPILED_FILE, ROOT_DIR):

	VAL_PATH = os.path.join(ROOT_DIR,'val')
	PLAT_PATH = os.path.join(ROOT_DIR,'plat')
	TEST_PATH = os.path.join(ROOT_DIR,'test')

	global_filelist = []

	for val, val_dir, val_files in os.walk(VAL_PATH):
		for file in val_files:
			if (file.endswith(".cmake")) or (file.endswith(".h")):
				continue
			else:
				global_filelist.append(os.path.join(val,file))

	for plat, plat_dir, plat_files in os.walk(PLAT_PATH):
        	for file in plat_files:
                	if (file.endswith(".cmake")) or (file.endswith(".json")) or (file.endswith(".h")) or (file.endswith(".dts")):
                        	continue
	                else:
        	                global_filelist.append(os.path.join(plat,file))

	for test, test_dir, test_files in os.walk(TEST_PATH):
        	for file in test_files:
                	if (file.endswith(".cmake")) or (file.endswith(".h")):
                        	continue
	                else:
        	                global_filelist.append(os.path.join(test,file))

	not_compiled_files = [not_compiled for not_compiled in global_filelist if not_compiled not in COMPILED_FILE]
	for i in not_compiled_files:
		print(i)


if __name__ == "__main__":
	COMPILED_FILE = sys.argv[1]
	ROOT_DIR = sys.argv[2]

	compile_check(COMPILED_FILE, ROOT_DIR)
