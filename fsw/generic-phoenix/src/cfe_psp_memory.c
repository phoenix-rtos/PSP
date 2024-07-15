/************************************************************************
 * NASA Docket No. GSC-18,719-1, and identified as “core Flight System: Bootes”
 *
 * Copyright (c) 2020 United States Government as represented by the
 * Administrator of the National Aeronautics and Space Administration.
 * All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ************************************************************************/

/******************************************************************************
** File:  cfe_psp_memory.c
**
**      OSX x86 Linux Version
**
** Purpose:
**   cFE PSP Memory related functions. This is the implementation of the cFE
**   memory areas that have to be preserved, and the API that is designed to allow
**   access to them. It also contains memory related routines to return the
**   address of the kernel code used in the cFE checksum.
**
** History:
**   2006/09/29  A. Cudmore      | Initial version for OS X
**
******************************************************************************/

/*
**  Include Files
*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>

/*
** cFE includes
*/
#include "common_types.h"
#include "osapi.h"

/*
** Types and prototypes for this module
*/
#include "cfe_psp.h"

/*
** PSP Specific defines
*/
#include "cfe_psp_config.h"
#include "cfe_psp_memory.h"

#include "target_config.h"


#define CFE_PSP_CDS_SIZE           (GLOBAL_CONFIGDATA.CfeConfig->CdsSize)
#define CFE_PSP_RESET_AREA_SIZE    (GLOBAL_CONFIGDATA.CfeConfig->ResetAreaSize)
#define CFE_PSP_USER_RESERVED_SIZE (GLOBAL_CONFIGDATA.CfeConfig->UserReservedSize)

typedef struct
{
    CFE_PSP_ReservedMemoryBootRecord_t BootRecord;
    CFE_PSP_ExceptionStorage_t         ExceptionStorage;
} CFE_PSP_PhoenixReservedAreaFixedLayout_t;

/*
** Internal prototypes for this module
*/
void CFE_PSP_InitCDS(void);
void CFE_PSP_InitResetArea(void);
void CFE_PSP_InitVolatileDiskMem(void);
void CFE_PSP_InitUserReservedArea(void);

/*
** Global variables
*/

void *ResetAreaPtr;
void *CDSAreaPtr;
void *UserAreaPtr;

CFE_PSP_ReservedMemoryMap_t CFE_PSP_ReservedMemoryMap;
/*
*********************************************************************************
** CDS related functions
*********************************************************************************
*/
/*
**
*/
/******************************************************************************
**
**  Purpose: This function is used by the ES startup code to initialize the
**            Critical Data store area
**
**
**  Arguments:
**    (none)
**
**  Return:
**    (none)
*/

void CFE_PSP_InitCDS(void)
{
    CFE_PSP_ReservedMemoryMap.CDSMemory.BlockPtr = malloc(CFE_PSP_USER_RESERVED_SIZE);
    if (CFE_PSP_ReservedMemoryMap.CDSMemory.BlockPtr == NULL)
    {
        printf("CFE_PSP: Error: Could not allocate User Reserved Area\n");
        CFE_PSP_Panic(CFE_PSP_ERROR);
    }
    CFE_PSP_ReservedMemoryMap.CDSMemory.BlockSize = CFE_PSP_CDS_SIZE;
}

/******************************************************************************
**
**  Purpose:
**   This is an internal function to delete the CDS Shared memory segment.
**
**  Arguments:
**    (none)
**
**  Return:
**    (none)
*/
void CFE_PSP_DeleteCDS(void)
{
    free(CFE_PSP_ReservedMemoryMap.CDSMemory.BlockPtr);
    CFE_PSP_ReservedMemoryMap.CDSMemory.BlockPtr = NULL;
}

/*----------------------------------------------------------------
 *
 * Implemented per public API
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
int32 CFE_PSP_GetCDSSize(uint32 *SizeOfCDS)
{
    int32 return_code;

    if (SizeOfCDS == NULL)
    {
        return_code = OS_ERROR;
    }
    else
    {
        *SizeOfCDS  = CFE_PSP_ReservedMemoryMap.CDSMemory.BlockSize;
        return_code = OS_SUCCESS;
    }
    return return_code;
}

/*----------------------------------------------------------------
 *
 * Implemented per public API
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
int32 CFE_PSP_WriteToCDS(const void *PtrToDataToWrite, uint32 CDSOffset, uint32 NumBytes)
{
    uint8 *CopyPtr;
    int32  return_code;

    if (PtrToDataToWrite == NULL)
    {
        return_code = OS_ERROR;
    }
    else
    {
        if ((CDSOffset < CFE_PSP_ReservedMemoryMap.CDSMemory.BlockSize) &&
            ((CDSOffset + NumBytes) <= CFE_PSP_ReservedMemoryMap.CDSMemory.BlockSize))
        {
            CopyPtr = CFE_PSP_ReservedMemoryMap.CDSMemory.BlockPtr;
            CopyPtr += CDSOffset;
            memcpy((char *)CopyPtr, (char *)PtrToDataToWrite, NumBytes);

            return_code = OS_SUCCESS;
        }
        else
        {
            return_code = OS_ERROR;
        }

    } /* end if PtrToDataToWrite == NULL */

    return return_code;
}

/*----------------------------------------------------------------
 *
 * Implemented per public API
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
int32 CFE_PSP_ReadFromCDS(void *PtrToDataToRead, uint32 CDSOffset, uint32 NumBytes)
{
    uint8 *CopyPtr;
    int32  return_code;

    if (PtrToDataToRead == NULL)
    {
        return_code = OS_ERROR;
    }
    else
    {
        if ((CDSOffset < CFE_PSP_ReservedMemoryMap.CDSMemory.BlockSize) &&
            ((CDSOffset + NumBytes) <= CFE_PSP_ReservedMemoryMap.CDSMemory.BlockSize))
        {
            CopyPtr = CFE_PSP_ReservedMemoryMap.CDSMemory.BlockPtr;
            CopyPtr += CDSOffset;
            memcpy((char *)PtrToDataToRead, (char *)CopyPtr, NumBytes);

            return_code = OS_SUCCESS;
        }
        else
        {
            return_code = OS_ERROR;
        }

    } /* end if PtrToDataToWrite == NULL */

    return return_code;
}

/*
*********************************************************************************
** ES Reset Area related functions
*********************************************************************************
*/

/******************************************************************************
**
**  Purpose:
**    This function is used by the ES startup code to initialize the
**     ES Reset Area.
**
**  Arguments:
**    (none)
**
**  Return:
**    (none)
*/
void CFE_PSP_InitResetArea(void)
{
    size_t                                    total_size;
    void                                     *block_ptr;
    CFE_PSP_PhoenixReservedAreaFixedLayout_t *FixedBlocksPtr;

    total_size = sizeof(CFE_PSP_PhoenixReservedAreaFixedLayout_t) + CFE_PSP_RESET_AREA_SIZE;

    block_ptr = malloc(total_size);
    if (block_ptr == NULL)
    {
        printf("CFE_PSP: Error: Could not allocate Reset Area\n");
        CFE_PSP_Panic(CFE_PSP_ERROR);
    }

    FixedBlocksPtr = (CFE_PSP_PhoenixReservedAreaFixedLayout_t *)block_ptr;

    CFE_PSP_ReservedMemoryMap.BootPtr             = &FixedBlocksPtr->BootRecord;
    CFE_PSP_ReservedMemoryMap.ExceptionStoragePtr = &FixedBlocksPtr->ExceptionStorage;

    block_ptr = (char *)block_ptr + sizeof(*FixedBlocksPtr);

    CFE_PSP_ReservedMemoryMap.ResetMemory.BlockPtr  = block_ptr;
    CFE_PSP_ReservedMemoryMap.ResetMemory.BlockSize = CFE_PSP_RESET_AREA_SIZE;
}

/******************************************************************************
**
**  Purpose:
**   This is an internal function to delete the Reset Area Shared memory segment.
**
**  Arguments:
**    (none)
**
**  Return:
**    (none)
*/
void CFE_PSP_DeleteResetArea(void)
{
    void *block_ptr;

    block_ptr =
        (char *)CFE_PSP_ReservedMemoryMap.ResetMemory.BlockPtr - sizeof(CFE_PSP_PhoenixReservedAreaFixedLayout_t);

    free(block_ptr);
    CFE_PSP_ReservedMemoryMap.ResetMemory.BlockPtr = NULL;
}

/*----------------------------------------------------------------
 *
 * Implemented per public API
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
int32 CFE_PSP_GetResetArea(cpuaddr *PtrToResetArea, uint32 *SizeOfResetArea)
{
    int32 return_code;

    if (SizeOfResetArea == NULL || PtrToResetArea == NULL)
    {
        return_code = OS_ERROR;
    }
    else
    {
        *PtrToResetArea  = (cpuaddr)CFE_PSP_ReservedMemoryMap.ResetMemory.BlockPtr;
        *SizeOfResetArea = CFE_PSP_ReservedMemoryMap.ResetMemory.BlockSize;
        return_code      = OS_SUCCESS;
    }

    return return_code;
}

/*
*********************************************************************************
** ES User Reserved Area related functions
*********************************************************************************
*/

/******************************************************************************
**
**  Purpose:
**    This function is used by the ES startup code to initialize the
**      ES user reserved area.
**
**  Arguments:
**    (none)
**
**  Return:
**    (none)
*/
void CFE_PSP_InitUserReservedArea(void)
{
    CFE_PSP_ReservedMemoryMap.UserReservedMemory.BlockPtr = malloc(CFE_PSP_USER_RESERVED_SIZE);
    if (CFE_PSP_ReservedMemoryMap.UserReservedMemory.BlockPtr == NULL)
    {
        printf("CFE_PSP: Error: Could not allocate User Reserved Area\n");
        CFE_PSP_Panic(CFE_PSP_ERROR);
    }
    CFE_PSP_ReservedMemoryMap.UserReservedMemory.BlockSize = CFE_PSP_USER_RESERVED_SIZE;
}

/******************************************************************************
**
**  Purpose:
**   This is an internal function to delete the User Reserved Shared memory segment.
**
**  Arguments:
**    (none)
**
**  Return:
**    (none)
*/
void CFE_PSP_DeleteUserReservedArea(void)
{
    free(CFE_PSP_ReservedMemoryMap.UserReservedMemory.BlockPtr);
    CFE_PSP_ReservedMemoryMap.UserReservedMemory.BlockPtr = NULL;
}

/*----------------------------------------------------------------
 *
 * Implemented per public API
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
int32 CFE_PSP_GetUserReservedArea(cpuaddr *PtrToUserArea, uint32 *SizeOfUserArea)
{
    int32 return_code;

    if (SizeOfUserArea == NULL)
    {
        return_code = CFE_PSP_ERROR;
    }
    else
    {
        *PtrToUserArea  = (cpuaddr)CFE_PSP_ReservedMemoryMap.UserReservedMemory.BlockPtr;
        *SizeOfUserArea = CFE_PSP_ReservedMemoryMap.UserReservedMemory.BlockSize;
        return_code     = CFE_PSP_SUCCESS;
    }

    return return_code;
}

/*
*********************************************************************************
** ES Volatile disk memory related functions
*********************************************************************************
*/

/******************************************************************************
**
**  Purpose:
**   This function is used by the ES startup code to initialize the memory
**   used by the volatile disk. On a desktop/posix platform this is currently
**   a no-op, because the volatile disk is being mapped to the desktop.
**
**  Arguments:
**    (none)
**
**  Return:
**    (none)
*/
void CFE_PSP_InitVolatileDiskMem(void)
{
    /*
    ** Here, we want to clear out the volatile ram disk contents
    ** on a power on reset
    */
}

/*----------------------------------------------------------------
 *
 * Implemented per public API
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
int32 CFE_PSP_GetVolatileDiskMem(cpuaddr *PtrToVolDisk, uint32 *SizeOfVolDisk)
{
    int32 return_code;

    if (SizeOfVolDisk == NULL)
    {
        return_code = CFE_PSP_ERROR;
    }
    else
    {
        *PtrToVolDisk  = (cpuaddr)CFE_PSP_ReservedMemoryMap.VolatileDiskMemory.BlockPtr;
        *SizeOfVolDisk = CFE_PSP_ReservedMemoryMap.VolatileDiskMemory.BlockSize;
        return_code    = CFE_PSP_SUCCESS;
    }

    return return_code;
}

/*
*********************************************************************************
** ES BSP Top Level Reserved memory initialization
*********************************************************************************
*/

/******************************************************************************
**
**  Purpose:
**    This function performs the top level reserved memory initialization.
**
**  Arguments:
**    (none)
**
**  Return:
**    (none)
*/
void CFE_PSP_SetupReservedMemoryMap(void)
{
    printf("CFE_PSP: Setting up reserved memory map\n");
    CFE_PSP_InitCDS();
    CFE_PSP_InitResetArea();
    CFE_PSP_InitVolatileDiskMem();
    CFE_PSP_InitUserReservedArea();
}

int32 CFE_PSP_InitProcessorReservedMemory(uint32 RestartType)
{
    return CFE_PSP_SUCCESS;
}

/******************************************************************************
**
**  Purpose:
**    This function cleans up all of the shared memory segments in the
**     Linux/OSX ports.
**
**  Arguments:
**    (none)
**
**  Return:
**    (none)
*/
void CFE_PSP_DeleteProcessorReservedMemory(void)
{
    CFE_PSP_DeleteCDS();
    CFE_PSP_DeleteResetArea();
    CFE_PSP_DeleteUserReservedArea();
}

/*
*********************************************************************************
** ES BSP kernel memory segment functions
*********************************************************************************
*/

/*----------------------------------------------------------------
 *
 * Implemented per public API
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
int32 CFE_PSP_GetKernelTextSegmentInfo(cpuaddr *PtrToKernelSegment, uint32 *SizeOfKernelSegment)
{
    /* Check pointers */
    if (PtrToKernelSegment == NULL || SizeOfKernelSegment == NULL)
    {
        return CFE_PSP_ERROR;
    }

    /* Set to known values */
    *PtrToKernelSegment  = (cpuaddr)0x0;
    *SizeOfKernelSegment = 0;

    return CFE_PSP_ERROR_NOT_IMPLEMENTED;
}

/*----------------------------------------------------------------
 *
 * Implemented per public API
 * See description in header file for argument/return detail
 *
 *-----------------------------------------------------------------*/
int32 CFE_PSP_GetCFETextSegmentInfo(cpuaddr *PtrToCFESegment, uint32 *SizeOfCFESegment)
{
    /* Check pointers */
    if (PtrToCFESegment == NULL || SizeOfCFESegment == NULL)
    {
        return CFE_PSP_ERROR;
    }

    /* Set to known values */
    *PtrToCFESegment  = (cpuaddr)0x0;
    *SizeOfCFESegment = 0;

    return CFE_PSP_ERROR_NOT_IMPLEMENTED;
}
