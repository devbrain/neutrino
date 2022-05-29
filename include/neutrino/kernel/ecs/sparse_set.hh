//
// Created by igor on 12/07/2020.
//

#ifndef NEUTRINO_ECS_SPARSE_SET_HH
#define NEUTRINO_ECS_SPARSE_SET_HH

#include <memory_resource>
#include <vector>

namespace neutrino::ecs {
  template <typename Value>
  class sparse_set {
    public:
      class iterator {
          friend class sparse_set;

        public:
          using value_type = Value;
          using pointer = value_type*;
          using const_pointer = const value_type*;
          using reference = value_type&;
          using const_reference = const value_type&;
          using iterator_category = std::random_access_iterator_tag;
          using size_type = std::size_t;
          using difference_type = std::ptrdiff_t;
          reference operator * () noexcept;
          const_reference operator * () const noexcept;

          pointer operator -> () noexcept {
            return &(operator * ());
          }

          const_pointer operator -> () const noexcept {
            return &(operator * ());
          }

          reference operator [] (difference_type n) noexcept {
            return *(*this + n);
          }

          const_reference operator [] (difference_type n) const noexcept {
            return *(*this + n);
          }

          bool operator == (const iterator& other);

          bool operator != (const iterator& other) {
            return !(*this == other);
          }

          iterator& operator += (difference_type n) noexcept;
          iterator operator + (difference_type n) const noexcept;
          iterator operator - (difference_type n) const noexcept;
          difference_type operator - (iterator n) const noexcept;
          iterator& operator -= (difference_type n) noexcept;

          iterator& operator ++ (int);

          iterator& operator ++ ();

          iterator& operator -- (int);

          iterator& operator -- ();

          bool operator < (iterator b) const noexcept {
            auto n = (b - *this);
            return n < 0;
          }

          bool operator > (iterator b) const noexcept {
            return b < *this;
          }

          bool operator >= (iterator b) const noexcept {
            return !(*this < b);
          }

          bool operator <= (iterator b) const noexcept {
            return !(*this > b);
          }

          iterator& operator = (const iterator& other) = default;
          iterator (const iterator& other) = default;

          iterator ()
              : owner (nullptr), cursor (0) {
          }

        private:
          iterator (sparse_set<Value>& aowner, std::size_t acursor)
              : owner (aowner), cursor (acursor) {

          }

        private:
          sparse_set<Value>& owner;
          std::size_t cursor;
      };

      friend class iterator;

    public:
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
      explicit sparse_set (std::pmr::memory_resource& mr);
      sparse_set (std::pmr::memory_resource& mr, std::initializer_list<value_type> list);

      template <typename It>
      sparse_set (std::pmr::memory_resource& mr, It first, It last);

      sparse_set (const sparse_set&) = default;
      sparse_set& operator = (const sparse_set&) = default;
      sparse_set (sparse_set&&) noexcept = default;
      sparse_set& operator = (sparse_set&&) noexcept = default;
      ~sparse_set () = default;

      void swap (sparse_set<Value>& other) noexcept;

      [[nodiscard]] bool empty () const noexcept;
      [[nodiscard]] std::size_t size () noexcept;

      // Operations
      [[nodiscard]] bool exists (const value_type& v) const noexcept;

      // return false if v exists
      bool insert (const value_type& v);

      // return false if v exists
      bool erase (const value_type& v) noexcept;
      /*
      iterator begin () noexcept;
      iterator end () noexcept;

      const_iterator cbegin () noexcept;
      const_iterator cend () noexcept;
      reverse_iterator rbegin () noexcept;
      reverse_iterator rend () noexcept;
      const_reverse_iterator crbegin () noexcept;
      const_reverse_iterator crend () noexcept;
      */

      
      iterator begin() noexcept {
          return iterator(*this, 0);
      }

      // ----------------------------------------------------------------------------------------
      
      iterator end() noexcept {
          return iterator(*this, members);
      }

      // ----------------------------------------------------------------------------------------
      
      const_iterator cbegin() noexcept {
          return iterator(*this, 0);
      }

      // ----------------------------------------------------------------------------------------
      
      const_iterator cend() noexcept {
          return iterator(*this, members);
      }

      // ----------------------------------------------------------------------------------------
      
      reverse_iterator rbegin() noexcept {
          return reverse_iterator(begin());
      }

      // ----------------------------------------------------------------------------------------
      
      reverse_iterator rend() noexcept {
          return reverse_iterator(end());
      }

      // ----------------------------------------------------------------------------------------
      
      const_reverse_iterator crbegin() noexcept {
          return const_reverse_iterator(cbegin());
      }

      // ----------------------------------------------------------------------------------------
      
      const_reverse_iterator crend() noexcept {
          return const_reverse_iterator(cend());
      }

      void clear ();
    private:
      std::pmr::polymorphic_allocator<value_type> allocator;
      std::pmr::vector<value_type> sparse;
      std::pmr::vector<value_type> dense;
      std::size_t members;
  };
} // ns neutrino
// ============================================================================================
// Implementation
// ============================================================================================
namespace neutrino::ecs {
  template <typename Entity>
  sparse_set<Entity>::sparse_set (std::pmr::memory_resource& mr)
      : allocator (&mr),
        sparse (allocator),
        dense (allocator),
        members (0) {

  }

  // ----------------------------------------------------------------------------------------
  template <typename Entity>
  void sparse_set<Entity>::clear () {
    sparse.clear ();
    dense.clear ();
    members = 0;
  }

  // ----------------------------------------------------------------------------------------
  template <typename Entity>
  sparse_set<Entity>::sparse_set (std::pmr::memory_resource& mr, std::initializer_list<value_type> list)
      : allocator (&mr),
        sparse (allocator),
        dense (allocator),
        members (0) {
    for (value_type v: list) {
      insert (v);
    }
  }

  // ----------------------------------------------------------------------------------------
  template <typename Entity>
  template <typename It>
  sparse_set<Entity>::sparse_set (std::pmr::memory_resource& mr, It first, It last)
      : allocator (&mr),
        sparse (allocator),
        dense (allocator),
        members (0) {
    for (It i = first; i != last; i++) {
      auto v = *i;
      insert (v);
    }
  }

  // ----------------------------------------------------------------------------------------
  template <typename Entity>
  void sparse_set<Entity>::swap (sparse_set<Entity>& other) noexcept {
    std::swap (allocator, other.allocator);
    std::swap (sparse, other.sparse);
    std::swap (dense, other.dense);
    std::swap (members, other.members);
  }

  // ----------------------------------------------------------------------------------------
  template <typename Entity>
  bool sparse_set<Entity>::empty () const noexcept {
    return members == 0;
  }

  // ----------------------------------------------------------------------------------------
  template <typename Entity>
  std::size_t sparse_set<Entity>::size () noexcept {
    return members;
  }

  // ----------------------------------------------------------------------------------------
  template <typename Entity>
  bool sparse_set<Entity>::exists (const value_type& v) const noexcept {
    if (v >= sparse.size ()) {
      return false;
    }

    auto a = sparse[v];
    return a < members && dense[a] == v;
  }

  // ----------------------------------------------------------------------------------------
  template <typename Entity>
  bool sparse_set<Entity>::insert (const value_type& v) {
    if (static_cast<std::size_t>(v) >= sparse.size ()) {
      sparse.resize (v + 1);
    }
    if (members == dense.size ()) {
      dense.resize (members + 1);
    }
    auto a = sparse[v];
    if (a < members && dense[a] == v) {
      return false;
    }
    sparse[v] = members;
    dense[members] = v;
    members++;
    return true;
  }

  // ----------------------------------------------------------------------------------------
  template <typename Entity>
  bool sparse_set<Entity>::erase (const value_type& v) noexcept {
    if (v < sparse.size ()) {
      auto a = sparse[v];
      if (members == 0) {
        return false;
      }
      auto n = members - 1;
      if (a <= n && dense[a] == v) {
        auto e = dense[n];
        members = n;
        dense[a] = e;
        sparse[e] = a;
        return true;
      }
      return false;
    }
    return false;
  }

  // ----------------------------------------------------------------------------------------
  

  // ========================================================================================
  template <typename Entity>
  typename sparse_set<Entity>::iterator::reference sparse_set<Entity>::iterator::operator * () noexcept {
    return owner.dense[cursor];
  }

  // ----------------------------------------------------------------------------------------
  template <typename Entity>
  typename sparse_set<Entity>::iterator::const_reference sparse_set<Entity>::iterator::operator * () const noexcept {
    return owner.dense[cursor];
  }

  // ----------------------------------------------------------------------------------------
  template <typename Entity>
  bool sparse_set<Entity>::iterator::operator == (const iterator& other) {
    return (cursor == other.cursor && &owner == &other.owner);
  }

  // ----------------------------------------------------------------------------------------
  template <typename Entity>
  typename sparse_set<Entity>::iterator& sparse_set<Entity>::iterator::operator += (difference_type n) noexcept {
    cursor += n;
    return *this;
  }

  // ----------------------------------------------------------------------------------------
  template <typename Entity>
  typename sparse_set<Entity>::iterator sparse_set<Entity>::iterator::operator + (difference_type n) const noexcept {
    return iterator (owner, cursor + n);
  }

  // ----------------------------------------------------------------------------------------
  template <typename Entity>
  typename sparse_set<Entity>::iterator sparse_set<Entity>::iterator::operator - (difference_type n) const noexcept {
    return iterator (owner, cursor - n);
  }

  // ----------------------------------------------------------------------------------------
  template <typename Entity>
  typename sparse_set<Entity>::iterator::difference_type
  sparse_set<Entity>::iterator::operator - (iterator n) const noexcept {
    return cursor - n.cursor;
  }

  // ----------------------------------------------------------------------------------------
  template <typename Entity>
  typename sparse_set<Entity>::iterator& sparse_set<Entity>::iterator::operator -= (difference_type n) noexcept {
    return *this += (-n);
  }

  // ----------------------------------------------------------------------------------------
  template <typename Entity>
  typename sparse_set<Entity>::iterator& sparse_set<Entity>::iterator::operator ++ (int n) {
    auto temp = *this;
    if (n) {
      cursor += n;
    }
    else {
      cursor++;
    }
    return temp;
  }

  // ----------------------------------------------------------------------------------------
  template <typename Entity>
  typename sparse_set<Entity>::iterator& sparse_set<Entity>::iterator::operator ++ () {
    cursor++;
    return *this;
  }

  // ----------------------------------------------------------------------------------------
  template <typename Entity>
  typename sparse_set<Entity>::iterator& sparse_set<Entity>::iterator::operator -- (int n) {
    auto temp = *this;
    if (n) {
      cursor -= n;
    }
    else {
      cursor--;
    }
    return temp;
  }

  // ----------------------------------------------------------------------------------------
  template <typename Entity>
  typename sparse_set<Entity>::iterator& sparse_set<Entity>::iterator::operator -- () {
    cursor--;
    return *this;
  }
} // ns neutrino

#endif

