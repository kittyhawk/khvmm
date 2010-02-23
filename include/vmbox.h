/*********************************************************************
 *
 * Copyright (C) 2008, Udo Steinberg, IBM Corporation
 *
 * File path:       vmbox.h
 * Description:     Virtual BlueGene SRAM Mailbox
 *
 * All rights reserved
 *
 ********************************************************************/
#pragma once

#include "std.h"

/*
 * Virtual Mailbox Descriptor
 */
class vmbox_desc_t
{
    private:
        u16_t   offs;
        u16_t   size;

    public:
        vmbox_desc_t () : offs (0), size (0) {};
        vmbox_desc_t (unsigned o, unsigned s) : offs (o), size (s) {};
};

/*
 * Virtual Mailbox Table
 */
class vmbox_table_t
{
    friend class vtest_t;

    private:
        struct {
            vmbox_desc_t to_core;
            vmbox_desc_t to_host;
        } mbox[4];

    public:
        static inline void *operator new (size_t, word_t addr)
        {
            return reinterpret_cast<void *>(addr);
        }
};

/*
 * Virtual Mailbox
 */
class vmbox_t
{
    private:
        u16_t   cmd;
        u16_t   len;
        u16_t   result;
        u16_t   crc;
        char    payload[];

    public:
        enum Result
        {
            SUCCESS,
            BAD_CMD,
            BAD_CRC,
            EINVAL,
            ENOSYS,
        };

        enum Command
        {
            TO_HOST_NONE = 0,
            TO_HOST_READY,
            TO_HOST_PRINT,
            TO_HOST_RAS,
            TO_HOST_TERMINATE,
            TO_HOST_NETWORK_INIT,
            TO_HOST_HSSBIST_SEND_DONE,
            TO_HOST_HSSBIST_RESULT,
            TO_HOST_RAS_ASCII,

            TO_CORE_NONE = 0,
            TO_CORE_WRITE,
            TO_CORE_FILL,
            TO_CORE_READ,
            TO_CORE_LAUNCH,
            TO_CORE_STDIN,
            TO_CORE_RESET,
            TO_CORE_HSSBIST_RUN,
            TO_CORE_CRC32_RESET,
            TO_CORE_ELF_SCTN_CRC
        };

        static inline void *operator new (size_t, word_t addr)
        {
            return reinterpret_cast<void *>(addr);
        }

        void alert();
};
