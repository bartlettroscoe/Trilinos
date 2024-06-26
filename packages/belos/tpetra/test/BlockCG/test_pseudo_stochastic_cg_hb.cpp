// @HEADER
// *****************************************************************************
//                 Belos: Block Linear Solvers Package
//
// Copyright 2004-2016 NTESS and the Belos contributors.
// SPDX-License-Identifier: BSD-3-Clause
// *****************************************************************************
// @HEADER
//
// This driver reads a problem from a Harwell-Boeing (HB) file.
// The right-hand-side corresponds to a randomly generated solution.
// The initial guesses are all set to zero.
//
// NOTE: No preconditioner is used in this case.

#include "BelosConfigDefs.hpp"
#include "BelosLinearProblem.hpp"
#include "BelosTpetraAdapter.hpp"
#include "BelosPseudoBlockStochasticCGSolMgr.hpp"

#include <Tpetra_Core.hpp>
#include <Tpetra_CrsMatrix.hpp>
// I/O for Harwell-Boeing files
#define HIDE_TPETRA_INOUT_IMPLEMENTATIONS
#include <Tpetra_MatrixIO.hpp>

#include <Teuchos_CommandLineProcessor.hpp>
#include <Teuchos_ParameterList.hpp>
#include <Teuchos_StandardCatchMacros.hpp>

template <typename ScalarType>
int run (int argc, char *argv[])
{
  using ST = typename Tpetra::MultiVector<ScalarType>::scalar_type;
  using OP = typename Tpetra::Operator<ST>;
  using MV = typename Tpetra::MultiVector<ST>;
  
  using STS = typename Teuchos::ScalarTraits<ST>;
  using MT = typename STS::magnitudeType;
  
  using OPT = typename Belos::OperatorTraits<ST,MV,OP>;
  using MVT = typename Belos::MultiVecTraits<ST,MV>;

  using Teuchos::Comm;
  using Teuchos::ParameterList;
  using Teuchos::RCP;
  using Teuchos::rcp;
  using Teuchos::rcpFromRef;

  Tpetra::ScopeGuard tpetraScope (&argc, &argv);

  RCP<const Comm<int> > comm = Tpetra::getDefaultComm();
  const int MyPID = comm->getRank();

  bool success = false;
  bool verbose = false;

  try
  {
    // Get test parameters from command-line processor
    bool proc_verbose = false;
    bool debug = false;
    int frequency = -1;
    int numrhs = 1;
    int blocksize = 1;
    int maxiters = -1;
    std::string filename ("bcsstk14.hb");
    MT tol = 1.0e-5;

    Teuchos::CommandLineProcessor cmdp (false, true);
    cmdp.setOption ("verbose", "quiet", &verbose, "Print messages and "
                    "results.");
    cmdp.setOption ("debug", "nodebug", &debug, "Run debugging checks.");
    cmdp.setOption ("frequency", &frequency, "Solver's frequency for printing "
                    "residuals (#iters).  -1 means \"never\"; 1 means \"every "
                    "iteration.\"");
    cmdp.setOption ("tol", &tol, "Relative residual tolerance used by solver.");
    cmdp.setOption ("filename", &filename, "Filename for Harwell-Boeing test "
                    "matrix.");
    cmdp.setOption ("num-rhs", &numrhs, "Number of right-hand sides for which "
                    "to solve.");
    cmdp.setOption ("max-iters", &maxiters, "Maximum number of iterations per "
                    "linear system (-1 := adapted to problem/block size).");
    cmdp.setOption ("block-size", &blocksize, "Block size to be used by the "
                    "solver.");
    if (cmdp.parse (argc, argv) != Teuchos::CommandLineProcessor::PARSE_SUCCESSFUL) {
      return -1;
    }
    if (debug) {
      verbose = true;
    }
    if (!verbose) {
      frequency = -1;  // reset frequency if test is not verbose
    }

    proc_verbose = verbose && MyPID == 0;
    if (proc_verbose) {
      std::cout << Belos::Belos_Version () << std::endl << std::endl;
    }

    // Get the data from the HB file and build the Map,Matrix
    RCP<Tpetra::CrsMatrix<ST> > A;
    Tpetra::Utils::readHBMatrix (filename, comm, A);
    RCP<const Tpetra::Map<> > map = A->getDomainMap ();

    // Create initial vectors
    RCP<MV> B, X;
    X = rcp (new MV (map, numrhs));
    MVT::MvRandom (*X);
    B = rcp (new MV (map, numrhs));
    OPT::Apply (*A, *X, *B);
    MVT::MvInit (*X, STS::zero ());

    // Other information used by block solver (can be user specified)
    const int NumGlobalElements = B->getGlobalLength();
    if (maxiters == -1) {
      maxiters = NumGlobalElements/blocksize - 1; // maximum number of iterations to run
    }

    ParameterList belosList;
    belosList.set( "Block Size", blocksize );              // Blocksize to be used by iterative solver
    belosList.set( "Maximum Iterations", maxiters );       // Maximum number of iterations allowed
    belosList.set( "Convergence Tolerance", tol );         // Relative convergence tolerance requested
    int verbLevel = Belos::Errors + Belos::Warnings;
    if (debug) {
      verbLevel += Belos::Debug;
    }
    if (verbose) {
      verbLevel += Belos::TimingDetails + Belos::FinalSummary + Belos::StatusTestDetails;
    }
    belosList.set( "Verbosity", verbLevel );
    if (verbose) {
      if (frequency > 0) {
        belosList.set( "Output Frequency", frequency );
      }
    }
    
    // Construct an unpreconditioned linear problem instance.
    Belos::LinearProblem<ST,MV,OP> problem( A, X, B );
    bool set = problem.setProblem ();
    if (! set) {
      if (proc_verbose) {
        std::cout << std::endl << "ERROR:  Belos::LinearProblem failed to set up correctly!" << std::endl;
      }
      return EXIT_FAILURE;
    }

    // Start the block CG iteration
    using solver_type = typename Belos::PseudoBlockStochasticCGSolMgr<ST,MV,OP>;
    solver_type solver (rcpFromRef (problem), rcpFromRef (belosList));

    // Print out information about problem
    if (proc_verbose) {
      std::cout << std::endl << std::endl;
      std::cout << "Dimension of matrix: " << NumGlobalElements << std::endl;
      std::cout << "Number of right-hand sides: " << numrhs << std::endl;
      std::cout << "Block size used by solver: " << blocksize << std::endl;
      std::cout << "Max number of CG iterations: " << maxiters << std::endl;
      std::cout << "Relative residual tolerance: " << tol << std::endl;
      std::cout << std::endl;
    }
    
    // Perform solve
    Belos::ReturnType ret = solver.solve();
    
    // Compute actual residuals.
    bool badRes = false;
    std::vector<MT> actual_resids (numrhs);
    std::vector<MT> rhs_norm (numrhs);
    MV resid (map, numrhs);
    OPT::Apply (*A, *X, resid);
    MVT::MvAddMv (-STS::one (), resid, STS::one (), *B, resid);
    MVT::MvNorm (resid, actual_resids);
    MVT::MvNorm (*B, rhs_norm);
    if (proc_verbose) {
      std::cout << "---------- Actual Residuals (normalized) ----------" << std::endl << std::endl;
    }
    for (int i=0; i<numrhs; i++) {
      MT actRes = actual_resids[i]/rhs_norm[i];
      if (proc_verbose) {
        std::cout << "Problem " << i << " : \t" << actRes << std::endl;
      }
      if (actRes > tol) badRes = true;
    }

    success = ret == Belos::Converged && ! badRes;
    if (success) {
      if (proc_verbose) {
        std::cout << "\nEnd Result: TEST PASSED" << std::endl;
      }
    } else {
      if (proc_verbose) {
        std::cout << "\nEnd Result: TEST FAILED" << std::endl;
      }
    }
  }
  TEUCHOS_STANDARD_CATCH_STATEMENTS(verbose, std::cerr, success);

  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}

int main(int argc, char *argv[]) {
  return run<double>(argc,argv);

  // wrapped with a check: CMake option Trilinos_ENABLE_FLOAT=ON
  // return run<float>(argc,argv);
}


