//Unpublished Copyright (c) 2015 Qi Zheng, All Rights Reserved.
//This is a parser program that reads the output of GPGPU-sim, and records the active warp numbers when there is a GPU core stall. The output of the parser is a csv file that can be directly feed into Excel or Matlab.
//The output format is:
//  <Cycle count>,<#warp of core 0>,<#warp of core 1>,<#warp of core 2>,...

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <string>

using namespace std;

char *benchmark;
int SMnum;
int64_t alllines;
long long curcycle;
bool isstart;
map<int, int> buffer;

char* outputfile;
int kernelcount;
FILE* outfp;

void init(){
	alllines = 0;
        curcycle = 0;
        isstart = true;
        buffer.clear();
        for(int i=0;i<SMnum;i++){
          buffer[i] = -1;
        }
        kernelcount = 0;
        outfp = NULL;
}

void print_buffer(){
  //Print the stats
  fprintf(outfp,"%lld,",curcycle);
  assert(buffer.size() == SMnum);
  map<int, int>::iterator it = buffer.begin();
  for(int i = 0;i < SMnum-1;i++,it++){
    int temp = it->second;
    if(temp == -1) fprintf(outfp,"%d,",0);
    else fprintf(outfp,"%d,",temp);
  }
  int temp = buffer.rbegin()->second;
  if(temp == -1) fprintf(outfp,"%d\n",0);
  else fprintf(outfp,"%d\n",temp);
  
  //Clear
  buffer.clear();
  for(int i=0;i<SMnum;i++){
    buffer[i] = -1;
  }
}

int parse_line(char* input){
  const char* ref = "Core";
  char curhead[4];
  memcpy(curhead, input, 4);
  if(memcmp(ref, curhead,4)){
    return 0;
  }

  int sid, warpnum;
  long long cycle;
  sscanf(input,"%*s %d %*s %lld %*s %d",&sid, &cycle,&warpnum);
  assert(sid < SMnum);

  //Process trace
  if(cycle == curcycle){
    assert(buffer.find(sid)->second == -1);
    buffer[sid] = warpnum;
  }else{
    if(isstart){
      isstart = false;
      curcycle = cycle;
    }else{
      print_buffer();
      curcycle = cycle;
      assert(buffer.find(sid)->second == -1);
      buffer[sid] = warpnum;
    }
  }

  return 1;
}

int detect_new(char* input){
  const char* ref = "New kernel";
  char curhead[10];
  memcpy(curhead, input, 10);
  if(memcmp(ref, curhead,10)){
    return 0;
  }

  fclose(outfp);

  int warpsize;
  sscanf(input,"%*s %*s %*s %d",&warpsize);

  curcycle = 0;
  isstart = true;
  buffer.clear();
  for(int i=0;i<SMnum;i++){
    buffer[i] = -1;
  }
  outfp = NULL;

  char tmpstring[15];
  sprintf(tmpstring,"%d",kernelcount);
  outputfile = strcat(benchmark,"-kernel-");
  outputfile = strcat(outputfile,tmpstring);
  outputfile = strcat(outputfile,".csv");

  outfp = fopen(outputfile,"w");
  fprintf(outfp, "-1,");
  for(int i=0;i<SMnum-1;i++){
    fprintf(outfp,"%d,",warpsize);
  }
  fprintf(outfp,"%d\n",warpsize);
  
  kernelcount++;

  return 1;
}

void process_trace(){
        int i;
        char* line = NULL;
        size_t len = 0;
        ssize_t readlen;

        FILE* fp = fopen(benchmark,"r");
        if(fp == NULL){
            fprintf(stderr,"Cannot open %s\n", benchmark);
        }
       	while((readlen = getline(&line, &len, fp)) != -1){
		alllines++;
                if(detect_new(line)){
                  continue;
                }
                parse_line(line);
        }	

        if(line) free(line);
}

int main(int argc, char **argv){
        if(argc != 3){
                fprintf(stderr, "Usage: executable <GPGPU-sim output> <#SM>\n");
                exit(-1);
        }
	benchmark = argv[1];
        SMnum = atoi(argv[2]);
        if(SMnum <= 0){
          fprintf(stderr,"Wrong #SM!\n");
          assert(0);
        }

	init();	
	process_trace();
	return 0;
}

