#ifndef HEATMAPSIMCOMMAND_H
#define HEATMAPSIMCOMMAND_H

/*
 *  heatmapsimcommand.h
 *  Mothur
 *
 *  Created by Sarah Westcott on 6/8/09.
 *  Copyright 2009 Schloss Lab UMASS Amherst. All rights reserved.
 *
 */

#include "command.hpp"
#include "inputdata.h"
#include "validcalculator.h"
#include "heatmapsim.h"
#include "nameassignment.hpp"

class HeatMapSimCommand : public Command {

public:
	HeatMapSimCommand(string);
	~HeatMapSimCommand(){}
	
	vector<string> setParameters();
	string getCommandName()			{ return "heatmap.sim";				}
	string getCommandCategory()		{ return "OTU-Based Approaches";	}
	
	string getHelpString();	
    string getOutputPattern(string);	
	string getCitation() { return "http://www.mothur.org/wiki/Heatmap.sim"; }
	string getDescription()		{ return "generate a heatmap indicating the pairwise distance between multiple samples using a variety of calculators"; }

	int execute(); 
	void help() { m->mothurOut(getHelpString()); }	
	
	
private:
	
	vector<Calculator*> heatCalculators;
	HeatMapSim* heatmap;
	
	bool abort, allLines;
	set<string> labels; //holds labels to be used
	string format, groups, label, calc, sharedfile, phylipfile, columnfile, countfile, namefile,  inputfile;
	vector<string> Estimators, Groups, outputNames;
	int fontsize;
	
	int runCommandShared();
	int runCommandDist();


};

#endif

