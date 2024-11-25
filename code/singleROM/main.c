#include "pico/stdlib.h"
#include <stdlib.h>
#include "hardware/clocks.h"

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

#define RSTMS 100

// Bit masks.
#define ADDRMASK    0b00000000000000001111111111111111
#define DATAMASK    0b00000100011111110000000000000000
//                                    FEDCBA9876543210
//                    FEDCBA9876543210
#define LOWDATAMASK 0b00000000011111110000000000000000
#define NWRMASK     0b00001000000000000000000000000000
#define RSTMASK     0b00010000000000000000000000000000
#define A15MASK     0b00000000000000001000000000000000

//                                    FEDCBA9876543210
#define ADDRBANKED  0b00000000000000000100000000000000 // 0x4000 (memory region 0x4000 -> 0x7FFF)

//                     76543210
#define DATAMASKLOW  0b01111111
#define DATAMASKHIGH 0b10000000
#define BANKMASK     0b00111111  // Supports banks 0-63


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


#define ROM_BANK(N) (rom + ((N-1) * 0x4000u)) // The reason for N-1 is because rombank will be accessed with a base of (0x4000) + the bank relative address

// pico pi 2040: 264kB of SRAM, 2MB of flash storage

const unsigned char * p_rombank_offsets[] = {
    ROM_BANK(1),  ROM_BANK(1),  // 32K (Note: Bank 0 select points to Bank 1)
    ROM_BANK(2),  ROM_BANK(3),  // 64K
    ROM_BANK(4),  ROM_BANK(5),  ROM_BANK(6),  ROM_BANK(7), // 128K
    ROM_BANK(8),  ROM_BANK(9),  ROM_BANK(10), ROM_BANK(11),
    ROM_BANK(12), ROM_BANK(13), ROM_BANK(14), ROM_BANK(15), // 256K

    ROM_BANK(16), ROM_BANK(17), ROM_BANK(18), ROM_BANK(19),
    ROM_BANK(20), ROM_BANK(21), ROM_BANK(22), ROM_BANK(23),
    ROM_BANK(24), ROM_BANK(25), ROM_BANK(26), ROM_BANK(27),
    ROM_BANK(28), ROM_BANK(29), ROM_BANK(30), ROM_BANK(31), // 512K

    ROM_BANK(32), ROM_BANK(33), ROM_BANK(34), ROM_BANK(35),
    ROM_BANK(36), ROM_BANK(37), ROM_BANK(38), ROM_BANK(39),
    ROM_BANK(40), ROM_BANK(41), ROM_BANK(42), ROM_BANK(43),
    ROM_BANK(44), ROM_BANK(45), ROM_BANK(46), ROM_BANK(47),

    ROM_BANK(48), ROM_BANK(49), ROM_BANK(50), ROM_BANK(51),
    ROM_BANK(52), ROM_BANK(53), ROM_BANK(54), ROM_BANK(55),
    ROM_BANK(56), ROM_BANK(57), ROM_BANK(58), ROM_BANK(59),
    ROM_BANK(60), ROM_BANK(61), ROM_BANK(62), ROM_BANK(63), // 1MB


};

void __not_in_flash_func( handleROM_MD2() ) {
  // Initial bank, point it to BANK 1
  const uint8_t * rombank = ROM_BANK(1); // rom;
  
  // Start endless loop
  while( 1 ) {
    uint32_t data = gpio_get_all();
    uint32_t addr = data & ADDRMASK;
    uint32_t wr = !( data & NWRMASK );
    uint32_t a15 = ( data & A15MASK );
    
    if ( !a15 && !wr ) {
      // Data output.
      gpio_set_dir_out_masked( DATAMASK );
      
      // Get data byte based on whether it's in upper or lower 16K rom bank region.
      uint8_t rombyte;
      if ( addr & ADDRBANKED ) rombyte = rombank[ addr ];
      else                     rombyte = rom[ addr ];
      
      uint32_t gpiobyte = 0;
      gpiobyte  =   ( rombyte & DATAMASKLOW )         << DATAOFFSETLOW;
      gpiobyte |= ( ( rombyte & DATAMASKHIGH ) >> 7 ) << DATAOFFSETHIGH;
      
      // And put it out.
      gpio_put_all( gpiobyte );
      
    } else {
      // Data input.
      gpio_set_dir_in_masked( DATAMASK );
    }

    if ( wr ) {
      // Handle MD2 style bank switch register write at address 0x0001
      if (addr == 0x0001) {
          rombank = p_rombank_offsets[(data >> DATAOFFSETLOW) & BANKMASK];
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
  
  // Reset.
  gpio_put( RST, 1 );
  sleep_ms( RSTMS );
  gpio_put( RST, 0 );
  
  // Set RST pin back to INPUT, so we can use GPIO_PUT as non-masked.
  gpio_set_dir( RST, GPIO_IN );
  gpio_pull_down( RST );
  
  handleROM_MD2();

}
