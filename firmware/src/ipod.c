
#include <avr/io.h>         // IO port definitions
#include <stdint.h>         // Typedefs
#include <util/delay.h>     // Delay functions
#include <avr/pgmspace.h>   // tools for reading directly off flash

#include "ipod.h"

volatile uint8_t ipod_seeking = 0;

typedef struct {
    uint8_t mode;
    uint8_t command_h;
    uint8_t command_l;
    uint8_t parameters[4];
    uint8_t param_len;
    uint8_t checksum;
} PROGMEM const ipod_command;


// See: http://ipodlinux.org/wiki/Apple_Accessory_Protocol
// checksum: 0x100 - ((0x03 + mode + command_l + command_h
//                     + parameters + param_len) & 0xff)
ipod_command IPOD_REMOTE_MODE       = { 0x00, 0x01, 0x02, {}, 0, 0xfa };
ipod_command IPOD_BUTTON_RELEASED   = { 0x02, 0x00, 0x00, {}, 0, 0xfb };
ipod_command IPOD_PLAY_PAUSE        = { 0x02, 0x00, 0x01, {}, 0, 0xfa };
ipod_command IPOD_PLAY          = { 0x02, 0x00, 0x00, {0x01}, 1, 0xf9 };
ipod_command IPOD_PAUSE         = { 0x02, 0x00, 0x00, {0x02}, 1, 0xf8 };
ipod_command IPOD_STOP              = { 0x02, 0x00, 0x80, {}, 0, 0x7b };
ipod_command IPOD_NEXT              = { 0x02, 0x00, 0x08, {}, 0, 0xf3 };
ipod_command IPOD_PREV              = { 0x02, 0x00, 0x10, {}, 0, 0xeb };
ipod_command IPOD_ON      = { 0x02, 0x00, 0x00, {0x00, 0x08}, 2, 0xf1 };
ipod_command IPOD_OFF     = { 0x02, 0x00, 0x00, {0x00, 0x04}, 2, 0xf5 };
ipod_command IPOD_MUTE          = { 0x02, 0x00, 0x00, {0x04}, 1, 0xf6 };
ipod_command IPOD_SHUFFLE       = { 0x02, 0x00, 0x00, {0x80}, 1, 0x7a };
ipod_command IPOD_REPEAT  = { 0x02, 0x00, 0x00, {0x00, 0x01}, 2, 0xf8 };


void ipod_enable_port(){
    IPOD_STX_PORT_DIR |= _BV(IPOD_STX_BIT);
    IPOD_STX_PORT     |= _BV(IPOD_STX_BIT);
}

void ipod_disable_port(){
    IPOD_STX_PORT_DIR &= ~_BV(IPOD_STX_BIT);
    IPOD_STX_PORT     &= ~_BV(IPOD_STX_BIT);
}

void send_byte( uint8_t c )
{
    c = ~c; 
    IPOD_STX_PORT &= ~_BV(IPOD_STX_BIT);            // start bit 
    for( uint8_t i = 10; i; i-- ){                  // 10 bits 
        _delay_us( IPOD_S_DELAY );                  // bit duration 
        if( c & 1 ) 
            IPOD_STX_PORT &= ~_BV(IPOD_STX_BIT);    // data bit 0 
        else 
            IPOD_STX_PORT |= _BV(IPOD_STX_BIT);     // data bit 1 or stop bit 
        c >>= 1; 
    }
    _delay_us(30); // Wait at least 10 us between bytes
}

void _ipod_send_command( const ipod_command *command )
{
    // Send the header
    send_byte( 0xff );
    send_byte( 0x55 );
    
    // Total length
    send_byte( 0x03 + pgm_read_byte(&command->param_len) );
    
    // Mode byte
    send_byte( pgm_read_byte(&command->mode) );
    
    // Command word
    send_byte( pgm_read_byte(&command->command_h) );
    send_byte( pgm_read_byte(&command->command_l) );
    
    // Send parameters
    uint8_t i = 0;
    while ( i < pgm_read_byte(&command->param_len) ){
        send_byte( pgm_read_byte(&command->parameters[i]) );
        i++;
    }
    
    // Send the checksum
    send_byte( pgm_read_byte(&command->checksum) );
}

void ipod_send_command( const ipod_command *command )
{
    if (ipod_seeking)
        ipod_seek_stop();
        
    _ipod_send_command( &IPOD_REMOTE_MODE );
    for (uint8_t i=0; i<3; i++){
        _ipod_send_command( command );
        _delay_ms(10);
    }
    _ipod_send_command( &IPOD_BUTTON_RELEASED );
}

void ipod_play()
{
    ipod_send_command(&IPOD_PLAY);
}

void ipod_pause()
{
    ipod_send_command(&IPOD_PAUSE);
}

void ipod_off()
{
    ipod_send_command(&IPOD_OFF);
}

void ipod_on()
{
    ipod_send_command(&IPOD_ON);
}

void ipod_shuffle()
{
    ipod_send_command(&IPOD_SHUFFLE);
}

void ipod_repeat()
{
    ipod_send_command(&IPOD_REPEAT);
}

void ipod_next()
{
    ipod_send_command(&IPOD_NEXT);
}

void ipod_prev()
{
    ipod_send_command(&IPOD_PREV);
}

// By sending the iPod the next or previous command and not sending the button
// released command it will start seeking on it's own. Then when the button
// released command is sent it will stop seeking.
void ipod_seek(const ipod_command *command )
{
    _ipod_send_command(&IPOD_REMOTE_MODE);
    _ipod_send_command(command);
    _delay_ms(10);
    _ipod_send_command(command);
    ipod_seeking = 1;
}

void ipod_seek_forward()
{
    ipod_seek(&IPOD_NEXT);
}

void ipod_seek_backward()
{
    ipod_seek(&IPOD_PREV);
}

void ipod_seek_stop()
{
    _ipod_send_command( &IPOD_BUTTON_RELEASED );
    _delay_ms(5);
    _ipod_send_command( &IPOD_BUTTON_RELEASED );
    ipod_seeking = 0;
}


