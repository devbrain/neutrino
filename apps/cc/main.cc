//
// Created by igor on 7/15/24.
//
#include <iostream>
#include "cc_application.hh"
#include "tmx_export.hh"
#include "props_editor/props_editor_app.hh"
#include <cxxopts.hpp>

enum run_mode_t {
    RUN_GAME,
    RUN_PROPS_EDITOR,
    RUN_TMX_EXPORT,
    RUN_EXIT
};

run_mode_t run_mode;

void parse_cmd_line(const int argc, char** argv) {
    cxxopts::Options options("CC", "Crystal Caves");
    options.add_options()
        ("p, pros-editor", "Run props editor", cxxopts::value <bool>()->default_value("false"))
        ("e, export", "Export to tmx", cxxopts::value <bool>()->default_value("false"))
        ("h, help", "Print help");
    const auto result = options.parse(argc, argv);

    if (result.count("help")) {
        std::cout << options.help() << std::endl;
        run_mode = RUN_EXIT;
        return;
    }
    if (result["p"].as <bool>()) {
        run_mode = RUN_PROPS_EDITOR;
    } else if (result["e"].as <bool>()) {
        run_mode = RUN_TMX_EXPORT;
    } else {
        run_mode = RUN_GAME;
    }
}

// --------------------------------------------------------------------------------
void run_game() {
    cc_application app("/home/igor/proj/ares/games/CAVES/");
    if (!app.is_configured()) {
        app.init(320, 200);
    }
    app.run();
}

// --------------------------------------------------------------------------------
void run_props_editor() {
    props_editor_app app("/home/igor/proj/ares/games/CAVES/");
    app.init(1024, 768);
    app.run();
}

// --------------------------------------------------------------------------------
int main(int argc, char* argv[]) {
    try {
        parse_cmd_line(argc, argv);
    } catch (cxxopts::exceptions::exception& e) {
        std::cerr << "Error parsing command line" << std::endl << e.what() << std::endl;
        return 1;
    }
    switch (run_mode) {
        case RUN_GAME:
            run_game();
            break;
        case RUN_PROPS_EDITOR:
            run_props_editor();
            break;
        case RUN_TMX_EXPORT:
            tmx_export();
            break;
        case RUN_EXIT:
            break;
    }
    return 0;
}
