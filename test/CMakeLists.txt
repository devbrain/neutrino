my_add_executable(unittest
        main.cc

        utils/test_introspection.cc
        utils/test_type_list.cc
        utils/test_on_of.cc
        utils/test_constexpr_for.cc
        utils/test_type_name.cc
        utils/test_sorted_array.cc
        utils/string_constant_test.cc
        utils/string_tokenizer_test.cc
        utils/string_utils_test.cc
        utils/io/base_64_test.cc
        utils/io/binary_reader_writer.cc
        utils/io/memory_stream.cc
        utils/io/zlib_test.cc



        engine/test_observer_subject.cc
        #engine/test_message_broker.cc
        )

target_link_libraries(unittest
        PRIVATE

        neutrino::engine
        neutrino::utils

        thirdparty::doctest
        )