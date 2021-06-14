//
// Created by igor on 03/06/2020.
//

#ifndef NEUTRINO_UTILS_ARRAY_VIEW_HH
#define NEUTRINO_UTILS_ARRAY_VIEW_HH
/* Implement a simple 1-dimensional array_view class
 *
 * (C) Copyright Marshall Clow 2016
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See http://www.boost.org/LICENSE_1_0.txt)
 *
 */

#include <iterator>
#include <algorithm>
#include <limits>
#include <cassert>
#include <vector>

namespace neutrino::utils
{
    template<class T>
    class array_view1d
    {
    public:
        // types
        typedef T value_type;
        typedef const value_type* pointer;
        typedef const value_type* const_pointer;
        typedef const value_type& reference;
        typedef const value_type& const_reference;
        typedef const_pointer const_iterator;
        typedef const_iterator iterator;
        typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
        typedef const_reverse_iterator reverse_iterator;
        typedef size_t size_type;
        typedef ptrdiff_t difference_type;

        constexpr array_view1d() noexcept
                : data_(nullptr), size_(0U)
        {}

        constexpr array_view1d(const value_type* p, size_type len) noexcept
                : data_(p), size_(len)
        {}

        constexpr array_view1d(const std::vector<T>& v) noexcept
                : data_(v.data()), size_(v.size())
        {}
        template<size_type N>
        constexpr array_view1d(const value_type(& arr)[N]) noexcept
                : data_(arr), size_(N)
        {}

        constexpr array_view1d(const array_view1d& rhs) noexcept
                : data_(rhs.data_), size_(rhs.size_)
        {}

        constexpr array_view1d& operator=(const array_view1d& rhs) noexcept
        {
            data_ = rhs.data_;
            size_ = rhs.size_;
            return *this;
        }

        constexpr const_iterator begin() const noexcept
        { return cbegin(); }
        constexpr const_iterator end() const noexcept
        { return cend(); }
        constexpr const_iterator cbegin() const noexcept
        { return data_; }
        constexpr const_iterator cend() const noexcept
        { return data_ + size_; }

        const_reverse_iterator rbegin() const noexcept
        { return const_reverse_iterator(cend()); }
        const_reverse_iterator rend() const noexcept
        { return const_reverse_iterator(cbegin()); }
        const_reverse_iterator crbegin() const noexcept
        { return const_reverse_iterator(cend()); }
        const_reverse_iterator crend() const noexcept
        { return const_reverse_iterator(cbegin()); }

        constexpr size_type size() const noexcept
        { return size_; }
        constexpr size_type length() const noexcept
        { return size_; }
        constexpr size_type max_size() const noexcept
        { return std::numeric_limits<size_type>::max(); }
        constexpr bool empty() const noexcept
        { return size_ == 0; }
        constexpr const_pointer data() const noexcept
        { return data_; }

        constexpr const_reference operator[](size_type __pos) const noexcept
        { return data_[__pos]; }

        constexpr const_reference at(size_type __pos) const
        {
            if (__pos >= size())
                throw std::out_of_range("array_view1d::at");
            return data_[__pos];
        }

        constexpr const_reference front() const
        {
            if (empty())
                throw std::out_of_range("array_view1d::front");
            return data_[0];
        }

        constexpr const_reference back() const
        {
            if (empty())
                throw std::out_of_range("array_view1d::back");
            return data_[size_ - 1];
        }

        constexpr void clear() noexcept
        {
            data_ = nullptr;
            size_ = 0;
        }

        constexpr void remove_prefix(size_type n)
        {
            assert(n <= size());
            data_ += n;
            size_ -= n;
        }

        constexpr void remove_suffix(size_type n)
        {
            assert(n <= size());
            size_ -= n;
        }

        constexpr void swap(array_view1d& other) noexcept
        {
            const value_type* p = data_;
            data_ = other.data_;
            other.data_ = p;

            size_type sz = size_;
            size_ = other.size_;
            other.size_ = sz;
        }

        constexpr bool operator==(array_view1d rhs)
        {
            if (size_ != rhs.size()) return false;
            for (size_type i = 0; i < size_; ++i)
                if (data_[i] != rhs[i])
                    return false;
            return true;
        }

        constexpr bool operator!=(array_view1d rhs)
        {
            if (size_ != rhs.size()) return true;
            for (size_type i = 0; i < size_; ++i)
                if (data_[i] != rhs[i])
                    return true;
            return false;
        }

    private:
        const_pointer data_;
        size_type size_;
    };
}
#endif
