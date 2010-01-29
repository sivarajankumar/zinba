#include <iostream>
#include <fstream>
#include <stdio.h>
#include "bcanalysis.h"
#include <sstream>
#include <string>
#include <cstring>
#include <stdexcept>
#include <math.h>
#include <iomanip>
#include <vector>
#include <cstdlib>
#include <ctime>

using namespace std;

bcanalysis::bcanalysis(){
	chromCounter = 0;
	tbSizeFlag = 0;
}

bcanalysis::~bcanalysis(){
	map<const char*, int>::iterator i;
	for(i=chroms.begin();i!=chroms.end();++i){
		delete [] i->first;	
	}
}

int bcanalysis::processSignals(const char* outputFile,int extend){

	unsigned short int currchr;int eraseInd;
	const char * chromReport;
	unsigned short int * basepair = NULL;
	unsigned short int printFLAG = 0;
//	unsigned long int aStart,aStop;
	unsigned int aStart,aStop;
	
	while(!signal_slist.empty()){
		eraseInd = 0;
		currchr = signal_slist[0].chrom;
		chromReport = getKey(currchr);
		cout << "\tProcessing " << chromReport << ".........." << endl;
		basepair = new unsigned short int[chr_size[currchr]+1];
		for(int in = chr_size[currchr]; in--;)
			basepair[in] = 0;
		
		for(int i = 0;i <= (int) signal_slist.size();i++){
			if(signal_slist[i].chrom==currchr){
				aStart = signal_slist[i].pos;
				aStop = chr_size[currchr];
				if((aStart+extend-1) <= chr_size[currchr])
					aStop = aStart + extend - 1;
//				for(unsigned long int rpos = aStart; rpos <= aStop;rpos++)
				for(unsigned int rpos = aStart; rpos <= aStop;rpos++)
					basepair[rpos]++;
			}else{
				if(outputData(outputFile,currchr,printFLAG,basepair) != 0)
					return 1;
				printFLAG = 1;
				delete [] basepair;
				basepair = NULL;
				
				eraseInd = i;
				i = (int) signal_slist.size();
			}
		}
		signal_slist.erase(signal_slist.begin(),signal_slist.begin()+eraseInd);
	}
	return 0;
}

int bcanalysis::outputData(const char * outputFile, unsigned short int currChr,unsigned short int pFLAG,unsigned short int basepair[]){
	FILE * fh;
	const char * chrom = getKey(currChr);
	if(pFLAG == 0){
		fh = fopen(outputFile,"w");
		fprintf(fh,"track type=wiggle_0 name=\"%s\" desc=\"%s\" visibility=full\n",outputFile,outputFile);
	}else{
		fh = fopen(outputFile,"a");
	}
	if(fh == NULL){
		cout << "\nERROR: Can't open output file: " << outputFile << endl;
		return 1;
	}
	fprintf(fh,"fixedStep chrom=%s start=1 step=1\n",chrom);

	for(int posP = 1; posP <= chr_size[currChr];posP++)
		fprintf(fh,"%hu\n",basepair[posP]);
	fclose (fh);
	return 0;
}

unsigned short int bcanalysis::getHashValue(char *currChrom){
	map<const char*, int>::iterator i;
	i = chroms.find(currChrom);
	if(i == chroms.end()){
		char * chromosome = new char[128];
		strcpy(chromosome,currChrom);
		chroms[chromosome] = chromCounter;
		intsToChrom[chromCounter] = chromosome;
		return(chromCounter++);
	}else{
		return i->second;	
	}
}

const char * bcanalysis::getKey(unsigned short int chrom){
	map<int, const char*>::iterator i;
	i = intsToChrom.find(chrom);
	if(i == intsToChrom.end()){
		cout << chrom << endl;
		cout << "REALLY REALLY BAD ERROR!" << endl;
		exit(1);
	}else{
		return i->second;	
	}
}

int bcanalysis::importRawSignal(const char * signalFile,int extension,const char * filetype,const char * twoBitFile){

	if(tbSizeFlag == 0){
		FILE * tempTB;
		time_t rtime;
		struct tm *timeinfo;
		
		char tInfo[128];// = "tempInfo.txt";
		char tChrSize[128];// = "tempChromSize.txt";
		char sysCall[256];
		time(&rtime);
		timeinfo=localtime(&rtime);
		strftime(tInfo,128,"tempInfo_%H_%M_%S.txt",timeinfo);
		strftime(tChrSize,128,"tempChromSize_%H_%M_%S.txt",timeinfo);
		
		tempTB = fopen(tInfo,"w");
		fprintf(tempTB,"library(zinba);\ntwobitinfo(infile=\"%s\",outfile=\"%s\");\n",twoBitFile,tChrSize);
		fclose (tempTB);
		
		cout << "\tGetting chromosome lengths from .2bit file: " << twoBitFile << endl;
		int s = 1;
		int twobitCount = 0;
		sprintf(sysCall,"R CMD BATCH %s /dev/null",tInfo);
		while(s != 0){
			s = system(sysCall);
			twobitCount++;
			if(twobitCount < 5 && s != 0){
				cout << "Trying twoBitInfo again, s is" << s << endl;
			}else if(twobitCount >= 5 && s != 0){
				cout << "TwoBitInfo failed, exiting" << endl;
				return 1;
			}
		}
		remove(tInfo);
		
		tempTB = fopen(tChrSize,"r");
		char cChrom[128];
//		unsigned long int cStart;
		unsigned long int cStart;
		while(!feof(tempTB)){
			int ret = fscanf(tempTB,"%s%lu",cChrom,&cStart);
			unsigned short int chromInt = getHashValue(cChrom);
			chr_size[chromInt] = cStart;
		}
		fclose(tempTB);
		remove(tChrSize);
		tbSizeFlag = 1;
	}
	
	const char * bowtie = "bowtie";
	const char * bed = "bed";
	const char * tagAlign = "tagAlign";
	int rval = 0;
	
	if(strcmp(filetype,bed) == 0){
		rval = importBed(signalFile,extension);
	}else if (strcmp(filetype,bowtie) == 0){
		rval = importBowtie(signalFile,extension);
	}else if(strcmp(filetype,tagAlign) == 0){
		rval = importTagAlign(signalFile,extension);
	}else{
		cout << "Unrecognized type of file " << filetype << ", must be either bowtie, bed, or tagAlign" << endl;
		return 1;
	}
	
	if(rval == 0){
		cout << "\tLoaded " << signal_slist.size() << " reads" << endl;
		cout << "\tSorting reads ...";
		sort (signal_slist.begin(), signal_slist.end());
//		signal_slist.sort();
	}else{
		cout << "Stopping basealigncounts" << endl;
		return 1;
	}
	return 0;
}

int bcanalysis::importBowtie(const char * signalFile,int extension){
	
	FILE * fh;
	fh = fopen(signalFile,"r");
	if(fh == NULL){
		cout << "ERROR: Unable to open file containing reads " << signalFile << endl;
		return 1;
	}
	
	char cChrom[128];
	unsigned int pos;
//	unsigned long int pos;
//	unsigned short int sval = 1;
	char strand[1];
	char minus[] = "-";
	char line[512];char seq[128];
	char name[128];char sscore[128];int ival;
	int rval;
	//	slist<bwRead>::iterator back =  signal_slist.previous(signal_slist.end());
	//	slist<bwRead>::iterator iback = input_slist.previous(input_slist.end());
	
	while(!feof(fh)){
//		rval = fscanf(fh,"%s%s%s%lu%s%s%i",name,strand,cChrom,&pos,seq,sscore,&ival);
		rval = fscanf(fh,"%s%s%s%u%s%s%i",name,strand,cChrom,&pos,seq,sscore,&ival);
		fgets(line,512,fh);
		if(rval == 7){
			if(strcmp(strand,minus) == 0){
				if((pos + strlen(seq)) >= extension)
					pos = (pos + strlen(seq)) - extension + 1;
				else
					pos = 1;
			}
			unsigned short int chromInt = getHashValue(cChrom);
//			bwRead sig(chromInt,pos,sval);
			bwRead sig(chromInt,pos);
			signal_slist.push_back(sig);
//			back = signal_slist.insert_after(back,sig);
		}
	}
	fclose(fh);
	return 0;
}

int bcanalysis::importTagAlign(const char * signalFile,int extension){
	
	FILE * fh;
	fh = fopen(signalFile,"r");
	if(fh == NULL){
		cout << "ERROR: Unable to open file containing reads " << signalFile << endl;
		return 1;
	}
	
	char cChrom[128];
	unsigned int pos;
//	unsigned long int pos;
//	unsigned short int sval = 1;
	char strand[1];
	char minus[] = "-";
//	unsigned long int start;unsigned long int stop;
	unsigned int start;unsigned int stop;
	char seq[128];int score;
	int rval;
	//	slist<bwRead>::iterator back =  signal_slist.previous(signal_slist.end());
	//	slist<bwRead>::iterator iback = input_slist.previous(input_slist.end());
	
	while(!feof(fh)){
//		rval = fscanf(fh,"%s%lu%lu%s%i%s",cChrom,&start,&stop,seq,&score,strand);
		rval = fscanf(fh,"%s%u%u%s%i%s",cChrom,&start,&stop,seq,&score,strand);
		if(rval == 6){
			if(strcmp(strand,minus) == 0){
				if(stop >= extension)
					pos = stop - extension + 1;
				else
					pos = 1;
			}
			unsigned short int chromInt = getHashValue(cChrom);
//			bwRead sig(chromInt,pos,sval);
			bwRead sig(chromInt,pos);
			signal_slist.push_back(sig);
//			back = signal_slist.insert_after(back,sig);
		}
	}
	fclose(fh);
	return 0;
}

int bcanalysis::importBed(const char * signalFile,int extension){
	
	FILE * fh;
	fh = fopen(signalFile,"r");
	if(fh == NULL){
		cout << "ERROR: Unable to open file containing reads " << signalFile << endl;
		return 1;
	}
	
	char cChrom[128];
	unsigned int pos;
//	unsigned long int pos;
	unsigned short int sval = 1;
	char strand[1];
	char minus[] = "-";
	unsigned int start;unsigned int stop;
//	unsigned long int start;unsigned long int stop;
	char name[128];int bscore;
	int rval;
	//	slist<bwRead>::iterator back =  signal_slist.previous(signal_slist.end());
	//	slist<bwRead>::iterator iback = input_slist.previous(input_slist.end());
	
	while(!feof(fh)){
//		rval = fscanf(fh,"%s%lu%lu%s%i%s",cChrom,&start,&stop,name,&bscore,strand);
		rval = fscanf(fh,"%s%u%u%s%i%s",cChrom,&start,&stop,name,&bscore,strand);
		if(rval == 6){
			if(strcmp(strand,minus) == 0){
				if(stop >= extension)
					pos = stop - extension + 1;
				else
					pos = 1;
			}
			unsigned short int chromInt = getHashValue(cChrom);
//			bwRead sig(chromInt,pos,sval);
			bwRead sig(chromInt,pos);
			signal_slist.push_back(sig);
//			back = signal_slist.insert_after(back,sig);
		}
	}
	fclose(fh);
	return 0;
}

