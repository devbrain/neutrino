//
// Created by igor on 6/27/18.
//

#ifndef CORE_ITERATOR_HH
#define CORE_ITERATOR_HH

#include <cstddef>
#include <iterator>

#if defined(_MSC_VER)
#pragma warning ( push )
#pragma warning ( disable : 4996 )
#endif

namespace neutrino::sdl::detail
{
    //-------------------------------------------------------------------
    // Raw iterator with random access
    //-------------------------------------------------------------------
    template<typename T>
    class raw_iterator :
            public std::iterator<std::random_access_iterator_tag, T, ptrdiff_t, T *, T &>
    {
    public:
        explicit raw_iterator(T *ptr = nullptr);
        raw_iterator(const raw_iterator<T> &other) = default;

        ~raw_iterator() = default;

        raw_iterator<T> &operator=(const raw_iterator<T> &other) = default;

        raw_iterator<T> &operator=(T *ptr);


        operator bool() const;

        bool operator==(const raw_iterator<T> &other) const;
        bool operator!=(const raw_iterator<T> &other) const;
        raw_iterator<T> &operator+=(const ptrdiff_t &movement);
        raw_iterator<T> &operator-=(const ptrdiff_t &movement);
        raw_iterator<T> &operator++();
        raw_iterator<T> &operator--();
        raw_iterator<T> operator++(int);
        raw_iterator<T> operator--(int);
        raw_iterator<T> operator+(const ptrdiff_t &movement);
        raw_iterator<T> operator-(const ptrdiff_t &movement);
        ptrdiff_t operator-(const raw_iterator<T> &other);
        T &operator*();
        const T &operator*() const;
        T *operator->();

        T *getPtr() const;
        const T *getConstPtr() const;
        void setPtr (T* p);
    protected:
        T *m_ptr;
    };

    // =============================================================================================================
    template<typename T>
    class reverse_raw_iterator : public raw_iterator<T>
    {
    public:
        explicit reverse_raw_iterator(T *ptr = nullptr);
        explicit reverse_raw_iterator(const raw_iterator<T> &other);
        reverse_raw_iterator(const reverse_raw_iterator<T> &other) = default;

        ~reverse_raw_iterator() = default;
        reverse_raw_iterator<T> &operator=(const reverse_raw_iterator<T> &other) = default;

        reverse_raw_iterator<T> &operator=(const raw_iterator<T> &other);
        reverse_raw_iterator<T> &operator=(T *ptr);

        reverse_raw_iterator<T> &operator+=(const ptrdiff_t &movement);

        reverse_raw_iterator<T> &operator-=(const ptrdiff_t &movement);

        reverse_raw_iterator<T> &operator++();
        reverse_raw_iterator<T> &operator--();
        reverse_raw_iterator<T> operator++(int);
        reverse_raw_iterator<T> operator--(int);
        reverse_raw_iterator<T> operator+(const int &movement);
        reverse_raw_iterator<T> operator-(const int &movement);
        ptrdiff_t operator-(const reverse_raw_iterator<T> &other);
        raw_iterator<T> base();
    };
} // ns core
// ==========================================================================================
// Implementation
// ==========================================================================================
namespace neutrino::sdl::detail
{
    template<typename T>
    raw_iterator<T>::raw_iterator(T *ptr)
    {
        m_ptr = ptr;
    }
    // ---------------------------------------------------------------------------------------
    template<typename T>
    raw_iterator<T> &raw_iterator<T>::operator=(T *ptr)
    {
        m_ptr = ptr;
        return (*this);
    }
    // ---------------------------------------------------------------------------------------
    template<typename T>
    raw_iterator<T>::operator bool() const
    {
        if (m_ptr)
        {
            return true;
        } else
        {
            return false;
        }
    }
    // ---------------------------------------------------------------------------------------
    template<typename T>
    bool raw_iterator<T>::operator==(const raw_iterator<T> &other) const
    {
        return (m_ptr == other.getConstPtr());
    }
    // ---------------------------------------------------------------------------------------
    template<typename T>
    bool raw_iterator<T>::operator!=(const raw_iterator<T> &other) const
    {
        return (m_ptr != other.getConstPtr());
    }
    // ---------------------------------------------------------------------------------------
    template<typename T>
    raw_iterator<T> &raw_iterator<T>::operator+=(const ptrdiff_t &movement)
    {
        m_ptr += movement;
        return (*this);
    }
    // ---------------------------------------------------------------------------------------
    template<typename T>
    raw_iterator<T> &raw_iterator<T>::operator-=(const ptrdiff_t &movement)
    {
        m_ptr -= movement;
        return (*this);
    }
    // ---------------------------------------------------------------------------------------
    template<typename T>
    raw_iterator<T> &raw_iterator<T>::operator++()
    {
        ++m_ptr;
        return (*this);
    }
    // ---------------------------------------------------------------------------------------
    template<typename T>
    raw_iterator<T> &raw_iterator<T>::operator--()
    {
        --m_ptr;
        return (*this);
    }
    // ---------------------------------------------------------------------------------------
    template<typename T>
    raw_iterator<T> raw_iterator<T>::operator++(int)
    {
        auto temp(*this);
        ++m_ptr;
        return temp;
    }
    // ---------------------------------------------------------------------------------------
    template<typename T>
    raw_iterator<T> raw_iterator<T>::operator--(int)
    {
        auto temp(*this);
        --m_ptr;
        return temp;
    }
    // ---------------------------------------------------------------------------------------
    template<typename T>
    raw_iterator<T> raw_iterator<T>::operator+(const ptrdiff_t &movement)
    {
        auto oldPtr = m_ptr;
        m_ptr += movement;
        auto temp(*this);
        m_ptr = oldPtr;
        return temp;
    }
    // ---------------------------------------------------------------------------------------
    template<typename T>
    raw_iterator<T> raw_iterator<T>::operator-(const ptrdiff_t &movement)
    {
        auto oldPtr = m_ptr;
        m_ptr -= movement;
        auto temp(*this);
        m_ptr = oldPtr;
        return temp;
    }
    // ---------------------------------------------------------------------------------------
    template<typename T>
    ptrdiff_t raw_iterator<T>::operator-(const raw_iterator<T> &other)
    {
        return std::distance(other.getPtr(), this->getPtr());
    }
    // ---------------------------------------------------------------------------------------
    template<typename T>
    T &raw_iterator<T>::operator*()
    {
        return *m_ptr;
    }
    // ---------------------------------------------------------------------------------------
    template<typename T>
    const T &raw_iterator<T>::operator*() const
    {
        return *m_ptr;
    }
    // ---------------------------------------------------------------------------------------
    template<typename T>
    T *raw_iterator<T>::operator->()
    {
        return m_ptr;
    }
    // ---------------------------------------------------------------------------------------
    template<typename T>
    T *raw_iterator<T>::getPtr() const
    {
        return m_ptr;
    }
    // ---------------------------------------------------------------------------------------
    template<typename T>
    const T *raw_iterator<T>::getConstPtr() const
    {
        return m_ptr;
    }
    // ---------------------------------------------------------------------------------------
    template<typename T>
    void raw_iterator<T>::setPtr (T* p)
    {
        m_ptr = p;
    }
    // =======================================================================================
    template<typename T>
    reverse_raw_iterator<T>::reverse_raw_iterator(T *ptr)
            : raw_iterator<T>(ptr)
    {
    }
    // ---------------------------------------------------------------------------------------
    template<typename T>
    reverse_raw_iterator<T>::reverse_raw_iterator(const raw_iterator<T> &other)
    {
        this->m_ptr = other.getPtr();
    }
    // ---------------------------------------------------------------------------------------
    template<typename T>
    reverse_raw_iterator<T> &reverse_raw_iterator<T>::operator=(const raw_iterator<T> &other)
    {
        this->m_ptr = other.getPtr();
        return (*this);
    }
    // ---------------------------------------------------------------------------------------
    template<typename T>
    reverse_raw_iterator<T> &reverse_raw_iterator<T>::operator=(T *ptr)
    {
        this->setPtr(ptr);
        return (*this);
    }
    // ---------------------------------------------------------------------------------------
    template<typename T>
    reverse_raw_iterator<T> &reverse_raw_iterator<T>::operator+=(const ptrdiff_t &movement)
    {
        this->m_ptr -= movement;
        return (*this);
    }
    // ---------------------------------------------------------------------------------------
    template<typename T>
    reverse_raw_iterator<T> &reverse_raw_iterator<T>::operator-=(const ptrdiff_t &movement)
    {
        this->m_ptr += movement;
        return (*this);
    }
    // ---------------------------------------------------------------------------------------
    template<typename T>
    reverse_raw_iterator<T> &reverse_raw_iterator<T>::operator++()
    {
        --this->m_ptr;
        return (*this);
    }
    // ---------------------------------------------------------------------------------------
    template<typename T>
    reverse_raw_iterator<T> &reverse_raw_iterator<T>::operator--()
    {
        ++this->m_ptr;
        return (*this);
    }
    // ---------------------------------------------------------------------------------------
    template<typename T>
    reverse_raw_iterator<T> reverse_raw_iterator<T>::operator++(int)
    {
        auto temp(*this);
        --this->m_ptr;
        return temp;
    }
    // ---------------------------------------------------------------------------------------
    template<typename T>
    reverse_raw_iterator<T> reverse_raw_iterator<T>::operator--(int)
    {
        auto temp(*this);
        ++this->m_ptr;
        return temp;
    }
    // ---------------------------------------------------------------------------------------
    template<typename T>
    reverse_raw_iterator<T> reverse_raw_iterator<T>::operator+(const int &movement)
    {
        auto oldPtr = this->m_ptr;
        this->m_ptr -= movement;
        auto temp(*this);
        this->m_ptr = oldPtr;
        return temp;
    }
    // ---------------------------------------------------------------------------------------
    template<typename T>
    reverse_raw_iterator<T> reverse_raw_iterator<T>::operator-(const int &movement)
    {
        auto oldPtr = this->m_ptr;
        this->m_ptr += movement;
        auto temp(*this);
        this->m_ptr = oldPtr;
        return temp;
    }
    // ---------------------------------------------------------------------------------------
    template<typename T>
    ptrdiff_t reverse_raw_iterator<T>::operator-(const reverse_raw_iterator<T> &other)
    {
        return std::distance(this->getPtr(), other.getPtr());
    }
    // ---------------------------------------------------------------------------------------
    template<typename T>
    raw_iterator<T> reverse_raw_iterator<T>::base()
    {
        raw_iterator<T> forwardIterator(this->m_ptr);
        ++forwardIterator;
        return forwardIterator;
    }
} // ns core

#if defined(_MSC_VER)
#pragma warning ( pop )
#endif

#endif
