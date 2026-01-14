#pragma once
#include <stdexcept>
#include <type_traits>

#include <derive-cpp/meta/member_ptr_class.hpp>

namespace derivecpp {

/// A utility that 'bounces' calls to a free function, to a member function of a given instance.
/// Enabled by default.
/// - Runtime checks ensure only one trampoline is active at once.
/// - Not thread safe.
/// - Allows for bouncing calls to google mock methods.
template <auto* Function, auto MockFunction> class Trampoline {
  public:
    using FunctionLocationType = decltype(Function);
    using FunctionPtrType = std::remove_pointer_t<FunctionLocationType>;
    using MockFunctionType = decltype(MockFunction);
    using MockType = member_pointer_class_t<MockFunctionType>;

    static inline FunctionLocationType func = Function;
    static constexpr MockFunctionType mockFunc = MockFunction;
    static inline FunctionPtrType realFunc = *func;
    static inline MockType* sMock = nullptr;

    template <typename... Args>
    static auto trampolineImpl(Args... args)
        -> decltype((std::declval<MockType*>()->*mockFunc)(args...)) {
        return (sMock->*mockFunc)(args...);
    }

    explicit Trampoline(MockType* mock) : mMock(*mock) {
        if (sMock != nullptr) {
            throw std::runtime_error("Trampoline already in use");
        }
        sMock = &mMock;
        Enable();
    }

    void Disable() {
        CheckOwnership();
        *func = realFunc;
    }

    void Enable() {
        CheckOwnership();
        *func = &trampolineImpl;
    }

    ~Trampoline() {
        *func = realFunc;
        sMock = nullptr;
    }

  private:
    void CheckOwnership() {
        if (sMock != &mMock) {
            throw std::runtime_error("Trampoline not owned by this instance");
        }
    }

    MockType& mMock;
    static_assert(std::is_member_function_pointer_v<MockFunctionType>);
    static_assert(std::is_function_v<std::remove_pointer_t<FunctionPtrType>>);
};
} // namespace derivecpp
