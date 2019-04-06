/*++

Module Name:

    driver.h

Abstract:

    This file contains the driver definitions.

Environment:

    Kernel-mode Driver Framework

--*/

#define INITGUID

#include <ntddk.h>
#include <wdf.h>


EXTERN_C_START

//
// WDFDRIVER Events
//

NTSTATUS NTAPI AvastHVNtClose(HANDLE Handle);
VOID AvastHVUnload(IN PDRIVER_OBJECT DriverObject);
DRIVER_INITIALIZE DriverEntry;

EXTERN_C_END
