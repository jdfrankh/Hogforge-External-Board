;Hogforge Laser Test - matches activateFiberLaserDebug scan pattern
;Step 10

; --- Setup ---


G31 ; Home All chamber items
G32 Z60 ; Prep The build chamber
M21 ; Turn the fan off
M19 ; Request User to Insert Material -- A break is here
G33 ; Move the bar

M15 T10 ;Activate the Vacuum Pump for 3 Minutes
M16 ; Turn the pump off
M20 ; Turn Fan on
M17 T10 ; Turn Argon Solenoid On -- Have fan circulate new argon
M18 ; Turn Argon Solenoid Off



 ; G28 ; Home the Galvo -- DISABLED
G0 X13500 Y13500            ; Rapid to build plate origin (calls M55 internally)
G92 X0 Y0                  ; Set this position as coordinate origin (matches activateFiberLaserDebug)

M57 L1              ; Guide beam off

M13 P70 F50000 W128  ; 70% power, 50kHz PRR, 128 width
M58 F3000 ; Set speed (mm/min) — 300000 mm/min ≈ 5000 mm/s
M54                 ; Re-enable laser after rapid move

G4 S1;

; --- Perimeter ---
; Loop 1 - Left side: X=0, Y=0..1900
G0 X0 Y0

; Loop 2 - Top: X=0..1900, Y=2000
G1 X0 Y2000
; Loop 3 - Right side: X=2000, Y=0..1900
G1 X2000 Y2000

; Loop 4 - Bottom: X=0..1900, Y=0
G1 X0 Y0

; --- Raster fill: X=0..1900 step 100, Y=0..1900 step 100 per column ---
; Col X=0
G1 X0 Y0

G1 X0 Y1900
; Col X=100
G0 X100 Y0
G1 X100 Y1900
; Col X=200
G0 X200 Y0
G1 X200 Y1900
; Col X=300
G0 X300 Y0
G1 X300 Y1900
; Col X=400
G0 X400 Y0
G1 X400 Y1900
; Col X=500
G0 X500 Y0
G1 X500 Y1900
; Col X=600
G0 X600 Y0
G1 X600 Y1900
; Col X=700
G0 X700 Y0
G1 X700 Y1900
; Col X=800
G0 X800 Y0
G1 X800 Y1900
; Col X=900
G0 X900 Y0
G1 X900 Y1900
; Col X=1000
G0 X1000 Y0
G1 X1000 Y1900
; Col X=1100
G0 X1100 Y0
G1 X1100 Y1900
; Col X=1200
G0 X1200 Y0
G1 X1200 Y1900
; Col X=1300
G0 X1300 Y0
G1 X1300 Y1900
; Col X=1400
G0 X1400 Y0
G1 X1400 Y1900
; Col X=1500
G0 X1500 Y0
G1 X1500 Y1900
; Col X=1600
G0 X1600 Y0
G1 X1600 Y1900
; Col X=1700
G0 X1700 Y0
G1 X1700 Y1900
; Col X=1800
G0 X1800 Y0
G1 X1800 Y1900
; Col X=1900
G0 X1900 Y0
G1 X1900 Y1900

M55 ; Laser off
M21 ; Turn Fan off
