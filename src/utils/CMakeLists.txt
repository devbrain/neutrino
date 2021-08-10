my_add_library_static(utils
		SOURCES

		${PROJECT_INCLUDE}/neutrino/utils/mp/callable/helpers.hpp
		${PROJECT_INCLUDE}/neutrino/utils/mp/callable/callable.hpp
		${PROJECT_INCLUDE}/neutrino/utils/mp/callable/function.hpp
		${PROJECT_INCLUDE}/neutrino/utils/mp/callable/member_function.hpp
		${PROJECT_INCLUDE}/neutrino/utils/mp/callable/functor.hpp

		${PROJECT_INCLUDE}/neutrino/utils/mp/type_name/type_name.msvc.hpp
		${PROJECT_INCLUDE}/neutrino/utils/mp/type_name/type_name.gcc.7_3.hpp
		${PROJECT_INCLUDE}/neutrino/utils/mp/type_name/type_name.hpp
		${PROJECT_INCLUDE}/neutrino/utils/mp/type_name/type_name.msvc.19_21.hpp
		${PROJECT_INCLUDE}/neutrino/utils/mp/type_name/type_name.gcc.8_1.hpp
		${PROJECT_INCLUDE}/neutrino/utils/mp/type_name/type_name.clang.5.hpp
		${PROJECT_INCLUDE}/neutrino/utils/mp/type_name/type_name.clang.hpp
		${PROJECT_INCLUDE}/neutrino/utils/mp/type_name/type_name.gcc.hpp

		${PROJECT_INCLUDE}/neutrino/utils/mp/typelist.hh
		${PROJECT_INCLUDE}/neutrino/utils/mp/if_then_else.hh
		${PROJECT_INCLUDE}/neutrino/utils/mp/all_same.hh
		${PROJECT_INCLUDE}/neutrino/utils/mp/one_of.hh
		${PROJECT_INCLUDE}/neutrino/utils/mp/constexpr_for.hh
		${PROJECT_INCLUDE}/neutrino/utils/mp/introspection.hh

		${PROJECT_INCLUDE}/neutrino/utils/pp_for_each.h
		${PROJECT_INCLUDE}/neutrino/utils/macros.hh
		${PROJECT_INCLUDE}/neutrino/utils/exception.hh
		exception.cc
		${PROJECT_INCLUDE}/neutrino/utils/logger.hh
		${PROJECT_INCLUDE}/neutrino/utils/register_at_exit.hh
		${PROJECT_INCLUDE}/neutrino/utils/singleton.hh
		${PROJECT_INCLUDE}/neutrino/utils/array_view.hh
		${PROJECT_INCLUDE}/neutrino/utils/sorted_array.hh

		io/base64.cc
		${PROJECT_INCLUDE}/neutrino/utils/io/base64.hh
		io/binary_reader.cc
		${PROJECT_INCLUDE}/neutrino/utils/io/binary_reader.hh
		io/binary_writer.cc
		${PROJECT_INCLUDE}/neutrino/utils/io/binary_writer.hh
		io/ios_init.hh
		io/memory_stream_buf.cc
		${PROJECT_INCLUDE}/neutrino/utils/io/memory_stream_buf.hh
		${PROJECT_INCLUDE}/neutrino/utils/io/buffered_stream_buf.hh
		${PROJECT_INCLUDE}/neutrino/utils/io/buffered_bidi_stream_buf.hh
		${PROJECT_INCLUDE}/neutrino/utils/io/unbuffered_stream_buf.hh
		${PROJECT_INCLUDE}/neutrino/utils/io/deflating_stream.hh
		io/deflating_stream.cc
		${PROJECT_INCLUDE}/neutrino/utils/io/inflating_stream.hh
		io/inflating_stream.cc
		${PROJECT_INCLUDE}/neutrino/utils/io/stream_copier.hh
		io/stream_copier.cc
        io/zstd_wrapper.c
		io/zstd_wrapper.h

		strings/ascii.cc
		${PROJECT_INCLUDE}/neutrino/utils/strings/ascii.hh

        strings/number_parser.cc
		${PROJECT_INCLUDE}/neutrino/utils/strings/number_parser.hh

        strings/number_formatter.cc
		${PROJECT_INCLUDE}/neutrino/utils/strings/number_formatter.hh

        strings/numeric_string.cc
		${PROJECT_INCLUDE}/neutrino/utils/strings/numeric_string.hh

		strings/string_tokenizer.cc
		${PROJECT_INCLUDE}/neutrino/utils/strings/string_tokenizer.hh

		${PROJECT_INCLUDE}/neutrino/utils/strings/string_constant.hh
		${PROJECT_INCLUDE}/neutrino/utils/strings/string_utils.hh

		fpe/fpe.cc
		${PROJECT_INCLUDE}/neutrino/utils/fpe/fpe.hh
		${PROJECT_INCLUDE}/neutrino/utils/fpe/fpe_c99.hh
		${PROJECT_INCLUDE}/neutrino/utils/fpe/fpe_windows.hh



		whereami.cc
		${PROJECT_INCLUDE}/neutrino/utils/whereami.hh

		thirdparty/whereami.c
		thirdparty/whereami.h

		${PROJECT_INCLUDE}/neutrino/utils/byte_order.hh
		${PROJECT_INCLUDE}/neutrino/utils/override.hh

		${PROJECT_INCLUDE}/neutrino/utils/spimpl.h
		${PROJECT_INCLUDE}/neutrino/utils/switch_by_string.hh

		${PROJECT_INCLUDE}/neutrino/utils/observer.hh
		observer.cc
		DEPENDS_PUBLIC
			thirdparty::zlib
			thirdparty::zstd
			thirdparty::strong_type
			thirdparty::double-conversion
)

target_compile_definitions(neutrino_utils PRIVATE
		NEUTRINO_PATH_PRIVATE="${PROJECT_INCLUDE_PRIVATE}"
        NEUTRINO_PATH_INCLUDE="${PROJECT_INCLUDE}")
