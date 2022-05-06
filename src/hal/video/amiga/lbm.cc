//
// Created by igor on 06/05/2022.
//

#include "lbm.hh"
#include "amiga_image.hh"
#include "hal/video/amiga/iff/ea/ea_events.hh"
#include "hal/video/amiga/iff/ea/ea.hh"
#include "hal/video/amiga/iff/fourcc.hh"
#include "hal/video/amiga/iff/iff_parser.hh"
#include <neutrino/utils/exception.hh>

namespace neutrino::hal::detail
{

  namespace
  {
    constexpr auto ILBM = formats::iff::fourcc("ILBM");
    constexpr auto PBM  = formats::iff::fourcc("PBM ");
    constexpr auto ACBM = formats::iff::fourcc("ACBM");
    // ===========================================================================================================
    class lbm_parser : public formats::iff::ea_events
    {
      public:
        std::vector<formats::image::amiga::image> m_images;
      private:
        void on_form_start(formats::iff::chunk_type name) override
        {
          using namespace formats::image::amiga;
          image_type type = image_type::ILBM;
          if (name == PBM) {
            type = image_type::PBM;
          } else if (name == ACBM) {
            type = image_type::ACBM;
          }  else if (name == ILBM) {
            type = image_type::ILBM;
          } else {
            RAISE_EX("Unknown IFF image type ", name);
          }
          m_images.emplace_back(type);
        };
        void on_form_end([[maybe_unused]] formats::iff::chunk_type name) override {};
        void on_chunk(std::istream& is, formats::iff::chunk_type type, [[maybe_unused]] uint64_t offset, std::size_t size) override
        {
          auto s = type.to_string();
          m_images.back().update(type, is, size);
        }
      private:

    };
  }

  bool is_lbm(std::istream& is)
  {
    using namespace formats::iff;
    ea_tester tester({
                         ILBM,
                         PBM,
                         ACBM
                     });
    std::size_t pos = is.tellg();
    is.seekg (0, std::ios::end);
    auto input_length = (std::size_t)is.tellg() - pos;
    is.seekg (pos, std::ios::beg);
    iff_parser<ea_iff>(is, &tester, input_length);
    is.seekg (pos, std::ios::beg);
    bool rc = tester.valid();
    return rc;
  }
  // ------------------------------------------------------------------------------------------------------
  surface load_lbm (std::istream& is) {
    using namespace formats::iff;

    auto pos = is.tellg();
    is.seekg (0, std::ios::end);
    auto input_length = is.tellg() - pos;
    is.seekg (pos, std::ios::beg);

    lbm_parser parser;
    iff_parser<ea_iff>(is, &parser, input_length);
    if (parser.m_images.empty())
    {
      RAISE_EX("No image in LBM is found");
    }
    surface out;
    if (!parser.m_images.front().convert(out)) {
      RAISE_EX("LBM conversion failed");
    }
    return out;
  }

}