/* Copyright 2013, by the California Institute of Technology.  ALL RIGHTS
   RESERVED.  United States Government Sponsorship acknowledged.  Any 
   commercial use must be negotiated with the Office of Technology Transfer
   at the California Institute of Technology.

   This software may be subject to U.S. export control laws.  By accepting
   this software, the user agrees to comply with all applicable U.S. export
   laws and regulations.  User has the responsibility to obtain export 
   licenses, or other export authority as may be required before exporting
   such information to foreign countries or providing access to foreign
   persons.

   Please do not redistribute this file separate from the NGDCS source
   distribution.  For questions regarding this software, please contact the
   author, Alan Mazer, alan.s.mazer@jpl.nasa.gov */


#include <gtkmm.h>
#include "definitions.h"
#include "maxon.h"

extern const char *const maxonDate = "$Date: 2014/01/17 23:51:50 $";

#define NODE_ID                     1

#define MAX_FOLLOWING_ERROR         1000

#define HOMING_SPEED_SWITCH         500
#define HOMING_SPEED_INDEX          10
#define HOMING_MAX_CURRENT_IN_MA    500
#define HOMING_ACCELERATION         1000
#define HOMING_MAX_VELOCITY         500

#define POSITION_MAX_VELOCITY       2000
#define POSITION_MAX_ACCELERATION   4000

#define POS_POLL_INTERVAL_IN_MS     100

static void *handle;

#if 1
extern "C" {
int VCS_ActivateHomingMode(void*, u_short, u_int*) { return 0; }
int VCS_ActivatePositionMode(void*, u_short, u_int*) { return 0; }
int VCS_ClearFault(void*, u_short, u_int*) { return 0; }
int VCS_CloseDevice(void*, u_int* pErrorCode) { *pErrorCode = 0; return 0; }
int VCS_FindHome(void*, u_short, char, u_int*) { return 0; }
int VCS_GetDeviceNameSelection(int, char*, u_short, int*, u_int* pErrorCode) {
    *pErrorCode = 0; return 0; }
int VCS_GetEnableState(void*, u_short, int*, u_int*) { return 0; }
int VCS_GetFaultState(void*, u_short, int*, u_int* pErrorCode) {
    *pErrorCode = 0; return 0; }
int VCS_GetHomingParameter(void*, u_short, u_int*, u_int*, u_int*, long*,
    u_short*, long*, u_int*) { return 0; }
int VCS_GetInterfaceNameSelection(char*, char*, int, char*, u_short, int*,
    u_int*) { return 0; }
int VCS_GetMaxAcceleration(void*, u_short, u_int*, u_int*) { return 0; }
int VCS_GetMaxFollowingError(void*, u_short, u_int*, u_int*) { return 0; }
int VCS_GetMaxProfileVelocity(void*, u_short, u_int*, u_int*) { return 0; }
int VCS_GetMovementState(void*, u_short, int*, u_int*) { return 0; }
int VCS_GetOperationMode(void*, u_short, char*, u_int*) { return 0; }
int VCS_GetPortNameSelection(char*, char*, char*, int, char*, u_short, int*,
    u_int*) { return 0; }
int VCS_GetPositionIs(void*, u_short, long*, u_int*) { return 0; }
int VCS_GetProtocolStackNameSelection(char*, int, char*, u_short, int*, u_int*)
    { return 0; }
int VCS_GetVelocityUnits(void*, u_short, u_char*, char*, u_int*) { return 0; }
void* VCS_OpenDevice(char*, char*, char*, char*, u_int*) { return NULL; }
int VCS_ResetDevice(void*, u_short, u_int*) { return 0; }
int VCS_SetEnableState(void*, u_short, u_int*) { return 0; }
int VCS_SetHomingParameter(void*, u_short, u_int, u_int, u_int, long, u_short,
    long, u_int*) { return 0; }
int VCS_SetMaxFollowingError(void*, u_short, u_int, u_int*) { return 0; }
int VCS_SetMaxProfileVelocity(void*, u_short, u_int, u_int*) { return 0; }
int VCS_SetMaxAcceleration(void*, u_short, u_int, u_int*) { return 0; }
int VCS_SetPositionMust(void*, u_short, long, u_int*) { return 0; }
}
#endif

int Maxon_init_device(char *error_msg)
{
    char device_name[132];
    char protocol_stack_name[132];
    char interface_name[132];
    char port_name[132];
    u_int error_code;
    int end_flag;
    u_int max_following_error;
    u_char veldim;
    char notation;

        /* init */

    *error_msg = '\0';

        /* figure out what USB port we need to use */

    if (!VCS_GetDeviceNameSelection(true, device_name, sizeof(device_name),
            &end_flag, &error_code)) {
        sprintf(error_msg,
            "Can't determine Maxon device name (error 0x%08x) -- "
		"driver installed?  EPOS Studio closed?",
            error_code);
        return -1;
    }
    if (!VCS_GetProtocolStackNameSelection(device_name, true,
            protocol_stack_name, sizeof(protocol_stack_name), &end_flag,
	    &error_code)) {
        sprintf(error_msg,
            "Can't determine Maxon protocol stack (error 0x%08x) -- "
		"driver installed?  EPOS Studio closed?",
            error_code);
        return -1;
    }
    if (!VCS_GetInterfaceNameSelection(device_name, protocol_stack_name, true,
            interface_name, sizeof(interface_name), &end_flag, &error_code)) {
        sprintf(error_msg,
            "Can't determine Maxon interface name (error 0x%08x) -- "
		"driver installed?  EPOS Studio closed?",
            error_code);
        return -1;
    }
    if (!VCS_GetPortNameSelection(device_name, protocol_stack_name,
	    interface_name, true, port_name, sizeof(port_name), &end_flag,
	    &error_code)) {
        sprintf(error_msg,
            "Can't determine Maxon port name (error 0x%08x) -- "
		"driver installed?  EPOS Studio closed?",
            error_code);
        return -1;
    }

        /* open the device */

    if (!(handle=VCS_OpenDevice(device_name, protocol_stack_name,
            interface_name, port_name, &error_code))) {
        sprintf(error_msg,
	    "Can't open Maxon device (error 0x%08x)", error_code);
        return -1;
    }

        /* clear any existing fault */

    (void)VCS_ClearFault(handle, NODE_ID, &error_code);

        /* set max following error */

    if (!VCS_SetMaxFollowingError(handle, NODE_ID, MAX_FOLLOWING_ERROR,
            &error_code)) {
        sprintf(error_msg, "Can't set Maxon max following error (error 0x%08x)",
            error_code);
        return -1;
    }
    if (!VCS_GetMaxFollowingError(handle, NODE_ID, &max_following_error,
            &error_code)) {
        sprintf(error_msg, "Can't get Maxon max following error (error 0x%08x)",
            error_code);
        return -1;
    }

        /* set velocity units */

    if (!VCS_GetVelocityUnits(handle, NODE_ID, &veldim, &notation,
                &error_code) ||
            veldim != VD_RPM || notation != VN_STANDARD) {
        sprintf(error_msg, "Can't set Maxon velocity units (error 0x%08x)",
            error_code);
        return 0;
    }
    return 0;
}


int Maxon_home(char *error_msg)
{
    u_int error_code;
    int enable;
    int faulted;
    char opmode;
    u_int max_profile_velocity;
    u_int accel, speed_switch, speed_index;
    long home_offset, home_pos;
    u_short current_threshold;
    int reached;
    long current_pos, last_pos;
    int status;
    int first_time;
    struct timespec t;

        /* init */

    *error_msg = '\0';
    if (!VCS_GetFaultState(handle, NODE_ID, &faulted, &error_code)) {
        sprintf(error_msg,
            "Can't determine Maxon fault state in homing code (error 0x%08x)",
            error_code);
        return -1;
    }
    if (faulted) {
        if (!VCS_ClearFault(handle, NODE_ID, &error_code)) {
            if (!VCS_ResetDevice(handle, NODE_ID, &error_code)) {
                sprintf(error_msg,
                    "Can't reset Maxon in homing code (error 0x%08x)",
                    error_code);
                return -1;
            }
        }
        if (!VCS_GetFaultState(handle, NODE_ID, &faulted, &error_code)) {
            sprintf(error_msg,
                "Can't determine Maxon fault state in homing code "
		    "(error 0x%08x)",
                error_code);
            return -1;
        }
        if (faulted) {
            if (!VCS_ResetDevice(handle, NODE_ID, &error_code)) {
                sprintf(error_msg,
                    "Can't reset Maxon in homing code (error 0x%08x)",
                    error_code);
                return -1;
            }
            if (!VCS_GetFaultState(handle, NODE_ID, &faulted, &error_code)) {
                sprintf(error_msg,
                    "Can't determine Maxon fault state in homing code "
			"(error 0x%08x)",
                    error_code);
                return -1;
            }
            if (faulted) {
                strcpy(error_msg, "Maxon faulted and unrecoverable");
                return -1;
            }
        }
    }

        /* activate homing mode */

    if (!VCS_ActivateHomingMode(handle, NODE_ID, &error_code) ||
            !VCS_GetOperationMode(handle, NODE_ID, &opmode, &error_code) ||
            opmode != OMD_HOMING_MODE) {
        sprintf(error_msg,
            "Can't activate Maxon homing mode (error 0x%08x)",
            error_code);
        return -1;
    }

        /* set enable state */

    if (!VCS_SetEnableState(handle, NODE_ID, &error_code)) {
        sprintf(error_msg,
            "Can't set Maxon enable state (error 0x%08x)",
            error_code);
        return -1;
    }
    if (!VCS_GetEnableState(handle, NODE_ID, &enable, &error_code)) {
        sprintf(error_msg,
            "Can't get Maxon enable state (error 0x%08x)",
            error_code);
        return -1;
    }

        /* set max profile velocity */

    if (!VCS_SetMaxProfileVelocity(handle, NODE_ID, HOMING_MAX_VELOCITY,
                &error_code) ||
            !VCS_GetMaxProfileVelocity(handle, NODE_ID, &max_profile_velocity,
                &error_code)) {
        sprintf(error_msg,
            "Can't set Maxon max profile velocity (error 0x%08x)",
            error_code);
        return -1;
    }

        /* set homing parameters */

    if (!VCS_SetHomingParameter(handle, NODE_ID, HOMING_ACCELERATION,
                HOMING_SPEED_SWITCH, HOMING_SPEED_INDEX, 0,
                HOMING_MAX_CURRENT_IN_MA, 0, &error_code) ||
            !VCS_GetHomingParameter(handle, NODE_ID, &accel, &speed_switch,
                &speed_index, &home_offset, &current_threshold, &home_pos,
                &error_code)) {
        sprintf(error_msg,
            "Can't set Maxon homing parameters (error 0x%08x)",
            error_code);
        return -1;
    }

        /* find home -- we really depend here on the "reached" flag being
           set or (worst case) movement stopping, because power cycling
           the controller seems to set the current position to 0, and if
           I request the current position here as I do in the positioning
           routine, I would stop immediately, seeing a 0 position, even 
           if the mechanism weren't really homed */

    if (!VCS_FindHome(handle, NODE_ID, HM_CURRENT_THRESHOLD_POSITIVE_SPEED,
            &error_code)) {
        sprintf(error_msg, "Maxon can't find home (error 0x%08x)", error_code);
        return -1;
    }
    reached = 0;
    first_time = 1;
    for (;;) {
        status = VCS_GetMovementState(handle, NODE_ID, &reached, &error_code);
        if (!status) {
            sprintf(error_msg,
                "Can't determine current Maxon state (error 0x%08x)",
                error_code);
            return -1;
        }
        if (reached)
            break;
        status = VCS_GetPositionIs(handle, NODE_ID, &current_pos, &error_code);
        if (!status) {
            sprintf(error_msg,
                "Maxon can't determine current position (error 0x%08x)",
                error_code);
            return -1;
        }
        if (first_time)
            first_time = 0;
        else if (current_pos == last_pos) /* implies fault */ /*lint !e530 last_pos not initialized */
            break;
        last_pos = current_pos;
        t.tv_nsec = POS_POLL_INTERVAL_IN_MS * 1000000;
        (void)nanosleep(&t, NULL);
    }
    if (error_code != 0) {
        sprintf(error_msg, "Maxon can't find home (error 0x%08x)", error_code);
        return -1;
    }

        /* handle fault */

    if (!VCS_GetFaultState(handle, NODE_ID, &faulted, &error_code)) {
        sprintf(error_msg, "Can't determine Maxon fault state (error 0x%08x)",
            error_code);
        return -1;
    }
    if (faulted) {
        if (!VCS_ClearFault(handle, NODE_ID, &error_code))
            sprintf(error_msg, "Motor fault; can't clear fault (error 0x%08x)",
                error_code);
        else sprintf(error_msg, "Motor fault; cleared fault state");
        return -1;
    }
    return 0;
}


int Maxon_position(int position, char *error_msg)
{
    u_int error_code;
    int enable;
    int faulted;
    char opmode;
    u_int max_profile_velocity;
    u_int max_acceleration;
    int reached;
    long current_pos, last_pos;
    int status;
    int first_time;
    struct timespec t;

        /* init */

    *error_msg = '\0';
    if (!VCS_GetFaultState(handle, NODE_ID, &faulted, &error_code)) {
        sprintf(error_msg,
            "Can't determine Maxon fault state in position code (error 0x%08x)",
            error_code);
        return -1;
    }
    if (faulted) {
        if (!VCS_ClearFault(handle, NODE_ID, &error_code)) {
            sprintf(error_msg,
                "Can't clear Maxon fault in position code (error 0x%08x)",
                error_code);
            return -1;
        }
    }
    if (!VCS_GetFaultState(handle, NODE_ID, &faulted, &error_code)) {
        sprintf(error_msg,
            "Can't determine Maxon fault state in position code (error 0x%08x)",
            error_code);
        return -1;
    }
    if (faulted) {
        sprintf(error_msg, "Maxon faulted; can't clear");
        return -1;
    }

        /* activate position mode */

    if (!VCS_ActivatePositionMode(handle, NODE_ID, &error_code) ||
            !VCS_GetOperationMode(handle, NODE_ID, &opmode, &error_code) ||
            opmode != OMD_POSITION_MODE) {
        sprintf(error_msg,
            "Can't activate Maxon position mode (error 0x%08x)",
            error_code);
        return -1;
    }

        /* set enable state */

    if (!VCS_SetEnableState(handle, NODE_ID, &error_code) ||
            !VCS_GetEnableState(handle, NODE_ID, &enable, &error_code)) {
        sprintf(error_msg,
            "Can't set Maxon enable state (error 0x%08x)", error_code);
        return -1;
    }

        /* set max profile velocity */

    if (!VCS_SetMaxProfileVelocity(handle, NODE_ID, POSITION_MAX_VELOCITY,
                &error_code) ||
            !VCS_GetMaxProfileVelocity(handle, NODE_ID, &max_profile_velocity,
                &error_code)) {
        sprintf(error_msg,
            "Can't set Maxon max profile velocity (error 0x%08x)",
            error_code);
        return -1;
    }

        /* set max acceleration */

    if (!VCS_SetMaxAcceleration(handle, NODE_ID, POSITION_MAX_ACCELERATION,
                &error_code) ||
            !VCS_GetMaxAcceleration(handle, NODE_ID, &max_acceleration,
                &error_code)) {
        sprintf(error_msg,
            "Can't set Maxon max acceleration (error 0x%08x)",
            error_code);
        return -1;
    }

        /* move to position */

    if (!VCS_SetPositionMust(handle, NODE_ID, position, &error_code)) {
        sprintf(error_msg,
            "Can't set Maxon position (error 0x%08x)", error_code);
        return -1;
    }
    reached = 0;
    first_time = 1;
    for (;;) {
        status = VCS_GetMovementState(handle, NODE_ID, &reached, &error_code);
        if (!status) {
            sprintf(error_msg,
                "Can't determine current Maxon state (error 0x%08x)",
                error_code);
            return -1;
        }
        if (reached)
            break;
        status = VCS_GetPositionIs(handle, NODE_ID, &current_pos, &error_code);
        if (!status) {
            sprintf(error_msg,
                "Can't determine current Maxon position (error 0x%08x)",
                error_code);
            return -1;
        }
        if (current_pos == position)
            break;
        if (first_time)
            first_time = 0;
        else if (current_pos == last_pos) /* implies fault *//*lint !e530 last_pos not initialized */
            break;
        last_pos = current_pos;
        t.tv_nsec = POS_POLL_INTERVAL_IN_MS * 1000000;
        (void)nanosleep(&t, NULL);
    }
    if (error_code != 0) {
        sprintf(error_msg,
            "Can't set Maxon position (error 0x%08x)", error_code);
        return -1;
    }

        /* handle fault */

    if (!VCS_GetFaultState(handle, NODE_ID, &faulted, &error_code)) {
        sprintf(error_msg,
            "Can't determine Maxon fault state (error 0x%08x)",
            error_code);
        return -1;
    }
    if (faulted) {
        if (!VCS_ClearFault(handle, NODE_ID, &error_code))
            sprintf(error_msg, "Motor fault; can't clear fault (error 0x%08x)",
                error_code);
        else sprintf(error_msg, "Motor fault; cleared fault state");
        return -1;
    }
    return 0;
}


int Maxon_close_device(char *error_msg)
{
    u_int error_code;
    if (error_msg)
        *error_msg = '\0';
    if (!VCS_CloseDevice(handle, &error_code)) {
        if (error_msg)
            sprintf(error_msg, "Can't close device (error 0x%08x)", error_code);
        return -1;
    }
    return 0;
}
