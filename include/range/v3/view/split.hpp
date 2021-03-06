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

#ifndef RANGES_V3_VIEW_SPLIT_HPP
#define RANGES_V3_VIEW_SPLIT_HPP

#include <utility>
#include <type_traits>
#include <range/v3/range_fwd.hpp>
#include <range/v3/begin_end.hpp>
#include <range/v3/range.hpp>
#include <range/v3/range_traits.hpp>
#include <range/v3/range_concepts.hpp>
#include <range/v3/range_facade.hpp>
#include <range/v3/utility/functional.hpp>
#include <range/v3/utility/semiregular.hpp>
#include <range/v3/utility/static_const.hpp>
#include <range/v3/algorithm/adjacent_find.hpp>
#include <range/v3/view/view.hpp>
#include <range/v3/view/all.hpp>
#include <range/v3/view/iota.hpp>
#include <range/v3/view/indirect.hpp>
#include <range/v3/view/take_while.hpp>

namespace ranges
{
    inline namespace v3
    {
        /// \addtogroup group-views
        /// @{
        template<typename Rng, typename Fun>
        struct split_view
          : range_facade<split_view<Rng, Fun>>
        {
        private:
            friend range_access;
            Rng rng_;
            semiregular_t<invokable_t<Fun>> fun_;

            template<bool IsConst>
            struct cursor
            {
            private:
                friend range_access;
                friend split_view;
                bool zero_;
                range_iterator_t<Rng> cur_;
                range_sentinel_t<Rng> last_;
                using fun_ref_t = semiregular_ref_or_val_t<invokable_t<Fun>, IsConst>;
                fun_ref_t fun_;

                struct search_pred
                {
                    bool zero_;
                    range_iterator_t<Rng> first_;
                    range_sentinel_t<Rng> last_;
                    fun_ref_t fun_;
                    bool operator()(range_iterator_t<Rng> cur) const
                    {
                        return (zero_ && cur == first_) || (cur != last_ && !fun_(cur, last_).first);
                    }
                };
                using reference_ =
                    indirect_view<
                        take_while_view<
                            iota_view<range_iterator_t<Rng>>,
                            search_pred,
                            is_infinite<Rng>::value>>;
                reference_ current() const
                {
                    return reference_{{view::iota(cur_), {zero_, cur_, last_, fun_}}};
                }
                void next()
                {
                    RANGES_ASSERT(cur_ != last_);
                    // If the last match consumed zero elements, bump the position.
                    advance_bounded(cur_, (int)zero_, last_);
                    zero_ = false;
                    for(; cur_ != last_; ++cur_)
                    {
                        std::pair<bool, range_difference_t<Rng>> p = fun_(cur_, last_);
                        if(p.first)
                        {
                            advance(cur_, p.second);
                            zero_ = (0 == p.second);
                            return;
                        }
                    }
                }
                bool done() const
                {
                    return cur_ == last_;
                }
                bool equal(cursor const &that) const
                {
                    return cur_ == that.cur_;
                }
                cursor(fun_ref_t fun, range_iterator_t<Rng> first, range_sentinel_t<Rng> last)
                  : cur_(first), last_(last), fun_(fun)
                {
                    // For skipping an initial zero-length match
                    auto p = fun(first, last);
                    zero_ = p.first && 0 == p.second;
                }
            public:
                cursor() = default;
            };
            cursor<false> begin_cursor()
            {
                return {fun_, ranges::begin(rng_), ranges::end(rng_)};
            }
            CONCEPT_REQUIRES(Invokable<Fun const, range_iterator_t<Rng>, range_sentinel_t<Rng>>())
            cursor<true> begin_cursor() const
            {
                return {fun_, ranges::begin(rng_), ranges::end(rng_)};
            }
        public:
            split_view() = default;
            split_view(Rng rng, Fun fun)
              : rng_(std::move(rng))
              , fun_(std::move(fun))
            {}
        };

        namespace view
        {
            struct split_fn
            {
            private:
                friend view_access;
                template<typename T>
                static auto bind(split_fn split, T && t)
                RANGES_DECLTYPE_AUTO_RETURN
                (
                    make_pipeable(std::bind(split, std::placeholders::_1, bind_forward<T>(t)))
                )
                template<typename Rng>
                struct element_pred
                {
                    range_value_t<Rng> val_;
                    std::pair<bool, range_difference_t<Rng>>
                    operator()(range_iterator_t<Rng> cur, range_sentinel_t<Rng> end) const
                    {
                        using P = std::pair<bool, range_difference_t<Rng>>;
                        RANGES_ASSERT(cur != end);
                        return *cur == val_ ? P{true, 1} : P{false, 0};
                    }
                };
                template<typename Rng, typename Sub>
                struct subrange_pred
                {
                    all_t<Sub> sub_;
                    range_difference_t<Sub> len_;
                    subrange_pred() = default;
                    subrange_pred(Sub && sub)
                      : sub_(all(std::forward<Sub>(sub))), len_(distance(sub_))
                    {}
                    std::pair<bool, range_difference_t<Rng>>
                    operator()(range_iterator_t<Rng> cur, range_sentinel_t<Rng> end) const
                    {
                        RANGES_ASSERT(cur != end);
                        if(SizedIteratorRange<range_iterator_t<Rng>, range_sentinel_t<Rng>>() && 
                            distance(cur, end) < len_)
                            return {false, 0};
                        auto pat_cur = ranges::begin(sub_);
                        auto pat_end = ranges::end(sub_);
                        for(;; ++cur, ++pat_cur)
                        {
                            if(pat_cur == pat_end)
                                return {true, len_};
                            if(cur == end || !(*cur == *pat_cur))
                                return {false, 0};
                        }
                    }
                };
            public:
                template<typename Rng, typename Fun>
                using FunctionConcept = meta::and_<
                    ForwardIterable<Rng>,
                    Function<Fun, range_iterator_t<Rng>, range_sentinel_t<Rng>>,
                    Convertible<
                        concepts::Function::result_t<Fun, range_iterator_t<Rng>, range_sentinel_t<Rng>>,
                        std::pair<bool, range_difference_t<Rng>>>>;

                template<typename Rng>
                using ElementConcept = meta::and_<
                    ForwardIterable<Rng>,
                    Regular<range_value_t<Rng>>>;

                template<typename Rng, typename Sub>
                using SubRangeConcept = meta::and_<
                    ForwardIterable<Rng>,
                    ForwardIterable<Sub>,
                    EqualityComparable<range_value_t<Rng>, range_value_t<Sub>>>;

                template<typename Rng, typename Fun,
                    CONCEPT_REQUIRES_(FunctionConcept<Rng, Fun>())>
                split_view<all_t<Rng>, Fun> operator()(Rng && rng, Fun fun) const
                {
                    return {all(std::forward<Rng>(rng)), std::move(fun)};
                }
                template<typename Rng,
                    CONCEPT_REQUIRES_(ElementConcept<Rng>())>
                split_view<all_t<Rng>, element_pred<Rng>> operator()(Rng && rng, range_value_t<Rng> val) const
                {
                    return {all(std::forward<Rng>(rng)), {std::move(val)}};
                }
                template<typename Rng, typename Sub,
                    CONCEPT_REQUIRES_(SubRangeConcept<Rng, Sub>())>
                split_view<all_t<Rng>, subrange_pred<Rng, Sub>> operator()(Rng && rng, Sub && sub) const
                {
                    return {all(std::forward<Rng>(rng)), {std::forward<Sub>(sub)}};
                }

            #ifndef RANGES_DOXYGEN_INVOKED
                template<typename Rng, typename T,
                    CONCEPT_REQUIRES_(!Convertible<T, range_value_t<Rng>>())>
                void operator()(Rng &&, T &&) const volatile
                {
                    CONCEPT_ASSERT_MSG(ForwardIterable<Rng>(),
                        "The object on which view::split operates must be a model of the "
                        "ForwardIterable concept.");
                    CONCEPT_ASSERT_MSG(Convertible<T, range_value_t<Rng>>(),
                        "The delimiter argument to view::split must be one of the following: "
                        "(1) A single element of the range's value type, where the value type is a "
                        "model of the Regular concept, "
                        "(2) A ForwardIterable whose value type is EqualityComparable to the input "
                        "range's value type, or "
                        "(3) A Function that is callable with two arguments: the range's iterator "
                        "and sentinel, and that returns a std::pair<bool, D>, where D is the "
                        "input range's difference_type.");
                }
            #endif
            };

            /// \relates split_fn
            /// \ingroup group-views
            namespace
            {
                constexpr auto&& split = static_const<view<split_fn>>::value;
            }
        }
        /// @}
    }
}

#endif
