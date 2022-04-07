//###########################################################################
// This file is part of LImA, a Library for Image Acquisition
//
// Copyright (C) : 2009-2011
// European Synchrotron Radiation Facility
// BP 220, Grenoble 38043
// FRANCE
//
// This is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//###########################################################################
//
// XpadInterface.cpp
// Created on: Aout 01, 2013
// Author: Hector PEREZ PONCE

#include "imXpadInterface.h"
#include "imXpadCamera.h"
#include "imXpadConfig.h"

using namespace lima;
using namespace lima::imXpad;

Interface::Interface(Camera& cam) :
    m_cam(cam), m_det_info(cam), m_sync(cam)
{
    DEB_CONSTRUCTOR();

    HwDetInfoCtrlObj *det_info = &m_det_info;
    m_cap_list.push_back(det_info);

    m_bufferCtrlObj = m_cam.getBufferCtrlObj();
    HwBufferCtrlObj *buffer = m_cam.getBufferCtrlObj();
    m_cap_list.push_back(buffer);

    HwSyncCtrlObj *sync = &m_sync;
    m_cap_list.push_back(sync);

#ifdef WITH_CONFIG
    m_config = new Config(m_cam);
    m_cap_list.push_back(m_config);
#endif
}

Interface::~Interface() {
    DEB_DESTRUCTOR();
#ifdef WITH_CONFIG
    delete m_config;
#endif
}

void Interface::getCapList(CapList &cap_list) const {
    DEB_MEMBER_FUNCT();
    cap_list = m_cap_list;
}

void Interface::reset(ResetLevel reset_level) {
    DEB_MEMBER_FUNCT();
    DEB_PARAM() << DEB_VAR1(reset_level);

    //stopAcq();
    m_cam.reset();
}

void Interface::prepareAcq() {
    DEB_MEMBER_FUNCT();
    
    Size image_size;
    ImageType image_type;

    m_cam.prepareAcq();
    
    m_det_info.getMaxImageSize(image_size);
    m_det_info.getDefImageType(image_type);
    
    FrameDim frame_dim(image_size, image_type);
    m_bufferCtrlObj->setFrameDim(frame_dim);
    m_bufferCtrlObj->setNbConcatFrames(1);
    m_bufferCtrlObj->setNbBuffers(1);
}

void Interface::startAcq() {
    DEB_MEMBER_FUNCT();
    m_cam.startAcq();
}

void Interface::stopAcq() {
    DEB_MEMBER_FUNCT();

    m_cam.stopAcq();    
}

void Interface::getStatus(StatusType& status) {
    DEB_MEMBER_FUNCT();
    Camera::XpadStatus xpadStatus;

    m_cam.getStatus(xpadStatus);
    switch (xpadStatus.state) {
    case Camera::XpadStatus::Idle:
        status.acq = AcqReady;
        status.det = DetIdle;
        //std::cout << "Camera idle" << std::endl;
        break;
    case Camera::XpadStatus::Acquiring:
        status.det = DetExposure;
        status.acq = AcqRunning;
        //std::cout << "Camera acquiring" << std::endl;
        break;
    case Camera::XpadStatus::CalibrationManipulation:
        status.det = DetReadout;
        status.acq = AcqRunning;
        //std::cout << "Camera loading/saving a calibration" << std::endl;
        break;
    case Camera::XpadStatus::Calibrating:
        status.det = DetExposure;
        status.acq = AcqRunning;
        //std::cout << "Camera calibrating" << std::endl;
        break;
    case Camera::XpadStatus::DigitalTest:
        status.det = DetExposure;
        status.acq = AcqRunning;
        //std::cout << "Camera performing digital test" << std::endl;
        break;
    case Camera::XpadStatus::Resetting:
        status.det = DetExposure;
        status.acq = AcqRunning;
        //std::cout << "Camera resetting" << std::endl;
        break;

    }
}

int Interface::getNbHwAcquiredFrames() {
    DEB_MEMBER_FUNCT();
    return m_cam.getNbHwAcquiredFrames();
}
