//
// Created by igor on 03/07/2021.
//

#ifndef NEUTRINO_DEMOSCENE_HH
#define NEUTRINO_DEMOSCENE_HH

#include <neutrino/engine/main_window.hh>
#include <neutrino/demoscene/vga.hh>

namespace neutrino::demoscene {

  class main_scene;

  class scene : public engine::main_window {
      friend class main_scene;

    public:
      scene (int w, int h);
      ~scene () override;
    protected:
      virtual void init (vga& screen) = 0;
      virtual void effect (vga& screen) = 0;
    private:
      void after_window_opened () override;
    private:
      std::unique_ptr<vga> m_vga;
      main_scene* m_main_scene;
  };
}

#endif
