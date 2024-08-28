//
// Created by igor on 7/15/24.
//
#include <iostream>
#include "cc_application.hh"
#include <cxxopts.hpp>





int main(int argc, char* argv[]) {
    cc_application app("/home/igor/proj/ares/games/CAVES/");
    app.init(320, 200);
    app.run();
    return 0;
}
