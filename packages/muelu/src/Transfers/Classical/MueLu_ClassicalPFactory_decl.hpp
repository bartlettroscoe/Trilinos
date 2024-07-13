// @HEADER
// *****************************************************************************
//        MueLu: A package for multigrid based preconditioning
//
// Copyright 2012 NTESS and the MueLu contributors.
// SPDX-License-Identifier: BSD-3-Clause
// *****************************************************************************
// @HEADER

#ifndef MUELU_CLASSICALPFACTORY_DECL_HPP
#define MUELU_CLASSICALPFACTORY_DECL_HPP

#include <Xpetra_MultiVectorFactory_fwd.hpp>
#include <Xpetra_VectorFactory_fwd.hpp>
#include <Xpetra_CrsGraphFactory_fwd.hpp>
#include <Xpetra_Map_fwd.hpp>
#include <Xpetra_MapFactory_fwd.hpp>
#include <Xpetra_Import_fwd.hpp>
#include <Xpetra_Vector_fwd.hpp>

#include "MueLu_ConfigDefs.hpp"
#include "MueLu_PerfUtils_fwd.hpp"
#include "MueLu_PFactory.hpp"
#include "MueLu_ClassicalPFactory_fwd.hpp"
#include "MueLu_ClassicalMapFactory_fwd.hpp"
#include "MueLu_AmalgamationInfo_fwd.hpp"
#include "MueLu_LWGraph_fwd.hpp"
#include "MueLu_Level_fwd.hpp"

namespace MueLu {

template <class Scalar        = DefaultScalar,
          class LocalOrdinal  = DefaultLocalOrdinal,
          class GlobalOrdinal = DefaultGlobalOrdinal,
          class Node          = DefaultNode>
class ClassicalPFactory : public PFactory {
#undef MUELU_CLASSICALPFACTORY_SHORT
#include "MueLu_UseShortNames.hpp"

 public:
  // Defining types that require the short names included above
  using point_type = typename ClassicalMapFactory::point_type;

  //! @name Constructors/Destructors.
  //@{

  //! Constructor
  ClassicalPFactory() {}

  //! Destructor.
  virtual ~ClassicalPFactory() {}
  //@}

  RCP<const ParameterList> GetValidParameterList() const;

  //! Input
  //@{

  void DeclareInput(Level& fineLevel, Level& coarseLevel) const;

  //@}

  //! @name Build methods.
  //@{

  void Build(Level& fineLevel, Level& coarseLevel) const;
  void BuildP(Level& fineLevel, Level& coarseLevel) const;

 private:
  // Utility algorithms
  void GenerateStrengthFlags(const Matrix& A, const LWGraph& graph, Teuchos::Array<size_t>& eis_rowptr, Teuchos::Array<bool>& edgeIsStrong) const;

  // Ghosting Algorithms
  void GhostCoarseMap(const Matrix& A, const Import& Importer, const ArrayRCP<const LO> myPointType, const RCP<const Map>& coarseMap, RCP<const Map>& coarseColMap) const;

  // Coarsening algorithms
  void Coarsen_ClassicalModified(const Matrix& A, const RCP<const Matrix>& Aghost, const LWGraph& graph, RCP<const Map>& coarseColMap, RCP<const Map>& coarseDomainMap, LO num_c_points, LO num_f_points, const Teuchos::ArrayView<const LO>& myPointType, const Teuchos::ArrayView<const LO>& myPointType_ghost, const Teuchos::Array<LO>& cpoint2pcol, const Teuchos::Array<LO>& pcol2cpoint, Teuchos::Array<size_t>& eis_rowptr, Teuchos::Array<bool>& edgeIsStrong, RCP<LocalOrdinalVector>& BlockNumber, RCP<const Import> remoteOnlyImporter, RCP<Matrix>& P) const;
  void Coarsen_Direct(const Matrix& A, const RCP<const Matrix>& Aghost, const LWGraph& graph, RCP<const Map>& coarseColMap, RCP<const Map>& coarseDomainMap, LO num_c_points, LO num_f_points, const Teuchos::ArrayView<const LO>& myPointType, const Teuchos::ArrayView<const LO>& myPointType_ghost, const Teuchos::Array<LO>& cpoint2pcol, const Teuchos::Array<LO>& pcol2cpoint, Teuchos::Array<size_t>& eis_rowptr, Teuchos::Array<bool>& edgeIsStrong, RCP<LocalOrdinalVector>& BlockNumber, RCP<Matrix>& P) const;
  void Coarsen_Ext_Plus_I(const Matrix& A, const RCP<const Matrix>& Aghost, const LWGraph& graph, RCP<const Map>& coarseColMap, RCP<const Map>& coarseDomainMap, LO num_c_points, LO num_f_points, const Teuchos::ArrayView<const LO>& myPointType, const Teuchos::ArrayView<const LO>& myPointType_ghost, const Teuchos::Array<LO>& cpoint2pcol, const Teuchos::Array<LO>& pcol2cpoint, Teuchos::Array<size_t>& eis_rowptr, Teuchos::Array<bool>& edgeIsStrong, RCP<LocalOrdinalVector>& BlockNumber, RCP<Matrix>& P) const;

  //@}

};  // class ClassicalPFactory

}  // namespace MueLu

#define MUELU_CLASSICALPFACTORY_SHORT
#endif  // MUELU_CLASSICALPFACTORY_DECL_HPP
