#include "main.h"
#include <stdint.h>
#include <stddef.h> 

#ifndef __SPEAKERS_H__
#define __SPEAKERS_H__

#define SPEAKER_PIN  P9_6
#define AUDIO_SAMPLE_RATE_HZ    (48000u)
#define AUDIO_BUFFER_SIZE       (256u)

extern EventGroupHandle_t wall_event;
#define WALL_EVENT_BIT (1<<1)
#define VICTORY_EVENT_BIT (1<<2) 
#define CONNECTION_EVENT_BIT (1<<3)


#define TEST_FREQUENCY_HZ   1000
#define TEST_AMPLITUDE      32767
#define VOLUME_PERCENT      300

// Audio Commands
typedef enum
{
    AUDIO_COMMAND_TEST,    // Square Wave (Beep)
    AUDIO_COMMAND_PLAY,    // Continuous Sine Tone
    AUDIO_COMMAND_VICTORY, // Synthesized Melody
    AUDIO_COMMAND_STOP     // Silence
} audio_command_t;

typedef struct
{
    audio_command_t command;
} audio_message_t;

extern QueueHandle_t q_audio;

void task_audio_init(void);


cy_rslt_t speakers_init();
int speaker_write(const int16_t* data, size_t len);
//void speaker_enable();
void speaker_wave(int16_t* buffer, size_t num_samples, uint32_t frequency, int16_t amplitude);
void speaker_volume(int16_t* buffer, size_t num_samples, uint16_t volume_percent);
int speaker_test_blocking(void);
void speaker_bang(int16_t* buffer, size_t num_samples, int16_t amplitude);
void speaker_click(int16_t* buffer, size_t num_samples, int16_t amplitude);
void speaker_startup(int16_t* buffer, size_t buffer_size, int sample_rate);
void wall_hit_note(int16_t* buffer, size_t buffer_size, int sample_rate, float frequency);
void beep(int16_t* buffer, size_t buffer_size, int sample_rate, float frequency);
void speaker_victory(int16_t* buffer, size_t buffer_size, int sample_rate);
void speaker_mario(int16_t* buffer, size_t buffer_size, int sample_rate);
void speaker_mario_victory(int16_t* buffer, size_t buffer_size, int sample_rate);
void speaker_pokemon(int16_t* buffer, size_t buffer_size, int sample_rate);
void speaker_mario_coin(int16_t* buffer, size_t buffer_size, int sample_rate);

#endif
//////////////////////////////////////////////////////////////////////////
