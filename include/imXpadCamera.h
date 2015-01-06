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
/*
 * XpadCamera.h
 * Created on: August 01, 2013
 * Author: Hector PEREZ PONCE
 */
#ifndef XPADCAMERA_H_
#define XPADCAMERA_H_

#include <stdlib.h>
#include <limits>
#include <stdarg.h>
#include <strings.h>
#include "lima/HwMaxImageSizeCallback.h"
#include "lima/HwBufferMgr.h"
#include "lima/HwInterface.h"
#include "imXpadInterface.h"
#include <ostream>
#include "lima/Debug.h"
#include "imXpadClient.h"

namespace lima {
namespace imXpad {

const int xPixelSize = 1;
const int yPixelSize = 1;

class BufferCtrlObj;

/*******************************************************************
 * \class Camera
 * \brief object controlling the Xpad camera
 *******************************************************************/
class Camera {
    DEB_CLASS_NAMESPC(DebModCamera, "Camera", "Xpad");

public:
    struct XpadStatus {
    public:
        enum XpadState {
            Idle, ///< The detector is not acquiringing data
            Test, ///< The detector is performing a digital test
            Resetting, ///< The detector is resetting.
            Running ///< The detector is acquiring data.
        };
        XpadState state;
        int frame_num; ///< The current frame number, within a group, being acquired, only valid when not {@link #Idle}
        int completed_frames; ///< The number of frames completed, only valid when not {@link #Idle}
    };

    struct XpadDigitalTest{
        enum DigitalTest {
            Flat, ///< Test using a flat value all over the detector
            Strips, ///< Test using a strips all over the detector
            Gradient ///< Test using a gradient values all over the detector
        };
    };

    struct Calibration{
        enum OTN{
            Slow,
            Medium,
            Fast
        };
    };

    Camera(std::string hostname, int port, std::string xpad_model);
    ~Camera();

    int init();
    void reset();
    void prepareAcq();
    void startAcq();
    void stopAcq();

    // -- detector info object
    void getImageType(ImageType& type);
    void setImageType(ImageType type);
    void getDetectorType(std::string& type);
    void getDetectorModel(std::string& model);
    void getDetectorImageSize(Size& size);
    void getPixelSize(double& size_x, double& size_y);

    // -- Buffer control object
    HwBufferCtrlObj* getBufferCtrlObj();
    void setNbFrames(int nb_frames);
    void getNbFrames(int& nb_frames);
    void readFrame(void *ptr, int frame_nb);
    void readFrameExpose(void *ptr, int frame_nb);
    int getNbHwAcquiredFrames();

    //-- Synch control object
    void setTrigMode(TrigMode mode);
    void getTrigMode(TrigMode& mode);
    void setExpTime(double exp_time);
    void getExpTime(double& exp_time);
    void setLatTime(double lat_time);
    void getLatTime(double& lat_time);

    //-- Status
    void getStatus(XpadStatus& status,bool = false);
    bool isAcqRunning() const;

    //---------------------------------------------------------------
    //- XPAD Stuff
    //! Get list of USB connected devices
    std::string getUSBDeviceList(void);

    //! Connect to a selected QuickUSB device
    int setUSBDevice(unsigned short module);

    //! Define the Detecto Model
    int defineDetectorModel(unsigned short model);

    //! Check if detector is Ready to work
    int askReady();

    //! Perform a Digital Test
    int digitalTest(unsigned short DT_value, unsigned short mode);

    //! Read global configuration values from file and load them in the detector
    int loadConfigGFromFile(char *fpath);

    //! Save global configuration values from detector to file
    int saveConfigGToFile(char *fpath);

    //! Send value to register for all chips in global configuration
    int loadConfigG(unsigned short reg, unsigned short value);

    //! Read values from register and from all chips in global configuration
    int readConfigG(unsigned short reg, void *values);

    //! Load default values for global configuration
    int loadDefaultConfigGValues();

    //! Increment of one unit of the global ITHL register
    int ITHLIncrease();

    //! Decrement of one unit in the global ITHL register
    int ITHLDecrease();

    //!	Load of flat config of value: flat_value (on each pixel)
    int loadFlatConfigL(unsigned short flat_value);

    //! Read local configuration values from file and save it in detector
    int loadConfigLFromFile(char *fpath);

    //! Save local configuration values from detector to file
    int saveConfigLToFile(char *fpath);

    //! Set the exposure parameters
    int setExposureParameters(unsigned int NumImage, double Texp,double Tovf,unsigned short trigger_mode,unsigned short BusyOutSel);

    //! Perform a Calibration over the noise
    int calibrationOTN(unsigned short OTNCalibration);

    //! Reset the detector modules
    int resetModules();

    //! Indicate to Server to liberate thread
    void exit();

private:

    // Xpad specific

    /*     TYPE OF SYTEM     */
#define XPAD_S10                    0
#define XPAD_C10                    1
#define XPAD_A10                    2
#define XPAD_S70                    3
#define XPAD_S70C                   4
#define XPAD_S140                   5
#define XPAD_S340                   6
#define XPAD_S540                   7
#define XPAD_S540V                  8
#define XPAD_S1400                  9

   /*     GLOBAL REGISTERS     */
#define AMPTP                       31
#define IMFP                        59
#define IOTA                        60
#define IPRE                        61
#define ITHL                        62
#define ITUNE                       63
#define IBUFF                       64

#define IMG_LINE	120
#define IMG_COLUMN 	80


    enum DATA_TYPE {IMG, CONFIG};
    enum IMG_TYPE  {B2,B4};

    XpadClient              *m_xpad;

    //---------------------------------
    //- LIMA stuff
    std::string             m_hostname;
    int                     m_port;
    std::string             m_configName;
    std::string             m_sysName;
    int                     m_nb_frames;
    int                     m_acq_frame_nb;

    mutable Cond            m_cond;
    bool                    m_quit;
    bool                    m_wait_flag;
    bool                    m_thread_running;

    class                   AcqThread;
    AcqThread               *m_acq_thread;

    //---------------------------------
    //- XPAD stuff
    unsigned short	        m_modules_mask;
    unsigned short          m_chip_mask;
    unsigned short          m_xpad_model;
    unsigned short          m_xpad_format;
    Size                    m_image_size;
    IMG_TYPE                m_pixel_depth;    
    unsigned int            m_xpad_trigger_mode;
    unsigned int            m_exp_time_usec;
    int				        m_module_number;
    int                     m_chip_number;




    // Buffer control object
    SoftBufferCtrlObj m_bufferCtrlObj;
};

} // namespace imXpad
} // namespace lima

#endif /* XPADCAMERA_H_ */
