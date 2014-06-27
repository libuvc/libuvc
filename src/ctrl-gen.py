#!/usr/bin/env python
from __future__ import print_function
import sys

class IntField(object):
    def __init__(self, name, position, length, signed):
        self.name = name
        self.position = position
        self.length = length
        self.signed = signed

        if not self.length in [1, 2, 4]:
            raise Exception("bad length " + str(self.length))

        self.user_type = ('u' if not signed else '') + 'int' + str(length * 8) + '_t'

    def getter_sig(self):
        return "{0}* {1}".format(self.user_type, self.name)

    def unpack(self):
        if self.length == 1:
            return "*{0} = data[{1}];".format(self.name, self.position)
        elif self.length == 2:
            return "*{0} = SW_TO_SHORT(data + {1});".format(self.name, self.position)
        elif self.length == 4:
            return "*{0} = DW_TO_INT(data + {1});".format(self.name, self.position)

    def setter_sig(self):
        return "{0} {1}".format(self.user_type, self.name)

    def pack(self):
        if self.length == 1:
            return "data[{0}] = {1};".format(self.position, self.name)
        elif self.length == 2:
            return "SHORT_TO_SW({0}, data + {1});".format(self.name, self.position)
        elif self.length == 4:
            return "INT_TO_DW({0}, data + {1});".format(self.name, self.position)

CT_CONTROLS = [
    ['scanning_mode', 'SCANNING_MODE', 1,
     [(IntField('mode', 0, 1, False),
       '0: interlaced, 1: progressive')]],
    ['ae_mode', 'AE_MODE', 1,
     [(IntField('mode', 0, 1, False),
       'TODO')]],
    ['ae_priority', 'AE_PRIORITY', 1,
     [(IntField('priority', 0, 1, False),
       'TODO')]],
    ['exposure_abs', 'EXPOSURE_TIME_ABSOLUTE', 4,
     [(IntField('time', 0, 4, False),
       'TODO')]],
    ['exposure_rel', 'EXPOSURE_TIME_RELATIVE', 1,
     [(IntField('step', 0, 1, True),
       'TODO')]],
    ['focus_abs', 'FOCUS_ABSOLUTE', 2,
     [(IntField('focus', 0, 2, False),
       'TODO')]],
    ['focus_rel', 'FOCUS_RELATIVE', 2,
     [(IntField('focus_rel', 0, 1, True),
       'TODO'),
      (IntField('speed', 1, 1, False),
       'TODO')]],
    ['focus_simple_range', 'FOCUS_SIMPLE', 1,
     [(IntField('focus', 0, 1, False),
       'TODO')]],
    ['focus_auto', 'FOCUS_AUTO', 1,
     [(IntField('state', 0, 1, False),
       'TODO')]],
    ['iris_abs', 'IRIS_ABSOLUTE', 2,
     [(IntField('iris', 0, 2, False),
       'TODO')]],
    ['iris_rel', 'IRIS_RELATIVE', 1,
     [(IntField('iris_rel', 0, 1, False),
       'TODO')]],
    ['zoom_abs', 'ZOOM_ABSOLUTE', 2,
     [(IntField('focal_length', 0, 2, False),
       'TODO')]],
    ['zoom_rel', 'ZOOM_RELATIVE', 3,
     [(IntField('zoom_rel', 0, 1, True),
       'TODO'),
      (IntField('digital_zoom', 1, 1, False),
       'TODO'),
      (IntField('speed', 2, 1, False),
       'TODO')]],
    ['pantilt_abs', 'PANTILT_ABSOLUTE', 8,
     [(IntField('pan', 0, 4, True),
       'TODO'),
      (IntField('tilt', 4, 4, True),
       'TODO')]],
    ['pantilt_rel', 'PANTILT_RELATIVE', 4,
     [(IntField('pan_rel', 0, 1, True),
       'TODO'),
      (IntField('pan_speed', 1, 1, False),
       'TODO'),
      (IntField('tilt_rel', 2, 1, True),
       'TODO'),
      (IntField('tilt_speed', 3, 1, False),
       'TODO')]],
    ['roll_abs', 'ROLL_ABSOLUTE', 2,
     [(IntField('roll', 0, 2, True),
       'TODO')]],
    ['roll_rel', 'ROLL_RELATIVE', 2,
     [(IntField('roll_rel', 0, 1, True),
       'TODO'),
      (IntField('speed', 1, 1, False),
       'TODO')]],
    ['privacy', 'PRIVACY', 1,
     [(IntField('privacy', 0, 1, False),
       'TODO')]],
    ['digital_window', 'DIGITAL_WINDOW', 12,
     [(IntField('window_top', 0, 2, False),
       'TODO'),
      (IntField('window_left', 2, 2, False),
       'TODO'),
      (IntField('window_bottom', 4, 2, False),
       'TODO'),
      (IntField('window_right', 6, 2, False),
       'TODO'),
      (IntField('num_steps', 8, 2, False),
       'TODO'),
      (IntField('num_steps_units', 10, 2, False),
       'TODO')]],
    ['digital_roi', 'REGION_OF_INTEREST', 10,
     [(IntField('roi_top', 0, 2, False),
       'TODO'),
      (IntField('roi_left', 2, 2, False),
       'TODO'),
      (IntField('roi_bottom', 4, 2, False),
       'TODO'),
      (IntField('roi_right', 6, 2, False),
       'TODO'),
      (IntField('auto_controls', 8, 2, False),
       'TODO')]],
]

SU_CONTROLS = [
    ['input_select', 'INPUT_SELECT', 1,
     [(IntField('selector', 0, 1, False),
       'TODO')]],
]

PU_CONTROLS = [
    ['backlight_compensation', 'BACKLIGHT_COMPENSATION', 2,
     [(IntField('backlight_compensation', 0, 2, False),
       'device-dependent backlight compensation mode;' +
       'zero means backlight compensation is disabled'),
     ]],
    ['brightness', 'BRIGHTNESS', 2,
     [(IntField('brightness', 0, 2, True),
       'TODO')]],
    ['contrast', 'CONTRAST', 2,
     [(IntField('contrast', 0, 2, False),
       'TODO')]],
    ['contrast_auto', 'CONTRAST_AUTO', 1,
     [(IntField('contrast_auto', 0, 1, False),
       'TODO')]],
    ['gain', 'GAIN', 2,
     [(IntField('gain', 0, 2, False),
       'TODO')]],
    ['power_line_frequency', 'POWER_LINE_FREQUENCY', 1,
     [(IntField('power_line_frequency', 0, 1, False),
       'TODO')]],
    ['hue', 'HUE', 2,
     [(IntField('hue', 0, 2, True),
       'TODO')]],
    ['hue_auto', 'HUE_AUTO', 1,
     [(IntField('hue_auto', 0, 1, False),
       'TODO')]],
    ['saturation', 'SATURATION', 2,
     [(IntField('saturation', 0, 2, False),
       'TODO')]],
    ['sharpness', 'SHARPNESS', 2,
     [(IntField('sharpness', 0, 2, False),
       'TODO')]],
    ['gamma', 'GAMMA', 2,
     [(IntField('gamma', 0, 2, False),
       'TODO')]],
    ['white_balance_temperature', 'WHITE_BALANCE_TEMPERATURE', 2,
     [(IntField('temperature', 0, 2, False),
       'TODO')]],
    ['white_balance_temperature_auto', 'WHITE_BALANCE_TEMPERATURE_AUTO', 1,
     [(IntField('temperature_auto', 0, 1, False),
       'TODO')]],
    ['white_balance_component', 'WHITE_BALANCE_COMPONENT', 4,
     [(IntField('blue', 0, 2, False),
       'TODO'),
      (IntField('red', 2, 2, False),
       'TODO')]],
    ['white_balance_component_auto', 'WHITE_BALANCE_COMPONENT_AUTO', 1,
     [(IntField('white_balance_component_auto', 0, 1, False),
       'TODO')]],
    ['digital_multiplier', 'DIGITAL_MULTIPLIER', 2,
     [(IntField('multiplier_step', 0, 2, False),
       'TODO')]],
    ['digital_multiplier_limit', 'DIGITAL_MULTIPLIER_LIMIT', 2,
     [(IntField('multiplier_step', 0, 2, False),
       'TODO')]],
    ['analog_video_standard', 'ANALOG_VIDEO_STANDARD', 1,
     [(IntField('video_standard', 0, 1, False),
       'TODO')]],
    ['analog_video_lock_status', 'ANALOG_LOCK_STATUS', 1,
     [(IntField('status', 0, 1, False),
       'TODO')]],
]

# @todo Encoding unit controls

GETTER_TEMPLATE = """/** @ingroup ctrl */
uvc_error_t uvc_get_{function_name}(uvc_device_handle_t *devh, {args_signature}, enum uvc_req_code req_code) {{
  uint8_t data[{control_length}];
  uvc_error_t ret;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_GET, req_code,
    UVC_{unit}_{control_name}_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data)) {{
    {unpack}
    return UVC_SUCCESS;
  }} else {{
    return ret;
  }}
}}
"""

SETTER_TEMPLATE = """/** @ingroup ctrl */
uvc_error_t uvc_set_{function_name}(uvc_device_handle_t *devh, {args_signature}) {{
  uint8_t data[{control_length}];
  uvc_error_t ret;

  {pack}

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_SET, UVC_SET_CUR,
    UVC_{unit}_{control_name}_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data))
    return UVC_SUCCESS;
  else
    return ret;
}}
"""

def gen_decl(unit, function_name, control_name, control_length, fields):
    get_args_signature = ', '.join([field.getter_sig() for (field, desc) in fields])
    set_args_signature = ', '.join([field.setter_sig() for (field, desc) in fields])

    return "uvc_error_t uvc_get_{function_name}(uvc_device_handle_t *devh, {args_signature}, enum uvc_req_code req_code);\n".format(**{
        "function_name": function_name,
        "args_signature": get_args_signature
    }) + "uvc_error_t uvc_set_{function_name}(uvc_device_handle_t *devh, {args_signature});\n".format(**{
        "function_name": function_name,
        "args_signature": set_args_signature
    })

def gen_ctrl(unit, function_name, control_name, control_length, fields):
    get_args_signature = ', '.join([field.getter_sig() for (field, desc) in fields])
    set_args_signature = ', '.join([field.setter_sig() for (field, desc) in fields])
    unpack = "\n    ".join([field.unpack() for (field, desc) in fields])
    pack = "\n  ".join([field.pack() for (field, desc) in fields])

    return GETTER_TEMPLATE.format(**{
        "unit": unit,
        "function_name": function_name,
        "control_name": control_name,
        "control_length": control_length,
        "args_signature": get_args_signature,
        "unpack": unpack,
    }) + "\n\n" + SETTER_TEMPLATE.format(**{
        "unit": unit,
        "function_name": function_name,
        "control_name": control_name,
        "control_length": control_length,
        "args_signature": set_args_signature,
        "pack": pack,
    })



if __name__ == '__main__':
    if sys.argv[1] == 'def':
        print("""/* This is an AUTO-GENERATED file! Update it with the output of `ctrl-gen.py def`. */
#include "libuvc/libuvc.h"
#include "libuvc/libuvc_internal.h"

static const int REQ_TYPE_SET = 0x21;
static const int REQ_TYPE_GET = 0xa1;
""")
        fun = gen_ctrl
    elif sys.argv[1] == 'decl':
        fun = gen_decl
    else:
        print("Invalid mode {1}".format(*sys.argv))
        sys.exit(-1)

    for ctrl in CT_CONTROLS:
        code = fun('CT', *ctrl)
        print(code)

    for ctrl in PU_CONTROLS:
        code = fun('PU', *ctrl)
        print(code)

    for ctrl in SU_CONTROLS:
        code = fun('SU', *ctrl)
        print(code)
