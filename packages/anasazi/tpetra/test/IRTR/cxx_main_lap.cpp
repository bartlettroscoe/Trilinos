// @HEADER
// *****************************************************************************
//                 Anasazi: Block Eigensolvers Package
//
// Copyright 2004 NTESS and the Anasazi contributors.
// SPDX-License-Identifier: BSD-3-Clause
// *****************************************************************************
// @HEADER
//
// This test is for SIRTR/IRTR solving a standard (Ax=xl) Hermitian
// eigenvalue problem where the operator (A) is the 1D finite-differenced Laplacian
// operator.

#include "AnasaziConfigDefs.hpp"
#include "AnasaziTypes.hpp"

#include "AnasaziTpetraAdapter.hpp"
#include "AnasaziBasicEigenproblem.hpp"
#include "AnasaziRTRSolMgr.hpp"
#include <Teuchos_CommandLineProcessor.hpp>

#include <Tpetra_Core.hpp>
#include <Tpetra_CrsMatrix.hpp>

using namespace Teuchos;
using Tpetra::Operator;
using Tpetra::CrsMatrix;
using Tpetra::MultiVector;
using Tpetra::Map;

int main(int argc, char *argv[])
{
  using std::cout;
  using std::endl;

  typedef double                              ST;
  typedef ScalarTraits<ST>                   SCT;
  typedef SCT::magnitudeType                  MT;
  typedef MultiVector<ST>                     MV;
  typedef MV::global_ordinal_type             GO;
  typedef Operator<ST>                        OP;
  typedef Anasazi::MultiVecTraits<ST,MV>     MVT;
  typedef Anasazi::OperatorTraits<ST,MV,OP>  OPT;
  const ST ONE  = SCT::one();

  Tpetra::ScopeGuard tpetraScope (&argc, &argv);

  int MyPID = 0;
  int NumImages = 1;

  RCP<const Teuchos::Comm<int> > comm = Tpetra::getDefaultComm ();

  MyPID = rank(*comm);
  NumImages = size(*comm);

  bool testFailed;
  bool verbose = false;
  bool debug = false;
  bool skinny = true;
  std::string which("LR");
  int nev = 4;
  int blockSize = 4;
  MT tol = 1.0e-6;

  CommandLineProcessor cmdp(false,true);
  cmdp.setOption("verbose","quiet",&verbose,"Print messages and results.");
  cmdp.setOption("skinny","hefty",&skinny,"Use a skinny (low-mem) or hefty (higher-mem) implementation of IRTR.");
  cmdp.setOption("debug","nodebug",&debug,"Print debugging information.");
  cmdp.setOption("sort",&which,"Targetted eigenvalues (SR or LR).");
  cmdp.setOption("nev",&nev,"Number of eigenvalues to compute.");
  cmdp.setOption("blockSize",&blockSize,"Block size for the algorithm.");
  cmdp.setOption("tol",&tol,"Tolerance for convergence.");
  if (cmdp.parse(argc,argv) != CommandLineProcessor::PARSE_SUCCESSFUL) {
    return -1;
  }
  if (debug) verbose = true;
  if (blockSize < nev) {
    blockSize = nev;
  }

  if (MyPID == 0) {
    cout << Anasazi::Anasazi_Version() << endl << endl;
  }

  // -- Set finite difference grid
  const int ROWS_PER_PROC = 10;
  int dim = ROWS_PER_PROC * NumImages;

  // create map
  RCP<const Map<> > map = rcp (new Map<> (dim, 0, comm));
  RCP<CrsMatrix<ST> > K = rcp(new CrsMatrix<ST>(map,4));
  int base = MyPID*ROWS_PER_PROC;
  if (MyPID != NumImages-1) {
    for (int i=0; i<ROWS_PER_PROC; ++i) {
      K->insertGlobalValues(static_cast<GO>(base+i  ), tuple<GO>(base+i  ), tuple<ST>( 2));
      K->insertGlobalValues(static_cast<GO>(base+i  ), tuple<GO>(base+i+1), tuple<ST>(-1));
      K->insertGlobalValues(static_cast<GO>(base+i+1), tuple<GO>(base+i  ), tuple<ST>(-1));
      K->insertGlobalValues(static_cast<GO>(base+i+1), tuple<GO>(base+i+1), tuple<ST>( 2));
    }
  }
  else {
    for (int i=0; i<ROWS_PER_PROC-1; ++i) {
      K->insertGlobalValues(static_cast<GO>(base+i  ), tuple<GO>(base+i  ), tuple<ST>( 2));
      K->insertGlobalValues(static_cast<GO>(base+i  ), tuple<GO>(base+i+1), tuple<ST>(-1));
      K->insertGlobalValues(static_cast<GO>(base+i+1), tuple<GO>(base+i  ), tuple<ST>(-1));
      K->insertGlobalValues(static_cast<GO>(base+i+1), tuple<GO>(base+i+1), tuple<ST>( 2));
    }
  }
  K->fillComplete();

  // Create initial vectors
  RCP<MV> ivec = rcp( new MV(map,blockSize) );
  ivec->randomize ();

  // Create eigenproblem
  RCP<Anasazi::BasicEigenproblem<ST,MV,OP> > problem =
    rcp( new Anasazi::BasicEigenproblem<ST,MV,OP>(K,ivec) );
  //
  // Inform the eigenproblem that the operator K is symmetric
  problem->setHermitian(true);
  //
  // Set the number of eigenvalues requested
  problem->setNEV( nev );
  //
  // Inform the eigenproblem that you are done passing it information
  bool boolret = problem->setProblem();
  if (boolret != true) {
    if (MyPID == 0) {
      cout << "Anasazi::BasicEigenproblem::SetProblem() returned with error." << endl
           << "End Result: TEST FAILED" << endl;
    }
    return -1;
  }

  // Set verbosity level
  int verbosity = Anasazi::Errors + Anasazi::Warnings + Anasazi::FinalSummary + Anasazi::TimingDetails;
  if (verbose) {
    verbosity += Anasazi::IterationDetails;
  }
  if (debug) {
    verbosity += Anasazi::Debug;
  }

  // Eigensolver parameters
  int maxIters = 450;
  //
  // Create parameter list to pass into the solver manager
  ParameterList MyPL;
  MyPL.set( "Skinny Solver", skinny);
  MyPL.set( "Verbosity", verbosity );
  MyPL.set( "Which", which );
  MyPL.set( "Block Size", blockSize );
  MyPL.set( "Maximum Iterations", maxIters );
  MyPL.set( "Convergence Tolerance", tol );
  //
  // Create the solver manager
  Anasazi::RTRSolMgr<ST,MV,OP> MySolverMgr(problem, MyPL);

  // Solve the problem to the specified tolerances or length
  Anasazi::ReturnType returnCode = MySolverMgr.solve();
  testFailed = false;
  if (returnCode != Anasazi::Converged) {
    testFailed = true;
  }

  // Get the eigenvalues and eigenvectors from the eigenproblem
  Anasazi::Eigensolution<ST,MV> sol = problem->getSolution();
  RCP<MV> evecs = sol.Evecs;
  int numev = sol.numVecs;

  if (numev > 0) {
    std::ostringstream os;
    os.setf(std::ios::scientific, std::ios::floatfield);
    os.precision(6);

    // Compute the direct residual
    std::vector<MT> normV( numev );
    SerialDenseMatrix<int,ST> T(numev,numev);
    for (int i=0; i<numev; i++) {
      T(i,i) = sol.Evals[i].realpart;
    }
    RCP<MV> Kvecs = MVT::Clone( *evecs, numev );

    OPT::Apply( *K, *evecs, *Kvecs );

    MVT::MvTimesMatAddMv( -ONE, *evecs, T, ONE, *Kvecs );
    MVT::MvNorm( *Kvecs, normV );

    os << "Direct residual norms computed in Tpetra_IRTR_lap_test.exe" << endl
       << std::setw(20) << "Eigenvalue" << std::setw(20) << "Residual  " << endl
       << "----------------------------------------" << endl;
    for (int i=0; i<numev; i++) {
      if ( SCT::magnitude(sol.Evals[i].realpart) != SCT::zero() ) {
        normV[i] = SCT::magnitude(normV[i]/sol.Evals[i].realpart);
      }
      os << std::setw(20) << sol.Evals[i].realpart << std::setw(20) << normV[i] << endl;
      if ( normV[i] > tol ) {
        testFailed = true;
      }
    }
    if (MyPID==0) {
      cout << endl << os.str() << endl;
    }
  }

  if (testFailed) {
    if (MyPID==0) {
      cout << "End Result: TEST FAILED" << endl;
    }
    return -1;
  }
  //
  // Default return value
  //
  if (MyPID==0) {
    cout << "End Result: TEST PASSED" << endl;
  }
  return 0;

}
