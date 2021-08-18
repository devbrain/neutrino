set(src
        dictBuilder/fastcover.c
        dictBuilder/zdict.c
        dictBuilder/cover.c
        dictBuilder/divsufsort.c
        compress/zstd_compress_literals.c
        compress/huf_compress.c
        compress/zstd_double_fast.c
        compress/zstd_lazy.c
        compress/zstd_compress_superblock.c
        compress/fse_compress.c
        compress/zstdmt_compress.c
        compress/zstd_compress_sequences.c
        compress/hist.c
        compress/zstd_opt.c
        compress/zstd_ldm.c
        compress/zstd_fast.c
        compress/zstd_compress.c
        common/entropy_common.c
        common/pool.c
        common/debug.c
        common/xxhash.c
        common/fse_decompress.c
        common/error_private.c
        common/zstd_common.c
        common/threading.c
        decompress/zstd_decompress.c
        decompress/zstd_decompress_block.c
        decompress/huf_decompress.c
        decompress/zstd_ddict.c
        )

set(hdr
        dictBuilder/cover.h
        dictBuilder/divsufsort.h
        compress/zstd_compress_internal.h
        compress/zstd_compress_literals.h
        compress/zstd_fast.h
        compress/zstd_cwksp.h
        compress/zstdmt_compress.h
        compress/zstd_compress_superblock.h
        compress/zstd_compress_sequences.h
        compress/zstd_double_fast.h
        compress/zstd_ldm.h
        compress/zstd_ldm_geartab.h
        compress/zstd_lazy.h
        compress/zstd_opt.h
        compress/hist.h
        common/debug.h
        common/bitstream.h
        common/zstd_trace.h
        common/fse.h
        common/threading.h
        common/error_private.h
        common/cpu.h
        common/zstd_deps.h
        common/xxhash.h
        common/huf.h
        common/pool.h
        common/mem.h
        common/compiler.h
        common/zstd_internal.h
        decompress/zstd_ddict.h
        decompress/zstd_decompress_internal.h
        decompress/zstd_decompress_block.h
        zdict.h
        zstd.h
        zstd_errors.h
        )

add_library(neutrino_zstd STATIC ${src} ${hdr})
add_library(thirdparty::zstd ALIAS neutrino_zstd)
set_target_properties(neutrino_zstd PROPERTIES  FOLDER ThirdParty)
target_include_directories(neutrino_zstd PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(neutrino_zstd PRIVATE XXH_NAMESPACE=ZSTD_)