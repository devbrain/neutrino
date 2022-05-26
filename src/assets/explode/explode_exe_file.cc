//
// Created by igor on 14/02/2021.
//

#include <stdexcept>

#include "neutrino/assets/explode/explode_exe_file.hh"

#include "assets/explode/mz/knowledge_dynamics.hh"
#include "assets/explode/mz/unlzexe.hh"
#include "assets/explode/mz/unpklite.hh"
#include "assets/explode/mz/unexepack.hh"

namespace
{
    template<typename DECODER>
    void decode(formats::explode::mz::input_exe_file& iexe, std::vector<char>& out_buff)
    {
        DECODER decoder(iexe);
        formats::explode::mz::full_exe_file fo(decoder.decomp_size());
        decoder.unpack(fo);
        formats::explode::mz::io::inmem_output ow(out_buff);
        fo.write(ow);
    }
} // anon. ns
// ========================================================================================================
namespace neutrino::assets
{
    bool explode_exe_file(const char* input, std::size_t input_size, std::vector<char>& output)
    {
      using namespace formats::explode::mz;
        try
        {
            io::inmem_input inp((unsigned char*) (input), input_size);

            input_exe_file iexe(inp);
            if (unlzexe::accept(iexe))
            {
                decode<unlzexe>(iexe, output);
            } else
            {
                if (unpklite::accept(iexe))
                {
                    decode<unpklite>(iexe, output);
                } else
                {
                    if (iexe.is_exepack())
                    {
                        decode<unexepack>(iexe, output);
                    } else
                    {
                        if (knowledge_dynamics::accept(iexe))
                        {
                            decode<knowledge_dynamics>(iexe, output);
                        } else
                        {
                            return false;
                        }
                    }
                }
            }
        }
        catch (std::runtime_error& )
        {
            return false;
        }
        return true;
    }
}
