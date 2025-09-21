

IF (${PACKAGE_NAME}_ENABLE_COMPLEX)

  TRIBITS_ADD_EXECUTABLE_AND_TEST(
    DenseMatrix_test  
    SOURCES cxx_main.cpp
    NUM_MPI_PROCS 1
    COMM serial mpi
    ARGS -v
    STANDARD_PASS_OUTPUT
    )

  TRIBITS_ADD_EXECUTABLE_AND_TEST(
    SymDenseMatrix_test
    SOURCES cxx_main_sym.cpp
    NUM_MPI_PROCS 1
    COMM serial mpi
    ARGS -v
    STANDARD_PASS_OUTPUT
    )

  TRIBITS_ADD_EXECUTABLE_AND_TEST(
    BandDenseMatrix_test  
    SOURCES cxx_main_band.cpp
    NUM_MPI_PROCS 1
    COMM serial mpi
    ARGS -v
    STANDARD_PASS_OUTPUT
    )

ENDIF()

TRIBITS_ADD_EXECUTABLE_AND_TEST(
  SerialSymDenseMatrix_UnitTests
  SOURCES SerialSymDenseMatrix_UnitTests.cpp
  NUM_MPI_PROCS 1
  COMM serial mpi
  ARGS -v
  ADDED_EXE_TARGET_NAME_OUT SymDenseMatrix_UnitTests_TARGET_NAME
  )
if (TARGET ${SymDenseMatrix_UnitTests_TARGET_NAME})
  target_link_libraries(${SymDenseMatrix_UnitTests_TARGET_NAME}
    PRIVATE GTest::gtest_main)
endif()

TRIBITS_ADD_EXECUTABLE_AND_TEST(
  DenseSolver_test
  SOURCES cxx_main_solver.cpp
  NUM_MPI_PROCS 1
  COMM serial mpi
  ARGS -v
  STANDARD_PASS_OUTPUT
  )

TRIBITS_ADD_EXECUTABLE_AND_TEST(
  BandDenseSolver_test
  SOURCES cxx_main_band_solver.cpp
  NUM_MPI_PROCS 1
  COMM serial mpi
  ARGS -v
  STANDARD_PASS_OUTPUT
  )

TRIBITS_ADD_EXECUTABLE_AND_TEST(
  SpdDenseSolver_test
  SOURCES cxx_main_spd_solver.cpp
  NUM_MPI_PROCS 1
  COMM serial mpi
  ARGS -v
  STANDARD_PASS_OUTPUT
  )

TRIBITS_ADD_EXECUTABLE_AND_TEST(
  QRDenseSolver_test
  SOURCES cxx_main_qr_solver.cpp
  NUM_MPI_PROCS 1
  COMM serial mpi
  ARGS -v
  STANDARD_PASS_OUTPUT
  )
