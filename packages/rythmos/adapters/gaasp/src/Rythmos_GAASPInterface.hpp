//@HEADER
// ***********************************************************************
//
//                     Rythmos Package
//                 Copyright (2006) Sandia Corporation
//
// Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
// license for use of this work by or on behalf of the U.S. Government.
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA
// Questions? Contact Todd S. Coffey (tscoffe@sandia.gov)
//
// ***********************************************************************
//@HEADER

#ifndef Rythmos_GAASP_INTERFACE_H
#define Rythmos_GAASP_INTERFACE_H

#include "Thyra_ModelEvaluator.hpp"
#include "GModelBase.h"
#include "InitGaaspOO.h"
#include "GForwardSolve.h"
#include "boost/shared_ptr.hpp"

namespace Rythmos {

class GAASPInterface {
  public:
    GAASPInterface();
    ~GAASPInterface() {};
    void forwardSolve();
    void adjointSolve();
    void computeErrorEstimate();
    void setThyraModelEvaluator(Teuchos::RCP<Thyra::ModelEvaluator<double> > tModel);
  private:
    Teuchos::RCP<Thyra::ModelEvaluator<double> > tModel_;
    boost::shared_ptr<GAASP::GModelBase> gModel_;
    double sTime_;    // Start time
    double eTime_;    // End time
    double timeStep_; // Time step size
    GAASP::TMethodFlag method_; // Time integration method pair
    GAASP::TErrorFlag qtyOfInterest_; // Quantity of Interest
    std::vector<double> initialCondition_; // Model DE initial condition
    bool forwardSolveCompleted_;
    double **fSoln_;  // Forward solution
    double *mesh_;    // Forward solution mesh
    double **fDer_;   // Forward solution derivatives
    int nSteps_;      // (size of the Forward solution mesh_) - 1
    bool adjointSolveCompleted_;
    double **aSoln_;   // Adjoint solution

};

} // namespace Rythmos


#endif // Rythmos_GAASP_INTERFACE_H
