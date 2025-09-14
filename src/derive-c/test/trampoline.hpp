#pragma once
#include <type_traits>
#include <stdexcept>

template<typename T>
struct member_pointer_class;

template<typename C, typename M>
struct member_pointer_class<M C::*> {
    using type = C;
};

template<typename T>
using member_pointer_class_t = typename member_pointer_class<T>::type;

/// A utility that 'bounces' calls to a free function, to a member function of a given instance.
/// - Runtime checks ensure only one trampoline is active at once.
/// - Not thread safe.
/// - Allows for bouncing calls to google mock methods. 
template<auto* Function, auto MockFunction>
class Trampoline {
public:
    using FunctionLocationType = decltype(Function);
    using FunctionPtrType = std::remove_pointer_t<FunctionLocationType>;
    using MockFunctionType = decltype(MockFunction);
    using MockType = member_pointer_class_t<MockFunctionType>;

    static inline FunctionLocationType func = Function;
    static constexpr MockFunctionType mockFunc = MockFunction;
    static inline FunctionPtrType realFunc = *func;
    static inline MockType* sMock = nullptr;
    
    template<typename... Args>
    static auto trampolineImpl(Args... args) -> decltype((std::declval<MockType*>()->*mockFunc)(args...)) {
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
