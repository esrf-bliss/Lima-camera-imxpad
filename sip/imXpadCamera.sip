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
 * imXpadCamera.h
 * Created on: August 01, 2013
 * Author: Hector PEREZ PONCE
 */
namespace imXpad {

/*******************************************************************
 * \class Camera
 * \brief object controlling the imXpad camera
 *******************************************************************/
class Camera {
%TypeHeaderCode
#include <imXpadCamera.h>
%End

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
    void getImageType(ImageType& type /Out/);
    void setImageType(ImageType type);
    void getDetectorType(std::string& type /Out/);
    void getDetectorModel(std::string& model /Out/);
    void getDetectorImageSize(Size& size /Out/);
    void getPixelSize(double& size_x /Out/, double& size_y /Out/);

    // -- Buffer control object
	/*    HwBufferCtrlObj* getBufferCtrlObj();
    void setNbFrames(int nb_frames);
    void getNbFrames(int& nb_frames);
    void readFrame(void *ptr, int frame_nb);
    int getNbHwAcquiredFrames();
*/	

    //-- Synch control object
/*    void setTrigMode(TrigMode mode);
    void getTrigMode(TrigMode& mode);
    void setExpTime(double exp_time);
    void getExpTime(double& exp_time);
    void setLatTime(double lat_time);
    void getLatTime(double& lat_time);
*/
    //-- Status
   // void getStatus(XpadStatus& status);
   // bool isAcqRunning() const;

    //---------------------------------------------------------------
    //- XPAD Stuff
    //! Get list of USB connected devices
    std::string getUSBDeviceList();

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

    //! Read local configuration values from file and save it in detector's SRAM
    //int loadConfigLFromFileToSRAM(char *fpath);

    //! Read local configuration values from detector's SRAM to detector
    //int loadConfigLSRAMToDetector();

    //! Read local configuration values from file and save it in detector
    int loadConfigLFromFile(char *fpath);

    //! Save local configuration values from detector to file
    int saveConfigLToFile(char *fpath);

    //! Set the exposure parameters
    //int setExposureParameters(double Texp,double Tovf,unsigned short trigger_mode,unsigned short BusyOutSel);
    int setExposureParameters(unsigned int NumImage, double Texp,double Tovf,unsigned short trigger_mode,unsigned short BusyOutSel);

    //! Perform a Calibration over the noise
    int calibrationOTN(unsigned short OTNCalibration);

    //! Reset the detector modules
    int resetModules();

    //! Indicate to Server to liberate thread
    void exit();


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
    void calibrateOTNSlow (std::string path);
    //! Calibrate over the noise Medium and save dacl and configg files in path
    void calibrateOTNMedium (std::string path);
    //! Calibrate over the noise High and save dacl and configg files in path
    void calibrateOTNHigh (std::string path);
    //! upload the calibration (dacl + config) that is stored in path
    void uploadCalibration(std::string path);
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



};

}; // namespace imXpad

