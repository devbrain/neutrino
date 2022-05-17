//
// Created by igor on 17/05/2022.
//

#ifndef INCLUDE_NEUTRINO_KERNEL_GFX_ANIMATION_STATE_HH
#define INCLUDE_NEUTRINO_KERNEL_GFX_ANIMATION_STATE_HH

#include <chrono>
#include <vector>
#include <tuple>
#include <cstdint>

#include <neutrino/kernel/rc/types.hh>
#include <neutrino/kernel/rc/animation_description.hh>

namespace neutrino::kernel {
  class animation_state {
    public:
      [[nodiscard]] animation_state_id_t add(animation_description::const_sequence sq);

      void update (std::chrono::milliseconds ms);
      [[nodiscard]] std::tuple<tile_handle, animation_description::kind_t> frame(animation_state_id_t index) const;
    private:
      struct animation_entry {
        explicit animation_entry(animation_description::const_sequence sq);

        animation_description::const_sequence sequence;
        std::size_t current_frame;
        std::chrono::milliseconds current_time;
      };
      std::vector<animation_entry> m_entries;
  };
}

#endif //INCLUDE_NEUTRINO_KERNEL_GFX_ANIMATION_STATE_HH
