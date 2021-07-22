/*
// @HEADER
// ************************************************************************
//
//        Piro: Strategy package for embedded analysis capabilitites
//                  Copyright (2010) Sandia Corporation
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
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
// Questions? Contact Andy Salinger (agsalin@sandia.gov), Sandia
// National Laboratories.
//
// ************************************************************************
// @HEADER
*/

#include "Piro_ConfigDefs.hpp"

#ifdef HAVE_PIRO_TEMPUS
#include "Piro_TempusSolver.hpp"
#include "Tempus_StepperFactory.hpp"
#include "../../tempus/test/TestUtils/Tempus_ConvergenceTestUtils.hpp"

#include "Piro_Test_ThyraSupport.hpp"
#include "Piro_TempusIntegrator.hpp"

#include "SinCosModel.hpp"
#include "Piro_Test_MockObserver.hpp"

#include "Teuchos_UnitTestHarness.hpp"

#include "Teuchos_Ptr.hpp"
#include "Teuchos_Array.hpp"
#include "Teuchos_Tuple.hpp"
#include "Piro_Helpers.hpp" 

#include <stdexcept>
#include<mpi.h>

using namespace Teuchos;
using namespace Piro;
using namespace Piro::Test;


namespace Thyra {
  typedef ModelEvaluatorBase MEB;
} // namespace Thyra

// Setup support

// Floating point tolerance
const double tol = 1.0e-8;

void test_sincos_asa(Teuchos::FancyOStream &out, bool &success)
{
  const std::string sens_method_string = "Adjoint";
  std::string soln_outfile_name; 
  std::string dgdp_outfile_name; 
  std::string errfile_name; 

  soln_outfile_name = "Tempus_BackwardEuler_SinCos_Sens_ASA_Soln.dat"; 
  errfile_name = "Tempus_BackwardEuler_SinCos_Sens_ASA_Error.dat";

  const RCP<MockObserver<double> > observer(new MockObserver<double>);
  std::vector<double> StepSize;
  std::vector<double> ErrorNorm;
  const int nTimeStepSizes = 7;
  //const int nTimeStepSizes = 12;
  double dt = 0.2;
  double order = 0.0;
  Teuchos::RCP<const Teuchos::Comm<int> > comm =
    Teuchos::DefaultComm<int>::getComm();
  Teuchos::RCP<Teuchos::FancyOStream> my_out =
    Teuchos::fancyOStream(Teuchos::rcpFromRef(std::cout));
  my_out->setProcRankAndSize(comm->getRank(), comm->getSize());
  my_out->setOutputToRootOnly(0);

  SENS_METHOD sens_method;
  if (sens_method_string == "None") sens_method = Piro::NONE;
  else if (sens_method_string == "Forward") sens_method = Piro::FORWARD;
  else if (sens_method_string == "Adjoint") sens_method = Piro::ADJOINT;

  for (int n=0; n<nTimeStepSizes; n++) {

    // Read params from .xml file
    RCP<ParameterList> pList =
      getParametersFromXmlFile("input_Tempus_BackwardEuler_SinCos.xml");

    // Setup the SinCosModel
    RCP<ParameterList> scm_pl = sublist(pList, "SinCosModel", true);
    RCP<SinCosModel> model = Teuchos::rcp(new SinCosModel(scm_pl));

    dt /= 2;

     // Set up Tempus PL
    RCP<ParameterList> tempus_pl = sublist(pList, "Tempus", true);
    
    //Set up sensitivity PL
    // IKT 7/2/2021: TODO - ask Eric Phipps about these options, 
    // as w/o them I got runtime errors.
    ParameterList& sens_pl = tempus_pl->sublist("Sensitivities");
    sens_pl.set("Mass Matrix Is Identity", false); // Just for testing
    ParameterList& interp_pl =
      tempus_pl->sublist("Default Integrator").sublist("Solution History").sublist("Interpolator");
    interp_pl.set("Interpolator Type", "Lagrange");
    interp_pl.set("Order", 0);

    // Set FSAL to false, because it is not currently setup for ASA.
    tempus_pl->sublist("Default Stepper").set("Use FSAL", false);

    // Set IC consistency check to false, because it is not currently
    // setup for ASA.
    tempus_pl->sublist("Default Stepper")
           .set("Initial Condition Consistency Check", false);

    // Setup the Integrator and reset initial time step
    tempus_pl->sublist("Default Integrator")
       .sublist("Time Step Control").set("Initial Time Step", dt);
    
    Teuchos::RCP<Piro::TempusIntegrator<double> > integrator 
        = Teuchos::rcp(new Piro::TempusIntegrator<double>(tempus_pl, model, sens_method));
    order = integrator->getStepper()->getOrder();

    // Initial Conditions
    double t0 = tempus_pl->sublist("Default Integrator")
      .sublist("Time Step Control").get<double>("Initial Time");
    double tfinal = tempus_pl->sublist("Default Integrator")
      .sublist("Time Step Control").get<double>("Final Time");
    RCP<const Thyra::VectorBase<double> > x0 =
      model->getExactSolution(t0).get_x();
    const int num_param = model->get_p_space(0)->dim();
    RCP<Thyra::MultiVectorBase<double> > DxDp0 =
      Thyra::createMembers(model->get_x_space(), num_param);
    for (int i=0; i<num_param; ++i)
      Thyra::assign(DxDp0->col(i).ptr(),
                    *(model->getExactSensSolution(i, t0).get_x()));
    integrator->initializeSolutionHistory(t0, x0, Teuchos::null, Teuchos::null,
                                DxDp0, Teuchos::null, Teuchos::null);

    integrator->initializeSolutionHistory(t0, x0, Teuchos::null, Teuchos::null,
                                DxDp0, Teuchos::null, Teuchos::null);
    const RCP<const Tempus::SolutionHistory<double> > solutionHistory = integrator->getSolutionHistory();
    const RCP<const Tempus::TimeStepControl<double> > timeStepControl = integrator->getTimeStepControl();
    const Teuchos::RCP<Tempus::IntegratorObserver<double> > tempusObserver 
          = Teuchos::rcp(new ObserverToTempusIntegrationObserverAdapter<double>(integrator, solutionHistory, timeStepControl, observer, 
				                                                false, false, sens_method));
    integrator->setObserver(tempusObserver);
   
    const RCP<Thyra::NonlinearSolverBase<double> > stepSolver = Teuchos::null;

    RCP<ParameterList> stepper_pl = Teuchos::rcp(&(tempus_pl->sublist("Default Stepper")), false);

    RCP<Tempus::StepperFactory<double> > sf = Teuchos::rcp(new Tempus::StepperFactory<double>());
    const RCP<Tempus::Stepper<double> > stepper = sf->createStepper(stepper_pl, model);
    const RCP<TempusSolver<double> > tempus_solver = 
         rcp(new TempusSolver<double>(integrator, stepper, stepSolver, model, tfinal, sens_method_string));
    
    const Thyra::MEB::InArgs<double> inArgs = tempus_solver->getNominalValues();
    Thyra::MEB::OutArgs<double> outArgs = tempus_solver->createOutArgs();
    const int solutionResponseIndex = tempus_solver->Ng() - 1;
    const int parameterIndex = 0;
    const Thyra::MEB::Derivative<double> dxdp_deriv =
        Thyra::create_DgDp_mv(*tempus_solver, solutionResponseIndex, parameterIndex, Thyra::MEB::DERIV_MV_JACOBIAN_FORM);
    const RCP<Thyra::MultiVectorBase<double> > dxdp = dxdp_deriv.getMultiVector();
    outArgs.set_DgDp(solutionResponseIndex, parameterIndex, dxdp_deriv);

    //Integrate in time 
    tempus_solver->evalModel(inArgs, outArgs);

    // Test if at 'Final Time'
    double time = integrator->getTime();
    double timeFinal = tempus_pl->sublist("Default Integrator")
       .sublist("Time Step Control").get<double>("Final Time");
    TEST_FLOATING_EQUALITY(time, timeFinal, 1.0e-14);

    // Time-integrated solution and the exact solution along with
    // sensitivities (relying on response g(x) = x).  Note we must transpose
    // dg/dp since the integrator returns it in gradient form.
    RCP<const Thyra::VectorBase<double> > x = integrator->getX();
    RCP<const Thyra::MultiVectorBase<double> > DgDp = integrator->getDgDp();
    RCP<Thyra::MultiVectorBase<double> > DxDp =
      Thyra::createMembers(model->get_x_space(), num_param);
    {
      dgdp_outfile_name = "Tempus_BackwardEuler_SinCos_Sens_ASA_DgDp_" + std::to_string(n) + ".dat"; 
      std::ofstream ftmp(dgdp_outfile_name);
      Thyra::ConstDetachedMultiVectorView<double> dgdp_view(*DgDp);
      Thyra::DetachedMultiVectorView<double> dxdp_view(*DxDp);
      const int num_g = DgDp->domain()->dim();
      ftmp << std::fixed << std::setprecision(7) << time; 
      for (int i=0; i<num_g; ++i) {
        for (int j=0; j<num_param; ++j) {
          dxdp_view(i,j) = dgdp_view(j,i);
          if (comm->getRank() == 0) {
            ftmp << std::setw(11) << dgdp_view(j,i); 
	  }
	}
      }
      if (comm->getRank() == 0) {
        ftmp << "\n";
        ftmp.close();
      }
    }
    RCP<const Thyra::VectorBase<double> > x_exact =
      model->getExactSolution(time).get_x();
    RCP<Thyra::MultiVectorBase<double> > DxDp_exact =
      Thyra::createMembers(model->get_x_space(), num_param);
    for (int i=0; i<num_param; ++i) {
      Thyra::assign(DxDp_exact->col(i).ptr(),
                    *(model->getExactSensSolution(i, time).get_x()));
    }

    // Plot sample solution, exact solution, and adjoint solution
    if (comm->getRank() == 0 && n == nTimeStepSizes-1) {
      typedef Thyra::DefaultProductVector<double> DPV;
      typedef Thyra::DefaultMultiVectorProductVector<double> DMVPV;

      std::ofstream ftmp(soln_outfile_name);
      RCP<const Tempus::SolutionHistory<double> > solutionHistory =
        integrator->getSolutionHistory();
      for (int i=0; i<solutionHistory->getNumStates(); i++) {
        RCP<const Tempus::SolutionState<double> > solutionState = (*solutionHistory)[i];
        const double time_i = solutionState->getTime();
        RCP<const DPV> x_prod_plot =
          Teuchos::rcp_dynamic_cast<const DPV>(solutionState->getX());
        RCP<const Thyra::VectorBase<double> > x_plot =
          x_prod_plot->getVectorBlock(0);
        RCP<const DMVPV > adjoint_prod_plot =
          Teuchos::rcp_dynamic_cast<const DMVPV>(x_prod_plot->getVectorBlock(1));
        RCP<const Thyra::MultiVectorBase<double> > adjoint_plot =
          adjoint_prod_plot->getMultiVector();
        RCP<const Thyra::VectorBase<double> > x_exact_plot =
          model->getExactSolution(time_i).get_x();
        ftmp << std::fixed << std::setprecision(7)
             << time_i
             << std::setw(11) << get_ele(*(x_plot), 0)
             << std::setw(11) << get_ele(*(x_plot), 1)
             << std::setw(11) << get_ele(*(adjoint_plot->col(0)), 0)
             << std::setw(11) << get_ele(*(adjoint_plot->col(0)), 1)
             << std::setw(11) << get_ele(*(adjoint_plot->col(1)), 0)
             << std::setw(11) << get_ele(*(adjoint_plot->col(1)), 1)
             << std::setw(11) << get_ele(*(x_exact_plot), 0)
             << std::setw(11) << get_ele(*(x_exact_plot), 1)
             << std::endl;
      }
      ftmp.close();
    }

    //Compare solution from observer and x to verify observer routines 
    const RCP<const Thyra::VectorBase<double> > solution =
      observer->lastSolution();
    TEST_COMPARE_FLOATING_ARRAYS(
      arrayFromVector(*solution),
      arrayFromVector(*x),
      tol);

    //IKT, 7/4/2021: we'll want to add some capability to observe DgDp eventually, maybe,
    //in which case we'd want too add logic similar to the following.
    /*//Compare solution_dxdp from observer and DxDp to verify observer routines 
    const RCP<const Thyra::MultiVectorBase<double> > solution_dxdp =
      observer->lastSolution_dxdp();
    for (int np = 0; np < DxDp->domain()->dim(); np++) { 
      Teuchos::RCP<const Thyra::VectorBase<double>> DxDp_vec = DxDp->col(np);
      Teuchos::RCP<const Thyra::VectorBase<double>> solution_dxdp_vec = solution_dxdp->col(np);
      TEST_COMPARE_FLOATING_ARRAYS(
        arrayFromVector(*solution_dxdp_vec),
        arrayFromVector(*DxDp_vec),
        tol);
    }*/

    // Calculate the error
    RCP<Thyra::VectorBase<double> > xdiff = x->clone_v();
    RCP<Thyra::MultiVectorBase<double> > DxDpdiff = DxDp->clone_mv();
    Thyra::V_StVpStV(xdiff.ptr(), 1.0, *x_exact, -1.0, *(x));
    Thyra::V_VmV(DxDpdiff.ptr(), *DxDp_exact, *DxDp);
    StepSize.push_back(dt);
    double L2norm = Thyra::norm_2(*xdiff);
    L2norm *= L2norm;
    Teuchos::Array<double> L2norm_DxDp(num_param);
    Thyra::norms_2(*DxDpdiff, L2norm_DxDp());
    for (int i=0; i<num_param; ++i)
      L2norm += L2norm_DxDp[i]*L2norm_DxDp[i];
    L2norm = std::sqrt(L2norm);
    ErrorNorm.push_back(L2norm);

    //*my_out << " n = " << n << " dt = " << dt << " error = " << L2norm << "\n";

  }
  // Check the order and intercept
  double slope = Tempus_Test::computeLinearRegressionLogLog<double>(StepSize, ErrorNorm);
  *my_out << "  Stepper = BackwardEuler" << std::endl;
  *my_out << "  =========================" << std::endl;
  *my_out << "  Expected order: " << order << std::endl;
  *my_out << "  Observed order: " << slope << std::endl;
  *my_out << "  =========================" << std::endl;
  TEST_FLOATING_EQUALITY( slope, order, 0.015 );
  TEST_FLOATING_EQUALITY( ErrorNorm[0], 0.151746, 1.0e-4 );

  if (comm->getRank() == 0) {
    std::ofstream ftmp(errfile_name);
    double error0 = 0.8*ErrorNorm[0];
    for (int n=0; n<nTimeStepSizes; n++) {
      ftmp << StepSize[n]  << "   " << ErrorNorm[n] << "   "
           << error0*(StepSize[n]/StepSize[0]) << std::endl;
    }
    ftmp.close();
  }

}

TEUCHOS_UNIT_TEST(Piro_TempusSolver, SinCos_AdjointSensitivities)
{
  test_sincos_asa(out, success);
}

#endif /* HAVE_PIRO_TEMPUS */
