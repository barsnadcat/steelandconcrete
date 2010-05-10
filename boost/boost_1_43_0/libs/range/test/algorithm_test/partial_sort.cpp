// Boost.Range library
//
//  Copyright Neil Groves 2009. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//
// For more information, see http://www.boost.org/libs/range/
//
#include <boost/range/algorithm/partial_sort.hpp>

#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>

#include <boost/assign.hpp>
#include <boost/bind.hpp>
#include <algorithm>
#include <functional>
#include <list>
#include <numeric>
#include <deque>
#include <vector>

namespace boost
{
    namespace
    {
        struct partial_sort_policy
        {
            template<class Container, class Iterator>
            void test_partial_sort(Container& cont, Iterator mid)
            {
                boost::partial_sort(cont, mid);
            }

            template<class Container, class Iterator>
            void reference_partial_sort(Container& cont, Iterator mid)
            {
                std::partial_sort(cont.begin(), mid, cont.end());
            }
        };

        template<class BinaryPredicate>
        struct partial_sort_pred_policy
        {
            template<class Container, class Iterator>
            void test_partial_sort(Container& cont, Iterator mid)
            {
                boost::partial_sort(cont, mid, BinaryPredicate());
            }

            template<class Container, class Iterator>
            void reference_partial_sort(Container& cont, Iterator mid)
            {
                std::partial_sort(cont.begin(), mid, cont.end(), BinaryPredicate());
            }
        };

        template<class Container, class TestPolicy>
        void test_partial_sort_tp_impl(Container& cont, TestPolicy policy)
        {
            Container reference(cont);
            Container test(cont);

            typedef BOOST_DEDUCED_TYPENAME range_iterator<Container>::type iterator_t;

            BOOST_CHECK_EQUAL( reference.size(), test.size() );
            if (reference.size() != test.size())
                return;

            iterator_t reference_mid = reference.begin();
            iterator_t test_mid = test.begin();

            bool complete = false;
            while (!complete)
            {
                if (reference_mid == reference.end())
                    complete = true;

                policy.test_partial_sort(test, test_mid);
                policy.reference_partial_sort(reference, reference_mid);

                BOOST_CHECK_EQUAL_COLLECTIONS(
                    reference.begin(), reference.end(),
                    test.begin(), test.end()
                    );

                if (reference_mid != reference.end())
                {
                    ++reference_mid;
                    ++test_mid;
                }
            }
        }

        template<class Container>
        void test_partial_sort_impl(Container& cont)
        {
            test_partial_sort_tp_impl(cont, partial_sort_policy());
        }

        template<class Container, class BinaryPredicate>
        void test_partial_sort_impl(Container& cont, BinaryPredicate pred)
        {
            test_partial_sort_tp_impl(cont, partial_sort_pred_policy<BinaryPredicate>());
        }

        template<class Container>
        void test_partial_sort_impl()
        {
            using namespace boost::assign;

            Container cont;
            test_partial_sort_impl(cont);
            test_partial_sort_impl(cont, std::less<int>());
            test_partial_sort_impl(cont, std::greater<int>());

            cont.clear();
            cont += 1;
            test_partial_sort_impl(cont);
            test_partial_sort_impl(cont, std::less<int>());
            test_partial_sort_impl(cont, std::greater<int>());

            cont.clear();
            cont += 1,2,3,4,5,6,7,8,9;
            test_partial_sort_impl(cont);
            test_partial_sort_impl(cont, std::less<int>());
            test_partial_sort_impl(cont, std::greater<int>());
        }

        void test_partial_sort()
        {
            test_partial_sort_impl< std::vector<int> >();
            test_partial_sort_impl< std::deque<int> >();
        }
    }
}

boost::unit_test::test_suite*
init_unit_test_suite(int argc, char* argv[])
{
    boost::unit_test::test_suite* test
        = BOOST_TEST_SUITE( "RangeTestSuite.algorithm.partial_sort" );

    test->add( BOOST_TEST_CASE( &boost::test_partial_sort ) );

    return test;
}
