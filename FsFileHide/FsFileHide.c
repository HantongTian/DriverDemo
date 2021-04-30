/*++

Module Name:

    FsFileHide.c

Abstract:

    This is the main module of the FsFileHide miniFilter driver.

Environment:

    Kernel mode

--*/

#include <fltKernel.h>
#include <dontuse.h>
#include <suppress.h>
#include "FsFileHide.h"
#include <ntstrsafe.h>

#pragma prefast(disable:__WARNING_ENCODE_MEMBER_FUNCTION_POINTER, "Not valid for kernel mode drivers")

#pragma warning(disable:4133)


PFLT_FILTER gFilterHandle;
ULONG_PTR OperationStatusCtx = 1;

#define PTDBG_TRACE_ROUTINES            0x00000001
#define PTDBG_TRACE_OPERATION_STATUS    0x00000002

ULONG gTraceFlags = 3;

ULONG gLogLevel = 3;


#define  FSTAG  '1tht'

#define PT_DBG_PRINT( _dbgLevel, _string )          \
    (FlagOn(gTraceFlags,(_dbgLevel)) ?              \
        DbgPrint _string :                          \
        ((int)0))

#define LOG_PRINT( _dbgLevel, _string, ...)          \
    ((_dbgLevel)<gTraceFlags) ?              \
        DbgPrint (_string,## __VA_ARGS__) :                          \
        ((int)0)

/*************************************************************************
    Prototypes
*************************************************************************/

EXTERN_C_START

DRIVER_INITIALIZE DriverEntry;
NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    );

NTSTATUS
FsFileHideInstanceSetup (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_SETUP_FLAGS Flags,
    _In_ DEVICE_TYPE VolumeDeviceType,
    _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
    );

VOID
FsFileHideInstanceTeardownStart (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
    );

VOID
FsFileHideInstanceTeardownComplete (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
    );

NTSTATUS
FsFileHideUnload (
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
    );

NTSTATUS
FsFileHideInstanceQueryTeardown (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
    );

FLT_PREOP_CALLBACK_STATUS
FsFileHidePreOperation (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    );

VOID
FsFileHideOperationStatusCallback (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PFLT_IO_PARAMETER_BLOCK ParameterSnapshot,
    _In_ NTSTATUS OperationStatus,
    _In_ PVOID RequesterContext
    );

FLT_POSTOP_CALLBACK_STATUS
FsFileHidePostOperation (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    );

//����IRP_MJ_DIRECTORY_CONROL postOperation ������
FLT_POSTOP_CALLBACK_STATUS
MyDirControlPostOperation(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_opt_ PVOID CompletionContext,
	_In_ FLT_POST_OPERATION_FLAGS Flags
);

FLT_PREOP_CALLBACK_STATUS
FsFileHidePreOperationNoPostOperation (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    );

BOOLEAN
FsFileHideDoRequestOperationStatus(
    _In_ PFLT_CALLBACK_DATA Data
    );

EXTERN_C_END

//
//  Assign text sections for each routine.
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, FsFileHideUnload)
#pragma alloc_text(PAGE, FsFileHideInstanceQueryTeardown)
#pragma alloc_text(PAGE, FsFileHideInstanceSetup)
#pragma alloc_text(PAGE, FsFileHideInstanceTeardownStart)
#pragma alloc_text(PAGE, FsFileHideInstanceTeardownComplete)
#endif

//
//  operation registration
//

CONST FLT_OPERATION_REGISTRATION Callbacks[] = {

#if 0 // TODO - List all of the requests to filter.
    { IRP_MJ_CREATE,
      0,
      FsFileHidePreOperation,
      FsFileHidePostOperation },

    { IRP_MJ_CREATE_NAMED_PIPE,
      0,
      FsFileHidePreOperation,
      FsFileHidePostOperation },

    { IRP_MJ_CLOSE,
      0,
      FsFileHidePreOperation,
      FsFileHidePostOperation },

    { IRP_MJ_READ,
      0,
      FsFileHidePreOperation,
      FsFileHidePostOperation },

    { IRP_MJ_WRITE,
      0,
      FsFileHidePreOperation,
      FsFileHidePostOperation },

    { IRP_MJ_QUERY_INFORMATION,
      0,
      FsFileHidePreOperation,
      FsFileHidePostOperation },

    { IRP_MJ_SET_INFORMATION,
      0,
      FsFileHidePreOperation,
      FsFileHidePostOperation },

    { IRP_MJ_QUERY_EA,
      0,
      FsFileHidePreOperation,
      FsFileHidePostOperation },

    { IRP_MJ_SET_EA,
      0,
      FsFileHidePreOperation,
      FsFileHidePostOperation },

    { IRP_MJ_FLUSH_BUFFERS,
      0,
      FsFileHidePreOperation,
      FsFileHidePostOperation },

    { IRP_MJ_QUERY_VOLUME_INFORMATION,
      0,
      FsFileHidePreOperation,
      FsFileHidePostOperation },

    { IRP_MJ_SET_VOLUME_INFORMATION,
      0,
      FsFileHidePreOperation,
      FsFileHidePostOperation },
#endif

    { IRP_MJ_DIRECTORY_CONTROL,
      0,
	  NULL,
	  MyDirControlPostOperation },
#if 0
    { IRP_MJ_FILE_SYSTEM_CONTROL,
      0,
      FsFileHidePreOperation,
      FsFileHidePostOperation },

    { IRP_MJ_DEVICE_CONTROL,
      0,
      FsFileHidePreOperation,
      FsFileHidePostOperation },

    { IRP_MJ_INTERNAL_DEVICE_CONTROL,
      0,
      FsFileHidePreOperation,
      FsFileHidePostOperation },

    { IRP_MJ_SHUTDOWN,
      0,
      FsFileHidePreOperationNoPostOperation,
      NULL },                               //post operations not supported

    { IRP_MJ_LOCK_CONTROL,
      0,
      FsFileHidePreOperation,
      FsFileHidePostOperation },

    { IRP_MJ_CLEANUP,
      0,
      FsFileHidePreOperation,
      FsFileHidePostOperation },

    { IRP_MJ_CREATE_MAILSLOT,
      0,
      FsFileHidePreOperation,
      FsFileHidePostOperation },

    { IRP_MJ_QUERY_SECURITY,
      0,
      FsFileHidePreOperation,
      FsFileHidePostOperation },

    { IRP_MJ_SET_SECURITY,
      0,
      FsFileHidePreOperation,
      FsFileHidePostOperation },

    { IRP_MJ_QUERY_QUOTA,
      0,
      FsFileHidePreOperation,
      FsFileHidePostOperation },

    { IRP_MJ_SET_QUOTA,
      0,
      FsFileHidePreOperation,
      FsFileHidePostOperation },

    { IRP_MJ_PNP,
      0,
      FsFileHidePreOperation,
      FsFileHidePostOperation },

    { IRP_MJ_ACQUIRE_FOR_SECTION_SYNCHRONIZATION,
      0,
      FsFileHidePreOperation,
      FsFileHidePostOperation },

    { IRP_MJ_RELEASE_FOR_SECTION_SYNCHRONIZATION,
      0,
      FsFileHidePreOperation,
      FsFileHidePostOperation },

    { IRP_MJ_ACQUIRE_FOR_MOD_WRITE,
      0,
      FsFileHidePreOperation,
      FsFileHidePostOperation },

    { IRP_MJ_RELEASE_FOR_MOD_WRITE,
      0,
      FsFileHidePreOperation,
      FsFileHidePostOperation },

    { IRP_MJ_ACQUIRE_FOR_CC_FLUSH,
      0,
      FsFileHidePreOperation,
      FsFileHidePostOperation },

    { IRP_MJ_RELEASE_FOR_CC_FLUSH,
      0,
      FsFileHidePreOperation,
      FsFileHidePostOperation },

    { IRP_MJ_FAST_IO_CHECK_IF_POSSIBLE,
      0,
      FsFileHidePreOperation,
      FsFileHidePostOperation },

    { IRP_MJ_NETWORK_QUERY_OPEN,
      0,
      FsFileHidePreOperation,
      FsFileHidePostOperation },

    { IRP_MJ_MDL_READ,
      0,
      FsFileHidePreOperation,
      FsFileHidePostOperation },

    { IRP_MJ_MDL_READ_COMPLETE,
      0,
      FsFileHidePreOperation,
      FsFileHidePostOperation },

    { IRP_MJ_PREPARE_MDL_WRITE,
      0,
      FsFileHidePreOperation,
      FsFileHidePostOperation },

    { IRP_MJ_MDL_WRITE_COMPLETE,
      0,
      FsFileHidePreOperation,
      FsFileHidePostOperation },

    { IRP_MJ_VOLUME_MOUNT,
      0,
      FsFileHidePreOperation,
      FsFileHidePostOperation },

    { IRP_MJ_VOLUME_DISMOUNT,
      0,
      FsFileHidePreOperation,
      FsFileHidePostOperation },

#endif // TODO

    { IRP_MJ_OPERATION_END }
};

//
//  This defines what we want to filter with FltMgr
//

CONST FLT_REGISTRATION FilterRegistration = {

    sizeof( FLT_REGISTRATION ),         //  Size
    FLT_REGISTRATION_VERSION,           //  Version
    0,                                  //  Flags

    NULL,                               //  Context
    Callbacks,                          //  Operation callbacks

    FsFileHideUnload,                           //  MiniFilterUnload

    FsFileHideInstanceSetup,                    //  InstanceSetup
    FsFileHideInstanceQueryTeardown,            //  InstanceQueryTeardown
    NULL/*FsFileHideInstanceTeardownStart*/,            //  InstanceTeardownStart
    NULL/*FsFileHideInstanceTeardownComplete*/,         //  InstanceTeardownComplete

    NULL,                               //  GenerateFileName
    NULL,                               //  GenerateDestinationFileName
    NULL                                //  NormalizeNameComponent

};



NTSTATUS
FsFileHideInstanceSetup (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_SETUP_FLAGS Flags,
    _In_ DEVICE_TYPE VolumeDeviceType,
    _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
    )
/*++

Routine Description:

    This routine is called whenever a new instance is created on a volume. This
    gives us a chance to decide if we need to attach to this volume or not.

    If this routine is not defined in the registration structure, automatic
    instances are always created.

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance and its associated volume.

    Flags - Flags describing the reason for this attach request.

Return Value:

    STATUS_SUCCESS - attach
    STATUS_FLT_DO_NOT_ATTACH - do not attach

--*/
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );
    UNREFERENCED_PARAMETER( VolumeDeviceType );
    UNREFERENCED_PARAMETER( VolumeFilesystemType );

    PAGED_CODE();

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("FsFileHide!FsFileHideInstanceSetup: Entered\n") );

    return STATUS_SUCCESS;
}


NTSTATUS
FsFileHideInstanceQueryTeardown (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
    )
/*++

Routine Description:

    This is called when an instance is being manually deleted by a
    call to FltDetachVolume or FilterDetach thereby giving us a
    chance to fail that detach request.

    If this routine is not defined in the registration structure, explicit
    detach requests via FltDetachVolume or FilterDetach will always be
    failed.

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance and its associated volume.

    Flags - Indicating where this detach request came from.

Return Value:

    Returns the status of this operation.

--*/
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("FsFileHide!FsFileHideInstanceQueryTeardown: Entered\n") );

    return STATUS_SUCCESS;
}


VOID
FsFileHideInstanceTeardownStart (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
    )
/*++

Routine Description:

    This routine is called at the start of instance teardown.

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance and its associated volume.

    Flags - Reason why this instance is being deleted.

Return Value:

    None.

--*/
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("FsFileHide!FsFileHideInstanceTeardownStart: Entered\n") );
}


VOID
FsFileHideInstanceTeardownComplete (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
    )
/*++

Routine Description:

    This routine is called at the end of instance teardown.

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance and its associated volume.

    Flags - Reason why this instance is being deleted.

Return Value:

    None.

--*/
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("FsFileHide!FsFileHideInstanceTeardownComplete: Entered\n") );
}


/*************************************************************************
    MiniFilter initialization and unload routines.
*************************************************************************/

NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    This is the initialization routine for this miniFilter driver.  This
    registers with FltMgr and initializes all global data structures.

Arguments:

    DriverObject - Pointer to driver object created by the system to
        represent this driver.

    RegistryPath - Unicode string identifying where the parameters for this
        driver are located in the registry.

Return Value:

    Routine can return non success error codes.

--*/
{
    NTSTATUS status;

    UNREFERENCED_PARAMETER( RegistryPath );

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("FsFileHide!DriverEntry: Entered\n") );

    //
    //  Register with FltMgr to tell it our callback routines
    //

    status = FltRegisterFilter( DriverObject,
                                &FilterRegistration,
                                &gFilterHandle );

    FLT_ASSERT( NT_SUCCESS( status ) );

    if (NT_SUCCESS( status )) {

        //
        //  Start filtering i/o
        //
		PT_DBG_PRINT(PTDBG_TRACE_ROUTINES,
			("FsFileHide!DriverEntry: FltRegisterFilter success\n"));

        status = FltStartFiltering( gFilterHandle );

        if (!NT_SUCCESS( status )) {
			LOG_PRINT(LOG_ERROR, "FsFileHide!DriverEntry:FltStartFiltering error[%d] ", status);
            FltUnregisterFilter( gFilterHandle );
        }
    }

    return status;
}

NTSTATUS
FsFileHideUnload (
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
    )
/*++

Routine Description:

    This is the unload routine for this miniFilter driver. This is called
    when the minifilter is about to be unloaded. We can fail this unload
    request if this is not a mandatory unload indicated by the Flags
    parameter.

Arguments:

    Flags - Indicating if this is a mandatory unload.

Return Value:

    Returns STATUS_SUCCESS.

--*/
{
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("FsFileHide!FsFileHideUnload: Entered\n") );

    FltUnregisterFilter( gFilterHandle );

    return STATUS_SUCCESS;
}


/*************************************************************************
    MiniFilter callback routines.
*************************************************************************/
FLT_PREOP_CALLBACK_STATUS
FsFileHidePreOperation (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )
/*++

Routine Description:

    This routine is a pre-operation dispatch routine for this miniFilter.

    This is non-pageable because it could be called on the paging path

Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - The context for the completion routine for this
        operation.

Return Value:

    The return value is the status of the operation.

--*/
{
    NTSTATUS status;

    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CompletionContext );

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("FsFileHide!FsFileHidePreOperation: Entered\n") );

    //
    //  See if this is an operation we would like the operation status
    //  for.  If so request it.
    //
    //  NOTE: most filters do NOT need to do this.  You only need to make
    //        this call if, for example, you need to know if the oplock was
    //        actually granted.
    //

    if (FsFileHideDoRequestOperationStatus( Data )) {

        status = FltRequestOperationStatusCallback( Data,
                                                    FsFileHideOperationStatusCallback,
                                                    (PVOID)(++OperationStatusCtx) );
        if (!NT_SUCCESS(status)) {

            PT_DBG_PRINT( PTDBG_TRACE_OPERATION_STATUS,
                          ("FsFileHide!FsFileHidePreOperation: FltRequestOperationStatusCallback Failed, status=%08x\n",
                           status) );
        }
    }

    // This template code does not do anything with the callbackData, but
    // rather returns FLT_PREOP_SUCCESS_WITH_CALLBACK.
    // This passes the request down to the next miniFilter in the chain.

    return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}



VOID
FsFileHideOperationStatusCallback (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PFLT_IO_PARAMETER_BLOCK ParameterSnapshot,
    _In_ NTSTATUS OperationStatus,
    _In_ PVOID RequesterContext
    )
/*++

Routine Description:

    This routine is called when the given operation returns from the call
    to IoCallDriver.  This is useful for operations where STATUS_PENDING
    means the operation was successfully queued.  This is useful for OpLocks
    and directory change notification operations.

    This callback is called in the context of the originating thread and will
    never be called at DPC level.  The file object has been correctly
    referenced so that you can access it.  It will be automatically
    dereferenced upon return.

    This is non-pageable because it could be called on the paging path

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    RequesterContext - The context for the completion routine for this
        operation.

    OperationStatus -

Return Value:

    The return value is the status of the operation.

--*/
{
    UNREFERENCED_PARAMETER( FltObjects );

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("FsFileHide!FsFileHideOperationStatusCallback: Entered\n") );

    PT_DBG_PRINT( PTDBG_TRACE_OPERATION_STATUS,
                  ("FsFileHide!FsFileHideOperationStatusCallback: Status=%08x ctx=%p IrpMj=%02x.%02x \"%s\"\n",
                   OperationStatus,
                   RequesterContext,
                   ParameterSnapshot->MajorFunction,
                   ParameterSnapshot->MinorFunction,
                   FltGetIrpName(ParameterSnapshot->MajorFunction)) );
}


FLT_POSTOP_CALLBACK_STATUS
FsFileHidePostOperation (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    )
/*++

Routine Description:

    This routine is the post-operation completion routine for this
    miniFilter.

    This is non-pageable because it may be called at DPC level.

Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - The completion context set in the pre-operation routine.

    Flags - Denotes whether the completion is successful or is being drained.

Return Value:

    The return value is the status of the operation.

--*/
{
    UNREFERENCED_PARAMETER( Data );
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CompletionContext );
    UNREFERENCED_PARAMETER( Flags );

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("FsFileHide!FsFileHidePostOperation: Entered\n") );

	LOG_PRINT(LOG_INFO, "DirControl FsFileHidePostOperation Entered");

    return FLT_POSTOP_FINISHED_PROCESSING;
}

//cmd ������ʹ��dirʱ�ߵ����
void ParseFileBoth(PFLT_CALLBACK_DATA Data)
{
	if (Data->Iopb->Parameters.DirectoryControl.QueryDirectory.FileInformationClass != FileFullDirectoryInformation)
	{
		return;
	}

	int iChange = 0;
	int iRemoveAll = 1;
	ULONG nextOffset = 0;

	PFILE_FULL_DIR_INFORMATION pCurrentFileInfo = NULL;
	PFILE_FULL_DIR_INFORMATION pNextFileInfo = NULL;
	PFILE_FULL_DIR_INFORMATION pPreviousFileInfo = NULL;

	if (Data->Iopb->Parameters.DirectoryControl.QueryDirectory.MdlAddress != NULL)
	{
		LOG_PRINT(LOG_INFO, "DirControl MdlAddress != NULL");
		pCurrentFileInfo = (PFILE_FULL_DIR_INFORMATION)MmGetSystemAddressForMdlSafe(
			Data->Iopb->Parameters.DirectoryControl.QueryDirectory.MdlAddress,
			NormalPagePriority);
	}
	else
	{
		pCurrentFileInfo = (PFILE_FULL_DIR_INFORMATION)Data->Iopb->Parameters.DirectoryControl.QueryDirectory.DirectoryBuffer;
	}

	if (!pCurrentFileInfo)
	{
		LOG_PRINT(LOG_ERROR, "DirControl currentFileInfo is NULL");
		return;
	}

	pPreviousFileInfo = pCurrentFileInfo;

	wchar_t *pFileName = NULL;
	do
	{
		nextOffset = pCurrentFileInfo->NextEntryOffset;//��¼��һ���ڵ��ƫ����
		pNextFileInfo = (PFILE_FULL_DIR_INFORMATION)((PCHAR)(pCurrentFileInfo)+nextOffset);//��һ���ڵ�

		pFileName = ExAllocatePoolWithTag(PagedPool, pCurrentFileInfo->FileNameLength + sizeof(wchar_t), FSTAG);
		if (!pFileName)
		{
			LOG_PRINT(LOG_ERROR, "DirControl filename ExAllocatePool error!");
			continue;
		}
		RtlZeroMemory(pFileName, pCurrentFileInfo->FileNameLength + sizeof(wchar_t));

		RtlCopyMemory(pFileName, pCurrentFileInfo->FileName, pCurrentFileInfo->FileNameLength);

		if (0 == _wcsicmp(pFileName, L"test.txt"))
		{
			LOG_PRINT(LOG_INFO, "DirControl need hide[%S]", pFileName);
			if (nextOffset == 0)
			{
				pPreviousFileInfo->NextEntryOffset = 0;
				LOG_PRINT(LOG_INFO, "DirControl lastnode");
			}
			else
			{
				LOG_PRINT(LOG_INFO, "DirControl not lastnode, erase it!");
				pPreviousFileInfo->NextEntryOffset = (ULONG)((PCHAR)pCurrentFileInfo - (PCHAR)pPreviousFileInfo) + nextOffset;
			}
			iChange = 1;
		}
		else
		{
			pPreviousFileInfo = pCurrentFileInfo;
			iRemoveAll = 0;
		}

		ExFreePoolWithTag(pFileName, FSTAG);
		pFileName = NULL;

		pCurrentFileInfo = pNextFileInfo;

	} while (nextOffset != 0);

	//����޸��˽���������޸ĵ�����޸�״̬
	if (iChange)
	{
		if (iRemoveAll)
		{
			Data->IoStatus.Status = STATUS_NO_MORE_FILES;
		}
		else
		{
			FltSetCallbackDataDirty(Data);
		}
	}

	return;
}

void ParseFileIdBoth(PFLT_CALLBACK_DATA Data)
{
	if (Data->Iopb->Parameters.DirectoryControl.QueryDirectory.FileInformationClass != FileIdBothDirectoryInformation)
	{
		return;
	}

	int iChange = 0;
	int iRemoveAll = 1;
	ULONG nextOffset = 0;

	PFILE_ID_BOTH_DIR_INFORMATION pCurrentFileInfo = NULL;
	PFILE_ID_BOTH_DIR_INFORMATION pNextFileInfo = NULL;
	PFILE_ID_BOTH_DIR_INFORMATION pPreviousFileInfo = NULL;

	if (Data->Iopb->Parameters.DirectoryControl.QueryDirectory.MdlAddress != NULL)
	{
		LOG_PRINT(LOG_INFO, "DirControl MdlAddress != NULL");
		pCurrentFileInfo = (PFILE_ID_BOTH_DIR_INFORMATION)MmGetSystemAddressForMdlSafe(
			Data->Iopb->Parameters.DirectoryControl.QueryDirectory.MdlAddress,
			NormalPagePriority);
	}
	else
	{
		pCurrentFileInfo = (PFILE_ID_BOTH_DIR_INFORMATION)Data->Iopb->Parameters.DirectoryControl.QueryDirectory.DirectoryBuffer;
	}

	if (!pCurrentFileInfo)
	{
		LOG_PRINT(LOG_ERROR, "DirControl currentFileInfo is NULL");
		return;
	}

	pPreviousFileInfo = pCurrentFileInfo;

	wchar_t *pFileName = NULL;
	do
	{
		nextOffset = pCurrentFileInfo->NextEntryOffset;//��¼��һ���ڵ��ƫ����
		pNextFileInfo = (PFILE_ID_BOTH_DIR_INFORMATION)((PCHAR)(pCurrentFileInfo)+nextOffset);//��һ���ڵ�

		pFileName = ExAllocatePoolWithTag(PagedPool, pCurrentFileInfo->FileNameLength + sizeof(wchar_t), FSTAG);
		if (!pFileName)
		{
			LOG_PRINT(LOG_ERROR, "DirControl filename ExAllocatePool error!");
			continue;
		}
		RtlZeroMemory(pFileName, pCurrentFileInfo->FileNameLength + sizeof(wchar_t));

		RtlCopyMemory(pFileName, pCurrentFileInfo->FileName, pCurrentFileInfo->FileNameLength);

		if (0 == _wcsicmp(pFileName, L"test.txt"))
		{
			LOG_PRINT(LOG_INFO, "DirControl need hide[%S]", pFileName);
			if (nextOffset == 0)
			{
				pPreviousFileInfo->NextEntryOffset = 0;
				LOG_PRINT(LOG_INFO, "DirControl lastnode");
			}
			else
			{
				LOG_PRINT(LOG_INFO, "DirControl not lastnode, erase it!");
				pPreviousFileInfo->NextEntryOffset = (ULONG)((PCHAR)pCurrentFileInfo - (PCHAR)pPreviousFileInfo) + nextOffset;
			}
			iChange = 1;
		}
		else
		{
			pPreviousFileInfo = pCurrentFileInfo;
			iRemoveAll = 0;
		}

		ExFreePoolWithTag(pFileName, FSTAG);
		pFileName = NULL;

		pCurrentFileInfo = pNextFileInfo;

	} while (nextOffset != 0);


	//����޸��˽���������޸ĵ�����޸�״̬
	if (iChange)
	{
		if (iRemoveAll)
		{
			Data->IoStatus.Status = STATUS_NO_MORE_FILES;
		}
		else
		{
			FltSetCallbackDataDirty(Data);
		}
	}

	return;
}

//ʵ���ļ�����
FLT_POSTOP_CALLBACK_STATUS 
MyDirControlPostOperation(
	PFLT_CALLBACK_DATA Data, 
	PCFLT_RELATED_OBJECTS FltObjects,
	PVOID CompletionContext, 
	FLT_POST_OPERATION_FLAGS Flags)
{
	UNREFERENCED_PARAMETER(Data);
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(CompletionContext);
	UNREFERENCED_PARAMETER(Flags);

	PAGED_CODE();

//	PT_DBG_PRINT(PTDBG_TRACE_ROUTINES,
//		("FsFileHide!FsFileHidePostOperation: Entered\n"));

	//int iChange = 0;
	//int iRemoveAll = 1;
	//ULONG nextOffset = 0;
	wchar_t *pFileDir = NULL;
	PFLT_FILE_NAME_INFORMATION fileName = NULL;
	//PFILE_ID_BOTH_DIR_INFORMATION pCurrentFileInfo = NULL;
	//PFILE_ID_BOTH_DIR_INFORMATION pNextFileInfo = NULL;
	//PFILE_ID_BOTH_DIR_INFORMATION pPreviousFileInfo = NULL;
	UNICODE_STRING strFilePathName;

	do
	{
		if (!NT_SUCCESS(Data->IoStatus.Status))
		{
			//LOG_PRINT(LOG_INFO, "DirControl IoStatus.Status not sucess[%d]", Data->IoStatus.Status);
			break;
		}

		if ( (Data->Iopb->MinorFunction != IRP_MN_QUERY_DIRECTORY)
			|| (Data->Iopb->Parameters.DirectoryControl.QueryDirectory.Length <=0)
			)
		{
			//LOG_PRINT(LOG_INFO, "DirControl Data Info not ok, Minor is[%d], class [%d], length[%d]",
			//	IRP_MN_QUERY_DIRECTORY, FileIdBothDirectoryInformation, Data->Iopb->Parameters.DirectoryControl.QueryDirectory.Length );
			break;
		}

		if ((Data->Iopb->Parameters.DirectoryControl.QueryDirectory.FileInformationClass != FileIdBothDirectoryInformation)
			&& (Data->Iopb->Parameters.DirectoryControl.QueryDirectory.FileInformationClass != FileFullDirectoryInformation))
		{
			/*
			LOG_PRINT(LOG_INFO, "DirControl Data Info not ok, Minor is[%d], class [%d], length[%d]",
				IRP_MN_QUERY_DIRECTORY,
				Data->Iopb->Parameters.DirectoryControl.QueryDirectory.FileInformationClass,
				Data->Iopb->Parameters.DirectoryControl.QueryDirectory.Length );*/
			break;
		}

		NTSTATUS status;
		
		status = FltGetFileNameInformation(Data, \
			FLT_FILE_NAME_OPENED | FLT_FILE_NAME_QUERY_ALWAYS_ALLOW_CACHE_LOOKUP, \
			&fileName);
		if (!NT_SUCCESS(status))
		{
			LOG_PRINT(LOG_ERROR, "DirControl FltGetFileNameInformation error[%d]", status);
			break;
		}

		status = FltParseFileNameInformation(fileName);
		if (!NT_SUCCESS(status))
		{
			LOG_PRINT(LOG_ERROR, "DirControl FltParseFileNameInformation error[%d]", status);
			break;
		}

		pFileDir = ExAllocatePoolWithTag(PagedPool, fileName->Name.Length+sizeof(wchar_t)*2, FSTAG);
		if (!pFileDir)
		{
			LOG_PRINT(LOG_ERROR, "DirControl filedir ExAllocatePool error!");
			break;
		}
		RtlZeroMemory(pFileDir, fileName->Name.Length + sizeof(wchar_t) * 2);

		RtlCopyMemory(pFileDir, fileName->Name.Buffer, fileName->Name.Length);
		RtlInitUnicodeString(&strFilePathName, pFileDir);
		if (strFilePathName.Buffer[strFilePathName.Length / sizeof(wchar_t) - 1] != '\\')
		{
			RtlAppendUnicodeToString(&strFilePathName, L"\\");
		}

		if (0 == wcsstr(pFileDir, L"\\test\\"))
		{
			LOG_PRINT(LOG_INFO, "DirControl szFileDir[%wZ][%S]", strFilePathName, pFileDir);
			break;
		}

		if (Data->Iopb->Parameters.DirectoryControl.QueryDirectory.FileInformationClass == FileIdBothDirectoryInformation)
		{
			LOG_PRINT(LOG_INFO, "DirControl ParseFileIdBoth");
			ParseFileIdBoth(Data);
		}

		if (Data->Iopb->Parameters.DirectoryControl.QueryDirectory.FileInformationClass == FileFullDirectoryInformation)
		{
			LOG_PRINT(LOG_INFO, "DirControl ParseFileBoth");
			ParseFileBoth(Data);
		}

	} while (FALSE);

	if (fileName)
	{
		FltReleaseFileNameInformation(fileName);
	}

	if (pFileDir)
	{
		ExFreePoolWithTag(pFileDir, FSTAG);
	}
#if 0
		if (Data->Iopb->Parameters.DirectoryControl.QueryDirectory.MdlAddress != NULL)
		{
			LOG_PRINT(LOG_INFO, "DirControl MdlAddress != NULL");
			pCurrentFileInfo = (PFILE_ID_BOTH_DIR_INFORMATION)MmGetSystemAddressForMdlSafe(
				Data->Iopb->Parameters.DirectoryControl.QueryDirectory.MdlAddress,
				NormalPagePriority);
		}
		else
		{
			pCurrentFileInfo = (PFILE_ID_BOTH_DIR_INFORMATION)Data->Iopb->Parameters.DirectoryControl.QueryDirectory.DirectoryBuffer;
		}

		if (!pCurrentFileInfo)
		{
			LOG_PRINT(LOG_ERROR, "DirControl currentFileInfo is NULL");
			break;
		}

		pPreviousFileInfo = pCurrentFileInfo;

		wchar_t *pFileName = NULL;
		do
		{
			nextOffset = pCurrentFileInfo->NextEntryOffset;//��¼��һ���ڵ��ƫ����
			pNextFileInfo = (PFILE_ID_BOTH_DIR_INFORMATION)((PCHAR)(pCurrentFileInfo)+nextOffset);//��һ���ڵ�

			pFileName = ExAllocatePoolWithTag(PagedPool, pCurrentFileInfo->FileNameLength + sizeof(wchar_t), FSTAG);
			if (!pFileName)
			{
				LOG_PRINT(LOG_ERROR, "DirControl filename ExAllocatePool error!");
				continue;
			}
			RtlZeroMemory(pFileName, pCurrentFileInfo->FileNameLength + sizeof(wchar_t));

			RtlCopyMemory(pFileName, pCurrentFileInfo->FileName, pCurrentFileInfo->FileNameLength);

			if (0 == _wcsicmp(pFileName, L"test.txt") )
			{
				LOG_PRINT(LOG_INFO, "DirControl need hide[%S]", pFileName);
				if (nextOffset == 0)
				{
					pPreviousFileInfo->NextEntryOffset = 0;
					LOG_PRINT(LOG_INFO, "DirControl lastnode");
				}
				else
				{
					LOG_PRINT(LOG_INFO, "DirControl not lastnode, erase it!");
					pPreviousFileInfo->NextEntryOffset = (ULONG)((PCHAR)pCurrentFileInfo - (PCHAR)pPreviousFileInfo) + nextOffset;
				}
				iChange = 1;
			}
			else
			{
				pPreviousFileInfo = pCurrentFileInfo;
				iRemoveAll = 0;
			}

			ExFreePoolWithTag(pFileName, FSTAG);
			pFileName = NULL;

			pCurrentFileInfo = pNextFileInfo;

		} while (nextOffset != 0);

	} while (FALSE);

	if (pFileDir)
	{
		ExFreePoolWithTag(pFileDir, FSTAG);
	}

	if (fileName)
	{
		FltReleaseFileNameInformation(fileName);
	}

	//����޸��˽���������޸ĵ�����޸�״̬
	if (iChange)
	{
		if (iRemoveAll)
		{
			Data->IoStatus.Status = STATUS_NO_MORE_FILES;
		}
		else
		{
			FltSetCallbackDataDirty(Data);
		}
	}
#endif
	return FLT_POSTOP_FINISHED_PROCESSING;
}

FLT_PREOP_CALLBACK_STATUS
FsFileHidePreOperationNoPostOperation (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )
/*++

Routine Description:

    This routine is a pre-operation dispatch routine for this miniFilter.

    This is non-pageable because it could be called on the paging path

Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - The context for the completion routine for this
        operation.

Return Value:

    The return value is the status of the operation.

--*/
{
    UNREFERENCED_PARAMETER( Data );
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CompletionContext );

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("FsFileHide!FsFileHidePreOperationNoPostOperation: Entered\n") );

    // This template code does not do anything with the callbackData, but
    // rather returns FLT_PREOP_SUCCESS_NO_CALLBACK.
    // This passes the request down to the next miniFilter in the chain.

    return FLT_PREOP_SUCCESS_NO_CALLBACK;
}


BOOLEAN
FsFileHideDoRequestOperationStatus(
    _In_ PFLT_CALLBACK_DATA Data
    )
/*++

Routine Description:

    This identifies those operations we want the operation status for.  These
    are typically operations that return STATUS_PENDING as a normal completion
    status.

Arguments:

Return Value:

    TRUE - If we want the operation status
    FALSE - If we don't

--*/
{
    PFLT_IO_PARAMETER_BLOCK iopb = Data->Iopb;

    //
    //  return boolean state based on which operations we are interested in
    //

    return (BOOLEAN)

            //
            //  Check for oplock operations
            //

             (((iopb->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL) &&
               ((iopb->Parameters.FileSystemControl.Common.FsControlCode == FSCTL_REQUEST_FILTER_OPLOCK)  ||
                (iopb->Parameters.FileSystemControl.Common.FsControlCode == FSCTL_REQUEST_BATCH_OPLOCK)   ||
                (iopb->Parameters.FileSystemControl.Common.FsControlCode == FSCTL_REQUEST_OPLOCK_LEVEL_1) ||
                (iopb->Parameters.FileSystemControl.Common.FsControlCode == FSCTL_REQUEST_OPLOCK_LEVEL_2)))

              ||

              //
              //    Check for directy change notification
              //

              ((iopb->MajorFunction == IRP_MJ_DIRECTORY_CONTROL) &&
               (iopb->MinorFunction == IRP_MN_NOTIFY_CHANGE_DIRECTORY))
             );
}
