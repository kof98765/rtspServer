#include <iostream>
#include <string>
#include "stdint.h"
extern "C"
{
#include "x264.h"
#include "x264_config.h"
};
using namespace std;
int main()
{
x264_param_t param;
x264_t* m_h = NULL;
x264_param_default(&param);
param.i_width=100;
param.i_height=100;
printf("???\n");
if( ( m_h = x264_encoder_open( &param ) ) == NULL )
	 {
		 fprintf( stderr, "x264 [error]: x264_encoder_open failed\n" );
		 
	 }
	 std::cout<<m_h<<"\n";
	 while(1);
return 0;
}
