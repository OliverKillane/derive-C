#pragma once

#include "trampoline.hpp"

#define FIXTURE_MOCK(fixture, ret, name, ...)                                                      \
    MOCK_METHOD(ret, name##_mock, __VA_ARGS__);                                                    \
    derivecpp::Trampoline<&name, &fixture::name##_mock> name##_tramp{this};
