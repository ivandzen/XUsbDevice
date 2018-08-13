#ifndef USBDESCRIPTORS_H
#define USBDESCRIPTORS_H
#include <core/common/arrayref.h>
#include <string.h>

#define __packed __attribute__((packed))

typedef struct __packed
{
    uint8_t   bmRequest;
    uint8_t   bRequest;
    uint16_t  wValue;
    uint16_t  wIndex;
    uint16_t  wLength;
}
UsbSetupRequest;

typedef enum
{
	UsbFeature_EP_HALT         = 0,
	UsbFeature_REMOTE_WAKEUP   = 1,
	UsbFeature_TEST_MODE       = 2
}
UsbFeature;

typedef enum
{
    UsbDescType_Device        	= 0x01,
    UsbDescType_Configuration 	= 0x02,
    UsbDescType_String        	= 0x03,
    UsbDescType_Interface     	= 0x04,
    UsbDescType_Endpoint      	= 0x05,
	UsbDescType_DeviceQualifier = 0x06,
	UsbDescType_OtherSpeedConfiguration = 0x07,
	UsbDescType_BOS 			= 0x0F
}
UsbDescType;

typedef enum
{
    UsbEPDir_Out    = 0x00,
    UsbEPDir_In     = 0x80
}
UsbEPDirection;

typedef enum
{
    UsbEPType_Control       = 0x00,
    UsbEPType_Isochronous   = 0x01,
    UsbEPType_Bulk          = 0x02,
    UsbEPType_Interrupt     = 0x03
}
UsbEPType;

#define UsbEPTypeMask 0x03

typedef enum
{
    UsbEPSync_NoSync    = 0x00,
    UsbEPSync_Async     = 0x04,
    UsbEPSync_Adaptive  = 0x08,
    UsbEPSync_Sync      = 0x0C
}
UsbEPSync;

typedef enum
{
    UsbEPUsage_Data     = 0x00,
    UsbEPUsage_Feedback = 0x10,
    UsbEPUsage_Explicit = 0x20,
}
UsbEPUsage;

typedef enum
{
    REQ_RECIPIENT_DEVICE                       = 0x00,
    REQ_RECIPIENT_INTERFACE                    = 0x01,
    REQ_RECIPIENT_ENDPOINT                     = 0x02,
    REQ_RECIPIENT_MASK                         = 0x03
}
UsbReqRecipient;

typedef enum
{
    REQ_TYPE_STANDARD                          = 0x00,
    REQ_TYPE_CLASS                             = 0x20,
    REQ_TYPE_VENDOR                            = 0x40,
    REQ_TYPE_MASK                              = 0x60
}
UsbReqType;

typedef enum
{
    REQ_GET_STATUS                             = 0x00,
    REQ_CLEAR_FEATURE                          = 0x01,
    REQ_SET_FEATURE                            = 0x03,
    REQ_SET_ADDRESS                            = 0x05,
    REQ_GET_DESCRIPTOR                         = 0x06,
    REQ_SET_DESCRIPTOR                         = 0x07,
    REQ_GET_CONFIGURATION                      = 0x08,
    REQ_SET_CONFIGURATION                      = 0x09,
    REQ_GET_INTERFACE                          = 0x0A,
    REQ_SET_INTERFACE                          = 0x0B,
    REQ_SYNCH_FRAME                            = 0x0C
}
UsbRequest;

/////////////////////////////////////////////////////////////////////////////////////////////////////////

class __packed UsbDescriptor :
        public ArrayRef<uint8_t>
{
public:
	UsbDescriptor() :
		ArrayRef<uint8_t>((uint8_t*)nullptr, 0)
	{}

    UsbDescriptor(uint8_t * data, Length_t length) :
        ArrayRef<uint8_t>(data, length)
    {}

    inline bool init(uint8_t length,
                     uint8_t type)
    {
        if(length > size())
            return false;
        fields()->bLength = length;
        fields()->bDescriptorType = type;
        return true;
    }

    inline bool isValid() const { return (data() != nullptr) && size() != 0; }

    inline uint8_t bLength() const { return fields()->bLength; }

    inline UsbDescType bDescriptorType() const { return UsbDescType(fields()->bDescriptorType); }

protected:
    inline uint8_t * restFields() const { return fields()->restFields; }

private:
    typedef struct __packed
    {
        uint8_t	bLength;            //Size of the Descriptor in Bytes (18 bytes)
        uint8_t	bDescriptorType;    //Descriptor Type
        uint8_t restFields[];
    }
    UsbDescriptorFields;

    inline UsbDescriptorFields * fields() const { return reinterpret_cast<UsbDescriptorFields*>(data()); }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////

class __packed UsbDeviceDescriptor :
        public UsbDescriptor
{
public:
    enum BCDUsb
	{
        USB_1_0 = 0x0100,
        USB_1_1 = 0x0101,
        USB_2_0 = 0x0200,
        USB_2_1 = 0x0201
    };

    typedef struct __packed
    {
        uint16_t bcdUSB; //USB Specification Number which device complies too.

        uint8_t	bDeviceClass; // Class Code (Assigned by USB Org)
                                // If equal to Zero, each interface specifies it’s own class code
                                // If equal to 0xFF, the class code is vendor specified.
                                // Otherwise field is valid Class Code.

        uint8_t	bDeviceSubClass; //Subclass Code (Assigned by USB Org)

        uint8_t	bDeviceProtocol; //Protocol Code (Assigned by USB Org)

        uint8_t	bMaxPacketSize; //Maximum Packet Size for Zero Endpoint. Valid Sizes are 8, 16, 32, 64

        uint16_t	idVendor; //Vendor ID (Assigned by USB Org)

        uint16_t	idProduct; //Product ID (Assigned by Manufacturer)

        uint16_t	bcdDevice; //Device Release Number

        uint8_t	iManufacturer; //Index of Manufacturer String Descriptor

        uint8_t	iProduct; //Index of Product String Descriptor

        uint8_t	iSerialNumber; //Index of Serial Number String Descriptor

        uint8_t	bNumConfigurations; //Number of Possible Configurations
    }
    DevDescriptorFields;

    static const int SIZE = 2 + sizeof(DevDescriptorFields);

	UsbDeviceDescriptor(uint8_t * data, Length_t length) :
		UsbDescriptor(data, length)
	{}

    inline bool init(uint16_t bcd,
                     uint8_t deviceClass,
                     uint8_t deviceSubClass,
                     uint8_t deviceProtocol,
                     uint8_t maxPacketSize,
                     uint16_t vendorID,
                     uint16_t productID,
                     uint16_t bcdDev,
                     uint8_t manufacturerStr,
                     uint8_t productStr,
                     uint8_t serial,
                     uint8_t numConfigs)
    {
        if(!UsbDescriptor::init(SIZE, UsbDescType_Device))
            return false;
        fields()->bcdUSB = bcd;
        fields()->bDeviceClass = deviceClass;
        fields()->bDeviceSubClass = deviceSubClass;
        fields()->bDeviceProtocol = deviceProtocol;
        fields()->bMaxPacketSize = maxPacketSize;
        fields()->idVendor = vendorID;
        fields()->idProduct = productID;
        fields()->bcdDevice = bcdDev;
        fields()->iManufacturer = manufacturerStr;
        fields()->iProduct = productStr;
        fields()->iSerialNumber = serial;
        fields()->bNumConfigurations = numConfigs;
        return true;
    }

    inline uint16_t bcdUSB() const { return fields()->bcdUSB; }

    inline uint8_t	bDeviceClass() const { return fields()->bDeviceClass; }

    inline uint8_t	bDeviceSubClass() const { return fields()->bDeviceSubClass; }

    inline uint8_t	bDeviceProtocol() const { return fields()->bDeviceProtocol; }

    inline uint8_t	bMaxPacketSize() const { return fields()->bMaxPacketSize; }

    inline uint16_t	idVendor() const { return fields()->idVendor; }

    inline uint16_t	idProduct() const { return fields()->idProduct; }

    inline uint16_t	bcdDevice() const { return fields()->bcdDevice; }

    inline uint8_t	iManufacturer() const { return fields()->iManufacturer; }

    inline uint8_t	iProduct() const { return fields()->iProduct; }

    inline uint8_t	iSerialNumber() const { return fields()->iSerialNumber; }

    inline uint8_t	bNumConfigurations() const { return fields()->bNumConfigurations; }

private:

    inline DevDescriptorFields * fields() const { return reinterpret_cast<DevDescriptorFields*>(restFields()); }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////

class UsbEPDescriptor :
        public UsbDescriptor
{
public:
	static const int DEFAULT_LENGTH = 7;

	typedef enum
	{
		EP_CTRL = 0,
		EP_ISOC = 1,
		EP_BULK = 2,
		EP_INTR = 3
	}
	EPType_t;

    typedef enum
    {
        SYNC_MODE_NONE                             = 0,
        SYNC_MODE_ASYNC                            = (1 << 2),
        SYNC_MODE_ADAPTIVE                         = (2 << 2),
        SYNC_MODE_SYNC                             = (3 << 2)
    }
    IsoEPSync_t;

    typedef enum
    {
        USAGE_MODE_DATA                            = 0,
        USAGE_MODE_FEEDBACK                        = (1 << 4),
        USAGE_MODE_EXFBDATA                        = (2 << 4),
        USAGE_MODE_RESERVED                        = (3 << 4)
    }
    IsoEPUsageMode;

	UsbEPDescriptor() :
		UsbDescriptor()
	{}

    UsbEPDescriptor(uint8_t * data, Length_t length) :
        UsbDescriptor(data, length)
    {}

    inline bool init(uint8_t length,
                     uint8_t address,
                     uint8_t attributes,
                     uint16_t maxPacketSize,
                     uint8_t interval)
    {
        if(!UsbDescriptor::init(length, UsbDescType_Endpoint))
            return false;
        fields()->bEndpointAddress = address;
        fields()->bmAttributes = attributes;
        fields()->wMaxPacketSize = maxPacketSize;
        fields()->bInterval = interval;
        return true;
    }

    inline uint8_t bEndpointAddress() const { return fields()->bEndpointAddress; }

    inline uint8_t bmAttributes() const { return fields()->bmAttributes; }

    inline uint16_t	wMaxPacketSize() const { return fields()->wMaxPacketSize; }

    inline uint8_t	bInterval() const { return fields()->bInterval; }

protected:
    inline uint8_t * restFields() const { return UsbDescriptor::restFields() + sizeof(EPDescriptorFields); }

private:
    typedef struct __packed
    {
        uint8_t bEndpointAddress; //Endpoint Address
                                  //Bits 0..3b Endpoint Number.
                                  //Bits 4..6b Reserved. Set to Zero
                                  //Bits 7 Direction 0 = Out, 1 = In (Ignored for Control Endpoints)

        uint8_t bmAttributes;     //Bits 0..1 Transfer Type
                                  //00 = Control
                                  //01 = Isochronous
                                  //10 = Bulk
                                  //11 = Interrupt
                                  //Bits 2..7 are reserved. If Isochronous endpoint,
                                  //Bits 3..2 = Synchronisation Type (Iso Mode)
                                  //00 = No Synchonisation
                                  //01 = Asynchronous
                                  //10 = Adaptive
                                  //11 = Synchronous
                                  //Bits 5..4 = Usage Type (Iso Mode)
                                  //00 = Data Endpoint
                                  //01 = Feedback Endpoint
                                  //10 = Explicit Feedback Data Endpoint
                                  //11 = Reserved

        uint16_t	wMaxPacketSize; //Maximum Packet Size this endpoint is capable of sending or receiving

        uint8_t	bInterval;        //Interval for polling endpoint data transfers.
                                  //Value in frame counts. Ignored for Bulk & Control Endpoints.
                                  //Isochronous must equal 1 and field may range from 1 to 255 for interrupt endpoints.
    }
    EPDescriptorFields;

    inline EPDescriptorFields * fields() const { return reinterpret_cast<EPDescriptorFields*>(UsbDescriptor::restFields()); }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////

class UsbClassSpecificDescriptor :
        public UsbDescriptor
{
public:
	enum CSType_t
	{
		CS_UNDEFINED 		= 0x20,
		CS_DEVICE 			= 0x21,
		CS_CONFIGURATION 	= 0x22,
		CS_STRING 			= 0x23,
		CS_INTERFACE 		= 0x24,
		CS_ENDPOINT 		= 0x25
	};

    UsbClassSpecificDescriptor(uint8_t * data, Length_t length) :
        UsbDescriptor (data, length)
    {}

    inline bool init(uint8_t length, CSType_t type)
    {
    	return UsbDescriptor::init(length, uint8_t(type));
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////

class __packed UsbStringDescriptor :
        public UsbDescriptor
{
public:
	UsbStringDescriptor() :
		UsbDescriptor(nullptr, 0),
		_idx(0xFF)
	{}

	UsbStringDescriptor(uint8_t idx,
						uint8_t * data,
						uint16_t len) :
			UsbDescriptor(data, len),
			_idx(idx)
	{}

	inline bool init(const uint16_t * langIDs, uint8_t count)
	{
	    if(!UsbDescriptor::init(2 + count * 2, UsbDescType_String))
	        return false;
	    memset(restFields(), 0, count * 2);
	    for(int i = 0; i < count; ++i)
	    {
	    	restFields()[i * 2] = langIDs[i] & 0x00FF;
	    	restFields()[i * 2 + 1] = (langIDs[i] & 0xFF00) >> 8;
	    }
	    return true;
	}

    inline bool init(const char * str)
    {
        uint8_t len = uint8_t(strlen(str));
        if(!UsbDescriptor::init(2 + len * 2, UsbDescType_String))
            return false;
        memset(restFields(), 0, len * 2);
        for(int i = 0; i < len; ++i)
        	restFields()[i * 2] = str[i];
        return true;
    }

    inline bool isValid() const
    {
    	return UsbDescriptor::isValid() && (_idx != 0xFF);
    }

    inline uint8_t idx() const { return _idx; }

    inline operator unsigned char() const { return _idx; }

private:
    uint8_t _idx;
};


/////////////////////////////////////////////////////////////////////////////////////////////////////////

class UsbInterfaceDescriptor :
        public UsbDescriptor
{
public:
	static const uint8_t MaxEndpoints = 8;

    UsbInterfaceDescriptor(uint8_t * data, Length_t length) :
        UsbDescriptor (data, length),
        _totalLength(0)
    {
    	memset(_inEps, 0, MaxEndpoints * sizeof(UsbEPDescriptor*));
    	memset(_outEps, 0, MaxEndpoints * sizeof(UsbEPDescriptor*));
    }

    inline bool init(uint8_t ifaceNumber,
                     uint8_t altSetting,
                     uint8_t ifaceClass,
                     uint8_t ifaceSubClass,
                     uint8_t protocol,
					 const UsbStringDescriptor & ifaceStr)
    {
    	if(!UsbDescriptor::init(2 + sizeof(InterfaceDescriptorFields),
    							UsbDescType_Interface))
    		return false;
    	_totalLength = 2 + sizeof(InterfaceDescriptorFields);
    	fields()->bInterfaceNumber = ifaceNumber;
    	fields()->bAlternateSetting = altSetting;
    	fields()->bNumEndpoints = 0;
    	fields()->bInterfaceClass = ifaceClass;
    	fields()->bInterfaceSubClass = ifaceSubClass;
    	fields()->bInterfaceProtocol = protocol;
    	fields()->iInterface = ifaceStr.idx();
    	return true;
    }

    inline UsbClassSpecificDescriptor beginCSDescriptor() const
    {
        if(_totalLength >= size())
            return UsbClassSpecificDescriptor(nullptr, 0);
        return UsbClassSpecificDescriptor(data() + _totalLength,
                                          size() - _totalLength);
    }

    inline bool endCSDescriptor(const UsbClassSpecificDescriptor & desc)
    {
        if(!desc.isValid())
            return false;
        uint16_t new_length = _totalLength + desc.bLength();
        if(new_length > size())
            return false;
        _totalLength = new_length;
        return true;
    }

    inline UsbEPDescriptor beginEP() const
    {
        if(_totalLength >= size())
            return UsbEPDescriptor(nullptr, 0);
        return UsbEPDescriptor(data() + _totalLength,
                               size() - _totalLength);
    }

    inline bool endEP(UsbEPDescriptor & ep)
    {
        if(!ep.isValid())
            return false;
        uint16_t new_length = _totalLength + ep.bLength();
        if(new_length > size())
            return false;
        _totalLength = new_length;
        ++fields()->bNumEndpoints;

        if(ep.bEndpointAddress() & 0x80)
        	_inEps[ep.bEndpointAddress() & 0x0F] = &ep;
        else
        	_outEps[ep.bEndpointAddress() & 0x0F] = &ep;

        return true;
    }

    inline uint8_t	bInterfaceNumber() const { return fields()->bInterfaceNumber; }

    inline uint8_t	bAlternateSetting() const { return fields()->bAlternateSetting; }

    inline uint8_t	bNumEndpoints() const { return fields()->bNumEndpoints; }

    inline uint8_t	bInterfaceClass() const { return fields()->bInterfaceClass; }

    inline uint8_t	bInterfaceSubClass() const { return fields()->bInterfaceSubClass; }

    inline uint8_t	bInterfaceProtocol() const { return fields()->bInterfaceProtocol; }

    inline uint8_t	iInterface() const { return fields()->iInterface; }

    inline uint16_t totalLength() const { return _totalLength; }

    inline UsbEPDescriptor * getInEndpoint(uint8_t idx) const { return _inEps[idx]; }

    inline UsbEPDescriptor * getOutEndpoint(uint8_t idx) const { return _outEps[idx]; }

private:
    typedef struct __packed
    {
        uint8_t	bInterfaceNumber;   //Number of Interface

        uint8_t	bAlternateSetting;  //Value used to select alternative setting

        uint8_t	bNumEndpoints;      //Number of Endpoints used for this interface

        uint8_t	bInterfaceClass;    //Class Code (Assigned by USB Org)

        uint8_t	bInterfaceSubClass; //Subclass Code (Assigned by USB Org)

        uint8_t	bInterfaceProtocol; //Protocol Code (Assigned by USB Org)

        uint8_t	iInterface;         //Index of String Descriptor Describing this interface
    }
    InterfaceDescriptorFields;

    uint16_t  			_totalLength;
    UsbEPDescriptor	* 	_inEps[MaxEndpoints];
    UsbEPDescriptor	* 	_outEps[MaxEndpoints];

    inline InterfaceDescriptorFields * fields() const { return reinterpret_cast<InterfaceDescriptorFields*>(restFields()); }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////

class UsbConfigDescriptor :
        public UsbDescriptor
{
public:
	UsbConfigDescriptor(uint8_t * data, Length_t length) :
		UsbDescriptor(data, length)
	{}

    inline bool init(uint8_t configValue,
    				 const UsbStringDescriptor & configStr,
                     uint8_t attributes,
                     uint8_t maxPower)
    {
        if(!UsbDescriptor::init(2 + sizeof(ConfDescriptorFields), UsbDescType_Configuration))
            return false;

        fields()->wTotalLength = 2 + sizeof(ConfDescriptorFields);
        fields()->bNumInterfaces = 0;
        fields()->bConfigurationValue = configValue;
        fields()->iConfiguration = configStr.idx();
        fields()->bmAttributes = attributes;
        fields()->bMaxPower = maxPower;
        return true;
    }

    inline uint16_t	wTotalLength() const { return fields()->wTotalLength; }

    inline uint8_t	bNumInterfaces() const { return fields()->bNumInterfaces; }

    inline uint8_t	bConfigurationValue() const { return fields()->bConfigurationValue; }

    inline uint8_t	iConfiguration() const { return fields()->iConfiguration; }

    inline uint8_t	bmAttributes() const { return fields()->bmAttributes; }

    inline uint8_t	bMaxPower() const { return fields()->bMaxPower; }

    inline UsbInterfaceDescriptor beginInterface() const
    {
        uint16_t totalLength = wTotalLength();
        if(totalLength >= size())
            return UsbInterfaceDescriptor(nullptr, 0);
        return UsbInterfaceDescriptor(data() + totalLength,
                                      size() - totalLength);
    }

    inline bool endInterface(const UsbInterfaceDescriptor & iface) const
    {
        if(!iface.isValid())
            return false;
        uint16_t new_length = fields()->wTotalLength + iface.totalLength();
        if(new_length > size())
            return false;
        fields()->wTotalLength = new_length;
        ++fields()->bNumInterfaces;
        return true;
    }

private:
    typedef struct __packed
    {
        uint16_t	wTotalLength; //Total length in bytes of data returned

        uint8_t     bNumInterfaces; //Number of Interfaces

        uint8_t     bConfigurationValue; //Value to use as an argument to select this configuration

        uint8_t     iConfiguration; //Index of String Descriptor describing this configuration

        uint8_t     bmAttributes;   //D7 Reserved, set to 1. (USB 1.0 Bus Powered)
                                    //D6 Self Powered
                                    //D5 Remote Wakeup
                                    //D4..0 Reserved, set to 0.

        uint8_t	bMaxPower; //Maximum Power Consumption in 2mA units
    }
    ConfDescriptorFields;

    inline ConfDescriptorFields * fields() const { return reinterpret_cast<ConfDescriptorFields*>(restFields()); }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////

#endif // USBDESCRIPTORS_H
