/*********************************************************************
 *
 * Copyright (C) 2008, Udo Steinberg, IBM Corporation
 *
 * File path:       vtomal.h
 * Description:     Virtual BlueGene TCP/IP Offload Memory Access Layer
 *
 * All rights reserved
 *
 ********************************************************************/
#pragma once

#include "std.h"

class vtomal_t
{
    public:
        // See: bgp/arch/include/bpcore/bgp_tomal_memmap.h
        enum Register
        {
            CCR       = 0x0000,   // ConfigurationControl
            RID       = 0x0060,   // RevisionID
            CMBA      = 0x0200,   // ConsumerMemoryBaseAddress
            PDEC      = 0x0400,   // PacketDataEngineControl
            TXRNC     = 0x0600,   // TxRawNotificationControl
            TXRMINT   = 0x0610,   // TxRawMinTimer
            TXRMAXT   = 0x0620,   // TxRawMaxTimer
            TXRMAXFN0 = 0x06C0,   // TxRawMaxFrameNum - Reg0
            TXRMINFN0 = 0x06D0,   // TxRawMinFrameNum - Reg0
            TXRFPSC   = 0x0650,   // TxRawFramePerServiceControl
            TXRHCDAH0 = 0x0660,   // TxRawHWCurrentDescriptorAddressHigh - Reg0
            TXRHCDAL0 = 0x0670,   // TxRawHWCurrentDescriptorAddressLow  - Reg0
            TXRPFC0   = 0x0690,   // TxRawPendingFrameCount - Reg0
            TXRAPF0   = 0x06A0,   // TxRawAddPostedFrames - Reg0
            TXRNTF0   = 0x06B0,   // TxRawNumberOfTransmittedFrames - Reg0
            TXRES0    = 0x06E0,   // TxRawEventStatus - Reg0
            TXREM0    = 0x06F0,   // TxRawIEventMask  - Reg0
            RXRNC     = 0x0F00,   // RxRawNotificationControl
            RXRMINT   = 0x0F10,   // RxRawMinTimer
            RXRMAXT   = 0x0F20,   // RxRawMaxTimer
            RXRMAXFN0 = 0x1080,   // RxRawMaxFrameNum - Reg0
            RXRMINFN0 = 0x1090,   // RxRawMinFrameNum - Reg0
            RXRHCDAH0 = 0x1020,   // RxRawHWCurrentDescriptorAddressHigh - Reg0
            RXRHCDAL0 = 0x1030,   // RxRawHWCurrentDescriptorAddressLow  - Reg0
            RXRAFB0   = 0x1040,   // RxRawAddFreeBytes - Reg0
            RXRTBS0   = 0x1050,   // RxRawTotalBuffersSize - Reg0
            RXRNRF0   = 0x1060,   // RxRawNumberOfReceivedFrames - Reg0
            RXRDFC0   = 0x1070,   // RxRawDroppedFramesCount  - Reg0
            RXRES0    = 0x10A0,   // RxRawEventStatus - Reg0
            RXREM0    = 0x10B0,   // RxRawEventMask - Reg0
            SNCES0    = 0x1800,   // SoftwareNonCriticalErrorsStatus - Reg0
            SNCEE0    = 0x1810,   // SoftwareNonCriticalErrorEnable  - Reg0
            SNCEM0    = 0x1820,   // SoftwareNonCriticalErrorMask - Reg0
            RXDBFS    = 0x1900,   // ReceiveDataBufferFreeSpace
            TXDB0FS   = 0x1910,   // TransmitDataBuffer0FreeSpace
            TXDB1FS   = 0x1920,   // TransmitDataBuffer1FreeSpace
            RXMACS0   = 0x1B20,   // RxMACStatus - Reg0
            RXMACSE0  = 0x1B30,   // RxMACStatusEnable - Reg0
            RXMACSM0  = 0x1B40,   // RxMACStatusMask - Reg0
            TXMACS0   = 0x1B50,   // TxMACStatus - Reg0
            TXMACSE0  = 0x1B60,   // TxMACStatusEnable - Reg0
            TXMACSM0  = 0x1B70,   // TxMACStatusMask - Reg0
            PLBMOES   = 0x1D00,   // PLBMasterOperationErrorsStatus
            PLBMOEE   = 0x1D10,   // PLBMasterOperationErrorsEnable
            PLBMOEM   = 0x1D20,   // PLBMasterOperationErrorsMask
            PLBSOES   = 0x1D60,   // PLBSlaveOperationalErrorsStatus
            PLBSOEE   = 0x1D70,   // PLBSlaveOperationalErrorsEnable
            PLBSOEM   = 0x1D80,   // PLBSlaveOperationalErrorsMask
            FMID      = 0x1D90,   // FailedMasterID
            FMA       = 0x1DA0,   // FailedMasterAddress
            HES       = 0x1E00,   // HardwareErrorsStatus
            HEE       = 0x1E10,   // HardwareErrorsEnable
            HEM       = 0x1E20,   // HardwareErrorsMask
            SCES      = 0x1F00,   // SoftwareCriticalErrorsStatus
            SCEE      = 0x1F10,   // SoftwareCriticalErrorsEnable
            SCEM      = 0x1F20,   // SoftwareCriticalErrorsMask
            RXDBAD    = 0x1F30,   // ReceiveDescriptorBadCodeFEC
            TXDBAD    = 0x1F40,   // TransmitDescriptorBadCodeFEC
            ISR       = 0x1F80,   // InterruptStatus
            IRR       = 0x1F90,   // InterruptRoute
            RXMBSC0   = 0x2060    // RxMACBadStatusCounter - Reg0
        };

        word_t gpr_load     (Register, word_t *);
        word_t gpr_store    (Register, word_t *);
};
