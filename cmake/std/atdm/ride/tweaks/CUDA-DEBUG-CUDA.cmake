# This test segfaults in the 'debug' builds on this system (#2466)
ATDM_SET_ENABLE(Belos_Tpetra_PseudoBlockCG_hb_test_MPI_4_DISABLE ON)

INCLUDE("${CMAKE_CURRENT_LIST_DIR}/CUDA_COMMON_TWEAKS.cmake")
