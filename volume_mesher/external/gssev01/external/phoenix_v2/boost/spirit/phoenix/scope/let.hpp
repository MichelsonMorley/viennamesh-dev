/*=============================================================================
    Copyright (c) 2001-2004 Joel de Guzman
    Copyright (c) 2004 Daniel Wallin

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef PHOENIX_SCOPE_LET_HPP
#define PHOENIX_SCOPE_LET_HPP

#if !defined(PHOENIX_LOCAL_LIMIT)
# define PHOENIX_LOCAL_LIMIT PHOENIX_LIMIT
#endif

#include <boost/spirit/phoenix/scope/scoped_environment.hpp>
#include <boost/spirit/phoenix/scope/detail/local_variable.hpp>
#include <boost/spirit/phoenix/core/actor.hpp>
#include <boost/spirit/fusion/algorithm/transform.hpp>
#include <boost/spirit/fusion/sequence/generate.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/bool.hpp>

namespace boost { namespace phoenix
{
    template <typename Base, typename Vars, typename Map>
    struct let_actor : Base
    {
        typedef typename
            mpl::fold<
                Vars
              , mpl::false_
              , detail::compute_no_nullary
            >::type
        no_nullary;
        
        template <typename Env>
        struct apply
        {
            typedef typename 
                fusion::meta::generate<
                    typename mpl::apply2<
                        fusion::function::transform
                      , Vars
                      , detail::initialize_local<Env>
                    >::type
                >::type 
            locals_type;

            typedef typename Base::template
                apply<scoped_environment<Env, Env, locals_type, Map> >::type
            result;
            
            typedef typename 
                detail::unwrap_local_reference<result>::type 
            type;
        };

        let_actor(Base const& base, Vars const& vars)
            : Base(base), vars(vars) {}

        template <typename Env>
        typename apply<Env>::type
        eval(Env const& env) const
        {
            typedef typename 
                fusion::meta::generate<
                    typename mpl::apply2<
                        fusion::function::transform
                      , Vars
                      , detail::initialize_local<Env>
                    >::type
                >::type 
            locals_type;

            locals_type locals = 
                fusion::generate(
                    fusion::transform(
                        vars
                      , detail::initialize_local<Env>(env)));
            
            typedef typename apply<Env>::type RT;
            return RT(Base::eval(
                scoped_environment<Env, Env, locals_type, Map>(
                    env
                  , env
                  , locals)));
        }

        Vars vars;
    };
    
    template <typename Vars, typename Map>
    struct let_actor_gen
    {
        template <typename Base>
        actor<let_actor<Base, Vars, Map> > const
        operator[](actor<Base> const& base) const
        {
            return let_actor<Base, Vars, Map>(base, vars);
        }

        let_actor_gen(Vars const& vars)
            : vars(vars) {}

        Vars vars;
    };

    template <typename Key>
    struct local_variable; // forward
    struct assign_eval; // forward

    struct let_gen
    {
        template <typename K0, typename V0>
        let_actor_gen<
            fusion::tuple<V0>
          , detail::map_local_index_to_tuple<K0>
        >
        operator()(
            actor<composite<assign_eval, fusion::tuple<local_variable<K0>, V0> > > const& a0
        ) const
        {
            return fusion::tuple<V0>(fusion::get<1>(a0));
        }
    
        template <typename K0, typename K1, typename V0, typename V1>
        let_actor_gen<
            fusion::tuple<V0, V1>
          , detail::map_local_index_to_tuple<K0, K1>
        >
        operator()(
            actor<composite<assign_eval, fusion::tuple<local_variable<K0>, V0> > > const& a0
          , actor<composite<assign_eval, fusion::tuple<local_variable<K1>, V1> > > const& a1
        ) const
        {
            return fusion::tuple<V0, V1>(fusion::get<1>(a0), fusion::get<1>(a1));
        }
        
        //~ // Bring in the rest...
        #define PHOENIX_LOCAL_GEN_NAME let_actor_gen
        #include <boost/spirit/phoenix/scope/detail/local_gen.hpp>
        #undef PHOENIX_LOCAL_GEN_NAME
    };

    let_gen const let = let_gen();
}}

#endif
