#ifndef DATAWINS_H_
#define DATAWINS_H_

#include <iostream>

class dataWins {
	
	public:
			dataWins();
			~dataWins();
			dataWins(unsigned short int, unsigned long int, unsigned long int,int,double,double,double,double);
			dataWins(const dataWins&);
			bool operator <(dataWins) const;

			unsigned short int chrom;//2
			unsigned long int start;//8
			unsigned long int stop;
			int eCount;
			double iCount;
			double gcPerc;
			double alignPerc;
			double cnvScore;
	
	private:
			
};

#endif /*DATAWINS_H_*/