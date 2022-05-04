//
// Created by igor on 20/09/2020.
//

#ifndef NEUTRINO_RESOURCE_LOADER_HH
#define NEUTRINO_RESOURCE_LOADER_HH

#include <istream>
#include <memory>
#include "rc/resource_type.hh"


class resource_loader
{
public:
    virtual ~resource_loader() = default;
    virtual std::unique_ptr<std::istream> load(resource_t rc) = 0;
};


#endif //NEUTRINO_RESOURCE_LOADER_HH
