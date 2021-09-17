//
// Created by igor on 03/08/2021.
//

#include <neutrino/utils/exception.hh>
#include <neutrino/utils/strings/string_constant.hh>
#include <cstring>

static const char* remove_slash (const char* x) {
  if (x[0] == '/' || x[0] == '\\') {
    return x + 1;
  }
  return x;
}

namespace neutrino {
  bool exception::s_use_function_sig = false;

  bool exception::use_function_signature (bool v) {
    std::swap (s_use_function_sig, v);
    return v;
  }

  const char* exception::what () const noexcept {
    if (m_what.empty ()) {
      try {
        std::ostringstream os;
        os << _create (true, s_use_function_sig ? m_function.c_str () : "", m_source.c_str (), m_line, m_text);
        const exception* cause = m_chain.get ();
        while (cause) {
          os << "\n" << "|-> "
             << _create (true, s_use_function_sig ? cause->m_function.c_str () : "",
                         cause->m_source.c_str (), cause->m_line, cause->m_text);
          cause = cause->m_chain.get ();
        }
        m_what = os.str ();
      }
      catch (...) {
        return "Failed to produce what() message";
      }
    }
    return m_what.c_str ();
  }

  const char* exception::_transform_source (const char* source) {
#if defined(NEUTRINO_PATH_INCLUDE)
    auto pfx1 = utils::string_factory (NEUTRINO_PATH_INCLUDE);
#else
    auto pfx1 = utils::string_factory("");
#endif
#if defined(NEUTRINO_PATH_PRIVATE)
    auto pfx2 = utils::string_factory (NEUTRINO_PATH_PRIVATE);
#else
    auto pfx2 = utils::string_factory("");
#endif

    if constexpr(pfx1.length () < pfx2.length ()) {
      const char* p = strstr (source, pfx2.c_str ());
      if (!p) {
        p = strstr (source, pfx1.c_str ());
        if (p) {
          return remove_slash (source + pfx1.length ());
        }
      }
      else {
        return remove_slash (source + pfx2.length ());
      }
    }
    else {
      const char* p = strstr (source, pfx1.c_str ());
      if (!p) {
        p = strstr (source, pfx2.c_str ());
        if (p) {
          return remove_slash (source + pfx2.length ());
        }
      }
      else {
        return remove_slash (source + pfx1.length ());
      }
    }
    return source;
  }
}