#!/bin/bash -l

if [ "${SALLOC_CTEST_TIME_LIMIT_MINUTES}" == "" ] ; then
  SALLOC_CTEST_TIME_LIMIT_MINUTES=1:30:00
  # This is just running tests, not the entire build!
fi

if [ "${Trilinos_CTEST_DO_ALL_AT_ONCE}" == "" ] ; then
  export Trilinos_CTEST_DO_ALL_AT_ONCE=TRUE
fi

set -x

source $WORKSPACE/Trilinos/cmake/std/atdm/load-env.sh $JOB_NAME

#source $WORKSPACE/Trilinos/cmake/ctest/drivers/atdm/ctest-s-driver-config-build.sh

set -x

atdm_run_script_on_compute_node \
  $WORKSPACE/Trilinos/cmake/ctest/drivers/atdm/ctest-s-driver.sh \
  $PWD/ctest-s-driver.out \
  ${SALLOC_CTEST_TIME_LIMIT_MINUTES}
