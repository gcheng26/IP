#include <stdio.h>
#include <math.h>

float sqrt3(const float x) ;


int main(){

float y= sqrt (96);
int a=5;
int b=8;
printf("%f using sqrt \n %f using new method \n", y, sqrt3(96));
int x,z;
z=b>>2;
x= ( (a*a)-b < ((a-1)*(a-1))-b)? a:a-1;
printf ("%d \t %d ",x,z);
return 0;
}





float sqrt3(const float x)  
{
  union
  {
    int i;
    float x;
  } u;

  u.x = x;
  u.i = (1<<29) + (u.i >> 1) - (1<<22); 
  return u.x;
} 
