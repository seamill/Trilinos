// @HEADER
// ************************************************************************
//
//                           Intrepid2 Package
//                 Copyright (2007) Sandia Corporation
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
// Questions? Contact Kyungjoo Kim  (kyukim@sandia.gov), or
//                    Mauro Perego  (mperego@sandia.gov)
//
// ************************************************************************
// @HEADER

/** \file   test_01.hpp
    \brief  Unit tests for the Intrepid2::HCURL_QUAD_In_FEM class.
    \author Created by P. Bochev, D. Ridzal, K. Peterson, Kyungjoo Kim
*/


#include "Intrepid2_config.h"

#ifdef HAVE_INTREPID2_DEBUG
#define INTREPID2_TEST_FOR_DEBUG_ABORT_OVERRIDE_TO_CONTINUE
#endif

#include "Intrepid2_Types.hpp"
#include "Intrepid2_Utils.hpp"

#include "Intrepid2_PointTools.hpp"
#include "Intrepid2_HCURL_QUAD_In_FEM.hpp"

#include "Teuchos_oblackholestream.hpp"
#include "Teuchos_RCP.hpp"

namespace Intrepid2 {

  namespace Test {

#define INTREPID2_TEST_ERROR_EXPECTED( S )                              \
    try {                                                               \
      ++nthrow;                                                         \
      S ;                                                               \
    }                                                                   \
    catch (std::exception err) {                                        \
      ++ncatch;                                                         \
      *outStream << "Expected Error ----------------------------------------------------------------\n"; \
      *outStream << err.what() << '\n';                                 \
      *outStream << "-------------------------------------------------------------------------------" << "\n\n"; \
    }


    template<typename ValueType, typename DeviceSpaceType>
    int HCURL_QUAD_In_FEM_Test01(const bool verbose) {

      Teuchos::RCP<std::ostream> outStream;
      Teuchos::oblackholestream bhs; // outputs nothing

      if (verbose)
        outStream = Teuchos::rcp(&std::cout, false);
      else
        outStream = Teuchos::rcp(&bhs,       false);

      Teuchos::oblackholestream oldFormatState;
      oldFormatState.copyfmt(std::cout);

      typedef typename
        Kokkos::Impl::is_space<DeviceSpaceType>::host_mirror_space::execution_space HostSpaceType ;

      *outStream << "DeviceSpace::  "; DeviceSpaceType::print_configuration(*outStream, false);
      *outStream << "HostSpace::    ";   HostSpaceType::print_configuration(*outStream, false);

      *outStream
        << "===============================================================================\n"
        << "|                                                                             |\n"
        << "|                 Unit Test (Basis_HCURL_QUAD_In_FEM)                         |\n"
        << "|                                                                             |\n"
        << "|     1) Conversion of Dof tags into Dof ordinals and back                    |\n"
        << "|     2) Basis values for VALUE and CURL operators                            |\n"
        << "|                                                                             |\n"
        << "|  Questions? Contact  Pavel Bochev  (pbboche@sandia.gov),                    |\n"
        << "|                      Denis Ridzal  (dridzal@sandia.gov),                    |\n"
        << "|                      Kara Peterson (kjpeter@sandia.gov),                    |\n"
        << "|                      Kyungjoo Kim  (kyukim@sandia.gov).                     |\n"
        << "|                                                                             |\n"
        << "|  Intrepid's website: http://trilinos.sandia.gov/packages/intrepid           |\n"
        << "|  Trilinos website:   http://trilinos.sandia.gov                             |\n"
        << "|                                                                             |\n"
        << "===============================================================================\n";
      
      typedef Kokkos::DynRankView<ValueType,DeviceSpaceType> DynRankView;
      typedef Kokkos::DynRankView<ValueType,HostSpaceType>   DynRankViewHost;
#define ConstructWithLabel(obj, ...) obj(#obj, __VA_ARGS__)

      const ValueType tol = tolerence();
      int errorFlag = 0;

      // for virtual function, value and point types are declared in the class
      typedef ValueType outputValueType;
      typedef ValueType pointValueType;

      typedef Basis_HCURL_QUAD_In_FEM<DeviceSpaceType,outputValueType,pointValueType> QuadBasisType;

      *outStream
        << "\n"
        << "===============================================================================\n"
        << "| TEST 1: Basis creation, exceptions tests                                    |\n"
        << "===============================================================================\n";

      try{

        ordinal_type nthrow = 0, ncatch = 0;
#ifdef HAVE_INTREPID2_DEBUG
        const ordinal_type order = 5;
        QuadBasisType quadBasis(order);

        // Define array containing array of nodes to evaluate
        DynRankView ConstructWithLabel(quadNodes, 9, 2);

        // Generic array for the output values; needs to be properly resized depending on the operator type
        const ordinal_type numFields = quadBasis.getCardinality();
        const ordinal_type numPoints = quadNodes.dimension(0);
        const ordinal_type spaceDim  = quadBasis.getBaseCellTopology().getDimension();

        {
          DynRankView ConstructWithLabel(vals, numFields, numPoints, spaceDim);

          // exception #1: GRAD cannot be applied to HCURL functions
          INTREPID2_TEST_ERROR_EXPECTED( quadBasis.getValues(vals, quadNodes, OPERATOR_GRAD) );

          // exception #2: DIV cannot be applied to HCURL functions
          INTREPID2_TEST_ERROR_EXPECTED( quadBasis.getValues(vals, quadNodes, OPERATOR_DIV) );
        }

        // Exceptions 3-7: all bf tags/bf Ids below are wrong and should cause getDofOrdinal() and
        // getDofTag() to access invalid array elements thereby causing bounds check exception
        {
          INTREPID2_TEST_ERROR_EXPECTED( quadBasis.getDofOrdinal(3,0,0) );
          INTREPID2_TEST_ERROR_EXPECTED( quadBasis.getDofOrdinal(1,0,5) );
          INTREPID2_TEST_ERROR_EXPECTED( quadBasis.getDofOrdinal(0,4,0) );
          INTREPID2_TEST_ERROR_EXPECTED( quadBasis.getDofTag(numFields) );
          INTREPID2_TEST_ERROR_EXPECTED( quadBasis.getDofTag(-1) );
        }

        // Exceptions 8-15 test exception handling with incorrectly dimensioned input/output arrays
        {
          DynRankView ConstructWithLabel(vals, numFields, numPoints, spaceDim);
          {
            // exception #8: input points array must be of rank-2
            DynRankView ConstructWithLabel(badPoints, 4, 5, 3);
            INTREPID2_TEST_ERROR_EXPECTED( quadBasis.getValues(vals, badPoints, OPERATOR_VALUE) );
          }
          {
            // exception #9 dimension 1 in the input point array must equal space dimension of the cell
            DynRankView ConstructWithLabel(badPoints, 4, 3);
            INTREPID2_TEST_ERROR_EXPECTED( quadBasis.getValues(vals, badPoints, OPERATOR_VALUE) );
          }
          {
            // exception #10 output values must be of rank-3 for OPERATOR_VALUE in 2D
            DynRankView ConstructWithLabel(badVals, 4, 3);
            INTREPID2_TEST_ERROR_EXPECTED( quadBasis.getValues(badVals, quadNodes, OPERATOR_VALUE) );
          }
          {
            // exception #11 output values must be of rank-2 for OPERATOR_CURL
            DynRankView ConstructWithLabel(badVals, 4, 3, 2);
            INTREPID2_TEST_ERROR_EXPECTED( quadBasis.getValues(badVals, quadNodes, OPERATOR_CURL) );
          }
          {
            // exception #12 incorrect 0th dimension of output array (must equal number of basis functions)
            DynRankView ConstructWithLabel(badVals, quadBasis.getCardinality() + 1, quadNodes.dimension(0), quadBasis.getBaseCellTopology().getDimension());
            INTREPID2_TEST_ERROR_EXPECTED( quadBasis.getValues(badVals, quadNodes, OPERATOR_VALUE) ) ;
          }
          {
            // exception #13 incorrect 1st  dimension of output array (must equal number of points)
            DynRankView ConstructWithLabel(badVals, quadBasis.getCardinality(), quadNodes.dimension(0) + 1, quadBasis.getBaseCellTopology().getDimension() );
            INTREPID2_TEST_ERROR_EXPECTED( quadBasis.getValues(badVals, quadNodes, OPERATOR_VALUE) ) ;
          }
          {
            // exception #14: incorrect 2nd dimension of output array for VALUE (must equal the space dimension)
            DynRankView ConstructWithLabel(badVals, quadBasis.getCardinality(), quadNodes.dimension(0), quadBasis.getBaseCellTopology().getDimension() - 1);
            INTREPID2_TEST_ERROR_EXPECTED( quadBasis.getValues(badVals, quadNodes, OPERATOR_VALUE) ) ;
          }
        }
#endif
        if (nthrow != ncatch) {
          errorFlag++;
          *outStream << std::setw(70) << "^^^^----FAILURE!" << "\n";
          *outStream << "# of catch ("<< ncatch << ") is different from # of throw (" << nthrow << ")\n";
        }
      } catch (std::exception err) {
        *outStream << "UNEXPECTED ERROR !!! ----------------------------------------------------------\n";
        *outStream << err.what() << '\n';
        *outStream << "-------------------------------------------------------------------------------" << "\n\n";
        errorFlag = -1000;
      };

      *outStream
        << "\n"
        << "===============================================================================\n"
        << "| TEST 2: correctness of tag to enum and enum to tag lookups                  |\n"
        << "===============================================================================\n";

      try {
        const ordinal_type order = 5;
        QuadBasisType quadBasis(order);

        const ordinal_type numFields = quadBasis.getCardinality();
        const auto allTags = quadBasis.getAllDofTags();

        // Loop over all tags, lookup the associated dof enumeration and then lookup the tag again
        const ordinal_type dofTagSize = allTags.dimension(0);
        for (ordinal_type i=0;i<dofTagSize;++i) {
          const ordinal_type bfOrd = quadBasis.getDofOrdinal(allTags(i,0), allTags(i,1), allTags(i,2));
          
          const auto myTag = quadBasis.getDofTag(bfOrd);
          if( !( (myTag(0) == allTags(i,0)) &&
                 (myTag(1) == allTags(i,1)) &&
                 (myTag(2) == allTags(i,2)) &&
                 (myTag(3) == allTags(i,3)) ) ) {
            errorFlag++;
            *outStream << std::setw(70) << "^^^^----FAILURE!" << "\n";
            *outStream << " getDofOrdinal( {"
                       << allTags(i,0) << ", "
                       << allTags(i,1) << ", "
                       << allTags(i,2) << ", "
                       << allTags(i,3) << "}) = " << bfOrd <<" but ";
            *outStream << " getDofTag(" << bfOrd << ") = {"
                       << myTag(0) << ", "
                       << myTag(1) << ", "
                       << myTag(2) << ", "
                       << myTag(3) << "}\n";
          }
        }
        
        // Now do the same but loop over basis functions
        for(ordinal_type bfOrd=0;bfOrd<numFields;++bfOrd) {
          const auto myTag  = quadBasis.getDofTag(bfOrd);
          const ordinal_type myBfOrd = quadBasis.getDofOrdinal(myTag(0), myTag(1), myTag(2));
          if( bfOrd != myBfOrd) {
            errorFlag++;
            *outStream << std::setw(70) << "^^^^----FAILURE!" << "\n";
            *outStream << " getDofTag(" << bfOrd << ") = { "
                       << myTag(0) << ", "
                       << myTag(1) << ", "
                       << myTag(2) << ", "
                       << myTag(3) << "} but getDofOrdinal({"
                       << myTag(0) << ", "
                       << myTag(1) << ", "
                       << myTag(2) << ", "
                       << myTag(3) << "} ) = " << myBfOrd << "\n";
          }
        }
      } catch (std::logic_error err){
        *outStream << "UNEXPECTED ERROR !!! ----------------------------------------------------------\n";
        *outStream << err.what() << '\n';
        *outStream << "-------------------------------------------------------------------------------" << "\n\n";
        errorFlag = -1000;
      };

      *outStream
        << "\n"
        << "===============================================================================\n"
        << "| TEST 3: correctness of basis function values                                |\n"
        << "===============================================================================\n";

      outStream->precision(20);

      // VALUE: Each row pair gives the 4x2 correct basis set values at an evaluation point: (P,F,D) layout
      const ValueType basisValues[] = {
        1,0,1,0,1,0, /**/ 0.5,0,0.5,0,0.5,0, /**/ 0,0,0,0,0,0,
        0,0,0,0,0,0, /**/ 0.5,0,0.5,0,0.5,0, /**/ 1,0,1,0,1,0,
        0,1,0,0.5,0,0, /**/ 0,1,0,0.5,0,0, /**/   0,1,0,0.5,0,0,
        0,0,0,0.5,0,1, /**/ 0,0,0,0.5,0,1, /**/   0,0,0,0.5,0,1,
      };

      // CURL: correct values in (F,P) format
      const ValueType basisCurls[] = {
        0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5,
        -0.5, -0.5, -0.5, -0.5, -0.5, -0.5, -0.5, -0.5, -0.5,
        -0.5, -0.5, -0.5, -0.5, -0.5, -0.5, -0.5, -0.5, -0.5,
        0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5
      };

      try{
        const ordinal_type order = 1;
        QuadBasisType quadBasis(order);

        // Define array containing array of nodes to evaluate
        DynRankViewHost ConstructWithLabel(quadNodesHost, 9, 2);

        quadNodesHost(0,0) = -1.0;  quadNodesHost(0,1) = -1.0;
        quadNodesHost(1,0) =  0.0;  quadNodesHost(1,1) = -1.0;
        quadNodesHost(2,0) =  1.0;  quadNodesHost(2,1) = -1.0;
        quadNodesHost(3,0) = -1.0;  quadNodesHost(3,1) =  0.0;
        quadNodesHost(4,0) =  0.0;  quadNodesHost(4,1) =  0.0;
        quadNodesHost(5,0) =  1.0;  quadNodesHost(5,1) =  0.0;
        quadNodesHost(6,0) = -1.0;  quadNodesHost(6,1) =  1.0;
        quadNodesHost(7,0) =  0.0;  quadNodesHost(7,1) =  1.0;
        quadNodesHost(8,0) =  1.0;  quadNodesHost(8,1) =  1.0;

        auto quadNodes = Kokkos::create_mirror_view(typename DeviceSpaceType::memory_space(), quadNodesHost);
        Kokkos::deep_copy(quadNodes, quadNodesHost);

        // Generic array for the output values; needs to be properly resized depending on the operator type
        const ordinal_type numFields = quadBasis.getCardinality();
        const ordinal_type numPoints = quadNodes.dimension(0);
        const ordinal_type spaceDim  = quadBasis.getBaseCellTopology().getDimension();

        *outStream << " -- Testing OPERATOR_VALUE \n";
        {
          // Check VALUE of basis functions: resize vals to rank-3 container:
          DynRankView ConstructWithLabel(vals, numFields, numPoints, spaceDim);
          quadBasis.getValues(vals, quadNodes, OPERATOR_VALUE);
          auto vals_host = Kokkos::create_mirror_view(typename HostSpaceType::memory_space(), vals);
          Kokkos::deep_copy(vals_host, vals);


          for (ordinal_type i = 0; i < numFields; ++i) {
            for (ordinal_type j = 0; j < numPoints; ++j) {
              for (ordinal_type k = 0; k < spaceDim; ++k) {

                // compute offset for (P,F,D) data layout: indices are P->j, F->i, D->k
                const ordinal_type l = k + i * spaceDim * numPoints + j * spaceDim;
                if (std::abs(vals_host(i,j,k) - basisValues[l]) > tol) {
                  errorFlag++;
                  *outStream << std::setw(70) << "^^^^----FAILURE!" << "\n";

                  // Output the multi-index of the value where the error is:
                  *outStream << " At multi-index { ";
                  *outStream << i << " ";*outStream << j << " ";*outStream << k << " ";
                  *outStream << "}  computed value: " << vals(i,j,k)
                             << " but reference value: " << basisValues[l] << "\n";
                }
              }
            }
          }
        }
        *outStream << " -- Testing OPERATOR_CURL \n";
        {
          // Check CURL of basis function: resize vals to rank-2 container
          DynRankView ConstructWithLabel(vals, numFields, numPoints);
          quadBasis.getValues(vals, quadNodes, OPERATOR_CURL);
          for (ordinal_type i = 0; i < numFields; ++i) {
            for (ordinal_type j = 0; j < numPoints; ++j) {
              const ordinal_type l =  j + i * numPoints;
              if (std::abs(vals(i,j) - basisCurls[l]) > tol) {
                errorFlag++;
                *outStream << std::setw(70) << "^^^^----FAILURE!" << "\n";

                // Output the multi-index of the value where the error is:
                *outStream << " At multi-index { ";
                *outStream << i << " ";*outStream << j << " ";
                *outStream << "}  computed curl component: " << vals(i,j)
                           << " but reference curl component: " << basisCurls[l] << "\n";
              }
            }
          }
        }
      } catch (std::logic_error err) {
        *outStream << err.what() << "\n\n";
        errorFlag = -1000;
      }

      if (errorFlag != 0)
        std::cout << "End Result: TEST FAILED\n";
      else
        std::cout << "End Result: TEST PASSED\n";

      // reset format state of std::cout
      std::cout.copyfmt(oldFormatState);
      return errorFlag;
    }
  }
}




























