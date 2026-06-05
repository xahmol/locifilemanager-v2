// oric.h - Oric Atmos hardware constants and register definitions
// For use with Oscar64 compiler, bare-metal target

#ifndef ORIC_H
#define ORIC_H

#include <stdint.h>
#include <stdbool.h>

// -------------------------------------------------------------------------
// VIA 6522 at $0300
// -------------------------------------------------------------------------

typedef volatile struct {
    uint8_t prb;    // $0300  Port B: bits 0-2=row select, 3=tape/kbd-sense, 4=printer strobe,
                    //                5=cassette motor, 6=AY BC1, 7=AY BDIR
    uint8_t pra;    // $0301  Port A (with handshake): AY data bus / printer data
    uint8_t ddrb;   // $0302  DDR B: ROM sets to $F7 (bit 3 = input, rest output)
    uint8_t ddra;   // $0303  DDR A
    uint8_t t1lo;   // $0304  Timer 1 counter low
    uint8_t t1hi;   // $0305  Timer 1 counter high / start
    uint8_t t1llo;  // $0306  Timer 1 latch low
    uint8_t t1lhi;  // $0307  Timer 1 latch high
    uint8_t t2lo;   // $0308  Timer 2 low
    uint8_t t2hi;   // $0309  Timer 2 high
    uint8_t sr;     // $030A  Shift register
    uint8_t acr;    // $030B  Auxiliary control register
    uint8_t pcr;    // $030C  Peripheral control register (CB2 = AY column drive)
    uint8_t ifr;    // $030D  Interrupt flag register (bit 6 = Timer 1)
    uint8_t ier;    // $030E  Interrupt enable register
    uint8_t pra2;   // $030F  Port A without handshake (use for AY data bus writes)
} VIA_t;

#define VIA (*((VIA_t *)0x0300))

// VIA Port B initial value (ROM sets DDRB = $F7; bit 3 always input)
#define VIA_DDRB_INIT   0xF7

// -------------------------------------------------------------------------
// AY-3-8912 control sequences (via VIA Port B bits 6-7 = BC1/BDIR)
//
// Write sequence for AY register N = value V:
//   1. VIA.pra2 = N;  VIA.prb |= 0xC0;          (BDIR=1, BC1=1 = latch address)
//   2. VIA.prb  = (VIA.prb & 0x3F) | 0x80;      (BDIR=1, BC1=0 = write data)
//      VIA.pra2 = V;
//   3. VIA.prb &= 0x3F;                           (BDIR=0, BC1=0 = inactive)
//
// PCR ($030C) is separately used for CB2 = AY keyboard column drive assertion.
// PCR bits 7-5: 111=CB2 high (deassert), 110=CB2 low (assert)
// -------------------------------------------------------------------------

// AY register numbers
#define AY_REG_MIXER    7    // Mixer control (enable/disable tone+noise per channel)
#define AY_REG_IOA      14   // External Port A (keyboard column drive, active-low)
#define AY_REG_IOB      15   // External Port B (not connected on Oric)

// -------------------------------------------------------------------------
// Screen
// -------------------------------------------------------------------------

#define TEXTVRAM        0xBB80U    // Text screen RAM: 40 columns × 28 rows
#define SCREEN_COLS     40
#define SCREEN_ROWS     28
#define SCREEN_SIZE     (SCREEN_COLS * SCREEN_ROWS)   // 1120 bytes

// -------------------------------------------------------------------------
// Serial attribute codes (write to screen RAM with bit 6 = 0)
// A byte in screen RAM with bit 6 = 0: serial attribute, affects rest of row
// A byte in screen RAM with bit 6 = 1: display character from character ROM
// -------------------------------------------------------------------------

// Foreground (INK) colors — values 0–7
#define A_FWBLACK       0
#define A_FWRED         1
#define A_FWGREEN       2
#define A_FWYELLOW      3
#define A_FWBLUE        4
#define A_FWMAGENTA     5
#define A_FWCYAN        6
#define A_FWWHITE       7

// Background (PAPER) colors — values 16–23
#define A_BGBLACK       16
#define A_BGRED         17
#define A_BGGREEN       18
#define A_BGYELLOW      19
#define A_BGBLUE        20
#define A_BGMAGENTA     21
#define A_BGCYAN        22
#define A_BGWHITE       23

// Character mode switches (write to screen RAM as attributes)
#define A_STD           8    // Standard character set
#define A_ALT           9    // Alternate (mosaic/graphics) character set
#define A_HIRES         28   // Switch to HIRES mode (do not use in text apps)

// Convenience: ensure bit 6 is set so a byte displays as a character (not attribute)
// ASCII 'A'=$41 already has bit 6 set; 'a'=$61 has bit 6 set.
// Space ($20) does NOT have bit 6 set — it is an attribute on Oric.
// To display a literal space, use $60 (grave accent) which is the "blank" character.
#define CH_ORIC(c)      ((uint8_t)((c) | 0x40))
#define CH_SPACE        0x20    // Space = serial attribute on Oric (sets INK+PAPER state)
#define CH_INVSPACE     0xA0    // Inverse space (displays as solid ink-color block)
#define CH_BLANK        0x60    // Oric "blank" character (displays as paper-color space)

// -------------------------------------------------------------------------
// Overlay RAM at $C000–$FFFF
//
// Normally this region is occupied by the Atmos ROM (16 KB).
// When LOCI is connected and acting as disk controller, the register at
// $0314 (Microdisc-compatible) can enable overlay RAM underneath the ROM.
//
// NOTE: Overlay RAM requires LOCI active. Not available in Oricutron.
// Reference: https://osdk.org/index.php?page=articles&ref=ART14
// -------------------------------------------------------------------------

#define MICRODISCCFG    (*((volatile uint8_t *)0x0314))
#define OVERLAY_ON      0xFD   // %11111101 — enable overlay RAM (LOCI required)
#define OVERLAY_OFF     0xFF   // %11111111 — restore ROM

#define OVERLAY_BASE    0xC000U   // Start of overlay RAM
#define OVERLAY_SIZE    0x4000U   // 16 KB

// -------------------------------------------------------------------------
// Oric ROM system vectors (do not call when overlay RAM is active)
// -------------------------------------------------------------------------

#define XGETKY          ((void (*)())0x023B)  // Patchable keyboard vector → GTORKB
#define GTORKB          0xEB78                // Get key with auto-repeat (ROM entry)

// IRQ RAM vector (ROM chains through here; we install our handler here)
#define IRQ_VEC_LO      (*((volatile uint8_t *)0x0245))
#define IRQ_VEC_HI      (*((volatile uint8_t *)0x0246))

// -------------------------------------------------------------------------
// VIA Timer 1 (100 Hz system IRQ)
// -------------------------------------------------------------------------

// ROM sets Timer 1 in free-run mode for 100 Hz (latch value ≈ 9984)
// For custom timer: write latch to $0306/$0307, start with write to $0305
#define TIMER1_100HZ    9984    // Latch value for 100 Hz @ 1 MHz Oric clock

#endif  // ORIC_H
