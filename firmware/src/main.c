/* spoof.c - Some software that pretends to be a tape deck for a Mazda head
             unit. It also interprets button presses on the head unit and can
             transmit them to an iPod.

    This code was written for an atmel ATtiny45.
*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h> 
#include <util/delay.h>

#include "commands.h"
#include "ipod.h"

// Pin definitions
#define INPUT_PIN       PB4 // pin that's connected to the radio's data bus
#define OUTPUT_PIN      PB3 // pin that controls a transistor which grounds the bus
#define POWER_SENSE PB2 // pin that's connected to ACC

// The receive buffer for commands from the HU
volatile uint8_t buffer[buffer_size];
volatile uint8_t buffer_pos = 0;
volatile uint8_t byte_pos = 0;

// A counter for to tell how many times the timer has ovrerflowed
volatile uint8_t command_delay = 0;

// A buffer to keep track of the previous playback control command
volatile uint8_t control_buffer = 0;


#define _PR pgm_read_byte
// Send a message on the Mazda radio bus
void send_message(radio_command *message)
{
    for (uint8_t nibble = 0; nibble < _PR(&message->length); nibble++){
        uint8_t n_message = _PR(&message->command[nibble/2]) >> 4*((nibble+1)%2);
        
        for (int8_t i = 3; i >= 0 ; i--){
            PORTB |= _BV(OUTPUT_PIN); // Pull the bus down
            if (n_message & _BV(i)) _delay_us(1700);
            else                    _delay_us(500);
            
            PORTB &= ~_BV(OUTPUT_PIN); // Release the bus
            if (n_message & _BV(i)) _delay_us(1300);
            else                    _delay_us(2500);
        }
    }
}

void parse_radio_message()
{
    // Anyone home?
    if (buffer[0] == 0x08){
        send_message(&TAPE_power_on);
        _delay_ms(8);
        send_message(&TAPE_cassette_present);
    }
    
    // Wake up!
    if (buffer[0] == 0x09){
        send_message(&TAPE_cassette_present);
        _delay_ms(10);
        send_message(&TAPE_stopped);
    }
    
    // Control command
    if (buffer[0] == 0x01){
        
        // Extract the specific subcommand and command
        uint8_t subcommand = buffer[1] >> 4;
        uint8_t command = (buffer[1] << 4) | (buffer[2] >> 4);
        
        // Playback control
        if (subcommand == 0x1){
            uint8_t command_diff = command ^ control_buffer;
            control_buffer = command;
            
            if (command_diff & PLAY)
                on_play_changed( command );
            
            if (command_diff & FASTFORWARD)
                on_fastforward_changed( command );
            
            if (command_diff & REWIND)
                on_rewind_changed( command );
            
            if (command_diff & STOP)
                on_stop_changed( command );
        }
        
        // Set configuration data
        if (subcommand == 0x4){
            if (command == 0)   
                on_repeat_changed( command );
            
            if (command & REPEAT)
                on_repeat_changed( command );
            
            if (command & RANDOM)
                on_random_pressed();
            
            if (command & SEEK_UP)
                on_seek_up_pressed();
            
            if (command & SEEK_DOWN)
                on_seek_down_pressed();
        }
    }
}

int main()
{
    // 8 bit timer (prescaler = 64)
    TCCR0A = _BV(WGM01);
    TCCR0B = _BV(CS01) | _BV(CS00);
    OCR0A = 117; // Trigger an interrupt every ~7.5ms
    TIMSK = _BV(OCIE0A);    
    
    // Interrupt on pin change (both types)
    GIMSK = _BV(PCIE) | _BV(INT0);
    PCMSK = _BV(PCINT4); // We want pin 3 (PB4)
    MCUCR = _BV(ISC00); // Trigger INT0 on any change
    
    // Set up the output pin
    DDRB = _BV(OUTPUT_PIN);
    
    // Set the ipod output pin
    ipod_enable_port();
    
    // Enable interrupts
    sei();
    
    // Do nothing, everything is handled by interrupts
    while(1);
    return 0;
}

// Pin change interrupt
ISR(PCINT0_vect)
{
    uint8_t elapsed_time = TCNT0;  // Store the timer value
    uint8_t pin = PINB & _BV(INPUT_PIN); // Store the pin value
    
    // If the bus is pulled to ground, a bit is starting to be transmitted
    // so the timer is reset and we can return while we wait for the 
    // bus to be pulled back to 5V.
    if (!pin){
        TCNT0 = 0;
        return;
    }
    
    // A logical 1 is when the bus is held low for ~1.7ms so we accept
    // anything from ~1.4ms to ~2ms as a logical 1.
    if (elapsed_time > 22 && elapsed_time < 32)
        buffer[buffer_pos] |= 1<<(7-byte_pos);
    
    // Anything else is assumed to be a 0, increase the byte position
    byte_pos++;
    
    // We've filled up a byte, move to the next one
    if (byte_pos > 7){
        byte_pos = 0;
        buffer_pos++;
        buffer[buffer_pos] = 0; // very important, clear the old value
    }
}

// Timer compare match interrupt
ISR(TIMER0_COMPA_vect)
{
    if (!buffer_pos){
        // clean up any bits that have been stored because of noise
        // (very useful on startup)
        byte_pos = 0;
        buffer[0] = 0;
        return;
    }
    
    // Wait one overflow to ensure that the bus is quiet
    if (command_delay == 0){
        command_delay++;
        return;
    }
    
    // The message is intended for the tape deck
    if ((buffer[0] & 0xf0) == 0x00)
        parse_radio_message();
    
    buffer_pos = 0;
    byte_pos = 0;
    buffer[0] = 0;
    command_delay = 0;
}

// Pin change interrupt to detect when accessory power has been cut
ISR(INT0_vect)
{
    if (PINB & _BV(PB2)){ // power is on
        ipod_enable_port();
        first_play = 1;
    } else {
        ipod_off();
        ipod_disable_port(); // save some power
    }
}

