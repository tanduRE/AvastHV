/*++

Module Name:

    driver.c

Abstract:

    This file contains the driver entry points and callbacks.

Environment:

    Kernel-mode Driver Framework

--*/

#include "driver.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, AvastHVUnload)
#pragma alloc_text (PAGE, AvastHVNtClose)
#endif

#define SSDT_INDEX_NTCLOSE 15

typedef NTSTATUS(NTAPI* NtClose_t)(_In_ HANDLE Handle);
NtClose_t pNtClose = NULL;

typedef NTSTATUS(NTAPI* AvastHyperVisorHook_t)(_In_ ULONG index, _In_ ULONG_PTR handler, _In_ PVOID orgfunc);
AvastHyperVisorHook_t pAvastHyperVisorHook = NULL;



NTSTATUS NTAPI AvastHVNtClose(_In_ HANDLE Handle)
{
	DbgPrint("NtClose call\n");
	return pNtClose(Handle);
}

VOID AvastHVUnload(_In_ PDRIVER_OBJECT DriverObject)
{
	UNREFERENCED_PARAMETER(DriverObject);
}

NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
	UNREFERENCED_PARAMETER(RegistryPath);

	NTSTATUS ntStatus				= STATUS_SUCCESS;
	PDEVICE_OBJECT pDeviceObject	= NULL;
	PFILE_OBJECT pFileObject		= NULL;
	UNICODE_STRING usAswVmm			= { 0 };
	UNICODE_STRING usNtClose		= { 0 };
	PIRP pIrp						= NULL;
	IO_STATUS_BLOCK ioStatus		= { 0 };
	KEVENT kEvent					= { 0 };
	ULONG_PTR dwAvastHVFuncs[17]	= { 0 };
	ULONG dwAvastCommand			= 6;

	DriverObject->DriverUnload = AvastHVUnload;

	RtlInitUnicodeString(&usAswVmm, L"\\Device\\aswVmm");
	RtlInitUnicodeString(&usNtClose, L"NtClose");

	pNtClose = (NtClose_t)(ULONG_PTR)MmGetSystemRoutineAddress(&usNtClose);

	KeInitializeEvent(&kEvent, SynchronizationEvent, FALSE);

	ntStatus = IoGetDeviceObjectPointer(&usAswVmm, 1, &pFileObject, &pDeviceObject);

	if (NT_SUCCESS(ntStatus))
	{
		DbgPrint("IoGetDeviceObjectPointer successfully\n");

		pIrp = IoBuildDeviceIoControlRequest(0xA000E804,
			pDeviceObject,
			&dwAvastCommand,
			4,
			&dwAvastHVFuncs,
			sizeof(dwAvastHVFuncs),
			FALSE,
			&kEvent,
			&ioStatus);

		if (pIrp)
		{
			if (IofCallDriver(pDeviceObject, pIrp) == STATUS_PENDING)
			{
				KeWaitForSingleObject(&kEvent, Executive, KernelMode, FALSE, 0);
			}

			DbgPrint("Print Avast Hypervisor functions\n");
			DbgPrint("--------------------------------\n");

			for (int i = 0; i < 17; i++)
				DbgPrint("[%i] -> 0x%p\n", i, dwAvastHVFuncs[i]);

			DbgPrint("--------------------------------\n");


			pAvastHyperVisorHook = (AvastHyperVisorHook_t)(dwAvastHVFuncs[3]);


			NTSTATUS ntStatusHook = pAvastHyperVisorHook(SSDT_INDEX_NTCLOSE, (ULONG_PTR)AvastHVNtClose, &pNtClose);

			DbgPrint("Avast Hook Status 0x%X\n", ntStatusHook);

		}
		else
		{
			ntStatus = STATUS_UNSUCCESSFUL;
			DbgPrint("IoBuildDeviceIoControlRequest Error = 0x%X\n", ioStatus.Status);
		}
	}
	else
	{
		DbgPrint("IoGetDeviceObjectPointer Error = 0x%X\n", ntStatus);
	}

	return ntStatus;
}
