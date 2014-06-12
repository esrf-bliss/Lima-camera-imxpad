/*
 * XpadDetInfoCtrlObj.cpp
 *
 * Created on: Aout 01, 2013
 * Author: Hector PEREZ PONCE
 */

#include <cstdlib>
#include "imXpadInterface.h"
#include "imXpadCamera.h"

using namespace lima;
using namespace lima::imXpad;

DetInfoCtrlObj::DetInfoCtrlObj(Camera& cam) : m_cam(cam) {
	DEB_CONSTRUCTOR();
}

DetInfoCtrlObj::~DetInfoCtrlObj() {
	DEB_DESTRUCTOR();
}

void DetInfoCtrlObj::getMaxImageSize(Size& size) {
	DEB_MEMBER_FUNCT();
    m_cam.getDetectorImageSize(size);
}

void DetInfoCtrlObj::getDetectorImageSize(Size& image_size) {
    DEB_MEMBER_FUNCT();
   //getMaxImageSize(image_size);
    m_cam.getDetectorImageSize(image_size);

}

void DetInfoCtrlObj::getDefImageType(ImageType& image_type) {
	DEB_MEMBER_FUNCT();
	m_cam.getImageType(image_type);
}

void DetInfoCtrlObj::getCurrImageType(ImageType& image_type) {
	DEB_MEMBER_FUNCT();
	m_cam.getImageType(image_type);
}

void DetInfoCtrlObj::setCurrImageType(ImageType image_type) {
	DEB_MEMBER_FUNCT();
	m_cam.setImageType(image_type);
}

void DetInfoCtrlObj::getPixelSize(double& xsize, double& ysize) {
	DEB_MEMBER_FUNCT();
	m_cam.getPixelSize(xsize, ysize);
}

void DetInfoCtrlObj::getDetectorType(std::string& type) {
	DEB_MEMBER_FUNCT();
	m_cam.getDetectorType(type);
}

void DetInfoCtrlObj::getDetectorModel(std::string& model) {
	DEB_MEMBER_FUNCT();
	m_cam.getDetectorModel(model);
}

void DetInfoCtrlObj::registerMaxImageSizeCallback(HwMaxImageSizeCallback& cb) {
	DEB_MEMBER_FUNCT();
//	m_cam->registerMaxImageSizeCallback(cb);
}

void DetInfoCtrlObj::unregisterMaxImageSizeCallback(HwMaxImageSizeCallback& cb) {
	DEB_MEMBER_FUNCT();
//	m_cam->unregisterMaxImageSizeCallback(cb);
}

