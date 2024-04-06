
#ifndef __COMMANDS_H__
#define __COMMANDS_H__

#include <stdint.h>
#include <avr/pgmspace.h> 

#define buffer_size 16

typedef struct {
    uint8_t command[buffer_size];
    uint8_t length;
} PROGMEM const radio_command;

extern void send_message(radio_command*);
extern volatile uint8_t first_play;
extern volatile uint8_t control_buffer;
extern volatile uint8_t ipod_seeking;

extern radio_command TAPE_power_on;
extern radio_command TAPE_cassette_present;
extern radio_command TAPE_stopped;
extern radio_command TAPE_playing;
extern radio_command TAPE_playback;
extern radio_command TAPE_random_playback;
extern radio_command TAPE_repeat_playback;
extern radio_command TAPE_seeking;
extern radio_command TAPE_fast_rewind;
extern radio_command TAPE_fast_fastforward;

#define PLAY        0x01
#define FASTFORWARD 0x04
#define REWIND      0x08
#define STOP        0x60
#define REPEAT      0x01
#define RANDOM      0x02
#define SEEK_UP     0x10
#define SEEK_DOWN   0x20

void on_play_changed( uint8_t );
void on_fastforward_changed( uint8_t );
void on_rewind_changed( uint8_t );
void on_stop_changed( uint8_t );
void on_repeat_changed( uint8_t );
void on_random_pressed();
void on_seek_up_pressed();
void on_seek_down_pressed();

#endif // __COMMANDS_H__
