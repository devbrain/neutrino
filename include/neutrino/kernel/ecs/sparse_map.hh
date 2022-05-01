//
// Created by igor on 12/07/2020.
//

#ifndef NEUTRINO_ECS_sparse_map_HH
#define NEUTRINO_ECS_sparse_map_HH

#include <memory_resource>
#include <vector>
#include <neutrino/utils/exception.hh>

namespace neutrino::ecs
{
    template <typename Key, typename Value>
    class sparse_map
    {
    public:
        class iterator
        {
            friend class sparse_map;
        public:
            using key_type = Key;
            using value_type = std::pair<key_type, Value>;
            using pointer = value_type * ;
            using const_pointer = const value_type *;
            using reference = value_type & ;
            using const_reference = const value_type &;
            using iterator_category = std::random_access_iterator_tag;
            using size_type = std::size_t;
            using difference_type = std::ptrdiff_t;
            reference operator*() noexcept;
            const_reference operator*() const noexcept;

            pointer operator->() noexcept { return &(operator*()); }
            const_pointer operator->() const noexcept { return &(operator*()); }
            reference operator [] (difference_type n) noexcept { return *(*this + n); }
            const_reference operator [] (difference_type n) const noexcept { return *(*this + n); }
            bool operator==(const iterator& other);
            bool operator!=(const iterator& other) { return !(*this == other); }

            iterator& operator+=(difference_type n) noexcept;
            iterator operator+(difference_type n) const noexcept;
            iterator operator-(difference_type n) const noexcept;
            difference_type operator-(iterator n) const noexcept;
            iterator& operator-=(difference_type n) noexcept;

            iterator operator++(int);

            iterator& operator++();

            iterator& operator--(int);

            iterator& operator--();

            bool operator<(iterator b) const noexcept {
                auto n = (b - *this);
                return n < 0;
            }
            bool operator>(iterator b) const noexcept {
                return b < *this;
            }
            bool operator>=(iterator b) const noexcept {
                return !(*this < b);
            }
            bool operator<=(iterator b) const noexcept {
                return !(*this > b);
            }

            iterator& operator = (const iterator& other) = default;
            iterator (const iterator& other) = default;
            iterator()
                : owner(nullptr), cursor(0) {}

        private:
            iterator(sparse_map<Key, Value>& aowner, std::size_t acursor)
            : owner(&aowner), cursor(acursor)
            {

            }
        private:
            sparse_map<Key, Value>* owner;
            std::size_t cursor;
        };
        friend class iterator;
    public:
        using key_type = Key;
        using value_type = Value;
        using size_type = std::size_t;
        using pointer = value_type*;
        using const_pointer = const pointer;
        using reference = value_type&;
        using const_reference = const value_type&;
        using const_iterator = const iterator;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = const reverse_iterator;
    public:
        explicit sparse_map(std::pmr::memory_resource& mr);

        sparse_map(const sparse_map&) = default;
        sparse_map& operator = (const sparse_map&) = default;
        sparse_map(sparse_map&&)  noexcept = default;
        sparse_map& operator = (sparse_map&&)  noexcept = default;
        ~sparse_map() = default;

        void swap(sparse_map<Key, Value>& other) noexcept;

        [[nodiscard]] bool empty() const noexcept;
        [[nodiscard]] std::size_t size() noexcept;

        // Operations
        [[nodiscard]] bool exists (const key_type& k) const noexcept;

        Value& get(const key_type& k);
        const Value& get(const key_type& k) const;
        // return false if v exists

        bool insert (const key_type& k, const value_type& v);

        template<typename ... Args>
        bool emplace (const key_type& k, Args&& ... args);

        // return false if v exists
        bool erase(const key_type& k) noexcept;

        void clear();
        iterator begin() noexcept;
        iterator end() noexcept;

        const_iterator cbegin() noexcept;
        const_iterator cend() noexcept;
        reverse_iterator rbegin() noexcept;
        reverse_iterator rend() noexcept;
        const_reverse_iterator crbegin() noexcept;
        const_reverse_iterator crend() noexcept;

    private:
        std::pmr::polymorphic_allocator<value_type> allocator;
        std::pmr::vector<key_type> sparse;
        std::pmr::vector<std::pair<key_type, value_type>> dense;
        std::size_t members;
    };
} // ns neutrino
// ============================================================================================
// Implementation
// ============================================================================================
namespace neutrino::ecs
{
    template <typename Entity, typename Component>
    sparse_map<Entity,Component>::sparse_map(std::pmr::memory_resource& mr)
    : allocator(&mr),
    sparse(allocator),
    dense(allocator),
    members(0)
    {

    }
    // ----------------------------------------------------------------------------------------
    template <typename Entity, typename Component>
    void sparse_map<Entity,Component>::swap(sparse_map<Entity,Component>& other) noexcept
    {
        std::swap(allocator, other.allocator);
        std::swap(sparse, other.sparse);
        std::swap(dense, other.dense);
        std::swap(members, other.members);
    }
    // ----------------------------------------------------------------------------------------
    template <typename Entity, typename Component>
    bool sparse_map<Entity,Component>::empty() const noexcept
    {
        return members == 0;
    }
    // ----------------------------------------------------------------------------------------
    template <typename Entity, typename Component>
    std::size_t sparse_map<Entity,Component>::size() noexcept
    {
        return members;
    }
    // ----------------------------------------------------------------------------------------
    template <typename Entity, typename Component>
    bool sparse_map<Entity,Component>::exists (const key_type& k) const noexcept
    {
        if (k >= sparse.size())
        {
            return false;
        }

        auto a = sparse[k];
        return a < members && dense[a].first == k;
    }
    // ----------------------------------------------------------------------------------------
    template <typename Entity, typename Component>
    Component& sparse_map<Entity,Component>::get(const key_type& k)
    {
        if (k >= sparse.size())
        {
            RAISE_EX("Not found");
        }

        auto a = sparse[k];
        if ( a < members && dense[a].first == k)
        {
            return dense[a].second;
        }
        RAISE_EX("Not found");
    }
    // ----------------------------------------------------------------------------------------
    template <typename Entity, typename Component>
    const Component& sparse_map<Entity,Component>::get(const key_type& k) const
    {
        if (k >= sparse.size())
        {
            RAISE_EX("Not found");
        }

        auto a = sparse[k];
        if ( a < members && dense[a].first == k)
        {
            return dense[a].second;
        }
        RAISE_EX("Not found");
    }
    // ----------------------------------------------------------------------------------------
    template <typename Entity, typename Component>
    bool sparse_map<Entity,Component>::insert (const key_type& k, const value_type& val)
    {
        if (k >= sparse.size())
        {
            sparse.resize(k+1);
        }
        if (members == dense.size())
        {
            dense.resize(members+1);
        }
        auto a = sparse[k];
        if (a < members && dense[a].first == k)
        {
            return false;
        }
        sparse[k] = members;
        dense[members] = std::make_pair(k, val);
        members++;
        return true;
    }
    // ----------------------------------------------------------------------------------------
    template <typename Entity, typename Component>
    template<typename ... Args>
    bool sparse_map<Entity,Component>::emplace (const key_type& k, Args&& ... args)
    {
        if (k >= sparse.size())
        {
            sparse.resize(k+1);
        }
        if (members == dense.size())
        {
            dense.resize(members+1);
        }
        auto a = sparse[k];
        if (a < members && dense[a].first == k)
        {
            return false;
        }
        sparse[k] = members;
        dense[members].first = k;
        Component c {std::forward<Args>(args)...};
        std::swap(dense[members].second, c);
        members++;
        return true;
    }
    // ----------------------------------------------------------------------------------------
    template <typename Entity, typename Component>
    void sparse_map<Entity,Component>::clear()
    {
        sparse.resize(0);
        dense.resize(0);
    }
    // ----------------------------------------------------------------------------------------
    template <typename Entity, typename Component>
    bool sparse_map<Entity,Component>::erase(const key_type& k) noexcept
    {
        if (k < sparse.size())
        {
            if (members == 0)
            {
                return false;
            }
            auto a = sparse[k];
            auto n = members - 1;
            if (a <= n && dense[a].first == k)
            {
                auto e = dense[n];
                members = n;
                dense[a] = e;
                sparse[e.first] = a;
                return true;
            }
            return false;
        }
        return false;
    }
    // ----------------------------------------------------------------------------------------
    template <typename Entity, typename Component>
    typename sparse_map<Entity,Component>::iterator sparse_map<Entity,Component>::begin() noexcept
    {
        return iterator(*this, 0);
    }
    // ----------------------------------------------------------------------------------------
    template <typename Entity, typename Component>
    typename sparse_map<Entity,Component>::iterator sparse_map<Entity,Component>::end() noexcept
    {
        return iterator(*this, members);
    }
    // ----------------------------------------------------------------------------------------
    template <typename Entity, typename Component>
    typename sparse_map<Entity,Component>::const_iterator sparse_map<Entity,Component>::cbegin() noexcept
    {
        return iterator(*this, 0);
    }
    // ----------------------------------------------------------------------------------------
    template <typename Entity, typename Component>
    typename sparse_map<Entity,Component>::const_iterator sparse_map<Entity,Component>::cend() noexcept
    {
        return iterator(*this, members);
    }
    // ----------------------------------------------------------------------------------------
    template <typename Entity, typename Component>
    typename sparse_map<Entity,Component>::reverse_iterator sparse_map<Entity,Component>::rbegin() noexcept
    {
        return reverse_iterator (begin());
    }
    // ----------------------------------------------------------------------------------------
    template <typename Entity, typename Component>
    typename sparse_map<Entity,Component>::reverse_iterator sparse_map<Entity,Component>::rend() noexcept
    {
        return reverse_iterator (end());
    }
    // ----------------------------------------------------------------------------------------
    template <typename Entity, typename Component>
    typename sparse_map<Entity,Component>::const_reverse_iterator sparse_map<Entity,Component>::crbegin() noexcept
    {
        return const_reverse_iterator (cbegin());
    }
    // ----------------------------------------------------------------------------------------
    template <typename Entity, typename Component>
    typename sparse_map<Entity,Component>::const_reverse_iterator sparse_map<Entity,Component>::crend() noexcept
    {
        return const_reverse_iterator (cend());
    }
    // ========================================================================================
    template <typename Entity, typename Component>
    typename sparse_map<Entity,Component>::iterator::reference sparse_map<Entity,Component>::iterator::operator*() noexcept
    {
        return owner->dense[cursor];
    }
    // ----------------------------------------------------------------------------------------
    template <typename Entity, typename Component>
    typename sparse_map<Entity,Component>::iterator::const_reference sparse_map<Entity,Component>::iterator::operator*() const noexcept
    {
        return owner->dense[cursor];
    }
    // ----------------------------------------------------------------------------------------
    template <typename Entity, typename Component>
    bool sparse_map<Entity,Component>::iterator::operator==(const iterator& other)
    {
        if (!owner && !other.owner) {
          return true;
        }
        return (cursor == other.cursor && owner == other.owner);
    }
    // ----------------------------------------------------------------------------------------
    template <typename Entity, typename Component>
    typename sparse_map<Entity,Component>::iterator& sparse_map<Entity,Component>::iterator::operator+=(difference_type n) noexcept
    {
        cursor += n;
        return *this;
    }
    // ----------------------------------------------------------------------------------------
    template <typename Entity, typename Component>
    typename sparse_map<Entity,Component>::iterator sparse_map<Entity,Component>::iterator::operator+(difference_type n) const noexcept
    {
        return iterator(owner, cursor + n);
    }
    // ----------------------------------------------------------------------------------------
    template <typename Entity, typename Component>
    typename sparse_map<Entity,Component>::iterator sparse_map<Entity,Component>::iterator::operator-(difference_type n) const noexcept
    {
        return iterator(owner, cursor - n);
    }
    // ----------------------------------------------------------------------------------------
    template <typename Entity, typename Component>
    typename sparse_map<Entity,Component>::iterator::difference_type sparse_map<Entity,Component>::iterator::operator-(iterator n) const noexcept
    {
        return cursor - n.cursor;
    }
    // ----------------------------------------------------------------------------------------
    template <typename Entity, typename Component>
    typename sparse_map<Entity,Component>::iterator& sparse_map<Entity,Component>::iterator::operator-=(difference_type n) noexcept
    {
        return *this += (-n);
    }
    // ----------------------------------------------------------------------------------------
    template <typename Entity, typename Component>
    typename sparse_map<Entity,Component>::iterator sparse_map<Entity,Component>::iterator::operator++(int n)
    {
        auto temp = *this;
        if (n)
        {
            cursor += n;
        } else
        {
            cursor++;
        }
        return temp;
    }
    // ----------------------------------------------------------------------------------------
    template <typename Entity, typename Component>
    typename sparse_map<Entity,Component>::iterator& sparse_map<Entity,Component>::iterator::operator++()
    {
        cursor ++;
        return *this;
    }
    // ----------------------------------------------------------------------------------------
    template <typename Entity, typename Component>
    typename sparse_map<Entity,Component>::iterator& sparse_map<Entity,Component>::iterator::operator--(int n)
    {
        auto temp = *this;
        if (n)
        {
            cursor -= n;
        } else {
            cursor--;
        }
        return temp;
    }
    // ----------------------------------------------------------------------------------------
    template <typename Entity, typename Component>
    typename sparse_map<Entity,Component>::iterator& sparse_map<Entity,Component>::iterator::operator--()
    {
        cursor --;
        return *this;
    }
} // ns neutrino

#endif

