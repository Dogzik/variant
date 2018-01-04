#pragma once
#include <utility>
#include <algorithm>
#include <cstdio>
#include <memory>
#include <exception>
#include <type_traits>
#include <limits>


struct monostate {};

constexpr bool operator<(monostate, monostate) noexcept { return false; }
constexpr bool operator>(monostate, monostate) noexcept { return false; }
constexpr bool operator<=(monostate, monostate) noexcept { return true; }
constexpr bool operator>=(monostate, monostate) noexcept { return true; }
constexpr bool operator==(monostate, monostate) noexcept { return true; }
constexpr bool operator!=(monostate, monostate) noexcept { return false; }


template<typename T>
struct _Is_in_place_index_specialization
{
	static constexpr bool value = 0;
};

template<size_t I>
struct _Is_in_place_index_specialization<std::in_place_index_t<I>>
{
	static constexpr bool value = 1;
};



constexpr size_t gcd(size_t a, size_t b) {
	while (b != 0)
	{
		size_t t = b;
		b = a % b;
		a = t;
	}
	return a;
}

template<typename T, typename ... TAIL>
struct get_align
{
	static constexpr size_t value = (alignof(T)* get_align<TAIL...>::value) / gcd(alignof(T), get_align<TAIL...>::value);
};

template<typename T>
struct get_align<T>
{
	static constexpr size_t value = alignof(T);
};

template<typename T, typename ... TAIL>
constexpr size_t get_align_v = get_align<T, TAIL...>::value;

template<typename T, typename ... TAIL>
struct get_size
{
	static constexpr size_t value = std::max(sizeof(T), get_size<TAIL...>::value);
};

template<typename T>
struct get_size<T>
{
	static constexpr size_t value = sizeof(T);
};

template<typename T, typename ... TAIL>
constexpr size_t get_size_v = get_size<T, TAIL...>::value;


constexpr size_t variant_npos = static_cast<size_t>(-1);


template<typename T, typename ... Ts>
struct type_choser : type_choser<Ts...>
{
	using type_choser<Ts...>::f;
	static T f(T);
};

template<typename T>
struct type_choser<T>
{
	static T f(T);
};

template<typename U, typename T, typename ... Ts>
using type_choser_t = decltype(type_choser<T, Ts...>::f(std::declval<U>()));


struct bad_variant_access : public std::exception
{
	bad_variant_access() noexcept
		: exception()
		, reason("bad_variant_access")
	{}

	bad_variant_access(const char* why) noexcept
		: exception()
		, reason(why)
	{}

	const char* what() const noexcept override
	{
		return reason;
	}

	~bad_variant_access() override = default;
private:
	char const* reason;
};


template<typename T, typename U, typename ... Us>
constexpr size_t get_type_ind() noexcept
{
	if constexpr(std::is_same_v<T, U>)
	{
		return 0;
	}
	else
	{
		return 1 + get_type_ind<T, Us...>();
	}
}

template<typename T>
constexpr size_t get_type_ind() noexcept
{
	return 0;
}

template<size_t CNT>
using variant_index_t = std::conditional_t<(CNT < static_cast<size_t>(std::numeric_limits<signed char>::max())), signed char,
	std::conditional_t<(CNT < static_cast<size_t>(std::numeric_limits<short>::max())), short, int>>;