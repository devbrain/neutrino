#include <iostream>
#include <fstream>
#include <neutrino/utils/console.hh>
#include <neutrino/utils/io/binary_reader.hh>

class dax_header_entry {
  public:
    static dax_header_entry load(std::istream& istream);

    void debug() const;
  private:
    dax_header_entry() = default;
  private:
    uint32_t m_id;
    std::size_t m_offset;
    std::size_t m_raw_size;
    std::size_t m_compressed_size;
};

dax_header_entry dax_header_entry::load(std::istream& is) {
  neutrino::utils::io::binary_reader reader(is, neutrino::utils::io::binary_reader::LITTLE_ENDIAN_BYTE_ORDER);
  uint8_t id;
  uint32_t offs;
  uint16_t rs,cs;
  reader >> id >> offs >> rs >> cs;
  dax_header_entry ret;
  ret.m_id = id;
  ret.m_offset = offs;
  ret.m_compressed_size = cs;
  ret.m_raw_size = rs;
  return ret;
}

void dax_header_entry::debug() const {
  std::cout << "Header: id = " << m_id << ", offset = " << m_offset
  << ", raw size = " << m_raw_size << ", compressed size = " << m_compressed_size << std::endl;
}

// ===========================================================
// DAX FILE
// ===========================================================
class dax_file {
  public:
    static dax_file load(std::istream& is);
  private:
    dax_file();
};

dax_file::dax_file () {

}

dax_file dax_file::load(std::istream& is) {
  neutrino::utils::io::binary_reader reader(is, neutrino::utils::io::binary_reader::LITTLE_ENDIAN_BYTE_ORDER);
  uint16_t dataOffset;
  reader >> dataOffset;
  dataOffset += 2;
  static constexpr auto HEADER_ENTRY_SIZE = 9;
  auto num_headers = (dataOffset - 2)/ HEADER_ENTRY_SIZE;
  std::cout << "Found " << num_headers << " headers" << std::endl;
  for (int i=0; i<num_headers; i++) {
    dax_header_entry dh = dax_header_entry::load (is);
    dh.debug();
  }
  return {};
}


int main(int argc, char* argv[]) {
    neutrino::utils::console c;

   // std::ifstream ifs("/home/igor/games/ssi/Champions of Krynn/TILES.DAX", std::ios::binary);
    std::ifstream ifs("/home/igor/games/ssi/Champions of Krynn/8X8D1.DAX", std::ios::binary);
    auto dax = dax_file::load (ifs);

    return 0;
}