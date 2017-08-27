/*
parse.cpp 
input: <word id>,<suggestion id>
*/

#include<iostream>
#include<vector>
#include<map>
#include<forward_list>
#include<unistd.h>
using namespace std;

typedef forward_list<int> Intlist;

struct SFrase{
  int count;
  const int id;
  //  forward_list<SFrase *> nextwords;
  map<int,SFrase *> lwords;
  SFrase(int z):id(z),count(1){}
  void AddNode(Intlist &);
  void PrintNode(string);
  ~SFrase();
};


SFrase::~SFrase(){
  for(auto  it=lwords.begin();it!=lwords.end();it++)
    delete it->second;
}
void SFrase::AddNode( Intlist & wordlist){
  bool found=false;
  int nid=wordlist.front(); //first word in suggestion
  SFrase *elem;
  try{
    elem=lwords.at(nid);
    elem->count++;   
  }catch (const std::out_of_range& oor) {
    elem=new SFrase(nid);
    lwords[nid]=elem;
  }
  wordlist.pop_front();
  if(!wordlist.empty())
    elem->AddNode(wordlist);
}


void SFrase::PrintNode(string x){
  if(id)//a zero id is root of tree and should not be printed
    x+=to_string(id)+'('+to_string(count)+')';
  if(!lwords.empty()){
    for(auto  it=lwords.begin();it!=lwords.end();it++){
      it->second->PrintNode(x+' ');
    }
  }else
    cout<<x<<endl;
}


//----------------------------------------
SFrase fr(0);
int startId(-1),endId(-1);

void AddInfo2tree( vector<int> &sug){
  Intlist fwlsug;
  for(int i=0;i<sug.size();i++){
    if(startId!=-1 && startId > sug[i]) continue;
    if(endId!=-1 && endId < sug[i]) continue;
    auto it=fwlsug.before_begin();
    for(int j=i;j<sug.size() && ((j-i)<6);j++)
      it=fwlsug.insert_after(it,sug[j]);
    fr.AddNode(fwlsug);
    fwlsug.clear();
  }
}

int main(int argc, char *argv[])
{
  //  int NodeId(0);
  //parse input arguments
  int opt;
  while((opt=getopt(argc,argv,"s:e:i:h"))!=-1){
    switch(opt){
    case 'h': cerr<<"./parse [-s <start word ID>] [-e <end word ID>] [-i <start node ID>"<<endl; break;
    case 's': startId=atoi(optarg); break;
    case 'e': endId=atoi(optarg); break;
      //    case 'i': NodeId=atoi(optarg); break;
    }
  }
  cerr<<"word id in range:[";  if(startId!=-1) cerr<<startId; else cerr <<"any";
  cerr<<':'; if(endId!=-1) cerr<<endId; else cerr<< "any"; cerr<<']'<<endl;

  //parsing input data
  int count=0;
  string ins;
  size_t pos=0;
  int numsug=-1;
  Intlist fwlsug;
  vector<int> sug;
  while(getline(cin,ins)){
     pos=ins.find(',');
     int numword=atoi(ins.c_str());
     int cursug=atoi(ins.substr(pos+1,ins.size()-pos).c_str());
     if(numsug!=cursug){
	     if(numsug!=-1){
	       AddInfo2tree(sug);
	       sug.clear();
	     }
	     numsug=cursug;
     }
     sug.push_back(numword);
     count++;
     if(!(count % 1000)) cerr<<count/1000<<" (thousands) \r";
  }
  AddInfo2tree(sug);

  fr.PrintNode("");


}
