
int  mapmem (int pin_in, int rambank);
void dis (int pin, int * b1);
void dis_data (int pin, int * b1);
void dis_code (int pin, int * b1);
int  opcode_len (int opcode);
char * get_opcode_model (int opcode);
int get_a12(int pin);   
int get_r16(int pin);
int get_a16(int pin);
int get_r8(int pin);
int get_d9(int pin);
int get_d9bit(int pin);
int get_reg(int pin);
void print_data_label (int addr, int rambank);
void print_code_label (int addr, int formatted);
int  checkmem(void);
void search_text (int memsize);



//---0---- ---1---- --2,3--- --4-7--- --8-F---


// Format: 5 chars=NAME
//         1 char= param   template
//                  =      nop
//                 2=a12   absolute
//                 6=r16   relative 16
//                 7=a16   absolute 16
//                 8=r8    relative 8
//                 9=d9 
//                 @=@Ri   indirect
//                 #=#i8   immediate
//                 ^=#i8,d9 immediate
//                 %=#i8,@Ri   
//                 b=d9,b3    bit manipulation
//                 r=d9,b3,r8 bit branch
//                 z=#i8,r8
//                 x=d9,r8
//                 v=@Ri,#i8,r8
//                 c=@Ri,r8
//                 !=      invalid!!



char op[5*16][8] =
//---0---- ---1---- --2,3--- --4-7--- --8-F---
 {"NOP   ","BR   8","LD   9","LD   @","CALL 2",
  "CALLR6","BRF  6","ST   9","ST   @","CALL 2",
  "CALLF7","JMPF 7","MOV  ^","MOV  %","JMP  2",
  "MUL   ","BE   z","BE   x","BE   v","JMP  2",
  "DIV   ","BNE  z","BNE  x","BNE  v","BPC  r",
  "LDF   ","STF   ","DBNZ x","DBNZ c","BPC  r",
  "PUSH 9","PUSH 9","INC  9","INC  @","BP   r",
  "POP  9","POP  9","DEC  9","DEC  @","BP   r",
  "BZ   8","ADD  #","ADD  9","ADD  @","BN   r",
  "BNZ  8","ADDC #","ADDC 9","ADDC @","BN   r",
  "RET   ","SUB  #","SUB  9","SUB  @","NOT1 b",
  "RETI  ","SUBC #","SUBC 9","SUBC @","NOT1 b",
  "ROR   ","LDC   ","XCH  9","XCH  @","CLR1 b",
  "RORC  ","OR   #","OR   9","OR   @","CLR1 b",
  "ROL   ","AND  #","AND  9","AND  @","SET1 b",
  "ROLC  ","XOR  #","XOR  9","XOR  @","SET1 b",
 };


#define MEM_UNUSED       0
#define MEM_UNKNOWN      1  // used, but unknown yet.
#define MEM_CODE         2
#define MEM_CODE_LABELED 3
#define MEM_DATA         4
#define MEM_GRAPHICS     5
#define MEM_FONT8        6  // 8-bit wide font
#define MEM_TEXT         7  // null-terminated text
#define MEM_INVALID      8  // can't determine proper usage.

#define BNK_BANK0    0   // System variables and the CPU stack
#define BNK_BANK1    1   // For use by game software; by VMS OS before entering game mode
#define BNK_UNKNOWN  2   // unknown usage
#define BNK_VARIOUS  3   // has been observed with multiple usages
                         // (note- the code isn't this smart, yet. It won't
                         //  really remap code it's already visitied.)

typedef struct {int addr; char * text;} addrlist_type;


// This list need not be ordered because i'm lazy. And the VMU doesn't produce that
// much code that a decent computer can't cut through it all quick enough.
//
// This portion provided by Alexander Villagran -- thanks!

addrlist_type SFR[] =
  {   { 0x100, "ACC" }, 
      { 0x101, "PSW" }, 
      { 0x102, "B" }, 
      { 0x103, "C" },     // C Register 
      { 0x104, "TRL" }, 
      { 0x105, "TRH" }, 
      { 0x106, "SP" }, 
      { 0x107, "PCON" }, 
      { 0x108, "IE" }, 
      { 0x109, "IP" },    // Interrupt Priority Ranking Control Register 
      // 0x10A - 0x10C - Not Used 
      { 0x10D, "EXT" },   // External Memory Control Register
      { 0x10e, "OCR"},    // Oscillation Control Register - Picks either 32Khz, 600Khz (flash and LCD), and 5 Mhz (Dreamcast Connected) 
      // 0x10f - Not Used 
      { 0x110, "T0CON" }, // Timer/Counter 0 Control Register 
      { 0x111, "T0PRR" }, // Timer 0 Prescaler Data Register 
      { 0x112, "T0L" }, 
      { 0x113, "T0LR" },  // Timer 0 Low Reload Register 
      { 0x114, "T0H" }, 
      { 0x115, "T0HR" },  // Timer 0 High Reload Register 
      // 0x116-0x117 - Not Used 
      { 0x118, "T1CNT" }, // Timer 1 Control Register 
      // 0x119 - Not Used 
      { 0x11A, "T1LC" },  // Timer 1 Low Compare Data Register 
      { 0x11B, "T1L" },   // Timer 1 Low Register 
      { 0x11B, "T1LR" },  // Timer 1 Low Reload Register 
      { 0x11C, "T1HC" },  // Timer 1 High Compare Data Register 
      { 0x11D, "T1H" },   // Timer 1 High Register 
      { 0x11D, "T1HR" },  // Timer 1 High Reload Register 
      // 0x11E - 0x11F - Not used 
      { 0x120, "MCR"},    // Mode Control Register 
      // 0x121 - Not Used 
      { 0x122, "STAD" },  // Start Addresss Register 
      { 0x123, "CNR" },   // Character Number Register 
      { 0x124, "TDR" },   // Time Division Register 
      { 0x125, "XBNK"},   // Bank Address Register 
      // 0x126 - Not Used 
      { 0x127, "VCCR"},   // LCD Contrast Control Register 
      // 0x128-0x12f - Not Used 
      { 0x130, "SCON0"},  // SIO0 Control Register 
      { 0x131, "SBUF0"},  // SIO0 Buffer 
      { 0x132, "SBR"},    // SIO Baud Rate Generator Register 
      // 0x133 - Not Used 
      { 0x134, "SCON1" }, // SIO1 Control Register 
      { 0x135, "SBUF1" }, // SIO1 Buffer 
      // 0x136-0x143 - Not Used 
      { 0x144, "P1" }, 
      { 0x145, "P1DDR"}, 
      { 0x146, "P1FCR"},  // Port 1 Function Control Register 
      // 0x147-0x14b - Not Used 
      { 0x14c, "P3" }, 
      { 0x14d, "P3DDR"}, 
      { 0x14e, "P3INT"}, 
      // 0x14F-0x15B - Not Used 
      { 0x154, "FLASHA16"}, // bit 0 controls flash A17 bit for LDF/STF
      { 0x15C, "P7" },    // Port 7 Latch 
      { 0x15D, "I01CR" }, // External Interrupt 0, 1 Control Register 
      { 0x15E, "I23CR" }, // External Interrupt 2, 3 Control Register 
      { 0x15F, "ISL" },   // Input Signal Selection Register 
      // 0x160 - 0x162 - Not Used [actually these are used by the BIOS]
      { 0x163, "VSEL"},   // VMS Control Register 
      { 0x164, "VRMAD1"}, // Work RAM Access Address 1 
      { 0x165, "VRMAD2"}, // Work RAM Access Address 2 
      { 0x166, "VTRBF"},  // Send/Receive Buffer 
      { 0x167, "VLREG"},  // Length registration 
      // 0x168-0x17E - Not Used 
      { 0x17F, "BTCR" },  // Base Timer Control Register 
      { 0x180, "XRAM" },  // 0x180-0x1FB - XRAM (Bank 0)[Lines 0 -> 15] 
      { 0x180, "XRAM" },  // 0x180-0x1FB - XRAM (Bank 1)[Lines 16 -> 31] 
      { 0x180, "XRAM" },  // 0x180-0x185 - XRAM (Bank 2)[4 Icons on bottom of LCD - DO NOT USE!] 
      // 0x1FB - 0x1FF - Not Used 
      { -1, "EOL"}        // ---End of list---
  }; 


addrlist_type MEM[] =
  {
// address 0-0xff are in the "OS" bank
//
// 000-003                    Index registers, bank 0 [default]
// 004-007                    Index registers, bank 1 [doesn't seem used by BIOS]
// 008-00b                    Index registers, bank 2 [doesn't seem used by BIOS]
// 00c-00f                    Index registers, bank 3 [doesn't seem used by BIOS]
 
// 010-015 Buffer used by clock mode to convert current date and time to BCD (Binary Coded Decimal)
      {0x017, "CD_YEARHI"},    // Current date, year (high byte)
      {0x018, "CD_YEARLO"},    // Current date, year (low byte)
      {0x019, "CD_MONTH"},     // Current date, month
      {0x01A, "CD_DAY"},       // Current date, day
      {0x01B, "CD_HOUR"},      // Current time, hour
      {0x01C, "CD_MINUTE"},    // Current time, minute
      {0x01D, "CD_SECOND"},    // Current time, second
      {0x01E, "CD_HALFSEC"},   // Current time, halfsecond (0 or 1)
      {0x01F, "CD_LEAPYR"},    // odd=leapyear, even=not leapyear

//{0x020                has a decoded value (0-3==>1,2,4,8) of bits 2&3 of P7 (MEM023)
//{0x023                stores bits 2&3 of P7

      {0x031, "CD_CLOCKSET"},  // FF=date set, 00=not
      {0x033, "AUTO_SLEEP_TIMER"}, //  Auto power-off timer incremented at 2 Hz by T1
      {0x034, "T1SoftCtr2"}, // General purpose counter incremented at 2 Hz by T1
                             // used to time the 2 second beep, blink icons, [autorepeat timer?]
      {0x035, "SLEEP_MODE"}, // Bit-mapped: Bit 0 toggles when user presses sleep
                             //   Bit 6: 1=disables sleep (both auto and user)
                             //   Bit 7: 1=GetBtn will return $FE instead of autosleeping
      {0x050, "CD_YRDIV4HI"},  // Current date, year divided by four (high byte)
      {0x051, "CD_YRDIV4LO"},  // Current date, year divided by four (low byte)

      {0x060, "CURSOR_X"},     // Cursor position, column (0-7)
      {0x061, "CURSOR_Y"},     // Cursor position, row (0-3)

      {0x067, "LCD_BKGROUND"},// Screen background color (0 or 0xFF)

      {0x06D, "GAME_LASTBLK"}, //  Last block used by mini-game
      {0x06E, "BATT_CHECK_DISABLE"}, // Battery check flag. $FF = disable automatic battery check, $00 = enable automatic battery check.
      {0x06F, "FLASHA16_SHADOW"},    // Save a FLASH bank that is saved and restored

      {0x070, "BUTTONS_PRESSED"}, // P3 xor'ed with $FF; 1=button pressed
      {0x071, "BUTTONS_READ"},    // bitmap:1=selected button is pressed & not masked
      {0x072, "BUTTONS_LAST"},    // bitmap:1=ignore because we've seen before, 0=active



      {0x080, "STACK"},        //       Stack

 // addresses 0x100-0x1ff are in the user bank
 // I imagine that the OS uses some of these addresses for itself
 // when the game isn't running. If so, there should be a "BIOS-only"
 // flag so that these are only decoded if checking out the BIOS.
 // Others (such as FL_*) are used by both the BIOS and game for
 // communication.

      {0x17C, "FL_FINAL"},     // 1=wait for last byte to finalize writing
      {0x17D, "FL_ADDR_MSB"},  // Flash read/write start address (24 bits big endian)
      {0x17E, "FL_ADDR_MED"},
      {0x17F, "FL_ADDR_LSB"},
      {0x17F, "FL_BUFFER"},    // 0x180-0x1FF

      {   -1, "EOL"}        // ---End of list---
  };



// predefined label list: 
addrlist_type LABELS[] =
   {  { 0x0000, "reset"},    // Reset
      { 0x0003, "int0"},     // INT0 interrupt (external)
      { 0x000b, "int1"},     // INT1 interrupt (external)
      { 0x0013, "int2_T0L"}, // INT2 interrupt (external) or T0L overflow
      { 0x001b, "int3_BT"},  // INT3 interrupt (external) or Base Timer overflow
      { 0x0023, "intT0H"},   // T0H overflow
      { 0x002b, "intT1"},    // T1H or T1L overflow
      { 0x0033, "intSIO0"},  // SIO0 interrupt
      { 0x003b, "intSIO1"},  // SIO1 interrupt
      { 0x0043, "intRFB"},   // RFB interrupt
      { 0x004b, "intP3"},    // P3 interrupt
      { 0x0100, "writeFlash"},  // link to ROM code
      { 0x0108, "writeFlash2"}, // link to ROM code
      { 0x0110, "verifyFlash"}, // link to ROM code
      { 0x0120, "readFlash"},   // link to ROM code
      { 0x0130, "intT1link"},   // link to ROM's T1 interrupt code
      { 0x0140, "unknownlink"},
      { 0x01f0, "quit"},   // exit vector

      // future use: this can be used in decoding BIOS
      // BIOS Version 1.002,1998/06/04,315-6124-03
      {0x0473, "font0"}, 
      {0x0f17, "fishgraphics"}, 

      { -1, "EOL"}        // ---End of list---
   };


typedef struct {int code[3]; char * text;} codelist_type;

// predefined code comments list:
codelist_type CODECMTS[] =
   {  { {0xB8, 0x0D,   -1}, "      ;execute code in other bank"},  // NOT1   EXT, 0
      { {0x23, 0x4c, 0xFF}, "     ;allow us to read buttons"},     // MOV    #$ff,P3
     
      { {0x98, 0x4c,   -1}, ";branch if up button pressed"},    // BN     P3, 0, xxx
      { {0x99, 0x4c,   -1}, ";branch if down button pressed"},  // BN     P3, 1, xxx
      { {0x9a, 0x4c,   -1}, ";branch if left button pressed"},  // BN     P3, 2, xxx
      { {0x9b, 0x4c,   -1}, ";branch if right button pressed"}, // BN     P3, 3, xxx
      { {0x9c, 0x4c,   -1}, ";branch if A button pressed"},     // BN     P3, 4, xxx
      { {0x9d, 0x4c,   -1}, ";branch if B button pressed"},     // BN     P3, 5, xxx
      { {0x9e, 0x4c,   -1}, ";branch if Mode button pressed"},  // BN     P3, 6, xxx
      { {0x9f, 0x4c,   -1}, ";branch if Sleep button pressed"}, // BN     P3, 7. xxx
     
      { {0x78, 0x4c,   -1}, ";branch if up button isn't pressed"},    // BP     P3, 0, xxx
      { {0x79, 0x4c,   -1}, ";branch if down button isn't pressed"},  // BP     P3, 1, xxx
      { {0x7a, 0x4c,   -1}, ";branch if left button isn't pressed"},  // BP     P3, 2, xxx
      { {0x7b, 0x4c,   -1}, ";branch if right button isn't pressed"}, // BP     P3, 3, xxx
      { {0x7c, 0x4c,   -1}, ";branch if A button isn't pressed"},     // BP     P3, 4, xxx
      { {0x7d, 0x4c,   -1}, ";branch if B button isn't pressed"},     // BP     P3, 5, xxx
      { {0x7e, 0x4c,   -1}, ";branch if Mode button isn't pressed"},  // BP     P3, 6, xxx
      { {0x7f, 0x4c,   -1}, ";branch if Sleep button isn't pressed"}, // BP     P3, 7. xxx
      { {0x03, 0x4c,   -1}, "          ;read buttons"}, //  LD     P3

      { {0x78, 0x5c,   -1}, ";branch if Dreamcast connected"},       // BP     P7, 0, L05E4
      { {0x98, 0x5c,   -1}, ";branch if Dreamcast isn't connected"}, // BN     P7, 0, L05E4


      { {0x23, 0x27, 0x00}, "   ;turn off LCD"},                 // MOV    #$00,VCCR
      { {0x23, 0x27, 0x80}, "   ;turn on LCD"},                  // MOV    #$00,VCCR
//23 20 00 |              MOV    #$00,MCR
//23 20 09 |              MOV    #$09,MCR

      { {0xf8, 0x07,   -1}, "     ;hold- CPU & timers sleep until interrupt"}, // SET1   PCON, 0
      { {0xf9, 0x07,   -1}, "     ;halt- CPU sleeps until interrupt"},         // SET1   PCON, 1

      { {0x03, 0x66,   -1}, " ;read from work RAM"}, //  LD     VTRBF
      { {0x13, 0x66,   -1}, " ;write to work RAM"},  //  ST     VTRBF

      { {0x23, 0x0e, 0xa1}, "    ;set 32 kHz clock speed (normal)"},      //   MOV    #$a1,OCR
      { {0x23, 0x0e, 0x81}, "    ;set 600 kHz clock speed (LCD speed)"}, //   MOV    #$81,OCR
      { {0x23, 0x0e, 0xa3}, "    ;set 32 kHz clock speed (normal), stop RC clock"}, //   MOV    #$a3,OCR
      { {0x23, 0x0e, 0x92}, "    ;set 6 MHz clock speed (docked)"},     //   MOV    #$92,OCR
      { {0xfd, 0x0e,   -1}, ";set subclock mode (set to 32kHz)"},     //   SET1   OCR,5   
      { {0xdd, 0x0e,   -1}, ";clear subclock mode (set to 600kHz)"},  //   CLR1   OCR,5  

      { {0xdf, 0x08,   -1}, "       ;disable all interrupts"},   //  CLR1   IE, 7
      { {0xd8, 0x4e,   -1}, "    ;disable port 3 interrupts"},   //  CLR1   P3INT, 0
      { {0xf8, 0x4e,   -1}, "    ;enable port 3 interrupts"},    //  SET1   P3INT, 0
      { {0xd9, 0x4e,   -1}, "    ;clear port 3 interrupt flag"}, //  CLR1   P3INT, 1

      { {0x9f, 0x01,   -1}, ";branch if carry clear"}, // BN     PSW, 7, L0B6F
      { {0x7f, 0x01,   -1}, ";branch if carry set"},   // BP     PSW, 7, L0B8D
      { {0xdf, 0x01,   -1}, "       ;clear carry flag"},      // CLR1   PSW, 7
      { {0xff, 0x01,   -1}, "       ;set carry flag"},        // SET1   PSW, 7

      { {0xf9, 0x01,   -1}, "      ;access game ram bank"},   // SET1   PSW, 1
      { {0xd9, 0x01,   -1}, "      ;access OS ram bank"},     // CLR1   PSW, 1

      { {0x30,   -1,   -1}, "               ; B:ACC:C <- ACC:C * B"},           //  MUL
      { {0x40,   -1,   -1}, "               ; ACC:C remainder B <- ACC:C / B"}, //  DIV

      { {0x23, 0x06, 0x7f}, "     ;init stack pointer"}, //  MOV    #$7f,SP
      { {0x21, 0x00, 0x00}, "     ;start user's game"},  //  JMPF   reset (used only in OS)

      { {0x23, 0x81, 0x00}, " ;turn off File icon (if xbnk=2)"}, // MOV    #$00,SFR181
      { {0x23, 0x82, 0x00}, " ;turn off Game icon (if xbnk=2)"}, // MOV    #$00,SFR182
      { {0x23, 0x83, 0x00}, " ;turn off Clock icon (if xbnk=2)"},// MOV    #$00,SFR183
      { {0x23, 0x84, 0x00}, " ;turn off Flash icon (if xbnk=2)"},// MOV    #$00,SFR184
      { {0x23, 0x81, 0x40}, " ;turn on File icon (if xbnk=2)"},  // MOV    #$10,SFR181
      { {0x23, 0x82, 0x10}, " ;turn on Game icon (if xbnk=2)"},  // MOV    #$40,SFR182
      { {0x23, 0x83, 0x04}, " ;turn on Clock icon (if xbnk=2)"}, // MOV    #$04,SFR183
      { {0x23, 0x84, 0x01}, " ;turn on Flash icon (if xbnk=2)"}, // MOV    #$01,SFR184
      { {0xbe, 0x81,   -1}, "   ;blink File Icon (if xbnk=2)"},    // NOT1   SFR181, 6
      { {0xbc, 0x82,   -1}, "   ;blink Game Icon (if xbnk=2)"},    // NOT1   SFR182, 4
      { {0xba, 0x83,   -1}, "   ;blink Clock Icon (if xbnk=2)"},   // NOT1   SFR183, 2

      // Timer 1:
      { {0xd9, 0x18,   -1}, "   ;Clear Timer1 low overflow"},   // CLR1   T1CNT, 1
      { {0xdb, 0x18,   -1}, "   ;Clear Timer1 high overflow"},  // CLR1   T1CNT, 3
      { {0x23, 0x18, 0x00}, "  ;Stop Timer1 (sound off)"},     // MOV    #$00,T1CNT
      { {0x23, 0x18, 0x50}, "  ;Start Timer1 low (sound on)"}, // MOV    #$50,T1CNT

      // Flash
      { {0xf9, 0x54,   -1}, ";enable FLASH write, part 1 of 3"}, // SET1   FLASHA16, 1
      { {0xd9, 0x54,   -1}, ";enable FLASH write, part 3 of 3"}, // CLR1   FLASHA16, 1
      { {0x23, 0x54, 0x01}, ";access saved-game area of FLASH"}, // MOV    #$01,FLASHA16  
      { {0x23, 0x54, 0x00}, ";access VMU area of FLASH"},        // MOV    #$00,FLASHA16  

 
      // Serial I/O
      { {0xd8, 0x44,   -1}, ";Clears the P10 latch (P10/S00)"},  // CLR1   P1, 0
      { {0xda, 0x44,   -1}, ";Clears the P12 latch (P12/SCK0)"}, // CLR1   P1, 2
      { {0xdb, 0x44,   -1}, ";Clears the P13 latch (P13/S01)"},  // CLR1   P1, 3

      { {0x23, 0x31, 0x00}, ";Clear the serial tx buffer"},      // MOV    #$00,SBUF0
      { {0x23, 0x35, 0x00}, ";Clear the serial rx buffer"},      // MOV    #$00,SBUF1
      { {0xfb, 0x30,   -1}, ";Start sending character"},         // SET1   SCON0, 3  
      { {0xd9, 0x34,   -1}, ";Resets the transfer end flag (rx)"}, // CLR1   SCON1, 1
      { {0xfb, 0x34,   -1}, ";Starts transfer (rx)"},              // SET1   SCON1, 3
 
      { {0x99, 0x30,   -1}, ";If data is done transmitting, branch"}, // BN     SCON0, 1, Lxxxx

      { {-1, -1, -1},      "EOL"}        // ---End of list---
   };



typedef struct {int entry; int exit;} firmwarecall_type;

// List describing entry and exit points for built-in firmware:
// The entry point is the code executed after the instruction that modifies
// the EXT register (typically NOT1 EXT,0); the exit point is where execution
// resumes.
firmwarecall_type FIRMWARECALL[] =
   {  { 0x102, 0x105},  // writeFlash
      { 0x10a, 0x10b},  // writeFlash2
      { 0x112, 0x115},  // verifyFlash
      { 0x122, 0x125},  // readFlash
      { 0x136, 0x139},  // t1link
      { 0x142, 0x145},  // bios sleep routine
      { 0x1f2,    -1},  // exit vector; doesn't return
// not included     mapmem (0x108); // unknown
//     mapmem (0x140); // unknown
      {   -1,     -1}   // end of list
   };