//
// Created by igor on 17/05/2022.
//

#include "animation_state.hh"
#include <neutrino/utils/exception.hh>

namespace neutrino::kernel {
  animation_state::animation_entry::animation_entry(assets::animation_description::const_sequence sq)
  : sequence(std::move(sq)), current_frame(0), current_time(0) {
  }

  assets::animation_state_id_t animation_state::add(assets::animation_description::const_sequence sq) {
    m_entries.emplace_back (std::move(sq));
    return assets::animation_state_id_t (m_entries.size()-1);
  }

  void animation_state::clear () {
    m_entries.clear();
  }

  void animation_state::update (std::chrono::milliseconds ms) {
    for (auto& e : m_entries) {
      auto time_in_frame = e.current_time + ms;
      bool flipped = false;
      while (e.current_frame < e.sequence.size()) {
        auto delay = std::get<1>(e.sequence[e.current_frame]);
        if (time_in_frame >= delay) {
          e.current_frame++;
          if (e.current_frame == e.sequence.size()) {
            if (e.sequence.kind() == assets::animation_description::CIRCULAR) {
              e.current_frame = 0;
            }
          }
          flipped = true;
          time_in_frame -= delay;
        } else {
          break;
        }
      }
      if (!flipped) {
        e.current_time = time_in_frame;
      } else {
        e.current_time = std::chrono::milliseconds (0);
      }
    }
  }

  std::tuple<assets::tile_handle, assets::animation_description::kind_t> animation_state::frame(assets::animation_state_id_t index) const {
    ENFORCE(index.value_of() < m_entries.size());
    const auto& e = m_entries[index.value_of()];
    auto kind = e.sequence.kind();
    if (e.current_frame >= e.sequence.size()) {
      if (kind == assets::animation_description::LINEAR) {
        return {std::get<0>(e.sequence[e.sequence.size() - 1]), kind};
      }
      return {assets::tile_handle(),kind};
    }
    return {std::get<0>(e.sequence[e.current_frame]), kind};
  }
}