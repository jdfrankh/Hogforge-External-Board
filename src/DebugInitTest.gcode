; Hogforge Laser Test - Build plate 9500 x 12000 split into separate squares
; Power sweep: 10% .. 80%
; Two layer test

G31 ; Home All chamber items
G32 Z5 ; Prep build chamber - in mm
M21 ; Turn fan off
M19 ; Request user to insert material
G33 ; Move the bar
M20 ; Turn fan on


M19 ; Prompt the User to activate the argon tank 


M55 ; Turn off fiber laser
M21 ; Turn fan off
