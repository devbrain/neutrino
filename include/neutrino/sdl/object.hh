//
// Created by igor on 01/06/2020.
//

#ifndef NEUTRINO_SDL_OBJECT_HH
#define NEUTRINO_SDL_OBJECT_HH

#include <neutrino/sdl/sdl2.hh>
#include <utility>
namespace neutrino::sdl
{
    namespace detail
    {
        template<typename T>
        struct deleter_traits;

#define d_SDL_DELETER_TRAITS(T, F)                      \
            template <>                                 \
            struct deleter_traits<T>                    \
            {                                           \
                static void call (T* ptr) noexcept      \
                {                                       \
                    if (ptr)                            \
                    {                                   \
                        F (ptr);                        \
                    }                                   \
                }                                       \
            }

        d_SDL_DELETER_TRAITS (SDL_Surface, SDL_FreeSurface);
        d_SDL_DELETER_TRAITS (SDL_PixelFormat, SDL_FreeFormat);
        d_SDL_DELETER_TRAITS (SDL_Window, SDL_DestroyWindow);
        d_SDL_DELETER_TRAITS (SDL_Texture, SDL_DestroyTexture);
        d_SDL_DELETER_TRAITS (SDL_Renderer, SDL_DestroyRenderer);
        d_SDL_DELETER_TRAITS (SDL_Palette, SDL_FreePalette);
        d_SDL_DELETER_TRAITS (SDL_RWops, SDL_FreeRW);
    } // ns detail

    template<class SDLOBJECT>
    class object
    {
    public:
        object();
        object(SDLOBJECT* obj, bool owner);
        object(const object&) = delete;
        object& operator=(const object&) = delete;

        object(object&& other) noexcept;
        object& operator=(object&& other) noexcept;

        virtual ~object() noexcept;

        [[nodiscard]] SDLOBJECT* handle() noexcept;
        [[nodiscard]] const SDLOBJECT* handle() const noexcept;
        [[nodiscard]] SDLOBJECT* const_handle() const noexcept;

        [[nodiscard]] SDLOBJECT* operator->() noexcept ;
        [[nodiscard]] const SDLOBJECT* operator->() const noexcept ;

        [[nodiscard]] bool is_null() const noexcept;
        [[nodiscard]] explicit operator bool() const noexcept ;


        void swap (object<SDLOBJECT> &s) // noexcept
        {
            std::swap(s.m_object, m_object);
            std::swap(s.m_owner, m_owner);
        }
    private:
        SDLOBJECT* m_object;
        bool m_owner;
    };
} // ns sdl
// ===================================================================================
// Implementation
// ===================================================================================
namespace neutrino::sdl
{
    template <class SDLOBJECT>
    inline
    object<SDLOBJECT>::object()
    : m_object(nullptr), m_owner(false)
    {

    }
    // -----------------------------------------------------------------------------------------------
    template <class SDLOBJECT>
    inline
    object<SDLOBJECT>::object(SDLOBJECT* obj, bool owner)
            : m_object(obj),
              m_owner(owner)
    {

    }
    // ----------------------------------------------------------------------------------------------
    template <class SDLOBJECT>
    inline
    object<SDLOBJECT>::object(object&& other) noexcept
            : m_object(other.m_object),
              m_owner(other.m_owner)
    {
        other.m_owner = false;
    }
    // ----------------------------------------------------------------------------------------------
    template <class SDLOBJECT>
    inline
    object<SDLOBJECT>& object<SDLOBJECT>::operator=(object && other) noexcept
    {
        if (this != &other)
        {
            if (m_owner)
            {
                detail::deleter_traits<SDLOBJECT>::call(m_object);
            }
            m_object = other.m_object;
            m_owner = other.m_owner;
            other.m_owner = false;
        }
        return *this;
    }
    // ----------------------------------------------------------------------------------------------
    template <class SDLOBJECT>
    inline
    object<SDLOBJECT>::~object() noexcept
    {
        if (m_owner)
        {
            detail::deleter_traits<SDLOBJECT>::call(m_object);
        }
    }
    // ----------------------------------------------------------------------------------------------
    template <class SDLOBJECT>
    inline
    SDLOBJECT* object<SDLOBJECT>::handle() noexcept
    {
        return m_object;
    }
    // ----------------------------------------------------------------------------------------------
    template <class SDLOBJECT>
    inline
    const SDLOBJECT* object<SDLOBJECT>::handle() const noexcept
    {
        return m_object;
    }
    // ----------------------------------------------------------------------------------------------
    template <class SDLOBJECT>
    inline
    SDLOBJECT* object<SDLOBJECT>::const_handle() const noexcept
    {
        return const_cast<SDLOBJECT*>(m_object);
    }
    // ----------------------------------------------------------------------------------------------
    template <class SDLOBJECT>
    inline
    SDLOBJECT* object<SDLOBJECT>::operator->() noexcept
    {
        return handle();
    }
    // ----------------------------------------------------------------------------------------------
    template <class SDLOBJECT>
    inline
    const SDLOBJECT* object<SDLOBJECT>::operator->() const noexcept
    {
        return handle();
    }
    // ----------------------------------------------------------------------------------------------
    template <class SDLOBJECT>
    inline
    bool object<SDLOBJECT>::is_null() const noexcept
    {
        return m_object == nullptr;
    }
    // ----------------------------------------------------------------------------------------------
    template <class SDLOBJECT>
    inline
    object<SDLOBJECT>::operator bool() const noexcept
    {
        return m_object != nullptr;
    }
}
#endif
