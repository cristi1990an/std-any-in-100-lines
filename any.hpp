#pragma once

#include <iostream>
#include <memory>
#include <concepts>

namespace detail
{
	struct init_wrap_t {};
	static constexpr init_wrap_t init_wrap;

	struct wrapper_base
	{
		wrapper_base() = default;
		wrapper_base(const wrapper_base&) = delete;
		wrapper_base(wrapper_base&&) = delete;
		wrapper_base& operator=(const wrapper_base&) = delete;
		wrapper_base& operator=(wrapper_base&&) = delete;

		virtual constexpr std::unique_ptr<wrapper_base> make_copy() const = 0;
		virtual constexpr bool try_copy_assign(wrapper_base* copy_to) const = 0;
		virtual constexpr ~wrapper_base() = default;
	};

	template<typename T>
	struct wrapper : public wrapper_base
	{
		wrapper() = delete;

		template<typename ... Args>
			requires std::constructible_from<T, Args...>
		constexpr wrapper(init_wrap_t, Args&&...args)
			: value_(std::forward<Args>(args)...)
		{

		}

		constexpr std::unique_ptr<wrapper_base> make_copy() const final
		{
			return std::make_unique<wrapper>(init_wrap, value_);
		}

		constexpr bool try_copy_assign(wrapper_base* copy_to) const final
		{
			if (auto* other = dynamic_cast<wrapper*>(copy_to))
			{
				other->value_ = value_;
				return true;
			}
			return false;
		}

		[[msvc::no_unique_address]] T value_;
	};
}

class any
{
public:
	constexpr any() = default;
	constexpr any(any&& other) noexcept = default;
	constexpr any& operator=(any&& other) noexcept = default;

	constexpr any(const any& other)
		: data_{ other.data_ ? other.data_->make_copy() : nullptr }
	{

	}

	template<typename T> requires (!std::same_as<any, T>)
		constexpr any(const T& value)
		: data_{ std::make_unique<detail::wrapper<T>>(detail::init_wrap, value) }
	{

	}

	template<typename T> requires (!std::same_as<any, T>)
		constexpr any& operator=(const T& value)
	{
		auto* wrapper = dynamic_cast<detail::wrapper<T>*>(data_.get());

		if (wrapper)
		{
			wrapper->value_ = value;
		}
		else
		{
			data_ = std::make_unique<detail::wrapper<T>>(detail::init_wrap, value);
		}

		return *this;
	}


	constexpr any& operator=(const any& other)
	{
		const bool ok = other.data_->try_copy_assign(data_.get());
		if (!ok)
		{
			data_ = other.data_->make_copy();
		}

		return *this;
	}

	template<typename T>
	constexpr T* cast_to() const noexcept
	{
		auto* wrapper = dynamic_cast<detail::wrapper<T>*>(data_.get());
		return wrapper ? std::addressof(wrapper->value_) : nullptr;
	}

private:
	std::unique_ptr<detail::wrapper_base> data_;
};
