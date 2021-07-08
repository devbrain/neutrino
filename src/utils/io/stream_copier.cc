//
// Created by igor on 08/07/2021.
//

#include <vector>
#include <neutrino/utils/io/stream_copier.hh>

namespace neutrino::utils::io
{
    std::streamsize stream_copier::copy_stream(std::istream& istr, std::ostream& ostr, std::size_t bufferSize)
    {

        std::vector<char> buffer(bufferSize);
        std::streamsize len = 0;
        istr.read(buffer.data(), bufferSize);
        std::streamsize n = istr.gcount();
        while (n > 0)
        {
            len += n;
            ostr.write(buffer.data(), n);
            if (istr && ostr)
            {
                istr.read(buffer.data(), bufferSize);
                n = istr.gcount();
            } else
            { n = 0; }
        }
        return len;
    }

    std::streamsize stream_copier::copy_to_string(std::istream& istr, std::string& str, std::size_t bufferSize)
    {


        std::vector<char> buffer(bufferSize);
        std::streamsize len = 0;
        istr.read(buffer.data(), bufferSize);
        std::streamsize n = istr.gcount();
        while (n > 0)
        {
            len += n;
            str.append(buffer.data(), static_cast<std::string::size_type>(n));
            if (istr)
            {
                istr.read(buffer.data(), bufferSize);
                n = istr.gcount();
            } else
            { n = 0; }
        }
        return len;
    }

    std::streamsize stream_copier::copy_stream_unbuffered(std::istream& istr, std::ostream& ostr)
    {
        char c = 0;
        std::streamsize len = 0;
        istr.get(c);
        while (istr && ostr)
        {
            ++len;
            ostr.put(c);
            istr.get(c);
        }
        return len;
    }

}