;Hogforge Dry Run Test
;Test the Movement of the Plates, General Gcode movement, and everything else

G31 ; Home All
G32 Z60 ; Prep The build chamber
M21 ; Turn the fan off
M19 ; Request User to Insert Material -- A break is here
G33 ; Move the bar

M15 T10 ;Activate the Vacuum Pump for 3 Minutes
M16 ; Turn the pump off
M20 ; Turn Fan on
M17 T10 ; Turn Argon Solenoid On -- Have fan circulate new argon
M18 ; Turn Argon Solenoid Off


M13 P50 F50000 W150 ; Configure Laser Parameters


G0 X13500 Y13500            ; Rapid to origin (internally calls M55)


;----- Print is ready!


; G1 X Y calls the Laser on and off
; G0 X Y executes a move.
; M13 Configures the power output

G4 S5 ; Wait 5 seconds 

; Layer 0 finished -- Execute the following:

M21 ; Turn the fan off
G34 Z.5 ; Execute a small step
G33 ; Move the bar
M20 ; Turn the fan on

;Begin Layer 1 ----

G4 S5 ; Wait 5 seconds 

; End of Layer 1 -----

M21 ; Turn the fan off
G34 Z.5 ; Execute a small step -- should coordinate with layer height
G33 ; Move the bar
M20 ; Turn the fan on

; Begin of Layer 2

G4 S5 ; Wait 5 seconds 

; End of Layer 2 -----

M21 ; Turn the fan off
G34 Z.5 ; Execute a small step -- should coordinate with layer height
G33 ; Move the bar
M20 ; Turn the fan on

; Begin of Layer 3

G4 S5 ; Wait 5 seconds 

; End of Layer 3 -----

M21 ; Turn the fan off
G34 Z.5 ; Execute a small step -- should coordinate with layer height
G33 ; Move the bar
M20 ; Turn the fan on


; Begin of Layer 3

G4 S5 ; Wait 5 seconds 

; End of Layer 3 -----

M21 ; Turn the fan off
G34 Z.5 ; Execute a small step -- should coordinate with layer height
G33 ; Move the bar
M20 ; Turn the fan on


; Begin of Layer 3

G4 S5 ; Wait 5 seconds 

; End of Layer 3 -----

M21 ; Turn the fan off
G34 Z.5 ; Execute a small step -- should coordinate with layer height
G33 ; Move the bar
M20 ; Turn the fan on


; Begin of Layer 3

G4 S5 ; Wait 5 seconds 

; End of Layer 3 -----

M21 ; Turn the fan off
G34 Z.5 ; Execute a small step -- should coordinate with layer height
G33 ; Move the bar
M20 ; Turn the fan on


; Begin of Layer 4

G4 S5 ; Wait 5 seconds 

; End of Layer 4 -----

M21 ; Turn the fan off
G34 Z.5 ; Execute a small step -- should coordinate with layer height
G33 ; Move the bar
M20 ; Turn the fan on

; Begin of Layer 5

G4 S5 ; Wait 5 seconds 

; End of Layer 5 -----

M21 ; Turn the fan off
G34 Z.5 ; Execute a small step -- should coordinate with layer height
G33 ; Move the bar
M20 ; Turn the fan on

; Begin of Layer 6

G4 S5 ; Wait 5 seconds 

; End of Layer 6 -----

M21 ; Turn the fan off
G34 Z.5 ; Execute a small step -- should coordinate with layer height
G33 ; Move the bar
M20 ; Turn the fan on

; Begin of Layer 7

G4 S5 ; Wait 5 seconds 

; End of Layer 7 -----

M21 ; Turn the fan off
G34 Z.5 ; Execute a small step -- should coordinate with layer height
G33 ; Move the bar
M20 ; Turn the fan on


; Begin of Layer 8

G4 S5 ; Wait 5 seconds 

; End of Layer 8 -----

M21 ; Turn the fan off
G34 Z.5 ; Execute a small step -- should coordinate with layer height
G33 ; Move the bar
M20 ; Turn the fan on


; Begin of Layer 9

G4 S5 ; Wait 5 seconds 

; End of Layer 9 -----

M21 ; Turn the fan off
G34 Z.5 ; Execute a small step -- should coordinate with layer height
G33 ; Move the bar
M20 ; Turn the fan on


; Begin of Layer 10

G4 S5 ; Wait 5 seconds 

; End of Layer 10 -----

M21 ; Turn the fan off
G34 Z.5 ; Execute a small step -- should coordinate with layer height
G33 ; Move the bar
M20 ; Turn the fan on




; ------ Print is finished!

M21 ; Turn the fan off
M14 ; Turn the laser off
M15 T180 ; Activate the Pump for 3 minutes


; Gcode finished!!!

