#ifndef __hrmexercise_H__
#define __hrmexercise_H__

#include <Elementary.h>
#include <app.h>
#include <dlog.h>
#include <efl_extension.h>
#include <sensor.h>

#define _PRINT_MSG_LOG_BUFFER_SIZE_ 1024
#define PRINT_MSG(fmt, args...) do { char _log_[_PRINT_MSG_LOG_BUFFER_SIZE_]; \
    snprintf(_log_, _PRINT_MSG_LOG_BUFFER_SIZE_, fmt, ##args); _add_entry_text(_log_); } while (0)

typedef struct {
    Evas_Object *win;
    Evas_Object *navi;
} appdata_s;

void _add_entry_text(const char *text);
Evas_Object *_new_button(appdata_s *ad, Evas_Object *display, char *name, void *cb);
void _create_new_cd_display(appdata_s *ad, char *name, void *cb);
Eina_Bool _pop_cb(void *data, Elm_Object_Item *item);
void win_back_cb(void *data, Evas_Object *obj, void *event_info);

#ifndef PACKAGE
#define PACKAGE "org.example.sensor"
#endif

#ifdef  LOG_TAG
#undef  LOG_TAG
#endif
#define LOG_TAG "sensor"

#endif /* __hrmexercise_H__ */
