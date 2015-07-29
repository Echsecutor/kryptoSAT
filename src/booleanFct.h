/*****************************************************************************
 *
 * @file booleanFct.h
 *
 * @section DESCRIPTION
 *
 * Interface and implementations of boolean function representations.
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



#ifndef BOOLEANFCT_H
#define BOOLEANFCT_H

#include <list>



enum booleanFctTypes{
  //0 Arguments:
  BFT_TRUE,
  BFT_FALSE,
  BFT_INPUT,
  //1 Argument:
  BFT_NOT,
  //Arbitrary number of arguments (associative)
  BFT_AND,
  BFT_OR,
  BFT_XOR
};



//forwards:

class BF;

template <booleanFctTypes TYPE>
class booleanFct;

bool boolCompare(const BF* const x,const BF* const y);



/// interface + transparent list specialisation
class BF : public virtual list<BF*>{

 public:

  /// destructor deletes all data and pointers
  virtual ~BF(){disposeChildren();}

  /// to recursively delete and clear children
  void disposeChildren(){
    for(list<BF*>::iterator it = begin(); it != end(); it++){
      delete (*it);//calls destructor and hence recursively dispose
      (*it)=0;
    }
    clear();
  }

  /// recursive sorting
  void recursiveSort(){
    for(list<BF*>::iterator it = begin(); it != end(); it++){
      (*it)->recursiveSort();
    }
    sort(boolCompare);
  }

  /// virtual copy constructor idiom
  virtual BF* clone()const = 0;

  /// return the booleanFctType of the specialisation
  virtual booleanFctTypes myType() const=0;

  /// evaluate all children and combine outputs according to function ;)
  virtual bool evaluate(const bool *const input) const=0;

  /// Return the id (>=1) of one of the variables on which the fundtion
  /// depends. If it depends on NOT the variable returns -id. Most
  /// useful if the function dependce on only one variable.
  virtual long getDependence() const=0;

  virtual size_t getNumberOfVars() const=0;

  virtual string toString() const=0;

};





/// implementation template for BF
template <booleanFctTypes TYPE>
class booleanFct : public virtual BF{

 protected:
  size_t nbrOfVars;

 public:
  booleanFctTypes myType() const{return TYPE;}

  booleanFct<TYPE>(size_t _nbrOfVars):nbrOfVars(_nbrOfVars){}
  booleanFct<TYPE>(size_t _nbrOfVars, BF* child):nbrOfVars(_nbrOfVars){push_back(child);};

  booleanFct<TYPE>* clone()const{
    return new booleanFct<TYPE>(*this);
  }

  booleanFct<TYPE>(const booleanFct<TYPE>& f):nbrOfVars(f.getNumberOfVars()){
    for(list<BF*>::const_iterator it = f.begin(); it != f.end(); it++){
      push_back((*it)->clone());
    }
  }

  bool evaluate(const bool *const input) const;
  size_t getNumberOfVars() const{return nbrOfVars;}
  long getDependence() const{return 0;}

  string toString() const;

};

/// full specialisation: return specified variable from input.
template<>
class booleanFct<BFT_INPUT>: public virtual BF{

 protected:
  size_t nbrOfVars;
  size_t inputVar;

 public:
  booleanFctTypes myType() const{return BFT_INPUT;}

  booleanFct<BFT_INPUT>(size_t _nbrOfVars, size_t _inputVar):nbrOfVars(_nbrOfVars),inputVar(_inputVar){}

  booleanFct<BFT_INPUT>* clone()const{
    return new booleanFct<BFT_INPUT>(*this);
  }

  booleanFct<BFT_INPUT>(const booleanFct<BFT_INPUT>& f):nbrOfVars(f.nbrOfVars),inputVar(f.inputVar){}


  /// Caution: non existing variables evaluate to false after writing to CERR
  bool evaluate(const bool *const input) const{
    if(inputVar>=nbrOfVars||inputVar<0){
      cerr<< "ERR: variable " << inputVar << " does not exist!" <<endl;
      return false;
    }
    return input[inputVar];
  }

  bool addArgument(BF* child){
    return false;
  }

  size_t getNumberOfVars() const{return nbrOfVars;}

  //variable numbers have to start with 1
  long getDependence() const{return inputVar + 1;}

  string toString() const{
    stringstream re;
    re << "X" << getDependence();
    return re.str();
  }
};




// all other types are partial specialisations of the generic one

// constants (ignoring children):
template<>
bool booleanFct<BFT_TRUE>::evaluate(const bool*const input) const{
  return true;
}

template<>
string booleanFct<BFT_TRUE>::toString() const{
  string re="1";
  return re;
}


template<>
bool booleanFct<BFT_FALSE>::evaluate(const bool*const input) const{
  return false;
}

template<>
string booleanFct<BFT_FALSE>::toString() const{
  string re="0";
  return re;
}

/// negate single argument. If no argument was given: error msg + returns false.
/// ignores all but the first child
template<>
bool booleanFct<BFT_NOT>::evaluate(const bool*const input) const{
  if (size()<1){
    cerr << "WARN: NOT without argument!"<<endl;
    return false;
  }
  return ! front()->evaluate(input);
}

/// dependence recives a minus sign
template<>
long booleanFct<BFT_NOT>::getDependence() const{
  if (size()<1){
    return 0;
  }
  return - front()->getDependence();
}

template<>
string booleanFct<BFT_NOT>::toString() const{
  string re="!";
  re+=front()->toString();
  return re;
}


/// conjunction of all arguments. If no argument was given returns true.
template<>
bool booleanFct<BFT_AND>::evaluate(const bool*const input) const{
  bool re = true;
  for(list<BF*>::const_iterator it = begin();it!=end();it++){
    re = re && (*it)->evaluate(input);
  }
  return re;
}

template<>
long booleanFct<BFT_AND>::getDependence() const{
  if (size()<1){
    return 0;
  }
  return front()->getDependence();
}

template<>
string booleanFct<BFT_AND>::toString() const{
  string re="(";
  for(list<BF*>::const_iterator it = begin();it!=end();it++){
    re+=(*it)->toString();
    if(next(it)!=end()){
      re+=" AND ";
    }
  }
  re+=")";
  return re;
}



/// Disjunction of all arguments. If no argument was given returns false.
template<>
bool booleanFct<BFT_OR>::evaluate(const bool*const input) const{
  bool re = false;
  for(list<BF*>::const_iterator it = begin();it!=end();it++){
    re = re || (*it)->evaluate(input);
  }
  return re;
}

template<>
long booleanFct<BFT_OR>::getDependence() const{
  if (size()<1){
    return 0;
  }
  return front()->getDependence();
}

template<>
string booleanFct<BFT_OR>::toString() const{
  string re="(";
  for(list<BF*>::const_iterator it = begin();it!=end();it++){
    re+=(*it)->toString();
    if(next(it)!=end()){
      re+=" OR ";
    }
  }
  re+=")";
  return re;
}


/// sum (XOR) of all arguments. If no argument was given returns false.
/// notice: a XOR b <=> a != b
template<>
bool booleanFct<BFT_XOR>::evaluate(const bool*const input) const{
  bool re = false;
  for(list<BF*>::const_iterator it = begin();it!=end();it++){
    re = re != (*it)->evaluate(input);
  }
  return re;
}

template<>
long booleanFct<BFT_XOR>::getDependence() const{
  if (size()<1){
    return 0;
  }
  return front()->getDependence();
}

template<>
string booleanFct<BFT_XOR>::toString() const{
  string re="(";
  for(list<BF*>::const_iterator it = begin();it!=end();it++){
    re+=(*it)->toString();
    if(next(it)!=end()){
      re+=" XOR ";
    }
  }
  re+=")";
  return re;
}



/// for ordering the terms in an ANF, introduce a <= relation on
/// conjunctions of variables. Can be used to compare any two BFs.
/// Notice that this compares the representation (form) NOT the
/// abstract boolean function (of course ;)
/// In particular, no sorting is performed!
int compare(const BF* const x,const BF* const y){
  if(x==0){
    cerr << "ERR: null pointer comparison!"<<endl;
    if(y==0){
      return 0;
    }
    return -42;
  }
  if(y==0){
    cerr << "ERR: null pointer comparison!"<<endl;
    return 42;
  }

  //  cout << "Comparing " << x->toString() << " with " << y->toString()<<endl;

  if(x->size() < y->size()){
    return -1;
  }else if(x->size() > y->size()){
    return 1;
  }

  //recursive comparison of children
  if(x->size()>0){
    list<BF*>::const_iterator i=x->begin();
    for(list<BF*>::const_iterator j=y->begin();j!=y->end();j++){
      int re=compare(*i,*j);
      if(re!=0){
        return re;
      }
      i++;
    }
    return 0;
  }

  //leaf comparison:
  if(x->getDependence() < y->getDependence()){
    return -1;
  }else if(x->getDependence() > y->getDependence()){
    return 1;
  }else if(x->myType() != y->myType()){
    //arbitrary ordering of types without variable dependence
    return (int)y->front()->myType() - (int)x->front()->myType();
  }

  return 0;
}



std::ostream& operator<<(ostream& os, const  BF& obj){
  os << obj.toString();
  return os;
}


/// for use in sorting
bool boolCompare(const BF* const x,const BF* const y){
  return compare(x,y)<0;
}


/// comparison operators
inline bool operator< (const BF& x,const BF& y){
  return compare(&x,&y)<0;
}


inline bool operator==(const BF& x,const BF& y){
  return compare(&x,&y)==0;
}


inline bool operator!=(const BF& lhs, const BF& rhs){return !(lhs == rhs);}
inline bool operator> (const BF& lhs, const BF& rhs){return rhs < lhs;}
inline bool operator<=(const BF& lhs, const BF& rhs){return !(lhs > rhs);}
inline bool operator>=(const BF& lhs, const BF& rhs){return !(lhs < rhs);}


#endif
