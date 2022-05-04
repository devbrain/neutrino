//
// Created by igor on 20/09/2020.
//

#ifndef NEUTRINO_BI_LOADER_HH
#define NEUTRINO_BI_LOADER_HH

#include "rc/resource_loader.hh"

class bi_loader : public resource_loader
{
private:
    std::unique_ptr<std::istream> load(resource_t rc) override;
};


#endif //NEUTRINO_BI_LOADER_HH
