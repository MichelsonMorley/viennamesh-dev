/*=============================================================================
    Copyright (c) 2001-2006 Joel de Guzman

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef BOOST_PP_IS_ITERATING
#if !defined(FUSION_VECTOR_TIE_07192005_1242)
#define FUSION_VECTOR_TIE_07192005_1242

#include <boost/preprocessor/iterate.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>
#include <boost/preprocessor/repetition/enum_params_with_a_default.hpp>
#include <boost/fusion/sequence/container/vector/vector.hpp>

namespace boost { namespace fusion
{
    struct void_;
        
    namespace result_of
    {
        template <
            BOOST_PP_ENUM_PARAMS_WITH_A_DEFAULT(
                FUSION_MAX_VECTOR_SIZE, typename T, void_)
          , typename Extra = void_
        >
        struct vector_tie;
    }

#define BOOST_FUSION_REF(z, n, data) BOOST_PP_CAT(T, n)&

#define BOOST_PP_FILENAME_1 <boost/fusion/sequence/generation/vector_tie.hpp>
#define BOOST_PP_ITERATION_LIMITS (1, FUSION_MAX_VECTOR_SIZE)
#include BOOST_PP_ITERATE()
    
#undef BOOST_FUSION_REF

}}

#endif
#else // defined(BOOST_PP_IS_ITERATING)
///////////////////////////////////////////////////////////////////////////////
//
//  Preprocessor vertical repetition code
//
///////////////////////////////////////////////////////////////////////////////

#define N BOOST_PP_ITERATION()

    namespace result_of
    {
        template <BOOST_PP_ENUM_PARAMS(N, typename T)>
        struct vector_tie<BOOST_PP_ENUM_PARAMS(N, T)>
        {
            typedef vector<BOOST_PP_ENUM(N, BOOST_FUSION_REF, _)> type;
        };
    }

    template <BOOST_PP_ENUM_PARAMS(N, typename T)>
    inline vector<BOOST_PP_ENUM(N, BOOST_FUSION_REF, _)>
    vector_tie(BOOST_PP_ENUM_BINARY_PARAMS(N, T, & _))
    {
        return vector<BOOST_PP_ENUM(N, BOOST_FUSION_REF, _)>(
            BOOST_PP_ENUM_PARAMS(N, _));
    }

#undef N
#endif // defined(BOOST_PP_IS_ITERATING)

