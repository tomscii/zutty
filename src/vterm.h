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

#include "charvdev.h"

#include <cstdint>
#include <functional>
#include <memory>

namespace zutty {

   class Vterm
   {
   public:
      Vterm () = default;

      Vterm (uint16_t glyphPx, uint16_t glyphPy,
             uint16_t winPx, uint16_t winPy,
             int ptyFd);

      ~Vterm () = default;

      void setRefreshHandler (
         const std::function <void (const Vterm&)>& refreshHandler);

      void resize (uint16_t winPx, uint16_t winPy);

      void copyCells (CharVdev::Cell * const dest);

      enum class VtKey
      {
         NONE,
         Insert, Delete, Home, End, Up, Down, Left, Right,
         PageUp, PageDown,
         F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12
      };

      inline int writePty (uint8_t ch);
      inline int writePty (const char* cstr);
      int writePty (VtKey key);

      inline void readPty ();

      uint64_t seqNo = 0; // update counter
      bool exit = false;

      uint16_t winPx;
      uint16_t winPy;
      uint16_t nCols;
      uint16_t nRows;

   private:
      CharVdev::Cell& cell (uint32_t idx);
      CharVdev::Cell& cell (uint16_t row, uint16_t col);

      void debugStop ();
      void unhandledInput (char ch);
      void traceNormalInput ();
      void resetTerminal ();
      void resetAttrs ();
      void fillScreen (uint16_t ch);

      enum class InputState
      {
         Normal,
         Escape,
         EscapeHash,
         SelectCharset,
         CSI,
         CSI_priv
      };

      void setState (InputState inputState);

      uint32_t setCur ();
      uint32_t startOfThisLine ();
      uint32_t startOfNextLine ();
      void eraseRange (uint32_t start, uint32_t end);
      void moveLines (uint16_t startY, uint16_t countY, int offset);

      void normalizePosition ();
      void advancePosition ();
      void showCursor ();
      void hideCursor ();
      void insertChar (uint16_t unicode_pt);

      // DEC control sequence handlers, prefixed with input state
      void inp_LF ();        // Line Feed
      void inp_CR ();        // Carriage Return
      void inp_HT ();        // Horizontal Tab

      void esc_IND ();       // Index
      void esc_RI ();        // Reverse Index
      void esc_NEL ();       // Next Line
      void esc_HTS ();       // Horizontal Tab Set
      void esc_DECSC ();     // Save Cursor
      void esc_DECRC ();     // Restore Cursor

      void csi_CUU ();       // Cursor Up
      void csi_CUD ();       // Cursor Down
      void csi_CUF ();       // Cursor Forward
      void csi_CUB ();       // Cursor Backward
      void csi_CNL ();       // Cursor Next Line
      void csi_CPL ();       // Cursor Previous Line
      void csi_CHA ();       // Cursor Character Absolute
      void csi_VPA ();       // Line Position Absolute
      void csi_CUP ();       // Cursor Position a.k.a. HVP
      void csi_SU ();        // Pan Down / Scroll Up
      void csi_SD ();        // Pan Up / Scroll Down
      void csi_CHT ();       // Character Tabulation

      void csi_ED ();        // Erase in Display
      void csi_EL ();        // Erase in Line
      void csi_IL ();        // Insert Line
      void csi_DL ();        // Delete Line
      void csi_ICH ();       // Insert Characters
      void csi_DCH ();       // Delete Characters
      void csi_ECH ();       // Erase Characters

      void csi_STBM ();      // Set Top and Bottom Margins
      void csi_TBC ();       // Tabulation Clear

      void csi_SM ();        // Set Mode
      void csi_RM ();        // Reset Mode
      void csipriv_SM ();    // Set Mode (private)
      void csipriv_RM ();    // Reset Mode (private)
      void csi_SGR ();       // Select Graphic Rendition

      void csi_priDA ();
      void esch_DECALN ();

      uint16_t glyphPx;
      uint16_t glyphPy;
      int ptyFd;

      std::function <void (const Vterm&)> refreshVideo;

      std::shared_ptr <CharVdev::Cell> cells;
      uint32_t cur;
      uint32_t top;
      uint16_t posX = 0;
      uint16_t posY = 0;
      CharVdev::Cell attrs; // prototype cell with current attributes
      CharVdev::Color* fg = &attrs.fg;
      CharVdev::Color* bg = &attrs.bg;
      CharVdev::Color defaultFg = {255, 255, 255};
      CharVdev::Color defaultBg = {0, 0, 0};
      CharVdev::Color palette256 [256];

      enum class CursorKeyMode
      {
         ANSI, Application
      };
      CursorKeyMode cursorKeyMode = CursorKeyMode::ANSI;
      bool showCursorMode = true;
      bool altScreenBufferMode = false;
      bool autoWrapMode = true;

      unsigned char inputBuf [4096];
      int readPos = 0;
      int lastEscBegin = 0;
      int lastNormalBegin = 0;
      int lastStopPos = 0;

      InputState inputState = InputState::Normal;
      constexpr const static size_t maxEscOps = 16;
      uint32_t inputOps [maxEscOps];
      size_t nInputOps = 0;
      uint16_t unicode_cp = 0;
      uint8_t utf8_rem = 0;
      std::vector <uint16_t> tabStops;
   };

} // namespace zutty

#include "vterm.icc"
