#include <iostream>
#include <neutrino/utils/console.hh>
#include <neutrino/utils/io/binary_reader.hh>
#include <neutrino/assets/fs/phys_fs.hh>
#include "keen3/egahead.hh"

int main (int argc, char* argv[]) {

  neutrino::utils::console c;
  neutrino::assets::phys_fs fs("/home/igor/games/slordax/Slordax-The-Unknown-Enemy_DOS_EN/");
  auto e = fs.root()->load ("egahead.slo");
  if (!e) {
    std::cerr << "egahead.slo is missing" << std::endl;
  }

  if (std::get_if<1>(&e.value())) {
    std::shared_ptr<neutrino::assets::abstract_file> f = std::get<1>(e.value());
    auto ifs = f->open();
    if (ifs) {
      neutrino::appogee::keen3::egahead egahead (*ifs);
    } else {
      std::cerr << "Open failed" << std::endl;
    }
  } else {
    std::cerr << "file expected" << std::endl;
  }
  return 0;
}