file(GLOB FILENAMES *.cpp)

foreach(filename ${FILENAMES})
  get_filename_component(basename ${filename} NAME_WE)
  add_executable(${basename} ${filename})
  target_link_libraries(${basename} PUBLIC mockturtle)

  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${basename}.cmake")
    include(${basename}.cmake)
  endif()
endforeach()


# add_executable( fanout_aware_rewriting fanout_aware_rewriting.cpp )
# target_link_libraries( fanout_aware_rewriting mockturtle )
