#include <iostream>
#include <neutrino/utils/console.hh>
#include <neutrino/assets/fs/phys_fs.hh>
#include "keen3/egahead.hh"

int main (int argc, char* argv[]) {

  neutrino::utils::console c;
  neutrino::assets::phys_fs fs ("/home/igor/games/slordax/Slordax-The-Unknown-Enemy_DOS_EN/");
  auto e = fs.root ()->load ("egahead.slo");
  if (!e) {
    std::cerr << "egahead.slo is missing" << std::endl;
  }

  auto& f = neutrino::assets::fs_cast<neutrino::assets::abstract_file> (e);
  auto ifs = f.open ();
  if (ifs) {
    neutrino::appogee::keen3::egahead egahead (*ifs);
  }
  else {
    std::cerr << "Open failed" << std::endl;
  }

  return 0;
}