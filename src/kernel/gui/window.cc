//
// Created by igor on 21/10/2021.
//

#include <neutrino/kernel/gui/window.hh>
#include <imgui.h>

namespace neutrino::kernel::gui {
  int window::_convert_flags (flags f) {
    int r = 0;
    if (f.contains (flags::NO_TITLE_BAR)) {
      r |= ImGuiWindowFlags_NoTitleBar;
    }
    if (f.contains (flags::NO_RESIZE)) {
      r |= ImGuiWindowFlags_NoResize;
    }
    if (f.contains (flags::NO_MOVE)) {
      r |= ImGuiWindowFlags_NoMove;
    }
    if (f.contains (flags::NO_SCROLLBAR)) {
      r |= ImGuiWindowFlags_NoScrollbar;
    }
    if (f.contains (flags::NO_SCROLL_WITH_MOUSE)) {
      r |= ImGuiWindowFlags_NoScrollWithMouse;
    }
    if (f.contains (flags::NO_COLLAPSE)) {
      r |= ImGuiWindowFlags_NoCollapse;
    }
    if (f.contains (flags::ALWAYS_AUTO_RESIZE)) {
      r |= ImGuiWindowFlags_AlwaysAutoResize;
    }
    if (f.contains (flags::NO_BACKGROUND)) {
      r |= ImGuiWindowFlags_NoBackground;
    }
    if (f.contains (flags::NO_SAVED_SETTINGS)) {
      r |= ImGuiWindowFlags_NoSavedSettings;
    }
    if (f.contains (flags::NO_MOUSE_INPUTS)) {
      r |= ImGuiWindowFlags_NoMouseInputs;
    }
    if (f.contains (flags::MENU_BAR)) {
      r |= ImGuiWindowFlags_MenuBar;
    }
    if (f.contains (flags::HORIZONTAL_SCROLLBAR)) {
      r |= ImGuiWindowFlags_HorizontalScrollbar;
    }
    if (f.contains (flags::NO_FOCUS_ON_APPEARING)) {
      r |= ImGuiWindowFlags_NoFocusOnAppearing;
    }
    if (f.contains (flags::NO_BRING_TO_FRONT_ON_FOCUS)) {
      r |= ImGuiWindowFlags_NoBringToFrontOnFocus;
    }
    if (f.contains (flags::ALWAYS_VERTICAL_SCROLLBAR)) {
      r |= ImGuiWindowFlags_AlwaysVerticalScrollbar;
    }
    if (f.contains (flags::ALWAYS_HORIZONTAL_SCROLLBAR)) {
      r |= ImGuiWindowFlags_AlwaysHorizontalScrollbar;
    }
    if (f.contains (flags::ALWAYS_USE_WINDOW_PADDING)) {
      r |= ImGuiWindowFlags_AlwaysUseWindowPadding;
    }
    if (f.contains (flags::NO_NAV_INPUTS)) {
      r |= ImGuiWindowFlags_NoNavInputs;
    }
    if (f.contains (flags::NO_NAV_FOCUS)) {
      r |= ImGuiWindowFlags_NoNavFocus;
    }
    if (f.contains (flags::UNSAVED_DOCUMENT)) {
      r |= ImGuiWindowFlags_UnsavedDocument;
    }
    if (f.contains (flags::NO_NAV)) {
      r |= ImGuiWindowFlags_NoNav;
    }
    if (f.contains (flags::NO_DECORATION)) {
      r |= ImGuiWindowFlags_NoDecoration;
    }
    if (f.contains (flags::NO_INPUTS)) {
      r |= ImGuiWindowFlags_NoInputs;
    }
    return r;
  }

  window::window(const char* name, bool* open_var, flags f)
  : m_status(ImGui::Begin (name, open_var, _convert_flags (f))) {
  }

  window::~window() {
    ImGui::End();
  }
}