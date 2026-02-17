// ReSharper disable CppUseTypeTraitAlias
#ifndef HASHLIB_ALL_IN_ONE
#pragma once
#include "config.hpp"
#include <algorithm>
#ifdef __cpp_lib_endian
#include <bit>
#endif
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <string>
#include <type_traits>
#include <utility>
#endif


namespace hashlib {
    namespace detail {
        template<bool C>
        using bool_constant = std::integral_constant<bool, C>;

        template<bool C, typename T = void>
        using enable_if_t = typename std::enable_if<C, T>::type;

        template<bool C, typename T, typename U>
        using conditional_t = typename std::conditional<C, T, U>::type;

        template<typename...>
        using void_t = void;

        template<typename T>
        using remove_const_t = typename std::remove_const<T>::type;

        template<typename T>
        using remove_reference_t = typename std::remove_reference<T>::type;

        template<typename T>
        using remove_pointer_t = typename std::remove_pointer<T>::type;

        template<typename T>
        using remove_cv_t = typename std::remove_cv<T>::type;

        template<typename T>
        using remove_cvref_t = remove_cv_t<remove_reference_t<T>>;

        template<typename T>
        using add_pointer_t = typename std::add_pointer<T>::type;

        template <typename T>
        struct type_identity {
            using type = T;
        };

        template<typename T>
        using type_identity_t = typename type_identity<T>::type;

        template<typename...>
        struct conjunction : std::true_type {};

        template<typename B1>
        struct conjunction<B1> : B1 {};

        template<typename B1, typename... Bn>
        struct conjunction<B1, Bn...> : conditional_t<bool(B1::value), conjunction<Bn...>, B1> {};

        template<typename...>
        struct disjunction : std::false_type {};

        template<typename B1>
        struct disjunction<B1> : B1 {};

        template<typename B1, typename... Bn>
        struct disjunction<B1, Bn...> : conditional_t<bool(B1::value), B1, disjunction<Bn...>>  {};

        template<typename B>
        struct negation : bool_constant<!bool(B::value)> {};

        template<typename T, typename U>
        using is_derived_from = conjunction<std::is_base_of<U, T>, std::is_convertible<T*, U*>>;

        template<typename T>
        using iter_reference_t = decltype(*std::declval<T&>());

        template<typename T>
        using iter_value_t = remove_cvref_t<iter_reference_t<T>>;

#ifndef __cpp_lib_nonmember_container_access
        template<typename C>
        HASHLIB_CXX17_CONSTEXPR auto data(C& c) -> decltype(c.data()) {
            return c.data();
        }

        template<typename C>
        HASHLIB_CXX17_CONSTEXPR auto data(const C& c) -> decltype(c.data()) {
            return c.data();
        }

        template<typename T, std::size_t N>
        HASHLIB_CXX17_CONSTEXPR auto data(T(&array)[N]) noexcept -> T* {
            return array;
        }

        template<typename E>
        HASHLIB_CXX17_CONSTEXPR auto data(std::initializer_list<E> il) noexcept -> const E* {
            return il.begin();
        }
#else
        using std::data;
#endif

        template<typename T>
        using iter_cat_t = typename std::iterator_traits<T>::iterator_category;

        template<typename, typename, typename = void>
        struct is_iterator_impl : std::false_type {};

        template<typename T, typename Tag>
        struct is_iterator_impl<T, Tag, void_t<iter_cat_t<T>>> : is_derived_from<iter_cat_t<T>, Tag> {};

        template<typename, typename, typename = void>
        struct is_sentinel_for_impl : std::false_type {};

        template<typename T, typename It>
        struct is_sentinel_for_impl<T, It, void_t<decltype(std::declval<It&&>() != std::declval<T&&>())>> : std::true_type {};

        template<typename T>
        using range_iter_t = decltype(std::begin(std::declval<T&>()));

        template<typename T>
        using range_sent_t = decltype(std::end(std::declval<T&>()));

        template<typename T>
        using range_reference_t = iter_reference_t<range_iter_t<T>>;

        template<typename T>
        using range_value_t = iter_value_t<range_iter_t<T>>;

        template<typename, typename, typename = void>
        struct is_range_impl : std::false_type {};

        template<typename T, typename Tag>
        struct is_range_impl<T, Tag, enable_if_t<
            is_iterator_impl<range_iter_t<T>, Tag>::value &&
            is_sentinel_for_impl<range_sent_t<T>, range_iter_t<T>>::value
        >> : std::true_type {};

        template<typename, typename = void>
        struct is_contiguous_range_impl : std::false_type {};

        template<typename T>
        struct is_contiguous_range_impl<T, enable_if_t<
            is_range_impl<T, std::random_access_iterator_tag>::value &&
            std::is_same<
                decltype(data(std::declval<T&>())), // use ADL
                add_pointer_t<iter_reference_t<range_iter_t<T>>>
            >::value
        >> : std::true_type {};

        template<typename T>
        using is_output_iterator = is_iterator_impl<T, std::output_iterator_tag>;

        template<typename T>
        using is_input_iterator = is_iterator_impl<T, std::input_iterator_tag>;

        template<typename T>
        using is_forward_iterator = is_iterator_impl<T, std::forward_iterator_tag>;

        template<typename T>
        using is_bidirectional_iterator = is_iterator_impl<T, std::bidirectional_iterator_tag>;

        template<typename T>
        using is_random_access_iterator = is_iterator_impl<T, std::random_access_iterator_tag>;

        template<typename T, typename It>
        using is_sentinel_for = is_sentinel_for_impl<T, It>;

        template<typename T>
        using is_input_range = is_range_impl<T, std::input_iterator_tag>;

        template<typename T>
        using is_forward_range = is_range_impl<T, std::forward_iterator_tag>;

        template<typename T>
        using is_bidirectional_range = is_range_impl<T, std::bidirectional_iterator_tag>;

        template<typename T>
        using is_random_access_range = is_range_impl<T, std::random_access_iterator_tag>;

        template<typename T>
        using is_contiguous_range = is_contiguous_range_impl<T>;

#ifdef __cpp_lib_endian
        using std::endian;
#else
        enum class endian {
#if defined(_MSC_VER) && !defined(__clang__)
            little = 0,
            big    = 1,
            native = little
#else
            little = __ORDER_LITTLE_ENDIAN__,
            big    = __ORDER_BIG_ENDIAN__,
            native = __BYTE_ORDER__
#endif
        };
#endif

        static_assert(endian::native == endian::big || endian::native == endian::little, "unsupported mixed-endian.");

        constexpr auto is_little_endian() noexcept -> bool {
            return endian::native == endian::little;
        }
    }

    HASHLIB_MOD_EXPORT using byte = unsigned char;

    HASHLIB_MOD_EXPORT HASHLIB_CXX17_INLINE constexpr std::size_t dynamic_extent = static_cast<std::size_t>(-1);

    template<typename T, std::size_t N = dynamic_extent>
    class span;

    namespace detail {
        template<typename>
        struct is_span: std::false_type {};

        template<typename T, std::size_t N>
        struct is_span<span<T, N>> : std::true_type {};

        template<typename>
        struct is_std_array : std::false_type {};

        template<typename T, size_t N>
        struct is_std_array<std::array<T, N>> : std::true_type {};
    }

    HASHLIB_MOD_EXPORT template<typename T, std::size_t N>
    class span {
    public:
        using element_type = T;
        using value_type = detail::remove_cv_t<T>;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using pointer = T*;
        using const_pointer = const T*;
        using reference = T&;
        using const_reference = const T&;
        using iterator = T*;
        using const_iterator = const T*;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    public:
        span() = default;

        HASHLIB_CXX17_CONSTEXPR span(pointer data, size_type size) noexcept : data_(data), size_(size) {
            assert(data_ || size_ == 0);
        }

        template<size_type M, detail::enable_if_t<N == dynamic_extent || M == N>* = nullptr>
        HASHLIB_CXX17_CONSTEXPR span(detail::type_identity_t<T>(&array)[M]) noexcept : span(array, N) {}

        template<size_type M, detail::enable_if_t<N == dynamic_extent || M == N>* = nullptr>
        HASHLIB_CXX17_CONSTEXPR span(std::array<value_type, M>& array) noexcept : span(array.data(), N) {}

        template<size_type M, detail::enable_if_t<
            std::is_const<T>::value &&
            (N == dynamic_extent || M == N)
        >* = nullptr>
        HASHLIB_CXX17_CONSTEXPR span(const std::array<value_type, M>& array) noexcept : span(array.data(), N) {}

        template<typename U, detail::enable_if_t<
            std::is_const<T>::value &&
            std::is_same<U, detail::remove_const_t<T>>::value>
        * = nullptr>
        HASHLIB_CXX17_CONSTEXPR span(span<U> other) : span(other.data(), other.size()) {}

        template<typename Range, detail::enable_if_t<
            detail::is_contiguous_range<Range>::value &&
            std::is_same<detail::range_value_t<Range>, value_type>::value &&
            !detail::is_span<detail::remove_cvref_t<Range>>::value &&
            !detail::is_std_array<detail::remove_cvref_t<Range>>::value &&
            std::is_convertible<decltype(detail::data(std::declval<Range&>())), pointer>::value
        >* = nullptr>
        HASHLIB_CXX17_CONSTEXPR span(Range& rng): span(detail::data(rng), std::distance(std::begin(rng), std::end(rng))) {}

        HASHLIB_CXX17_CONSTEXPR span(const span& other) noexcept = default;

        span& operator= (const span& other) noexcept = default;

        ~span() = default;

        HASHLIB_CXX17_CONSTEXPR auto first(size_type count) const noexcept -> span {
            assert(count <= size_);
            return span(data_, count);
        }

        HASHLIB_CXX17_CONSTEXPR auto last(size_type count) const noexcept -> span {
            assert(count <= size_);
            return span(data_ + (size_ - count), count);
        }

        HASHLIB_CXX17_CONSTEXPR auto subspan(size_type pos, size_type count = dynamic_extent) const noexcept -> span {
            assert(pos <= size_);
            assert(count == dynamic_extent || count <= size_ - pos);
            return span(
                data_ + pos,
                count == dynamic_extent ? size_ - pos : count
            );
        }

        HASHLIB_CXX17_CONSTEXPR auto size() const noexcept -> size_type {
            return size_;
        }

        HASHLIB_CXX17_CONSTEXPR auto size_bytes() const noexcept -> size_type {
            return size() * sizeof(T);
        }

        HASHLIB_CXX17_CONSTEXPR auto empty() const noexcept -> bool {
            return size_ == 0;
        }

        HASHLIB_CXX17_CONSTEXPR auto operator[](size_type index) const noexcept -> T& {
            assert(index < size_);
            return data_[index];
        }

        HASHLIB_CXX17_CONSTEXPR auto front() const noexcept -> T& {
            assert(!empty());
            return *data();
        }

        HASHLIB_CXX17_CONSTEXPR auto back() const noexcept -> T& {
            assert(!empty());
            return *(data() + size() - 1);
        }

        HASHLIB_CXX17_CONSTEXPR auto data() const noexcept -> T* {
            return static_cast<T*>(data_);
        }

        HASHLIB_CXX17_CONSTEXPR auto begin() const noexcept -> iterator {
            return static_cast<T*>(data_);
        }

        HASHLIB_CXX17_CONSTEXPR auto end() const noexcept -> iterator {
            return begin() + size_;
        }

        HASHLIB_CXX17_CONSTEXPR auto cbegin() const noexcept -> const_iterator {
            return begin();
        }

        HASHLIB_CXX17_CONSTEXPR auto cend() const noexcept -> const_iterator {
            return end();
        }

        HASHLIB_CXX17_CONSTEXPR auto rbegin() const noexcept -> reverse_iterator {
            return reverse_iterator(end());
        }

        HASHLIB_CXX17_CONSTEXPR auto rend() const noexcept -> reverse_iterator {
            return reverse_iterator(begin());
        }

        HASHLIB_CXX17_CONSTEXPR auto crbegin() const noexcept -> const_reverse_iterator {
            return const_reverse_iterator(cend());
        }

        HASHLIB_CXX17_CONSTEXPR auto crend() const noexcept -> const_reverse_iterator {
            return const_reverse_iterator(cbegin());
        }

    private:
        pointer data_{};
        size_type size_{};
    };

    HASHLIB_MOD_EXPORT template<typename T>
    HASHLIB_NODISCARD auto as_bytes(span<T> s) noexcept -> span<const byte> {
        return {reinterpret_cast<const byte*>(s.data()), s.size_bytes()};
    }

    HASHLIB_MOD_EXPORT template<typename T, detail::enable_if_t<!std::is_const<T>::value>* = nullptr>
    HASHLIB_NODISCARD auto as_writable_bytes(span<T> s) noexcept -> span<byte> {
        return {reinterpret_cast<byte*>(s.data()), s.size_bytes()};
    }


    namespace detail {
        template<typename T>
        class auto_restorer {
            static_assert(std::is_same<T, remove_cvref_t<T>>::value, "`T` shall be a non-cv-qualified, non-reference type");
            static_assert(
                std::is_move_constructible<T>::value && std::is_move_assignable<T>::value &&
                std::is_copy_constructible<T>::value && std::is_copy_assignable<T>::value,
                "`T` shall be a copyable type"
            );
        public:
            HASHLIB_CXX20_CONSTEXPR explicit auto_restorer(T& target) noexcept(std::is_nothrow_copy_constructible<T>::value) : ref_(target), old_(target) {}

            auto_restorer(const auto_restorer&) = delete;

            HASHLIB_CXX20_CONSTEXPR ~auto_restorer() {
                ref_ = std::move(old_);
            }

            auto operator= (const auto_restorer&) -> auto_restorer& = delete;

        private:
            T& ref_;
            T old_;
        };

        template<typename T>
        using is_byte_like = disjunction<
            std::is_same<remove_const_t<T>, char>,
            std::is_same<remove_const_t<T>, unsigned char>,
            std::is_same<remove_const_t<T>, signed char>
#ifdef __cpp_lib_byte
            ,std::is_same<remove_const_t<T>, std::byte>
#endif
        >;

        HASHLIB_CXX17_INLINE constexpr char hex_table[] = "0123456789abcdef";
    }


    HASHLIB_MOD_EXPORT template<typename Base>
    class context : private Base {
    public:
        using Base::digest_size;
        using Base::update;

        context() = default;

        explicit context(span<const byte> bytes) : context() {
            this->update(bytes);
        }

        template<typename InputIt, typename Sentinel, detail::enable_if_t<
            detail::is_input_iterator<InputIt>::value &&
            detail::is_sentinel_for<Sentinel, InputIt>::value &&
            detail::is_byte_like<detail::iter_value_t<InputIt>>::value
        >* = nullptr>
        explicit context(InputIt first, Sentinel last) : context() {
            this->update(std::move(first), std::move(last));
        }

        template<typename Range, detail::enable_if_t<
            detail::is_input_range<Range>::value &&
            detail::is_byte_like<detail::range_value_t<Range>>::value
        >* = nullptr>
        explicit context(Range&& rng) : context(std::begin(rng), std::end(rng)) {}

        template<typename Range, detail::enable_if_t<
            detail::is_input_range<Range>::value &&
            detail::is_byte_like<detail::range_value_t<Range>>::value
        >* = nullptr>
        auto update(Range&& rng) -> void {
            this->update(std::begin(rng), std::end(rng));
        }

        HASHLIB_NODISCARD auto digest() noexcept -> std::array<byte, digest_size> {
            std::array<byte, digest_size> result;
            std::size_t i = 0;
            static_assert(digest_size <= sizeof(decltype(this->do_digest())), "what the f**k?");
            for (auto unit : this->do_digest()) {
                if (i == digest_size) break;
                for (auto byte_ : this->unit_to_bytes(unit)) {
                    result[i++] = byte_;
                }
            }
            assert(i == digest_size);
            return result;
        }

        HASHLIB_NODISCARD auto hexdigest() -> std::string {
            std::string result;
            result.resize(2 * digest_size);
            std::size_t i = 0;
            for (auto byte_ : digest()) {
                result[i++] = detail::hex_table[byte_ / 16];
                result[i++] = detail::hex_table[byte_ % 16];
            }
            return result;
        }

        HASHLIB_CXX17_CONSTEXPR auto clear() noexcept -> void {
            *this = context{};
        }

        template<typename Range, detail::enable_if_t<
            detail::is_input_range<Range>::value &&
            detail::is_byte_like<detail::range_value_t<Range>>::value
        >* = nullptr>
        auto operator<< (Range&& rng) -> context& {
            this->update(std::forward<Range>(rng));
            return *this;
        }
    };
}