#ifndef MAXBP_H_
#define MAXBP_H_

#include <iostream>
#include <cstring>

class maxbp{

	public:
			maxbp();
			~maxbp();
			maxbp(unsigned short int, unsigned long int, int);
			maxbp(const maxbp&);
			bool operator <(maxbp) const;
			bool operator ==(maxbp) const;
			bool operator >(maxbp) const;

			unsigned short int chrom;//2
			unsigned long int position;//8
			int score;
	
	private:
			
};

#endif /*MAXBP_H_*/