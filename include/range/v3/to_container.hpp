/// \file
// Range v3 library
//
//  Copyright Eric Niebler 2013-2014
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// Project home: https://github.com/ericniebler/range-v3
//

#ifndef RANGES_V3_TO_CONTAINER_HPP
#define RANGES_V3_TO_CONTAINER_HPP

#include <range/v3/range_fwd.hpp>
#include <range/v3/range_concepts.hpp>
#include <range/v3/range_traits.hpp>
#include <range/v3/utility/meta.hpp>
#include <range/v3/utility/common_iterator.hpp>
#include <range/v3/utility/static_const.hpp>

#ifndef RANGES_NO_STD_FORWARD_DECLARATIONS
// Non-portable forward declarations of standard containers
RANGES_BEGIN_NAMESPACE_STD
    template<class Value, class Alloc /*= allocator<Value>*/>
    class vector;
RANGES_END_NAMESPACE_STD
#else
#include <vector>
#endif

namespace ranges
{
    inline namespace v3
    {
        /// \cond
        namespace detail
        {
            template<typename Rng, typename Cont, typename I = range_common_iterator_t<Rng>>
            using ConvertibleToContainer = meta::fast_and<
                Iterable<Cont>,
                meta::not_<Range<Cont>>,
                Movable<Cont>,
                Convertible<range_value_t<Rng>, range_value_t<Cont>>,
                Constructible<Cont, I, I>>;

            template<typename ContainerMetafunctionClass>
            struct to_container_fn
              : pipeable<to_container_fn<ContainerMetafunctionClass>>
            {
                template<typename Rng,
                    typename Cont = meta::apply<ContainerMetafunctionClass, range_value_t<Rng>>,
                    CONCEPT_REQUIRES_(Iterable<Rng>() && detail::ConvertibleToContainer<Rng, Cont>())>
                Cont operator()(Rng && rng) const
                {
                    static_assert(!is_infinite<Rng>::value,
                        "Attempt to convert an infinite range to a container.");
                    using I = range_common_iterator_t<Rng>;
                    // BUGBUG size may be known here, even though I may be an InputIterator
                    return Cont{I{begin(rng)}, I{end(rng)}};
                }
            };
        }
        /// \endcond

        /// \addtogroup group-core
        /// @{

        /// \ingroup group-core
        namespace
        {
            constexpr auto&& to_vector = static_const<detail::to_container_fn<meta::quote<std::vector>>>::value;
        }

        /// \brief For initializing a container of the specified type with the elements of an Iterable
        template<template<typename...> class ContT>
        detail::to_container_fn<meta::quote<ContT>> to_()
        {
            return {};
        }

        /// \overload
        template<template<typename...> class ContT, typename Rng,
            typename Cont = meta::apply<meta::quote<ContT>, range_value_t<Rng>>,
            CONCEPT_REQUIRES_(Iterable<Rng>() && detail::ConvertibleToContainer<Rng, Cont>())>
        Cont to_(Rng && rng)
        {
            return std::forward<Rng>(rng) | ranges::to_<ContT>();
        }

        /// \overload
        template<template<typename...> class ContT, typename T,
            typename Cont = meta::apply<meta::quote<ContT>, T>,
            CONCEPT_REQUIRES_(detail::ConvertibleToContainer<std::initializer_list<T>, Cont>())>
        Cont to_(std::initializer_list<T> list)
        {
            return list | ranges::to_<ContT>();
        }

        /// \overload
        template<typename Cont>
        detail::to_container_fn<meta::always<Cont>> to_()
        {
            return {};
        }

        /// \overload
        template<typename Cont, typename Rng,
            CONCEPT_REQUIRES_(Iterable<Rng>() && detail::ConvertibleToContainer<Rng, Cont>())>
        Cont to_(Rng && rng)
        {
            return std::forward<Rng>(rng) | ranges::to_<Cont>();
        }

        /// \overload
        template<typename Cont, typename T,
            CONCEPT_REQUIRES_(detail::ConvertibleToContainer<std::initializer_list<T>, Cont>())>
        Cont to_(std::initializer_list<T> list)
        {
            return list | ranges::to_<Cont>();
        }

        /// @}
    }
}

#endif
