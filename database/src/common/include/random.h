/***********************************************************************
 * ebase library - an embedded C++ database library                    *
 * Copyright (C) 2001, Jun Sun, jsun@mvista.com or jsun@junsun.net.    *
 *                                                                     *
 * This library is free software; you can redistribute it and/or       *
 * modify it under the terms of the GNU Lesser General Public          *
 * License as published by the Free Software Foundation; either        *
 * version 2.1 of the License, or (at your option) any later version.  *
 ***********************************************************************
 */

#ifndef _RANDOM_H_
#define _RANDOM_H_

#include "../../internal.h"

// temp typedef.  Will be removed once float64 is installed
typedef  	double float64;

/*------------------------------------------------------------------------
CLASS 
    Random number generator

    This implementation is a linear congruential generator (LCG) for
    32-bit machine.

------------------------------------------------------------------------*/
class Random
{
 public:

  //////////
  // Constructor.  The seed is set by the current time.
  //
  Random();

  //////////
  // Constructor with a seed
  // [in] seed  The seed for generator.
  //
  Random(EbUint32 seed);

  //////////
  // Set the seed for the generator.
  // [in] seed  The seed for generator.
  //
  void SetSeed(EbUint32);

  //////////
  // Generate a double which is between 0 and 1.
  // Return_ A sample value which is a double and is between 0 and 1
  //
  float64 Sample();

  //////////
  // Generate a random number which is between a and b (inclusive).
  // [in] a  The lower bound of the range (inclusive).
  // [in] b  The upper bound of the range (inclusive).
  //
  // Return_ A sample value which is a double and is between 0 and 1
  //
  EbInt32 Sample(EbInt32 a, EbInt32 b);

 private:
    EbUint32 _seed;
};



static const EbUint32 A   = (EbUint32)314159269L;
static const EbUint32 C 	= (EbUint32)453806245L;
static const EbUint32 M 	= (EbUint32)2147483648L;

inline Random::Random() 
{
    // _seed = Timer::GetSystemTime();
    _seed = 11111;
}

inline Random::Random(EbUint32 n)
{
    _seed = n;
}

inline void
Random::SetSeed(EbUint32 n)
{
    _seed = n;
}

inline float64 
Random::Sample()
{
    _seed = (_seed * A + C) % M;
    if (_seed==0) _seed = C;
    return (float64)_seed / (float64)M;
}

inline EbInt32
Random::Sample(EbInt32 a, EbInt32 b) 
{
    _seed = (_seed * A + C) % M;
    if (_seed==0) _seed = C;
    return a + _seed % (b-a+1);
}

#endif /* _RANDOM_H_ */
