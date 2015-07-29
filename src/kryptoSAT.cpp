/*****************************************************************************
 *
 * @file kryptoSAT.cpp
 *
 * @section DESCRIPTION
 *
 * Main for the reference implementation.
 *
 * @author  Sebastian Schmittner <sebastian@schmittner.pw>
 *
 * @version 1.0.2015-06-13
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

#include <iostream>
#include <sstream>
#include <string>

#include <stdexcept>

using namespace std;

#include "functionParser.h"
using namespace functionParser;

#include "booleanFct.h"
#include "rng.h"
#include "kryptoSAT.h"

using namespace kryptoSAT;

/// store the parsed command line arguments here
class state{
public:

  bool batchMode;
  bool generateMode;
  bool encryptMode;
  bool decryptMode;

  string pubFile;
  string privFile;
  string clearFile;
  string cipherFile;
  string outFile;

  unsigned int k;
  size_t n;
  size_t m;

  booleanFct<BFT_AND>* publicKey;
  bool* privateKey;

  bool* clearText;
  size_t clearTextLength;
  booleanFct<BFT_XOR>** cipher;


  size_t salt;
  rng * r;

  //  size_t alpha;
  size_t beta;

  state():
    batchMode(false),
    generateMode(false),
    encryptMode(false),
    decryptMode(false),
    k(3),
    n(1024),
    m(0),
    publicKey(0),
    privateKey(0),
    clearText(0),
    clearTextLength(0),
    cipher(0),
    //    alpha(0),
    beta(3)
  {
    r=new mersenneTwisterRNG();
    salt=r->getGoodSeed();
  }

  ~state(){
    delete publicKey;
    publicKey=0;
    delete privateKey;
    privateKey=0;
    delete clearText;
    clearText=0;
    if(cipher!=0){
      for(unsigned int i=0;i<clearTextLength;i++){
	cipher[i]->disposeChildren();
	delete cipher[i];
	cipher[i]=0;
      }
    }
    delete[] cipher;
    cipher=0;
    delete r;//=>crash??
  }

  bool conflict(){
    if(batchMode && outFile.compare("")==0){
      cerr << "Conflict: Trying to enter batch mode without output file."<<endl;
      return true;
    }

    if (generateMode && decryptMode){
      cerr << "Conflict: Generating a random key to decrypt a given cipher does not make sense."<<endl;
      return true;
    }

    if(encryptMode && (clearFile.compare("")==0 || (pubFile=="" && ! generateMode))){
      cerr << "Conflict: I can not encrypt without a public key and clear text."<<endl;
      return true;
    }

    if(decryptMode && (cipherFile.compare("")==0 || (privFile=="" && ! generateMode))){
      cerr << "Conflict: I can not decrypt without a private key and cipher."<<endl;
      return true;
    }

    return false;
  }

  void checkM(){
    if(m==0){
      m = n * 5;
    }
  }

};


void help(){
  cout << "kryptoSAT [-h] [-b] [-g [-ksat LITERALSPERCLAUSE=3] [-n VARIABLES=1024] [-m CLAUSES=5n]] [-k PUBLICKEYFILE]  [-K PRIVATEKEYFILE] [-be BETA=3] [-c CIPHERFILE] [-t CLEARTEXTFILE] [-s SALT] [-o OUTFILE]"<<endl;
  cout << "-h\tDisplay this help."<<endl;
  cout << "-b\tBatchmode enforced. If conflicts are encountered, exit with an error instead of entering interactive mode. Must be used with -o."<<endl;
  cout << "-g\tGenerate new key pair. Conflicts with -k and -K."<<endl;
  cout << "\t-ksat\twith ksat=LITERALSPERCLAUSE literals per clause. Implies -g."<<endl;
  cout << "\t-n\twith n=VARIABLES private key size. Implies -g."<<endl;
  cout << "\t-m\twith m=CLAUSES clauses. Implies -g. "<<endl;
  cout << "-k\tRead public key from PUBLICKEYFILE. Conflicts with -g."<<endl;
  cout << "-K\tRead private key from PRIVATEKEYFILE. Conflicts with -g."<<endl;
  //  cout << "-al\tSet alpha=ALPHA parameter for encryption. CAUTION 2<alpha<8 recommended." <<endl;
  cout << "-be\tSet beta=BETA parameter for encryption." <<endl;

  cout << "-c\tRead cipher from CIPHERFILE and decrypt with private key, if given."<<endl;
  cout << "-t\tRead clear text from CLEARTEXTFILE and encrypt with public key, if given."<<endl;
  cout << "-s\tSet the salt for encryption to SALT."<<endl;
  cout << "-o\tTry to do something useful with the other options given and write the output to OUTFILE. If this option is omitted or conflicting options are given, kryptoSAT will enter an interactive mode, unless -b is specified."<<endl;
  cout <<endl;
  exit(0);
}

/// verify pub(priv)=1
bool checkKeyPair(state& I){
  if(I.publicKey==0||I.privateKey==0){
    cerr << "ERR: No key pair loaded."<<endl;
    return false;
  }

  bool re = I.publicKey->evaluate(I.privateKey);
  if(!re){
    cerr<<"Invalid key pair!"<<endl;
  }else{
    cout << "\n\t[OK]\tpub(priv)=1. Key pair valid."<<endl;
  }
  return re;
}


bool generateKeyPair(state& I){
  cout << "Generating new key pair..."<<endl;
  I.r->randomise(I.r->getGoodSeed());
  I.privateKey = generatePrivateKey(I.r,I.n);
  cout << "Private key generated"<<endl;
  //  writeBool(cout, I.privateKey, I.n);
  I.publicKey = generatePublicKey(I.r, I.privateKey,I.n,I.m, I.k);
  cout << "Public key generated"<<endl;
  //writeCNF(cout, I.publicKey);
  return (I.publicKey !=0 && I.privateKey !=0);
}

bool readPublicKey(state& I){
  if(!I.batchMode){
    string in;
    cout << "Public key file name (" << I.pubFile <<"):";
    getline(cin,in);
    if(in.compare("")!=0){
      I.pubFile=in;
    }
  }

  I.publicKey=readCNF(I.pubFile.c_str());
  I.n = I.publicKey->getNumberOfVars();

  return I.publicKey!=0;
}

bool readPrivateKey(state& I){
  if(!I.batchMode){
    string in;
    cout << "Private key file name (" << I.privFile <<"):";
    getline(cin,in);
    if(in.compare("")!=0){
      I.privFile=in;
    }
  }

  I.privateKey=readBool(I.privFile.c_str(),I.n);

  return I.privateKey!=0;
}



bool readText(state& I){
  if(!I.batchMode){
    string in;
    cout << "Text file name (" << I.clearFile <<"):";
    getline(cin,in);
    if(in.compare("")!=0){
      I.clearFile=in;
    }
  }

  I.clearText=readBool(I.clearFile.c_str(),I.clearTextLength);

  return I.clearText!=0;
}



bool readCipher(state& I){

  if(!I.batchMode){
    string in;
    cout << "Cipher file name (" << I.cipherFile <<"):";
    getline(cin,in);
    if(in.compare("")!=0){
      I.cipherFile=in;
    }
  }

  ifstream file(I.cipherFile);

  cout << "Reading cipher from " << I.cipherFile<<endl;

  stringstream * temp = 0;
  I.salt=0;
  bool found=false;
  for (string line; getline(file, line)&& !found;) {
    if(line.size()>0){
      if(line.at(0)=='s'){
	istringstream iss(line);
	string token;
        found = getline(iss, token, ' ') && getline(iss, token, ' ');
	I.salt = stol(token);
        found = found && getline(iss, token, ' ');
        I.clearTextLength = stol(token);
        found = found && getline(iss, token, ' ');
	I.beta = stol(token);
        continue;
      }
    }
  }

  if(!found){
    cerr <<"ERR: File format error."<<endl;
    return false;
  }
  cout << "Reading cipher of length " << I.clearTextLength << " salt = " <<I.salt<<endl;

  I.cipher = new booleanFct<BFT_XOR>*[I.clearTextLength];
  size_t i=0;

  for (string line; getline(file, line); ) {
    if(line.size()>0){
      if(line.at(0)=='p'){
        if(temp!=0){
          I.cipher[i] = readANF(*temp);
          delete temp;
          if(I.cipher[i]==0){
            cerr << "ERR: Error reading cipher."<<endl;
            return false;
          }
          i++;
        }
        temp = new stringstream();
      }
      if(temp!=0){
        *temp << line <<endl;
      }
    }
  }
  file.close();

  //last anf
  I.cipher[i] = readANF(*temp);
  delete temp;
  if(I.cipher[i]==0){
    cerr << "ERR: Error reading cipher."<<endl;
    return false;
  }
  i++;

  if(i != I.clearTextLength){
    cout << "ERR: Read " << i << " encrypted bits, although text length should be " << I.clearTextLength <<endl;
    return false;
  }

  cout << "\n\t[OK]\tCipher read."<<endl;

  return true;
}



bool encrypt(state& I){
  if(I.clearText==0){
    cerr <<"ERR: No clear text loaded!"<<endl;
    return false;
  }
  if(I.publicKey==0){
    cerr <<"ERR: No public key loaded!"<<endl;
    return false;
  }
  size_t seed=0;
  size_t pow=1;
  for(size_t i=0;i<I.clearTextLength;i++){
    seed+= pow * I.clearText[i];
    pow*=2;
    if(pow!=pow){//overflow
      pow=1;
    }
  }
  cout << "Salting clear text with " << I.salt<<endl;
  //todo: this is not exactly a salted hash ;) -> currently relying on the rng seed() to do the hashing
  seed = I.salt ^ seed;
  I.r->seed(seed);

  I.cipher = new booleanFct<BFT_XOR>*[I.clearTextLength];
  cout << "Sorting public key..."<<endl;
  I.publicKey->recursiveSort();

  cout << "Starting encryption..."<<endl;

  for(size_t i=0;i<I.clearTextLength;i++){
    I.cipher[i] = encrypt(I.r,I.n, I.publicKey, I.clearText[i], I.beta);
  }
  cout <<"\n\t[OK]\tEncryption done"<<endl;

  return true;
}

bool decrypt(state& I){
  if(I.cipher==0){
    cerr <<"ERR: No cipher loaded!"<<endl;
    return false;
  }
  if(I.privateKey==0){
    cerr <<"ERR: No private key loaded!"<<endl;
    return false;
  }

  I.clearText = new bool[I.clearTextLength];
  cout << "Starting decryption..."<<endl;

  for(size_t i=0;i<I.clearTextLength;i++){
    I.clearText[i]= I.cipher[i]->evaluate(I.privateKey);
  }
  cout <<"done"<<endl;

  return true;
}



bool verifyCipher(state& I){
  stringstream old;
  if(I.cipher==0){
    cerr << "ERR: No cipher loaded!"<<endl;
    return false;
  }
  if(I.clearText==0){
    cerr << "ERR: No clear text loaded!"<<endl;
    return false;
  }
  if(I.privateKey==0){
    cerr << "ERR: No private key loaded!"<<endl;
    return false;
  }

  cout << "saving current cipher"<<endl;
  old << "c Cipher"<<endl;
  old << "s "<<I.salt<<endl;
  bool re=true;
  for(size_t i=0;i<I.clearTextLength;i++){
    re= re && writeANF(old, I.cipher[i]);
    delete I.cipher[i];
  }
  delete[] I.cipher;

  if(!re){
    cerr << "Writing cipher failed!"<<endl;
    return false;
  }

  cout << "Re-encrypting..."<<endl;
  encrypt(I);


  stringstream newC;

  cout << "Comparing..."<<endl;
  newC << "c Cipher"<<endl;
  newC << "s "<<I.salt<<endl;
  for(size_t i=0;i<I.clearTextLength;i++){
    re= re && writeANF(newC, I.cipher[i]);
  }

  if(!re){
    cerr << "Writing cipher failed!"<<endl;
    return false;
  }


  string newLine;
  for (string line; getline(old, line); ) {
    getline(newC,newLine);
    if(line.compare(newLine)!=0){
      cerr <<"\n\t[fail]\tMismatch!"<<endl;
      return false;
    }
  }
  cout <<"\n\t[OK]\tEncryptions match."<<endl;

  return true;
}

bool saveText(state& I){
  bool re = writeBool(I.outFile.c_str(),I.clearText, I.clearTextLength);
  if (re){
    cout << "Wrote clear text to " << I.outFile <<endl;
  }else{
    cerr <<"ERR: Error clear text cipher."<<endl;
  }
  return re;
}

bool saveCipher(state& I){
  ofstream out(I.outFile.c_str());
  if (!out.good()){
    cerr<< "ERR: could not open file " << I.outFile << " for writing." <<endl;
    return false;
  }

  bool re=true;

  out << "c Cipher"<<endl;
  out << "c Format of the next line: 's salt textLength beta'"<<endl;
  out << "s "<<I.salt<< " " << I.clearTextLength << " " << I.beta << endl << "c" <<endl;

  for(size_t i=0;i<I.clearTextLength;i++){
    out << "c ----------------------------------------"<<endl;
    out << "c --------------next bit------------------"<<endl;
    out << "c ----------------------------------------"<<endl;
    re= re && writeANF(out, I.cipher[i]);
  }

  out.close();
  if (re){
    cout << "Wrote cipher to " << I.outFile <<endl;
  }else{
    cerr <<"ERR: Error saving cipher."<<endl;
  }
  return re;
}

bool savePrivateKey(state& I){
  bool re= writeBool(I.outFile.c_str(), I.privateKey, I.n);
  if (re){
    cout << "Wrote private Key to " << I.outFile <<endl;
  }else{
    cerr <<"ERR: Error saving private key."<<endl;
  }
  return re;
}

bool savePublicKey(state& I){
  bool re= writeCNF(I.outFile.c_str(), I.publicKey);
  if (re){
    cout << "Wrote public Key to " << I.outFile <<endl;
  }else{
    cerr <<"ERR: Error saving public key."<<endl;
  }
  return re;
}



///interactive mode
int menu(state& I){


  while(true){
    cout << endl;
    cout << "-------------------------------------------------------"<<endl;
    cout << "Status:"<<endl;
    cout << "-------------------------------------------------------"<<endl;
    cout << "Private Key:\t";
    if (I.privateKey==0){cout << "--";}else{cout << "OK (n=" << I.n <<")";}
    cout << "\nPublic Key:\t";
    if (I.publicKey==0){cout << "--";}else{cout << "OK (m=" << I.publicKey->size() <<")";}

    cout << "\nClear text:\t";
    if (I.clearText==0){cout << "--";}else{cout << "OK ("<<I.clearTextLength<<")";}
    cout << "\nCipher:     \t";
    if (I.cipher==0){cout << "--";}else{cout << "OK ("<<I.clearTextLength<<")";}
    cout << "\n-------------------------------------------------------"<<endl;
    cout <<endl;

    cout << "-------------------------------------------------------"<<endl;
    cout << "Options:"<<endl;
    cout << "-------------------------------------------------------"<<endl;

    cout << "0)\tQuit" <<endl;
    cout << "1)\tGenerate Key pair" <<endl;
    cout << "2)\tLoad Private Key" <<endl;
    cout << "3)\tLoad Public Key" <<endl;
    cout << "4)\tLoad clear Text" <<endl;
    cout << "5)\tLoad cipher" <<endl;
    cout << "6)\tEncrypt" <<endl;
    cout << "7)\tDecipher" <<endl;
    cout << "8)\tSave data to file" <<endl;
    cout << "9)\tCheck key pair" <<endl;
    cout << "10)\tVerify honest encryption" <<endl;

    cout << "-------------------------------------------------------"<<endl;
    cout <<":";
    string in;
    getline(cin,in);
    cout << "\n-------------------------------------------------------"<<endl;
    try{
      switch (stoi(in)){
      case 0:
        return 0;
      case 1:
        try{
          cout <<"I will now generate a new key pair."<<endl;
          cout << "Private key length (" << I.n << "):";
          getline(cin,in);
          if(stol(in)>0){
            I.n=stoul(in);
          }
        }catch(const invalid_argument& e){}

        try{
          cout << "Number of clauses in public key (" << I.m << "):";
          getline(cin,in);
          if(stol(in)>0){
            I.m=stoul(in);
          }
        }catch(const invalid_argument& e){}

        try{
          cout << "Variables per clause (" << I.k << "):";
          getline(cin,in);
          if(stoi(in)>2){
            I.k=stoi(in);
          }
        }catch(const invalid_argument& e){}


        generateKeyPair(I);
        break;
      case 2:
        readPrivateKey(I);
        break;
      case 3:
        readPublicKey(I);
        break;
      case 4:
        readText(I);
        break;
      case 5:
        readCipher(I);
        break;
      case 6:
        encrypt(I);
        break;
      case 7:
        decrypt(I);
        break;
      case 8:
        cout << "File name:";
        getline(cin,I.outFile);
        cout << "\n-------------------------------------------------------"<<endl;
        cout << "Options:"<<endl;
        cout << "-------------------------------------------------------"<<endl;

        cout << "1)\tSave private key" <<endl;
        cout << "2)\tSave public key" <<endl;
        cout << "3)\tSave clear text" <<endl;
        cout << "4)\tSave cipher" <<endl;

        getline(cin,in);

        switch(stoi(in)){
        case 1:
          savePrivateKey(I);
          break;
        case 2:
          savePublicKey(I);
          break;
        case 3:
          saveText(I);
          break;
        case 4:
          saveCipher(I);
          break;
        }
        break;
      case 9:
        checkKeyPair(I);
        break;
      case 10:
        verifyCipher(I);
        break;
      }
    }catch(const invalid_argument& e){
      return 0;
    }

  }//loop
}




int main(int args, char *arg[]){

  state I;

  //kryptoSAT [-h] [-b] [-g [-ksat LITERALSPERCLAUSE=3] [-n VARIABLES=10^5] [-m CLAUSES=n2^K]] [-k PUBLICKEYFILE]  [-K PRIVATEKEYFILE] [-c CIPHERFILE] [-t CLEARTEXTFILE] [-o OUTFILE]

  for (int i=1;i<args;i++){
    if(strcmp(arg[i],"-K")==0){
      i++;
      I.privFile=arg[i];
    }else if(strcmp(arg[i],"-b")==0){
      I.batchMode=true;
    }else if(strcmp(arg[i],"-g")==0){
      I.generateMode=true;
    }else if(strcmp(arg[i],"-ksat")==0){
      i++;
      I.k=atoi(arg[i]);
      I.generateMode=true;
    }else if(strcmp(arg[i],"-n")==0){
      i++;
      I.n=atol(arg[i]);
      I.generateMode=true;
    }else if(strcmp(arg[i],"-m")==0){
      i++;
      I.m=atol(arg[i]);
      I.generateMode=true;
    }else if(strcmp(arg[i],"-k")==0){
      i++;
      I.pubFile=arg[i];
    }else if(strcmp(arg[i],"-c")==0){
      i++;
      I.cipherFile=arg[i];
      I.decryptMode=true;
    }else if(strcmp(arg[i],"-t")==0){
      i++;
      I.clearFile=arg[i];
      I.encryptMode=true;
    }else if(strcmp(arg[i],"-s")==0){
      i++;
      I.salt=atol(arg[i]);
      /*    }else if(strcmp(arg[i],"-al")==0){
            i++;
            I.alpha=atol(arg[i]);
      */
    }else if(strcmp(arg[i],"-be")==0){
      i++;
      I.beta=atol(arg[i]);
    }else if(strcmp(arg[i],"-o")==0){
      i++;
      I.outFile=arg[i];
    }else{
      help();//in particular -h --help -? -nonsense ;)
    }

  }


  // set m to default, if not specified
  I.checkM();

  if(!I.batchMode){
    cout << "kryptoSAT  Copyright (C) 2015 Sebastian E. Schmittner"<<endl;
    cout <<"This program comes with ABSOLUTELY NO WARRANTY."<<endl;
    cout <<"This is free software, and you are welcome to redistribute it and/or modify" <<endl;
    cout << "it under the terms of the GNU General Public License as published by"<<endl;
    cout << "the Free Software Foundation, either version 3 of the License, or"<<endl;
    cout <<"(at your option) any later version."<<endl<<endl;
    cout << "For details read the appropriate parts of the License."<<endl;
    cout << "\nYou should have received a copy of the GNU General Public License"<<endl;
    cout <<"along with KryptoSAT.  If not, see <http://www.gnu.org/licenses/>."<<endl<<endl;
  }

  if(I.conflict()){
    cout << "Conflicting arguments encountered."<<endl;
    if(I.batchMode){
      cerr << "ERR: Conflict in batch mode!"<<endl;
      return -1;
    }
    cout << "I am not sure what to do. Entering menu."<<endl;

    return menu(I);

  }else if(I.generateMode){
    generateKeyPair(I);
    if(I.outFile.compare("")!=0){
      string ori(I.outFile);
      string pubName(I.outFile);
      string privName(I.outFile);
      pubName+=".pub";
      privName+=".priv";
      I.outFile=privName;
      savePrivateKey(I);
      I.outFile=pubName;
      savePublicKey(I);
      I.outFile=ori;
    }
  }

  if(I.encryptMode){
    if(!I.generateMode){
      readPublicKey(I);
    }
    if(!readText(I)){
      cerr << "ERR: Error reading text!"<<endl;
      return menu(I);
    }
    if(!encrypt(I)){
      cerr << "ERR: Encryption failed!"<<endl;
      return menu(I);
    }
    if(I.outFile.compare("")!=0){
      string ori(I.outFile);
      string cipherName(I.outFile);
      cipherName+=".cipher";
      I.outFile=cipherName;
      saveCipher(I);
      I.outFile=ori;
    }
  }
  if(I.decryptMode){
    if(!readPrivateKey(I)){
      cerr << "ERR: Error reading key!"<<endl;
      return menu(I);
    }
    if(!readCipher(I)){
      cerr << "ERR: Error reading cipher!"<<endl;
      return menu(I);
    }
    if(!decrypt(I)){
      cerr << "ERR: Decryption failed!"<<endl;
      return menu(I);
    }
    if(I.outFile.compare("")!=0){
      string ori(I.outFile);
      string cipherName(I.outFile);
      cipherName+=".clear";
      I.outFile=cipherName;
      saveText(I);
      I.outFile=ori;
    }
  }

  if(I.outFile.compare("")==0){
    return menu(I);
  }

  return 0;
}

