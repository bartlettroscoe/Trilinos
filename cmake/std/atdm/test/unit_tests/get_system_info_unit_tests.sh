#!/bin/bash

#
# Get location of directories
#

CURRENT_SCRIPTS_DIR=`echo $BASH_SOURCE | sed "s/\(.*\)\/.*\.sh/\1/g"`

if [[ "$(uname)" == "Darwin" ]]; then
  ATDM_CONFIG_SCRIPT_DIR="../.."
  ATDM_UTIL_SCRIPT_GET_KNOWN_SYSTEM_INFO="${ATDM_CONFIG_SCRIPT_DIR}/utils/get_known_system_info.sh"
  SHUNIT2_DIR="${ATDM_CONFIG_SCRIPT_DIR}/../../../commonTools/test/shunit2"
else
  ATDM_CONFIG_SCRIPT_DIR=`readlink -f ${CURRENT_SCRIPTS_DIR}/../..`
  ATDM_UTIL_SCRIPT_GET_KNOWN_SYSTEM_INFO=`readlink -f ${ATDM_CONFIG_SCRIPT_DIR}/utils/get_known_system_info.sh`
  SHUNIT2_DIR=`readlink -f ${ATDM_CONFIG_SCRIPT_DIR}/../../../commonTools/test/shunit2`
fi


#
# Unit test helper functions
# 


function init_system_info_test_env() {
  SNLSYSTEM=
  SEMS_PLATFORM=
  ATDM_SYSTEM_NAME=
  SNLCLUSTER=
  HOST=
  ATDM_CONFIG_SEMS_GET_PLATFORM=/fake/path/for/unit/testing/
  ATDM_CONFIG_GET_KNOW_SYSTEM_INFO_REAL_HOSTNAME_OVERRIDE_FOR_UNIT_TESTING=dummyhost
  ATDM_CONFIG_BUILD_NAME=
  ATDM_CONFIG_DISABLE_WARNINGS=ON
}


function known_system_by_hostname_test_helper() {
  init_system_info_test_env
  realhostname=$1
  expected_cdash_hostname=$2
  expected_system_name=$3
  export ATDM_CONFIG_GET_KNOW_SYSTEM_INFO_REAL_HOSTNAME_OVERRIDE_FOR_UNIT_TESTING=${realhostname}
  export ATDM_CONFIG_BUILD_NAME=default
  source ${ATDM_CONFIG_SCRIPT_DIR}/utils/get_system_info.sh
  ${_ASSERT_EQUALS_} ${realhostname} "${ATDM_CONFIG_REAL_HOSTNAME}"
  ${_ASSERT_EQUALS_} ${expected_cdash_hostname} "${ATDM_CONFIG_CDASH_HOSTNAME}"
  ${_ASSERT_EQUALS_} ${expected_system_name}  "${ATDM_CONFIG_SYSTEM_NAME}"
  ${_ASSERT_EQUALS_} "${ATDM_CONFIG_SCRIPT_DIR}/${expected_system_name}" \
    "${ATDM_CONFIG_SYSTEM_DIR}"
  assertEquals "" "${ATDM_CONFIG_CUSTOM_SYSTEM_DIR}"
  # NOTE: Have to use assertEquals instead of macro ${_ASSERT_EQUALS_} beause
  # latter does not work with empty values?
}


#
# Test specific some known configurations
#

function test_ride_default() {
  known_system_by_hostname_test_helper ride11 ride ride
}

function test_white_default() {
  known_system_by_hostname_test_helper white22 white ride
}

function test_vortex_default() {
  known_system_by_hostname_test_helper vortex60 vortex ats2
}


#
# Run the unit tests
#

. ${SHUNIT2_DIR}/shunit2
