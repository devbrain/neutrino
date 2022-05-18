//
// Created by igor on 15/05/2022.
//

#ifndef INCLUDE_NEUTRINO_KERNEL_RC_WORLD_TILE_HANDLE_HH
#define INCLUDE_NEUTRINO_KERNEL_RC_WORLD_TILE_HANDLE_HH

#include <cstdint>
#include <limits>
#include <ostream>
#include <neutrino/kernel/rc/types.hh>
#include <neutrino/utils/exception.hh>

namespace neutrino::kernel {
  union tile_handle {
    uint16_t x[2];
    uint32_t id;
    // atlas layout
    // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    // |F|E|D|C|B|A|9|8|7|6|5|4|3|2|1|0|
    // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //  ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^
    //  | | | | Animation Counter     |
    //  | | | \ Atlas ID             /
    //  | | +-- Flip Vert
    //  | + --- Flip Horizontal
    //  + ----- Animation sequence

    constexpr static uint16_t ATLAS_MASK     = 0b0001111111111111;
    constexpr static uint16_t ANIMATION_MASK = 0b1000000000000000;
    constexpr static uint16_t HFLIP_MASK     = 0b0100000000000000;
    constexpr static uint16_t VFLIP_MASK     = 0b0010000000000000;
    constexpr static uint16_t FLIP_MASK      = HFLIP_MASK | VFLIP_MASK;

    tile_handle(atlas_id_t atlas_id, cell_id_t cell_id)
    : id(std::numeric_limits<uint32_t>::max())
    {
      if (!is_invalid (atlas_id) && !is_invalid (cell_id)) {
        x[0] = static_cast<uint16_t> (atlas_id.value_of ()) & ATLAS_MASK;
        ENFORCE(x[0] == atlas_id.value_of());
        x[1] = static_cast<uint16_t> (cell_id.value_of ());
        ENFORCE(x[1] == cell_id.value_of());
      }
    }

    tile_handle(atlas_id_t atlas_id, cell_id_t cell_id, bool flip_v, bool flip_h)
        : id(std::numeric_limits<uint32_t>::max())
    {
      if (!is_invalid (atlas_id) && !is_invalid (cell_id)) {
        x[0] = static_cast<uint16_t> (atlas_id.value_of ()) & ATLAS_MASK;
        ENFORCE(x[0] == atlas_id.value_of());
        if (flip_v) {
          x[0] = x[0] | VFLIP_MASK;
        }
        if (flip_h) {
          x[0] = x[0] | HFLIP_MASK;
        }
        x[1] = static_cast<uint16_t> (cell_id.value_of ());
        ENFORCE(x[1] == cell_id.value_of());
      }
    }

    explicit tile_handle(animation_seq_id_t anim_seq_id)
    : id(std::numeric_limits<uint32_t>::max()) {
      if (!is_invalid (anim_seq_id)) {
        x[0] = ANIMATION_MASK | (make_invalid<animation_state_id_t>().value_of() & ATLAS_MASK);
        x[1] = static_cast<uint16_t> (anim_seq_id.value_of ());
        ENFORCE(x[1] == anim_seq_id.value_of());
      }
    }

    tile_handle()
        : id(std::numeric_limits<uint32_t>::max()) {}

    [[nodiscard]] bool is_flipped() const {
      return (x[0] & FLIP_MASK) == FLIP_MASK;
    }

    [[nodiscard]] bool is_hflipped() const {
      return (x[0] & HFLIP_MASK) == HFLIP_MASK;
    }

    [[nodiscard]] bool is_vflipped() const {
      return (x[0] & VFLIP_MASK) == VFLIP_MASK;
    }

    [[nodiscard]] bool is_animation() const {
      return (x[0] & ANIMATION_MASK) == ANIMATION_MASK;
    }

    tile_handle& operator = (animation_state_id_t v) {
      ENFORCE(is_animation());
      x[0] = ANIMATION_MASK | (v.value_of() & ATLAS_MASK);
      return *this;
    }

    explicit operator animation_state_id_t  () const {
      ENFORCE(is_animation());
      return animation_state_id_t (x[0] & ATLAS_MASK);
    }

    explicit operator animation_seq_id_t () const {
      ENFORCE(is_animation());
      return animation_seq_id_t (x[1]);
    }

    explicit operator atlas_id_t () const {
      ENFORCE(!is_animation());
      return atlas_id_t (x[0] & ATLAS_MASK);
    }

    explicit operator cell_id_t () const {
      ENFORCE(!is_animation());
      return cell_id_t (x[1]);
    }

    [[nodiscard]] bool empty() const {
      return id == std::numeric_limits<uint32_t>::max();
    }

    explicit operator bool () const {
      return !empty();
    }
  };

  inline
  std::ostream& operator << (std::ostream& os, const tile_handle& th) {
    bool empty = th.empty();
    bool ani = th.is_animation();
    bool hf = th.is_hflipped();
    bool vf = th.is_vflipped();
    os << "E:" << empty <<",A:" << ani << ",H:" << hf << ",V:" << vf << ":" <<
                                                                            (th.x[0] & tile_handle::ATLAS_MASK)
                                                                            << ","
                                                                            <<th.x[1];
    return os;
  }
}

#endif //INCLUDE_NEUTRINO_KERNEL_RC_WORLD_TILE_HANDLE_HH
