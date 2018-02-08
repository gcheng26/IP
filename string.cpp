
#include <string>
#include <iostream>
#include <bits/stdc++.h>
using namespace std;

string passstring()
{
	string stringy;
	for(int i=0; i<5;i++){
	stringy[i]='A';
	}
	return stringy;
}

int main() {
	
	string str1 = "Welcome";
	string str2	= "Welcale";
	
	if (str1.compare(str2) != 0)
  cout << str1 << " is not " << str2 << '\n';
  
  if (str1.compare(0,3,str2,0,3)==0)
  cout << str1 << " is the same as " << str2 << '\n';
  
  else
  cout<< "strings didn't match " << endl;
  
  
  string one= passstring();
  cout<< passstring();
	
	return 0;
}

