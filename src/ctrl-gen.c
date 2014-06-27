/* This is an AUTO-GENERATED file! Update it with the output of `ctrl-gen.py def`. */
#include "libuvc/libuvc.h"
#include "libuvc/libuvc_internal.h"

static const int REQ_TYPE_SET = 0x21;
static const int REQ_TYPE_GET = 0xa1;

/** @ingroup ctrl */
uvc_error_t uvc_get_scanning_mode(uvc_device_handle_t *devh, uint8_t* mode, enum uvc_req_code req_code) {
  uint8_t data[1];
  uvc_error_t ret;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_GET, req_code,
    UVC_CT_SCANNING_MODE_CONTROL << 8,
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


/** @ingroup ctrl */
uvc_error_t uvc_set_scanning_mode(uvc_device_handle_t *devh, uint8_t mode) {
  uint8_t data[1];
  uvc_error_t ret;

  data[0] = mode;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_SET, UVC_SET_CUR,
    UVC_CT_SCANNING_MODE_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data))
    return UVC_SUCCESS;
  else
    return ret;
}

/** @ingroup ctrl */
uvc_error_t uvc_get_ae_mode(uvc_device_handle_t *devh, uint8_t* mode, enum uvc_req_code req_code) {
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


/** @ingroup ctrl */
uvc_error_t uvc_set_ae_mode(uvc_device_handle_t *devh, uint8_t mode) {
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

/** @ingroup ctrl */
uvc_error_t uvc_get_ae_priority(uvc_device_handle_t *devh, uint8_t* priority, enum uvc_req_code req_code) {
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


/** @ingroup ctrl */
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

/** @ingroup ctrl */
uvc_error_t uvc_get_exposure_abs(uvc_device_handle_t *devh, uint32_t* time, enum uvc_req_code req_code) {
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
    *time = DW_TO_INT(data + 0);
    return UVC_SUCCESS;
  } else {
    return ret;
  }
}


/** @ingroup ctrl */
uvc_error_t uvc_set_exposure_abs(uvc_device_handle_t *devh, uint32_t time) {
  uint8_t data[4];
  uvc_error_t ret;

  INT_TO_DW(time, data + 0);

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

/** @ingroup ctrl */
uvc_error_t uvc_get_exposure_rel(uvc_device_handle_t *devh, int8_t* step, enum uvc_req_code req_code) {
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


/** @ingroup ctrl */
uvc_error_t uvc_set_exposure_rel(uvc_device_handle_t *devh, int8_t step) {
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

/** @ingroup ctrl */
uvc_error_t uvc_get_focus_abs(uvc_device_handle_t *devh, uint16_t* focus, enum uvc_req_code req_code) {
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
    *focus = SW_TO_SHORT(data + 0);
    return UVC_SUCCESS;
  } else {
    return ret;
  }
}


/** @ingroup ctrl */
uvc_error_t uvc_set_focus_abs(uvc_device_handle_t *devh, uint16_t focus) {
  uint8_t data[2];
  uvc_error_t ret;

  SHORT_TO_SW(focus, data + 0);

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

/** @ingroup ctrl */
uvc_error_t uvc_get_focus_rel(uvc_device_handle_t *devh, int8_t* focus_rel, uint8_t* speed, enum uvc_req_code req_code) {
  uint8_t data[2];
  uvc_error_t ret;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_GET, req_code,
    UVC_CT_FOCUS_RELATIVE_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data)) {
    *focus_rel = data[0];
    *speed = data[1];
    return UVC_SUCCESS;
  } else {
    return ret;
  }
}


/** @ingroup ctrl */
uvc_error_t uvc_set_focus_rel(uvc_device_handle_t *devh, int8_t focus_rel, uint8_t speed) {
  uint8_t data[2];
  uvc_error_t ret;

  data[0] = focus_rel;
  data[1] = speed;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_SET, UVC_SET_CUR,
    UVC_CT_FOCUS_RELATIVE_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data))
    return UVC_SUCCESS;
  else
    return ret;
}

/** @ingroup ctrl */
uvc_error_t uvc_get_focus_simple_range(uvc_device_handle_t *devh, uint8_t* focus, enum uvc_req_code req_code) {
  uint8_t data[1];
  uvc_error_t ret;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_GET, req_code,
    UVC_CT_FOCUS_SIMPLE_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data)) {
    *focus = data[0];
    return UVC_SUCCESS;
  } else {
    return ret;
  }
}


/** @ingroup ctrl */
uvc_error_t uvc_set_focus_simple_range(uvc_device_handle_t *devh, uint8_t focus) {
  uint8_t data[1];
  uvc_error_t ret;

  data[0] = focus;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_SET, UVC_SET_CUR,
    UVC_CT_FOCUS_SIMPLE_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data))
    return UVC_SUCCESS;
  else
    return ret;
}

/** @ingroup ctrl */
uvc_error_t uvc_get_focus_auto(uvc_device_handle_t *devh, uint8_t* state, enum uvc_req_code req_code) {
  uint8_t data[1];
  uvc_error_t ret;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_GET, req_code,
    UVC_CT_FOCUS_AUTO_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data)) {
    *state = data[0];
    return UVC_SUCCESS;
  } else {
    return ret;
  }
}


/** @ingroup ctrl */
uvc_error_t uvc_set_focus_auto(uvc_device_handle_t *devh, uint8_t state) {
  uint8_t data[1];
  uvc_error_t ret;

  data[0] = state;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_SET, UVC_SET_CUR,
    UVC_CT_FOCUS_AUTO_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data))
    return UVC_SUCCESS;
  else
    return ret;
}

/** @ingroup ctrl */
uvc_error_t uvc_get_iris_abs(uvc_device_handle_t *devh, uint16_t* iris, enum uvc_req_code req_code) {
  uint8_t data[2];
  uvc_error_t ret;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_GET, req_code,
    UVC_CT_IRIS_ABSOLUTE_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data)) {
    *iris = SW_TO_SHORT(data + 0);
    return UVC_SUCCESS;
  } else {
    return ret;
  }
}


/** @ingroup ctrl */
uvc_error_t uvc_set_iris_abs(uvc_device_handle_t *devh, uint16_t iris) {
  uint8_t data[2];
  uvc_error_t ret;

  SHORT_TO_SW(iris, data + 0);

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_SET, UVC_SET_CUR,
    UVC_CT_IRIS_ABSOLUTE_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data))
    return UVC_SUCCESS;
  else
    return ret;
}

/** @ingroup ctrl */
uvc_error_t uvc_get_iris_rel(uvc_device_handle_t *devh, uint8_t* iris_rel, enum uvc_req_code req_code) {
  uint8_t data[1];
  uvc_error_t ret;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_GET, req_code,
    UVC_CT_IRIS_RELATIVE_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data)) {
    *iris_rel = data[0];
    return UVC_SUCCESS;
  } else {
    return ret;
  }
}


/** @ingroup ctrl */
uvc_error_t uvc_set_iris_rel(uvc_device_handle_t *devh, uint8_t iris_rel) {
  uint8_t data[1];
  uvc_error_t ret;

  data[0] = iris_rel;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_SET, UVC_SET_CUR,
    UVC_CT_IRIS_RELATIVE_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data))
    return UVC_SUCCESS;
  else
    return ret;
}

/** @ingroup ctrl */
uvc_error_t uvc_get_zoom_abs(uvc_device_handle_t *devh, uint16_t* focal_length, enum uvc_req_code req_code) {
  uint8_t data[2];
  uvc_error_t ret;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_GET, req_code,
    UVC_CT_ZOOM_ABSOLUTE_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data)) {
    *focal_length = SW_TO_SHORT(data + 0);
    return UVC_SUCCESS;
  } else {
    return ret;
  }
}


/** @ingroup ctrl */
uvc_error_t uvc_set_zoom_abs(uvc_device_handle_t *devh, uint16_t focal_length) {
  uint8_t data[2];
  uvc_error_t ret;

  SHORT_TO_SW(focal_length, data + 0);

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_SET, UVC_SET_CUR,
    UVC_CT_ZOOM_ABSOLUTE_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data))
    return UVC_SUCCESS;
  else
    return ret;
}

/** @ingroup ctrl */
uvc_error_t uvc_get_zoom_rel(uvc_device_handle_t *devh, int8_t* zoom_rel, uint8_t* digital_zoom, uint8_t* speed, enum uvc_req_code req_code) {
  uint8_t data[3];
  uvc_error_t ret;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_GET, req_code,
    UVC_CT_ZOOM_RELATIVE_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data)) {
    *zoom_rel = data[0];
    *digital_zoom = data[1];
    *speed = data[2];
    return UVC_SUCCESS;
  } else {
    return ret;
  }
}


/** @ingroup ctrl */
uvc_error_t uvc_set_zoom_rel(uvc_device_handle_t *devh, int8_t zoom_rel, uint8_t digital_zoom, uint8_t speed) {
  uint8_t data[3];
  uvc_error_t ret;

  data[0] = zoom_rel;
  data[1] = digital_zoom;
  data[2] = speed;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_SET, UVC_SET_CUR,
    UVC_CT_ZOOM_RELATIVE_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data))
    return UVC_SUCCESS;
  else
    return ret;
}

/** @ingroup ctrl */
uvc_error_t uvc_get_pantilt_abs(uvc_device_handle_t *devh, int32_t* pan, int32_t* tilt, enum uvc_req_code req_code) {
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
    *pan = DW_TO_INT(data + 0);
    *tilt = DW_TO_INT(data + 4);
    return UVC_SUCCESS;
  } else {
    return ret;
  }
}


/** @ingroup ctrl */
uvc_error_t uvc_set_pantilt_abs(uvc_device_handle_t *devh, int32_t pan, int32_t tilt) {
  uint8_t data[8];
  uvc_error_t ret;

  INT_TO_DW(pan, data + 0);
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

/** @ingroup ctrl */
uvc_error_t uvc_get_pantilt_rel(uvc_device_handle_t *devh, int8_t* pan_rel, uint8_t* pan_speed, int8_t* tilt_rel, uint8_t* tilt_speed, enum uvc_req_code req_code) {
  uint8_t data[4];
  uvc_error_t ret;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_GET, req_code,
    UVC_CT_PANTILT_RELATIVE_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data)) {
    *pan_rel = data[0];
    *pan_speed = data[1];
    *tilt_rel = data[2];
    *tilt_speed = data[3];
    return UVC_SUCCESS;
  } else {
    return ret;
  }
}


/** @ingroup ctrl */
uvc_error_t uvc_set_pantilt_rel(uvc_device_handle_t *devh, int8_t pan_rel, uint8_t pan_speed, int8_t tilt_rel, uint8_t tilt_speed) {
  uint8_t data[4];
  uvc_error_t ret;

  data[0] = pan_rel;
  data[1] = pan_speed;
  data[2] = tilt_rel;
  data[3] = tilt_speed;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_SET, UVC_SET_CUR,
    UVC_CT_PANTILT_RELATIVE_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data))
    return UVC_SUCCESS;
  else
    return ret;
}

/** @ingroup ctrl */
uvc_error_t uvc_get_roll_abs(uvc_device_handle_t *devh, int16_t* roll, enum uvc_req_code req_code) {
  uint8_t data[2];
  uvc_error_t ret;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_GET, req_code,
    UVC_CT_ROLL_ABSOLUTE_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data)) {
    *roll = SW_TO_SHORT(data + 0);
    return UVC_SUCCESS;
  } else {
    return ret;
  }
}


/** @ingroup ctrl */
uvc_error_t uvc_set_roll_abs(uvc_device_handle_t *devh, int16_t roll) {
  uint8_t data[2];
  uvc_error_t ret;

  SHORT_TO_SW(roll, data + 0);

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_SET, UVC_SET_CUR,
    UVC_CT_ROLL_ABSOLUTE_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data))
    return UVC_SUCCESS;
  else
    return ret;
}

/** @ingroup ctrl */
uvc_error_t uvc_get_roll_rel(uvc_device_handle_t *devh, int8_t* roll_rel, uint8_t* speed, enum uvc_req_code req_code) {
  uint8_t data[2];
  uvc_error_t ret;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_GET, req_code,
    UVC_CT_ROLL_RELATIVE_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data)) {
    *roll_rel = data[0];
    *speed = data[1];
    return UVC_SUCCESS;
  } else {
    return ret;
  }
}


/** @ingroup ctrl */
uvc_error_t uvc_set_roll_rel(uvc_device_handle_t *devh, int8_t roll_rel, uint8_t speed) {
  uint8_t data[2];
  uvc_error_t ret;

  data[0] = roll_rel;
  data[1] = speed;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_SET, UVC_SET_CUR,
    UVC_CT_ROLL_RELATIVE_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data))
    return UVC_SUCCESS;
  else
    return ret;
}

/** @ingroup ctrl */
uvc_error_t uvc_get_privacy(uvc_device_handle_t *devh, uint8_t* privacy, enum uvc_req_code req_code) {
  uint8_t data[1];
  uvc_error_t ret;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_GET, req_code,
    UVC_CT_PRIVACY_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data)) {
    *privacy = data[0];
    return UVC_SUCCESS;
  } else {
    return ret;
  }
}


/** @ingroup ctrl */
uvc_error_t uvc_set_privacy(uvc_device_handle_t *devh, uint8_t privacy) {
  uint8_t data[1];
  uvc_error_t ret;

  data[0] = privacy;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_SET, UVC_SET_CUR,
    UVC_CT_PRIVACY_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data))
    return UVC_SUCCESS;
  else
    return ret;
}

/** @ingroup ctrl */
uvc_error_t uvc_get_digital_window(uvc_device_handle_t *devh, uint16_t* window_top, uint16_t* window_left, uint16_t* window_bottom, uint16_t* window_right, uint16_t* num_steps, uint16_t* num_steps_units, enum uvc_req_code req_code) {
  uint8_t data[12];
  uvc_error_t ret;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_GET, req_code,
    UVC_CT_DIGITAL_WINDOW_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data)) {
    *window_top = SW_TO_SHORT(data + 0);
    *window_left = SW_TO_SHORT(data + 2);
    *window_bottom = SW_TO_SHORT(data + 4);
    *window_right = SW_TO_SHORT(data + 6);
    *num_steps = SW_TO_SHORT(data + 8);
    *num_steps_units = SW_TO_SHORT(data + 10);
    return UVC_SUCCESS;
  } else {
    return ret;
  }
}


/** @ingroup ctrl */
uvc_error_t uvc_set_digital_window(uvc_device_handle_t *devh, uint16_t window_top, uint16_t window_left, uint16_t window_bottom, uint16_t window_right, uint16_t num_steps, uint16_t num_steps_units) {
  uint8_t data[12];
  uvc_error_t ret;

  SHORT_TO_SW(window_top, data + 0);
  SHORT_TO_SW(window_left, data + 2);
  SHORT_TO_SW(window_bottom, data + 4);
  SHORT_TO_SW(window_right, data + 6);
  SHORT_TO_SW(num_steps, data + 8);
  SHORT_TO_SW(num_steps_units, data + 10);

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_SET, UVC_SET_CUR,
    UVC_CT_DIGITAL_WINDOW_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data))
    return UVC_SUCCESS;
  else
    return ret;
}

/** @ingroup ctrl */
uvc_error_t uvc_get_digital_roi(uvc_device_handle_t *devh, uint16_t* roi_top, uint16_t* roi_left, uint16_t* roi_bottom, uint16_t* roi_right, uint16_t* auto_controls, enum uvc_req_code req_code) {
  uint8_t data[10];
  uvc_error_t ret;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_GET, req_code,
    UVC_CT_REGION_OF_INTEREST_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data)) {
    *roi_top = SW_TO_SHORT(data + 0);
    *roi_left = SW_TO_SHORT(data + 2);
    *roi_bottom = SW_TO_SHORT(data + 4);
    *roi_right = SW_TO_SHORT(data + 6);
    *auto_controls = SW_TO_SHORT(data + 8);
    return UVC_SUCCESS;
  } else {
    return ret;
  }
}


/** @ingroup ctrl */
uvc_error_t uvc_set_digital_roi(uvc_device_handle_t *devh, uint16_t roi_top, uint16_t roi_left, uint16_t roi_bottom, uint16_t roi_right, uint16_t auto_controls) {
  uint8_t data[10];
  uvc_error_t ret;

  SHORT_TO_SW(roi_top, data + 0);
  SHORT_TO_SW(roi_left, data + 2);
  SHORT_TO_SW(roi_bottom, data + 4);
  SHORT_TO_SW(roi_right, data + 6);
  SHORT_TO_SW(auto_controls, data + 8);

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_SET, UVC_SET_CUR,
    UVC_CT_REGION_OF_INTEREST_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data))
    return UVC_SUCCESS;
  else
    return ret;
}

/** @ingroup ctrl */
uvc_error_t uvc_get_backlight_compensation(uvc_device_handle_t *devh, uint16_t* backlight_compensation, enum uvc_req_code req_code) {
  uint8_t data[2];
  uvc_error_t ret;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_GET, req_code,
    UVC_PU_BACKLIGHT_COMPENSATION_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data)) {
    *backlight_compensation = SW_TO_SHORT(data + 0);
    return UVC_SUCCESS;
  } else {
    return ret;
  }
}


/** @ingroup ctrl */
uvc_error_t uvc_set_backlight_compensation(uvc_device_handle_t *devh, uint16_t backlight_compensation) {
  uint8_t data[2];
  uvc_error_t ret;

  SHORT_TO_SW(backlight_compensation, data + 0);

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_SET, UVC_SET_CUR,
    UVC_PU_BACKLIGHT_COMPENSATION_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data))
    return UVC_SUCCESS;
  else
    return ret;
}

/** @ingroup ctrl */
uvc_error_t uvc_get_brightness(uvc_device_handle_t *devh, int16_t* brightness, enum uvc_req_code req_code) {
  uint8_t data[2];
  uvc_error_t ret;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_GET, req_code,
    UVC_PU_BRIGHTNESS_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data)) {
    *brightness = SW_TO_SHORT(data + 0);
    return UVC_SUCCESS;
  } else {
    return ret;
  }
}


/** @ingroup ctrl */
uvc_error_t uvc_set_brightness(uvc_device_handle_t *devh, int16_t brightness) {
  uint8_t data[2];
  uvc_error_t ret;

  SHORT_TO_SW(brightness, data + 0);

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_SET, UVC_SET_CUR,
    UVC_PU_BRIGHTNESS_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data))
    return UVC_SUCCESS;
  else
    return ret;
}

/** @ingroup ctrl */
uvc_error_t uvc_get_contrast(uvc_device_handle_t *devh, uint16_t* contrast, enum uvc_req_code req_code) {
  uint8_t data[2];
  uvc_error_t ret;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_GET, req_code,
    UVC_PU_CONTRAST_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data)) {
    *contrast = SW_TO_SHORT(data + 0);
    return UVC_SUCCESS;
  } else {
    return ret;
  }
}


/** @ingroup ctrl */
uvc_error_t uvc_set_contrast(uvc_device_handle_t *devh, uint16_t contrast) {
  uint8_t data[2];
  uvc_error_t ret;

  SHORT_TO_SW(contrast, data + 0);

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_SET, UVC_SET_CUR,
    UVC_PU_CONTRAST_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data))
    return UVC_SUCCESS;
  else
    return ret;
}

/** @ingroup ctrl */
uvc_error_t uvc_get_contrast_auto(uvc_device_handle_t *devh, uint8_t* contrast_auto, enum uvc_req_code req_code) {
  uint8_t data[1];
  uvc_error_t ret;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_GET, req_code,
    UVC_PU_CONTRAST_AUTO_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data)) {
    *contrast_auto = data[0];
    return UVC_SUCCESS;
  } else {
    return ret;
  }
}


/** @ingroup ctrl */
uvc_error_t uvc_set_contrast_auto(uvc_device_handle_t *devh, uint8_t contrast_auto) {
  uint8_t data[1];
  uvc_error_t ret;

  data[0] = contrast_auto;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_SET, UVC_SET_CUR,
    UVC_PU_CONTRAST_AUTO_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data))
    return UVC_SUCCESS;
  else
    return ret;
}

/** @ingroup ctrl */
uvc_error_t uvc_get_gain(uvc_device_handle_t *devh, uint16_t* gain, enum uvc_req_code req_code) {
  uint8_t data[2];
  uvc_error_t ret;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_GET, req_code,
    UVC_PU_GAIN_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data)) {
    *gain = SW_TO_SHORT(data + 0);
    return UVC_SUCCESS;
  } else {
    return ret;
  }
}


/** @ingroup ctrl */
uvc_error_t uvc_set_gain(uvc_device_handle_t *devh, uint16_t gain) {
  uint8_t data[2];
  uvc_error_t ret;

  SHORT_TO_SW(gain, data + 0);

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_SET, UVC_SET_CUR,
    UVC_PU_GAIN_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data))
    return UVC_SUCCESS;
  else
    return ret;
}

/** @ingroup ctrl */
uvc_error_t uvc_get_power_line_frequency(uvc_device_handle_t *devh, uint8_t* power_line_frequency, enum uvc_req_code req_code) {
  uint8_t data[1];
  uvc_error_t ret;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_GET, req_code,
    UVC_PU_POWER_LINE_FREQUENCY_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data)) {
    *power_line_frequency = data[0];
    return UVC_SUCCESS;
  } else {
    return ret;
  }
}


/** @ingroup ctrl */
uvc_error_t uvc_set_power_line_frequency(uvc_device_handle_t *devh, uint8_t power_line_frequency) {
  uint8_t data[1];
  uvc_error_t ret;

  data[0] = power_line_frequency;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_SET, UVC_SET_CUR,
    UVC_PU_POWER_LINE_FREQUENCY_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data))
    return UVC_SUCCESS;
  else
    return ret;
}

/** @ingroup ctrl */
uvc_error_t uvc_get_hue(uvc_device_handle_t *devh, int16_t* hue, enum uvc_req_code req_code) {
  uint8_t data[2];
  uvc_error_t ret;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_GET, req_code,
    UVC_PU_HUE_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data)) {
    *hue = SW_TO_SHORT(data + 0);
    return UVC_SUCCESS;
  } else {
    return ret;
  }
}


/** @ingroup ctrl */
uvc_error_t uvc_set_hue(uvc_device_handle_t *devh, int16_t hue) {
  uint8_t data[2];
  uvc_error_t ret;

  SHORT_TO_SW(hue, data + 0);

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_SET, UVC_SET_CUR,
    UVC_PU_HUE_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data))
    return UVC_SUCCESS;
  else
    return ret;
}

/** @ingroup ctrl */
uvc_error_t uvc_get_hue_auto(uvc_device_handle_t *devh, uint8_t* hue_auto, enum uvc_req_code req_code) {
  uint8_t data[1];
  uvc_error_t ret;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_GET, req_code,
    UVC_PU_HUE_AUTO_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data)) {
    *hue_auto = data[0];
    return UVC_SUCCESS;
  } else {
    return ret;
  }
}


/** @ingroup ctrl */
uvc_error_t uvc_set_hue_auto(uvc_device_handle_t *devh, uint8_t hue_auto) {
  uint8_t data[1];
  uvc_error_t ret;

  data[0] = hue_auto;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_SET, UVC_SET_CUR,
    UVC_PU_HUE_AUTO_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data))
    return UVC_SUCCESS;
  else
    return ret;
}

/** @ingroup ctrl */
uvc_error_t uvc_get_saturation(uvc_device_handle_t *devh, uint16_t* saturation, enum uvc_req_code req_code) {
  uint8_t data[2];
  uvc_error_t ret;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_GET, req_code,
    UVC_PU_SATURATION_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data)) {
    *saturation = SW_TO_SHORT(data + 0);
    return UVC_SUCCESS;
  } else {
    return ret;
  }
}


/** @ingroup ctrl */
uvc_error_t uvc_set_saturation(uvc_device_handle_t *devh, uint16_t saturation) {
  uint8_t data[2];
  uvc_error_t ret;

  SHORT_TO_SW(saturation, data + 0);

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_SET, UVC_SET_CUR,
    UVC_PU_SATURATION_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data))
    return UVC_SUCCESS;
  else
    return ret;
}

/** @ingroup ctrl */
uvc_error_t uvc_get_sharpness(uvc_device_handle_t *devh, uint16_t* sharpness, enum uvc_req_code req_code) {
  uint8_t data[2];
  uvc_error_t ret;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_GET, req_code,
    UVC_PU_SHARPNESS_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data)) {
    *sharpness = SW_TO_SHORT(data + 0);
    return UVC_SUCCESS;
  } else {
    return ret;
  }
}


/** @ingroup ctrl */
uvc_error_t uvc_set_sharpness(uvc_device_handle_t *devh, uint16_t sharpness) {
  uint8_t data[2];
  uvc_error_t ret;

  SHORT_TO_SW(sharpness, data + 0);

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_SET, UVC_SET_CUR,
    UVC_PU_SHARPNESS_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data))
    return UVC_SUCCESS;
  else
    return ret;
}

/** @ingroup ctrl */
uvc_error_t uvc_get_gamma(uvc_device_handle_t *devh, uint16_t* gamma, enum uvc_req_code req_code) {
  uint8_t data[2];
  uvc_error_t ret;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_GET, req_code,
    UVC_PU_GAMMA_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data)) {
    *gamma = SW_TO_SHORT(data + 0);
    return UVC_SUCCESS;
  } else {
    return ret;
  }
}


/** @ingroup ctrl */
uvc_error_t uvc_set_gamma(uvc_device_handle_t *devh, uint16_t gamma) {
  uint8_t data[2];
  uvc_error_t ret;

  SHORT_TO_SW(gamma, data + 0);

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_SET, UVC_SET_CUR,
    UVC_PU_GAMMA_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data))
    return UVC_SUCCESS;
  else
    return ret;
}

/** @ingroup ctrl */
uvc_error_t uvc_get_white_balance_temperature(uvc_device_handle_t *devh, uint16_t* temperature, enum uvc_req_code req_code) {
  uint8_t data[2];
  uvc_error_t ret;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_GET, req_code,
    UVC_PU_WHITE_BALANCE_TEMPERATURE_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data)) {
    *temperature = SW_TO_SHORT(data + 0);
    return UVC_SUCCESS;
  } else {
    return ret;
  }
}


/** @ingroup ctrl */
uvc_error_t uvc_set_white_balance_temperature(uvc_device_handle_t *devh, uint16_t temperature) {
  uint8_t data[2];
  uvc_error_t ret;

  SHORT_TO_SW(temperature, data + 0);

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_SET, UVC_SET_CUR,
    UVC_PU_WHITE_BALANCE_TEMPERATURE_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data))
    return UVC_SUCCESS;
  else
    return ret;
}

/** @ingroup ctrl */
uvc_error_t uvc_get_white_balance_temperature_auto(uvc_device_handle_t *devh, uint8_t* temperature_auto, enum uvc_req_code req_code) {
  uint8_t data[1];
  uvc_error_t ret;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_GET, req_code,
    UVC_PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data)) {
    *temperature_auto = data[0];
    return UVC_SUCCESS;
  } else {
    return ret;
  }
}


/** @ingroup ctrl */
uvc_error_t uvc_set_white_balance_temperature_auto(uvc_device_handle_t *devh, uint8_t temperature_auto) {
  uint8_t data[1];
  uvc_error_t ret;

  data[0] = temperature_auto;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_SET, UVC_SET_CUR,
    UVC_PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data))
    return UVC_SUCCESS;
  else
    return ret;
}

/** @ingroup ctrl */
uvc_error_t uvc_get_white_balance_component(uvc_device_handle_t *devh, uint16_t* blue, uint16_t* red, enum uvc_req_code req_code) {
  uint8_t data[4];
  uvc_error_t ret;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_GET, req_code,
    UVC_PU_WHITE_BALANCE_COMPONENT_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data)) {
    *blue = SW_TO_SHORT(data + 0);
    *red = SW_TO_SHORT(data + 2);
    return UVC_SUCCESS;
  } else {
    return ret;
  }
}


/** @ingroup ctrl */
uvc_error_t uvc_set_white_balance_component(uvc_device_handle_t *devh, uint16_t blue, uint16_t red) {
  uint8_t data[4];
  uvc_error_t ret;

  SHORT_TO_SW(blue, data + 0);
  SHORT_TO_SW(red, data + 2);

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_SET, UVC_SET_CUR,
    UVC_PU_WHITE_BALANCE_COMPONENT_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data))
    return UVC_SUCCESS;
  else
    return ret;
}

/** @ingroup ctrl */
uvc_error_t uvc_get_white_balance_component_auto(uvc_device_handle_t *devh, uint8_t* white_balance_component_auto, enum uvc_req_code req_code) {
  uint8_t data[1];
  uvc_error_t ret;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_GET, req_code,
    UVC_PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data)) {
    *white_balance_component_auto = data[0];
    return UVC_SUCCESS;
  } else {
    return ret;
  }
}


/** @ingroup ctrl */
uvc_error_t uvc_set_white_balance_component_auto(uvc_device_handle_t *devh, uint8_t white_balance_component_auto) {
  uint8_t data[1];
  uvc_error_t ret;

  data[0] = white_balance_component_auto;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_SET, UVC_SET_CUR,
    UVC_PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data))
    return UVC_SUCCESS;
  else
    return ret;
}

/** @ingroup ctrl */
uvc_error_t uvc_get_digital_multiplier(uvc_device_handle_t *devh, uint16_t* multiplier_step, enum uvc_req_code req_code) {
  uint8_t data[2];
  uvc_error_t ret;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_GET, req_code,
    UVC_PU_DIGITAL_MULTIPLIER_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data)) {
    *multiplier_step = SW_TO_SHORT(data + 0);
    return UVC_SUCCESS;
  } else {
    return ret;
  }
}


/** @ingroup ctrl */
uvc_error_t uvc_set_digital_multiplier(uvc_device_handle_t *devh, uint16_t multiplier_step) {
  uint8_t data[2];
  uvc_error_t ret;

  SHORT_TO_SW(multiplier_step, data + 0);

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_SET, UVC_SET_CUR,
    UVC_PU_DIGITAL_MULTIPLIER_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data))
    return UVC_SUCCESS;
  else
    return ret;
}

/** @ingroup ctrl */
uvc_error_t uvc_get_digital_multiplier_limit(uvc_device_handle_t *devh, uint16_t* multiplier_step, enum uvc_req_code req_code) {
  uint8_t data[2];
  uvc_error_t ret;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_GET, req_code,
    UVC_PU_DIGITAL_MULTIPLIER_LIMIT_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data)) {
    *multiplier_step = SW_TO_SHORT(data + 0);
    return UVC_SUCCESS;
  } else {
    return ret;
  }
}


/** @ingroup ctrl */
uvc_error_t uvc_set_digital_multiplier_limit(uvc_device_handle_t *devh, uint16_t multiplier_step) {
  uint8_t data[2];
  uvc_error_t ret;

  SHORT_TO_SW(multiplier_step, data + 0);

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_SET, UVC_SET_CUR,
    UVC_PU_DIGITAL_MULTIPLIER_LIMIT_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data))
    return UVC_SUCCESS;
  else
    return ret;
}

/** @ingroup ctrl */
uvc_error_t uvc_get_analog_video_standard(uvc_device_handle_t *devh, uint8_t* video_standard, enum uvc_req_code req_code) {
  uint8_t data[1];
  uvc_error_t ret;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_GET, req_code,
    UVC_PU_ANALOG_VIDEO_STANDARD_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data)) {
    *video_standard = data[0];
    return UVC_SUCCESS;
  } else {
    return ret;
  }
}


/** @ingroup ctrl */
uvc_error_t uvc_set_analog_video_standard(uvc_device_handle_t *devh, uint8_t video_standard) {
  uint8_t data[1];
  uvc_error_t ret;

  data[0] = video_standard;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_SET, UVC_SET_CUR,
    UVC_PU_ANALOG_VIDEO_STANDARD_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data))
    return UVC_SUCCESS;
  else
    return ret;
}

/** @ingroup ctrl */
uvc_error_t uvc_get_analog_video_lock_status(uvc_device_handle_t *devh, uint8_t* status, enum uvc_req_code req_code) {
  uint8_t data[1];
  uvc_error_t ret;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_GET, req_code,
    UVC_PU_ANALOG_LOCK_STATUS_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data)) {
    *status = data[0];
    return UVC_SUCCESS;
  } else {
    return ret;
  }
}


/** @ingroup ctrl */
uvc_error_t uvc_set_analog_video_lock_status(uvc_device_handle_t *devh, uint8_t status) {
  uint8_t data[1];
  uvc_error_t ret;

  data[0] = status;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_SET, UVC_SET_CUR,
    UVC_PU_ANALOG_LOCK_STATUS_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data))
    return UVC_SUCCESS;
  else
    return ret;
}

/** @ingroup ctrl */
uvc_error_t uvc_get_input_select(uvc_device_handle_t *devh, uint8_t* selector, enum uvc_req_code req_code) {
  uint8_t data[1];
  uvc_error_t ret;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_GET, req_code,
    UVC_SU_INPUT_SELECT_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data)) {
    *selector = data[0];
    return UVC_SUCCESS;
  } else {
    return ret;
  }
}


/** @ingroup ctrl */
uvc_error_t uvc_set_input_select(uvc_device_handle_t *devh, uint8_t selector) {
  uint8_t data[1];
  uvc_error_t ret;

  data[0] = selector;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_SET, UVC_SET_CUR,
    UVC_SU_INPUT_SELECT_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data))
    return UVC_SUCCESS;
  else
    return ret;
}

