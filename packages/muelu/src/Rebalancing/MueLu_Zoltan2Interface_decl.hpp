// @HEADER
//
// ***********************************************************************
//
//        MueLu: A package for multigrid based preconditioning
//                  Copyright 2012 Sandia Corporation
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
// Questions? Contact
//                    Jonathan Hu       (jhu@sandia.gov)
//                    Andrey Prokopenko (aprokop@sandia.gov)
//                    Ray Tuminaro      (rstumin@sandia.gov)
//
// ***********************************************************************
//
// @HEADER
#ifndef MUELU_ZOLTAN2INTERFACE_DECL_HPP
#define MUELU_ZOLTAN2INTERFACE_DECL_HPP

#include "MueLu_ConfigDefs.hpp"

#if defined(HAVE_MUELU_ZOLTAN2) && defined(HAVE_MPI)

#include <Xpetra_Matrix.hpp>
#include <Xpetra_MultiVectorFactory.hpp>
#include <Xpetra_VectorFactory.hpp>

#include "MueLu_SingleLevelFactoryBase.hpp"
#include "MueLu_Zoltan2Interface_fwd.hpp"

#include "MueLu_Level_fwd.hpp"
#include "MueLu_FactoryBase_fwd.hpp"
#include "MueLu_Utilities_fwd.hpp"

#if defined(HAVE_MUELU_ZOLTAN)
#include "MueLu_ZoltanInterface.hpp"
#endif

namespace MueLu {


  /*!
    @class Zoltan2Interface
    @brief Interface to Zoltan2 library.
    @ingroup Rebalancing

    This interface provides access to partitioning methods in Zoltan2.
    Currently, it supports RCB and multijagged as well as all graph partitioning algorithms from Zoltan2.

    ## Input/output of Zoltan2Interface ##

    ### User parameters of Zoltan2Interface ###
    Parameter | type | default | master.xml | validated | requested | description
    ----------|------|---------|:----------:|:---------:|:---------:|------------
    | A                                      | Factory | null  |   | * | * | Generating factory of the matrix A used during the prolongator smoothing process |
    | Coordinates                            | Factory | null  |   | * | (*) | Factory generating coordinates vector used for rebalancing. The coordinates are only needed when the chosen algorithm is 'multijagged' or 'rcb'.
    | ParameterList                          | ParamterList | null |  | * |  | Zoltan2 parameters
    | number of partitions                   | GO      | - |  |  |  | Short-cut parameter set by RepartitionFactory. Avoid repartitioning algorithms if only one partition is necessary (see details below)

    The * in the @c master.xml column denotes that the parameter is defined in the @c master.xml file.<br>
    The * in the @c validated column means that the parameter is declared in the list of valid input parameters (see Zoltan2Interface::GetValidParameters).<br>
    The * in the @c requested column states that the data is requested as input with all dependencies (see Zoltan2Interface::DeclareInput).

    ### Variables provided by Zoltan2Interface ###

    After Zoltan2Interface::Build the following data is available (if requested)

    Parameter | generated by | description
    ----------|--------------|------------
    | Partition       | Zoltan2Interface   | GOVector based on the Row map of A (DOF-based) containing the process id the DOF should be living in after rebalancing/repartitioning

    The "Partition" vector is used as input for the RepartitionFactory class.
    If Re-partitioning/rebalancing is necessary it uses the "Partition" variable to create the corresponding Xpetra::Import object which then is used
    by the RebalanceFactory classes (e.g., RebalanceAcFactory, RebalanceTransferFactory,...) to rebalance the coarse level operators.

    The RepartitionHeuristicFactory calculates how many partitions are to be built when performing rebalancing.
    It stores the result in the "number of partitions" variable on the current level (type = GO).
    If it is "number of partitions=1" we skip the Zoltan2 call and just create an dummy "Partition" vector containing zeros only.
    If no repartitioning is necessary (i.e., just keep the current partitioning) we return "Partition = Teuchos::null".
    If "number of partitions" > 1, the algorithm tries to find the requested number of partitions.

  */

  //FIXME: this class should not be templated
  template <class Scalar,
            class LocalOrdinal = typename Xpetra::Matrix<Scalar>::local_ordinal_type,
            class GlobalOrdinal = typename Xpetra::Matrix<Scalar, LocalOrdinal>::global_ordinal_type,
            class Node = typename Xpetra::Matrix<Scalar, LocalOrdinal, GlobalOrdinal>::node_type>
  class Zoltan2Interface : public SingleLevelFactoryBase {
#undef MUELU_ZOLTAN2INTERFACE_SHORT
#include "MueLu_UseShortNames.hpp"

  public:

    //! @name Constructors/Destructors
    //@{

    //! Constructor
    Zoltan2Interface();

    //! Destructor
    virtual ~Zoltan2Interface() { }
    //@}

    RCP<const ParameterList> GetValidParameterList() const;

    //! @name Input
    //@{
    void DeclareInput(Level& currentLevel) const;
    //@}

    //! @name Build methods.
    //@{
    void Build(Level& currentLevel) const;

    //@}

  private:
    RCP<ParameterList> defaultZoltan2Params;

  };  //class Zoltan2Interface

#ifdef HAVE_MUELU_EPETRA

#if ((defined(EPETRA_HAVE_OMP) && (!defined(HAVE_TPETRA_INST_OPENMP) || !defined(HAVE_TPETRA_INST_INT_INT))) || \
    (!defined(EPETRA_HAVE_OMP) && (!defined(HAVE_TPETRA_INST_SERIAL) || !defined(HAVE_TPETRA_INST_INT_INT))))

#if defined(HAVE_MUELU_ZOLTAN)
  // Stub partial specialization of Zoltan2Interface for EpetraNode
  // Use ZoltanInterface instead of Zoltan2Interface
  template<>
  class Zoltan2Interface<double,int,int,Xpetra::EpetraNode> : public SingleLevelFactoryBase {
    typedef double              Scalar;
    typedef int                 LocalOrdinal;
    typedef int                 GlobalOrdinal;
    typedef Xpetra::EpetraNode  Node;
#undef MUELU_ZOLTAN2INTERFACE_SHORT
#include "MueLu_UseShortNames.hpp"
  public:
    typedef typename Teuchos::ScalarTraits<Scalar>::magnitudeType real_type;
    typedef Xpetra::MultiVector<real_type,LO,GO,NO> RealValuedMultiVector;

    Zoltan2Interface() {
      level_           = rcp(new Level());
      zoltanInterface_ = rcp(new ZoltanInterface());

      level_->SetLevelID(1);
    }
    virtual ~Zoltan2Interface() {
      zoltanInterface_ = Teuchos::null;
      level_           = Teuchos::null;
    }
    RCP<const ParameterList> GetValidParameterList() const {
      RCP<ParameterList> validParamList = rcp(new ParameterList());

      validParamList->set< RCP<const FactoryBase> >   ("A",             Teuchos::null, "Factory of the matrix A");
      validParamList->set< RCP<const FactoryBase> >   ("Coordinates",   Teuchos::null, "Factory of the coordinates");
      validParamList->set< RCP<const FactoryBase> >   ("number of partitions", Teuchos::null, "Instance of RepartitionHeuristicFactory.");
      validParamList->set< RCP<const ParameterList> > ("ParameterList", Teuchos::null, "Zoltan2 parameters");

      return validParamList;
    };
    void DeclareInput(Level& currentLevel) const {
      Input(currentLevel, "A");
      Input(currentLevel, "number of partitions");
      const ParameterList& pL = GetParameterList();
      // We do this dance, because we don't want "ParameterList" to be marked as used.
      // Is there a better way?
      Teuchos::ParameterEntry entry = pL.getEntry("ParameterList");
      RCP<const Teuchos::ParameterList> providedList = Teuchos::any_cast<RCP<const Teuchos::ParameterList> >(entry.getAny(false));
      if (providedList != Teuchos::null && providedList->isType<std::string>("algorithm")) {
        const std::string algo = providedList->get<std::string>("algorithm");
        if (algo == "multijagged" || algo == "rcb")
          Input(currentLevel, "Coordinates");
      } else
        Input(currentLevel, "Coordinates");
    };
    void Build(Level& currentLevel) const {
      this->GetOStream(Warnings0) << "Tpetra does not support <double,int,int,EpetraNode> instantiation, "
          "switching Zoltan2Interface to ZoltanInterface" << std::endl;

      // Put the data into a fake level
      level_->Set("A",                    Get<RCP<Matrix> >      (currentLevel, "A"));
      level_->Set("Coordinates",          Get<RCP<RealValuedMultiVector> >(currentLevel, "Coordinates"));
      level_->Set("number of partitions", currentLevel.Get<GO>("number of partitions"));

      level_->Request("Partition", zoltanInterface_.get());
      zoltanInterface_->Build(*level_);

      RCP<Xpetra::Vector<GO,LO,GO,NO> > decomposition;
      level_->Get("Partition", decomposition, zoltanInterface_.get());
      Set(currentLevel, "Partition", decomposition);
    };

  private:
    RCP<Level>           level_;            // fake Level
    RCP<ZoltanInterface> zoltanInterface_;
  };
#else
  // Stub partial specialization of Zoltan2Interface for EpetraNode
  template<>
  class Zoltan2Interface<double,int,int,Xpetra::EpetraNode> : public SingleLevelFactoryBase {
  public:
    Zoltan2Interface() { throw Exceptions::RuntimeError("Tpetra does not support <double,int,int,EpetraNode> instantiation"); }
    virtual ~Zoltan2Interface() { }
    RCP<const ParameterList> GetValidParameterList() const { return Teuchos::null; };
    void DeclareInput(Level& level) const {};
    void Build(Level &level) const {};
  };
#endif // HAVE_MUELU_ZOLTAN

#endif

#endif // HAVE_MUELU_EPETRA

} //namespace MueLu

#define MUELU_ZOLTAN2INTERFACE_SHORT
#endif //if defined(HAVE_MUELU_ZOLTAN2) && defined(HAVE_MPI)

#endif // MUELU_ZOLTAN2INTERFACE_DECL_HPP
