###########################################################################
# This file is part of LImA, a Library for Image Acquisition
#
#  Copyright (C) : 2009-2017
#  European Synchrotron Radiation Facility
#  CS40220 38043 Grenoble Cedex 9 
#  FRANCE
# 
#  Contact: lima@esrf.fr
# 
#  This is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 3 of the License, or
#  (at your option) any later version.
# 
#  This software is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
# 
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, see <http://www.gnu.org/licenses/>.
############################################################################
set(NAME "imxpad")

set(CMAKE_SHARED_LINKER_FLAGS "-Wl,--add-needed")

set(${NAME}_srcs src/imXpadCamera.cpp  src/imXpadInterface.cpp
	 src/imXpadDetInfoCtrlObj.cpp src/imXpadSyncCtrlObj.cpp
	 src/imXpadClient.cpp)
	 
add_library(lima${NAME} SHARED ${imxpad_srcs})

#INCLUDES
target_include_directories(lima${NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(lima${NAME} PUBLIC "${CMAKE_SOURCE_DIR}/third-party/libconfig/lib")


#LIBRARY
target_link_libraries(lima${NAME} limacore)

include(CameraVERSION)
install(TARGETS lima${NAME} LIBRARY DESTINATION lib)

if(LIMA_ENABLE_PYTHON)
    limatools_run_sip_for_camera(${NAME})
    install(FILES python/__init__.py DESTINATION "${PYTHON_SITE_PACKAGES_DIR}/Lima/ImXPAD")
endif()

if(LIMA_ENABLE_TESTS)
	add_subdirectory(test/)
endif()
