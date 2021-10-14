// Copyright 2015-2020 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once
#include "Stream.h"
#include "sdkconfig.h"

#if CONFIG_TINYUSB_VIDEO_ENABLED
#include "esp_event.h"

ESP_EVENT_DECLARE_BASE(ARDUINO_USB_VIDEO_EVENTS);

typedef enum {
    ARDUINO_USB_VIDEO_ANY_EVENT = ESP_EVENT_ANY_ID,
    ARDUINO_USB_VIDEO_DATA_EVENT,
    ARDUINO_USB_VIDEO_MAX_EVENT,
} arduino_usb_video_event_t;

typedef union {
    struct {
        uint16_t len;
    } data;
} arduino_usb_video_event_data_t;

class USBVideo {
private:
    uint8_t _ctl;
    uint8_t _stm;
public:
    USBVideo(uint8_t ctl=0, uint8_t stm=0);
    void begin(void);
    void end(void);
    bool streaming(void);
    bool sendFrame(void *buffer, size_t len);

    void onEvent(esp_event_handler_t callback);
    void onEvent(arduino_usb_video_event_t event, esp_event_handler_t callback);
};

void video_task(void);

#endif /* CONFIG_TINYUSB_VIDEO_ENABLED */
