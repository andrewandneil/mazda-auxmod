
#include <util/delay.h>
#include "commands.h"
#include "ipod.h"

// Commands that are useful to us
// See: http://nikosapi.org/wiki/index.php/Mazda_Radio#Messages_Sent_by_the_Tape_Deck
radio_command TAPE_power_on         = {{0x88, 0x12}, 4};
radio_command TAPE_cassette_present = {{0x8B, 0x90, 0x40, 0x0C, 0x30}, 9};
radio_command TAPE_stopped          = {{0x89, 0x0C, 0xE0}, 5};
radio_command TAPE_playing          = {{0x89, 0x41, 0x50}, 5};
radio_command TAPE_playback         = {{0x8B, 0x90, 0x40, 0x01, 0x00}, 9};
radio_command TAPE_random_playback  = {{0x8B, 0x90, 0x60, 0x01, 0xE0}, 9};
radio_command TAPE_repeat_playback  = {{0x8B, 0x90, 0x50, 0x01, 0xF0}, 9};
radio_command TAPE_seeking          = {{0x89, 0x51, 0x60}, 5};
radio_command TAPE_fast_rewind      = {{0x8B, 0x93, 0x40, 0x11, 0xE0}, 9};
radio_command TAPE_fast_fastforward = {{0x8B, 0x92, 0x40, 0x11, 0xD0}, 9};

volatile uint8_t random_mode = 0; // Has the random button been pressed?
volatile uint8_t first_play = 1;  // Has the radio just been powered on?

// The default status message that's sent when the tape deck is playing
// a tape normally. Usful to get the radio back to a known state after
// certain buttons are pressed (like FF and REW)
void return_to_normal_mode()
{
    send_message(&TAPE_playing);
    _delay_ms(7);
    send_message(&TAPE_playback);
    control_buffer &= ~PLAY;
    random_mode = 0;
}

void on_play_changed( uint8_t command ){
    if (command & PLAY){
        
        // This hack solves an annoying problem where the radio will cut off
        // the tape deck input for about 2 seconds on startup. Here we simply
        // imitate what the tape deck does and it seems to work quite well.
        if (first_play){
            send_message(&TAPE_playing);
            first_play = 0;
            _delay_ms(900);
        }
        
        return_to_normal_mode();
        ipod_play();
    }
}

void on_fastforward_changed( uint8_t command ){
    if (command & FASTFORWARD){
        if (ipod_seeking)
            ipod_seek_stop();
        else
            ipod_seek_forward();
        
        return_to_normal_mode();
        control_buffer &= ~FASTFORWARD;
    }
}

void on_rewind_changed( uint8_t command ){
    if (command & REWIND){
        if (ipod_seeking)
            ipod_seek_stop();
        else
            ipod_seek_backward();
        
        return_to_normal_mode();
        control_buffer &= ~REWIND;
    }
}

void on_stop_changed( uint8_t command ){
    if (command & STOP){
        send_message(&TAPE_stopped);
        ipod_pause();
    }
}

void on_repeat_changed( uint8_t command ){
    if (command & REPEAT){
        // A hack to prevent the radio from freaking out, once again, we're
        // simply imitating the tape deck
        send_message(&TAPE_repeat_playback);
        _delay_ms(8);
        send_message(&TAPE_repeat_playback);
        _delay_ms(8);
        send_message(&TAPE_playing);
        random_mode = 0;
    } else {
        return_to_normal_mode();
    }
    
    ipod_repeat();
}

void on_random_pressed(){
    if (random_mode)
        send_message(&TAPE_playback);
    else
        send_message(&TAPE_random_playback);
    
    random_mode = ~random_mode;
    ipod_shuffle();
}

// This brings the radio back into a known state after one of the seek
// buttons is pressed
void fast_seek_response(radio_command *seek_command)
{
    send_message(&TAPE_seeking);
    _delay_ms(8);
    send_message(seek_command);
    _delay_ms(8);
    send_message(&TAPE_seeking);
    _delay_ms(8);
    return_to_normal_mode();
}

void on_seek_up_pressed(){
    ipod_next();
    fast_seek_response(&TAPE_fast_fastforward);
}

void on_seek_down_pressed(){
    ipod_prev();
    fast_seek_response(&TAPE_fast_rewind);
}

