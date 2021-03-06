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

#ifndef HS_PROBLEMFACTORY_HPP
#define HS_PROBLEMFACTORY_HPP

#include "HS_Problem_001.hpp"
#include "HS_Problem_002.hpp"
#include "HS_Problem_003.hpp"
#include "HS_Problem_004.hpp"
#include "HS_Problem_005.hpp"
#include "HS_Problem_006.hpp"
#include "HS_Problem_007.hpp"
#include "HS_Problem_008.hpp"
#include "HS_Problem_009.hpp"
#include "HS_Problem_010.hpp"
#include "HS_Problem_011.hpp"
#include "HS_Problem_012.hpp"
#include "HS_Problem_013.hpp"
#include "HS_Problem_014.hpp"
#include "HS_Problem_015.hpp"
#include "HS_Problem_016.hpp"
#include "HS_Problem_017.hpp"
#include "HS_Problem_018.hpp"
#include "HS_Problem_019.hpp"
#include "HS_Problem_020.hpp"
#include "HS_Problem_021.hpp"
#include "HS_Problem_022.hpp"
#include "HS_Problem_023.hpp"
#include "HS_Problem_024.hpp"
#include "HS_Problem_025.hpp"
#include "HS_Problem_026.hpp"
#include "HS_Problem_027.hpp"
#include "HS_Problem_028.hpp"
#include "HS_Problem_029.hpp"
#include "HS_Problem_030.hpp"
#include "HS_Problem_031.hpp"
#include "HS_Problem_032.hpp"
#include "HS_Problem_033.hpp"
#include "HS_Problem_034.hpp"
#include "HS_Problem_035.hpp"
#include "HS_Problem_036.hpp"
#include "HS_Problem_037.hpp"
#include "HS_Problem_038.hpp"
#include "HS_Problem_039.hpp"
#include "HS_Problem_040.hpp"

namespace HS {
template<class Real> 
class ProblemFactory {
public:
  Teuchos::RCP<ROL::NonlinearProgram<Real> > getProblem(int n) {
    Teuchos::RCP<ROL::NonlinearProgram<Real> > np;
    switch(n) {
      case   1: np = Teuchos::rcp( new Problem_001<Real>() ); break;
      case   2: np = Teuchos::rcp( new Problem_002<Real>() ); break;
      case   3: np = Teuchos::rcp( new Problem_003<Real>() ); break;
      case   4: np = Teuchos::rcp( new Problem_004<Real>() ); break;
      case   5: np = Teuchos::rcp( new Problem_005<Real>() ); break;
      case   6: np = Teuchos::rcp( new Problem_006<Real>() ); break;
      case   7: np = Teuchos::rcp( new Problem_007<Real>() ); break;
      case   8: np = Teuchos::rcp( new Problem_008<Real>() ); break;
      case   9: np = Teuchos::rcp( new Problem_009<Real>() ); break;
      case  10: np = Teuchos::rcp( new Problem_010<Real>() ); break;
      case  11: np = Teuchos::rcp( new Problem_011<Real>() ); break;
      case  12: np = Teuchos::rcp( new Problem_012<Real>() ); break;
      case  13: np = Teuchos::rcp( new Problem_013<Real>() ); break;
      case  14: np = Teuchos::rcp( new Problem_014<Real>() ); break;
      case  15: np = Teuchos::rcp( new Problem_015<Real>() ); break;
      case  16: np = Teuchos::rcp( new Problem_016<Real>() ); break;
      case  17: np = Teuchos::rcp( new Problem_017<Real>() ); break;
      case  18: np = Teuchos::rcp( new Problem_018<Real>() ); break;
      case  19: np = Teuchos::rcp( new Problem_019<Real>() ); break;
      case  20: np = Teuchos::rcp( new Problem_020<Real>() ); break;
      case  21: np = Teuchos::rcp( new Problem_021<Real>() ); break;
      case  22: np = Teuchos::rcp( new Problem_022<Real>() ); break;
      case  23: np = Teuchos::rcp( new Problem_023<Real>() ); break;
      case  24: np = Teuchos::rcp( new Problem_024<Real>() ); break;
      case  25: np = Teuchos::rcp( new Problem_025<Real>() ); break;
      case  26: np = Teuchos::rcp( new Problem_026<Real>() ); break;
      case  27: np = Teuchos::rcp( new Problem_027<Real>() ); break;
      case  28: np = Teuchos::rcp( new Problem_028<Real>() ); break;
      case  29: np = Teuchos::rcp( new Problem_029<Real>() ); break;
      case  30: np = Teuchos::rcp( new Problem_030<Real>() ); break;
      case  31: np = Teuchos::rcp( new Problem_031<Real>() ); break;
      case  32: np = Teuchos::rcp( new Problem_032<Real>() ); break;
      case  33: np = Teuchos::rcp( new Problem_033<Real>() ); break;
      case  34: np = Teuchos::rcp( new Problem_034<Real>() ); break;
      case  35: np = Teuchos::rcp( new Problem_035<Real>() ); break;
      case  36: np = Teuchos::rcp( new Problem_036<Real>() ); break;
      case  37: np = Teuchos::rcp( new Problem_037<Real>() ); break;
      case  38: np = Teuchos::rcp( new Problem_038<Real>() ); break;
      case  39: np = Teuchos::rcp( new Problem_039<Real>() ); break;
      case  40: np = Teuchos::rcp( new Problem_040<Real>() ); break;


      default:
        TEUCHOS_TEST_FOR_EXCEPTION(true,std::logic_error,"Unknown problem number.");
      break;
    }
    return np;
  }
};
}
 
#endif // HS_PROBLEMFACTORY_HPP

