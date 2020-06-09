tribits_package(TrilinosBuildStats)

tribits_add_advanced_test( Results
  OVERALL_WORKING_DIRECTORY TEST_NAME
  OVERALL_NUM_MPI_PROCS 1
  TEST_0 CMND "${${PROJECT_NAME}_BINARY_DIR}/gather_build_stats.sh"
    WORKING_DIRECTORY "${${PROJECT_NAME}_BINARY_DIR}"
    SKIP_CLEAN_WORKING_DIRECTORY  # Critical or you delete the entire working dir!
    ALWAYS_FAIL_ON_NONZERO_RETURN
  TEST_1 CMND echo
    ARGS "ToDo: Run some tool that will give nice summary build stats to STDOUT!"
    ALWAYS_FAIL_ON_NONZERO_RETURN
  ADDED_TEST_NAME_OUT Results_TEST_NAME
  )

if (Results_TEST_NAME)
  set_tests_properties( ${Results_TEST_NAME} PROPERTIES
    ATTACHED_FILES "${${PROJECT_NAME}_BINARY_DIR}/build_stats.csv")
endif()

tribits_package_postprocess()
