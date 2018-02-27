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
#include "lima/CtAcquisition.h"
#include "lima/CtSaving.h"

#include "../include/imXpadCamera.h"

using namespace std;
using namespace lima;
using namespace lima::imXpad;

DEB_GLOBAL(DebModTest);

int main(int argc, char *argv[])
{
    DEB_GLOBAL_FUNCT();
    DebParams::setTypeFlags(lima::DebTypeTrace);
    //DebParams::setFormatFlags(DebParams::AllFlags);

    Camera 		*cam;
    Interface 	*HWI;
    CtControl   *CT;

    string hostname = argv[1];
    int port = atoi(argv[2]);

    cam = new Camera(hostname, port);
    HWI = new Interface(*cam);
    CT = new CtControl(HWI);
    CtSaving *CTs = CT->saving();
    CtAcquisition *CTa = CT->acquisition();

    CTs->setDirectory("./Images");
    CTs->setFormat(CtSaving::EDF);
    CTs->setPrefix("id24_");
    CTs->setSuffix(".edf");
    CTs->setSavingMode(CtSaving::AutoFrame);
    //CTs->setSavingMode(CtSaving::Manual);
    CTs->setOverwritePolicy(CtSaving::Overwrite);

    CTa->setAcqExpoTime(1);
    CTa->setAcqNbFrames(1);

    cam->calibrationBEAM(100,50,0);
    cam->waitAcqEnd();
    cam->calibrationBEAM(100,50,0);
    cam->waitAcqEnd();

    /*for (int i=0; i<10; i++){
		cout << i << endl;
        CT->prepareAcq();
        CT->startAcq();
        sleep(1);
        CT->stopAcq();
    }*/

    return 0;
}
