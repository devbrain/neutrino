//
// Created by igor on 30/06/2021.
//

#include <neutrino/hal/video/texture.hh>
#include <hal/cast.hh>

#include "texture_impl.hh"

namespace neutrino::hal
{
    texture::texture(std::unique_ptr<detail::texture_impl>&& t)
    : m_pimpl(std::move(t))
    {

    }

    texture::operator bool() const
    {
        return m_pimpl->texture.handle() != nullptr;
    }

    // returns: pixel format, texture_access, w, h
    std::tuple<pixel_format, texture::access, unsigned, unsigned> texture::query() const
    {
        auto[px, acc, w, h] = m_pimpl->texture.query();
        texture::access a = (acc == sdl::texture::access::STATIC) ? access::STATIC :
                            (acc == sdl::texture::access::STREAMING) ? access::STREAMING : access::TARGET;
        return {pixel_format{px.value()}, a, w, h};
    }

    uint8_t texture::alpha() const
    {
        return m_pimpl->texture.alpha();
    }

    void texture::alpha(uint8_t a)
    {
        m_pimpl->texture.alpha(a);
    }

    blend_mode texture::blend() const
    {
        return cast(m_pimpl->texture.blend());
    }

    void texture::blend(blend_mode bm)
    {
        m_pimpl->texture.blend(cast(bm));
    }

    std::optional<color> texture::color_mod() const
    {
        return cast<color>(m_pimpl->texture.color_mod());
    }

    void texture::color_mod(const color& c)
    {
        m_pimpl->texture.color_mod(cast(c));
    }

    /*
    Use this function to lock  whole texture for write-only pixel access.
    returns: pointer to pixels and pitch, ie., the length of one row in bytes
    */
    std::pair<void*, std::size_t> texture::lock() const
    {
        return m_pimpl->texture.lock();
    }
    /*
    Use this function to lock a portion of the texture for write-only pixel access.
    returns: pointer to pixels and pitch, ie., the length of one row in bytes
    */
    std::pair<void*, std::size_t> texture::lock(const math::rect& r) const
    {
        return m_pimpl->texture.lock(cast(r));
    }

    void texture::unlock() const
    {
        m_pimpl->texture.unlock();
    }

    // slow updates
    void texture::update(const void* pixels, std::size_t pitch)
    {
        m_pimpl->texture.update(pixels, pitch);
    }

    void texture::update(const math::rect& area, const void* pixels, std::size_t pitch)
    {
        m_pimpl->texture.update(cast(area), pixels, pitch);
    }
}