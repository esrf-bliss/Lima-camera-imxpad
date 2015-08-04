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
// XpadInterface.h
// Created on: August 01, 2013
// Author: Hector PEREZ PONCE

#ifndef XPADINTERFACE_H_
#define XPADINTERFACE_H_

#include "lima/HwInterface.h"
#include <sys/time.h>

namespace lima {
namespace imXpad {

class Interface;
class Camera;

/*******************************************************************
 * \class DetInfoCtrlObj
 * \brief Control object providing Xpad detector info interface
 *******************************************************************/

class DetInfoCtrlObj: public HwDetInfoCtrlObj {

DEB_CLASS_NAMESPC(DebModCamera, "DetInfoCtrlObj","Xpad");

public:
	DetInfoCtrlObj(Camera& cam);
	virtual ~DetInfoCtrlObj();

	virtual void getMaxImageSize(Size& max_image_size);
    virtual void getDetectorImageSize(Size& det_image_size);

	virtual void getDefImageType(ImageType& def_image_type);
	virtual void getCurrImageType(ImageType& curr_image_type);
	virtual void setCurrImageType(ImageType curr_image_type);

	virtual void getPixelSize(double& x_size, double &y_size);
	virtual void getDetectorType(std::string& det_type);
	virtual void getDetectorModel(std::string& det_model);

	virtual void registerMaxImageSizeCallback(HwMaxImageSizeCallback& cb);
	virtual void unregisterMaxImageSizeCallback(HwMaxImageSizeCallback& cb);

private:
	Camera& m_cam;
};

/*******************************************************************
 * \class BufferCtrlObj
 * \brief Control object providing Xpad buffering interface
 *******************************************************************/
class BufferCtrlObj : public HwBufferCtrlObj
{
    DEB_CLASS_NAMESPC(DebModCamera, "BufferCtrlObj", "Xpad");

 public:
    BufferCtrlObj(Camera& simu);
    virtual ~BufferCtrlObj();

    virtual void setFrameDim(const FrameDim& frame_dim);
    virtual void getFrameDim(      FrameDim& frame_dim);

    virtual void setNbBuffers(int  nb_buffers);
    virtual void getNbBuffers(int& nb_buffers);

    virtual void setNbConcatFrames(int  nb_concat_frames);
    virtual void getNbConcatFrames(int& nb_concat_frames);

    virtual void getMaxNbBuffers(int& max_nb_buffers);

    virtual void *getBufferPtr(int buffer_nb, int concat_frame_nb = 0);
    virtual void *getFramePtr(int acq_frame_nb);

    virtual void getStartTimestamp(Timestamp& start_ts);
    virtual void getFrameInfo(int acq_frame_nb, HwFrameInfoType& info);

    virtual void registerFrameCallback(HwFrameCallback& frame_cb);
    virtual void unregisterFrameCallback(HwFrameCallback& frame_cb);

 private:
    BufferCtrlMgr& m_buffer_mgr;
};

/*******************************************************************
 * \class SyncCtrlObj
 * \brief Control object providing Xpad synchronization interface
 *******************************************************************/

class SyncCtrlObj: public HwSyncCtrlObj {
DEB_CLASS_NAMESPC(DebModCamera,"SyncCtrlObj","Xpad");

public:
	SyncCtrlObj(Camera& cam); //, BufferCtrlObj& buffer);
	virtual ~SyncCtrlObj();

	virtual bool checkTrigMode(TrigMode trig_mode);
	virtual void setTrigMode(TrigMode trig_mode);
	virtual void getTrigMode(TrigMode& trig_mode);

	virtual void setExpTime(double exp_time);
	virtual void getExpTime(double& exp_time);

    virtual void setLatTime(double lat_time);
    virtual void getLatTime(double& lat_time);

	virtual void setNbHwFrames(int nb_frames);
	virtual void getNbHwFrames(int& nb_frames);

	virtual void getValidRanges(ValidRangesType& valid_ranges);

private:
	Camera& m_cam;
};

/*******************************************************************
 * \class Interface
 * \brief Xpad hardware interface
 *******************************************************************/

class Interface: public HwInterface {
DEB_CLASS_NAMESPC(DebModCamera, "Interface", "Xpad");

public:
	Interface(Camera& cam);
	virtual ~Interface();

	virtual void getCapList(CapList&) const;
	virtual void reset(ResetLevel reset_level);
	virtual void prepareAcq();
	virtual void startAcq();
	virtual void stopAcq();
	virtual void getStatus(StatusType& status);
	virtual int getNbHwAcquiredFrames();

private:
	Camera& m_cam;
	CapList m_cap_list;
	DetInfoCtrlObj m_det_info;
    HwBufferCtrlObj*  m_bufferCtrlObj;
    SyncCtrlObj m_sync;
};

} // namespace imXpad
} // namespace lima

#endif /* XPADINTERFACE_H_ */
