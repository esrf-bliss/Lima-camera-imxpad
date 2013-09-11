//###########################################################################
// This file is part of LImA, a Library for Image
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
// XpadCamera.cpp
// Created on: August 01, 2013
// Author: Hector PEREZ PONCE

#include <sstream>
#include <iostream>
#include <string>
#include <math.h>
#include <iomanip>
#include "XpadCamera.h"
#include "Exceptions.h"
#include "Debug.h"
#include <unistd.h>

using namespace lima;
using namespace lima::Xpad;
using namespace std;


//---------------------------
//- utility thread
//---------------------------
class Camera::AcqThread: public Thread {
    DEB_CLASS_NAMESPC(DebModCamera, "Camera", "AcqThread");
public:
    AcqThread(Camera &aCam);
    virtual ~AcqThread();

protected:
    virtual void threadFunction();

private:
    Camera& m_cam;
};

//---------------------------
// @brief  Ctor
//---------------------------m_npixels

Camera::Camera(string hostname, int port, string xpad_model) : m_hostname(hostname), m_port(port){
    DEB_CONSTRUCTOR();

    //	DebParams::setModuleFlags(DebParams::AllFlags);
    //  DebParams::setTypeFlags(DebParams::AllFlags);
    //	DebParams::setFormatFlags(DebParams::AllFlags);
    m_acq_thread = new AcqThread(*this);
    m_acq_thread->start();
    m_xpad = new XpadClient();

    if	(xpad_model == "XPAD_S70")	{m_xpad_model = XPAD_S70; m_modules_mask = 1; m_chip_mask = 127; m_module_number = 1; m_chip_number = 7;}

    else
        THROW_HW_ERROR(Error) << "[ "<< "Xpad Model not supported"<< " ]";

    DEB_TRACE() << "--> Number of Modules 		 = " << m_module_number ;
    DEB_TRACE() << "--> Number of Chips 		 = " << m_chip_number ;

    //ATTENTION: Modules should be ordered!
    m_image_size = Size(IMG_COLUMN * m_chip_number, IMG_LINE * m_module_number);

}

Camera::~Camera() {
    DEB_DESTRUCTOR();
    m_xpad->disconnectFromServer();
    delete m_xpad;
    delete m_acq_thread;
}

void Camera::init() {
    DEB_MEMBER_FUNCT();
    DEB_TRACE() << "********** Inside of Camera::init ***********";

    stringstream cmd1, cmd2;
    int dataPort, value;
    string message;

    if (m_xpad->connectToServer(m_hostname, m_port) < 0) {
        THROW_HW_ERROR(Error) << "[ " << m_xpad->getErrorMessage() << " ]";
    }
    if ((dataPort = m_xpad->initServerDataPort()) < 0) {
        THROW_HW_ERROR(Error) << "[ " << m_xpad->getErrorMessage() << " ]";
    }

    this->DefineDetectorModel(m_xpad_model);
    DEB_TRACE() << "da.server assigned dataport " << dataPort;

    DEB_TRACE() << "********** Outside of Camera::init ***********";
}

void Camera::reset() {
    DEB_MEMBER_FUNCT();
    DEB_TRACE() << "********** Inside of Camera::reset ***********";

    stringstream cmd1;
    cmd1 << "ResetModule";
    m_xpad->sendWait(cmd1.str());
    DEB_TRACE() << "Reset of detector  -> OK";
    //m_xpad->disconnectFromServer();
    //init(0);

    DEB_TRACE() << "********** Outside of Camera::reset ***********";
}

void Camera::prepareAcq() {
    DEB_MEMBER_FUNCT();
    DEB_TRACE() << "********** Inside of Camera::prepareAcq ***********";

    int value;
    stringstream cmd1;
    cmd1 << "SetExposureParameters " << m_exp_time_usec << " " << 4000 << " " << m_xpad_trigger_mode << " " << 0;
    m_xpad->sendWait(cmd1.str(), value);
    if(!value)
        DEB_TRACE() << "Default exposure parameter applied SUCCESFULLY";

    DEB_TRACE() << "********** Outside of Camera::prepareAcq ***********";
}

void Camera::startAcq() {
    DEB_MEMBER_FUNCT();
    DEB_TRACE() << "********** Inside of Camera::startAcq ***********";

    stringstream cmd;
    m_acq_frame_nb = 0;
    StdBufferCbMgr& buffer_mgr = m_bufferCtrlObj.getBuffer();
    buffer_mgr.setStartTimestamp(Timestamp::now());

    //cmd << "xstrip timing start " << m_sysName;
    //m_xpad->sendWait(cmd.str());

    cmd <<  "Expose";
    m_xpad->sendWait(cmd.str());

    AutoMutex aLock(m_cond.mutex());
    m_wait_flag = false;
    m_quit = false;
    m_cond.broadcast();
    // Wait that Acq thread start if it's an external trigger
    while (m_xpad_trigger_mode == ExtTrigMult && !m_thread_running)
        m_cond.wait();

    DEB_TRACE() << "********** Outside of Camera::startAcq ***********";
}

void Camera::stopAcq() {
    DEB_MEMBER_FUNCT();
    DEB_TRACE() << "********** Inside of Camera::stopAcq ***********";

    /*stringstream cmd;
    cmd <<  "AbortExposure ";
    m_xpad->sendWait(cmd.str());*/


    AutoMutex aLock(m_cond.mutex());
    m_wait_flag = true;
    while (m_thread_running)
        m_cond.wait();
    DEB_TRACE() << "********** Outside of Camera::stopAcq ***********";
}

void Camera::readFrame(void *bptr, int frame_nb) {
    DEB_MEMBER_FUNCT();
    DEB_TRACE() << "********** Inside of Camera::readFrame ***********";
    stringstream cmd;
    string retstring;
    unsigned int num = m_chip_number;
    DEB_TRACE() << "reading frame " << frame_nb;

    if (m_xpad_format==0)
        cmd << "ReadImage2B";
    else if (m_xpad_format==1)
        cmd << "ReadImage4B";

    m_xpad->sendWait(cmd.str(), retstring);
    DEB_TRACE() << retstring;
    m_xpad->getData(bptr, num, m_xpad_format);

    if (m_xpad_format==0){
        unsigned short *dptr;
        dptr = (unsigned short *)bptr;
        std::cout << *dptr << " " << *(dptr+1) << " " << *(dptr+2) << " ... " << *(dptr+67200-3) << " " << *(dptr+67200-2) << " " << *(dptr+67200-1) << std::endl;
    }
    else if (m_xpad_format==1){
        unsigned int *dptr;
        dptr = (unsigned int *)bptr;
        std::cout << *dptr << " " << *(dptr+1) << " " << *(dptr+2) << " ... " << *(dptr+67200-3) << " " << *(dptr+67200-2) << " " << *(dptr+67200-1) << std::endl;
    }
    DEB_TRACE() << "********** Outside of Camera::readFrame ***********";
}

void Camera::getStatus(XpadStatus& status) {
    DEB_MEMBER_FUNCT();
    DEB_TRACE() << "********** Inside of Camera::getStatus ***********";

    stringstream cmd;
    string str;
    unsigned short pos, pos2;
    cmd << "GetStatus";
    m_xpad->sendWait(cmd.str(), str);
    pos = str.find(":");
    string state = str.substr (0, pos);
    if (state.compare("Idle") == 0) {
        status.state = XpadStatus::Idle;
    } else if (state.compare("Digital_Test") == 0) {
        status.state = XpadStatus::Test;
    } else if (state.compare("Resetting") == 0) {
        status.state = XpadStatus::Resetting;
    } else {
        status.state = XpadStatus::Running;
    }

    DEB_TRACE() << "XpadStatus.state is [" << status.state << "]";

    DEB_TRACE() << "********** Outside of Camera::getStatus ***********";

}

int Camera::getNbHwAcquiredFrames() {
    DEB_MEMBER_FUNCT();
    return m_acq_frame_nb;
}

void Camera::AcqThread::threadFunction() {
    DEB_MEMBER_FUNCT();
    AutoMutex aLock(m_cam.m_cond.mutex());
    StdBufferCbMgr& buffer_mgr = m_cam.m_bufferCtrlObj.getBuffer();

    while (!m_cam.m_quit) {
        while (m_cam.m_wait_flag && !m_cam.m_quit) {
            DEB_TRACE() << "Wait";
            m_cam.m_thread_running = false;
            m_cam.m_cond.broadcast();
            m_cam.m_cond.wait();
        }
        DEB_TRACE() << "AcqThread Running";
        m_cam.m_thread_running = true;
        if (m_cam.m_quit)
            return;

        m_cam.m_cond.broadcast();
        aLock.unlock();

        bool continueFlag = true;
        while (continueFlag && (!m_cam.m_nb_frames || m_cam.m_acq_frame_nb < m_cam.m_nb_frames)) {
            XpadStatus status;
            m_cam.getStatus(status);
            if (status.state == status.Idle || status.completed_frames > m_cam.m_acq_frame_nb) {

                void *bptr = buffer_mgr.getFrameBufferPtr(m_cam.m_acq_frame_nb);

                m_cam.readFrame(bptr, m_cam.m_acq_frame_nb);

                HwFrameInfoType frame_info;
                frame_info.acq_frame_nb = m_cam.m_acq_frame_nb;
                continueFlag = buffer_mgr.newFrameReady(frame_info);
                DEB_TRACE() << "acqThread::threadFunction() newframe ready ";
                ++m_cam.m_acq_frame_nb;
            } else {
                AutoMutex aLock(m_cam.m_cond.mutex());
                continueFlag = !m_cam.m_wait_flag;
                usleep(1000);
                /*if (m_cam.m_wait_flag) {
                    stringstream cmd;
                    cmd << "xstrip timing stop " << m_cam.m_sysName;
                    m_cam.m_xpad->sendWait(cmd.str());
                } else {
                    usleep(1000);
                }*/
            }
            DEB_TRACE() << "acquired " << m_cam.m_acq_frame_nb << " frames, required " << m_cam.m_nb_frames << " frames";
        }
        aLock.lock();
        m_cam.m_wait_flag = true;
    }
}

Camera::AcqThread::AcqThread(Camera& cam) :
    m_cam(cam) {
    AutoMutex aLock(m_cam.m_cond.mutex());
    m_cam.m_wait_flag = true;
    m_cam.m_quit = false;
    aLock.unlock();
    pthread_attr_setscope(&m_thread_attr, PTHREAD_SCOPE_PROCESS);
}

Camera::AcqThread::~AcqThread() {
    AutoMutex aLock(m_cam.m_cond.mutex());
    m_cam.m_quit = true;
    m_cam.m_cond.broadcast();
    aLock.unlock();
}

void Camera::getDetectorImageSize(Size& size) {
    DEB_MEMBER_FUNCT();
    //size = Size(m_npixels, 1);
    size = m_image_size;
}

void Camera::getPixelSize(double& size_x, double& size_y) {
    DEB_MEMBER_FUNCT();
    //sizex = xPixelSize;
    //sizey = yPixelSize;
    size_x = 130; // pixel size is 130 micron
    size_y = 130;
}

void Camera::getImageType(ImageType& pixel_depth) {
    DEB_MEMBER_FUNCT();
    //type = m_image_type;
    switch( m_xpad_format )
    {
    case 0:
        pixel_depth = Bpp16;
        break;

    case 1:
        pixel_depth = Bpp32;
        break;
    }
}

void Camera::setImageType(ImageType pixel_depth) {
    //this is what xpad generates but may need Bpp32 for accummulate
    DEB_MEMBER_FUNCT();
    //m_image_type = type;
    switch( pixel_depth )
    {
    case Bpp16:
        m_pixel_depth = B2;
        m_xpad_format = 0;
        break;

    case Bpp32:
        m_pixel_depth = B4;
        m_xpad_format = 1;
        break;
    default:
        DEB_ERROR() << "Pixel Depth is unsupported: only 16 or 32 bits is supported" ;
        throw LIMA_HW_EXC(Error, "Pixel Depth is unsupported: only 16 or 32 bits is supported");
        break;
    }
}

void Camera::getDetectorType(std::string& type) {
    DEB_MEMBER_FUNCT();
    type = "XPAD";
}

void Camera::getDetectorModel(std::string& model) {
    DEB_MEMBER_FUNCT();
    //stringstream ss;
    ////	ss << "Revision " << xpad_get_revision(m_sys);
    //model = ss.str();
    if(m_xpad_model == XPAD_S10) model = "XPAD_S10";
    else if(m_xpad_model == XPAD_C10) model = "XPAD_C10";
    else if(m_xpad_model == XPAD_A10) model = "XPAD_A10";
    else if(m_xpad_model == XPAD_S70) model = "XPAD_S70";
    else if(m_xpad_model == XPAD_S70C) model = "XPAD_S70C";
    else if(m_xpad_model == XPAD_S140) model = "XPAD_S140";
    else if(m_xpad_model == XPAD_S340) model = "XPAD_S340";
    else if(m_xpad_model == XPAD_S540) model = "XPAD_S540";
    else if(m_xpad_model == XPAD_S540V) model = "XPAD_S540V";
    else throw LIMA_HW_EXC(Error, "Xpad Type not supported");
}

HwBufferCtrlObj* Camera::getBufferCtrlObj() {
    return &m_bufferCtrlObj;
}


void Camera::setTrigMode(TrigMode mode) {
    DEB_MEMBER_FUNCT();
    DEB_TRACE() << "Camera::setTrigMode - " << DEB_VAR1(mode);
    DEB_PARAM() << DEB_VAR1(mode);

    /*switch (mode) {
    case IntTrig:
    case IntTrigMult:
    case ExtTrigMult:
        m_trigger_mode = mode;
        break;
    case ExtTrigSingle:
    case ExtGate:
    case ExtStartStop:
    case ExtTrigReadout:
    default:
        THROW_HW_ERROR(Error) << "Cannot change the Trigger Mode of the camera, this mode is not managed !";
        break;
    }*/

    switch( mode )
    {
    case IntTrig:
        m_xpad_trigger_mode = 0;
        break;
    case ExtGate:
        m_xpad_trigger_mode = 1;
        break;
    case ExtTrigSingle:
        m_xpad_trigger_mode = 2;
        break;
    default:
        DEB_ERROR() << "Error: Trigger mode unsupported: only IntTrig, ExtGate or ExtTrigSingle" ;
        throw LIMA_HW_EXC(Error, "Trigger mode unsupported: only IntTrig, ExtGate or ExtTrigSingle");
        break;
    }
}

void Camera::getTrigMode(TrigMode& mode) {
    DEB_MEMBER_FUNCT();
    //mode = m_trigger_mode;
    switch( m_xpad_trigger_mode )
    {
    case 0:
        mode = IntTrig;
        break;
    case 1:
        mode = ExtTrigSingle;
        break;
    case 2:
        mode = ExtGate;
        break;
    default:
        break;
    }
    DEB_RETURN() << DEB_VAR1(mode);
}

void Camera::setExpTime(double exp_time_sec) {
    DEB_MEMBER_FUNCT();
    DEB_TRACE() << "Camera::setExpTime - " << DEB_VAR1(exp_time_sec);

    //m_exp_time = exp_time;
    m_exp_time_usec = exp_time_sec * 1e6;
}

void Camera::getExpTime(double& exp_time_sec) {
    DEB_MEMBER_FUNCT();
    DEB_TRACE() << "Camera::getExpTime";
    ////	AutoMutex aLock(m_cond.mutex());
    //exp_time = m_exp_time;

    exp_time_sec = m_exp_time_usec / 1e6;
    DEB_RETURN() << DEB_VAR1(exp_time_sec);
}

void Camera::setLatTime(double lat_time) {
    DEB_MEMBER_FUNCT();
    DEB_PARAM() << DEB_VAR1(lat_time);

    if (lat_time != 0.)
        THROW_HW_ERROR(Error) << "Latency not managed";
}

void Camera::getLatTime(double& lat_time) {
    DEB_MEMBER_FUNCT();
    lat_time = 0;
}

void Camera::setNbFrames(int nb_frames) {
    DEB_MEMBER_FUNCT();
    DEB_TRACE() << "Camera::setNbFrames - " << DEB_VAR1(nb_frames);
    if (m_nb_frames < 0) {
        THROW_HW_ERROR(Error) << "Number of frames to acquire has not been set";
    }
    m_nb_frames = nb_frames;
}

void Camera::getNbFrames(int& nb_frames) {
    DEB_MEMBER_FUNCT();
    DEB_TRACE() << "Camera::getNbFrames";
    DEB_RETURN() << DEB_VAR1(m_nb_frames);
    nb_frames = m_nb_frames;
}

bool Camera::isAcqRunning() const {
    AutoMutex aLock(m_cond.mutex());
    return m_thread_running;
}

/////////////////////////
// XPAD Specific
/////////////////////////


//-----------------------------------------------------
//      setExposureParam
//-----------------------------------------------------
/*void Camera::setExposureParameters(  unsigned Texp,unsigned Twait,unsigned Tinit,
                                     unsigned Tshutter,unsigned Tovf,unsigned trigger_mode, unsigned n,unsigned p,
                                     unsigned nbImages,unsigned BusyOutSel,unsigned formatIMG,unsigned postProc,
                                     unsigned GP1,unsigned GP2,unsigned GP3,unsigned GP4)
*/

void Camera::getUSBDeviceList(void){
    DEB_MEMBER_FUNCT();
    DEB_TRACE() << "********** Inside of Camera::getUSBDeviceList ***********";

    string message;
    stringstream cmd;

    cmd.str(string());
    cmd << "GetUSBDeviceList";
    m_xpad->sendWait(cmd.str(), message);
    DEB_TRACE() << "List of USB devices connected: " << message;

    DEB_TRACE() << "********** Outside of Camera::getUSBDeviceList ***********";

}

void Camera::setUSBDevice(unsigned short device){
    DEB_MEMBER_FUNCT();
    DEB_TRACE() << "********** Inside of Camera::setUSBDevice ***********";

    int ret;
    stringstream cmd;

    cmd.str(string());
    cmd << "SetUSBDevice " << device;
    m_xpad->sendWait(cmd.str(), ret);
    if(!ret)
        DEB_TRACE() << "Setting active USB device to " << device;
    else
        throw LIMA_HW_EXC(Error, "Error in SetModule");

    DEB_TRACE() << "********** Outside of Camera::setUSBDevice ***********";

}

void Camera::DefineDetectorModel(unsigned short model){
    DEB_MEMBER_FUNCT();
    DEB_TRACE() << "********** Inside of Camera::DefineDetectorModel ***********";

    int ret;
    stringstream cmd;

    cmd.str(string());
    cmd << "DefineDetectorModel " << model;
    m_xpad->sendWait(cmd.str(), ret);
    if(!ret)
        DEB_TRACE() << "Defining Detector Model to " << model;
    else
        throw LIMA_HW_EXC(Error, "Error in DefineDetectorModel");


    DEB_TRACE() << "********** Outside of Camera::DefineDetectorModel ***********";

}

void Camera::askReady(){
    DEB_MEMBER_FUNCT();
    DEB_TRACE() << "********** Inside of Camera::askReady ***********";

    int ret;
    stringstream cmd;

    cmd.str(string());
    cmd << "AskReady";
    m_xpad->sendWait(cmd.str(), ret);
    if(!ret)
        DEB_TRACE() << "Module " << m_modules_mask << " is READY";
    else
        throw LIMA_HW_EXC(Error, "Error in AskReady!");


    DEB_TRACE() << "********** Outside of Camera::askRead ***********";
}

void Camera::digitalTest(unsigned short DT_value, unsigned short mode){
    DEB_MEMBER_FUNCT();
    DEB_TRACE() << "********** Inside of Camera::digitalTest ***********";

    int ret;
    stringstream cmd;

    cmd.str(string());
    cmd << "DigitalTest " << DT_value << " " << mode ;
    m_xpad->sendWait(cmd.str(), ret);
    if(!ret)
        DEB_TRACE() << "Digital Test performed SUCCESFULLY";
    else
        throw LIMA_HW_EXC(Error, "Error in Digital Test!");

    DEB_TRACE() << "********** Outside of Camera::digitalTest ***********";
}

void Camera::loadConfigGFromFile(char *fpath){
    DEB_MEMBER_FUNCT();
    DEB_TRACE() << "********** Inside of Camera::loadConfigGFromFile ***********";

    int ret;
    stringstream cmd;

    ifstream file(fpath, ios::in);
    if (file.is_open()){

        unsigned short module = 0;
        unsigned short reg = 0;
        unsigned short regVal[m_chip_number];

        cmd.str(string());
        cmd << "LoadConfigGFromFile ";

        while(!file.eof()){
            file >> module;
            cmd << module << " ";
            file >> reg;
            cmd << reg << " ";
            for (int i=0; i<m_chip_number ; i++){
                file >> regVal[i];
                cmd << regVal[i] << " ";
            }
        }
        file.close();
    }

    m_xpad->sendWait(cmd.str(), ret);
    if(!ret)
        DEB_TRACE() << "Global configuration loaded from file SUCCESFULLY";
    else
        throw LIMA_HW_EXC(Error, "Error loading global configuration from file!");


    DEB_TRACE() << "********** Outside of Camera::loadConfigGFromFile ***********";
}

void Camera::saveConfigGToFile(char *fpath){
    DEB_MEMBER_FUNCT();
    DEB_TRACE() << "********** Inside of Camera::saveConfigGToFile ***********";

    int ret;
    unsigned short  regid;

    stringstream cmd;

    //File is being open to be writen
    ofstream file(fpath, ios::out);
    if (file.is_open()){

        string          retString;

        //All registers are being read
        for (unsigned short counter=0; counter<7; counter++){
            switch(counter){
            case 0: regid=AMPTP;break;
            case 1: regid=IMFP;break;
            case 2: regid=IOTA;break;
            case 3: regid=IPRE;break;
            case 4: regid=ITHL;break;
            case 5: regid=ITUNE;break;
            case 6: regid=IBUFF;
            }

            //Each register value is read from the detector
            cmd.str(string());
            cmd << "ReadConfigG " << regid;
            m_xpad->sendWait(cmd.str(), retString);

            if (retString.length() > 1){
                //Register values are being stored in the file
                file << m_modules_mask << " ";
                file << retString.c_str() << " ";
                file << endl;
                ret = 0;
            }
            else{
                //message = "ERROR: Could not read global configurations values from server";
                //this->ShowError();
                ret = -1;
            }
        }
        file.close();
    }

    if(!ret)
        DEB_TRACE() << "Global configuration saved to file SUCCESFULLY";
    else
        throw LIMA_HW_EXC(Error, "Error saving global configuration to file!");


    DEB_TRACE() << "********** Outside of Camera::saveConfigGToFile ***********";
}

void Camera::loadConfigG(unsigned short reg, unsigned short value){
    DEB_MEMBER_FUNCT();
    DEB_TRACE() << "********** Inside of Camera::loadConfigG ***********";

    string ret;
    stringstream cmd;

    cmd.str(string());
    cmd << "LoadConfigG "<< reg << " " << value ;
    m_xpad->sendWait(cmd.str(), ret);
    if(ret.length()>1)
        DEB_TRACE() << "Loading global register: " << reg << " with value= " << value;
    else
        throw LIMA_HW_EXC(Error, "Error loading global configuration!");

    DEB_TRACE() << "********** Outside of Camera::loadConfigG ***********";
}

void Camera::readConfigG(unsigned short reg, void *values){
    DEB_MEMBER_FUNCT();
    DEB_TRACE() << "********** Inside of Camera::readConfigG ***********";

    string message;
    stringstream cmd;

    unsigned short *ret = (unsigned short *)values;
    unsigned short regid;

    cmd.str(string());
    cmd << "ReadConfigG " << " " << reg;
    m_xpad->sendWait(cmd.str(), message);

    if(message.length()>1){
        char *value = new char[message.length()+1];
        strcpy(value,message.c_str());

        char *p = strtok(value," ,.");
        ret[0] = (unsigned short)atoi(p);
        for (int i=1; i<8 ; i++){
            p = strtok(NULL, " ,.");
            ret[i] = (unsigned short)atoi(p);
        }
    }
    else
        ret = NULL;

    if(ret!=NULL)
        DEB_TRACE() << "Reading global configuration performed SUCCESFULLY";
    else
        throw LIMA_HW_EXC(Error, "Could not read global configuration!");

    DEB_TRACE() << "********** Outside of Camera::readConfigG ***********";
}

void Camera::ITHLIncrease(){
    DEB_MEMBER_FUNCT();
    DEB_TRACE() << "********** Inside of Camera::ITHLIncrease ***********";

    int ret;
    stringstream cmd;

    cmd.str(string());
    cmd << "ITHLIncrease";
    m_xpad->sendWait(cmd.str(), ret);
    if(!ret)
        DEB_TRACE() << "ITHL was increased SUCCESFULLY";
    else
        throw LIMA_HW_EXC(Error, "Error in ITHLIncrease!");

    DEB_TRACE() << "********** Outside of Camera::ITHLIncrease ***********";
}

void Camera::ITHLDecrease(){
    DEB_MEMBER_FUNCT();
    DEB_TRACE() << "********** Inside of Camera::ITHLDecrease ***********";

    int ret;
    stringstream cmd;

    cmd.str(string());
    cmd << "ITHLDecrease";
    m_xpad->sendWait(cmd.str(), ret);
    if(!ret)
        DEB_TRACE() << "ITHL was decreased SUCCESFULLY";
    else
        throw LIMA_HW_EXC(Error, "Error in ITHLDecrease!");

    DEB_TRACE() << "********** Outside of Camera::ITHLDecrease ***********";
}

void Camera::loadFlatConfigL(unsigned short flat_value)
{
    DEB_MEMBER_FUNCT();

    DEB_TRACE() << "********** Inside of Camera::loadFlatConfig ***********";

    int ret;
    stringstream cmd;

    cmd.str(string());
    cmd <<  "LoadFlatConfigL " << " " << flat_value*8+1;
    m_xpad->sendWait(cmd.str(), ret);
    if(!ret)
        DEB_TRACE() << "Loading local configurations values, with flat value: " <<  flat_value << " -> OK" ;
    else
        throw LIMA_HW_EXC(Error, "Error in loadFlatConfigL!");

    DEB_TRACE() << "********** Outside of Camera::loadFlatConfig ***********";
}

void Camera::loadConfigLFromFileToSRAM(char *fpath){
    DEB_MEMBER_FUNCT();
    DEB_TRACE() << "********** Inside of Camera::loadConfigLFromFileToSRAM ***********";

    stringstream cmd;
    int ret;

    int LineNumber = m_module_number*IMG_LINE;
    int ColumnNumber = m_chip_number*IMG_COLUMN;

    ifstream file(fpath, ios::in);

    //Opening the file to read
    if (file.is_open()){

        unsigned short *data = new unsigned short [LineNumber*ColumnNumber];
        string retstring;

        cmd.str(string());
        cmd << "ReceiveConfigLData";
        m_xpad->sendWait(cmd.str(),retstring);

        //Reading values from columns for each row. The chip number is estimated depending on the column number
        unsigned int counter = 0;
        while(!file.eof()){
            file >> data[counter];
            counter ++;
        }
        file.close();
        sendData_2B(m_xpad->m_skt, data, LineNumber*ColumnNumber);


        cmd.str(string());
        cmd << "LoadConfigLFromFileToSRAM";
        m_xpad->sendWait(cmd.str(), ret);
    }
    else
        ret = -1;

    if(!ret)
        DEB_TRACE() << "Local configuration data was readed from file and sent to SRAM SUCCESFULLY";
    else
        throw LIMA_HW_EXC(Error, "Error reading local configuration data from file!");


    DEB_TRACE() << "********** Outside of Camera::loadConfigLFromFileToSRAM ***********";
}

void Camera::loadConfigLSRAMToDetector(){
    DEB_MEMBER_FUNCT();
    DEB_TRACE() << "********** Inside of Camera::loadConfigLFromFileToSRAM ***********";

    stringstream cmd;
    int ret;

    cmd.str(string());
    cmd << "LoadConfigLSRAMToDetector";
    m_xpad->sendWait(cmd.str(), ret);

    if(!ret)
        DEB_TRACE() << "Local configuration data was readed from file and sent to SRAM SUCCESFULLY";
    else
        throw LIMA_HW_EXC(Error, "Error reading local configuration data from file!");


    DEB_TRACE() << "********** Outside of Camera::loadConfigLFromFileToSRAM ***********";
}

void Camera::saveConfigLToFile(char *fpath){
    DEB_MEMBER_FUNCT();
    DEB_TRACE() << "********** Inside of Camera::saveConfigLToFile ***********";

    stringstream cmd;
    unsigned int num = m_chip_number;

    int LineNumber = m_module_number*IMG_LINE;
    int ColumnNumber = m_chip_number*IMG_COLUMN;

    unsigned int image_size = LineNumber * ColumnNumber;

    unsigned short *ret = new unsigned short [image_size];
    string retstring;

    cmd.str(string());
    cmd << "ReadConfigL";
    m_xpad->sendWait(cmd.str(),retstring);
    DEB_TRACE() << retstring;
    m_xpad->getData(ret, num, 0);

    ofstream file(fpath, ios::out);
    if (file.is_open()){
        for (int i=0;i<LineNumber;i++) {
            for (int j=0;j<ColumnNumber;j++){
                file << ret[i*ColumnNumber+j] << " ";
            }
            file << endl;
        }
        file.close();
    }
    else
        ret = NULL;

    if(ret != NULL)
        DEB_TRACE() << "Local configuration data was saved to file SUCCESFULLY";
    else
        throw LIMA_HW_EXC(Error, "Error saving local configuration data to file!");


    DEB_TRACE() << "********** Outside of Camera::saveConfigLToFile ***********";
}

void Camera::setExposureParameters(double Texp,double Tovf,unsigned short trigger_mode,unsigned short BusyOutSel){

    DEB_MEMBER_FUNCT();
    DEB_TRACE() << "********** Inside of Camera::SetExposureParameters ***********";

    int ret;
    stringstream cmd;

    cmd.str(string());
    cmd <<  "ExposeParam " << Texp  << " " << Tovf  << " " << trigger_mode  << " " << BusyOutSel;
    m_xpad->sendWait(cmd.str(), ret);
    if(!ret)
        DEB_TRACE() << "Set of exposure parameters SUCCESFULL ";
    else
        throw LIMA_HW_EXC(Error, "Error in SetExposureParameters!");

    DEB_TRACE() << "********** Outside of Camera::SetExposureParameters ***********";

}

void Camera::calibrationOTN(unsigned short OTNConfiguration){

    DEB_MEMBER_FUNCT();
    DEB_TRACE() << "********** Inside of Camera::calibrationOTN ***********";

    int ret;
    stringstream cmd;

    cmd <<  "CalibrationOTN " << OTNConfiguration;
    m_xpad->sendWait(cmd.str(), ret);
    if(!ret)
        DEB_TRACE() << "Set of exposure parameters SUCCESFULL ";
    else
        throw LIMA_HW_EXC(Error, "Error in SetExposureParameters!");

    DEB_TRACE() << "********** Outside of Camera::calibrationOTN ***********";

}

// Send image 2 Bytes to socket
int Camera::sendData_2B(int sockd, const void *vptr, size_t n) {
    size_t      nleft;
    ssize_t     nwritten;
    const unsigned short *buffer;

    buffer = (unsigned short*) vptr;
    nleft  = n * sizeof (unsigned short);
    while ( nleft > 0 ) {
        if ( (nwritten = write(sockd, buffer, nleft)) <= 0 ) {
            throw LIMA_HW_EXC(Error, "Error writing in TCP socket");
        }
        nleft  -= nwritten;
        buffer += nwritten;
    }
    return n;
}






//-----------------------------------------------------
//
//-----------------------------------------------------
/*void Camera::setAcquisitionType(short acq_type)
{
    DEB_MEMBER_FUNCT();
    m_acquisition_type = (Camera::XpadAcqType)acq_type;

    DEB_TRACE() << "m_acquisition_type = " << m_acquisition_type  ;
}*/


//-----------------------------------------------------
//
//-----------------------------------------------------
/*void Camera::loadAllConfigG(unsigned long modNum, unsigned long chipId , unsigned long* config_values)
{
    DEB_MEMBER_FUNCT();

    //- Transform the module number into a module mask on 8 bits
    //- eg: if modNum = 4, mask_local = 8
    unsigned long mask_local_module = 0x00;
    SET(mask_local_module,(modNum-1));// minus 1 because modNum start at 1
    unsigned long mask_local_chip = 0x00;
    SET(mask_local_chip,(chipId-1));// minus 1 because chipId start at 1

    if(xpci_modLoadAllConfigG(mask_local_module,mask_local_chip,
                                                config_values[0],//- CMOS_TP
                                                config_values[1],//- AMP_TP,
                                                config_values[2],//- ITHH,
                                                config_values[3],//- VADJ,
                                                config_values[4],//- VREF,
                                                config_values[5],//- IMFP,
                                                config_values[6],//- IOTA,
                                                config_values[7],//- IPRE,
                                                config_values[8],//- ITHL,
                                                config_values[9],//- ITUNE,
                                                config_values[10]//- IBUFFER
                                ) == 0)
    {
        DEB_TRACE() << "loadAllConfigG for module " << modNum  << ", and chip " << chipId << " -> OK" ;
        DEB_TRACE() << "(loadAllConfigG for mask_local_module " << mask_local_module  << ", and mask_local_chip " << mask_local_chip << " )" ;
    }
    else
    {
        throw LIMA_HW_EXC(Error, "Error in loadAllConfigG!");
    }
}

//-----------------------------------------------------
//
//-----------------------------------------------------
void Camera::loadConfigG(const vector<unsigned long>& reg_and_value)
{
    DEB_MEMBER_FUNCT();

    if(xpci_modLoadConfigG(m_modules_mask, m_chip_number, reg_and_value[0], reg_and_value[1])==0)
    {
        DEB_TRACE() << "loadConfigG: " << reg_and_value[0] << ", with value: " << reg_and_value[1] << " -> OK" ;
    }
    else
    {
        throw LIMA_HW_EXC(Error, "Error in loadConfigG!");
    }
}

//-----------------------------------------------------
//
//-----------------------------------------------------
void Camera::loadAutoTest(unsigned known_value)
{
    DEB_MEMBER_FUNCT();

    unsigned int mode = 0; //- 0 -> flat
    if(xpci_modLoadAutoTest(m_modules_mask, known_value, mode)==0)
    {
        DEB_TRACE() << "loadAutoTest with value: " << known_value << " ; in mode: " << mode << " -> OK" ;
    }
    else
    {
        throw LIMA_HW_EXC(Error, "Error in loadAutoTest!");
    }
}

//-----------------------------------------------------
//		Save the config L (DACL) to XPAD RAM
//-----------------------------------------------------
void Camera::saveConfigL(unsigned long modNum, unsigned long calibId, unsigned long chipId, unsigned long curRow, unsigned long* values)
{
    DEB_MEMBER_FUNCT();

    //- Transform the module number into a module mask on 8 bits
    //- eg: if modNum = 4, mask_local = 8
    unsigned long mask_local = 0x00;
    SET(mask_local,(modNum-1));// -1 because modNum start at 1
    //- because start at 1 at high level and 0 at low level
    chipId = chipId - 1;

    //- Call the xpix fonction
    if(xpci_modSaveConfigL(mask_local,calibId,chipId,curRow,(unsigned int*) values) == 0)
    {
        DEB_TRACE() << "saveConfigL for module: " << modNum << " | chip: " << chipId << " | row: " << curRow << " -> OK" ;
    }
    else
    {
        throw LIMA_HW_EXC(Error, "Error in xpci_modSaveConfigL!");
    }
}

//-----------------------------------------------------
//		Save the config G to XPAD RAM
//-----------------------------------------------------
void Camera::saveConfigG(unsigned long modNum, unsigned long calibId, unsigned long reg,unsigned long* values)
{
    DEB_MEMBER_FUNCT();

    //- Transform the module number into a module mask on 8 bits
    //- eg: if modNum = 4, mask_local = 8
    unsigned long mask_local = 0x00;
    SET(mask_local,(modNum-1)); // -1 because modNum start at 1

    //- Call the xpix fonction
    if(xpci_modSaveConfigG(mask_local,calibId,reg,(unsigned int*) values) == 0)
    {
        DEB_TRACE() << "saveConfigG for module: " << modNum << " | reg: " << reg << " -> OK" ;
    }
    else
    {
        throw LIMA_HW_EXC(Error, "Error in xpci_modSaveConfigG!");
    }
}

//-----------------------------------------------------
//		Load the config to detector chips
//-----------------------------------------------------
void Camera::loadConfig(unsigned long modNum, unsigned long calibId)
{
    DEB_MEMBER_FUNCT();

    //- Transform the module number into a module mask on 8 bits
    //- eg: if modNum = 4, mask_local = 8
    unsigned long mask_local = 0x00;
    SET(mask_local,(modNum-1));// -1 because modNum start at 1

    //- Call the xpix fonction
    if(xpci_modDetLoadConfig(mask_local,calibId) == 0)
    {
        DEB_TRACE() << "loadConfig for module: " << modNum << " | calibID: " << calibId << " -> OK" ;
    }
    else
    {
        throw LIMA_HW_EXC(Error, "Error in xpci_modDetLoadConfig!");
    }
}

//-----------------------------------------------------
//		Get the modules config (Local aka DACL)
//-----------------------------------------------------
unsigned short*& Camera::getModConfig()
{
    DEB_MEMBER_FUNCT();

    DEB_TRACE() << "Lima::Camera::getModConfig -> xpci_getModConfig -> 1" ;
    //- Call the xpix fonction
    if(xpci_getModConfig(m_modules_mask,m_chip_number,m_dacl) == 0)
    {
        DEB_TRACE() << "Lima::Camera::getModConfig -> xpci_getModConfig -> OK" ;
    }
    else
    {
        throw LIMA_HW_EXC(Error, "Error in xpci_getModConfig!");
    }
    DEB_TRACE() << "Lima::Camera::getModConfig -> xpci_getModConfig -> 2" ;

    return m_dacl;
}

//-----------------------------------------------------
//		Reset the detector
//-----------------------------------------------------
void Camera::reset()
{
    DEB_MEMBER_FUNCT();
    unsigned int ALL_MODULES = 0xFF;
    if(xpci_modRebootNIOS(ALL_MODULES) == 0)
    {
        DEB_TRACE() << "reset -> xpci_modRebootNIOS -> OK" ;
    }
    else
    {
        throw LIMA_HW_EXC(Error, "Error in xpci_modRebootNIOS!");
    }
}

//-----------------------------------------------------
//		calibrate over the noise Slow
//-----------------------------------------------------
void Camera::calibrateOTNSlow ( string path)
{
    DEB_MEMBER_FUNCT();

    m_calibration_path = path;
    m_calibration_type = Camera::OTN_SLOW;

    this->post(new yat::Message(XPAD_DLL_CALIBRATE), kPOST_MSG_TMO);
}

//-----------------------------------------------------
//		upload a calibration
//-----------------------------------------------------
void Camera::uploadCalibration(string path)
{
    DEB_MEMBER_FUNCT();

    m_calibration_path = path;
    m_calibration_type = Camera::UPLOAD;

    this->post(new yat::Message(XPAD_DLL_CALIBRATE), kPOST_MSG_TMO);
}

//-----------------------------------------------------
//		upload the wait times between images
//-----------------------------------------------------
void Camera::uploadExpWaitTimes(unsigned long *pWaitTime, unsigned size)
{
    DEB_MEMBER_FUNCT();

    //- Check the number of values
    if (size != m_nb_frames)
    {
        throw LIMA_HW_EXC(Error, "Error in uploadExpWaitTimes: number of values does not correspond to number of images");
    }

    if(imxpad_uploadExpWaitTimes(m_modules_mask,(unsigned int*)pWaitTime,size) == 0)
    {
        DEB_TRACE() << "uploadExpWaitTimes -> imxpad_uploadExpWaitTimes -> OK" ;
    }
    else
    {
        throw LIMA_HW_EXC(Error, "Error in imxpad_uploadExpWaitTimes!");
    }
}

//-----------------------------------------------------
//		increment the ITHL
//-----------------------------------------------------
void Camera::incrementITHL()
{
    DEB_MEMBER_FUNCT();

    if(imxpad_incrITHL(m_modules_mask) == 0)
    {
        DEB_TRACE() << "incrementITHL -> imxpad_incrITHL -> OK" ;
    }
    else
    {
        throw LIMA_HW_EXC(Error, "Error in imxpad_incrITHL!");
    }
}

//-----------------------------------------------------
//		decrement the ITHL
//-----------------------------------------------------
void Camera::decrementITHL()
{
    DEB_MEMBER_FUNCT();

    if(imxpad_decrITHL(m_modules_mask) == 0)
    {
        DEB_TRACE() << "decrementITHL -> imxpad_decrITHL -> OK" ;
    }
    else
    {
        throw LIMA_HW_EXC(Error, "Error in imxpad_decrITHL!");
    }
}

//-----------------------------------------------------
//		Set the specific parameters
//-----------------------------------------------------
void Camera::setSpecificParameters( unsigned deadtime, unsigned init,
                                    unsigned shutter, unsigned ovf,
                                    unsigned n,       unsigned p,
                                    unsigned GP1,     unsigned GP2,    unsigned GP3,      unsigned GP4)
{

    DEB_MEMBER_FUNCT();

    DEB_TRACE() << "Setting Specific Parameters ..." ;

    m_time_between_images_usec  = deadtime; //- Temps entre chaque image
    m_time_before_start_usec    = init;     //- Temps initial
    m_shutter_time_usec         = shutter;
    m_ovf_refresh_time_usec     = ovf;
    m_specific_param_n          = n;
    m_specific_param_p          = p;
    m_specific_param_GP1		= GP1;
    m_specific_param_GP2		= GP2;
    m_specific_param_GP3		= GP3;
    m_specific_param_GP4		= GP4;
}


*/















