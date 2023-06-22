/*@HEADER
// ***********************************************************************
//
//       Ifpack2: Templated Object-Oriented Algebraic Preconditioner Package
//                 Copyright (2009) Sandia Corporation
//
// Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
// license for use of this work by or on behalf of the U.S. Government.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact Michael A. Heroux (maherou@sandia.gov)
//
// ***********************************************************************
//@HEADER
*/

/// @file Ifpack2_Details_FastILU_Base_def.hpp

#ifndef __IFPACK2_FASTILU_BASE_DEF_HPP__
#define __IFPACK2_FASTILU_BASE_DEF_HPP__

#include <Ifpack2_Details_CrsArrays.hpp>
#include "Tpetra_BlockCrsMatrix.hpp"
#include "Tpetra_BlockCrsMatrix_Helpers.hpp"
#include "Ifpack2_Details_getCrsMatrix.hpp"
#include <KokkosKernels_Utils.hpp>
#include <Kokkos_Timer.hpp>
#include <Teuchos_TimeMonitor.hpp>
#include <stdexcept>

namespace Ifpack2
{
namespace Details
{

template <typename View1, typename View2, typename View3>
std::vector<std::vector<typename View3::non_const_value_type>> decompress_matrix(
  const View1& row_map,
  const View2& entries,
  const View3& values,
  const int block_size)
{
  using size_type = typename View1::non_const_value_type;
  using lno_t     = typename View2::non_const_value_type;
  using scalar_t  = typename View3::non_const_value_type;

  const size_type nrows = row_map.extent(0) - 1;
  std::vector<std::vector<scalar_t>> result;
  result.resize(nrows);
  for (auto& row : result) {
    row.resize(nrows, 0.0);
  }

  std::cout << "cols: " << entries.extent(0) << std::endl;

  for (size_type row_idx = 0; row_idx < nrows; ++row_idx) {
    const size_type row_nnz_begin = row_map(row_idx);
    const size_type row_nnz_end   = row_map(row_idx + 1);
    for (size_type row_nnz = row_nnz_begin; row_nnz < row_nnz_end; ++row_nnz) {
      const lno_t col_idx      = entries(row_nnz);
      const scalar_t value     = values.extent(0) > 0 ? values(row_nnz) : 1;
      result[row_idx][col_idx] = value;
    }
  }

  return result;
}

template <typename scalar_t>
void print_matrix(const std::vector<std::vector<scalar_t>>& matrix) {
  for (const auto& row : matrix) {
    for (const auto& item : row) {
      std::printf("%.2f ", item);
    }
    std::cout << std::endl;
  }
}

template <typename View>
void print_view(const View& view, const std::string& name)
{
  std::cout << name << "(" << view.extent(0) << "): ";
  for (size_t i = 0; i < view.extent(0); ++i) {
    std::cout << view(i) << ", ";
  }
  std::cout << std::endl;
}

template<class Scalar, class LocalOrdinal, class GlobalOrdinal, class Node>
FastILU_Base<Scalar, LocalOrdinal, GlobalOrdinal, Node>::
FastILU_Base(Teuchos::RCP<const TRowMatrix> A) :
  mat_(A),
  initFlag_(false),
  computedFlag_(false),
  nInit_(0),
  nComputed_(0),
  nApply_(0),
  initTime_(0.0),
  computeTime_(0.0),
  applyTime_(0.0),
  crsCopyTime_(0.0),
  params_(Params::getDefaults()) {}

template<class Scalar, class LocalOrdinal, class GlobalOrdinal, class Node>
Teuchos::RCP<const Tpetra::Map<LocalOrdinal,GlobalOrdinal,Node> >
FastILU_Base<Scalar, LocalOrdinal, GlobalOrdinal, Node>::
getDomainMap () const
{
  return mat_->getDomainMap();
}

template<class Scalar, class LocalOrdinal, class GlobalOrdinal, class Node>
Teuchos::RCP<const Tpetra::Map<LocalOrdinal,GlobalOrdinal,Node> >
FastILU_Base<Scalar, LocalOrdinal, GlobalOrdinal, Node>::
getRangeMap () const
{
  return mat_->getRangeMap();
}

template<class Scalar, class LocalOrdinal, class GlobalOrdinal, class Node>
void FastILU_Base<Scalar, LocalOrdinal, GlobalOrdinal, Node>::
apply (const Tpetra::MultiVector<Scalar,LocalOrdinal,GlobalOrdinal,Node> &X,
       Tpetra::MultiVector<Scalar,LocalOrdinal,GlobalOrdinal,Node> &Y,
       Teuchos::ETransp mode,
       Scalar alpha,
       Scalar beta) const
{
  const std::string timerName ("Ifpack2::FastILU::apply");
  Teuchos::RCP<Teuchos::Time> timer = Teuchos::TimeMonitor::lookupCounter (timerName);
  if (timer.is_null ()) {
    timer = Teuchos::TimeMonitor::getNewCounter (timerName);
  }
  Teuchos::TimeMonitor timeMon (*timer);

  if(!isInitialized() || !isComputed())
  {
    throw std::runtime_error(std::string("Called ") + getName() + "::apply() without first calling initialize() and/or compute().");
  }
  if(X.getNumVectors() != Y.getNumVectors())
  {
    throw std::invalid_argument(getName() + "::apply: X and Y have different numbers of vectors (pass X and Y with exactly matching dimensions)");
  }
  if(X.getLocalLength() != Y.getLocalLength())
  {
    throw std::invalid_argument(getName() + "::apply: X and Y have different lengths (pass X and Y with exactly matching dimensions)");
  }
  //zero out applyTime_ now, because the calls to applyLocalPrec() will add to it
  applyTime_ = 0;
  int  nvecs = X.getNumVectors();
  auto nrowsX = X.getLocalLength();
  auto nrowsY = Y.getLocalLength();
  if(nvecs == 1)
  {
    auto x2d = X.getLocalViewDevice(Tpetra::Access::ReadOnly);
    auto y2d = Y.getLocalViewDevice(Tpetra::Access::ReadWrite);
    ImplScalarArray x1d (const_cast<ImplScalar*>(x2d.data()), nrowsX);
    ImplScalarArray y1d (const_cast<ImplScalar*>(y2d.data()), nrowsY);

    applyLocalPrec(x1d, y1d);
  }
  else
  {
    //Solve each vector one at a time (until FastILU supports multiple RHS)
    auto x2d = X.getLocalViewDevice(Tpetra::Access::ReadOnly);
    auto y2d = Y.getLocalViewDevice(Tpetra::Access::ReadWrite);
    for(int i = 0; i < nvecs; i++)
    {
      auto xColView1d = Kokkos::subview(x2d, Kokkos::ALL(), i);
      auto yColView1d = Kokkos::subview(y2d, Kokkos::ALL(), i);
      ImplScalarArray x1d (const_cast<ImplScalar*>(xColView1d.data()), nrowsX);
      ImplScalarArray y1d (const_cast<ImplScalar*>(yColView1d.data()), nrowsY);

      applyLocalPrec(x1d, y1d);
    }
  }
}

template<class Scalar, class LocalOrdinal, class GlobalOrdinal, class Node>
void FastILU_Base<Scalar, LocalOrdinal, GlobalOrdinal, Node>::
setParameters (const Teuchos::ParameterList& List)
{
  //Params constructor does all parameter validation, and sets default values
  params_ = Params(List, getName());
}

template<class Scalar, class LocalOrdinal, class GlobalOrdinal, class Node>
void FastILU_Base<Scalar, LocalOrdinal, GlobalOrdinal, Node>::
initialize()
{
  const std::string timerName ("Ifpack2::FastILU::initialize");
  Teuchos::RCP<Teuchos::Time> timer = Teuchos::TimeMonitor::lookupCounter (timerName);
  if (timer.is_null ()) {
    timer = Teuchos::TimeMonitor::getNewCounter (timerName);
  }
  Teuchos::TimeMonitor timeMon (*timer);

  if(mat_.is_null())
  {
    throw std::runtime_error(std::string("Called ") + getName() + "::initialize() but matrix was null (call setMatrix() with a non-null matrix first)");
  }

  if (params_.blockCrs) {
    std::cout << "JGF before conversion:" << std::endl;
    auto crs_matrix = Ifpack2::Details::getCrsMatrix(this->mat_);
    CrsArrayReader<Scalar, ImplScalar, LocalOrdinal, GlobalOrdinal, Node>::getStructure(mat_.get(), localRowPtrsHost_, localRowPtrs_, localColInds_);
    CrsArrayReader<Scalar, ImplScalar, LocalOrdinal, GlobalOrdinal, Node>::getValues(mat_.get(), localValues_, localRowPtrsHost_);
    print_view(localRowPtrs_, "rowptrs");
    print_view(localColInds_, "colids");
    print_view(localValues_,  "localValues_");
    print_matrix(decompress_matrix(localRowPtrs_, localColInds_, localValues_, 1));

    localRowPtrsHost2_ = localRowPtrsHost_;
    localRowPtrs2_     = localRowPtrs_;
    localColInds2_     = localColInds_;
    localValues2_      = localValues_;

    auto crs_row_map = crs_matrix->getRowMap();
    auto crs_col_map = crs_matrix->getColMap();

    // Populate blocks, they must be fully populated with entries so fill with zeroes
    const auto nrows = localRowPtrsHost_.size() - 1;
    std::vector<LocalOrdinal> local_new_rowmap(nrows+1);
    LocalOrdinal new_nnz_count = 0;
    const auto block_size = params_.blockCrsSize;
    const auto blocks_per_row = nrows / block_size; // assumes square matrix
    std::vector<bool> row_block_active(blocks_per_row, false);
    assert(nrows % block_size == 0);
    // Sizing
    for (size_t row = 0; row < nrows; ++row) {
      local_new_rowmap[row] = new_nnz_count;
      LocalOrdinal row_itr = localRowPtrs_(row);

      if (row % block_size == 0) {
        // We are starting a fresh sequence of blocks in the vertical
        row_block_active.assign(row_block_active.size(), false);
      }

      for (size_t row_block_idx = 0; row_block_idx < blocks_per_row; ++row_block_idx) {
        const LocalOrdinal first_possible_col_in_block      = row_block_idx * block_size;
        const LocalOrdinal first_possible_col_in_next_block = (row_block_idx+1) * block_size;
        LocalOrdinal curr_nnz_col = localColInds_(row_itr);
        if (curr_nnz_col >= first_possible_col_in_block && curr_nnz_col < first_possible_col_in_next_block) {
          // This block has at least one nnz in this row
          row_block_active[row_block_idx] = true;
        }
        if (row_block_active[row_block_idx]) {
          new_nnz_count += block_size;
          for (LocalOrdinal possible_col = first_possible_col_in_block; possible_col < first_possible_col_in_next_block; ++possible_col) {
            curr_nnz_col = localColInds_(row_itr);
            if (possible_col == curr_nnz_col) {
              // Already a non-zero entry, skip
              ++row_itr;
            }
          }
        }
      }
    }
    local_new_rowmap[nrows] = new_nnz_count;

    std::vector<LocalOrdinal> local_col_ids_to_insert(new_nnz_count);
    std::vector<Scalar>       local_vals_to_insert(new_nnz_count, 0.0);

    for (size_t row = 0; row < nrows; ++row) {
      LocalOrdinal row_itr = localRowPtrs_(row);
      LocalOrdinal row_end = localRowPtrs_(row+1);
      LocalOrdinal row_itr_new = local_new_rowmap[row];

      if (row % block_size == 0) {
        // We are starting a fresh sequence of blocks in the vertical
        row_block_active.assign(row_block_active.size(), false);
      }

      for (size_t row_block_idx = 0; row_block_idx < blocks_per_row; ++row_block_idx) {
        const LocalOrdinal first_possible_col_in_block      = row_block_idx * block_size;
        const LocalOrdinal first_possible_col_in_next_block = (row_block_idx+1) * block_size;
        LocalOrdinal curr_nnz_col = localColInds_(row_itr);
        if (curr_nnz_col >= first_possible_col_in_block && curr_nnz_col < first_possible_col_in_next_block) {
          // This block has at least one nnz in this row
          row_block_active[row_block_idx] = true;
        }
        if (row_block_active[row_block_idx]) {
          for (LocalOrdinal possible_col = first_possible_col_in_block; possible_col < first_possible_col_in_next_block; ++possible_col, ++row_itr_new) {
            local_col_ids_to_insert[row_itr_new] = possible_col;
            curr_nnz_col = localColInds_(row_itr);
            if (possible_col == curr_nnz_col && row_itr < row_end) {
              // Already a non-zero entry
              local_vals_to_insert[row_itr_new] = localValues_(row_itr);
              ++row_itr;
            }
          }
        }
      }
    }

    // Create new TCrsMatrix with the new filled data
    using local_crs = typename TCrsMatrix::local_matrix_device_type;
    local_crs kk_crs_matrix_block_filled(
      "a-blk-filled", nrows, nrows, new_nnz_count, local_vals_to_insert.data(), local_new_rowmap.data(), local_col_ids_to_insert.data());
    TCrsMatrix crs_matrix_block_filled(crs_row_map, crs_col_map, kk_crs_matrix_block_filled);
    mat_ = Teuchos::RCP(&crs_matrix_block_filled, false);

    std::cout << "JGF after filling:" << std::endl;
    CrsArrayReader<Scalar, ImplScalar, LocalOrdinal, GlobalOrdinal, Node>::getStructure(mat_.get(), localRowPtrsHost_, localRowPtrs_, localColInds_);
    CrsArrayReader<Scalar, ImplScalar, LocalOrdinal, GlobalOrdinal, Node>::getValues(mat_.get(), localValues_, localRowPtrsHost_);
    print_view(localRowPtrs_, "rowptrs");
    print_view(localColInds_, "colids");
    print_view(localValues_,  "localValues_");
    print_matrix(decompress_matrix(localRowPtrs_, localColInds_, localValues_, 1));

    auto bcrs_matrix = Tpetra::convertToBlockCrsMatrix(crs_matrix_block_filled, params_.blockCrsSize);
    mat_ = bcrs_matrix;

    std::cout << "JGF after conversion:" << std::endl;
    CrsArrayReader<Scalar, ImplScalar, LocalOrdinal, GlobalOrdinal, Node>::getStructure(mat_.get(), localRowPtrsHost_, localRowPtrs_, localColInds_);
    CrsArrayReader<Scalar, ImplScalar, LocalOrdinal, GlobalOrdinal, Node>::getValues(mat_.get(), localValues_, localRowPtrsHost_);
    print_view(localRowPtrs_, "rowptrs");
    print_view(localColInds_, "colids");
    print_view(localValues_,  "localValues_");
    print_matrix(decompress_matrix(localRowPtrs_, localColInds_, localValues_, 1));

    // std::cout << "JGF after converting back:" << std::endl;
    // mat_ = Tpetra::convertToCrsMatrix(*bcrs_matrix);
    // CrsArrayReader<Scalar, ImplScalar, LocalOrdinal, GlobalOrdinal, Node>::getStructure(mat_.get(), localRowPtrsHost_, localRowPtrs_, localColInds_);
    // CrsArrayReader<Scalar, ImplScalar, LocalOrdinal, GlobalOrdinal, Node>::getValues(mat_.get(), localValues_, localRowPtrsHost_);
    // print_view(localRowPtrs_, "rowptrs");
    // print_view(localColInds_, "colids");
    // print_view(localValues_,  "localValues_");
    // print_matrix(decompress_matrix(localRowPtrs_, localColInds_, localValues_, 1));
  }

  Kokkos::Timer copyTimer;
  CrsArrayReader<Scalar, ImplScalar, LocalOrdinal, GlobalOrdinal, Node>::getStructure(mat_.get(), localRowPtrsHost_, localRowPtrs_, localColInds_);
  crsCopyTime_ = copyTimer.seconds();

  if (params_.use_metis)
  {
    assert(!params_.blockCrs);
    const std::string timerNameMetis ("Ifpack2::FastILU::Metis");
    Teuchos::RCP<Teuchos::Time> timerMetis = Teuchos::TimeMonitor::lookupCounter (timerNameMetis);
    if (timerMetis.is_null ()) {
      timerMetis = Teuchos::TimeMonitor::getNewCounter (timerNameMetis);
    }
    Teuchos::TimeMonitor timeMonMetis (*timerMetis);
    #ifdef HAVE_IFPACK2_METIS
    idx_t nrows = localRowPtrsHost_.size() - 1;
    if (nrows > 0) {
      // reorder will convert both graph and perm/iperm to the internal METIS integer type
      metis_perm_  = MetisArrayHost(Kokkos::ViewAllocateWithoutInitializing("metis_perm"),  nrows);
      metis_iperm_ = MetisArrayHost(Kokkos::ViewAllocateWithoutInitializing("metis_iperm"), nrows);

      // copy ColInds to host
      auto localColIndsHost_ = Kokkos::create_mirror_view(localColInds_);
      Kokkos::deep_copy(localColIndsHost_, localColInds_);

      // prepare for calling metis
      idx_t nnz = localColIndsHost_.size();
      MetisArrayHost metis_rowptr;
      MetisArrayHost metis_colidx;

      bool metis_symmetrize = true;
      if (metis_symmetrize) {
        // symmetrize
        using OrdinalArrayMirror = typename OrdinalArray::host_mirror_type;
        KokkosKernels::Impl::symmetrize_graph_symbolic_hashmap<
          OrdinalArrayHost, OrdinalArrayMirror, MetisArrayHost, MetisArrayHost, Kokkos::HostSpace::execution_space>
          (nrows, localRowPtrsHost_, localColIndsHost_, metis_rowptr, metis_colidx);

        // remove diagonals
        idx_t old_nnz = nnz = 0;
        for (idx_t i = 0; i < nrows; i++) {
          for (LocalOrdinal k = old_nnz; k < metis_rowptr(i+1); k++) {
            if (metis_colidx(k) != i) {
              metis_colidx(nnz) = metis_colidx(k);
              nnz++;
            }
          }
          old_nnz = metis_rowptr(i+1);
          metis_rowptr(i+1) = nnz;
        }
      } else {
        // copy and remove diagonals
        metis_rowptr = MetisArrayHost(Kokkos::ViewAllocateWithoutInitializing("metis_rowptr"), nrows+1);
        metis_colidx = MetisArrayHost(Kokkos::ViewAllocateWithoutInitializing("metis_colidx"), nnz);
        nnz = 0;
        metis_rowptr(0) = 0;
        for (idx_t i = 0; i < nrows; i++) {
          for (LocalOrdinal k = localRowPtrsHost_(i); k < localRowPtrsHost_(i+1); k++) {
            if (localColIndsHost_(k) != i) {
              metis_colidx(nnz) = localColIndsHost_(k);
              nnz++;
            }
          }
          metis_rowptr(i+1) = nnz;
        }
      }

      // call metis
      int info = METIS_NodeND(&nrows, metis_rowptr.data(), metis_colidx.data(),
                              NULL, NULL, metis_perm_.data(), metis_iperm_.data());
      if (METIS_OK != info) {
        throw std::runtime_error(std::string("METIS_NodeND returned info = " + info));
      }
    }
    #else
    throw std::runtime_error(std::string("TPL METIS is not enabled"));
    #endif
  }

  initLocalPrec();  //note: initLocalPrec updates initTime
  std::cout << "JGF MADE IT PAST INIT!!" << std::endl;
  initFlag_ = true;
  nInit_++;
}

template<class Scalar, class LocalOrdinal, class GlobalOrdinal, class Node>
bool FastILU_Base<Scalar, LocalOrdinal, GlobalOrdinal, Node>::
isInitialized() const
{
  return initFlag_;
}

template<class Scalar, class LocalOrdinal, class GlobalOrdinal, class Node>
void FastILU_Base<Scalar, LocalOrdinal, GlobalOrdinal, Node>::
compute()
{
  if(!initFlag_)
  {
    throw std::runtime_error(getName() + ": initialize() must be called before compute()");
  }

  const std::string timerName ("Ifpack2::FastILU::compute");
  Teuchos::RCP<Teuchos::Time> timer = Teuchos::TimeMonitor::lookupCounter (timerName);
  if (timer.is_null ()) {
    timer = Teuchos::TimeMonitor::getNewCounter (timerName);
  }
  Teuchos::TimeMonitor timeMon (*timer);

  //get copy of values array from matrix
  Kokkos::Timer copyTimer;
  CrsArrayReader<Scalar, ImplScalar, LocalOrdinal, GlobalOrdinal, Node>::getValues(mat_.get(), localValues_, localRowPtrsHost_);
  crsCopyTime_ += copyTimer.seconds(); //add to the time spent getting rowptrs/colinds
  computeLocalPrec(); //this updates computeTime_
  computedFlag_ = true;
  nComputed_++;
}

template<class Scalar, class LocalOrdinal, class GlobalOrdinal, class Node>
bool FastILU_Base<Scalar, LocalOrdinal, GlobalOrdinal, Node>::
isComputed() const
{
  return computedFlag_;
}

template<class Scalar, class LocalOrdinal, class GlobalOrdinal, class Node>
Teuchos::RCP<const Tpetra::RowMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Node> >
FastILU_Base<Scalar, LocalOrdinal, GlobalOrdinal, Node>::
getMatrix() const
{
  return mat_;
}

template<class Scalar, class LocalOrdinal, class GlobalOrdinal, class Node>
int FastILU_Base<Scalar, LocalOrdinal, GlobalOrdinal, Node>::
getNumInitialize() const
{
  return nInit_;
}

template<class Scalar, class LocalOrdinal, class GlobalOrdinal, class Node>
int FastILU_Base<Scalar, LocalOrdinal, GlobalOrdinal, Node>::
getNumCompute() const
{
  return nComputed_;
}

template<class Scalar, class LocalOrdinal, class GlobalOrdinal, class Node>
int FastILU_Base<Scalar, LocalOrdinal, GlobalOrdinal, Node>::
getNumApply() const
{
  return nApply_;
}

template<class Scalar, class LocalOrdinal, class GlobalOrdinal, class Node>
double FastILU_Base<Scalar, LocalOrdinal, GlobalOrdinal, Node>::
getInitializeTime() const
{
  return initTime_;
}

template<class Scalar, class LocalOrdinal, class GlobalOrdinal, class Node>
double FastILU_Base<Scalar, LocalOrdinal, GlobalOrdinal, Node>::
getComputeTime() const
{
  return computeTime_;
}

template<class Scalar, class LocalOrdinal, class GlobalOrdinal, class Node>
double FastILU_Base<Scalar, LocalOrdinal, GlobalOrdinal, Node>::
getApplyTime() const
{
  return applyTime_;
}

template<class Scalar, class LocalOrdinal, class GlobalOrdinal, class Node>
double FastILU_Base<Scalar, LocalOrdinal, GlobalOrdinal, Node>::
getCopyTime() const
{
  return crsCopyTime_;
}

template<class Scalar, class LocalOrdinal, class GlobalOrdinal, class Node>
void FastILU_Base<Scalar, LocalOrdinal, GlobalOrdinal, Node>::
checkLocalILU() const
{
  //if the underlying type of this doesn't implement checkLocalILU, it's an illegal operation
  throw std::runtime_error(std::string("Preconditioner type Ifpack2::Details::") + getName() + " doesn't support checkLocalILU().");
}

template<class Scalar, class LocalOrdinal, class GlobalOrdinal, class Node>
void FastILU_Base<Scalar, LocalOrdinal, GlobalOrdinal, Node>::
checkLocalIC() const
{
  //if the underlying type of this doesn't implement checkLocalIC, it's an illegal operation
  throw std::runtime_error(std::string("Preconditioner type Ifpack2::Details::") + getName() + " doesn't support checkLocalIC().");
}

template<typename Scalar, typename LocalOrdinal, typename GlobalOrdinal, typename Node>
std::string FastILU_Base<Scalar, LocalOrdinal, GlobalOrdinal, Node>::description() const
{
  std::ostringstream os;
  //Output is a YAML dictionary
  os << "\"Ifpack2::Details::" << getName() << "\": {";
  os << "Initialized: " << (isInitialized() ? "true" : "false") << ", ";
  os << "Computed: " << (isComputed() ? "true" : "false") << ", ";
  os << "Sweeps: " << getSweeps() << ", ";
  os << "Triangular solve type: " << getSpTrsvType() << ", ";
  if (getSpTrsvType() == "Fast") {
    os << "# of triangular solve iterations: " << getNTrisol() << ", ";
  }
  if(mat_.is_null())
  {
    os << "Matrix: null";
  }
  else
  {
    os << "Global matrix dimensions: [" << mat_->getGlobalNumRows() << ", " << mat_->getGlobalNumCols() << "]";
    os << ", Global nnz: " << mat_->getGlobalNumEntries();
  }
  return os.str();
}

template<typename Scalar, typename LocalOrdinal, typename GlobalOrdinal, typename Node>
void FastILU_Base<Scalar, LocalOrdinal, GlobalOrdinal, Node>::
setMatrix(const Teuchos::RCP<const TRowMatrix>& A)
{
  if(A.is_null())
  {
    throw std::invalid_argument(std::string("Ifpack2::Details::") + getName() + "::setMatrix() called with a null matrix. Pass a non-null matrix.");
  }
  //bmk note: this modeled after RILUK::setMatrix
  if(mat_.get() != A.get())
  {
    mat_ = A;
    initFlag_ = false;
    computedFlag_ = false;
  }
}

template<typename Scalar, typename LocalOrdinal, typename GlobalOrdinal, typename Node>
typename FastILU_Base<Scalar, LocalOrdinal, GlobalOrdinal, Node>::Params
FastILU_Base<Scalar, LocalOrdinal, GlobalOrdinal, Node>::
Params::getDefaults()
{
  Params p;
  p.use_metis = false;
  p.sptrsv_algo = FastILU::SpTRSV::Fast;
  p.nFact = 5;          // # of sweeps for computing fastILU
  p.nTrisol = 5;        // # of sweeps for applying fastSpTRSV
  p.level = 0;          // level of ILU
  p.omega = 1.0;        // damping factor for fastILU
  p.shift = 0;
  p.guessFlag = true;
  p.blockSizeILU = 1;   // # of nonzeros / thread, for fastILU
  p.blockSize = 1;      // # of rows / thread, for SpTRSV
  p.blockCrs = false;   // whether to use block CRS for fastILU
  p.blockCrsSize = 3;   // block size for block CRS
  return p;
}

template<typename Scalar, typename LocalOrdinal, typename GlobalOrdinal, typename Node>
FastILU_Base<Scalar, LocalOrdinal, GlobalOrdinal, Node>::
Params::Params(const Teuchos::ParameterList& pL, std::string precType)
{
  *this = getDefaults();
  //For each parameter, check that if the parameter exists, it has the right type
  //Then get the value and sanity check it
  //If the parameter doesn't exist, leave it as default (from Params::getDefaults())
  //"sweeps" aka nFact
  #define TYPE_ERROR(name, correctTypeName) {throw std::invalid_argument(precType + "::setParameters(): parameter \"" + name + "\" has the wrong type (must be " + correctTypeName + ")");}
  #define CHECK_VALUE(param, member, cond, msg) {if(cond) {throw std::invalid_argument(precType + "::setParameters(): parameter \"" + param + "\" has value " + std::to_string(member) + " but " + msg);}}

  //metis
  if(pL.isParameter("metis"))
  {
    if(pL.isType<bool>("metis"))
      use_metis = pL.get<bool>("metis");
    else
      TYPE_ERROR("metis", "bool");
  }

  if(pL.isParameter("sweeps"))
  {
    if(pL.isType<int>("sweeps"))
    {
      nFact = pL.get<int>("sweeps");
      CHECK_VALUE("sweeps", nFact, nFact < 1, "must have a value of at least 1");
    }
    else
      TYPE_ERROR("sweeps", "int");
  }
  std::string sptrsv_type = "Fast";
  if(pL.isParameter("triangular solve type")) {
    sptrsv_type = pL.get<std::string> ("triangular solve type");
  }
  if (sptrsv_type == "Standard Host") {
    sptrsv_algo = FastILU::SpTRSV::StandardHost;
  } else if (sptrsv_type == "Standard") {
    sptrsv_algo = FastILU::SpTRSV::Standard;
  }

  //"triangular solve iterations" aka nTrisol
  if(pL.isParameter("triangular solve iterations"))
  {
    if(pL.isType<int>("triangular solve iterations"))
    {
      nTrisol = pL.get<int>("triangular solve iterations");
      CHECK_VALUE("triangular solve iterations", nTrisol, nTrisol < 1, "must have a value of at least 1");
    }
    else
      TYPE_ERROR("triangular solve iterations", "int");
  }
  //"level"
  if(pL.isParameter("level"))
  {
    if(pL.isType<int>("level"))
    {
      level = pL.get<int>("level");
    }
    else if(pL.isType<double>("level"))
    {
      //Level can be read as double (like in ILUT), but must be an exact integer
      //Any integer used for level-of-fill can be represented exactly in double (so use exact compare)
      double dval = pL.get<double>("level");
      double ipart;
      double fpart = modf(dval, &ipart);
      level = ipart;
      CHECK_VALUE("level", level, fpart != 0, "must be an integral value");
    }
    else
    {
      TYPE_ERROR("level", "int");
    }
    CHECK_VALUE("level", level, level < 0, "must be nonnegative");
  }
  if(pL.isParameter("damping factor"))
  {
    if(pL.isType<double>("damping factor"))
      omega = pL.get<double>("damping factor");
    else
      TYPE_ERROR("damping factor", "double");
  }
  if(pL.isParameter("shift"))
  {
    if(pL.isType<double>("shift"))
      shift = pL.get<double>("shift");
    else
      TYPE_ERROR("shift", "double");
  }
  //"guess" aka guessFlag
  if(pL.isParameter("guess"))
  {
    if(pL.isType<bool>("guess"))
      guessFlag = pL.get<bool>("guess");
    else
      TYPE_ERROR("guess", "bool");
  }
  //"block size" aka blkSz
  if(pL.isParameter("block size for ILU"))
  {
    if(pL.isType<int>("block size for ILU"))
    {
      blockSizeILU = pL.get<int>("block size for ILU");
      CHECK_VALUE("block size for ILU", blockSizeILU, blockSizeILU < 1, "must have a value of at least 1");
    }
    else
      TYPE_ERROR("block size for ILU", "int");
  }
  //"block size" aka blkSz
  if(pL.isParameter("block size for SpTRSV"))
  {
    if(pL.isType<int>("block size for SpTRSV"))
      blockSize = pL.get<int>("block size for SpTRSV");
    else
      TYPE_ERROR("block size for SpTRSV", "int");
  }
  //"block crs" aka blockCrs
  if(pL.isParameter("block crs"))
  {
    if(pL.isType<bool>("block crs"))
      blockCrs = pL.get<bool>("block crs");
    else
      TYPE_ERROR("block crs", "bool");
  }
  //"block crs block size" aka blockCrsSize
  if(pL.isParameter("block crs block size"))
  {
    if(pL.isType<int>("block crs block size"))
      blockCrsSize = pL.get<int>("block crs block size");
    else
      TYPE_ERROR("block crs block size", "int");
  }
  #undef CHECK_VALUE
  #undef TYPE_ERROR
}

#define IFPACK2_DETAILS_FASTILU_BASE_INSTANT(S, L, G, N) \
template class Ifpack2::Details::FastILU_Base<S, L, G, N>;

} //namespace Details
} //namespace Ifpack2

#endif
