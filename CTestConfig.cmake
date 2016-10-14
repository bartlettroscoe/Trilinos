INCLUDE(SetDefaultAndFromEnv)

# Must match what is in CDash project 'Trilinos'
SET(CTEST_NIGHTLY_START_TIME "04:00:00 UTC") # 10 PM MDT or 9 PM MST

# Set actual CTest/CDash settings

IF (NOT DEFINED CTEST_DROP_METHOD)
  SET_DEFAULT_AND_FROM_ENV(CTEST_DROP_METHOD "http")
ENDIF()

IF (CTEST_DROP_METHOD STREQUAL "http")
  SET_DEFAULT_AND_FROM_ENV(CTEST_DROP_SITE "testing.sandia.gov")
  SET_DEFAULT_AND_FROM_ENV(CTEST_PROJECT_NAME "Trilinos")
  SET_DEFAULT_AND_FROM_ENV(CTEST_DROP_LOCATION "/cdash/submit.php?project=Trilinos")
  SET_DEFAULT_AND_FROM_ENV(CTEST_TRIGGER_SITE "")
  SET_DEFAULT_AND_FROM_ENV(CTEST_DROP_SITE_CDASH TRUE)
  # Temp secondary submit to experimental CDash site (see Trilinos TRIL-79)
  SET_DEFAULT_AND_FROM_ENV(TRIBITS_2ND_CTEST_DROP_SITE
    "testing.sandia.gov")
  SET_DEFAULT_AND_FROM_ENV(TRIBITS_2ND_CTEST_DROP_LOCATION
    "/extended/cdash/submit.php?project=Trilinos")
ENDIF()
