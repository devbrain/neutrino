fetchcontent_declare(
        CALLABLE_TRAITS_EXT
        GIT_REPOSITORY https://github.com/boostorg/callable_traits.git
        GIT_PROGRESS TRUE
        GIT_SHALLOW TRUE
        GIT_TAG master
        TIMEOUT 10
        UPDATE_COMMAND ""
)

fetchcontent_makeavailable(CALLABLE_TRAITS_EXT)

