############################################################################
# This file is part of LImA, a Library for Image Acquisition
#
# Copyright (C) : 2009-2011
# European Synchrotron Radiation Facility
# BP 220, Grenoble 38043
# FRANCE
#
# This is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This software is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, see <http://www.gnu.org/licenses/>.
############################################################################
#=============================================================================
#
# file :        imXPAD.py
#
# description : Python source for the imXPAD and its commands.
#                The class is derived from Device. It represents the
#                CORBA servant object which will be accessed from the
#                network. All commands which can be executed on the
#                Pilatus are implemented in this file.
#
# project :     TANGO Device Server
#
# copyleft :    European Synchrotron Radiation Facility
#               BP 220, Grenoble 38043
#               FRANCE
#
#=============================================================================
#         (c) - Bliss - ESRF
#=============================================================================
#
import PyTango
import os,glob
from Lima import Core
from Lima import imXpad as XpadAcq
from Lima.Server import AttrHelper


class imXPAD(PyTango.Device_4Impl):

    Core.DEB_CLASS(Core.DebModApplication, 'LimaCCDs')

#------------------------------------------------------------------
#    Device constructor
#------------------------------------------------------------------
    def __init__(self,*args) :
        PyTango.Device_4Impl.__init__(self,*args)

        self.init_device()

#------------------------------------------------------------------
#    Device destructor
#------------------------------------------------------------------
    def delete_device(self):
        pass
    
#------------------------------------------------------------------
#    Device initialization
#------------------------------------------------------------------
    @Core.DEB_MEMBER_FUNCT
    def init_device(self):
        self._config_name = None
        self._ITHL_offset = 0

        try:
            
            self.set_state(PyTango.DevState.ON)
            self.get_device_properties(self.get_device_class())
        
        except Exception as e:
            print ("Error in init_device Method")
            print (e)
            
        #Dictionaries with the Types. 
        self.__AcquisitionMode = {'STANDARD':XpadAcq.Camera.XpadAcquisitionMode.Standard,
                                  'COMPUTERBURST':XpadAcq.Camera.XpadAcquisitionMode.ComputerBurst,
                                  'DETECTORBURST':XpadAcq.Camera.XpadAcquisitionMode.DetectorBurst,
                                 # 'SingleBunch16bits':XpadAcq.Camera.XpadAcquisitionMode.SingleBunch16bits,
                                 # 'SingleBunch32bits':XpadAcq.Camera.XpadAcquisitionMode.SingleBunch32bits,
                                 # 'Stacking16bits':XpadAcq.Camera.XpadAcquisitionMode.Stacking16bits,
                                 # 'Stacking32bits':XpadAcq.Camera.XpadAcquisitionMode.Stacking32bits,
                                 }

        self.__OutputSignal = {'ExposureBusy' : XpadAcq.Camera.XpadOutputSignal.ExposureBusy,
                               'ShutterBusy' : XpadAcq.Camera.XpadOutputSignal.ShutterBusy,
                               'BusyUpdateOverflow' : XpadAcq.Camera.XpadOutputSignal.BusyUpdateOverflow,
                               'PixelCounterEnabled' : XpadAcq.Camera.XpadOutputSignal.PixelCounterEnabled,
                               'ExternalGate' : XpadAcq.Camera.XpadOutputSignal.ExternalGate,
                               'ExposureReadDone' : XpadAcq.Camera.XpadOutputSignal.ExposureReadDone,
                               'DataTransfer' : XpadAcq.Camera.XpadOutputSignal.DataTransfer,
                               'RAMReadyImageBusy' : XpadAcq.Camera.XpadOutputSignal.RAMReadyImageBusy,
                               'XPADToLocalDDR' : XpadAcq.Camera.XpadOutputSignal.XPADToLocalDDR,
                               'LocalDDRToPC' : XpadAcq.Camera.XpadOutputSignal.LocalDDRToPC}
         
        self.__ImageFileFormat = {'Ascii':XpadAcq.Camera.XpadImageFileFormat.Ascii,
                                  'Binary':XpadAcq.Camera.XpadImageFileFormat.Binary}
        
        ##Set DEFAULT configuration to store input values
        ##(there are no getters for these attributes) 

        # print("Setting values by Default")
        
        _imXPADCam.setAcquisitionMode(XpadAcq.Camera.XpadAcquisitionMode.Standard)

        self.__FlatFieldCorrectionFlag = {'ON' : True,
                                        'OFF': False}
        _imXPADCam.setFlatFieldCorrectionFlag(True)
        
        self.__GeometricalCorrectionFlag = {'ON' : True,
                                            'OFF': False}
        _imXPADCam.setGeometricalCorrectionFlag(True)

        self.__ImageTransferFlag =  {'ON' : True,
                                     'OFF': False}

        _imXPADCam.setImageFileFormat(XpadAcq.Camera.XpadImageFileFormat.Binary)

        _imXPADCam.setOutputSignalMode(XpadAcq.Camera.XpadOutputSignal.BusyUpdateOverflow)

        
    @Core.DEB_MEMBER_FUNCT
    def getAttrStringValueList(self, attr_name):
        if attr_name.startswith('config_name') :
            full_path = os.path.join(self.config_path,'*.cfg')
            return [os.path.splitext(os.path.basename(x))[0] for x in glob.glob(full_path)]
        else:
            return AttrHelper.get_attr_string_value_list(self, attr_name)

    @Core.DEB_MEMBER_FUNCT
    def loadConfig(self,config_prefix) :
        if config_prefix == self._config_name: return
        config_path = self.config_path        
        _imXPADCam.loadConfigGFromFile(os.path.join(config_path,'%s.cfg' % config_prefix))
        _imXPADCam.loadConfigLFromFile(os.path.join(config_path,'%s.cfl' % config_prefix))
        self._config_name = config_prefix
        self._ITHL_offset = 0

    @Core.DEB_MEMBER_FUNCT
    def saveConfig(self,config_prefix) :
        config_path = self.config_path
        print ('saveConfig',config_path,config_prefix)
        _imXPADCam.saveConfigGToFile(os.path.join(config_path,'%s.cfg' % config_prefix))
        _imXPADCam.saveConfigLToFile(os.path.join(config_path,'%s.cfl' % config_prefix))
        self._config_name = config_prefix
        self._ITHL_offset = 0

    def __getattr__(self,name) :
        return AttrHelper.get_attr_4u(self, name, _imXPADCam)
     
    def read_config_name(self,attr):
        config_name = self._config_name
        if config_name is None:
            config_name = "MEMORY"
        attr.set_value(config_name)

    def write_config_name(self,attr):
        config_name = attr.get_write_value()
        if config_name == "MEMORY": return
        self.loadConfig(config_name)

    def read_ITHL_offset(self,attr):
        attr.set_value(self._ITHL_offset)
        
    def write_ITHL_offset(self,attr):
        new_ithl = attr.get_write_value()
        prev_ithl = self._ITHL_offset
        diff = new_ithl - prev_ithl
        while(diff):
            if(diff > 0):
                _imXPADCam.ITHLIncrease()
                diff -= 1
                self._ITHL_offset += 1
            else:
                _imXPADCam.ITHLDecrease()
                diff += 1
                self._ITHL_offset -= 1

#==================================================================
#
#    imXPAD command methods
#
#==================================================================

    def calibrationOTNPulse(self, attr):
        print ("OTNpulseCalibration in %s", attr)
        _imXPADCam.calibrationOTNPulse(attr)
        self._config_name = None
        self._ITHL_offset = 0

    def calibrationOTN(self,attr):
        print ("OTNcalibration in %s", attr)
        _imXPADCam.calibrationOTN(attr)
        self._config_name = None
        self._ITHL_offset = 0

    def calibrationBEAM(self, values):
        print ("BeamCalibration in %s", values)
        time = int(values[0])
        ITHLmax = int(values[1])
        calibConfig = int(values[2])
        _imXPADCam.calibrationBEAM(time, ITHLmax, calibConfig)
        self._config_name = None
        self._ITHL_offset = 0

    def ITHLIncrease(self):
        print ("ITHLIncrease")
        _imXPADCam.ITHLIncrease()
        self._ITHL_offset += 1

    def ITHLDecrease(self):
        print ("ITHLDecrease")
        _imXPADCam.ITHLDecrease()
        self._ITHL_offset -= 1
    
    def askReady(self):
        print ("In askReady Command")
        val= _imXPADCam.askReady()
        return bool(val == 0)
    
    def getModuleMask(self):
        print ("Get module mask")
        try:
            val= _imXPADCam.getModuleMask()
        except Exception as e:
            print (e)
            raise e
        return val
    
    def abort(self):       
        print ("In abort")
        pass
    
    def resetModules(self):
        _imXPADCam.resetModules()
        
    def saveConfigLToFile(self,config_prefix) :
        print ("saveConfigLToFile in %s", config_prefix)
        config_path = self.config_path
        full_path = os.path.join(config_path,'%s.cfl' % config_prefix)
        _imXPADCam.saveConfigLToFile(full_path)  
        
    def loadConfigLFromFile(self,config_prefix) :
        print ("loadConfigLFromFile in %s", config_prefix)
        config_path = self.config_path
        full_path = os.path.join(config_path,'%s.cfl' % config_prefix)
        _imXPADCam.loadConfigLFromFile(full_path)
    
    def saveConfigGToFile(self,config_prefix) :
        print ("saveConfigGToFile in %s", config_prefix)
        config_path = self.config_path
        full_path = os.path.join(config_path,'%s.cfg' % config_prefix)
        _imXPADCam.saveConfigGToFile(full_path)
        
    def loadConfigGFromFile(self,config_prefix) :
        print ("loadConfigGFromFile %s", config_prefix)
        config_path = self.config_path
        full_path = os.path.join(config_path,'%s.cfg' % config_prefix)
        _imXPADCam.loadConfigGFromFile(full_path)
        
    def loadConfigG(self, config):
        print ("loadConfigG in %s", config)
        reg = long(config[0])
        value = long(config[1])
        val = _imXPADCam.loadConfigG(reg, value)
        return val
    
    def readConfigG (self, config):
        print ("readConfigG in %s", config)
        reg = long(config)
#    value = long(config[1])
        val = _imXPADCam.readConfigG(reg)
        return val
    
    def getUSBDeviceList(self):
        print ("getUSBDeviceList in")
        val = _imXPADCam.getUSBDeviceList()
        print (val)
        return val

    def xpadInit(self):
        print (" in xpadInit")
        _imXPADCam.xpadInit()
      
    def setUSBDevice(self, module):
        print ("setUSBDevice in %s", module)
        val =long(module)
        _imXPADCam.setUSBDevice(val)

    def defineDetectorModel(self, model):
        print(" in defineDetectorModel Method")
        val = model
        _imXPADCam.defineDetectorModel(val)

    def digitalTest(self, mode):
        print(" in digitalTest Method")
        val = mode
        _imXPADCam.digitalTest(val)

    def loadFlatConfigL(self, value):
        print("In loadFlatConfigL")
        val = value
        _imXPADCam.loadFlatConfigL(val)    

    def exit(self):
        _imXPADCam.exit()

    def loadDefaultConfigGValues(self):
        _imXPADCam.loadDefaultConfigGValues()
#==================================================================
#
#    imXPAD read/write attribute methods
#
#==================================================================       
   
#@see __getattr__        
      
#==================================================================
#
#    imXPADClass class definition
#
#==================================================================
class imXPADClass(PyTango.DeviceClass):

    class_property_list = {}

    device_property_list = {
        'cam_ip_address' :
        [PyTango.DevString,
         "Camera ip address",[]],
        'port' :
        [PyTango.DevLong,
         'ip port',[]],
        'config_path' :
        [PyTango.DevString,
         "Config path",['/tmp']]
        }
    
    
    cmd_list = {
        'getAttrStringValueList':
        [[PyTango.DevString, "Attribute name"],
         [PyTango.DevVarStringArray, "Authorized String value list"]],
                
       'getUSBDeviceList':
        [[PyTango.DevVoid, "getUSBDeviceList"],
         [PyTango.DevString,""]],
        
        'setUSBDevice':
        [[PyTango.DevLong, "set USB Device"],
         [PyTango.DevVoid,""]],         
                
        'defineDetectorModel':
        [[PyTango.DevLong, "set Detector Model"],
         [PyTango.DevVoid,""]],                         
                         
        'xpadInit':
        [[PyTango.DevVoid, ""],
         [PyTango.DevVoid,""]],           
                           
        'askReady':
        [[PyTango.DevVoid, "Ask the detector if ready"],
         [PyTango.DevBoolean,""]],
                
        'getModuleMask':
        [[PyTango.DevVoid, ""],
         [PyTango.DevULong,"Mask of the available modules"]], # Hexadecimal?
                
        'digitalTest':
        [[PyTango.DevLong, "digitalTest"],
         [PyTango.DevVoid,""]],
               
        'saveConfigGToFile':
        [[PyTango.DevString, "Save G Config file prefix"],
         [PyTango.DevVoid,""]],    
                 
        'saveConfigLToFile':
        [[PyTango.DevString, "Save L Config file prefix"],
         [PyTango.DevVoid,""]],                  

        'saveConfig':
        [[PyTango.DevString, "Save Config file prefix"],
         [PyTango.DevVoid,""]],                  
        
        'calibrationOTN':
        [[PyTango.DevLong, "OTN  Calibration"],
         [PyTango.DevVoid,""]],
                   
        'calibrationOTNPulse':
        [[PyTango.DevLong, "OTN Pulse Calibration "],
         [PyTango.DevVoid,""]],
                       
        'calibrationBEAM':
        [[PyTango.DevVarLongArray, "Beam Calibration"],
         [PyTango.DevVoid,""]],        
               
        'loadConfig':
        [[PyTango.DevVarStringArray, "Config file prefix"], 
         [PyTango.DevVoid,""]],
        
        'loadConfigG':
        [[PyTango.DevVarStringArray, "Config file prefix"], # input 2 parameters (reg,value)
         [PyTango.DevVoid,""]],
        
        'readConfigG':
        [[PyTango.DevVarLongArray, "readConfigG"],
         [PyTango.DevVoid,""]], # return integer array          
                            
        'loadDefaultConfigGValues':
        [[PyTango.DevVoid, "loadDefaultConfigGValues"],
         [PyTango.DevVoid,""]],
                    
        'loadFlatConfigL':
        [[PyTango.DevLong, "Load of flat config of value: flat_value (on each pixel)"],
         [PyTango.DevVoid,""]],
                     
        'loadConfigLFromFile':
        [[PyTango.DevString, "Load of config of File"],
         [PyTango.DevVoid,""]],
                
        'loadConfigGFromFile':
        [[PyTango.DevString, "Load of config of File"],
         [PyTango.DevVoid,""]],
                
        'exit':
        [[PyTango.DevVoid, ""],
         [PyTango.DevVoid,""]],

        'ITHLIncrease':
        [[PyTango.DevVoid, "Increment of one unit in the global ITHL register"],
         [PyTango.DevVoid,""]],                 

        'ITHLDecrease':
        [[PyTango.DevVoid, "Decrement of one unit in the global ITHL register"],
         [PyTango.DevVoid,""]],

            }

    attr_list = {                  
        "Acquisition_Mode":
        [[PyTango.DevString, 
         PyTango.SCALAR, 
         PyTango.READ_WRITE]],
                 
        "Output_Signal":
        [[PyTango.DevString, 
         PyTango.SCALAR, 
         PyTango.READ_WRITE]],
                 
        "Flat_Field_Correction_Flag":
        [[PyTango.DevString, 
         PyTango.SCALAR, 
         PyTango.READ_WRITE]],
                 
        "Image_Transfer_Flag":
        [[PyTango.DevString, 
         PyTango.SCALAR, 
         PyTango.READ_WRITE]],
        
        "Over_Flow_Time":
        [[PyTango.DevShort, 
         PyTango.SCALAR, 
         PyTango.READ_WRITE]],
                 
        "Geometrical_Correction_Flag":
        [[PyTango.DevString, 
         PyTango.SCALAR, 
         PyTango.READ_WRITE]],    
                          
        "Image_File_Format":
        [[PyTango.DevString, 
         PyTango.SCALAR, 
         PyTango.READ_WRITE]],

        "config_name":
        [[PyTango.DevString,
          PyTango.SCALAR,
          PyTango.READ_WRITE]],

        "ITHL_offset":
        [[PyTango.DevLong, 
         PyTango.SCALAR, 
         PyTango.READ_WRITE]],
        }

    def __init__(self,name) :
        PyTango.DeviceClass.__init__(self,name)
        self.set_type(name)

#----------------------------------------------------------------------------
# Plugins
#----------------------------------------------------------------------------
_imXPADCam = None
_imXPADInterface = None

def get_control(cam_ip_address = "localhost",port=3456,**keys) :
    print (cam_ip_address,port)
    global _imXPADCam
    global _imXPADInterface
    port = int(port)
    print ("Getting control for REBIRX: %s / %s" % (cam_ip_address, port))
    if _imXPADCam is None:
        _imXPADCam = XpadAcq.Camera(cam_ip_address,port)
        _imXPADInterface = XpadAcq.Interface(_imXPADCam)
    return Core.CtControl(_imXPADInterface)

def get_tango_specific_class_n_device():
    return imXPADClass,imXPAD
