/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef _VAL_RHI_H_
#define _VAL_RHI_H_

#include "pal_interfaces.h"
#include "val_def.h"
#include "val_framework.h"

typedef enum {
    RHI_DA_DEV_VCA = 0,
    RHI_DA_DEV_CERTIFICATE = 1,
    RHI_DA_DEV_MEASUREMENTS = 2,
    RHI_DA_DEV_INTERFACE_REPORT = 3,
    RHI_DA_DEV_EXTENSION_EVIDENCE = 4,
} val_da_devcomm_obj_te;

typedef enum {
    RHI_DA_TDI_CONFIG_UNLOCKED = 0,
    RHI_DA_TDI_CONFIG_LOCKED = 1,
    RHI_DA_TDI_CONFIG_RUN = 2,
} val_da_vdev_tdistate_te;

#define RHI_SMC_NOT_SUPPORTED 0xFFFFFFFFFFFFFFFF

#define RHI_SESSION_VER 0x10000
#define RHI_FAL_VER 0x10000
#define RHI_DA_VER 0x10000
#define RHI_HOSTCONF_VER 0x10000

#define RHI_FAL_LOG_SIZE 0x1000
#define RHI_HOSTCONF_IPA_CHANGE_ALIGNMENT 0x1000

/* RHI Host Session States */
#define RHI_HSS_SESSION_UNCONNECTED 		    0
#define RHI_HSS_CONNECTION_IN_PROGRESS 		    1
#define RHI_HSS_CONNECTION_ESTABLISHED 		    2
#define RHI_HSS_IO_IN_PROGRESS 			        3
#define RHI_HSS_IO_COMPLETE 			        4
#define RHI_HSS_BUFFER_SIZE_DETERMINED 		    5
#define RHI_HSS_CONNECTION_CLOSE_IN_PROGRESS 	6

/* RHI Session error Status Codes */
#define RHI_SESS_SUCCESS 			            0
#define RHI_SESS_PEER_NOT_AVAILABLE 		    1
#define RHI_SESS_INVALID_STATE_FOR_OPERATION 	2
#define RHI_SESS_INVALID_SESSION_ID 		    3
#define RHI_SESS_ACCESS_FAILED 			        4
#define RHI_SESS_CONNECTION_TYPE_NOT_SUPPORTED  5

/* RHI FAL ABI Status Codes */
#define RHI_FAL_SUCCESS 			            0
#define RHI_FAL_ACCESS_FAILED 			        1

/* RHI DA ABI Status Codes */
#define RHI_DA_SUCCESS 				            0
#define RHI_DA_ERROR_INCOMPLETE 		        1
#define RHI_DA_ERROR_DATA_NOT_AVAILABLE 	    2
#define RHI_DA_ERROR_INVALID_VDEV_ID 		    3
#define RHI_DA_ERROR_INVALID_OBJECT 		    4
#define RHI_DA_ERROR_INPUT 			            5
#define RHI_DA_ERROR_DEVICE 			        6
#define RHI_DA_ERROR_INVALID_OFFSET 		    7
#define RHI_DA_ERROR_ACCESS_FAILED 		        8
#define RHI_DA_ERROR_BUSY 			            9
#define RHI_DA_ABORTED_OPERATION_HAD_COMPLETED  10
#define RHI_DA_ERROR_OBJECT_UNSUPPORTED 	    11

/* RHI ABI fids */
#define RHI_SESSION_VERSION 			        0xC5000040
#define RHI_SESSION_FEATURES 			        0xC5000041
#define RHI_SESSION_OPEN 			            0xC5000042
#define RHI_SESSION_CLOSE 			            0xC5000043
#define RHI_SESSION_SEND 			            0xC5000044
#define RHI_SESSION_RECEIVE 			        0xC5000045
#define RHI_FAL_VERSION 			            0xC5000046
#define RHI_FAL_FEATURES 			            0xC5000047
#define RHI_FAL_GET_SIZE 			            0xC5000048
#define RHI_FAL_READ	 			            0xC5000049
#define RHI_DA_VERSION 			                0xC500004A
#define RHI_DA_FEATURES 			            0xC500004B
#define RHI_DA_VDEV_CONTINUE                    0xC5000051
#define RHI_DA_VDEV_GET_MEASUREMENTS            0xC5000052
#define RHI_DA_VDEV_GET_INTERFACE_REPORT        0xC5000053
#define RHI_DA_VDEV_SET_TDI_STATE               0xC5000054
#define RHI_DA_VDEV_P2P_UNBIND                  0xC5000055
#define RHI_DA_VDEV_ABORT                       0xC5000056
#define RHI_DA_OBJECT_SIZE                      0xC500004C
#define RHI_DA_OBJECT_READ                      0xC500004D
#define RHI_HOSTCONF_VERSION                    0xC500004E
#define RHI_HOSTCONF_FEATURES                   0xC500004F
#define RHI_HOSTCONF_GET_IPA_CHANGE_ALIGNMENT   0xC5000050

#endif
