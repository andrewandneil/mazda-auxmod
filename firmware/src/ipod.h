
#ifndef __IPOD_H__
#define __IPOD_H__


#define IPOD_S_DELAY         100
#define IPOD_STX_PORT        PORTB
#define IPOD_STX_PORT_DIR    DDRB
#define IPOD_STX_BIT         PB1

void ipod_enable_port();
void ipod_disable_port();
void ipod_play();
void ipod_pause();
void ipod_off();
void ipod_on();
void ipod_shuffle();
void ipod_repeat();
void ipod_next();
void ipod_prev();
void ipod_seek_forward();
void ipod_seek_backward();
void ipod_seek_stop();


#endif // __IPOD_H__
