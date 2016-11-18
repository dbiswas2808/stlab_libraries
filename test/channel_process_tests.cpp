/*
Copyright 2016 Felix Petriconi
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/

/**************************************************************************************************/

#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>

#include <stlab/channel.hpp>

#include "channel_test_helper.hpp"


template <std::size_t N>
struct sum
{
    int _number_additions{ 0 };
    int _x{ 0 };
    stlab::process_state_scheduled _state{ stlab::await_forever };

    void await(int x)
    {
        _x += x;
        ++_number_additions;
        if (_number_additions == N)
        {
            _state = stlab::yield_immediate;
        }
    }

    int yield()
    {
        auto result = _x;
        _state = stlab::await_forever;
        _number_additions = 0;
        _x = 0;
        return result;
    }

    auto state() const { return _state; }
};

BOOST_FIXTURE_TEST_SUITE(int_channel_process_void_functor, channel_test_fixture<int>)

    BOOST_AUTO_TEST_CASE(int_channel_process_with_one_step) {
        BOOST_TEST_MESSAGE("int channel process with one step");

        std::atomic_int index{ 0 };
        std::vector<int> results(10);

        auto check = _receive | sum<1>() | [&_index = index, &_results = results](int x) { _results[_index++] = x; };

        _receive.set_ready();
        for (auto i = 0; i < 10; ++i) _send(i);

        wait_until_done([&_index = index] { return _index == 10; });

        for (auto i = 0; i < 10; ++i) {
            BOOST_REQUIRE_EQUAL(i, results[i]);
        }
    }

    BOOST_AUTO_TEST_CASE(int_channel_process_with_two_steps) {
        BOOST_TEST_MESSAGE("int channel process with two steps");

        std::atomic_int index{ 0 };
        std::vector<int> results(5);

        auto check = _receive | sum<2>() | [&_index = index, &_results = results](int x) { _results[_index++] = x; };

        _receive.set_ready();
        for (auto i = 0; i < 10; ++i) _send(i);

        wait_until_done([&_index = index] { return _index == 5; });

        int expectation[] = { 1, 5, 9, 13, 17 };
        for (auto i = 0; i < 5; ++i) {
            BOOST_REQUIRE_EQUAL(i, results[i]);
        }
    }


    BOOST_AUTO_TEST_CASE(int_channel_process_with_many_steps) {
        BOOST_TEST_MESSAGE("int channel process with many steps");

        std::atomic_int result{ 0 };

        auto check = _receive | sum<10>() | [&_result = result](int x) { _result = x; };

        _receive.set_ready();
        for (auto i = 0; i < 10; ++i) _send(i);

        wait_until_done([&_result = result] { return _result != 0; });

        BOOST_REQUIRE_EQUAL(45, result);
    }


    #if 0
    #ifdef STLAB_CHANNEL_MOVE_ONLY_SUPPORT

    BOOST_AUTO_TEST_CASE(move_only_int_channel_void_functor) {
        BOOST_TEST_MESSAGE("move only int channel void functor");

        stlab::sender<std::unique_ptr<int>> send;
        stlab::receiver<std::unique_ptr<int>> receive;
        std::tie(send, receive) = stlab::channel<std::unique_ptr<int>>(stlab::default_scheduler());
        std::atomic_int result{ 0 };

        auto check = receive | [&_result = result](std::unique_ptr<int> x) { _result += *x; };

        receive.set_ready();
        for (int i = 0; i < 10; ++i) {
            auto arg = std::make_unique<int>();
            *arg = 1;
            send(std::move(arg));
        }

        while (result < 10) {
            std::this_thread::sleep_for(std::chrono::microseconds(1));
        }

        BOOST_REQUIRE_EQUAL(10, result);
    }

    #endif

    BOOST_AUTO_TEST_CASE(int_channel_split_void_functor) {
        BOOST_TEST_MESSAGE("int channel void functor");

        stlab::sender<int> send;
        stlab::receiver<int> receive;
        std::tie(send, receive) = stlab::channel<int>(stlab::default_scheduler());
        std::atomic_int result1{ 0 };
        std::atomic_int result2{ 0 };

        auto check1 = receive | [&_result = result1](int x) { _result += x; };
        auto check2 = receive | [&_result = result2](int x) { _result += x; };

        receive.set_ready();
        for (int i = 0; i < 10; ++i) send(1);

        while (result1 < 10 && result2 < 10) {
            std::this_thread::sleep_for(std::chrono::microseconds(1));
        }

        BOOST_REQUIRE_EQUAL(10, result1);
        BOOST_REQUIRE_EQUAL(10, result2);
    }

    #endif

BOOST_AUTO_TEST_SUITE_END()
