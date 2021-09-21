SET(RE2C_BUILD_RE2GO OFF CACHE INTERNAL "")

fetchcontent_declare(
        RE2C
        URL https://github.com/skvadrik/re2c/archive/refs/tags/2.2.zip
)

fetchcontent_makeavailable(RE2C)
add_executable(lexical::re2c ALIAS re2c)

set_target_properties(re2c PROPERTIES FOLDER "ThirdParty")
set_target_properties(re2c_objects_autogen PROPERTIES FOLDER "ThirdParty")
set_target_properties(re2c_objects_autogen_ver_to_vernum PROPERTIES FOLDER "ThirdParty")
