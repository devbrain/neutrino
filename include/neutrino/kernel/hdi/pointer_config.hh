//
// Created by igor on 19/04/2022.
//

#ifndef INCLUDE_NEUTRINO_KERNEL_HDI_POINTER_CONFIG_HH
#define INCLUDE_NEUTRINO_KERNEL_HDI_POINTER_CONFIG_HH

namespace neutrino {
  class pointer_config_base {
    public:
      virtual ~pointer_config_base();

      virtual void ignore_motion(bool f) = 0;
      virtual void ignore_clicks(bool f) = 0;
      virtual void ignore_releases(bool f) = 0;
  };
}

#endif //INCLUDE_NEUTRINO_KERNEL_HDI_POINTER_CONFIG_HH
