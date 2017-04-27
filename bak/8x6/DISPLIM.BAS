      
       '8x6 PRESSURE PROFILE DISPLAY PROGRAM        DISP8X6.BAS  5/7/98
       'NASA LEWIS RESEARCH CENTER                  MARK R. WOIKE
       'REVISED FOR USE WITH NEW PC
       'ADDED TIME DELAYS

       DEFINT A-P
       BASADR = 704                              'BASE ADDRESS OF A/D CARD

       DIM AR(100)                               'OLD PRESSURE BAR HEIGHTS
       DIM AXH(5)                                'HIGH BYTES FOR A/D CONV.
       DIM AXL(5)                                'LOW BYTES FOR A/D CONV.
       DIM AY(100)                               'NEW PRESSURE BAR HEIGHTS
       DIM BUS$(100)                             'ASCII CONVERSION COEFF ARRAY
       DIM BYTE(1000)                            'RAW PRESSURE DATA ARRAY
       DIM D$(5000)                              'ASCII SERIAL MESSAGE BUFFER
       DIM QCTS(1000)                            'PRESSURE DATA IN CTS.
       DIM QPRES(1000)                           'PRESSURE DATA IN EU (PSIA)
       DIM V(5)                                  'SETRA VOLTAGES FROM A/D
       DIM X0(100)                               'ZERO CONV. COEFF. IN EU
       DIM X1(100)                               'SPAN CONV. COEFF. IN EU
       DIM A%(720)                               'BUFFERS USED IN PRINT
       B% = 0: D% = 0: F% = 0: N% = 0: R% = 0    '       SCREEN ROUTINE
'____________________________________________________________________________

       EMPTY$ = ""                               'INITIALIZE PARAMETERS
       BREAK$ = CHR$(3)                          'ASSOCIATED WITH SERIAL
       RET$ = CHR$(13)                           'COMMUNICATION
       XON$ = CHR$(17)
       XOFF$ = CHR$(19)
       FALSE = 0
       TRUE = NOT FALSE
       PAUSE = FALSE
'____________________________________________________________________________


       COMM$ = "COM1:9600,N,8,1,RS,CS,DS,CD"     'OPEN COMPUTER SERIAL PORT
       PRINT "INITIALIZING DPT6400"              'COMMUNICATION WITH DPT UNIT
       AOPEN = 0
C1:    OPEN COMM$ FOR RANDOM AS #1
       GOTO D1
C2:    AOPEN = AOPEN + 1
       IF AOPEN = 11 THEN GOTO C3
       BEEP
       GOTO C1
C3:    PRINT "PORT ERROR"
       CLOSE
       END
'____________________________________________________________________________

D1:    ASEND = 0                                 'OPEN UP COMMUNICATIONS WITH
D2:    MSG$ = BREAK$                             'DPT UNIT.
       GOSUB SEND                                'SEND BREAK TO UNIT AND WAIT
       FOR AT = 1 TO 5000                        'FOR RESPONSE OF PROMPT *
       IF EOF(1) THEN GOTO D3
       GOSUB PROMPT
       GOTO E1
D3:    NEXT AT
       ASEND = ASEND + 1
       IF ASEND < 11 THEN GOTO D2
       PRINT "BAD DPT COMMUNICATIONS"
       CLOSE
       END
'____________________________________________________________________________

       'SEND INITIALIZING
       'PARAMETERS TO DPT UNIT

E1:    MSG$ = BREAK$                             'CLEAR DPT UNIT
       GOSUB SEND                                'WAIT FOR PROMPT
       GOSUB PROMPT
       MSG$ = "IN9 64" + RET$                    'PROGRAM STATUS WORD
       GOSUB SEND
       GOSUB PROMPT
       MSG$ = "IN8 0-0 17" + RET$                'OUTPUT DELIMITER OPTIONS
       GOSUB SEND
       GOSUB PROMPT
       MSG$ = "IN7 0" + RET$                     'SCAN OUTPUT OPTIONS
       GOSUB SEND
       GOSUB PROMPT
       MSG$ = "IN6 21" + RET$                    'RS232 COMMUNICATION
       GOSUB SEND                                'PROTOCOL
       GOSUB PROMPT
       MSG$ = "IN5 64" + RET$                    '64 CHANNELS
       GOSUB SEND
       GOSUB PROMPT
       MSG$ = "IN4 8" + RET$                     '8 AVERAGES PER SCAN
       GOSUB SEND
       GOSUB PROMPT

       PRINT "INITIALIZING COMPLETE"

'____________________________________________________________________________

CALIB: PRINT "START CALIBRATION"

       MSG$ = BREAK$
       GOSUB SEND
       GOSUB PROMPT
      
       GOSUB CAL
       PRINT "CAL PRESS 1", STR$(QCAL1)
       PRINT "CAL PRESS 2", STR$(QCAL2)
       MSG$ = "IN0" + STR$(QCAL1) + RET$
       GOSUB SEND
       GOSUB PROMPT
       MSG$ = "IN1" + STR$(QCAL2) + RET$
       GOSUB SEND
       GOSUB PROMPT
       MSG$ = "CA1 1-64" + RET$
       GOSUB SEND
       GOSUB PROMPT
     
       PRINT "SENDING CALIBRATION COEFFICIENTS"

'____________________________________________________________________________

	
	MSG$ = "DS1 1-64" + RET$
      
	GOSUB SEND
	GOSUB COL

	FOR I = 1 TO 64
	BUS$(I) = ""
	NEXT I

	C = 0
	FOR B = 1 TO A
	FOR PTR = 1 TO LEN(D$(B))
	CHAR$ = MID$(D$(B), PTR, 1)
	IF CHAR$ <> RET$ AND CHAR$ <> CHR$(10) THEN BUS$(C) = BUS$(C) + CHAR$
	IF CHAR$ = RET$ THEN C = C + 1
	NEXT PTR
	NEXT B

	FOR B = 1 TO 64
	X1(B) = (VAL(RIGHT$(BUS$(B), 7))) / 409.6
	X0(B) = VAL(MID$(BUS$(B), 6, 7))
	NEXT B

	PRINT "CALIBRATION IS COMPLETE"

LOOP0: GOSUB SCR
'____________________________________________________________________________

LOOP1:
      
       MSG$ = "SC4 1-64" + RET$
       GOSUB SEND
       GOSUB COL1
      
       FOR I = 1 TO 2000
       FOR J = 1 TO 20
       DELAY = 1
       NEXT J
       NEXT I


       K = 0
       FOR B = 1 TO A
       FOR PTR = 1 TO LEN(D$(B))
       CHAR$ = MID$(D$(B), PTR, 1)
       IF CHAR$ <> RET$ AND CHAR$ <> CHR$(10) THEN GOSUB CONV
       NEXT PTR
       NEXT B

       M = 1
       FOR N = 0 TO 260
       QCTS(M) = BYTE(N) * 16 ^ 3 + BYTE(N + 1) * 16 ^ 2 + BYTE(N + 2) * 16 + BYTE(N + 3)
       IF QCTS(M) > 4095 THEN QCTS(M) = -1! * (65536 - QCTS(M))
       M = M + 1
       N = N + 3
       NEXT N

       FOR I = 3 TO 66
       QPRES(I - 2) = QCTS(I) * X1(I - 2) + X0(I - 2)
       NEXT I
'_____________________________________________________________________________

       GOSUB DRW

       CK = INP(BASADR + 2)
       '***** (CODED OUT UNTIL PRINTER IS FIXED)
       '***** IF CK < 96 THEN GOSUB DATAPRINT      'EGAPRINT ****
       IF CK < 64 THEN GOTO CALIB

       PK$ = " "
       PK$ = INKEY$
       IF PK$ = "P" OR PK$ = "p" THEN GOSUB EGAPRINT
       IF PK$ = "D" OR PK$ = "d" THEN GOSUB DATAPRINT
       IF PK$ = "S" OR PK$ = "s" THEN GOTO DCAL

       GOTO LOOP1
       PRINT "SCANNING COMPLETE"

ED:     END
'____________________________________________________________________________


CONV:  BYTE(K) = ASC(CHAR$) - 48
       K = K + 1
       RETURN
'____________________________________________________________________________

COL:    A = 1
F1:     IF EOF(1) THEN GOTO F1
F2:     IF LOC(1) < 128 THEN GOTO F3
	PAUSE = TRUE
	MSG$ = XOFF$
	GOSUB SEND
F3:     DPTMSG$ = ""
	DPTMSG$ = INPUT$(LOC(1), #1)
	IF DPTMSG$ = "*" THEN RETURN
	D$(A) = DPTMSG$
	A = A + 1
	IF LOC(1) > 0 THEN GOTO F2
	IF NOT PAUSE GOTO F4
	PAUSE = FALSE
	MSG$ = XON$
	GOSUB SEND
F4:     GOTO F1
	RETURN

'____________________________________________________________________________

COL1:   A = 1
	TIMER ON
	ON TIMER(3) GOSUB RST
H1:     IF EOF(1) THEN GOTO H1
H2:     IF LOC(1) < 128 THEN GOTO H3
	PAUSE = TRUE
	MSG$ = XOFF$
	GOSUB SEND
H3:     DPTMSG$ = ""
	DPTMSG$ = INPUT$(LOC(1), #1)
	IF DPTMSG$ = "*" THEN GOTO H5
	D$(A) = DPTMSG$
	A = A + 1
	IF LOC(1) > 0 THEN GOTO H2
	IF NOT PAUSE GOTO H4
	PAUSE = FALSE
	MSG$ = XON$
	GOSUB SEND
H4:     GOTO H1
H5:     TIMER OFF
	RETURN

'____________________________________________________________________________

RST:    MSG$ = BREAK$
	GOSUB SEND
	RETURN
'____________________________________________________________________________


SEND:   MSGLEFT$ = MSG$
	WHILE (MSGLEFT$ <> EMPTY$)
	NEXTCHAR$ = LEFT$(MSGLEFT$, 1)
	MSGLEFT$ = MID$(MSGLEFT$, 2)
	PRINT #1, NEXTCHAR$;
	WEND
	RETURN


PROMPT: MSG$ = ""
	WHILE (MSG$ <> "*")
	MSG$ = INPUT$(1, #1)
	WEND
	RETURN
'____________________________________________________________________________

CAL:
	FOR J = 0 TO 3

	OUT BASADR + 2, J
	FOR I = 1 TO 1000
	DELAY = 400
	NEXT I

	OUT BASADR + 1, 0
	FOR I = 1 TO 1000
	DELAY = 400
	NEXT I

G1:     IF INP(BASADR + 2) >= 128 GOTO G1
	AXL(J) = INP(BASADR)
	FOR I = 1 TO 1000
	DELAY = 400
	NEXT I

	AXH(J) = INP(BASADR + 1)
	FOR I = 1 TO 1000
	DELAY = 400
	NEXT I

	NEXT J

	FOR J = 0 TO 3
	V(J) = ((AXH(J) * 16! + AXL(J) / 16!) * 10! / 4096!) - 5!
	NEXT J
	QCAL1 = (5! * (V(1) - V(0)))
	QCAL2 = (10! * (V(3) - V(2)))
	RETURN


'____________________________________________________________________________


SCR:    FOR A = 1 TO 64
	AR(A) = 319
	NEXT A

	CLS
	SCREEN 9
	LINE (0, 270)-(640, 350), 0, BF
	LINE (320, 0)-(320, 350)

	LINE (360, 30)-(396, 35), 15, BF
	LINE (360, 85)-(396, 90), 15, BF
	LINE (396, 30)-(480, 30)
	LINE (480, 30)-(480, 35)
	LINE (480, 35)-(396, 35)
	LINE (396, 35)-(396, 30)

	LINE (396, 85)-(480, 85)
	LINE (480, 85)-(480, 90)
	LINE (480, 90)-(396, 90)
	LINE (396, 90)-(396, 85)

	J = 400
	FOR I = 1 TO 17
	LINE (J, 30)-(J - 5, 35)
	LINE (J, 90)-(J - 5, 85)
	J = J + 5
	NEXT I

	LINE (480, 30)-(494, 27)
	LINE (494, 27)-(537, 27)
	LINE (537, 27)-(640, 25)
	LINE (640, 25)-(640, 30)
	LINE (640, 30)-(537, 32)
	LINE (537, 32)-(494, 32)
	LINE (494, 32)-(480, 35)
	LINE (480, 35)-(480, 32)
	PAINT (550, 29), 15, 15

	LINE (480, 85)-(494, 88)
	LINE (494, 88)-(537, 88)
	LINE (537, 88)-(640, 90)
	LINE (640, 90)-(640, 95)
	LINE (640, 95)-(537, 93)
	LINE (537, 93)-(494, 93)
	LINE (494, 93)-(480, 90)
	LINE (480, 90)-(480, 85)
	PAINT (550, 90), 15, 15

	LINE (520, 32)-(534, 42)
	LINE (520, 88)-(534, 78)

	COLOR 15
	LINE (498, 57)-(498, 63)
	LINE (498, 63)-(512, 63)
	LINE (512, 63)-(512, 57)
	LINE (512, 57)-(498, 57)

	LINE (435, 57)-(470, 57)
	LINE (470, 57)-(470, 63)
	LINE (470, 63)-(435, 63)
	LINE (435, 63)-(435, 57)

	COLOR 15
	K = 37
	FOR I = 1 TO 5
	LINE (480, K)-(480, K + 6)
	LINE (396, K)-(396, K + 6)
	K = K + 10
	NEXT I

	K = 32
	FOR I = 1 TO 6
	LINE (494, K)-(494, K + 6)
	LINE (537, K)-(537, K + 6)
	K = K + 10
	NEXT I

	COLOR 14
	K = 368
	FOR I = 1 TO 39
	CIRCLE (K, 60), 1
	PAINT (K, 60), 14, 14
	K = K + 7
	NEXT I

	COLOR 15
	K = 122
	FOR I = 1 TO 7
	LINE (K, 43)-(K, 73)
	LINE (K, 73)-(K + 6, 73)
	LINE (K + 6, 73)-(K + 6, 43)
	LINE (K + 6, 43)-(K + 20, 43)
	K = K + 20
	NEXT I

	K = 120
	FOR I = 1 TO 7
	LINE (K, 45)-(K, 75)
	LINE (K, 75)-(K + 10, 75)
	LINE (K + 10, 75)-(K + 10, 45)
	LINE (K + 10, 45)-(K + 20, 45)
	K = K + 20
	NEXT I

	LINE (110, 75)-(110, 45)
	LINE (110, 45)-(120, 45)
	LINE (110, 75)-(105, 70)
	LINE (105, 70)-(105, 48)
	LINE (105, 48)-(110, 43)
	LINE (110, 43)-(122, 43)

	LINE (260, 45)-(260, 75)
	LINE (260, 75)-(265, 45)
	LINE (265, 45)-(270, 45)
	LINE (270, 45)-(270, 75)

	LINE (305, 43)-(265, 43)
	LINE (265, 43)-(265, 35)
	LINE (265, 35)-(260, 35)
	LINE (260, 35)-(260, 30)
	LINE (260, 30)-(270, 30)
	LINE (270, 30)-(270, 38)
	LINE (270, 38)-(305, 38)

	LINE (262, 43)-(262, 38)
	LINE (262, 38)-(257, 38)
	LINE (257, 38)-(257, 30)

	LINE (305, 75)-(270, 75)
	LINE (270, 75)-(270, 85)
	LINE (305, 80)-(273, 80)
	LINE (273, 80)-(273, 85)
	LINE (273, 85)-(267, 85)
	LINE (267, 85)-(267, 80)

	LINE (267, 80)-(188, 80)
	LINE (188, 80)-(188, 85)
	LINE (188, 85)-(182, 85)
	LINE (182, 85)-(182, 80)
	LINE (185, 85)-(185, 75)
	LINE (182, 80)-(113, 80)
	LINE (113, 80)-(113, 85)
	LINE (113, 85)-(107, 85)
	LINE (107, 85)-(107, 80)
	LINE (110, 85)-(110, 75)

	LINE (105, 80)-(80, 80)
	LINE (110, 75)-(80, 75)
	LINE (85, 75)-(90, 70)
	LINE (90, 70)-(90, 48)
	LINE (90, 48)-(85, 43)

	LINE (110, 43)-(80, 43)
	LINE (110, 39)-(80, 39)
	LINE (110, 39)-(110, 32)
	LINE (110, 32)-(115, 32)
	LINE (115, 32)-(115, 25)

	LINE (113, 43)-(113, 35)
	LINE (113, 35)-(118, 35)
	LINE (118, 35)-(118, 25)

	LINE (116, 43)-(116, 38)
	LINE (116, 38)-(121, 38)
	LINE (121, 38)-(121, 25)

	LINE (80, 75)-(75, 77)
	LINE (75, 77)-(70, 80)
	LINE (70, 80)-(65, 85)
	LINE (80, 80)-(73, 83)
	LINE (73, 83)-(70, 87)
	LINE (70, 87)-(65, 85)

	LINE (80, 43)-(75, 41)
	LINE (75, 41)-(70, 38)
	LINE (70, 38)-(65, 33)
	LINE (80, 39)-(73, 35)
	LINE (73, 35)-(70, 31)
	LINE (70, 31)-(65, 33)

	LINE (360, 120)-(360, 320)
	LINE (360, 320)-(639, 320)
	LINE (639, 320)-(639, 120)
	K = 120
	FOR I = 1 TO 21
	LINE (356, K)-(364, K)
	LINE (636, K)-(639, K)
	K = K + 10
	NEXT I
	LOCATE 9, 42
	PRINT "20"
	LOCATE 13, 42
	PRINT "15"
	LOCATE 16, 42
	PRINT "10"
	LOCATE 20, 42
	PRINT " 5"
	LOCATE 23, 42
	PRINT " 0"

	LINE (40, 120)-(40, 320)
	LINE (40, 320)-(303, 320)
	LINE (303, 320)-(303, 120)
	K = 120
	FOR I = 1 TO 21
	LINE (34, K)-(44, K)
	LINE (299, K)-(307, K)
	K = K + 10
	NEXT I
	LOCATE 9, 1
	PRINT "30"
	LOCATE 13, 1
	PRINT "25"
	LOCATE 16, 1
	PRINT "20"
	LOCATE 20, 1
	PRINT "15"
	LOCATE 23, 1
	PRINT "10"

	COLOR 14
	K = 110
	FOR I = 1 TO 16
	CIRCLE (K, 75), 2
	PAINT (K, 75), 14, 14
	K = K + 10
	NEXT I

	CIRCLE (50, 43), 2
	PAINT (50, 43), 14, 14
	CIRCLE (50, 75), 2
	PAINT (50, 75), 14, 14
	CIRCLE (95, 43), 2
	PAINT (95, 43), 14, 14
	CIRCLE (95, 75), 2
	PAINT (95, 75), 14, 14
	CIRCLE (285, 43), 2
	PAINT (285, 43), 14, 14
	CIRCLE (285, 75), 2
	PAINT (285, 75), 14, 14

	'LINE (285, 75)-(300, 100)
	'LINE (300, 100)-(300, 125)
	'LINE (298, 122)-(300, 125)
	'LINE (300, 125)-(302, 122)

	LINE (285, 75)-(290, 100)
	LINE (290, 100)-(290, 125)
	LINE (288, 122)-(290, 125)
	LINE (290, 125)-(292, 122)

	LINE (285, 43)-(280, 100)
	LINE (280, 100)-(280, 125)
	LINE (278, 122)-(280, 125)
	LINE (280, 125)-(282, 122)

	LINE (285, 43)-(270, 100)
	LINE (270, 100)-(270, 125)
	LINE (268, 122)-(270, 125)
	LINE (270, 125)-(272, 122)


	LINE (95, 75)-(100, 100)
	LINE (100, 100)-(100, 125)
	LINE (98, 122)-(100, 125)
	LINE (100, 125)-(102, 122)

	LINE (95, 75)-(90, 100)
	LINE (90, 100)-(90, 125)
	LINE (88, 122)-(90, 125)
	LINE (90, 125)-(92, 122)

	LINE (95, 43)-(95, 67)   '**
	LINE (82, 85)-(80, 100)  '**
	LINE (80, 100)-(80, 125)
	LINE (78, 122)-(80, 125)
	LINE (80, 125)-(82, 122)

	LINE (95, 67)-(70, 100)   '**
	LINE (70, 100)-(70, 125)
	LINE (68, 122)-(70, 125)
	LINE (70, 125)-(72, 122)

	LINE (50, 43)-(60, 100)
	LINE (60, 100)-(60, 125)
	LINE (58, 122)-(60, 125)
	LINE (60, 125)-(62, 122)

	LINE (50, 75)-(50, 100)
	LINE (50, 100)-(50, 125)
	LINE (48, 122)-(50, 125)
	LINE (50, 125)-(52, 122)

	COLOR 7
	LINE (469, 63)-(469, 320)
	LINE (511, 63)-(511, 320)
	LINE (105, 81)-(105, 320)
	LINE (265, 81)-(265, 320)
	COLOR 15

	LOCATE 1, 38
	PRINT "ATM= "

	RETURN
'____________________________________________________________________________


DRW:    FOR I = 1 TO 64
	AY(I) = ((QPRES(I) * 10) + .5)
	NEXT I

	FOR I = 26 TO 64
	IF AY(I) > 200 THEN AY(I) = 200
	IF AY(I) < 0 THEN AY(I) = 0
	AY(I) = 320 - AY(I)
	NEXT I
			      
	FOR I = 1 TO 25
	IF AY(I) > 300 THEN AY(I) = 300
	IF AY(I) < 100 THEN AY(I) = 100
	AY(I) = 320 - AY(I) + 100
	NEXT I

	AW = 365
	FOR I = 26 TO 64
	AWY = AW + 5
	IF AY(I) = AR(I) GOTO SA1
	IF AY(I) > AR(I) THEN LINE (AW, AY(I))-(AWY, AR(I)), 0, BF
	IF AY(I) < AR(I) THEN LINE (AW, AR(I))-(AWY, AY(I)), 5, BF
SA1:    AW = AW + 7
	AR(I) = AY(I)
	NEXT I

	AW = 46
	FOR I = 1 TO 25
	AWY = AW + 8
	IF AY(I) = AR(I) GOTO SA2
	IF AY(I) > AR(I) THEN LINE (AW, AY(I))-(AWY, AR(I)), 0, BF
	IF AY(I) < AR(I) THEN LINE (AW, AR(I))-(AWY, AY(I)), 4, BF
SA2:    AW = AW + 10
	AR(I) = AY(I)
	NEXT I


	LOCATE 1, 1
	PRINT DATE$
	LOCATE 1, 70
	PRINT TIME$
	LOCATE 1, 43
	PRINT USING "##.##"; QPRES(1)
	RETURN
'_____________________________________________________________________________
DATAPRINT:
				   

	LPRINT TIME$, DATE$
	LPRINT " "
	LPRINT "COMPRESSOR STATICS                    TEST SECTION STATICS"
	LPRINT " "
	FOR I = 1 TO 25
	LPRINT USING " ## "; I;
	LPRINT USING " ###.### "; QPRES(I);
	LPRINT "                           ";
	LPRINT USING " ## "; (I + 25);
	LPRINT USING " ###.### "; QPRES(I + 25)
	NEXT I

	FOR I = 51 TO 64
	LPRINT "                                        ";
	LPRINT USING " ## "; (I);
	LPRINT USING " ###.### "; QPRES(I)
	NEXT I
	LPRINT CHR$(12)
	RETURN
'_____________________________________________________________________________
EGAPRINT:
	GOSUB DATAPRINT
	LPRINT CHR$(27); "@"
	FOR N% = 1 TO 4: LPRINT : NEXT N%
	WIDTH "LPT1:", 255
	LPRINT CHR$(27); "A"; CHR$(8);
	FOR R% = 80 TO 1 STEP -1
	LPRINT CHR$(9);
	GET (R% * 8 - 8, 0)-(R% * 8 - 1, 349), A%
	DEF SEG
	B% = VARPTR(A%(2))
	LPRINT CHR$(27); "Z"; CHR$(120); CHR$(5);
	FOR N% = 0 TO 1400
	D% = PEEK(B% + N%): F% = 0: IF N% < 4 THEN D% = 0
	IF D% AND 128 THEN F% = 1
	IF D% AND 64 THEN F% = F% + 2
	IF D% AND 32 THEN F% = F% + 4
	IF D% AND 16 THEN F% = F% + 8
	IF D% AND 8 THEN F% = F% + 16
	IF D% AND 4 THEN F% = F% + 32
	IF D% AND 2 THEN F% = F% + 64
	IF D% AND 1 THEN F% = F% + 128
	LPRINT CHR$(F%);
	NEXT N%
	LPRINT
	NEXT R%
	FOR N% = 1 TO 11: LPRINT : NEXT N%
	LPRINT CHR$(27); "@"
	LPRINT CHR$(12)
	RETURN
'___________________________________________________________________________

DCAL:   CLS                                                'SCREEN DISPLAY
	LOCATE 5, 10                                         'FOR CALIBRATION
	PRINT "CALIBRATION PRESSURE VALUES"                  'PRESSURES (SETRAS)
T1:     GOSUB CAL
	LOCATE 10, 10
	PRINT "SPAN1 PRESSURE -  SETRA PT1 = ";
	PRINT USING "##.##"; QCAL2;
	PRINT " PSIA"
	LOCATE 12, 10
	PRINT "VAC. PRESSURE  -  SETRA PT2 = ";
	PRINT USING "##.##"; QCAL1;
	PRINT " PSIA"
	LOCATE 17, 10
	PRINT "PRESS  Q  TO EXIT SCREEN AND CONTINUE SCANNING"
	PKEY$ = INKEY$
	IF PKEY$ = "Q" OR PKEY$ = "q" THEN GOTO T2
	GOTO T1
T2:     GOTO LOOP0
	RETURN
'____________________________________________________________________________

