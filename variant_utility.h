#pragma once
#include <utility>
#include <algorithm>
#include <cstdio>
#include <memory>
#include <exception>
#include <type_traits>
#include <limits>
#include <algorithm>

struct monostate {};

constexpr bool operator<(monostate, monostate) noexcept { return false; }
constexpr bool operator>(monostate, monostate) noexcept { return false; }
constexpr bool operator<=(monostate, monostate) noexcept { return true; }
constexpr bool operator>=(monostate, monostate) noexcept { return true; }
constexpr bool operator==(monostate, monostate) noexcept { return true; }
constexpr bool operator!=(monostate, monostate) noexcept { return false; }


template<typename T, template <typename...> typename TT>
struct is_specialization : std::false_type {};

template<template <typename...> typename TT, typename ... Ts>
struct is_specialization<TT<Ts...>, TT> : std::true_type {};


template<typename T>
struct is_in_place_index_specialization
{
	static constexpr bool value = 0;
};

template<size_t I>
struct is_in_place_index_specialization<std::in_place_index_t<I>>
{
	static constexpr bool value = 1;
};

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

template<typename U, typename T, typename ... Ts>
struct is_unique
{
	static constexpr size_t value = std::is_same_v<U, T> + is_unique<U, Ts...>::value;
};

template<typename U, typename T>
struct is_unique<U, T>
{
	static constexpr size_t value = std::is_same_v<U, T>;
};

template<typename U, typename ... Ts>
inline constexpr bool is_unique_v = is_unique<U, Ts...>::value == 1;


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

	template<typename STORAGE>
	void construct_from_storage(size_t ind, STORAGE&& other) {}

	template<typename STORAGE>
	void assign_from_storage(size_t ind, STORAGE&& other) {}

	template<typename T>
	void assign_from(size_t ind, T&& other) {}
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

	storage() noexcept {}
	storage(storage const&) = default;

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

	template<typename ... Args>
	void construct_from_args(size_t ind, Args&& ... args)
	{
		if (ind == 0)
		{
			new(&head) T0(std::forward<Args>(args)...);
		}
		else
		{
			tail.construct_from(ind - 1, std::forward<Args>(args)...);
		}
	}

	template<typename STORAGE>
	void construct_from_storage(size_t ind, STORAGE&& other)
	{
		if (ind == 0)
		{
			new(&head) T0(std::forward<STORAGE>(other).head);
		}
		else
		{
			tail.construct_from_storage(ind - 1, std::forward<STORAGE>(other).tail);
		}
	}

	template<typename STORAGE>
	void assign_from_storage(size_t ind, STORAGE&& other)
	{
		if (ind == 0)
		{
			head = std::forward<STORAGE>(other).head;
		}
		else
		{
			tail.assign_from_storage(ind - 1, std::forward<STORAGE>(other).tail);
		}
	}

	template<typename T>
	void assign_from(size_t ind, T&& other)
	{
		if (ind == 0)
		{
			head = std::forward<T>(other);
		}
		else
		{
			tail.assign_from_storage(ind - 1, std::forward<T>(other));
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

	storage() noexcept {}
	storage(storage const&) = default;

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

	template<typename ... Args>
	void construct_from_args(size_t ind, Args&& ... args)
	{
		if (ind == 0)
		{
			new(&head) T0(std::forward<Args>(args)...);
		}
		else
		{
			tail.construct_from(ind - 1, std::forward<Args>(args)...);
		}
	}

	template<typename STORAGE>
	void construct_from_storage(size_t ind, STORAGE&& other) 
	{
		if (ind == 0)
		{
			new(&head) T0(std::forward<STORAGE>(other).head);
		}
		else
		{
			tail.construct_from_storage(ind - 1, std::forward<STORAGE>(other).tail);
		}
	}

	template<typename STORAGE>
	void assign_from_storage(size_t ind, STORAGE&& other)
	{
		if (ind == 0)
		{
			head = std::forward<STORAGE>(other).head;
		}
		else
		{
			tail.assign_from_storage(ind - 1, std::forward<STORAGE>(other).tail);
		}
	}

	template<typename T>
	void assign_from(size_t ind, T&& other)
	{
		if (ind == 0)
		{
			head = std::forward<T>(other);
		}
		else
		{
			tail.assign_from_storage(ind - 1, std::forward<T>(other));
		}
	}

	~storage() noexcept {}
};

template<size_t I, typename STORAGE, std::enable_if_t<(I == 0), int> = 0>
constexpr decltype(auto) raw_get(STORAGE&& st)
{
	return (std::forward<STORAGE>(st).head);
}

template<size_t I, typename STORAGE, std::enable_if_t<(I != 0), int> = 0>
constexpr decltype(auto) raw_get(STORAGE&& st)
{
	return raw_get<I - 1>(std::forward<STORAGE>(st).tail);
}

template<bool triv_destr, typename ... Ts>
struct destroyable_storage : storage_t<Ts...>
{
protected:
	using index_t = variant_index_t<sizeof...(Ts)>;
	using base = storage_t<Ts...>;

	index_t cur_type;

	constexpr void set_index(size_t ind) noexcept
	{
		cur_type = static_cast<index_t>(ind);
	}

public:
	destroyable_storage() = default;
	destroyable_storage(destroyable_storage const&) = default;
	destroyable_storage(destroyable_storage&&) = default;

	template<size_t I, typename ... Args>
	constexpr explicit destroyable_storage(std::integral_constant<size_t, I>, Args&& ... args)
		noexcept(std::is_nothrow_constructible_v<base, std::integral_constant<size_t, I>, Args...>)
		: base(std::integral_constant<size_t, I>{}, std::forward<Args>(args)...)
		, cur_type(static_cast<index_t>(I))
	{}

	constexpr bool valueless_by_exception() const noexcept
	{
		return index() == variant_npos;
	}

	constexpr size_t index() const noexcept
	{
		return static_cast<size_t>(cur_type);
	}

	constexpr base& get_storage() & noexcept
	{
		return *this;
	}

	constexpr const base& get_storage() const & noexcept
	{
		return *this;
	}

	constexpr base&& get_storage() && noexcept
	{
		return std::move(*this);
	}

	constexpr const base&& get_storage() const && noexcept
	{
		return std::move(*this);
	}

	~destroyable_storage() noexcept = default;
};

template<typename ... Ts>
struct destroyable_storage<0, Ts...> : destroyable_storage<1, Ts...>
{
	using base = destroyable_storage<1, Ts...>;

	using base::base;
	destroyable_storage() = default;
	destroyable_storage(destroyable_storage const&) = default;
	destroyable_storage(destroyable_storage&&) = default;

	~destroyable_storage() noexcept
	{
		if (!base::valueless_by_exception())
		{
			base::reset(base::index());
		}
	}
};

template<typename ... Ts>
using destroyable_storage_t = destroyable_storage<std::conjunction_v<std::is_trivially_destructible<Ts>...>, Ts...>;

template<bool copyable, typename ... Ts>
struct copy_storage : destroyable_storage_t<Ts...>
{
	using copy_base = destroyable_storage_t<Ts...>;

	using copy_base::copy_base;
	copy_storage() = default;
	copy_storage(copy_storage const& other)
	{
		copy_base::cur_type = other.cur_type;
		if (!copy_base::valueless_by_exception())
		{
			copy_base::base::construct_from_storage(other.index(), other);
		}
	}
	copy_storage(copy_storage&&) = default;
};


template<typename ... Ts>
struct copy_storage<0, Ts...> : copy_storage<1, Ts...>
{
	using copy_base = copy_storage<1, Ts...>;

	using copy_base::copy_base;
	copy_storage() = default;
	copy_storage(copy_storage const&) = delete;
	copy_storage(copy_storage&&) = default;
};

template<typename ... Ts>
using copy_storage_t = copy_storage<std::conjunction_v<std::is_copy_constructible<Ts>...>, Ts...>;

template<bool movable, typename ... Ts>
struct move_storage : copy_storage_t<Ts...>
{
	using move_base = copy_storage_t<Ts...>;
	
	using move_base::move_base;
	move_storage() = default;
	move_storage(move_storage const&) = default;
	move_storage(move_storage&& other) noexcept(std::conjunction_v<std::is_nothrow_move_constructible<Ts>...>)
	{
		move_base::cur_type = other.cur_type;
		if (!move_base::valueless_by_exception())
		{
			move_base::construct_from_storage(other.index(), std::move(other));
		}
	}
};

template<typename ... Ts>
struct move_storage<0, Ts...> : move_storage<1, Ts...>
{
	using move_base = move_storage<1, Ts...>;

	using move_base::move_base;
	move_storage() = default;
	move_storage(move_storage const&) = default;
	move_storage(move_storage&&) = delete;
};

template<typename ... Ts>
using move_storage_t = move_storage<std::conjunction_v<std::is_move_constructible<Ts>...>, Ts...>;


template<bool move_assign, typename ... Ts>
struct move_assign_storage : move_storage_t<Ts...>
{
	using move_assign_base = move_storage_t<Ts...>;

	using move_assign_base::move_assign_base;
	using move_assign_base::valueless_by_exception;
	using move_assign_base::reset;
	using move_assign_base::index;
	using move_assign_base::set_index;
	using move_assign_base::construct_from_storage;
	using move_assign_base::assign_from_storage;

	void smart_reset() noexcept
	{
		if (!valueless_by_exception())
		{
			reset(index());
		}
		set_index(variant_npos);
	}


	move_assign_storage() = default;
	move_assign_storage(move_assign_storage const&) = default;
	move_assign_storage(move_assign_storage&& other) = default;
	move_assign_storage& operator=(move_assign_storage&& other) noexcept(std::conjunction_v<std::is_nothrow_move_constructible<Ts>..., std::is_nothrow_move_assignable<Ts>...>)
	{
		if (other.valueless_by_exception())
		{
			smart_reset();
		}
		else
		{
			if (index() == other.index())
			{
				assign_from_storage(index(), std::move(other));
			}
			else
			{
				smart_reset();
				set_index(other.index());
				try
				{
					construct_from_storage(index(), std::move(other));
				}
				catch (...)
				{
					set_index(variant_npos);
				}
			}
		}
		return *this;
	}
};

template<typename ... Ts>
struct move_assign_storage<0, Ts...> : move_assign_storage<1, Ts...>
{
	using move_assign_base = move_assign_storage<1, Ts...>;

	using move_assign_base::move_assign_base;
	move_assign_storage() = default;
	move_assign_storage(move_assign_storage const&) = default;
	move_assign_storage(move_assign_storage&& other) = default;
	move_assign_storage& operator=(move_assign_storage&& other) = delete;
};

template<typename ... Ts>
using move_assign_storage_t = move_assign_storage<std::conjunction_v<std::is_move_constructible<Ts>..., std::is_move_assignable<Ts>...>, Ts...>;

template<bool copy_assign, typename ... Ts>
struct copy_assign_storage : move_assign_storage_t<Ts...>
{
	using copy_assign_base = move_assign_storage_t<Ts...>;

	using copy_assign_base::copy_assign_base;
	using copy_assign_base::index;
	using copy_assign_base::set_index;
	using copy_assign_base::smart_reset;
	using copy_assign_base::assign_from_storage;
	using copy_assign_base::construct_from_storage;

	copy_assign_storage() = default;
	copy_assign_storage(copy_assign_storage const&) = default;
	copy_assign_storage(copy_assign_storage&& other) = default;
	copy_assign_storage& operator=(copy_assign_storage&& other) = default;
	copy_assign_storage& operator=(copy_assign_storage const& other)
	{
		if (other.valueless_by_exception())
		{
			smart_reset();
		}
		else
		{
			if (index() == other.index())
			{
				assign_from_storage(index(), other);
			}
			else
			{
				smart_reset();
				try
				{
					set_index(other.index());
					construct_from_storage(index(), other);
				} 
				catch (...)
				{
					set_index(variant_npos);
					return this->operator=(copy_assign_storage(other));
				}
			}
		}
		return *this;
	}
};

template<typename ... Ts>
struct copy_assign_storage<0, Ts...> : copy_assign_storage<1, Ts...>
{
	using copy_assign_base = copy_assign_storage<1, Ts...>;

	using copy_assign_base::copy_assign_base;

	copy_assign_storage() = default;
	copy_assign_storage(copy_assign_storage const&) = default;
	copy_assign_storage(copy_assign_storage&& other) = default;
	copy_assign_storage& operator=(copy_assign_storage&& other) = default;
	copy_assign_storage& operator=(copy_assign_storage const& other) = delete;
};

template<typename ... Ts>
using copy_assign_storage_t = copy_assign_storage<std::conjunction_v<std::is_copy_constructible<Ts>..., std::is_copy_assignable<Ts>...>, Ts...>;

auto comparer = [](auto&& action) constexpr {
	return ([action](auto&& a, auto&& b) constexpr -> bool  {
		if constexpr(std::is_same_v<decltype(a), decltype(b)>)
		{
			return action(a, b);
		}
		else
		{
			return 0;
		}
	});
};

auto less = comparer([](auto&& x, auto&& y) constexpr -> bool  {return x < y; });
auto greater = comparer([](auto&& x, auto&& y) constexpr -> bool  {return x > y; });
auto equal = comparer([](auto&& x, auto&& y) constexpr -> bool  {return x == y; });
auto neq = comparer([](auto&& x, auto&& y) constexpr -> bool  {return x != y; });
auto leq = comparer([](auto&& x, auto&& y) constexpr -> bool  {return x <= y});
auto geq = comparer([](auto&& x, auto&& y) constexpr -> bool  {return x >= y});