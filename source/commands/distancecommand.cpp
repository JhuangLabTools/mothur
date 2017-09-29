/*
 *  distancecommand.cpp
 *  Mothur
 *
 *  Created by Sarah Westcott on 5/7/09.
 *  Copyright 2009 Schloss Lab UMASS Amherst. All rights reserved.
 *
 */

#include "distancecommand.h"

//**********************************************************************************************************************
vector<string> DistanceCommand::setParameters(){	
	try {
		CommandParameter pcolumn("column", "InputTypes", "", "", "none", "none", "OldFastaColumn","column",false,false); parameters.push_back(pcolumn);
		CommandParameter poldfasta("oldfasta", "InputTypes", "", "", "none", "none", "OldFastaColumn","",false,false); parameters.push_back(poldfasta);
		CommandParameter pfasta("fasta", "InputTypes", "", "", "none", "none", "none","phylip-column",false,true, true); parameters.push_back(pfasta);
		CommandParameter poutput("output", "Multiple", "column-lt-square-phylip", "column", "", "", "","phylip-column",false,false, true); parameters.push_back(poutput);
		CommandParameter pcalc("calc", "Multiple", "nogaps-eachgap-onegap", "onegap", "", "", "","",false,false); parameters.push_back(pcalc);
		CommandParameter pcountends("countends", "Boolean", "", "T", "", "", "","",false,false); parameters.push_back(pcountends);
		CommandParameter pcompress("compress", "Boolean", "", "F", "", "", "","",false,false); parameters.push_back(pcompress);
		CommandParameter pprocessors("processors", "Number", "", "1", "", "", "","",false,false, true); parameters.push_back(pprocessors);
		CommandParameter pcutoff("cutoff", "Number", "", "1.0", "", "", "","",false,false, true); parameters.push_back(pcutoff);
		CommandParameter pseed("seed", "Number", "", "0", "", "", "","",false,false); parameters.push_back(pseed);
        CommandParameter pinputdir("inputdir", "String", "", "", "", "", "","",false,false); parameters.push_back(pinputdir);
		CommandParameter poutputdir("outputdir", "String", "", "", "", "", "","",false,false); parameters.push_back(poutputdir);
		
		vector<string> myArray;
		for (int i = 0; i < parameters.size(); i++) {	myArray.push_back(parameters[i].name);		}
		return myArray;
	}
	catch(exception& e) {
		m->errorOut(e, "DistanceCommand", "setParameters");
		exit(1);
	}
}
//**********************************************************************************************************************
string DistanceCommand::getHelpString(){	
	try {
		string helpString = "";
		helpString += "The dist.seqs command reads a file containing sequences and creates a distance file.\n";
		helpString += "The dist.seqs command parameters are fasta, oldfasta, column, calc, countends, output, compress, cutoff and processors.  \n";
		helpString += "The fasta parameter is required, unless you have a valid current fasta file.\n";
		helpString += "The oldfasta and column parameters allow you to append the distances calculated to the column file.\n";
		helpString += "The calc parameter allows you to specify the method of calculating the distances.  Your options are: nogaps, onegap or eachgap. The default is onegap.\n";
		helpString += "The countends parameter allows you to specify whether to include terminal gaps in distance.  Your options are: T or F. The default is T.\n";
		helpString += "The cutoff parameter allows you to specify maximum distance to keep. The default is 1.0.\n";
		helpString += "The output parameter allows you to specify format of your distance matrix. Options are column, lt, and square. The default is column.\n";
		helpString += "The processors parameter allows you to specify number of processors to use.  The default is 1.\n";
		helpString += "The compress parameter allows you to indicate that you want the resulting distance file compressed.  The default is false.\n";
		helpString += "The dist.seqs command should be in the following format: \n";
		helpString += "dist.seqs(fasta=yourFastaFile, calc=yourCalc, countends=yourEnds, cutoff= yourCutOff, processors=yourProcessors) \n";
		helpString += "Example dist.seqs(fasta=amazon.fasta, calc=eachgap, countends=F, cutoff= 2.0, processors=3).\n";
		helpString += "Note: No spaces between parameter labels (i.e. calc), '=' and parameters (i.e.yourCalc).\n";
		return helpString;
	}
	catch(exception& e) {
		m->errorOut(e, "DistanceCommand", "getHelpString");
		exit(1);
	}
}
//**********************************************************************************************************************
string DistanceCommand::getOutputPattern(string type) {
    try {
        string pattern = "";
        
        if (type == "phylip") {  pattern = "[filename],[outputtag],dist"; } 
        else if (type == "column") { pattern = "[filename],dist"; }
        else { m->mothurOut("[ERROR]: No definition for type " + type + " output pattern.\n"); m->setControl_pressed(true);  }
        
        return pattern;
    }
    catch(exception& e) {
        m->errorOut(e, "DistanceCommand", "getOutputPattern");
        exit(1);
    }
}
//**********************************************************************************************************************
DistanceCommand::DistanceCommand(){	
	try {
		abort = true; calledHelp = true; 
		setParameters();
		vector<string> tempOutNames;
		outputTypes["phylip"] = tempOutNames;
		outputTypes["column"] = tempOutNames;
	}
	catch(exception& e) {
		m->errorOut(e, "DistanceCommand", "DistanceCommand");
		exit(1);
	}
}
//**********************************************************************************************************************
DistanceCommand::DistanceCommand(string option) {
	try {
		abort = false; calledHelp = false;   
		Estimators.clear();
				
		//allow user to run help
		if(option == "help") { help(); abort = true; calledHelp = true; }
		else if(option == "citation") { citation(); abort = true; calledHelp = true;}
		
		else {
			vector<string> myArray = setParameters();
			
			OptionParser parser(option);
			map<string, string> parameters = parser.getParameters();
			
			ValidParameters validParameter("dist.seqs");
			map<string, string>::iterator it2;
		
			//check to make sure all parameters are valid for command
			for (it2 = parameters.begin(); it2 != parameters.end(); it2++) { 
				if (validParameter.isValidParameter(it2->first, myArray, it2->second) != true) {  abort = true;  }
			}
			
			//initialize outputTypes
			vector<string> tempOutNames;
			outputTypes["phylip"] = tempOutNames;
			outputTypes["column"] = tempOutNames;
		
			//if the user changes the input directory command factory will send this info to us in the output parameter 
			string inputDir = validParameter.validFile(parameters, "inputdir", false);		
			if (inputDir == "not found"){	inputDir = "";		}
			else {
				string path;
				it2 = parameters.find("fasta");
				//user has given a template file
				if(it2 != parameters.end()){ 
					path = m->hasPath(it2->second);
					//if the user has not given a path then, add inputdir. else leave path alone.
					if (path == "") {	parameters["fasta"] = inputDir + it2->second;		}
				}
				
				it2 = parameters.find("oldfasta");
				//user has given a template file
				if(it2 != parameters.end()){ 
					path = m->hasPath(it2->second);
					//if the user has not given a path then, add inputdir. else leave path alone.
					if (path == "") {	parameters["oldfasta"] = inputDir + it2->second;		}
				}
				
				it2 = parameters.find("column");
				//user has given a template file
				if(it2 != parameters.end()){ 
					path = m->hasPath(it2->second);
					//if the user has not given a path then, add inputdir. else leave path alone.
					if (path == "") {	parameters["column"] = inputDir + it2->second;		}
				}
			}

			//check for required parameters
			fastafile = validParameter.validFile(parameters, "fasta", true);
			if (fastafile == "not found") { 				
				fastafile = m->getFastaFile(); 
				if (fastafile != "") { m->mothurOut("Using " + fastafile + " as input file for the fasta parameter."); m->mothurOutEndLine(); 
					ifstream inFASTA;
					m->openInputFile(fastafile, inFASTA);
					alignDB = SequenceDB(inFASTA); 
					inFASTA.close();
				}else { 	m->mothurOut("You have no current fastafile and the fasta parameter is required."); m->mothurOutEndLine(); abort = true; }
			}else if (fastafile == "not open") { abort = true; }	
			else{
				ifstream inFASTA;
				m->openInputFile(fastafile, inFASTA);
				alignDB = SequenceDB(inFASTA); 
				inFASTA.close();
				m->setFastaFile(fastafile);
			}
			
			oldfastafile = validParameter.validFile(parameters, "oldfasta", true);
			if (oldfastafile == "not found") { oldfastafile = ""; }
			else if (oldfastafile == "not open") { abort = true; }	
			
			column = validParameter.validFile(parameters, "column", true);
			if (column == "not found") { column = ""; }
			else if (column == "not open") { abort = true; }	
			else { m->setColumnFile(column); }
			
			//if the user changes the output directory command factory will send this info to us in the output parameter 
			outputDir = validParameter.validFile(parameters, "outputdir", false);		if (outputDir == "not found"){	
				outputDir = "";	
				outputDir += m->hasPath(fastafile); //if user entered a file with a path then preserve it	
			}

			//check for optional parameter and set defaults
			// ...at some point should added some additional type checking...
			calc = validParameter.validFile(parameters, "calc", false);			
			if (calc == "not found") { calc = "onegap";  }
			else { 
				 if (calc == "default")  {  calc = "onegap";  }
			}
			m->splitAtDash(calc, Estimators);

			string temp;
			temp = validParameter.validFile(parameters, "countends", false);	if(temp == "not found"){	temp = "T";	}
            countends = m->isTrue(temp);
			
			temp = validParameter.validFile(parameters, "cutoff", false);		if(temp == "not found"){	temp = "1.0"; }
			m->mothurConvert(temp, cutoff); 
			
			temp = validParameter.validFile(parameters, "processors", false);	if (temp == "not found"){	temp = m->getProcessors();	}
			m->setProcessors(temp);
			m->mothurConvert(temp, processors);
			
			temp = validParameter.validFile(parameters, "compress", false);		if(temp == "not found"){  temp = "F"; }
			convert(temp, compress);

			output = validParameter.validFile(parameters, "output", false);		if(output == "not found"){	output = "column"; }
            if (output == "phylip") { output = "lt";  }
			
			if (((column != "") && (oldfastafile == "")) || ((column == "") && (oldfastafile != ""))) { m->mothurOut("If you provide column or oldfasta, you must provide both."); m->mothurOutEndLine(); abort=true; }
			
			if ((column != "") && (oldfastafile != "") && (output != "column")) { m->mothurOut("You have provided column and oldfasta, indicating you want to append distances to your column file. Your output must be in column format to do so."); m->mothurOutEndLine(); abort=true; }
			
			if ((output != "column") && (output != "lt") && (output != "square")) { m->mothurOut(output + " is not a valid output form. Options are column, lt and square. I will use column."); m->mothurOutEndLine(); output = "column"; }

		}
				
	}
	catch(exception& e) {
		m->errorOut(e, "DistanceCommand", "DistanceCommand");
		exit(1);
	}
}
//**********************************************************************************************************************

int DistanceCommand::execute(){
	try {
		
		if (abort == true) { if (calledHelp) { return 0; }  return 2;	}
		
		int startTime = time(NULL);
		
		//save number of new sequence
		numNewFasta = alignDB.getNumSeqs();
		
		//sanity check the oldfasta and column file as well as add oldfasta sequences to alignDB
		if ((oldfastafile != "") && (column != ""))  {	if (!(sanityCheck())) { return 0; }  }
		
		if (m->getControl_pressed()) { return 0; }
		
		numSeqs = alignDB.getNumSeqs();
		
		if (!alignDB.sameLength()) {  m->mothurOut("[ERROR]: your sequences are not the same length, aborting."); m->mothurOutEndLine(); return 0; }
		
		string outputFile;
        
        map<string, string> variables; 
        variables["[filename]"] = outputDir + m->getRootName(m->getSimpleName(fastafile));
		if (output == "lt") { //does the user want lower triangle phylip formatted file 
            variables["[outputtag]"] = "phylip";
			outputFile = getOutputFileName("phylip", variables);
			m->mothurRemove(outputFile); outputTypes["phylip"].push_back(outputFile);
			
			//output numSeqs to phylip formatted dist file
		}else if (output == "column") { //user wants column format
			outputFile = getOutputFileName("column", variables);
			outputTypes["column"].push_back(outputFile);
			
			//so we don't accidentally overwrite
			if (outputFile == column) { 
				string tempcolumn = column + ".old"; 
				rename(column.c_str(), tempcolumn.c_str());
			}
			
			m->mothurRemove(outputFile);
		}else { //assume square
			variables["[outputtag]"] = "square";
			outputFile = getOutputFileName("phylip", variables);
			m->mothurRemove(outputFile);
			outputTypes["phylip"].push_back(outputFile);
		}
        
        m->mothurOut("\nSequence\tTime\tNum_Dists_Below_Cutoff\n");

        createProcesses(outputFile);
		
		if (m->getControl_pressed()) { outputTypes.clear();  m->mothurRemove(outputFile); return 0; }
		
		ifstream fileHandle;
		fileHandle.open(outputFile.c_str());
		if(fileHandle) {
			m->gobble(fileHandle);
			if (fileHandle.eof()) { m->mothurOut(outputFile + " is blank. This can result if there are no distances below your cutoff.");  m->mothurOutEndLine(); }
		}
		
		//append the old column file to the new one
		if ((oldfastafile != "") && (column != ""))  {
			//we had to rename the column file so we didnt overwrite above, but we want to keep old name
			if (outputFile == column) { 
				string tempcolumn = column + ".old";
				m->appendFiles(tempcolumn, outputFile);
				m->mothurRemove(tempcolumn);
			}else{
				m->appendFiles(outputFile, column);
				m->mothurRemove(outputFile);
				outputFile = column;
			}
			
			if (outputDir != "") { 
				string newOutputName = outputDir + m->getSimpleName(outputFile);
				rename(outputFile.c_str(), newOutputName.c_str());
				m->mothurRemove(outputFile);
				outputFile = newOutputName;
			}
		}
		
		if (m->getControl_pressed()) { outputTypes.clear();  m->mothurRemove(outputFile); return 0; }
		
		//set phylip file as new current phylipfile
		string current = "";
		itTypes = outputTypes.find("phylip");
		if (itTypes != outputTypes.end()) {
			if ((itTypes->second).size() != 0) { current = (itTypes->second)[0]; m->setPhylipFile(current); }
		}
		
		//set column file as new current columnfile
		itTypes = outputTypes.find("column");
		if (itTypes != outputTypes.end()) {
			if ((itTypes->second).size() != 0) { current = (itTypes->second)[0]; m->setColumnFile(current); }
		}
		
		m->mothurOutEndLine();
		m->mothurOut("Output File Names: "); m->mothurOutEndLine();
		m->mothurOut(outputFile); m->mothurOutEndLine();
		m->mothurOutEndLine();
		m->mothurOut("It took " + toString(time(NULL) - startTime) + " seconds to calculate the distances for " + toString(numSeqs) + " sequences."); m->mothurOutEndLine();


		if (m->isTrue(compress)) {
			m->mothurOut("Compressing..."); m->mothurOutEndLine();
			m->mothurOut("(Replacing " + outputFile + " with " + outputFile + ".gz)"); m->mothurOutEndLine();
			system(("gzip -v " + outputFile).c_str());
			outputNames.push_back(outputFile + ".gz");
		}else { outputNames.push_back(outputFile); }

		return 0;
		
	}
	catch(exception& e) {
		m->errorOut(e, "DistanceCommand", "execute");
		exit(1);
	}
}
/**************************************************************************************************/
/////// need to fix to work with calcs and sequencedb
void driverColumn(distanceData* params){
    try {
        ValidCalculators validCalculator;
        DistCalc* distCalculator;
        if (params->countends) {
            for (int i=0; i<params->Estimators.size(); i++) {
                if (validCalculator.isValidCalculator("distance", params->Estimators[i]) == true) {
                    if (params->Estimators[i] == "nogaps")			{	distCalculator = new ignoreGaps();	}
                    else if (params->Estimators[i] == "eachgap")	{	distCalculator = new eachGapDist();	}
                    else if (params->Estimators[i] == "onegap")		{	distCalculator = new oneGapDist();	}
                }
            }
        }else {
            for (int i=0; i<params->Estimators.size(); i++) {
                if (validCalculator.isValidCalculator("distance", params->Estimators[i]) == true) {
                    if (params->Estimators[i] == "nogaps")		{	distCalculator = new ignoreGaps();					}
                    else if (params->Estimators[i] == "eachgap"){	distCalculator = new eachGapIgnoreTermGapDist();	}
                    else if (params->Estimators[i] == "onegap")	{	distCalculator = new oneGapIgnoreTermGapDist();		}
                }
            }
        }
        
        int startTime = time(NULL);
        long long numSeqs = params->alignDB.getNumSeqs();
        
        //column file
        ofstream outFile;
        params->m->openOutputFile(params->dFileName, outFile);
        outFile.setf(ios::fixed, ios::showpoint);
        outFile << setprecision(4);
        
        if((params->output == "lt") && params->startLine == 0){	outFile << numSeqs << endl;	}
        
        
        params->count = 0;
        for(int i=params->startLine;i<params->endLine;i++){
            if(params->output == "lt")	{
                string name = params->alignDB.get(i).getName();
                if (name.length() < 10) { //pad with spaces to make compatible
                    while (name.length() < 10) {  name += " ";  }
                }
                outFile << name;
            }
            for(int j=0;j<i;j++){
                
                if (params->m->getControl_pressed()) { break;  }
                
                //if there was a column file given and we are appending, we don't want to calculate the distances that are already in the column file
                //the alignDB contains the new sequences and then the old, so if i an oldsequence and j is an old sequence then break out of this loop
                if ((i >= params->numNewFasta) && (j >= params->numNewFasta)) { break; }
                
                distCalculator->calcDist(params->alignDB.get(i), params->alignDB.get(j));
                double dist = distCalculator->getDist();
                
                if(dist <= params->cutoff){
                    if (params->output == "column") { outFile << params->alignDB.get(i).getName() << ' ' << params->alignDB.get(j).getName() << ' ' << dist << endl; }
                    params->count++;
                }
                if (params->output == "lt") {  outFile  << '\t' << dist; }
            }
            
            if (params->output == "lt") { outFile << endl; }
            
            if(i % 100 == 0){
                params->m->mothurOutJustToScreen(toString(i) + "\t" + toString(time(NULL) - startTime) + "\t" + toString(params->count) +"\n");
            }
            
        }
        params->m->mothurOutJustToScreen(toString(params->endLine-1) + "\t" + toString(time(NULL) - startTime)+ "\t" + toString(params->count) +"\n");
        
        outFile.close();
        delete distCalculator;
    }
    catch(exception& e) {
        params->m->errorOut(e, "DistanceCommand", "driver");
        exit(1);
    }
}
/**************************************************************************************************/
/////// need to fix to work with calcs and sequencedb
void driverPhylip(distanceData* params){
    try {
        ValidCalculators validCalculator;
        DistCalc* distCalculator;
        if (params->countends) {
            for (int i=0; i<params->Estimators.size(); i++) {
                if (validCalculator.isValidCalculator("distance", params->Estimators[i]) == true) {
                    if (params->Estimators[i] == "nogaps")			{	distCalculator = new ignoreGaps();	}
                    else if (params->Estimators[i] == "eachgap")	{	distCalculator = new eachGapDist();	}
                    else if (params->Estimators[i] == "onegap")		{	distCalculator = new oneGapDist();	}
                }
            }
        }else {
            for (int i=0; i<params->Estimators.size(); i++) {
                if (validCalculator.isValidCalculator("distance", params->Estimators[i]) == true) {
                    if (params->Estimators[i] == "nogaps")		{	distCalculator = new ignoreGaps();					}
                    else if (params->Estimators[i] == "eachgap"){	distCalculator = new eachGapIgnoreTermGapDist();	}
                    else if (params->Estimators[i] == "onegap")	{	distCalculator = new oneGapIgnoreTermGapDist();		}
                }
            }
        }
        
        int startTime = time(NULL);
        
        //column file
        ofstream outFile;
        params->m->openOutputFile(params->dFileName, outFile);
        outFile.setf(ios::fixed, ios::showpoint);
        outFile << setprecision(4);
        
        long long numSeqs = params->alignDB.getNumSeqs();
        if(params->startLine == 0){	outFile << numSeqs << endl;	}
        
        params->count = 0;
        for(int i=params->startLine;i<params->endLine;i++){
            
            string name = params->alignDB.get(i).getName();
            //pad with spaces to make compatible
            if (name.length() < 10) { while (name.length() < 10) {  name += " ";  } }
            
            outFile << name << '\t';
            
            for(int j=0;j<numSeqs;j++){
                
                if (params->m->getControl_pressed()) { break; }
                
                distCalculator->calcDist(params->alignDB.get(i), params->alignDB.get(j));
                double dist = distCalculator->getDist();
                
                if(dist <= params->cutoff){ params->count++; }
                
                outFile << dist << '\t'; 
            }
            
            outFile << endl; 
            
            if(i % 100 == 0){
                params->m->mothurOutJustToScreen(toString(i) + "\t" + toString(time(NULL) - startTime) + "\t" + toString(params->count) +"\n");
            }
        }
        params->m->mothurOutJustToScreen(toString(params->endLine-1) + "\t" + toString(time(NULL) - startTime)+ "\t" + toString(params->count) +"\n");
        
        
        outFile.close();
        delete distCalculator;
    }
    catch(exception& e) {
        params->m->errorOut(e, "DistanceCommand", "driver");
        exit(1);
    }
}

/**************************************************************************************************/
void DistanceCommand::createProcesses(string filename) {
	try {
        unsigned long long numDists = 0;
        
        if (output == "square") {
            numDists = numSeqs * numSeqs;
        }else {
            for(int i=0;i<numSeqs;i++){
                for(int j=0;j<i;j++){
                    numDists++;
                    if (numDists > processors) { break; }
                }
            }
        }
        
        if (numDists < processors) { processors = numDists; }
        
        vector<linePair> lines;
        for (int i = 0; i < processors; i++) {
            linePair tempLine;
            lines.push_back(tempLine);
            if (output != "square") {
                lines[i].start = int (sqrt(float(i)/float(processors)) * numSeqs);
                lines[i].end = int (sqrt(float(i+1)/float(processors)) * numSeqs);
            }else{
                lines[i].start = int ((float(i)/float(processors)) * numSeqs);
                lines[i].end = int ((float(i+1)/float(processors)) * numSeqs);
            }
            
        }
        
        //create array of worker threads
        vector<thread*> workerThreads;
        vector<distanceData*> data;
        
        long long num = alignDB.getNumSeqs();
        
        time_t start, end;
        time(&start);
        //Lauch worker threads
        for (int i = 0; i < processors-1; ++i) {
            distanceData* dataBundle = new distanceData(lines[i+1].start, lines[i+1].end, (filename + toString(i+1) + ".temp"),
                                                      cutoff, alignDB, Estimators, m, output, numNewFasta, countends);
            data.push_back(dataBundle);
            
            if (output != "square") { workerThreads.push_back(new thread(driverColumn, dataBundle)); }
            else { workerThreads.push_back(new thread(driverPhylip, dataBundle)); }
        }
        
        distanceData* dataBundle = new distanceData(lines[0].start, lines[0].end, filename, cutoff, alignDB, Estimators, m, output, numNewFasta, countends);
        if (output != "square") { driverColumn(dataBundle); }
        else { driverPhylip(dataBundle); }
        long long distsBelowCutoff = dataBundle->count;
        
        for (int i = 0; i < processors-1; ++i) {
            distsBelowCutoff += data[i]->count;
            workerThreads[i]->join();
            delete data[i];
            delete workerThreads[i];
        }
        
        
        time(&end);
        m->mothurOut("It took " + toString(difftime(end, start)) + " secs to find distances for " + toString(num) + " sequences. " + toString(distsBelowCutoff) + " distances below cutoff " + toString(cutoff) + ".\n\n");

		//append and remove temp files
		for (int i=0;i<processors-1;i++) {
			m->appendFiles((filename + toString(i+1) + ".temp"), filename);
			m->mothurRemove((filename + toString(i+1) + ".temp"));
		}
		
	}
	catch(exception& e) {
		m->errorOut(e, "DistanceCommand", "createProcesses");
		exit(1);
	}
}
/**************************************************************************************************/
//its okay if the column file does not contain all the names in the fasta file, since some distance may have been above a cutoff,
//but no sequences can be in the column file that are not in oldfasta. also, if a distance is above the cutoff given then remove it.
//also check to make sure the 2 files have the same alignment length.
bool DistanceCommand::sanityCheck() {
	try{
		bool good = true;
		
		//make sure the 2 fasta files have the same alignment length
		ifstream in;
		m->openInputFile(fastafile, in);
		int fastaAlignLength = 0;
		if (in) { 
			Sequence tempIn(in);
			fastaAlignLength = tempIn.getAligned().length();
		}
		in.close();
		
		ifstream in2;
		m->openInputFile(oldfastafile, in2);
		int oldfastaAlignLength = 0;
		if (in2) { 
			Sequence tempIn2(in2);
			oldfastaAlignLength = tempIn2.getAligned().length();
		}
		in2.close();
		
		if (fastaAlignLength != oldfastaAlignLength) { m->mothurOut("fasta files do not have the same alignment length."); m->mothurOutEndLine(); return false;  }
		
		//read fasta file and save names as well as adding them to the alignDB
		set<string> namesOldFasta;
		
		ifstream inFasta;
		m->openInputFile(oldfastafile, inFasta);
		
		while (!inFasta.eof()) {
			if (m->getControl_pressed()) {  inFasta.close(); return good;  }
		
			Sequence temp(inFasta);
			
			if (temp.getName() != "") {
				namesOldFasta.insert(temp.getName());  //save name
				alignDB.push_back(temp);  //add to DB
			}
			
			m->gobble(inFasta);
		}
		
		inFasta.close();
		
		//read through the column file checking names and removing distances above the cutoff
		ifstream inDist;
		m->openInputFile(column, inDist);
		
		ofstream outDist;
		string outputFile = column + ".temp";
		m->openOutputFile(outputFile, outDist);
		
		string name1, name2;
		float dist;
		while (!inDist.eof()) {
			if (m->getControl_pressed()) {  inDist.close(); outDist.close(); m->mothurRemove(outputFile); return good;  }
		
			inDist >> name1 >> name2 >> dist; m->gobble(inDist);
			
			//both names are in fasta file and distance is below cutoff
			if ((namesOldFasta.count(name1) == 0) || (namesOldFasta.count(name2) == 0)) {  good = false; break;  }
			else{
				if (dist <= cutoff) {
					outDist << name1 << '\t' << name2 << '\t' << dist << endl;
				}
			}
		}
		
		inDist.close();
		outDist.close();
		
		if (good) {
			m->mothurRemove(column);
			rename(outputFile.c_str(), column.c_str());
		}else{
			m->mothurRemove(outputFile); //temp file is bad because file mismatch above
		}
		
		return good;
		
	}
	catch(exception& e) {
		m->errorOut(e, "DistanceCommand", "sanityCheck");
		exit(1);
	}
}
/**************************************************************************************************/




