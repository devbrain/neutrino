function(assign_source_group)
	set(options "")
    set(oneValueArgs "")
    set(multiValueArgs ROOTS SOURCES)
	
    cmake_parse_arguments(PARAMS "${options}" "${oneValueArgs}"
                          "${multiValueArgs}" ${ARGN} )
						  
    foreach(_source IN ITEMS ${PARAMS_SOURCES})
		if (PARAMS_ROOTS)
			foreach(_root IN ITEMS ${PARAMS_ROOTS})
				string(FIND "${_source}" "${_root}" out)
					if("${out}" EQUAL 0)
						string(REPLACE "${_root}" "" _source_rel "${_source}")
						break()
					endif()
			endforeach()
		else ()
			if (IS_ABSOLUTE "${_source}")
				file(RELATIVE_PATH _source_rel "${CMAKE_CURRENT_SOURCE_DIR}" "${_source}")
			else()
				set(_source_rel "${_source}")
			endif()
		endif()
        get_filename_component(_source_path "${_source_rel}" PATH)
        string(REPLACE "/" "\\" _source_path_msvc "${_source_path}")
        source_group("${_source_path_msvc}" FILES "${_source}")
    endforeach()
endfunction()
 
# ---------------------------------------------------------------------------------------------- 
function(my_add_executable EXENAME)
	set(options "")
	set(oneValueArgs FOLDER)
	set(multiValueArgs SOURCES)

	cmake_parse_arguments(PARAMS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

	if (PARAMS_SOURCES)
		set (_sources ${PARAMS_SOURCES})
	else ()
		set (_sources ${ARGN})
	endif()

    foreach(_source IN ITEMS ${_sources})
        assign_source_group(
				SOURCES
				${_source}
				)
    endforeach()

    add_executable(${EXENAME} ${_sources})
	target_include_directories(${EXENAME} PRIVATE ${PROJECT_INCLUDE})
	if (PARAMS_FOLDER)
		set_target_properties (${EXENAME} PROPERTIES FOLDER ${PARAMS_FOLDER})
	endif()
	if(MSVC)
  		target_compile_options(${EXENAME} PRIVATE /W4 /WX)
    else()
  		target_compile_options(${EXENAME} PRIVATE -Wall -Wextra -pedantic)
	endif()
endfunction()
 
# ---------------------------------------------------------------------------------------------- 
macro(my_add_library_headers LIBNAME)
	set(options "")
	set(oneValueArgs "")
	set(multiValueArgs DEPENDS SOURCES)

	cmake_parse_arguments(PARAMS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )
	if (PARAMS_SOURCES)
		set (_sources ${PARAMS_SOURCES})
	else ()
		set (_sources ${ARGN})
	endif()

    foreach(_source IN ITEMS ${_sources})
        assign_source_group(
			ROOTS 
				${PROJECT_INCLUDE}/neutrino/${LIBNAME}/ 
			SOURCES 
				${_source}
			)
    endforeach()

    set (_lib neutrino_${LIBNAME})
    add_library(${_lib} INTERFACE)
    add_library(neutrino::${LIBNAME} ALIAS ${_lib})
	target_sources(${_lib} INTERFACE ${_sources})
	target_include_directories(${_lib} INTERFACE $<BUILD_INTERFACE:${PROJECT_INCLUDE}>)
	target_include_directories(${_lib} SYSTEM INTERFACE $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include>)

	if (PARAMS_DEPENDS)
		foreach(_dep IN ITEMS ${PARAMS_DEPENDS})
			target_link_libraries(${_lib} INTERFACE ${_dep})
		endforeach()
	endif()

endmacro()

# ----------------------------------------------------------------------------------------------
macro(my_add_library_static LIBNAME)
	set(options "")
	set(oneValueArgs "")
	set(multiValueArgs DEPENDS_PUBLIC DEPENDS_PRIVATE SOURCES)

	cmake_parse_arguments(PARAMS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )
	if (PARAMS_SOURCES)
		set (_sources ${PARAMS_SOURCES})
	else ()
		set (_sources ${ARGN})
	endif()

	foreach(_source IN ITEMS ${_sources})
		assign_source_group(
				ROOTS
					${PROJECT_INCLUDE}/neutrino/${LIBNAME}/
				SOURCES
				${_source}
		)
	endforeach()

	set (_lib neutrino_${LIBNAME})
	add_library(${_lib} STATIC ${_sources})
	add_library(neutrino::${LIBNAME} ALIAS ${_lib})
	target_include_directories(${_lib}
			INTERFACE
				$<BUILD_INTERFACE:${PROJECT_INCLUDE}>
			PRIVATE
				${PROJECT_INCLUDE_PRIVATE}
			SYSTEM INTERFACE
				$<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include>
			)
	if(MSVC)
		target_compile_options(${_lib} PRIVATE /W4 /WX)
	else()
		target_compile_options(${_lib} PRIVATE -Wall -Wextra -pedantic)
	endif()

	if (PARAMS_DEPENDS_PUBLIC)
		foreach(_dep IN ITEMS ${PARAMS_DEPENDS_PUBLIC})
			target_link_libraries(${_lib} PUBLIC ${_dep})
		endforeach()
	endif()

	if (PARAMS_DEPENDS_PRIVATE)
		foreach(_dep IN ITEMS ${PARAMS_DEPENDS_PRIVATE})
			target_link_libraries(${_lib} PRIVATE ${_dep})
		endforeach()
	endif()
endmacro()
