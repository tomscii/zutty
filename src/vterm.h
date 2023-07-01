/* This file is part of Zutty.
 * Copyright (C) 2020 Tom Szilagyi
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * See the file LICENSE for the full license.
 */

#pragma once

#include "frame.h"
#include "utf8.h"

#include <cstdint>
#include <functional>
#include <memory>

namespace zutty
{
   enum class VtKey
   {
      NONE,

      Space, Return, Backspace, Tab, Backtick, Tilde,
      Up, Down, Left, Right,
      Insert, Delete, Home, End, PageUp, PageDown,
      F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
      F13, F14, F15, F16, F17, F18, F19, F20,
      K0, K1, K2, K3, K4, K5, K6, K7, K8, K9,

      KP_F1, KP_F2, KP_F3, KP_F4,
      KP_Insert, KP_Delete,
      KP_Up, KP_Down, KP_Left, KP_Right,
      KP_Home, KP_End, KP_PageUp, KP_PageDown, KP_Begin,
      KP_Plus, KP_Minus, KP_Star, KP_Slash, KP_Comma, KP_Dot,
      KP_Space, KP_Equal, KP_Tab, KP_Enter,
      KP_0, KP_1, KP_2, KP_3, KP_4, KP_5, KP_6, KP_7, KP_8, KP_9,

      Print
   };

   enum class VtModifier: uint8_t
   {
      none = 0,
      shift = 1,
      control = 2,
      shift_control = 3,
      alt = 4,
      shift_alt = 5,
      control_alt = 6,
      shift_control_alt = 7
   };
   constexpr VtModifier operator| (VtModifier m1, VtModifier m2)
   {
      return static_cast <VtModifier> (
         static_cast <uint8_t> (m1) | static_cast <uint8_t> (m2));
   }
   constexpr VtModifier operator& (VtModifier m1, VtModifier m2)
   {
      return static_cast <VtModifier> (
         static_cast <uint8_t> (m1) & static_cast <uint8_t> (m2));
   }

   enum class MouseTrackingMode: uint8_t
   { Disabled = 0, X10_Compat, VT200, VT200_ButtonEvent, VT200_AnyEvent };
   enum class MouseTrackingEnc: uint8_t
   { Default = 0, UTF8, SGR, URXVT };
   struct MouseTrackingState
   {
      MouseTrackingMode mode = MouseTrackingMode::Disabled;
      MouseTrackingEnc enc = MouseTrackingEnc::Default;
      bool focusEventMode = false;
   };

   class Vterm
   {
   public:
      Vterm (uint16_t glyphPx, uint16_t glyphPy,
             uint16_t winPx, uint16_t winPy,
             int ptyFd);

      ~Vterm () = default;

      using RefreshHandlerFn = std::function <void (const Frame&)>;
      void setRefreshHandler (const RefreshHandlerFn&);

      using OscHandlerFn = std::function <void (int, const std::string&)>;
      void setOscHandler (const OscHandlerFn&);

      using BellHandlerFn = std::function <void ()>;
      void setBellHandler (const BellHandlerFn&);

      void resize (uint16_t winPx, uint16_t winPy);

      void redraw ();

      // mapping of a certain VtKey to a sequence of input characters
      struct InputSpec
      {
         VtKey key;
         const char * input;
         size_t length = 0;

         size_t getLength () const
         {
            return length ? length : strlen (input);
         }
      };

      int writePty (VtKey key, VtModifier modifiers = VtModifier::none,
                    bool userInput = false);
      int writePty (uint8_t ch, VtModifier modifiers = VtModifier::none,
                    bool userInput = false);
      int writePty (const char* cstr, bool userInput = false);

      bool readPty ();

      const MouseTrackingState& getMouseTrackingState () const;

      void setHasFocus (bool);
      void mouseWheelUp ();
      void mouseWheelDown ();
      void pageUp ();
      void pageDown ();

      void selectStart (int pX, int pY, bool cycleSnapTo);
      void selectExtend (int pX, int pY, bool cycleSnapTo);
      void selectUpdate (int pX, int pY);
      bool selectFinish (std::string& utf8_selection);
      void selectClear ();
      void selectRectangularModeToggle ();

      void pasteSelection (const std::string& utf8_selection);

   private:
      std::string getLocalEcho (const unsigned char *const begin,
                                const unsigned char *const end);
      void processInput (const unsigned char *const input, int size);
      void processInput (const std::string& str);

      int writePty (const uint8_t* ucstr, size_t len, bool userInput = false);

      // table entry for deciding which set of InputSpecs to use
      struct InputSpecTable
      {
         std::function <bool ()> predicate;
         const InputSpec * specs = nullptr;
         bool visited = false;
      };

      InputSpecTable * getInputSpecTable ();
      void resetInputSpecTable ();
      const InputSpec * selectInputSpecs ();
      const InputSpec & getInputSpec (VtKey key);

      void unhandledInput (unsigned char ch);
      void traceNormalInput ();
      void resetTerminal ();
      void resetAttrs ();
      void resetScreen ();
      void clearScreen ();
      void fillScreen (uint16_t ch);

      enum class InputState: uint8_t
      {
         Normal,
         IgnoreSequence,
         Escape,
         Escape_VT52,
         Esc_SPC,
         Esc_Hash,
         Esc_Pct,
         SelectCharset,
         CSI,
         CSI_priv,
         CSI_Quote,
         CSI_DblQuote,
         CSI_Bang,
         CSI_SPC,
         CSI_GT,
         DCS,
         DCS_Esc,
         OSC,
         OSC_Esc,
         VT52_CUP_Arg1,
         VT52_CUP_Arg2
      };
      const char* strInputState (InputState is)
      {
         static const char* enumerators [] =
         {
         "Normal",
         "IgnoreSequence",
         "Escape",
         "Escape_VT52",
         "Esc_SPC",
         "Esc_Hash",
         "Esc_Pct",
         "SelectCharset",
         "CSI",
         "CSI_priv",
         "CSI_Quote",
         "CSI_DblQuote",
         "CSI_Bang",
         "CSI_SPC",
         "CSI_GT",
         "DCS",
         "DCS_Esc",
         "OSC",
         "OSC_Esc",
         "VT52_CUP_Arg1",
         "VT52_CUP_Arg2"
         };
         return enumerators [(int) is];
      }

      void setState (InputState inputState);

      void normalizeCursorPos ();
      bool isCursorInsideMargins ();
      void eraseRow (uint16_t pY);
      void eraseRows (uint16_t startY, uint16_t count);
      void copyRow (uint16_t dstY, uint16_t srcY);
      void insertRows (uint16_t startY, uint16_t count);
      void deleteRows (uint16_t startY, uint16_t count);
      void insertCols (uint16_t startX, uint16_t count);
      void deleteCols (uint16_t startX, uint16_t count);

      void showCursor ();
      void hideCursor ();
      void inputGraphicChar (unsigned char ch);
      void placeGraphicChar ();
      void jumpToNextTabStop ();
      void setFgFromPalIx ();
      void setBgFromPalIx ();

      // DEC control sequence handlers, prefixed with input state
      void inp_LF ();        // Line Feed
      void inp_CR ();        // Carriage Return
      void inp_HT ();        // Horizontal Tab

      void esc_DCS (unsigned char fin); // Designate Character Set
      bool esc_IND ();       // Index
      void esc_RI ();        // Reverse Index
      void esc_NEL ();       // Next Line
      void esc_BI ();        // Back Index
      void esc_FI ();        // Forward Index
      void esc_HTS ();       // Horizontal Tab Set
      void csi_SCOSC_SLRM (); // disambiguation
      void csi_SCOSC ();     // Save Cursor Position
      void csi_SCORC ();     // Restore Cursor Position
      void esc_DECSC ();     // Save Cursor and Attributes
      void esc_DECRC ();     // Restore Cursor and Attributes
      void esc_RIS ();       // Reset to Initial State
      void csi_DECSTR ();    // DEC Soft Terminal Reset

      void csi_CUU ();       // Cursor Up
      void csi_CUD ();       // Cursor Down
      void csi_CUF ();       // Cursor Forward
      void csi_CUB ();       // Cursor Backward
      void csi_CNL ();       // Cursor Next Line
      void csi_CPL ();       // Cursor Previous Line
      void csi_CHA ();       // Cursor Character Absolute
      void csi_HPA ();       // Character Position Absolute
      void csi_HPR ();       // Character Position Relative
      void csi_VPA ();       // Line Position Absolute
      void csi_VPR ();       // Line Position Relative
      void csi_CUP ();       // Cursor Position a.k.a. HVP
      void csi_SU ();        // Pan Down / Scroll Up
      void csi_SD ();        // Pan Up / Scroll Down
      void csi_CHT ();       // Character Tabulation
      void csi_CBT ();       // Character Backwards Tabulation
      void csi_REP ();       // Repeat last graphic character

      void csi_ED ();        // Erase in Display
      void csi_EL ();        // Erase in Line
      void csi_IL ();        // Insert Line
      void csi_DL ();        // Delete Line
      void csi_ICH ();       // Insert Characters
      void csi_DCH ();       // Delete Characters
      void csi_ECH ();       // Erase Characters
      void csi_DECIC ();     // Insert Column
      void csi_DECDC ();     // Delete Column

      void csi_STBM ();      // Set Top and Bottom Margins
      void csi_SLRM ();      // Set Left and Right Margins
      void csi_TBC ();       // Tabulation Clear

      void csi_SM ();        // Set Mode
      void csi_RM ();        // Reset Mode
      void csi_privSM ();    // Set Mode (private)
      void csi_privRM ();    // Reset Mode (private)
      void csi_SGR ();       // Select Graphic Rendition

      void csi_ecma48_SL (); // Shift Left
      void csi_ecma48_SR (); // Shift Right

      void csi_priDA ();     // Device Attributes (Primary)
      void csi_secDA ();     // Device Attributes (Secondary)
      void csi_DSR ();       // Device State Report
      void esch_DECALN ();   // DEC Alignment Pattern Generator
      void handle_DCS ();    // Device Control String
      void handle_OSC ();    // Operating System Command
      void csiq_DECSCL ();   // DEC Set Compatibility Level
      void csi_XTWINOPS ();  // Xterm window operations
      void csi_XTMODKEYS (); // Xterm key modifier options

      void dcs_DECRQSS (const std::string&); // DEC Request Status String

      void osc_PaletteQuery (int, const std::string&);
      void osc_DynamicColorQuery (int, const std::string&);

      uint16_t winPx;
      uint16_t winPy;
      uint16_t nCols;
      uint16_t nRows;
      uint16_t glyphPx;
      uint16_t glyphPy;
      int ptyFd;

      RefreshHandlerFn onRefresh;
      OscHandlerFn onOsc;
      bool haveOscHandler = false;
      BellHandlerFn onBell;

      // Cell storage, display and input state

      Frame frame_pri;
      Frame frame_alt;
      Frame* cf;              // current frame (primary or alternative)
      uint16_t posX = 0;      // current cursor horizontal position (on-screen)
      uint16_t posY = 0;      // current cursor vertical position (on-screen)
      uint16_t marginTop;     // current margin top (copy of frame field)
      uint16_t marginBottom;  // current margin bottom (copy of frame field)
      bool lastCol = false;

      CharVdev::Cell attrs;   // prototype cell with current attributes
      Color* fg = &attrs.fg;
      Color* bg = &attrs.bg;
      Color palette256 [256];
      Color rgb_fg;
      Color rgb_bg;
      int defaultFgPalIx; // if -1, set from opts.fg, else idx into palette256
      int defaultBgPalIx; // if -1, set from opts.bg, else idx into palette256
      int fgPalIx;
      int bgPalIx;
      bool reverseVideo = false;
      bool hasFocus = false;

      unsigned char inputBuf [32 * 1024];
      int readPos = 0;
      int lastEscBegin = 0;
      int lastNormalBegin = 0;
      int lastStopPos = 0;

      InputState inputState = InputState::Normal;
      constexpr const static size_t maxEscOps = 16;
      uint32_t inputOps [maxEscOps];
      size_t nInputOps = 0;
      Utf8Decoder utf8dec;
      std::vector <unsigned char> argBuf;
      unsigned char scsDst;  // Select charset / destination designator
      unsigned char scsMod;  // Select charset / selector (intermediate)

      VtModifier modifiers = VtModifier::none;

      // Terminal state - N.B.: keep resetTerminal () in sync with this!

      bool showCursorMode = true;
      bool altScreenBufferMode = false;
      bool autoWrapMode = true;
      bool autoNewlineMode = false;
      bool keyboardLocked = false;
      bool insertMode = false;
      bool bkspSendsDel = true;
      bool localEcho = false;
      bool bracketedPasteMode = false;
      bool altScrollMode = false;
      bool altSendsEscape = true;
      uint8_t modifyOtherKeys = 1;

      bool horizMarginMode = false;
      uint16_t nColsEff = 0;
      uint16_t hMargin = 0;

      std::vector <uint16_t> tabStops;

      enum class CompatibilityLevel: uint8_t
      { VT52, VT100, VT400  };
      CompatibilityLevel compatLevel = CompatibilityLevel::VT400;

      enum class CursorKeyMode: uint8_t
      { ANSI, Application };
      CursorKeyMode cursorKeyMode = CursorKeyMode::ANSI;

      enum class KeypadMode: uint8_t
      { Normal, Application };
      KeypadMode keypadMode = KeypadMode::Normal;

      enum class OriginMode: uint8_t
      { Absolute, ScrollingRegion };
      OriginMode originMode = OriginMode::Absolute;

      enum class ColMode: uint8_t
      { C80, C132 };
      ColMode colMode = ColMode::C80;

      void switchColMode (ColMode colMode);
      void switchScreenBufferMode (bool altScreenBufferMode);

      enum class Charset: uint8_t // sync w/charCodes definition!
      { UTF8, DecSpec, DecSuppl, DecUserPref, DecTechn, IsoLatin1, IsoUK };

      struct CharsetState
      {
         Charset g [4] =
         { Charset::UTF8, Charset::UTF8, Charset::UTF8, Charset::UTF8 };

         // Locking shift states (index into g[]):
         uint8_t gl = 0; // G0 in GL
         uint8_t gr = 2; // G2 in GR

         // Single shift state (0 if none active):
         // 0 - not active; 2: G2 in GL; 3: G3 in GL
         uint8_t ss = 0;
      };
      CharsetState charsetState;

      // address with Charset; point to array of 96 unicode points:
      static const uint16_t* charCodes [];

      struct SavedCursor_SCO
      {
         bool isSet = false;
         uint16_t posX = 0;
         uint16_t posY = 0;
         bool lastCol = false;
      };
      struct SavedCursor_DEC: SavedCursor_SCO
      {
         CharVdev::Cell attrs;
         OriginMode originMode = OriginMode::Absolute;
         CharsetState charsetState = CharsetState {};
         // NYI: selective erase mode
      };
      SavedCursor_SCO savedCursor_SCO;
      SavedCursor_DEC savedCursor_DEC_pri;
      SavedCursor_DEC savedCursor_DEC_alt;
      SavedCursor_DEC* savedCursor_DEC = &savedCursor_DEC_pri;

      bool selectUpdatesTop = false;
      bool selectUpdatesLeft = false;

      MouseTrackingState mouseTrk;

      #ifdef DEBUG
      void traceFunction (const char* func);
      // step debugger facilities
      int debugStep = 0;
      int debugCnt = 0;
      void debugKey ();
      void debugBreak ();
      #endif // DEBUG
   };

} // namespace zutty

#include "vterm.icc"
