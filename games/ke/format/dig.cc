//
// See dig.hh and ~/proj/ke_dump/docs/dig.md.
//

#include <ke/format/dig.hh>
#include <bstream/span_reader.hh>
#include <bstream/span_writer.hh>

#include <musac/sdk/io_stream.hh>
#include <musac/error.hh>

#include <cstring>
#include <vector>

namespace rs {

    namespace {
        constexpr size_t DIG_HEADER_SIZE = 20; // magic(4) + count(2) + reserved(14)
        constexpr size_t DIG_ENTRY_SIZE  = 20;


    }

    struct dig_decoder::impl {
        struct sample_desc {
            uint32_t data_offset{};
            uint32_t data_size{}; // bytes == samples (8-bit mono)
            uint16_t rate{};
            uint16_t flags{};
            uint32_t name1_offset{};
            uint32_t name2_offset{};
        };

        std::vector<uint8_t> bank;          // the whole DIG resource
        std::vector<sample_desc> samples;   // parsed descriptor table
        uint16_t active{0};
        size_t pos{0};                      // decode cursor within the active sample's PCM

        [[nodiscard]] std::string name_at(uint32_t off) const {
            if (off == 0 || off >= bank.size()) {
                return {};
            }
            const char* p = reinterpret_cast<const char*>(bank.data() + off);
            return std::string(p, ::strnlen(p, bank.size() - off));
        }
    };

    dig_decoder::dig_decoder() : m_pimpl(std::make_unique<impl>()) {}
    dig_decoder::~dig_decoder() = default;

    bool dig_decoder::accept(musac::io_stream* rwops) {
        if (!rwops) {
            return false;
        }
        const int64_t here = rwops->tell();
        char magic[4] = {};
        const bool ok = rwops->read(magic, 4) == 4 && std::memcmp(magic, "DIG*", 4) == 0;
        rwops->seek(here, musac::seek_origin::set);
        return ok;
    }

    const char* dig_decoder::get_name() const {
        return "DIG";
    }

    void dig_decoder::open(musac::io_stream* rwops) {
        if (!rwops) {
            throw musac::decoder_error("DIG: null stream");
        }
        const int64_t size = rwops->get_size();
        if (size < static_cast<int64_t>(DIG_HEADER_SIZE)) {
            throw musac::decoder_error("DIG: stream smaller than the header");
        }
        rwops->seek(0, musac::seek_origin::set);
        m_pimpl->bank.resize(static_cast<size_t>(size));
        if (rwops->read(m_pimpl->bank.data(), m_pimpl->bank.size()) != m_pimpl->bank.size()) {
            throw musac::decoder_error("DIG: short read");
        }

        const std::vector<uint8_t>& bank = m_pimpl->bank;
        if (std::memcmp(bank.data(), "DIG*", 4) != 0) {
            throw musac::decoder_error("DIG: bad magic");
        }
        bstream::span_reader reader(bank, bstream::endian::little);
        reader.seek(4);
        const auto n = reader.read<uint16_t>();

        m_pimpl->samples.clear();
        m_pimpl->samples.reserve(n);
        for (size_t i = 1; i <= n; ++i) {
            // Descriptor i (1-based) sits at offset i * 20 (the header occupies slot 0).
            const size_t off = static_cast<size_t>(i) * DIG_ENTRY_SIZE;
            if (off + DIG_ENTRY_SIZE > bank.size()) {
                throw musac::decoder_error("DIG: descriptor table truncated");
            }
            reader.seek(off);
            impl::sample_desc d;
            reader >> d.data_offset >> d.data_size >> d.rate >> d.flags
                   >> d.name1_offset >> d.name2_offset;

            if (d.data_offset > bank.size() || d.data_size > bank.size() - d.data_offset) {
                throw musac::decoder_error("DIG: sample payload out of range");
            }
            m_pimpl->samples.push_back(d);
        }
        if (m_pimpl->samples.empty()) {
            throw musac::decoder_error("DIG: empty bank");
        }

        m_pimpl->active = 0;
        m_pimpl->pos = 0;
        set_is_open(true);
    }

    musac::channels_t dig_decoder::get_channels() const {
        return 1; // 8-bit mono
    }

    musac::sample_rate_t dig_decoder::get_rate() const {
        return m_pimpl->samples.empty() ? 11025u : m_pimpl->samples[m_pimpl->active].rate;
    }

    bool dig_decoder::rewind() {
        m_pimpl->pos = 0;
        return true;
    }

    std::chrono::microseconds dig_decoder::duration() const {
        if (m_pimpl->samples.empty()) {
            return std::chrono::microseconds(0);
        }
        const impl::sample_desc& d = m_pimpl->samples[m_pimpl->active];
        const musac::sample_rate_t rate = d.rate ? d.rate : 11025u;
        return std::chrono::microseconds(static_cast<int64_t>(d.data_size) * 1'000'000 / rate);
    }

    bool dig_decoder::seek_to_time(std::chrono::microseconds pos) {
        if (m_pimpl->samples.empty()) {
            return false;
        }
        const impl::sample_desc& d = m_pimpl->samples[m_pimpl->active];
        const musac::sample_rate_t rate = d.rate ? d.rate : 11025u;
        int64_t sample = pos.count() * static_cast<int64_t>(rate) / 1'000'000;
        if (sample < 0) {
            sample = 0;
        }
        if (sample > static_cast<int64_t>(d.data_size)) {
            sample = d.data_size;
        }
        m_pimpl->pos = static_cast<size_t>(sample);
        return true;
    }

    uint16_t dig_decoder::sample_count() const {
        return static_cast<uint16_t>(m_pimpl->samples.size());
    }

    bool dig_decoder::select_sample(uint16_t index) {
        if (index >= m_pimpl->samples.size()) {
            return false;
        }
        m_pimpl->active = index;
        m_pimpl->pos = 0;
        return true;
    }

    uint16_t dig_decoder::selected_sample() const {
        return m_pimpl->active;
    }

    std::string dig_decoder::sample_id(uint16_t index) const {
        return index < m_pimpl->samples.size() ? m_pimpl->name_at(m_pimpl->samples[index].name1_offset)
                                               : std::string{};
    }

    std::string dig_decoder::sample_label(uint16_t index) const {
        return index < m_pimpl->samples.size() ? m_pimpl->name_at(m_pimpl->samples[index].name2_offset)
                                               : std::string{};
    }

    std::vector<uint8_t> dig_decoder::extract_sample(uint16_t index) const {
        if (index >= m_pimpl->samples.size()) {
            return {};
        }
        const impl::sample_desc& d = m_pimpl->samples[index];
        std::vector<uint8_t> out(DIG_HEADER_SIZE + DIG_ENTRY_SIZE + d.data_size, 0);

        bstream::span_writer writer(out, bstream::endian::little);
        writer.write_exact("DIG*", 4);
        writer.write<uint16_t>(1); // one sample
        writer.seek(DIG_HEADER_SIZE);
        writer.write<uint32_t>(DIG_HEADER_SIZE + DIG_ENTRY_SIZE);
        writer.write(d.data_size);
        writer.write(d.rate);
        writer.write(d.flags);
        // name offsets stay 0
        std::memcpy(out.data() + DIG_HEADER_SIZE + DIG_ENTRY_SIZE,
                    m_pimpl->bank.data() + d.data_offset, d.data_size);
        return out;
    }

    size_t dig_decoder::do_decode(float* buf, size_t len, bool& call_again) {
        if (m_pimpl->active >= m_pimpl->samples.size()) {
            call_again = false;
            return 0;
        }
        const impl::sample_desc& d = m_pimpl->samples[m_pimpl->active];
        const uint8_t* pcm = m_pimpl->bank.data() + d.data_offset;
        const size_t total = d.data_size;
        const size_t remaining = total - m_pimpl->pos;
        const size_t todo = remaining < len ? remaining : len;

        // 8-bit unsigned PCM (0x80 = silence) -> float in [-1, 1).
        for (size_t i = 0; i < todo; ++i) {
            buf[i] = (static_cast<float>(pcm[m_pimpl->pos + i]) - 128.0f) / 128.0f;
        }
        m_pimpl->pos += todo;
        call_again = m_pimpl->pos < total;
        return todo;
    }

} // namespace rs
