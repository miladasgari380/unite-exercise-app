

/*
 * Modified By: Milad Asgari
 * Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Licensed under the Apache License, Version 2.0 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Useful links:
 * https://developer.tizen.org/ko/forums/native-application-development/register-back-callback-tiny-application.?langswitch=ko
 *
 */

#include <sensor.h>
#include "hrmexercise.h"
#include <tizen.h>
#include <device/power.h>
#include <feedback.h>
#include <device/haptic.h>

#define BUFLEN 200
#define MAX_HR 152

Evas_Object *GLOBAL_DEBUG_BOX;
Evas_Object *start, *stop;
Evas_Object *conform;
Evas_Object *box;
sensor_listener_h listener;
Evas_Object *event_label;
Evas_Object *event_label_animation;
Evas_Object *datetime;
long long time_ref;


#define arraySize  72000 // 60 minutes

static int ppgCounter = 0;
static int accCounter= 0;
static int gravityCounter = 0;
static int hrmCounter = 0;
static int gyroscpoeCounter = 0;
static int pressureCounter = 0;
static int stressCounter = 0;
typedef struct collect {
	float ppg;
	float hrm;
	float accx;
	float accy;
	float accz;
	float grax;
	float gray;
	float graz;
	float gyrx;
	float gyry;
	float gyrz;
	float pressure;
	float stress;
	unsigned long long timestamp ;
}collectedData ;
collectedData allcollectedData[arraySize];

#define SENSORNUM 6
sensor_type_e sensorTypeList[] = {SENSOR_HRM_LED_GREEN, SENSOR_HRM, SENSOR_ACCELEROMETER,
				SENSOR_GRAVITY, SENSOR_GYROSCOPE, SENSOR_PRESSURE};
sensor_listener_h listenerList [SENSORNUM];
sensor_h sensorList [SENSORNUM];

bool writeToFileFlag = false;
bool recreate_UI = false;
bool vibrate = false;

char time_file[256];

void sensorWriteToFile(){
	//Write the PPG values in the file
	char filename[256];
	sprintf(filename, "/opt/usr/media/Downloads/data_%s_exercise.csv",time_file);
	FILE * fp = fopen(filename, "w");
	fprintf(fp , "timestamp\tppg\thrm\taccx\taccy\taccz\tgrax\tgray\tgraz\tgyrx\tgyry\tgrz\tpressure\tstress\n");
	for(int i = 0; i < hrmCounter; i++ ) {
		fprintf (fp, "%llu\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\n",
				allcollectedData[i].timestamp,allcollectedData[i].ppg,allcollectedData[i].hrm,
				allcollectedData[i].accx,allcollectedData[i].accy, allcollectedData[i].accz,
				allcollectedData[i].grax, allcollectedData[i].gray, allcollectedData[i].graz,
				allcollectedData[i].gyrx, allcollectedData[i].gyry, allcollectedData[i].gyrz,
				allcollectedData[i].pressure, allcollectedData[i].stress);
	}
	fclose (fp);
}

void setTimeForFileName(){ // set time in time_file variable
//The current time for the filename
	time_t raw_time;
	struct tm* time_info;
	time(&raw_time);
	time_info = localtime(&raw_time);
	sprintf(time_file,"%d%s%d%s%d%s%d%s%d",(time_info->tm_year+1900), time_info->tm_mon+1<10? "0" : "", time_info->tm_mon+1, time_info->tm_mday<10? "0" : "", time_info->tm_mday, time_info->tm_hour<10? "0" : "", time_info->tm_hour, time_info->tm_min<10? "0" : "", time_info->tm_min);

}

unsigned long long getCurrentTimeStamp (){
	struct timespec spec;
	clock_gettime(CLOCK_REALTIME, &spec);
	unsigned long long ts = spec.tv_sec * 1000LL + spec.tv_nsec / 1000000LL;
	return ts;
}


void on_sensor_event(sensor_h sensor, sensor_event_s *event, void *user_data)
{
    // Select a specific sensor with a sensor handle
    sensor_type_e type;
    sensor_get_type(sensor, &type);

    switch (type) {
    case SENSOR_HRM:
    	dlog_print(DLOG_INFO, LOG_TAG, "%d" , event->values[0]);

    	allcollectedData[hrmCounter].timestamp = getCurrentTimeStamp();
    	allcollectedData[hrmCounter].hrm = event->values[0];
    	hrmCounter++;

    	allcollectedData[stressCounter].stress = 0;
    	stressCounter++;

    	// stopwatch
    	char stopwatch_time[100];
    	long long current_time = getCurrentTimeStamp();
    	int time_diff = current_time - time_ref;
    	int sec = time_diff / 1000;
    	int min = sec / 60;
    	int hour = min / 60;
    	sec = sec % 60;
    	min = min % 60;
    	sprintf(stopwatch_time,"<font_size=50><color=#ffffff>%02d:%02d:%02d</color></font_size>", hour, min, sec);
    	elm_object_text_set(datetime, stopwatch_time);

    	char a[100];
    	if(event->values[0] < MAX_HR){
    		//sprintf(a,"%d", (int)event->values[0]);
    		sprintf(a,"<color=#29AB87>%d</color>", (int)event->values[0]);
    		elm_object_text_set(event_label, a);
    		vibrate = true;
    	}
    	else{
    		sprintf(a,"<color=#FF0800>%d</color>", (int)event->values[0]);
    		elm_object_text_set(event_label,  _(a));

    		// Vibration
    		if (vibrate){
				int error, num;
				error = device_haptic_get_count(&num);
				haptic_device_h handle;
				haptic_effect_h effect_handle;
				error = device_haptic_open(0, &handle);
				error = device_haptic_vibrate(handle, 5000, 50, &effect_handle);
				vibrate = false;
    		}
    	}
    	elm_object_text_set(event_label_animation,  "❤");
    	evas_object_show(event_label_animation);
    	//elm_object_style_set(event_label_animation, "slide_bounce");

    	if ((int)event->values[0] != 0 && hrmCounter % (int)((60.0 * 20.0)/(float)event->values[0]) == 0) {
			Elm_Transit *transit = elm_transit_add();
			elm_transit_object_add(transit, event_label_animation);
			elm_transit_effect_zoom_add(transit, 0.5, 1.5);
			// elm_transit_duration_set(transit, (60.0/(float)event->values[0]));
			elm_transit_duration_set(transit, (0.5));
			elm_transit_go(transit);
    	}
    	break;
    case SENSOR_HRM_LED_GREEN:
    		allcollectedData[ppgCounter].ppg = event->values[0];
    		ppgCounter++;
    		if (ppgCounter % 7200 == 0){ // every 12 minutues
			int error;
			error = device_power_release_lock(POWER_LOCK_CPU);
			error = device_power_request_lock(POWER_LOCK_CPU, 0);
		}
    		break;
    case SENSOR_ACCELEROMETER:
    		allcollectedData[accCounter].accx = event->values[0];
    		allcollectedData[accCounter].accy = event->values[1];
    		allcollectedData[accCounter].accz = event->values[2];
    		accCounter++;
    		break;

    case SENSOR_GRAVITY:
    		allcollectedData[gravityCounter].grax = event->values[0];
    		allcollectedData[gravityCounter].gray = event->values[1];
    		allcollectedData[gravityCounter].graz = event->values[2];
    		gravityCounter++;
    		break;
    case SENSOR_GYROSCOPE:
    		allcollectedData[gyroscpoeCounter].gyrx = event->values[0];
    		allcollectedData[gyroscpoeCounter].gyry = event->values[1];
    		allcollectedData[gyroscpoeCounter].gyrz = event->values[2];
    		gyroscpoeCounter++;
    		break;
    case SENSOR_PRESSURE:
    		allcollectedData[pressureCounter].pressure = event->values[0];
    		pressureCounter++;
    		break;

    default:
        dlog_print(DLOG_ERROR, LOG_TAG, "Not an HRM event");
    }
}

void _sensor_accuracy_changed_cb(sensor_h sensor, unsigned long long timestamp,
                                 sensor_data_accuracy_e accuracy, void *data)
{
    dlog_print(DLOG_DEBUG, LOG_TAG, "Sensor accuracy change callback invoked");
}

void _sensor_start_cb(void *data, Evas_Object *obj, void *event_info)
{
    void *user_data = NULL;
    char out[100];

    int err;
	err = device_power_request_lock(POWER_LOCK_CPU, 0);
	err = device_power_request_lock(POWER_LOCK_DISPLAY, 0);

	// stopwatch
	time_ref = getCurrentTimeStamp();

	// Initialization
	setTimeForFileName();
	hrmCounter = 0;
	ppgCounter = 0;
	accCounter= 0;
	gravityCounter = 0;
	gyroscpoeCounter = 0;
	pressureCounter = 0;
	stressCounter = 0;
	writeToFileFlag = true; // for accidently closing app
	recreate_UI = false;

    for (int i=0; i < SENSORNUM; i++) {
		// Retrieving a Sensor
		//sensor_type_e type = SENSOR_HRM;
    		sensor_type_e type = sensorTypeList[i];
		//sensor_h sensor;

		bool supported;
		int error = sensor_is_supported(type, &supported);
		if (error != SENSOR_ERROR_NONE) {
			dlog_print(DLOG_ERROR, LOG_TAG, "sensor_is_supported error: %d", error);
			return;
		}

//		if(supported){
//			dlog_print(DLOG_DEBUG, LOG_TAG, "HRM is%s supported", supported ? "" : " not");
//			sprintf(out,"HRM is%s supported", supported ? "" : " not");
//			elm_object_text_set(event_label, out);
//		}

		// Get sensor list
		int count;
		sensor_h *list;

		error = sensor_get_sensor_list(type, &list, &count);
		if (error != SENSOR_ERROR_NONE) {
			dlog_print(DLOG_ERROR, LOG_TAG, "sensor_get_sensor_list error: %d", error);
		} else {
			dlog_print(DLOG_DEBUG, LOG_TAG, "Number of sensors: %d", count);
			free(list);
		}

		error = sensor_get_default_sensor(type, &sensorList[i]);
		if (error != SENSOR_ERROR_NONE) {
			dlog_print(DLOG_ERROR, LOG_TAG, "sensor_get_default_sensor error: %d", error);
			return;
		}

		dlog_print(DLOG_DEBUG, LOG_TAG, "sensor_get_default_sensor");

		// Registering a Sensor Event
		//error = sensor_create_listener(sensor, &listener);
		error = sensor_create_listener(sensorList[i], &listenerList[i]);

		if (error != SENSOR_ERROR_NONE) {
			dlog_print(DLOG_ERROR, LOG_TAG, "sensor_create_listener error: %d", error);
			return;
		}

		dlog_print(DLOG_DEBUG, LOG_TAG, "sensor_create_listener");

		int min_interval = 0;
		error = sensor_get_min_interval(sensorList[i], &min_interval);
		if (error != SENSOR_ERROR_NONE) {
			dlog_print(DLOG_ERROR, LOG_TAG, "sensor_get_min_interval error: %d", error);
			return;
		}

		dlog_print(DLOG_DEBUG, LOG_TAG, "Minimum interval of the sensor: %d", min_interval);


		// Callback for sensor value change
		error = sensor_listener_set_event_cb(listenerList[i], 50, on_sensor_event, user_data);
		//error = sensor_listener_set_event_cb(&listenerList[i], min_interval, on_sensor_event, user_data);
		if (error != SENSOR_ERROR_NONE) {
			dlog_print(DLOG_ERROR, LOG_TAG, "sensor_listener_set_event_cb error: %d", error);
			return;
		}

		dlog_print(DLOG_DEBUG, LOG_TAG, "sensor_listener_set_event_cb");

		error = sensor_listener_set_option(listenerList[i], SENSOR_OPTION_ALWAYS_ON);
		if (error != SENSOR_ERROR_NONE) {
			dlog_print(DLOG_ERROR, LOG_TAG, "sensor_listener_set_option error: %d", error);
			return;
		}

		dlog_print(DLOG_DEBUG, LOG_TAG, "sensor_listener_set_option");

		if (type == SENSOR_HRM) {
			// Registering the Accuracy Changed Callback
//			error = sensor_listener_set_accuracy_cb(&listenerList[i], _sensor_accuracy_changed_cb, user_data);
//			if (error != SENSOR_ERROR_NONE) {
//				dlog_print(DLOG_ERROR, LOG_TAG, "sensor_listener_set_accuracy_cb error: %d", error);
//				return;
//			}
//
//			dlog_print(DLOG_DEBUG, LOG_TAG, "sensor_listener_set_accuracy_cb");
//
//			error = sensor_listener_set_interval(&listenerList[i], 100);
//			if (error != SENSOR_ERROR_NONE) {
//				dlog_print(DLOG_ERROR, LOG_TAG, "sensor_listener_set_interval error: %d", error);
//				return;
//			}
//
//			dlog_print(DLOG_DEBUG, LOG_TAG, "sensor_listener_set_intervals");

//			error = sensor_listener_set_option(listenerList[i], SENSOR_OPTION_ALWAYS_ON);
//			if (error != SENSOR_ERROR_NONE) {
//				dlog_print(DLOG_ERROR, LOG_TAG, "sensor_listener_set_option error: %d", error);
//				return;
//			}
//
//			dlog_print(DLOG_DEBUG, LOG_TAG, "sensor_listener_set_option");
		}

		error = sensor_listener_start(listenerList[i]);
		if (error != SENSOR_ERROR_NONE) {
			dlog_print(DLOG_ERROR, LOG_TAG, "sensor_listener_start error: %d", error);
			return;
		}

		dlog_print(DLOG_DEBUG, LOG_TAG, "sensor_listener_start");

		sensor_event_s event;
		error = sensor_listener_read_data(listenerList[i], &event);
		if (error != SENSOR_ERROR_NONE) {

			dlog_print(DLOG_ERROR, LOG_TAG, "sensor_listener_read_data error: %d", error);
			return;
		}

		switch (type) {

		case SENSOR_HRM:
			dlog_print(DLOG_INFO, LOG_TAG, "%f" , event.values[0]);
			sprintf(out,"%f", event.values[0]);
			elm_object_text_set(event_label, out);
			break;
		default:
			dlog_print(DLOG_ERROR, LOG_TAG, "Not an HRM event");
		}

		dlog_print(DLOG_DEBUG, LOG_TAG, out);

		char *name = NULL;
		char *vendor = NULL;
		float min_range = 0.0;
		float max_range = 220.0;
		float resolution = 0.0;

		error = sensor_get_name(sensorList[i], &name);
		if (error != SENSOR_ERROR_NONE) {
			dlog_print(DLOG_ERROR, LOG_TAG, "sensor_get_name error: %d", error);
			return;
		}

		dlog_print(DLOG_DEBUG, LOG_TAG, "Sensor name: %s", name);
		free(name);

		error = sensor_get_vendor(sensorList[i], &vendor);
		if (error != SENSOR_ERROR_NONE) {
			dlog_print(DLOG_ERROR, LOG_TAG, "sensor_get_vendor error: %d", error);
			return;
		}


		dlog_print(DLOG_DEBUG, LOG_TAG, "Sensor vendor: %s", vendor);
		free(vendor);

		error = sensor_get_type(sensorList[i], &type);
		if (error != SENSOR_ERROR_NONE) {
			dlog_print(DLOG_ERROR, LOG_TAG, "sensor_get_type error: %d", error);
			return;
		}

		dlog_print(DLOG_DEBUG, LOG_TAG, "Sensor type: %s",
				type == SENSOR_ACCELEROMETER               ? "Accelerometer"
			  : type == SENSOR_GRAVITY                     ? "Gravity sensor"
			  : type == SENSOR_LINEAR_ACCELERATION         ? "Linear acceleration sensor"
			  : type == SENSOR_MAGNETIC                    ? "Magnetic sensor"
			  : type == SENSOR_ROTATION_VECTOR             ? "Rotation Vector sensor"
			  : type == SENSOR_ORIENTATION                 ? "Orientation sensor"
			  : type == SENSOR_GYROSCOPE                   ? "Gyroscope sensor"
			  : type == SENSOR_LIGHT                       ? "Light sensor"
			  : type == SENSOR_PROXIMITY                   ? "Proximity sensor"
			  : type == SENSOR_PRESSURE                    ? "Pressure sensor"
			  : type == SENSOR_ULTRAVIOLET                 ? "Ultraviolet sensor"
			  : type == SENSOR_TEMPERATURE                 ? "Temperature sensor"
			  : type == SENSOR_HUMIDITY                    ? "Humidity sensor"
			  : type == SENSOR_HRM                         ? "Heart Rate Monitor sensor (Since Tizen 2.3.1)"
			  : type == SENSOR_HRM_LED_GREEN               ? "HRM (LED Green) sensor (Since Tizen 2.3.1)"
			  : type == SENSOR_HRM_LED_IR                  ? "HRM (LED IR) sensor (Since Tizen 2.3.1)"
			  : type == SENSOR_HRM_LED_RED                 ? "HRM (LED RED) sensor (Since Tizen 2.3.1)"
			  : type == SENSOR_LAST                        ? "End of sensor enum values" : "Custom sensor");

		error = sensor_get_min_range(sensorList[i], &min_range);
		if (error != SENSOR_ERROR_NONE) {
			dlog_print(DLOG_ERROR, LOG_TAG, "sensor_get_min_range error: %d", error);
			return;
		}

		dlog_print(DLOG_DEBUG, LOG_TAG, "Minimum range of the sensor: %f", min_range);

		error = sensor_get_max_range(sensorList[i], &max_range);
		if (error != SENSOR_ERROR_NONE) {
			dlog_print(DLOG_ERROR, LOG_TAG, "sensor_get_max_range error: %d", error);
			return;
		}

		dlog_print(DLOG_DEBUG, LOG_TAG, "Maximum range of the sensor: %f", max_range);

		error = sensor_get_resolution(sensorList[i], &resolution);
		if (error != SENSOR_ERROR_NONE) {
			dlog_print(DLOG_ERROR, LOG_TAG, "sensor_get_resolution error: %d", error);
			return;
		}

		dlog_print(DLOG_DEBUG, LOG_TAG, "Resolution of the sensor: %f", resolution);
    }

    elm_object_disabled_set(start, EINA_TRUE);
    elm_object_disabled_set(stop, EINA_FALSE);
}

void _sensor_stop_cb(void *data, Evas_Object *obj, void *event_info)
{
	for (int i = 0; i < SENSORNUM; i++){
		int error = sensor_listener_unset_event_cb(listenerList[i]);
		if (error != SENSOR_ERROR_NONE) {
			dlog_print(DLOG_ERROR, LOG_TAG, "sensor_listener_unset_event_cb error: %d", error);
		}

		error = sensor_listener_stop(listenerList[i]);
		if (error != SENSOR_ERROR_NONE) {
			dlog_print(DLOG_ERROR, LOG_TAG, "sensor_listener_stop error: %d", error);
		}

		error = sensor_destroy_listener(listenerList[i]);
		if (error != SENSOR_ERROR_NONE) {
			dlog_print(DLOG_ERROR, LOG_TAG, "sensor_destroy_listener error: %d", error);
		}
	}

    elm_object_disabled_set(start, EINA_FALSE);
    elm_object_disabled_set(stop, EINA_TRUE);

	device_power_release_lock(POWER_LOCK_CPU);
	device_power_release_lock(POWER_LOCK_DISPLAY);

    // Finalize data
	writeToFileFlag = false;
	sensorWriteToFile();

	recreate_UI = true;
}

void _add_entry_text(const char *text)
{
    Evas_Coord c_y;

    elm_entry_entry_append(GLOBAL_DEBUG_BOX, text);
    elm_entry_entry_append(GLOBAL_DEBUG_BOX, "<br>");
    elm_entry_cursor_end_set(GLOBAL_DEBUG_BOX);
    elm_entry_cursor_geometry_get(GLOBAL_DEBUG_BOX, NULL, &c_y, NULL, NULL);
    elm_scroller_region_show(GLOBAL_DEBUG_BOX, 0, c_y, 0, 0);
}

static void win_delete_request_cb(void *data, Evas_Object *obj, void *event_info)
{
    elm_exit();
}

Eina_Bool _pop_cb(void *data, Elm_Object_Item *item)
{
    elm_win_lower(((appdata_s *)data)->win);
    return EINA_FALSE;
}

Evas_Object *_new_button(appdata_s *ad, Evas_Object *display, char *name, void *cb)
{
    // Create a button
    Evas_Object *bt = elm_button_add(display);
    elm_object_text_set(bt, name);
    elm_object_style_set(bt, "bottom");
    evas_object_smart_callback_add(bt, "clicked", (Evas_Smart_Cb) cb, ad);
    evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, 0.0);
    evas_object_size_hint_align_set(bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
    elm_box_pack_end(display, bt);
    evas_object_show(bt);
    return bt;
}


void _create_new_cd_display(appdata_s *ad, char *name, void *cb)
{

    // Create main box
    box = elm_box_add(conform);
    elm_object_content_set(conform, box);
    elm_box_horizontal_set(box, EINA_FALSE);
    evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
    evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_show(box);

    start = _new_button(ad, box, "Start", _sensor_start_cb);

    event_label = elm_label_add(box);
    event_label_animation = elm_label_add(box);
    //evas_object_resize(box,400,100);
    elm_object_text_set(event_label, "Press Start and wait <br>to see your heart rate!");
    elm_box_pack_end(box, event_label);

    //evas_object_size_hint_align_set(event_label_animation, EVAS_HINT_FILL, EVAS_HINT_FILL);
    //evas_object_size_hint_weight_set(event_label_animation, 0.3, 0.3);
    elm_box_pack_end(box, event_label_animation);
    evas_object_show(event_label);

    // stopwatch
    datetime = elm_label_add(box);
    evas_object_size_hint_weight_set(datetime, 1.0, 0.0);
    evas_object_size_hint_align_set(datetime, 0.5, 0.5);
    elm_object_text_set(datetime, "<font_size=50><color=#ffffff>00:00:00</color></font_size>");
    elm_box_pack_end(box, datetime);
    evas_object_show(datetime);

    stop = _new_button(ad, box, "Stop", _sensor_stop_cb);
    evas_object_size_hint_weight_set(start, 0.0, 0.2);
    evas_object_size_hint_weight_set(stop, 0.0, 0.2);
}

static void create_base_gui(appdata_s *ad)
{
    // Setting the window
    ad->win = elm_win_util_standard_add(PACKAGE, PACKAGE);
    elm_win_conformant_set(ad->win, EINA_TRUE);
    elm_win_autodel_set(ad->win, EINA_TRUE);
    elm_win_indicator_mode_set(ad->win, ELM_WIN_INDICATOR_SHOW);
    elm_win_indicator_opacity_set(ad->win, ELM_WIN_INDICATOR_OPAQUE);
    evas_object_smart_callback_add(ad->win, "delete, request", win_delete_request_cb, NULL);

    /* Create conformant */
    conform = elm_conformant_add(ad->win);

    // back button
    eext_object_event_callback_add(conform, EEXT_CALLBACK_BACK, win_back_cb, ad);

    evas_object_size_hint_weight_set(conform, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_win_resize_object_add(ad->win, conform);
    evas_object_show(conform);

    // Create a naviframe
    ad->navi = elm_naviframe_add(conform);
    evas_object_size_hint_align_set(ad->navi, EVAS_HINT_FILL, EVAS_HINT_FILL);
    evas_object_size_hint_weight_set(ad->navi, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

    elm_object_content_set(conform, ad->navi);
    evas_object_show(ad->navi);

    // Fill the list with items
    //create_buttons_in_main_window(ad);
    _create_new_cd_display(ad, "Sensor", _pop_cb);

    eext_object_event_callback_add(ad->navi, EEXT_CALLBACK_BACK, eext_naviframe_back_cb, NULL);

    // Show the window after base gui is set up
    evas_object_show(ad->win);
}

static bool app_create(void *data)
{
    /*
     * Hook to take necessary actions before main event loop starts
     * Initialize UI resources and application's data
     * If this function returns true, the main loop of application starts
     * If this function returns false, the application is terminated
     */
    create_base_gui((appdata_s *)data);

    return true;
}

static void app_resume(void *data)
{
	if(recreate_UI) {
		//evas_object_del(box);
		//create_base_gui((appdata_s *)data);
		elm_object_text_set(event_label, "<font_size=25>Press Start and wait <br>to see your heart rate!</font_size>");
		elm_object_text_set(datetime, "<font_size=50><color=#ffffff>00:00:00</color></font_size>");
		//evas_object_hide(event_label_animation);
	}
	//recreate_UI = false;
//	create_base_gui((appdata_s *)data);
}

static void app_pause(void *data)
{

}


// handler for the back button
void win_back_cb(void *data, Evas_Object *obj, void *event_info) {
	if(writeToFileFlag) { // monitoring is started but not finished
		sensorWriteToFile();
		device_power_release_lock(POWER_LOCK_CPU);
		device_power_release_lock(POWER_LOCK_DISPLAY);
	}
	// appdata_s *ad = data;
	// evas_object_del(ad->win);
	ecore_main_loop_quit();
	ecore_evas_shutdown();
	ecore_shutdown();
	exit(1);
}

void app_terminate(void *data)
{
	if(writeToFileFlag) { // monitoring is started but not finished
		sensorWriteToFile();
		device_power_release_lock(POWER_LOCK_CPU);
		device_power_release_lock(POWER_LOCK_DISPLAY);
	}
	//evas_object_del(conform);
}

int main(int argc, char *argv[])
{
    appdata_s ad;
    memset(&ad, 0x00, sizeof(appdata_s));

//    ui_app_lifecycle_callback_s event_callback;
//    memset(&event_callback, 0x00, sizeof(ui_app_lifecycle_callback_s));

    ui_app_lifecycle_callback_s event_callback = {0, };

    event_callback.create = app_create;
    event_callback.resume = app_resume;
    event_callback.pause = app_pause;
    event_callback.terminate = app_terminate;

    int ret = ui_app_main(argc, argv, &event_callback, &ad);
    if (ret != APP_ERROR_NONE)
        dlog_print(DLOG_ERROR, LOG_TAG, "ui_app_main() failed with error: %d", ret);
    return ret;
}
