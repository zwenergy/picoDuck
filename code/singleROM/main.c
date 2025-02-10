#include "pico/stdlib.h"
#include <stdlib.h>
#include "hardware/vreg.h"

// Re-read the input pins when switching from output to input direction.
// (otherwise the actual data might be wrong).
#define DATA_RE_READ

// Enable SRAM banking.
#define SRAMBANKING

// Enable some delay before "activating" cart activities.
//#define ENABLE_BOOT_DELAY

// When enabling FLASHWRITE, the SRAM content will be saved to the Flash
// after an adjustable amount of time (REWRITEWAIT_MS).
// This only works when reading the ROM from RAM, not from Flash
// (the array has to be non-const).
#define FLASHWRITE

// Set the clock to 300 MHz and also increase the voltage.
#define OVERCLOCKMAX

// Sizes.
#define SRAMSIZE ( 8192 * 4 )

#ifdef FLASHWRITE
#include "pico/multicore.h"
#include "hardware/flash.h"
#include "hardware/structs/bus_ctrl.h"

// Address in the Flash for the SRAM.
#define FLASHADDR ( PICO_FLASH_SIZE_BYTES - SRAMSIZE )

// Waiting time after a write to the "SRAM" before the Flash is rewritten.
#define REWRITEWAIT_MS 5000

// Idle time between checks of the listener.
#define REWRITECHECKINT_MS 500

#endif

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

#define RSTMS 50
#define BOOTDELAYMS 5000

// Bit masks.
#define ADDRMASK 0b00000000000000001111111111111111
#define DATAMASK 0b00000100011111110000000000000000
#define LOWDATAMASK 0b00000000011111110000000000000000
#define HIDATAMASK  0b00000100000000000000000000000000
#define NWRMASK  0b00001000000000000000000000000000
#define RSTMASK  0b00010000000000000000000000000000
#define A15MASK  0b00000000000000001000000000000000

#define DATABIT7 0b10000000

#define SRAM_RANGE_MASK          0b00000000000000000001111111111111
#define ROM_BANKMASK_UNSHIFTED   0b00000000000011110000000000000000
#define RAM_BANKMASK_UNSHIFTED   0b00000000111100000000000000000000


#define DATAMASKLOW  0b01111111
#define DATAMASKHIGH 0b10000000

// Addresses.
#define MD0BANK 0x1000

uint8_t sram[ SRAMSIZE ];

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

#ifdef FLASHWRITE
volatile uint32_t rewriteFlash = 0;

void readSRAMfromFlash() {
  // Offset addr after the RAM (XIP_BASE).
  uint8_t* addr = (uint8_t*) ( XIP_BASE + FLASHADDR );
  
  // Go over byte-wise.
  // TODO: This is stupid, just read out 32b chunks.
  for ( int i = 0; i < SRAMSIZE; ++i ) {
    sram[ i ] = *addr;
    ++addr;
  }
}

void __not_in_flash_func( writeSRAMToFlash( ) ) {
  // Not interrupt-safe.
  uint32_t ints = save_and_disable_interrupts();
  
  uint32_t curAddr = FLASHADDR;

  // Bytes to be erased have to be a multiple of the sector size.
  // The SRAM is 32768 bytes large.
  // A flash sector is 4096 bytes.
  // So it's naturally a multiple.
  flash_range_erase( curAddr, SRAMSIZE );

  // And write.
  // Bytes to be erased have to be a multiple of the page size.
  // The SRAM is 32768 bytes large.
  // A flash page size is 256 bytes.
  // So it's naturally a multiple.
  flash_range_program( curAddr, sram, SRAMSIZE );
  
  // Restore interrupts.
  restore_interrupts ( ints );
}

void __not_in_flash_func( rewriteFlashListener() ) {
  // This function basically just
  // waits for the Flash to be rewritten.
  // In order to avoid a lot of re-writes, we first wait a bit to make sure,
  // all SRAM writing is finished.
  
  while ( 1 ) {
    // Check if we should rewrite.
    if ( rewriteFlash ) {
      
      while ( 1 ) {
        // Toggle the flag.
        rewriteFlash = 0;
        
        //gpio_put( LED_INT, 1 );
        
        // Wait a bit.
        sleep_ms( REWRITEWAIT_MS );
        
        // Did no new write happen?
        if ( !rewriteFlash ) {
          writeSRAMToFlash();
          rewriteFlash = 0;
          
          //gpio_put( LED_INT, 0 );
          break;
        }
      }
    }
    
    sleep_ms( REWRITECHECKINT_MS );
  }
}
#endif

void __not_in_flash_func( handleROM() ) {
  
  // Initial bank.
  uint8_t* rombank = rom;
  uint8_t* srambank = sram;
  
  // Start endless loop.
  while( 1 ) {
    uint32_t data = gpio_get_all();
    uint32_t addr = data & ADDRMASK;
    
    // Build an index for a jump table.
    uint32_t n_wr = ( data & NWRMASK ) >> ( NWR - 3 );
    uint32_t sel = ( addr >> 13 ) | n_wr;
    
    // Declarations before switch statement.
    
    switch ( sel ) {
      case 0b0000:
      case 0b0001:
      case 0b0010:
      case 0b0011:
      {
        // Write in ROM area (potentially banking).
        gpio_set_dir_in_masked( DATAMASK );
        
        // Potentially re-read the data.
#ifdef DATA_RE_READ
        data = gpio_get_all();
#endif
        
        if ( addr == MD0BANK ) {
          uint32_t writeData =  ( data & ROM_BANKMASK_UNSHIFTED ) >> DATAOFFSETLOW;
          
          rombank = rom + writeData * 32768;
          
#ifdef SRAMBANKING
          writeData = ( data & RAM_BANKMASK_UNSHIFTED ) >> ( DATAOFFSETLOW + 4 );
          srambank = sram + writeData * 8192;
#endif
        }
        
        break;
      }
      
      case 0b1000:
      case 0b1001:
      case 0b1010:
      case 0b1011:
      {
        // Regular ROM read.
        gpio_set_dir_out_masked( DATAMASK );
        uint32_t rombyte = rombank[ addr ];
        
        uint32_t gpiobyte = 0;
        gpiobyte |= ( ( rombyte & DATAMASKLOW ) >> 0 ) << DATAOFFSETLOW;
        gpiobyte |= ( ( rombyte & DATAMASKHIGH ) >> 7 ) << DATAOFFSETHIGH;
        
        // And put it out.
        gpio_put_all( gpiobyte );
        
        break;
      }        
        
      case 0b0101:
      {
        // Write to SRAM.
        gpio_set_dir_in_masked( DATAMASK );
        
        // Potentially re-read the data.
#ifdef DATA_RE_READ
        data = gpio_get_all();
#endif
        
        uint32_t sramAddr = ( data & SRAM_RANGE_MASK );
        uint32_t writeData = ( ( data & LOWDATAMASK ) >> DATAOFFSETLOW );
        
        if ( data & HIDATAMASK ) {
          srambank[ sramAddr ] = writeData | DATABIT7;
        } else {
          srambank[ sramAddr ] = writeData;
        }
        
        //writeData = ( data & HIDATAMASK ? ( writeData | DATABIT7 ) : writeData );
        
#ifdef FLASHWRITE
        rewriteFlash = 1;
#endif
        
        break;
      }
        
      case 0b1101:
      {
        // Read from RAM.
        uint32_t sramAddr = ( data & SRAM_RANGE_MASK );
        
        uint32_t rambyte = srambank[ sramAddr ];
        
        uint32_t gpiobyte = 0;
        gpiobyte |= ( ( rambyte & DATAMASKLOW ) >> 0 ) << DATAOFFSETLOW;
        gpiobyte |= ( ( rambyte & DATAMASKHIGH ) >> 7 ) << DATAOFFSETHIGH;
        
        // And put it out.
        gpio_put_all( gpiobyte );
        gpio_set_dir_out_masked( DATAMASK );
        
        break;
      }
        
      case 0b0100: // Write but not ROM or RAM
      case 0b0110: // Write but not ROM or RAM
      case 0b0111: // Write but not ROM or RAM
      case 0b1100: // Read but not ROM or RAM
      case 0b1110: // Read but not ROM or RAM
      case 0b1111: // Read but not ROM or RAM
      default:
        gpio_set_dir_in_masked( DATAMASK );        
    }
  }
}

int main() {
#ifdef FLASHWRITE
  readSRAMfromFlash();
  
  // Set priority of this core (0) lower.
  bus_ctrl_hw->priority |= BUSCTRL_BUS_PRIORITY_PROC1_BITS;
#endif
  
  // Set higher freq.
#ifndef OVERCLOCKMAX
  set_sys_clock_khz(250000, true);
#else
  vreg_set_voltage(VREG_VOLTAGE_1_20);
  sleep_ms(1000);
  set_sys_clock_khz(300000, true);
#endif
  
  // Init GPIO.
  initGPIO();
  
#ifndef FLASHWRITE
  // Turn on LED.
  gpio_put( LED_INT, 1 );
#endif
  
#ifdef ENABLE_BOOT_DELAY
  sleep_ms( BOOTDELAYMS );
#endif
  
  // Reset.
  gpio_put( RST, 1 );
  sleep_ms( RSTMS );
  gpio_put( RST, 0 );
  
  // Set RST pin back to INPUT, so we can use GPIO_PUT as non-masked.
  gpio_set_dir( RST, GPIO_IN );
  gpio_pull_down( RST );
  
#ifdef FLASHWRITE
  multicore_launch_core1( handleROM );
  rewriteFlashListener();
#else
  handleROM();
#endif
  
  return 0;
}
