//
// Created by igor on 21/10/2021.
//

#ifndef INCLUDE_NEUTRINO_KERNEL_GUI_GUI_HH
#define INCLUDE_NEUTRINO_KERNEL_GUI_GUI_HH

#include <neutrino/kernel/gui/window.hh>
#include <neutrino/utils/spimpl.h>

namespace neutrino::kernel::gui {
  class system {
    public:
      system();
      ~system();
    private:
      struct impl;
      spimpl::unique_impl_ptr<impl> m_pimpl;
  };
}

#endif //INCLUDE_NEUTRINO_KERNEL_GUI_GUI_HH
