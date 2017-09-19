#ifndef __BITMASK_H__
#define __BITMASK_H__

#include <type_traits>

template<typename Enum>  
struct EnableBitMaskOperators  
{
    static const bool enable = false;
};

#define ENABLE_BITMASK_OPERATORS(x)  \
template<>                           \
struct EnableBitMaskOperators<x>     \
{                                    \
    static const bool enable = true; \
}

template<typename Enum>
typename std::underlying_type<Enum>::type e2ut(Enum e) { return static_cast<typename std::underlying_type<Enum>::type>(e); }

template<typename Enum>  
typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum>::type  
operator |(Enum lhs, Enum rhs)  
{
    using underlying = typename std::underlying_type<Enum>::type;
    return static_cast<Enum>(static_cast<underlying>(lhs) | static_cast<underlying>(rhs));
}

template<typename Enum>
typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum>::type &operator |=(Enum &lhs, Enum rhs)
{
    using underlying = typename std::underlying_type<Enum>::type;
    return (lhs = static_cast<Enum> (static_cast<underlying>(lhs) | static_cast<underlying>(rhs)));
}

template<typename Enum>
typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum>::type operator &(Enum lhs, Enum rhs)
{
    using underlying = typename std::underlying_type<Enum>::type;
    return static_cast<Enum>(static_cast<underlying>(lhs) & static_cast<underlying>(rhs));
}

template<typename Enum>
typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum>::type &operator &=(Enum &lhs, Enum rhs)
{
    using underlying = typename std::underlying_type<Enum>::type;
    return (lhs = static_cast<Enum> (static_cast<underlying>(lhs) & static_cast<underlying>(rhs)));
}

template<typename Enum>
typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum>::type operator ^(Enum lhs, Enum rhs)
{
    using underlying = typename std::underlying_type<Enum>::type;
    return static_cast<Enum>(static_cast<underlying>(lhs) ^ static_cast<underlying>(rhs));
}

template<typename Enum>
typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum>::type &operator ^=(Enum &lhs, Enum rhs)
{
    using underlying = typename std::underlying_type<Enum>::type;
    return (lhs = static_cast<Enum> (static_cast<underlying>(lhs) ^ static_cast<underlying>(rhs)));
}

template<typename Enum>
typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum>::type
operator ~(Enum val)
{
    using underlying = typename std::underlying_type<Enum>::type;
    return static_cast<Enum>(~static_cast<underlying>(val));
}

#endif
