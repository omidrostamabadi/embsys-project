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
#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include <sstream>
#include <mysql/mysql.h>
#include <helper.h>

// defining global constants
const int MAX_RECORDING_SECONDS = 15;       //maximum time of recording
const int RECORDING_BUFFER_SECONDS = 16;    //buffersize, providing a safe pad of 1 seconds more than

int gRecordingDeviceCount = 0;          //number of recording devices in host machine
SDL_AudioSpec gReceivedRecordingSpec;   //the available spec for recording
uint32_t sound_threshold = 15000;
std::string file_name = "eng.txt";
std::ofstream eng_file;

//Audio device IDs
SDL_AudioDeviceID recordingDeviceId = 0;
SDL_AudioDeviceID playbackDeviceId = 0;

Uint8* gRecordingBuffer = NULL;         //the buffer holding the recorded sound
Uint32 gBufferByteSize = 0; 
Uint32 gBufferBytePosition = 0;         //specifies the current location in gRecordingBuffer
Uint32 gBufferByteMaxPosition = 0;      //defines an upper bound for gBufferBytePosition
Uint32 gBufferByteRecordedPosition = 0; //defines the place where the recording has stopped

void audioRecordingCallback( void*, Uint8*, int);
void setRecordingSpec(SDL_AudioSpec*);
void close();                           //frees the allocated buffers and terminates SDL
void reportError(const char*);          //printing proper error messages to the screen
void calculate_energy();



void start_recording();
void stop_recording();

void audio_manager() {
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) //make sure SDL initilizes correctly
        reportError("Initilizing SDL error.");
    
    gRecordingDeviceCount = SDL_GetNumAudioDevices(SDL_TRUE); //get total number of recording devices
    std::cout << "Total recording devices found: "<< gRecordingDeviceCount << std::endl;
    
    for (int i=0; i<gRecordingDeviceCount; i++)
        std::cout<<i<<": "<<SDL_GetAudioDeviceName(i, SDL_TRUE);

    int index;
    int bytesPerSample;
    int bytesPerSecond;
    SDL_AudioSpec desiredRecordingSpec, desiredPlaybackSpec;

    index = 0;
    if (index >= gRecordingDeviceCount) {
        std::cout<<"Error: out of range device selected."<<std::endl;
        exit(1);
    }

    std::cout << "Using " <<index << ": "<< SDL_GetAudioDeviceName(index, SDL_TRUE)<<" for recording"<<std::endl;
    //SDL_AudioSpec desiredRecordingSpec, desiredPlaybackSpec;
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

    eng_file.open(file_name, std::ios::out);
    

    start_recording();

    while(true) {
      // start_recording();
      // // stop_recording();
      // calculate_energy();
      // gBufferBytePosition = 0;
      usleep(1 * 1000);
    }
    
    //main loop of the program
    /*while(!quit) {
        // check for events (e.g. key press)
        while(SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT)
				quit = true;
		
            if (e.type != SDL_KEYDOWN) continue; // proceed only if there is a key down
            
            short key = e.key.keysym.sym;

            if (key == SDLK_q) //if q is pressed, exit
                quit = true;
            
            next_state = get_next_state(current_state, key);

            switch(current_state) {    
                case WAITING:
                    if (next_state == RECORDING) {
                        start_recording();
                    }
                break;
                
                case RECORDING: 
                    if (next_state == RECORDED) {
                        stop_recording();
                    }
                    else if (next_state == PAUSE) {
                        // to be implemented :p
                    }
                break;
                
                case RECORDED:
                    if (next_state == PLAYBACK) {
                        gBufferBytePosition = 0;                            //preparing for playback 
                        SDL_PauseAudioDevice(playbackDeviceId, SDL_FALSE);  //start playback
                    }
                break;

                default:
                    quit = true;   
                break;
            }
            current_state = next_state;
            
            // show menu if needed
            switch(current_state) {
                case WAITING: case RECORDING: case RECORDED: case PLAYBACK: case ERROR:
                    show_menu(current_state);
                default: break;
            }
        }

        switch(current_state) {
            case DEVICE_SELECTION:
                std::cout<<"Select one of the devices listed above for recording: ";
                //int index;
                cin >> index;
                if (index >= gRecordingDeviceCount) {
                    std::cout<<"Error: out of range device selected."<<std::endl;
                    exit(1);
                }
                std::cout << "Using " <<index << ": "<< SDL_GetAudioDeviceName(index, SDL_TRUE)<<" for recording"<<std::endl;
                //SDL_AudioSpec desiredRecordingSpec, desiredPlaybackSpec;
                setRecordingSpec(&desiredRecordingSpec);
                setPlaybackSpec(&desiredPlaybackSpec);
                //opening the device for recording
                recordingDeviceId = SDL_OpenAudioDevice( SDL_GetAudioDeviceName( index, SDL_TRUE ), SDL_TRUE, 
                &desiredRecordingSpec, &gReceivedRecordingSpec, SDL_AUDIO_ALLOW_FORMAT_CHANGE );
                //opening the device for playback
                playbackDeviceId = SDL_OpenAudioDevice( NULL, SDL_FALSE, 
                &desiredPlaybackSpec, &gReceivedPlaybackSpec, SDL_AUDIO_ALLOW_FORMAT_CHANGE );
                //calculating some audio quantities based on received spec for recording and playback
                bytesPerSample = gReceivedRecordingSpec.channels * 
                ( SDL_AUDIO_BITSIZE( gReceivedRecordingSpec.format ) / 8 );
                bytesPerSecond = gReceivedRecordingSpec.freq * bytesPerSample;
                gBufferByteSize = RECORDING_BUFFER_SECONDS * bytesPerSecond;
                gBufferByteMaxPosition = MAX_RECORDING_SECONDS * bytesPerSecond;
                gRecordingBuffer = new Uint8[ gBufferByteSize ];
                memset( gRecordingBuffer, 0, gBufferByteSize );
                current_state = WAITING;
                show_menu(current_state);
                break;
            
            case RECORDING:
                //Lock callback
                SDL_LockAudioDevice( recordingDeviceId );

                //check if the buffer has reached maximum capacity
                if ( gBufferBytePosition > gBufferByteMaxPosition ) {
                    //Stop recording audio
                    SDL_PauseAudioDevice( recordingDeviceId, SDL_TRUE ); //stop recording
                    std::cout << "You reached recording limit!"<<std::endl;
                    current_state = RECORDED;
                    gBufferByteRecordedPosition = gBufferBytePosition;
                }
                SDL_UnlockAudioDevice( recordingDeviceId );
                break;
            
            case PLAYBACK: //playing the recorded voice back
                SDL_LockAudioDevice(playbackDeviceId);
                if (gBufferBytePosition > gBufferByteRecordedPosition) { //stop playback
                    SDL_PauseAudioDevice(playbackDeviceId, SDL_TRUE);
                    current_state = RECORDED;
                    show_menu(current_state);
                    std::cout << "Finished playback\n";
                }
                SDL_UnlockAudioDevice(playbackDeviceId);
                break; 
            default:
            break; 

        }
    }*/

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
    // from stream to buffer
    // std::cout << "Got " << len << " Bytes callback\n";
    memcpy(&gRecordingBuffer[0], stream, len);
    gBufferBytePosition += len;
    // stop_recording();
    calculate_energy();
    gBufferBytePosition = 0;
}

void close() {
    if (gRecordingBuffer != NULL) {
        delete[] gRecordingBuffer;
        gRecordingBuffer = NULL;
    }
    SDL_Quit();
}

void reportError(const char* msg) { //reports the proper error message then terminates
    std::cout<<"An error happend. "<<msg<<" "<<"SDL_ERROR: "<<SDL_GetError()<<std::endl;
    exit(1);
}

void start_recording() {
    // LOG("Now started recording", std::cout, "AUDIO MANAGER")
    gBufferBytePosition = 0;                                //reseting the buffer
    gBufferByteRecordedPosition = 0;                        //reseting the buffer
    SDL_PauseAudioDevice(recordingDeviceId, SDL_FALSE);     //start recording
}

void stop_recording() {
    SDL_PauseAudioDevice(recordingDeviceId, SDL_TRUE);
    // gBufferByteRecordedPosition = gBufferBytePosition;
    // gBufferBytePosition = 0; //preparing for playback
}

void calculate_energy() {
  static uint32_t win_size = 20000;
  uint32_t l_start, l_end, l_energy;
  for(int i = 0; i < gBufferBytePosition; i += win_size / 2) {
    l_start = i; 
    l_end = i + win_size;
    if(l_end >= gBufferBytePosition)
      return;
    l_energy = 0;
    for(uint32_t j = l_start; j < l_end; j++) {
      l_energy += gRecordingBuffer[j] * gRecordingBuffer[j];
    }
    l_energy /= (l_end - l_start);
    eng_file << l_energy << std::endl;
    if(l_energy > sound_threshold) {
      std::cout << "Energy is " << l_energy << std::endl;
    }
  }
}