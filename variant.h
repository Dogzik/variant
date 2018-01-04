#pragma once
#include "variant_utility.h"


template<size_t I, typename T, char f = 2>
struct variant_alternative;

template <size_t I, typename T>
using variant_alternative_t = typename variant_alternative<I, T>::type;

template <typename T>
struct variant_size;

template <typename T0, typename ... Ts> struct variant;

template <typename X, typename ... Us>
constexpr bool holds_alternative(const variant<Us...>& v) noexcept;

template <typename T0, typename ... Ts>
struct variant
{
	static_assert(std::conjunction_v<std::is_object<T0>, std::is_object<Ts>...,
		std::negation<std::is_array<T0>>, std::negation<std::is_array<Ts>>...>,
		"Arrays, references or void are not permitted");

	template<size_t I>
	using get_type = variant_alternative_t<I, variant>;

	template<std::enable_if_t<std::is_default_constructible_v<T0>, int> = 0>
	constexpr variant() noexcept(std::is_nothrow_default_constructible_v<T0>)
		: cur_type(0)
	{
		new(reinterpret_cast<T0*>(&data)) T0();
	}

	variant(const variant&) = delete;
	variant(variant&&) = delete;

	template<typename T, typename TT = type_choser_t<T, T0, Ts...>,
		std::enable_if_t<!std::is_same_v<std::decay_t<T>, variant>
		&& !std::_Is_specialization<std::decay_t<T>, std::in_place_type_t>::value
		&& !_Is_in_place_index_specialization<std::decay_t<T>>::value
		&& std::is_constructible_v<TT, T>, int> = 0>
	constexpr variant(T&& t) noexcept(std::is_nothrow_constructible_v<TT, T>)
	{
		set_index(get_type_ind<TT, T0, Ts...>());
		new(reinterpret_cast<TT*>(&data)) TT(std::forward<T>(t));
	}

	constexpr size_t index() const noexcept
	{
		return static_cast<size_t>(cur_type);
	}

	constexpr bool valueless_by_exception() const noexcept
	{
		return index() == variant_npos;
	}

	template <size_t I>
	constexpr get_type<I>& get()
	{
		using ret_type = get_type<I>;
		if (I != index())
		{
			throw bad_variant_access("Wrong variant index");
		}
		return static_cast<ret_type&>(*reinterpret_cast<ret_type*>(&data));
	}

	template <size_t I>
	constexpr get_type<I> const& get() const
	{
		using ret_type = get_type<I>;
		if (I != index())
		{
			throw bad_variant_access("Wrong variant index");
		}
		return static_cast<ret_type const&>(*reinterpret_cast<ret_type const*>(&data));
	}

	template <typename T>
	constexpr T& get()
	{
		if (!holds_alternative<T>(*this))
		{
			throw bad_variant_access("Variant doesn't hold this type");
		}
		return static_cast<T&>(*reinterpret_cast<T*>(&data));
	}

	template <typename T>
	constexpr T const& get() const
	{
		if (!holds_alternative<T>(*this))
		{
			throw bad_variant_access("Variant doesn't hold this type");
		}
		return static_cast<T const&>(*reinterpret_cast<T const*>(&data));
	}

	template<typename T>
	constexpr std::add_pointer_t<T> get_if() noexcept
	{
		if (!holds_alternative<T>(*this))
		{
			return nullptr;
		}
		return reinterpret_cast<std::add_pointer_t<T>>(&data);
	}

	template<typename T>
	constexpr std::add_pointer_t<const T> get_if() const noexcept
	{
		if (!holds_alternative<T>(*this))
		{
			return nullptr;
		}
		return reinterpret_cast<std::add_pointer_t<const T>>(&data);
	}

	template<size_t I>
	constexpr std::add_pointer_t<get_type<I>> get_if() noexcept
	{
		return get_if<get_type<I>>();
	}

	template<size_t I>
	constexpr std::add_pointer_t<const get_type<I>> get_if() const noexcept
	{
		return get_if<get_type<I>>();
	}
private:
	using index_t = variant_index_t<sizeof...(Ts) + 1>;

	std::aligned_storage_t<get_size_v<T0, Ts...>, get_align_v<T0, Ts...>> data;
	index_t cur_type;

	constexpr void set_index(size_t ind) noexcept
	{
		cur_type = static_cast<index_t>(ind);
	}
};

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
struct variant_size<const T> : variant_size<T> {};

template <typename T, typename ... Ts>
struct variant_size<variant<T, Ts...>> : std::integral_constant<size_t, sizeof...(Ts) + 1> {};

template <typename T>
inline constexpr size_t variant_size_v = variant_size<T>::value;

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
	return v.get<I>();
}

template <size_t I, typename ... Ts>
constexpr decltype(auto) get(variant<Ts...> const& v)
{
	return v.get<I>();
}

template <size_t I, typename ... Ts>
constexpr decltype(auto) get(variant<Ts...>&& v)
{
	return std::move(v.get<I>());
}

template <size_t I, typename ... Ts>
constexpr decltype(auto) get(variant<Ts...> const&& v)
{
	return std::move(v.get<I>());
}

template <typename T, typename ... Ts>
constexpr decltype(auto) get(variant<Ts...>& v)
{
	return v.get<T>();
}

template <typename T, typename ... Ts>
constexpr decltype(auto) get(variant<Ts...> const& v)
{
	return v.get<T>();
}

template <typename T, typename ... Ts>
constexpr decltype(auto) get(variant<Ts...>&& v)
{
	return std::move(v.get<T>());
}

template <typename T, typename ... Ts>
constexpr decltype(auto) get(variant<Ts...> const&& v)
{
	return std::move(v.get<T>());
}

template <size_t I, typename ... Ts>
constexpr decltype(auto) get_if(variant<Ts...>* pv) noexcept
{
	return (!pv ? nullptr : pv->get_if<I>());
}

template <size_t I, typename ... Ts>
constexpr decltype(auto) get_if(variant<Ts...> const* pv) noexcept
{
	return (!pv ? nullptr : pv->get_if<I>());
}

template <typename T, typename ... Ts>
constexpr decltype(auto) get_if(variant<Ts...>* pv) noexcept
{
	if (!pv)
	{
		return nullptr;
	}
	else
	{
		return pv->get_if<T>();
	}
}

template <typename T, typename ... Ts>
constexpr decltype(auto) get_if(variant<Ts...> const* pv) noexcept
{
	if (!pv)
	{
		return nullptr;
	}
	else
	{
		return pv->get_if<T>();
	}
}


