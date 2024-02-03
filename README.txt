Changes since 3.1:

  Running as a DA under MTOS/Geneva in ST Low rez will no longer
    cause a lockup.

Changes since 7/16:

  The main window now uses 3D attributes.

  Only the necessary portions of the main window are redrawn now. Before,
  the whole window would always redraw.

  The Sector editor can now handle disks with up to 1024 bytes/sector.

  The Sector +/- buttons don't draw if you are not in Sector mode and
  press the left/right arrow keys.

  In the dialog for selecting the filenames to be copied, there are now
  buttons labeled "Main". These will take whatever filename was last
  edited in the main window and use it for the Source or Dest.

  Whenever MemFile receives an ACC_ID message, it now looks to see if
  NeoDesk is present. This allows NeoDesk to know that MemFile has been
  loaded after it was.

  The "addresses might not be alterable" message won't appear for files
  or sectors.
  
  Clicking on the up/down gadgets in the main window will now wait a short
  time after the first click before repeating. Fast computers won't go more
  than one screen on a single click anymore.

  If Decimal mode is used and a memory address goes past 8 digits in length,
  it now gets truncated.


7/16/95

 MemFile 3.1, Copyright ½ 1987-1995, Gribnif Software. All rights reserved.


  This program and its resource file may be freely distributed on the
  conditions that a copy of this DOC file is included and no payment of any
  type (including "copying" fees) is incurred upon the recipient of the
  program. No warranty is made as to the compatibility of this program and
  neither the author nor Gribnif Software is responsible for any loss of
  data which may result from the use of this program. Use it at your own
  risk!


  Revisions since Version 3.0:

    Bug Fixes:
      o Fixed some redraw errors in the main window
      o Using [Shift][Fkey] to program a memory location will not result
        in garbage in the main window the next time you edit.
      o Only the necessary portions of the main window are redrawn now.
        Before, the whole window would always redraw.
      o The Sector +/- buttons don't draw if you are not in Sector mode and
        press the left/right arrow keys.
      o Clicking on the up/down gadgets in the main window will now wait a
        short time after the first click before repeating. Fast computers
        won't go more than one screen on a single click anymore.
      o If Decimal mode is used and a memory address goes past 8 digits in
        length, it now gets truncated.

    Enchancements:
      o The Sector editor can now handle disks with up to 1024 bytes/sector.
        Some operations are also faster now because of this change.
      o MemFile now supports the AP_TERM message, so Geneva or MultiTOS
        can tell it to terminate gracefully.
      o MemFile can ask NeoDesk 4 for information, rather than depending
        on the Accessories list in NeoDesk.
      o The name of a file or disk drive can be passed as a parameter when
        run as an NPG or PRG
      o [Baskspace] can be used in Hex edit mode, to go backward one space
      o Hardware registers ($FE0000->$FFFFFF) can now be edited.
      o MemFile only rewrites bytes that have actually changed. This way,
        individual hardware registers can be changed without resetting
        others to their old values.
      o Inaccessable memory addresses show up as a slightly larger dot.
      o In the dialog for selecting the filenames to be copied, there are
        now buttons labeled "Main". These will take whatever filename was
        last edited in the main window and use it for the Source or Dest.
      o The drives dialog can be Canceled.
      o All dialogs have a 3D appearance when using TOS 3.4 or newer (or
        Geneva.)
      o Only one RSC file is used now.
      o Changes to this manual are denoted by the "|" in the first column.

 
  Overview:

    MemFile is a program that allows you to view and edit any portion of
  your computer's memory, a file on any disk drive, or the individual
  sectors of any drive. Because MemFile can be used as a desk accessory, it
  has the added advantage that it is available from any GEM application, so
  you can easily call it up if, say for instance, you are debugging a
  program and you want to make sure that it has written a file correctly
  without having to leave the program you are working on. It is also a
  moveable window which means that you can re-position it to another
  location on the screen or even click on another open window.  The current
  MemFile package includes the following files:

|   MEMFIL31.DOC -- You're reading it
|   MEMFIL31.ACC -- The program
|   MEMFIL31.RSC -- The program's resource file


  Installation:

    MemFile can be run as a program by simply renaming MEMFIL31.ACC to
| MEMFIL31.PRG. In this mode, you can drag a file to the PRG icon and have
| MemFile load that file automatically. Also, if passed a parameter
| containing just the name of a disk drive (like "A:" or "c:\"), MemFile
| will start in Sector mode with that drive.

|   MemFile can be used as a NeoDesk Program by renaming the fiel to
| MEMFIL31.NPG. In this mode, you can not only do any of the parameter
| passing described above, but MemFile will also use parts of NeoDesk's
| code for some of its operations.

|   To run MemFile as a desk accessory, copy MEMFIL31.ACC and MEMFIL31.RSC
| to the root directory of the disk you normally boot from and perform a
  system reset.

    If you are using NeoDesk 2.05 or newer, you can configure NeoDesk so
  that you can drag a file or disk drive icon to a MemFile icon on the
  desktop:

    1. If you are using NeoDesk 2.05, then create a text file called
|      "NEO_ACC.INF" with the line "MEMFIL31" in it, and save this file to
       the direcory where you keep NeoDesk's files.

|   2. If you are using NeoDesk 3, then you can simply enter "MEMFIL31" in
       the "Accessories..." list in the "Set Preferences..." dialog.

|   3. If you are using NeoDesk 4, you do not need to do anything. MemFile
|      and NeoDesk will talk to each other automatically.

|   4. Open a window to the location where MEMFIL31.ACC is stored and
       drag this icon onto the desktop within NeoDesk.

|   When a file is dragged to the MemFile icon, the copy of MemFile
| already in memory will open in File mode. When a disk drive is dragged
| to the icon, it opens in Sector mode. If you are using NeoDesk 4 and the
| accessory is not already in memory, and you are using Geneva or
| MultiTOS, NeoDesk will load the desk accessory from disk.


  General Use:

    When you select the "MemFile" item from the desktop's menu bar, a
  window appears. When you use MemFile for the first time after booting,
  the program displays the contents of your machine's memory starting at
  the bottom of memory (address 0). At this time you can click on either of
  the two buttons on the right side of the window to toggle between
  hexadecimal or decimal addresses. You can also click on either of the
  other two buttons labeled "File" and "Sector" to switch the display to
  those functions.

    Which ones of the other buttons are valid depends upon the mode you are
  in: if you are in Memory, you can click on either the editable address in
  hex or decimal, but you cannot click on any of the buttons underneath
  these two without switching modes.

    You can edit the information contained in the display by clicking on
  either the two-digit bytes in the second column or their ASCII
  representation in the third column of the display. A cursor will appear,
  which can be moved with the arrow keys. When you are editing the bytes,
  only the 0-9, a-f (or A-F) keys will have any effect. You can either use
  the keys that appear on the main keyboard for this or the ones on the
  numberic keypad. The hex letters A-F can also be entered on the keypad by
  referring to the follwing diagram:
 
         --- --- --- ---
        | A | B | C | D |
         --- --- --- ---
        | 7 | 8 | 9 | E |
         --- --- --- ---
        | 4 | 5 | 6 | F |
         --- --- --- ---
        | 1 | 2 | 3 |Ent|
         ------- ---|   |
        | 0     |###|   |    Note: The decimal key is unused in this mode.
         ------- --- ---

|   When editing the Hex column, the Backspace key on the keyboard acts
| the same as the left arrow key, it moves the cursor back one space. 

    When editing the ASCII values contained in the second column, you can
  enter anything, including control characters. When you are done editing,
  hit the Return key. An alert asking if you want to rewrite the
  information will appear. Click on "Yes" only if you are certain that the
  changes you have made are correct, otherwise you may damage system
  memory, forcing you to re-boot or, worse, damage important information on
  a disk. If you would like the alert to default to "Yes", then use
  Shift-Return to exit editing mode.
 
    Normally, the ASCII column of the display contains all the individual
  characters that make up the area being viewed. The one exception to this
  is the NUL character (0x00) which cannot normally be displayed. MemFile
  shows a decimal point (".") in any location where a NUL should be. Please
  be aware that if you attempt to edit the ASCII column, the only way to
  actually enter a true NUL character is with the decimal key ON THE
  KEYPAD. Do NOT enter a decimal point from the main keyboard if what you
  want is a NUL! The surest way to change a byte to a NUL is to change its
  value in the Hex column to zero instead.

|   Some memory addresses cannot be read or modified, especially the ones
| that are in between hardware registers for the computer's various
| external functions. Addresses of this type will show up as a slightly
| larger dot. When the cursor is on an address like this, nothing can be
| typed on the keyboard. You must move to another address in order to
| change anything.

|   Unlike previous versions of MemFile, this one will only rewrite the
| values that you have actually changed in the edit window. This means
| that hardware registers and system variables which get updated while you
| are in edit mode will not be written to unless you try to change them.
| Previous versions would rewrite all 128 bytes that are in the edit
| window, whether they changed or not. Replacing the old value into
| certain memory locations can be disasterous.

    An additional hint is that anytime you need to enter a number in an
  editable field that allows you to enter hexadecimal, you can use the
  keypad, as described above, to produce the A-F characters.


  The Search Feature:
  
    The "Search" button calls-up a dialog which allows you to search for a
  string of characters within the area that is currently being displayed.
  Click on either the ASCII or the Hex box, depending upon how you want to
  enter the string. After you have clicked on one or the other the cursor
  will appear on the field you selected, allowing you to edit the
  information there. The ASCII field may contain any key that can be typed
  on the keyboard.  The Hex field, however, must only contain the digits
  0-9 and A-F (or lowercase a-f).

    The other option you have here is to specify whether or not you want to
  be alerted when the search has completed. If this options is on, a bell
  tone will be produced when the search has finished, assuming you did not
  cancel the search part way through. The tone will continue to sound every
  few seconds, but can be stopped by pressing a key, the left mouse button,
  or performing a window operation. This feature was intended to allow you
  to do something else (dare I say it?) away from the computer while a
  lengthy search is being performed.

    When you have finished editing the field you chose, click on the
  "Done!" button at the bottom of the dialog. The search will then begin.
  You should see the scroll bar move as the search progresses though, in
  the case of a Memory search, the bar may not move very often.

    If the string of characters is found the display will be redrawn at the
  location where the string is and the location of the start of the string
  will be written in the information line at the top of the window. If the
  string is not found a message appears along the top of the window and the
  display remains at its last location.

    If you are doing a memory search, the location where MemFile stores the
  string you are looking for in memory is excluded from the search. Also,
  if the search goes past phystop, a message will appear in the title bar
  of the window. At this point you may opt to stop the search if you know
  that you do not actually have anymore RAM after phystop.

    You can stop the search at any time by pressing both of the keyboard's
  Shift keys simultaneously. The last location checked in the search will
  be displayed.

    If you want to look for the next occurence of the same string or if you
  want to resume a search that was stopped, merely press the Search button
  a second time. When the dialog appears, click on the "Done!" button and
  the search will proceed. If no other button has been selected since
  Search was invoked the first time, the search will continue from the
  point where it left off. If, however, any button (including the scroll
  bar) has been selected since the initial search, the new search will
  begin from the lowest address on the current display. This means that the
  same string will be found a second time (assuming the previous search was
  not stopped). To find the next occurence, you must then activate the
  Search feature one more time.

    When entering a string in ASCII, there is no way to include the NUL
  character (ASCII zero). The only way to enter this character is by using
  the Hex field and entering "00" for one of the bytes. Please also note
  that when you select Search a second time after having entered a NUL, the
  ASCII string will be truncated wherever the NUL appeared. The Hex
  representation will remain intact.

    If you do not like having the Alert function default to "Off", merely
  change the resource file using a resource editor so that the button is
  preselected.


  The "Copy" Dialog:
  
    This feature allows you to copy any portion of any source to any
  destination type. Additionally, it lets you fill the source with a
  particular byte, and allows for the output to be in a "hex dump" format.
  Because of the large number of options in this dialog and the potential
  for accidentally destroying valuable information with this feature, you
  should read the next section very carefully before using Copy.

    This dialog box consists of two main options, and a few minor ones.
  The Source and Destination describe where to take the data from and where
  to send it to. No matter which source you choose, you must enter valid
  numbers for the Start and End that appear to its right. Only the
  information which appears immediately to the right of the source you
  choose is used. After you have chosen the source, enter the destination
  for the copy operation.

    In the case of Memory the start, end, and destination start are all
  absolute addresses. In the case of a file, they are all relative to the
  beginning of the file. For a Sector copy, entire sectors are manipulated,
  with start, end, and destination start specifying the sector to begin
  reading or writing.

    If you are copying to or from a file, you must select the
  "Filenames..." button. This will bring up a second dialog which lets you
  see which files have been chosen for the source and destination.
| Choosing the button marked "Select..." underneath either field will call
| up the GEM item selector so that you can choose a file. Choosing the
| "Main" button will copy the name of the last file edited in the main
| window into the corresponding filename field. Of course, if you are not
  copying to a file, then you do not need to specify a destination
  filename, and the same is also true of the source filename if the source
  is not a file.

    The "Hex Dump" option will produce a listing of the hexadecimal values
  of the source bytes, along with their ASCII counterparts, on the
  destination device.

    If the "Fill Source?" option is set to "Yes", then the character whose
  hexadecimal value appears below the Yes button will be used to fill the
  entire source area, even if the source and destination overlap. Since
  using this option means that whatever is copied also gets modified, be
  EXTREMELY careful when using it!

    If the destination is a file and it does not exist, then it is created
  when copying. If the destination start is past the end of the destination
  file, then the file is filled to that point with NULs (ASCII character
  0).

    If the destination is a range of disk sectors, then the first source
  address is copied to the beginning of the starting sector in the
  destination. If the ending address of the source does is not in an
  increment the size of a sector, then only the beginning portion of the
  last sector on the disk will be modified.

    Assuming all the options have been set correctly, you can select the
  button labeled "Copy It!" to perform the copy operation. Depending on the
  type of operation you are attempting to perform there may be one or two
  warning messages but, for the most part, there are not. So be very
  careful!


  The Memory Editor:

    The memory editor can display and edit any address from zero to the
  highest available location as specified by the system variable "phystop".
  This location is normally just below the maximum address that can be
  accessed, and depends upon how much RAM your machine has. The only time
  it is not just below the top is when you have installed programs such as
  "reset-proof" ramdisks which change the value of phystop. MemFile can
  display any address in ST RAM up to 0x3FFFFE, however, you will not be
  able to alter any memory location greater than the RAM capacity of your
  machine (normally address 0xFFFFF on a 1040ST). For this reason, whenever
| an address between phystop and 0x3FFFFE or after the ROM start is
  displayed, a warning message appears at the top of the window.

    You can select either of the editable address fields and enter a new
  value. Addresses in decimal contain only the digits 0-9 and addresses in
  hexadecimal contain 0-9 and A-F (or a-f). Pressing Return causes the
  display to be redrawn, starting at the new location. If the location you
  gave was higher than the highest location available, the address will be
  moved to the highest location.

    If you use a 68000- or 68010-based ST and you want to view a memory
  location in ROM, you cannot do so by using the scroll bar because the
  entire area from 0x400000 to 0xF90000 is inaccessible. Instead, you must
  begin by using either of the editable address fields to enter an address
  from the start of ROM (0xFC0000 for the ST, 0xE00000 for the STe) to
  0xFFFFFF. The scroll bar will then assume that the "lowest" address is
  actually the start of ROM.  Enter any RAM address to switch back to that
  area. If you are using an Atari TT or other 68020- or 68030-based
  machine, then you will be able to examine any address all the way up to
  0xFFFFFFFF, in one continuous block. For a list of ways to get around in
  memory quickly, refer to the section called "Getting from Here to There".

    In Memory mode, you can click on the scroll bar to the right of the
  display to move up or down in 128 byte intervals. Clicking on the arrow
  buttons moves the address up or down 16 bytes at a time. You can also
  drag the slider to a new position. You can select any portion of this
  scroll bar and hold the left mouse button down to repeat the function
  continuously.


  The File Editor:

    Whenever you select this option, the item selector appears. You can
  then select any existing file to edit.

    After you have selected a file, the display will show the contents of
  the file from its beginning. You can move to any location within the file
  using the scroll bar and edit by clicking on the appropriate location in
  the display.

    An offset into the file, relative to the strat can also be entered by
  selecting either the Hex or Decimal editable field.

    You can select the editable filename field if you want to switch to
  another file contained in the same directory as the previous one. This is
  sometimes easier than calling up the selector, which can be done by
  clicking on the "Selector..." button. If the new file is not found or it
  cannot be opened for both read and write, the program will try to re-open
  the previous file. If this fails, the file selector will appear once
  again. This time, if the file you select is not available, you will be
  returned to the Memory editor.

    If you close the MemFile window or leave the GEM program you are
  currently in with the window open, the file will also be closed.
  However, the next time you select MemFile from the menu bar it will try
  to re-open the file you were working on last. If this is file is on a
  floppy, you should make sure the same floppy is still in the appropriate
  drive. Please note that this also means that the system considers a file
  to be open as long as you are in the File mode. If you want to be able to
  access the file from any other application without risking possible
  dmamge to its contents, you must either leave the File option or close
  the MemFile window first.

    Because MemFile cannot actually increase or decrease the length of a
  file, attempting to edit a zero-length file will cause an error message
  to be appear.

    In this mode, clicking on the arrow keys has the same effect as
  clicking on the gray area of the scroll bar, the display moves forward or
  backward 128 bytes.


  The Sector Editor:

    When you first choose this option, the bootsector of the drive your
  system was started with is displayed. Also, some statistics about the
  disk in that drive are shown at the very bottom of the window.

    You can switch to a new sector on the disk by clicking on either of the
  + or - buttons to change the sector one at-a-time, or by clicking on the
  number of sectors itself to edit it. To switch to another disk drive,
  select the "Drives..." button. A dialog containing the valid drives in
  boldface will appear. Click on any of the active drives to begin editing
  sectors on that disk. If MemFile encounters any problem when reading or
| writing a sector, or if you select the Cancel button in this dialog, you
  are returned to the Memory editor. Only those disks whose boot sectors
| report them as having no more than 1024 bytes per sector can be viewed.

    The "Swap" button forces the program to re-read the bootsector of the
  active drive. This is useful when switching disks in a floppy drive.  If
  you do not use this function (or re-select the current drive from the
  Drives menu) the program may try to access a sector on the disk which
  does not exist, causing an error.

    MemFile always tries to read the "boot sector" of the disk whenever a
  new disk is inserted or a new drive is selected. If it cannot read the
  boot sector, the program displays an error message and displays the
  Drives list so you can either insert a disk whose boot sector is good, or
  switch to another drive alltogether.

    Here, the scroll bar can be used to move the display throughout a given
  sector and an offset within the sector can be entered in either the
  Decimal or Hex editable field.


  Keyboard Commands:
 
    Many of the main functions can be activated using simple keystrokes
  rather than by using the mouse. This is especially useful with the scroll
  bar, as it can be moved repeatedly until both Shift keys are pressed,
  rather than having to hold-down the mouse button for long periods of
  time.

    A complete keyboard map is available by selecting the button from the
  MemFile window, or by pressing the Help key.

  
  Getting from Here to There:
  
    There are several ways you can get quickly from one place to another in
  MemFile. You can hold down the right mouse button while clicking on a
  byte in the hex column of the display in order to treat that byte and the
  three following it as the longword address for the next offset to
  display. An example of this would be clicking on the byte at 0x4F2 to
  quickly jump to the start of the ROMs. This works in all three editing
  modes, though is probably not very useful in Sector mode.

    The function keys on the keyboard can have offsets assigned to them.
  If you hold down one of the Shift keys while pressing a function key, then
  MemFile will remember the lowest address currently in the display. Pressing
  the same function key without Shift will cause the display to jump to that
  location. This works in all three editing modes.

    When in Memory editing mode, you can press the "R" key on the keyboard
  to toggle between ROM and RAM addresses. If you are at an address in
  ROM, then pressing the key will move you to the beginning of RAM, if in
  RAM it will move you to the start of ROM.

    Another useful key, which works in any mode, is the Backspace key. It
  automagically moves you to the last offset before either a new offset was
  entered manually, or one of the features listed above was used to jump to
  a new location.
  

  Comments:

    Note that the scroll bar in File Editing mode only moves in increments
  of 128 bytes. This is due to the the small buffer that MemFile uses.

    For some reason, the memory locations between the highest RAM address
  and 0x3FFFFF do weird things on a normal ST. If you try to edit the
  locations, some of the bytes appear to change just by moving the cursor
  over them. This is because MemFile is somehow fed conflicting information
  when displaying and beginning the edit. Don't worry about it, you can't
  change these locations anyway and they really are quite useless. They
  just sit there and look nice.

    This program works only in medium and high resolution for obvious
  reasons. Booting in low-rez will not result in any message, however even
  though the program does not run, it is still in memory just sitting there
  doing nothing. Thought you might like to know where that memory is
  disappearing to.

    The Search feature can be rather slow, though it has sped up since
  previous versions.


  If you have any comments or suggestions please feel free to send them to
either of the addresses below.

                                Internet: gribnif@genie.geis.com
                                GEnie:    GRIBNIF

                                Gribnif Software
                                P. O. Box 779
                                Northampton, MA  01061
                                (413) 247-5620
