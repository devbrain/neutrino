//
// Created by igor on 21/10/2021.
//

#include <neutrino/kernel/gui/gui.hh>
#include "imgui.h"

namespace neutrino::kernel::gui {
  struct system::impl {
    impl() {
      IMGUI_CHECKVERSION();
      m_ctx = ImGui::CreateContext();
      m_io = &ImGui::GetIO();
    }
    ~impl() {
      ImGui::DestroyContext (m_ctx);
    }
    ImGuiContext* m_ctx;
    ImGuiIO* m_io;
  };

  system::system() {
    m_pimpl = spimpl::make_unique_impl<impl>();
  }

  system::~system() = default;
}