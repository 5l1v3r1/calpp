/*
 * C++ to IL compiler/generator multimedia functors
 * 
 * Copyright (C) 2010, 2011 Artur Kornacki
 *
 * This file is part of CAL++.
 *
 * CAL++ is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * CAL++ is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with CAL++.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef __CAL_IL_FUNCTORS_MULTIMEDIA_H
#define __CAL_IL_FUNCTORS_MULTIMEDIA_H

#include <boost/static_assert.hpp>
#include <boost/type_traits.hpp>
#include <boost/format.hpp>

namespace cal {
namespace il {
namespace detail {

//
// bitalign
//

template<class S1, class S2, class S3>
struct cal_ternary_bitalign 
{
    typedef S1 value_type;
    static const int temp_reg_count=0;

    static std::string emitCode( const std::string& r, const std::string& s0, const std::string& s1, const std::string& s2, int t0 )
    {
        typedef boost::is_same<S1,S2> assert_value;
        BOOST_STATIC_ASSERT( assert_value::value );
        BOOST_STATIC_ASSERT( S1::type_size==S3::type_size ); 
        BOOST_STATIC_ASSERT( S1::component_count==S3::component_count );

        std::string    res;

        for(int i=1;i<=S1::component_count;i++) {
            res += (boost::format("bitalign %1%,%2%,%3%,%4%\n") % mask_output(make_swizzle(r,i,0,0,0)) 
                                                                % make_swizzle(s0,i,0,0,0) 
                                                                % make_swizzle(s1,i,0,0,0) 
                                                                % make_swizzle(s2,i,0,0,0) ).str();
        }

        return res;
    }
};

//
// bytealign
//

template<class S1, class S2, class S3>
struct cal_ternary_bytealign 
{
    typedef S1 value_type;
    static const int temp_reg_count=0;

    static std::string emitCode( const std::string& r, const std::string& s0, const std::string& s1, const std::string& s2, int t0 )
    {
        typedef boost::is_same<S1,S2> assert_value;
        BOOST_STATIC_ASSERT( assert_value::value );
        BOOST_STATIC_ASSERT( S1::type_size==S3::type_size ); 
        BOOST_STATIC_ASSERT( S1::component_count==S3::component_count );

        std::string    res;

        for(int i=1;i<=S1::component_count;i++) {
            res += (boost::format("bytealign %1%,%2%,%3%,%4%\n") % mask_output(make_swizzle(r,i,0,0,0)) 
                                                                 % make_swizzle(s0,i,0,0,0) 
                                                                 % make_swizzle(s1,i,0,0,0) 
                                                                 % make_swizzle(s2,i,0,0,0) ).str();
        }

        return res;
    }
};

//
// bfi
//

template<class S1, class S2, class S3>
struct cal_ternary_bfi 
{
    typedef S1 value_type;
    static const int temp_reg_count=0;

    static std::string emitCode( const std::string& r, const std::string& s0, const std::string& s1, const std::string& s2, int t0 )
    {
        typedef boost::is_same<S1,S2> assert_value;
        BOOST_STATIC_ASSERT( assert_value::value );
        BOOST_STATIC_ASSERT( S1::type_size==S3::type_size ); 
        BOOST_STATIC_ASSERT( S1::component_count==S3::component_count );

        return (boost::format("bfi %1%,%2%,%3%,%4%\n") % r  % s0 % s1 % s2).str();
    }
};

//
// bitextract
// 

template<class S1, class S2, class S3>
struct cal_ternary_bitextract 
{
    typedef S1 value_type;
    static const int temp_reg_count=0;

    static std::string emitCode( const std::string& r, const std::string& s0, const std::string& s1, const std::string& s2, int t0 )
    {
        BOOST_STATIC_ASSERT(sizeof(S1) != sizeof(S1));
        return std::string();
    }
};

template<>
struct cal_ternary_bitextract<uint_type,uint_type,uint_type>
{
    typedef uint_type value_type;
    static const int temp_reg_count=0;

    static std::string emitCode( const std::string& r, const std::string& s0, const std::string& s1, const std::string& s2, int t0 )
    {
        return (boost::format("ubit_extract %1%,%2%,%3%,%4%\n") % r % s0 % s1 % s2).str();
    }
};

template<>
struct cal_ternary_bitextract<uint2_type,uint2_type,uint2_type>
{
    typedef uint2_type value_type;
    static const int temp_reg_count=0;

    static std::string emitCode( const std::string& r, const std::string& s0, const std::string& s1, const std::string& s2, int t0 )
    {
        return (boost::format("ubit_extract %1%,%2%,%3%,%4%\n") % r % s0 % s1 % s2).str();
    }
};

template<>
struct cal_ternary_bitextract<uint4_type,uint4_type,uint4_type>
{
    typedef uint4_type value_type;
    static const int temp_reg_count=0;

    static std::string emitCode( const std::string& r, const std::string& s0, const std::string& s1, const std::string& s2, int t0 )
    {
        return (boost::format("ubit_extract %1%,%2%,%3%,%4%\n") % r % s0 % s1 % s2).str();
    }
};

template<>
struct cal_ternary_bitextract<int_type,uint_type,uint_type>
{
    typedef int_type value_type;
    static const int temp_reg_count=0;

    static std::string emitCode( const std::string& r, const std::string& s0, const std::string& s1, const std::string& s2, int t0 )
    {
        return (boost::format("ibit_extract %1%,%2%,%3%,%4%\n") % r % s0 % s1 % s2).str();
    }
};

template<>
struct cal_ternary_bitextract<int2_type,uint2_type,uint2_type>
{
    typedef int2_type value_type;
    static const int temp_reg_count=0;

    static std::string emitCode( const std::string& r, const std::string& s0, const std::string& s1, const std::string& s2, int t0 )
    {
        return (boost::format("ibit_extract %1%,%2%,%3%,%4%\n") % r % s0 % s1 % s2).str();
    }
};

template<>
struct cal_ternary_bitextract<int4_type,uint4_type,uint4_type>
{
    typedef int4_type value_type;
    static const int temp_reg_count=0;

    static std::string emitCode( const std::string& r, const std::string& s0, const std::string& s1, const std::string& s2, int t0 )
    {
        return (boost::format("ibit_extract %1%,%2%,%3%,%4%\n") % r % s0 % s1 % s2).str();
    }
};

} // detail
} // cal
} // il

#endif
