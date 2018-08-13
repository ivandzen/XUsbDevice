/*
 * XUsbDevice.cpp
 *
 *  Created on: Sep 3, 2017
 *      Author: ivan
 */

#include <plugins/usb/device/XUsbDevice.h>


#define LOBYTE(x)  ((uint8_t)(x & 0x00FF))
#define HIBYTE(x)  ((uint8_t)((x & 0xFF00) >>8))
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))
#define  SWAPBYTE(addr)        (((uint16_t)(*((uint8_t *)(addr)))) + \
                               (((uint16_t)(*(((uint8_t *)(addr)) + 1))) << 8))

XUsbEndpoint::XUsbEndpoint(const UsbEPDescriptor & descriptor,
				 	 	   XUsbIface * iface) :
		UsbEPDescriptor(descriptor),
		_handle(nullptr),
		_status(0),
		_iface(iface),
		_opened(false)
{}

/////////////////////////////////////////////////////////////////////////////////////////

void XUsbEndpoint::reportStatus(XUsbZeroEndpoint * ep0)
{
	ep0->transmit(reinterpret_cast<uint8_t*>(&_status), 2);
}

/////////////////////////////////////////////////////////////////////////////////////////

void XUsbZeroEndpoint::setupStage(uint8_t * pdata)
{
    _request.bmRequest     = *(uint8_t *)  (pdata);
    _request.bRequest      = *(uint8_t *)  (pdata +  1);
    _request.wValue        = SWAPBYTE      (pdata +  2);
    _request.wIndex        = SWAPBYTE      (pdata +  4);
    _request.wLength       = SWAPBYTE      (pdata +  6);

    _state = EP0_SETUP;
    _dataLength = _request.wLength;

    switch (_request.bmRequest & 0x1F)
    {
    case REQ_RECIPIENT_DEVICE:
        stdDevReq(&_request);
        break;

    case REQ_RECIPIENT_INTERFACE:
        stdItfReq(&_request);
        break;

    case REQ_RECIPIENT_ENDPOINT:
        stdEPReq(&_request);
        break;

    default:
    	XUsbInEndpoint::stall();
        break;
    }
}

/////////////////////////////////////////////////////////////////////////////////////////

bool XUsbZeroEndpoint::epDataOut(uint8_t * pdata)
{
	if ( _state == EP0_DATA_OUT)
	{
		if(_outRemLength > XUsbOutEndpoint::wMaxPacketSize())
		{
			_outRemLength -=  XUsbOutEndpoint::wMaxPacketSize();
			XUsbOutEndpoint::receive(pdata, MIN(_outRemLength , XUsbOutEndpoint::wMaxPacketSize()));
		}
		else
		{
			if(isDeviceConfigured())
				ep0RxReady(&_request);
			ctlSendStatus();
		}
	}
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////

bool XUsbZeroEndpoint::epDataIn(uint8_t * pdata)
{
	if (_state == EP0_DATA_IN)
	{
		if(_inRemLength > XUsbOutEndpoint::wMaxPacketSize())
		{
			_inRemLength -=  XUsbOutEndpoint::wMaxPacketSize();
			XUsbInEndpoint::transmit(pdata, _inRemLength);

			/* Prepare endpoint for premature end of transfer */
			XUsbOutEndpoint::receive(nullptr, 0);
		}
		else
		{ /* last packet is MPS multiple, so send ZLP packet */
			if((_inTotalLength % XUsbOutEndpoint::wMaxPacketSize() == 0) &&
				(_inTotalLength >= XUsbOutEndpoint::wMaxPacketSize()) &&
				(_inTotalLength < _dataLength ))
			{
				XUsbInEndpoint::transmit(nullptr, 0);
				_dataLength = 0;

				/* Prepare endpoint for premature end of transfer */
				XUsbOutEndpoint::receive(nullptr, 0);
			}
			else
			{
				if(isDeviceConfigured())
					ep0TxSent(&_request);
				ctlReceiveStatus();
			}
		}
	}

	//! @attention нужно этот код активировать на всякий случай
	/*
	if (_dev_test_mode == 1)
	{
		runTestMode();
		_dev_test_mode = 0;
	}
	*/

	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////

#define MAX_PACKET 64

XUsbDevice::XUsbDevice(void * handle, bool selfPowered) :
		XUsbZeroEndpoint(handle, MAX_PACKET),
		_handle(handle),
		_dev_test_mode(false),
		_dev_old_state(DEV_DEFAULT),
		_dev_state(DEV_DEFAULT),
		_dev_address(0),
	    _dev_config_status(selfPowered),
	    _dev_remote_wakeup(0),
	    _dev_config(1)
{
	for(int i = 0; i < UsbInterfaceDescriptor::MaxEndpoints; ++i)
	{
		_inEndpoints[i] = nullptr;
		_outEndpoints[i] = nullptr;
	}

	_inEndpoints[0] = this;
	_outEndpoints[0] = this;

	for(int i = 0; i < USB_MAX_CONFIGS; ++i)
		_configs[i] = nullptr;

	static uint8_t strDesc0[4];
	static const uint16_t LANGID = 0x0409;
	_strings[0] = UsbStringDescriptor(0, strDesc0, 4);
	_strings[0].init(&LANGID, 1);
}

/////////////////////////////////////////////////////////////////////////////////////////

XUsbDevice::~XUsbDevice()
{
	for(int i = 0; i < USB_MAX_STRINGS; ++i)
		if(_strings[i].isValid())
			delete[] _strings[i].data();
}

/////////////////////////////////////////////////////////////////////////////////////////

bool  XUsbDevice::dataOutStage(uint8_t epnum, uint8_t * pdata)
{
	if((epnum == 0) || (_dev_state == DEV_CONFIGURED))
		return _outEndpoints[epnum]->epDataOut(pdata);
	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////

bool  XUsbDevice::dataInStage(uint8_t epnum, uint8_t * pdata)
{
	if((epnum == 0) || (_dev_state == DEV_CONFIGURED))
		return _inEndpoints[epnum]->epDataIn(pdata);
	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////

void  XUsbDevice::SOF()
{
	if(_dev_state == DEV_CONFIGURED)
	{
	}
}

/////////////////////////////////////////////////////////////////////////////////////////

void  XUsbDevice::suspend()
{
	_dev_old_state =  _dev_state;
	_dev_state  = DEV_SUSPENDED;
}

/////////////////////////////////////////////////////////////////////////////////////////

void  XUsbDevice::resume()
{
	_dev_state = _dev_old_state;
}

/////////////////////////////////////////////////////////////////////////////////////////

void  XUsbDevice::isoOutIncomplete(uint8_t epnum)
{
	//_outEndpoints[epnum & 0x7F]->isoOutIncomplete();
}

/////////////////////////////////////////////////////////////////////////////////////////

void XUsbDevice::isoInIncomplete(uint8_t epnum)
{
	//_inEndpoints[epnum & 0x7F]->isoInIncomplete();
}

/////////////////////////////////////////////////////////////////////////////////////////

void XUsbDevice::reset()
{
    /* Open EP0 OUT */
	_inEndpoints[0]->open();

    /* Open EP0 IN */
	_outEndpoints[0]->open();

    /* Upon Reset call user call back */
    _dev_state = DEV_DEFAULT;

    _configs[_dev_config]->deInit();
    //resetEvent();
}

/////////////////////////////////////////////////////////////////////////////////////////

void  XUsbDevice::stdDevReq(UsbSetupRequest * req)
{
    switch (req->bRequest) {
    case REQ_GET_DESCRIPTOR:
        getDescriptor (req);
        break;

    case REQ_SET_ADDRESS:
        setAddress(req);
        break;

    case REQ_SET_CONFIGURATION:
        setConfig (req);
        break;

    case REQ_GET_CONFIGURATION:
        getConfig (req);
        break;

    case REQ_GET_STATUS:
        getStatus (req);
        break;

    case REQ_SET_FEATURE:
        setFeature (req);
        break;

    case REQ_CLEAR_FEATURE:
        clrFeature (req);
        break;

    default:
        ctlError();
        break;
    }
}

/////////////////////////////////////////////////////////////////////////////////////////

void  XUsbDevice::stdItfReq(UsbSetupRequest * req)
{
	static uint8_t ifalt = 0;
    bool ret = true;

    switch (_dev_state)
    {
    case DEV_CONFIGURED:
    {
    	switch (req->bmRequest & REQ_TYPE_MASK)
        {
        case REQ_TYPE_CLASS :
        {
        	if(!_configs[_dev_config]->setupRequest(LOBYTE(req->wIndex), req))
        		ctlError();
        	break;
        }

        case REQ_TYPE_STANDARD:
        {
        	switch (req->bRequest)
        	{
        	case REQ_GET_INTERFACE :
        		ctlTransmit(&ifalt, 1);
        		break;

        	case REQ_SET_INTERFACE :
        	{
        		break;
        	}
        	}
        }
        }

        if((req->wLength == 0) && ret)
        	ctlSendStatus();

        break;
    }

    default:
        ctlError();
        break;
    }
}

/////////////////////////////////////////////////////////////////////////////////////////

void  XUsbDevice::stdEPReq(UsbSetupRequest * req)
{
    uint8_t ep_addr  = LOBYTE(req->wIndex);

    XUsbEndpoint * ep;
    if(ep_addr & 0x80)
    	ep = _inEndpoints[ep_addr & 0x7F];
    else
    	ep = _outEndpoints[ep_addr & 0x7F];

    if ((req->bmRequest & 0x60) == 0x20)
    {
    	ep->setupRequest(req);
    	return;
    }

    switch(_dev_state)
    {
    case DEV_ADDRESSED :
    {
    	if ((ep_addr != 0x00) && (ep_addr != 0x80))
    		ep->stall();
    	return;
    }

    case DEV_CONFIGURED :
    {
    	switch (req->bRequest)
    	{
    	case REQ_SET_FEATURE :
    	{
    		if ((req->wValue == UsbFeature_EP_HALT) &&
    			(ep_addr != 0x00) && (ep_addr != 0x80))
    				ep->stall();

    		ctlSendStatus();
    		break;
    	}

    	case REQ_CLEAR_FEATURE :
    	{
    		if ((req->wValue == UsbFeature_EP_HALT) &&
    			((ep_addr & 0x7F) != 0x00))
    			ep->clearStall();

    		ctlSendStatus();
    		break;
    	}

    	case REQ_GET_STATUS:
    		ep->reportStatus(this);
    		break;

    	}
    	break;
    }

    default :
    	ctlError();
    	break;
    }
}

/////////////////////////////////////////////////////////////////////////////////////////

void XUsbDevice::ep0RxReady(UsbSetupRequest * req)
{
	_configs[_dev_config]->ep0RxReady(req);
}

/////////////////////////////////////////////////////////////////////////////////////////

void XUsbDevice::ep0TxSent(UsbSetupRequest * req)
{
	_configs[_dev_config]->ep0TxSent(req);
}

/////////////////////////////////////////////////////////////////////////////////////////

void XUsbDevice::getDescriptor(UsbSetupRequest *req)
{
    uint16_t len = 0;
    uint8_t * pbuf = nullptr;

    switch (req->wValue >> 8)
    {
  #if (USBD_LPM_ENABLED == 1)
    case UsbDesc::DescBOS:
      pbuf = getBOSDescriptor(_dev_speed, &len);
      break;
  #endif
    case UsbDescType_Device:
    {
    	pbuf = _devDescData;
    	len = UsbDeviceDescriptor::SIZE;
    	break;
    }

    case UsbDescType_Configuration :
    {
    	if(XUsbConfiguration * config = _configs[_dev_config])
    	{
    		pbuf = config->data();
    		len = config->wTotalLength();
    	}
        break;

    }

    case UsbDescType_String :
    {
    	uint8_t str_idx = (uint8_t)(req->wValue);
    	if((str_idx < USB_MAX_STRINGS) && (_strings[str_idx].isValid()))
    	{
    		pbuf = _strings[str_idx].data();
    		len = _strings[str_idx].bLength();
    	}
    	else
    		ctlError();
    	break;
    }

    case UsbDescType_DeviceQualifier :
        ctlError();
        break;

    case UsbDescType_OtherSpeedConfiguration :
    	ctlError();
    	break;

    default:
        ctlError();
        return;
    }

    if((len != 0)&& (req->wLength != 0))
    {
        len = MIN(len , req->wLength);
        ctlTransmit(pbuf, len);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////

void XUsbDevice::setAddress(UsbSetupRequest *req)
{
    uint8_t  dev_addr;

    if ((req->wIndex == 0) && (req->wLength == 0))
    {
        dev_addr = (uint8_t)(req->wValue) & 0x7F;

        if (_dev_state == DEV_CONFIGURED)
        	ctlError();
        else
        {
            _dev_address = dev_addr;
            HAL_XUsbDevice_SetAddress(_handle, dev_addr);
            ctlSendStatus();

            if (dev_addr != 0)
                _dev_state  = DEV_ADDRESSED;
            else
                _dev_state  = DEV_DEFAULT;
        }
    }
    else
    	ctlError();
}

/////////////////////////////////////////////////////////////////////////////////////////

void XUsbDevice::setConfig(UsbSetupRequest *req)
{
    static uint8_t  cfgidx = (uint8_t)(req->wValue);

    if (cfgidx > USB_MAX_CONFIGS ||
    	_configs[cfgidx] == nullptr)
    {
        ctlError();
        return;
    }

    switch (_dev_state)
    {
    case DEV_ADDRESSED:
    {
    	if (cfgidx)
    	{
    		_dev_config = cfgidx;
    		_dev_state = DEV_CONFIGURED;
    		if(!_configs[cfgidx]->initDefaultIface(this))
    		{
    			ctlError();
    			return;
    		}
    		ctlSendStatus();
    	}
    	else
    		ctlSendStatus();
    	break;
    }

    case DEV_CONFIGURED:
    {
    	if (cfgidx == 0)
    	{
    		_dev_state = DEV_ADDRESSED;
    		_configs[_dev_config]->deInit();
    		_dev_config = cfgidx;
    		ctlSendStatus();
    	}
    	else  if (cfgidx != _dev_config)
    	{
    		_configs[_dev_config]->deInit();
    		_dev_config = cfgidx;
    		if(!_configs[cfgidx]->initDefaultIface(this))
    		{
    			ctlError();
    			return;
    		}
    		ctlSendStatus();
    	}
    	else
    		ctlSendStatus();

    	break;
    }

    default:
    	ctlError();
    	break;
    }
}

/////////////////////////////////////////////////////////////////////////////////////////

void XUsbDevice::getConfig(UsbSetupRequest *req)
{

    if (req->wLength != 1)
        ctlError();
    else
    {
        switch (_dev_state )
        {
        case DEV_ADDRESSED:
        case DEV_CONFIGURED:
            ctlTransmit ((uint8_t *)&_dev_config, 1);
            break;

        default:
            ctlError();
            break;
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////////////

void XUsbDevice::getStatus(UsbSetupRequest *)
{
    switch (_dev_state)
    {
    case DEV_ADDRESSED:
    case DEV_CONFIGURED:

  #if ( USBD_SELF_POWERED == 1)
        _dev_config_status = CONFIG_SELF_POWERED;
  #else
        _dev_config_status = 0;
  #endif

        if (_dev_remote_wakeup)
            _dev_config_status |= CONFIG_REMOTE_WAKEUP;

        ctlTransmit ((uint8_t *)&_dev_config_status, 2);
        break;

    default :
        ctlError();
        break;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void XUsbDevice::setFeature(UsbSetupRequest * req)
{
	//! @todo разобраться, что нужно делать с фичами
    if (req->wValue == UsbFeature_REMOTE_WAKEUP)
    {
        _dev_remote_wakeup = 1;
        _configs[_dev_config]->setupRequest(0, req);
        ctlSendStatus();
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void XUsbDevice::clrFeature(UsbSetupRequest * req)
{
    switch (_dev_state) {
    case DEV_ADDRESSED:
    case DEV_CONFIGURED:
    {
        if (req->wValue == UsbFeature_REMOTE_WAKEUP)
        {
            _dev_remote_wakeup = 0;
            _configs[_dev_config]->setupRequest(0, req);
            ctlSendStatus();
        }
        break;
    }

    default :
        ctlError();
        break;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

UsbStringDescriptor XUsbDevice::createStr(const char * str)
{
	size_t len = strlen(str);
	for(int i = 0; i < USB_MAX_STRINGS; ++i)
		if(!_strings[i].isValid())
		{
			//! Аллоцируется новый буфер под дескриптор строки
			//! Его длина равна длине стандартного заголовка usb(2 байта) +
			//! длина строки в символах * 2 (кодировка в юникоде)
			UsbStringDescriptor desc(i, new uint8_t[2 + len * 2], 2 + len * 2);
			if(!desc.init(str))
			{
				delete[] desc.data();
				return UsbStringDescriptor();
			}

			_strings[i] = desc;
			return _strings[i];
		}
	return UsbStringDescriptor();
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void XUsbDevice::addConfig(XUsbConfiguration * config)
{
	uint8_t idx = config->bConfigurationValue();
	assert((idx < USB_MAX_CONFIGS) && (_configs[idx] == nullptr));
	_configs[idx] = config;
}

