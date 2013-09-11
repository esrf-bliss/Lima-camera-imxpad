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
#include "HwMaxImageSizeCallback.h"
#include "HwBufferMgr.h"
#include "HwInterface.h"
#include "XpadInterface.h"
#include <ostream>
#include "Debug.h"
#include "XpadClient.h"

using namespace std;

namespace lima {
namespace Xpad {

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
        int group_num; ///< The current group number being acquired, only valid when not {@link #Idle}
        int frame_num; ///< The current frame number, within a group, being acquired, only valid when not {@link #Idle}
        int scan_num; ///< The current scan number, within a frame, being acquired, only valid when not {@link #Idle}
        int cycle; ///< Not supported yet
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

    Camera(string hostname, int port, string xpad_model);
    ~Camera();

    void init();
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
    int getNbHwAcquiredFrames();

    //-- Synch control object
    void setTrigMode(TrigMode mode);
    void getTrigMode(TrigMode& mode);
    void setExpTime(double exp_time);
    void getExpTime(double& exp_time);
    void setLatTime(double lat_time);
    void getLatTime(double& lat_time);

    //-- Status
    void getStatus(XpadStatus& status);
    bool isAcqRunning() const;

    //---------------------------------------------------------------
    //- XPAD Stuff
    //! Get list of USB connected devices
    void getUSBDeviceList(void);

    //! Connect to a selected QuickUSB device
    void setUSBDevice(unsigned short module);

    //! Define the Detecto Model
    void DefineDetectorModel(unsigned short model);

    //! Check if detector is Ready to work
    void askReady();

    //! Perform a Digital Test
    void digitalTest(unsigned short DT_value, unsigned short mode);

    //! Read global configuration values from file and load them in the detector
    void loadConfigGFromFile(char *fpath);

    //! Save global configuration values from detector to file
    void saveConfigGToFile(char *fpath);

    //! Send value to register for all chips in global configuration
    void loadConfigG(unsigned short reg, unsigned short value);

    //! Read values from register and from all chips in global configuration
    void readConfigG(unsigned short reg, void *values);

    //! Increment of one unit of the global ITHL register
    void ITHLIncrease();

    //! Decrement of one unit in the global ITHL register
    void ITHLDecrease();

    //!	Load of flat config of value: flat_value (on each pixel)
    void loadFlatConfigL(unsigned short flat_value);

    //! Read local configuration values from file and save it in detector's SRAM
    void loadConfigLFromFileToSRAM(char *fpath);

    //! Read local configuration values from detector's SRAM to detector
    void loadConfigLSRAMToDetector();

    //! Save local configuration values from detector to file
    void saveConfigLToFile(char *fpath);

    //! Set the exposure parameters
    void setExposureParameters(double Texp,double Tovf,unsigned short trigger_mode,unsigned short BusyOutSel);

    //! Perform a Calibration over the noise
    void calibrationOTN(unsigned short OTNCalibration);


    //! Set all the config G
    //void setAllConfigG(const vector<long>& allConfigG);
    //!	Set the Acquisition type between fast and slow
    //void setAcquisitionType(short acq_type);
    /*    //!	Load of flat config of value: flat_value (on each pixel)
    void loadFlatConfig(unsigned flat_value);
    //! Load all the config G
    void loadAllConfigG(unsigned long modNum, unsigned long chipId , unsigned long* config_values);
    //! Load a wanted config G with a wanted value
    void loadConfigG(const vector<unsigned long>& reg_and_value);
    //! Load a known value to the pixel counters
    void loadAutoTest(unsigned known_value);
    //! Save the config L (DACL) to XPAD RAM
    void saveConfigL(unsigned long modMask, unsigned long calibId, unsigned long chipId, unsigned long curRow,unsigned long* values);
    //! Save the config G to XPAD RAM
    void saveConfigG(unsigned long modMask, unsigned long calibId, unsigned long reg,unsigned long* values);
    //! Load the config to detector chips
    void loadConfig(unsigned long modMask, unsigned long calibId);
    //! Get the modules config (Local aka DACL)
    unsigned short*& getModConfig();
    //! Reset the detector
    void reset();
    //! Set the exposure parameters
    void setExposureParameters( unsigned Texp,unsigned Twait,unsigned Tinit,
                                unsigned Tshutter,unsigned Tovf,unsigned mode, unsigned n,unsigned p,
                                unsigned nbImages,unsigned BusyOutSel,unsigned formatIMG,unsigned postProc,
                                unsigned GP1,unsigned GP2,unsigned GP3,unsigned GP4);
    //! Calibrate over the noise Slow and save dacl and configg files in path
    void calibrateOTNSlow (string path);
    //! Calibrate over the noise Medium and save dacl and configg files in path
    void calibrateOTNMedium (string path);
    //! Calibrate over the noise High and save dacl and configg files in path
    void calibrateOTNHigh (string path);
    //! upload the calibration (dacl + config) that is stored in path
    void uploadCalibration(string path);
    //! upload the wait times between each images in case of a sequence of images (Twait from setExposureParameters should be 0)
    void uploadExpWaitTimes(unsigned long *pWaitTime, unsigned size);
    //! increment the ITHL
    void incrementITHL();
    //! decrement the ITHL
    void decrementITHL();
    //! set the specific parameters (deadTime,init time, shutter ...
    void setSpecificParameters( unsigned deadtime, unsigned init,
                                unsigned shutter, unsigned ovf,
                                unsigned n,       unsigned p,
                                unsigned GP1,     unsigned GP2,    unsigned GP3,      unsigned GP4);*/



private:
    int sendData_2B(int sockd, const void *vptr, size_t n);

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
    string                  m_hostname;
    int                     m_port;
    string                  m_configName;
    string                  m_sysName;
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
    Size                    m_image_size;
    IMG_TYPE                m_pixel_depth;
    unsigned int            m_xpad_format;
    unsigned int            m_xpad_trigger_mode;
    unsigned int            m_exp_time_usec;
    int				        m_module_number;
    int                     m_chip_number;




    // Buffer control object
    SoftBufferCtrlObj m_bufferCtrlObj;
};

} // namespace Xpad
} // namespace lima

#endif /* XPADCAMERA_H_ */
