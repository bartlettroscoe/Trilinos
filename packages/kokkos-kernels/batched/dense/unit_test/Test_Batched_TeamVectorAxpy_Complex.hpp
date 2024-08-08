//@HEADER
// ************************************************************************
//
//                        Kokkos v. 4.0
//       Copyright (2022) National Technology & Engineering
//               Solutions of Sandia, LLC (NTESS).
//
// Under the terms of Contract DE-NA0003525 with NTESS,
// the U.S. Government retains certain rights in this software.
//
// Part of Kokkos, under the Apache License v2.0 with LLVM Exceptions.
// See https://kokkos.org/LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//@HEADER

#if defined(KOKKOSKERNELS_INST_COMPLEX_DOUBLE)
TEST_F(TestCategory, batched_scalar_teamvector_axpy_nt_dcomplex_dcomplex) {
  test_batched_teamvector_axpy<TestDevice, Kokkos::complex<double>, Kokkos::complex<double>>();
}

TEST_F(TestCategory, batched_scalar_teamvector_axpy_nt_dcomplex_double) {
  test_batched_teamvector_axpy<TestDevice, Kokkos::complex<double>, double>();
}
#endif
