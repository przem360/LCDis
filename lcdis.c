/*
 * LCDIS - LC86104C/108C disassembler
 * Copyright 1999-2000 John Maushammer  john@maushammer.com
 * Portions by Alexander Villagran
 * Helpful suggestions and code fixes from Marcus Comstedt
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *
 * Release notes:
 *   Beta 0.1 - initial beta release
 *   Beta 0.2 - fixed DBNZ bug - not handled by mapmem. Added ascii output.
 *   Beta 0.3 - fixed R16 rollover problem, which was throwing off mapmem and causing it to mess up memory
 *              when it accessed an out-of-bound index. Surprisingly, this only happened in RELEASE, not
 *              DEBUG. Now an out-of-bound PC causes a fatal error.
 *            - a certain branch instruction didn't use print_data_label
 *            - Added use of MEM_INVALID to check for jumps into the middle of existing code (ie. into the
 *              second or third byte of a multi-byte instruction). Usually this means an error, but not
 *              necessarily.
 *   Beta 0.4 - Added user-defined entry points to trace.
 *            - Added call stack trace buffer for errors found during mapmem
 *            - Now labels mapmem entry points (i.e. L0000)
 *   Beta 0.5 - Added graphics disassembly options
 *   Beta 0.6 - Added predefined labels and some auto-commenting of certain code
 *            - Added better display of icon data
 *   Beta 0.61- Added autocommenting of MUL and DIV, some better ANSI compatibility
 *   Beta 0.7 - mapmem thought that "BR" was a conditional branch; it's really always taken.
 *            - mapmem deals with jump to other banks more realistically (FIRMWARECALL)
 *            - mapmem doesn't continue to follow code once it hits an illegal instruction
 *            - removed legacy mem_use[branchaddr]=MEM_CODE_LABELED instructions
 *            - changed "*** WARNING: used as both code and data:" error to more accurate
 *              "*** WARNING: this is the target of a possibly misaligned jump:"
 *              The general behavior here is a bit better.
 *            - Added bad vein checking ("STRICT") so disassembler doesn't run amok and errors
 *              can be found easier.
 *   Beta 0.8 - Added assembly out option, compatible with Marcus's assembler. It doesn't
 *              define variables yet; instead it just uses the address. Tested with boom, football,
 *              and chao - reassembly is an exact match of disassembly.
 *   Beta 0.81- Added new auto-commenting for PSW.1 and Flash read/write call entry (12/12/99)
 *
 *   Rel  1.0 - Added BIOS flag (alters program flow assumptions)
 *            - Fixed error where illegal opcode was list as 2 decimal (not hex) digits.
 *            - added unknown opcodes $50 and $51. These seem to be one byte instructions.
 *            - correct clock speed auto-comments: LCD=fast/600kHZ, normal=slow/32kHz
 *            - cleaned up spacing of auto-comments, but still uneven
 *            - supressed ascii output if all 0xff's
 *   Rel 1.01 - supressed ascii output if none of characters are printable
 *            - corrected readFlash address, added verifyFlash
 *            - added more autocomments
 *            - added MEM - labeling of memory registers. Has some problems
 *              (search on "rambank problems"), but does a pretty good job of
 *              distinguishing between user and OS banks. Most notably, doesn't
 *              know what the result of "POP PSW" will be; doesn't return the
 *              state of PSW after a call; and doesn't detect code that is used
 *              both ways (example: memory clearing function of BIOS).
 *              This results in about 30 undefined conditions in the BIOS code;
 *              LCDIS displays its two guesses.
 *   Rel 1.02 - Added support for "BPC d9,b3,r8" instruction.
 *            - Changed LABELS to better describe interrupts (according to Marcus)
 *            - Added "FONT8," directive for printing 8-bit-wide font tables
 *            - fixed so that fonts and graphics can be labeled
 *            ERRATA- this release mistakenly labelled 1.01 in executable
 *   Rel 1.03 - fixed wrong commenting on MUL and DIV instructions:
 *              was "; B:ACC:C <- ACC:B * C"   now "; B:ACC:C <- ACC:C * B"
 *              was "; ACC:C remainder B <- ACC:B / C"
 *              now "; ACC:C remainder B <- ACC:C / B"
 *            - blank line after unconditional branch (like after JMP or RET), for readability
 *            - split out and comment the VM description lines (at $200 and $210):
 * 0200-  |     BYTE   "TRICKSTYLE JR.  "                 ; File comment on VM (16 bytes)
 * 0210-  |     BYTE   "TRICKSTYLE JR. VMU GAME.        " ; File comment on Dreamcast (32 bytes)
 *            - Automatic null-terminated string decoding (uses BYTE   "text")
 *            - add more MEM mem register definitions & auto-commenting.
 *
 *
 *   Desired features (future)---------------------------------------------------------
 *            - look up indirect variable names (ie. for "MOV #$xx,MEM000")
 *            - auto decoding of this construct (either order) to indicate effective addr
 *              2381- 23 04 4e |              MOV    #$4e,TRL
 *              2384- 23 05 27 |              MOV    #$27,TRH
 *            - Some sort of comment integration from a file, so that object code
 *              can be integrated with a comments-only file to generated annotated
 *              source. This respects the object code writer's copyright while
 *              allowing people to distribute their annotations.
 *   Rel 1.04 - A few more autocomments relating to the serial port.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <ctype.h>
#include "lcdis.h"

// This define needed for SUN environments:
#if (defined (sparc) || defined (__sparc__) || defined (__sparc))
#define strncmp(x,y,z) strncasecmp(x,y,z)
#endif

// big ugly global variables:
unsigned char mem[0x10000];     // 64K of memory max.
unsigned char mem_use[0x10000]; // memory usage:
unsigned char mem_bnk[0x10000]; // memory usage:
int strictmode=0;               // strict mode that stops at first sign of memmap going amok.
int asmout=0;                   // for compiler-compatible output.
int biosmode=0;                 // for disassembling bios

int main (int argc, char * argv[])
{
  FILE * fin;
  int memsize;
  int pin, p1;    // address counters
  int count;
  int i;

  printf ("; LC86104C/108C disassembler. (C) 1999-2000 John Maushammer.\n"
          ";  Version 1.04                             john@maushammer.com\n"
          ";  GNU public liscense - see www.gnu.org\n;\n;\n");

  if (argc < 2)
  {  printf ("lcdis inputfile.vms {[entrypoint] ...}> outputfile\n\n"
             "  STRICT         - kills bad 'veins'; helps prevent disassembly of bad code and\n"
             "                   finds errors, but stops before all good code is disassembled\n"
             "  ASMOUT         - outputs cleaner code for use with an assembler\n"
             "  BIOS           - interpret file as a BIOS (use before ENTRY)\n"
             "  ENTRYn         - define code starting at address n\n"
             "  GRAPHBYTESn,b  - define b bytes of graphics at address n\n"
             "  FONT8,n,b      - define b bytes of 8-bit wide fonts at address n\n"
             "  GRAPHPAGESn,p  - define p pages (=0xC0 bytes) of graphics at address n\n\n"
             "In addition to the standard entry points, other points can be disassembled.\n"
             "Enter these as parameters after the input file. Use decimal or the 0x notation.\n"
             "Example:\n"
             "    lcdis football.vms ENTRY0x139 > football.lst\n\n");
     return (1);
  }

  printf ("; Source file=%s, ", argv[1]);
  if ( (fin = fopen(argv[1],"rb")) == NULL)
  {  printf ("can not open!\n");
     return (1);
  }

  memset(mem,     0x00,       0x10000); // clear memory
  memset(mem_use, MEM_UNUSED, 0x10000); // clear memory
  memset(mem_bnk, BNK_UNKNOWN,0x10000); // clear memory

  memsize=fread((void *) mem, sizeof (char), 0x10000, fin);
  fclose(fin);
  printf ("%d (0x%04x) bytes.\n", memsize, memsize);
  memset(mem_use, MEM_UNKNOWN, memsize); // clear memory

  if (argc >= 3)  // if extra command-line arguments, then parse here
  {
    for (i=2; i<argc; i++)
    {
       if (strcmp(argv[i], "STRICT")==0)
       {
           printf ("; Strict mode enabled.\n");
           strictmode=1;
       }
       else
       if (strcmp(argv[i], "ASMOUT")==0)
       {
           printf ("; Assembler output mode output enabled.\n");
           asmout=1;
       }
       else
       if (strcmp(argv[i], "BIOS")==0)
       {
           printf ("; BIOS disassembly mode enabled.\n");
           biosmode=1;
       }
       else
       if (strncmp(argv[i], "ENTRY", 5)==0)
       {
           if (1==sscanf(& (argv[i][5]), "%i", &pin))
           {  printf ("; Mapping memory...   user-defined point $%04x\n", pin);
              mapmem (pin, BNK_BANK1);
           }
           else
              printf ("WARNING: cannot parse value in '%s'. Must use decimal or 0x notation.\n", argv[i]);

       }
       else
       if (strncmp(argv[i], "GRAPHBYTES", 10)==0)
       {
           if (2==sscanf(& (argv[i][10]), "%i,%i", &pin, &count))
           {  printf ("; Mapping graphics... user-defined point $%04x-$%04x ($%04x bytes)\n", pin, pin+count-1, count);
              while (count-- && (pin>=0) && (pin <= 0xFFFF))
                 mem_use[pin++]=MEM_GRAPHICS;
           }
           else
              printf ("WARNING: cannot parse value in '%s'. Must use decimal or 0x notation.\n", argv[i]);

       }
       else
       if (strncmp(argv[i], "FONT8,", 6)==0)
       {
           if (2==sscanf(& (argv[i][6]), "%i,%i", &pin, &count))
           {  printf ("; Mapping 8-bit-wide font... user-defined point $%04x-$%04x ($%04x bytes)\n", pin, pin+count-1, count);
              while (count-- && (pin>=0) && (pin <= 0xFFFF))
                 mem_use[pin++]=MEM_FONT8;
           }
           else
              printf ("WARNING: cannot parse value in '%s'. Must use decimal or 0x notation.\n", argv[i]);

       }
       else
       if (strncmp(argv[i], "GRAPHPAGES", 10)==0)
       {
           if (2==sscanf(& (argv[i][10]), "%i,%i", &pin, &count))
           {
              printf ("; Mapping graphics... user-defined point $%04x-$%04x ($%04x pages) \n", pin, pin+(0xC0*count)-1, count);
              count *= 0xC0;    // packed format
              while (count-- && (pin>=0) && (pin <= 0xFFFF))
                 mem_use[pin++]=MEM_GRAPHICS;
           }
           else
              printf ("WARNING: cannot parse value in '%s'. Must use decimal or 0x notation.\n", argv[i]);

       }
       else
       {   printf ("WARNING: unknown command line directive %s\n", argv[i]);
       }
    }
  }


  printf ("; Mapping memory...   reset/start entry point\n");
  // actually the bios probably starts with bank0, but it's code
  // sets it, so it's irrelevant.
  mapmem (0x00, BNK_BANK1);  // reset/start

  printf ("; Mapping memory...   interrupt entry points\n");
  // bank is unknown unless the programmer only uses one bank or takes
  // special precautions
  mapmem (0x03, BNK_UNKNOWN);  // external interrupt 0?
  mapmem (0x0b, BNK_UNKNOWN);  // timer/counter 0 interrupt
  mapmem (0x13, BNK_UNKNOWN);  // external interrupt 1?
  mapmem (0x1b, BNK_UNKNOWN);  // timer/counter 1 interrupt
  mapmem (0x23, BNK_UNKNOWN);  // divider circuit/port 1/port 3 interrupt?
  mapmem (0x2b, BNK_UNKNOWN);  // interrupt
  mapmem (0x33, BNK_UNKNOWN);  // interrupt
  mapmem (0x3b, BNK_UNKNOWN);  // interrupt
  mapmem (0x43, BNK_BANK0);    // interrupt (only acted on by BIOS, not chao nor football, so I guess it's bank0)
  mapmem (0x4b, BNK_UNKNOWN);  // interrupt

  if (biosmode)
  {
     printf ("; Mapping memory...   BIOS entry points\n");
     mapmem (0x100, BNK_BANK1); // writeFlash
     mapmem (0x108, BNK_BANK1); // writeFlash2
     mapmem (0x110, BNK_BANK1); // readFlash
     mapmem (0x120, BNK_BANK1); // int120link
     mapmem (0x130, BNK_BANK1); // intT1link    (ROM's T1 interrupt code)
     mapmem (0x140, BNK_BANK1); // unknown
     mapmem (0x1f0, BNK_BANK1); // quit
  }

  search_text(memsize);
  printf ("; Done mapping memory.\n");

  printf ("\n\n;------------------------------------------------------------------\n\n");

  if (asmout)
    printf ("             .include \"sfr.i\"\n\n"
            "             .org 0\n\n\n");

  // simple straight-through disassembly: (all code)
  for (pin=0; pin<memsize; )
  {
     dis(pin, &p1);
     pin = p1;
  }

  return(0);
}


// FUNCTION dis
//
// inputs
//   pin = address counter to disassemble
//
// outputs
//   b1 = address of next instruction (or -1 if RETURN)
//
// calls
//   dis_code or dis_data if needed
//
// Model line:
//   "0593- 23 00 00 |              MOV    #$00,ACC"


void dis (int pin, int * b1)
{
   int i;
   unsigned char opcode;
   int quoteopen;

   opcode = mem[pin];

// Debug code to force all memory to be interpreted as code or data:
//   mem_use[pin] = MEM_DATA;
//   mem_use[pin] = MEM_CODE;   // it's executable

   if (pin <0 || pin>0xFFFF)
      printf ("ERROR: attempted to dissassemble illegal address %04x!\n", pin);

   switch (mem_use[pin])
   {
     case MEM_UNUSED:    // not loaded from file
        *b1=-1;          // can't branch anywhere
        break;

     case MEM_UNKNOWN:   // if unknown, then assume it's data
     case MEM_DATA:
          dis_data (pin, b1);
        break;

      case MEM_GRAPHICS:
        if (!asmout)
          printf ("%04x-          | ", pin);

        print_code_label(pin,2);  // print label if possible
        printf ("BYTE   ");

        printf ("$%02x", mem[pin]);
        for (i=1; i<6; i++)    /* 6 bytes per line */
           printf (",$%02x", mem[pin+i]);

        printf ("          ;graphics \"");
        for (i=0; i<6; i++)    /* 6 bytes per line */
           printf ("%c%c%c%c%c%c%c%c",
                               mem[pin+i] & 128 ? '#' : '.',
                               mem[pin+i] & 64 ? '#' : '.',
                               mem[pin+i] & 32 ? '#' : '.',
                               mem[pin+i] & 16? '#' : '.',
                               mem[pin+i] & 8 ? '#' : '.',
                               mem[pin+i] & 4 ? '#' : '.',
                               mem[pin+i] & 2 ? '#' : '.',
                               mem[pin+i] & 1 ? '#' : '.');

        printf ("\"\n");
        *b1=pin+6;
        break;

      case MEM_FONT8:
        if (!asmout)
          printf ("%04x-          | ", pin);

        print_code_label(pin,2);  // print label if possible
        printf ("BYTE   $%02x               ;font \"%c%c%c%c%c%c%c%c\"\n",
                      mem[pin],
                      mem[pin] & 128 ? '#' : '.',
                      mem[pin] & 64 ? '#' : '.',
                      mem[pin] & 32 ? '#' : '.',
                      mem[pin] & 16? '#' : '.',
                      mem[pin] & 8 ? '#' : '.',
                      mem[pin] & 4 ? '#' : '.',
                      mem[pin] & 2 ? '#' : '.',
                      mem[pin] & 1 ? '#' : '.');
        *b1=pin+1;
        break;

     case MEM_TEXT:
         if (!asmout)
            printf ("%04x-          | ", pin);
         printf ("             BYTE   \"");
         quoteopen=1;
         i=0;
         while (mem_use[pin] == MEM_TEXT)   // or until broken by a $00
         {

// This output must be compatible with fixup_string in lexar.c of Marcus's assembler.
// that function supports most c escape sequences, but not \\ nor \", so we just
// print the hex byte equivalents.

           if (   !isprint (mem[pin]) // print as hex byte 
               || (mem[pin]=='\\')    // vmuasm has no escape sequence for this
               || (mem[pin]=='\"'))   // vmuasm has no escape sequence for this
           {
              if (quoteopen)
              {  printf ("\"");
                 quoteopen=0;
              }
              if (i!=0)
                printf (",");  // use a comma only if we've added something to the string.
              printf ("$%02x", (unsigned int) ((unsigned char) mem[pin]));
           }
           else   // print quoted text
           {
              if (!quoteopen)
              {  printf (",\"");
                 quoteopen=1;
              }
              printf ("%c", mem[pin]);
           }

           if (mem[pin++] == 0)  // end-of-line?
             break;              // then break out of it
           i++;  // count chars so we can use ',' only if needed
         }  
         if (quoteopen)
         {  printf ("\"");
            quoteopen=0;
         }
         printf ("\n");
         *b1=pin;
        break;

     case MEM_INVALID:
        printf ("*** WARNING: this is the target of a possibly misaligned jump:\n");
        // fall into code section

     case MEM_CODE:
     case MEM_CODE_LABELED:
          dis_code (pin, b1);
        break;

      default:
          printf ("FATAL INTERNAL ERROR: unknown type of memory\n");
          exit(-1);
        break;
   }
}


// FUNCTION dis_data
//
// inputs
//   pin = address counter to disassemble
//
// outputs
//   b1 = address of next instruction (or -1 if RETURN)
//
// Model line:
//   "0593- 23 00 00 |              MOV    #$00,ACC"
//    0200-  |     BYTE   "TRICKSTYLE JR.  "                 ;File comment on VM (16 bytes)
//    0210-  |     BYTE   "TRICKSTYLE JR. VMU GAME.        " ;File comment on Dreamcast (32 bytes)


void dis_data (int pin, int * b1)
{
   int i,i2;
   int valid;
   int textsize;
   int printdefault=0;   // set if we don't have a special way to display the data.
                         // Sometimes set if the special data can't print the data correctly.
   unsigned char opcode;
   opcode = mem[pin];


   // Handle game icon data:
   if ((pin >= 0x280) && (pin < 0x280+mem[0x240]*0x200) && !biosmode)
   {           // icon data for display on dreamcast
      if (((pin-0x280) & 0x1FF) == 0x0)   // in icon boundry?
      {  if (!asmout)
            printf ("%04x-          |   ;icon #%d\n", pin, (pin-0x280)/0x200);
         else
            printf ("  ;icon #%d\n", (pin-0x280)/0x200);
      }

      if (!asmout)
         printf ("%04x-          | ", pin);
      printf ("             BYTE   $%02x", opcode);

      for (i=1; i<16; i++)
         printf (",$%02x", mem[pin+i]);
      printf ("    ");            // pad to make comment align
      printf ("  ;icon \"");

      for (i=0; i<16; i++)
      {
        if (mem[pin+i] & 0xF0)
           printf ("%x", mem[pin+i]>>4);  // print hex digit 1-F
        else
           printf (" ");                  // print space instead of '0'

        if (mem[pin+i] & 0x0F)
           printf ("%x", mem[pin+i]&0xf);  // print hex digit 1-F
        else
           printf (" ");                  // print space instead of '0'
      }

      printf ("\"");
      *b1=pin+16;   // icons are always lines of 16 bytes
   }
   else        // handle game name fields
   if ((pin == 0x200) || (pin == 0x210))
   {
      textsize = (pin==0x200) ? 16 : 32;
      valid=1;   // check validity
      for (i=0; (i<textsize) && valid; i++)
         if (!isprint (mem[pin+i]) || (mem[pin+i] & 0x80))  // not valid if not printable or has most significant bit set
           valid=0;

      if (valid)
      {
        if (!asmout)
           printf ("%04x-          | ", pin);
        printf ("             BYTE   \"");
        for (i=0; i<textsize; i++)
          printf ("%c", mem[pin+i]);
        printf ((pin==0x200) ? "\"                 ;File comment on VM (16 bytes)"
                             : "\" ;File comment on Dreamcast (32 bytes)");
        *b1=pin+textsize;   // icons are always lines of 16 bytes
      }
      else
        printdefault=1;  // can't print correctly, so default to binary.
   }
   else
     printdefault=1;


   if (printdefault)   // general data
   {           
      if (!asmout)
         printf ("%04x-          | ", pin);
      printf ("             BYTE   $%02x", opcode);
      i=pin+1;
      i2=!isprint (opcode & 0x7F);  // i2 is true as long as bytes are nonprintable

      while ( (i & 0x7) &&
              ( (mem_use[i]==MEM_UNKNOWN) || (mem_use[i]==MEM_UNKNOWN)) )
      {  i2 &= !isprint (mem[i] & 0x7F); // i2 is true as long as bytes are all 0xFF
         printf (",$%02x", mem[i++]);
      }

      if (!i2) // if all data isn't 0xFF, then
      {        // print ASCII representation
         for (i2=8-(i-pin); i2; i2--)
           printf ("    ");            // pad to make comment align
         printf ("  ;ascii \"");
         for (i2=pin; i2<i; i2++)
           printf ("%c", (isprint (mem[i2] & 0x7F)) ? (mem[i2] & 0x7F) : '.');
         printf ("\"");
      }
      *b1=i;
   }

   printf ("\n");           // end of line
}



// FUNCTION dis_code
//
// inputs
//   pin = address counter to disassemble
//
// outputs
//   b1 = address of next instruction (or -1 if RETURN)
//
// Model line:
//   "0593- 23 00 00 |              MOV    #$00,ACC"

void dis_code (int pin, int * b1)
{
   char * model;
   int found;
   int i,i2;
   unsigned char opcode;

   opcode = mem[pin];

//Debug code: insert this to see what bank each line was calculated to be in
//            (BNK0, BNK1, BNK2=unknown)
//printf ("BNK%d ", mem_bnk[pin]);

   if (!asmout)
   {
      printf ("%04x- ", pin);

      for (i=0; i<3; i++)
         if (i<opcode_len(opcode))// print raw bytes:
            printf ("%02x ", mem[pin+i]);
         else
            printf ("   ");

      printf ("| ");
   }

   // print label if wanted
   if (mem_use[pin] == MEM_CODE_LABELED)
      print_code_label(pin,1);  // formatted
   else
      printf ("             ");

   model = get_opcode_model(opcode);
   printf ("%5.5s  ", model);

   // calculate next word
   *b1 = pin + opcode_len (opcode);

   switch (model[5])
   {
      case ' ':   // no parameters
         break;

      case '2':   // a12   absolute
         print_code_label (get_a12(pin),0);
         // printf ("$%04x", get_a12(pin));
         break;

      case '6':   // r16   relative
         print_code_label (get_r16(pin),0);
         // printf ("$%04x", get_r16(pin));
         break;

      case '7':   // a16   absolute
         print_code_label (get_a16(pin),0);
         // printf ("$%04x", get_a16(pin));
         break;

      case '8':   // r8    relative
         print_code_label (get_r8(pin),0);
         // printf ("$%04x", get_r8(pin));
         break;

      case '9':   // d9    direct
         print_data_label (get_d9(pin), mem_bnk[pin]);
         // printf ("$%03x", get_d9(pin));
         break;

      case '@':   // @Ri   indirect
         printf ("@R%d", get_reg(pin));
         break;

      case '#':   // #     immediate
         printf ("#$%02x", mem[pin+1]);
         break;

      case '^':   // ^=#i8,d9 immediate
         printf ("#$%02x,", mem[pin+2]);
         print_data_label (get_d9(pin), mem_bnk[pin]);
         // printf ("#$%02x,$%03x", mem[pin+2], get_d9(pin));
         break;

      case '%':   // %=#i8,@Ri
         printf ("#$%02x, @R%d", mem[pin+1], get_reg(pin));
         break;

      case 'b':   // b=d9,b3    bit manipulation
         print_data_label(get_d9bit(pin), mem_bnk[pin]);
         printf (", %d", mem[pin]&7);
         // printf ("$%03x, %d", get_d9bit(pin), mem[pin]&7);
         break;

      case 'r':   // r=d9,b3,r8 bit branch
         print_data_label (get_d9bit(pin), mem_bnk[pin]);
         printf (", %d, ", mem[pin]&7);
         print_code_label (get_r8(pin+1),0);
         // printf ("$%03x, %d, $%04x", get_d9bit(pin), mem[pin]&7, get_r8(pin+1));
         break;

      case 'z':   // z=#i8,r8
         printf ("#$%02x,", mem[pin+1]);
         print_code_label (get_r8(pin+1),0);
         // printf ("#$%02x,$%04x", mem[pin+1], get_r8(pin+1));
         break;

      case 'x':   // x=d9,r8
         print_data_label (get_d9(pin), mem_bnk[pin]);
         printf (",");
         print_code_label (get_r8(pin+1),0);
         // printf ("$%03x,$%04x", get_d9(pin), get_r8(pin+1));
         break;

      case 'v':   // v=@Ri,#i8,r8
         printf ("@R%d,#$%02x,", get_reg(pin), mem[pin+1]);
         print_code_label (get_r8(pin+1),0);
         // printf ("@R%d,#$%02x,$%04x", get_reg(pin), mem[pin+1], get_r8(pin+1));
         break;

      case 'c':   // c=@Ri,r8
         printf ("@R%d,", get_reg(pin));
         print_code_label (get_r8(pin),0);
         // printf ("@R%d,$%04x", get_reg(pin), get_r8(pin));
         break;

      case '!':   // illegal
         printf ("$%02x                ;illegal opcode", (int) opcode);
         break;

      default:
         printf ("! ! !\nFATAL Error: unexpected model %c in op[] table!\n", model[5]);
         exit (-1);  // not graceful
   }

//   printf ("       [model %c] ", model[5]);  // helpful for debugging


   // print pre-defined comments for particular instructions:
   found=0;
   for (i=0; (CODECMTS[i].code[0] != -1) && !found; i++)  // check whole list
   {
      found = 1;
      for (i2=0; (i2<3) && found; i2++)
         if ((mem[pin+i2]!=CODECMTS[i].code[i2]) && CODECMTS[i].code[i2] != -1)
            found=0;   // didn't fit pattern
      if (found)
         printf ("      %s", CODECMTS[i].text);
   }


   // add a comment for indirect variable names
// *            - look up indirect variable names (ie. for "MOV #$xx,MEM000")
 // (future) !!!



   printf ("\n");

   // add a blank line after some instructions:
   if (    (strncmp(model, "JMP  ", 5) == 0)
        || (strncmp(model, "JMPF ", 5) == 0)
        || (strncmp(model, "RET", 3) == 0)      // RET and RETI
        || (strncmp(model, "BR   ", 5) == 0)    // unconditional branches
        || (strncmp(model, "BRF  ", 5) == 0))
      printf (asmout ? "\n" : "               |\n");
}




// input: executable address
//        rambank (in future versions, this may expand to a system state)
//
// warning: is fooled by branches that cover both cases
//          i.e. BZ followed by a BNZ will always go to one of those
//          cases, but this code will think that the BNZ case may not
//          execute and continue mapping. Nice programmers would use
//          a BR instruction in the second case, but those that aren't
//          used to such new-fangled technology might not (like me).
//
//          Also, computed gotos (if possible) are not honored
//
//          rambank problems:
//
//          rambank does not handle POP PSW (restoration). A complimentry
//          bug is that calls don't affect rambank when they return, so if
//          you assume that calls restore PSW after they are done with it,
//          then missing POP PSW is ok. This does lead to some uncommented
//          code at the end of routines (especially if push PSW is one of
//          last things saved on the stack), but usually this matches up
//          with the pushes quite nicely.
//
// Returns: 0=valid code
//          1=invalid code

int mapmem (int pin_in, int rambank)
{
   int    branchaddr;
   char * model;
   unsigned char opcode;
   int    i;
   int    pin;                // pc during trace
   int    entry;
   int    badvein;

   static int  level=0;       // static during recursion
   static int  calltrace[200];
// debug variables:
// int x; char junk[200];


   pin=pin_in;                // start trace at passed parameter

   //-- record mapmem recursion
   if (level<200)
      calltrace[level]=pin;
   level++;
   //--

   while (1)
   {

      if (pin <0 || pin>0xFFFF)
      {   printf ("FATAL INTERNAL ERROR: attempted to map illegal address %04x!\n", pin);
          exit (-1);
      }

      if (    (mem_use[pin] == MEM_DATA)       // <- this is impossible so far because we don't mark any code data yet.
           || (mem_use[pin] == MEM_GRAPHICS)
           || (mem_use[pin] == MEM_UNUSED))
      {
         printf ("WARNING: branch exists to data/graphics/unused code at $%04x\n"
                 "         trace stack: ", pin);
         for (i=0; i<level; i++)
           printf ("%04x ", calltrace[i]);
         printf ("\n\n");
         level--;
         mem_use[pin_in] = MEM_INVALID;  // we'll label it invalid.
         return 1;    // only explore the good stuff
      }
      if (mem_use[pin] == MEM_INVALID)
      {
         printf ("WARNING: branch exists to invalid code at $%04x\n"
                 "         trace stack: ", pin);
         for (i=0; i<level; i++)
           printf ("%04x ", calltrace[i]);
         printf ("\n\n");
         level--;
         return 1;    // only explore the good stuff
      }

      if (mem_use[pin] != MEM_UNKNOWN)
      {
         level--;
         mem_use[pin_in] = MEM_CODE_LABELED;  // we'll label the main entry point

         return 0;    // only explore the unknown
      }

      // figure out if mode change
      if ((mem[pin] == 0xf9) && (mem[pin+1] == 0x01))
         rambank = BNK_BANK1;   // access game ram bank     SET1   PSW, 1
      if ((mem[pin] == 0xd9) && (mem[pin+1] == 0x01))
         rambank = BNK_BANK0;   // access OS ram bank       CLR1   PSW, 1
      if ((mem[pin] == 0x71) && (mem[pin+1] == 0x01))
         rambank = BNK_UNKNOWN; // restore bank from stack  POP    PSW

      mem_use[pin] = MEM_CODE;   // it's executable
      if (mem_bnk[pin] == BNK_UNKNOWN)
        mem_bnk[pin] = rambank;
      else
        if (mem_bnk[pin] != rambank)   // if found a conflicting instance
          mem_bnk[pin] = BNK_VARIOUS;

///// use one or both of these for debugging:
/////dis(pin, &x);
/////gets (junk);

      opcode = mem[pin];
      model = get_opcode_model(opcode);

      // Flag illegal code
      if (model[5] == '!')
      {
         printf ("WARNING: illegal instruction found at $%04x\n"
                 "         trace stack: ", pin);
         for (i=0; i<level; i++)
           printf ("%04x ", calltrace[i]);
         printf ("\n\n");
         level--;    // don't continue to follow this vein
         return 1;   // and tell caller not to, either.
      }

      if (strncmp(model, "CALL ", 5) == 0)
      {
         branchaddr = get_a12(pin);
         badvein=mapmem(branchaddr, rambank);        // recurse
         if (badvein && strictmode)
         {  level--;
            mem_use[pin_in] = MEM_CODE_LABELED;  // we'll label the main entry point
            return 1;                                  // end of the line
         }
      }
      else
      if (strncmp(model, "CALLF", 5) == 0)
      {
         branchaddr = get_a16(pin);
         badvein=mapmem(branchaddr, rambank);        // recurse
         if (badvein && strictmode)
         {  level--;
            mem_use[pin_in] = MEM_CODE_LABELED;  // we'll label the main entry point
            return 1;                                  // end of the line
         }
      }
      else
      if (strncmp(model, "CALLR", 5) == 0)
      {
         branchaddr = get_r16(pin);
         badvein=mapmem(branchaddr, rambank);       // recurse
         if (badvein && strictmode)
         {  level--;
            mem_use[pin_in] = MEM_CODE_LABELED;  // we'll label the main entry point
            return 1;                                  // end of the line
         }
      }
      else
      if (strncmp(model, "JMP  ", 5) == 0)
      {
         branchaddr = get_a12(pin);
         badvein=mapmem(branchaddr, rambank);         // recurse
         level--;
         mem_use[pin_in] = MEM_CODE_LABELED;  // we'll label the main entry point
         return badvein;                                  // end of the line
      }
      else
      if (strncmp(model, "JMPF ", 5) == 0)
      {
         branchaddr = get_a16(pin);
         badvein=mapmem(branchaddr, rambank);         // recurse
         level--;
         mem_use[pin_in] = MEM_CODE_LABELED;  // we'll label the main entry point
         return badvein;
      }
      else
      if (strncmp(model, "RET", 3) == 0)     // RET and RETI
      {
         level--;
         mem_use[pin_in] = MEM_CODE_LABELED;  // we'll label the main entry point
         return 0;                            // a dead-end
      }
      else
      if (strncmp(model, "BRF  ", 5) == 0)
      {
         branchaddr = get_r16(pin);
         badvein=mapmem(branchaddr, rambank);       // recurse
         level--;
         mem_use[pin_in] = MEM_CODE_LABELED;        // we'll label the main entry point
         return badvein;                                  // a dead end
      }
      else
      if (strncmp(model, "BR   ", 5) == 0)
      {
         branchaddr = get_r8(pin);
         badvein=mapmem(branchaddr, rambank);       // recurse
         level--;
         mem_use[pin_in] = MEM_CODE_LABELED;        // we'll label the main entry point
         return badvein;                            // a dead end
      }
      else       // These branch instructions are all assumed to be takeable or non-taken:
      if (   (model[0]=='B')                     // if branch instruction
          || (strncmp(model, "DBNZ ", 5) == 0))  // funnily named branch instruction
      {
//         rambank = BNK_UNKNOWN;  // we don't know if branch is taken or not
         switch (model[5])
         {
            case '8':   // r8    relative
            case 'c':   // c=@Ri,r8
               branchaddr= get_r8(pin);
               break;
            case 'r':   // r=d9,b3,r8 bit branch
               branchaddr= get_r8(pin+1);
               break;
            case 'z':   // z=#i8,r8
            case 'x':   // x=d9,r8
            case 'v':   // v=@Ri,#i8,r8
               branchaddr= get_r8(pin+1);
               break;

            default:
               printf ("FATAL Error: unexpected branch model %c in op[] table!\n", model[5]);
         }
         badvein=mapmem(branchaddr, rambank);    // recurse
         if (badvein && strictmode)
         {  level--;
            mem_use[pin_in] = MEM_CODE_LABELED;  // we'll label the main entry point
            return 1;                                  // end of the line
         }
      }
      else   // Code could change execution location by switching banks:
      if ( (mem[pin] == 0xB8) && (mem[pin+1]==0x0D) )  // {0xB8, 0x0D,   -1} NOT1   EXT, 0
      {
         if (!biosmode)
         {  entry= pin + opcode_len(opcode);  // calc PC of code after this instruction
                                              // (always 2 now, but might be 1 if some other way of modifing EXT is trapped)
            for (i=0; FIRMWARECALL[i].entry != -1; i++)
            {
               if (FIRMWARECALL[i].entry == entry)    // did we find exit?
               {
                  // special case: after the NOT1 EXT,0 instruction, there is some code
                  // that doesn't look like it's executed, but looks like its inserted
                  // "just in case". Usually it's a jump to try the NOT1 EXT,0 portion
                  // again. It may be junk code. We'll disassemble just one opcode to
                  // make it look pretty.
                  if (mem_use[entry] == MEM_UNKNOWN)   // if it would otherwise not be disassembled...
                     mem_use[entry] = MEM_CODE;

                  if (FIRMWARECALL[i].exit == -1)
                  {                        // treat like a return (this is the exit vector)
                     level--;
                     mem_use[pin_in] = MEM_CODE_LABELED;  // we'll label the main entry point
                     return 0;                            // a dead end
                  }
                  else
                  {                        // treat like a jump
                     badvein=mapmem(FIRMWARECALL[i].exit, rambank);        // recurse
                     level--;
                     mem_use[pin_in] = MEM_CODE_LABELED;  // we'll label the main entry point

                     return badvein;                            // a dead end
                  }
               }
            }

            printf ("WARNING: NOT1 EXT,0 encountered at unexpected address %04x.\n"
                    "         This code calls a routine in the firmware and the return address\n"
                    "         is unknown (not in FIRMWARECALL table).\n\n", entry);
            level--;
            mem_use[pin_in] = MEM_CODE_LABELED;  // we'll label the main entry point
            return 1;                            // a dead end since we don't know where to go
            // we consider this an error because it is unexpected code.
         }
         else          // handle "NOT1 EXT, 0" in biosmode
         {
            level--;
            mem_use[pin_in] = MEM_CODE_LABELED;  // we'll label the main entry point
            mem_use[pin+2] = MEM_CODE_LABELED;   // label the jump to user code
            return 1;                            // treat as a dead end
         }
      }

      // continue evaluating code until end
      pin++;   // usage for pin has been marked as code already

      // mark remaining bytes of this instruction as code
      for (i=0; i<opcode_len(opcode)-1; i++)
      {
         if (mem_use[pin] != MEM_UNKNOWN)
         {
            printf ("WARNING: misaligned code found at $%04x\n"
                 "         trace stack: ", pin);
            for (i=0; i<level; i++)
              printf ("%04x ", calltrace[i]);
            printf ("\n\n");

            mem_use[pin_in] = MEM_CODE_LABELED;  // we'll label the main entry point
            return 1;    // don't continue in this vein because it's messed up (probably)
                         // see example below
         }

         mem_use[pin++] = MEM_INVALID;
           // mark 2nd through 3rd bytes of an instruction as invalid parts to start executing.
           // This is a good assumption, but some people are really tricky and do this on purpose
           //
           // 6502 Example:      CMP #$13    ;return if A=13
           //                    BEQ x1+1
           //                X1: BEQ $60     ; this branch is never taken. $60 opcode is RET and is executed if A=13
           //                                ; you could put a clear-carry there, instead, if you wanted C clear on A=13
           //                    ...
      }

   }  // while 1
}




int opcode_len (int opcode)
{
   char * model;

   model = get_opcode_model(opcode);

   switch (model[5])
   {

     case ' ':   // no parameters
     case '@':   // @Ri   indirect
     case '!':   // illegal
        return 1;

     case '2':   // a12   absolute
     case '8':   // r8    relative
     case '9':   // d9    direct
     case '#':   // #     immediate
     case '%':   // %=#i8,@Ri
     case 'b':   // b=d9,b3    bit manipulation
     case 'c':   // c=@Ri,r8
        return 2;

     case '6':   // r16   relative
     case '7':   // a16   absolute
     case '^':   // ^=#i8,d9 immediate
     case 'r':   // r=d9,b3,r8 bit branch
     case 'z':   // z=#i8,r8
     case 'x':   // x=d9,r8
     case 'v':   // v=@Ri,#i8,r8
        return 3;
   }
}


//------------------------------------------------------------------------------------
// get_opcode_model
//  in: opcode
// out: pointer to model of opcode
//------------------------------------------------------------------------------------

char * get_opcode_model (int opcode)
{
   //lookup table saves table entries and make op[] match LC86104C datasheet

                        //0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
   static int lookup[16]={0,1,2,2,3,3,3,3,4,4,4, 4, 4, 4, 4, 4};

   return op[lookup[opcode & 0x0F] + (opcode >> 4)*5];
}



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
//                 !       invalid!


int get_a12(int pin)
{
  return (  ((pin+2) & 0xF000)               // get first part from PC
          | ((mem[pin] & 0x07) << 8 )        // get some bits
          | ((mem[pin] & 0x10) ? 0x800 : 0)  // out of place bit
          | (mem[pin+1]));                   // lower 8 bits
}


int get_r16(int pin)
{ // warning: byte order is reversed of A16!
  return ( 0xFFFF & (pin + 3 - 1 +(mem[pin+2]<<8 | mem[pin+1])));
}


int get_a16(int pin)
{
  return (mem[pin+1]<<8 | mem[pin+2]);
}

int get_r8(int pin)
{
   return ( 0xFFFF & (pin + 2 + (int) ((char) mem[pin+1])));
}

int get_d9(int pin)
{
   return ( (mem[pin]&1)<<8 | mem[pin+1]);
}
int get_d9bit(int pin)
{
   return ( (mem[pin]&0x10)<<4 | mem[pin+1]);
}

int get_reg(int pin)
{
   return (mem[pin] & 0x3);
}


// could be modified to return SFR comments, or to identify
// registers (MEM00-MEM0F) which isn't done because they may
// not all be used as registers (depends on PSW)
//
// Outputs:
//   Pre-define label name
//   MEMxxx when bank is known
//   MEMUxx when bank is unknown
//   SFR1xx when sfr label isn't known
//   $xx    when in assembly mode


void print_data_label (int addr, int rambank)
{
   int found=0;
   int i;

   int bankedaddr;


   if (addr < 0x100)    // accessing memory
   {

      if (!asmout)
      {
         if (rambank==BNK_BANK0)   // determine bank
           bankedaddr=addr;
         else
         if (rambank==BNK_BANK1)
           bankedaddr=addr + 0x100;
         else
         {
           rambank = BNK_UNKNOWN;
           bankedaddr=addr;
         }

         if (rambank != BNK_UNKNOWN)
            for (i=0; (MEM[i].addr != -1) && (!found); i++)
               if ((bankedaddr) == MEM[i].addr)
               {
                  printf ("%s", MEM[i].text);
                  found++;
               }

         if (!found)
         {
            if (rambank != BNK_UNKNOWN)
               printf ("MEM%03X", bankedaddr);
            else
            {
               printf ("MEMU%02X", bankedaddr);
               printf ("[unknown bank; BANK0=");
               print_data_label (addr, BNK_BANK0);
               printf (", BANK1=");
               print_data_label (addr, BNK_BANK1);
               printf ("]");
            }
         }
      }
      else
         printf ("$%03x", addr); // assembly-compatible output
                                 // might be nice to add bank info in comment!!!
   }
   else                 // accessing an SFR
   {
      for (i=0; (SFR[i].addr != -1) && (!found); i++)
         if (addr == SFR[i].addr)
         {
            printf ("%s", SFR[i].text);
            found++;
         }

      if (!found)
      {
         if (!asmout)
            printf ("SFR%03X", addr);
         else
            printf ("$%03x", addr);
      }
   }
}

// print code label
//      addr: address
// formatted: 0=just the text, ma'am
//            1=add the colon and print exactly 13 characters
//            2=if not found, don't print hex value- just print 13 spaces
void print_code_label (int addr, int formatted)
{
   int found;
   int i;
   char temp[50];

   found=0;
   for (i=0; (LABELS[i].addr != -1) && (!found); i++)
      if (addr == LABELS[i].addr)
      {
         if (formatted)
         {
            sprintf (temp,"%s:", LABELS[i].text);   // add the colon
            printf ("%-13s", temp);                 // fill to 13 spaces
         }
         else
            printf ("%s", LABELS[i].text);
         found++;
      }
   if (!found)
   {
      if (formatted==2)  // used to label graphics and fonts
        printf ("             ");
      else
      {
        printf ("L%04X", addr);
        if (formatted)
          printf (":       ");
      }
   }
}


//
// Search for text strings and mark them with usage MEM_TEXT
// 
// Text strings:
//    are null terminated
//    are 4-128 characters long
//    don't have 8th bit set
//    are printable or $0d or $0a
//    have at least one alpha character
//
//    doesn't check from 0-0x23F. Usually there is no text there,
//    but more importantly, we don't want to convert the text
//    game name comments to strings (they are better printed out 
//    by dis_data).
//

void search_text (int memsize)
{
   int valid, foundnull, alphacount;
   int pin, i;
   int stringsfound=0;
   printf ("; Searching for text strings...");

   for (pin=0x240; pin<memsize; )
   {
      valid=1;
      foundnull=0;
      alphacount=0;
      for (i=0; (i<128) && valid && !foundnull; i++)
      {
         if ( (mem_use[pin+i]!=MEM_UNKNOWN) &&
              (mem_use[pin+i]!=MEM_DATA))    // it's used for something else, not text.
           valid=0;
         else
         if (mem[pin+i]==0)
           foundnull=1;
         else
         if (    (   !isprint (mem[pin+i])  // not valid if not printable
                  || (mem[pin+i] & 0x80))   // or has most significant bit set  
              && (mem[pin+i] != 0x0d)        //    unless it's a CR
              && (mem[pin+i] != 0x0a)        // or unless it's a LF
            )
           valid=0;

         if (isalpha(mem[pin+i]))
            alphacount++;
      }

      if ((i==128) || (i<5) || !valid || alphacount==0)  // if too long or too short or not valid or didn't have alphas
      {
         pin+=i; // this section was bad, go on to next one...
      }
      else         // found a string!
      {
         while (--i >= 0)
           mem_use[pin++]=MEM_TEXT;
         stringsfound++;
      }
   }

   printf (" (%d strings found)\n", stringsfound);
}

