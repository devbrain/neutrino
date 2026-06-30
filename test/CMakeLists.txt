# =============================================================================
# Neutrino Test Suite
# =============================================================================

add_executable(neutrino_tests
    test_main.cc
    test_neutrino.cc
)

target_link_libraries(neutrino_tests
    PRIVATE
        neutrino
        doctest::doctest
)

add_test(NAME neutrino_tests COMMAND neutrino_tests)
