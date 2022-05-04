//
// Created by igor on 20/09/2020.
//


#include <neutrino/utils/macros.hh>
#include <neutrino/utils/exception.hh>
#include <neutrino/utils/io/memory_stream_buf.hh>

#include "bi_loader.hh"
#include "native_bi_data.inc"

#define RET_IO(A) std::make_unique<neutrino::utils::io::memory_input_stream>((const char*)A, PPCAT(A,_size));

std::unique_ptr<std::istream> bi_loader::load(resource_t rc)
{
    switch (rc)
    {
        case resource_t::BLOCKS  :
            return RET_IO(___game_BLOCKS);
        case resource_t::PALETTE :
            return RET_IO(___game_PALETTE);
        case resource_t::FONT    :
            return RET_IO(___game_FONT);
        case resource_t::LEVELS1 :
            return RET_IO(___game_LEVELS1);
        case resource_t::LEVELS2 :
            return RET_IO(___game_LEVELS2);
        case resource_t::LOGO    :
            return RET_IO(___game_BILD00_BRI);
        case resource_t::TITLE1  :
            return RET_IO(___game_BILD1RET_BRI);
        case resource_t::TITLE2  :
            return RET_IO(___game_BILD2RET_BRI);
        case resource_t::PIC1    :
            return RET_IO(___game_BILD01_BRI);
        case resource_t::PIC2    :
            return RET_IO(___game_BILD02_BRI);
        case resource_t::PIC3    :
            return RET_IO(___game_BILD1_BRI);
        case resource_t::PIC4    :
            return RET_IO(___game_BILD2_BRI);
        case resource_t::PIC5    :
            return RET_IO(___game_BILD3_BRI);
        case resource_t::PIC6    :
            return RET_IO(___game_BILD4_BRI);
        case resource_t::PIC7    :
            return RET_IO(___game_BILD5_BRI);
        case resource_t::PIC8    :
            return RET_IO(___game_BILD6_BRI);
        case resource_t::PIC9    :
            return RET_IO(___game_BILD7_BRI);
        case resource_t::PIC10   :
            return RET_IO(___game_BILD8_BRI);
    }
    RAISE_EX("Unknown resource ", (int)rc);
}