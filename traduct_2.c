//
//  traduct.c
//  
//
//  Created by Yves BAZIN on 04/03/2020.
//




#include "stdio.h"
#include <stdlib.h>
#include <string.h>



char** str_split(char* a_str, const char * a_delim)
{
    char** result    = 0;
    size_t count     = 0;
    char* tmp        = a_str;
    char* last_comma = 0;
    char delim;
    //delim = a_delim;

    count=20;
    
    result = malloc(sizeof(char*) * count);
    
    if (result)
    {
        size_t idx  = 0;
        char* token = strtok(a_str, a_delim);
        
        while (token)
        {
            //assert(idx < count);
            *(result + idx++) = strdup(token);
            token = strtok(0, a_delim);
        }
        //assert(idx == count - 1);
        *(result + idx) = 0;
    }
    
    return result;
}


int main(int argc,char* argv[])
{
    int num;
    FILE *fptr;
    int counter;
    char * line = NULL;
    int  duration[10000];
    unsigned char  address[10000];
   // unsigned char  address[10000];
    size_t len = 0;
    ssize_t read;
    char delim[] = "\", \"";
    
    //printf("Program Name Is: %s",argv[0]);
    if(argc==1)
        printf("\nNo Extra Command Line Argument Passed Other Than Program Name");
    if(argc>=2)
    {
        //printf("\nNumber Of Arguments Passed: %d",argc);
        //printf("\n----Following Are The Command Line Arguments Passed----");
        //for(counter=0;counter<argc;counter++)
          //  printf("\nargv[%d]: %s\n",counter,argv[counter]);
        
        fptr = fopen(argv[1],"r");
        
        if(fptr == NULL)
        {
            printf("Error!\n");
            exit(1);
        }
        else{
            read = getline(&line, &len, fptr);
            while ((read = getline(&line, &len, fptr)) != -1) {
                counter++;
                
            }
            printf("%d\n",counter);
           // return 0;
            fptr = fopen(argv[1],"r");
            read = getline(&line, &len, fptr);
            //printf("file found\n");
            while ((read = getline(&line, &len, fptr)) != -1) {
                //printf("Retrieved line of length %zu:\n", read);
               // printf("%s", line);
                
                
                char** tokens;
                
               
                
                tokens = str_split(line, "\", \"");
                
                if (tokens)
                {
                    //counter++;
                    /*
                    int i;
                    for (i = 0; *(tokens + i); i++)
                    {
                        printf("%s\n", *(tokens + i));
                        free(*(tokens + i));
                    }
                    printf("\n");
                    free(tokens);
                     */
                    int decal;
                    int ad;
                    int value;
                   //printf("line %s %s %s\n",*(tokens+1),*(tokens+2)+3,*(tokens+3)+1);
                   sscanf(*(tokens+1),"%d",&decal);
                   sscanf(*(tokens+2)+3,"%x",&ad);
                   sscanf(*(tokens+3)+1,"%x",&value);
                    printf("%c%c%c%c",ad,value,decal & 0xFF,(decal &0xFF00)>>8);
                    free(tokens);
                }
                
            }
            
            fclose(fptr);
        }
    }
    
    
    

    return 0;
}
