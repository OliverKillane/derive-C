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

template<auto* Function, auto MockFunction>
class Trampoline {
public:
    using FunctionType = decltype(Function);
    using MockFunctionType = decltype(MockFunction);
    using MockType = member_pointer_class_t<MockFunctionType>;

    static inline FunctionType func = Function;
    static constexpr MockFunctionType mockFunc = MockFunction;
    static inline MockType* sMock = nullptr;
    
    // Use a proper function pointer instead of lambda with auto
    template<typename... Args>
    static auto trampolineImpl(Args... args) -> decltype((std::declval<MockType*>()->*mockFunc)(args...)) {
        return (sMock->*mockFunc)(args...);
    }

    explicit Trampoline(MockType* mock) : mMock(*mock) {
        if (sMock != nullptr) {
            throw std::runtime_error("Trampoline already in use");
        }
        sMock = &mMock;
        *func = &trampolineImpl;
    }

    ~Trampoline() {
        *func = nullptr;
        sMock = nullptr;
    }
    
private:
    MockType& mMock;
    static_assert(std::is_member_function_pointer_v<MockFunctionType>);
    static_assert(std::is_function_v<std::remove_pointer_t<std::remove_pointer_t<FunctionType>>>);
};
