# TeleCortex
Arduino firmware for receiving TeleCortex messages which communicate LED data via GCode-like serial commands

![TeleCortex TimeCrime DJing](https://i.imgur.com/rzYJR6z.gif)

## The protocol

Based on GCode, with a sprinkling of ethernet.

Each command is a list of fields that are separated by whitespace. A field can be interpreted as a command, parameter, or for any other special purpose. It consists of one letter directly followed by a string of non-whitespace, printable characters.

### Fields

<table>
  <tr>
    <td>N</td>
    <td>Sequence Number (two ascii hex bytes which wrap around at 0xFF)</td>
  </tr>
  <tr>
    <td>P</td>
    <td>Command - Probing / querying commands (no payload required)</td>
  </tr>
  <tr>
    <td>M</td>
    <td>Command - Setting commands (may require payload)</td>
  </tr>
  <tr>
    <td>Q</td>
    <td>Parameter - Panel number</td>
  </tr>
  <tr>
    <td>S</td>
    <td>Parameter - Arbitrary integer parameter</td>
  </tr>
  <tr>
    <td>V</td>
    <td>Parameter - Base 64 encoded payload</td>
  </tr>
  <tr>
    <td>*</td>
    <td>Checksum (two ascii hex bytes representing the XOR of each character in the message before the checksum)</td>
  </tr>
</table>


### Responses

Server can respond with a status followed by an optional sequence number.

Status can be

* "OK {N}" = command N completed succesfully

* "RS {N}" = resend from command N

* E{error number} = command failed, Error Codes section for details

* "IDLE" = command buffer has been empty for a while

The error response has an optional parameter, V which is sometimes used in debugging

### Error Codes (incomplete)

<table>
  <tr>
    <td>E001</td>
    <td>Warning / recoverable error.</td>
  </tr>
  <tr>
    <td>E002</td>
    <td>Malloc Error
Board ran out of memory (usually too many pixels)</td>
  </tr>
  <tr>
    <td>E003</td>
    <td>EEPROM Error
Bad EEPROM
Writing to disabled EEPROM</td>
  </tr>
  <tr>
    <td>E005</td>
    <td>Invalid Panel Config
Panels not defined correctly in setup()</td>
  </tr>
  <tr>
    <td>E010</td>
    <td>Invalid Line Number
Line numbers not contiguous</td>
  </tr>
  <tr>
    <td>E011</td>
    <td>Invalid Command
Command Does not exist</td>
  </tr>
  <tr>
    <td>E012</td>
    <td>Invalid Panel number
Panel number (Q Parameter) could be too big
Q parameter holds panel ID</td>
  </tr>
  <tr>
    <td>E013</td>
    <td>Invalid S Parameter
Pixel offset bigger than panel</td>
  </tr>
  <tr>
    <td>E014</td>
    <td>Invalid payload
Payload (V Parameter) could be too long for buffer
Payload may not have all Base64 characters
Payload may be empty
V parameter holds length of payload</td>
  </tr>
  <tr>
    <td>E019</td>
    <td>Invalid Command Checksum
</td>
  </tr>
</table>


## GCodes

### M110: Set Current Line Number

Needed so that chunks of GCode can be replayed

Parameters:

* N = New linenum

Raises: None

### M500 Store current settings in EEPROM

Not Implemented

Settings will be read on the next startup or M501.

### M501 Read all parameters from EEPROM

Not Implemented

Or, undo changes.

### M502 Reset current settings to defaults

Not Implemented

as set in Configurations.h. (Follow with M500 to reset the EEPROM too.)

### M503 Print the current settings

Not Implemented

Not the settings stored in EEPROM.

### M508 Write EEPROM Code

Parameters:

* S = Offset

* V = Code (null terminated)

Raises: E013

### M509 Dump EEPROM Code

Sends a hexdump of EEPROM Code Section over serial

### P2205: Get UID

Causes server to respond with it’s unique Identifier.

Parameters: None,

Returns:

* S = UID (8 byte hex)

Notes: Arduino can’t see FTDI serial number, must be set in EEPROM

### P2206: Get Panel Count

Causes the server to respond with the number of panels it thinks it has

Parameters: None

Raises: None

### M2205: Set UID

deprecated?

Sets the server’s UID in EEPROM

Parameters:

* S = UID (8 byte hex)

### P2206: Get Panel Count

Get the panel count in EEPROM

### M2600: Set Panel - RGB Payload

Causes server to write panel frame to framebuffer in RBG format

Parameters:

* Q = Panel number (int)

* V = Panel payload (base 64 encoded)

* S = Pixel offset

Raises: E012, E013

### M2601: Set Panel - HSV Payload

Causes server to write panel frame to framebuffer in HSV format

Parameters:

* Q = Panel number (int)

* V = Panel payload (base 64 encoded)

* S = Pixel offset

Raises: E012, E013

### M2602: Set Panel - RGB Single

Causes the server to write a single RGB pixel from the base64 encoded payload to the entire panel.

Parameters:

* Q = Panel number (int)

* V = Panel payload (base 64 encoded)

### M2603: Set Panel - HSV Single

Causes the server to write a single HSV pixel from the base64 encoded payload to the entire panel.

Parameters:

* Q = Panel number (int)

* V = Panel payload (base 64 encoded)

### P2600: Dump Panel - RGB

Server responds with RGB dump of panel

### P2601: Get Panel - HSV

deprecated?

Server responds with HSV dump of panel

### M2610: Force Display All

Calls the LED library to display the frame buffers on the LED matrix

### M2611: Force Display Singular

Not Implemented

Calls the LED library to display the frame buffers on the LED matrix

Parameters:

* Q = Panel number (int)

## More information
[Blog posts](http://blog.laserphile.com/search/label/Cortex)
[The protocol]()
[Python client](https://github.com/Laserphile/Python-TeleCortex)
