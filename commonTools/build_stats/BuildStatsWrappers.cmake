################################################################################
#
# Set up for build stats compiler wrappers and gathering up build stats
#
################################################################################


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

    set(gather_build_status "${${PROJECT_NAME}_BINARY_DIR}/gather_build_stats.sh")
    configure_file("${BUILD_STATS_SRC_DIR}/gather_build_stats.sh"
      "${gather_build_status}" COPYONLY)

  endif()

endfunction()


# Generate the build stats compiler wrapper for a single language.
#
function(generate_build_stats_wrapper_for_lang  lang)

  string(TOLOWER "${lang}" lang_lc)
  set(compiler_wrapper
    "${${PROJECT_NAME}_BINARY_DIR}/build_stat_${lang_lc}_wrapper.sh")

  # Override the compiler with the wrapper but remember the original compiler
  if ("${CMAKE_${lang}_COMPILER_ORIG}" STREQUAL "")
    set(CMAKE_${lang}_COMPILER_ORIG "${CMAKE_${lang}_COMPILER}"
      CACHE FILEPATH "Original non-wrappeed ${lang} compiler" FORCE )
    set(CMAKE_${lang}_COMPILER "${compiler_wrapper}"
      CACHE FILEPATH "Overwritten build stats ${lang} compiler wrapper" FORCE )
  endif()

  message("-- " "Generated build stats compiler wrapper for ${lang}")
  set(BUILD_STAT_COMPILER_WRAPPER_INNER_COMPILER "${CMAKE_${lang}_COMPILER_ORIG}")
  configure_file("${BUILD_STATS_SRC_DIR}/build_stat_lang_wrapper.sh.in"
    "${compiler_wrapper}" @ONLY)

endfunction()

# NOTE: The above implementation will make sure the compiler wrapper will get
# updated if the *.sh.in template file changes and just reconfiguring.
# Actaully, you should be able to fix the wrapper and just type 'make' and it
# should reconfigure and update automatically.


# Set up install targets for the build stats scripts
#
# This can only be called after these install dirs have been set!
#
function(install_build_stats_scripts)

  if (${PROJECT_NAME}_USE_BUILD_PERF_WRAPPERS)

    set(gather_build_status "${${PROJECT_NAME}_BINARY_DIR}/gather_build_stats.sh")
    install(PROGRAMS "${gather_build_status}" 
      DESTINATION "${${PROJECT_NAME}_INSTALL_RUNTIME_DIR}")

    install_build_stats_wrapper_for_lang(C)
    install_build_stats_wrapper_for_lang(CXX)
    if (${PROJECT_NAME}_ENABLE_Fortran)
      install_build_stats_wrapper_for_lang(Fortran)
    endif()

  endif()

endfunction()


# Install the build stats compiler wrapper for a single language.
#
function(install_build_stats_wrapper_for_lang  lang)
  string(TOLOWER "${lang}" lang_lc)
  set(compiler_wrapper
    "${${PROJECT_NAME}_BINARY_DIR}/build_stat_${lang_lc}_wrapper.sh")
  install(PROGRAMS "${compiler_wrapper}"
    DESTINATION "${${PROJECT_NAME}_INSTALL_RUNTIME_DIR}")
endfunction()


