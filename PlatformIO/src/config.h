#ifndef TENSORFLOW_LITE_MICRO_EXAMPLES_PERSON_DETECTION_CONFIG_H_
#define TENSORFLOW_LITE_MICRO_EXAMPLES_PERSON_DETECTION_CONFIG_H_

// VFlip
#define CONFIG_CAMERA_VFLIP 1

// Select device
//#define CONFIG_CAMERA_MODEL_WROVER_KIT 1
//#define CONFIG_CAMERA_MODEL_ESP_EYE 1
//#define CONFIG_CAMERA_MODEL_M5STACK_PSRAM 1
//#define CONFIG_CAMERA_MODEL_M5STACK_WIDE 1
#define CONFIG_CAMERA_MODEL_AI_THINKER 1
//#define CONFIG_CAMERA_MODEL_CUSTOM 1

// Other Camera setting
#ifdef CONFIG_CAMERA_MODEL_CUSTOM
#define CONFIG_CAMERA_PIN_PWDN   26
#define CONFIG_CAMERA_PIN_RESET  -1
#define CONFIG_CAMERA_PIN_XCLK   32
#define CONFIG_CAMERA_PIN_SIOD   13
#define CONFIG_CAMERA_PIN_SIOC   12

#define CONFIG_CAMERA_PIN_Y9     39
#define CONFIG_CAMERA_PIN_Y8     36
#define CONFIG_CAMERA_PIN_Y7     23
#define CONFIG_CAMERA_PIN_Y6     18
#define CONFIG_CAMERA_PIN_Y5     15
#define CONFIG_CAMERA_PIN_Y4     4
#define CONFIG_CAMERA_PIN_Y3     14
#define CONFIG_CAMERA_PIN_Y2     5
#define CONFIG_CAMERA_PIN_VSYNC  27
#define CONFIG_CAMERA_PIN_HREF   25
#define CONFIG_CAMERA_PIN_PCLK   19
#endif

#endif  // TENSORFLOW_LITE_MICRO_EXAMPLES_PERSON_DETECTION_CONFIG_H_
