// ***********************************************************************
//
//          Tpetra: Templated Linear Algebra Services Package
//                 Copyright (2008) Sandia Corporation
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
// ************************************************************************
// @HEADER

#include "Tpetra_Distributor.hpp"
#include "Tpetra_Details_Behavior.hpp"
#include "Tpetra_Details_gathervPrint.hpp"
#include "Tpetra_Util.hpp"
#include "Tpetra_Details_makeValidVerboseStream.hpp"
#include "Teuchos_StandardParameterEntryValidators.hpp"
#include "Teuchos_VerboseObjectParameterListHelpers.hpp"
#include <numeric>

namespace Tpetra {
  // We set default values of Distributor's Boolean parameters here,
  // in this one place.  That way, if we want to change the default
  // value of a parameter, we don't have to search the whole file to
  // ensure a consistent setting.
  namespace {
    // Default value of the "Debug" parameter.
    const bool tpetraDistributorDebugDefault = false;
  } // namespace (anonymous)

  Teuchos::Array<std::string>
  distributorSendTypes ()
  {
    Teuchos::Array<std::string> sendTypes;
    sendTypes.push_back ("Isend");
    sendTypes.push_back ("Rsend");
    sendTypes.push_back ("Send");
    sendTypes.push_back ("Ssend");
    return sendTypes;
  }

  Distributor::
  Distributor (const Teuchos::RCP<const Teuchos::Comm<int> >& comm,
               const Teuchos::RCP<Teuchos::FancyOStream>& /* out */,
               const Teuchos::RCP<Teuchos::ParameterList>& plist)
    : plan_(comm)
    , lastRoundBytesSend_ (0)
    , lastRoundBytesRecv_ (0)
  {
    this->setParameterList(plist);
  }

  Distributor::
  Distributor (const Teuchos::RCP<const Teuchos::Comm<int> >& comm)
    : Distributor (comm, Teuchos::null, Teuchos::null)
  {}

  Distributor::
  Distributor (const Teuchos::RCP<const Teuchos::Comm<int> >& comm,
               const Teuchos::RCP<Teuchos::FancyOStream>& out)
    : Distributor (comm, out, Teuchos::null)
  {}

  Distributor::
  Distributor (const Teuchos::RCP<const Teuchos::Comm<int> >& comm,
               const Teuchos::RCP<Teuchos::ParameterList>& plist)
    : Distributor (comm, Teuchos::null, plist)
  {}

  Distributor::
  Distributor (const Distributor& distributor)
    : plan_(distributor.plan_)
    , actor_(distributor.actor_)
    , verbose_ (distributor.verbose_)
    , reverseDistributor_ (distributor.reverseDistributor_)
    , lastRoundBytesSend_ (distributor.lastRoundBytesSend_)
    , lastRoundBytesRecv_ (distributor.lastRoundBytesRecv_)
  {
    using Teuchos::ParameterList;
    using Teuchos::RCP;
    using Teuchos::rcp;

    RCP<const ParameterList> rhsList = distributor.getParameterList ();
    RCP<ParameterList> newList = rhsList.is_null () ? Teuchos::null :
      Teuchos::parameterList (*rhsList);
    this->setParameterList (newList);
  }

  void Distributor::swap (Distributor& rhs) {
    using Teuchos::ParameterList;
    using Teuchos::parameterList;
    using Teuchos::RCP;

    std::swap (plan_, rhs.plan_);
    std::swap (actor_, rhs.actor_);
    std::swap (verbose_, rhs.verbose_);
    std::swap (reverseDistributor_, rhs.reverseDistributor_);
    std::swap (lastRoundBytesSend_, rhs.lastRoundBytesSend_);
    std::swap (lastRoundBytesRecv_, rhs.lastRoundBytesRecv_);

    // Swap parameter lists.  If they are the same object, make a deep
    // copy first, so that modifying one won't modify the other one.
    RCP<ParameterList> lhsList = this->getNonconstParameterList ();
    RCP<ParameterList> rhsList = rhs.getNonconstParameterList ();
    if (lhsList.getRawPtr () == rhsList.getRawPtr () && ! rhsList.is_null ()) {
      rhsList = parameterList (*rhsList);
    }
    if (! rhsList.is_null ()) {
      this->setMyParamList (rhsList);
    }
    if (! lhsList.is_null ()) {
      rhs.setMyParamList (lhsList);
    }

    // We don't need to swap timers, because all instances of
    // Distributor use the same timers.
  }

  bool
  Distributor::getVerbose()
  {
    return Details::Behavior::verbose("Distributor") ||
      Details::Behavior::verbose("Tpetra::Distributor");
  }

  std::unique_ptr<std::string>
  Distributor::
  createPrefix(const char methodName[]) const
  {
    return Details::createPrefix(
      plan_.comm_.getRawPtr(), "Distributor", methodName);
  }

  void
  Distributor::
  setParameterList (const Teuchos::RCP<Teuchos::ParameterList>& plist)
  {
    using ::Tpetra::Details::Behavior;
    using Teuchos::FancyOStream;
    using Teuchos::getIntegralValue;
    using Teuchos::includesVerbLevel;
    using Teuchos::ParameterList;
    using Teuchos::parameterList;
    using Teuchos::RCP;
    using std::endl;

    if (! plist.is_null()) {
      RCP<const ParameterList> validParams = getValidParameters ();
      plist->validateParametersAndSetDefaults (*validParams);

      const bool barrierBetween =
        plist->get<bool> ("Barrier between receives and sends");
      const Details::EDistributorSendType sendType =
        getIntegralValue<Details::EDistributorSendType> (*plist, "Send type");
      const bool useDistinctTags = plist->get<bool> ("Use distinct tags");
      {
        // mfh 03 May 2016: We keep this option only for backwards
        // compatibility, but it must always be true.  See discussion of
        // Github Issue #227.
        const bool enable_cuda_rdma =
          plist->get<bool> ("Enable MPI CUDA RDMA support");
        TEUCHOS_TEST_FOR_EXCEPTION
          (! enable_cuda_rdma, std::invalid_argument, "Tpetra::Distributor::"
           "setParameterList: " << "You specified \"Enable MPI CUDA RDMA "
           "support\" = false.  This is no longer valid.  You don't need to "
           "specify this option any more; Tpetra assumes it is always true.  "
           "This is a very light assumption on the MPI implementation, and in "
           "fact does not actually involve hardware or system RDMA support.  "
           "Tpetra just assumes that the MPI implementation can tell whether a "
           "pointer points to host memory or CUDA device memory.");
      }

      // We check this property explicitly, since we haven't yet learned
      // how to make a validator that can cross-check properties.
      // Later, turn this into a validator so that it can be embedded in
      // the valid ParameterList and used in Optika.
      TEUCHOS_TEST_FOR_EXCEPTION
        (! barrierBetween && sendType == Details::DISTRIBUTOR_RSEND,
         std::invalid_argument, "Tpetra::Distributor::setParameterList: " << endl
         << "You specified \"Send type\"=\"Rsend\", but turned off the barrier "
         "between receives and sends." << endl << "This is invalid; you must "
         "include the barrier if you use ready sends." << endl << "Ready sends "
         "require that their corresponding receives have already been posted, "
         "and the only way to guarantee that in general is with a barrier.");

      // Now that we've validated the input list, save the results.
      plan_.sendType_ = sendType;
      plan_.barrierBetweenRecvSend_ = barrierBetween;
      plan_.useDistinctTags_ = useDistinctTags;

      // ParameterListAcceptor semantics require pointer identity of the
      // sublist passed to setParameterList(), so we save the pointer.
      this->setMyParamList (plist);
    }
  }

  Teuchos::RCP<const Teuchos::ParameterList>
  Distributor::getValidParameters () const
  {
    using Teuchos::Array;
    using Teuchos::ParameterList;
    using Teuchos::parameterList;
    using Teuchos::RCP;
    using Teuchos::setStringToIntegralParameter;

    const bool barrierBetween = Details::barrierBetween_default;
    const bool useDistinctTags = Details::useDistinctTags_default;
    const bool debug = tpetraDistributorDebugDefault;

    Array<std::string> sendTypes = distributorSendTypes ();
    const std::string defaultSendType ("Send");
    Array<Details::EDistributorSendType> sendTypeEnums;
    sendTypeEnums.push_back (Details::DISTRIBUTOR_ISEND);
    sendTypeEnums.push_back (Details::DISTRIBUTOR_RSEND);
    sendTypeEnums.push_back (Details::DISTRIBUTOR_SEND);
    sendTypeEnums.push_back (Details::DISTRIBUTOR_SSEND);

    RCP<ParameterList> plist = parameterList ("Tpetra::Distributor");
    plist->set ("Barrier between receives and sends", barrierBetween,
                "Whether to execute a barrier between receives and sends in do"
                "[Reverse]Posts().  Required for correctness when \"Send type\""
                "=\"Rsend\", otherwise correct but not recommended.");
    setStringToIntegralParameter<Details::EDistributorSendType> ("Send type",
      defaultSendType, "When using MPI, the variant of send to use in "
      "do[Reverse]Posts()", sendTypes(), sendTypeEnums(), plist.getRawPtr());
    plist->set ("Use distinct tags", useDistinctTags, "Whether to use distinct "
                "MPI message tags for different code paths.  Highly recommended"
                " to avoid message collisions.");
    plist->set ("Debug", debug, "Whether to print copious debugging output on "
                "all processes.");
    plist->set ("Timer Label","","Label for Time Monitor output");
    plist->set ("Enable MPI CUDA RDMA support", true, "Assume that MPI can "
                "tell whether a pointer points to host memory or CUDA device "
                "memory.  You don't need to specify this option any more; "
                "Tpetra assumes it is always true.  This is a very light "
                "assumption on the MPI implementation, and in fact does not "
                "actually involve hardware or system RDMA support.");

    // mfh 24 Dec 2015: Tpetra no longer inherits from
    // Teuchos::VerboseObject, so it doesn't need the "VerboseObject"
    // sublist.  However, we retain the "VerboseObject" sublist
    // anyway, for backwards compatibility (otherwise the above
    // validation would fail with an invalid parameter name, should
    // the user still want to provide this list).
    Teuchos::setupVerboseObjectSublist (&*plist);
    return Teuchos::rcp_const_cast<const ParameterList> (plist);
  }


  size_t Distributor::getTotalReceiveLength() const
  { return plan_.getTotalReceiveLength(); }

  size_t Distributor::getNumReceives() const
  { return plan_.getNumReceives(); }

  bool Distributor::hasSelfMessage() const
  { return plan_.hasSelfMessage(); }

  size_t Distributor::getNumSends() const
  { return plan_.getNumSends(); }

  size_t Distributor::getMaxSendLength() const
  { return plan_.getMaxSendLength(); }

  Teuchos::ArrayView<const int> Distributor::getProcsFrom() const
  { return plan_.getProcsFrom(); }

  Teuchos::ArrayView<const size_t> Distributor::getLengthsFrom() const
  { return plan_.getLengthsFrom(); }

  Teuchos::ArrayView<const int> Distributor::getProcsTo() const
  { return plan_.getProcsTo(); }

  Teuchos::ArrayView<const size_t> Distributor::getLengthsTo() const
  { return plan_.getLengthsTo(); }

  Teuchos::RCP<Distributor>
  Distributor::getReverse(bool create) const {
    if (reverseDistributor_.is_null () && create) {
      createReverseDistributor ();
    }
    TEUCHOS_TEST_FOR_EXCEPTION
      (reverseDistributor_.is_null () && create, std::logic_error, "The reverse "
       "Distributor is null after createReverseDistributor returned.  "
       "Please report this bug to the Tpetra developers.");
    return reverseDistributor_;
  }


  void
  Distributor::createReverseDistributor() const
  {
    reverseDistributor_ = Teuchos::rcp(new Distributor(plan_.comm_));
    reverseDistributor_->plan_ = *plan_.getReversePlan();
    reverseDistributor_->verbose_ = verbose_;

    // requests_: Allocated on demand.
    // reverseDistributor_: See note below

    // mfh 31 Mar 2016: These are statistics, kept on calls to
    // doPostsAndWaits or doReversePostsAndWaits.  They weren't here
    // when I started, and I didn't add them, so I don't know if they
    // are accurate.
    reverseDistributor_->lastRoundBytesSend_ = 0;
    reverseDistributor_->lastRoundBytesRecv_ = 0;

    // I am my reverse Distributor's reverse Distributor.
    // Thus, it would be legit to do the following:
    //
    // reverseDistributor_->reverseDistributor_ = Teuchos::rcp (this, false);
    //
    // (Note use of a "weak reference" to avoid a circular RCP
    // dependency.)  The only issue is that if users hold on to the
    // reverse Distributor but let go of the forward one, this
    // reference won't be valid anymore.  However, the reverse
    // Distributor is really an implementation detail of Distributor
    // and not meant to be used directly, so we don't need to do this.
    reverseDistributor_->reverseDistributor_ = Teuchos::null;
  }

  void
  Distributor::doWaits()
  {
    actor_.doWaits(plan_);
  }

  void Distributor::doReverseWaits() {
    // call doWaits() on the reverse Distributor, if it exists
    if (! reverseDistributor_.is_null()) {
      reverseDistributor_->doWaits();
    }
  }

  std::string Distributor::description () const {
    std::ostringstream out;

    out << "\"Tpetra::Distributor\": {";
    const std::string label = this->getObjectLabel ();
    if (label != "") {
      out << "Label: " << label << ", ";
    }
    out << "How initialized: "
        << Details::DistributorHowInitializedEnumToString (plan_.howInitialized_)
        << ", Parameters: {"
        << "Send type: "
        << DistributorSendTypeEnumToString (plan_.sendType_)
        << ", Barrier between receives and sends: "
        << (plan_.barrierBetweenRecvSend_ ? "true" : "false")
        << ", Use distinct tags: "
        << (plan_.useDistinctTags_ ? "true" : "false")
        << ", Debug: " << (verbose_ ? "true" : "false")
        << "}}";
    return out.str ();
  }

  std::string
  Distributor::
  localDescribeToString (const Teuchos::EVerbosityLevel vl) const
  {
    using Teuchos::toString;
    using Teuchos::VERB_HIGH;
    using Teuchos::VERB_EXTREME;
    using std::endl;

    // This preserves current behavior of Distributor.
    if (vl <= Teuchos::VERB_LOW || plan_.comm_.is_null ()) {
      return std::string ();
    }

    auto outStringP = Teuchos::rcp (new std::ostringstream ());
    auto outp = Teuchos::getFancyOStream (outStringP); // returns RCP
    Teuchos::FancyOStream& out = *outp;

    const int myRank = plan_.comm_->getRank ();
    const int numProcs = plan_.comm_->getSize ();
    out << "Process " << myRank << " of " << numProcs << ":" << endl;
    Teuchos::OSTab tab1 (out);

    out << "selfMessage: " << hasSelfMessage () << endl;
    out << "numSends: " << getNumSends () << endl;
    if (vl == VERB_HIGH || vl == VERB_EXTREME) {
      out << "procsTo: " << toString (plan_.procIdsToSendTo_) << endl;
      out << "lengthsTo: " << toString (plan_.lengthsTo_) << endl;
      out << "maxSendLength: " << getMaxSendLength () << endl;
    }
    if (vl == VERB_EXTREME) {
      out << "startsTo: " << toString (plan_.startsTo_) << endl;
      out << "indicesTo: " << toString (plan_.indicesTo_) << endl;
    }
    if (vl == VERB_HIGH || vl == VERB_EXTREME) {
      out << "numReceives: " << getNumReceives () << endl;
      out << "totalReceiveLength: " << getTotalReceiveLength () << endl;
      out << "lengthsFrom: " << toString (plan_.lengthsFrom_) << endl;
      out << "startsFrom: " << toString (plan_.startsFrom_) << endl;
      out << "procsFrom: " << toString (plan_.procsFrom_) << endl;
    }

    out.flush (); // make sure the ostringstream got everything
    return outStringP->str ();
  }

  void
  Distributor::
  describe (Teuchos::FancyOStream& out,
            const Teuchos::EVerbosityLevel verbLevel) const
  {
    using std::endl;
    using Teuchos::VERB_DEFAULT;
    using Teuchos::VERB_NONE;
    using Teuchos::VERB_LOW;
    using Teuchos::VERB_MEDIUM;
    using Teuchos::VERB_HIGH;
    using Teuchos::VERB_EXTREME;
    const Teuchos::EVerbosityLevel vl =
      (verbLevel == VERB_DEFAULT) ? VERB_LOW : verbLevel;

    if (vl == VERB_NONE) {
      return; // don't print anything
    }
    // If this Distributor's Comm is null, then the the calling
    // process does not participate in Distributor-related collective
    // operations with the other processes.  In that case, it is not
    // even legal to call this method.  The reasonable thing to do in
    // that case is nothing.
    if (plan_.comm_.is_null ()) {
      return;
    }
    const int myRank = plan_.comm_->getRank ();
    const int numProcs = plan_.comm_->getSize ();

    // Only Process 0 should touch the output stream, but this method
    // in general may need to do communication.  Thus, we may need to
    // preserve the current tab level across multiple "if (myRank ==
    // 0) { ... }" inner scopes.  This is why we sometimes create
    // OSTab instances by pointer, instead of by value.  We only need
    // to create them by pointer if the tab level must persist through
    // multiple inner scopes.
    Teuchos::RCP<Teuchos::OSTab> tab0, tab1;

    if (myRank == 0) {
      // At every verbosity level but VERB_NONE, Process 0 prints.
      // By convention, describe() always begins with a tab before
      // printing.
      tab0 = Teuchos::rcp (new Teuchos::OSTab (out));
      // We quote the class name because it contains colons.
      // This makes the output valid YAML.
      out << "\"Tpetra::Distributor\":" << endl;
      tab1 = Teuchos::rcp (new Teuchos::OSTab (out));

      const std::string label = this->getObjectLabel ();
      if (label != "") {
        out << "Label: " << label << endl;
      }
      out << "Number of processes: " << numProcs << endl
          << "How initialized: "
          << Details::DistributorHowInitializedEnumToString (plan_.howInitialized_)
          << endl;
      {
        out << "Parameters: " << endl;
        Teuchos::OSTab tab2 (out);
        out << "\"Send type\": "
            << DistributorSendTypeEnumToString (plan_.sendType_) << endl
            << "\"Barrier between receives and sends\": "
            << (plan_.barrierBetweenRecvSend_ ? "true" : "false") << endl
            << "\"Use distinct tags\": "
            << (plan_.useDistinctTags_ ? "true" : "false") << endl
            << "\"Debug\": " << (verbose_ ? "true" : "false") << endl;
      }
    } // if myRank == 0

    // This is collective over the Map's communicator.
    if (vl > VERB_LOW) {
      const std::string lclStr = this->localDescribeToString (vl);
      Tpetra::Details::gathervPrint (out, lclStr, *plan_.comm_);
    }

    out << "Reverse Distributor:";
    if (reverseDistributor_.is_null ()) {
      out << " null" << endl;
    }
    else {
      out << endl;
      reverseDistributor_->describe (out, vl);
    }
  }

  size_t
  Distributor::
  createFromSends(const Teuchos::ArrayView<const int>& exportProcIDs)
  {
    return plan_.createFromSends(exportProcIDs);
  }

  void
  Distributor::
  createFromSendsAndRecvs (const Teuchos::ArrayView<const int>& exportProcIDs,
                           const Teuchos::ArrayView<const int>& remoteProcIDs)
  {
    plan_.createFromSendsAndRecvs(exportProcIDs, remoteProcIDs);
  }

} // namespace Tpetra
