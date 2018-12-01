/*
sortngram.cpp

https://github.com/khusairu
https://github.com/mansayk/fastmorph/commits?author=khusairu&since=2017-07-31&until=2017-08-31

*/

#include<iostream>
#include<fstream>
#include<algorithm>
#include<vector>
#include<string>
#include<cstdlib>
#include<unistd.h>

struct NODEID{
  unsigned int nodeid;
  unsigned int parentid;
  unsigned int wordid;
  unsigned int weight;
  NODEID(){nodeid=parentid=wordid=weight=0;}
};

using namespace std;
vector<NODEID> vel;
bool velsort(unsigned i, unsigned j){ return vel[i].weight > vel[j].weight;}

int main(int argc, char *argv[]){
  string filename("outputfile.txt");
  //parse input arguments
  int opt;
  while((opt=getopt(argc,argv,"o:h"))!=-1){
    switch(opt){
    case 'h': cerr<<"./sortngram [-o <output filename>] [-h]"<<endl; break;
    case 'o': filename=optarg; break;
    }
  }


  
  //  cout<<"size:"<<sizeof(int)<<endl;
  string ins;
  NODEID el;
  int pos;
  //Load tree into memory
  char * endptr;
  unsigned point=0;
  unsigned short ciclecounter=0;
  while(getline(cin,ins)){
    if(ins.length()>0){
      if(ins[0]>='0' && ins[0]<='9'){
	el.nodeid=strtoul(ins.c_str(),&endptr,10);
	el.parentid=strtoul(++endptr,&endptr,10);
	el.wordid=strtoul(++endptr,&endptr,10);
	el.weight=strtoul(++endptr,&endptr,10);
       	while(point<=el.nodeid){ vel.push_back(NODEID()); ++point;}
	vel[el.nodeid]=el;
	if(!++ciclecounter) cout<<"Load tree... "<<point<<'\r';
      }
    }
  }
  cout<<endl<<"size of array for tree:"<<vel.size()<<endl;
  cout << "Ordering by level..."<<endl;
  vector <unsigned char> level(point,0);
  int sum[7]={0};
  for(auto element: vel)
    if(element.nodeid){
      level[element.nodeid]=level[element.parentid]+1;
      sum[level[element.nodeid]]++;
    }
  //part of code below based on a source confidence, the source must have parent node id less than child id
  for(int o=0;o<7;o++)
    cout<<"level:"<<o<<"->"<<sum[o]<<endl;

  ofstream fout;
  fout.open(filename);
  cout<<"Sorting by weight and saving to "<<filename<<endl;
  vector <unsigned> index;
  for(int lev=1;lev<7;++lev){
    cout<<"Level: "<<lev<<endl;  
    index.clear();
    for(int i=0;i<point;i++)
      if(level[i]==lev){
	index.push_back(i);
	vel[i].parentid=vel[vel[i].parentid].nodeid;
      }
    cout<<"Number of elements: "<<index.size()<<endl;
    sort(index.begin(),index.end(),velsort);
    //now nodeid will be position at index array
    for(int i=0;i<index.size();i++){
      vel[index[i]].nodeid=i;
      fout<<lev<<' '<<vel[index[i]].nodeid<<' '<<vel[index[i]].parentid<<' '<<vel[index[i]].wordid<<' '<<vel[index[i]].weight<<endl;
    }
  }
  fout.close();
    
}
