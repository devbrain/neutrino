//
// Created by igor on 06/06/2021.
//
#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>
#include <neutrino/utils/console.hh>

int main(int argc, char** argv) {
    doctest::Context context;
    neutrino::utils::console console;
    
    context.applyCommandLine(argc, argv);

    
    int res = context.run(); // run

    if(context.shouldExit()) // important - query flags (and --exit) rely on the user doing this
        return res;          // propagate the result of the tests
    
       
    return res; // the result from doctest is propagated here as well
}