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

set(CMAKE_SHARED_LINKER_FLAGS "-Wl,--add-needed")

set(imxpad_srcs src/imXpadCamera.cpp  src/imXpadInterface.cpp
	 src/imXpadDetInfoCtrlObj.cpp src/imXpadSyncCtrlObj.cpp
	 src/imXpadClient.cpp)
	 
file(STRINGS "VERSION" imxpad_vers)
add_library(limaimxpad SHARED ${imxpad_srcs})

#INCLUDES
target_include_directories(limaimxpad PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(limaimxpad PUBLIC "${CMAKE_SOURCE_DIR}/third-party/libconfig/lib")


#LIBRARY
target_link_libraries(limaimxpad limacore)

set_target_properties(limaimxpad PROPERTIES VERSION ${imxpad_vers} SOVERSION "1")

install(TARGETS limaimxpad LIBRARY DESTINATION lib)

if(COMPILE_SIP)
    set(NAME "imxpad")
    include(CameraRunSIP)
    install(FILES python/__init__.py DESTINATION "${PYTHON_SITE_PACKAGES_DIR}/Lima/ImXPAD")
endif()

if(COMPILE_TEST)
	add_subdirectory(test/)
endif()
