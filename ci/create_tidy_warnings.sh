#!/bin/sh

#################################################################
#								#
# Copyright (c) 2020-2022 YottaDB LLC and/or its subsidiaries.	#
# All rights reserved.						#
#								#
#	This source code contains the intellectual property	#
#	of its copyright holder(s), and is made available	#
#	under a license.  If you do not know the terms of	#
#	the license, please stop and do not read further.	#
#								#
#################################################################

# Runs clang-tidy on all C files and store the warnings in `$output_dir/sorted_warnings.ref`.
# This also stores various intermediate artifacts in `$output_dir`.

set -euv

# Make sure that `sort` behaves consistently
export LC_ALL=C

# This script stores files in `$output_dir` so they can be downloaded from the pipeline without downloading the build artifacts.
# ${a+x} avoids an error from `set -u`: https://stackoverflow.com/a/13864829/7669110
if [ -z "${1+x}" ]; then
	output_dir=.
else
	output_dir="$1"
fi
output_dir="$(realpath $output_dir)"

# clang-tidy is sensitive to the directory `compile-commands.json` is stored in.
if [ -z "${2+x}" ]; then
	build_dir=.
else
	build_dir="$2"
fi
build_dir="$(realpath $build_dir)"

if ! [ -e "$build_dir"/compile_commands.json ]; then
	echo 'error: cannot run "clang-tidy" without a "compile_commands.json" file'
	echo 'help: try running "cmake -D CMAKE_EXPORT_COMPILE_COMMANDS=ON .."'
	exit 1
fi

root="$(git rev-parse --show-toplevel)"

# NOTE: this does *not* run on header files, because YDB style is not to
# include headers in other headers when they were included in the original .c
# file. So trying to run `clang-tidy` on them gives lots of errors about
# unknown typedefs.

# NOTE: doesn't run `insecureAPI` because it has many thousands of false positives
# In particular, it warns any time you use strcpy.

# NOTE: discards 'x warnings generated' output since it clutters up the logs.
cd "$root"
mkdir -p "$output_dir"
find . -name '*.c' \
	| grep -E 'sr_(linux|unix|port|x86_64)/.*\.c$' \
	| xargs -n 1 -P $(getconf _NPROCESSORS_ONLN) clang-tidy-14 --quiet -p="$build_dir" '--checks=-clang-analyzer-security.insecureAPI.*' \
	>"$output_dir/tidy_warnings.txt" 2>/dev/null

cd "$output_dir"
"$root"/ci/sort_warnings.sh tidy_warnings.txt sorted_warnings.txt
