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

#include "Rythmos_GAASPErrorEstimate.hpp"

namespace Rythmos {

double GAASPErrorEstimate::getTotalError() {
  TEST_FOR_EXCEPT(true);
  return(-1.0);
}

std::string GAASPErrorEstimate::description() const
{
  std::string name = "Rythmos::GAASPErrorEstimate";
  return(name);
}

void GAASPErrorEstimate::describe(
  Teuchos::FancyOStream                &out
  ,const Teuchos::EVerbosityLevel      verbLevel
  ) const
{
  out << description() << "::describe" << std::endl;
}

void GAASPErrorEstimate::setParameterList(Teuchos::RCP<Teuchos::ParameterList> const& paramList)
{
  paramList_ = paramList;
}

Teuchos::RCP<Teuchos::ParameterList> GAASPErrorEstimate::getParameterList()
{
  return(paramList_);
}

Teuchos::RCP<Teuchos::ParameterList> GAASPErrorEstimate::unsetParameterList()
{
  Teuchos::RCP<Teuchos::ParameterList> temp_param_list = paramList_;
  paramList_ = Teuchos::null;
  return(temp_param_list);
}


} // namespace Rythmos


