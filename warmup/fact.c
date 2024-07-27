#include "common.h"
int helper(int num){
    if(num == 1)
        return 1;
    return helper(num-1) * num;
}
int main(int argc, char **argv){
	/*int a;
    scanf("%d",a);*/
  // printf("%s", argv[1]);
    int firstDigit;
    int secondDigit = -1;
    char* s = argv[1];
    int size = 0;
    while(argv[1][size] != '\0'){
      size++;
    }
    //printf("%d",size);
    //printf("%d",s[0]);
    if(size == 0){
      printf("Huh?\n");
      return 0;
    }
    if( size > 2 ){
        printf("Huh?\n");
        return 0;    
    }else{
	if(size >= 1){
	  if(s[0] - 0 < 49 || s[0]-0>57){
		    printf("Huh?\n");
		    return 0;
	    }else{
	    firstDigit = s[0] - 48;	   
	    }
	}   
	if(size >= 2){
	  if(s[1] - 0 < 48 ||s[1]-0>57 ){
	   	printf("Huh?\n");
		return 0;
	   }else{
	   	secondDigit = s[1] - 48;
	   }
	}
    }
    int num;
    if(secondDigit == -1){
      num = firstDigit;
    }else{
      num = 10*firstDigit + secondDigit;
    }
    if(num > 12){
    	printf("Overflow\n");
	return 0;
    }else{
   	int result = helper(num);
   	printf("%d\n",result);
    }
    /*if(num < 0){
        printf("Huh?");
    }else if(num >= 12){
    else{
        int answer = helper(num);
        printf("%d", answer);
    }
    */
    return 0;
}
