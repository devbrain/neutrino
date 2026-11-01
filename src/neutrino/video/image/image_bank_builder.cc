//
// Created by igor on 29/10/2026.
//

#include <neutrino/video/image/image_bank_builder.hh>

#include <utility>

#include <failsafe/enforce.hh>

namespace neutrino {
    image_bank_builder::image_bank_builder(bool indexed, const palette* external, std::optional <palette> owned,
                                           std::optional <uint8_t> transparent_index)
        : m_indexed(indexed),
          m_external(external),
          m_owned(std::move(owned)),
          m_transparent_index(transparent_index) {
    }

    image_bank_builder image_bank_builder::indexed(const palette& pal, std::optional <uint8_t> transparent_index) {
        return image_bank_builder{true, &pal, std::nullopt, transparent_index};
    }

    image_bank_builder image_bank_builder::indexed(palette&& pal, std::optional <uint8_t> transparent_index) {
        return image_bank_builder{true, nullptr, std::move(pal), transparent_index};
    }

    image_bank_builder image_bank_builder::rgba() {
        return image_bank_builder{false, nullptr, std::nullopt, std::nullopt};
    }

    // Looked up on every use rather than cached: a pointer into m_owned would dangle after a move.
    const palette* image_bank_builder::bank_palette() const noexcept {
        return m_owned ? &*m_owned : m_external;
    }

    image_builder image_bank_builder::next(dim size) const {
        if (m_indexed) {
            return image_builder::indexed(size, *bank_palette(), m_transparent_index);
        }
        return image_builder::rgba(size);
    }

    std::size_t image_bank_builder::add(image_builder&& picture, uint32_t id) {
        image img = std::move(picture).build();
        const std::size_t position = m_bank.entries.size();

        if (m_indexed) {
            ENFORCE(img.format() == pixel_format::INDEX8,
                "Picture", position, "is not indexed; the bank is INDEX8");
            ENFORCE(img.get_palette().get() == bank_palette()->get(),
                "Picture", position, "does not use the bank's palette");
            const std::optional <uint32_t> key = m_transparent_index
                                                     ? std::optional <uint32_t>{*m_transparent_index}
                                                     : std::nullopt;
            ENFORCE(img.color_key() == key,
                "Picture", position, "does not use the bank's transparent index");
        } else {
            ENFORCE(img.format() == pixel_format::RGBA32,
                "Picture", position, "is not RGBA32; the bank is RGBA");
        }

        m_bank.entries.push_back({std::move(img), id});
        return position;
    }

    std::size_t image_bank_builder::size() const noexcept {
        return m_bank.entries.size();
    }

    image_bank image_bank_builder::build() && {
        return std::move(m_bank);
    }
}
