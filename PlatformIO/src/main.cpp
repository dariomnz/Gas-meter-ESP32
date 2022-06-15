

#include "model_data.h"
#include "tensorflow/lite/micro/kernels/micro_ops.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/version.h"
#include "opencv2/opencv.hpp"

#include <string>
#include <Arduino.h>
#include <stdio.h>
#include "esp_spiffs.h"
#include <sys/types.h>
#include "system.h"
#include "app_camera_esp.h"
#include <vector>
#include <SPIFFS.h>

// #include <WiFi.h>
#include <influxDB.h>

#include <WiFiMulti.h>
WiFiMulti wifiMulti;

// WiFi AP SSID
#define WIFI_SSID "ONEPLUS"
// WiFi password
#define WIFI_PASSWORD "123456789"

enum COLOR{
  PURPLE = 5,
  BLUE = 6,
  RED = 9,
  GREEN = 10,
  YELLOW = 11,
  WHITE = 15,
};

#define FILE_PHOTO "/captura.bmp"
#define FILE_SPIFFS_CAPTURA "/spiffs/captura.bmp"
#define FILE_SPIFFS_TEMPL "/spiffs/templ.bmp"
#define FILE_SPIFFS_MATCH_RESULT "/spiffs/match_result.bmp"

cv::Mat read_image_specific_res(const std::string &fileName);
void write_image_specific_res(const std::string &fileName);
void config_spiffs();
void img_blur();
void MatchingMethod();
void capture(bool flash = false);
std::string run_tflite();

bool use_mask;
int match_method;
int max_Trackbar = 5;

#define FLASH_GPIO_NUM 4
void setup() {
  
  Serial.begin(115200);
  pinMode(FLASH_GPIO_NUM, OUTPUT);

  disp_infos();
  Serial.printf("Set up start\n");
  unsigned long StartTime = millis();



  capture();
  config_spiffs();
  MatchingMethod();
  std::string reading = run_tflite();
  if (reading != "0")
  {
    WiFi.mode(WIFI_STA);
    wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to wifi");
    while (wifiMulti.run() != WL_CONNECTED) {
      Serial.print(".");
      delay(100);
    }
    Serial.println();

    Serial.print("Connected to WiFi network with IP Address: ");
    Serial.println(WiFi.localIP());

    send_data(reading);
  }

  Serial.printf("Set up complete\n");
  unsigned long CurrentTime = millis();
  double ElapsedTime = (CurrentTime - StartTime) / 1000.0;
  Serial.printf("The program took %lf seconds to execute\n", ElapsedTime);

  esp_sleep_enable_timer_wakeup(30000000*60);
  esp_deep_sleep_start();
}

void loop() {
}

void config_spiffs(){
  Serial.println("Initializing SPIFFS");
    
  esp_vfs_spiffs_conf_t conf = {
    .base_path = "/spiffs",
    .partition_label = NULL,
    .max_files = 30,
    .format_if_mount_failed = false
  };
  
  // Use settings defined above to initialize and mount SPIFFS filesystem.
  // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
  esp_err_t ret = esp_vfs_spiffs_register(&conf);

  if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            Serial.println("Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            Serial.println("Failed to find SPIFFS partition");
        } else {
            Serial.print("Failed to initialize SPIFFS (");
            Serial.println(esp_err_to_name(ret));
        }
        return;
    }else{
      Serial.println("SPIFFS good mounted");

    }
  
  size_t total = 0, used = 0;
  ret = esp_spiffs_info(conf.partition_label, &total, &used);
  if (ret != ESP_OK) {
      Serial.print("Failed to get SPIFFS partition information");
      Serial.println(esp_err_to_name(ret));
  } else {
      Serial.print("Partition size: total: ");
      Serial.print(total);
      Serial.print(" used: ");
        Serial.println(used);
  }

  // Open renamed file for reading
  Serial.println("Reading file");
  FILE* f = fopen("/spiffs/hello.txt", "r");
  if (f == NULL) {
      Serial.println("Failed to open file for reading");
      return;
  }
  char line[64];
  fgets(line, sizeof(line), f);
  fclose(f);
  // strip newline
  char* pos = strchr(line, '\n');
  if (pos) {
      *pos = '\0';
  }
  Serial.printf("Read from file: %s\n",line);

}

void capture(bool flash){
  app_camera_init();

  if (!SPIFFS.begin(false)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
  }
  else {
    delay(500);
    Serial.println("SPIFFS mounted successfully");
  }

  camera_fb_t * fb = NULL;
  if (flash){
    digitalWrite(FLASH_GPIO_NUM, HIGH);
    delay(500);
  } 
  fb = esp_camera_fb_get();
  if (flash){digitalWrite(FLASH_GPIO_NUM, LOW);} 
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }else{
    Serial.println("Camera capture OK");
  }
  uint8_t *out;
  size_t out_len;
  
  int height_menos = fb->height * 0.5;
  camera2bmp(fb->buf, fb->len, fb->width, fb->height, 0, height_menos, fb->format, &out, &out_len);
  
  if (out_len == 0) {
      Serial.println("Failed to convert image");
  }

  if (SPIFFS.exists(FILE_PHOTO)){
    SPIFFS.remove(FILE_PHOTO);
  }
  File file = SPIFFS.open(FILE_PHOTO, FILE_WRITE);
  if (!file) {
      Serial.println("Failed to open file in writing mode");
      return;
    }
    else {
      size_t escrito = 0;
      int buff_size = 10240;
      while (true)
      {
        file.seek(0);
        for(int i=0;i<out_len;i+=buff_size)
        {  
          size_t len = buff_size;
          if (i+buff_size > out_len){
            len = out_len - i;
          }
          escrito = file.write(&out[i], len);
          Serial.printf("Just written: %d,for %d of %d = %.2f %%\n",escrito,i,out_len,(i/(float)out_len)*100.0);
          delay(50);
          if (escrito == 0)
          {
            Serial.printf("Fail in write, restart write\n");
            break;
          }
        }
        if (escrito != 0)
        {
          break;
        }
      }
      Serial.printf("The picture has been saved in %s - Size: %d bytes of %d bytes\n",FILE_PHOTO,file.size(),out_len);
    }
  file.close();

  esp_camera_fb_return(fb); 
  fb = NULL;
  esp_camera_deinit();
  SPIFFS.end();
}


cv::Mat read_image_specific_res(const std::string &fileName){
  delay(100);
  Serial.printf("Reading image: %s\n",fileName.c_str());
  cv::Mat out_img = cv::imread(fileName, cv::IMREAD_GRAYSCALE);
  if(out_img.empty()) {
      Serial.printf("ERROR: Cannot read the image: %s\n", fileName.c_str());
      return out_img;
  }
  Serial.printf("Image read from %s of %dx%dx%d, with %d pixel depth\n",fileName.c_str(), out_img.cols, out_img.rows, out_img.channels(), out_img.depth());
  
  delay(100);
  return out_img;
}

void write_image_specific_res(const std::string &fileName, cv::Mat &img){
  delay(100);
  if (remove(fileName.c_str()) == 0) {
    Serial.printf("The file %s is deleted successfully.\n",fileName.c_str());
  } else {
    Serial.printf("The file %s is not deleted.\n",fileName.c_str());
  }
  bool result = cv::imwrite(fileName,img);
  if (result){
    Serial.printf("Image saved in: %s\n",fileName.c_str());
  }else{
    Serial.printf("ERROR: Fail to save image in: %s\n",fileName.c_str());
  }
  delay(100);
}


void MatchingMethod()
{
  cv::Mat img = read_image_specific_res(FILE_SPIFFS_CAPTURA);
  cv::blur(img, img, cv::Size(3, 3));
  cv::equalizeHist(img,img);

  cv::Mat templ = read_image_specific_res(FILE_SPIFFS_TEMPL);
  
  int scale = 2;
  cv::resize(img,img,cv::Size(img.cols/scale,img.rows/scale));
  cv::resize(templ,templ,cv::Size(templ.cols/scale,templ.rows/scale));
  Serial.println("Resize of images");
  Serial.printf("Image read of %dx%dx%d, with %d pixel depth\n",
   img.cols, img.rows, img.channels(), img.depth());
  Serial.printf("Image read of %dx%dx%d, with %d pixel depth\n",
   templ.cols, templ.rows, templ.channels(), templ.depth());

  float scales[] = {0.6,0.7,0.8,0.9,1,1.1,1.2,1.3,1.4,1.5,1.6};  
  float bestScale = 1; 
  double bestVal = 1; 
  cv::Point bestLoc; 
  cv::Mat result;
  for (float i : scales)
  {
    cv::Mat scale_img;
    cv::resize(img,scale_img,cv::Size(img.cols*i,img.rows*i));
    int result_cols = scale_img.cols - templ.cols + 1;
    int result_rows = scale_img.rows - templ.rows + 1;
    result.create(result_rows, result_cols, CV_32F );
    Serial.printf("Create result for the matchTemplate scale:%.2f\n",i);
    match_method = cv::TM_SQDIFF_NORMED; 
    cv::matchTemplate( scale_img, templ, result, match_method);

    double minVal, maxVal, val; 
    cv::Point minLoc, maxLoc, matchLoc;
    cv::minMaxLoc( result, &minVal, &maxVal, &minLoc, &maxLoc);
    matchLoc = minLoc; 
    val = minVal;
    if (bestVal > val)
    {
      bestVal = val;
      bestLoc = matchLoc;
      bestScale = i;
    }
  }

  result.release();
  img.release();
  float scale_template = 1/bestScale;
  bestLoc.x *= scale_template;
  bestLoc.y *= scale_template;
  Serial.println("MatchTemplate finish");
  cv::Rect numeros(bestLoc.x, bestLoc.y, templ.cols*scale_template , templ.rows*scale_template);
  Serial.printf("Rect numeros of %dx%d, width %d height %d\n",
   numeros.x,numeros.y,numeros.width,numeros.height);
  templ.release();

  
  img = read_image_specific_res(FILE_SPIFFS_CAPTURA);
  cv::blur(img, img, cv::Size(3, 3));
  numeros.x = numeros.x * scale;
  numeros.y = numeros.y * scale;
  numeros.width = numeros.width * scale;
  numeros.height = numeros.height * scale;
  Serial.printf("Rect scaled numeros of %dx%d, width %d height %d\n",
   numeros.x,numeros.y,numeros.width,numeros.height);
  

  Serial.println("Start to save individual digits");
  for (int i = 0; i < 8; i++)
  {
    char path[100];
    sprintf(path, "/spiffs/%d.bmp", i);
    int numero_width = numeros.width/8;
    int numero_height = numeros.height;
    
    cv::Rect crop_rect(numeros.x+i*numero_width,numeros.y,numero_width,numero_height);
    Serial.printf("Rect for %d number of %dx%d, width %d height %d\n",
     i, crop_rect.x,crop_rect.y,crop_rect.width,crop_rect.height);
    cv::Mat img_numero = img(crop_rect);
    
    cv::resize(img_numero,img_numero,cv::Size(28,28));
 
    Serial.printf("Image resize of %dx%dx%d, with %d pixel depth\n",
     img_numero.cols, img_numero.rows, img_numero.channels(), img_numero.depth());
    
    write_image_specific_res(path,img_numero);
  }

  cv::rectangle( img,
   cv::Point(numeros.x,numeros.y),
   cv::Point( numeros.x + numeros.width , numeros.y + numeros.height ),
   cv::Scalar::all(0), 2, 8, 0 );
  write_image_specific_res(FILE_SPIFFS_MATCH_RESULT,img);
  img.release();
}


tflite::ErrorReporter* error_reporter = nullptr;
const tflite::Model* model = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;
TfLiteTensor* input = nullptr;
// An area of memory to use for input, output, and intermediate arrays.
constexpr int kTensorArenaSize = 16 * 1024 * 8;
static uint8_t* tensor_arena;

std::string run_tflite(){
  std::string out = "0";
  tensor_arena = (uint8_t *)mi_malloc(kTensorArenaSize);
  static tflite::MicroErrorReporter micro_error_reporter;
  error_reporter = &micro_error_reporter;

  model = tflite::GetModel(converted_model_tflite);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    Serial.printf(
        "Model provided is schema version %d not equal "
        "to supported version %d.\n",
        model->version(), TFLITE_SCHEMA_VERSION);
    return out;
  }
  
  
  static tflite::MicroMutableOpResolver<7> micro_op_resolver;
  micro_op_resolver.AddFullyConnected();
  micro_op_resolver.AddReshape();
  micro_op_resolver.AddMul();
  micro_op_resolver.AddAdd();
  micro_op_resolver.AddConv2D();
  micro_op_resolver.AddMaxPool2D();
  micro_op_resolver.AddSoftmax();
  static tflite::MicroInterpreter static_interpreter(
      model, micro_op_resolver, tensor_arena, kTensorArenaSize,
      error_reporter);
  interpreter = &static_interpreter;
  
  TfLiteStatus allocate_status = interpreter->AllocateTensors();
  if (allocate_status != kTfLiteOk) {
    Serial.printf("AllocateTensors() failed\n");
    return out;
  }else{
    Serial.printf("AllocateTensors() OK\n");
  }


  int best_salida[8];
  cv::Mat images[8];
  for (int i = 0; i < 8; i++)
  {
    char path[20];
    sprintf(path, "/spiffs/%d.bmp", i);
    cv::Mat img = read_image_specific_res(path);
    img.convertTo(img,CV_32F);
    images[i] = img;
  }

  for (int i = 0; i < 8; i++)
  {
    delay(200);
    cv::Mat img = images[i];

    int bytes = img.total() * img.elemSize();
    
    input = interpreter->input(0);
    memcpy(input->data.f, img.data, bytes);
    Serial.printf("Termino load img to input\n");

    if (kTfLiteOk != interpreter->Invoke()) {
      Serial.printf("Invoke failed.\n");
    }
    Serial.printf("Invoke neural network\n");
  
    TfLiteIntArray* output_dims = interpreter->output(0)->dims;
    auto output_size = output_dims->data[output_dims->size - 1];
    
    if (interpreter->output(0)->type == kTfLiteFloat32)
    {
      Serial.printf("Output for %d.bmp: ",i);
      float best_salida_value = 0;
      for (int j = 0; j < output_size; j++){
        float salida = interpreter->output(0)->data.f[j];
        if (best_salida_value < salida)
        {
            best_salida_value = salida;
            best_salida[i] = j;
        }
        Serial.printf("(%d): %f ",j,salida);
      }
      Serial.printf("\nResult: %d\n",best_salida[i]);
    }else{
      Serial.printf("cannot handle output type %d yet\n",interpreter->output(0)->type);
    }
    img.release();
  }

  std::string str;
  for (int i = 0; i < 8; i++)
  {
    str.push_back(best_salida[i] + '0');
    if (i == 4)
    {
      str.push_back('.');
    }
  }
  
  Serial.printf("Total result: %s\n", str.c_str());
  mi_free(tensor_arena);
  return str;
}