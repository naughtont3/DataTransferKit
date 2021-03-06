/****************************************************************************
 * Copyright (c) 2012-2017 by the DataTransferKit authors                   *
 * All rights reserved.                                                     *
 *                                                                          *
 * This file is part of the DataTransferKit library. DataTransferKit is     *
 * distributed under a BSD 3-clause license. For the licensing terms see    *
 * the LICENSE file in the top-level directory.                             *
 ****************************************************************************/

#ifndef DTK_LINEAR_BVH_DECL_HPP
#define DTK_LINEAR_BVH_DECL_HPP

#include <Kokkos_ArithTraits.hpp>
#include <Kokkos_Array.hpp>
#include <Kokkos_View.hpp>

#include <DTK_DetailsAlgorithms.hpp>
#include <DTK_DetailsBox.hpp>
#include <DTK_DetailsNode.hpp>
#include <DTK_DetailsPredicate.hpp>
#include <DTK_DetailsTreeTraversal.hpp>
#include <DTK_DetailsUtils.hpp>

#include "DTK_ConfigDefs.hpp"

namespace DataTransferKit
{

/**
 * Bounding Volume Hierarchy.
 */
template <typename DeviceType>
class BVH
{
  public:
    BVH( Kokkos::View<Box const *, DeviceType> bounding_boxes );

    // Views are passed by reference here because internally Kokkos::realloc()
    // is called.
    template <typename Query>
    void query( Kokkos::View<Query *, DeviceType> queries,
                Kokkos::View<int *, DeviceType> &indices,
                Kokkos::View<int *, DeviceType> &offset ) const;
    template <typename Query>
    typename std::enable_if<
        std::is_same<typename Query::Tag, Details::NearestPredicateTag>::value,
        void>::type
    query( Kokkos::View<Query *, DeviceType> queries,
           Kokkos::View<int *, DeviceType> &indices,
           Kokkos::View<int *, DeviceType> &offset,
           Kokkos::View<double *, DeviceType> &distances ) const;

    KOKKOS_INLINE_FUNCTION
    Box bounds() const
    {
        if ( empty() )
            return Box();
        return ( size() > 1 ? _internal_nodes : _leaf_nodes )[0].bounding_box;
    }

    using SizeType = typename Kokkos::View<int *, DeviceType>::size_type;
    KOKKOS_INLINE_FUNCTION
    SizeType size() const { return _leaf_nodes.extent( 0 ); }

    KOKKOS_INLINE_FUNCTION
    bool empty() const { return size() == 0; }

  private:
    friend struct Details::TreeTraversal<DeviceType>;

    Kokkos::View<Node *, DeviceType> _leaf_nodes;
    Kokkos::View<Node *, DeviceType> _internal_nodes;
    /**
     * Array of indices that sort the boxes used to construct the hierarchy.
     * The leaf nodes are ordered so we need these to identify objects that
     * meet a predicate.
     */
    Kokkos::View<int *, DeviceType> _indices;
};

template <typename DeviceType, typename Query>
void queryDispatch(
    BVH<DeviceType> const bvh, Kokkos::View<Query *, DeviceType> queries,
    Kokkos::View<int *, DeviceType> &indices,
    Kokkos::View<int *, DeviceType> &offset, Details::NearestPredicateTag,
    Kokkos::View<double *, DeviceType> *distances_ptr = nullptr )
{
    using ExecutionSpace = typename DeviceType::execution_space;

    int const n_queries = queries.extent( 0 );

    Kokkos::realloc( offset, n_queries + 1 );
    fill( offset, 0 );

    Kokkos::parallel_for(
        REGION_NAME( "scan_queries_for_numbers_of_nearest_neighbors" ),
        Kokkos::RangePolicy<ExecutionSpace>( 0, n_queries ),
        KOKKOS_LAMBDA( int i ) { offset( i ) = queries( i )._k; } );
    Kokkos::fence();

    exclusivePrefixSum( offset );
    int const n_results = lastElement( offset );

    Kokkos::realloc( indices, n_results );
    fill( indices, -1 );
    if ( distances_ptr )
    {
        Kokkos::View<double *, DeviceType> &distances = *distances_ptr;
        Kokkos::realloc( distances, n_results );
        fill( distances, Kokkos::ArithTraits<double>::max() );

        Kokkos::parallel_for(
            REGION_NAME( "perform_nearest_queries_and_return_distances" ),
            Kokkos::RangePolicy<ExecutionSpace>( 0, n_queries ),
            KOKKOS_LAMBDA( int i ) {
                int count = 0;
                Details::TreeTraversal<DeviceType>::query(
                    bvh, queries( i ), [indices, offset, distances, i,
                                        &count]( int index, double distance ) {
                        indices( offset( i ) + count ) = index;
                        distances( offset( i ) + count ) = distance;
                        count++;
                    } );
            } );
        Kokkos::fence();
    }
    else
    {
        Kokkos::parallel_for(
            REGION_NAME( "perform_nearest_queries" ),
            Kokkos::RangePolicy<ExecutionSpace>( 0, n_queries ),
            KOKKOS_LAMBDA( int i ) {
                int count = 0;
                Details::TreeTraversal<DeviceType>::query(
                    bvh, queries( i ),
                    [indices, offset, i, &count]( int index, double distance ) {
                        indices( offset( i ) + count++ ) = index;
                    } );
            } );
        Kokkos::fence();
    }
    // NOTE: possible improvement is to find out if they are any -1 in indices
    // (resp. +infty in distances) and truncate if necessary
}

template <typename DeviceType, typename Query>
void queryDispatch( BVH<DeviceType> const bvh,
                    Kokkos::View<Query *, DeviceType> queries,
                    Kokkos::View<int *, DeviceType> &indices,
                    Kokkos::View<int *, DeviceType> &offset,
                    Details::SpatialPredicateTag )
{
    using ExecutionSpace = typename DeviceType::execution_space;

    int const n_queries = queries.extent( 0 );

    // Initialize view
    // [ 0 0 0 .... 0 0 ]
    //                ^
    //                N
    Kokkos::realloc( offset, n_queries + 1 );
    fill( offset, 0 );

    // Say we found exactly two object for each query:
    // [ 2 2 2 .... 2 0 ]
    //   ^            ^
    //   0th          Nth element in the view
    Kokkos::parallel_for(
        REGION_NAME( "first_pass_at_the_search_count_the_number_of_indices" ),
        Kokkos::RangePolicy<ExecutionSpace>( 0, n_queries ),
        KOKKOS_LAMBDA( int i ) {
            offset( i ) = Details::TreeTraversal<DeviceType>::query(
                bvh, queries( i ), []( int index ) {} );
        } );
    Kokkos::fence();

    // Then we would get:
    // [ 0 2 4 .... 2N-2 2N ]
    //                    ^
    //                    N
    exclusivePrefixSum( offset );

    // Let us extract the last element in the view which is the total count of
    // objects which where found to meet the query predicates:
    //
    // [ 2N ]
    int const n_results = lastElement( offset );
    // We allocate the memory and fill
    //
    // [ A0 A1 B0 B1 C0 C1 ... X0 X1 ]
    //   ^     ^     ^         ^     ^
    //   0     2     4         2N-2  2N
    Kokkos::realloc( indices, n_results );
    Kokkos::parallel_for( REGION_NAME( "second_pass" ),
                          Kokkos::RangePolicy<ExecutionSpace>( 0, n_queries ),
                          KOKKOS_LAMBDA( int i ) {
                              int count = 0;
                              Details::TreeTraversal<DeviceType>::query(
                                  bvh, queries( i ),
                                  [indices, offset, i, &count]( int index ) {
                                      indices( offset( i ) + count++ ) = index;
                                  } );
                          } );
    Kokkos::fence();
}

template <typename DeviceType>
template <typename Query>
void BVH<DeviceType>::query( Kokkos::View<Query *, DeviceType> queries,
                             Kokkos::View<int *, DeviceType> &indices,
                             Kokkos::View<int *, DeviceType> &offset ) const
{
    using Tag = typename Query::Tag;
    queryDispatch( *this, queries, indices, offset, Tag{} );
}

template <typename DeviceType>
template <typename Query>
typename std::enable_if<
    std::is_same<typename Query::Tag, Details::NearestPredicateTag>::value,
    void>::type
BVH<DeviceType>::query( Kokkos::View<Query *, DeviceType> queries,
                        Kokkos::View<int *, DeviceType> &indices,
                        Kokkos::View<int *, DeviceType> &offset,
                        Kokkos::View<double *, DeviceType> &distances ) const
{
    using Tag = typename Query::Tag;
    queryDispatch( *this, queries, indices, offset, Tag{}, &distances );
}

} // end namespace DataTransferKit

#endif
