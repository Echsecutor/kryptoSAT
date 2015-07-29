/*****************************************************************************
 *
 * @file kryptoSAT.h
 *
 * @section DESCRIPTION
 *
 * This is the implementation of the key generation, encryption and
 * decryption algorithms described in my kryptoSAT paper.
 *
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


#ifndef KRYPTOSAT_H
#define KRYPTOSAT_H

#include <cstring> //for size_t


#include "functionParser.h"
#include "booleanFct.h"
#include "rng.h"

#include "encrypt.h"


namespace kryptoSAT{

  bool* generatePrivateKey(rng* r, const size_t& length){
    bool * re = new bool[length];
    for(size_t i=0;i<length;i++){
      re[i]=r->randomBool();
    }
    return re;
  }

  booleanFct<BFT_AND>* generatePublicKey(rng* r, const bool* privateKey, const size_t& privateKeyLength, const size_t& nbrClauses, const unsigned int& varsPerClause){

    booleanFct<BFT_AND>* re = new booleanFct<BFT_AND>(privateKeyLength);

    while(re->size()<nbrClauses){
      size_t* vars= new size_t[varsPerClause];
      // generate random clause
      for(size_t j=0;j<varsPerClause;j++){
        bool varAccepted=false;
        size_t var;
        while(!varAccepted){
          var = r->randomInt(privateKeyLength);
          varAccepted=true;
          // check for doubles
          for(size_t k=0;k<j && varAccepted;k++){
            if(vars[k]==var){varAccepted=false;continue;}
          }
        }
        vars[j]=var;
      }//next variable

      bool clauseAccepted=false;
      while(!clauseAccepted){
        booleanFct<BFT_OR> *c=new  booleanFct<BFT_OR>(privateKeyLength);

        //generate data structure along with "signs"
        for(size_t j=0;j<varsPerClause;j++){
          if(r->randomBool()){
            c->push_back(new booleanFct<BFT_INPUT>(privateKeyLength,vars[j]));
          }else{
            c->push_back(new booleanFct<BFT_NOT>(privateKeyLength,new booleanFct<BFT_INPUT>(privateKeyLength,vars[j])));
          }
        }
        //planting, i.e. check if privateKey fullfills the clause, if not reroll signs:
        if(!c->evaluate(privateKey)){
          clauseAccepted=false;
          //cleanup
          c->disposeChildren();
          delete c;
          c=0;
        }else{
          clauseAccepted=true;

          //check for doubles
          bool found=false;
          for(list<BF*>::iterator i=re->begin();i!=re->end() && !found;i++){
            if(compare(*i,c)==0){
              found=true;
              continue;
            }
          }
          if(!found){
            re->push_back(c); //actually accept clause
          }else{
            //cleanup
            c->disposeChildren();
            delete c;
            c=0;
          }
        }
      }//wend clause accept

      delete[] vars;
    }//next clause

    return re;
  }


  /// the default is to use 3-SAT with with m = 2^3 *n clauses, where n is
  /// the number of variables, i.e. privateKeyLength
  booleanFct<BFT_AND>* generatePublicKey(rng* r, const bool* privateKey, const size_t& privateKeyLength){
    return generatePublicKey(r, privateKey, privateKeyLength, 8*privateKeyLength,3);
  }


  /// By default chooses alpha=m and beta=3
  booleanFct<BFT_XOR>* encrypt(rng * r,const size_t& privateKeyLength,const booleanFct<BFT_AND>* publicKey, const bool& input){
    return encrypt(r,privateKeyLength, publicKey, input, 3);
  }

  //actual encoding in seperate header



}//end namespace


#endif
