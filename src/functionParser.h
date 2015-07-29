/*****************************************************************************
 *
 * @file functioParser.h
 *
 * @section DESCRIPTION
 *
 * This collection of functions reads and writes plain text files in a
 * "simplified DIMACS CNF" format, as described in e.g.
 * http://www.dwheeler.com/essays/minisat-user-guide.html and in a
 * similar format for ANF.
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

#ifndef FUNCTIONPARSER_H
#define FUNCTIONPARSER_H

#include <iostream>
#include <fstream>
#include <string>
#include <cstring>

using namespace std;


#include "booleanFct.h"

namespace functionParser{


  bool * readBool(const char * file, size_t& nbrVars){

    ifstream inFile(file);
    string line;
    if (!inFile.is_open()){
      cerr<< "ERR: could not open file" << file << " for reading." <<endl;
      return 0;
    }
    bool* re=0;
    while ( inFile.good() ){
      getline(inFile,line);
      //ignore empty/comment lines
      if( line.size()>0 && line.at(0)!='c' && line.at(0)!='#' ){
        if (re!=0){
          cerr<<"Unrecognised file format!"<<endl;
          return 0;
        }

	istringstream iss(line);
	string token;
	getline(iss, token, ' ');


        if(token[0]=='0' || token[0]=='1'){
          //simplest file format:
          if(line.size()==1 || token[1]=='0' || token[1]=='1'){
            //no spaces:
            nbrVars=line.size();
            re = new bool[nbrVars];
            for(unsigned int i=0;i<line.size();i++){
              if(token[i]=='1'){
                re[i]=true;
              }else{
                re[i]=false;
              }
            }//next
          }else{
            //with spaces/commas/...
            nbrVars=line.size()/2;
            re = new bool[nbrVars];
            for(unsigned int i=0;2*i<line.size();i++){
              if(token[2*i]=='1'){
                re[i]=true;
              }else{
                re[i]=false;
              }
            }//next
          }
        }//end if simple format

        //todo: advanced format ;)

      }//end if comment
    }//wend
    inFile.close();
    return re;
  }



  bool writeBool(ostream& outFile, const bool*const B, const size_t& nbrVars){
    outFile << "c This was written by the functionParser of KryptoSAT."<<endl;
    outFile << "c It contains a bool array in the simplest human readable format."<<endl<<"c"<<endl;
    for(size_t i=0;i<nbrVars;i++){
      outFile << B[i];
    }
    outFile<<endl;
    return true;
  }


  bool writeBool(const char * file, const bool* const B, const size_t& nbrVars){
    ofstream outFile(file);

    if (!outFile.good()){
      cerr<< "ERR: could not open file " << file << " for writing." <<endl;
      return false;
    }
    bool re = writeBool(outFile, B, nbrVars);
    outFile.close();
    return re;
  }






  booleanFct<BFT_AND>* readCNF(istream& cnfFile){

    size_t nbrVars=0;
    size_t nbrClauses=0;

    size_t actualNbrClauses=0;

    booleanFct<BFT_AND>* re=0;
    string line;

    while ( cnfFile.good() ){
      getline(cnfFile,line);
      //ignore empty/comment lines
      if( line.size()>0 && line.at(0)!='c' && line.at(0)!='#' ){
	istringstream iss(line);
	string token;

        /// first non comment lines specifies nbr of variables and clauses in the format
        /// "p cnf nbrVars nbrClauses"
        if(nbrVars==0){
	  getline(iss, token, ' ');
          if (token!="p"){
            cerr<< "ERR: unrecognized file format. 'p' missing." <<endl;
            return 0;
          }
	  getline(iss, token, ' ');
          if (token!="cnf"){
            cerr<< "ERR: unrecognized file format. 'cnf' missing." <<endl;
            return 0;
          }
	  getline(iss, token, ' ');
          nbrVars = stol(token);
	  getline(iss, token, ' ');
          nbrClauses = stol(token);

          re = new booleanFct<BFT_AND>(nbrVars);

          /// all following lines define clauses in the format
          /// [-]varNbr ... [-]varNbr 0
          /// where - indocates NOT
        }else{
          bool finishedClause=false;
          booleanFct<BFT_OR>* bigOr = new booleanFct<BFT_OR>(nbrVars);
          while(getline(iss, token, ' ')){
            if(finishedClause){
              cerr<< "ERR: unrecognized file format. '0' has to indicate end of line." <<endl;
              return 0;
            }
            long VN = stol(token);
            if (VN<0){
              VN = -VN;
              bigOr->push_back(new booleanFct<BFT_NOT>(nbrVars,new booleanFct<BFT_INPUT>(nbrVars,VN-1)));
            }else if (VN>0){
              bigOr->push_back(new booleanFct<BFT_INPUT>(nbrVars,VN-1));
            }else{
              finishedClause=true;
            }
          }
          if(!finishedClause){
            cerr<< "ERR: unrecognized file format. end of clause has to be indicated with '0'." <<endl;
            return 0;
          }

          re->push_back(bigOr);
          actualNbrClauses++;
        }
      }
    }

    if(actualNbrClauses != nbrClauses){
      cerr<< "ERR: unrecognized file format. Specified number of clauses does not match given number of clauses." <<endl;
      return 0;
    }

    return re;
  }//end readCNF


  booleanFct<BFT_AND>* readCNF(const char * file){

    ifstream cnfFile(file);
    if (!cnfFile.is_open()){
      cerr<< "ERR: could not open file" << file << " for reading." <<endl;
      return 0;
    }

    booleanFct<BFT_AND>* re = readCNF(cnfFile);

    cnfFile.close();
    return re;
  }



  booleanFct<BFT_XOR>* readANF(istream& anfFile){

    size_t nbrVars=0;
    size_t nbrClauses=0;

    size_t actualNbrClauses=0;

    booleanFct<BFT_XOR>* re=0;
    string line;

    while ( anfFile.good() ){
      getline(anfFile,line);
      //ignore empty/comment lines
      if( line.size()>0 && line.at(0)!='c' && line.at(0)!='#' ){
	istringstream iss(line);
	string token;

        /// first non comment lines specifies nbr of variables and clauses in the format
        /// "p cnf nbrVars nbrClauses"
        if(nbrVars==0){
	  getline(iss, token, ' ');
          if (token != "p"){
            cerr<< "ERR: unrecognized file format. 'p' missing." <<endl;
            return 0;
          }
	  getline(iss, token, ' ');
          if (token!="anf"){
            cerr<< "ERR: unrecognized file format. 'anf' missing." <<endl;
            return 0;
          }
	  getline(iss, token, ' ');
          nbrVars = stol(token);
	  getline(iss, token, ' ');
          nbrClauses = stol(token);

	  cout << "Reading " << nbrClauses << " clauses with " << nbrVars << " variables"<<endl;

          re = new booleanFct<BFT_XOR>(nbrVars);

          /// all following lines define clauses in the format
          /// [-]varNbr ... [-]varNbr 0
          /// where - indocates NOT
        }else{
          bool finishedClause=false;
          booleanFct<BFT_AND>* bigAnd = new booleanFct<BFT_AND>(nbrVars);

          while(getline(iss, token, ' ')){
            if(finishedClause && token!="0"){
              cerr<< "ERR: unrecognized file format. '0' has to indicate end of line. (Multiple '0's allowed.)" <<endl;
              return 0;
            }
            long VN = stol(token);
            if (VN<0){
              cerr<< "ERR: unrecognized file format. ANF must not contain negations" <<endl;
              return 0;
            }else if (VN>0){
              bigAnd->push_back(new booleanFct<BFT_INPUT>(nbrVars,VN-1));
            }else{
              finishedClause=true;
            }
          }
          if(!finishedClause){
            cerr<< "ERR: unrecognized file format. End of summand has to be indicated with '0'." <<endl;
            return 0;
          }

          //make sure that each and has at least one child
          if (bigAnd->size()<1){
            bigAnd->push_back(new booleanFct<BFT_TRUE>(nbrVars));
          }

          re->push_back(bigAnd);
	  // cout << "found clause: " << re->toString()<<endl;
          actualNbrClauses++;
        }
      }
    }

    if(actualNbrClauses != nbrClauses){
      cerr<< "ERR: unrecognized file format. Specified number of summands does not match given number of summands." <<endl;
      return 0;
    }

    if(re==0){
      cerr << "ERR: No ANF specification found in file!"<<endl;
    }

    if(re->size()==0){
      cerr << "ERR: No clauses found in file!"<<endl;
    }
    
    return re;
  }//end readANF


  booleanFct<BFT_XOR>* readANF(const char * file){
    ifstream anfFile(file);

    if (!anfFile.is_open()){
      cerr<< "ERR: could not open file" << file << " for reading." <<endl;
      return 0;
    }
    booleanFct<BFT_XOR>* re = readANF(anfFile);
    anfFile.close();
    return re;
  }






  /// return indicates success
  bool writeCNF(ostream& cnfFile, const booleanFct<BFT_AND>*const cnf){

    cnfFile << "c This cnf file was written by the cnfparser of KryptoSAT."<<endl;
    cnfFile << "c The format is specified in e.g. http://www.dwheeler.com/essays/minisat-user-guide.html"<<endl<<"c"<<endl;

    cnfFile << "p cnf " << cnf->getNumberOfVars() << " " << cnf->size()<<endl;

    for(list<BF*>::const_iterator it = cnf->begin();it != cnf->end();it++){
      if((*it)->myType() != BFT_OR){
        cerr << "ERR: Function is not in CNF!"<<endl;
        return false;
      }

      for(list<BF*>::const_iterator j = (*it)->begin();j != (*it)->end();j++){
        cnfFile << (*j)->getDependence() << " ";
      }
      cnfFile << "0"<<endl;
    }

    return true;

  }//end writeCNF

  bool writeCNF(const char * file, const booleanFct<BFT_AND>*const cnf){
    ofstream cnfFile(file);

    if (!cnfFile.good()){
      cerr<< "ERR: could not open file " << file << " for writing." <<endl;
      return false;
    }
    bool re = writeCNF(cnfFile, cnf);
    cnfFile.close();
    return re;
  }


  /// return indicates success
  bool writeANF(ostream& anfFile, const booleanFct<BFT_XOR>* const anf){

    //    anfFile<< "c This anf file was written by the function parser of KryptoSAT."<<endl;
    anfFile << "c The format of the next line is 'p anf numberOfVariables NumberOfSummands'"<<endl;
    anfFile << "p anf " << anf->getNumberOfVars() << " " << anf->size()<<endl;

    anfFile<<"c The following lines specify the summands, one per line."<<endl;
    anfFile<<"c Each summand is a conjunction of variables (without negation)."<<endl;
    anfFile << "c These are given as a space seperated list of their indices terminated by '0'."<<endl;
    anfFile << "c A double  '0 0' indicates the constant summand '1'."<<endl;


    for(list<BF*>::const_iterator i = anf->begin();i != anf->end();i++){
      if((*i)->myType() != BFT_AND){
        cerr << "ERR: Function is not in ANF!"<<endl;
        return false;
      }

      for(list<BF*>::const_iterator j = (*i)->begin();j != (*i)->end();j++){
        anfFile << (*j)->getDependence() << " ";
      }
      anfFile << "0"<<endl;
      anfFile.flush();//preventing strange memory greedy buffer behaviour...?
    }

    return true;

  }//end writeANF

  bool writeANF(const char * file, const booleanFct<BFT_XOR>* const anf){
    ofstream anfFile(file);

    if (!anfFile.good()){
      cerr<< "ERR: could not open file " << file << " for writing." <<endl;
      return false;
    }

    bool re =   writeANF(anfFile,anf);
    anfFile.close();
    return re;
  }





}//end namespace


#endif
