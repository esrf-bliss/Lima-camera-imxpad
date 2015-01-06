//###########################################################################
// This file is part of LImA, a Library for Image Acquisition
//
// Copyright (C) : 2009-2013
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
#include "lima/HwInterface.h"
#include "lima/CtControl.h"
#include "lima/CtAccumulation.h"
#include "lima/CtAcquisition.h"
#include "lima/CtSaving.h"
#include "lima/CtShutter.h"
#include "lima/Constants.h"

#include "imXpadCamera.h"
#include "imXpadInterface.h"
#include "lima/Debug.h"
//#include "Exception.h"
#include <iostream>
#include <unistd.h>
#include <sys/stat.h>

using namespace std;
using namespace lima;
using namespace lima::imXpad;

DEB_GLOBAL(DebModTest);

int main(int argc, char *argv[])
{
    DEB_GLOBAL_FUNCT();
    //	DebParams::setModuleFlags(DebParams::AllFlags);
    //	DebParams::setTypeFlags(DebParams::AllFlags);
    DebParams::setTypeFlags(lima::DebTypeTrace);
    //	DebParams::setFormatFlags(DebParams::AllFlags);

    Camera *m_camera;
    Interface *m_interface;
    CtControl* m_control;

    //Xpad configuration properties
    string hostname = "127.0.0.1";
    string xpad_model = "XPAD_S70C";

    hostname = argv[1];
    int port = atoi(argv[2]);

    try {

        unsigned short *ret = new unsigned short[8];
        Camera::XpadStatus status;
        //int nframes = 1;

        //MUST BE SET BEFORE CREATING AN INTERFACE
        m_camera = new Camera(hostname, port, xpad_model);
        m_camera->setImageType(Bpp32);

        //CREATING AN INTERFACE AND A CONTROL
        m_interface = new Interface(*m_camera);
        m_control = new CtControl(m_interface);

        if(!m_camera->init()){

            //****DIGITAL TEST
            //if(!m_camera->digitalTest(40,Camera::XpadDigitalTest::Strips)){
/*
            int frame_number = 0;

            unsigned int val;
            unsigned int *image = new unsigned int[67200];

            //Command to recover Digital Test image
            m_camera->readFrame(image, frame_number);

                //Saving Digital Test image to disk
                mkdir("./Images",S_IRWXU |  S_IRWXG |  S_IRWXO);
                ofstream file("./Images/DigitalTest.raw", ios::out|ios::binary);
                if (file.is_open()){
                    for (int i=0;i<120;i++) {
                        for (int j=0;j<560;j++){
                            val = image[i*560+j];
                            file.write((char *)&val, sizeof(unsigned int));
                        }
                    }
                    file.close();
                }
            //}
*/
/*            //****GLOBAL CONFIGURATION FONCTIONS
            m_camera->loadConfigG(IMFP,0);
            m_camera->loadConfigG(ITHL,0);
            m_camera->readConfigG(ITUNE,ret);

            cout << "Global Configuration readed with SUCCESS" << endl << "Register = "\
                 << ret[0] << " Values: " << ret[1] << " " << ret[2] << " " << ret[3] << " "\
                 << ret[4] << " " << ret[5] << " " << ret[6] << " " << ret[7];

            m_camera->loadDefaultConfigGValues();

            m_camera->ITHLIncrease();
            m_camera->ITHLDecrease();
            m_camera->loadConfigGFromFile("./Calibration/ConfigGlobalSlow.cfg");

            m_camera->saveConfigGToFile("./ConfigGlobalTest.cfg");

            //****LOCAL CONFIGURATION FONCTIONS
*/          //m_camera->loadFlatConfigL(30);
            //m_camera->loadConfigLFromFile("./Calibration/ConfigLocalSlow.cfl");
            //m_camera->saveConfigLToFile("./ConfigLocalTest.clf");

/*
            //****CALIBRATION OVER THE NOISE
            mkdir("./Calibration",S_IRWXU |  S_IRWXG |  S_IRWXO);
            m_camera->calibrationOTN(Camera::Calibration::Slow);
            m_camera->saveConfigLToFile("./Calibration/ConfigLocalSlow.cfl");
            m_camera->saveConfigGToFile("./Calibration/ConfigGlobalSlow.cfg");
*/

            CtSaving* saving = m_control->saving();
            saving->setDirectory("./Images");
            saving->setFormat(CtSaving::EDF);
            saving->setPrefix("id24_");
            saving->setSuffix(".edf");
            saving->setSavingMode(CtSaving::AutoFrame);
            //saving->setSavingMode(CtSaving::Manual);
            saving->setOverwritePolicy(CtSaving::Overwrite);

            int nframes = 1;
            m_camera->setTrigMode(lima::IntTrig);
            m_control->acquisition()->setAcqExpoTime(1);
            m_control->acquisition()->setAcqNbFrames(nframes);
            m_control->prepareAcq();
            m_control->startAcq();
            m_camera->stopAcq();

            //m_camera->resetModules();
            m_camera->exit();
        }

    } catch (Exception e) {
        DEB_ERROR() << "LIMA Exception: " << e;
    } catch (...) {
        DEB_ERROR() << "Unkown exception!";
    }
    return 0;
}
