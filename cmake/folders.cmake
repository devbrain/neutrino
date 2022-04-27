macro(set_target_folder tgt name)
	set_target_properties(${tgt} PROPERTIES FOLDER ${name})
endmacro()

macro(set_thirdparty tgt)
	set_target_folder(${tgt} "ThirdParty")
endmacro()