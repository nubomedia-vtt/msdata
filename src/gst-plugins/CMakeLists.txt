include_directories(
  ${KmsGstCommons_INCLUDE_DIRS}
  ${gstreamer-1.5_INCLUDE_DIRS}
  ${gstreamer-video-1.5_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${opencv_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS}
)

set(MSDATA_SOURCES
  kmsdatachannelexample.c
  kmsshowdata.c
  kmsshowdata.h
  kmssenddata.c
  kmssenddata.h
  kmsfacedetectormetadata.h
  kmsfacedetectormetadata.cpp
  kmsimageoverlaymetadata.h
  kmsimageoverlaymetadata.cpp
  chartermetadata.h
  chartermetadata.cpp
)

add_library(msdata MODULE ${MSDATA_SOURCES})

target_link_libraries(msdata
  ${KmsGstCommons_LIBRARIES}
  ${gstreamer-1.5_LIBRARIES}
  ${gstreamer-video-1.5_LIBRARIES}
  ${opencv_LIBRARIES}
  ${Boost_LIBRARIES}
)

install(
  TARGETS msdata
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_GST_PLUGINS_DIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
