/*
 * XUsbDevice.h
 *
 *  Created on: Sep 3, 2017
 *      Author: ivan
 */

#ifndef XUSBDEVICE_H_
#define XUSBDEVICE_H_

#include <plugins/usb/common/usbdescriptors.h>
#include "XUsbDevice_Config.h"

/////////////////////////////////////////////////////////////////////////////////////////

class XUsbIface;
class XUsbZeroEndpoint;

class XUsbEndpoint :
		public UsbEPDescriptor
{
public:
	XUsbEndpoint(const UsbEPDescriptor & descriptor,
				 XUsbIface * iface);

	virtual ~XUsbEndpoint() {}

	virtual bool setupRequest(UsbSetupRequest *) { return false; }

	inline void open()
	{
		if(!_opened)
		{
			HAL_XUsbDevice_OpenEP(_handle, bEndpointAddress(), wMaxPacketSize(), bmAttributes() & UsbEPTypeMask);
			_opened = true;
		}
	}

	inline void close()
	{
		if(_opened)
		{
			HAL_XUsbDevice_CloseEP(_handle, bEndpointAddress());
			_opened = false;
		}
	}

	inline void stall()
	{
		HAL_XUsbDevice_StallEP(_handle, bEndpointAddress());
		_status = 0x0001;
	}

	inline void clearStall()
	{
		HAL_XUsbDevice_ClearStallEP(_handle, bEndpointAddress());
		_status = 0x0001;
	}

	inline void flush()
	{
		HAL_XUsbDevice_Flush(_handle, bEndpointAddress());
	}

	void reportStatus(XUsbZeroEndpoint * ep0);

	inline void setHandle(void * handle) { _handle = handle; }

	inline void * handle() const { return _handle; }

	XUsbIface * iface() const { return _iface; }

private:
	void * 		_handle;
	uint16_t	_status;
	XUsbIface *	_iface;
	bool		_opened;
};

/////////////////////////////////////////////////////////////////////////////////////////

class __packed XUsbInEndpoint :
		public XUsbEndpoint
{
public:
	explicit XUsbInEndpoint(const XUsbEndpoint & source) :
		XUsbEndpoint(source)
	{}

	inline bool init(uint8_t length,
		             uint8_t epnum,
		             uint8_t attributes,
		             uint16_t maxPacketSize,
		             uint8_t interval)
	{
		return XUsbEndpoint::init(length, epnum | 0x80, attributes, maxPacketSize, interval);
	}

	virtual bool epDataIn(uint8_t * pdata) = 0;

	inline void transmit(uint8_t * pbuf, uint16_t size)
	{
		HAL_XUsbDevice_Transmit(handle(), bEndpointAddress(), pbuf, size);
	}
};

/////////////////////////////////////////////////////////////////////////////////////////

class __packed XUsbOutEndpoint :
		public XUsbEndpoint
{
public:
	explicit XUsbOutEndpoint(const XUsbEndpoint & source) :
		XUsbEndpoint(source)
	{}

	inline bool init(uint8_t length,
	                 uint8_t epnum,
	                 uint8_t attributes,
	                 uint16_t maxPacketSize,
	                 uint8_t interval)
	{
		return XUsbEndpoint::init(length, epnum & 0x7F, attributes, maxPacketSize, interval);
	}

	virtual bool epDataOut(uint8_t * pdata) = 0;

	inline void receive(uint8_t * pbuf, uint16_t size)
	{
		HAL_XUsbDevice_Receive(handle(), bEndpointAddress(), pbuf, size);
	}
};

/////////////////////////////////////////////////////////////////////////////////////////

class __packed XUsbZeroEndpoint :
		public XUsbInEndpoint,
		public XUsbOutEndpoint
{
public:
	XUsbZeroEndpoint(void * handle,
					uint8_t max_packet) :
		XUsbInEndpoint(XUsbEndpoint(UsbEPDescriptor(_inEpData, UsbEPDescriptor::DEFAULT_LENGTH), nullptr)),
		XUsbOutEndpoint(XUsbEndpoint(UsbEPDescriptor(_outEpData, UsbEPDescriptor::DEFAULT_LENGTH), nullptr)),
	    _state(EP0_IDLE),
	    _inTotalLength(0),
	    _inRemLength(0),
	    _outTotalLength(0),
	    _outRemLength(0),
	    _dataLength(0)
	{
		XUsbInEndpoint::setHandle(handle);
		XUsbOutEndpoint::setHandle(handle);

		XUsbInEndpoint::init(UsbEPDescriptor::DEFAULT_LENGTH, 0x80, 0x00, max_packet, 0);
		XUsbOutEndpoint::init(UsbEPDescriptor::DEFAULT_LENGTH, 0x00, 0x00, max_packet, 0);
	}

	inline void ctlTransmit(uint8_t * pdata, uint16_t len)
	{
		/* Set EP0 State */
		_state      	= EP0_DATA_IN;
		_inTotalLength 	= len;
		_inRemLength	= len;
		/* Start the transfer */
		XUsbInEndpoint::transmit(pdata, len);
	}

	inline void ctlReceive(uint8_t * pdata, uint16_t len)
	{
		/* Set EP0 State */
		_state = EP0_DATA_OUT;
		_outTotalLength = len;
		_outRemLength   = len;
		/* Start the transfer */
		XUsbOutEndpoint::receive(pdata, len);
	}

	inline void ctlSendStatus()
	{
		/* Set EP0 State */
		_state = EP0_STATUS_IN;

		/* Start the transfer */
		XUsbInEndpoint::transmit(nullptr, 0);
	}

	inline void ctlReceiveStatus()
	{
	    /* Set EP0 State */
	    _state = EP0_STATUS_OUT;
	   /* Start the transfer */
	    XUsbOutEndpoint::receive (nullptr, 0);
	}

	void setupStage(uint8_t * pdata);

	inline void ctlError()
	{
		XUsbInEndpoint::stall();
		XUsbOutEndpoint::stall();
	}

protected:
	virtual bool isDeviceConfigured() const = 0;

	virtual void stdDevReq(UsbSetupRequest * req) = 0;

	virtual void stdItfReq(UsbSetupRequest * req) = 0;

	virtual void stdEPReq(UsbSetupRequest * req) = 0;

	virtual void ep0TxSent(UsbSetupRequest * req) = 0;

	virtual void ep0RxReady(UsbSetupRequest * req) = 0;

	virtual bool epDataOut(uint8_t * pdata) final override;

	virtual bool epDataIn(uint8_t * pdata) final override;

private:
    typedef enum
	{
        EP0_IDLE,
        EP0_SETUP,
        EP0_DATA_IN,
        EP0_DATA_OUT,
        EP0_STATUS_IN,
        EP0_STATUS_OUT,
        EP0_STALL
    }
    EP0State;

    EP0State 		_state;
    uint32_t		_inTotalLength;
    uint32_t		_inRemLength;
    uint32_t		_outTotalLength;
    uint32_t		_outRemLength;
    uint16_t		_dataLength;
    UsbSetupRequest _request;
    uint8_t 		_inEpData[UsbEPDescriptor::DEFAULT_LENGTH];
    uint8_t 		_outEpData[UsbEPDescriptor::DEFAULT_LENGTH];
};

/////////////////////////////////////////////////////////////////////////////////////////

class XUsbConfiguration;

class __packed XUsbDevice :
	public XUsbZeroEndpoint
{
	friend class XUsbIface;

public:
    typedef enum
	{
        USBD_SPEED_HIGH  = 0,
        USBD_SPEED_FULL  = 1,
        USBD_SPEED_LOW   = 2,
    }
    Speed;

    enum DevConfig
	{
        CONFIG_REMOTE_WAKEUP    = 0x0002,
        CONFIG_SELF_POWERED     = 0x0001
    };


	explicit XUsbDevice(void * handle, bool selfPowered);

	bool init(uint16_t bcd,
              uint8_t deviceClass,
              uint8_t deviceSubClass,
              uint8_t deviceProtocol,
              uint8_t maxPacketSize,
              uint16_t vendorID,
              uint16_t productID,
              uint16_t bcdDev,
              const char * manufacturerStr,
              const char * productStr,
			  const char * serial,
              uint8_t numConfigs)
	{
		return UsbDeviceDescriptor(DataPtr(_devDescData), UsbDeviceDescriptor::SIZE).
					init(bcd, deviceClass, deviceSubClass, deviceProtocol,
						 maxPacketSize, vendorID, productID, bcdDev,
						 createStr(manufacturerStr), createStr(productStr),
						 createStr(serial), numConfigs);
	}

	virtual ~XUsbDevice();

    bool dataOutStage(uint8_t epnum, uint8_t * pdata);

    bool dataInStage(uint8_t epnum, uint8_t * pdata);

    void SOF();

    void reset();

    void suspend();

    void resume();

    void isoOutIncomplete(uint8_t epnum);

    void isoInIncomplete(uint8_t epnum);

    virtual void connected() {}

    virtual void disconnected() {}

    inline void * handle() const { return _handle; }

    UsbStringDescriptor createStr(const char * str);

    void addConfig(XUsbConfiguration * config);

protected:
	virtual void runTestMode() {}

private:
	virtual bool isDeviceConfigured() const final override { return _dev_state == DEV_CONFIGURED; }

    virtual void stdDevReq(UsbSetupRequest * req) final override;

    virtual void stdItfReq(UsbSetupRequest * req) final override;

    virtual void stdEPReq(UsbSetupRequest * req) final override;

	virtual void ep0TxSent(UsbSetupRequest * req) final override;

	virtual void ep0RxReady(UsbSetupRequest * req) final override;

    void 	setAddress(UsbSetupRequest * req);

    void	setConfig(UsbSetupRequest * req);

    void 	getConfig(UsbSetupRequest * req);

    void	getStatus(UsbSetupRequest * req);

    void	setFeature(UsbSetupRequest * req);

    void	clrFeature(UsbSetupRequest * req);

    void 	getDescriptor(UsbSetupRequest * req);

    inline void	setInEndpoint(uint8_t epnum, XUsbInEndpoint * ep)
    {
    	assert(epnum < UsbInterfaceDescriptor::MaxEndpoints);

    	if(_inEndpoints[epnum] != nullptr)
    		_inEndpoints[epnum]->close();

    	_inEndpoints[epnum] = ep;
    	if(ep != nullptr)
    	{
    		ep->setHandle(_handle);
    		ep->open();
    	}
    }

    inline void setOutEndpoint(uint8_t epnum, XUsbOutEndpoint * ep)
    {
    	assert(epnum < UsbInterfaceDescriptor::MaxEndpoints);

    	if(_outEndpoints[epnum] != nullptr)
    		_outEndpoints[epnum]->close();

    	_outEndpoints[epnum] = ep;
    	if(ep != nullptr)
    	{
    		ep->setHandle(_handle);
    		ep->open();
    	}
    }

    enum DeviceState
	{
        DEV_DEFAULT,
        DEV_ADDRESSED,
        DEV_CONFIGURED,
        DEV_SUSPENDED
    };

    void *				_handle;
    bool				_dev_test_mode;
    DeviceState         _dev_old_state;
    DeviceState         _dev_state;
    uint8_t				_dev_address;
    uint32_t            _dev_config_status;
    uint32_t            _dev_remote_wakeup;
    uint8_t				_dev_config;
    UsbStringDescriptor	_strings[USB_MAX_STRINGS];
    XUsbConfiguration *	_configs[USB_MAX_CONFIGS];
    XUsbInEndpoint *	_inEndpoints[UsbInterfaceDescriptor::MaxEndpoints];
    XUsbOutEndpoint *	_outEndpoints[UsbInterfaceDescriptor::MaxEndpoints];
    uint8_t				_devDescData[UsbDeviceDescriptor::SIZE];
};

/////////////////////////////////////////////////////////////////////////////////////////

class XUsbIface :
		public UsbInterfaceDescriptor
{
public:
	explicit XUsbIface(const UsbInterfaceDescriptor & self) :
		UsbInterfaceDescriptor(self),
		_device(nullptr)
	{}

	virtual bool setupRequest(UsbSetupRequest * req) = 0;

	virtual void ep0RxReady(UsbSetupRequest * req) = 0;

	virtual void ep0TxSent(UsbSetupRequest * req) = 0;

	void build(XUsbDevice * dev)
	{
		_device = dev;
		for(int epnum = 1; epnum < UsbInterfaceDescriptor::MaxEndpoints; ++epnum)
		{
			_device->setInEndpoint(epnum, static_cast<XUsbInEndpoint*>(getInEndpoint(epnum)));
			_device->setOutEndpoint(epnum, static_cast<XUsbOutEndpoint*>(getOutEndpoint(epnum)));
		}
	}

	inline bool isInitialized() const { return _device != nullptr; }

	inline void release() { _device = nullptr; }

	inline XUsbEndpoint beginEP()
	{
		return XUsbEndpoint(UsbInterfaceDescriptor::beginEP(), this);
	}

	inline bool endEP(XUsbEndpoint & ep) { return UsbInterfaceDescriptor::endEP(ep); }

protected:
	inline void ep0Transmit(uint8_t * pbuf, uint16_t size)
	{
		_device->ctlTransmit(pbuf, size);
	}

	inline void ep0Receive(uint8_t * pbuf, uint16_t size)
	{
		_device->ctlReceive(pbuf, size);
	}

private:
	XUsbDevice * _device;
};

/////////////////////////////////////////////////////////////////////////////////////////

#define USB_MAX_IFACES 8

class __packed XUsbConfiguration :
	public UsbConfigDescriptor
{
public:
	static const uint8_t DefaultIface = 0;

	typedef enum
	{
		XUSBCFG_SUCCESS = 0,
		XUSBCFG_ERR_BEG_IFACE = -1,
		XUSBCFG_ERR_BUILD_IFACE = - 2,
	}
	ErrCode;

	explicit XUsbConfiguration(const UsbConfigDescriptor & other) :
		UsbConfigDescriptor(other),
		_iface(0)
	{
		for(int i = 0; i < USB_MAX_IFACES; ++i)
			_interfaces[i] = nullptr;
	}

	virtual ~XUsbConfiguration() {}

	inline bool initIface(uint8_t idx, XUsbDevice * device) const
	{
		if((idx < USB_MAX_IFACES) && (_interfaces[idx] != nullptr))
		{
			_interfaces[idx]->build(device);
			return true;
		}

		return false;
	}

	inline bool initDefaultIface(XUsbDevice * device) const
	{
		return initIface(DefaultIface, device);
	}

	inline void deInit()
	{
		//! @todo
	}

	inline bool setupRequest(uint8_t iface, UsbSetupRequest * req)
	{
		return _interfaces[iface]->setupRequest(req);
	}

	inline void ep0RxReady(UsbSetupRequest * req)
	{
		_interfaces[_iface]->ep0RxReady(req);
	}

	inline void ep0TxSent(UsbSetupRequest * req)
	{
		_interfaces[_iface]->ep0TxSent(req);
	}

	inline bool endInterface(XUsbIface & iface)
	{
		uint8_t ifaceNum = iface.bInterfaceNumber();
		if((ifaceNum > USB_MAX_INTERFACES) ||
			(_interfaces[ifaceNum] != nullptr))
			return false;
		_interfaces[ifaceNum] = &iface;
		return UsbConfigDescriptor::endInterface(iface);
	}

private:
	uint8_t			_iface;
	XUsbIface 	* 	_interfaces[USB_MAX_IFACES];
};

#endif /* XUSBDEVICE_H_ */
