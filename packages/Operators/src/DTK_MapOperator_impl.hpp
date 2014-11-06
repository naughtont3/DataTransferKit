//---------------------------------------------------------------------------//
/*
  Copyright (c) 2014, Stuart R. Slattery
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are
  met:

  *: Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.

  *: Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.

  *: Neither the name of the Oak Ridge National Laboratory nor the
  names of its contributors may be used to endorse or promote products
  derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
  HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
//---------------------------------------------------------------------------//
/*!
 * \brief DTK_MapOperator_impl.hpp
 * \author Stuart R. Slattery
 * \brief Map operator interface.
 */
//---------------------------------------------------------------------------//

#ifndef DTK_MAPOPERATOR_IMPL_HPP
#define DTK_MAPOPERATOR_IMPL_HPP

#include "DTK_DBC.hpp"

#include <Thyra_MultiVectorStdOps.hpp>

namespace DataTransferKit
{
//---------------------------------------------------------------------------//
// Constructor.
template<class Scalar>
MapOperator<Scalar>::MapOperator()
{ /* ... */ }

//---------------------------------------------------------------------------//
// Destructor.
template<class Scalar>
MapOperator<Scalar>::~MapOperator()
{ /* ... */ }

//---------------------------------------------------------------------------//
// Setup the map operator.
template<class Scalar>
void MapOperator<Scalar>::setup(
    const Teuchos::RCP<FunctionSpace>& domain_space,
    const Teuchos::RCP<FunctionSpace>& range_space,
    const Teuchos::RCP<Teuchos::ParameterList>& parameters )
{
    bool not_implemented = true;
    DTK_INSIST( !not_implemented );
}

//---------------------------------------------------------------------------//
// Get the range space.
template<class Scalar>
Teuchos::RCP<const Thyra::VectorSpaceBase<Scalar> > 
MapOperator<Scalar>::range() const
{
    DTK_REQUIRE( Teuchos::nonnull(b_coupling_matrix) );
    if ( Teuchos::nonnull(b_mass_matrix_inv) )
    {
	return b_mass_matrix_inv->range();
    }
    return b_coupling_matrix->range();
}

//---------------------------------------------------------------------------//
// Get the domain space.
template<class Scalar>
Teuchos::RCP<const Thyra::VectorSpaceBase<Scalar> > 
MapOperator<Scalar>::domain() const
{
    DTK_REQUIRE( Teuchos::nonnull(b_coupling_matrix) );
    return b_coupling_matrix->domain();
}

//---------------------------------------------------------------------------//
// Clone the operator. The subclass must implement this function.
template<class Scalar>
Teuchos::RCP<const Thyra::LinearOpBase<Scalar> > 
MapOperator<Scalar>::clone() const
{
    bool not_implemented = true;
    DTK_INSIST( !not_implemented );
    return Teuchos::null;
}
 
//---------------------------------------------------------------------------//
// Check if the given operator is supported.
template<class Scalar>
bool MapOperator<Scalar>::opSupportedImpl( Thyra::EOpTransp M_trans ) const
{
    return ( M_trans == Thyra::NOTRANS );
}

//---------------------------------------------------------------------------//
// Apply the map operator to data defined on the entities by computing g =
// Minv*(v+A*f).
template<class Scalar>
void MapOperator<Scalar>::applyImpl( 
    const Thyra::EOpTransp M_trans,
    const Thyra::MultiVectorBase<Scalar>& domain_dofs,
    const Teuchos::Ptr<Thyra::MultiVectorBase<Scalar> >& range_dofs,
    const Scalar alpha,
    const Scalar beta ) const
{
    DTK_INSIST( opSupportedImpl(M_trans) );
    DTK_REQUIRE( Teuchos::nonnull(b_coupling_matrix) );
    DTK_REQUIRE( Teuchos::nonnull(range_dofs) );
  
    // Make a temporary copy of the range dofs if beta is not zero.
    Teuchos::RCP<Thyra::MultiVectorBase<Scalar> > range_copy;
    if ( 0.0 != beta )
    {
	range_copy = range_dofs->clone_mv();
	Thyra::assign( range_copy.ptr(), *range_dofs );
    }

    // Make a work vector.
    Teuchos::RCP<Thyra::MultiVectorBase<Scalar> > work = 
	Thyra::createMembers( b_coupling_matrix->range(),
			      domain_dofs.domain()->dim() );

    // A*f
    DTK_CHECK( domain_dofs.range()->isCompatible(
		   *(b_coupling_matrix->domain()) ) );
    b_coupling_matrix->apply( 
	Thyra::NOTRANS, domain_dofs, work.ptr(), 1.0, 0.0 );

    // v+A*f
    if ( Teuchos::nonnull(b_forcing_vector) )
    {
	Thyra::Vp_V( work.ptr(), *b_forcing_vector );
    }

    // Minv*(v+A*f)
    if ( Teuchos::nonnull(b_mass_matrix_inv) )
    {
	DTK_CHECK( work->range()->isCompatible(
		       *(b_mass_matrix_inv->domain()) ) );
	b_mass_matrix_inv->apply( 
	    Thyra::NOTRANS, *work, range_dofs, 1.0, 0.0 );
    }
    else
    {
	Thyra::assign( range_dofs, *work );
    }

    // g = alpha*g + beta*f
    Thyra::Vt_S( range_dofs, alpha );
    if ( 0.0 != beta )
    {
	Thyra::update( beta, *range_copy, range_dofs );
    }
}

//---------------------------------------------------------------------------//

} // end namespace DataTransferKit

# endif // end DTK_MAPOPERATOR_IMPL_HPP

//---------------------------------------------------------------------------//
// end DTK_MapOperator_impl.hpp
//---------------------------------------------------------------------------//
