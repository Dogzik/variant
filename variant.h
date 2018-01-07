#pragma once
#include "variant_utility.h"


template <typename T0, typename ... Ts> struct variant;

template <typename Visitor, typename ... Variants>
constexpr decltype(auto) visit(Visitor&& vis, Variants&& ... vars);

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
struct variant_size<variant<T, Ts...>> : std::integral_constant<size_t, sizeof...(Ts) + 1> {};

template <typename T>
inline constexpr size_t variant_size_v = variant_size<T>::value;

template <typename X, typename ... Us>
constexpr bool holds_alternative(const variant<Us...>& v) noexcept;

template <typename T0, typename ... Ts>
struct variant : move_storage_t<T0, Ts...>
{
	using variant_base = move_storage_t<T0, Ts...>;

	using variant_base::valueless_by_exception;
	using variant_base::set_index;
	using variant_base::index;
	using variant_base::reset;
	using variant_base::construct_from_storage;

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
			, std::enable_if_t<(I < sizeof...(Ts) + 1) 
			&& (std::is_constructible_v<T, Args...>)
			&& is_unique_v<T, T0, Ts...>, int> = 0>
	constexpr explicit variant(std::in_place_type_t<T>, Args&&... args)
		: variant_base(type_ind<get_type_ind<T, T0, Ts...>()>{}, std::forward<Args>(args)...)
	{}

	variant(const variant&) = default;
	variant(variant&&) = default;
	~variant() noexcept = default;

	template <size_t I, typename ... Args,
			std::enable_if_t<(I < (sizeof...(Ts) + 1)) 
			&& std::is_constructible_v<get_type<I>, Args...>, int> = 0,
			typename TT = get_type<I>>
	TT& emplace(Args&&... args)
	{
		if (valueless_by_exception())
		{
			reset(index());
		}
		try
		{
			new(this) variant(std::in_place_index_t<I>{}, std::forward<Args>(args)...);
		}
		catch (...)
		{
			set_index(variant_npos);
		}
		return raw_get<I>(*this);
	}

	template <typename T, typename ... Args,
			std::enable_if_t<is_unique_v<T, T0, Ts...>
			&& std::is_constructible_v<T, Args...>, int> = 0>
	T& emplace(Args&&... args)
	{
		constexpr size_t I = get_type_ind<T, T0, Ts...>();
		return emplace<I>(std::forward<Args>(args)...);
	}

	void swap(variant& other) 
		noexcept(std::conjunction_v<std::is_nothrow_move_constructible<Ts>..., std::is_nothrow_move_constructible<T0>,
				std::is_nothrow_swappable<Ts>..., std::is_nothrow_swappable<T0>>)
	{
		static_assert(std::conjunction_v<std::is_move_constructible<Ts>..., std::is_move_constructible<T0>>,
			"All types have to be move constructible");
		static_assert(std::conjunction_v<std::is_swappable<Ts>..., std::is_swappable<T0>>,
			"All types have to be swappable");
		swap_impl(other);
	}

private:

	void swap_impl(variant& other)
	{
		if (index() != other.index())
		{
			variant tmp(std::move(*this));
			emplace_from(std::move(other));
			other.emplace_from(std::move(tmp));
		}
		else
		{
			if (!valueless_by_exception())
			{
				//swap_storage(index(), *this, other);
				auto swapper = [](auto&& a, auto && b) -> void
				{
					if constexpr(std::is_same_v<decltype(a), decltype(b)>)
					{
						std::swap(a, b);
					}
					else
					{
						return;
					}
				};
				visit(swapper, *this, other);
			}
		}
	}

	void emplace_from(variant&& other)
	{
		if (!valueless_by_exception())
		{
			reset(index());
		}
		set_index(other.index());
		if (!other.valueless_by_exception())
		{
			construct_from_storage(index(), std::move(other));
		}
	}
};


template<typename ... Ts, 
	std::enable_if_t<std::conjunction_v<std::is_move_constructible<Ts>...>
					&& std::conjunction_v<std::is_move_constructible<Ts>...>, int> = 0>
void swap(variant<Ts...>& a, variant<Ts...>& b) noexcept(noexcept(a.swap(b)))
{
	a.swap(b);
}

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

template <size_t I, typename ... Ts>
constexpr decltype(auto) get_if(variant<Ts...>* pv) noexcept
{
	static_assert(I < sizeof...(Ts), "Variant Index out of range");
	return ((!pv || (pv->index() != I)) ? nullptr : &get<I>(*pv));
}

template <size_t I, typename ... Ts>
constexpr decltype(auto) get_if(variant<Ts...> const* pv) noexcept
{
	static_assert(I < sizeof...(Ts), "Variant Index out of range");
	return ((!pv || (pv->index() != I)) ? nullptr : &get<I>(*pv));
}

template <typename T, typename ... Ts>
constexpr decltype(auto) get_if(variant<Ts...>* pv) noexcept
{
	constexpr size_t I = get_type_ind<T, Ts...>();
	static_assert(is_unique_v<T, Ts...>, "T has to be unique varriant alternative");
	return get_if<I>(pv);
}

template <typename T, typename ... Ts>
constexpr decltype(auto) get_if(variant<Ts...> const* pv) noexcept
{
	constexpr size_t I = get_type_ind<T, Ts...>();
	static_assert(is_unique_v<T, Ts...>, "T has to be unique varriant alternative");
	return get_if<I>(pv);
}


template<typename T, size_t ... dimens>
struct multi_array
{
	constexpr const T& access() const
	{
		return data;
	}
	T data;
};


template<typename T, size_t first_dim, size_t ... rest_dims>
struct multi_array<T, first_dim, rest_dims...>
{
	template<typename ... size_ts>
	constexpr const T& access(size_t first_ind, size_ts ... other_inds) const
	{
		return data_arr[first_ind].access(other_inds...);
	}

	multi_array<T, rest_dims...> data_arr[first_dim];
};

template<typename ... Ts>
struct many_vars;

template<size_t I, typename T, typename ... Ts>
struct get_ith_type
{
	typedef typename get_ith_type<I - 1, Ts...>::type type;
};

template<typename T, typename ... Ts>
struct get_ith_type<0, T, Ts...>
{
	typedef T type;
};

template<size_t I, typename ... Ts>
using get_ith_type_t = typename get_ith_type<I, Ts...>::type;

template<typename arr_type, typename varians, typename idex_seq>
struct table_impl;

template<typename ret, typename vis, typename ... vars, size_t dim_first, size_t ... dim_rest, size_t ... inds>
struct table_impl<multi_array<ret (*)(vis, vars...), dim_first, dim_rest...>, many_vars<vars...>, std::index_sequence<inds...>>
{
	using cur_var = get_ith_type_t<sizeof...(inds), std::decay_t<vars>...>;

	using arr_type = multi_array<ret(*)(vis, vars...), dim_first, dim_rest...>;

	static constexpr arr_type make_table()
	{
		return make_many(std::make_index_sequence<variant_size_v<cur_var>>{});
	}

	template<size_t ... var_inds>
	static constexpr arr_type make_many(std::index_sequence<var_inds...>)
	{
		using smaller_arr = multi_array<ret(*)(vis, vars...), dim_rest...>;
		return arr_type{ make_one<var_inds, smaller_arr>()... };
	}

	template<size_t ind, typename arr>
	static constexpr arr make_one()
	{
		return table_impl<arr, many_vars<vars...>, std::index_sequence<inds..., ind>>::make_table();
	}
};

template<typename ret, typename vis, typename ... vars, size_t ... inds>
struct table_impl<multi_array<ret(*)(vis, vars...)>, many_vars<vars...>, std::index_sequence<inds...>>
{
	using arr_type = multi_array<ret(*)(vis, vars...)>;

	static constexpr decltype(auto) invoke(vis visitor, vars ... variants)
	{
		return std::forward<vis>(visitor)(get<inds>(std::forward<vars>(variants))...);
	}

	static constexpr arr_type make_table()
	{
		return arr_type{ & invoke };
	}
};

template<typename ret, typename vis, typename ... vars>
struct gen_table
{
	using func_ptr = ret(*)(vis, vars...);

	using arr_type = multi_array<func_ptr, variant_size_v<std::remove_reference_t<vars>>...>;

	static constexpr arr_type table = table_impl<arr_type, many_vars<vars...>, std::index_sequence<>>::make_table();

};

template<typename Visitor, typename ... Variants>
constexpr decltype(auto) visit(Visitor&& vis, Variants&& ... vars)
{
	if ((vars.valueless_by_exception() || ...))
	{
		throw bad_variant_access();
	}

	using ret_type = decltype(std::forward<Visitor>(vis)(get<0>(std::forward<Variants>(vars))...));

	constexpr auto& v_table = gen_table<ret_type, Visitor&&, Variants&&...>::table;
	auto func_ptr = v_table.access(vars.index()...);

	return (*func_ptr)(std::forward<Visitor>(vis), std::forward<Variants>(vars)...);
}