      
       '10x10 PRESSURE PROFILE DISPLAY PROGRAM      DISP.BAS    5/7/98
       'NASA LEWIS RESEARCH CENTER                  MARK R. WOIKE
       'REVISION FOR USE WITH NEW COMPUTER
      
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

E1:
       MSG$ = BREAK$                             'CLEAR DPT UNIT AND
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
					      
						 'CALIBRATION PROCEDURE
CALIB: PRINT "START CALIBRATION"
      
       MSG$ = BREAK$                             'CLEAR DPT UNIT AND
       GOSUB SEND                                'WAIT FOR PROMPT
       GOSUB PROMPT
						 'POLL A/D CARD FOR CAL
       GOSUB CAL                                 'PRESSURE VALUES
       PRINT "ZERO PRESSURE", STR$(QCAL1)
       PRINT "SPAN RANGE 1 ", STR$(QCAL2)
       PRINT "SPAN RANGE 2 ", STR$(QCAL3)
       MSG$ = "IN0" + STR$(QCAL1) + RET$         'SEND CAL VALUES TO DPT UNIT
       GOSUB SEND
       GOSUB PROMPT
       MSG$ = "IN1" + STR$(QCAL2) + RET$
       GOSUB SEND
       GOSUB PROMPT
       MSG$ = "CA1 1-40" + RET$                  'CALIBRATE CHANNELS 1-40
       GOSUB SEND                                '(LOW RANGE CHANNELS 15PSI)
       GOSUB PROMPT

						 'POLL A/D CARD FOR CAL
       GOSUB CAL                                 'PRESSURE VALUES
       MSG$ = "IN0" + STR$(QCAL1) + RET$
       GOSUB SEND                                'SEND CAL VALUES TO DPT UNIT
       GOSUB PROMPT
       MSG$ = "IN1" + STR$(QCAL3) + RET$
       GOSUB SEND
       GOSUB PROMPT
       MSG$ = "CA1 57-64" + RET$                 'CALIBRATE CHANNELS 57-64
       GOSUB SEND                                '(HIGH RANGE CHANNELS 30PSI)
       GOSUB PROMPT


       PRINT "SENDING CALIBRATION COEFFICIENTS"
'___________________________________________________________________________

						 'COLLECTION AND SORTING OF
						 'CONVERSION COEFFICIENTS
						 'FOR DPT CHANNELS
       
       MSG$ = "DS1 1-64" + RET$                  'ISSUE COEFF. COLLECTION
       GOSUB SEND                                'COMMAND
       GOSUB COL


       FOR I = 1 TO 64                           'INITIALIZE RAW COEFF ARRAY
       BUS$(I) = ""
       NEXT I

       C = 0                                     'SORT SERIAL MESSAGE INTO
       FOR B = 1 TO A                            'ASCII CONV. COEFF. ARRAY
       FOR PTR = 1 TO LEN(D$(B))
       CHAR$ = MID$(D$(B), PTR, 1)
       IF CHAR$ <> RET$ AND CHAR$ <> CHR$(10) THEN BUS$(C) = BUS$(C) + CHAR$
       IF CHAR$ = RET$ THEN C = C + 1
       NEXT PTR
       NEXT B

       FOR B = 1 TO 64                            'CONVERT ASCII DATA INTO
       X1(B) = (VAL(RIGHT$(BUS$(B), 7))) / 409.6  'SPAN & ZERO COEFFICIENT
       X0(B) = VAL(MID$(BUS$(B), 6, 7))           'VALUES
       NEXT B

       PRINT "CALIBRATION IS COMPLETE"

LOOP0: GOSUB SCR

'____________________________________________________________________________

						 'SEND DPT UNIT INTO DATA
						 'SCAN AND COLLECT PRESSURE
						 'READINGS
LOOP1:
	MSG$ = "SC4 1-64" + RET$                 'ISSUE DATA SCAN COMMAND
	GOSUB SEND
	GOSUB COL1

	K = 0                                    'SORT ASCII PRESSURE DATA
	FOR B = 1 TO A                           'INTO BYTE VALUES
	FOR PTR = 1 TO LEN(D$(B))
	CHAR$ = MID$(D$(B), PTR, 1)
	IF CHAR$ <> RET$ AND CHAR$ <> CHR$(10) THEN GOSUB CONV
	NEXT PTR
	NEXT B


	M = 1                                    'CONVERT BYTES INTO CTS
	FOR N = 0 TO 260
	QCTS(M) = BYTE(N) * 16 ^ 3 + BYTE(N + 1) * 16 ^ 2 + BYTE(N + 2) * 16 + BYTE(N + 3)
	IF QCTS(M) > 4095 THEN QCTS(M) = -1! * (65536 - QCTS(M))
	M = M + 1
	N = N + 3
	NEXT N
						 'CONVERT CTS INTO PSIA
						 'USING CONVERSION COEFF
	FOR I = 3 TO 66
	QPRES(I - 2) = QCTS(I) * X1(I - 2) + X0(I - 2)
	NEXT I

	RAT = QPRES(58) / QPRES(57)              'COMPUTE PRESSURE RATIOS
	SRAT = QPRES(60) / QPRES(59)
'_____________________________________________________________________________

      GOSUB DRW                                  'DRAW BACKGROUND ON SCREEN

      CK = INP(BASADR + 2)                       'CHECK PUSHBUTTON STATUS
      'IF CK < 96 AND CK > 64 THEN GOSUB EGAPRINT 'PRINT ROUTINE CODED OUT
      IF CK < 64 THEN GOTO CALIB

      PKEY$ = ""                                 'READ SETRA PRESSURES
      PKEY$ = INKEY$
      IF PKEY$ = "S" OR PKEY$ = "s" THEN GOTO DCAL

      GOTO LOOP1                                 'REPEAT DATA SCAN LOOP

      PRINT "SCANNING COMPLETE"
ED:   END
'____________________________________________________________________________


CONV:  BYTE(K) = ASC(CHAR$) - 48                 'ROUTINE CONVERTS SERIAL
       K = K + 1                                 'ASCII DATA TO BYTE VALUES
       RETURN
'____________________________________________________________________________

COL:     A = 1                                   'SERIAL DATA COLLECTION
F1:      IF EOF(1) THEN GOTO F1                  'FOR COEFFICIENTS
F2:      IF LOC(1) < 128 THEN GOTO F3
	 PAUSE = TRUE
	 MSG$ = XOFF$
	 GOSUB SEND
F3:      DPTMSG$ = ""
	 DPTMSG$ = INPUT$(LOC(1), #1)
	 IF DPTMSG$ = "*" THEN RETURN
	 D$(A) = DPTMSG$
	 A = A + 1
	 IF LOC(1) > 0 THEN GOTO F2
	 IF NOT PAUSE GOTO F4
	 PAUSE = FALSE
	 MSG$ = XON$
	 GOSUB SEND
F4:      GOTO F1
	 RETURN


'____________________________________________________________________________

COL1:    A = 1                                   'SERIAL DATA COLLECTION
	 TIMER ON                                'FOR RAW VALUES
	 ON TIMER(2) GOSUB RST
H1:      IF EOF(1) THEN GOTO H1
H2:      IF LOC(1) < 128 THEN GOTO H3
	 PAUSE = TRUE
	 MSG$ = XOFF$
	 GOSUB SEND
H3:      DPTMSG$ = ""
	 DPTMSG$ = INPUT$(LOC(1), #1)
	 IF DPTMSG$ = "*" THEN GOTO H5
	 D$(A) = DPTMSG$
	 A = A + 1
	 IF LOC(1) > 0 THEN GOTO H2
	 IF NOT PAUSE GOTO H4
	 PAUSE = FALSE
	 MSG$ = XON$
	 GOSUB SEND
H4:      GOTO H1
H5:      TIMER OFF
	 RETURN
'____________________________________________________________________________

RST:    MSG$ = BREAK$                           'TO REST UNIT IF HUNG UP
	GOSUB SEND
	RETURN
'___________________________________________________________________________

SEND:   MSGLEFT$ = MSG$                          'ROUTINE SENDS MESSAGE
	WHILE (MSGLEFT$ <> EMPTY$)               'MSG$ TO DPT UNIT
	NEXTCHAR$ = LEFT$(MSGLEFT$, 1)
	MSGLEFT$ = MID$(MSGLEFT$, 2)
	PRINT #1, NEXTCHAR$;
	WEND
	RETURN


PROMPT: MSG$ = ""                                'ROUTINE WAITS FOR DPT
	WHILE (MSG$ <> "*")                      'READY RESPONSE *
	MSG$ = INPUT$(1, #1)
	WEND
	RETURN
'____________________________________________________________________________

						 'A/D CALIBRATION PRESSURE
						 'READ ROUTINE
CAL:    FOR J = 0 TO 5
	OUT BASADR + 2, J                        'SELECT CHANNEL FOR A/D CONV
	FOR I = 1 TO 1000
	DELAY = 200
	NEXT I
       
	OUT BASADR + 1, 0                        'START 12 BIT A/D CONVERSION
	FOR I = 1 TO 1000
	DELAY = 200
	NEXT I

G1:     IF INP(BASADR + 2) >= 128 GOTO G1
	AXL(J) = INP(BASADR)                     'INPUT LOW BYTE
	FOR I = 1 TO 1000
	DELAY = 200
	NEXT I
	
	AXH(J) = INP(BASADR + 1)                 'INPUT HIGH BYTE
	FOR I = 1 TO 1000
	DELAY = 200
	NEXT I
	
	NEXT J
						 'CONVERT BYTES TO VOLTAGES

	FOR J = 0 TO 5
	V(J) = ((AXH(J) * 16! + AXL(J) / 16!) * 10! / 4096!) - 5!
	NEXT J
			     
						  'COMPUTE CAL PRESSURE VALUES
	 QCAL2 = (10! * (V(1) - V(0))) + .02      'SPAN1 PRESSURE ON CH. 0 & 1
	 QCAL3 = 10! * (V(3) - V(2))              'SPAN2 PRESSURE ON CH. 2 & 3
	 QCAL1 = 4! * (V(5) - V(4))               'ZERO PRESSURE  ON CH. 4 & 5
       
	RETURN


'____________________________________________________________________________

						 'DRAW ROUTINE FOR SCREEN
SCR:    CLS                                      'BACKGROUNG INFO.
	SCREEN 9

	FOR I = 1 TO 64                          'INITIALIZE INITIAL PRESSURE
	AR(I) = 299                              'BAR HEIGHTS
	NEXT I

	COLOR 3                                  'BLANK OUT BOTTOM OF SCREEN
	LINE (0, 270)-(640, 350), 0, BF

	LINE (65, 60)-(65, 300)                  'DRAW IN AXIS
	LINE (65, 300)-(600, 300)
	LINE (600, 300)-(600, 60)

	K = 60                                   'DRAW IN INDICATION BARS
	FOR I = 1 TO 7                           'ON AXIS
	LINE (59, K)-(71, K)
	LINE (594, K)-(606, K)
	K = K + 40
	NEXT I

	J = 60
	FOR I = 1 TO 25
	LINE (63, J)-(67, J)
	LINE (598, J)-(602, J)
	J = J + 10
	NEXT I

	LINE (80, 295)-(80, 305)                 'DRAW IN PORT LOCATION
	LINE (153, 295)-(153, 305)               'LINES ON BOTTOM AXIS
	LINE (243, 295)-(243, 305)
	LINE (333, 295)-(333, 305)
	LINE (423, 295)-(423, 305)
	LINE (513, 295)-(513, 305)
	LINE (585, 295)-(585, 305)

	LOCATE 5, 7                             'PRESSURE VALUE LABELS
	PRINT "6"
	LOCATE 8, 7
	PRINT "5"
	LOCATE 10, 7
	PRINT "4"
	LOCATE 13, 7
	PRINT "3"
	LOCATE 16, 7
	PRINT "2"
	LOCATE 19, 7
	PRINT "1"

	LOCATE 5, 77
	PRINT " 6"
	LOCATE 8, 77
	PRINT " 5"
	LOCATE 10, 77
	PRINT " 4"
	LOCATE 13, 77
	PRINT " 3"
	LOCATE 16, 77
	PRINT " 2"
	LOCATE 19, 77
	PRINT " 1"

	LOCATE 13, 1                             'PSIA LABEL
	PRINT "P"
	LOCATE 14, 1
	PRINT "S"
	LOCATE 15, 1
	PRINT "I"
	LOCATE 16, 1
	PRINT "A"

	LOCATE 23, 10                              'PORT NO. LABELS
	PRINT "579"
	LOCATE 23, 19
	PRINT "575"
	LOCATE 23, 30
	PRINT "570"
	LOCATE 23, 41
	PRINT "565"
	LOCATE 23, 52
	PRINT "560"
	LOCATE 23, 64
	PRINT "555"
	LOCATE 23, 73
	PRINT "551"

	LOCATE 2, 19                             'PRIMARY COMPRESSOR
	PRINT "MAIN INLET= "                     'PRESSURE LABELS
	LOCATE 3, 19
	PRINT "MAIN EXIT = "
	LOCATE 4, 19
	PRINT "MAIN RATIO= "

	LOCATE 2, 51                             'SECONDARY COMPRESSOR
	PRINT "SEC. INLET= "                     'PRESSURE LABELS
	LOCATE 3, 51
	PRINT "SEC. EXIT = "
	LOCATE 4, 51
	PRINT "SEC. RATIO= "

	LOCATE 1, 38
	PRINT "ATM= "



	RETURN
'____________________________________________________________________________

						 'FOREGROUND DISPLAY ROUTINE
DRW:    LOCATE 1, 1
	PRINT DATE$
	LOCATE 1, 70
	PRINT TIME$

	LOCATE 1, 43                             'DISPLAY PRIMARY COMPRESSOR
	PRINT USING "##.##"; QPRES(30)           'DATA

	LOCATE 2, 31
	PRINT USING "##.##"; QPRES(57)
	LOCATE 3, 31
	PRINT USING "##.##"; QPRES(58)
	LOCATE 4, 31
	PRINT USING "##.###"; RAT

	LOCATE 2, 63                             'DISPLAY SECONDARY COMPRESSOR
	PRINT USING "##.##"; QPRES(59)           'DATA
	LOCATE 3, 63
	PRINT USING "##.##"; QPRES(60)
	LOCATE 4, 63
	PRINT USING "##.###"; SRAT

	FOR I = 1 TO 30                          'CONVERT PSIA DATA READINGS
	AY(I) = 300 - ((QPRES(I) * 40))          'TO PIXELS
	IF AY(I) < 60 THEN AY(I) = 60            'BARS ARE DISPLAYED FROM
	IF AY(I) > 299 THEN AY(I) = 299          '0 TO 7 PSIA
	NEXT I

	CM = 2                                   'DRAW IN PRESSURE BARS
	AW = 592                                 'NEW PRESSURE VALUES ARE
	FOR I = 1 TO 29                          'COMPARED WITH OLD PRESSURE
						 'VALUES AND BAR HEIGHT IS
	AWY = AW - 14                            'UPDATED TO NEW VALUE
	IF AY(I) = AR(I) THEN GOTO S1
	IF AY(I) > AR(I) THEN LINE (AW, (AY(I) - 1))-(AWY, AR(I)), 0, BF
	IF AY(I) < AR(I) THEN LINE (AW, AR(I))-(AWY, AY(I)), CM, BF
S1:     AW = AW - 18
	AR(I) = AY(I)
	NEXT I

	RETURN
	
'_____________________________________________________________________________
EGAPRINT:                                                'PRINT ROUTINE
	
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
'____________________________________________________________________________

DCAL: CLS                                                'SCREEN DISPLAY
    LOCATE 5, 10                                         'FOR CALIBRATION
    PRINT "CALIBRATION PRESSURE VALUES"                  'PRESSURES (SETRAS)
T1: GOSUB CAL
    LOCATE 10, 10
    PRINT "SPAN1 PRESSURE -  SETRA PT1 = ";
    PRINT USING "##.##"; QCAL2;
    PRINT " PSIA"
    LOCATE 11, 10
    PRINT "SPAN2 PRESSURE -  SETRA PT2 = ";
    PRINT USING "##.##"; QCAL3;
    PRINT " PSIA"
    LOCATE 12, 10
    PRINT "VAC. PRESSURE  -  SETRA PT3 = ";
    PRINT USING "##.##"; QCAL1;
    PRINT " PSIA"
    LOCATE 17, 10
    PRINT "PRESS  Q  TO EXIT SCREEN AND CONTINUE SCANNING"
    PKEY$ = INKEY$
    IF PKEY$ = "Q" OR PKEY$ = "q" THEN GOTO T2
    GOTO T1
T2: GOTO LOOP0
    RETURN
'____________________________________________________________________________






    


  
  

