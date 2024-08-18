#include "pico/stdlib.h"
#include <stdlib.h>

#include "rom.h"

// Pin Definitions.
#define A0 0
#define A1 1
#define A2 2
#define A3 3
#define A4 4
#define A5 5
#define A6 6
#define A7 7
#define A8 8
#define A9 9
#define A10 10
#define A11 11
#define A12 12
#define A13 13
#define A14 14
#define A15 15

#define D0 16
#define D1 17
#define D2 18
#define D3 19
#define D4 20
#define D5 21
#define D6 22
#define D7 26

#define NWR 27
#define RST 28

#define DATAOFFSETLOW 16
#define DATAOFFSETHIGH 26

#define LED_INT 25

#define RSTMS 10

// Bit masks.
#define ADDRMASK 0b00000000000000001111111111111111
#define DATAMASK 0b00000100011111110000000000000000
#define LOWDATAMASK 0b00000000011111110000000000000000
#define NWRMASK  0b00001000000000000000000000000000
#define RSTMASK  0b00010000000000000000000000000000
#define A15MASK  0b00000000000000001000000000000000

#define DATAMASKLOW  0b01111111
#define DATAMASKHIGH 0b10000000

// Bank switch
// 1: Dynamic 32 kB banks (e.g., Puppet Knight)
// 2: Fixed 16 kB bank and dynamic 16 kB bank (e.g., Armour Force)
unsigned int BANKSWITCH = 1;

void initGPIO() {
  // Set all pins to input first.
  gpio_init_mask( ADDRMASK | DATAMASK | NWRMASK );
  gpio_set_dir_in_masked( ADDRMASK | DATAMASK | NWRMASK );
  
  // Except the LED.
  gpio_init( LED_INT );
  gpio_set_dir( LED_INT, GPIO_OUT );
  
  // And reset.
  gpio_init( RST );
  gpio_set_dir( RST, GPIO_OUT );
}

void main() {
  // Set higher freq.
  set_sys_clock_khz(250000, true);
  
  // Init GPIO.
  initGPIO();
  
  // Turn on LED.
  gpio_put( LED_INT, 1 );
  
  // Reset.
  gpio_put( RST, 1 );
  sleep_ms( RSTMS );
  gpio_put( RST, 0 );
  
  // Initial bank.
  uint8_t* rombank = rom;
  
  // Start endless loop.
  while( 1 ) {
    uint32_t data = gpio_get_all();
    uint32_t addr = data & ADDRMASK;
    uint32_t wr = !( data & NWRMASK );
    uint32_t a15 = ( data & A15MASK );
    
    if ( !a15 && !wr ) {
      // Data output.
      gpio_set_dir_out_masked( DATAMASK );
      
      // Get data byte.
      uint8_t rombyte;
      if ( BANKSWITCH == 1 ) {
        rombyte = rombank[ addr ];
        
      } else if ( BANKSWITCH == 2 ) {
        // Check if lower part
        if ( addr < 16384 ) {
          rombyte = rom[ addr ];
        } else {
          rombyte = rombank[ addr ];
        }
      }
      
      uint32_t gpiobyte = 0;
      gpiobyte |= ( ( rombyte & DATAMASKLOW ) >> 0 ) << DATAOFFSETLOW;
      gpiobyte |= ( ( rombyte & DATAMASKHIGH ) >> 7 ) << DATAOFFSETHIGH;
      
      // And put it out.
      gpio_put_masked( DATAMASK, gpiobyte );
      
    } else {
      // Data input.
      gpio_set_dir_in_masked( DATAMASK );
    }
    
    if ( wr ) {
      // Bank change?
      if ( addr == 0xb000 ) {
        // Bank switch type 1
        BANKSWITCH = 1;
        
        uint32_t writeData = ( data & LOWDATAMASK ) >> DATAOFFSETLOW;
        if ( writeData == 0 ) {
          rombank = rom;
        } else if ( writeData == 1 ) {
          rombank = rom + 32768;
        }
      }
        
      else if ( addr == 0x0001 ) {
        // Bank switch type 2
        BANKSWITCH = 2;
        
        uint32_t writeData = (( data & LOWDATAMASK ) >> ( DATAOFFSETLOW )) & 0b1111;
        
        // Use a case structure instead of a dynamic multiplcation
        // to hopefully speed up things.
        uint32_t offset = 0;
        switch ( writeData ) {
          case 0:
          case 1:
            offset = 0;
            break;
          case 2:
            offset = 1 * 16384;
            break;
          case 3:
            offset = 2 * 16384;
            break;
          case 4:
            offset = 3 * 16384;
            break;
          case 5:
            offset = 5 * 16384;
            break;
          case 6:
            offset = 5 * 16384;
            break;
          case 7:
            offset = 6 * 16384;
            break;
          case 8:
            offset = 7 * 16384;
            break;
          case 9:
            offset = 8 * 16384;
            break;
          case 10:
            offset = 9 * 16384;
            break;
          case 11:
            offset = 10 * 16384;
            break;
          case 12:
            offset = 11 * 16384;
            break;
          case 13:
            offset = 12 * 16384;
            break;
          case 14:
            offset = 13 * 16384;
            break;

          // Something went wrong.
          default:
            offset = 42;
            break;
        }
        
        if ( offset != 42 ) {
         rombank = rom + offset; 
        }
      }
    }
  }
}
