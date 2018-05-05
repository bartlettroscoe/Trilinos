# Disable test that runs over 30 min currently (#2446)
ATDM_SET_ENABLE(PanzerAdaptersSTK_MixedPoissonExample-ConvTest-Hex-Order-3_DISABLE ON)

# This test segfaults in the 'debug' builds on this system (#2466)
ATDM_SET_ENABLE(Belos_Tpetra_PseudoBlockCG_hb_test_MPI_4_DISABLE ON)
