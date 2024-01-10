/* ************************************************************************
*   File: weather.c                                       EmpireMUD 2.0b5 *
*  Usage: functions handling time, the weather, and temperature           *
*                                                                         *
*  EmpireMUD code base by Paul Clarke, (C) 2000-2024                      *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  EmpireMUD based upon CircleMUD 3.0, bpl 17, by Jeremy Elson.           *
*  CircleMUD (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include <math.h>

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "handler.h"
#include "interpreter.h"
#include "db.h"
#include "skills.h"
#include "constants.h"
#include "vnums.h"

/**
* Contents:
*   Seasons Engine
*   Weather Handling
*   Time Handling
*   Moon System
*   Temperature System
*/

// external vars
extern unsigned long main_game_pulse;

// local prototypes
void another_hour();
void send_hourly_sun_messages();


 //////////////////////////////////////////////////////////////////////////////
//// SEASONS ENGINE //////////////////////////////////////////////////////////

/**
* This is called at startup and once per game day to update the seasons for the
* entire map.
*
* It uses a lot of math to determine 3 things:
*  1. Past the arctic line, always Winter.
*  2. Between the tropics, alternate Spring, Summer, Autumn (no Winter).
*  3. Everywhere else, seasons are based on time of year, and gradually move
*     North/South each day. There should always be a boundary between the
*     Summer and Winter regions (thus all the squirrelly numbers).
*
* A near-copy of this function exists in util/evolve.c for map evolutions.
*/
void determine_seasons(void) {
	double half_y, latitude;
	int ycoord, day_of_year;
	bool northern;
	
	// basic defintions and math
	#define north_y_arctic  ARCTIC_LATITUDE
	#define south_y_arctic  (90.0 - ARCTIC_LATITUDE)
	
	#define north_y_tropics  TROPIC_LATITUDE
	#define south_y_tropics  (90.0 - TROPIC_LATITUDE)
	
	// basic slope of the seasonal gradients
	double north_a_slope = ((north_y_arctic - 1) - (north_y_tropics + 1)) / 120.0;	// summer/winter
	double north_b_slope = ((north_y_arctic - 1) - (north_y_tropics + 1)) / 90.0;	// spring/autumn
	
	double south_a_slope = ((south_y_tropics - 1) - (south_y_arctic + 1)) / 120.0;	// same but for the south
	double south_b_slope = ((south_y_tropics - 1) - (south_y_arctic + 1)) / 90.0;
	
	#define pick_slope(north, south)  (northern ? north : south)
	
	// Day_of_year is the x-axis of the graph that determines the season at a
	// given y-coord. Month 0 is january; year is 0-359 days.
	day_of_year = DAY_OF_YEAR(main_time_info);
	
	for (ycoord = 0; ycoord < MAP_HEIGHT; ++ycoord) {
		latitude = Y_TO_LATITUDE(ycoord);
		northern = (latitude >= 0.0);
		
		// tropics? -- take half the tropic value, convert to percent, multiply by map height
		if (ABSOLUTE(latitude) < TROPIC_LATITUDE) {
			if (main_time_info.month < 2) {
				y_coord_to_season[ycoord] = TILESET_SPRING;
			}
			else if (main_time_info.month >= 10) {
				y_coord_to_season[ycoord] = TILESET_AUTUMN;
			}
			else {
				y_coord_to_season[ycoord] = TILESET_SUMMER;
			}
			continue;
		}
		
		// arctic? -- take half the arctic value, convert to percent, check map edges
		if (ABSOLUTE(latitude) > ARCTIC_LATITUDE) {
			y_coord_to_season[ycoord] = TILESET_WINTER;
			continue;
		}
		
		// all other regions: first split the map in half (we'll invert for the south)	
		if (northern) {
			half_y = latitude - north_y_tropics; // simplify by moving the y axis to match the tropics line
		}
		else {
			half_y = 90.0 - ABSOLUTE(latitude) - south_y_arctic;	// adjust to remove arctic
		}
	
		if (day_of_year < 6 * 30) {	// first half of year
			if (half_y >= (day_of_year + 1) * pick_slope(north_a_slope, south_a_slope)) {	// first winter line
				y_coord_to_season[ycoord] = northern ? TILESET_WINTER : TILESET_SUMMER;
			}
			else if (half_y >= (day_of_year - 89) * pick_slope(north_b_slope, south_b_slope)) {	// spring line
				y_coord_to_season[ycoord] = northern ? TILESET_SPRING : TILESET_AUTUMN;
			}
			else {
				y_coord_to_season[ycoord] = northern ? TILESET_SUMMER : TILESET_WINTER;
			}
			continue;
		}
		else {	// 2nd half of year
			if (half_y >= (day_of_year - 360) * -pick_slope(north_a_slope, south_a_slope)) {	// second winter line
				y_coord_to_season[ycoord] = northern ? TILESET_WINTER : TILESET_SUMMER;
			}
			else if (half_y >= (day_of_year - 268) * -pick_slope(north_b_slope, south_b_slope)) {	// autumn line
				y_coord_to_season[ycoord] = northern ? TILESET_AUTUMN : TILESET_SPRING;
			}
			else {
				y_coord_to_season[ycoord] = northern ? TILESET_SUMMER : TILESET_WINTER;
			}
			continue;
		}
	
		// fail? we should never reach this
		y_coord_to_season[ycoord] = TILESET_SUMMER;
	}
}


 //////////////////////////////////////////////////////////////////////////////
//// WEATHER HANDLING ////////////////////////////////////////////////////////

/**
* Reset weather data on startup (or request).
*/
void reset_weather(void) {
	weather_info.pressure = 960;
	if ((main_time_info.month >= 5) && (main_time_info.month <= 8)) {
		weather_info.pressure += number(1, 50);
	}
	else {
		weather_info.pressure += number(1, 80);
	}

	weather_info.change = 0;

	if (weather_info.pressure <= 980) {
		weather_info.sky = SKY_LIGHTNING;
	}
	else if (weather_info.pressure <= 1000) {
		weather_info.sky = SKY_RAINING;
	}
	else if (weather_info.pressure <= 1020) {
		weather_info.sky = SKY_CLOUDY;
	}
	else {
		weather_info.sky = SKY_CLOUDLESS;
	}
}


/**
* Hourly update of weather conditions. This is global but I dream of some day
* coming up with a good way to do local weather systems that move around. -pc
*/
void weather_change(void) {
	int diff, change, was_state;
	descriptor_data *desc;
	bool is_inside;
	char_data *ch;
	
	if ((main_time_info.month >= 4) && (main_time_info.month <= 8))
		diff = (weather_info.pressure > 985 ? -2 : 2);
	else
		diff = (weather_info.pressure > 1015 ? -2 : 2);

	weather_info.change += (dice(1, 4) * diff + dice(2, 6) - dice(2, 6));

	weather_info.change = MIN(weather_info.change, 12);
	weather_info.change = MAX(weather_info.change, -12);

	weather_info.pressure += weather_info.change;

	weather_info.pressure = MIN(weather_info.pressure, 1040);
	weather_info.pressure = MAX(weather_info.pressure, 960);

	change = 0;

	switch (weather_info.sky) {
		case SKY_CLOUDLESS:
			if (weather_info.pressure < 990)
				change = 1;
			else if (weather_info.pressure < 1010)
				if (!number(0, 3))
					change = 1;
			break;
		case SKY_CLOUDY:
			if (weather_info.pressure < 970)
				change = 2;
			else if (weather_info.pressure < 990) {
				if (!number(0, 3))
					change = 2;
				else
					change = 0;
			}
			else if (weather_info.pressure > 1030)
				if (!number(0, 3))
					change = 3;
			break;
		case SKY_RAINING:
			if (weather_info.pressure < 970) {
				if (!number(0, 3))
					change = 4;
				else
					change = 0;
			}
			else if (weather_info.pressure > 1030)
				change = 5;
			else if (weather_info.pressure > 1010)
				if (!number(0, 3))
					change = 5;
			break;
		case SKY_LIGHTNING:
			if (weather_info.pressure > 1010)
				change = 6;
			else if (weather_info.pressure > 990)
				if (!number(0, 3))
					change = 6;
			break;
		default:
			change = 0;
			weather_info.sky = SKY_CLOUDLESS;
			break;
	}
	
	// save for later
	was_state = weather_info.sky;
	
	// SKY_x: update sky based on change
	switch (change) {
		case 1: {
			weather_info.sky = SKY_CLOUDY;
			break;
		}
		case 2: {
			weather_info.sky = SKY_RAINING;
			break;
		}
		case 3: {
			weather_info.sky = SKY_CLOUDLESS;
			break;
		}
		case 4: {
			weather_info.sky = SKY_LIGHTNING;
			break;
		}
		case 5: {
			weather_info.sky = SKY_CLOUDY;
			break;
		}
		case 6: {
			weather_info.sky = SKY_RAINING;
			break;
		}
	}
	
	// messaging to outdoor players
	if (change > 0) {
		LL_FOREACH(descriptor_list, desc) {
			if (STATE(desc) != CON_PLAYING || (ch = desc->character) == NULL) {
				continue;	// not playing
			}
			if (!SHOW_STATUS_MESSAGES(ch, SM_WEATHER)) {
				continue;	// does not want weather messages
			}
			if (!AWAKE(ch)) {
				continue;	// sleeping
			}
			if (ROOM_AFF_FLAGGED(IN_ROOM(ch), ROOM_AFF_NO_WEATHER)) {
				continue;	// no weather here
			}
			if (!IS_OUTDOORS(ch) && !CAN_LOOK_OUT(IN_ROOM(ch))) {
				continue;	// can't see weather from here
			}
			
			// for messaging
			is_inside = !IS_OUTDOORS(ch);
			
			// SKY_x: ok, we can see weather
			switch (weather_info.sky) {
				case SKY_CLOUDY: {
					if (was_state != SKY_RAINING) {
						msg_to_char(ch, "\t%cThe sky%s starts to get cloudy.\t0\r\n", CUSTOM_COLOR_CHAR(ch, CUSTOM_COLOR_WEATHER), (is_inside ? " outside" : ""));
					}
					else {	// was raining
						if (get_room_temperature(IN_ROOM(ch)) <= -1 * config_get_int("temperature_discomfort")) {
							msg_to_char(ch, "\t%cThe snow stops%s.\t0\r\n", CUSTOM_COLOR_CHAR(ch, CUSTOM_COLOR_WEATHER), (is_inside ? " outside" : ""));
						}
						else {
							msg_to_char(ch, "\t%cThe rain%s stops.\t0\r\n", CUSTOM_COLOR_CHAR(ch, CUSTOM_COLOR_WEATHER), (is_inside ? " outside" : ""));
						}
					}
					break;
				}
				case SKY_RAINING: {
					if (was_state != SKY_LIGHTNING) {
						if (get_room_temperature(IN_ROOM(ch)) <= -1 * config_get_int("temperature_discomfort")) {
							msg_to_char(ch, "\t%cIt starts to snow%s.\t0\r\n", CUSTOM_COLOR_CHAR(ch, CUSTOM_COLOR_WEATHER), (is_inside ? " outside" : ""));
						}
						else {
							msg_to_char(ch, "\t%cIt starts to rain%s.\t0\r\n", CUSTOM_COLOR_CHAR(ch, CUSTOM_COLOR_WEATHER), (is_inside ? " outside" : ""));
						}
					}
					else {	// was lightning
						if (get_room_temperature(IN_ROOM(ch)) <= -1 * config_get_int("temperature_discomfort")) {
							msg_to_char(ch, "\t%cThe blizzard%s subsides, leaving behind a tranquil scene as snow falls gently from above.\t0\r\n", CUSTOM_COLOR_CHAR(ch, CUSTOM_COLOR_WEATHER), (is_inside ? " outside" : ""));
						}
						else {
							msg_to_char(ch, "\t%cThe intense lightning storm%s gives way to a soothing rain.\t0\r\n", CUSTOM_COLOR_CHAR(ch, CUSTOM_COLOR_WEATHER), (is_inside ? " outside" : ""));
						}
					}
					break;
				}
				case SKY_CLOUDLESS: {
					msg_to_char(ch, "\t%cThe clouds%s disappear.\t0\r\n", CUSTOM_COLOR_CHAR(ch, CUSTOM_COLOR_WEATHER), (is_inside ? " outside" : ""));
					break;
				}
				case SKY_LIGHTNING: {
					if (get_room_temperature(IN_ROOM(ch)) <= -1 * config_get_int("temperature_discomfort")) {
						msg_to_char(ch, "\t%cThe gentle snowfall%s becomes a serious blizzard.\t0\r\n", CUSTOM_COLOR_CHAR(ch, CUSTOM_COLOR_WEATHER), (is_inside ? " outside" : ""));
					}
					else {
						msg_to_char(ch, "\t%cLightning starts to show in the sky%s.\t0\r\n", CUSTOM_COLOR_CHAR(ch, CUSTOM_COLOR_WEATHER), (is_inside ? " outside" : ""));
					}
					break;
				}
			}
		}
	}
}


/**
* Called once per game hour.
*/
void weather_and_time(void) {
	another_hour();
	weather_change();
}


 //////////////////////////////////////////////////////////////////////////////
//// TIME HANDLING ///////////////////////////////////////////////////////////

/**
* Advances time by an hour and triggers things which happen on specific hours.
*/
void another_hour(void) {
	long lny;
	
	// update main time
	++main_time_info.hours;
	if (main_time_info.hours > 23) {
		// day change
		main_time_info.hours -= 24;
		++main_time_info.day;
		
		// seasons move daily
		determine_seasons();
		
		// month change
		if (main_time_info.day > 29) {
			main_time_info.day = 0;
			++main_time_info.month;
			
			// year change
			if (main_time_info.month > 11) {
				main_time_info.month = 0;
				++main_time_info.year;
				
				// run annual update
				annual_world_update();
			}
		}
		else {	// not day 30
			// check if we've missed a new year recently
			lny = data_get_long(DATA_LAST_NEW_YEAR);
			if (lny && lny + SECS_PER_MUD_YEAR < time(0)) {
				annual_world_update();
			}
		}
	}
	
	// hour-based updates
	switch (main_time_info.hours) {
		case 0: {	// midnight
			run_external_evolutions();
			break;
		}
		case 1: {	// 1am shipment
			process_shipping();
			break;
		}
		case 7: {	// 7am shipment
			process_shipping();
			break;
		}
		case 12: {	// noon
			process_imports();
			break;
		}
		case 13: {	// 1pm shipment
			process_shipping();
			break;
		}
		case 19: {	// 7pm shipment
			process_shipping();
			break;
		}
	}
	
	// all hours
	check_empire_storage_timers();
	
	// and announce it to the players
	send_hourly_sun_messages();
}


/**
* Determines the number of hours of sunlight in the room today, based on
* latitude and time of year.
*
* @param room_data *room The location.
* @return double The number of hours of sunlight today.
*/
double get_hours_of_sun(room_data *room) {
	double latitude, days_percent, max_hours;
	struct time_info_data tinfo;
	int y_coord, doy;
	double hours = 12.0;
	
	// this takes latitude in degrees but uses it in radians for the tangent...
	// it fits the graph we need though
	#define HOURS_SUN_AT_SOLSTICE(latitude, flip)  (((flip) ? -2.5 : 2.5) * tan((latitude) / 48) + 12)
	
	if ((y_coord = Y_COORD(room)) == -1) {
		return hours;	// no location: default
	}
	latitude = Y_TO_LATITUDE(y_coord);
	tinfo = get_local_time(room);
	doy = DAY_OF_YEAR(tinfo);
	
	// bound it to -67..67 because anything beyond that is in the arctic circle
	latitude = MAX(-67.0, MIN(67.0, latitude));
	
	// set max_hours to the number of hours at the solstice
	// and set days_percent to percent of the way to that solstice from the equinox
	if (doy >= FIRST_EQUINOX_DOY && doy < NORTHERN_SOLSTICE_DOY) {
		// march-june: days before the solstice
		max_hours = HOURS_SUN_AT_SOLSTICE(latitude, FALSE);
		days_percent = (doy - FIRST_EQUINOX_DOY) / 90.0;
	}
	else if (doy >= NORTHERN_SOLSTICE_DOY && doy < LAST_EQUINOX_DOY) {
		// june-september: days after the solstice
		max_hours = HOURS_SUN_AT_SOLSTICE(latitude, FALSE);
		days_percent = 1.0 - ((doy - NORTHERN_SOLSTICE_DOY) / 90.0);
	}
	else if (doy >= LAST_EQUINOX_DOY && doy < SOUTHERN_SOLSTICE_DOY) {
		// september-december: days before the solstice
		max_hours = HOURS_SUN_AT_SOLSTICE(latitude, TRUE);
		days_percent = (doy - LAST_EQUINOX_DOY) / 90.0;
	}
	else {
		// december-march: days after the solstice
		if (doy < SOUTHERN_SOLSTICE_DOY) {
			doy += 360;	// to make it "days after the solstice"
		}
		max_hours = HOURS_SUN_AT_SOLSTICE(latitude, TRUE);
		days_percent = 1.0 - ((doy - SOUTHERN_SOLSTICE_DOY) / 90.0);
	}
	
	if (max_hours > 12.0) {
		hours = days_percent * (max_hours - 12.0) + 12.0;
	}
	else if (max_hours < 12.0) {
		hours = 12.0 - (days_percent * (12.0 - max_hours));
	}
	
	// bound it to 0-24 hours of daylight
	return MAX(0.0, MIN(24.0, hours));
}


/**
* Determines exact local time based on east/west position.
*
* @param room_data *room The location.
* @return struct time_info_data The local time data.
*/
struct time_info_data get_local_time(room_data *room) {
	double longitude, percent;
	struct time_info_data tinfo;
	int x_coord;
	
	// ensure we're using local time & determine location
	if (!config_get_bool("use_local_time") || (x_coord = (room ? X_COORD(room) : -1)) == -1) {
		return main_time_info;
	}
	
	// determine longitude
	longitude = X_TO_LONGITUDE(x_coord) + 180.0;	// longitude from 0-360 instead of -/+180
	percent = 1.0 - (longitude / 360.0);	// percentage of the way west
	
	tinfo = main_time_info;	// copy
	
	// adjust hours backward for distance from east end
	tinfo.hours -= ceil(24.0 * percent - PERCENT_THROUGH_CURRENT_HOUR);
	
	// adjust back days/months/years if needed
	if (tinfo.hours < 0) {
		tinfo.hours += 24;
		if (--tinfo.day < 0) {
			tinfo.day += 30;
			if (--tinfo.month < 0) {
				tinfo.month += 12;
				--tinfo.year;
			}
		}
	}
	
	return tinfo;
}


/**
* @param room_data *room Any location.
* @return int One of SUN_RISE, SUN_LIGHT, SUN_SET, or SUN_DARK.
*/
int get_sun_status(room_data *room) {
	double hour, sun_mod, longitude, percent;
	int x_coord;
	
	if ((x_coord = X_COORD(room)) == -1) {
		// no x-coord (not in a mappable spot)
		hour = main_time_info.hours + PERCENT_THROUGH_CURRENT_HOUR;
	}
	else {
		// determine exact time
		longitude = X_TO_LONGITUDE(x_coord) + 180.0;	// longitude from 0-360 instead of -/+180
		percent = 1.0 - (longitude / 360.0);	// percentage of the way west
		hour = main_time_info.hours - (24.0 * percent - PERCENT_THROUGH_CURRENT_HOUR);
		if (hour < 0.0) {
			hour += 24.0;
		}
	}
	
	
	// sun_mod is subtracted in the morning and added in the evening
	sun_mod = get_hours_of_sun(room) / 2.0;
	
	if (sun_mod == 0.0) {
		return SUN_DARK;	// perpetual night
	}
	else if (sun_mod == 12.0) {
		return SUN_LIGHT;	// perpetual light;
	}
	else if (ABSOLUTE(hour - (12.0 - sun_mod)) < 0.5) {
		return SUN_RISE;
	}
	else if (ABSOLUTE(hour - (12.0 + sun_mod)) < 0.5) {
		return SUN_SET;
	}
	else if (hour > 12.0 - sun_mod && hour < 12.0 + sun_mod) {
		return SUN_LIGHT;
	}
	else {
		return SUN_DARK;
	}
}


/**
* Gets the numbers of days from the solstice a given room will experience
* zenith passage -- the day(s) of the year where the sun passes directly over-
* head at noon. This only occurs in the tropics; everywhere else returns -1.
*
* @param room_data *room The location.
* @return int Number of days before/after the solstice that the zenith occurs (or -1 if no zenith).
*/
int get_zenith_days_from_solstice(room_data *room) {
	double latitude, percent_from_solstice;
	int y_coord;
	
	if ((y_coord = Y_COORD(room)) == -1) {
		return -1;	// exit early if not on the map
	}
	
	latitude = Y_TO_LATITUDE(y_coord);
	if (latitude > TROPIC_LATITUDE || latitude < -TROPIC_LATITUDE) {
		return -1;	// not in the tropics
	}
	
	// percent of days from the solstice to equinox that the zenith happens
	percent_from_solstice = 1.0 - ABSOLUTE(latitude / TROPIC_LATITUDE);
	return (int)round(percent_from_solstice * 90);	// 90 days in 1/4 year
}


/**
* Determines whether or not today is the day of zenith passage -- the day(s) of
* the year where the sun passes directly overhead at noon. This only occurs in
* the tropics; everywhere else returns FALSE all year.
*
* @param room_data *room The location to check.
* @return bool TRUE if the current day (in that room) is the day of zenith passage, or FALSE if not.
*/
bool is_zenith_day(room_data *room) {
	int zenith_days, y_coord, doy;
	struct time_info_data tinfo;
	
	if ((y_coord = Y_COORD(room)) == -1 || (zenith_days = get_zenith_days_from_solstice(room)) < 0) {
		return FALSE;	// exit early if not on the map or no zenith
	}
	
	// get day of year
	tinfo = get_local_time(room);
	
	if (Y_TO_LATITUDE(y_coord) > 0) {
		// northern hemisphere: no need to wrap here (unlike the southern solstice)
		return (ABSOLUTE(NORTHERN_SOLSTICE_DOY - DAY_OF_YEAR(tinfo)) == zenith_days);
	}
	else {
		// southern hemisphere
		doy = DAY_OF_YEAR(tinfo);
		if (doy < 90) {
			// wrap it around to put it in range of the december solstice
			doy += 360;
		}
		return (ABSOLUTE(SOUTHERN_SOLSTICE_DOY - doy) == zenith_days);
	}
}


/**
* To be called at the end of the hourly update to show players sunrise/sunset.
*/
void send_hourly_sun_messages(void) {
	struct time_info_data tinfo;
	descriptor_data *desc;
	int sun;
	
	LL_FOREACH(descriptor_list, desc) {
		if (STATE(desc) != CON_PLAYING || desc->character == NULL) {
			continue;
		}
		if (!AWAKE(desc->character) || !IS_OUTDOORS(desc->character)) {
			continue;
		}
		
		// get local time
		tinfo = get_local_time(IN_ROOM(desc->character));
		
		// check for sun changes
		sun = get_sun_status(IN_ROOM(desc->character));
		if (sun != GET_LAST_LOOK_SUN(desc->character)) {
			qt_check_day_and_night(desc->character);
			GET_LAST_LOOK_SUN(desc->character) = sun;
			
			switch (sun) {
				case SUN_RISE: {
					if (SHOW_STATUS_MESSAGES(desc->character, SM_SUN_AUTO_LOOK) && !HAS_INFRA(desc->character) && !PRF_FLAGGED(desc->character, PRF_HOLYLIGHT) && !FIGHTING(desc->character)) {
						// show map if needed
						look_at_room(desc->character);
						msg_to_char(desc->character, "\r\n");
					}
					if (SHOW_STATUS_MESSAGES(desc->character, SM_SUN)) {
						msg_to_char(desc->character, "\t%cThe sun rises over the horizon.\t0\r\n", CUSTOM_COLOR_CHAR(desc->character, CUSTOM_COLOR_SUN));
					}
					break;
				}
				case SUN_LIGHT: {
					if (SHOW_STATUS_MESSAGES(desc->character, SM_SUN)) {
						msg_to_char(desc->character, "\t%cThe day has begun.\t0\r\n", CUSTOM_COLOR_CHAR(desc->character, CUSTOM_COLOR_SUN));
					}
					break;
				}
				case SUN_SET: {
					if (SHOW_STATUS_MESSAGES(desc->character, SM_SUN)) {
						msg_to_char(desc->character, "\t%cThe sun slowly disappears beneath the horizon.\t0\r\n", CUSTOM_COLOR_CHAR(desc->character, CUSTOM_COLOR_SUN));
					}
					break;
				}
				case SUN_DARK: {
					if (SHOW_STATUS_MESSAGES(desc->character, SM_SUN_AUTO_LOOK) && !HAS_INFRA(desc->character) && !PRF_FLAGGED(desc->character, PRF_HOLYLIGHT) && !FIGHTING(desc->character)) {
						look_at_room(desc->character);
						msg_to_char(desc->character, "\r\n");
					}
					if (SHOW_STATUS_MESSAGES(desc->character, SM_SUN)) {
						msg_to_char(desc->character, "\t%cThe night has begun.\t0\r\n", CUSTOM_COLOR_CHAR(desc->character, CUSTOM_COLOR_SUN));
					}
					break;
				}
			}
		}	// end sun-change
		
		// check and show zenith
		if (tinfo.hours == 12 && is_zenith_day(IN_ROOM(desc->character))) {
			// I think this should ignore the SM_SUN setting and show anyway -pc
			msg_to_char(desc->character, "\t%cYou watch as the sun passes directly overhead -- today is the zenith passage!\t0\r\n", CUSTOM_COLOR_CHAR(desc->character, CUSTOM_COLOR_SUN));
		}
	}
}


 //////////////////////////////////////////////////////////////////////////////
//// MOON SYSTEM /////////////////////////////////////////////////////////////

/**
* Determines the current phase of the moon based on how long its cycle is, and
* based on how long the mud has been alive.
*
* @param double cycle_days Number of days in the moon's cycle (Earth's moon is 29.53 days real time or 29.13 days game time).
* @return moon_phase_t One of the PHASE_ values.
*/
moon_phase_t get_moon_phase(double cycle_days) {
	double long_count_day, cycle_time;
	int phase;
	
	// exact number of days the mud has been running (all moons are 'new' at the time of DATA_START_WORLD)
	long_count_day = (double)(time(0) - data_get_long(DATA_WORLD_START)) / (double)SECS_PER_MUD_DAY;
	
	// determine how far into the current cycle we are
	if (cycle_days > 0.0) {
		cycle_time = fmod(long_count_day, cycle_days) / cycle_days;
	}
	else {	// div/0 safety
		cycle_time = 0.0;
	}
	
	phase = round((NUM_PHASES-1.0) * cycle_time);
	phase = MAX(0, MIN(NUM_PHASES-1, phase));
	return (moon_phase_t)phase;
}


/**
* Determines where the moon is in the sky. Phases rise roughly 3 hours apart.
*
* @param moon_phase_t phase Any moon phase.
* @param int hour Time of day from 0..23 hours.
*/
moon_pos_t get_moon_position(moon_phase_t phase, int hour) {
	int moonrise = (phase * 3) + 7, moonset = (phase * 3) + 19;
	double percent, pos;
	
	// check bounds/wraparound
	if (moonrise > 23) {
		moonrise -= 24;
	}
	if (moonset > 23) {
		moonset -= 24;
	}
	
	// shift moonset forward IF it's lower than moonrise (moon rises late)
	if (moonset < moonrise) {
		moonset += 24;	// tomorrow
	}
	
	// shift the incoming hour if it's before moonrise to see if it's in tomorrow's moonrise
	if (hour < moonrise) {
		hour += 24;
	}
	
	// now see how far inside or outside of this they are as a percentage
	percent = (double)(hour - moonrise) / (double)(moonset - moonrise);
	
	// outside of that range and the moon is not up
	if (percent <= 0.0 || percent >= 1.0) {
		return MOON_POS_DOWN;
	}
	
	pos = NUM_MOON_POS * percent;
	
	// determine which way to round to give some last-light
	if (percent < 0.5) {
		return (moon_pos_t) ceil(pos);
	}
	else {
		return (moon_pos_t) floor(pos);
	}
}


/**
* Computes the light radius at night based on any visible moon(s) in the sky.
*
* @param room_data *room A location.
* @return int Number of tiles you can see at night in that room.
*/
int compute_night_light_radius(room_data *room) {
	generic_data *moon, *next_gen;
	struct time_info_data tinfo;
	int dist, best = 0, second = 0;
	moon_phase_t phase;
	
	int max_light_radius_base = config_get_int("max_light_radius_base");
	
	tinfo = get_local_time(room);
	
	HASH_ITER(hh, generic_table, moon, next_gen) {
		if (GEN_TYPE(moon) != GENERIC_MOON || GET_MOON_CYCLE(moon) < 1 || GEN_FLAGGED(moon, GEN_IN_DEVELOPMENT)) {
			continue;	// not a moon or invalid cycle
		}
		phase = get_moon_phase(GET_MOON_CYCLE_DAYS(moon));
		if (get_moon_position(phase, tinfo.hours) != MOON_POS_DOWN) {
			// moon is up: record it if better
			if (moon_phase_brightness[phase] > best) {
				second = best;
				best = moon_phase_brightness[phase];
			}
			else if (moon_phase_brightness[phase] > second) {
				second = moon_phase_brightness[phase];
			}
		}
	}
	
	// compute
	dist = best + second/2;
	dist = MAX(1, MIN(dist, max_light_radius_base));
	return dist;
}


/**
* Lets a player look at a moon by name.
*
* @param char_data *ch The player.
* @param char *name The argument typed by the player after 'look [at]'.
* @param int *number Optional: For multi-list number targeting (look 4.moon; may be NULL)
*/
bool look_at_moon(char_data *ch, char *name, int *number) {
	char buf[MAX_STRING_LENGTH], copy[MAX_INPUT_LENGTH], *tmp = copy;
	struct time_info_data tinfo;
	generic_data *moon, *next_gen;
	moon_phase_t phase;
	moon_pos_t pos;
	int num;
	
	if (!IS_OUTDOORS(ch)) {
		return FALSE;
	}
	
	skip_spaces(&name);
	
	if (!number) {
		strcpy(tmp, name);
		number = &num;
		num = get_number(&tmp);
	}
	else {
		tmp = name;
	}
	if (*number == 0) {	// can't target 0.moon
		return FALSE;
	}
	
	tinfo = get_local_time(IN_ROOM(ch));
	
	HASH_ITER(hh, generic_table, moon, next_gen) {
		if (GEN_TYPE(moon) != GENERIC_MOON || GET_MOON_CYCLE(moon) < 1 || GEN_FLAGGED(moon, GEN_IN_DEVELOPMENT)) {
			continue;	// not a moon or invalid cycle
		}
		if (str_cmp(tmp, "moon") && !isname(tmp, GEN_NAME(moon))) {
			continue;	// not a name match (or "look 2.moon")
		}
		
		// find moon in the sky
		phase = get_moon_phase(GET_MOON_CYCLE_DAYS(moon));
		pos = get_moon_position(phase, tinfo.hours);
		
		// qualify it some more -- allow new moon in direct sunlight (unlike show-visible-moons)
		if (pos == MOON_POS_DOWN) {
			continue;	// moon is down
		}
		if (--(*number) > 0) {
			continue;	// number-dot syntax
		}
		
		// ok: show it
		snprintf(buf, sizeof(buf), "%s is %s, %s.\r\n", GEN_NAME(moon), moon_phases_long[phase], moon_positions[pos]);
		send_to_char(CAP(buf), ch);
		act("$n looks at $t.", TRUE, ch, GEN_NAME(moon), NULL, TO_ROOM | ACT_STR_OBJ);
		return TRUE;
	}
	
	return FALSE;	// nope
}


/**
* Displays any visible moons to the player, one per line.
*
* @param char_data *ch The person to show the moons to.
*/
void show_visible_moons(char_data *ch) {
	struct time_info_data tinfo;
	char buf[MAX_STRING_LENGTH];
	generic_data *moon, *next_gen;
	moon_phase_t phase;
	moon_pos_t pos;
	
	tinfo = get_local_time(IN_ROOM(ch));
	
	HASH_ITER(hh, generic_table, moon, next_gen) {
		if (GEN_TYPE(moon) != GENERIC_MOON || GET_MOON_CYCLE(moon) < 1 || GEN_FLAGGED(moon, GEN_IN_DEVELOPMENT)) {
			continue;	// not a moon or invalid cycle
		}
		
		// find moon in the sky
		phase = get_moon_phase(GET_MOON_CYCLE_DAYS(moon));
		pos = get_moon_position(phase, tinfo.hours);
		
		// qualify it some more
		if (pos == MOON_POS_DOWN) {
			continue;	// moon is down
		}
		if (phase == PHASE_NEW && get_sun_status(IN_ROOM(ch)) == SUN_LIGHT) {
			continue;	// new moon not visible in strong sunlight
		}
		
		// ok: show it
		snprintf(buf, sizeof(buf), "%s is %s, %s.\r\n", GEN_NAME(moon), moon_phases_long[phase], moon_positions[pos]);
		send_to_char(CAP(buf), ch);
	}
}


 //////////////////////////////////////////////////////////////////////////////
//// TEMPERATURE SYSTEM //////////////////////////////////////////////////////

/**
* Modifies a base temperature based on a TEMPERATURE_ type const.
*
* @param int base_temp The base temperature, before modifications.
* @param int temp_type Any TEMPERATURE_ type const.
* @return int The final temperature.
*/
int apply_temperature_type(int base_temp, int temp_type) {
	int temperature = base_temp;
	
	// TEMPERATURE_x: modifications based on temperature
	switch (temp_type) {
		case TEMPERATURE_FREEZING: {
			temperature = -1 * config_get_int("temperature_extreme");
			break;
		}
		case TEMPERATURE_COLD: {
			temperature = -1 * config_get_int("temperature_discomfort");
			break;
		}
		case TEMPERATURE_COOL: {
			temperature = -1 * config_get_int("temperature_discomfort") + 1;
			break;
		}
		case TEMPERATURE_NEUTRAL: {
			temperature = 0;
			break;
		}
		case TEMPERATURE_WARM: {
			temperature = config_get_int("temperature_discomfort") - 1;
			break;
		}
		case TEMPERATURE_HOT: {
			temperature = config_get_int("temperature_discomfort");
			break;
		}
		case TEMPERATURE_SWELTERING: {
			temperature = config_get_int("temperature_extreme");
			break;
		}
		case TEMPERATURE_MILDER: {
			if (temperature > 0) {
				temperature = MAX(0, temperature - 20);
			}
			else if (temperature < 0) {
				temperature = MIN(0, temperature + 20);
			}
			break;
		}
		case TEMPERATURE_HARSHER: {
			if (temperature > 0) {
				temperature += 20;
			}
			else if (temperature < 0) {
				temperature -= 20;
			}
			break;
		}
		case TEMPERATURE_COOLER: {
			temperature -= 15;
			break;
		}
		case TEMPERATURE_COOLER_WHEN_HOT: {
			if (temperature > 0) {
				temperature = MAX(0, temperature - 15);
			}
			break;
		}
		case TEMPERATURE_WARMER: {
			temperature += 15;
			break;
		}
		case TEMPERATURE_WARMER_WHEN_COLD: {
			if (temperature < 0) {
				temperature = MIN(0, temperature + 15);
			}
			break;
		}
	}
	
	return temperature;
}


/**
* Calculates temperature for a hypothetical room based on various data. This
* can be used for real rooms or for hypotheticals.
*
* @param int temp_type Any TEMPERATURE_ type const; pass TEMPERATURE_USE_CLIMATE as a default.
* @param bitvector_t climates Any CLIM_ flags that apply here.
* @param int season Any TILESET_ season const.
* @param int sun Any SUN_ const.
* @return int The rounded temperature value.
*/
int calculate_temperature(int temp_type, bitvector_t climates, int season, int sun) {
	int climate_val, season_count, season_val, sun_count, sun_val, bit;
	double season_mod, sun_mod, temperature, cold_mod, heat_mod;
	
	// init
	climate_val = 0;
	season_val = season_temperature[season];
	sun_val = sun_temperature[sun];
	season_count = sun_count = 0;
	season_mod = sun_mod = 0.0;
	cold_mod = heat_mod = 1.0;
	
	// determine climate modifiers
	for (bit = 0; climates; ++bit, climates >>= 1) {
		if (IS_SET(climates, BIT(0))) {
			climate_val += climate_temperature[bit].base_add;
			
			if (climate_temperature[bit].season_weight != NO_TEMP_MOD) {
				season_mod += climate_temperature[bit].season_weight;
				++season_count;
			}
			if (climate_temperature[bit].sun_weight != NO_TEMP_MOD) {
				sun_mod += climate_temperature[bit].sun_weight;
				++sun_count;
			}
			if (climate_temperature[bit].cold_modifier != NO_TEMP_MOD) {
				cold_mod *= climate_temperature[bit].cold_modifier;
			}
			if (climate_temperature[bit].heat_modifier != NO_TEMP_MOD) {
				heat_mod *= climate_temperature[bit].heat_modifier;
			}
		}
	}
	
	// final math
	temperature = climate_val;
	
	if (season_count > 0) {
		season_mod /= season_count;
		temperature += season_val * season_mod;
	}
	else {
		temperature += sun_val;
	}
	
	if (sun_count > 0) {
		sun_mod /= sun_count;
		temperature += sun_val * sun_mod;
	}
	else {
		temperature += sun_val;
	}
	
	// overall modifiers
	if (temperature < 0) {
		temperature *= cold_mod;
	}
	if (temperature > 0) {
		temperature *= heat_mod;
	}
	
	return apply_temperature_type(round(temperature), temp_type);
}


/**
* Cancels all affects caused by temperature.
*
* @param char_data *ch The player.
* @param int keep_type If not NOTHING, will keep an affect of this type ONLY.
* @param bool send_messages If TRUE, sends wear-off messages for the affects.
*/
void cancel_temperature_penalties(char_data *ch, int keep_type, bool send_messages) {
	int iter;
	
	// terminate this list with a NOTHING
	any_vnum type_list[] = { ATYPE_COOL_PENALTY, ATYPE_COLD_PENALTY, ATYPE_WARM_PENALTY, ATYPE_HOT_PENALTY, NOTHING };
	
	for (iter = 0; type_list[iter] != NOTHING; ++iter) {
		if (type_list[iter] != keep_type) {
			affect_from_char(ch, type_list[iter], send_messages);
		}
	}
}


/**
* Checks whether a player should have penalties from high or low temperature,
* adds them if needed, or removes them if not.
*
* @param char_data *ch The player.
*/
void check_temperature_penalties(char_data *ch) {
	int atype, iter, limit, extreme, room_temp, temperature;
	struct affected_type *af;
	obj_data *obj;
	bool any, room_safe;
	char buf[MAX_STRING_LENGTH];
	
	// some items provide warmth when lit
	#define IS_WARM_OBJ(obj)  (GET_LIGHT_IS_LIT(obj) && LIGHT_FLAGGED((obj), LIGHT_FLAG_LIGHT_FIRE | LIGHT_FLAG_COOKING_FIRE))
	
	if (IS_NPC(ch) || !IN_ROOM(ch)) {
		return;	// no temperature
	}
	if (IS_GOD(ch) || IS_IMMORTAL(ch) || AFF_FLAGGED(ch, AFF_IMMUNE_TEMPERATURE) || ISLAND_FLAGGED(IN_ROOM(ch), ISLE_NO_TEMPERATURE_PENALTIES) || !config_get_bool("temperature_penalties") || get_temperature_type(IN_ROOM(ch)) == TEMPERATURE_ALWAYS_COMFORTABLE) {
		// no penalties-- remove them, though, in case it was just shut off
		cancel_temperature_penalties(ch, NOTHING, TRUE);
		return;
	}
	
	// base temperature and numbers
	limit = config_get_int("temperature_discomfort");
	extreme = config_get_int("temperature_extreme");
	temperature = get_relative_temperature(ch);
	room_temp = get_room_temperature(IN_ROOM(ch));
	room_safe = (room_temp < limit && room_temp > (-1 * limit));
	if (extreme < limit) {
		extreme = limit;	// in case it's not set correctly
	}
	
	// little fires everywhere (only if cold)
	if (temperature < 0) {
		// look for a warm obj
		any = FALSE;
		
		// room
		if (!any) {
			DL_FOREACH2(ROOM_CONTENTS(IN_ROOM(ch)), obj, next_content) {
				if (IS_WARM_OBJ(obj)) {
					any = TRUE;
					break;	// only need one
				}
			}
		}
		
		// equipped
		for (iter = 0; iter < NUM_WEARS && !any; ++iter) {
			if ((obj = GET_EQ(ch, iter)) && IS_WARM_OBJ(obj)) {
				any = TRUE;
			}
		}
		
		// inventory
		if (!any) {
			DL_FOREACH2(ch->carrying, obj, next_content) {
				if (IS_WARM_OBJ(obj)) {
					any = TRUE;
					break;	// only need one
				}
			}
		}
		
		if (any) {
			temperature += config_get_int("temperature_from_fire");
		}
	}
	else if (temperature > 0) {
		// look for temperature relief from water (any water tile + not sitting on a vehicle)
		if ((ROOM_SECT_FLAGGED(IN_ROOM(ch), SECTF_SHALLOW_WATER) || WATER_SECT(IN_ROOM(ch))) && !GET_SITTING_ON(ch)) {
			temperature -= config_get_int("temperature_from_water");
		}
	}
	
	// do we need penalties
	if (ABSOLUTE(temperature) >= limit && !room_safe) {
		if (temperature > 0) {
			// start HOT penalty
			if (temperature >= extreme) {
				// extreme hot
				atype = ATYPE_HOT_PENALTY;
				cancel_temperature_penalties(ch, atype, FALSE);
				
				// message only if it's new
				if (SHOW_STATUS_MESSAGES(ch, SM_TEMPERATURE | SM_EXTREME_TEMPERATURE) && !affected_by_spell(ch, atype)) {
					snprintf(buf, sizeof(buf), "\t%cYou start to feel faint in the sweltering temperature -- you're too hot!\t0", CUSTOM_COLOR_CHAR(ch, CUSTOM_COLOR_TEMPERATURE));
					act(buf, FALSE, ch, NULL, NULL, TO_CHAR | TO_SLEEP);
				}
				
				// pain
				apply_dot_effect(ch, atype, MAX(SECS_PER_REAL_UPDATE, SECS_PER_MUD_HOUR), DAM_DIRECT, 5, 1000, ch);
				af = create_flag_aff(atype, UNLIMITED, AFF_DISTRACTED, ch);
				affect_join(ch, af, NOBITS);
			}
			else {
				// mild hot
				atype = ATYPE_WARM_PENALTY;
				cancel_temperature_penalties(ch, atype, FALSE);
				
				// message only if it's new
				if (SHOW_STATUS_MESSAGES(ch, SM_TEMPERATURE | SM_EXTREME_TEMPERATURE) && !affected_by_spell(ch, atype)) {
					snprintf(buf, sizeof(buf), "\t%cYou start to feel like you're getting too warm.\t0", CUSTOM_COLOR_CHAR(ch, CUSTOM_COLOR_TEMPERATURE));
					act(buf, FALSE, ch, NULL, NULL, TO_CHAR | TO_SLEEP);
				}
				
				// discomfort
				af = create_flag_aff(atype, UNLIMITED, AFF_SLOWER_ACTIONS, ch);
				affect_join(ch, af, NOBITS);
			}
			
			// stats penalties (warm/hot)
			af = create_flag_aff(atype, UNLIMITED, AFF_POOR_REGENS | AFF_THIRSTIER, ch);
			affect_join(ch, af, NOBITS);
		}
		else {
			// start COLD penalty
			if (temperature <= -1 * extreme) {
				// extreme cold
				atype = ATYPE_COLD_PENALTY;
				cancel_temperature_penalties(ch, atype, FALSE);
				
				// message only if it's new
				if (SHOW_STATUS_MESSAGES(ch, SM_TEMPERATURE | SM_EXTREME_TEMPERATURE) && !affected_by_spell(ch, atype)) {
					snprintf(buf, sizeof(buf), "\t%cThe bitter cold is starting to get to you -- you're freezing!\t0", CUSTOM_COLOR_CHAR(ch, CUSTOM_COLOR_TEMPERATURE));
					act(buf, FALSE, ch, NULL, NULL, TO_CHAR | TO_SLEEP);
				}
				
				// pain
				apply_dot_effect(ch, atype, MAX(SECS_PER_REAL_UPDATE, SECS_PER_MUD_HOUR), DAM_DIRECT, 5, 1000, ch);
				af = create_flag_aff(atype, UNLIMITED, AFF_DISTRACTED, ch);
				affect_join(ch, af, NOBITS);
			}
			else {
				// mild cold
				atype = ATYPE_COOL_PENALTY;
				cancel_temperature_penalties(ch, atype, FALSE);
				
				// message only if it's new
				if (SHOW_STATUS_MESSAGES(ch, SM_TEMPERATURE | SM_EXTREME_TEMPERATURE) && !affected_by_spell(ch, atype)) {
					snprintf(buf, sizeof(buf), "\t%cYou're starting to feel a little too cold.\t0", CUSTOM_COLOR_CHAR(ch, CUSTOM_COLOR_TEMPERATURE));
					act(buf, FALSE, ch, NULL, NULL, TO_CHAR | TO_SLEEP);
				}
				
				af = create_flag_aff(atype, UNLIMITED, AFF_SLOWER_ACTIONS, ch);
				affect_join(ch, af, NOBITS);
			}
			
			// stats penalties (cool/cold)
			af = create_flag_aff(atype, UNLIMITED, AFF_SLOW | AFF_HUNGRIER, ch);
			affect_join(ch, af, NOBITS);
		}
	}
	else if (ABSOLUTE(temperature) < limit) {
		// remove all penalties if the player has cooled down or warmed up
		cancel_temperature_penalties(ch, NOTHING, TRUE);
	}
}


/**
* Computes a player's current temperature including any warmth/cooling gear
* and effects.
*
* Warmth is penalized by half of the player's cooling attribute, and vice
* versa, meaning if a player has 100 warmth and 20 cooling, they are really
* at 90 warmth.
*
* @param char_data *ch The player (NPCs always return 0).
* @return int The adjusted temperature that the character is feeling (usually -100 to 100; 0 is pleasant).
*/
int get_relative_temperature(char_data *ch) {
	int temp, warm, cool;
	
	if (IS_NPC(ch)) {
		return 0;	// NPCs do not have this property
	}
	if (get_temperature_type(IN_ROOM(ch)) == TEMPERATURE_ALWAYS_COMFORTABLE) {
		return 0;	// always comfortable here
	}
	
	temp = GET_TEMPERATURE(ch);
	warm = GET_WARMTH(ch);
	warm = MAX(0, warm);		// do not apply negative warmth
	cool = GET_COOLING(ch);
	cool = MAX(0, cool);		// do not apply negative cooling
	
	// penalize half of the other trait
	if (temp < 0) {
		temp += warm - (cool / 2);
	}
	else if (temp > 0) {
		temp -= cool + (warm / 2);
	}
	
	// character bonus
	if (HAS_BONUS_TRAIT(ch, BONUS_WARM_RESIST) && temp > 0) {
		temp = MAX(0, temp - 10);
	}
	else if (HAS_BONUS_TRAIT(ch, BONUS_COLD_RESIST) && temp < 0) {
		temp = MIN(0, temp + 10);
	}
	
	return temp;
}


/**
* Determine the temperature of a room. Positive numbers are hot and negative
* numbers are cold. These do not use real-life units; temperature is counter-
* balanced by a player's WARMTH or COOLING trait.
*
* @param room_data *room Get the temperature for this room.
* @return int The temperature (zero is neutral).
*/
int get_room_temperature(room_data *room) {
	int temperature;
	room_data *home;
	
	if (!IS_ADVENTURE_ROOM(room) && (home = HOME_ROOM(room)) != room) {
		// inside of a building: get home temperature then apply own temperature type to it
		temperature = get_room_temperature(home);
		temperature = apply_temperature_type(temperature, get_temperature_type(room));
	}
	else {
		// normal room
		temperature = calculate_temperature(get_temperature_type(room), get_climate(room), GET_SEASON(room), get_sun_status(room));
	}
	
	return temperature;
}


/**
* Determines what TEMPERATURE_ type const to use for a given room, based on
* whether or not it's in a building or adventure.
*
* @param room_data *room The room to check.
* @return int Any TEMPERATURE_ type const.
*/
int get_temperature_type(room_data *room) {
	int ttype = TEMPERATURE_USE_CLIMATE;
	
	if (!room) {
		return ttype;	// missing arg?
	}
	
	// check for a temperature type (building or room template)
	if (GET_BUILDING(room) && IS_COMPLETE(room)) {
		ttype = GET_BLD_TEMPERATURE_TYPE(GET_BUILDING(room));
	}
	else if (GET_ROOM_TEMPLATE(room)) {
		ttype = GET_RMT_TEMPERATURE_TYPE(GET_ROOM_TEMPLATE(room));
	}
	else {
		// try sector(s)
		ttype = GET_SECT_TEMPERATURE_TYPE(SECT(room));
		
		// attempt base if cascade is required here
		if (ttype == TEMPERATURE_USE_CLIMATE && ROOM_SECT_FLAGGED(room, SECTF_INHERIT_BASE_CLIMATE)) {
			ttype = GET_SECT_TEMPERATURE_TYPE(BASE_SECT(room));
		}
	}
	
	// check adventure, too, IF we're on use-climate
	if (ttype == TEMPERATURE_USE_CLIMATE && COMPLEX_DATA(room) && COMPLEX_DATA(room)->instance) {
		ttype = GET_ADV_TEMPERATURE_TYPE(INST_ADVENTURE(COMPLEX_DATA(room)->instance));
	}
	
	return ttype;
}


/**
* Initializes a player's temperature, generally when they get a free restore on
* login. If they would be comfortable, sets them to room temperature. Otherwise
* it will set them to neutral temperature.
*
* @param char_data *ch The player.
*/
void reset_player_temperature(char_data *ch) {
	int room_temp, warm, cool, limit;
	
	if (IS_NPC(ch) || !IN_ROOM(ch)) {
		return;	// no temperature
	}
	
	// basic values
	room_temp = get_room_temperature(IN_ROOM(ch));
	limit = config_get_int("temperature_discomfort");
	warm = MAX(0, GET_WARMTH(ch));
	cool = MAX(0, GET_COOLING(ch));
	
	if (room_temp > 0 && (room_temp - cool + (warm / 2)) >= limit) {
		// going to be too warm -- start at 0
		GET_TEMPERATURE(ch) = 0;
	}
	else if (room_temp < 0 && (room_temp + warm - (cool / 2)) <= limit) {
		// going to be too cold -- start at 0
		GET_TEMPERATURE(ch) = 0;
	}
	else {
		// player should be comfortable -- start at room temp
		GET_TEMPERATURE(ch) = room_temp;
	}
}


/**
* Gives a user-readable word for a given temperature.
*
* @param int temperature A temperature (normally -100 to 100).
* @return const char* An adjective for it such as "chilly" or "sweltering".
*/
const char *temperature_to_string(int temperature) {
	int iter;
	
	struct temperature_name_t {
		int min_temp;
		const char *text;
	} temperature_name[] = {
		// { over temp, show text }
		{ INT_MIN, "freezing" },
		{ -49, "frigid" },
		{ -39, "icy" },
		{ -34, "frosty" },
		{ -29, "cold" },
		{ -24, "chilly" },
		{ -19, "cool" },
		{ -9, "pleasant" },
		{ 10, "balmy" },
		{ 20, "warm" },
		{ 25, "hot" },
		{ 30, "scorching" },
		{ 35, "sweltering" },
		{ 40, "blistering" },
		{ 50, "searing" },

		{ INT_MAX, "\n" }	// must be last
	};
	
	for (iter = 0; *temperature_name[iter].text != '\n'; ++iter) {
		if (temperature_name[iter].min_temp <= temperature && temperature_name[iter+1].min_temp > temperature) {
			return temperature_name[iter].text;
		}
	}
	
	// should not get here but
	return "searing";
}


/**
* Shifts the player's temperature slightly toward room temperature. Ideally
* this should run during the 5-second "real updates".
*
* @param char_data *ch The player experiencing temperature and life.
*/
void update_player_temperature(char_data *ch) {
	int ambient, was_temp, relative, limit;
	double change;
	bool gain = FALSE, loss = FALSE;
	bool showed_warm_room = FALSE, showed_cold_room = FALSE;
	
	if (IS_NPC(ch) || !IN_ROOM(ch)) {
		return;	// no temperature
	}
	
	ambient = get_room_temperature(IN_ROOM(ch));
	limit = config_get_int("temperature_discomfort");
	
	// check if room itself changed
	if (AWAKE(ch) && ambient != GET_LAST_MESSAGED_TEMPERATURE(ch) && SHOW_STATUS_MESSAGES(ch, SM_TEMPERATURE) && get_temperature_type(IN_ROOM(ch)) != TEMPERATURE_ALWAYS_COMFORTABLE && !ROOM_AFF_FLAGGED(IN_ROOM(ch), ROOM_AFF_NO_WEATHER)) {
		if (ambient > GET_LAST_MESSAGED_TEMPERATURE(ch)) {
			// higher temp
			msg_to_char(ch, "\t%cIt's %s %s%s.\t0\r\n", CUSTOM_COLOR_CHAR(ch, CUSTOM_COLOR_TEMPERATURE), (ambient >= limit) ? "getting hot" : "warming up", IS_OUTDOORS(ch) ? "out here" : "in here", (ambient <= (-1 * limit) ? ", but still quite cold" : ""));
			showed_warm_room = TRUE;
		}
		else {	// lower temp
			msg_to_char(ch, "\t%cIt's %s %s%s.\t0\r\n", CUSTOM_COLOR_CHAR(ch, CUSTOM_COLOR_TEMPERATURE), (ambient <= -1 * limit) ? "getting cold" : "cooling down", IS_OUTDOORS(ch) ? "out here" : "in here", (ambient >= limit ? ", but still too hot" : ""));
			showed_cold_room = TRUE;
		}
		
		// update temp for later
		GET_LAST_MESSAGED_TEMPERATURE(ch) = ambient;
	}
	
	// check player's own temperature
	if (GET_TEMPERATURE(ch) != ambient) {
		was_temp = get_relative_temperature(ch);
		change = (double)ambient / ((double) SECS_PER_MUD_HOUR / SECS_PER_REAL_UPDATE);
		change = ABSOLUTE(change);	// change is positive
		change = MAX(1.0, change);	// minimum of 1
		
		// apply
		if (GET_TEMPERATURE(ch) < ambient) {
			GET_TEMPERATURE(ch) += change;
			GET_TEMPERATURE(ch) = MIN(ambient, GET_TEMPERATURE(ch));
			gain = TRUE;
		}
		else {
			GET_TEMPERATURE(ch) -= change;
			GET_TEMPERATURE(ch) = MAX(ambient, GET_TEMPERATURE(ch));
			loss = TRUE;
		}
		
		// messaging?
		if (SHOW_STATUS_MESSAGES(ch, SM_TEMPERATURE | SM_EXTREME_TEMPERATURE) && get_temperature_type(IN_ROOM(ch)) != TEMPERATURE_ALWAYS_COMFORTABLE) {
			relative = get_relative_temperature(ch);
			
			if (gain && relative > was_temp && GET_LAST_WARM_TIME(ch) < time(0) - 60) {
				if (relative >= limit - (limit / 10) && SHOW_STATUS_MESSAGES(ch, SM_TEMPERATURE | SM_EXTREME_TEMPERATURE)) {
					msg_to_char(ch, "\t%cYou're getting too hot!\t0\r\n", CUSTOM_COLOR_CHAR(ch, CUSTOM_COLOR_TEMPERATURE));
				}
				else if (!showed_warm_room && relative >= (limit / 2) && SHOW_STATUS_MESSAGES(ch, SM_TEMPERATURE)) {
					msg_to_char(ch, "\t%cYou're getting warm.\t0\r\n", CUSTOM_COLOR_CHAR(ch, CUSTOM_COLOR_TEMPERATURE));
				}
				else if (!showed_warm_room && relative <= (-1 * limit) && SHOW_STATUS_MESSAGES(ch, SM_TEMPERATURE)) {
					msg_to_char(ch, "\t%cYou're warming up%s.\t0\r\n", CUSTOM_COLOR_CHAR(ch, CUSTOM_COLOR_TEMPERATURE), (relative <= -1 * limit) ? " but still quite cold" : "");
				}
			
				GET_LAST_WARM_TIME(ch) = time(0);
			}
			else if (loss && relative < was_temp && GET_LAST_COLD_TIME(ch) < time(0) - 60) {
				limit *= -1;	// negative limit
				if (relative <= (limit + (limit / -10)) && SHOW_STATUS_MESSAGES(ch, SM_TEMPERATURE | SM_EXTREME_TEMPERATURE)) {
					msg_to_char(ch, "\t%cYou're getting too cold!\t0\r\n", CUSTOM_COLOR_CHAR(ch, CUSTOM_COLOR_TEMPERATURE));
				}
				else if (!showed_cold_room && relative <= (limit / 2) && SHOW_STATUS_MESSAGES(ch, SM_TEMPERATURE)) {
					msg_to_char(ch, "\t%cYou're getting cold.\t0\r\n", CUSTOM_COLOR_CHAR(ch, CUSTOM_COLOR_TEMPERATURE));
				}
				else  if (!showed_cold_room && relative >= (-1 * limit) && SHOW_STATUS_MESSAGES(ch, SM_TEMPERATURE)) {
					msg_to_char(ch, "\t%cYou're cooling down%s.\t0\r\n", CUSTOM_COLOR_CHAR(ch, CUSTOM_COLOR_TEMPERATURE), (relative >= -1 * limit) ? " but still rather warm" : "");
				}
			
				GET_LAST_COLD_TIME(ch) = time(0);
			}
		}
		
		// probably not worth triggering a character save for a temperature change alone
		// ... but they almost certainly save when this runs, anyway, due to other events
		
		// update MSDP under the assumption there was a change
		update_MSDP_temperature(ch, FALSE, UPDATE_SOON);
	}
}


/**
* Warms (or cools) a player from drinking a liquid.
*
* @param char_data *ch The player who drank the liquid.
* @param int hours_drank How much they drank (in game hours).
* @param any_vnum liquid Which generic liquid vnum they drank.
* @return int 1 if it warmed the player up at all; -1 if it cooled them down; 0 if neither or both.
*/
int warm_player_from_liquid(char_data *ch, int hours_drank, any_vnum liquid) {
	generic_data *gen = real_generic(liquid);
	int val = 0, ambient, amount;
	bool any = FALSE;
	
	if (!ch || !gen || hours_drank < 1 || GEN_TYPE(gen) != GENERIC_LIQUID) {
		return 0;	// no work
	}
	
	// how much heating/cooling is possible
	amount = round(hours_drank / 4.0);
	amount = MAX(1, amount);
	
	ambient = get_room_temperature(IN_ROOM(ch));
	
	if (IS_SET(GET_LIQUID_FLAGS(gen), LIQF_COOLING) && GET_TEMPERATURE(ch) > ambient - 5) {
		// cool
		GET_TEMPERATURE(ch) -= amount;
		
		// don't cool past 0 from drinking
		GET_TEMPERATURE(ch) = MAX(0, GET_TEMPERATURE(ch));
		
		val -= 1;
		any = TRUE;
	}
	
	if (IS_SET(GET_LIQUID_FLAGS(gen), LIQF_WARMING) && GET_TEMPERATURE(ch) < ambient + 5) {
		// warm
		GET_TEMPERATURE(ch) += amount;
		
		// don't warm past 0 from drinking
		GET_TEMPERATURE(ch) = MIN(0, GET_TEMPERATURE(ch));
		
		val += 1;
		any = TRUE;
	}
	
	if (any) {
		update_MSDP_temperature(ch, FALSE, UPDATE_SOON);
	}
	
	return val;
}
