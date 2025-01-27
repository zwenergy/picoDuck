// Reset pulse after loading?
// (not recommended for the MD laptop)
#define RSTPULSE

// CRC check len
#define CRCLEN 65536

#include "pico/stdlib.h"
#include <stdlib.h>

#include "rom_menu.h"
#include "roms.h"

// Include string.h for memcpy
#include <string.h>

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

#define RSTMS 100

// Bit masks.
#define ADDRMASK 0b00000000000000001111111111111111
#define DATAMASK 0b00000100011111110000000000000000
#define LOWDATAMASK 0b00000000011111110000000000000000
#define NWRMASK  0b00001000000000000000000000000000
#define RSTMASK  0b00010000000000000000000000000000
#define A15MASK  0b00000000000000001000000000000000

#define DATAMASKLOW  0b01111111
#define DATAMASKHIGH 0b10000000

// Reset function.
void doReset() {
  gpio_set_dir( RST, GPIO_OUT );
  gpio_put( RST, 1 );
  sleep_ms( RSTMS );
  gpio_put( RST, 0 );
  
  gpio_pull_down( RST );
  
  // Set RST pin back to INPUT, so we can use GPIO_PUT as non-masked.
  gpio_set_dir( RST, GPIO_IN );
}


void __not_in_flash_func( initGPIO() ) {
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

// CRC32
uint32_t crc32(uint8_t* data, uint32_t length ) {  
  uint32_t byte, crc, mask;
  uint32_t cnt = 0;

  crc = 0xFFFFFFFF;
  while ( cnt < length ) {
    byte = data[ cnt ];
    crc = crc ^ byte;
    for ( int32_t j = 7; j >= 0; j-- ) {
      mask = -( crc & 1 );
      crc = ( crc >> 1 ) ^ ( 0xEDB88320 & mask );
    }
    cnt++;
  }
  
  return ~crc;
}

void __not_in_flash_func( handleMD1() ) {
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
      rombyte = rombank[ addr ];
      
      uint32_t gpiobyte = 0;
      gpiobyte |= ( ( rombyte & DATAMASKLOW ) >> 0 ) << DATAOFFSETLOW;
      gpiobyte |= ( ( rombyte & DATAMASKHIGH ) >> 7 ) << DATAOFFSETHIGH;
      
      // And put it out.
      gpio_put_all( gpiobyte );
      
      continue;
      
    } else {
      // Data input.
      gpio_set_dir_in_masked( DATAMASK );
    }
    
    if ( wr ) {
      uint32_t writeData;
      // Bank change?
      if ( addr == 0xB000 ) {
            
        writeData = ( data & LOWDATAMASK ) >> DATAOFFSETLOW;
        if ( writeData == 0 ) {
          rombank = rom;
        } else if ( writeData == 1 ) {
          rombank = rom + 32768;
        }
      }
    }
  }
}

void __not_in_flash_func( handleMD2() ) {
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
      // Check if lower part
      if ( addr < 16384 ) {
        rombyte = rom[ addr ];
      } else {
        rombyte = rombank[ addr ];
      }
      
      uint32_t gpiobyte = 0;
      gpiobyte |= ( ( rombyte & DATAMASKLOW ) >> 0 ) << DATAOFFSETLOW;
      gpiobyte |= ( ( rombyte & DATAMASKHIGH ) >> 7 ) << DATAOFFSETHIGH;
      
      // And put it out.
      gpio_put_all( gpiobyte );
      
      continue;
      
    } else {
      // Data input.
      gpio_set_dir_in_masked( DATAMASK );
    }
    
    if ( wr ) {
      uint32_t writeData;
      // Bank change?
      if ( addr == 0x001 ) {
        // Type 2
        writeData = ( ( data & LOWDATAMASK ) >> DATAOFFSETLOW )& 0b111;
        
        if ( writeData ) {
          rombank = rom + ( writeData - 1 ) * 16384;
        } else {
          rombank = rom; 
        }
        
        continue;
        
      }
    }
  }
}


void __not_in_flash_func( handleMenu() ) {
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
      rombyte = rombank[ addr ];
      
      uint32_t gpiobyte = 0;
      gpiobyte |= ( ( rombyte & DATAMASKLOW ) >> 0 ) << DATAOFFSETLOW;
      gpiobyte |= ( ( rombyte & DATAMASKHIGH ) >> 7 ) << DATAOFFSETHIGH;
      
      // And put it out.
      gpio_put_all( gpiobyte );
      
      continue;
      
    } else {
      // Data input.
      gpio_set_dir_in_masked( DATAMASK );
    }
    
    if ( wr ) {
      uint32_t writeData;
      // Bank change?
      if ( addr == 0x002 ) {
        // Read again.
        data = gpio_get_all();
        // Menu item choose. We load the new ROM into RAM.
        writeData = ( ( data & LOWDATAMASK ) >> DATAOFFSETLOW ) - 1;
        
        memcpy( rom, romStorage + ( writeData * ROMENTRYSIZE ), ROMENTRYSIZE );
        
        // Calc. CRC
        uint32_t crc = crc32( rom, CRCLEN );
        
        // MD1 ROM?
        if ( crc == 0x35FB1616 || // Puppet Knight
             crc == 0xCD2730AC ) { // Suleiman's Treasure
          doReset();
          handleMD1();
          
        } else {
          // All other known ROMs are no banking or MD2 banking.
          doReset();
          handleMD2();
        }
      }
    }
  }
}

void main() {
  // Set higher freq.
  set_sys_clock_khz(250000, true);
  
  // Init GPIO.
  initGPIO();
  
  // Turn on LED.
  gpio_put( LED_INT, 1 );
  
  #ifdef RSTPULSE
  // Reset.
  doReset();
  #endif
  
  handleMenu();
}
