.import bus_err
.export Sup_bus
.import processor

berr equ 8

old_err:ds.l    1

Sup_bus:
        move.b  processor(pc), d0
        cmpi.b  #3, d0
        bhi.b   badcpu
        move.l  a0, d0
        beq.b   super
        move.l  old_err(pc), berr.w
super:	sf	bus_err
	pea	(a0)
        move.w  #$20, -(sp)
        trap    #1
        addq    #2, sp
        tst.l   (sp)+
        bne.b   supex
        move.l  berr.w, old_err
        clr     d1
        move.b  processor(pc), d1
        asl     #2, d1
        lea     mbtbl(pc), a0
        move.l  0(a0,d1.w), berr.w
supex:  rts

badcpu: pea	(a0)
        move.w  #$20, -(sp)
        trap    #1
        addq    #6, sp
        rts

add_it:		/* d0 contains mode field, d2 contains register */
        andi.b  #7, d2
        andi.b	#7, d0
        subq.b  #5, d0
        blt.b   ax
        beq.b   ai2
        subq.b  #1, d0
        beq.b   ai2		; d16(An)
        subq.b  #1, d2
        blt.b   ai2		; x.w
        beq.b   ai4		; x.l
        subq.b  #1, d2
        beq.b   ai2		; d16(pc)
        subq.b  #1, d2
        beq.b   ai2		; d8(pc,rn)
        btst	#12, d1		; #: test size
        beq.b	ai4		; long
ai2:    addq    #2, a0		; int
ax:     rts
ai4:    addq    #4, a0
        rts

mb00:   pea     (a0)
	movem.w	d0-d2, -(sp)
        move.w  16(sp), d0
        move    d0, d1
        movea.l 20(sp), a0
        btst    #14, d0
        bne.b   nomv
        btst    #15, d0
        bne.b   nomv
again:  cmp.w	-2(a0), d0
        beq.b	got_it
        subq	#2, a0
        bra.b	again
got_it: asr	#3, d0
	move	d1, d2
	bsr.b   add_it
        move.w  d1, d0
        asr.w   #6, d0
        move	d1, d2
        rol	#7, d2
        bsr.b   add_it
        move.l  a0, 20(sp)
nomv:   movem.w	(sp)+, d0-d2
        movea.l (sp)+, a0
        addq    #8, sp
mbx:    st      bus_err
        rte

mb10:   bset    #7, 8(sp)
        bra.b   mbx

mb20:
mb30:   bclr    #0, 10(sp)
        bra.b   mbx

mbtbl:  dc.l    mb00, mb10, mb20, mb30
