//###########################################################################
// This file is part of LImA, a Library for Image Acquisition
//
// Copyright (C) : 2009-2015
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
#include "imXpadConfig.h"
#include "imXpadCamera.h"
using namespace lima;
using namespace lima::imXpad;

Config::Config(Camera& cam) : 
  m_cam(cam)
{
}

Config::~Config()
{
}

void Config::store(Setting& setting)
{
  setting.set("geometrical_correction",m_cam.getGeometricalCorrectionFlag());
  setting.set("flat_field_correction",m_cam.getFlatFieldCorrectionFlag());
  setting.set("dead_noisy_pixel_correction",m_cam.getDeadNoisyPixelCorrectionFlag());
}

void Config::restore(const Setting& setting)
{
  bool geom_flag;
  if(setting.get("geometrical_correction",geom_flag))
    m_cam.setGeometricalCorrectionFlag(geom_flag);

  bool flatfield_flag;
  if(setting.get("flat_field_correction",flatfield_flag))
    m_cam.setFlatFieldCorrectionFlag(flatfield_flag);

  bool dead_noisy_flag;
  if(setting.get("dead_noisy_pixel_correction",dead_noisy_flag))
    m_cam.setDeadNoisyPixelCorrectionFlag(dead_noisy_flag);
}
