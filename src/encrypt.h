/*****************************************************************************
 *
 * @file encrypt.h
 *
 * @section DESCRIPTION
 *
 * This is the second implementation of the encryption algorithms
 * described in my kryptoSAT paper. It is somewhat optimised to run in
 * reasonable time.
 *
 *
 * @author  Sebastian Schmittner <sebastian@schmittner.pw>
 *
 * @version 2.1.2015-07-23
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


#ifndef ENCODE_H
#define ENCODE_H

#define ENCODERVERSION 2
#include <ctime>
#include <limits>
#include <cstddef>
#include <assert.h>

#include "functionParser.h"
#include "booleanFct.h"
#include "rng.h"

namespace kryptoSAT{


  //format for ANF: list<list<unsigned int>>
  //unsigned int = variable number>0
  //0=const 1

  //forwards
  list<list<unsigned int>>& addToANF(list<list<unsigned int>>& g,const list<list<unsigned int>>& gp);
  list<list<unsigned int>>& multiplyToANF(list<list<unsigned int>>& g,const list<list<unsigned int>>& gp, bool sortResult=true);




  //to string
  template<class T>
    std::ostream& operator<<(ostream& os, const  list<T>& obj){
    os << "(";
    for(typename list<T>::const_iterator i=obj.begin(); i != obj.end();i++){
      os << *i;
      if(next(i)!=obj.end()){
        os <<", ";
      }
    }
    os << ")";

    return os;
  }


  int compare(const list<unsigned int>& x,const list<unsigned int>& y){
    //    cout << "comparing " << x << " with " << y << endl;
    /*    if(x.size()<y.size())
          return -1;
          if(y.size()<x.size())
          return 1;
    */
    list<unsigned int>::const_iterator i=x.begin();
    list<unsigned int>::const_iterator j=y.begin();
    while(true){
      if(i==x.end()){
        if(j==y.end()){
          return 0;
        }
        return -1;
      }
      if(j==y.end())
        return 1;
      if (*i<*j)
        return -1;
      if (*i>*j)
        return 1;
      j++;
      i++;
    }
  }

  /// comparison operators
  inline bool operator< (const list<unsigned int>& x,const list<unsigned int>& y){
    return compare(x,y)<0;
  }

  bool cmp (const list<unsigned int>& x,const list<unsigned int>& y){//explicitly needed in sort...?
    return x<y;
  }

  inline bool operator==(const list<unsigned int>& x,const list<unsigned int>& y){
    return compare(x,y)==0;
  }

  inline bool operator!=(const list<unsigned int>& lhs, const list<unsigned int>& rhs){return !(lhs == rhs);}
  inline bool operator> (const list<unsigned int>& lhs, const list<unsigned int>& rhs){return rhs < lhs;}
  inline bool operator<=(const list<unsigned int>& lhs, const list<unsigned int>& rhs){return !(lhs > rhs);}
  inline bool operator>=(const list<unsigned int>& lhs, const list<unsigned int>& rhs){return !(lhs < rhs);}




  //////////////////////////////////////////////////////////////////////////


  /// takes two function in ANF and replaces the first one by a newly
  /// generated function given by the product (AND) of the two
  /// if !sortResult, the result needs to be run through sortANF(*,subsort=false) some time later to bring it into ANF
  list<list<unsigned int>>& multiplyToANF(list<list<unsigned int>>& g,const list<list<unsigned int>>& gp, bool sortResult){

    //    cout << "----Multiplying " << g << " with " << gp << "----"<<endl;

    //multiplication by 0
    if (g.size()==0){
      //      cout << "=0, nothing to do.\n----Multiplication done----"<<endl;
      return g;
    }

    if (gp.size()==0){
      g.clear();
      //      cout << "=0, nothing to do.\n----Multiplication done----"<<endl;
      return g;
    }


    //multiplication by 1
    if(gp.size()==1 && gp.front().front()==0){
      //      cout << "=" << g << " nothing to do.\n----Multiplication done----"<<endl;
      return g;
    }

    if(g.size()==1 && g.front().front()==0){
      g=gp;
      //      cout << "=" << g << " nothing to do.\n----Multiplication done----"<<endl;
      return g;
    }


    //actual multiplication
    //    cout << "non trivial multiplication"<<endl;

    list<list<unsigned int>> l;
    l.swap(g);


    for(list<list<unsigned int>>::const_iterator i=gp.begin();i!=gp.end();i++){
      for(list<list<unsigned int>>::const_iterator j=l.begin();j!=l.end();j++){

        //product of two conjunctions of literals

        ///     cout << "multiply " << *i << " * " << *j <<endl;

        //check if one factor is 1
        list<unsigned int> prod(*i);
        if((*i).front()==0){
          prod=list<unsigned int>(*j);
        }else if((*j).front() != 0){
          //if not, multiply

          for(list<unsigned int>::const_iterator lit2=(*j).begin();lit2!=(*j).end();lit2++){
            list<unsigned int>::iterator lit1=prod.begin();
            while(lit1!=prod.end() && *lit1<*lit2){
              lit1++;
            }
            if(lit1 == prod.end() || *lit1 > *lit2){
              lit1=prod.insert(lit1,*lit2);
            }
          }
        }

        //      cout << "Product " << prod <<endl;

        //add product to big sum
        if(sortResult){
          list<list<unsigned int>>::iterator k=g.begin();
          while(k!=g.end() && *k < prod)
            k++;
          if(k==g.end() || *k > prod){
            k=g.insert(k,prod);
          }else{
            k=g.erase(k);
          }
        }else{
          g.push_back(prod);
        }

        //      cout << "current sum " << g <<endl;
      }
    }

    //    cout << "Product: "<< g <<endl;
    //    cout << "-----Multiplication done-----"<<endl;
    return g;
  }


  /// takes two function in ANF and replaces the first one by a newly
  /// generated function given by the sum (XOR) of the two.
  /// Resulting ANF and inputs are (to be) ordered
  /// return equals modified first argument
  list<list<unsigned int>>& addToANF(list<list<unsigned int>>& g,const list<list<unsigned int>>& gp){

    //    cout << "----Adding ----\n" << g << "\n+\n" << gp <<endl;

    list<list<unsigned int>>::iterator j=g.begin();
    for(list<list<unsigned int>>::const_iterator i=gp.begin();i!=gp.end();i++){
      while(j!=g.end() && *j < *i)
        j++;
      if(j==g.end() || *j > *i){
        j=g.insert(j,*i);
      }else{
        j=g.erase(j);
      }
    }

    //    cout << "Sum: " << g << "\n----Addition done ----" << endl;

    return g;
  }


  /// several concats followed by sortANF are equivalent to several additions
  list<list<unsigned int>>& concatANF(list<list<unsigned int>>& g,const list<list<unsigned int>>& gp){
    g.insert(g.end(),gp.begin(),gp.end());
    return g;
  }




  //////////////////////////////////////////////////////////////////////////

  /// if subSort: sort all summands (if !subSort, these need to be already sorted)
  /// sort sum and delete matching pairs (XOR)
  list<list<unsigned int>>& sortANF(list<list<unsigned int>>& g, bool subSort=true){
    if(subSort){
      for(list<list<unsigned int>>::iterator i=g.begin();i!=g.end();i++){
        i->sort();
      }
    }
    //    cout << "sum sorting"<<endl;
    g.sort(cmp);
    //    cout <<"deleting"<<endl;
    for(list<list<unsigned int>>::iterator i=g.begin();i!=g.end();){
      if(next(i)!=g.end() && *next(i) == *i){
        i=g.erase(i);
        i=g.erase(i);
      }else{
        i++;
      }
    }
    return g;
  }

  /// works on the list of variables but eventually leaves it unchanged
  /// return is sorted if variables are sorted
  list<list<unsigned int>>* RandomFunction(rng * r,list<unsigned int> variables){
    //    cout << "Generating random function depending on " << variables<<endl;

    list<list<unsigned int>>* re = new list<list<unsigned int>>;
    if(r->randomBool()){
      re->push_back(list<unsigned int>(1,0));
    }
    for(list<unsigned int>::iterator v = variables.begin();v!=variables.end();){
      unsigned int V=*v;
      //      list<unsigned int>::iterator n = next(v);
      v=variables.erase(v);
      list<list<unsigned int>>* sub = RandomFunction(r,variables);
      for(list<list<unsigned int>>::iterator subI=sub->begin(); subI!=sub->end(); subI++){
        if(subI->front()==0){
          *(subI->begin()) = V;
        }else{
          subI->push_front(V);
        }
        re->push_back(*subI);
      }
      delete sub;
      //      v=variables.insert(n,V);
    }

    return re;
  }





  //////////////////////////////////////////////////////////////////////////

  /// Caution: expects sorted public key for the numbering of clauses to be consistent!
  booleanFct<BFT_XOR>* encrypt(rng * r,const size_t& privateKeyLength, const booleanFct<BFT_AND>* publicKey, const bool& input, const size_t& beta_){


    if(publicKey->size() > numeric_limits<unsigned int>::max() || privateKeyLength > (unsigned int)numeric_limits<int>::max()){
      cerr << "ERR: fast encode is limited to int, i.e. key length " << std::numeric_limits<int>::max()<<endl;
    }


    unsigned int n=privateKeyLength;
    unsigned int m = publicKey->size();
    unsigned int beta=beta_;
    bool Y = input;

    cout << "--------- Encryption --------"<<endl;
    cout << "Encrypting: " << Y<<endl;
    //    cout << "With public Key: " << publicKey->toString()<<endl;
    cout << "n = " << n <<endl;
    cout << "m = " << m <<endl;
    cout << "beta = " << beta <<endl;


    //cipher (level 1: XOR, level 2:AND)
    list<list<unsigned int>> g;

    // generate a permutation of m elements
    list<unsigned int> notUsed;
    for(unsigned int i=0;i<m;i++){
      notUsed.push_back(i);
    }

    unsigned int* s = new unsigned int[m];
    for(unsigned int i=0;i<m;i++){
      bool set=false;
      unsigned int invProb=notUsed.size();
      for(list<unsigned int>::iterator j=notUsed.begin();j!=notUsed.end()&&!set;){
        if(r->randomInt(invProb)==0){
          s[i]= *j;
          j=notUsed.erase(j);
          set=true;
          continue;
        }else{
          j++;
        }
        invProb--;
      }
    }

    /*
      cout << "Permutation generated" << endl;

      for(unsigned int i=0;i<m;i++){
      cout << "s["<<i<<"]="<<s[i]<<endl;
      }
    */

    //ANF of clauses
    //nClause[i] represents the s[i]-th negated clause in the public key
    list<list<unsigned int>>* nClause= new list<list<unsigned int>>[m];
    //list of variables on which nClause[i] depends
    list<unsigned int>* depends = new list<unsigned int>[m];

    //generate these lists
    unsigned int cN=0;
    for(list<BF*>::const_iterator k=publicKey->begin(); k!=publicKey->end();k++){
      //inverse search
      unsigned int i=0;
      bool found=false;
      for(; i<m && !found;i++){
        if(s[i]==cN){
          found=true;
        }
      }
      i--;
      if(!found){
        cerr << "Error in permutation. No inverse of " << cN<<endl;
        return 0;
      }


      //  cout << "s^{-1}("<<cN<<")="<<i<<endl;

      if(nClause[i].size()!=0 || depends[i].size()!=0){
        cerr << "Error in permutation, clause exists."<<endl;
        return 0;
      }


      //        cout << "Converting " << (*k)->toString() <<endl;

      nClause[i].push_back(list<unsigned int>(1,0));//constant 1

      for(list<BF*>::const_iterator lit=(*k)->begin(); lit!=(*k)->end();lit++){
        int V =(*lit)->getDependence();

        //      cout << "Literal " << (*lit)->toString()<< " = " <<V<<endl;
        list<list<unsigned int>> cur;

        if(V>0){
          cur.push_back(list<unsigned int>(1,0));
          cur.push_back(list<unsigned int>(1,V));
          depends[i].push_back(V);
        }else{
          cur.push_back(list<unsigned int>(1,-V));
          depends[i].push_back(-V);
        }

        multiplyToANF(nClause[i],cur);
      }
      //      cout << "negated clause: " << nClause[i]<<endl;
      cN++;
    }


    delete[] s;

    //    cout << "Lists prepared"<<endl;
    /*    cout << "negated clauses:" <<endl;

          for(unsigned int i=0; i<m;i++){
          cout << "c_s["<<i<<"] = " << nClause[i]<<endl;
          cout << "depends on " << depends[i]<<endl;
          }
    */

    clock_t overall = clock();
    size_t randomFunctionsTimer=0;
    size_t dependenciesTimer=0;
    size_t multiplicationTimer=0;
    size_t additionTimer=0;

    clock_t start;
    for(unsigned int i=0;i<m;i++){
      //generate the cipher summand from the set of clauses (s[i], s[i+1],...,s[i+\beta])

      for(unsigned int j=0;j<beta;j++){
        //      cout << "Generating R_{" << i << "," << j <<"}"<<endl;

        start = clock();

        list<unsigned int> Rdepends;
        for(unsigned int k=0;k<beta;k++){
          if(k!=j){
            //      cout << "inserting " << depends[(k+i)%m]<<endl;
            Rdepends.insert(Rdepends.end(),depends[(k+i)%m].begin(),depends[(k+i)%m].end());
          }
        }
        //      cout << "gathered:" << Rdepends <<endl;

        //delete doubles
        Rdepends.sort();
        for(list<unsigned int>::iterator i=Rdepends.begin();i!=Rdepends.end();){
          if(next(i)!=Rdepends.end() && *next(i)==*i){
            i=Rdepends.erase(i);
          }else{
            i++;
          }
        }
        dependenciesTimer+=clock()-start;

        start = clock();

        list<list<unsigned int>>* R = RandomFunction(r,Rdepends);

        randomFunctionsTimer+=clock()-start;

        //sortANF(*R);

        //        cout << "Generated random function " << *R<<endl;

        start = clock();

        multiplyToANF(*R,nClause[(i+beta)%m],false);

        multiplicationTimer+=clock()-start;

        start = clock();

        //        addToANF(g,*R);
        concatANF(g,*R);

        additionTimer+=clock()-start;

        delete R;
      }
    }//next tuple

    start = clock();
    sortANF(g,false);
    additionTimer+=clock()-start;


    cout << "Encryption done in\t\t\t" << 1000.0 * (clock()-overall) / CLOCKS_PER_SEC  << " ms"<<endl;
    cout << "There of:\trandom functions: \t" << 1000.0 * randomFunctionsTimer / CLOCKS_PER_SEC  << " ms"<<endl;
    cout << "\t\tdependency lists: \t" << 1000.0 * dependenciesTimer / CLOCKS_PER_SEC  << " ms"<<endl;
    cout << "\t\tANF multiplication: \t" << 1000.0 * multiplicationTimer / CLOCKS_PER_SEC  << " ms"<<endl;
    cout << "\t\tANF addition: \t\t" << 1000.0 * additionTimer / CLOCKS_PER_SEC  << " ms"<<endl;


    delete[] nClause;
    delete[] depends;


    // Y to ANF
    if(Y){
      addToANF(g,list<list<unsigned int>>(1,list<unsigned int>(1,0)));
    }

    cout << "Converting back to usual representation"<<endl;

    booleanFct<BFT_XOR>* re= new booleanFct<BFT_XOR>(n);

    for(list<list<unsigned int>>::const_iterator i=g.begin();i!=g.end();i++){
      re->push_back(new booleanFct<BFT_AND>(n));
      for(list<unsigned int>::const_iterator j=(*i).begin();j!=(*i).end();j++){
        if(*j==0){
          re->back()->push_back(new booleanFct<BFT_TRUE>(n));
        }else{
          re->back()->push_back(new booleanFct<BFT_INPUT>(n,*j - 1));
        }
      }
    }

    cout << "--------- Encryption done --------"<<endl;
    //    cout << "Cipher = " <<g->toString()<<endl;
    return re;
  }


}//end namespace


#endif
