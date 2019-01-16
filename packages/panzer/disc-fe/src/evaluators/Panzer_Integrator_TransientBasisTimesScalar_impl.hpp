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

#ifndef PANZER_EVALUATOR_TRANSIENT_BASISTIMESSCALAR_IMPL_HPP
#define PANZER_EVALUATOR_TRANSIENT_BASISTIMESSCALAR_IMPL_HPP

#include "Intrepid2_FunctionSpaceTools.hpp"
#include "Panzer_IntegrationRule.hpp"
#include "Panzer_BasisIRLayout.hpp"
#include "Kokkos_ViewFactory.hpp"

namespace panzer {

//**********************************************************************
template<typename EvalT, typename Traits>
Integrator_TransientBasisTimesScalar<EvalT, Traits>::
Integrator_TransientBasisTimesScalar(
  const Teuchos::ParameterList& p) :
  residual( p.get<std::string>("Residual Name"), 
	    p.get< Teuchos::RCP<panzer::BasisIRLayout> >("Basis")->functional),
  scalar( p.get<std::string>("Value Name"), 
	  p.get< Teuchos::RCP<panzer::IntegrationRule> >("IR")->dl_scalar),
  num_qp(0),
  num_nodes(0)
{
  Teuchos::RCP<Teuchos::ParameterList> valid_params = this->getValidParameters();
  p.validateParameters(*valid_params);

  const auto ir = p.get<Teuchos::RCP<panzer::IntegrationRule> >("IR");
  const auto basis = p.get< Teuchos::RCP<BasisIRLayout> >("Basis")->getBasis();

  bd_ = *basis;
  id_ = *ir;


  // Verify that this basis supports the gradient operation
  TEUCHOS_TEST_FOR_EXCEPTION(!basis->isScalarBasis(),std::logic_error,
                             "Integrator_TransientBasisTimesScalar: Basis of type \"" << basis->name() << "\" is not a "
                             "scalar basis");

  this->addEvaluatedField(residual);
  this->addDependentField(scalar);
    
  multiplier = p.get<double>("Multiplier");


  if (p.isType<Teuchos::RCP<const std::vector<std::string> > >("Field Multipliers")) {
    const auto & field_multiplier_names = *(p.get<Teuchos::RCP<const std::vector<std::string> > >("Field Multipliers"));

    for (const auto & name : field_multiplier_names) {
      PHX::MDField<const ScalarT,Cell,IP> tmp_field(name, ir->dl_scalar);
      field_multipliers.push_back(tmp_field);
    }
  }

  for (const auto & field : field_multipliers)
    this->addDependentField(field);

  std::string n = "Integrator_TransientBasisTimesScalar: " + residual.fieldTag().name();
  this->setName(n);
}

//**********************************************************************
template<typename EvalT, typename Traits>
void
Integrator_TransientBasisTimesScalar<EvalT, Traits>::
postRegistrationSetup(
  typename Traits::SetupData sd,
  PHX::FieldManager<Traits>& fm)
{
  this->utils.setFieldData(residual,fm);
  this->utils.setFieldData(scalar,fm);
  
  num_nodes = residual.extent(1);
  num_qp = scalar.extent(1);

  tmp = Kokkos::createDynRankView(residual.get_static_view(),"tmp",scalar.extent(0), num_qp); 
}

//**********************************************************************
template<typename EvalT, typename Traits>
void
Integrator_TransientBasisTimesScalar<EvalT, Traits>::
evaluateFields(
  typename Traits::EvalData workset)
{ 
  if (workset.template getDetails<WorksetStepperDetails>("Stepper").evaluate_transient_terms) {

    // for (int i=0; i < residual.size(); ++i)
    //   residual[i] = 0.0;

    Kokkos::deep_copy (residual.get_static_view(), ScalarT(0.0));

    for (index_t cell = 0; cell < workset.numCells(); ++cell) {
      for (std::size_t qp = 0; qp < num_qp; ++qp) {
        tmp(cell,qp) = multiplier * scalar(cell,qp);
        for (typename std::vector<PHX::MDField<const ScalarT,Cell,IP> >::iterator field = field_multipliers.begin();
            field != field_multipliers.end(); ++field)
          tmp(cell,qp) = tmp(cell,qp) * (*field)(cell,qp);
      }
    }

    if(workset.numCells()>0)
      Intrepid2::FunctionSpaceTools<PHX::exec_space>::
      integrate<ScalarT>(residual.get_view(),
                         tmp,
                         workset(this->details_idx_).getBasisIntegrationValues(bd_, id_).weighted_basis_scalar.get_view());
  }
}

//**********************************************************************
template<typename EvalT, typename TRAITS>
Teuchos::RCP<Teuchos::ParameterList> 
Integrator_TransientBasisTimesScalar<EvalT, TRAITS>::getValidParameters() const
{
  Teuchos::RCP<Teuchos::ParameterList> p = Teuchos::rcp(new Teuchos::ParameterList);
  p->set<std::string>("Residual Name", "?");
  p->set<std::string>("Value Name", "?");
  Teuchos::RCP<panzer::BasisIRLayout> basis;
  p->set("Basis", basis);
  Teuchos::RCP<panzer::IntegrationRule> ir;
  p->set("IR", ir);
  p->set<double>("Multiplier", 1.0);
  Teuchos::RCP<const std::vector<std::string> > fms;
  p->set("Field Multipliers", fms);
  return p;
}

//**********************************************************************

}

#endif

