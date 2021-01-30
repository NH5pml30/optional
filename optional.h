#pragma once

#include <type_traits>

constexpr inline struct nullopt_t {} nullopt;

constexpr inline struct in_place_t {} in_place;

namespace optional_internal
{
  template<typename T, typename = void>
  union storage;

  template<typename T>
  union storage<T, std::enable_if_t<std::is_trivially_destructible_v<T>>>
  {
    T value;
    struct {} dummy = {};

    template<typename... Args>
    static constexpr bool is_nothrow_constructible_v = std::is_nothrow_constructible_v<T, Args...>;

    constexpr storage() noexcept {}

    template<typename... Args>
    explicit constexpr storage(Args &&... args) noexcept(is_nothrow_constructible_v<Args &&...>)
        : value(std::forward<Args>(args)...)
    {
    }
  };

  template<typename T>
  union storage<T, std::enable_if_t<!std::is_trivially_destructible_v<T>>>
  {
    T value;
    struct {} dummy = {};

    template<typename... Args>
    static constexpr bool is_nothrow_constructible_v = std::is_nothrow_constructible_v<T, Args...>;

    constexpr storage() noexcept {}

    template<typename... Args>
    explicit constexpr storage(Args &&... args) noexcept(is_nothrow_constructible_v<Args &&...>)
        : value(std::forward<Args>(args)...)
    {
    }

    ~storage() {}
  };

  template<typename T, typename = void>
  class copy_base;

  template<typename T>
  class copy_base<T, std::enable_if_t<std::is_trivially_copyable_v<T>>>
  {
  protected:
    template<typename... Args>
    static constexpr bool is_nothrow_constructible_v = storage<T>::template is_nothrow_constructible_v<Args...>;

    template<typename... Args>
    explicit constexpr copy_base(in_place_t, Args &&... args) noexcept(
        is_nothrow_constructible_v<Args &&...>)
        : stg(std::forward<Args>(args)...), has_value_(true)
    {
    }

    constexpr copy_base(const copy_base &) = default;
    constexpr copy_base(copy_base &&) = default;

    constexpr copy_base &operator=(const copy_base &) = default;
    constexpr copy_base &operator=(copy_base &&) = default;

    constexpr copy_base() = default;
    ~copy_base() = default;

    void reset() noexcept
    {
      if (this->has_value_)
      {
        this->stg.value.~T();
        this->has_value_ = false;
      }
    }

    template<typename... Args>
    void emplace(Args &&... args) noexcept(is_nothrow_constructible_v<Args &&...>)
    {
      reset();
      new (&this->stg.value) T(std::forward<Args>(args)...);
      has_value_ = true;
    }
  protected:
    storage<T> stg;
    bool has_value_ = false;
  };

  template<typename T>
  class copy_base<T, std::enable_if_t<!std::is_trivially_copyable_v<T>>>
  {
  protected:
    template<typename... Args>
    static constexpr bool is_nothrow_constructible_v = storage<T>::template is_nothrow_constructible_v<Args...>;

    template<typename... Args>
    explicit constexpr copy_base(in_place_t, Args &&... args) noexcept(
        is_nothrow_constructible_v<Args &&...>)
        : stg(std::forward<Args>(args)...), has_value_(true)
    {
    }

    copy_base(copy_base &&other) noexcept(std::is_nothrow_move_constructible_v<T>)
    {
      if (other.has_value_)
        new (&stg.value) T(std::move(other.stg.value));
      has_value_ = other.has_value_;
    }

    copy_base(const copy_base &other) noexcept(std::is_nothrow_copy_constructible_v<T>)
    {
      if (other.has_value_)
        new (&stg.value) T(other.stg.value);
      has_value_ = other.has_value_;
    }

    copy_base &operator=(const copy_base &other) noexcept(
        std::is_nothrow_copy_constructible_v<T> && std::is_nothrow_copy_assignable_v<T>)
    {
      if (other.has_value_)
      {
        if (has_value_)
          stg.value = other.stg.value;
        else
          new (&stg.value) T(other.stg.value);
      }
      else if (has_value_)
        stg.value.~T();
      has_value_ = other.has_value_;
      return *this;
    }
    copy_base &operator=(copy_base &&other) noexcept(
        std::is_nothrow_move_constructible_v<T> && std::is_nothrow_move_assignable_v<T>)
    {
      if (other.has_value_)
      {
        if (has_value_)
          stg.value = std::move(other.stg.value);
        else
          new (&stg.value) T(std::move(other.stg.value));
      }
      else if (has_value_)
        stg.value.~T();
      has_value_ = other.has_value_;
      return *this;
    }

    constexpr copy_base() = default;
    ~copy_base() = default;

    void reset() noexcept
    {
      if (has_value_)
      {
        stg.value.~T();
        has_value_ = false;
      }
    }

    template<typename... Args>
    void emplace(Args &&... args) noexcept(is_nothrow_constructible_v<Args &&...>)
    {
      reset();
      new (&this->stg.value) T(std::forward<Args>(args)...);
      has_value_ = true;
    }

    storage<T> stg;
    bool has_value_ = false;
  };

  template<typename T, typename = void>
  class destroy_base;

  template<typename T>
  class destroy_base<T, std::enable_if_t<std::is_trivially_destructible_v<T>>>
      : public copy_base<T>
  {
  protected:
    using copy_base<T>::copy_base;

    constexpr destroy_base &operator=(const destroy_base &) = default;
    constexpr destroy_base &operator=(destroy_base &&) = default;

    ~destroy_base() = default;
  };

  template<typename T>
  class destroy_base<T, std::enable_if_t<!std::is_trivially_destructible_v<T>>>
      : public copy_base<T>
  {
  protected:
    using copy_base<T>::copy_base;

    destroy_base(const destroy_base &) = default;
    destroy_base(destroy_base &&) = default;

    constexpr destroy_base &operator=(const destroy_base &) = default;
    constexpr destroy_base &operator=(destroy_base &&) = default;

    ~destroy_base()
    {
      this->reset();
    }
  };
}  // namespace optional_internal

template<typename T>
class optional : public optional_internal::destroy_base<T>
{
  using base_t = optional_internal::destroy_base<T>;
public:
  constexpr optional() = default;
  constexpr optional(nullopt_t) noexcept : optional() {}

  constexpr optional(const optional &) = default;
  constexpr optional(optional &&) = default;

  constexpr optional &operator=(const optional &) = default;
  constexpr optional &operator=(optional &&) = default;

  constexpr optional(T value) noexcept(std::is_move_constructible_v<T>)
      : optional(in_place, std::move(value))
  {
  }

  template<typename... Args>
  explicit constexpr optional(in_place_t, Args &&... args) noexcept(
      base_t::template is_nothrow_constructible_v<Args &&...>)
      : base_t(in_place, std::forward<Args>(args)...)
  {
  }

  optional &operator=(nullopt_t) noexcept
  {
    this->reset();
    return *this;
  }

  constexpr explicit operator bool() const noexcept
  {
    return has_value();
  }

  constexpr T &operator*() noexcept
  {
    return this->stg.value;
  }
  constexpr T const &operator*() const noexcept
  {
    return this->stg.value;
  }
  constexpr T *operator->() noexcept
  {
    return &operator*();
  }
  constexpr T const *operator->() const noexcept
  {
    return &operator*();
  }

  constexpr bool has_value() const noexcept
  {
    return this->has_value_;
  }

  using base_t::reset;
  using base_t::emplace;
};

template<typename T>
constexpr bool operator==(optional<T> const &a, optional<T> const &b) noexcept(noexcept(*a == *b))
{
  return a.has_value() == b.has_value() && (!a || *a == *b);
}

template<typename T>
constexpr bool operator!=(optional<T> const &a, optional<T> const &b) noexcept(noexcept(a == b))
{
  return !(a == b);
}

template<typename T>
constexpr bool operator<(optional<T> const &a, optional<T> const &b) noexcept(noexcept(*a < *b))
{
  return (a && *a < *b) || (!a && b);
}

template<typename T>
constexpr bool operator<=(optional<T> const &a, optional<T> const &b) noexcept(noexcept(a == b) && noexcept(a < b))
{
  return a == b || a < b;
}

template<typename T>
constexpr bool operator>(optional<T> const &a, optional<T> const &b) noexcept(noexcept(a <= b))
{
  return !(a <= b);
}

template<typename T>
constexpr bool operator>=(optional<T> const &a, optional<T> const &b) noexcept(noexcept(a < b))
{
  return !(a < b);
}
