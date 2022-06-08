#include <fstream>
#include <iostream>
#include <cstdint>
#include <neutrino/utils/console.hh>
#include <neutrino/utils/io/binary_reader.hh>

#include <boost/pfr.hpp>


struct keen3_egahead {
    uint32_t    Latplansiz; /* Size of EGALATCH plane; should be one quarter of the size of the UNCOMPRESSED EGALATCH
                     size. This is the size of the four planes in the EGALATCH file */
uint32_t    Sprplansiz; /*  Size of EGASPRIT plane; should be one fifth of the size of the UNCOMPRESSED EGASPRIT
                     size. This is the size of the five planes in the EGASPRIT file */
uint32_t     Bmpdatstart; /* Where in the EGAHEAD the entries for unmasked graphics (Excluding font and tiles.)  
                     start. Should always be byte 48 */ 
uint32_t      Sprdatstart; /* Where in the EGAHEAD the entries for masked graphics (Sprites) start; by default
                     this is right after the unmasked graphics. */
uint16_t    Fontnum; /*     Number of 8x8 font entries are in the font; since font is written first in EGALATCH,
                     this x 8 in bytes defines the offset into each plane that the unmasked bitmaps start */
uint32_t    Fontloc; /*     Offset in plane where font data starts. Should be zero since font is first. */         
uint16_t    Unknum; //      Used for the ending screen until this was removed. Number of screen graphics.       
uint32_t    Unkloc; //      Used for screen graphics until removal. Offset in plane where screen data starts.   
uint16_t    Tilenum; //     Number of 16x16 tiles                                
uint32_t    Tileloc; /*     Offset in EGALATCH plane where tile data starts.     
                     = fontnum x 8 */                                        
uint16_t    Bmpnum; //      Number of unmasked bitmaps                           
uint32_t    Bmploc;//      Offset in plane where unmasked bitmap data starts.   
uint16_t    Spritenum; //   Number of sprite images                              
uint32_t    Spriteloc; /*   Offset in EGASPRIT plane of start of sprite data. Is,
                     of course, zero. */                                     
uint16_t    Compression; /* Add 2 to this byte if EGALATCH is compressed, add 1  
                     to it if EGAHEAD is compressed. Thus uncompressed    
                     graphics have this set at 0 and fully compressed at 3 */

    static keen3_egahead load(std::istream& is) {
        neutrino::utils::io::binary_reader br(is, neutrino::utils::io::binary_reader::stream_byte_order::LITTLE_ENDIAN_BYTE_ORDER);
        keen3_egahead out;
        boost::pfr::for_each_field(out, [&](auto& v) {
            br >> v;
        });
        return out;
    }                 

};

std::ostream& operator << (std::ostream& os, const keen3_egahead& v) {
    int k = 0;
    boost::pfr::for_each_field(v, [&](const auto& x) {
         os << "[" << k << "] " << x << std::endl;
         k++;   
    });
    return os;
}

int main(int argc, char* argv[]) {

    neutrino::utils::console c;
        

    std::ifstream ifs("c:\\games\\slordax\\egahead.slo", std::ios::binary);
    std::cout << keen3_egahead::load(ifs) << std::endl;
    return 0;
}