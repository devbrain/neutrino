//
// Created by igor on 31/05/2022.
//

#include <neutrino/assets/resources/resource.hh>

namespace neutrino::assets {
  resource_processing_context::~resource_processing_context() = default;
  void resource_processing_context::on_load_begin([[maybe_unused]] const resource_id& id) {}
  void resource_processing_context::on_load_end([[maybe_unused]] const resource_id& id) {}
  void resource_processing_context::on_load_failure([[maybe_unused]] const resource_id& id) {}

  void resource_processing_context::on_save_begin([[maybe_unused]] const resource_id& id) {}
  void resource_processing_context::on_save_end([[maybe_unused]] const resource_id& id) {}
  void resource_processing_context::on_save_failure([[maybe_unused]] const resource_id& id) {}
}