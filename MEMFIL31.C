/* 12/01/87  rev  11/26/88 rev 11/25/90	rev 7/15/95
***problem w/ address sprintf? ***
1k sectors

                            MemFile 3.1 by Dan Wilga

        This program and its resource may be freely distributed on the
        conditions that a copy of the README file is included and no payment
        of any type (including "copying" fees) is incurred upon the
        recipient of the program. No warranty is made as to the compatibility
        of this program and the author cannot be considered responsible for
        any loss of data which results from the use of this program.
        Use it at your own risk!

        This program, its resource file, and documentation are copyright
        1988 by Dan Wilga. This code may not be incorporated, either in
        its entirety or in part, in any product produced for profit
        without the written permission of Dan Wilga. It is also the wish
        of the author that this code be used solely as a guide in
        programming, not as a basis for other versions of MemFile. If you
        have suggestions for improvements, please feel free to suggest
        them.

        This code was written for Mark Williams C.
        Compile with the line:  cc -VGEMACC -o memfil30.acc
        
        Be sure to also include -VSMALL if you have version 3.0 or greater
        of MWC.
        

     A few random comments:
     
          Internally, the major difference between editing a sector or a file
          as opposed to straight memory is actually rather minimal. Instead of
          working with a large area in RAM or ROM, the program merely acts as
          though you were editing a portion of memory that just happens to be
          where the file or sector is stored. Simple, eh? Well, not quite. It's
          a little more complicated in that the "end" (max_addr) is actually
          considered to be some ficticious address which is the buffer start
          plus the file length or all the bytes on a disk.
          
          You will notice that the window does not use a form_do() call.
          This is because using form_do() would not allow anything to go on
          outside of the MemFile window. Instead, the mouse state is constantly
          polled by the event manager when the window is open. One problem that
          the previous version had was that whenever an object (such as a disk
          drive icon) was dragged over the MemFile window and then released,
          MemFile instantly thought you wanted to go into editing mode and
          activated the cursor. This was corrected by rather laboriously
          polling the mouse to see if it is inside or outside the MemFile
          window. That way, an object can be selected only when the mouse is
          first placed over the window with the button up and later pressed.
          
          The ST's memory map is pretty fragmented. There are many locations
          that cannot even be looked at without causing a processor exception.
          This is why the ROM addresses are so strange in their range.
          
          Oftentimes when an unexpected error occurs, MemFile returns to the
          memory editor. Memory is always there; a disk might not be.
          
          Throughout the program, Floprd() and Flopwr() are used for floppies
          and Rwabs() is used for anything else. This is because TOS seems to
          do an especially bad job of handling media change when it comes to
          Rwabs() and the Flop's seem to always work as correctly as one would
          expect from TOS.
          
          Whenever an editable field is selected, a form with an invisible box
          is actually displayed on top of the location of the field. It only
          looks like you're editing the one on the form. It also contains a
          very small button that exits the dialog when you hit Return.
          
          Adding keyboard commands was a real kludge. The scan code is first
          compared to values in a lookup table. If there is a match, then
          a few values are set and control passes to the section of main()
          that handles the buttons if they are pressed on the dialog itself.
*/
#include "memfil31.h"
#include "new_aes.h"
#include "vdi.h"
#include "tos.h"
#include "string.h"
#include "linea.h"
#include "stdlib.h"
typedef struct { int x, y, w, h; } Rect;
typedef struct
{
  int sides, bps, tps, spt;
  unsigned long secs;
} Bsec;
#include "memfilid.h"
#define TURBO_C
#include "neo_acc.h"
#include "multevnt.h"

#define SEC_SIZE  1024
#define Xrect(r) (r).x, (r).y, (r).w, (r).h
#define rindex strrchr
#define _PHYSTOP ((long)*((long *)0x42E))
#define MAX_RAM0  0x3FFF80L   /* highest possible RAM address for 68000/010 */
#define MAX_RAM2  0xFEFFFF80L
#define ROM_END   0xFFFF80L
#define MAX_COUNT 250                        /* number of events to wait */
#define KEYS      18                         /* keys that are mapped */
#define RING_BELL Crawio(7)
#define MIN_SLID  5
#define ULONG     (unsigned long)

OBJECT *form, *editor, *drives, *search, *map,
       *copy, *fnames;                      /* resource dialogs */
Rect windo, center, ed, drv, srch, map_rect,
       copy_rect, fnames_rect;              /* positions of the window itself */
                                            /* and the OBJECTs */
union
{
  unsigned int i;
  unsigned char c[2];
}cat;                                    /* used to convert 8088-style words */

int dum=0,
    hirez,                                   /* text height==16 */
    apid,
    text_h,                                  /* height of text in rasters */
    w_handle = -1,                           /* window handle (none to start) */
    hex,                                     /* flag is set if user wants hex */
    option = MEMORY,                       /* selects memory, file, or sector */
    f_handle = -1,                           /* handle of open file */
    drive,                                   /* drive currently in use */
    copysrc = COPSM,
    copydest = COPDM,
    cliparray[4], *cur_clip,
    retries,
    readable[8],
    pxarray[4];

#define iobuf (char *)_iobuf

unsigned int sector=0,                       /* sector being edited */
             edit1=ADDRHEX, edit2=ADDRDEC,  /* editable fields that are valid */
             last_slid=-1,
             _iobuf[SEC_SIZE/2],                     /* the one and only i/o buffer */
             maybe=0;                        /* true if address is > phystop */

char copysrcf[120],
     copydestf[120],
     should_redraw,
     processor,
     bus_err,
     opened,
     *mem=0L,                                /* starting addr being displayed */
     *start=0L,                              /* lowest address displayable */
     num_str[120], num2_str[50], num3_str[15],/* scratch */
     name[]="MemFile 3.1 by Dan Wilga", path[80], filename[13],
     title[50],                              /* string in window title bar */
     alter[]="These addresses might not be alterable",
     nums[]="0123456789ABCDEF",
     *old_s_ptr;                             /* position where search stopped */

unsigned long phystop,
              max_addr,                      /* highest displayable address */
              stack,                         /* used in Sup_bus() to hold stack */
              offset,                        /* into the file being edited */
              oldoff,
              file_len, oldmax, oldmem,
              rom_start,
              fkey[10],
              last_addr,
              max_ram;

Bsec boot;
FONT_HDR *fontp;
MFDB fdb1;                                   /* default to all zeros */
int neo_apid;
NEO_ACC *neo_acc;

long Sup_bus(void *stack);
  
/*********************************************************************
 This is a stripped-down sprintf() taken mostly from Mark Johnson's public
 domain C compiler. Mark *Williams* doesn't let you use any stdio from within
 a desk accessory, and hell, theirs is too long for what needs to be done
 here, anyway.
*/
/* this is the actual sprintf() entry point */
void cdecl spf(char *buf, char *fmt, ...) {
  void cdecl dopf(char *buf, char *fmt, ...);
  
  dopf(buf, fmt, (unsigned int *)&...);
}
void cdecl dopf(char *buf, char *fmt, ...) {
  char **pp, *ps, pad, lj, sign, larg;
  unsigned long n, *lp;
  register int c, w;
  unsigned int *ap = *(unsigned int **)...;
  char *pn( char *b, unsigned long n, int base, int w, int sign, int pad,
      int lj );
  
  while( (c = *fmt++) != 0 ){
    if (c == '%') {
      if( (lj = ((c=*fmt++) == '-')) != 0 ) c = *fmt++;
      if( c == '0' ){
        pad = '0';
        c = *fmt++;
      }
      else pad = ' ';
      w = 0;
      sign = 1;
      while (c >= '0' && c <= '9'){
        w = w*10 + c-'0';
        c = *fmt++;
      }
      if( (larg = (c == 'l')) != 0 ) c = *fmt++;
      switch (c) {
        case 'c': *buf++ = *ap++; 
            break;
        case 's': pp = (char **) ap;
            ps = *pp++;
            w -= strlen(ps);
            if( !lj ) while (w-- > 0) *buf++ = pad;
            ap = (unsigned int *) pp;
            while (*ps) *buf++ = *ps++;
            if( lj ) while (w-- > 0) *buf++ = pad;
            break;
        case 'u':
        case 'x': sign=0; goto do_pn;
        case 'U':
        case 'X': sign=0;
        case 'D': larg++;
        case 'd': case 'o':
do_pn:      if (larg) {
            lp = (unsigned long *)ap;
            n = *lp++;
            ap = (unsigned int *)lp;
            }
            else n = (long)(signed) *ap++;
            buf = pn(buf, n, c, w, sign, pad, lj);
            break;
        default:  *buf++ = c; 
            break;
      }
    }
    else  *buf++ = c;
  }
  *buf = 0;
}
char *pn( char *b, unsigned long n, int base, int w, int sign, int pad, int lj )
{
  int i;
  char nb[20];
  switch (base) {
  case 'o':
    base = 8;
    break;
  case 'x':
  case 'X':
    base = 16;
    break;
  default:
    base = 10;
  }
  i = 0;
  if( !n ) nb[i++] = '0';
  else
  {
    if( sign && (n&0x80000000L) ){
      n = (unsigned long)(-(long)n);
    } else sign = 0;
    for (; n; n /= base)
      nb[i++] = nums[n%base];
    if( sign ) nb[i++] = '-';
  }
  w -= i;
  if( !lj ) while (w-- > 0) *b++ = pad;
  else while (w-- > 0) nb[i++] = pad;
  while (i--) *b++ = nb[i];
  return b;
}
/*********************************************************************/
int getcookie( long num, long *val )
{
  struct Cookie { long c, v; } *cookie;
  
  cookie = *(struct Cookie **)0x5a0;
  if( cookie )
    do
      if( cookie->c == num )
      {
        *val = cookie->v;
        return(1);
      }
    while( (cookie++)->c );
  return(0);
}
/*********************************************************************/
void ack( int *buffer )                          /* acknowledge NeoDesk's message */
{
  int buf[8];
  
  buf[0] = DUM_MSG;                     /* always ack with this message */
  buf[1] = apid;
  if( neo_apid<0 ) neo_apid = buffer[1];
  appl_write( neo_apid, 16, buf );
}
/********************************************************************/
char *pathend( char *ptr )
{
  char *p;

  return( (p=rindex(ptr,'\\'))==0 ? ptr : p );
}
/***********************************************************************
  Copy one area on the screen to another for scrolling
*/
#define COPYTRAN Linea->copy_tran
#define CONTRL   Linea->contrl
#define INTIN    Linea->intin
#define PTSIN    Linea->ptsin

void blit( Rect *box1, Rect *box2 )
{
  union
  {
    int i[2];
    MFDB *l;
  } u;
  
  COPYTRAN = 0;
  u.l = &fdb1;
  CONTRL[3] = 1;
  CONTRL[7] = CONTRL[9] = u.i[0];
  CONTRL[8] = CONTRL[10] = u.i[1];
  PTSIN[2] = (PTSIN[0] = box1->x) + box1->w;
  PTSIN[3] = (PTSIN[1] = box1->y) + box1->h;
  PTSIN[6] = (PTSIN[4] = box2->x) + box2->w;
  PTSIN[7] = (PTSIN[5] = box2->y) + box2->h;
  INTIN[0] = 3;
  CONTRL[1] = 4;
  hide_mouse();
  copy_raster();
  show_mouse(0);
}
/***********************************************************************
  Convert an absolute sector number to a side, sector, and track number
*/
void calc_sec( Bsec *boot, unsigned int sector, int *s, int *sec, int *trk )
{
  *s = sector / boot->spt % boot->sides;
  *trk = sector / boot->spt >> (boot->sides-1);
  *sec = (sector % boot->spt) + 1;
}
/***********************************************************************
  Decide whether or not we may be looking at an area in memory that
  cannot be changed
*/
void check_mem(void)
{
  int old;
  
  old = maybe;
  maybe = option==MEMORY && ULONG mem > phystop /*&& ULONG mem < rom_start*/;
  if( maybe != old ) reset_title();
}
/***********************************************************************/
int form_alert1( char *s )
{
  return form_alert( 1, s );
}
int test_berr(void)
{
  if( bus_err )
  {
    bus_err=0;
    form_alert1( "[1][This operation cannot|be performed on one or|more of \
the addresses|in the display][Ok]" );
    return(1);
  }
  return(0);
}
/***********************************************************************
  Move the cursor around and do any editing
*/
void cursor( int box, int m_x, int m_y )
                /* which box was clicked-on, mouse position */
{
  unsigned char temp[128], *t, ch;               /* both temporaries */
  char *p, *c, had_berr, changed[8][16];
  int i, y0,
      x, y,                                /* grid (not raster) coord of curs */
      off,                                   /* value under cursor */
      hi,                                    /* is curs on the high nybble? */
      curs[4],                               /* raster coords of cursor */
      offx1, offx2, x1, x2,
      x_max, y_max, rable;                   /* maximum curs column, row */
  long key;                                  /* key pressed */
  
/**  if( ULONG mem >= rom_start && ULONG mem <= ROM_END+0x7f )
  {
    error( "|Heh! I only wish you|*could* modify the ROM!" );
    return;
  }**/
  stack = Sup_bus( 0L );                       /* can't read low memory unless */
  for( i=128, t=temp, p=mem-offset; --i>=0; )
    *t++ = *p++;
  Sup_bus( (void *)stack );                    /* restore stack pointer */
/***  if( test_berr() ) return; ***/
  memset( changed, 0, sizeof(changed) );
  objc_offset(form, BOX1, &x1, &y0);
  objc_offset(form, BOX2, &x2, &dum );
  if( box == BOX1 )                         /* editing the hex data */
  {
    x = ( m_x - x1 ) / 18;                  /* figure out what byte mouse was */
    hi = (m_x - x1 - 18 * x) < 7;           /* pressed-on; set high nybble flag */
  }
  else x = ( m_x - x2 ) / 8;
  y0++;
  y = ( m_y - center.y - form[box].ob_y - 2 ) / text_h;
  if( x < 0 ) x = 0;
  if( x > 15 ) x = 15;
  if( y < 0 ) y = 0;
  if( y > 7 ) y = 7;
  wind_update( BEG_UPDATE );               /* lock-out any other applications */
  cur_clip = cliparray;                    /* reset clipping */
  _set_clip();
  hide_mouse();
  num_str[1] = '\0';
  offx1 = x * 18 + x1;                     /* calculate initial cursor pos. */
  offx2 = x * 8 + x2;
  curs[0] = curs[2] = box==BOX1 ? offx1 + 8 * !hi : offx2;
  curs[1] = curs[3] = y * text_h + y0 - 1;
  curs[2] += 7;
  curs[3] += text_h - (hirez ? 1 : 3);     /* height is resolution dependent */
  off = x + 16 * y;
  x_max = 15;
  y_max = 7;
  if( option == FILE )
    if( (long) mem - (long) start + off > file_len )  /* past end of file */
    {
      show_mouse(0);
      wind_update( END_UPDATE );
      RING_BELL;
      return;
    }
    else
    {
      key = (long) start + file_len - (long) mem;
      if( key<128L )                              /* not all lines are full */
      {
        y_max = key>>4;                          /* figure-out where data end */
        x_max = key%16;
      }
    }
  set_wrt_mode(2);                                /* draw in XOR mode */
  draw_box( curs );                               /* draw the cursor */
  while( (i = ((key = Bconin(2)) >> 16) & 0xFF) != 0x1C && i != 0x72 )
                              /* continue until Return or Enter is pressed */
  {
    rable = readable[y]&(1<<x);
    switch( i )                                   /* isolate the scan code */
    {
      case 0x48:                                  /* up arrow */
        y--;
        check( &x, &y, x_max, y_max );
        break;
      case 0x50:                                  /* down arrow */
        y++;
        check( &x, &y, x_max, y_max );
        break;
      case 0x4D:                                  /* right arrow */
        if( box == BOX2 || !hi )     /* either editing ASCII or on low nybble */
        {
          x++;
          if( x > x_max ) x = 0;          /*   <---- "check" gets confused */
          check( &x, &y, x_max, y_max );          /* and tries to return to */
        }                                         /* the top otherwise */
        if( box == BOX1 ) hi = !hi;
        break;
      case 0x4B:                                  /* left arrow */
left:   if( box == BOX2 || hi )
        {
          x--;
          check( &x, &y, x_max, y_max );
        }
        if( box == BOX1 ) hi = !hi;
        break;
      default:
        ch = (char) key;                          /* isolate ASCII value */
        if( box == BOX1 )                         /* editing hex */
          if( i>=0x63 && i<=0x66 ) ch = i - 0x22; /* convert top keypad key*/
          else if( i==0x4A ) ch = 'E';            /* - keypad key */
          else if( i==0x4E ) ch = 'F';            /* + keypad key */
          else if( i==0xE ) goto left;		  /* BS key */
        if( ch && rable )                         /* has to be some char by now */
	  if( box == BOX2 )                       /* editing ASCII */
          {
            if( i==0x71 ) ch = 0;                 /* keypad period becomes NUL */
            temp[off] = ch;
            changed[y][x] = 1;
            if( ++x>x_max && y==y_max || x>15 )   /* advance cursor */
            {
              y++;
              x = 0;
              check( &x, &y, x_max, y_max );
            }
          }
          else                                      /* editing data in hex */
          {
            if( strchr("abcdef", ch) ) ch -= 'a'-'A';             /* convert a-f to A-F */
            if( (p=strchr(nums, ch)) != 0 )
            {
              ch = p-nums;		      /* convert hex digit to dec */
              temp[off] =                        /* change appropriate nybble */
                   hi ? (ch<<4) | (temp[off]&0x0F) : (temp[off]&0xF0) | ch;
              changed[y][x] = 1;
              if( !hi )                               /* advance cursor */
                if( ++x>x_max && y==y_max || x>15 )
                {
                  y++;
                  x = 0;
                  check( &x, &y, x_max, y_max );
                }
              hi = !hi;
            }
          }
        break;
    }
    if( rable )
    {
      to_hex( ch = temp[off] );
      init_text();
      put_str( offx1, curs[1], num_str );           /* draw new hex value */
      if( !ch ) ch = '.';                           /* is this a NUL?? */
      num_str[0] = ch;
      num_str[1] = '\0';
      put_str( offx2, curs[1], num_str );           /* draw new ASCII value */
    }
    else draw_box( curs );                        /* undraw cursor */
    offx1 = x * 18 + x1;                          /* calc new curs pos */
    offx2 = x * 8 + x2;
    curs[0] = curs[2] = box==BOX1 ? offx1 + 8 * !hi : offx2;
    curs[1] = curs[3] = y * text_h + y0 - 1;
    curs[2] += 7;
    curs[3] += text_h - (hirez ? 1 : 3);
    off = x + (y<<4);
    set_wrt_mode(2);                              /* XOR mode */
    draw_box( curs );                             /* draw new cursor */
  }
  if( readable[y]&(1<<x) )
  {
/*    set_wrt_mode(0);                                /* return to replace mode */*/
    ch = temp[off];                                 /* you've seen it all before */
    to_hex( ch );
    put_str( offx1, curs[1], num_str );
    if( !ch ) ch = '.';
    num_str[0] = ch;
    num_str[1] = '\0';
/*  set_wrt_mode(0); */
    put_str( offx2, curs[1], num_str );
  }
  else draw_box( curs );                        /* undraw cursor */
  show_mouse(0);
  wind_update( END_UPDATE );
  if( form_alert( (Kbshift(-1)&3) ? 1 : 2, "[0][Write changes?][Yes|No]" ) == 1 )
  {
    stack = Sup_bus( 0L );
    for( i=128, had_berr=0, t=temp, p=mem-offset, c=changed[0]; --i>=0; t++, p++ )
      if( *c++ && *p != *t )
        if( bus_err )
        {
          had_berr = 1;
          bus_err = 0;
        }
        else *p = *t;                 /* update the real memory, but only if changed */
    Sup_bus( (void *)stack );
/*    if( test_berr() ) return; */
    if( option == MEMORY && ((long) mem > phystop || had_berr) )
         display();          /* since changes probably didn't work, redisplay */
    if( option == FILE )                          /* rewrite the file portion */
    {
      key = file_len - offset < SEC_SIZE ? file_len - offset + 1L: SEC_SIZE;
      Fseek( offset, f_handle, 0 );               /* jump to start of portion */
      if( Fwrite( f_handle, key, start ) != key )
      {
        error( "|Error rewriting the file!" );
        goto_mem();                               /* return to memory editor */
        display();
      }
    }
    if( option == SECTOR )
      if( !write_one_sector( &boot, sector, drive, iobuf ) )
      {
        goto_mem();
        display();
      }
  }
  else display();
}
/**********
  Make sure the cursor stays "in bounds"
*/
void check( int *x, int *y, int x_max, int y_max )
{
  if( *y>y_max-(*x>x_max) ) *y = 0;
  else if( *y<0 ) *y = y_max - (*x>x_max);
  if( *x>x_max && *y==y_max || *x>15 ) *x = 0;
  else if( *x<0 ) *x = *y<y_max ? 15 : x_max;
}
/***********************************************************************
  Redraw the entire display area
*/
void display(void)
{
  init_text();
  redo_addr( 0, 7 );     /* redraw all eight lines in the display */
  redo_text( 0, 7 );
  slider( mem );         /* set the slider */
}
/********************************************************************/
void prn_buf( char *buf, unsigned long inlen, unsigned long *outlen )
{
  static unsigned long addr, addr0, left;
  static int bufleft;
  static unsigned char *iptr, *bptr, bbuf[75], cbuf[17], *cptr;
  unsigned char *ptr, *ptr2;
  
  if( !buf )
  {
    addr = addr0 = inlen;
    left = 0L;
    *(bptr = bbuf) = *cbuf = '\0';
  }
  else
  {
    bufleft = SEC_SIZE;
    ptr = iobuf;
    if( !left && !*bptr )
    {
      left = inlen;
      iptr = buf;
    }
again:
    while( left || *bptr )
    {
      if( !*bptr )
      {
        *(bptr = bbuf) = '\0';
        if( !((addr-addr0)&0xF) ) spf( bbuf, "  %s\r\n%08X: ",
            cptr=cbuf, addr );
        *cptr++ = *iptr<' ' || *iptr>='\x7f' ? '.' : *iptr;
        spf( bbuf+strlen(bbuf), " %02x", *iptr++ );
        addr++;
        left--;
      }
      *ptr++ = *bptr++;
      if( !--bufleft ) break;
    }
    if( buf == (char *)-1L )
    {
      ptr2 = bptr = bbuf;
      while( (addr-addr0)&0xF )
      {
        *ptr2++ = *ptr2++ = *ptr2++ = ' ';
        addr++;
      }
      *cptr++ = '\0';
      spf( ptr2, "  %s\r\n", cbuf );
      buf = 0L;
      goto again;
    }
    *outlen = SEC_SIZE-bufleft;
  }
}
void but_up(void)
{
  int dum, but;
  
  do
    graf_mkstate( &dum, &dum, &but, &dum );
  while( but&1 );
}
void do_copy(void)
{
  int done=0, i, button, shand, dhand, backward;
  register int err, dsec;
  unsigned long cstart, cend, clast, cstartsec, cendsec, clastsec, dflen,
      addrmax, addrmin, fillch;
  register unsigned char *addr, *ptr;
  unsigned long len, len2;
  char fname[13], *ptr1, *ptr2, sdrv, ddrv, buf2[SEC_SIZE], raw, fill, last_prn;
  Bsec sboot, dboot;
  static char errmsg[]="|The %s %s is|%s", src[]="source", dest[]="destination",
      end[]="end", /* *strt="start", badmem[]="in a bad memory location", */
      less[]="less than the start", drverr[]="|Invalid %s|drive letter",
      /* secerr[]="The %s %s offset|is greater than 0x1FF",
      endoff[]="ending offset", */
      endsec[]="ending sector", not_hex[]="The source and destination|cannot \
be the same when|using Hex Dump",
      equal[]="|The source and destination|are the same";
  
  copy[copysrc].ob_state = copy[copydest].ob_state |= SELECTED;
  while( !done )
  {
    form_dial( FMD_START, 0, 0, 0, 0, Xrect(copy_rect) );
    button = make_form( copy, copy_rect, copysrc+1, 0 );
    for( i=COPSM; i<=COPSS; i++ )
      if( copy[i].ob_state & SELECTED ) copysrc = i;
    for( i=COPDM; i<=COPDP; i++ )
      if( copy[i].ob_state & SELECTED ) copydest = i;
    shand = dhand = backward = 0;
    switch( button )
    {
      case COPYIT:
        err=1;
        raw = copy[COPFRAW].ob_state&SELECTED;
        fill = copy[COPFLYES].ob_state&SELECTED;
        if( !from_hex( copy[COPFLCH].ob_spec.tedinfo->te_ptext, &fillch ) )
            switch( copysrc )
        {
          case COPSM:
          case COPSF:
            if( !from_hex( copy[copysrc+1].ob_spec.tedinfo->te_ptext,
                &cstart ) )
              if( !from_hex( copy[copysrc+2].ob_spec.tedinfo->te_ptext,
                  &cend ) )
                if( cstart > cend ) cerror( errmsg, src, end, less );
                else err=0;
            if( !err && copysrc==COPSF )
              if( (shand=Fopen(copysrcf, 0)) < 0 )
              {
                err++;
                error( "|Could not open|the source file!" );
              }
              else if( (len=Fseek( cstart, shand, 0 )) != cstart )
              {
                err++;
                read_error( len==-64 ? 0L : len );
              }
            addrmin = cstart;
            addrmax = cend;
            break;
          default:
            cstart = 0L;
            cend = SEC_SIZE-1L;
            if( (sdrv=check_drv(COPSSDR))==0 ) cerror( drverr, src );
/****       else if( !from_hex( copy[COPSSO1].ob_spec.tedinfo->te_ptext,
                &cstart ) )
              if( !from_hex( copy[COPSSO2].ob_spec.tedinfo->te_ptext,
                  &cend ) )
                if( cstart > 0x1FF ) cerror( secerr, src, strt );
                else if( cend > 0x1FF ) cerror( secerr, src, dest );****/
                else
                {
                  cstartsec = atol( copy[COPSSS].ob_spec.tedinfo->te_ptext );
                  cendsec = atol( copy[COPSSE].ob_spec.tedinfo->te_ptext );
                  if( cstartsec > cendsec ) cerror( errmsg, src, endsec, less );
/****             else if( cstartsec==cendsec && cstart > cend )
                      cerror( errmsg, src, endoff, less );****/
                  else if( read_bootsec( sdrv-='A', &sboot ) ) err=0;
                }
            addrmin = cstartsec;
            addrmax = cendsec;
        }
        if( !err )
        {
          err=1;
          switch( copydest )
          {
            case COPDM:
              if( !raw && copysrc==COPSM )
              {
                cerror( not_hex );
                break;
              }
            case COPDF:
              if( !from_hex( copy[copydest+1].ob_spec.tedinfo->te_ptext,
                  &clast ) ) err=0;
              if( !err )
                if( copysrc==copydest-COPDM+COPSM &&
                    (copydest==COPDM || !strcmp(copysrcf,copydestf)) )
                  if( cstart==clast )
                  {
                    err++;
                    error( equal );
                  }
                  else if( !raw && copydest==COPDF ) cerror( not_hex );
                  else backward = clast > cstart;
              if( !err )
                if( copydest==COPDF )
                {
                  if( (dhand=Fopen(copydestf,2)) == -33 )
                      dhand=Fcreate(copydestf,0);
                  if( dhand < 0 )
                  {
                    err++;
                    open_error( "destination" );
                  }
                  else
                  {
                    dflen = Fseek( 0L, dhand, 2 );
                    dum=0;
                    for( len=dflen; len<clast && !err; len++ )
                      if( (len2=Fwrite(dhand,1L,&dum)) != 1 )
                      {
                        write_error(len2);
                        err++;
                      }
                    Fseek( clast, dhand, 0 );
                  }
                }
              break;
            case COPDS:
              clast = 0L;
              if( !raw ) cerror( "|You cannot copy to disk|sectors in Hex mode" );
              else if( (ddrv=check_drv(COPDSD)) == 0 ) cerror( drverr, dest );
/****         else if( !from_hex( copy[COPDSO].ob_spec.tedinfo->te_ptext,
                  &clast ) )
                if( clast > 0x1FF ) cerror( secerr, dest, strt ); ****/
                else
                {
                  clastsec = atol( copy[COPDSS].ob_spec.tedinfo->te_ptext);
                  ddrv -= 'A';
                  if( read_bootsec( ddrv, &dboot ) )
                    if( copysrc==COPSS && sdrv==ddrv )
                      if( /*cstart==clast &&*/ clastsec==cstartsec ) error( equal );
                      else
                      {
                        backward = clastsec>cstartsec;
                        err=0;
                      }
                    else err=0;
                }
                break;
            case COPDP:
              err=0;
              while( !err && !Bcostat(0) )
                if( form_alert1( 
                    "[1][|Printer not ready!][Retry|Cancel]" ) == 2 ) err++;
          }
        }
        if( !err && !raw && copydest==COPDM )
          if( form_alert1( "[1][Are you sure you want to|copy to \
memory as a Hex Dump|rather than as raw ASCII?][Do It!|Cancel]" ) == 2 ) err++;
        if( !err && copydest==COPDS )
          if( form_alert1( "[1][Copying to a disk's sectors|can be very \
dangerous.|Are you really sure?][Do It!|Cancel]" ) == 2 ) err++;
        addr = (char *)(backward ? addrmax : addrmin);
        if( copydest==COPDS ) dsec = (long)addr - addrmin + clastsec;
        if( (last_prn = !raw) != 0 ) prn_buf( 0L, copysrc==COPSS ? 0L :
            (long)addr, 0L );
        while( !err && ((!backward ? (long)addr<=addrmax :
            (long)addr>=addrmin && (long)addr!=0xFFFFFFFFL) || last_prn) )
        {
          if( !backward ) len = len2 = (long)addrmax-(long)addr < SEC_SIZE ? 
              addrmax-(long)addr+1 : SEC_SIZE;
          else len = len2 = (long)addr-(long)addrmin < SEC_SIZE ? 
              (long)addr-addrmin+1 : SEC_SIZE;
          if( len ) switch( copysrc )
          {
            case COPSM:
              stack = Sup_bus(0L);
              if( backward )                         /* memory-to-memory */
              {
                ptr = ptr2 = (char *)(cend - cstart + clast);
                while( (long)addr>=addrmin && (long)addr!=-1L ) *ptr-- =
                    *addr--;
                if( fill && !bus_err ) memset( addr+1, (int)fillch,
                    ptr2-ptr );
              }
              else if( copydest==COPDM )             /* ditto */
              {
                ptr = (char *)clast;
                len=addrmax-(long)addr+1;
                goto mem_copy;
              }
              else
              {
                ptr = iobuf;
mem_copy:       memcpy( ptr, addr, len );
                if( fill ) memset( addr, (int)fillch, len );
                addr+=len;
              }
              Sup_bus((void *)stack);
              if( test_berr() ) err++;
              break;
            case COPSF:
              if( copydest == COPDM && raw )
              {
                len = cend - cstart + 1;
                ptr = (char *)clast;
                stack = Sup_bus(0L);
              }
              else
              {
                ptr = iobuf;
                if( backward ) Fseek( (long)addr-len, shand, 0 );
              }
              if( (len2=Fread( shand, len, ptr )) < 0 || len != len2 )
              {
                err++;
                read_error(len2);
              }
              else if( fill )
              {
                memset( buf2, (int)fillch, len );
                if( backward ) Fseek( (long)addr-len, shand, 0 );
                Fwrite( shand, len, buf2 );
              }
              if( copydest == COPDM && raw )
              {
                Sup_bus((void *)stack);
                if( test_berr() ) err++;
              }
              addr += backward ? -len : len;
              break;
            case COPSS:
              if( (err = !read_one_sector( &sboot, (int)addr, sdrv, iobuf ))
                  == 0 && fill )
              {
                memset( buf2, (int)fillch, SEC_SIZE );
                err = !write_one_sector( &sboot, (int)addr, sdrv, buf2 );
              }
              addr += backward ? -1 : 1;
              break;
          }
          if( !err || copysrc==COPSF && (len2 > 0L || last_prn) )
          {
            len = copysrc == COPSS ? SEC_SIZE : len2;
            ptr = iobuf;
            if( !raw )
              if( !len2 ) ptr2 = (char *)-1L;
              else memcpy( ptr2=buf2, ptr, len2=len );
            do
            {
              if( !raw )
              {
                prn_buf( ptr2, len2, &len );
                if( !len ) break;
                ptr = iobuf;
              }
              switch( copydest )
              {
                case COPDM:
                  if( !raw )
                  {
                    memcpy( (char *)clast, ptr, len );
                    clast += len;
                  }
                  break;
                case COPDF:
                  if( (len2=Fwrite( dhand, len, ptr )) < 0L || len2 != len )
                  {
                    err++;
                    write_error(len2);
                  }
                  break;
                case COPDS:
                  if( len==boot.bps || (err = 
                      !read_one_sector( &dboot, dsec, ddrv, buf2 )) == 0 )
                  {
                    memcpy( buf2, ptr, len );
                    err = !write_one_sector( &dboot, dsec, ddrv, buf2 );
                    dsec += backward ? -1 : 1;
                  }
                  break;
                case COPDP:
                  while( len-- ) Bconout(0,*ptr++);
                  break;
              }
              len2 = 0;
              if( ptr2==(char *)-1L )
              {
                last_prn = 0;
                ptr2 = buf2;
              }
            }
            while( !raw && !err );
          }
        }
        if( shand > 0 ) Fclose(shand);
        if( dhand > 0 ) Fclose(dhand);
        break;
      case COPGETFN:
        while( (i=make_form( fnames, fnames_rect, FNSRC, 0 )) != FNDONE )
          if( i==FNSSEL || i==FNDSEL )
          {
            strcpy( num_str, ptr1 = (i==FNSSEL ? copysrcf : copydestf) );
            ptr2 = pathend(num_str);
            strcpy( fname, (*ptr2=='\\') + ptr2 );
            strcpy( ptr2, "\\*.*" );
            fsel_input( num_str, fname, &button );
            if( button )
            {
              *(pathend(num_str)+1) = '\0';
              strcpy( ptr1, num_str );
              strcat( ptr1, fname );
            }
            objc_draw( copy, ROOT, MAX_DEPTH, Xrect(copy_rect) );
          }
          else
          {
            ptr1 = i==FNSMAIN ? copysrcf : copydestf;
            strcpy( ptr1, path );
            strcat( ptr1, filename );
            but_up();
          }
        break;
      default:
        done++;
    }
  }
  form_dial( FMD_FINISH, 0, 0, 0, 0, Xrect(copy_rect) );
  copy[copysrc].ob_state = copy[copydest].ob_state &= ~SELECTED;
  if( option==SECTOR )
    if( norm_bootsec() ) read_sector();
    else goto_mem();
  else if( option==FILE ) reopen_file();
}
/**********/
void read_error( long err )
{
  if( err<0L ) cerror( "|Error %D in|reading source", err );
  else error( "|End of source|file encountered" );
}
/**********/
void write_error( long err )
{
  if( err<0L ) cerror( "|Error %D in writing|destination", err );
  else error( "|Destination disk full.|File has been truncated." );
}
/**********/
void open_error( char *ptr )
{
  cerror( "|Could not open the %s|file in read/write mode", ptr );
}
/**********/
int check_drv( int index )
{
  register char *sdrv;
  
  sdrv=copy[index].ob_spec.tedinfo->te_ptext;
  return( *sdrv < 'Q' && (Drvmap() & (1L<<(*sdrv-'A'))) ? *sdrv : 0 );
}
/**********/
void cerror( char *msg, ... )
{
  char buf[150];
  
  dopf( buf, msg, (unsigned int *)&... );
  error( buf );
}
/*********************************************************************/
void in_field(void)
{
  int i;
  char *ptr, *ptr0;
  
  ptr0 = ptr = form[FILENAME].ob_spec.tedinfo->te_ptext;
  for( i=0; i<13 && filename[i]; i++ )          /* convert filename for editable field */
    if( filename[i] == '.' ) while(ptr-ptr0<8) *ptr++ = ' ';
    else *ptr++ = filename[i];
  *ptr = '\0';
}
/***********************************************************************
  Put-up the file selector
*/
int do_select(void)
{
  int button, flag;
  
  strcpy( num_str, path );
  strcat( num_str, "*.*" );
  should_redraw++;
  if( fsel_input( num_str, filename, &button ) && button )
  {
    flag = pathend(num_str) - num_str;
    num_str[++flag] = '\0';
    strcat( num_str, filename );                  /* append filename */
    if( do_file(1) )                              /* try to open file */
    {
      num_str[flag] = '\0';
      strcpy( path, num_str );
      in_field();
      return( 1 );
    }
    goto_mem();
    return( 0 );
  }
  return( 2 );
}
/*********************************************************************
  Open a file
*/
int do_file( int reset )
{
  int j;
  
  if( f_handle >= 0 ) Fclose( f_handle );         /* close old file */
  if( ( f_handle = Fopen( num_str, 2 ) ) < 0L )
  {
    open_error("");
    return( opened=0 );
  }
  else
  {
    if( (file_len = Fseek( 0L, f_handle, 2 )) < 0L )
    {
      open_error("");
      return(opened=0);
    }
    if( !file_len-- )
    {
      error( "|This is a zero-length file" );
      return( 0 );
    }
    start = iobuf;
    max_addr = file_len & 0xFFFFFF80L;                /* new highest address*/
    j = form[SLIDER].ob_height * 128L / (max_addr+128L);
    if( j < MIN_SLID ) j = MIN_SLID;   /* slider box must have *some* height */
    else if( j > form[SLIDER].ob_height ) j = form[SLIDER].ob_height;
    form[SLIDBOX].ob_height = j;
    last_slid = -1;
    max_addr += (long) start;
    if( reset || offset > max_addr-(long)start )
    {
      offset = 0L;                      /* display from start of file */
      mem = start;
    }
    Fseek( offset, f_handle, 0 );            /* position file */
    Fread( f_handle, SEC_SIZE, start );         /* and read in one buffer's worth */
    return( opened=1 );
  }
}
/********************************************************************/
void draw_box( int *arr )
{
  static long patptr = -1L;
  
  set_pattern( (int *)&patptr, 1, 0 );
  set_fg_bp(0);
  filled_rect( *arr, *(arr+1), *(arr+2), *(arr+3) );
}
/***********************************************************************
  Ring the bell and pop-up an alert box with an error message
*/
void error( char *str )
{
  char temp[120];
  
  spf( temp, "[1][%s][Ok]", str );
  RING_BELL;
  form_alert1( temp );
}
/***********************************************************************
  Is the mouse over an object in the window? If so, return its index
*/
int find_obj( int x, int y )
{
  return( objc_find( form, 0, MAX_DEPTH, x, y ) );
}
/********************************************************************/
int from_hex( char *ptr, unsigned long *num )
{
  int flag=0;
  register char c;
  
  *num = 0L;
  for( ; *ptr && !flag; ptr++ )
  {
    c = *ptr;
    if( c > 0x60 ) c -= 0x20;  /* a-f -> A-F */
    if( c < '0' || c > '9' && ( c < 'A' || c > 'F' ) )
    {
      flag++;
      error( "|A number in hex may only|contain 0-9 and A-F" );
    }
    else *num = *num * 0x10L + (unsigned)(c>'9' ? c-'A'+10 : c-'0');
  }
  return(flag);
}
/***********************************************************************/
void gray(void)
{
  int i, wh;
  
  wh = w_handle>0;
  objc_change( form, FILENAME, 0, Xrect(center), i = option!=FILE ?
      DISABLED : 0, wh );
  objc_change( form, SELECTOR, 0, Xrect(center), i, wh );
  for( i=SECMINUS; i<=DRIVE; i++ )
    objc_change( form, i, 0, Xrect(center), option!=SECTOR ?
        DISABLED : 0, wh );
}
/***********************************************************************
  Jump to the memory editor
*/
void goto_mem(void)
{
  if( option == FILE ) oldoff = ULONG mem - ULONG start;
  option = MEMORY;
/*  copysrc = COPSM;
  copydest = COPDM;*/
  objc_change( form, MEMORY, 0, Xrect(center), SELECTED, 1 );
  objc_change( form, FILE, 0, Xrect(center), 0, 1 );
  objc_change( form, SECTOR, 0, Xrect(center), 0, 1 );
  gray();
  edit1 = edit2 = 0;
  offset = 0L;
  (long) mem = oldmem;
  (long) start = oldmem >= rom_start && processor<2 ? rom_start : 0L;
  max_addr = oldmax;
  form[SLIDBOX].ob_height = MIN_SLID;
  last_slid = -1;
  check_mem();
}
/********************************************************************/
void init_text(void)
{
/*  FBASE = fontp->font_data;
  FWIDTH = fontp->font_width;
  DELY = fontp->font_height;
  TEXTFG = 15;
  TEXTBG = 0;
  STYLE = 0;
  WEIGHT = 1;*/
  if( !neo_acc ) set_text_blt( fontp, 0, 0, 0, 15, 0 );
}
/********************************************************************/
int load_rsrc( char *name )
{
  if( !rsrc_load( name ) )
  {
    cerror( "%s is not|in the current|directory!", name );
    return(0);
  }
  return(1);
}
/********************************************************************/
int _form_do( OBJECT *fo_dotree, int fo_dostartob )
{
  int r, o;
  KEYTAB *kt;
  static char off[]={ 0x63, 0x64, 0x65, 0x66, 0x4A, 0x4E };
  char temp[128*3], *ou, *os, *oc;
  
  kt = Keytbl( (void *)-1L, (void *)-1L, (void *)-1L );
  memcpy( temp, ou=kt->unshift, 128 );
  memcpy( temp+128, os=kt->shift, 128 );
  memcpy( temp+256, oc=kt->capslock, 128 );
  for( r=0; r<sizeof(off); r++ )
  {
    o = off[r];
    temp[o] = temp[o+128] = temp[o+256] = r+'A';
  }
  Keytbl( temp, temp+128, temp+256 );
  r = form_do( fo_dotree, fo_dostartob );
  Keytbl( ou, os, oc );
  return(r);
}
int make_form( OBJECT *tree, Rect tree_rect, int edit, int redraw )
{
  int ret;
  
  objc_draw( tree, ROOT, MAX_DEPTH, Xrect(tree_rect) );
  tree[ret=_form_do( tree, edit )].ob_state &= ~SELECTED;
  if(redraw) form_dial( FMD_FINISH, 0, 0, 0, 0, Xrect(tree_rect) );
  return(ret);
}
/***********************************************************************
  Move backward or forward the specified amount in memory and scroll the
  display, if needed
*/
void move( long num, int redraw, int pos )

/* signed amount to move the top of the display by */
/* true if the display should be redrawn */
/* true if moving positively */
{
  int i, j;
  register long n;
  Rect box1, box2;

  if( num )
  {
    if( option == FILE ) num = (num + 127L) & 0xFFFFFF80L;
    if( pos ) n = ULONG mem + num > max_addr ? max_addr - ULONG mem :
        num;                  /* make sure we stay within bounds */
    else if( start ) n = ULONG mem + num < ULONG start ? ULONG start - ULONG mem : num;
    else n = ULONG mem + num > ULONG mem ? ULONG start - ULONG mem : num;
    if( n )
    {
      if( option == FILE && ( n>0 && ULONG mem - ULONG start + n > offset + SEC_SIZE-1L
          || n<0 && ULONG mem - ULONG start + n < offset ) )
      {                             /* read-in another block of the file */
        offset = (ULONG mem - ULONG start + n ) & 0xFFFFFE00L;
        Fseek( offset, f_handle, 0 );
        Fread( f_handle, SEC_SIZE, start );
      }
      (long) mem += n;
      if( redraw )
      {
        check_mem();
        if( pos ? ULONG n < 128L : ULONG n > 0xFFFFFF80L )
        {
          i = (8-abs(j=n>>4))<<1;
          if( n>0 ) memcpy( readable, readable+j, i );
          else memcpy( readable-j, readable, i );
          objc_offset(form, DISPLAY, &box1.x, &box1.y);  /* display's loc */
          i = (n>>4) * text_h * (n>0L);
          j = (-n>>4) * text_h * (n<0L);
          box2.x = box1.x;
          box2.w = box1.w = form[DISPLAY].ob_width;
          box2.y = box1.y += hirez ? 15 : 9;
          box2.h = box1.h = form[DISPLAY].ob_height - (hirez ? 21 : 13) - i - j;
          box1.y += i;
          box2.y += j;
          _set_clip();
          blit( &box1, &box2 );                     /* blit the saved part */
          init_text();
          i = ( n>0L ? 128 - (int)n : 0 ) >> 4;
          redo_addr( i, i+abs((int)n>>4)-1 );       /* redraw the rest */
          redo_text( i, i+abs((int)n>>4)-1 );
          slider( mem );
        }
        else display();              /* moved more than a whole screen */
      }
    }
  }
}
/********************************************************************/
void new_hex_mode( int mode )
{
  static char hex_str[] = "+ 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F",
      dec_str[] = "+  0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15";
      
  hex = mode;
  form[TOPSTR].ob_spec.tedinfo->te_ptext =
      hex ? hex_str : dec_str;          /* new string for offsets */
  if( w_handle>0 )
  {
    objc_draw( form, TOPSTR, 0, Xrect(center) );   /* redraw it */
    objc_change( form, HEX, 0, Xrect(center), hex*SELECTED, 1 );
    objc_change( form, DECIMAL, 0, Xrect(center), !hex*SELECTED, 1 );
    init_text();
    redo_addr( 0, 7 );                      /* change addresses */
  }
}
/********************************************************************/
void new_sector(void)
{
}
/********************************************************************/
void put_str( int x, int y, unsigned char *s )
{
/*  register unsigned char c;

  WMODE = 0;
  DSTY = y;
  while( *s )
  {
    c = *s++ - fontp->font_low_ade;
    DELX = fontp->font_char_off[c+1] - (SRCX = fontp->font_char_off[c]);
    DSTX = x;
    SRCY = 0;
    linea8();
    x+=8;
  }*/
  if( !neo_acc )
  {
    set_wrt_mode(0);
    while(*s)
    {
      text_blt( x, y, *s++ );
      x+=8;
    }
  }
  else call_w_save( neo_acc->gtext, x, y, s, hirez+1, 0 );
}
/********************************************************************
  The name says it all
*/
int read_bootsec( int drive, Bsec *boot )
{
  BPB *bpb;
  
  if( (bpb=Getbpb(drive)) == 0 )
  {
    error( "|Could not access drive!" );
    return(0);
  }
  if( bpb->recsiz > SEC_SIZE ) goto bad_disk;
  if( (drive>1 ? Rwabs( 0, iobuf, 1, 0, drive ) :
      Floprd( iobuf, 0L, drive, 1, 0, 0, 1 )) < 0L )
  {
    error( "|Could not read|the bootsector!" );
    return(0);
  }
  cat.c[0] = *(iobuf+25);
  cat.c[1] = *(iobuf+24);
  boot->spt = cat.i;
  cat.c[0] = *(iobuf+20);               /* MSB of the number of total sectors */
  cat.c[1] = *(iobuf+19);               /* LSB */
  boot->secs = cat.i;
  cat.c[0] = *(iobuf+12);               /* MSB of the number of bytes/sector */
  cat.c[1] = *(iobuf+11);               /* LSB */
  boot->bps = cat.i;
  cat.c[0] = *(iobuf+27);               /* MSB of the number of bytes/sector */
  cat.c[1] = *(iobuf+26);               /* LSB */
  boot->sides = cat.i;                  /* sides on medium */
  if( drive<=1 && (!boot->sides || !boot->spt) || boot->bps>SEC_SIZE || 
      boot->bps<=0 )
  {
bad_disk:
    error( "|Sorry, MemFile can\'t handle|the format of this disk" );
    return(0);
  }
  boot->tps = boot->sides && boot->spt ? boot->secs/boot->sides/boot->spt : 0;
  return(1);
}
int norm_bootsec(void)
{
  if( !read_bootsec( drive, &boot ) ) return(0);
  max_addr = (long)start - 128L + boot.bps;
  spf( form[SECINFO].ob_spec.tedinfo->te_ptext,
      "Sides:%2d|Sectors/track:%3d|Tracks/side:%4d|Total Sectors:%6U",
      boot.sides, boot.spt, boot.tps, boot.secs );
  objc_draw( form, SECINFO, 0, Xrect(center) );
  boot.secs--;                /* it starts at zero, so subtract one */
  if( sector > boot.secs ) sector = 0;/* might be too big from last disk */
  form[SLIDBOX].ob_height = form[SLIDER].ob_height * 128 / boot.bps;
  last_slid = -1;
  return(1);
}
/********************************************************************/
int read_one_sector( Bsec *boot, unsigned int sector, unsigned int drive,
    char *buf )
{
  long stat;
  int s, sec, trk;

  if( drive < 2 )      /* look! it's a floppy! */
  {
    calc_sec( boot, sector, &s, &sec, &trk );
    stat = Floprd( buf, 0L, drive, sec, trk, s, 1 );
  }
  else stat = Rwabs( 0, buf, 1, sector, drive );
  if( stat < 0L )
  {
    cerror( "|Error #%D reading|disk in drive %c:", stat, 'A' + drive );
    return(0);
  }
  return(1);
}
/***********************************************************************
  Gee, aren't these wonderfully descriptive function names wonderfully
  descriptive?
*/
int read_sector(void)        /* read one sector */
{
  if( !read_one_sector( &boot, sector, drive, start ) )
  {
    sector = 0;
    goto_mem();
    display();
    return(99);
  }
  spf( form[SECNUM].ob_spec.tedinfo->te_ptext, "%u", sector );
  objc_draw( form, SECNUM, 0, Xrect(center) );
  return(0);
}
/***********************************************************************
  Redraw part or all of the address column in the display
*/
void redo_addr( int y0, int y1 )
{
  register int y;
  int t_x, t_y;
  static char format[4] = "%8X";
  unsigned long ii, st;
  
  format[2] = hex ? 'X' : 'U';               /* choose hex or decimal */
  objc_offset( form, ADDRSTR, &t_x, &t_y );
  t_y += y0 * text_h;                        /* raster coord of starting y */
  hide_mouse();
  _set_clip();
  st = ULONG start != rom_start ?           /* start differs when RAM or ROM */
      (long) start : 0L;
  for( y=y0; y<=y1; y++, t_y+=text_h )
  {
    ii = ULONG mem - st + (y<<4);
    if( option != FILE || ii <= file_len )
    {
      spf( num_str, format, ii );
      num_str[8] = 0;
    }
    else strcpy( num_str, "        " );     /* past end of file: blank it out */
    put_str( t_x, t_y, num_str );           /* draw the address string */
  }
  show_mouse(0);
}
/******************************************************************
  Redraw a portion of the hex and ASCII representations that appear
  in the display
*/
void redo_text( int y, int b )             /* from line y to line b */
{
  int x0, y0,           /* coords of upper-lefthand corner of topmost hex str */
      c_x;                        /* x coord of ASCII string */
  register int i,
               t_x, t_y,          /* text x, y */
               x;
  char asc_str[17], byte, *astr, *m;
  
  objc_offset( form, BOX1, &x0, &y0 );
  objc_offset( form, BOX2, &c_x, &dum );
  hide_mouse();
  _set_clip();
  stack = Sup_bus(0L);
  asc_str[16] = num_str[2] = '\0';
  for( i=y*16, t_y=y*text_h+y0, m=mem+i-offset; y<=b; y++, t_y+=text_h )
  {
    readable[y] = -1;
    for( astr=asc_str, x=16, t_x=x0; --x>=0; i++, t_x+=18 )
    {
      if( option != FILE || (long) mem + i - (long) start <= file_len )
      {
        byte = *m++;
        if( bus_err )
        {
          bus_err = 0;
          num_str[0] = num_str[1] = *astr++ = '\xFA';   /* invalid address */
          readable[y] &= ~(1<<(15-x));
        }
        else
        {
          to_hex( byte );                 /* to_hex() leaves result in num_str */
          *astr++ = byte ? byte : '.';
        }
      }
      else
blank:    num_str[0] = num_str[1] = *astr++ = ' ';   /* past end of file */
      
      put_str( t_x, t_y, num_str );
    }
    put_str( c_x, t_y, asc_str );
  }
  Sup_bus((void *)stack);
  show_mouse(0);
}
/********************************************************************/
void reset_title(void)
{
  set_title( maybe ? alter : name );
}
/********************************************************************/
void _set_clip(void)
{
  if( neo_acc ) call_w_save( neo_acc->set_clip, cur_clip, 1 );
  else set_clip( *cur_clip, *(cur_clip+1), *(cur_clip+2), *(cur_clip+3), 1 );
}
/********************************************************************/
void set_title( char *str )
{
  strcpy( title, str );
  wind_set( w_handle, WF_NAME, title, 0, 0 );
}
/*********************************************************************
  Redraw the slider with its updated value
*/
double ultod( unsigned long l )
{
  return( (long)l < 0 ? (double)(l&0x7FFFFFFFL) + 2.147483648e+9 :
      (double)l );
}
void slider( char *ptr )
{
  int i, j;
  unsigned long l, m;
  double flt;
  
  if( (m = max_addr - ULONG start) != 0 )
  {
    l = ULONG ptr - ULONG start;
    j = form[SLIDER].ob_height-form[SLIDBOX].ob_height;
    if( processor<2 ) i = j*l/m;
    else i = j*ultod(l)/ultod(m);
  }
  else i=0;
  if( i>j ) i=j;
  if( i!=last_slid )
  {
    form[SLIDBOX].ob_y = form[SLIDER].ob_y + (last_slid=i);
    objc_draw( form, SLIDER, 0, Xrect(center) );
    objc_draw( form, SLIDBOX, 0, Xrect(center) );
  }
}
/*********************************************************************
  Remove the spaces from an editable field so that it can be understood
  by TOS as a filename
*/
void to_filename( char *str )
{
  register int i, j;

  j = i = 0;
  while( *str )                         /* until end of string */
  {
    if( *str != ' ' )                   /* ignore any spaces */
    {
      if( j==8 ) filename[i++] = '.';   /* insert the period */
      filename[i++] = *str;
    }
    str++;
    j++;
  }
  filename[i] = '\0';                   /* terminate the string */
  strcpy( num_str, path );              /* copy the current path */
  strcat( num_str, filename );          /* append the new filename */
}
/***********************************************************************
  Convert a byte to hex and leave the result in num_str
*/
void to_hex( int i )
{
  num_str[0] = nums[i>>4 & 0xF];
  num_str[1] = nums[i & 0xF];
  num_str[2] = 0;
}
/********************************************************************/
int write_one_sector( Bsec *boot, unsigned int sector, unsigned int drive,
    char *buf )
{
  long stat;
  int s, sec, trk;

  if( drive < 2 )
  {
    calc_sec( boot, sector, &s, &sec, &trk );
    stat = Flopwr( buf, 0L, drive, sec, trk, s, 1 );
  }
  else stat = Rwabs( 1, buf, 1, sector, drive );
  if( stat < 0L )
  {
    cerror( "|Error #%D writing|disk in drive %c:", stat, 'A' + drive );
    return(0);
  }
  return(1);
}
/********************************************************************/
void goto_file( int do_sel )
{
  int i;
  
  if( option == MEMORY )
  {
    oldmax = max_addr;                  /* save memory location so */
    oldmem = (long) mem;                /* there is something to */
  }                                     /* revert to later */
  switch( do_sel ? do_select() : 1 )
  {
    case 1:                   /* file was found and opened */
      objc_change( form, MEMORY, 0, Xrect(center), 0, i=w_handle>0 );
      objc_change( form, SECTOR, 0, Xrect(center), 0, i );
      objc_change( form, FILE, 0, Xrect(center), SELECTED, i );
      edit1 = FILENAME;                 /* two items that can be */
      edit2 = SELECTOR;                 /* selected now          */
      option = FILE;
      gray();
/*      copysrc = COPSF;
      copydest = COPDF; */
      check_mem();                      /* just in case "maybe" set */
      break;
    case 2:                   /* file not found or user cancelled */
      goto_mem();
  }
}
/**************************************************************************/
void drive_letter(void)
{
  form[DRIVE].ob_spec.free_string[6] =
      /*copy[COPSSDR].ob_spec.tedinfo->te_ptext[0] = */ drive + 'A';
}
/**************************************************************************/
void goto_sector( int chng )
{
  drive_letter();
  objc_change( form, SECTOR, 0, Xrect(center), SELECTED, chng ); /* change  */
  objc_change( form, FILE, 0, Xrect(center), 0, chng );          /* the     */
  objc_change( form, MEMORY, 0, Xrect(center), 0, chng );        /* buttons */
  edit1 = SECNUM;                  /* these two can now be selected */
  edit2 = SWAP;
  if( option == MEMORY )
  {
    oldmax = max_addr;               /* save last memory location */
    oldmem = (long) mem;
    start = iobuf;                /* reset starting memory location */
  }
  else if( option == FILE ) oldoff = ULONG mem - ULONG start;
  option = SECTOR;
  gray();
/*  copysrc = COPSS;
  copydest = COPDS;*/
  offset = 0L;
  mem = start;                     /* start at beginning of iobuf */
  check_mem();
}
/***********************************************************************/
void reopen_file(void)
{
  strcpy( num_str, path );
  strcat( num_str, filename ); /* try to re-open the file */
  if( !do_file( 0 ) )          /* can't open file? try selector */
    if( do_select() == 2 ) goto_mem(); /* all else failed */
}
/***********************************************************************/
int test_neo( int alert )
{
  if( !neo_acc || (long)neo_acc & 1 || neo_acc->nac_ver < 0x0205 )
  {
    if( alert ) form_alert1( "[1][|Bad NeoDesk version][Ok]" );
    neo_acc = 0L;
    return 0;
  }
  if( neo_apid<0 ) neo_apid = appl_find("NEODESK ");
  return 1;
}
int find_neo( int *buf )
{
  /* throw away if too many tries */
  if( ++retries>10 ) return 0;
  /* try to find NeoDesk */
  if( (neo_apid = appl_find("NEODESK ")) >= 0 )
  {
    /* put the message back in my queue */
    appl_write( apid, 16, buf );
    buf[0] = NEO_ACC_ASK;         /* 89 */
    buf[1] = apid;
    buf[3] = NEO_ACC_MAGIC;
    buf[4] = apid;
    appl_write( neo_apid, 16, buf );
    return 1;
  }
  else
  {
    retries = 0;
    return 0;
  }
}
void make_3d(void)
{
  int ok, dum;

  if( _GemParBlk.global[0] >= 0x340 && _GemParBlk.global[0] != 0x399 )  /* MagiX */
  {
    if( _GemParBlk.global[0] < 0x410 ) ok = 1;
    else
    {
      ok = 0;
      appl_getinfo( 13, &ok, &dum, &dum, &dum );
    }
    if( ok )
    {
      RSHDR *r = *(RSHDR **)&_GemParBlk.global[7];
      OBJECT *o;
      int i;
  
      for( i=r->rsh_nobs, o=(OBJECT *)((long)r+r->rsh_object); --i>=0; o++ )
        if( *(char *)&o->ob_type==88 ) o->ob_spec.index &= ~0x7FL;
    }
  }
}
void move_forms(void)
{
  int i;
  OBJECT *h, *m, *end;
  RSHDR *r = *(RSHDR **)&_GemParBlk.global[7];
  
  rsrc_gaddr( 0, MED1, &end );
  for( h=(OBJECT *)((long)r+r->rsh_object), m=end; h!=end; h++, m++ )
    *(Rect *)&h->ob_x = *(Rect *)&m->ob_x;
}
int min( int a, int b )
{
  return a>b ? b : a;
}
int max( int a, int b)
{
  return a<b ? b : a;
}
int rc_intersect(Rect *r1, Rect *r2)
{
   int xl, yu, xr, yd;                      /* left, upper, right, down */

   xl = max( r1->x, r2->x );
   yu = max( r1->y, r2->y );
   xr = min( r1->x + r1->w, r2->x + r2->w );
   yd = min( r1->y + r1->h, r2->y + r2->h );
   if( (r2->w = xr - (r2->x=xl)) <= 0 ) return 0;
   if( (r2->h = yd - (r2->y=yu)) <= 0 ) return 0;
   return 1;
}
void pause(void)
{
  evnt_timer( 120, 0 );
}
/***********************************************************************/
EMULTI emulti = { 0, 1, 1, 1,  0, 0, 0, 0, 0,  1, 0, 0, 0, 0,  0, 0 };
int main( int argc, char *argv[] )
{
  int wind_type,                   /* type of window being drawn */
      top_wind,                    /* true if MemFile is the topmost window */
      buffer[8],                   /* event manager stuff */
      i, j, flag, select,
      old_state,                   /* previous state of the selected object */
      pxarray[4],
      old_opt,                     /* old memory, file, or sector type */
      wake_me=0,                   /* alert the user to Search completion */
      count=0,                     /* counter for Search alert */
      inside=0,                    /* true if mouse is on top of the window */
      which = MU_MESAG,            /* which events to monitor */
      shift_flg,                   /* flags auto-repeat for key command */
      inc,
      s_type, old_s_type=0;        /* search for ASCII or hex */
  unsigned long new_mem;
  unsigned char *ptr;
  char *sa_str, *sh_str,           /* strings in Search dialog */
      *s_ptr, ok, discard=0;                  /* current search location */
  static char xref[KEYS][2] = { 0x62, GETMAP, 0x2, MEMORY, 0x3, FILE,
      0x4, SECTOR, 0x20, ADDRDEC, 0x23, ADDRHEX, 0x48, UPARR, 0x50, DNARR,
      0x1F, SEARCH, 0x52, SLIDER, 0x47, SLIDER, 0x2E, GETCOPY, 0x01, SWAP,
      0x4D, SECPLUS, 0x4B, SECMINUS, 0x6d, MEMORY, 0x6e, FILE, 0x6f,
      SECTOR };
  double flt;
  

  stack = Super(0L);
  if( getcookie( 0x5f435055L, (long *)&new_mem) ) processor = new_mem/10;
  max_ram = processor>1 ? MAX_RAM2 : MAX_RAM0;
  max_addr = max_ram;
  phystop = _PHYSTOP - 128L;        /* get top of memory (128 is display len) */
  if( (rom_start = *(*((long **)0x4F2L)+2)) < 0xE00000L ) rom_start=0xE00000L;
  path[0] = (drive=Dgetdrv()) + 'A'; /* set the default path */
  path[1] = ':';
  Super( (void *)stack );
  Dgetpath( &path[2], drive+1 ); /* it's not very likely there will be a path */
  strcat( path, "\\" );
  apid = appl_init();                     /* wake-up the AES */
  /* Tell the AES (or Geneva) that we understand AP_TERM message */
  if( _GemParBlk.global[0] >= 0x400 ) shel_write( SHW_MSGTYPE, 1, 0, 0L, 0L );
  
  linea_init();                        /* initialize linea() variables */
  fontp = Fonts->font[1+(hirez=Vdiesc->v_cel_ht==16)];

  if( _app && argc>=2 )
  {
    neo_acc = (NEO_ACC *)atol(argv[1]);
    neo_apid = -1;
    if( test_neo(0) )
    {
      argv[1] = argv[2];
      argc--;
    }
  }
  ok = 1;
  if( Vdiesc->v_rez_hz<580 )
  {
    ok = 0;
    if( _app ) form_alert1(  
        "[1][|MemFile cannot run|in this resolution][Ok]" );
  }
  else if( !load_rsrc( "memfil31.rsc" ) ) ok = 0;
  if( ok )                      /* only if all is well so far */
  {
    if( Linea->v_planes >= 4 ) make_3d();
    if( !hirez ) move_forms();
    wind_get(0, WF_CURRXYWH, cliparray, cliparray+1, cliparray+2, cliparray+3);
    wind_type = NAME|CLOSER|MOVER;
    rsrc_gaddr( 0, MAINWIND, &form );   /* find the forms */
    rsrc_gaddr( 0, EDITOR, &editor );
    rsrc_gaddr( 0, DRIVES, &drives );
    rsrc_gaddr( 0, SEARCHF, &search );
    rsrc_gaddr( 0, MAP, &map );
    rsrc_gaddr( 0, COPY, &copy );
    rsrc_gaddr( 0, FNAMES, &fnames );
    form_center( drives, &drv.x, &drv.y, &drv.w, &drv.h );
    form_center( search, &srch.x, &srch.y, &srch.w, &srch.h );
    form_center( map, &map_rect.x, &map_rect.y, &map_rect.w, &map_rect.h );
    form_center( copy, &copy_rect.x, &copy_rect.y, &copy_rect.w, &copy_rect.h );
    form_center( fnames, &fnames_rect.x, &fnames_rect.y, &fnames_rect.w,
        &fnames_rect.h );
    sa_str = search[SRCHASC].ob_spec.tedinfo->te_ptext;
    sh_str = search[SRCHHEX].ob_spec.tedinfo->te_ptext;
    *sa_str = *sh_str = '\0';                     /* blank the search strings */
    form[ADDRHEX].ob_spec.tedinfo->te_txtlen = 9;
    form[ADDRDEC].ob_spec.tedinfo->te_txtlen = 10;
    form[FILENAME].ob_spec.tedinfo->te_txtlen = 12;
    form[SECNUM].ob_spec.tedinfo->te_txtlen = 7;
    fnames[FNSRC].ob_spec.tedinfo->te_ptext = copysrcf;
    fnames[FNDEST].ob_spec.tedinfo->te_ptext = copydestf;
    fnames[FNSRC].ob_spec.tedinfo->te_txtlen = 
        fnames[FNDEST].ob_spec.tedinfo->te_txtlen = 41;
    for( i=COPSM; i<COPYIT; i++ )
      if( copy[i].ob_flags & EDITABLE && i!=COPFLCH )
          copy[i].ob_spec.tedinfo->te_ptext[0] = '\0';
    editor[2].ob_width = editor[2].ob_height = 0;  /* VERY small button */
    form[SLIDBOX].ob_height = MIN_SLID;
    form[SLIDBOX].ob_y = form[SLIDER].ob_y;
    gray();
    new_hex_mode(1);
    
    text_h = hirez ? 16 : 10;        /* height of any text, in rasters */

    drive_letter();
    wind_calc( 0, wind_type, 0, 0,       /* fit the window around the */
        form[BOX].ob_width, form[BOX].ob_height,  /* dialog */
        &dum, &dum, &windo.w, &windo.h );
    windo.x = cliparray[2] - windo.w >> 1;
    windo.y = cliparray[3] - windo.h >> 1;
    if( !_app || _GemParBlk.global[0] >= 0x400 ) menu_register( apid, "  MemFile" );
    if( _app )
    {
      graf_mouse( ARROW, 0L );
      if( argc>=2 )
      {
        ptr = pathend(strupr(argv[1]));
        if( ptr != argv[1] )
        {
          strcpy( path, argv[1] );
          *(pathend(path)+1) = '\0';
          ptr++;
        }
        else if( argv[1][1]==':' )	/* drive: */
        {
          drive = (argv[1][0]&0x5f)-'A';
          goto_sector(0);
          buffer[0] = AC_OPEN;
          goto do_msg;	/* so that find_neo() can write something */
        }	/* else keep default path for file */
        strcpy( filename, ptr );
        in_field();
        goto_file(0);
        offset = 0L;
        mem = start = iobuf;
      }
      buffer[0] = AC_OPEN;
      goto do_msg;	/* so that find_neo() can write something */
    }
  }
/**  else if( _app || _GemParBlk.global[1]==-1 )
  {
    appl_exit();
    Pterm0();		/* 3.2: used to lockup */
  } ***/
  
  for(;;)                /* gee, an infinite loop in GEM? how unusual... */
  {
    *(Rect *)&emulti.m2x = center;
    *(Rect *)&emulti.m1x = center;
    emulti.type = which;
    multi_evnt( &emulti, buffer );
    wind_get( 0, WF_TOP, &top_wind, &dum, &dum, &dum );   /* who's on top? */
    if( (top_wind = top_wind == w_handle) == 0 ) wake_me = 0;   /* is it us? */
    if( wake_me && count++==MAX_COUNT )
    {
      RING_BELL;
      count = 0;
    }
    if( emulti.event & MU_MESAG )                        /* message event */
    {
      if( buffer[0] != WM_REDRAW ) wake_me = 0;
do_msg:
      switch( buffer[0] )
      {
        case AP_TERM:
          appl_exit();
          return(0);
        case 0x400:   	/* ACC_ID */
          if( !neo_acc ) find_neo(buffer);
          break;
        case AC_OPEN:
        case NEO_AC_OPEN:                       /* same as AC_OPEN */
          if( discard )
          {
            discard = 0;
            break;
          }
ac_open:  if( w_handle < 0 && ok )              /* no window open */
          {
            if( !neo_acc && find_neo(buffer) ) break;
            test_neo(0);
            if( (w_handle=wind_create(wind_type,Xrect(windo))) < 0 )
            {
              form_alert1( 
                  "[1][There are no more windows|available. Please close a|window you no longer need.][Ok]" );
              break;
            }
            reset_title();
            wind_open( w_handle, Xrect(windo) );    /* open it */
            wind_get( w_handle, WF_WORKXYWH, &center.x, &center.y, &dum,
                &dum );                      /* get its location, area */
            form[BOX].ob_x = center.x;       /* move the dialog there */
            form[BOX].ob_y = center.y;
            center.w = form[BOX].ob_width;
            center.h = form[BOX].ob_height;
ac_open2:   switch( option )                 /* if its being re-opened */
            {
              case FILE:
                reopen_file();
                break;
              case SECTOR:
                if( norm_bootsec() ) read_sector();
                else goto_mem();
            }
            which = MU_BUTTON|MU_M1|MU_M2|MU_MESAG|MU_KEYBD;/* turn-on events */
            break;
          }                /* if the window was already open, make it the top */
        case WM_TOPPED:    /* make our window the topmost */
          if( w_handle > 0 ) wind_set( w_handle, WF_TOP, w_handle, 0, 0, 0 );
          break;
        case NEO_ACC_INI:                       /* NeoDesk knows we are here */
          if( buffer[3]==NEO_ACC_MAGIC )
          {
            retries = 0;
            neo_acc = *(NEO_ACC **)&buffer[4];  /* set pointer to Neo's fncns */
            neo_apid = buffer[6];               /* NeoDesk's AES ID */
            if( !test_neo(1) ) discard = 1;
          }
          break;  
        case NEO_ACC_PAS:
          if( buffer[3]==NEO_ACC_MAGIC )
            if( !neo_acc && find_neo(buffer) ) break;
            else while( call_w_save( neo_acc->list_files, &s_ptr ) )
              if( *(s_ptr+1)==':' && (*(s_ptr+strlen(s_ptr)-1) != '\\' ||
                  !*(s_ptr+3)) )
                if( !*(s_ptr+3) )
                {
                  drive = *s_ptr-'A';
                  goto_sector(0);
                  goto to_ac_open;
                }
                else
                {
                  strcpy( path, s_ptr );
                  *(pathend(path)+1) = '\0';
                  strcpy( filename, pathend(s_ptr)+1 );
                  in_field();
                  goto_file(0);
                  offset = 0L;
                  mem = start = iobuf;
to_ac_open:       ack(buffer);
                  if( w_handle<0 ) goto ac_open;
                  else
                  {
                    should_redraw++;
                    goto ac_open2;
                  }
                }       /* fall through to ack() if no valid icon */
        case NEO_CLI_RUN:                       /* run a batch file */
          if( !neo_acc ) find_neo(buffer);
          ack(buffer);                           /* you MUST ack NeoDesk!! */
          break;
        case NEO_ACC_BAD:
          if( buffer[3] == NEO_ACC_MAGIC ) neo_acc=0L;
          break;
        case WM_REDRAW:
wm_redraw:
          if( should_redraw )
          {
            buffer[4] = buffer[5] = 0;
            buffer[6] = buffer[7] = 16383;
          }
          if( w_handle > 0 )                                  /* just in case */
          {                          /* take the rectangles out for a walk */
            wind_update( BEG_UPDATE );
            hide_mouse();                           /* turn-off the mouse */
            wind_get( w_handle, WF_FIRSTXYWH, &pxarray[0], &pxarray[1],
                &pxarray[2], &pxarray[3] );
            while( pxarray[2] && pxarray[3] )
            {
/*              if( pxarray[0]+pxarray[2] > (i=cliparray[0]+cliparray[2]) )
                  pxarray[2] = i-pxarray[0];
              if( pxarray[1]+pxarray[3] > (i=cliparray[1]+cliparray[3]) )
                  pxarray[3] = i-pxarray[1]; */
              if( rc_intersect( (Rect *)cliparray, (Rect *)pxarray ) &&
                  rc_intersect( (Rect *)&buffer[4], (Rect *)pxarray ) )
              {
                center.w = pxarray[2];              /* clip the form redraw */
                center.h = pxarray[3];
                pxarray[2] += (center.x = pxarray[0]) - 1;
                pxarray[3] += (center.y = pxarray[1]) - 1;
                objc_draw( form, BOX, MAX_DEPTH, Xrect(center) );
                cur_clip = pxarray;             /* also clip the linea */
                display();                  /* redisplay with new clipping */
              }
              wind_get( w_handle, WF_NEXTXYWH, &pxarray[0], &pxarray[1],
                  &pxarray[2], &pxarray[3] );     /* get next rectangle */
            }
            show_mouse(0);                        /* mouse on */
            cur_clip = cliparray;                 /* reset clipping */
            _set_clip();
            center = *(Rect *)&form[BOX].ob_x;            /* reset form location */
            wind_update( END_UPDATE );
          }
          should_redraw=0;
          break;
        case AC_CLOSE:
          w_handle = -1;      /* may also have to close the file, so no break */
        case WM_CLOSED:
wm_close: if( option == FILE ) Fclose( f_handle );
          if( w_handle > 0 )
          {
            wind_close( w_handle );
            wind_delete( w_handle );
            w_handle = -1;
            graf_shrinkbox( 0, 0, 20, 30, Xrect(center) );
          }
          which = MU_MESAG;        /* turn off most events to save CPU time */
          if( _app )
          {
            appl_exit();
            return(0);
          }
          break;
        case WM_MOVED:        /* move the window somewhere else */
          *(long *)&windo.x = *(long *)&buffer[4];
          wind_set( w_handle, WF_CURRXYWH, buffer[4], buffer[5], buffer[6],
              buffer[7] );    /* AES tells you what to redraw later */
          wind_get( w_handle, WF_WORKXYWH, &center.x, &center.y, &dum, &dum );
          form[BOX].ob_x = center.x;
          form[BOX].ob_y = center.y;
          break;
      }
    }
    if( should_redraw ) goto wm_redraw;
    if( emulti.event & MU_M2 && emulti.event&MU_BUTTON ) inside = 0;
    if( emulti.event & MU_M1 && !(emulti.event&MU_BUTTON) ) inside = 1;
    if( emulti.event & MU_KEYBD )
    {
      wake_me = 0;
      emulti.key >>= 8;                                /* isolate the scan code */
      if( emulti.mouse_k==4 && emulti.key==0x11 ) goto wm_close;        /* ^W */
      if( emulti.key>=0x3B && emulti.key<=0x44 )
      {
        new_mem = fkey[emulti.key-0x3B];
        goto newmem;
      }
      else if( emulti.key>=0x54 && emulti.key<=0x5D )
      {
        new_mem = fkey[emulti.key-0x54] = (long)mem - (option!=MEMORY ?
            (long)start : 0L);
        spf( num_str, "F%d set to 0x%X (%Ud)", emulti.key-0x53, new_mem, new_mem );
        set_title( num_str );
      }
      else if( emulti.key==0xE )        /* Backsp */
      {
        new_mem = last_addr;
        goto newmem;
      }
      else if( emulti.key==0x13 )       /* R key */
      {
        if( ULONG mem >= rom_start && ULONG mem <= ROM_END+0x7f )
            new_mem = 0L;
        else new_mem = rom_start;
        goto newmem;
      }
      else if( emulti.key!=0x01 || option==SECTOR ) /* only allow Esc in SECTOR mode */
        for( i=0; i<KEYS; i++ )
          if( emulti.key == xref[i][0] )
          {
            select = xref[i][1];
            old_state = form[select].ob_state;
            emulti.mouse_y = emulti.key==0x47 ? 16000 : 0;  /* page down is special */
            shift_flg = emulti.mouse_k & 3;
            flag = 1;
            goto use_item;
          }
    }
    if( emulti.event&MU_BUTTON && top_wind && inside )
    {
      wake_me = 0;
      select = i = find_obj( emulti.mouse_x, emulti.mouse_y );
      old_state = form[i].ob_state;                     /* save its state */
        /* this is a bit confusing: any object that is selectable only at
           certain times has ob_flags bit 11 set in the dialog. Others are
           either touchexit or straight selectables */
      flag = i && ( form[i].ob_flags & (1<<11) &&
           ( i == edit1 || i == edit2 || option == SECTOR && i == DRIVE ) ||
           (form[i].ob_flags & SELECTABLE) ||
           ( form[i].ob_flags & TOUCHEXIT ) );
      if( flag && emulti.mouse_b&1 && !(form[i].ob_flags & TOUCHEXIT) )
      {                  /* wait for button to be released on top of object */
        flag = 0;       /* flag is now re-used; it says whether or not on obj */
        while( emulti.mouse_b & 1 )            /* while button is pressed */
        {
          graf_mkstate( &emulti.mouse_x, &emulti.mouse_y, &emulti.mouse_b,
              &emulti.mouse_k );
          i = find_obj( emulti.mouse_x, emulti.mouse_y );
          if( i != select && flag )              /* different item than first */
          {
            flag = 0;                             /* not on original object */
            objc_change( form, select, 0, Xrect(center), old_state, 1 ); /* reset */
          }
          else if( i == select && !flag )         /* (back) on original obj */
          {
            flag = 1;
            objc_change( form, select, 0, Xrect(center), old_state | SELECTED, 1 );
          }
        }
      }
      shift_flg = 0;                              /* shift keys aren't used */

use_item:

      if( form[select].ob_state != old_state ) objc_change( form,
          select, 0, Xrect(center), old_state, 1 );
      if( flag && select != SEARCH ) /* reset search pointer since it */
          old_s_ptr = 0L;            /* probably won't be in display anymore */
      old_opt = option;
      if( flag ) switch( select )    /* ended on an object */
      {
        case MEMORY:
          if( option != MEMORY )
          {
            if( option == FILE ) Fclose( f_handle );   /* close file, if necc */
            goto_mem();
            display();
          }
          break;
        case FILE:
          if( option != FILE )
            if( !opened ) goto_file(1);
            else
            {
              goto_file(0);
              mem = (start=iobuf) + oldoff;
              offset = oldoff&-SEC_SIZE;
              reopen_file();
              display();
            }
          break;
        case GETMAP:
          make_form( map, map_rect, 0, 1 );
          break;
        case DRIVE:
drv_labl: form[DRIVE].ob_state = old_state;      /* yeah, a goto in C. I know */
          new_mem = Drvmap();                /* get map of valid disk drives */
          for( i=0; i<16; i++ )          /* act/de-act buttons in drives list */
            drives[i+DRIVEA].ob_state = !(new_mem & (1<<i)) * DISABLED;
          drives[drive+DRIVEA].ob_state |= SELECTED;/* preselect current drive*/
          i = make_form( drives, drv, 0, 1 );
          if( i==DRVCANC )
          {
            goto_mem();
            display();
            break;
          }
          drive = i - DRIVEA;
        case SECTOR:
          drive_letter();
          if( option != SECTOR )
          {
            if( option == FILE ) Fclose( f_handle );
            goto_sector(1);
          }
          else if( select != DRIVE ) break;        /* rarely break */
          should_redraw = 1;
        case SWAP:
          if( !norm_bootsec() )
          {
            select = DRIVE;
            goto drv_labl;                  /* I hate to do this, but... */
          }
          read_sector();                    /* re-read the current sector */
          if( select != DRIVE )             /* closed drives list dialog */
              display();                    /* forces redraw, so don't bother */
          break;
        case HEX:
          if( !hex ) new_hex_mode( 1 );
          break;
        case DECIMAL:
          if( hex ) new_hex_mode( 0 );
          break;
        case UPARR:
        case DNARR:
        case SLIDER:
          i = form[SLIDBOX].ob_y + center.y;
          j = 0;
          if( shift_flg ) set_title( "Press BOTH Shift keys to stop" );
          do
          {
            new_mem = (long)mem;
            switch(select)
            {
              case UPARR:                /* up arrow on scroll bar pressed */
                move( option==FILE ? -128L : -16L, 1, 0 );
                break;
              case DNARR:                /* down arrow */
                move( option==FILE ? 128L : 16L, 1, 1 );
                break;
              case SLIDER:               /* gray area of slider */
                move( emulti.mouse_y<i ? -128L : 128L, 1, emulti.mouse_y>=i );
                break;
            }
            if( !shift_flg )
            {
              if( !j )
              {
                pause();
                j++;
              }
              graf_mkstate( &dum, &dum, &emulti.mouse_b, &dum );
            }
            else emulti.mouse_b = !((Kbshift(-1)&3) == 3);  /* until both shifts */
          }
          while( emulti.mouse_b&1 && (long)mem!=new_mem );
              /* repeat as long as button is pressed or display changes */
          if( shift_flg ) reset_title();
          but_up();
          break;
        case SLIDBOX:              /* drag the slider box */
          if( graf_dragbox( form[SLIDBOX].ob_width, form[SLIDBOX].ob_height,
              form[SLIDBOX].ob_x+center.x, form[SLIDBOX].ob_y+center.y,
              form[SLIDER].ob_x+center.x, form[SLIDER].ob_y+center.y,
              form[SLIDER].ob_width, form[SLIDER].ob_height, &dum, &i ) &&
              form[SLIDBOX].ob_height < form[SLIDER].ob_height )
          {             /* (nice and uncomplicated, eh?) */
            flt = (i-form[SLIDER].ob_y-center.y) * ultod(max_addr - ULONG start)
                / (form[SLIDER].ob_height - form[SLIDBOX].ob_height);
            if( flt>2.147483648e+9 ) new_mem = (unsigned long)(flt-2.147483648e+9)
                | 0x80000000L;
            else new_mem = (unsigned long)flt;
            new_mem = (new_mem & (processor>1 ? 0xFFFFFFF0L : 0x00FFFFF0L)) +
                ULONG start;
            move( new_mem - ULONG mem, 1, new_mem>=ULONG mem );
          }
          break;
        case BOX1:            /* button pressed within hex-value area */
          if( emulti.mouse_b&2 )
          {
            but_up();
            objc_offset( form, BOX1, &srch.x, &srch.y );
            i = (emulti.mouse_x-srch.x)/18;
            j = (emulti.mouse_y-srch.y)/text_h;
            if( i < 0 ) i = 0;
            if( i > 15 ) i = 15;
            if( j < 0 ) j = 0;
            if( j > 7 ) j = 7;
            ptr = mem - offset + (j<<4) + i;
            stack = Sup_bus(0L);
            new_mem = ((long)(unsigned)((*ptr<<8)|*(ptr+1))<<16)|
                ((unsigned)*(ptr+2)<<8)|*(ptr+3);
            Sup_bus((void *)stack);
            if( test_berr() ) break;
            goto newmem;
          }
        case BOX2:            /* within ASCII-value area */
          cursor( select, emulti.mouse_x, emulti.mouse_y );     /* do some editing */
          break;
        case SELECTOR:        /* user wants a file selector */
          do_select();
          break;
        case SEARCH:          /* search. the pfun begins */
          strcpy( num_str, sa_str );             /* save for later comparison */
          i = search[0].ob_x-srch.x;
          objc_offset( form, BOX1, &srch.x, &srch.y );
          search[0].ob_x = srch.x+i;                               /* don't     */
          search[0].ob_y = srch.y+i;                               /* center it */
          s_type = make_form( search, srch, 0, 0 );
                                           /* user selects ASCII or hex field */
          search[s_type].ob_state &= ~SELECTED;
          objc_draw( search, s_type, 0, Xrect(srch) );  /* redraw it as selected */
          inc = (long)start==rom_start ? 0x0FFF : 0xDFFF; /* increment mask */
          if( s_type == SRCHASC || s_type == SRCHHEX || s_type == SRCHXIT )
          {
            if( s_type == SRCHXIT )          /* 'Ok' button pressed */
            {
              i = SRCHXIT;
              s_type = old_s_type;           /* try to use last search type */
            }
            else
            {
              search[s_type].ob_flags |= EDITABLE;     /* now allow the user */
              i = _form_do( search, s_type );          /* to edit the field  */
              search[i].ob_state &= ~SELECTED;         /* reset its state */
            }
            if( i == SRCHXIT )               /* only search if 'Ok' pressed */
            {
              j = 0;
              if( s_type == SRCHHEX )        /* search for a hex string */
              {
                i = 0;
                while( *(sh_str+i) )        /* make sure all digits are valid */
                {                           /* and find length */
                  if( *(sh_str+i) > 0x60 ) *(sh_str+i) -= 0x20; /* a-f -> A-F */
                  if( *(sh_str+i) < '0' || *(sh_str+i) > '9'
                       && ( *(sh_str+i) < 'A' || *(sh_str+i) > 'F' ) )
                  {                                    /* not a hex digit */
                    error(
                        "|Bytes in hexadecimal must|contain only 0-9 and A-F");
                    i = -1;         /* gets incremented to zero shortly */
                    *sh_str = '\0';
                  }
                  i++;
                }
                if( i&1 )                    /* append a zero if only high */
                {
                  *(sh_str+(i++)) = '0';   /* nybble in byte was entered */
                  *(sh_str+i) = 0;
                }
                for( j=0; j<i; j+=2 )        /* convert hex to ASCII */
                  *(sa_str+(j>>1)) =
                    (*(sh_str+j)>'9' ? *(sh_str+j)-'7' : *(sh_str+j)-'0') << 4 |
                    (*(sh_str+j+1)>'9' ? *(sh_str+j+1)-'7' : *(sh_str+j+1)-'0');
                *(sa_str+(j>>1)) = '\0';     /* append a NUL */
                j = j>>1;       /* divide by 2 to get the length of ASCII str */
                objc_draw( search, SRCHASC, 0, Xrect(srch) );  /* draw with new info */
              }
              if( *sa_str || j )  /* either there was something on ASCII line */
              {                   /* or hex was entered */
                new_mem = (long) mem;
                emulti.times = 0;        /* have we been here before? don't know yet */
                if( !strcmp( num_str, sa_str )  /* continue search if strings */
                    && old_s_ptr )              /* match and ptr is valid */
                {
                  s_ptr = old_s_ptr;            /* where we left off before */
                  emulti.times++;                      /* we've been here before */
                }
                else
                {
                  s_ptr = mem;         /* start from lowest addr in display */
                  if( option == FILE ) (long) s_ptr -= offset;
                }
                old_s_type = s_type;
                if( !j )                   /* searching by ASCII */
                {
                  for( ; *(sa_str+j); j++ )  /* convert the string to hex */
                  {
                    to_hex( *(sa_str+j) );
                    *(sh_str+(j<<1)) = num_str[0];
                    *(sh_str+(j<<1)+1) = num_str[1];
                  }
                  *(sh_str+(j<<1)) = '\0';
                }
                objc_draw( search, SRCHHEX, 0, Xrect(srch) ); /* redraw hex string */
                should_redraw = 1;
                set_title( "Searching...Press BOTH shift keys to stop" );
                wind_update( BEG_UPDATE );   /* lock mouse while searching */
                graf_mouse( HOURGLASS, 0L );
                flag = i = 0;      /* here, flag=1=not found, 2=aborted */
/* the area is searched incrementally. when the source matches the dest byte
   the counter is increased and the next byte in the source string (if any)
   is compared next. if the comparison fails part way though the string, the
   search begins again at the location of the first match plus one */
                if( option==MEMORY ) stack = Sup_bus(0L);
                while( !flag && i<j )  /* no error and not end of string */
                {
                  if( option == MEMORY && (long) s_ptr == phystop + 128L )
                  {
                    Sup_bus((void *)stack);
                    set_title( "The search has reached phystop" );
                    stack = Sup_bus(0L);
                  }
                  if( s_ptr == sa_str )
                  {                     /* if this is the place in memory */
                    s_ptr += j-1;       /* where the string is stored, skip */
                    i = 0;              /* over it */
                  }
                  else if( *s_ptr == *(sa_str+i) && !emulti.times && !bus_err ) i++;
                           /* ignore match if we've searched this area before */
                  else if( i )          /* no match, but there was one before */
                  {
                    s_ptr -= i;            /* backup the pointer */
                    i = 0;
                  }
                  emulti.times = 0;              /* only ignore the very first match */
                  s_ptr++;                /* increment search pointer */
                  switch( option )        /* check for bounds and past buffer */
                  {
                    case MEMORY:
                      if( (long) s_ptr > max_addr + 0x7FL )/* reached highest */
                          flag = 1;                        /* address         */
                      if( !((long) s_ptr & inc) )   /* only redraw slider if  */
                      {
                        Sup_bus((void *)stack);
                        slider( s_ptr );          /* change might be visible */
                        stack = Sup_bus(0L);
                      }
                      break;
                    case FILE:
                      if( (long) s_ptr - (long) start + offset > file_len )
                          flag = 1;          /* end of file */
                      else if( s_ptr - start > SEC_SIZE-1L )  /* past end of block */
                      {
                        offset += SEC_SIZE;                /* read next block */
                        Fread( f_handle, SEC_SIZE, start );
                        s_ptr = start;                 /* point back to start */
                      }
                      if( !(s_ptr-start & 0x7FL) ) slider( s_ptr + offset );
                      break;
                    case SECTOR:
                      if( s_ptr - start > SEC_SIZE-1L )       /* end of sector */
                        if( sector == boot.secs ) flag = 1;  /* last sector */
                        else
                        {
                          sector++;
                          flag = read_sector();        /* read next one */
                          s_ptr = start;
                        }
/*                      if( !(s_ptr-start & 0x0FL) ) slider( s_ptr );*/
                  }
                  if( !flag && (Kbshift(-1)&0x03) == 0x03 )  /* abort if both */
                      flag = 2;                      /* shift keys held down */
                }
                if( option == MEMORY ) Sup_bus( (void *)stack );
                wind_update( END_UPDATE );          /* restore mouse control */
                mem = (char *) new_mem = old_s_ptr = (char *)(s_ptr - j);
                if( option == FILE )
                {
                  if( mem < start )      /* string resides in two file blocks */
                  {
                    offset -= SEC_SIZE;              /* go back to previous block */
                    Fseek( offset, f_handle, 0 );
                    Fread( f_handle, SEC_SIZE, start );
                    new_mem = (long) mem + SEC_SIZE;
                    mem = start + SEC_SIZE-0x80L;
                  }
                  (long)mem = (long)start + (long)( mem-start & 0xFFFFFF80L )
                      + offset;
                  old_s_ptr = (char *) new_mem;
                  new_mem += offset;   /* where the string is relative to */
                }                      /* top of file */
                switch( option )
                {
                  case MEMORY:
                    (long) mem &= 0xFFFFFFF0L;
                    if( (long) mem > max_addr ) (long) mem = max_addr;
                    break;
                  case SECTOR:
                    if( mem < start )             /* string is in two sectors */
                    {
                      sector--;                   /* read previous one */
                      read_sector();
                      new_mem = (long) old_s_ptr = (long) mem + SEC_SIZE;
                      mem = start + SEC_SIZE-0x80L;
                    }
                    mem = start + ( mem-start & 0xFFFFFFF0L );
                    if( mem-start > SEC_SIZE-0x80L ) mem = start + SEC_SIZE-0x80L;
                }
                check_mem();
                if( !flag )          /* found the string */
                {
                  if( ULONG start == rom_start ) new_mem += rom_start;
                  spf( title, "The string begins at %Ud (0x%X)",
                      new_mem - ULONG start, new_mem - ULONG start );
                  set_title( title );
                }
                else reset_title();
                if( flag == 1 ) set_title( "Could not find the string" );
                if( flag < 2 && search[ALERTON].ob_state & SELECTED )
                {
                  wake_me = 1;
                  count = MAX_COUNT;
                }
              }
            }
            search[s_type].ob_flags &= ~EDITABLE;
          }
          graf_mouse( ARROW, 0L );
          form_dial( FMD_FINISH, 0, 0, 0, 0, Xrect(srch) );  /* erase the search */
          break;                    /* dialog and force the display to redraw */
        case ADDRDEC:
        case ADDRHEX:
        case FILENAME:
        case SECNUM:                           /* create an editable field */
          strcpy( num3_str, form[select].ob_spec.tedinfo->te_ptext );
          editor[1].ob_spec =           /* make the TEDINFOs point to */
              form[select].ob_spec;     /* eachother */
          objc_offset( form, select, &ed.x, &ed.y );/* where is it on screen? */
          editor[0].ob_x = ed.x;        /* move edit dialog there */
          editor[0].ob_y = ed.y;
          editor[1].ob_width = ed.w = form[select].ob_width;
          editor[1].ob_height = ed.h = form[select].ob_height;
          hide_mouse();
          _form_do( editor, 1 );
          show_mouse(0);
          editor[2].ob_state &= ~SELECTED; /* there's only one button they can press */
          form_dial( FMD_FINISH, 0, 0, 0, 0, Xrect(ed) ); /* forces redraw later */
          switch( select )
          {
            case ADDRHEX:          /* new address in hex */
              if( from_hex( form[ADDRHEX].ob_spec.tedinfo->te_ptext,
                  &new_mem ) ) new_mem = (long)mem + (long)start;
            case ADDRDEC:          /* new decimal address */
              if( select == ADDRDEC ) new_mem = (long)
                  atol( form[ADDRDEC].ob_spec.tedinfo->te_ptext );
              should_redraw++;
newmem:       spf( form[ADDRDEC].ob_spec.tedinfo->te_ptext, "%U", new_mem );
              spf( form[ADDRHEX].ob_spec.tedinfo->te_ptext, "%X", new_mem );
              last_addr = ULONG mem - (option==MEMORY ? 0L : ULONG start);
              switch( option )
              {
                case MEMORY:
                  (long) mem = new_mem & (processor>1 ? 0xFFFFFFF0L :
                      0xFFFFF0L);
                  if( ULONG mem >= rom_start && processor<2 )
                  {                        /* may have to adjust memory area */
                    (long) start = rom_start;
                    max_addr = ROM_END;
                  }
                  else
                  {
                    (long) start = 0L;
                    max_addr = max_ram;
                  }
                  if( ULONG mem > max_addr ) ULONG mem = max_addr;
                  check_mem();
                  break;
                case FILE:
                  new_mem -= 127L;
                case SECTOR:
                  move( new_mem + ULONG start - ULONG mem, 0, 1 );
                  break;
              }
              if( !should_redraw ) display();
              break;
            case FILENAME:         /* new filename with same path as before */
              strcpy(num2_str, form[FILENAME].ob_spec.tedinfo->te_ptext);
              to_filename( num2_str );       /* convert editable to filename */
              if( !do_file( 1 ) )
              {
                to_filename( num3_str );
                strcpy( form[FILENAME].ob_spec.tedinfo->te_ptext,
                    num3_str );         /* copy new name into dialog */
                if( !do_file( 1 ) )
                  if( do_select() == 2 ) goto_mem();
              }
              break;
            case SECNUM:                /* new sector number */
              new_mem = atol( form[SECNUM].ob_spec.tedinfo->te_ptext );
              if( new_mem > boot.secs )   /* greater than the last sector */
              {
                sector = boot.secs;
                spf( num_str,
                    "|Sectors on this disk range|from 0-%u", boot.secs );
                error( num_str );
              }
              else
              {
                sector = new_mem;
                new_sector();
              }
              should_redraw = 1;
              read_sector();            /* read the new sector */
              break;
          }
          break;
        case GETCOPY:
          do_copy();
          break;
        default:                        /* sector + or - */
          if( option == SECTOR )
          {
            flag = 0;              /* set if sector could be changed */
            switch( select )
            {
              case SECPLUS:
                if( sector < boot.secs )
                {
                  sector++;
                  flag++;
                }
                break;
              case SECMINUS:
                if( sector > 0 )
                {
                  sector--;
                  flag++;
                }
                break;
            }
            if( flag )
            {
              new_sector();
              read_sector();
              display();
            }
          }
          break;
      }
      if( option != old_opt ) reset_title();
    }
  }
}
/* (en) fin */
