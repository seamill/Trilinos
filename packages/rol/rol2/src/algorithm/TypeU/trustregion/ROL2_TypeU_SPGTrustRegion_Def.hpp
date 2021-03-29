// @HEADER
// ************************************************************************
//
//               Rapid Optimization Library (ROL) Package
//                 Copyright (2014) Sandia Corporation
//
// Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
// license for use of this work by or on behalf of the U.S. Government.
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
// Questions? Contact lead developers:
//              Drew Kouri   (dpkouri@sandia.gov) and
//              Denis Ridzal (dridzal@sandia.gov)
//
// ************************************************************************
// @HEADER

#pragma once
#ifndef ROL2_TYPEU_SPGTRUSTREGION_DEF_H
#define ROL2_TYPEU_SPGTRUSTREGION_DEF_H

#include <deque>

/** \class ROL2::TypeU::SPGTrustRegion
    \brief Provides interface for truncated CG trust-region subproblem solver.
*/

namespace ROL2 { 
namespace TypeU {

template<typename Real>
SPGTrustRegion<Real>::SPGTrustRegion( ParameterList& parlist ) {

  auto& spglist  = parlist.sublist("Step").sublist("Trust Region").sublist("SPG");
  auto& slvrlist = spglist.sublist("Solver");

  // Spectral projected gradient parameters
  lambdaMin_ = slvrlist.get("Minimum Spectral Step Size",   1e-8);
  lambdaMax_ = slvrlist.get("Maximum Spectral Step Size",   1e8);
  gamma_     = slvrlist.get("Sufficient Decrease Tolerance",1e-4);
  maxSize_   = slvrlist.get("Maximum Storage Size",         10);
  maxit_     = slvrlist.get("Iteration Limit",              25);
  tol1_      = slvrlist.get("Absolute Tolerance",           1e-4);
  tol2_      = slvrlist.get("Relative Tolerance",           1e-2);
  useMin_    = slvrlist.get("Use Smallest Model Iterate",   true);
  useNMSP_   = slvrlist.get("Use Nonmonotone Search",       false);
}

template<typename Real>
void SPGTrustRegion<Real>::initialize( const Vector<Real>& x, 
                                       const Vector<Real>& g ) {
  pwa_  = x.clone();
  pwa1_ = x.clone();
  smin_ = x.clone();
  dwa_  = g.clone();
  gmod_ = g.clone();
}

template<typename Real>
void SPGTrustRegion<Real>::solve( Vector<Real>&           s,
                                  Real&                   snorm,
                                  Real&                   pRed,
                                  int&                    iflag,
                                  int&                    iter,
                                  Real                    del,
                                  TrustRegionModel<Real>& model ) {
  // TODO: Check if eps is really supposed to be sqrt of machine epsilon
  const Real zero(0), half(0.5), one(1), two(2), eps(std::sqrt(ROL_EPSILON<Real>));
  Real tol(eps), alpha(1), sHs(0), alphaTmp(1), mmax(0), qmin(0), q(0);
  Real gnorm(0), ss(0), gs(0);
  std::deque<Real> mqueue; 
  mqueue.push_back(0);

  gmod_->set(model.getGradient());

  // Compute Cauchy point
  pwa1_->set(gmod_->dual());
  s.set(*pwa1_); s.scale(-one);
  model.hessVec(*dwa_,s,s,tol);
  gs = gmod_->apply(s);
  sHs = dwa_->apply(s);
  snorm = std::sqrt(std::abs(gs));
  alpha = -gs/sHs;

  if (alpha*snorm >= del || sHs <= zero) alpha = del/snorm;

  q = alpha*(gs+half*alpha*sHs);
  gmod_->axpy(alpha,*dwa_);
  s.scale(alpha);

  if (useNMSP_ && useMin_) { 
    qmin = q; 
    smin_->set(s);
  }

  // Compute initial projected gradient
  pwa1_->set(gmod_->dual());
  pwa_->set(s); pwa_->axpy(-one,*pwa1_);
  snorm = pwa_->norm();

  if (snorm > del) pwa_->scale(del/snorm);

  pwa_->axpy(-one,s);
  gnorm = pwa_->norm();

  if (gnorm == zero) {
    snorm = s.norm();
    pRed  = -q;
  }
  else {

    Real gtol = std::min(tol1_,tol2_*gnorm);

    // Compute initial step
    Real lambda = std::max(lambdaMin_,std::min(one/gmod_->norm(),lambdaMax_));
    pwa_->set(s); pwa_->axpy(-lambda,*pwa1_);
    snorm = pwa_->norm();
    if (snorm > del) pwa_->scale(del/snorm);
    pwa_->axpy(-one,s);
    gs = gmod_->apply(*pwa_);
    ss = pwa_->dot(*pwa_);

    for (iter = 0; iter < maxit_; iter++) {

      // Evaluate model Hessian
      model.hessVec(*dwa_,*pwa_,s,tol);
      sHs = dwa_->apply(*pwa_);

      // Perform line search
      if (useNMSP_) { // Nonmonotone
        mmax = *std::max_element(mqueue.begin(),mqueue.end());
        alphaTmp = (-(one-gamma_)*gs + std::sqrt(std::pow((one-gamma_)*gs,two)-two*sHs*(q-mmax)))/sHs;
      }
      else alphaTmp = -gs/sHs; // Exact

      alpha = (sHs > zero ? std::min(one,std::max(zero,alphaTmp)) : one);

      // Update model quantities
      q += alpha*(gs+half*alpha*sHs);
      gmod_->axpy(alpha,*dwa_);
      s.axpy(alpha,*pwa_);

      // Update nonmonotone line search information
      if (useNMSP_) {
        if (static_cast<int>(mqueue.size())==maxSize_) mqueue.pop_front();
        mqueue.push_back(q);
        if (useMin_ && q <= qmin)  qmin = q; smin_->set(s); 
      }

      // Compute Projected gradient norm
      pwa1_->set(gmod_->dual());
      pwa_->set(s); pwa_->axpy(-one,*pwa1_);
      snorm = pwa_->norm();

      if (snorm > del) pwa_->scale(del/snorm);

      pwa_->axpy(-one,s);
      gnorm = pwa_->norm();

      if (gnorm < gtol) break;

      // Compute new spectral step
      lambda = (sHs <= eps ? lambdaMax_ : std::max(lambdaMin_,std::min(ss/sHs,lambdaMax_)));
      pwa_->set(s); pwa_->axpy(-lambda,*pwa1_);
      snorm = pwa_->norm();
      if (snorm > del) pwa_->scale(del/snorm);
      pwa_->axpy(-one,s);
      gs = gmod_->apply(*pwa_);
      ss = pwa_->dot(*pwa_);
    }

    if (useNMSP_ && useMin_) { q = qmin; s.set(*smin_); }
    iflag = (iter==maxit_ ? 1 : 0);
    pRed = -q;
    snorm = s.norm();
  }
}

} // namespace TypeU
} // namespace ROL2

#endif // ROL2_TYPEU_SPGTRUSTREGION_DEF_H
