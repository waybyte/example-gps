/**
 * @file main.c
 * @brief Exmaple application to demonstrate GPS Library
 * @version 0.1
 * @date 2022-07-05
 * 
 * @copyright Copyright (c) 2022 Waybyte Solutions
 * 
 */

#include <stdio.h>
#include <unistd.h>

#include <lib.h>
#include <ril.h>
#include <os_api.h>
#include <gpslib.h>

#define DGB_GPS_NMEA DBG_USER_START

static const struct point_t fence_center = {
	.lat = 28.620198302588097,
	.lng = 77.21172963589777
};

/**
 * URC Handler
 * @param param1	URC Code
 * @param param2	URC Parameter
 */
static void urc_callback(unsigned int param1, unsigned int param2)
{
	switch (param1) {
	case URC_SYS_INIT_STATE_IND:
		if (param2 == SYS_STATE_SMSOK) {
			/* Ready for SMS */
		}
		break;
	case URC_SIM_CARD_STATE_IND:
		switch (param2) {
		case SIM_STAT_NOT_INSERTED:
			debug(DBG_OFF, "SYSTEM: SIM card not inserted!\n");
			break;
		case SIM_STAT_READY:
			debug(DBG_INFO, "SYSTEM: SIM card Ready!\n");
			break;
		case SIM_STAT_PIN_REQ:
			debug(DBG_OFF, "SYSTEM: SIM PIN required!\n");
			break;
		case SIM_STAT_PUK_REQ:
			debug(DBG_OFF, "SYSTEM: SIM PUK required!\n");
			break;
		case SIM_STAT_NOT_READY:
			debug(DBG_OFF, "SYSTEM: SIM card not recognized!\n");
			break;
		default:
			debug(DBG_OFF, "SYSTEM: SIM ERROR: %d\n", param2);
		}
		break;
	case URC_GSM_NW_STATE_IND:
		debug(DBG_OFF, "SYSTEM: GSM NW State: %d\n", param2);
		break;
	case URC_GPRS_NW_STATE_IND:
		break;
	case URC_CFUN_STATE_IND:
		break;
	case URC_COMING_CALL_IND:
		debug(DBG_OFF, "Incoming voice call from: %s\n", ((struct ril_callinfo_t *)param2)->number);
		/* Take action here, Answer/Hang-up */
		break;
	case URC_CALL_STATE_IND:
		switch (param2) {
		case CALL_STATE_BUSY:
			debug(DBG_OFF, "The number you dialed is busy now\n");
			break;
		case CALL_STATE_NO_ANSWER:
			debug(DBG_OFF, "The number you dialed has no answer\n");
			break;
		case CALL_STATE_NO_CARRIER:
			debug(DBG_OFF, "The number you dialed cannot reach\n");
			break;
		case CALL_STATE_NO_DIALTONE:
			debug(DBG_OFF, "No Dial tone\n");
			break;
		default:
			break;
		}
		break;
	case URC_NEW_SMS_IND:
		debug(DBG_OFF, "SMS: New SMS (%d)\n", param2);
		/* Handle New SMS */
		break;
	case URC_MODULE_VOLTAGE_IND:
		debug(DBG_INFO, "VBatt Voltage: %d\n", param2);
		break;
	case URC_ALARM_RING_IND:
		break;
	case URC_FILE_DOWNLOAD_STATUS:
		break;
	case URC_FOTA_STARTED:
		break;
	case URC_FOTA_FINISHED:
		break;
	case URC_FOTA_FAILED:
		break;
	case URC_STKPCI_RSP_IND:
		break;
	default:
		break;
	}
}

static void gps_data_callback(struct gpsdata_t *data)
{
	static uint8_t geofence_inout = 0;
	struct point_t curr_point;

	curr_point.lat = data->lat;
	curr_point.lng = data->lng;

	/* this can be used to check say geofence */
	if (!geofence_inout && geofence_check(FENCE_TYPE_CIRCLE, &fence_center, 1000, &curr_point)) {
		/* inside geofence */
		geofence_inout = 1;
		printf("Entered geofence\n");
	} else if (geofence_inout && !geofence_check(FENCE_TYPE_CIRCLE, &fence_center, 1000, &curr_point)) {
		/* outside geofence */
		geofence_inout = 0;
		printf("Exited geofence\n");
	}
}

static void gps_nmea_callback(const char *nmea)
{
	/**
	 * nmea message should output when loglevel is set to 24
	 * To see messages on console, send command:
	 * loglevel=24
	 * 
	 * To turn off messages:
	 * loglevel=0
	 */
	debug(DGB_GPS_NMEA, nmea);
}

static void gps_event_callback(int event)
{
	switch (event)
	{
	case GPS_EVENT_FIRST_FIX:
		printf("GPS First fix after power on\n");
		break;
	case GPS_EVENT_MOTION_STOP:
		printf("Motion start event\n");
		break;
	case GPS_EVENT_MOTION_START:
		printf("Motion stop event\n");
		break;
	case GPS_EVENT_HARSHBRAKE:
		printf("Harsh braking\n");
		break;
	case GPS_EVENT_OVERACCEL:
		printf("Harsh Acceleration\n");
		break;
	case GPS_EVENT_OVERSPEED:
		printf("Vehicle overspeeding\n");
		break;
	case GPS_EVENT_SPEED_NORMAL:
		printf("Vehicle Speed back to normal\n");
		break;
	case GPS_EVENT_COG_CHANGED:
		/* Can be used to generate a location event */
		printf("Course over ground changed\n");
		break;
	case GPS_EVENT_HARSHTURN:
		printf("Harsh turn detected\n");
		break;
	case GPS_EVENT_NO_RESPONSE:
		printf("GPS Not responding\n");
		break;
	case GPS_EVENT_GPS_LOST:
		printf("GPS signal lost\n");
		break;
	case GPS_EVENT_GPS_LOCKED:
		printf("GPS signal restored\n");
		break;
	case GPS_EVENT_GPS_OK:
		printf("GPS response ok\n");
		break;
	default:
		printf("Unknown event %d\n", event);
		break;
	}
}

/**
 * Application main entry point
 */
int main(int argc, char *argv[])
{
	struct gpsconfig_t config;
	/*
	 * Initialize library and Setup STDIO
	 */
	logicrom_init("/dev/ttyS0", urc_callback);

	printf("System Ready\n");

#ifdef PLATFORM_EC200U
	/* Initialize EC200U-CN-AA onboard GNSS */
	gnss_hw_power(true);
	config.baud = GNSS_BAUD;
#else
	config.baud = 115200;
#endif
	config.harshbrake = DEF_HARSHBRAKE;
	config.harshturn = DEF_HARSHTURN;
	config.overaccel = DEF_OVERACCEL;
	config.speedlimit = DEF_SPEEDLIMIT;
	config.gps_datacallback = gps_data_callback;
	config.gps_nmea_cb = gps_nmea_callback;
	config.gps_event_cb = gps_event_callback;

#ifdef PLATFORM_EC200U
	gpslib_init(GNSS_PORT, &config);
#else
	gpslib_init("/dev/ttyS1", &config);
#endif

	printf("System Initialization finished\n");

	while (1) {
		/* Main task */
		sleep(1);
	}
}
