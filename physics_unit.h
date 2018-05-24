//  Created by Lau Ching Jun on 22/12/2015.

#ifndef physics_unit_h
#define physics_unit_h

#include <ratio>
#include <type_traits>

template <int... Is>
struct Dimen {
    constexpr static int count = sizeof...(Is);
    static int get(int i) {
        int array[] = {Is...};
        return array[i];
    }
};

// Dimen Append
template <typename D1, typename D2> struct DimenAppend;
template <int... Is1, int I2, int... Is2>
struct DimenAppend<Dimen<Is1...>, Dimen<I2, Is2...>> {
    using type = typename DimenAppend<Dimen<Is1..., I2>, Dimen<Is2...>>::type;
};
template <int... Is1>
struct DimenAppend<Dimen<Is1...>, Dimen<>> {
    using type = Dimen<Is1...>;
};

// Dimen Invert
template <typename D> struct DimenInvert;
template <int I, int... Is>
struct DimenInvert<Dimen<I, Is...>> {
    using type = typename DimenAppend<Dimen<-I>, typename DimenInvert<Dimen<Is...>>::type>::type;
};
template <>
struct DimenInvert<Dimen<>> {
    using type = Dimen<>;
};

// Dimen Sum
template <typename D1, typename D2> struct DimenSum;
template <int I1, int I2, int... Is1, int... Is2>
struct DimenSum<Dimen<I1, Is1...>, Dimen<I2, Is2...>> {
    using type = typename DimenAppend<Dimen<I1 + I2>, typename DimenSum<Dimen<Is1...>, Dimen<Is2...>>::type>::type;
};
template <>
struct DimenSum<Dimen<>, Dimen<>> {
    using type = Dimen<>;
};

// Dimen is zero
template <typename D> struct DimenIsZero;
template <int I, int... Is>
struct DimenIsZero<Dimen<I, Is...>> {
    constexpr static bool value = (I == 0) && DimenIsZero<Dimen<Is...>>::value;
};
template <>
struct DimenIsZero<Dimen<>> {
    constexpr static bool value = true;
};

// Unit
struct UnitBase {};
template <typename _V, typename _D, typename _M = std::ratio<1, 1>>
struct Unit : UnitBase {
    template <typename V, typename D, typename M> friend struct Unit;
    using V = _V;
    using D = _D;
    using M = _M;
    using StandardUnit = Unit<V, D, std::ratio<1, 1>>;
    
    // type operations
    using Invert = Unit<V, typename DimenInvert<D>::type, std::ratio<M::den, M::num>>;
    template <typename U>
    using Multiply = Unit<typename std::common_type<V, typename U::V>::type, typename DimenSum<D, typename U::D>::type, typename std::ratio_multiply<M, typename U::M>>;
    template <typename U>
    using Divide = Multiply<typename U::Invert>;
    template <typename Multiplier>
    using DeriveType = Unit<V, D, std::ratio_multiply<M, Multiplier>>;
    
    float multiplier() const {
        return (float)M::num / (float)M::den;
    }
    int num() const { return M::num; }
    int den() const { return M::den; }
    
    constexpr Unit() : _value(0) {}
    constexpr explicit Unit(const V &value) : _value(value) {}
    
    // auto convert from float if it is a dimensionless type
    template <typename D2 = D>
    constexpr Unit(const typename std::enable_if<DimenIsZero<D2>::value, V>::type &value) : _value(value) {}
    
    template <typename V2, typename M2>
    constexpr Unit(const Unit<V2, D, M2> &other) :
    _value(other._value * std::ratio_divide<M2, M>::num / std::ratio_divide<M2, M>::den) {}
    
    StandardUnit standard() const {
        return (StandardUnit) (*this);
    }
    
    template <typename U>
    constexpr U convert() const {
        return (U) (*this);
    }
    
    template <typename U>
    typename U::V value() const {
        return this->convert<U>()._value;
    }
    
    // unary
    inline Unit operator -() const {
        return Unit(-_value);
    }
    
    inline Unit operator +() const {
        return *this;
    }
    
    // binary operators with other same units
    template <typename V2, typename M2>
    inline Unit<typename std::common_type<V, V2>::type, D, M> operator +(const Unit<V2, D, M2> &other) const {
        return Unit(_value + ((Unit) other)._value);
    }
    template <typename V2, typename M2>
    inline Unit<typename std::common_type<V, V2>::type, D, M> operator -(const Unit<V2, D, M2> &other) const {
        return Unit(_value - ((Unit) other)._value);
    }
    
#define DEF_COMPARISON(op) \
template <typename V2, typename M2> \
inline bool operator op(const Unit<V2, D, M2> &other) const { \
    return _value op ((Unit) other)._value; \
}
    DEF_COMPARISON(==)
    DEF_COMPARISON(!=)
    DEF_COMPARISON(>)
    DEF_COMPARISON(<)
    DEF_COMPARISON(>=)
    DEF_COMPARISON(<=)
    
    // binary operators with numbers
    template <typename N>
    typename std::enable_if<std::is_arithmetic<N>::value, Unit>::type
    inline operator *(const N& n) const {
        return Unit(_value * n);
    }
    template <typename N>
    typename std::enable_if<std::is_arithmetic<N>::value, Unit>::type
    inline operator /(const N& n) const {
        return Unit(_value / n);
    }
    
    // Default ratio to 1 if multiply result is dimensionless
    template <typename U>
    using MultiplyResult = typename std::conditional<DimenIsZero<typename Multiply<U>::D>::value, typename Multiply<U>::StandardUnit, Multiply<U>>::type;
    template <typename U>
    using DivideResult = MultiplyResult<typename U::Invert>;
    
    // binary operator with other units
    template <typename U>
    typename std::enable_if<std::is_base_of<UnitBase, U>::value, MultiplyResult<U>>::type
    inline operator *(const U &o) const {
        return Multiply<U>(_value * o._value);
    }
    
    template <typename U>
    typename std::enable_if<std::is_base_of<UnitBase, U>::value, DivideResult<U>>::type
    inline operator /(const U &o) const {
        return Divide<U>(_value / o._value);
    }
    
#define DEF_OPERATEABLE(name, op) \
template <typename T1, typename T2> \
struct name { \
    template <typename X, typename Y, typename R = decltype(std::declval<X>() op std::declval<Y>())> \
    static std::true_type operable(X, Y); \
    static std::false_type operable(...); \
    constexpr static bool value = std::is_same<decltype(operable(std::declval<T1>(), std::declval<T2>())), std::true_type>::value; \
}
    
    DEF_OPERATEABLE(Addable, +);
    DEF_OPERATEABLE(Subtractable, -);
    DEF_OPERATEABLE(Multipliable, *);
    DEF_OPERATEABLE(Dividable, /);
    
#define DEF_IN_PLACE_OPERATOR(op, check) \
template <typename T> \
typename std::enable_if<check<Unit, T>::value, void>::type \
inline operator op ## =(const T& o) { \
    *this = *this op o; \
}
    
    DEF_IN_PLACE_OPERATOR(+, Addable)
    DEF_IN_PLACE_OPERATOR(-, Subtractable)
    DEF_IN_PLACE_OPERATOR(*, Multipliable)
    DEF_IN_PLACE_OPERATOR(/, Dividable)
    
    // dimensionless auto convertor
    template <typename D2 = D>
    operator typename std::enable_if<DimenIsZero<D2>::value, V>::type () {
        return _value;
    }
    
private:
    V _value;
};

// binary operator with number
template <typename U, typename N>
typename std::enable_if<std::is_base_of<UnitBase, U>::value && std::is_arithmetic<N>::value, U>::type
inline operator *(const N &n, const U &u) {
    return u * n;;
}

template <typename U, typename N>
typename std::enable_if<std::is_base_of<UnitBase, U>::value && std::is_arithmetic<N>::value, typename U::Invert>::type
inline operator /(const N &n, const U &u) {
    return typename U::Invert(n / u.template value<U>());
}

using Dimensionless = Unit<float, Dimen<0, 0, 0>>;
using Mass = Unit<float, Dimen<1, 0, 0>>;
using Length = Unit<float, Dimen<0, 1, 0>>;
using Time = Unit<float, Dimen<0, 0, 1>>;

using DDimensionless = Unit<double, Dimen<0, 0, 0>>;
using DMass = Unit<double, Dimen<1, 0, 0>>;
using DLength = Unit<double, Dimen<0, 1, 0>>;
using DTime = Unit<double, Dimen<0, 0, 1>>;

using Speed = Length::Divide<Time>;
using Acceleration = Speed::Divide<Time>;
using Force = Mass::Multiply<Acceleration>;
using Energy = Force::Multiply<Length>;
using Power = Energy::Divide<Time>;

using Kilogram = Mass;
using Gram = Kilogram::DeriveType<std::milli>;
using Pound = Kilogram::DeriveType<std::ratio<45359237, 100000000>>;

using Meter = Length;
using Kilometer = Meter::DeriveType<std::kilo>;
using Mile = Meter::DeriveType<std::ratio<1609344, 1000>>;
using Millimeter = Meter::DeriveType<std::milli>;
using Centimeter = Meter::DeriveType<std::centi>;
using Inch = Centimeter::DeriveType<std::ratio<254, 100>>;
using Foot = Inch::DeriveType<std::ratio<12, 1>>;
using Yard = Foot::DeriveType<std::ratio<3, 1>>;

using Second = Time;
using Millisecond = Second::DeriveType<std::milli>;
using Minute = Second::DeriveType<std::ratio<60, 1>>;
using Hour = Minute::DeriveType<std::ratio<60, 1>>;
using Day = Hour::DeriveType<std::ratio<24, 1>>;

using DSecond = DTime;
using DMillisecond = DSecond::DeriveType<std::milli>;
using DMinute = DSecond::DeriveType<std::ratio<60, 1>>;
using DHour = DMinute::DeriveType<std::ratio<60, 1>>;
using DDay = DHour::DeriveType<std::ratio<24, 1>>;

using MeterPerSecond = Speed;
using KilometerPerHour = Kilometer::Divide<Hour>;
using MilePerHour = Mile::Divide<Hour>;

using Joule = Energy;
using KiloJoule = Joule::DeriveType<std::kilo>;
using KiloCalorie = KiloJoule::DeriveType<std::ratio<4184, 1000>>;

using GForce = Acceleration::DeriveType<std::ratio<98, 10>>;

#endif /* physics_unit_h */
