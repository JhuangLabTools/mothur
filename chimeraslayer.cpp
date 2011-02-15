/*
 *  chimeraslayer.cpp
 *  Mothur
 *
 *  Created by westcott on 9/25/09.
 *  Copyright 2009 Pschloss Lab. All rights reserved.
 *
 */

#include "chimeraslayer.h"
#include "chimerarealigner.h"
#include "kmerdb.hpp"
#include "blastdb.hpp"

//***************************************************************************************************************
ChimeraSlayer::ChimeraSlayer(string file, string temp, bool trim, string mode, int k, int ms, int mms, int win, float div, 
int minsim, int mincov, int minbs, int minsnp, int par, int it, int inc, int numw, bool r) : Chimera()  {  	
	try {
		fastafile = file;
		templateFileName = temp; templateSeqs = readSeqs(temp);
		searchMethod = mode;
		kmerSize = k;
		match = ms;
		misMatch = mms;
		window = win;
		divR = div;
		minSim = minsim;
		minCov = mincov;
		minBS = minbs;
		minSNP = minsnp;
		parents = par;
		iters = it;
		increment = inc;
		numWanted = numw;
		realign = r; 
		trimChimera = trim;
	
		decalc = new DeCalculator();	
		
		doPrep();
	}
	catch(exception& e) {
		m->errorOut(e, "ChimeraSlayer", "ChimeraSlayer");
		exit(1);
	}
}
//***************************************************************************************************************
ChimeraSlayer::ChimeraSlayer(string file, string temp, bool trim, string name, string mode, string abunds, int k, int ms, int mms, int win, float div, 
							 int minsim, int mincov, int minbs, int minsnp, int par, int it, int inc, int numw, bool r) : Chimera()  {  	
	try {
		fastafile = file; templateSeqs = readSeqs(fastafile);
		templateFileName = temp; 
		searchMethod = mode;
		kmerSize = k;
		match = ms;
		misMatch = mms;
		window = win;
		divR = div;
		minSim = minsim;
		minCov = mincov;
		minBS = minbs;
		minSNP = minsnp;
		parents = par;
		iters = it;
		increment = inc;
		numWanted = numw;
		realign = r; 
		includeAbunds = abunds;
		trimChimera = trim;
		
		//read name file and create nameMapRank
		readNameFile(name);
		
		decalc = new DeCalculator();	
		
		createFilter(templateSeqs, 0.0); //just removed columns where all seqs have a gap
		
		//run filter on template
		for (int i = 0; i < templateSeqs.size(); i++) { runFilter(templateSeqs[i]);  }
		
	}
	catch(exception& e) {
		m->errorOut(e, "ChimeraSlayer", "ChimeraSlayer");
		exit(1);
	}
}
//***************************************************************************************************************
int ChimeraSlayer::readNameFile(string name) {
	try {
		ifstream in;
		m->openInputFile(name, in);
		
		int maxRank = 0;
		int minRank = 10000000;
		
		while(!in.eof()){
			
			if (m->control_pressed) { in.close(); return 0; }
			
			string thisname, repnames;
			
			in >> thisname;		m->gobble(in);		//read from first column
			in >> repnames;			//read from second column
			
			map<string, vector<string> >::iterator it = nameMapRank.find(thisname);
			if (it == nameMapRank.end()) {
				
				vector<string> splitRepNames;
				m->splitAtComma(repnames, splitRepNames);
				
				nameMapRank[thisname] = splitRepNames;	
				
				if (splitRepNames.size() > maxRank) { maxRank = splitRepNames.size(); }
				if (splitRepNames.size() < minRank) { minRank = splitRepNames.size(); }
				
			}else{	m->mothurOut(thisname + " is already in namesfile. I will use first definition."); m->mothurOutEndLine();  }
			
			m->gobble(in);
		}
		in.close();	
		
		//sanity check to make sure files match
		for (int i = 0; i < templateSeqs.size(); i++) {
			map<string, vector<string> >::iterator it = nameMapRank.find(templateSeqs[i]->getName());
			
			if (it == nameMapRank.end()) { m->mothurOut("[ERROR]: " + templateSeqs[i]->getName() + " is not in namesfile, but is in fastafile. Every name in fasta file must be in first column of names file."); m->mothurOutEndLine(); m->control_pressed = true;  }
		}
		
		if (maxRank == minRank) { m->mothurOut("[ERROR]: all sequences in namesfile have the same abundance, aborting."); m->mothurOutEndLine(); m->control_pressed = true;  }
		
		return 0;
		
	}
	catch(exception& e) {
		m->errorOut(e, "ChimeraSlayer", "readNameFile");
		exit(1);
	}
}

//***************************************************************************************************************
int ChimeraSlayer::doPrep() {
	try {
		
		//read in all query seqs
		vector<Sequence*> tempQuerySeqs = readSeqs(fastafile);
		
		vector<Sequence*> temp = templateSeqs;
		for (int i = 0; i < tempQuerySeqs.size(); i++) {  temp.push_back(tempQuerySeqs[i]);  }
		
		createFilter(temp, 0.0); //just removed columns where all seqs have a gap
		
		for (int i = 0; i < tempQuerySeqs.size(); i++) { delete tempQuerySeqs[i];  }
		
		if (m->control_pressed) {  return 0; } 
		
		//run filter on template
		for (int i = 0; i < templateSeqs.size(); i++) {  if (m->control_pressed) {  return 0; }  runFilter(templateSeqs[i]);  }
		
		string 	kmerDBNameLeft;
		string 	kmerDBNameRight;
	
		//generate the kmerdb to pass to maligner
		if (searchMethod == "kmer") { 
			string templatePath = m->hasPath(templateFileName);
			string rightTemplateFileName = templatePath + "right." + m->getRootName(m->getSimpleName(templateFileName));
			databaseRight = new KmerDB(rightTemplateFileName, kmerSize);
				
			string leftTemplateFileName = templatePath + "left." + m->getRootName(m->getSimpleName(templateFileName));
			databaseLeft = new KmerDB(leftTemplateFileName, kmerSize);	
		#ifdef USE_MPI
			for (int i = 0; i < templateSeqs.size(); i++) {
					
				if (m->control_pressed) { return 0; } 
					
				string leftFrag = templateSeqs[i]->getUnaligned();
				leftFrag = leftFrag.substr(0, int(leftFrag.length() * 0.33));
					
				Sequence leftTemp(templateSeqs[i]->getName(), leftFrag);
				databaseLeft->addSequence(leftTemp);	
			}
			databaseLeft->generateDB();
			databaseLeft->setNumSeqs(templateSeqs.size());
			
			for (int i = 0; i < templateSeqs.size(); i++) {
				if (m->control_pressed) { return 0; } 
					
				string rightFrag = templateSeqs[i]->getUnaligned();
				rightFrag = rightFrag.substr(int(rightFrag.length() * 0.66));
					
				Sequence rightTemp(templateSeqs[i]->getName(), rightFrag);
				databaseRight->addSequence(rightTemp);	
			}
			databaseRight->generateDB();
			databaseRight->setNumSeqs(templateSeqs.size());

		#else	
			//leftside
			kmerDBNameLeft = leftTemplateFileName.substr(0,leftTemplateFileName.find_last_of(".")+1) + char('0'+ kmerSize) + "mer";
			ifstream kmerFileTestLeft(kmerDBNameLeft.c_str());
			bool needToGenerateLeft = true;
			
			if(kmerFileTestLeft){	
				bool GoodFile = m->checkReleaseVersion(kmerFileTestLeft, m->getVersion());
				if (GoodFile) {  needToGenerateLeft = false;	}
			}
			
			if(needToGenerateLeft){	
			
				for (int i = 0; i < templateSeqs.size(); i++) {
					
					if (m->control_pressed) { return 0; } 
					
					string leftFrag = templateSeqs[i]->getUnaligned();
					leftFrag = leftFrag.substr(0, int(leftFrag.length() * 0.33));
					
					Sequence leftTemp(templateSeqs[i]->getName(), leftFrag);
					databaseLeft->addSequence(leftTemp);	
				}
				databaseLeft->generateDB();
				
			}else {	
				databaseLeft->readKmerDB(kmerFileTestLeft);	
			}
			kmerFileTestLeft.close();
			
			databaseLeft->setNumSeqs(templateSeqs.size());
			
			//rightside
			kmerDBNameRight = rightTemplateFileName.substr(0,rightTemplateFileName.find_last_of(".")+1) + char('0'+ kmerSize) + "mer";
			ifstream kmerFileTestRight(kmerDBNameRight.c_str());
			bool needToGenerateRight = true;
			
			if(kmerFileTestRight){	
				bool GoodFile = m->checkReleaseVersion(kmerFileTestRight, m->getVersion());
				if (GoodFile) {  needToGenerateRight = false;	}
			}
			
			if(needToGenerateRight){	
			
				for (int i = 0; i < templateSeqs.size(); i++) {
					if (m->control_pressed) { return 0; } 
					
					string rightFrag = templateSeqs[i]->getUnaligned();
					rightFrag = rightFrag.substr(int(rightFrag.length() * 0.66));
					
					Sequence rightTemp(templateSeqs[i]->getName(), rightFrag);
					databaseRight->addSequence(rightTemp);	
				}
				databaseRight->generateDB();
				
			}else {	
				databaseRight->readKmerDB(kmerFileTestRight);	
			}
			kmerFileTestRight.close();
			
			databaseRight->setNumSeqs(templateSeqs.size());
		#endif	
		}else if (searchMethod == "blast") {
		
			//generate blastdb
			databaseLeft = new BlastDB(-2.0, -1.0, match, misMatch);
			for (int i = 0; i < templateSeqs.size(); i++) { 	databaseLeft->addSequence(*templateSeqs[i]);	}
			databaseLeft->generateDB();
			databaseLeft->setNumSeqs(templateSeqs.size());
		}
		
		return 0;

	}
	catch(exception& e) {
		m->errorOut(e, "ChimeraSlayer", "doprep");
		exit(1);
	}
}
//***************************************************************************************************************
vector<Sequence*> ChimeraSlayer::getTemplate(Sequence* q) {
	try {
		
		vector<Sequence*> thisTemplate;
		
		int thisRank;
		string thisName = q->getName();
		map<string, vector<string> >::iterator itRank = nameMapRank.find(thisName); // you will find it because we already sanity checked
		thisRank = (itRank->second).size();
		
		//create list of names we want to put into the template
		set<string> namesToAdd;
		for (itRank = nameMapRank.begin(); itRank != nameMapRank.end(); itRank++) {
			if (itRank->first != thisName) {
				if (includeAbunds == "greaterequal") {
					if ((itRank->second).size() >= thisRank) {
						//you are more abundant than me or equal to my abundance
						for (int i = 0; i < (itRank->second).size(); i++) {
							namesToAdd.insert((itRank->second)[i]);
						}
					}
				}else if (includeAbunds == "greater") {
					if ((itRank->second).size() > thisRank) {
						//you are more abundant than me
						for (int i = 0; i < (itRank->second).size(); i++) {
							namesToAdd.insert((itRank->second)[i]);
						}
					}
				}else if (includeAbunds == "all") {
					//add everyone
					for (int i = 0; i < (itRank->second).size(); i++) {
						namesToAdd.insert((itRank->second)[i]);
					}
				}
			}
		}
		
		for (int i = 0; i < templateSeqs.size(); i++) {  
			if (namesToAdd.count(templateSeqs[i]->getName()) != 0) { 
				thisTemplate.push_back(templateSeqs[i]);
			}
		}
		
		string 	kmerDBNameLeft;
		string 	kmerDBNameRight;
		
		//generate the kmerdb to pass to maligner
		if (searchMethod == "kmer") { 
			string templatePath = m->hasPath(templateFileName);
			string rightTemplateFileName = templatePath + "right." + m->getRootName(m->getSimpleName(templateFileName));
			databaseRight = new KmerDB(rightTemplateFileName, kmerSize);
			
			string leftTemplateFileName = templatePath + "left." + m->getRootName(m->getSimpleName(templateFileName));
			databaseLeft = new KmerDB(leftTemplateFileName, kmerSize);	
#ifdef USE_MPI
			for (int i = 0; i < thisTemplate.size(); i++) {
				
				if (m->control_pressed) { return thisTemplate; } 
				
				string leftFrag = thisTemplate[i]->getUnaligned();
				leftFrag = leftFrag.substr(0, int(leftFrag.length() * 0.33));
				
				Sequence leftTemp(thisTemplate[i]->getName(), leftFrag);
				databaseLeft->addSequence(leftTemp);	
			}
			databaseLeft->generateDB();
			databaseLeft->setNumSeqs(thisTemplate.size());
			
			for (int i = 0; i < thisTemplate.size(); i++) {
				if (m->control_pressed) { return thisTemplate;  } 
				
				string rightFrag = thisTemplate[i]->getUnaligned();
				rightFrag = rightFrag.substr(int(rightFrag.length() * 0.66));
				
				Sequence rightTemp(thisTemplate[i]->getName(), rightFrag);
				databaseRight->addSequence(rightTemp);	
			}
			databaseRight->generateDB();
			databaseRight->setNumSeqs(thisTemplate.size());
			
#else	
			
			
			for (int i = 0; i < thisTemplate.size(); i++) {
				
				if (m->control_pressed) { return thisTemplate; } 
				
				string leftFrag = thisTemplate[i]->getUnaligned();
				leftFrag = leftFrag.substr(0, int(leftFrag.length() * 0.33));
				
				Sequence leftTemp(thisTemplate[i]->getName(), leftFrag);
				databaseLeft->addSequence(leftTemp);	
			}
			databaseLeft->generateDB();
			databaseLeft->setNumSeqs(thisTemplate.size());
				
			for (int i = 0; i < thisTemplate.size(); i++) {
				if (m->control_pressed) { return thisTemplate; } 
					
				string rightFrag = thisTemplate[i]->getUnaligned();
				rightFrag = rightFrag.substr(int(rightFrag.length() * 0.66));
					
				Sequence rightTemp(thisTemplate[i]->getName(), rightFrag);
				databaseRight->addSequence(rightTemp);	
			}
			databaseRight->generateDB();
			databaseRight->setNumSeqs(thisTemplate.size());
#endif	
		}else if (searchMethod == "blast") {
			
			//generate blastdb
			databaseLeft = new BlastDB(-2.0, -1.0, match, misMatch);
			for (int i = 0; i < thisTemplate.size(); i++) { if (m->control_pressed) { return thisTemplate; }  databaseLeft->addSequence(*thisTemplate[i]);	}
			databaseLeft->generateDB();
			databaseLeft->setNumSeqs(thisTemplate.size());
		}
		
		return thisTemplate;
		
	}
	catch(exception& e) {
		m->errorOut(e, "ChimeraSlayer", "getTemplate");
		exit(1);
	}
}

//***************************************************************************************************************
ChimeraSlayer::~ChimeraSlayer() { 	
	delete decalc;  
	if (templateFileName != "self") {
		if (searchMethod == "kmer") {  delete databaseRight;  delete databaseLeft;  }	
		else if (searchMethod == "blast") {  delete databaseLeft; }
	}
}
//***************************************************************************************************************
void ChimeraSlayer::printHeader(ostream& out) {
	m->mothurOutEndLine();
	m->mothurOut("Only reporting sequence supported by " + toString(minBS) + "% of bootstrapped results.");
	m->mothurOutEndLine();
	
	out << "Name\tLeftParent\tRightParent\tDivQLAQRB\tPerIDQLAQRB\tBootStrapA\tDivQLBQRA\tPerIDQLBQRA\tBootStrapB\tFlag\tLeftWindow\tRightWindow\n";
}
//***************************************************************************************************************
Sequence* ChimeraSlayer::print(ostream& out, ostream& outAcc) {
	try {
		Sequence* trim = NULL;
		if (trimChimera) { trim = trimQuery; }
		
		if (chimeraFlags == "yes") {
			string chimeraFlag = "no";
			if(  (chimeraResults[0].bsa >= minBS && chimeraResults[0].divr_qla_qrb >= divR)
			   ||
			   (chimeraResults[0].bsb >= minBS && chimeraResults[0].divr_qlb_qra >= divR) ) { chimeraFlag = "yes"; }
			
			
			if (chimeraFlag == "yes") {	
				if ((chimeraResults[0].bsa >= minBS) || (chimeraResults[0].bsb >= minBS)) {
					m->mothurOut(querySeq->getName() + "\tyes"); m->mothurOutEndLine();
					outAcc << querySeq->getName() << endl;
					
					if (trimChimera) {  
						int lengthLeft = spotMap[chimeraResults[0].winLEnd] - spotMap[chimeraResults[0].winLStart];
						int lengthRight = spotMap[chimeraResults[0].winREnd] - spotMap[chimeraResults[0].winRStart];
						
						string newAligned = trim->getAligned();

						if (lengthLeft > lengthRight) { //trim right
							for (int i = (spotMap[chimeraResults[0].winRStart]-1); i < newAligned.length(); i++) { newAligned[i] = '.'; }
						}else { //trim left
							for (int i = 0; i < spotMap[chimeraResults[0].winLEnd]; i++) { newAligned[i] = '.'; }
						}
						trim->setAligned(newAligned);
					}
						
				}
			}
			
			printBlock(chimeraResults[0], chimeraFlag, out);
			out << endl;
		}else {  out << querySeq->getName() << "\tno" << endl;  }
		
		return trim;
		
	}
	catch(exception& e) {
		m->errorOut(e, "ChimeraSlayer", "print");
		exit(1);
	}
}
//***************************************************************************************************************
Sequence* ChimeraSlayer::print(ostream& out, ostream& outAcc, data_results leftPiece, data_results rightPiece) {
	try {
		Sequence* trim = NULL;
				
		if (trimChimera) { 
			string aligned = leftPiece.trimQuery.getAligned() + rightPiece.trimQuery.getAligned();
			trim = new Sequence(leftPiece.trimQuery.getName(), aligned); 
		}
		
		if ((leftPiece.flag == "yes") || (rightPiece.flag == "yes")) {
			
			string chimeraFlag = "no";
			if (leftPiece.flag == "yes") {
				
				if(  (leftPiece.results[0].bsa >= minBS && leftPiece.results[0].divr_qla_qrb >= divR)
					||
					(leftPiece.results[0].bsb >= minBS && leftPiece.results[0].divr_qlb_qra >= divR) ) { chimeraFlag = "yes"; }
			}
			
			if (rightPiece.flag == "yes") {
				if ( (rightPiece.results[0].bsa >= minBS && rightPiece.results[0].divr_qla_qrb >= divR)
				 ||
				 (rightPiece.results[0].bsb >= minBS && rightPiece.results[0].divr_qlb_qra >= divR) ) { chimeraFlag = "yes"; }
			}
			
			bool rightChimeric = false;
			bool leftChimeric = false;
			
			if (chimeraFlag == "yes") {	
				//which peice is chimeric or are both
				if (rightPiece.flag == "yes") { if ((rightPiece.results[0].bsa >= minBS) || (rightPiece.results[0].bsb >= minBS)) { rightChimeric = true; } }
				if (leftPiece.flag == "yes") { if ((leftPiece.results[0].bsa >= minBS) || (leftPiece.results[0].bsb >= minBS))	{ leftChimeric = true;	} }
				
				if (rightChimeric || leftChimeric) {
					m->mothurOut(querySeq->getName() + "\tyes"); m->mothurOutEndLine();
					outAcc << querySeq->getName() << endl;
					
					if (trimChimera) {  
						string newAligned = trim->getAligned();
												
						//right side is fine so keep that
						if ((leftChimeric) && (!rightChimeric)) {
							for (int i = 0; i < leftPiece.spotMap[leftPiece.results[0].winREnd]; i++) { newAligned[i] = '.'; } 
						}else if ((!leftChimeric) && (rightChimeric)) { //leftside is fine so keep that
							for (int i = (rightPiece.spotMap[rightPiece.results[0].winLStart]-1); i < newAligned.length(); i++) { newAligned[i] = '.'; }
						}else { //both sides are chimeric, keep longest piece
							
							int lengthLeftLeft = leftPiece.spotMap[leftPiece.results[0].winLEnd] - leftPiece.spotMap[leftPiece.results[0].winLStart];
							int lengthLeftRight = leftPiece.spotMap[leftPiece.results[0].winREnd] - leftPiece.spotMap[leftPiece.results[0].winRStart];
							
							int longest = 1; // leftleft = 1, leftright = 2, rightleft = 3 rightright = 4
							int length = lengthLeftLeft;
							if (lengthLeftLeft < lengthLeftRight) { longest = 2;  length = lengthLeftRight; }
							
							int lengthRightLeft = rightPiece.spotMap[rightPiece.results[0].winLEnd] - rightPiece.spotMap[rightPiece.results[0].winLStart];
							int lengthRightRight = rightPiece.spotMap[rightPiece.results[0].winREnd] - rightPiece.spotMap[rightPiece.results[0].winRStart];
							
							if (lengthRightLeft > length) { longest = 3; length = lengthRightLeft;  }
							if (lengthRightRight > length) { longest = 4; }
							
							if (longest == 1) { //leftleft
								for (int i = (leftPiece.spotMap[leftPiece.results[0].winRStart]-1); i < newAligned.length(); i++) { newAligned[i] = '.'; }
							}else if (longest == 2) { //leftright
								//get rid of leftleft
								for (int i = (leftPiece.spotMap[leftPiece.results[0].winLStart]-1); i < (leftPiece.spotMap[leftPiece.results[0].winLEnd]-1); i++) { newAligned[i] = '.'; }
								//get rid of right
								for (int i = (rightPiece.spotMap[rightPiece.results[0].winLStart]-1); i < newAligned.length(); i++) { newAligned[i] = '.'; }
							}else if (longest == 3) { //rightleft
								//get rid of left
								for (int i = 0; i < leftPiece.spotMap[leftPiece.results[0].winREnd]; i++) { newAligned[i] = '.'; } 
								//get rid of rightright
								for (int i = (rightPiece.spotMap[rightPiece.results[0].winRStart]-1); i < newAligned.length(); i++) { newAligned[i] = '.'; }
							}else { //rightright
								//get rid of left
								for (int i = 0; i < leftPiece.spotMap[leftPiece.results[0].winREnd]; i++) { newAligned[i] = '.'; } 
								//get rid of rightleft
								for (int i = (rightPiece.spotMap[rightPiece.results[0].winLStart]-1); i < (rightPiece.spotMap[rightPiece.results[0].winLEnd]-1); i++) { newAligned[i] = '.'; }
							}
						}
							
						trim->setAligned(newAligned);
					}
					
				}
			}
			
			printBlock(leftPiece, rightPiece, leftChimeric, rightChimeric, chimeraFlag, out);
			out << endl;
		}else {  out << querySeq->getName() << "\tno" << endl;  }
		
		return trim;
		
	}
	catch(exception& e) {
		m->errorOut(e, "ChimeraSlayer", "print");
		exit(1);
	}
}

#ifdef USE_MPI
//***************************************************************************************************************
Sequence* ChimeraSlayer::print(MPI_File& out, MPI_File& outAcc, data_results leftPiece, data_results rightPiece) {
	try {
		MPI_Status status;
		bool results = false;
		string outAccString = "";
		string outputString = "";
		
		Sequence* trim = NULL;
		
		if (trimChimera) { 
			string aligned = leftPiece.trimQuery.getAligned() + rightPiece.trimQuery.getAligned();
			trim = new Sequence(leftPiece.trimQuery.getName(), aligned); 
		}
		
		
		if ((leftPiece.flag == "yes") || (rightPiece.flag == "yes")) {
			
			string chimeraFlag = "no";
			if (leftPiece.flag == "yes") {
				
				if(  (leftPiece.results[0].bsa >= minBS && leftPiece.results[0].divr_qla_qrb >= divR)
				   ||
				   (leftPiece.results[0].bsb >= minBS && leftPiece.results[0].divr_qlb_qra >= divR) ) { chimeraFlag = "yes"; }
			}
			
			if (rightPiece.flag == "yes") {
				if ( (rightPiece.results[0].bsa >= minBS && rightPiece.results[0].divr_qla_qrb >= divR)
					||
					(rightPiece.results[0].bsb >= minBS && rightPiece.results[0].divr_qlb_qra >= divR) ) { chimeraFlag = "yes"; }
			}
			
			bool rightChimeric = false;
			bool leftChimeric = false;
			
			if (chimeraFlag == "yes") {	
				//which peice is chimeric or are both
				if (rightPiece.flag == "yes") { if ((rightPiece.results[0].bsa >= minBS) || (rightPiece.results[0].bsb >= minBS)) { rightChimeric = true; } }
				if (leftPiece.flag == "yes") { if ((leftPiece.results[0].bsa >= minBS) || (leftPiece.results[0].bsb >= minBS))	{ leftChimeric = true;	} }
				
				if (rightChimeric || leftChimeric) {
					cout << querySeq->getName() <<  "\tyes" << endl;
					outAccString += querySeq->getName() + "\n";
					results = true;
					
					//write to accnos file
					int length = outAccString.length();
					char* buf2 = new char[length];
					memcpy(buf2, outAccString.c_str(), length);
				
					MPI_File_write_shared(outAcc, buf2, length, MPI_CHAR, &status);
					delete buf2;
					
					if (trimChimera) {  
						string newAligned = trim->getAligned();
						
						//right side is fine so keep that
						if ((leftChimeric) && (!rightChimeric)) {
							for (int i = 0; i < leftPiece.spotMap[leftPiece.results[0].winREnd]; i++) { newAligned[i] = '.'; } 
						}else if ((!leftChimeric) && (rightChimeric)) { //leftside is fine so keep that
							for (int i = (rightPiece.spotMap[rightPiece.results[0].winLStart]-1); i < newAligned.length(); i++) { newAligned[i] = '.'; }
						}else { //both sides are chimeric, keep longest piece
							
							int lengthLeftLeft = leftPiece.spotMap[leftPiece.results[0].winLEnd] - leftPiece.spotMap[leftPiece.results[0].winLStart];
							int lengthLeftRight = leftPiece.spotMap[leftPiece.results[0].winREnd] - leftPiece.spotMap[leftPiece.results[0].winRStart];
							
							int longest = 1; // leftleft = 1, leftright = 2, rightleft = 3 rightright = 4
							int length = lengthLeftLeft;
							if (lengthLeftLeft < lengthLeftRight) { longest = 2;  length = lengthLeftRight; }
							
							int lengthRightLeft = rightPiece.spotMap[rightPiece.results[0].winLEnd] - rightPiece.spotMap[rightPiece.results[0].winLStart];
							int lengthRightRight = rightPiece.spotMap[rightPiece.results[0].winREnd] - rightPiece.spotMap[rightPiece.results[0].winRStart];
							
							if (lengthRightLeft > length) { longest = 3; length = lengthRightLeft;  }
							if (lengthRightRight > length) { longest = 4; }
							
							if (longest == 1) { //leftleft
								for (int i = (leftPiece.spotMap[leftPiece.results[0].winRStart]-1); i < newAligned.length(); i++) { newAligned[i] = '.'; }
							}else if (longest == 2) { //leftright
								//get rid of leftleft
								for (int i = (leftPiece.spotMap[leftPiece.results[0].winLStart]-1); i < (leftPiece.spotMap[leftPiece.results[0].winLEnd]-1); i++) { newAligned[i] = '.'; }
								//get rid of right
								for (int i = (rightPiece.spotMap[rightPiece.results[0].winLStart]-1); i < newAligned.length(); i++) { newAligned[i] = '.'; }
							}else if (longest == 3) { //rightleft
								//get rid of left
								for (int i = 0; i < leftPiece.spotMap[leftPiece.results[0].winREnd]; i++) { newAligned[i] = '.'; } 
								//get rid of rightright
								for (int i = (rightPiece.spotMap[rightPiece.results[0].winRStart]-1); i < newAligned.length(); i++) { newAligned[i] = '.'; }
							}else { //rightright
								//get rid of left
								for (int i = 0; i < leftPiece.spotMap[leftPiece.results[0].winREnd]; i++) { newAligned[i] = '.'; } 
								//get rid of rightleft
								for (int i = (rightPiece.spotMap[rightPiece.results[0].winLStart]-1); i < (rightPiece.spotMap[rightPiece.results[0].winLEnd]-1); i++) { newAligned[i] = '.'; }
							}
						}
						
						trim->setAligned(newAligned);
					}
					
				}
			}
			
			outputString = getBlock(leftPiece, rightPiece, leftChimeric, rightChimeric, chimeraFlag);
			outputString += "\n";
		
			//write to output file
			int length = outputString.length();
			char* buf = new char[length];
			memcpy(buf, outputString.c_str(), length);
				
			MPI_File_write_shared(out, buf, length, MPI_CHAR, &status);
			delete buf;

		}else {  
			outputString += querySeq->getName() + "\tno\n";  
	
			//write to output file
			int length = outputString.length();
			char* buf = new char[length];
			memcpy(buf, outputString.c_str(), length);
				
			MPI_File_write_shared(out, buf, length, MPI_CHAR, &status);
			delete buf;
		}
		
		
		return trim;
	}
	catch(exception& e) {
		m->errorOut(e, "ChimeraSlayer", "print");
		exit(1);
	}
}
//***************************************************************************************************************
Sequence* ChimeraSlayer::print(MPI_File& out, MPI_File& outAcc) {
	try {
		MPI_Status status;
		bool results = false;
		string outAccString = "";
		string outputString = "";
		
		Sequence* trim = NULL;
		if (trimChimera) { trim = trimQuery; }
		
		if (chimeraFlags == "yes") {
			string chimeraFlag = "no";
			if(  (chimeraResults[0].bsa >= minBS && chimeraResults[0].divr_qla_qrb >= divR)
			   ||
			   (chimeraResults[0].bsb >= minBS && chimeraResults[0].divr_qlb_qra >= divR) ) { chimeraFlag = "yes"; }
			
			
			if (chimeraFlag == "yes") {	
				if ((chimeraResults[0].bsa >= minBS) || (chimeraResults[0].bsb >= minBS)) {
					cout << querySeq->getName() <<  "\tyes" << endl;
					outAccString += querySeq->getName() + "\n";
					results = true;
					
					//write to accnos file
					int length = outAccString.length();
					char* buf2 = new char[length];
					memcpy(buf2, outAccString.c_str(), length);
					
					MPI_File_write_shared(outAcc, buf2, length, MPI_CHAR, &status);
					delete buf2;
					
					if (trimChimera) {  
						int lengthLeft = spotMap[chimeraResults[0].winLEnd] - spotMap[chimeraResults[0].winLStart];
						int lengthRight = spotMap[chimeraResults[0].winREnd] - spotMap[chimeraResults[0].winRStart];
						
						string newAligned = trim->getAligned();
						if (lengthLeft > lengthRight) { //trim right
							for (int i = (spotMap[chimeraResults[0].winRStart]-1); i < newAligned.length(); i++) { newAligned[i] = '.'; }
						}else { //trim left
							for (int i = 0; i < (spotMap[chimeraResults[0].winLEnd]-1); i++) { newAligned[i] = '.'; }
						}
						trim->setAligned(newAligned);	
					}
				}
			}
			
			outputString = getBlock(chimeraResults[0], chimeraFlag);
			outputString += "\n";
			
			//write to output file
			int length = outputString.length();
			char* buf = new char[length];
			memcpy(buf, outputString.c_str(), length);
			
			MPI_File_write_shared(out, buf, length, MPI_CHAR, &status);
			delete buf;
			
		}else {  
			outputString += querySeq->getName() + "\tno\n";  
			
			//write to output file
			int length = outputString.length();
			char* buf = new char[length];
			memcpy(buf, outputString.c_str(), length);
			
			MPI_File_write_shared(out, buf, length, MPI_CHAR, &status);
			delete buf;
		}
		
		return trim;
	}
	catch(exception& e) {
		m->errorOut(e, "ChimeraSlayer", "print");
		exit(1);
	}
}
#endif

//***************************************************************************************************************
int ChimeraSlayer::getChimeras(Sequence* query) {
	try {
		if (trimChimera) { trimQuery = new Sequence(query->getName(), query->getAligned()); printResults.trimQuery = *trimQuery; }
		
		chimeraFlags = "no";
		printResults.flag = "no";

		//filter query
		spotMap = runFilter(query);	
		printResults.spotMap = spotMap;
		
		querySeq = query;
		
		//you must create a template
		vector<Sequence*> thisTemplate;
		if (templateFileName != "self") { thisTemplate = templateSeqs; }
		else { thisTemplate = getTemplate(query); } //fills thistemplate and creates the databases
		
		if (m->control_pressed) {  return 0;  }
		
		if (thisTemplate.size() == 0) {  return 0; } //not chimeric
		
		//referenceSeqs, numWanted, matchScore, misMatchPenalty, divR, minSimilarity
		Maligner maligner(thisTemplate, numWanted, match, misMatch, divR, minSim, minCov, searchMethod, databaseLeft, databaseRight);
		Slayer slayer(window, increment, minSim, divR, iters, minSNP);
		
		if (templateFileName == "self") {
			if (searchMethod == "kmer") {  delete databaseRight;  delete databaseLeft;  }	
			else if (searchMethod == "blast") {  delete databaseLeft; }
		}
	
		if (m->control_pressed) {  return 0;  }

		string chimeraFlag = maligner.getResults(query, decalc);
		if (m->control_pressed) {  return 0;  }
		vector<results> Results = maligner.getOutput();
	
		//found in testing realigning only made things worse
		if (realign) {
			ChimeraReAligner realigner(thisTemplate, match, misMatch);
			realigner.reAlign(query, Results);
		}

		if (chimeraFlag == "yes") {
			
			//get sequence that were given from maligner results
			vector<SeqDist> seqs;
			map<string, float> removeDups;
			map<string, float>::iterator itDup;
			map<string, string> parentNameSeq;
			map<string, string>::iterator itSeq;
			for (int j = 0; j < Results.size(); j++) {
				float dist = (Results[j].regionEnd - Results[j].regionStart + 1) * Results[j].queryToParentLocal;
				//only add if you are not a duplicate
				itDup = removeDups.find(Results[j].parent);
				if (itDup == removeDups.end()) { //this is not duplicate
					removeDups[Results[j].parent] = dist;
					parentNameSeq[Results[j].parent] = Results[j].parentAligned;
				}else if (dist > itDup->second) { //is this a stronger number for this parent
					removeDups[Results[j].parent] = dist;
					parentNameSeq[Results[j].parent] = Results[j].parentAligned;
				}
			}
			
			for (itDup = removeDups.begin(); itDup != removeDups.end(); itDup++) {
				itSeq = parentNameSeq.find(itDup->first);
				Sequence* seq = new Sequence(itDup->first, itSeq->second);
				
				SeqDist member;
				member.seq = seq;
				member.dist = itDup->second;
				
				seqs.push_back(member);
			}
			
			//limit number of parents to explore - default 3
			if (Results.size() > parents) {
				//sort by distance
				sort(seqs.begin(), seqs.end(), compareSeqDist);
				//prioritize larger more similiar sequence fragments
				reverse(seqs.begin(), seqs.end());
				
				for (int k = seqs.size()-1; k > (parents-1); k--)  {  
					delete seqs[k].seq;
					seqs.pop_back();	
				}
			}
			
			//put seqs into vector to send to slayer
			vector<Sequence*> seqsForSlayer;
			
			for (int k = 0; k < seqs.size(); k++) {  seqsForSlayer.push_back(seqs[k].seq);	}
			
			//mask then send to slayer...
			if (seqMask != "") {
				decalc->setMask(seqMask);
				
				//mask querys
				decalc->runMask(query);
				
				//mask parents
				for (int k = 0; k < seqsForSlayer.size(); k++) {
					decalc->runMask(seqsForSlayer[k]);
				}
				
				spotMap = decalc->getMaskMap();
			}
			
			if (m->control_pressed) {  for (int k = 0; k < seqs.size(); k++) {  delete seqs[k].seq;   }  return 0;  }

			//send to slayer
			chimeraFlags = slayer.getResults(query, seqsForSlayer);
			if (m->control_pressed) {  return 0;  }
			chimeraResults = slayer.getOutput();
			
			//free memory
			for (int k = 0; k < seqs.size(); k++) {  delete seqs[k].seq;   }
			
			printResults.spotMap = spotMap;
			printResults.flag = chimeraFlags;
			printResults.results = chimeraResults;
		}
		
		return 0;
	}
	catch(exception& e) {
		m->errorOut(e, "ChimeraSlayer", "getChimeras");
		exit(1);
	}
}
//***************************************************************************************************************
void ChimeraSlayer::printBlock(data_struct data, string flag, ostream& out){
	try {
		out << querySeq->getName() << '\t';
		out << data.parentA.getName() << "\t" << data.parentB.getName()  << '\t';
	
		out << data.divr_qla_qrb << '\t' << data.qla_qrb << '\t' << data.bsa << '\t';
		out << data.divr_qlb_qra << '\t' << data.qlb_qra << '\t' << data.bsb << '\t';
		
		out << flag << '\t' << spotMap[data.winLStart] << "-" << spotMap[data.winLEnd] << '\t' << spotMap[data.winRStart] << "-" << spotMap[data.winREnd] << '\t';
		
	}
	catch(exception& e) {
		m->errorOut(e, "ChimeraSlayer", "printBlock");
		exit(1);
	}
}
//***************************************************************************************************************
void ChimeraSlayer::printBlock(data_results leftdata, data_results rightdata, bool leftChimeric, bool rightChimeric, string flag, ostream& out){
	try {
		
		if ((leftChimeric) && (!rightChimeric)) { //print left
			out << querySeq->getName() << '\t';
			out << leftdata.results[0].parentA.getName() << "\t" << leftdata.results[0].parentB.getName()  << '\t';
			
			out << leftdata.results[0].divr_qla_qrb << '\t' << leftdata.results[0].qla_qrb << '\t' << leftdata.results[0].bsa << '\t';
			out << leftdata.results[0].divr_qlb_qra << '\t' << leftdata.results[0].qlb_qra << '\t' << leftdata.results[0].bsb << '\t';
		
			out << flag << '\t' << leftdata.spotMap[leftdata.results[0].winLStart] << "-" << leftdata.spotMap[leftdata.results[0].winLEnd] << '\t' << leftdata.spotMap[leftdata.results[0].winRStart] << "-" << leftdata.spotMap[leftdata.results[0].winREnd] << '\t';
		
		}else if ((!leftChimeric) && (rightChimeric)) {  //print right
			out << querySeq->getName() << '\t';
			out << rightdata.results[0].parentA.getName() << "\t" << rightdata.results[0].parentB.getName()  << '\t';
			
			out << rightdata.results[0].divr_qla_qrb << '\t' << rightdata.results[0].qla_qrb << '\t' << rightdata.results[0].bsa << '\t';
			out << rightdata.results[0].divr_qlb_qra << '\t' << rightdata.results[0].qlb_qra << '\t' << rightdata.results[0].bsb << '\t';
			
			out << flag << '\t' << rightdata.spotMap[rightdata.results[0].winLStart] << "-" << rightdata.spotMap[rightdata.results[0].winLEnd] << '\t' << rightdata.spotMap[rightdata.results[0].winRStart] << "-" << rightdata.spotMap[rightdata.results[0].winREnd] << '\t';			
			
		}else  { //print both results
			if (leftdata.flag == "yes") {
				out << querySeq->getName() + "_LEFT" << '\t';
				out << leftdata.results[0].parentA.getName() << "\t" << leftdata.results[0].parentB.getName()  << '\t';
				
				out << leftdata.results[0].divr_qla_qrb << '\t' << leftdata.results[0].qla_qrb << '\t' << leftdata.results[0].bsa << '\t';
				out << leftdata.results[0].divr_qlb_qra << '\t' << leftdata.results[0].qlb_qra << '\t' << leftdata.results[0].bsb << '\t';
				
				out << flag << '\t' << leftdata.spotMap[leftdata.results[0].winLStart] << "-" << leftdata.spotMap[leftdata.results[0].winLEnd] << '\t' << leftdata.spotMap[leftdata.results[0].winRStart] << "-" << leftdata.spotMap[leftdata.results[0].winREnd] << '\t';
			}
			
			if (rightdata.flag == "yes") {
				if (leftdata.flag == "yes") { out << endl; }
				
				out << querySeq->getName() + "_RIGHT"<< '\t';
				out << rightdata.results[0].parentA.getName() << "\t" << rightdata.results[0].parentB.getName()  << '\t';
				
				out << rightdata.results[0].divr_qla_qrb << '\t' << rightdata.results[0].qla_qrb << '\t' << rightdata.results[0].bsa << '\t';
				out << rightdata.results[0].divr_qlb_qra << '\t' << rightdata.results[0].qlb_qra << '\t' << rightdata.results[0].bsb << '\t';
				
				out << flag << '\t' << rightdata.spotMap[rightdata.results[0].winLStart] << "-" << rightdata.spotMap[rightdata.results[0].winLEnd] << '\t' << rightdata.spotMap[rightdata.results[0].winRStart] << "-" << rightdata.spotMap[rightdata.results[0].winREnd] << '\t';			
		
			}
		}
	}
	catch(exception& e) {
		m->errorOut(e, "ChimeraSlayer", "printBlock");
		exit(1);
	}
}
//***************************************************************************************************************
string ChimeraSlayer::getBlock(data_results leftdata, data_results rightdata, bool leftChimeric, bool rightChimeric, string flag){
	try {
		
		string out = "";
		
		if ((leftChimeric) && (!rightChimeric)) { //get left
			out += querySeq->getName() + "\t";
			out += leftdata.results[0].parentA.getName() + "\t" + leftdata.results[0].parentB.getName() + "\t";
			
			out += toString(leftdata.results[0].divr_qla_qrb) + "\t" + toString(leftdata.results[0].qla_qrb) + "\t" + toString(leftdata.results[0].bsa) + "\t";
			out += toString(leftdata.results[0].divr_qlb_qra) + "\t" + toString(leftdata.results[0].qlb_qra) + "\t" + toString(leftdata.results[0].bsb) + "\t";
			
			out += flag + "\t" + toString(leftdata.spotMap[leftdata.results[0].winLStart]) + "-" + toString(leftdata.spotMap[leftdata.results[0].winLEnd]) + "\t" + toString(leftdata.spotMap[leftdata.results[0].winRStart]) + "-" + toString(leftdata.spotMap[leftdata.results[0].winREnd]) + "\t";
			
		}else if ((!leftChimeric) && (rightChimeric)) {  //print right
			out += querySeq->getName() + "\t";
			out += rightdata.results[0].parentA.getName() + "\t" + rightdata.results[0].parentB.getName()  + "\t";
			
			out += toString(rightdata.results[0].divr_qla_qrb) + "\t" + toString(rightdata.results[0].qla_qrb) + "\t" + toString(rightdata.results[0].bsa) + "\t";
			out += toString(rightdata.results[0].divr_qlb_qra) + "\t" + toString(rightdata.results[0].qlb_qra) + "\t" + toString(rightdata.results[0].bsb) + "\t";
			
			out += flag + "\t" + toString(rightdata.spotMap[rightdata.results[0].winLStart]) + "-" + toString(rightdata.spotMap[rightdata.results[0].winLEnd]) + "\t" + toString(rightdata.spotMap[rightdata.results[0].winRStart]) + "-" + toString(rightdata.spotMap[rightdata.results[0].winREnd]) + "\t";			
			
		}else  { //print both results
			
			if (leftdata.flag == "yes") {
				out += querySeq->getName() + "_LEFT\t";
				out += leftdata.results[0].parentA.getName() + "\t" + leftdata.results[0].parentB.getName() + "\t";
				
				out += toString(leftdata.results[0].divr_qla_qrb) + "\t" + toString(leftdata.results[0].qla_qrb) + "\t" + toString(leftdata.results[0].bsa) + "\t";
				out += toString(leftdata.results[0].divr_qlb_qra) + "\t" + toString(leftdata.results[0].qlb_qra) + "\t" + toString(leftdata.results[0].bsb) + "\t";
				
				out += flag + "\t" + toString(leftdata.spotMap[leftdata.results[0].winLStart]) + "-" + toString(leftdata.spotMap[leftdata.results[0].winLEnd]) + "\t" + toString(leftdata.spotMap[leftdata.results[0].winRStart]) + "-" + toString(leftdata.spotMap[leftdata.results[0].winREnd]) + "\t";
			}
			
			if (rightdata.flag == "yes") {
				if (leftdata.flag == "yes") { out += "\n"; }
				out +=  querySeq->getName() + "_RIGHT\t";
				out += rightdata.results[0].parentA.getName() + "\t" + rightdata.results[0].parentB.getName()  + "\t";
				
				out += toString(rightdata.results[0].divr_qla_qrb) + "\t" + toString(rightdata.results[0].qla_qrb) + "\t" + toString(rightdata.results[0].bsa) + "\t";
				out += toString(rightdata.results[0].divr_qlb_qra) + "\t" + toString(rightdata.results[0].qlb_qra) + "\t" + toString(rightdata.results[0].bsb) + "\t";
				
				out += flag + "\t" + toString(rightdata.spotMap[rightdata.results[0].winLStart]) + "-" + toString(rightdata.spotMap[rightdata.results[0].winLEnd]) + "\t" + toString(rightdata.spotMap[rightdata.results[0].winRStart]) + "-" + toString(rightdata.spotMap[rightdata.results[0].winREnd]) + "\t";			
			}
		}
		
		return out;
		
	}
	catch(exception& e) {
		m->errorOut(e, "ChimeraSlayer", "getBlock");
		exit(1);
	}
}
//***************************************************************************************************************
string ChimeraSlayer::getBlock(data_struct data, string flag){
	try {
		
		string outputString = "";
		
		outputString += querySeq->getName() + "\t";
		outputString += data.parentA.getName() + "\t" + data.parentB.getName()  + "\t";
			
		outputString += toString(data.divr_qla_qrb) + "\t" + toString(data.qla_qrb) + "\t" + toString(data.bsa) + "\t";
		outputString += toString(data.divr_qlb_qra) + "\t" + toString(data.qlb_qra) + "\t" + toString(data.bsb) + "\t";
		
		outputString += flag + "\t" + toString(spotMap[data.winLStart]) + "-" + toString(spotMap[data.winLEnd]) + "\t" + toString(spotMap[data.winRStart]) + "-" + toString(spotMap[data.winREnd]) + "\t";
		
		return outputString;
	}
	catch(exception& e) {
		m->errorOut(e, "ChimeraSlayer", "getBlock");
		exit(1);
	}
}
//***************************************************************************************************************/

