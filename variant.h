#pragma once
#include "variant_utility.h"

template <typename T0, typename ... Ts> struct variant;

template<size_t I, typename T, char f = 2>
struct variant_alternative;

template <size_t I, typename T>
using variant_alternative_t = typename variant_alternative<I, T>::type;

template<size_t I, typename T, char f>
struct variant_alternative<I, const T, f>
{
	typedef std::add_const_t<typename variant_alternative<I, T>::type> type;
};

template<size_t I, typename ... Ts>
struct variant_alternative<I, variant<Ts...>, 2>
{
	static_assert(I < sizeof...(Ts), "Variant index out of bounds");
	typedef typename variant_alternative<I, variant<Ts...>, (I < sizeof...(Ts))>::type type;
};

template<size_t I, typename ... Ts>
struct variant_alternative<I, variant<Ts...>, 0>
{
	typedef no_type type;
};


template<size_t I, typename T0, typename ... Ts>
struct variant_alternative<I, variant<T0, Ts...>, 1>
{
	typedef typename variant_alternative<I - 1, variant<Ts...>, 1>::type type;
};

template<typename T0, typename ... Ts>
struct variant_alternative<0, variant<T0, Ts...>, 1>
{
	typedef T0 type;
};

template <typename T>
struct variant_size;

template <typename T>
struct variant_size<const T> : variant_size<T> {};

template <typename T, typename ... Ts>
struct variant_size<variant<T, Ts...>> : std::integral_constant<size_t, sizeof...(Ts)+1> {};

template <typename T>
inline constexpr size_t variant_size_v = variant_size<T>::value;

template <typename X, typename ... Us>
constexpr bool holds_alternative(const variant<Us...>& v) noexcept;

template <typename T0, typename ... Ts>
struct variant : destroyable_storage_t<T0, Ts...>
{
	using variant_base = destroyable_storage_t<T0, Ts...>;

	template<size_t I>
	using type_ind = std::integral_constant<size_t, I>;

	template<size_t I>
	using get_type = variant_alternative_t<I, variant>;

	static_assert(std::conjunction_v<std::is_object<T0>, std::is_object<Ts>...,
		std::negation<std::is_array<T0>>, std::negation<std::is_array<Ts>>...>,
		"Arrays, references or void are not permitted");

	template<std::enable_if_t<std::is_default_constructible_v<T0>, int> = 0>
	constexpr variant() noexcept(std::is_nothrow_default_constructible_v<T0>)
		: variant_base(type_ind<0>{})
	{}

	template<typename T, typename TT = type_choser_t<T, T0, Ts...>,
		std::enable_if_t<!std::is_same_v<std::decay_t<T>, variant>
		&& !is_specialization<std::decay_t<T>, std::in_place_type_t>::value
		&& !is_in_place_index_specialization<std::decay_t<T>>::value
		&& is_unique_v<TT, T0, Ts...>
		&& std::is_constructible_v<TT, T>, int> = 0>
	constexpr variant(T&& t) noexcept(std::is_nothrow_constructible_v<TT, T>)
		: variant_base(type_ind<get_type_ind<TT, T0, Ts...>()>{}, std::forward<T>(t))
	{}

	template<size_t I, typename ... Args,
			std::enable_if_t<(I < sizeof...(Ts) + 1), int> = 0
			, typename TT = variant_alternative_t<I, variant>
			, std::enable_if_t<(std::is_constructible_v<TT, Args...>), int> = 0>
	constexpr explicit variant(std::in_place_index_t<I>, Args&&... args)
		: variant_base(type_ind<I>{}, std::forward<Args>(args)...)
	{}

	template<typename T, typename ... Args
			, size_t I = get_type_ind<T, T0, Ts...>()
			, std::enable_if_t<(I < sizeof...(Ts) + 1) && (std::is_constructible_v<T, Args...>), int> = 0>
	constexpr explicit variant(std::in_place_type_t<T>, Args&&... args)
		: variant_base(type_ind<get_type_ind<T, T0, Ts...>()>{}, std::forward<Args>(args)...)
	{}

	variant(const variant&) = delete;
	variant(variant&&) = delete;

	~variant() noexcept = default;
private:
	
};

template <typename X, typename ... Us>
constexpr bool holds_alternative(const variant<Us...>& v) noexcept
{
	if (v.valueless_by_exception())
	{
		return 0;
	}
	else
	{
		return v.index() == get_type_ind<X, Us...>();
	}
}

template <size_t I, typename ... Ts>
constexpr decltype(auto) get(variant<Ts...>& v)
{
	static_assert(I < sizeof...(Ts), "Variant Index out of range");
	if (v.index() != I)
	{
		throw bad_variant_access("Wrong variant index or type");
	}
	return raw_get<I>(v.get_storage());
}

template <size_t I, typename ... Ts>
constexpr decltype(auto) get(variant<Ts...> const& v)
{
	static_assert(I < sizeof...(Ts), "Variant Index out of range");
	if (v.index() != I)
	{
		throw bad_variant_access("Wrong variant index or type");
	}
	return raw_get<I>(v.get_storage());
}

template <size_t I, typename ... Ts>
constexpr decltype(auto) get(variant<Ts...>&& v)
{
	static_assert(I < sizeof...(Ts), "Variant Index out of range");
	if (v.index() != I)
	{
		throw bad_variant_access("Wrong variant index or type");
	}
	return raw_get<I>(std::move(v).get_storage());
}

template <size_t I, typename ... Ts>
constexpr decltype(auto) get(variant<Ts...> const&& v)
{
	static_assert(I < sizeof...(Ts), "Variant Index out of range");
	if (v.index() != I)
	{
		throw bad_variant_access("Wrong variant index or type");
	}
	return raw_get<I>(std::move(v).get_storage());
}


template <typename T, typename ... Ts>
constexpr decltype(auto) get(variant<Ts...>& v)
{
	constexpr size_t I = get_type_ind<T, Ts...>();
	static_assert(is_unique_v<T, Ts...>, "T has to be unique varriant alternative");
	return get<I>(v);
}

template <typename T, typename ... Ts>
constexpr decltype(auto) get(variant<Ts...> const& v)
{
	constexpr size_t I = get_type_ind<T, Ts...>();
	static_assert(is_unique_v<T, Ts...>, "T has to be unique varriant alternative");
	return get<I>(v);
}

template <typename T, typename ... Ts>
constexpr decltype(auto) get(variant<Ts...>&& v)
{
	constexpr size_t I = get_type_ind<T, Ts...>();
	static_assert(is_unique_v<T, Ts...>, "T has to be unique varriant alternative");
	return get<I>(std::move(v));
}

template <typename T, typename ... Ts>
constexpr decltype(auto) get(variant<Ts...> const&& v)
{
	constexpr size_t I = get_type_ind<T, Ts...>();
	static_assert(is_unique_v<T, Ts...>, "T has to be unique varriant alternative");
	return get<I>(std::move(v));
}