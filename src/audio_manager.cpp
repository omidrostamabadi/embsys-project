#include <iostream>
#include <cstdlib>
#include <string>
#include <fstream>
#include <thread>
#include <atomic>
#include <chrono>
#include <cstring>
#include <stdio.h>
#include <ctime>
#include <unistd.h>
#include <iterator>
#include <thread>
#include <signal.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include <sstream>
#include <mysql/mysql.h>
#include <audio_helper.h>

// defining global constants
const int MAX_RECORDING_SECONDS = 15;       //maximum time of recording
const int RECORDING_BUFFER_SECONDS = 16;    //buffersize, providing a safe pad of 1 seconds more than

int gRecordingDeviceCount = 0;          //number of recording devices in host machine
SDL_AudioSpec gReceivedRecordingSpec;   //the available spec for recording
uint32_t sound_threshold = 400;

std::ofstream out_file;

//Audio device IDs
SDL_AudioDeviceID recordingDeviceId = 0;
SDL_AudioDeviceID playbackDeviceId = 0;

Uint8* gRecordingBuffer = NULL;         //the buffer holding the recorded sound
Uint32 gBufferByteSize = 0; 
Uint32 gBufferBytePosition = 0;         //specifies the current location in gRecordingBuffer
Uint32 gBufferByteMaxPosition = 0;      //defines an upper bound for gBufferBytePosition
Uint32 gBufferByteRecordedPosition = 0; //defines the place where the recording has stopped

/* Database variables */
static MYSQL *mysql_connection;

void audioRecordingCallback( void*, Uint8*, int);
void setRecordingSpec(SDL_AudioSpec*);
void close();                           //frees the allocated buffers and terminates SDL
void reportError(const char*);          //printing proper error messages to the screen
void calculate_energy();
void alrm_handler(int sig, siginfo_t *sig_info, void *void_var);


void start_recording();
void stop_recording();

int main() {
  out_file.open("outlog.txt", std::ios::out);
  SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");
    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_EVENTS | SDL_INIT_TIMER) < 0) //make sure SDL initilizes correctly
        reportError("Initilizing SDL error.");
    
    gRecordingDeviceCount = SDL_GetNumAudioDevices(SDL_TRUE); //get total number of recording devices
    LOG("Total recording devices found: " + std::to_string(gRecordingDeviceCount),
     out_file, "AUDIO MANAGER")

    int index;
    int bytesPerSample;
    int bytesPerSecond;
    SDL_AudioSpec desiredRecordingSpec, desiredPlaybackSpec;

    index = 0;
    if (index >= gRecordingDeviceCount) {
        out_file<<"Error: out of range device selected."<<std::endl;
        exit(1);
    }

    LOG("Recording device in use: " + std::string(SDL_GetAudioDeviceName(index, SDL_TRUE)),
     out_file, "AUDIO MANAGER")
    setRecordingSpec(&desiredRecordingSpec);
    //opening the device for recording
    recordingDeviceId = SDL_OpenAudioDevice( SDL_GetAudioDeviceName( index, SDL_TRUE ), SDL_TRUE, 
    &desiredRecordingSpec, &gReceivedRecordingSpec, SDL_AUDIO_ALLOW_FORMAT_CHANGE );
    //calculating some audio quantities based on received spec for recording and playback
    bytesPerSample = gReceivedRecordingSpec.channels * 
    ( SDL_AUDIO_BITSIZE( gReceivedRecordingSpec.format ) / 8 );
    bytesPerSecond = gReceivedRecordingSpec.freq * bytesPerSample;
    gBufferByteSize = RECORDING_BUFFER_SECONDS * bytesPerSecond;
    gBufferByteMaxPosition = MAX_RECORDING_SECONDS * bytesPerSecond;
    gRecordingBuffer = new Uint8[gBufferByteSize];
    memset(gRecordingBuffer, 0, gBufferByteSize);

    /* Connect to database */
    mysql_connection = mysql_init(NULL);
    MYSQL *mysql_connection_ret = NULL;
    mysql_connection_ret = mysql_real_connect(mysql_connection, db_host_name.c_str(), db_user_name.c_str(),
      db_password.c_str(), db_database_name.c_str(), db_port, NULL, 0);
    if(mysql_connection_ret == NULL) {
      std::cerr << __FILE__ << ":" << __LINE__ << ": Connection to db failed\n";
      return 0;
    }

    struct sigaction act;
    sigemptyset(&act.sa_mask);
    act.sa_flags = SA_SIGINFO | SA_RESTART; /* use sa_sigaction (with 3 args instead of 1) */
    act.sa_sigaction = alrm_handler;
    if(sigaction(SIGALRM, &act, NULL)) {
      LOG("Sigaction failed", out_file, "AUDIO MANAGER");
    }

    alarm(2);
    while(true) {
    }
    LOG("Exiting", out_file, "AUDIO MANAGER")
    close();
}

void setRecordingSpec(SDL_AudioSpec* desired) {
    SDL_zero(*desired);
    desired -> freq = 44100;
    desired -> format = AUDIO_F32;
    desired -> channels = 2;
    desired -> samples = 4096;
    desired -> callback = audioRecordingCallback;
}

void audioRecordingCallback(void* userdata, Uint8* stream, int len) {
    memcpy(&gRecordingBuffer[0], stream, len);
    gBufferBytePosition += len;
    calculate_energy();
    gBufferBytePosition = 0;
}

void close() {
    stop_recording();
    if (gRecordingBuffer != NULL) {
        delete[] gRecordingBuffer;
        gRecordingBuffer = NULL;
    }
    SDL_Quit();
}

void reportError(const char* msg) { //reports the proper error message then terminates
    LOG("SDL Error: " + std::string(SDL_GetError()), out_file, "AUDIO MANAGER")
    exit(1);
}

void start_recording() {
    gBufferBytePosition = 0;                                //reseting the buffer
    gBufferByteRecordedPosition = 0;                        //reseting the buffer
    SDL_PauseAudioDevice(recordingDeviceId, SDL_FALSE);     //start recording
}

void stop_recording() {
    SDL_PauseAudioDevice(recordingDeviceId, SDL_TRUE);
}

void insert_into_db(float energy) {
  char mysql_query_msg[MAX_MYSQL_QUERY];
  sprintf(mysql_query_msg, "INSERT INTO audio_table(ts, audio_energy) VALUES (NOW(), %.2f)", energy);
  CHECK_VOID(mysql_query(mysql_connection, mysql_query_msg), "Cannot insert into database", std::cerr)
}

void calculate_energy() {
  /* Bytes per milisecond. gBufferByteSize is enough for RECORDING_BUFFER_SECONDS of bytes, so
    for one second it should be divided by that value. Then to convert second -> milisecond the number
    should further be divided by 1000. */
  static int bpms = gBufferByteSize / RECORDING_BUFFER_SECONDS / 1000;
  /* Each sample is a float, so number of samples per second is 1/4 the bpms. */
  static int spms = bpms / 4;
  static uint32_t win_size = 512;
  float *aud_buffer = (float*) gRecordingBuffer;
  /* Samples are float */
  uint32_t buffer_sample_index = gBufferBytePosition / 4;
  uint32_t l_start, l_end;
  float l_energy;
  for(int i = 0; i < buffer_sample_index; i += win_size / 2) {
    l_start = i; 
    l_end = i + win_size;
    if(l_end >= buffer_sample_index)
      return;
    l_energy = 0;
    for(uint32_t j = l_start; j < l_end; j++) {
      l_energy += aud_buffer[j] * aud_buffer[j];
    }
    l_energy /= (l_end - l_start);
    l_energy *= 1e4;
    if(l_energy > sound_threshold) {
      stop_recording();
      alarm(5);
      insert_into_db(l_energy);
      LOG("Energy exceeds maximum -> " + std::to_string(l_energy), out_file, "AUDIO MANAGER")
      /* Skip next samples of this callback in time. Since a loud sound in real world usually lasts 
        for a long enough time to trigger multiple up thresholds
         and we do not want to report multiple events in a second */
      return; 
    }
  }
}

void alrm_handler(int sig, siginfo_t *sig_info, void *void_var) {
  start_recording();
}