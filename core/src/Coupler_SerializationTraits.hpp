//----------------------------------*-C++-*----------------------------------//
/*!
 * \file   Coupler_SerializationTraits.hpp
 * \author Stuart Slattery
 * \date   Fri Dec 09 09:25:58 2011
 * \brief  Serialization traits for coupler objects.
 */
//---------------------------------------------------------------------------//

#ifndef COUPLER_SERIALIZATIONTRAITS_HPP
#define COUPLER_SERIALIZATIONTRAITS_HPP

#include "Coupler_Point.hpp"
#include "Coupler_BoundingBox.hpp"

#include <Teuchos_SerializationTraits.hpp>

namespace Teuchos 
{
//===========================================================================//
/*!
 * \brief Serialization traits for coupler objects.
 */
//===========================================================================//

// Point.
template<typename Ordinal>
class SerializationTraits<Ordinal,Coupler::Point<int,double> >
    : public DirectSerializationTraits<Ordinal,Coupler::Point<int,double> >
{};

template<typename Ordinal>
class SerializationTraits<Ordinal,Coupler::Point<int,float> >
    : public DirectSerializationTraits<Ordinal,Coupler::Point<int,float> >
{};

template<typename Ordinal>
class SerializationTraits<Ordinal,Coupler::Point<long int,double> >
    : public DirectSerializationTraits<Ordinal,Coupler::Point<long int,double> >
{};

template<typename Ordinal>
class SerializationTraits<Ordinal,Coupler::Point<long int,float> >
    : public DirectSerializationTraits<Ordinal,Coupler::Point<long int,float> >
{};

// Bounding box.
template<typename Ordinal>
class SerializationTraits<Ordinal,Coupler::BoundingBox<int,double> >
    : public DirectSerializationTraits<Ordinal,Coupler::BoundingBox<int,double> >
{};

template<typename Ordinal>
class SerializationTraits<Ordinal,Coupler::BoundingBox<int,float> >
    : public DirectSerializationTraits<Ordinal,Coupler::BoundingBox<int,float> >
{};

template<typename Ordinal>
class SerializationTraits<Ordinal,Coupler::BoundingBox<long int,double> >
    : public DirectSerializationTraits<Ordinal,Coupler::BoundingBox<long int,double> >
{};

template<typename Ordinal>
class SerializationTraits<Ordinal,Coupler::BoundingBox<long int,float> >
    : public DirectSerializationTraits<Ordinal,Coupler::BoundingBox<long int,float> >
{};

} // end namepsace Teuchos

#endif // COUPLER_SERIALIZATIONTRAITS_HPP

//---------------------------------------------------------------------------//
//              end of Coupler_SerializationTraits.hpp
//---------------------------------------------------------------------------//
