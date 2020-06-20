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

      void resize (uint16_t winPx, uint16_t winPy);

      void copyCells (CharVdev::Cell * const dest);

      inline void readPty ();

      uint64_t seqNo = 0; // update counter
      bool exit = false;

      uint16_t winPx;
      uint16_t winPy;
      uint16_t nCols;
      uint16_t nRows;

      enum class InputState
      {
         Normal,
         Escape,
         CSI,
         CSI_priv
      };

   private:
      inline CharVdev::Cell& cell (uint32_t idx);
      inline CharVdev::Cell& cell (uint16_t row, uint16_t col);

      inline uint32_t setCur ();
      inline uint32_t startOfThisLine ();
      inline uint32_t startOfNextLine ();
      inline void eraseRange (uint32_t start, uint32_t end);
      inline void moveLines (uint16_t startY, uint16_t countY, int offset);

      inline void lineFeed ();
      inline void carriageReturn ();
      inline void advancePosition ();
      inline void showCursor ();
      inline void hideCursor ();

      // DEC control sequence handlers
      inline void esc_IND ();       // Index
      inline void esc_RI ();        // Reverse Index
      inline void esc_NEL ();       // Next Line
      inline void esc_DECSC ();     // Save Cursor
      inline void esc_DECRC ();     // Restore Cursor

      inline void csi_CUU ();       // Cursor Up
      inline void csi_CUD ();       // Cursor Down
      inline void csi_CUF ();       // Cursor Forward
      inline void csi_CUB ();       // Cursor Backward
      inline void csi_CUP ();       // Cursor Position a.k.a. HVP

      inline void csi_ED ();        // Erase in Display
      inline void csi_EL ();        // Erase in Line
      inline void csi_IL ();        // Insert Line
      inline void csi_DL ();        // Delete Line
      inline void csi_ICH ();       // Insert Characters
      inline void csi_DCH ();       // Delete Characters

      inline void csi_SM ();        // Set Mode
      inline void csi_RM ();        // Reset Mode
      inline void csi_SGR ();       // Select Graphic Rendition
      inline void csi_STBM ();      // Set Top and Bottom Margins

      inline void csipriv_SM ();    // Set Mode (private)
      inline void csipriv_RM ();    // Reset Mode (private)

      uint16_t glyphPx;
      uint16_t glyphPy;
      int ptyFd;

      std::shared_ptr <CharVdev::Cell> cells;
      uint32_t cur;
      uint32_t top;
      uint16_t posX = 0;
      uint16_t posY = 0;
      CharVdev::Cell attrs; // prototype cell with current attributes
      CharVdev::Color defaultFg = {255, 255, 255};
      CharVdev::Color defaultBg = {0, 0, 0};

      unsigned char inputBuf [1024];
      InputState inputState = InputState::Normal;
      constexpr const static size_t maxEscOps = 16;
      uint32_t inputOps [maxEscOps];
      size_t nInputOps;
   };

} // namespace zutty

#include "vterm.icc"
