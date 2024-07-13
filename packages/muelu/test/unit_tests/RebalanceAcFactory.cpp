// @HEADER
// *****************************************************************************
//        MueLu: A package for multigrid based preconditioning
//
// Copyright 2012 NTESS and the MueLu contributors.
// SPDX-License-Identifier: BSD-3-Clause
// *****************************************************************************
// @HEADER

/*
 * RebalanceAcFactory.cpp
 *
 *  Created on: 20.09.2011
 *      Author: tobias
 */

#include <Teuchos_UnitTestHarness.hpp>
#include <Teuchos_DefaultComm.hpp>
#include <Teuchos_ScalarTraits.hpp>
#include <Teuchos_RCP.hpp>
#include <Teuchos_ParameterList.hpp>
#include <Teuchos_CommandLineProcessor.hpp>
#include <Teuchos_GlobalMPISession.hpp>
#include <Teuchos_DefaultComm.hpp>
#include <Xpetra_MultiVectorFactory.hpp>
#include <Xpetra_VectorFactory.hpp>
#include <Xpetra_MatrixMatrix.hpp>
// Xpetra
#include <Xpetra_Map.hpp>
#include <Xpetra_MapFactory.hpp>
#include <Xpetra_CrsMatrixWrap.hpp>
#include <Xpetra_VectorFactory.hpp>
#include <Xpetra_MultiVectorFactory.hpp>
#include <Xpetra_Parameters.hpp>
// Galeri
#include <Galeri_XpetraParameters.hpp>
#include <Galeri_XpetraProblemFactory.hpp>
#include <Galeri_XpetraUtils.hpp>

#include <MueLu_TestHelpers.hpp>
#include <MueLu_Utilities.hpp>
#include <MueLu_Version.hpp>

#include <MueLu_PgPFactory.hpp>
#include <MueLu_RAPFactory.hpp>
#include <MueLu_GenericRFactory.hpp>
#include <MueLu_DirectSolver.hpp>
#include <MueLu_AmalgamationFactory.hpp>
#include <MueLu_RepartitionHeuristicFactory.hpp>
#include <MueLu_SaPFactory.hpp>
#include <MueLu_CoalesceDropFactory.hpp>
#include <MueLu_CoarseMapFactory.hpp>
//#include <MueLu_RebalanceAcFactory.hpp>
//#include <MueLu_TrilinosSmoother.hpp>
//#include <MueLu_TentativePFactory.hpp>
#include <MueLu_SmootherFactory.hpp>
#include <MueLu_RebalanceAcFactory.hpp>
#include <MueLu_RepartitionInterface.hpp>
#include <MueLu_IsorropiaInterface.hpp>
#include <MueLu_RebalanceTransferFactory.hpp>
#include <MueLu_RepartitionFactory.hpp>
#include "MueLu_CoordinatesTransferFactory.hpp"
#include "MueLu_TentativePFactory.hpp"
#include "MueLu_TransPFactory.hpp"
#include "MueLu_ZoltanInterface.hpp"

namespace MueLuTests {

// this macro declares the unit-test-class:
TEUCHOS_UNIT_TEST_TEMPLATE_4_DECL(RebalanceAcFactory, Constructor, Scalar, LocalOrdinal, GlobalOrdinal, Node) {
#include <MueLu_UseShortNames.hpp>
  MUELU_TESTING_SET_OSTREAM;
  MUELU_TESTING_LIMIT_SCOPE(Scalar, GlobalOrdinal, Node);
  out << "version: " << MueLu::Version() << std::endl;

  auto rAcFactory = rcp(new RebalanceAcFactory());
  TEST_ASSERT(!rAcFactory.is_null());
  TEST_EQUALITY(rAcFactory->NumRebalanceFactories() == 0, true);

  // Add Factory
  rAcFactory->AddRebalanceFactory(rcp(new RebalanceTransferFactory()));
  rAcFactory->AddRebalanceFactory(rcp(new RebalanceTransferFactory()));

  TEST_EQUALITY(rAcFactory->NumRebalanceFactories() == 2, true);

  auto paramList = rAcFactory->GetValidParameterList();
  TEST_ASSERT(!paramList.is_null());
}

TEUCHOS_UNIT_TEST_TEMPLATE_4_DECL(RebalanceAcFactory, BuildWithImporter, Scalar, LocalOrdinal, GlobalOrdinal, Node) {
#include <MueLu_UseShortNames.hpp>
  MUELU_TESTING_SET_OSTREAM;
  MUELU_TESTING_LIMIT_SCOPE(Scalar, GlobalOrdinal, Node);
  out << "version: " << MueLu::Version() << std::endl;

  if (TestHelpers::Parameters::getLib() == Xpetra::UseEpetra) {
    out << "skipping test for linAlgebra==UseEpetra" << std::endl;
    return;
  }

  RCP<const Teuchos::Comm<int> > comm = Teuchos::DefaultComm<int>::getComm();
  Teuchos::CommandLineProcessor clp(false);
  Galeri::Xpetra::Parameters<GO> matrixParameters(clp, 8748);  // manage parameters of the test case

  Level aLevel;
  Level corseLevel;
  RCP<const Map> map = MapFactory::Build(TestHelpers::Parameters::getLib(), matrixParameters.GetNumGlobalElements(), 0, comm);

  TestHelpers::TestFactory<SC, LO, GO, NO>::createTwoLevelHierarchy(aLevel, corseLevel);
  RCP<Matrix> A = TestHelpers::TestFactory<SC, LO, GO, NO>::Build1DPoisson(2);  // can be an empty operator
  corseLevel.Set("A", A);
  RCP<const Import> importer = ImportFactory::Build(A->getRowMap(), map);
  corseLevel.Set("Importer", importer);

  RCP<RebalanceAcFactory> RebalancedAFact = rcp(new RebalanceAcFactory());
  RebalancedAFact->SetDefaultVerbLevel(MueLu::Extreme);
  RebalancedAFact->Build(aLevel, corseLevel);
}

TEUCHOS_UNIT_TEST_TEMPLATE_4_DECL(RebalanceAcFactory, BuildWithoutImporter, Scalar, LocalOrdinal, GlobalOrdinal, Node) {
#include <MueLu_UseShortNames.hpp>
  MUELU_TESTING_SET_OSTREAM;
  MUELU_TESTING_LIMIT_SCOPE(Scalar, GlobalOrdinal, Node);
  out << "version: " << MueLu::Version() << std::endl;

  RCP<const Teuchos::Comm<int> > comm = Teuchos::DefaultComm<int>::getComm();
  Level aLevel;
  Level corseLevel;
  TestHelpers::TestFactory<SC, LO, GO, NO>::createTwoLevelHierarchy(aLevel, corseLevel);
  RCP<Matrix> A = TestHelpers::TestFactory<SC, LO, GO, NO>::Build1DPoisson(2);  // can be an empty operator
  corseLevel.Set("A", A);
  RCP<const Import> importer = Teuchos::null;
  corseLevel.Set("Importer", importer);

  RCP<RebalanceAcFactory> RebalancedAFact = rcp(new RebalanceAcFactory());
  RebalancedAFact->SetDefaultVerbLevel(MueLu::Extreme);
  RebalancedAFact->Build(aLevel, corseLevel);
}

#define MUELU_ETI_GROUP(Scalar, LO, GO, Node)                                                          \
  TEUCHOS_UNIT_TEST_TEMPLATE_4_INSTANT(RebalanceAcFactory, Constructor, Scalar, LO, GO, Node)          \
  TEUCHOS_UNIT_TEST_TEMPLATE_4_INSTANT(RebalanceAcFactory, BuildWithoutImporter, Scalar, LO, GO, Node) \
  TEUCHOS_UNIT_TEST_TEMPLATE_4_INSTANT(RebalanceAcFactory, BuildWithImporter, Scalar, LO, GO, Node)

#include <MueLu_ETI_4arg.hpp>

}  // namespace MueLuTests
