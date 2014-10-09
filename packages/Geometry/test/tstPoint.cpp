//---------------------------------------------------------------------------//
/*!
 * \file tstPoint.cpp
 * \author Stuart R. Slattery
 * \brief Point unit tests.
 */
//---------------------------------------------------------------------------//

#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <sstream>
#include <algorithm>
#include <cassert>

#include <DTK_Point.hpp>
#include <DTK_Entity.hpp>
#include <DTK_Box.hpp>
#include <DTK_AbstractObjectRegistry.hpp>

#include <Teuchos_UnitTestHarness.hpp>
#include <Teuchos_DefaultComm.hpp>
#include <Teuchos_CommHelpers.hpp>
#include <Teuchos_RCP.hpp>
#include <Teuchos_ArrayRCP.hpp>
#include <Teuchos_Array.hpp>
#include <Teuchos_OpaqueWrapper.hpp>
#include <Teuchos_TypeTraits.hpp>
#include <Teuchos_Tuple.hpp>
#include <Teuchos_AbstractFactoryStd.hpp>

//---------------------------------------------------------------------------//
// MPI Setup
//---------------------------------------------------------------------------//

template<class Ordinal>
Teuchos::RCP<const Teuchos::Comm<Ordinal> > getDefaultComm()
{
#ifdef HAVE_MPI
    return Teuchos::DefaultComm<Ordinal>::getComm();
#else
    return Teuchos::rcp(new Teuchos::SerialComm<Ordinal>() );
#endif
}

//---------------------------------------------------------------------------//
// Tests
//---------------------------------------------------------------------------//
// Array constructor 1d test.
TEUCHOS_UNIT_TEST( Point, array_1d_constructor_test )
{
    using namespace DataTransferKit;

    // Make point.
    double x = 3.2;
    Teuchos::Array<double> p(1);
    p[0] = x;
    Point<1> point(  0, 0, p );

    // Check Entity data.
    TEST_EQUALITY( point.name(), "DTK Point" );
    TEST_EQUALITY( point.entityType(), NODE );
    TEST_EQUALITY( point.id(), 0 );
    TEST_EQUALITY( point.ownerRank(), 0 );
    TEST_EQUALITY( point.physicalDimension(), 1 );
    TEST_EQUALITY( point.parametricDimension(), 0 );

    // Check the bounds.
    Teuchos::ArrayView<const double> point_coords;
    point.getCoordinates( point_coords );
    TEST_EQUALITY( point_coords[0], x );

    point.centroid( point_coords );
    TEST_EQUALITY( point_coords[0], x );

    // Compute the measure.
    TEST_EQUALITY( point.measure(), 0.0 );
}

//---------------------------------------------------------------------------//
// Array constructor 2d test.
TEUCHOS_UNIT_TEST( Point, array_2d_constructor_test )
{
    using namespace DataTransferKit;

    // Make point.
    double x = 3.2;
    double y = -9.233;
    Teuchos::Array<double> p(2);
    p[0] = x;
    p[1] = y;
    Point<2> point(  0, 0, p );

    // Check Entity data.
    TEST_EQUALITY( point.name(), "DTK Point" );
    TEST_EQUALITY( point.entityType(), NODE );
    TEST_EQUALITY( point.id(), 0 );
    TEST_EQUALITY( point.ownerRank(), 0 );
    TEST_EQUALITY( point.physicalDimension(), 2 );
    TEST_EQUALITY( point.parametricDimension(), 0 );

    // Check the bounds.
    Teuchos::ArrayView<const double> point_coords;
    point.getCoordinates( point_coords );
    TEST_EQUALITY( point_coords[0], x );
    TEST_EQUALITY( point_coords[1], y );

    point.centroid( point_coords );
    TEST_EQUALITY( point_coords[0], x );
    TEST_EQUALITY( point_coords[1], y );

    // Compute the measure.
    TEST_EQUALITY( point.measure(), 0.0 );
}

//---------------------------------------------------------------------------//
// Array constructor 3d test.
TEUCHOS_UNIT_TEST( Point, array_3d_constructor_test )
{
    using namespace DataTransferKit;

    // Make point.
    double x = 3.2;
    double y = -9.233;
    double z = 1.3;
    Teuchos::Array<double> p(3);
    p[0] = x;
    p[1] = y;
    p[2] = z;
    Point<3> point(  0, 0, p );

    // Check Entity data.
    TEST_EQUALITY( point.name(), "DTK Point" );
    TEST_EQUALITY( point.entityType(), NODE );
    TEST_EQUALITY( point.id(), 0 );
    TEST_EQUALITY( point.ownerRank(), 0 );
    TEST_EQUALITY( point.physicalDimension(), 3 );
    TEST_EQUALITY( point.parametricDimension(), 0 );

    // Check the bounds.
    Teuchos::ArrayView<const double> point_coords;
    point.getCoordinates( point_coords );
    TEST_EQUALITY( point_coords[0], x );
    TEST_EQUALITY( point_coords[1], y );
    TEST_EQUALITY( point_coords[2], z );

    point.centroid( point_coords );
    TEST_EQUALITY( point_coords[0], x );
    TEST_EQUALITY( point_coords[1], y );
    TEST_EQUALITY( point_coords[2], z );

    // Compute the measure.
    TEST_EQUALITY( point.measure(), 0.0 );
}

//---------------------------------------------------------------------------//
TEUCHOS_UNIT_TEST( Point, communication_test )
{
    using namespace DataTransferKit;

    // Register the point class to use the abstract compile-time interfaces.
    AbstractObjectRegistry<Entity,Point<3> >::registerDerivedClasses();

    // Get the communicator.
    Teuchos::RCP<const Teuchos::Comm<int> > comm = 
	Teuchos::DefaultComm<int>::getComm();
    int comm_rank = comm->getRank();
    int comm_size = comm->getSize();

    // Make a point.
    double x = 3.2;
    double y = -9.233;
    double z = 1.3;
    Teuchos::Array<double> p(3);
    p[0] = x;
    p[1] = y;
    p[2] = z;
    Entity entity;
    if ( 0 == comm_rank )
    {
	entity = Point<3>(0, 0, p);
    }

    // Broadcast the point with indirect serialization through the geometric
    // entity api.
    Teuchos::broadcast( *comm, 0, Teuchos::Ptr<Entity>(&entity) );

    // Check the coordinates.
    Teuchos::ArrayView<const double> coords;
    entity.centroid( coords );
    TEST_EQUALITY( coords.size(), 3 );
    TEST_EQUALITY( coords[0], x );
    TEST_EQUALITY( coords[1], y );
    TEST_EQUALITY( coords[2], z );

    // Broadcast an array.
    Teuchos::Array<Entity> points(2);
    double x_1 = 3.2 + comm_size;
    double y_1 = -9.233 + comm_size;
    double z_1 = 1.3 + comm_size;
    Teuchos::Array<double> p1(3);
    p1[0] = x_1;
    p1[1] = y_1;
    p1[2] = z_1;
    double x_2 = 3.2 - comm_size;
    double y_2 = -9.233 - comm_size;
    double z_2 = 1.3 - comm_size;
    Teuchos::Array<double> p2(3);
    p2[0] = x_2;
    p2[1] = y_2;
    p2[2] = z_2;
    if ( 0 == comm->getRank() )
    {
	points[0] = Point<3>(0, 0, p1);
	points[1] = Point<3>(1, 0, p2);
    }
    Teuchos::broadcast( *comm, 0, points() );
    points[0].centroid( coords );
    TEST_EQUALITY( coords[0], x_1 );
    TEST_EQUALITY( coords[1], y_1 );
    TEST_EQUALITY( coords[2], z_1 );
    points[1].centroid( coords );
    TEST_EQUALITY( coords[0], x_2 );
    TEST_EQUALITY( coords[1], y_2 );
    TEST_EQUALITY( coords[2], z_2 );
}

//---------------------------------------------------------------------------//
// end tstPoint.cpp
//---------------------------------------------------------------------------//

