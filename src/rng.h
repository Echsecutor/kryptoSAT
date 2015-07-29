/*****************************************************************************
 *
 * @file rng.h
 *
 * @section DESCRIPTION
 *
 * This is a simple interface and proof of concept implementation for
 * the random number generator used in kryptoSAT. Notice that the
 * actual implementation is crytically important.
 *
 * @author  Sebastian Schmittner <sebastian@schmittner.pw>
 *
 * @version 1.0.2015-04-09
 *
 * @section Version number format
 *
 * The Version number is formatted as "M.S.D" where M is the major
 * release branch (backward compatibility to all non-alpha releases of
 * the same branch is guaranteed), S is the state of this release (0
 * for alpha, 1 for beta, 2 for stable), and D is the date formatted
 * as yyyy-mm-dd.)
 *
 *
 * @copyright 2015 Sebastian Schmittner
 *
 * @section LICENSE
 *
 * This file is part of KryptoSAT.
 *
 * KryptoSAT is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * KryptoSAT is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with KryptoSAT.  If not, see <http://www.gnu.org/licenses/>.
 *
 *****************************************************************************/

#ifndef RNG_H
#define RNG_H

#include <ctime>

#include <cstring>//for size_t
#include <random>
using namespace std;


/// interface
class rng{
 public:

  virtual ~rng(){};


  /// Randomise the PRNG using all availlable entropy. This does not
  /// need to be reproducible. Used for kee generation.
  virtual void randomise(size_t entropy)=0;

  /// Seed for reproducible pseudo random sequences. Used for
  /// encryption (and honest encryption checking).
  virtual void seed(size_t seed)=0;

  virtual size_t getGoodSeed()=0;


  /// wrapper for the random number generator to extract an evenly
  /// distributed random 0/1
  virtual bool randomBool()=0;

  /// wrapper for the random number generator to extract an evenly
  /// distributed integer in [0,max)
  virtual size_t randomInt(size_t max)=0;

};


class mersenneTwisterRNG : public virtual rng{

 private:
  random_device rd;
  mt19937 engine;

 public:

    ~mersenneTwisterRNG(){}

  size_t getGoodSeed(){
    return rd();
  }

  void randomise(size_t entropy){
    cout << "randomised"<<endl;
    engine.seed(rd() ^ entropy);
  }

  void seed(size_t seed){
    cout << "seeded"<<endl;

    engine.seed(seed);
  }

  mersenneTwisterRNG(){}

  size_t randomInt(size_t max){
    uniform_int_distribution<size_t> uniform_dist(0, max-1);
    return uniform_dist(engine);
  }

  bool randomBool(){
    bool re=(generate_canonical<double, 1>(engine) <0.5);
    //    cout << "randomBool " << re <<endl;
    return re;
  }

};

#endif
