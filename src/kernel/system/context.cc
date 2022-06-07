//
// Created by igor on 02/06/2022.
//

#include <neutrino/kernel/system/context.hh>

namespace neutrino::kernel {
  audio_system& context::audio() {
      return std::get<audio_system>(data);
  }

  video_system& context::video() {
    return std::get<video_system>(data);
  }

}