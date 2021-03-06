
#include "mbed.h"
#include <cmath>
#include "DA7212.h"
#include "uLCD_4DGL.h"

#define bufferLength (32)

DA7212 audio;
uLCD_4DGL uLCD(D1, D0, D2); // serial tx, serial rx, reset pin;

Serial pc(USBTX, USBRX);
InterruptIn button2(SW2);
InterruptIn button3(SW3);

EventQueue queue_music(32 * EVENTS_EVENT_SIZE);
EventQueue queue_display(32 * EVENTS_EVENT_SIZE);
EventQueue queue_communicate(32 * EVENTS_EVENT_SIZE);
EventQueue queue_taiko(32 * EVENTS_EVENT_SIZE);
Thread music;
Thread dnn;
Thread display;
Thread receive_send;
Thread taiko;
int idC = 0;
int mode = 0;
int song_num = 0;
int stop = 1;

int16_t waveform[kAudioTxBufferSize];
char serialInBuffer[bufferLength];
int serialCount = 0;
int taiko_start = 0;
int hit_num = 0;
int hit_gesture;

int song_k66f[42] = {0};
int noteLength_k66f[42] = {0};
int Song_Length = 42;
int hit[42] = {0};

DigitalOut green_led(LED2);

int noteLength_taiko[42] = {
  1, 1, 1, 1, 1, 1, 2,
  1, 1, 1, 1, 1, 1, 2,
  1, 1, 1, 1, 1, 1, 2,
  1, 1, 1, 1, 1, 1, 2,
  1, 1, 1, 1, 1, 1, 2,
  1, 1, 1, 1, 1, 1, 2};

int song_taiko[42] = {
  261, 261, 392, 392, 440, 440, 392,
  349, 349, 330, 330, 294, 294, 261,
  392, 392, 349, 349, 330, 330, 294,
  392, 392, 349, 349, 330, 330, 294,
  261, 261, 392, 392, 440, 440, 392,
  349, 349, 330, 330, 294, 294, 261};

//
/*


int noteLength_k66f[42] = {
  1, 1, 1, 1, 1, 1, 2,
  1, 1, 1, 1, 1, 1, 2,
  1, 1, 1, 1, 1, 1, 2,
  1, 1, 1, 1, 1, 1, 2,
  1, 1, 1, 1, 1, 1, 2,
  1, 1, 1, 1, 1, 1, 2};
*/

//

#include "accelerometer_handler.h"
#include "config.h"
#include "magic_wand_model_data.h"

#include "tensorflow/lite/c/common.h"
#include "tensorflow/lite/micro/kernels/micro_ops.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/version.h"

// ************ Global Initialization of DNN ************
// Create an area of memory to use for input, output, and intermediate arrays.
// The size of this will depend on the model you're using, and may need to be
// determined by experimentation.
constexpr int kTensorArenaSize = 60 * 1024;
uint8_t tensor_arena[kTensorArenaSize];

// Whether we should clear the buffer next time we fetch data
bool should_clear_buffer = false;
bool got_data = false;

// The gesture index of the prediction
int gesture_index;

// Set up logging.
static tflite::MicroErrorReporter micro_error_reporter;
tflite::ErrorReporter* error_reporter = &micro_error_reporter;

// Map the model into a usable data structure. This doesn't involve any
// copying or parsing, it's a very lightweight operation.
const tflite::Model* model = tflite::GetModel(g_magic_wand_model_data);

static tflite::MicroOpResolver<6> micro_op_resolver;

// Build an interpreter to run the model with
static tflite::MicroInterpreter static_interpreter(
    model, micro_op_resolver, tensor_arena, kTensorArenaSize, error_reporter);
tflite::MicroInterpreter* interpreter = &static_interpreter;

TfLiteTensor* model_input = interpreter->input(0);

int input_length = model_input->bytes / sizeof(float);

TfLiteStatus setup_status = SetupAccelerometer(error_reporter);

// ************ End of Global Initialization of DNN ************


// Return the result of the last prediction
int PredictGesture(float* output) {
  // How many times the most recent gesture has been matched in a row
  static int continuous_count = 0;
  // The result of the last prediction
  static int last_predict = -1;

  // Find whichever output has a probability > 0.8 (they sum to 1)
  int this_predict = -1;
  for (int i = 0; i < label_num; i++) {
    if (output[i] > 0.8) this_predict = i;
  }

  // No gesture was detected above the threshold
  if (this_predict == -1) {
    continuous_count = 0;
    last_predict = label_num;
    return label_num;
  }

  if (last_predict == this_predict) {
    continuous_count += 1;
  } else {
    continuous_count = 0;
  }
  last_predict = this_predict;

  // If we haven't yet had enough consecutive matches for this gesture,
  // report a negative result
  if (continuous_count < config.consecutiveInferenceThresholds[this_predict]) {
    return label_num;
  }
  // Otherwise, we've seen a positive result, so clear all our variables
  // and report it
  continuous_count = 0;
  last_predict = -1;

  return this_predict;
}
/*
void playNote(int freq)
{
  for(int i = 0; i < kAudioTxBufferSize; i++) {
    waveform[i] = (int16_t) (sin((double)i * 2. * M_PI/(double) (kAudioSampleFrequency / freq)) * ((1<<16) - 1));
  }
  audio.spk.play(waveform, kAudioTxBufferSize);
}
*/

void playNote(int freq)
{
  taiko_start = 1;
  for(int i = 0; i < kAudioTxBufferSize; i++)
  {
    waveform[i] = (int16_t) (sin((double)i * 2. * M_PI/(double) (kAudioSampleFrequency / freq)) * ((1<<16) - 1));
  }
  audio.spk.play(waveform, kAudioTxBufferSize);
}

void loadSong(void)
{
  /*
  green_led = 0;
  int i = 0, j = 0;
  audio.spk.pause();
  
  char tmp[4];
  char ch;
  
  if(pc.readable()) {    
      for (i = 0; i < Song_Length; i++) {
          for (j = 0; j < 3; j++) {
              tmp[j] = pc.getc();
          }
          pc.getc();
          tmp[j] = '\0';
          song_k66f[i] = (int)atof(tmp);
          pc.printf("%d: %d\n\r", i, song_k66f[i]);
      }
      
      for (i = 0; i < Song_Length; i++) {
          ch = pc.getc();
          pc.getc();
          noteLength_k66f[i] = ch - '0';
          pc.printf("%d: %d\n\r", i, noteLength_k66f[i]);     
      }
  }
  green_led = 1;
*/
  stop = 0;
  green_led = 0;
  int i = 0;
  serialCount = 0;
  // audio.spk.pause();
  while(i < Song_Length)
  {
    if(pc.readable())
    {

      serialInBuffer[serialCount] = pc.getc();
      serialCount++;
      if(serialCount == 3)
      {
        serialInBuffer[serialCount] = '\0';
        song_k66f[i] = (int) atof(serialInBuffer);
        // pc.printf("%d: %d\n\r", i, song_k66f[i]);
        serialCount = 0;
        i++;
      }
    }
  }

  i = 0;
  while(i < Song_Length)
  {
    if(pc.readable())
    {
      serialInBuffer[serialCount] = pc.getc();
      serialCount++;
      if(serialCount == 1)
      {
        serialInBuffer[serialCount] = '\0';
        noteLength_k66f[i] = (int) atof(serialInBuffer);
        // uLCD.printf("%d: %d\n\r", i, noteLength_k66f[i]);
        serialCount = 0;
        i++;
      }
    }
  }

  green_led = 1;

  // play song
  /*
  for(int i = 0; i < Song_Length && stop != 1; i++) {
      int length = noteLength_k66f[i];
      while(length--) {
          // the loop below will play the note for the duration of 1s
          for(int j = 0; j < kAudioSampleFrequency / kAudioTxBufferSize; ++j) {
              queue_music.call(playNote, song_k66f[i]);
          }
          if(length < 1) wait(1.0);
      }
  }
  */
  for(int i = 0; i < 42 && stop != 1; i++)
  {
    int length = noteLength_k66f[i];
    while(length--)
    {
      // the loop below will play the note for the duration of 1s
      for(int j = 0; j < kAudioSampleFrequency / kAudioTxBufferSize; ++j)
      {
        queue_music.call(playNote, song_k66f[i]);
      }
      if(length >= 0) wait(1.0);
    }
  }
}

void taiko_play(void) 
{
  taiko_start = 1;
  for(int i = 0; i < 42; i++)
  {
    int length = noteLength_taiko[i];
    while(length--)
    {
      // the loop below will play the note for the duration of 1s
      for(int j = 0; j < kAudioSampleFrequency / kAudioTxBufferSize; ++j)
      {
        queue_music.call(playNote, song_taiko[i]);
      }
      if(length >= 0) wait(1.0);
    }
  }
  audio.spk.pause();
}


void uLCDu() {
  if (mode == 0) { 
    uLCD.cls();
    uLCD.printf("* Forward\n");
    uLCD.printf("  Backward\n");
    uLCD.printf("  change songs\n");
    uLCD.printf("  Taiko game\n");
  }
  else if (mode == 1) {
    uLCD.cls();
    uLCD.printf("  Forward\n");
    uLCD.printf("* Backward\n");
    uLCD.printf("  change songs\n");
    uLCD.printf("  Taiko game\n");
  }
  else if (mode == 2) { 
    uLCD.cls();
    uLCD.printf("  Forward\n");
    uLCD.printf("  Backward\n");
    uLCD.printf("* change songs\n");
    uLCD.printf("  Taiko game\n");
  }
  else if (mode == 3) {
    uLCD.cls();
    uLCD.printf("  Forward\n");
    uLCD.printf("  Backward\n");
    uLCD.printf("  change songs\n");
    uLCD.printf("* Taiko game\n");
  }
  else if (mode == 5) {
    uLCD.cls();
    uLCD.printf("Song PLAY\n");
    uLCD.locate(0,5);
    if (song_num == 0)
      uLCD.printf("Star\n");
    else if (song_num == 1)
      uLCD.printf("Do Do Do\n");
    else if (song_num == 2)
      uLCD.printf("La La La\n");    
  }
  else if (mode == 7) {
    uLCD.cls();
    uLCD.printf("Song Selection\n\n");
    uLCD.printf("* STAR\n");
    uLCD.printf("  Do Do Do\n");
    uLCD.printf("  Ra Ra Ra\n");
  }
  else if (mode == 8) {
    uLCD.cls();
    uLCD.printf("Song Selection\n\n");
    uLCD.printf("  STAR\n");
    uLCD.printf("* Do Do Do\n");
    uLCD.printf("  Ra Ra Ra\n");
  } 
  else if (mode == 9) {
    uLCD.cls();
    uLCD.printf("Song Selection\n\n");
    uLCD.printf("  STAR\n");
    uLCD.printf("  Do Do Do\n");
    uLCD.printf("* Ra Ra Ra\n");
  }
  else if(mode == 20) {
    float score = 0;
    float accurancy = 0;

    uLCD.cls();
    uLCD.printf("Start Taiko game\n");
    uLCD.locate(0,6);
    uLCD.printf("Score: %lf\n", score);

    queue_taiko.call(taiko_play);
    

    for (int i = 0; i < 42; i++) {
      int length = noteLength_taiko[i];
      while(length--) {
        hit_gesture = 0;
        uLCD.locate(0,4);
        uLCD.printf("%d", noteLength_taiko[i]);
        
        wait(0.5);
        hit[hit_num++] = hit_gesture;
        uLCD.locate(0,9);
        uLCD.printf("Hit_gesture: %d\n", hit_gesture);
        uLCD.locate(0,4);
        uLCD.printf("   ");
        wait(0.2);  // total wait only 0.7 here, but it will be the same period as 1sec as taiko_play
      }
    }
    
    for (int i = 0; i < 42; i++) {
      if (hit[i] == noteLength_taiko[i])
        accurancy = accurancy + 1;
    }
    score = accurancy / 42.0;
    uLCD.locate(0,6);
    uLCD.printf("Score: %lf %lf\n", accurancy, score);

  }   
}



void dnn_go(void) {
  while (true) {
    // Attempt to read new data from the accelerometer
    got_data = ReadAccelerometer(error_reporter, model_input->data.f,
                                  input_length, should_clear_buffer);

    // If there was no new data,
    // don't try to clear the buffer again and wait until next time
    if (!got_data) {
      should_clear_buffer = false;
      continue;
    }

    // Run inference, and report any error
    TfLiteStatus invoke_status = interpreter->Invoke();
    if (invoke_status != kTfLiteOk) {
      error_reporter->Report("Invoke failed on index: %d\n", begin_index);
      continue;
    }

    // Analyze the results to obtain a prediction
    gesture_index = PredictGesture(interpreter->output(0)->data.f);

    // Clear the buffer next time we read data
    should_clear_buffer = gesture_index < label_num;

    // Produce an output
    if (gesture_index < label_num) {
      // error_reporter->Report(config.output_message[gesture_index]);

      if (taiko_start == 1) {
        if (gesture_index == 0)
          hit_gesture = 1;
        else if (gesture_index == 2)
          hit_gesture = 2;
      }
      else {
        // mode selection
        if (0 <= mode && mode <= 3) {
          if (gesture_index == 0) {
            mode++;
            if (mode==4)
              mode = 0;
          }
          if (gesture_index == 2) {
            mode--;
            if (mode == -1)
              mode = 3;
          }
        }
        // change song selection
        if (6 <= mode && mode <= 8) {
          if (gesture_index == 0) {
            mode++;
            if (mode == 9)
              mode = 6;
          }
          if (gesture_index == 2) {
            mode--;
            if (mode == 5)
              mode = 8;
          }
        }
        queue_display.call(uLCDu);
      }
    }
  }
}

void modeSelection(void) {
  mode = 0;
  stop = 1;
  taiko_start = 0;
  audio.spk.pause();
  queue_display.call(uLCDu);
}

void confirm(void) {
  
  if (mode == 0) {
    song_num++;
    if (song_num == 3)
      song_num = 2;
    pc.printf("%d\n", song_num);
    mode = 5;
    uLCDu();
    loadSong();
  }
  else if (mode == 1) {
    song_num--;
    if (song_num == -1)
      song_num = 0;
    pc.printf("%d\n", song_num);
    mode = 5;
    uLCDu();
    loadSong();
  }
  else if (mode == 2){    // mode == 2
    mode = 7;
    uLCDu();
  }
  else if (mode == 3) { // Taiko
    mode = 20;
    uLCDu();
  }
  else if (mode == 7) {
    song_num = 0;
    pc.printf("%d\n", song_num);
    mode = 5;
    uLCDu();
    loadSong();
  }
  else if (mode == 8) {
    song_num = 1;
    pc.printf("%d\n", song_num);
    mode = 5;
    uLCDu();
    loadSong();
  }
  else if (mode == 9) {
    song_num = 2;
    pc.printf("%d\n", song_num);
    mode = 5;
    uLCDu();
    loadSong();
  }
  
}

int main(int argc, char* argv[]) {

  green_led = 1;
  music.start(callback(&queue_music, &EventQueue::dispatch_forever));
  receive_send.start(callback(&queue_communicate, &EventQueue::dispatch_forever));
  display.start(callback(&queue_display, &EventQueue::dispatch_forever));
  taiko.start(callback(&queue_taiko, &EventQueue::dispatch_forever));
  button2.rise(modeSelection);
  button3.rise(queue_communicate.event(confirm));

  uLCD.cls();
  uLCD.printf("* Forward\n");
  uLCD.printf("  Backward\n");
  uLCD.printf("  change songs\n");  
  uLCD.printf("  Taiko game\n");
  /*

  */

  if (model->version() != TFLITE_SCHEMA_VERSION) {
    error_reporter->Report(
        "Model provided is schema version %d not equal "
        "to supported version %d.",
        model->version(), TFLITE_SCHEMA_VERSION);
    return -1;
  }

  // Pull in only the operation implementations we need.
  // This relies on a complete list of all the ops needed by this graph.
  // An easier approach is to just use the AllOpsResolver, but this will
  // incur some penalty in code space for op implementations that are not
  // needed by this graph.

  micro_op_resolver.AddBuiltin(
      tflite::BuiltinOperator_DEPTHWISE_CONV_2D,
      tflite::ops::micro::Register_DEPTHWISE_CONV_2D());
  micro_op_resolver.AddBuiltin(tflite::BuiltinOperator_MAX_POOL_2D,
                               tflite::ops::micro::Register_MAX_POOL_2D());
  micro_op_resolver.AddBuiltin(tflite::BuiltinOperator_CONV_2D,
                               tflite::ops::micro::Register_CONV_2D());
  micro_op_resolver.AddBuiltin(tflite::BuiltinOperator_RESHAPE,
                               tflite::ops::micro::Register_RESHAPE(), 1);
  micro_op_resolver.AddBuiltin(tflite::BuiltinOperator_FULLY_CONNECTED,
                               tflite::ops::micro::Register_FULLY_CONNECTED());
  micro_op_resolver.AddBuiltin(tflite::BuiltinOperator_SOFTMAX,
                               tflite::ops::micro::Register_SOFTMAX());

  // Allocate memory from the tensor_arena for the model's tensors
  interpreter->AllocateTensors();

  // Obtain pointer to the model's input tensor

  if ((model_input->dims->size != 4) || (model_input->dims->data[0] != 1) ||
      (model_input->dims->data[1] != config.seq_length) ||
      (model_input->dims->data[2] != kChannelNumber) ||
      (model_input->type != kTfLiteFloat32)) {
    error_reporter->Report("Bad input tensor parameters in model");
    return -1;
  }
  
  if (setup_status != kTfLiteOk) {
    error_reporter->Report("Set up failed\n");
    return -1;
  }

  // error_reporter->Report("Set up successful...\n");
  
  dnn.start(dnn_go);
}