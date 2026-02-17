#ifndef HASHLIB_ALL_IN_ONE
#pragma once
#include "core.hpp"
#endif

namespace hashlib {
    namespace detail {
        class md5 {
        public:
            static constexpr std::size_t digest_size = 16;

        public:
            auto update(span<const byte> bytes) noexcept -> void {
                update(bytes.begin(), bytes.end());
            }

            template<typename InputIt, typename Sentinel, enable_if_t<
                is_input_iterator<InputIt>::value &&
                !is_random_access_iterator<InputIt>::value &&
                is_sentinel_for<Sentinel, InputIt>::value &&
                is_byte_like<iter_value_t<InputIt>>::value
            >* = nullptr>
            auto update(InputIt first, Sentinel last) -> void {
                byte temp[128];
                for (auto it = first; it != last;) {
                    std::size_t n = 0;
                    for (; it != last && n < sizeof(temp); ++it, ++n) {
                        temp[n] = static_cast<byte>(*it);
                    }
                    update({temp, n});
                }
            }

            template<typename RandomAccessIt, typename Sentinel, enable_if_t<
                is_random_access_iterator<RandomAccessIt>::value &&
                is_sentinel_for<Sentinel, RandomAccessIt>::value &&
                is_byte_like<iter_value_t<RandomAccessIt>>::value
            >* = nullptr>
            auto update(RandomAccessIt first, Sentinel last) -> void {
                std::size_t bytes_count = last - first;
                total_size_ += bytes_count;
                std::size_t i = 0;

                if (buffer_size_ > 0) {
                    std::size_t to_copy = std::min(bytes_count, 64 - buffer_size_);
                    std::copy_n(first, to_copy, buffer_.data() + buffer_size_);
                    buffer_size_ += to_copy;
                    i += to_copy;
                    if (buffer_size_ == 64) {
                        process_(w_table_(buffer_.data()));
                        buffer_size_ = 0;
                    }
                }

                for (; i + 63 < bytes_count; i += 64) {
                    (process_)((w_table_)(std::next(first, i)));
                }

                if (i < bytes_count) {
                    std::copy_n(std::next(first, i), bytes_count - i, buffer_.data());
                    buffer_size_ = bytes_count - i;
                }
            }

        protected:
            auto do_digest() noexcept -> std::array<std::uint32_t, 4> {
                auto_restorer<md5> _{*this};
                auto buffer_size = buffer_size_;
                auto total_size = total_size_;
                byte padding[128]{};
                padding[0] = 0x80;
                std::size_t padding_size = (buffer_size < 56) ? (56 - buffer_size) : (120 - buffer_size);
                update({padding, padding_size});
                std::uint64_t bits = total_size * 8;
                for (std::size_t i = 0; i < 8; ++i) padding[i] = (bits >> (8 * i)) & 0xff;
                update({padding, 8});
                return {a_, b_, c_, d_};
            }

            HASHLIB_ALWAYS_INLINE
            static auto unit_to_bytes(std::uint32_t unit) noexcept -> std::array<byte, 4> {
                byte* byte_ptr = reinterpret_cast<byte*>(&unit);
                if HASHLIB_CXX17_CONSTEXPR (is_little_endian()) {
                    return {byte_ptr[0], byte_ptr[1], byte_ptr[2], byte_ptr[3]};
                }
                else {
                    return {byte_ptr[3], byte_ptr[2], byte_ptr[1], byte_ptr[0]};
                }
            }

        private:
            auto process_(const std::array<std::uint32_t, 16>& w) noexcept -> void {
                static constexpr std::uint32_t K[64]{
                    0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
                    0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
                    0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
                    0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
                    0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
                    0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
                    0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
                    0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
                    0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
                    0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
                    0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
                    0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
                    0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
                    0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
                    0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
                    0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391
                };
                static constexpr std::uint32_t S[16]{7, 12, 17, 22, 5, 9, 14, 20, 4, 11, 16, 23, 6, 10, 15, 21};

                std::uint32_t a = a_, b = b_, c = c_, d = d_;
                for (std::size_t i = 0; i < 64; ++i) {
                    std::uint32_t F, g, cnt;
                    if (i < 16) {
                        F = (b & c) | ((~b) & d);
                        g = i;
                        cnt = S[i % 4];
                    }
                    else if (i < 32) {
                        F = (d & b) | ((~d) & c);
                        g = (5 * i + 1) % 16;
                        cnt = S[4 + i % 4];
                    }
                    else if (i < 48) {
                        F = b ^ c ^ d;
                        g = (3 * i + 5) % 16;
                        cnt = S[8 + i % 4];
                    }
                    else {
                        F = c ^ (b | (~d));
                        g = (7 * i) % 16;
                        cnt = S[12 + i % 4];
                    }
                    F = F + a + K[i] + w[g];
                    a = d;
                    d = c;
                    c = b;
                    b = b + ((F << cnt) | (F >> (32 - cnt)));
                }
                a_ += a;
                b_ += b;
                c_ += c;
                d_ += d;
            }

            template<typename RandomAccessIt>
            static auto w_table_(RandomAccessIt it) noexcept -> std::array<std::uint32_t, 16> {
                static_assert(is_random_access_iterator<RandomAccessIt>::value, "unexpected");
                std::array<std::uint32_t, 16> w; // NOLINT(*-pro-type-member-init)
                for (std::size_t i = 0; i < 16; ++i) {
                    w[i] = (std::uint32_t(it[i * 4])) |
                           (std::uint32_t(it[i * 4 + 1]) << 8) |
                           (std::uint32_t(it[i * 4 + 2]) << 16) |
                           (std::uint32_t(it[i * 4 + 3]) << 24);
                }
                return w;
            }

        private:
            std::array<byte, 64> buffer_{};
            std::size_t buffer_size_ = 0;
            std::uint64_t total_size_ = 0;
            std::uint32_t a_ = 0x67452301, b_ = 0xefcdab89, c_ = 0x98badcfe, d_ = 0x10325476;
        };
    }

    HASHLIB_MOD_EXPORT using md5 = context<detail::md5>;
}
