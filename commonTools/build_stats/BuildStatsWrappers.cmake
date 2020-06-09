set(BUILD_STATS_SRC_DIR "${CMAKE_CURRENT_LIST_DIR}")

# Generate the build stats compiler wrappers if asked to do so.
#
function(generate_build_stats_wrappers)

  set_default_and_from_env(${PROJECT_NAME}_USE_BUILD_PERF_WRAPPERS_DEFAULT OFF)
  advanced_set(${PROJECT_NAME}_USE_BUILD_PERF_WRAPPERS
    ${${PROJECT_NAME}_USE_BUILD_PERF_WRAPPERS_DEFAULT} CACHE BOOL
    "If set to 'ON', then compiler wrappers will be created and used to gather build stats."
    )

  if (${PROJECT_NAME}_USE_BUILD_PERF_WRAPPERS)

    generate_build_stats_wrapper_for_lang(C)
    generate_build_stats_wrapper_for_lang(CXX)
    if (${PROJECT_NAME}_ENABLE_Fortran)
      generate_build_stats_wrapper_for_lang(Fortran)
    endif()

    configure_file("${BUILD_STATS_SRC_DIR}/gather_build_stats.sh"
      "${${PROJECT_NAME}_BINARY_DIR}/gather_build_stats.sh" COPYONLY)

  endif()

endfunction()


function(generate_build_stats_wrapper_for_lang  lang)

  message("generate_build_stats_wrapper_for_lang: ${lang}")

  if ("${CMAKE_${lang}_COMPILER_ORIG}" STREQUAL "")

    message("-- " "Generated build stats compiler wrapper for ${lang}")

    set(CMAKE_${lang}_COMPILER_ORIG "${CMAKE_${lang}_COMPILER}"
      CACHE FILEPATH "Original non-wrappeed ${lang} compiler" FORCE )

    set(BUILD_STAT_COMPILER_WRAPPER_INNER_COMPILER "${CMAKE_${lang}_COMPILER}")

    string(TOLOWER "${lang}" lang_lc)
    set(compiler_wrapper
      "${${PROJECT_NAME}_BINARY_DIR}/build_stat_${lang_lc}_wrapper.sh")

    configure_file("${BUILD_STATS_SRC_DIR}/build_stat_lang_wrapper.sh.in"
      "${compiler_wrapper}" @ONLY)

    set(CMAKE_${lang}_COMPILER "${compiler_wrapper}"
      CACHE FILEPATH "Overwritten build stats ${lang} compiler wrapper" FORCE )

  endif()

endfunction()


