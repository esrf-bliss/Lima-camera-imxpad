ImXPAD Tango device
=======================

This is the reference documentation of the ImXPAD Tango device.

you can also find some useful information about the camera models/prerequisite/installation/configuration/compilation in the :ref:`ImXPAD camera plugin <camera-imxpad>` section.


Properties
----------

================= =============== =============== =========================================================================
Property name	  Mandatory	  Default value	  Description
================= =============== =============== =========================================================================
camera_ip_address Yes		  N/A		  IP address
port              No              3456            socket port number
model             No              XPAD_S70        detector model
usb_device_id     No              N/A             reserved, do not use
config_path       Yes             N/A             The configuration directory path (see loadConfig command)
================= =============== =============== =========================================================================

Attributes
----------

This camera device has no attribute.

Commands
--------

=======================	=============== =======================	===========================================
Command name		Arg. in		Arg. out		Description
=======================	=============== =======================	===========================================
Init			DevVoid 	DevVoid			Do not use
State			DevVoid		DevLong			Return the device state
Status			DevVoid		DevString		Return the device state as a string
getAttrStringValueList	DevString:	DevVarStringArray:	Return the authorized string value list for
			Attribute name	String value list	a given attribute name
loadConfig              DevString       DevVoid                 the config file prefix, the property 
                                                                config_path is mandatory
=======================	=============== =======================	===========================================



