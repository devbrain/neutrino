//
// DIG audio-bank decoder (magic "DIG*") -- a musac codec for Krypton Egg's digitised SFX.
// See ~/proj/ke_dump/docs/dig.md for the format. This lives in KE (not in musac) because
// DIG is a KE-specific format; it plugs into musac by subclassing musac::decoder.
//

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <musac/sdk/decoder.hh>
#include <musac/sdk/types.hh>

namespace rs {

    // A DIG resource is a bank of one or more 8-bit *unsigned* mono PCM samples (0x80 =
    // silence) at a per-sample rate (11025 Hz in the retail data). A musac decoder plays a
    // single stream, so this one decodes one selected sample of the bank at a time: open()
    // parses the whole descriptor table, select_sample() picks the active sample (default 0).
    class dig_decoder : public musac::decoder {
        public:
            dig_decoder();
            ~dig_decoder() override;

            // True if the stream begins with the "DIG*" magic.
            [[nodiscard]] static bool accept(musac::io_stream* rwops);

            [[nodiscard]] const char* get_name() const override;
            void open(musac::io_stream* rwops) override;
            [[nodiscard]] musac::channels_t get_channels() const override;   // always 1
            [[nodiscard]] musac::sample_rate_t get_rate() const override;    // active sample's rate
            bool rewind() override;
            [[nodiscard]] std::chrono::microseconds duration() const override;
            bool seek_to_time(std::chrono::microseconds pos) override;

            // -- DIG bank API --
            [[nodiscard]] uint16_t sample_count() const;
            bool select_sample(uint16_t index);                    // choose active sample; rewinds
            [[nodiscard]] uint16_t selected_sample() const;
            [[nodiscard]] std::string sample_id(uint16_t index) const;     // name1 (short id) or ""
            [[nodiscard]] std::string sample_label(uint16_t index) const;  // name2 (label) or ""

            // A single-sample DIG bank (magic + 1 descriptor + PCM) holding just sample
            // @p index. Fed back through a dig_decoder to play one SFX in isolation
            // (overlapping via neutrino::load_sfx). Empty if index is out of range.
            [[nodiscard]] std::vector<uint8_t> extract_sample(uint16_t index) const;

        protected:
            size_t do_decode(float* buf, size_t len, bool& call_again) override;

        private:
            struct impl;
            std::unique_ptr<impl> m_pimpl;
    };

} // namespace rs
