// Copyright 2015-2021 Espressif Systems (Shanghai) PTE LTD
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
#include "USBVideo.h"

#if CONFIG_TINYUSB_VIDEO_ENABLED

#include "esp32-hal-tinyusb.h"
//#include "esp_camera.h"

/* Time stamp base clock. It is a deprecated parameter. */
#define UVC_CLOCK_FREQUENCY    27000000
/* video capture path */
#define UVC_ENTITY_CAP_INPUT_TERMINAL  0x01
#define UVC_ENTITY_CAP_OUTPUT_TERMINAL 0x02

#define TUD_VIDEO_CAPTURE_DESC_LEN (\
    TUD_VIDEO_DESC_IAD_LEN\
    /* control */\
    + TUD_VIDEO_DESC_STD_VC_LEN\
    + (TUD_VIDEO_DESC_CS_VC_LEN + 1/*bInCollection*/)\
    + TUD_VIDEO_DESC_CAMERA_TERM_LEN\
    + TUD_VIDEO_DESC_OUTPUT_TERM_LEN\
    /* Interface 1, Alternate 0 */\
    + TUD_VIDEO_DESC_STD_VS_LEN\
    + (TUD_VIDEO_DESC_CS_VS_IN_LEN + 1/*bNumFormats x bControlSize*/)\
    + TUD_VIDEO_DESC_CS_VS_FMT_UNCOMPR_LEN\
    + TUD_VIDEO_DESC_CS_VS_FRM_UNCOMPR_CONT_LEN\
    + TUD_VIDEO_DESC_CS_VS_COLOR_MATCHING_LEN\
    /* Interface 1, Alternate 1 */\
    + TUD_VIDEO_DESC_STD_VS_LEN\
    + 7/* Endpoint */\
  )

/* Windows support YUY2 and NV12
 * https://docs.microsoft.com/en-us/windows-hardware/drivers/stream/usb-video-class-driver-overview */

#define TUD_VIDEO_DESC_CS_VS_FMT_YUY2(_fmtidx, _numfmtdesc, _frmidx, _asrx, _asry, _interlace, _cp) \
  TUD_VIDEO_DESC_CS_VS_FMT_UNCOMPR(_fmtidx, _numfmtdesc, TUD_VIDEO_GUID_YUY2, 16, _frmidx, _asrx, _asry, _interlace, _cp)
#define TUD_VIDEO_DESC_CS_VS_FMT_NV12(_fmtidx, _numfmtdesc, _frmidx, _asrx, _asry, _interlace, _cp) \
  TUD_VIDEO_DESC_CS_VS_FMT_UNCOMPR(_fmtidx, _numfmtdesc, TUD_VIDEO_GUID_NV12, 12, _frmidx, _asrx, _asry, _interlace, _cp)
#define TUD_VIDEO_DESC_CS_VS_FMT_M420(_fmtidx, _numfmtdesc, _frmidx, _asrx, _asry, _interlace, _cp) \
  TUD_VIDEO_DESC_CS_VS_FMT_UNCOMPR(_fmtidx, _numfmtdesc, TUD_VIDEO_GUID_M420, 12, _frmidx, _asrx, _asry, _interlace, _cp)
#define TUD_VIDEO_DESC_CS_VS_FMT_I420(_fmtidx, _numfmtdesc, _frmidx, _asrx, _asry, _interlace, _cp) \
  TUD_VIDEO_DESC_CS_VS_FMT_UNCOMPR(_fmtidx, _numfmtdesc, TUD_VIDEO_GUID_I420, 12, _frmidx, _asrx, _asry, _interlace, _cp)

#define TUD_VIDEO_CAPTURE_DESCRIPTOR(_itf, _stridx, _epin, _width, _height, _fps, _epsize) \
  TUD_VIDEO_DESC_IAD(_itf, 2, _stridx), \
  /* Video control 0 */ \
  TUD_VIDEO_DESC_STD_VC(_itf, 0, _stridx), \
    TUD_VIDEO_DESC_CS_VC( /* UVC 1.5*/ 0x0150, \
         /* wTotalLength - bLength */ \
         TUD_VIDEO_DESC_CAMERA_TERM_LEN + TUD_VIDEO_DESC_OUTPUT_TERM_LEN, \
         UVC_CLOCK_FREQUENCY, 1), \
      TUD_VIDEO_DESC_CAMERA_TERM(UVC_ENTITY_CAP_INPUT_TERMINAL, 0, 0,\
                                 /*wObjectiveFocalLengthMin*/0, /*wObjectiveFocalLengthMax*/0,\
                                 /*wObjectiveFocalLength*/0, /*bmControls*/0), \
      TUD_VIDEO_DESC_OUTPUT_TERM(UVC_ENTITY_CAP_OUTPUT_TERMINAL, VIDEO_TT_STREAMING, 0, 1, 0), \
  /* Video stream alt. 0 */ \
  TUD_VIDEO_DESC_STD_VS( (_itf+1), 0, 0, 0), \
    /* Video stream header for without still image capture */ \
    TUD_VIDEO_DESC_CS_VS_INPUT( /*bNumFormats*/1, \
        /*wTotalLength - bLength */\
        TUD_VIDEO_DESC_CS_VS_FMT_UNCOMPR_LEN\
        + TUD_VIDEO_DESC_CS_VS_FRM_UNCOMPR_CONT_LEN\
        + TUD_VIDEO_DESC_CS_VS_COLOR_MATCHING_LEN,\
        _epin, /*bmInfo*/0, /*bTerminalLink*/UVC_ENTITY_CAP_OUTPUT_TERMINAL, \
        /*bStillCaptureMethod*/0, /*bTriggerSupport*/0, /*bTriggerUsage*/0, \
        /*bmaControls(1)*/0), \
      /* Video stream format */ \
      TUD_VIDEO_DESC_CS_VS_FMT_YUY2(/*bFormatIndex*/1, /*bNumFrameDescriptors*/1, \
        /*bDefaultFrameIndex*/1, 0, 0, 0, /*bCopyProtect*/0), \
        /* Video stream frame format */ \
        TUD_VIDEO_DESC_CS_VS_FRM_UNCOMPR_CONT(/*bFrameIndex */1, 0, _width, _height, \
            _width * _height * 16, _width * _height * 16 * _fps, \
            _width * _height * 16, \
            (10000000/_fps), (10000000/_fps), 10000000, 100000), \
        TUD_VIDEO_DESC_CS_VS_COLOR_MATCHING(VIDEO_COLOR_PRIMARIES_BT709, VIDEO_COLOR_XFER_CH_BT709, VIDEO_COLOR_COEF_SMPTE170M), \
  /* VS alt 1 */\
  TUD_VIDEO_DESC_STD_VS((_itf+1), 1, 1, 0), \
    /* EP */ \
    TUD_VIDEO_DESC_EP_BULK(_epin, _epsize, 1)


ESP_EVENT_DEFINE_BASE(ARDUINO_USB_VIDEO_EVENTS);
esp_err_t arduino_usb_event_post(esp_event_base_t event_base, int32_t event_id, void *event_data, size_t event_data_size, TickType_t ticks_to_wait);
esp_err_t arduino_usb_event_handler_register_with(esp_event_base_t event_base, int32_t event_id, esp_event_handler_t event_handler, void *event_handler_arg);

#define FRAME_WIDTH   160
#define FRAME_HEIGHT  120
#define FRAME_RATE    10

uint16_t tusb_video_load_descriptor(uint8_t * dst, uint8_t * itf)
{
    uint8_t str_index = tinyusb_add_string_descriptor("TinyUSB Video");
    uint8_t ep_num = tinyusb_get_free_in_endpoint();
    TU_VERIFY (ep_num != 0);
    uint8_t descriptor[TUD_VIDEO_CAPTURE_DESC_LEN] = {
        // Interface number, string index, EP IN address, width, height, frame rate, EP size
        TUD_VIDEO_CAPTURE_DESCRIPTOR(*itf, str_index, (uint8_t)(0x80 | ep_num), FRAME_WIDTH, FRAME_HEIGHT, FRAME_RATE, 64)
    };
    *itf+=2;
    memcpy(dst, descriptor, TUD_VIDEO_CAPTURE_DESC_LEN);
    return TUD_VIDEO_CAPTURE_DESC_LEN;
}

static unsigned frame_num = 0;
static unsigned tx_busy = 0;
static unsigned interval_ms = 1000 / FRAME_RATE;
static uint8_t frame_buffer[FRAME_WIDTH * FRAME_HEIGHT * 16 / 8];

static void fill_color_bar(uint8_t *buffer, unsigned start_position)
{
  /* EBU color bars
   * See also https://stackoverflow.com/questions/6939422 */
  static uint8_t const bar_color[8][4] = {
    /*  Y,   U,   Y,   V */
    { 235, 128, 235, 128}, /* 100% White */
    { 219,  16, 219, 138}, /* Yellow */
    { 188, 154, 188,  16}, /* Cyan */
    { 173,  42, 173,  26}, /* Green */
    {  78, 214,  78, 230}, /* Magenta */
    {  63, 102,  63, 240}, /* Red */
    {  32, 240,  32, 118}, /* Blue */
    {  16, 128,  16, 128}, /* Black */
  };
  uint8_t *p;

  /* Generate the 1st line */
  uint8_t *end = &buffer[FRAME_WIDTH * 2];
  unsigned idx = (FRAME_WIDTH / 2 - 1) - (start_position % (FRAME_WIDTH / 2));
  p = &buffer[idx * 4];
  for (unsigned i = 0; i < 8; ++i) {
    for (int j = 0; j < FRAME_WIDTH / (2 * 8); ++j) {
      memcpy(p, &bar_color[i], 4);
      p += 4;
      if (end <= p) {
        p = buffer;
      }
    }
  }
  /* Duplicate the 1st line to the others */
  p = &buffer[FRAME_WIDTH * 2];
  for (unsigned i = 1; i < FRAME_HEIGHT; ++i) {
    memcpy(p, buffer, FRAME_WIDTH * 2);
    p += FRAME_WIDTH * 2;
  }
}

void video_task(void)
{
  static unsigned start_ms = 0;
  static unsigned already_sent = 0;

  if (!tud_video_n_streaming(0, 0)) {
    already_sent  = 0;
    frame_num     = 0;
    return;
  }

  if (!already_sent) {
    already_sent = 1;
    start_ms = millis();

    log_d("TX: %u", frame_num);
    fill_color_bar(frame_buffer, frame_num);
    tud_video_n_frame_xfer(0, 0, (void*)frame_buffer, FRAME_WIDTH * FRAME_HEIGHT * 16/8);
    // camera_fb_t *fb = esp_camera_fb_get();
    // if(fb){
    //   log_d("TX: %u", fb->len);
    //   tud_video_n_frame_xfer(0, 0, (void*)fb->buf, fb->len);
    //   esp_camera_fb_return(fb);
    // }
  }

  unsigned cur = millis();
  if (cur - start_ms < interval_ms) return; // not enough time
  if (tx_busy) return;
  start_ms += interval_ms;

  log_d("TX: %u", frame_num);
  fill_color_bar(frame_buffer, frame_num);
  tud_video_n_frame_xfer(0, 0, (void*)frame_buffer, FRAME_WIDTH * FRAME_HEIGHT * 16/8);
  // camera_fb_t *fb = esp_camera_fb_get();
  // if(fb){
  //   log_d("TX: %u", fb->len);
  //   tud_video_n_frame_xfer(0, 0, (void*)fb->buf, fb->len);
  //   esp_camera_fb_return(fb);
  // }
}



/** Invoked when compeletion of a frame transfer
 *
 * @param[in] ctl_idx    Destination control interface index
 * @param[in] stm_idx    Destination streaming interface index */
extern "C" void tud_video_frame_xfer_complete_cb(uint_fast8_t ctl_idx, uint_fast8_t stm_idx){
    log_d("");
    tx_busy = 0;
    ++frame_num;
}

/** Invoked when VS_COMMIT_CONTROL(SET_CUR) request received
 *
 * @param[in] ctl_idx     Destination control interface index
 * @param[in] stm_idx     Destination streaming interface index
 * @param[in] parameters  Video streaming parameters
 * @return video_error_code_t */
extern "C" int tud_video_commit_cb(uint_fast8_t ctl_idx, uint_fast8_t stm_idx, video_probe_and_commit_control_t const *parameters){
    log_d("[%u][%u]:", ctl_idx, stm_idx);
    /* convert unit to ms from 100 ns */
    interval_ms = parameters->dwFrameInterval / 10000;
    return VIDEO_ERROR_NONE;
}

/** Invoked when SET_POWER_MODE request received
 *
 * @param[in] ctl_idx    Destination control interface index
 * @param[in] stm_idx    Destination streaming interface index
 * @return video_error_code_t */
extern "C" int tud_video_power_mode_cb(uint_fast8_t ctl_idx, uint8_t power_mod){
    log_d("[%u]: %u", ctl_idx, power_mod);
    return VIDEO_ERROR_NONE;
}

USBVideo::USBVideo(uint8_t ctl, uint8_t stm):_ctl(ctl), _stm(stm){
    tinyusb_enable_interface(USB_INTERFACE_VIDEO, TUD_VIDEO_CAPTURE_DESC_LEN, tusb_video_load_descriptor);
}

void USBVideo::begin(){

// #define PWDN_GPIO_NUM     1
// #define RESET_GPIO_NUM    2
// #define XCLK_GPIO_NUM     42
// #define SIOD_GPIO_NUM     41
// #define SIOC_GPIO_NUM     18

// #define Y9_GPIO_NUM       16
// #define Y8_GPIO_NUM       39
// #define Y7_GPIO_NUM       40
// #define Y6_GPIO_NUM       15
// #define Y5_GPIO_NUM       13
// #define Y4_GPIO_NUM       5
// #define Y3_GPIO_NUM       12
// #define Y2_GPIO_NUM       14
// #define VSYNC_GPIO_NUM    38
// #define HREF_GPIO_NUM     4
// #define PCLK_GPIO_NUM     3

//   camera_config_t config;
//   config.ledc_channel = LEDC_CHANNEL_0;
//   config.ledc_timer = LEDC_TIMER_0;
//   config.pin_d0 = Y2_GPIO_NUM;
//   config.pin_d1 = Y3_GPIO_NUM;
//   config.pin_d2 = Y4_GPIO_NUM;
//   config.pin_d3 = Y5_GPIO_NUM;
//   config.pin_d4 = Y6_GPIO_NUM;
//   config.pin_d5 = Y7_GPIO_NUM;
//   config.pin_d6 = Y8_GPIO_NUM;
//   config.pin_d7 = Y9_GPIO_NUM;
//   config.pin_xclk = XCLK_GPIO_NUM;
//   config.pin_pclk = PCLK_GPIO_NUM;
//   config.pin_vsync = VSYNC_GPIO_NUM;
//   config.pin_href = HREF_GPIO_NUM;
//   config.pin_sscb_sda = SIOD_GPIO_NUM;
//   config.pin_sscb_scl = SIOC_GPIO_NUM;
//   config.pin_pwdn = PWDN_GPIO_NUM;
//   config.pin_reset = RESET_GPIO_NUM;
//   config.xclk_freq_hz = 8000000;
//   config.pixel_format = PIXFORMAT_YUV422;
//   config.frame_size = FRAMESIZE_QQVGA;
//   config.jpeg_quality = 12;
//   config.fb_count = 2;
//   config.fb_location = CAMERA_FB_IN_PSRAM;
//   config.grab_mode = CAMERA_GRAB_LATEST;

  // camera init
  // esp_err_t err = esp_camera_init(&config);
  // if (err != ESP_OK) {
  //   log_e("Camera init failed with error 0x%x", err);
  //   return;
  // }
}

void USBVideo::end(){

}

void USBVideo::onEvent(esp_event_handler_t callback){
    onEvent(ARDUINO_USB_VIDEO_ANY_EVENT, callback);
}

void USBVideo::onEvent(arduino_usb_video_event_t event, esp_event_handler_t callback){
    arduino_usb_event_handler_register_with(ARDUINO_USB_VIDEO_EVENTS, event, callback, this);
}

bool USBVideo::streaming(){
    return tud_video_n_streaming(_ctl, _stm);
}

bool USBVideo::sendFrame(void *buffer, size_t len){
    return tud_video_n_frame_xfer(_ctl, _stm, buffer, len);
}

#endif /* CONFIG_TINYUSB_VIDEO_ENABLED */
