/**
 * @defgroup ctrl Video capture and processing control
 */

#include "libuvc/libuvc.h"
#include "libuvc/libuvc_internal.h"

static const int REQ_TYPE_SET = 0x21;
static const int REQ_TYPE_GET = 0xa1;

uvc_error_t uvc_get_power_mode(uvc_device_handle_t *devh, enum uvc_device_power_mode *mode, enum uvc_req_code req_code) {
  uint8_t mode_char;
  uvc_error_t ret;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_GET, req_code,
    UVC_VC_VIDEO_POWER_MODE_CONTROL << 8,
    0,
    &mode_char,
    sizeof(mode_char),
    0);

  if (ret == 1) {
    *mode = mode_char;
    return UVC_SUCCESS;
  } else {
    return ret;
  }
}

uvc_error_t uvc_set_power_mode(uvc_device_handle_t *devh, enum uvc_device_power_mode mode) {
  uint8_t mode_char = mode;
  uvc_error_t ret;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_SET, UVC_SET_CUR,
    UVC_VC_VIDEO_POWER_MODE_CONTROL << 8,
    0,
    &mode_char,
    sizeof(mode_char),
    0);

  if (ret == 1)
    return UVC_SUCCESS;
  else
    return ret;
}

/***** CAMERA TERMINAL CONTROLS *****/

uvc_error_t uvc_get_ae_mode(uvc_device_handle_t *devh, int *mode, enum uvc_req_code req_code) {
  uint8_t data[1];
  uvc_error_t ret;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_GET, req_code,
    UVC_CT_AE_MODE_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data)) {
    *mode = data[0];
    return UVC_SUCCESS;
  } else {
    return ret;
  }
}

uvc_error_t uvc_set_ae_mode(uvc_device_handle_t *devh, int mode) {
  uint8_t data[1];
  uvc_error_t ret;

  data[0] = mode;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_SET, UVC_SET_CUR,
    UVC_CT_AE_MODE_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data))
    return UVC_SUCCESS;
  else
    return ret;
}

uvc_error_t uvc_get_ae_priority(uvc_device_handle_t *devh, uint8_t *priority, enum uvc_req_code req_code) {
  uint8_t data[1];
  uvc_error_t ret;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_GET, req_code,
    UVC_CT_AE_PRIORITY_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data)) {
    *priority = data[0];
    return UVC_SUCCESS;
  } else {
    return ret;
  }
}

uvc_error_t uvc_set_ae_priority(uvc_device_handle_t *devh, uint8_t priority) {
  uint8_t data[1];
  uvc_error_t ret;

  data[0] = priority;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_SET, UVC_SET_CUR,
    UVC_CT_AE_PRIORITY_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data))
    return UVC_SUCCESS;
  else
    return ret;
}

uvc_error_t uvc_get_exposure_abs(uvc_device_handle_t *devh, int *time, enum uvc_req_code req_code) {
  uint8_t data[4];
  uvc_error_t ret;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_GET, req_code,
    UVC_CT_EXPOSURE_TIME_ABSOLUTE_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data)) {
    *time = DW_TO_INT(data);
    return UVC_SUCCESS;
  } else {
    return ret;
  }
}

uvc_error_t uvc_set_exposure_abs(uvc_device_handle_t *devh, int time) {
  uint8_t data[4];
  uvc_error_t ret;

  INT_TO_DW(time, data);

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_SET, UVC_SET_CUR,
    UVC_CT_EXPOSURE_TIME_ABSOLUTE_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data))
    return UVC_SUCCESS;
  else
    return ret;
}

uvc_error_t uvc_get_exposure_rel(uvc_device_handle_t *devh, int *step, enum uvc_req_code req_code) {
  uint8_t data[1];
  uvc_error_t ret;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_GET, req_code,
    UVC_CT_EXPOSURE_TIME_RELATIVE_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data)) {
    *step = data[0];
    return UVC_SUCCESS;
  } else {
    return ret;
  }
}

uvc_error_t uvc_set_exposure_rel(uvc_device_handle_t *devh, int step) {
  uint8_t data[1];
  uvc_error_t ret;

  data[0] = step;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_SET, UVC_SET_CUR,
    UVC_CT_EXPOSURE_TIME_RELATIVE_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data))
    return UVC_SUCCESS;
  else
    return ret;
}

uvc_error_t uvc_get_focus_abs(uvc_device_handle_t *devh, short *focus, enum uvc_req_code req_code) {
  uint8_t data[2];
  uvc_error_t ret;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_GET, req_code,
    UVC_CT_FOCUS_ABSOLUTE_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data)) {
    *focus = SW_TO_SHORT(data);
    return UVC_SUCCESS;
  } else {
    return ret;
  }
}

uvc_error_t uvc_set_focus_abs(uvc_device_handle_t *devh, short focus) {
  uint8_t data[2];
  uvc_error_t ret;

  SHORT_TO_SW(focus, data);

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_SET, UVC_SET_CUR,
    UVC_CT_FOCUS_ABSOLUTE_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data))
    return UVC_SUCCESS;
  else
    return ret;
}

/** @todo focus_rel, focus_auto_control */
/** @todo iris_abs_ctrl, iris_rel_ctrl */
/** @todo zoom_abs, zoom_rel */

uvc_error_t uvc_get_pantilt_abs(uvc_device_handle_t *devh, int *pan, int *tilt, enum uvc_req_code req_code) {
  uint8_t data[8];
  uvc_error_t ret;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_GET, req_code,
    UVC_CT_PANTILT_ABSOLUTE_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data)) {
    *pan = DW_TO_INT(data);
    *tilt = DW_TO_INT(data + 4);
    return UVC_SUCCESS;
  } else {
    return ret;
  }
}

uvc_error_t uvc_set_pantilt_abs(uvc_device_handle_t *devh, int pan, int tilt) {
  uint8_t data[8];
  uvc_error_t ret;

  INT_TO_DW(pan, data);
  INT_TO_DW(tilt, data + 4);

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_SET, UVC_SET_CUR,
    UVC_CT_PANTILT_ABSOLUTE_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data))
    return UVC_SUCCESS;
  else
    return ret;
}

/** @todo pantilt_rel */

/** @todo roll_abs, roll_rel */

/** @todo privacy */

/***** SELECTOR UNIT CONTROLS *****/

/** @todo input_select */

/***** PROCESSING UNIT CONTROLS *****/

