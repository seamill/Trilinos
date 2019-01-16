// @HEADER
// ***********************************************************************
//
//           Panzer: A partial differential equation assembly
//       engine for strongly coupled complex multiphysics systems
//                 Copyright (2011) Sandia Corporation
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
// Questions? Contact Roger P. Pawlowski (rppawlo@sandia.gov) and
// Eric C. Cyr (eccyr@sandia.gov)
// ***********************************************************************
// @HEADER

#ifndef PANZER_RESPONSE_SCATTER_EVALUATOR_IPCoordinates_HPP
#define PANZER_RESPONSE_SCATTER_EVALUATOR_IPCoordinates_HPP

#include <iostream>
#include <string>

#include "PanzerDiscFE_config.hpp"
#include "Panzer_Dimension.hpp"
#include "Panzer_CellData.hpp"
#include "Panzer_Response_IPCoordinates.hpp"

#include "Phalanx_Evaluator_Macros.hpp"
#include "Phalanx_MDField.hpp"

#include "Panzer_Evaluator_WithBaseImpl.hpp"

namespace panzer {

/** This class handles responses with values aggregated
  * on each finite element cell.
  */
template<typename EvalT, typename Traits>
class ResponseScatterEvaluator_IPCoordinates : public panzer::EvaluatorWithBaseImpl<Traits>,
                                               public PHX::EvaluatorDerived<EvalT, Traits>  { 
public:

  //! A constructor with concrete arguments instead of a parameter list.
  ResponseScatterEvaluator_IPCoordinates(const std::string & response_name,
                                         const panzer::IntegrationDescriptor & description);

  void evaluateFields(typename Traits::EvalData d);

  void preEvaluate(typename Traits::PreEvalData d);
  void postEvaluate(typename Traits::PostEvalData d);

private:
  typedef typename EvalT::ScalarT ScalarT;

  std::string responseName_;

  panzer::IntegrationDescriptor id_;

  Teuchos::RCP<Response_IPCoordinates<EvalT> > responseObj_;
  std::vector<std::vector<ScalarT> > tmpCoords_;

  Teuchos::RCP<PHX::FieldTag> scatterHolder_; // dummy target
};

}

#include "Panzer_ResponseScatterEvaluator_IPCoordinates_impl.hpp"

#endif
