/* CE USB KWrapper - a USB kernel driver and user-space library
 * Copyright (C) 2012 RealVNC Ltd.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

// ceusbkwrapper.h: Library interface for user-side library for 
// kernel USB wrapper.
#ifndef CEUSBKWRAPPER_H
#define CEUSBKWRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif

// ceusbkwrapper_EXPORTS should only be defined when building ceusbkwrapper
#ifdef ceusbkwrapper_EXPORTS
#define ceusbkwrapper_API
#else
#define ceusbkwrapper_API __declspec(dllimport) 
#endif

struct UKW_DEVICE_PRIV;
typedef struct UKW_DEVICE_PRIV *UKW_DEVICE;
typedef UKW_DEVICE *PUKW_DEVICE, *LPUKW_DEVICE;

/**
 * Structure containing device descriptor information
 * matching the device descriptor described in the USB
 * specification as closely as possible.
 *
 * Multi-byte fields should be in native CPU endianness.
 */
typedef struct {
	UINT8 bLength;
	UINT8 bDescriptorType;
	UINT16 bcdUSB;
	UINT8 bDeviceClass;
	UINT8 bDeviceSubClass;
	UINT8 bDeviceProtocol;
	UINT8 bMaxPacketSize0;
	UINT16 idVendor;
	UINT16 idProduct;
	UINT16 bcdDevice;
	UINT8 iManufacturer;
	UINT8 iProduct;
	UINT8 iSerialNumber;
	UINT8 bNumConfigurations;
} UKW_DEVICE_DESCRIPTOR, *PUKW_DEVICE_DESCRIPTOR, *LPUKW_DEVICE_DESCRIPTOR;

/**
 * Structure containing control request information,
 * matching the device descriptor described in the USB specification
 * as closely as possible.
 *
 * Multi-byte fields should be in native CPU endianness.
 */
typedef struct {
	UINT8 bmRequestType;
	UINT8 bRequest;
	UINT16 wValue;
	UINT16 wIndex;
	UINT16 wLength;
} UKW_CONTROL_HEADER, *PUKW_CONTROL_HEADER, *LPUKW_CONTROL_HEADER;

// Collection of flags which can be used when issuing transfer requests
/* Indicates that the transfer direction is 'in' */
#define UKW_TF_IN_TRANSFER        0x00000001
/* Indicates that the transfer direction is 'out' */
#define UKW_TF_OUT_TRANSFER       0x00000000
/* Specifies that the transfer should complete as soon as possible,
 * even if no OVERLAPPED structure has been provided. */
#define UKW_TF_NO_WAIT            0x00000100
/* Indicates that transfers shorter than the buffer are ok */
#define UKW_TF_SHORT_TRANSFER_OK  0x00000200
#define UKW_TF_SEND_TO_DEVICE     0x00010000
#define UKW_TF_SEND_TO_INTERFACE  0x00020000
#define UKW_TF_SEND_TO_ENDPOINT   0x00040000
/* Don't block when waiting for memory allocations */
#define UKW_TF_DONT_BLOCK_FOR_MEM 0x00080000

/* Value to use when dealing with configuration values, such as UkwGetConfigDescriptor, 
 * to specify the currently active configuration for the device. */
#define UKW_ACTIVE_CONFIGURATION -1

/**
 * Returns the GUID of the USB Kernel Wrapper driver.
 *
 * This can be used to register for advertisement events, providing
 * notification of attached and detached devices.
 *
 * \return The GUID of the driver.
 */
ceusbkwrapper_API const GUID* WINAPI UkwDriverGUID();

/**
 * Opens a new handle to the USB Kernel Wrapper driver.
 * 
 * Will attempt to activate the driver if no suitable
 * builtin device is found.
 *
 * Further information about failures can be retrieved using GetLastError().
 * 
 * \return The handle to use in other API calls, or INVALID_HANDLE_VALUE on error.
 */
ceusbkwrapper_API HANDLE WINAPI UkwOpenDriver();

/**
 * Retrieves a list of devices into a pre-allocated list.
 * 
 * To determine if all devices have been retrieved it's necessary to check that
 * lpActualSize is less than Size. If lpActualSize is equal to Size then
 * it might be necessary to release the device list and try again with
 * a larger list size.
 * 
 * Further information about failures can be retrieved using GetLastError().
 * \param hDriver [in] A driver handle opened by calling UkwOpenDriver().
 * \param lpList [in] A pre-allocated list for devices.
 * \param Size [in] The size of the list in lpList.
 * \param lpActualSize [out] On success this will contain the number of entries in the list
 * \return TRUE on success, or FALSE on failure.
 */
ceusbkwrapper_API BOOL WINAPI UkwGetDeviceList(
	HANDLE hDriver,
	LPUKW_DEVICE lpList,
	DWORD Size,
	LPDWORD lpActualSize);

/**
 * Releases a list of devices from a pre-allocated list.
 * 
 * \param hDriver [in] A driver handle opened by calling UkwOpenDriver().
 * \param lpList [in] A pre-allocated list for devices.
 * \param Size [in] The size of the list in lpList.
 */
ceusbkwrapper_API void WINAPI UkwReleaseDeviceList(
	HANDLE hDriver,
	LPUKW_DEVICE lpList,
	DWORD Size);

/**
 * Retrieves the bus address and device address of a given device.
 *
 * The bus and device address numbers returned by this API will not
 * necessarily reflect the USB bus and device address if the USB 
 * system doesn't support retrieving them. If the system doesn't
 * support retrieval of the bus and device address then a unique
 * value with similar properties will be chosen.
 *
 * The session ID returned by this method is incremented for each device
 * attached to a system. This allows it to be used, in combination with the
 * bus and device address, to distinguish between devices.
 *
 * \param lpDevice [in] A device retrieved using UkwGetDeviceList().
 * \param lpBus [out] On return this will contain the bus number. This parameter is optional.
 * \param lpDevAddr [out] On return this will contain the device address. This parameter is optional.
 * \param lpSessionId [out] On return this will contain a unique session identifier. This parameter is optional.
 * \return TRUE on success, or FALSE on failure.
 */
ceusbkwrapper_API BOOL WINAPI UkwGetDeviceAddress(
	UKW_DEVICE lpDevice,
	unsigned char* lpBus,
	unsigned char* lpDevAddr,
	unsigned long* lpSessionId
	);

/**
 * Retrieves the active configuration of a given device.
 *
 * \param lpDevice [in] A device retrieved using UkwGetDeviceList().
 * \param pConfig [out] A pointer to a UCHAR to fill with the current active configuration.
 * \return TRUE on success, or FALSE on failure.
 */
ceusbkwrapper_API BOOL WINAPI UkwGetConfig(
	UKW_DEVICE lpDevice,
	PUCHAR pConfig);

/**
 * Retrieves the device descriptor of a given device.
 *
 * \param lpDevice [in] A device retrieved using UkwGetDeviceList().
 * \param config [in] The configuration to set on the device.
 * \return TRUE on success, or FALSE on failure.
 */
ceusbkwrapper_API BOOL WINAPI UkwSetConfig(
	UKW_DEVICE lpDevice,
	UCHAR config);

/**
 * Retrieves the device descriptor of a given device.
 *
 * \param lpDevice [in] A device retrieved using UkwGetDeviceList().
 * \param lpDeviceDescriptor [out] A pointer to a device descriptor to fill with information.
 * \return TRUE on success, or FALSE on failure.
 */
ceusbkwrapper_API BOOL WINAPI UkwGetDeviceDescriptor(
	UKW_DEVICE lpDevice,
	LPUKW_DEVICE_DESCRIPTOR lpDeviceDescriptor
	);

/**
 * Retrieves the config descriptor of a given device.
 *
 * If a configuration value of UKW_ACTIVE_CONFIGURATION is used then the
 * descriptor for the currently active configuration will be retrieved.
 *
 * \param lpDevice [in] A device retrieved using UkwGetDeviceList().
 * \param dwConfig [in] The configuration index (not bConfigurationNumber) of the descriptor to retrieve.
 * \param lpBuffer [in] Buffer to contain the descriptor.
 * \param dwBufferSize [in] Size of lpBuffer parameter.
 * \param lpActualSize [out] The actual number of bytes written.
 * \return TRUE on success, or FALSE on failure.
 */
ceusbkwrapper_API BOOL WINAPI UkwGetConfigDescriptor(
	UKW_DEVICE lpDevice,
	DWORD dwConfig,
	LPVOID lpBuffer,
	DWORD dwBufferSize,
	LPDWORD lpActualSize
	);

/**
 * Clears the halt and stall status on the provided endpoint.
 *
 * This clears both the host and remote state of the endpoint.
 *
 * The interface should have been previously claimed before calling this.
 *
 * \param lpDevice [in] A device retrieved using UkwGetDeviceList().
 * \param endpoint [in] The endpoint to clear.
 * \return TRUE on success, or FALSE on failure.
 */
ceusbkwrapper_API BOOL WINAPI UkwClearHalt(
	UKW_DEVICE lpDevice,
	UCHAR endpoint);

/**
 * Checks the halt status on the provided endpoint.
 *
 * This only checks the host state of the endpoint. If the remote device
 * stall state needs to be checked then a control transfer for GET_STATUS
 * should be issued to the device.
 *
 * The interface for the endpoint should have been previously claimed
 * before calling this.
 *
 * \param lpDevice [in] A device retrieved using UkwGetDeviceList().
 * \param endpoint [in] The endpoint to check for the halt status.
 * \param lpHalted [out] Set to TRUE if the endpoint is halted, FALSE otherwise.
 * \return TRUE on success, or FALSE on failure.
 */
ceusbkwrapper_API BOOL WINAPI UkwIsPipeHalted(
	UKW_DEVICE lpDevice,
	UCHAR endpoint,
	LPBOOL lpHalted);

/**
 * Claims an interface from the device.
 *
 * This does not have any corresponding bus I/O, it is just a way to ensure
 * the application has exclusive access to the interface and its endpoints.
 *
 * \param lpDevice [in] A device retrieved using UkwGetDeviceList().
 * \param dwInterface [in] The interface number to claim.
 * \return TRUE on success, or FALSE on failure.
 */
ceusbkwrapper_API BOOL WINAPI UkwClaimInterface(
	UKW_DEVICE lpDevice,
	DWORD dwInterface);

/**
 * Release an interface from the device.
 *
 * This does not have any corresponding bus I/O, it is just a way to ensure
 * the application has released exclusive access to the interface and its endpoints.
 *
 * \param lpDevice [in] A device retrieved using UkwGetDeviceList().
 * \param dwInterface [in] The interface number to release.
 * \return TRUE on success, or FALSE on failure.
 */
ceusbkwrapper_API BOOL WINAPI UkwReleaseInterface(
	UKW_DEVICE lpDevice,
	DWORD dwInterface);

/**
 * Sets the alternate setting for an interface of the device.
 *
 * \param lpDevice [in] A device retrieved using UkwGetDeviceList().
 * \param dwInterface [in] The interface number to modify the alternate setting.
 * \param dwAlternateSetting [in] The alternate setting to switch to.
 * \return TRUE on success, or FALSE on failure.
 */
ceusbkwrapper_API BOOL WINAPI UkwSetInterfaceAlternateSetting(
	UKW_DEVICE lpDevice,
	DWORD dwInterface,
	DWORD dwAlternateSetting);

/**
 * Closes a previously opened driver handle.
 *
 * Will attempt to deactivate the driver if it was
 * activated by UkwOpenDriver().
 * 
 * \param hDriver [in] The handle to close.
 */
ceusbkwrapper_API void WINAPI UkwCloseDriver(HANDLE hDriver);

/**
 * Attempts to cancel a pending asynchronous transfer.
 *
 * If dwFlags is 0 then this will only return once the request has
 * been cancelled. If dwFlags is UKW_RF_NO_WAIT then this function
 * will return immediately.
 *
 * \param lpDevice [in] A device retrieved using UkwGetDeviceList()
 * \param lpOverlapped [in] The overlapped structure passed to the transfer to cancel
 * \param dwFlags [in] Either 0 or UKW_TF_NO_WAIT.
 * \return TRUE on success, or FALSE on failure.
 */
ceusbkwrapper_API BOOL WINAPI UkwCancelTransfer(
	UKW_DEVICE lpDevice,
	LPOVERLAPPED lpOverlapped,
	DWORD dwFlags
	);

/**
 * Starts a control transfer with the a USB device.
 * 
 * The direction of the transfer can be controlled by the value of dwFlags.
 * If UKW_TF_IN_TRANSFER is set then data will be written to the buffer
 * provided in lpDataBuffer. If UKW_TF_OUT_TRANSFER is used then
 * data will be read from the buffer provided in lpDataBuffer.
 * 
 * The UKW_TF_NO_WAIT flag is ignored if lpOverlapped is not NULL.
 *
 * If the UKW_TF_NO_WAIT flag is provided then pBytesTransferred will
 * be ignored.
 * 
 * \param lpDevice [in] A device retrieved using UkwGetDeviceList()
 * \param dwFlags [in] A bitwise or combination of the UKW_TF_* flags.
 * \param lpHeader [in] An 8 byte control header. See the USB specification for the format.
 * \param lpDataBuffer [in] Pointer to a data buffer.
 * \param dwDataBufferSize [in] Size of the provided data buffer.
 * \param pBytesTransferred [out] Optional parameter which will be set to the number of bytes transferred on success.
 * \param lpOverlapped [in] Optional parameter. If specified then request will be asynchronous.
 * \return TRUE on success, or FALSE on failure.
 */
ceusbkwrapper_API BOOL WINAPI UkwIssueControlTransfer(
	UKW_DEVICE lpDevice,
	DWORD dwFlags,
	LPUKW_CONTROL_HEADER lpHeader,
	LPVOID lpDataBuffer,
	DWORD dwDataBufferSize,
	LPDWORD pBytesTransferred,
	LPOVERLAPPED lpOverlapped
	);

/**
 * Starts a bulk transfer with the a USB device.
 * 
 * The direction of the transfer can be controlled by the value of dwFlags.
 * If UKW_TF_IN_TRANSFER is set then data will be written to the buffer
 * provided in lpDataBuffer. If UKW_TF_OUT_TRANSFER is used then
 * data will be read from the buffer provided in lpDataBuffer.
 * 
 * The UKW_TF_NO_WAIT flag is ignored if lpOverlapped is not NULL.
 *
 * If the UKW_TF_NO_WAIT flag is provided then pBytesTransferred will
 * be ignored.
 * 
 * \param lpDevice [in] A device retrieved using UkwGetDeviceList()
 * \param dwFlags [in] A bitwise or combination of the UKW_TF_* flags.
 * \param Endpoint [in] The endpoint to send the bulk transfer to.
 * \param lpDataBuffer [in] Pointer to a data buffer.
 * \param dwDataBufferSize [in] Size of the provided data buffer.
 * \param pBytesTransferred [out] Optional parameter which will be set to the number of bytes transferred on success.
 * \param lpOverlapped [in] Optional parameter. If specified then request will be asynchronous.
 * \return TRUE on success, or FALSE on failure.
 */
ceusbkwrapper_API BOOL WINAPI UkwIssueBulkTransfer(
	UKW_DEVICE lpDevice,
	DWORD dwFlags,
	UCHAR Endpoint,
	LPVOID lpDataBuffer,
	DWORD dwDataBufferSize,
	LPDWORD pBytesTransferred,
	LPOVERLAPPED lpOverlapped
	);

/**
 * Resets a USB device.
 *
 * This will only succeed if the kernel driver has bound to the device, and not
 * just a specific interface.
 *
 * \param lpDevice [in] A device retrieved using UkwGetDeviceList()
 */
ceusbkwrapper_API BOOL WINAPI UkwResetDevice(
	UKW_DEVICE lpDevice);

/**
 * Re-enumerates a USB device.
 *
 * This will only succeed if the kernel driver has bound to the device, and not
 * just a specific interface.
 *
 * \param lpDevice [in] A device retrieved using UkwGetDeviceList()
 */
ceusbkwrapper_API BOOL WINAPI UkwReenumerateDevice(
	UKW_DEVICE lpDevice);

/**
 * Determine if a kernel driver is active on an interface.
 *
 * \param lpDevice [in] A device retrieved using UkwGetDeviceList()
 * \param dwInterface [in] The interface to check
 * \param active [out] Set to true if a kernel driver is active.
 */
ceusbkwrapper_API BOOL WINAPI UkwKernelDriverActive(
	UKW_DEVICE lpDevice, DWORD dwInterface, PBOOL active);

/**
 * Attach a kernel driver to an interface.
 *
 * The OS searches for an appropriate driver for the interface.
 *
 * \param lpDevice [in] A device retrieved using UkwGetDeviceList()
 * \param dwInterface [in] The interface to attach
 */
ceusbkwrapper_API BOOL WINAPI UkwAttachKernelDriver(
	UKW_DEVICE lpDevice, DWORD dwInterface);

/**
 * Detaches a kernel driver from an interface.
 *
 * \param lpDevice [in] A device retrieved using UkwGetDeviceList()
 * \param dwInterface [in] The interface to detach
 */
ceusbkwrapper_API BOOL WINAPI UkwDetachKernelDriver(
	UKW_DEVICE lpDevice, DWORD dwInterface);

#ifdef __cplusplus
}
#endif

#endif // CEUSBKWRAPPER_H