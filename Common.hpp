#ifndef _COMMON_HPP
#define _COMMON_HPP

namespace Common{
	//Variables that will be used throughout the program by all threads

	const int noFEE64 = 24;			//Not just the number in use but the highest number ID that is used
	const int noDSSD = 6;			//Total number of DSSD you are implanting in
	const int noChannel = 64;		//Will remain fixed. Number of channels per FEE
	const int masterFEE64 = 6;
	const int offsetMSBFEE64[noFEE64]{	0,0,0,1,
										1,2,0,0,
										1,2,2,1,
										0,0,0,0,
										1,0,0,1,
										2,2,0,0};

	//Pointer used by ROOT to access output file
	TFile * fOutRoot;
}
#endif