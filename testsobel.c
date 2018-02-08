#include <stdio.h>

#define col 4
#define row 4
float xconv(int a, int b);
float yconv(int a, int b);
const float imgmatrix[5][5]= {{200, 0 , 0 , 0 ,200},
							  { 0 ,200, 0 ,200, 0 },
							  { 0 , 0 ,200, 0 , 0 },
							  { 0 ,200, 0 ,200, 0 },
							  {200, 0 , 0 , 0 ,200}};
int main ()
{
	float xkernel[3][3]= {{-1, 0, 1},
						  {-2, 0, 2},
						  {-1, 0, 1}};

	float ykernel[3][3]= {{-1,-2, -1},
						  {0,  0,  0},
						  {1,  2,  1}};
						  
							
	float xconvmatrix[5][5]={0};
	float yconvmatrix[5][5]={0};

	for (int b=0; b<5; b++)
	{	
		for (int a=0; a<5; a++)
		{
			xconvmatrix[a][b]=xconv(a,b);
			printf ("%.0f ",xconvmatrix[a][b]);
		}
		printf("\n");
	}
	for (int b=0; b<5; b++)
	{	
		for (int a=0; a<5; a++)
		{
			yconvmatrix[a][b]=yconv(a,b);
			printf ("[%d][%d]%.0f\t ",a,b,yconvmatrix[a][b]);
		}
		printf("\n");
	}
return 0;
}

float xconv(int a, int b)
{
	float tempx=0;
	if (a==0)
	{
		if(b==0)
			tempx=(imgmatrix[a+1][b]*2)+(imgmatrix[a+1][b+1]);
		else if (b==col)
			tempx=(imgmatrix[a+1][b-1])+(imgmatrix[a+1][b]*2);
		else
			tempx=(imgmatrix[a+1][b-1])+(imgmatrix[a+1][b]*2)+(imgmatrix[a+1][b+1]);
	}
	else if (a==row)
	{
		if(b==0)
			tempx=(imgmatrix[a-1][b]*-2)-(imgmatrix[a-1][b+1]);
		else if (b==col)
			tempx=(imgmatrix[a-1][b]*-2)-(imgmatrix[a-1][b-1]);
		else
			tempx=(imgmatrix[a-1][b]*-2)-(imgmatrix[a-1][b-1])-(imgmatrix[a-1][b+1]);
	}
	else
	{
		if(b==0)
			tempx=(imgmatrix[a+1][b]*2)-(imgmatrix[a-1][b]*2)-(imgmatrix[a-1][b+1])+(imgmatrix[a+1][b+1]);
		else if (b==col)
			tempx=(imgmatrix[a+1][b-1])-(imgmatrix[a-1][b-1])-(imgmatrix[a-1][b]*2)+(imgmatrix[a+1][b]*2);
		else
			tempx=(imgmatrix[a+1][b-1])-(imgmatrix[a-1][b-1])-(imgmatrix[a-1][b]*2)+(imgmatrix[a+1][b]*2)-(imgmatrix[a-1][b+1]*1)+(imgmatrix[a+1][b+1]);
	}
	
	return tempx;
}

float yconv(int a, int b)
{
	float tempy=0;
	if (a==0)
	{
		if (b==0)
			tempy=(imgmatrix[a][b+1]*2)+(imgmatrix[a+1][b+1]);
		else if (b==col)
			tempy=(imgmatrix[a][b-1]*-2)-(imgmatrix[a+1][b-1]);
		else
			tempy=(imgmatrix[a][b+1]*2)+(imgmatrix[a+1][b+1])-(imgmatrix[a][b-1]*2)-(imgmatrix[a+1][b-1]);
	}
	else if (a==row)
	{
		if (b==0)
			tempy=(imgmatrix[a-1][b+1])+(imgmatrix[a][b+1]*2);
		else if (b==col)
			tempy=-(imgmatrix[a-1][b-1])-(imgmatrix[a][b-1]*2);
		else
			tempy=(imgmatrix[a-1][b+1])+(imgmatrix[a][b+1]*2)-(imgmatrix[a-1][b-1])-(imgmatrix[a][b-1]*2);
	}
	else
	{
		if(b==0)
			tempy=(imgmatrix[a-1][b+1])+(imgmatrix[a][b+1]*2)+(imgmatrix[a+1][b+1]);
		else if (b==col)
			tempy=-(imgmatrix[a-1][b-1])-(imgmatrix[a][b-1]*2)-(imgmatrix[a+1][b-1]);
		else
			tempy=(imgmatrix[a-1][b+1])+(imgmatrix[a][b+1]*2)+(imgmatrix[a+1][b+1])-(imgmatrix[a-1][b-1])-(imgmatrix[a][b-1]*2)-(imgmatrix[a+1][b-1]);			
	}

	return tempy;
}





