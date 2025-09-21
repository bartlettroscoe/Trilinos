// Unit tests for Teuchos::SerialDenseMatrix class using Google Test Framework
// together with Google Mock framework
#include <gtest/gtest.h>
#include "Teuchos_SerialSymDenseMatrix.hpp"

TEST(SerialDenseMatrixBasic, DummyConstruction) {
  Teuchos::SerialSymDenseMatrix<int,double> A(1);
  A(0,0) = 1.25;
  EXPECT_DOUBLE_EQ(1.25, A(0,0));
}
