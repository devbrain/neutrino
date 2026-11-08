//
// Created by igor on 29/10/2026.
//

#include <neutrino/video/image/image_bank_builder.hh>

#include <utility>

#include <failsafe/enforce.hh>

namespace neutrino {
    template<image_kind K>
    image_bank_builder <K>::image_bank_builder(const palette& pal, std::optional <uint8_t> transparent_index)
        requires (K == image_kind::indexed)
        : m_external(&pal),
          m_transparent_index(transparent_index) {
    }

    template<image_kind K>
    image_bank_builder <K>::image_bank_builder(palette&& pal, std::optional <uint8_t> transparent_index)
        requires (K == image_kind::indexed)
        : m_owned(std::move(pal)),
          m_transparent_index(transparent_index) {
    }

    template<image_kind K>
    image_bank_builder <K>::image_bank_builder()
        requires (K == image_kind::rgba) = default;

    // Looked up on every use rather than cached: a pointer into m_owned would dangle after a move.
    template<image_kind K>
    const palette* image_bank_builder <K>::bank_palette() const noexcept {
        return m_owned ? &*m_owned : m_external;
    }

    template<image_kind K>
    image_builder <K> image_bank_builder <K>::next(dim size) const {
        if constexpr (K == image_kind::indexed) {
            return image_builder <K>{size, *bank_palette(), m_transparent_index};
        } else {
            return image_builder <K>{size};
        }
    }

    template<image_kind K>
    std::size_t image_bank_builder <K>::add(image_builder <K>&& picture, uint32_t id) {
        image img = std::move(picture).build();
        const std::size_t position = m_bank.entries.size();

        if constexpr (K == image_kind::indexed) {
            ENFORCE(img.get_palette().get() == bank_palette()->get(),
                "Picture", position, "does not use the bank's palette");
            const std::optional <uint32_t> key = m_transparent_index
                                                     ? std::optional <uint32_t>{*m_transparent_index}
                                                     : std::nullopt;
            ENFORCE(img.color_key() == key,
                "Picture", position, "does not use the bank's transparent index");
        }

        m_bank.entries.push_back({std::move(img), id});
        return position;
    }

    template<image_kind K>
    std::size_t image_bank_builder <K>::size() const noexcept {
        return m_bank.entries.size();
    }

    template<image_kind K>
    image_bank image_bank_builder <K>::build() && {
        return std::move(m_bank);
    }

    // Exported through the extern template declarations in the header, which carry NEUTRINO_EXPORT.
    template class image_bank_builder <image_kind::indexed>;
    template class image_bank_builder <image_kind::rgba>;
}
