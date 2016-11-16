#!/bin/bash

#
# Test (and push) Trilinos on any machine that has the SEMS Dev Env available
#
# A) Setup to CHECKIN directory and symlink to checkin-test-sems.sh script:
#
# To use on Linux, first one must create a directory to run from and then
# symlink this script into it.
#
# On Linux, one can create any arbitrary directory and then symlink this
# script into it:
#
#   $ cd <some-build-dir>/
#   $ ln -s <trilinos-src-dir>/cmake/std/sems/checkin-test-sems.sh .
#
# On Linux, the directory path to Trilinos is picked up automatically from any
# arbitrary location.
#
# However, on OSX, it can't pick up the path to Trilinos from the symlink.  In
# this case, one can use one of the following standard directory structures
# and it will just work:
#
#   Trilinos/
#     CHECKIN/
#       ./checkin-test-sems.sh   # symlink
#
# or
#
#   <some-base-dir>/
#     Trilinos/
#     BUILDS/
#       CHECKIN/
#         ./checkin-test-sems.sh  # symlink
#
# Otherwise, if one does not want to use one of the standard directory
# structures and is not on Linux, then one will have to manually set the
# variable TRILINOS_DIR in the env before calling this script like:
#
#   $ env TRILINOS_DIR=<trilinos-src-dir> ./checkin-test-sems.sh [other options]
#
# B) Local testing and pushing:
#
# For local builds, run with:
#
#   $ ./checkin-test-sems.sh [other options] --local-do-all
#
# For testing and pushing, run with:
#
#   $ ./checkin-test-sems.sh [other options] --do-all --push
#
# NOTE: After the first time running this script (e.g. ./checkin-test-sems.sh
# --help), please modify the created file:
#
#    ./local-checkin-test-defaults.py
#
# to adjust the defaults for usage on your machine.  For example, you will
# want to adjust the parallel level -j<N> option to take better advantage of
# the cores on your local machine.  Also, youe will want to make
# --ctest-timeout larger if the machine you are on is particularly slow.
#

#
# A) Get the path to the Trilinos source directory
#
# This first tries the standard configurations (above).
#
# If that does not check out, then it tries to grab the directory path from
# the symlink.  On Linux that will work.  On OSX, it will not.  In that case,
# it just bombs out with an error message.
#

if [ "$TRILINOS_DIR" == "" ] ; then
  # First try standard directory configurations
  if [ -e ../../Trilinos ] ; then
    TRILINOS_DIR=../../Trilinos
  else
    # Grab from the symlink (only works on Linux)
    _ABS_FILE_PATH=`readlink -f $0` || \
     echo "Could not follow symlink to set TRILINOS_DIR!"
    if [ "$_ABS_FILE_PATH" != "" ] ; then
      _SCRIPT_DIR=`dirname $_ABS_FILE_PATH`
      TRILINOS_DIR=$_SCRIPT_DIR/../../..
    fi
  fi
fi

echo "TRILINOS_DIR = '$TRILINOS_DIR'"
if [ "$TRILINOS_DIR" == "" ] ; then
  echo "ERROR: Count not determine TRILINOS_DIR. Please use standard directory structure or set TRILINOS_DIR in env!"
  exit 1
fi

# Make sure the right env is loaded!
export TRILINOS_SEMS_DEV_ENV_VERBOSE=1
source $TRILINOS_DIR/cmake/load_ci_sems_dev_env.sh

#
# B) Set up the bulid configurations
#

# Leave the COMMON.config file for the user to fil in for any strange aspects
# to their system not coverted by SEMS (I can't imagine what that might be).

# All of the options needed for the --default-builds=MPI_RELEASE_DEBUG_SHARED
# is set in project-checkin-test-config.py.
echo "
" > MPI_RELEASE_DEBUG_SHARED.config

echo "
-DTPL_ENABLE_MPI=ON
-DCMAKE_BUILD_TYPE=RELEASE
-DTrilinos_ENABLE_DEBUG=ON
-DBUILD_SHARED_LIBS=ON
-DIfpack2_Cheby_belos_MPI_1_DISABLE=ON
-DPiro_EpetraSolver_MPI_4_DISABLE=ON
" > MPI_RELEASE_DEBUG_SHARED_ST.config

echo "
-DTPL_ENABLE_MPI=OFF
-DCMAKE_BUILD_TYPE=RELEASE
-DBUILD_SHARED_LIBS=ON
" > SERIAL_RELEASE_SHARED_ST.config

echo "
-DTPL_ENABLE_MPI=ON
-DCMAKE_BUILD_TYPE=RELEASE
-DTrilinos_ENABLE_DEBUG=ON
-DBUILD_SHARED_LIBS=OFF
-DTPL_FIND_SHARED_LIBS=OFF
-DTrilinos_LINK_SEARCH_START_STATIC=ON
-DIfpack2_Cheby_belos_MPI_1_DISABLE=ON
-DPiro_EpetraSolver_MPI_4_DISABLE=ON
" > MPI_RELEASE_DEBUG_STATIC_ST.config

echo "
-DTPL_ENABLE_MPI=OFF
-DCMAKE_BUILD_TYPE=RELEASE
-DBUILD_SHARED_LIBS=OFF
-DTPL_FIND_SHARED_LIBS=OFF
-DTrilinos_LINK_SEARCH_START_STATIC=ON
" > SERIAL_RELEASE_STATIC_ST.config

#
# C) Create a default local defaults file
#
#  After this is created, then modify the created file for your preferences!
# 

_LOCAL_CHECKIN_TEST_DEFAULTS=local-checkin-test-defaults.py
if [ -f $_LOCAL_CHECKIN_TEST_DEFAULTS ] ; then
  echo "File $_LOCAL_CHECKIN_TEST_DEFAULTS already exists, leaving it!"
else
  echo "Creating default file $_LOCAL_CHECKIN_TEST_DEFAULTS!"
  echo "
defaults = [
  \"-j4\",
  \"--ctest-timeout=300\",
  \"--disable-packages=PyTrilinos,Claps,TriKota\",
  \"--skip-case-no-email\",
  ]
  " > $_LOCAL_CHECKIN_TEST_DEFAULTS
fi
# Note the above defaults:
#
# -j4: Uses very few processes by default.  If you have more processes on your
#   machine, you may want to increase this.
#
# --ctest-timeout=300: A default 5 minute timeout for any individual test. You
#   might want to increase if you have a slower machine.
#
# --disable-packages: These are packages that most Trilinos developers will
#    never want to test by default.
#
# --skip-case-no-email: Don't send email for skipped test cases.
#
# To exclude tests, include the option:
#
#  "--ctest-options=-E '(test1|test2|...)'",
#

#
# D) Run the checkin-test.py script!
#
# NOTE: default args are read in from the local-checkin-test-defaults.py file!
#

$TRILINOS_DIR/checkin-test.py \
"$@"
