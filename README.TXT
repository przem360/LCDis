LCDIS - LC86104C/108C disassembler for the Sega VMU/VMS
        Copyright 1999-2001 John Maushammer    john@maushammer.com
        Release 1.04a   21 January 2000

	Now that I'm in the market for a cool job, I thought it would be good
	to add this note:

	DISCLAIMER:
	This is decent code, but it is not representive of my professional
	work. Usually, my code is better documented (a design document and/or
	better function descriptions) and much more modular so that expanding,
	maintaining, and reusing the code is easier. Because the maximum size
	of the code to be analyzed is relatively small (64K), and because the
	computer it will run on is relatively powerful, and because this is a
	non-real time application that is not often run (unlike a compiler),
	many optimizations were not done -- for example, all searches are
	linear. I have not included my usual self-checking to make sure the
	program works as intended, but I have tested the code in a back-ended
	way: I make sure that the compiler compiles lcdis's output back into
	the same binary code.

        Please email me if you find a bug, or can suggest an improvement.

        enjoy.


Features:
 - Sanyo LC86104C/108C disassembler, plus extra instructions used by the VMU
 - Actually simulates program execution, tracing all jumps and possible branches. This allows:
 -   Automatic determination of code/data space.
 -   Automatic determination of RAM banks accessed (mostly)
 - Special function registers and BIOS entry points are annotated.
 - Some instructions are automatically commented (i.e. branch if 'A' button pressed)
 - Disassembly output has been tested and found accurate.
 - Either easier-to-read or ready-to-assemble code can be generated.
 - User specification of graphic & font areas (which are commented graphically)
 - Portable GPL C code. (with C++ style comments).

Nice feature that may come:
 - Identification of memory locations used as variables
 - More instructions to be automatically annotated.

Usage:
  lcdis inputfile.vms {[options] ...}> outputfile

  The inputfile.vms is a binary file representing memory. The first
  byte in the file is stored at address $0000, and is the entry point.

  available options are:
  STRICT         - forces the code/data detection algorithm to error on the
                   conservative side so that it doesn't run amok disassembling
                   bad code. Usually results in more data/less code, but makes
                   it easier to find bad code.
  ASMOUT         - the output is compatible with Marcus's assembler (basically
                   lacks the raw data output to the left of the "|")
  BIOS           - interpret file as a BIOS (use before ENTRY)
  ENTRYn         - define code starting at address n
  GRAPHBYTESn,b  - define b bytes of graphics at address n
  GRAPHPAGESn,p  - define p pages (=0xC0 bytes) of graphics at address n
  FONT8,n,b      - define b bytes of 8-bit wide fonts at address n


  In addition to the standard entry points, other points can be disassembled.
  Enter these as parameters after the input file. Use decimal or the 0x notation.

  Examples:
      lcdis football.vms ENTRY0x100 ENTRY0x110 ENTRY0x120 > football.lst
      lcdis boom.vms GRAPHPAGES0x6d0,1 GRAPHPAGES0x88a,29 GRAPHPAGES0x1e4b,16 > boom.lst
      lcdis puzzle.vms GRAPHPAGES0xb5a,2 > puzzle.lst

  BIOS example: (for use with Version 1.002,1998/06/04,315-6124-03)
      lcdis vmbios.bin BIOS ENTRY0xe100 ENTRY0x1f0a ENTRY0x3b67 ENTRY0x3ecc FONT8,0x473,0x180 > vmbios.txt


Release platform:
   Windows win32 console application
   (of course, you can compile this for any platform you please, because
   open source is a great thing)

-------------------------------------------------------------------------
Memory Mapper Notes:
   The memory mapping feature is kindof neat. Before disassembly, LCDIS
   simulates execution at a number of entry points (reset and interrupt
   vectors). During the simulation, executable code is marked as such.
   Every branch is assumed to be able to be taken or not, so the following
   code would confuse the memory-mapper:

              BZ   label1
              BNZ  label2
              BYTE $12,$34,$56     ;this data will be interpreted as code

   Luckily, neither the football nor the animator programs seem to use this.
   If an illegal instruction is encountered, the memory mapping doesn't
   continue tracing that thread and issues a warning.

   There are a couple of areas that seem to be code, but aren't reachable
   by the memory mapper. (Example: $21FA in the football program) I'll have
   to figure out if they are really inaccessible, accessible by some exotic
   method, or really should be found.

   One not-so-exotic method that would mess this up- passing arguments to
   a subroutine with data following the CALL program. For example,
       CALL PRINT
       .TEXT "SAMPLE TEXT"
       .BYTE $00
       {code resumes here}


-------------------------------------------------------------------------
More notes:
   Seems to work well for football, DC Animator (boom), and puzzle, but
   not for the DC Animator 2 demo-- is Loren doing something funny with
   the code or is my disassembler messed up?

   Marcus explains (& I agree):
     As for the disassembler running berserk with the animator 2 demo, I'd
     say the demo is definitely bugged.  The offending instruction is this
     one:

               0631- 90 76    |              BNZ    L06A9

               $06A9 is in the middle of another instruction:

               06a7- 21 07 0b | L06A7:       JMPF   L070B
                              |
               06aa- 02 62    | L06AA:       LD     MEM062


               If you try to interpret $06A9 as code anyway, you'll find a
               call to $302, which is inside the color icon data, an
               obviously bogus jump target.

               So, I'd say that the branch offset of the BNZ is simply
               miscalculated.  If I replace it with a legal offset, the
               disassemble goes well without a single warning.  The code
               still runs fine.  I assume the branch is never taken.  I have
               no idea how Loren managed to get the branch offset wrong.
               Maybe he tried to jump a distance greater than 128 bytes, and
               his assembler doesn't do the required range check?


-------------------------------------------------------------------------
Release notes:
   Beta 0.1 - initial beta release
   Beta 0.2 - fixed DBNZ bug - not handled by memmap. Added ascii output.
   Beta 0.3 - fixed R16 rollover problem, which was throwing off memmap and
              causing it to mess up memory when it accessed an out-of-bound
              index. Surprisingly, this only happened in RELEASE, not DEBUG.
              Now an out-of-bound PC causes a fatal error.
            - a certain branch instruction didn't use print_data_label
            - Added use of MEM_INVALID to check for jumps into the middle of
              existing code (ie. into the second or third byte of a multi-byte
              instruction). Usually this means an error, but not necessarily.
   Beta 0.4 - Added user-defined entry points to trace.
            - Added call stack trace buffer for errors found during mapmem
            - Now labels mapmem entry points (i.e. L0000)
   Beta 0.5 - Added graphics disassembly options
   Beta 0.6 - Added predefined labels and some auto-commenting of certain code
            - Added better display of icon data
   Beta 0.61- Added autocommenting of MUL and DIV, some better ANSI
              compatibility
   Beta 0.7 - mapmem thought that "BR" was a conditional branch; it's really
              always taken.
            - mapmem deals with jump to other banks more realistically
              (FIRMWARECALL)
            - mapmem doesn't continue to follow code once it hits an illegal
              instruction
            - removed legacy mem_use[branchaddr]=MEM_CODE_LABELED instructions
            - changed "*** WARNING: used as both code and data:" error to more
              accurate "*** WARNING: this is the target of a possibly
              misaligned jump:" The general behavior here is a bit better.
            - Added bad vein checking ("STRICT") so disassembler doesn't run
              amok and errors can be found easier.
   Beta 0.8 - Added an assembly out option ("ASMOUT"), compatible with Marcus's
              assembler. It doesn't define variables yet; instead it just uses
              the address. Tested with boom, football, and Chao - Marcus's
              assembler recompiles the output of my program to produce the
              exact same .vms file.  I'd say it's ready for an initial release!
   Beta 0.81- Added new auto-commenting for PSW.1 and Flash read/write call
              entry (12/12/99)

   Rel  1.0 - Added BIOS flag (alters program flow assumptions)
            - Fixed error where illegal opcode was list as 2 decimal (not hex)
              digits.
            - added unknown opcodes $50 and $51. These seem to be one byte
              instructions.
            - corrected clock speed auto-comments: LCD=fast/600kHZ,
              normal=slow/32kHz
            - cleaned up spacing of auto-comments, but still uneven
            - suppressed ascii output if all 0xff's

   Rel 1.01 - suppressed ascii output if none of characters are printable
            - corrected readFlash address, added verifyFlash
            - added more autocomments
            - added MEM - labeling of memory registers. Has some problems
              (search on "rambank problems"), but does a pretty good job of
              distinguishing between user and OS banks. Most notably, doesn't
              know what the result of "POP PSW" will be; doesn't return the
              state of PSW after a call; and doesn't detect code that is used
              both ways (example: memory clearing function of BIOS).
              This results in about 30 undefined conditions in the BIOS code;
              LCDIS displays its two guesses.
   Rel 1.02 - Added support for "BPC d9,b3,r8" instruction.
            - Changed LABELS to better describe interrupts (according to Marcus)
            - Added "FONT8," directive for printing 8-bit-wide font tables
            - fixed so that fonts and graphics can be labeled
            ERRATA- this release mistakenly labelled 1.01 in executable
   Rel 1.03 - fixed wrong commenting on MUL and DIV instructions:
              was "; B:ACC:C <- ACC:B * C"   now "; B:ACC:C <- ACC:C * B"
              was "; ACC:C remainder B <- ACC:B / C"
              now "; ACC:C remainder B <- ACC:C / B"
            - blank line after unconditional branch (like after JMP or RET), for readability
            - split out and comment the VM description lines (at $200 and $210):
 0200-  |     BYTE   "TRICKSTYLE JR.  "                 ; File comment on VM (16 bytes)
 0210-  |     BYTE   "TRICKSTYLE JR. VMU GAME.        " ; File comment on Dreamcast (32 bytes)
            - Null-terminated string decoding (uses BYTE   "text")
   Rel 1.04 - A few more autocomments relating to the serial port.
   Rel 1.04a- Only changes to this Readme.txt file were made (extra disclaimer)


Desired features (future):
            - look up indirect variable names (ie. for "MOV #$xx,MEM000")
            - auto decoding of this construct (either order) to indicate effective addr
              2381- 23 04 4e |              MOV    #$4e,TRL
              2384- 23 05 27 |              MOV    #$27,TRH
            - Some sort of comment integration from a file, so that object code
              can be integrated with a comments-only file to generated annotated
              source. This respects the object code writer's copyright while
              allowing people to distribute their annotations.

-------------------------------------------------------------------------
              COPYRIGHT

LCDIS - LC86104C/108C disassembler
Copyright 1999-2000 John Maushammer  john@maushammer.com
Portions by Alexander Villagran
Boom object code by (and probably copyrighted) by Loren Peace
                                                       -- see www.booyaka.com
Boom graphics (c) 1999 by Mike Munn

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


