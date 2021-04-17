#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MB 1000000
int main(){
        long long currSize = 1000000;
        int i=0;
        while(i++<100){
                char * ptr = (char *)malloc(currSize);
                memset(ptr,0,currSize);
                if(!ptr) printf("memory occupied");
                printf("Memory used %lld MB\n",currSize/MB);
                currSize = 2 * currSize;
        }
}
