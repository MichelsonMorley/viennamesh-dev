/*=============================================================================
    Copyright (c) 2001-2006 Joel de Guzman
    Copyright (c) 2006 Dan Marsden

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(FUSION_PRIOR_IMPL_20060124_2006)
#define FUSION_PRIOR_IMPL_20060124_2006

#include <boost/fusion/sequence/view/zip_view/zip_view_iterator_fwd.hpp>
#include <boost/fusion/iterator/prior.hpp>
#include <boost/fusion/algorithm/transformation/transform.hpp>

namespace boost { namespace fusion {

    struct zip_view_iterator_tag;
    
    namespace detail
    {
        struct poly_prior
        {
            template<typename It>
            struct result
                : result_of::prior<It>
            {};

            template<typename It>
            typename result<It>::type
            operator()(const It& it) const
            {
                return fusion::prior(it);
            }
        };
    }

    namespace extension
    {
        template<typename Tag>
        struct prior_impl;

        template<>
        struct prior_impl<zip_view_iterator_tag>
        {
            template<typename Iterator>
            struct apply
            {
                typedef zip_view_iterator<
                    typename result_of::transform<typename Iterator::iterators, detail::poly_prior>::type,
                    typename Iterator::category> type;

                static type
                call(Iterator const& it)

                {
                    return type(
                        fusion::transform(it.iterators_, detail::poly_prior()));
                }
            };
        };
    }
}}

#endif
