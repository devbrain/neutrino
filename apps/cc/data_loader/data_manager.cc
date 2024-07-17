//
// Created by igor on 7/16/24.
//

#include "data_manager.hh"
#include <bsw/register_at_exit.hh>

data_manager_t* get_data_manager() {
	static data_manager_t* data_manager = nullptr;

	if (!data_manager) {
		data_manager = new data_manager_t;
		bsw::register_at_exit([]() {
			delete data_manager;
		});
	}
	return data_manager;
}