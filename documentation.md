# Getting Started
The code setup is easy, all important values that say adjustable next to them are able to be set to whatever you need.
You may find the pulsewidth is limited to low for some bigger tesla coils or regular solid state tesla coils, just increase
the value to whatever is suitable for your coil. And make sure you have the 4 libraries, Wire for i2c comm with the i2c lcd, 
Liquid_Crystal_I2C for easy controlling of the i2c lcd, MIDI for the midi comm, and TimerOne. All these you should be able 
to find in the arduino ide library manager. 

# Hardware
the hardware consists of the i2c 16x2 lcd wired to arduino sda and scl, and each button for next previous select and on off in 
their respective order are pins are 2, 3, 4, 5, and each button pulls the pins down to ground. last pin is the output which 
is pin 9. You can attach a fiber optic transmitter or a bnc connector or other shielded cable.
Make sure to shield the interupter with foil tape or metal enclosure and you can also put a ferrite bead around the wire going to 
the tesla coil if you are using a regular wire as well as an optocoupler at the tesla coil. As stated before a fiber optic 
connection is preffered due too complete isolation and no need for ferrite beads or extra optocouplers.

# UI
the UI is pretty simple, when you first turn on the interupter, you will see the burst mode page appear. You can press the 
select button to select different options indicated by a star. When the star is next to the page name, you are able to change modes,
pressing previous will take you to the single pulse mode, pressing next from the burst mode will take you to the continuous 
interuption mode, and pressing next again will take you to the midi mode. To exit the midi mode, you press the select button. When
in Burst mode, you can press the select button to cycle through five options, by default the name of the pasge will be selected, next
is on time in microseconds, Bps in hertz, burst on time, and burst off time. every other page is the same except with two options,
frequency and ontime, with midi mode not having frequency.
