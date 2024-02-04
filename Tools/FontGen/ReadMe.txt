// ----------------------------------------------------------------------------
//  _____                 _ __  __        _        _
// |  __ \               | |  \/  |      | |      | |
// | |__) | ___  __ _  __| | \  / | ___  | |___  __ |_
// |  _  / / _ \/ _` |/ _` | |\/| |/ _ \ | __\ \/ / __|
// | | \ \|  __/ (_| | (_| | |  | |  __/_| |_ >  <| |_
// |_|  \_\\___|\__,_|\__,_|_|  |_|\___(_)\__/_/\_\\__|
//                                                               
// ----------------------------------------------------------------------------
// Originally created on 12/18/2000 by Paul Nettle
//
// Copyright 2000, Fluid Studios, Inc., all rights reserved.
// ----------------------------------------------------------------------------

The FontGen tool generates font files (*.f) for use in applications that must
render their own fonts.

This tool grabs the rendered font from Windows and creates a bitmap of the
rendered font, which includes an alpha channel. This alpha channel can be used
for generating text that overlays well on any background of any color, because
it is to be blended in with the background, not overlaied on top.

Note, in order for this to work, Windows must be set to "Smooth edges of screen
fonts". On my Windows2000 box, this is under "effects" of the display
properties dialog. Note that this is only necessary to be set when running this
font generation tool, not when rendering the fonts, because it's your job to
create the font rendering code to work however you like. :)

Also note that the complete font placement and with is also stored. This means
that, when properly rendered, the characters of a string all appear properly
spaced out (kerned) and aligned with one another.

The format of the font file is very simple. There are 256 characters stored in
the file, and each character is stored in the following format:

  4 bytes	byteWidth    -- Width of the font (in bytes)
  4 bytes	byteHeight   -- Height of the font (in bytes)
  4 bytes	xOffset      -- X character placement offset (in pixels)
  4 bytes	yOffset      -- Y character placement offset (in pixels)
  4 bytes	screenWidth  -- Width of the font (in screen coordinates)
  4 bytes	screenHeight -- Height of the font (in screen coordinates)
 ?? bytes	fontData     -- Array of byteWidth by byteHeight pixels

		[All integer values are stored in Intel format]

The byte width & height of each character is so that you may read the character
from the file, but also so that you know how big the character is in memory.
These dimensions have nothing to do with character placement, aligntment or
kerning.

The remaining values are for pixel placement, alignment and kerning.

The X & Y offsets are used for pixel placement & alignment. When rendering a
character, you'll need some reference point for where to render, such as "draw
the letter Q at [187,224]". If you were to draw a string of letters starting
at that point, the letters would each have a different baseline. This means
that each letter is actually positioned at a different offset above or below
the given input coordinate. The same is true for the horizontal dimension. In
order to correctly place and align your characters, add the X & Y offsets to
your input coordinate. For example, given the input coordinate of [187,224] and
an X/Y offset of [-3,-8], the upper-left corner of the character would be at
[184,216]. You would then begin rendering the entire character (byteWidth by
byteHeight) at that new location.

The screen width & height define the character extents (see CDC::GetTextExtent
in the Microsoft MFC documentation). Use these values to determine how wide
a character is (in pixels), so that you can render them with proper kerning, or
to calculate the total dimensions of a string of characters.

The pixels are stored as alpha values. A value of 0 means the pixel is
completely transparent, and a value of 255 means that the pixel is completely
opaque. These are mono-color fonts, so no color information is stored in the
file, it's whatever color you decide to draw it in.

// ----------------------------------------------------------------------------
// ReadMe.txt - End of file
// ----------------------------------------------------------------------------
