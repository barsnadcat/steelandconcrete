/*
-----------------------------------------------------------------------------
This source file is part of QuickGUI
For the latest info, see http://www.ogre3d.org/addonforums/viewforum.php?f=13

Copyright (c) 2009 Stormsong Entertainment

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

(http://opensource.org/licenses/mit-license.php)
-----------------------------------------------------------------------------
*/

#ifndef QUICKGUITEXT_H
#define QUICKGUITEXT_H

#include "QuickGUIDesc.h"
#include "QuickGUIException.h"
#include "QuickGUIExportDLL.h"
#include "QuickGUISize.h"
#include "QuickGUITextAlignment.h"
#include "QuickGUIUVRect.h"
#include "QuickGUIOgreEquivalents.h"

#include "OgrePrerequisites.h"
#include "OgreUTFString.h"

#include <list>
#include <vector>

namespace Ogre
{
	class Font;
	class Texture;
}

namespace QuickGUI
{
	// forward declarations
	class Character;
	class Text;
	class TextLine;

	class _QuickGUIExport TextSegment
	{
	public:
		TextSegment();
		TextSegment(const Ogre::String FontName, const ColourValue& Color, const Ogre::UTFString& Text);
		TextSegment(const Ogre::String FontName, const Ogre::UTFString& Text);
		TextSegment(const ColourValue& Color, const Ogre::UTFString& Text);
		TextSegment(const Ogre::UTFString& Text);

		// Font of this segment of text
		Ogre::String fontName;
		// Color of this segment of text
		ColourValue color;
		// Array of code points
		Ogre::UTFString text;
	};

	class _QuickGUIExport TextDesc :
		public Desc
	{
	public:
		TextDesc();

		Size allottedSize;
		BrushFilterMode	brushFilterMode;
		HorizontalTextAlignment horizontalTextAlignment;
		float verticalLineSpacing;
		std::vector<TextSegment> segments;
		VerticalTextAlignment verticalTextAlignment;
		// Amount of pixels to shift text, after it has been horizontally and vertically aligned.
		Point offset;

		// Adds performance boost by letting Desc update its segments before serial writing,
		// Instead of updating segments for every text changing operation.
		Text* text;

		/**
		* Returns the class of Desc object this is.
		*/
		virtual Ogre::String getClass() { return "TextDesc"; }
		/**
		* Returns the height of the Text if it were treated as one line of text.
		*/
		float getTextHeight();
		/**
		* Returns the number of characters.
		*/
		int getTextLength();
		/**
		* Returns the width of the Text if it were treated as one line of text.
		*/
		float getTextWidth();

		/**
		* Restore properties to default values
		*/
		void resetToDefault();

		/**
		* Outlines how the desc class is written to XML and read from XML.
		*/
		virtual void serialize(SerialBase* b);
	};

	class _QuickGUIExport Text
	{
	public:
		friend class SerialWriter;
	public:
		/// Define Unicode constants
		enum 
		{
			UNICODE_NEL		= 0x0085,
			UNICODE_CR		= 0x000D,
			UNICODE_LF		= 0x000A,
			UNICODE_TAB		= 0x0009,
			UNICODE_SPACE	= 0x0020,
			UNICODE_ZERO	= 0x0030,
		};

		/**
		* Returns true if the given unicode character is a Tab, false otherwise.
		*/
		static inline bool	isTab			(Ogre::UTFString::unicode_char c) { return c == UNICODE_TAB; }
		/**
		* Returns true if the given unicode character is a Space, false otherwise.
		*/
		static inline bool	isSpace			(Ogre::UTFString::unicode_char c) { return c == UNICODE_SPACE; }
		/**
		* Returns true if the given unicode character is a NewLine ('\n' or '\r'), false otherwise.
		*/
		static inline bool	isNewLine		(Ogre::UTFString::unicode_char c) { return c == UNICODE_CR || c == UNICODE_LF || c == UNICODE_NEL; }
		/**
		* Returns true if the given unicode character is a Null character, false otherwise.
		*/
		static inline bool  isNullCharacter (Ogre::UTFString::unicode_char c) { return c == UNICODE_NEL; }
		/**
		* Returns true if the given unicode character is a Space, Tab, or NewLine, false otherwise.
		*/
		static inline bool	isWhiteSpace	(Ogre::UTFString::unicode_char c) { return isTab(c) || isSpace(c) || isNewLine(c); }
	public:
		/**
		* Converts '\n' and '\r' into "\\n", and '\t' into "\\t".
		*/
		static void convertWhitespaceToText(Ogre::UTFString& s);
		/**
		* Converts "\\n" and "\\r" into '\n', and "\\t" into '\t'.
		*/
		static void convertTextToWhiteSpace(Ogre::UTFString& s);
		/**
		* Retrieves a handle to the first font found.
		*/
		static Ogre::Font* getFirstAvailableFont();
		/**
		* Retrieves a handle to a font by its name.
		*/
		static Ogre::Font* getFont(const Ogre::String& fontName);
		/**
		* Returns the height in pixels of any glyphs within a particular font.
		* NOTE: All glyphs of the same font have the same height.
		*/
		static float getFontHeight(const Ogre::String& fontName);
		/**
		* Returns the height in pixels of any glyphs within a particular font.
		* NOTE: All glyphs of the same font have the same height.
		*/
		static float getFontHeight(Ogre::Font* fp);
		/**
		* Returns the Texture generated by the Ogre::Font class, for a given font.
		*/
		static Ogre::Texture* getFontTexture(const Ogre::String& fontName);
		/**
		* Returns the Texture generated by the Ogre::Font class, for a given font.
		*/
		static Ogre::Texture* getFontTexture(Ogre::Font* fp);
		/**
		* Returns the name of the Texture generated by the Ogre::Font class, for a given font.
		*/
		static Ogre::String getFontTextureName(const Ogre::String& fontName);
		/**
		* Returns the name of the Texture generated by the Ogre::Font class, for a given font.
		*/
		static Ogre::String getFontTextureName(Ogre::Font* fp);
		/**
		* Returns the pixel height of a particular code point for a specified font.
		*/
		static float getGlyphHeight(const Ogre::String& fontName, Ogre::UTFString::code_point cp);
		/**
		* Returns the pixel height of a particular code point for a specified font.
		*/
		static float getGlyphHeight(Ogre::Font* fp, Ogre::UTFString::code_point cp);
		/**
		* Returns the UV coordinates of a particular code point for a specified font.
		*/
		static UVRect getGlyphUVCoords(const Ogre::String& fontName, Ogre::UTFString::code_point cp);
		/**
		* Returns the UV coordinates of a particular code point for a specified font.
		*/
		static UVRect getGlyphUVCoords(Ogre::Font* fp, Ogre::UTFString::code_point cp);
		/**
		* Returns the pixel size of a particular code point for a specified font.
		*/
		static Size getGlyphSize(const Ogre::String& fontName, Ogre::UTFString::code_point cp);
		/**
		* Returns the pixel size of a particular code point for a specified font.
		*/
		static Size getGlyphSize(Ogre::Font* fp, Ogre::UTFString::code_point cp);
		/**
		* Returns the pixel width of a particular code point for a specified font.
		*/
		static float getGlyphWidth(const Ogre::String& fontName, Ogre::UTFString::code_point cp);
		/**
		* Returns the pixel width of a particular code point for a specified font.
		*/
		static float getGlyphWidth(Ogre::Font* fp, Ogre::UTFString::code_point cp);
		/**
		* Returns the pixel width of a particular string of text for a specified font.
		*/
		static float getTextWidth(const Ogre::String& fontName, Ogre::UTFString s);
		/**
		* Returns the pixel width of a particular string of text for a specified font.
		*/
		static float getTextWidth(Ogre::Font* fp, Ogre::UTFString s);
		/**
		* Saves a font texture to file.
		*/
		static void saveFontTextureToFile(const Ogre::String& fontName, const Ogre::String& fileName);
		/**
		* Saves a font texture to file.
		*/
		static void saveFontTextureToFile(Ogre::Font* fp, const Ogre::String& fileName);
	public:
		Text(TextDesc& d);
		~Text();

		/**
		* Inserts a character at the specified index.
		* NOTE: The Text class will delete characters on destruction.
		*/
		void addCharacter(Character* c, unsigned int index);
		/**
		* Inserts a character to the end of the text.
		* NOTE: The Text class will delete characters on destruction.
		*/
		void addCharacter(Character* c);
		/**
		* Adds text to this object.
		*/
		void addText(Ogre::UTFString s, Ogre::Font* fp, const ColourValue& cv);
		/**
		* Adds Text using Text Segments.
		*/
		void addText(std::vector<TextSegment> segments);
		/**
		* Adds new line of text to this object.
		*/
		void addTextLine(Ogre::UTFString s, Ogre::Font* fp, const ColourValue& cv);
		/**
		* Adds new line of Text using Text Segments.
		*/
		void addTextLine(std::vector<TextSegment> segments);

		/**
		* Removes all highlighting from text.
		*/
		void clearHighlights();
		/**
		* Clears text.
		*/
		void clearText();

		/**
		* Draws the text to the current render target.
		*/
		void draw(Point& position);

		/**
		* Returns true if the text has no characters, false otherwise.
		*/
		bool empty();

		/**
		* Returns the amount of height defined for this text. The height defines
		* how the text will be aligned.
		* NOTE: An allotted height of 0 means no vertical centering will occur.
		*/
		float getAllottedHeight();
		/**
		* Returns the allowable width and height for this text. Allotted width
		* controls line wrapping and horizontal centering.  Allotted height controls
		* vertical centering.
		* NOTE: A width of 0 means line wrapping and horizontal alignment are disabled.
		* NOTE: A height of 0 means vertical alignment is disabled.
		*/
		Size getAllottedSize();
		/**
		* Returns the amount of width defined for this text.  Word wrapping occurs
		* when the allotted width is smaller than the width of the text.
		* NOTE: An allotted width of 0 means no text wrapping will occur.
		* NOTE: An allotted width of 0 means no horizontal centering will occur.
		*/
		float getAllottedWidth();
		/**
		* Returns the average height of all TextLines.
		*/
		float getAverageTextLineHeight();
		/**
		* Returns the filtering used when drawing the skin of this widget.
		*/
		BrushFilterMode getBrushFilterMode();
		/**
		* Returns the Character at the given index.
		*/
		Character* getCharacter(unsigned int index);
		/**
		* Returns the position of the character given relative to the Text.
		* NOTE: The position of the Character class is relative to its TextLine.
		*/
		Point getCharacterPosition(unsigned int index);
		/**
		* Returns the y position of the character given relative to the Text.
		* NOTE: The y position of the Character class is relative to its TextLine.
		*/
		float getCharacterYPosition(unsigned int index);
		/**
		* Iterates through all TextLines and returns the index of the character
		* closest to the position given.
		*/
		int getCursorIndexAtPosition(const Point& p);
		/**
		* Returns the horizontal alignment of this text.
		*/
		HorizontalTextAlignment getHorizontalTextAlignment();
		/**
		* Returns the index of the next word after the index given, or -1 if
		* no more words remain.
		*/
		int getIndexOfNextWord(unsigned int index);
		/**
		* Returns the index of the previous word before the index given, or the beginning if
		* no more words are before the current one.
		*/
		int getIndexOfPreviousWord(unsigned int index);
		/**
		* Returns the index of the last character in the TextLine with the index given.
		*/
		int getIndexOfTextLineBegin(unsigned int index);
		/**
		* Returns the index of the first character in the TextLine with the index given.
		*/
		int getIndexOfTextLineEnd(unsigned int index);
		/**
		* Returns the number of characters in this Text.
		*/
		int getLength();
		/**
		* Returns the offset of the text, applied after text is aligned horizontally and vertically.
		*/
		Point getOffset();
		/**
		* Returns the position of the character at the index specified.
		*/
		Point getPositionAtCharacterIndex(unsigned int index);
		/**
		* Returns the Width and Height of this text.
		*/
		Size getSize();
		/**
		* Returns a string of characters representing the text.
		*/
		Ogre::UTFString getText();
		/**
		* Returns the desc object representing this Text instance.
		*/
		TextDesc* getTextDesc();
		/**
		* Returns the height of text.
		* NOTE: the allotted width affects the number of TextLines generated, which affects the height.
		*/
		float getTextHeight();
		/**
		* Returns the TextLine from the Character index given.
		*/
		TextLine* getTextLineFromIndex(unsigned int index);
		/**
		* Returns the index of the first character in the TextLine given.
		*/
		int getTextLineBeginIndex(TextLine* textLine);
		/**
		* Returns the index of the last character in the TextLine given.
		*/
		int getTextLineEndIndex(TextLine* textLine);
		/**
		* Returns the width of text as if it were all on one line.
		*/
		float getTextWidth();
		/**
		* Returns a list of Text Segments.  Each Text Segment has the same color and font.
		*/
		std::vector<TextSegment> getTextSegments();
		/**
		* Returns the number of pixels placed between each line of text, if there
		* are multiple lines of text.
		*/
		float getVerticalLineSpacing();
		/**
		* Returns the vertical alignment of this text.
		*/
		VerticalTextAlignment getVerticalTextAlignment();

		/**
		* Highlights all the text.
		*/
		void highlight();
		/**
		* Highlights the character at the index given.
		*/
		void highlight(unsigned int index);
		/**
		* Highlights all characters within the defined range.
		*/
		void highlight(unsigned int startIndex, unsigned int endIndex);
		/**
		* Searches text for c.  If allOccurrences is true, all characters of text matching c
		* will be highlighted, otherwise only the first occurrence is highlighted.
		*/
		void highlight(Ogre::UTFString::code_point c, bool allOccurrences);
		/**
		* Searches text for s.  If allOccurrences is true, all sub strings of text matching s
		* will be highlighted, otherwise only the first occurrence is highlighted.
		*/
		void highlight(Ogre::UTFString s, bool allOccurrences);

		/**
		* Removes a character from the index given.
		*/
		void removeCharacter(unsigned int index);

		/**
		* Defines the amount of height allowed for this text. Text written past
		* the allotted height will be clipped.
		* NOTE: An allotted height of 0 means no vertical alignment will occur.
		*/
		void setAllottedHeight(float pixelHeight);
		/**
		* Defines the allowable width and height for this text. Allotted width
		* controls line wrapping and horizontal centering.  Allotted height controls
		* vertical centering.
		* NOTE: A width of 0 will disable line wrapping and horizontal alignment.
		* NOTE: A height of 0 will disable vertical alignment.
		*/
		void setAllottedSize(Size s);
		/**
		* Defines the amount of width allowed for this text.  Word wrapping occurs
		* when the allotted width is smaller than the width of the text.
		* NOTE: An allotted width of 0 means no wrapping will occur.
		* NOTE: An allotted width of 0 means no horizontal alignment will occur.
		*/
		void setAllottedWidth(float pixelWidth);
		/**
		* Sets the filtering used when drawing the text.
		*/
		void setBrushFilterMode(BrushFilterMode m);
		/**
		* Sets all characters of the text to the specified color.
		*/
		void setColor(const ColourValue& cv);
		/**
		* Sets the character at the index given to the specified color.
		*/
		void setColor(const ColourValue& cv, unsigned int index);
		/**
		* Sets all characters within the defined range to the specified color.
		*/
		void setColor(const ColourValue& cv, unsigned int startIndex, unsigned int endIndex);
		/**
		* Searches text for c.  If allOccurrences is true, all characters of text matching c
		* will be colored, otherwise only the first occurrence is colored.
		*/
		void setColor(const ColourValue& cv, Ogre::UTFString::code_point c, bool allOccurrences);
		/**
		* Searches text for s.  If allOccurrences is true, all sub strings of text matching s
		* will be colored, otherwise only the first occurrence is colored.
		*/
		void setColor(const ColourValue& cv, Ogre::UTFString s, bool allOccurrences);
		/**
		* Sets all characters of the text to the specified font.
		*/
		void setFont(const Ogre::String& fontName);
		/**
		* Sets the character at the index given to the specified font.
		*/
		void setFont(const Ogre::String& fontName, unsigned int index);
		/**
		* Sets all characters within the defined range to the specified font.
		*/
		void setFont(const Ogre::String& fontName, unsigned int startIndex, unsigned int endIndex);
		/**
		* Searches text for c.  If allOccurrences is true, all characters of text matching c
		* will be changed to the font specified, otherwise only the first occurrence is changed.
		*/
		void setFont(const Ogre::String& fontName, Ogre::UTFString::code_point c, bool allOccurrences);
		/**
		* Searches text for s.  If allOccurrences is true, all sub strings of text matching s
		* will be changed to the font specified, otherwise only the first occurrence is changed.
		*/
		void setFont(const Ogre::String& fontName, Ogre::UTFString s, bool allOccurrences);
		/**
		* Sets whether or not the Text will be masked. (password box)
		*/
		void setMaskText(bool mask, Ogre::UTFString::code_point maskSymbol);
		/**
		* Sets the offset of the text, applied after text is aligned horizontally and vertically.
		*/
		void setOffset(Point offset);
		/**
		* Sets the text for this object.
		*/
		void setText(Ogre::UTFString s, Ogre::Font* fp, const ColourValue& cv);
		/**
		* Sets the Text using Text Segments.
		*/
		void setText(std::vector<TextSegment> segments);
		/**
		* Sets the Horizontal alignment of Text when drawn.
		*/
		void setHorizontalTextAlignment(HorizontalTextAlignment a);
		/**
		* Sets the number of pixels placed between each line of text, if there
		* are multiple lines of text.
		*/
		void setVerticalLineSpacing(float distance);
		/**
		* Sets the Vertical alignment of Text when drawn.
		*/
		void setVerticalTextAlignment(VerticalTextAlignment a);

		/**
		* Forces recalculation of TextLines.
		*/
		void update();

	protected:
		TextDesc* mTextDesc;

		bool mMaskText;
		Ogre::UTFString::code_point mMaskSymbol;

		Ogre::UTFString mText;
		// Store information of every character in this Text object
		std::list<Character*> mCharacters;

		std::vector<TextLine*> mTextLines;

		// Store whether TextLines need to be updated or not
		bool mTextLinesDirty;

		/**
		* Checks if TextLines need to be updated.  If so, they are
		* destroyed and recreated.
		*/
		void _checkTextLines();
		/**
		* Create Text Lines and populate them on a word by word basis
		*/
		void _createTextLines();
		/**
		* Destroys Text Lines and restores Character position to default. (0,0)
		*/
		void _destroyTextLines();
		/**
		* Draws Text Lines to current Render Target starting at point p.
		*/
		void _drawTextLines(Point& position);

	private:
	};
}

#endif
