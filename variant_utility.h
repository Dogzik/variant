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


template<typename T>
struct single_type
{
	static T f(T);
};

template<typename T, typename ... Ts>
struct type_choser : single_type<T>, type_choser<Ts...>
{
	using type_choser<Ts...>::f;
	using single_type<T>::f;
};

template<typename T>
struct type_choser<T> : single_type<T>
{
	using single_type<T>::f;
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

struct no_type;


template<bool triv_destr, typename ... Ts>
struct storage {
	void reset(size_t ind) {};
};

template<typename ... Ts>
using storage_t = storage<std::conjunction_v<std::is_trivially_destructible<Ts>...>, Ts...>;

template<typename T0, typename ... Ts>
struct storage<1, T0, Ts...>
{
	union
	{
		T0 head;
		storage_t<Ts...> tail;
	};

	template<typename ... Args>
	constexpr explicit storage(std::integral_constant<size_t, 0>, Args&& ... args) noexcept(std::is_nothrow_constructible_v<T0, Args...>)
		: head(std::forward<Args>(args)...)
	{}

	template<size_t I, typename ... Args>
	constexpr explicit storage(std::integral_constant<size_t, I>, Args&& ... args) 
			noexcept(std::is_nothrow_constructible_v<storage_t<Ts...>, std::integral_constant<size_t, I - 1>, Args...>)
		: tail(std::integral_constant<size_t, I - 1>{}, std::forward<Args>(args)...)
	{}

	void reset(size_t ind) noexcept
	{
		if (ind == 0)
		{
			head.~T0();
		}
		else
		{
			tail.reset(ind - 1);
		}
	}

	~storage() noexcept = default;
};


template<typename T0, typename ... Ts>
struct storage<0, T0, Ts...>
{
	union
	{
		T0 head;
		storage_t<Ts...> tail;
	};

	template<typename ... Args>
	constexpr explicit storage(std::integral_constant<size_t, 0>, Args&& ... args) noexcept(std::is_nothrow_constructible_v<T0, Args...>)
		: head(std::forward<Args>(args)...)
	{}

	template<size_t I, typename ... Args>
	constexpr explicit storage(std::integral_constant<size_t, I>, Args&& ... args)
		noexcept(std::is_nothrow_constructible_v<storage_t<Ts...>, std::integral_constant<size_t, I - 1>, Args...>)
		: tail(std::integral_constant<size_t, I - 1>{}, std::forward<Args>(args)...)
	{}

	void reset(size_t ind) noexcept
	{
		if (ind == 0)
		{
			head.~T0();
		}
		else
		{
			tail.reset(ind - 1);
		}
	}

	~storage() noexcept {}
};

template<bool triv_destr, typename ... Ts>
struct destroyable_storage : storage_t<Ts...>
{
	using index_t = variant_index_t<sizeof...(Ts)>;
	using base = storage_t<Ts...>;

	index_t cur_type;

	constexpr size_t index() const noexcept
	{
		return static_cast<size_t>(cur_type);
	}

	constexpr void set_index(size_t ind) noexcept
	{
		cur_type = static_cast<index_t>(ind);
	}

	constexpr bool valueless_by_exception() const noexcept
	{
		return index() == variant_npos;
	}

	template<size_t I, typename ... Args>
	constexpr explicit destroyable_storage(std::integral_constant<size_t, I>, Args&& ... args)
		noexcept(std::is_nothrow_constructible_v<base, std::integral_constant<size_t, I>, Args...>)
		: base(std::integral_constant<size_t, I>{}, std::forward<Args>(args)...)
		, cur_type(static_cast<index_t>(I))
	{}


	~destroyable_storage() noexcept = default;
};

template<typename ... Ts>
struct destroyable_storage<0, Ts...> : storage_t<Ts...>
{
	using index_t = variant_index_t<sizeof...(Ts)>;
	using base = storage_t<Ts...>;

	index_t cur_type;

	constexpr size_t index() const noexcept
	{
		return static_cast<size_t>(cur_type);
	}

	constexpr void set_index(size_t ind) noexcept
	{
		cur_type = static_cast<index_t>(ind);
	}

	constexpr bool valueless_by_exception() const noexcept
	{
		return index() == variant_npos;
	}

	template<size_t I, typename ... Args>
	constexpr explicit destroyable_storage(std::integral_constant<size_t, I>, Args&& ... args)
		noexcept(std::is_nothrow_constructible_v<base, std::integral_constant<size_t, I>, Args...>)
		: base(std::integral_constant<size_t, I>{}, std::forward<Args>(args)...)
		, cur_type(static_cast<index_t>(I))
	{}

	~destroyable_storage() noexcept
	{
		if (!valueless_by_exception())
		{
			base::reset(index());
		}
	}
};

template<typename ... Ts>
using destroyable_storage_t = destroyable_storage<std::conjunction_v<std::is_trivially_destructible<Ts>...>, Ts...>;