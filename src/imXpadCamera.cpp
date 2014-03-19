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
#include "imXpadCamera.h"
#include "Exceptions.h"
#include "Debug.h"
#include <unistd.h>

using namespace lima;
using namespace lima::imXpad;
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
    
    if	(xpad_model == "XPAD_S70"){
        m_xpad_model = XPAD_S70; m_modules_mask = 1; m_chip_mask = 127; m_module_number = 1; m_chip_number = 7;
    }
    else if	(xpad_model == "XPAD_S70C"){
        m_xpad_model = XPAD_S70; m_modules_mask = 1; m_chip_mask = 127; m_module_number = 1; m_chip_number = 7;
    }

    else
        THROW_HW_ERROR(Error) << "[ "<< "Xpad Model not supported"<< " ]";

    DEB_TRACE() << "--> Number of Modules 		 = " << m_module_number ;
    DEB_TRACE() << "--> Number of Chips 		 = " << m_chip_number ;

    //ATTENTION: Modules should be ordered!
    m_image_size = Size(IMG_COLUMN * m_chip_number, IMG_LINE * m_module_number);
    DEB_TRACE() << "--> Image size               = " << m_image_size;

}

Camera::~Camera() {
    DEB_DESTRUCTOR();
    m_xpad->disconnectFromServer();
    delete m_xpad;
    delete m_acq_thread;
}

int Camera::init() {
    DEB_MEMBER_FUNCT();
    DEB_TRACE() << "********** Inside of Camera::init ***********";

    stringstream cmd1, cmd2;
    int ret;
    string message;

    if (m_xpad->connectToServer(m_hostname, m_port) < 0) {
        THROW_HW_ERROR(Error) << "[ " << m_xpad->getErrorMessage() << " ]";
    }

    //this->setImageType(Bpp32);

    this->getUSBDeviceList();
    if (!this->setUSBDevice(0)){
        this->defineDetectorModel(m_xpad_model);
        ret = this->askReady();
    }

    DEB_TRACE() << "********** Outside of Camera::init ***********";

    return ret;
}

void Camera::reset() {
    DEB_MEMBER_FUNCT();
    DEB_TRACE() << "********** Inside of Camera::reset ***********";

    stringstream cmd1;
    cmd1 << "ResetModules";
    m_xpad->sendWait(cmd1.str());
    DEB_TRACE() << "Reset of detector  -> OK";

    DEB_TRACE() << "********** Outside of Camera::reset ***********";
}

void Camera::prepareAcq() {
    DEB_MEMBER_FUNCT();
    DEB_TRACE() << "********** Inside of Camera::prepareAcq ***********";

    int value;
    stringstream cmd1;
    cmd1 << "SetExposeParameters " << 1 << " " << m_exp_time_usec << " " << 4000 << " " << m_xpad_trigger_mode << " " << 0;
    m_xpad->sendWait(cmd1.str(), value);
    if(!value)
        DEB_TRACE() << "Default exposure parameter applied SUCCESFULLY";

    DEB_TRACE() << "********** Outside of Camera::prepareAcq ***********";
}

void Camera::startAcq() {
    DEB_MEMBER_FUNCT();
    DEB_TRACE() << "********** Inside of Camera::startAcq ***********";

    m_acq_frame_nb = 0;
    StdBufferCbMgr& buffer_mgr = m_bufferCtrlObj.getBuffer();
    buffer_mgr.setStartTimestamp(Timestamp::now());

    /*    stringstream cmd;
    string str;
    int value;
    int ret, dataSize;

    cmd <<  "Expose";
    m_xpad->sendWait(cmd.str(), str);
    if (str.compare("SERVER: Ready to send data")==0){
        cmd.str(std::string());

        cmd << "CLIENT: Ready to receive dataSize";
        m_xpad->sendWait(cmd.str(), dataSize);

        DEB_TRACE() << "Receiving: "  << dataSize;

        cmd.str(std::string());
        cmd << dataSize;
        m_xpad->sendWait(cmd.str(), ret);

        if(ret == 0){
            m_xpad->sendNoWait("OK");

            string dataString;
            int dataCount = 0;

            char data[dataSize];
            for(int i=0; i<dataSize; i++){
                value = m_xpad->getChar();
                data[i] = (char)value;
                if((value != 32) && (value != 13)){
                    dataString.append(1,data[i]);
                }
                else{
                    //cout << dataString << " ";
                    if (xpad_format==0){
                        buffer_short[dataCount] = atoi(dataString.c_str());
                    }
                    else if (xpad_format==1){
                        buffer_int[dataCount] = atoi(dataString.c_str());
                    }
                    dataString.clear();
                    dataCount++;
                }
            }
            //cout << "Total of data sent: " << dataCount << endl;
        }
    }*/

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
    string str;

    int num = m_chip_number;
    DEB_TRACE() << "reading frame " << frame_nb;

    m_xpad->getData(bptr,num, m_xpad_format);

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

void Camera::readFrameExpose(void *bptr, int frame_nb) {
    DEB_MEMBER_FUNCT();
    DEB_TRACE() << "********** Inside of Camera::readFrame ***********";
    stringstream cmd;
    string str;

    int num = m_chip_number;
    DEB_TRACE() << "reading frame " << frame_nb;

    m_xpad->getDataExpose(bptr,num, m_xpad_format);

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

void Camera::getStatus(XpadStatus& status,bool internal) {
    DEB_MEMBER_FUNCT();
    DEB_TRACE() << "********** Inside of Camera::getStatus ***********";

    stringstream cmd;
    string str;
    unsigned short pos, pos2;
    cmd << "GetStatus";
    
    if(internal || !m_thread_running)
      {
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
      }
    else
      status.state = XpadStatus::Running;

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
            m_cam.getStatus(status,true);
            if (status.state == status.Idle || status.completed_frames > m_cam.m_acq_frame_nb) {

                DEB_TRACE() << m_cam.m_acq_frame_nb;
                void *bptr = buffer_mgr.getFrameBufferPtr(m_cam.m_acq_frame_nb);

                m_cam.readFrameExpose(bptr, m_cam.m_acq_frame_nb);

                HwFrameInfoType frame_info;
                frame_info.acq_frame_nb = m_cam.m_acq_frame_nb;
                continueFlag = buffer_mgr.newFrameReady(frame_info);
                DEB_TRACE() << "acqThread::threadFunction() newframe ready ";
                ++m_cam.m_acq_frame_nb;
            } else {
                AutoMutex aLock(m_cam.m_cond.mutex());
                continueFlag = !m_cam.m_wait_flag;
                usleep(1000);
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
    //else if(m_xpad_model == XPAD_S140) model = "XPAD_S140";
    //else if(m_xpad_model == XPAD_S340) model = "XPAD_S340";
    //else if(m_xpad_model == XPAD_S540) model = "XPAD_S540";
    //else if(m_xpad_model == XPAD_S540V) model = "XPAD_S540V";
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
    case ExtTrigMult:
        m_xpad_trigger_mode = 3;
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
        mode = ExtGate;
        break;
    case 2:
        mode = ExtTrigSingle;
        break;
    case 3:
        mode = ExtTrigMult;
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


string Camera::getUSBDeviceList(void){
    DEB_MEMBER_FUNCT();
    DEB_TRACE() << "********** Inside of Camera::getUSBDeviceList ***********";

    string message;
    stringstream cmd;

    cmd.str(string());
    cmd << "GetUSBDeviceList";
    m_xpad->sendWait(cmd.str(), message);

    DEB_TRACE() << "List of USB devices connected: " << message;

    DEB_TRACE() << "********** Outside of Camera::getUSBDeviceList ***********";

    return message;

}

int Camera::setUSBDevice(unsigned short device){
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
        throw LIMA_HW_EXC(Error, "Setting USB device FAILED!");

    DEB_TRACE() << "********** Outside of Camera::setUSBDevice ***********";

    return ret;

}

int Camera::defineDetectorModel(unsigned short model){
    DEB_MEMBER_FUNCT();
    DEB_TRACE() << "********** Inside of Camera::DefineDetectorModel ***********";

    int ret;
    stringstream cmd;

    cmd.str(string());
    cmd << "DefineDetectorModel " << model;
    m_xpad->sendWait(cmd.str(), ret);

    if(!ret)
        DEB_TRACE() << "Defining detector model to " << model;
    else
        throw LIMA_HW_EXC(Error, "Defining detector model FAILED");


    DEB_TRACE() << "********** Outside of Camera::DefineDetectorModel ***********";

    return ret;

}

int Camera::askReady(){
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
        throw LIMA_HW_EXC(Error, "AskReady FAILED!");

    DEB_TRACE() << "********** Outside of Camera::askRead ***********";

    return ret;
}

int Camera::digitalTest(unsigned short DT_value, unsigned short mode){
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
        throw LIMA_HW_EXC(Error, "Digital Test FAILED!");

    DEB_TRACE() << "********** Outside of Camera::digitalTest ***********";

    return ret;
}

int Camera::loadConfigGFromFile(char *fpath){
    DEB_MEMBER_FUNCT();
    DEB_TRACE() << "********** Inside of Camera::loadConfigGFromFile ***********";

    int ret;
    stringstream cmd;

    ifstream file(fpath, ios::in);

    //Opening the file to read
    if (file.is_open()){

        string str;
        stringstream data;
        int dataSize;

        cmd.str(string());
        cmd << "LoadConfigGFromFile ";

        m_xpad->sendWait(cmd.str(), str);
        if (str.compare("SERVER: Ready to receive data")==0){
            cmd.str(string());
            while(!file.eof()){
                char temp;
                file.read(&temp,sizeof(char));
                data << temp;
            }
            string temp = data.str();
            cmd << temp.size();
            m_xpad->sendWait(cmd.str(), dataSize);

            if(temp.size() == dataSize){
                m_xpad->sendWait("OK", temp);
                m_xpad->sendWait(data.str(), temp);
                m_xpad->sendWait("Waiting for answer",ret);
            }
            else{
                m_xpad->sendWait("ERROR", ret);
            }
        }
        file.close();
    }

    if(!ret)
        DEB_TRACE() << "Global configuration loaded from file SUCCESFULLY";
    else
        throw LIMA_HW_EXC(Error, "Loading global configuration from file FAILED!");

    DEB_TRACE() << "********** Outside of Camera::loadConfigGFromFile ***********";

    return ret;
}

int Camera::saveConfigGToFile(char *fpath){
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
                ret = -1;
            }
        }
        file.close();
    }

    if(!ret)
        DEB_TRACE() << "Global configuration saved to file SUCCESFULLY";
    else
        throw LIMA_HW_EXC(Error, "Saving global configuration to file FAILED!");


    DEB_TRACE() << "********** Outside of Camera::saveConfigGToFile ***********";

    return ret;
}

int Camera::loadConfigG(unsigned short reg, unsigned short value){
    DEB_MEMBER_FUNCT();
    DEB_TRACE() << "********** Inside of Camera::loadConfigG ***********";

    string ret;
    stringstream cmd;

    cmd.str(string());
    cmd << "LoadConfigG "<< reg << " " << value ;
    m_xpad->sendWait(cmd.str(), ret);

    if(ret.length()>1){
        DEB_TRACE() << "Loading global register: " << reg << " with value= " << value;
        return 0;
    }
    else{
        throw LIMA_HW_EXC(Error, "Loading global configuration FAILED!");
        return -1;
    }

    DEB_TRACE() << "********** Outside of Camera::loadConfigG ***********";
}

int Camera::readConfigG(unsigned short reg, void *values){
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

    if(ret!=NULL){
        DEB_TRACE() << "Reading global configuration performed SUCCESFULLY";
        return 0;
    }
    else{
        throw LIMA_HW_EXC(Error, "Reading global configuration FAILED!");
        return -1;
    }

    DEB_TRACE() << "********** Outside of Camera::readConfigG ***********";
}

int Camera::loadDefaultConfigGValues(){
    DEB_MEMBER_FUNCT();
    DEB_TRACE() << "********** Inside of Camera::LoadDefaultConfigGValues ***********";

    string ret;
    stringstream cmd;
    int value, regid;

    for (int i=0; i<7; i++){
        switch (i){
        case 0: regid = AMPTP;value = 0;break;
        case 1: regid = IMFP;value = 50;break;
        case 2: regid = IOTA;value = 40;break;
        case 3: regid = IPRE;value = 60;break;
        case 4: regid = ITHL;value = 25;break;
        case 5: regid = ITUNE;value = 100;break;
        case 6: regid = IBUFF;value = 0;
        }
        cmd.str(string());
        cmd << "LoadConfigG "<< regid << " " << value ;
        m_xpad->sendWait(cmd.str(), ret);
    }

    if(ret.length()>1){
        DEB_TRACE() << "Loading global configuratioin with values:\nAMPTP = 0, IMFP = 50, IOTA = 40, IPRE = 60, ITHL = 25, ITUNE = 100, IBUFF = 0";
        return 0;
    }
    else{
        throw LIMA_HW_EXC(Error, "Loading default global configuration values FAILED!");
        return -1;
    }
    DEB_TRACE() << "********** Outside of Camera::LoadDefaultConfigGValues ***********";
}

int Camera::ITHLIncrease(){
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
        throw LIMA_HW_EXC(Error, "ITHL increase FAILED!");

    DEB_TRACE() << "********** Outside of Camera::ITHLIncrease ***********";

    return ret;
}

int Camera::ITHLDecrease(){
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
        throw LIMA_HW_EXC(Error, "ITHL decrease FAILED!");

    DEB_TRACE() << "********** Outside of Camera::ITHLDecrease ***********";

    return ret;
}

int Camera::loadFlatConfigL(unsigned short flat_value)
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
        throw LIMA_HW_EXC(Error, "Loading local configuratin with FLAT value FAILED!");

    DEB_TRACE() << "********** Outside of Camera::loadFlatConfig ***********";

    return ret;
}

int Camera::loadConfigLFromFile(char *fpath){
    DEB_MEMBER_FUNCT();
    DEB_TRACE() << "********** Inside of Camera::loadConfigLFromFile ***********";

    int ret;
    stringstream cmd;

    ifstream file(fpath, ios::in);

    //Opening the file to read
    if (file.is_open()){

        string str;
        stringstream data;
        int dataSize;

        cmd.str(string());
        cmd << "LoadConfigLFromFile ";

        m_xpad->sendWait(cmd.str(), str);
        if (str.compare("SERVER: Ready to receive data")==0){
            cmd.str(string());
            while(!file.eof()){
                char temp;
                file.read(&temp,sizeof(char));
                data << temp;
            }
            string temp = data.str();
            cmd << temp.size();
            m_xpad->sendWait(cmd.str(), dataSize);

            if(temp.size() == dataSize){
                m_xpad->sendWait("OK", str);
                if(str.compare("SERVER: OK")==0){
                    m_xpad->sendWait(data.str(), str);
                    m_xpad->sendWait("Waiting for answer",ret);
                }
            }
            else{
                m_xpad->sendWait("ERROR", ret);
            }
        }
        file.close();
    }

    if(!ret)
        DEB_TRACE() << "Local configuration loaded from file SUCCESFULLY";
    else
        throw LIMA_HW_EXC(Error, "Loading local configuration from file FAILED!");

    DEB_TRACE() << "********** Outside of Camera::loadConfigLFromFile ***********";

    return ret;


}

int Camera::saveConfigLToFile(char *fpath){
    DEB_MEMBER_FUNCT();
    DEB_TRACE() << "********** Inside of Camera::saveConfigLToFile ***********";

    stringstream cmd;
    string str;
    int ret, dataSize;

    cmd << "ReadConfigL";
    m_xpad->sendWait(cmd.str(),str);
    if (str.compare("SERVER: Ready to send data")==0){
        cmd.str(std::string());

        cmd << "CLIENT: Ready to receive dataSize";
        m_xpad->sendWait(cmd.str(), dataSize);

        DEB_TRACE() << "Receiving: "  << dataSize;

        cmd.str(std::string());
        cmd << dataSize;
        m_xpad->sendWait(cmd.str(), ret);

        if(ret == 0){
            m_xpad->sendNoWait("OK");

            string dataString;
            int dataCount = 0;

            ofstream file(fpath, ios::out);
            if (file.is_open()){
                for(int i=0; i<dataSize; i++)
                    file << (char) m_xpad->getChar();
                file.close();
                //cout << "Total of data sent: " << dataCount << endl;
            }
        }
    }
    else
        ret = -1;

    if(ret == 0)
        DEB_TRACE() << "Local configuration was saved to file SUCCESFULLY";
    else
        throw LIMA_HW_EXC(Error, "Saving local configuration to file FAILED!");

    DEB_TRACE() << "********** Outside of Camera::saveConfigLToFile ***********";

    return ret;
}

int Camera::setExposureParameters(unsigned int NumImages, double Texp,double Tovf,unsigned short trigger_mode,unsigned short BusyOutSel){

    DEB_MEMBER_FUNCT();
    DEB_TRACE() << "********** Inside of Camera::SetExposureParameters ***********";

    int ret;
    stringstream cmd;

    cmd.str(string());
    cmd <<  "ExposeParam " << NumImages << " " << Texp  << " " << Tovf  << " " << trigger_mode  << " " << BusyOutSel;
    m_xpad->sendWait(cmd.str(), ret);

    if(!ret)
        DEB_TRACE() << "Set of exposure parameters SUCCESFULL";
    else
        throw LIMA_HW_EXC(Error, "Setting exposure parameters FAILED!");

    DEB_TRACE() << "********** Outside of Camera::SetExposureParameters ***********";

    return ret;

}

int Camera::calibrationOTN(unsigned short OTNConfiguration){

    DEB_MEMBER_FUNCT();
    DEB_TRACE() << "********** Inside of Camera::calibrationOTN ***********";

    int ret;
    stringstream cmd;

    cmd <<  "CalibrationOTN " << OTNConfiguration;
    m_xpad->sendWait(cmd.str(), ret);

    if(!ret)
        DEB_TRACE() << "Calibration Over The Noise was SUCCESFULL ";
    else
        throw LIMA_HW_EXC(Error, "Calibration Over The Noise FAILED!");

    DEB_TRACE() << "********** Outside of Camera::calibrationOTN ***********";

    return ret;

}

int Camera::resetModules(){

    DEB_MEMBER_FUNCT();
    DEB_TRACE() << "********** Inside of Camera::calibrationOTN ***********";

    int ret;
    stringstream cmd;

    cmd <<  "ResetModules ";
    m_xpad->sendWait(cmd.str(), ret);

    if(!ret)
        DEB_TRACE() << "Modules were reset SUCCESFULLY ";
    else
        throw LIMA_HW_EXC(Error, "Resetting modules FAILED!");

    DEB_TRACE() << "********** Outside of Camera::calibrationOTN ***********";

    return ret;

}

void Camera::exit(){

    DEB_MEMBER_FUNCT();
    DEB_TRACE() << "********** Inside of Camera::exit ***********";
    stringstream cmd;

    cmd << "Exit";
    m_xpad->sendNoWait(cmd.str());

    DEB_TRACE() << "********** Outside of Camera::exit ***********";
}













