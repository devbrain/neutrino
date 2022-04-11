//
// Created by igor on 21/10/2021.
//

#ifndef INCLUDE_NEUTRINO_KERNEL_GUI_WINDOW_HH
#define INCLUDE_NEUTRINO_KERNEL_GUI_WINDOW_HH

#include <bitflags/bitflags.hpp>

namespace neutrino::kernel::gui {
  class window {
    public:
      BEGIN_RAW_BITFLAGS(flags)
        RAW_FLAG(NO_TITLE_BAR) // Disable title-bar
        RAW_FLAG(NO_RESIZE) // Disable user resizing with the lower-right grip
        RAW_FLAG(NO_MOVE) // Disable user moving the window
        RAW_FLAG(NO_SCROLLBAR) // Disable scrollbars (window can still scroll with mouse or programmatically)
        RAW_FLAG(NO_SCROLL_WITH_MOUSE) // Disable user vertically scrolling with mouse wheel. On child window, mouse wheel will be forwarded to the parent unless NoScrollbar is also set.
        RAW_FLAG(NO_COLLAPSE) // Disable user collapsing window by double-clicking on it
        RAW_FLAG(ALWAYS_AUTO_RESIZE) // Resize every window to its content every frame
        RAW_FLAG(NO_BACKGROUND) // Disable drawing background color (WindowBg, etc.) and outside border. Similar as using SetNextWindowBgAlpha(0.0f).
        RAW_FLAG(NO_SAVED_SETTINGS) // Never load/save settings in .ini file
        RAW_FLAG(NO_MOUSE_INPUTS) // Disable catching mouse, hovering test with pass through.
        RAW_FLAG(MENU_BAR) // Has a menu-bar
        RAW_FLAG(HORIZONTAL_SCROLLBAR) // Allow horizontal scrollbar to appear (off by default). You may use SetNextWindowContentSize(ImVec2(width,0.0f)); prior to calling Begin() to specify width. Read code in imgui_demo in the "Horizontal Scrolling" section.
        RAW_FLAG(NO_FOCUS_ON_APPEARING) // Disable taking focus when transitioning from hidden to visible state
        RAW_FLAG(NO_BRING_TO_FRONT_ON_FOCUS) // Disable bringing window to front when taking focus (e.g. clicking on it or programmatically giving it focus)
        RAW_FLAG(ALWAYS_VERTICAL_SCROLLBAR) // Always show vertical scrollbar (even if ContentSize.y < Size.y)
        RAW_FLAG(ALWAYS_HORIZONTAL_SCROLLBAR) // Always show horizontal scrollbar (even if ContentSize.x < Size.x)
        RAW_FLAG(ALWAYS_USE_WINDOW_PADDING) // Ensure child windows without border uses style.WindowPadding (ignored by default for non-bordered child windows, because more convenient)
        RAW_FLAG(NO_NAV_INPUTS) // No gamepad/keyboard navigation within the window
        RAW_FLAG(NO_NAV_FOCUS) // No focusing toward this window with gamepad/keyboard navigation (e.g. skipped by CTRL+TAB)
        RAW_FLAG(UNSAVED_DOCUMENT) // Display a dot next to the title. When used in a tab/docking context, tab is selected when clicking the X + closure is not assumed (will wait for user to stop submitting the tab). Otherwise closure is assumed when pressing the X, so if you keep submitting the tab may reappear at end of tab bar.
        RAW_FLAG(NO_NAV)
        RAW_FLAG(NO_DECORATION)
        RAW_FLAG(NO_INPUTS)
      END_RAW_BITFLAGS(flags)
    public:
      /**
       * Opens new window
       * @param name id of the window
       * @param open_var displays a Close button on the upper-right corner of the window, the pointed value will be set to false when the button is pressed
       * @param f flags
       */
      explicit window(const char* name, bool* open_var= nullptr, flags f = flags::empty());
      ~window();
    private:
      bool m_status;
    private:
      static int _convert_flags(flags f);
  };


}

#endif //INCLUDE_NEUTRINO_KERNEL_GUI_WINDOW_HH
