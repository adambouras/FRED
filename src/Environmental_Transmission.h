/*
  This file is part of the FRED system.

  Copyright (c) 2013-2016, University of Pittsburgh, John Grefenstette,
  David Galloway, Mary Krauland, Michael Lann, and Donald Burke.

  Based in part on FRED Version 2.9, created in 2010-2013 by
  John Grefenstette, Shawn Brown, Roni Rosenfield, Alona Fyshe, David
  Galloway, Nathan Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

//
//
// File: Environmental_Transmission.h
//

#ifndef _FRED_ENVIRONMENTAL_TRANSMISSION_H
#define _FRED_ENVIRONMENTAL_TRANSMISSION_H

#include "Transmission.h"

class Condition;

class Environmental_Transmission: public Transmission {

public:
  Environmental_Transmission() {}
  ~Environmental_Transmission() {}
  void setup(Condition* condition);
  void spread_infection(int day, int condition_id, Mixing_Group* mixing_group) {}

private:
};

#endif // _FRED_ENVIRONMENTAL_TRANSMISSION_H