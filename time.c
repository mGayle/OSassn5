#include <time.h>
#include <stdio.h>

int main()
{
	time_t file_time; 

	file_time = time( NULL ); 

	printf("Time as time_t: %d\n", file_time ); 
	printf("Time as string: %s\n", ctime( &file_time )); 

	//use time you stored off in file struct, 
	//ctime has end line on end. gonna need to declare new string, 
	//do string copy from return val of ctime. take off new line. 
	//you can use string length -1 from ctime and do string memcopy. 
	//need to iterate over dir struct and check that printing valid entries
	//using valid flag in dir array. 
	printf("%d  %s  %s\n", 65525, ctime( &file_time ), fileName );  

	return 0; 

}
