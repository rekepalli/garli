// GARLI version 0.951 source code
// Copyright  2005-2006 by Derrick J. Zwickl
// All rights reserved.
//
// This code may be used and modified for non-commercial purposes
// but redistribution in any form requires written permission.
// Please contact:
//
//  Derrick Zwickl
//	National Evolutionary Synthesis Center
//	2024 W. Main Street, Suite A200
//	Durham, NC 27705
//  email: zwickl@nescent.org
//


#ifndef _MODEL_
#define _MODEL_

#include "memchk.h"
#include <iostream>
#include <cassert>
#include <math.h>
#include <vector>
#include "stricl.h"
#include "rng.h"
#include "mlhky.h"
#include "errorexception.h"


class ModelSpecification;

extern rng rnd;
extern ModelSpecification modSpec;

	enum{//the types
		STATEFREQS = 1,
		RELATIVERATES = 2,
		ALPHASHAPE = 3,
		RATEMULTS = 4,
		RATEPROPS = 5,
		PROPORTIONINVARIANT = 6
		};

class BaseParameter {
protected:
	NxsString name;
	int type;
	int numElements;
	bool fixed;
	FLOAT_TYPE maxv,minv;
	FLOAT_TYPE mutationWeight;
	FLOAT_TYPE mutationProb;
	vector<FLOAT_TYPE*> vals; //this will be aliased to the actual 
						//parameter value within the model class (sneaky!)
	vector<FLOAT_TYPE> default_vals;

public:
	BaseParameter()	{
		numElements=1;
		maxv=minv=0.0;
		}

	BaseParameter(const char *n, FLOAT_TYPE **dv, int t, int numE, FLOAT_TYPE mn, FLOAT_TYPE mx) {
		vals.reserve(6);
		default_vals.reserve(6);
		name=n;
		type=t;
		numElements=numE;
		for(int i=0;i<numElements;i++){
			default_vals.push_back(*(dv[i]));
			vals.push_back(dv[i]);
			}
		minv=mn;
		maxv=mx;
		fixed=false;
		}
	~BaseParameter(){};
/*	void Report(ostream &out){
		out << "Type:\t" << name << "\n";
		if(numElements > 1)
			out << "Current value:\t";
		else 
			out << "Current values:\t";
		for(int i=0;i<numElements;i++)
			out << vals[

		}
*/
	void SetFixed(bool tf) {fixed=tf;}
	bool IsFixed() const {return fixed;}
	int Type() const {return type;}
	void SetWeight(FLOAT_TYPE w){mutationWeight=w;}
	FLOAT_TYPE GetWeight(){return mutationWeight;}
	void SetProb(FLOAT_TYPE p){mutationProb=p;}
	FLOAT_TYPE GetProb(){return mutationProb;}
	virtual void Mutator(FLOAT_TYPE) = 0;
	void SetToDefaultValues(){
		for(int e=0;e<numElements;e++) *vals[e] = default_vals[e];
		}
	};

class StateFrequencies:public BaseParameter{

public:
	StateFrequencies(FLOAT_TYPE **dv, int numE):BaseParameter("Base frequencies", dv, STATEFREQS, numE, 0.0, 1.0){};

	void Mutator(FLOAT_TYPE mutationShape){
		int freqToChange=int(rnd.uniform()*numElements);
		FLOAT_TYPE newFreq=*vals[freqToChange] * rnd.gamma( mutationShape );
		for(int b=0;b<numElements;b++)
			if(b!=freqToChange) *vals[b] *= (FLOAT_TYPE)((1.0-newFreq)/(1.0-*vals[freqToChange]));
		*vals[freqToChange]=newFreq;
		}
	};

class RelativeRates:public BaseParameter{
public:
	RelativeRates(const char *c, FLOAT_TYPE **dv, int numE):BaseParameter(c, dv, RELATIVERATES, numE, (FLOAT_TYPE)0.0, (FLOAT_TYPE)999.9){};

	void Mutator(FLOAT_TYPE mutationShape){
		if(numElements > 1){
			int rateToChange=int(rnd.uniform()*(numElements));
			
			if(rateToChange<numElements-1){
				*vals[rateToChange] *= rnd.gamma( mutationShape );
				if(*vals[rateToChange]>maxv) *vals[rateToChange]=maxv;
				}

			else{//if we alter the reference rate, which we are assuming
				//is the last one (GT for DNA models, fixed to 1.0)
				//scale all of the other rates
				FLOAT_TYPE scaler= rnd.gamma( mutationShape );
				for(int i=0;i<numElements-1;i++){
					*vals[i] /= scaler;
					}
				}
			}
		else {
			*vals[0] *= rnd.gamma( mutationShape );
			if(*vals[0]>maxv) *vals[0]=maxv;			
			}
		}
	};

class RateProportions:public BaseParameter{
public:
	RateProportions(FLOAT_TYPE **dv, int numE):BaseParameter("Rate props", dv, RATEPROPS, numE, 0.0, 1.0){};
	void Mutator(FLOAT_TYPE mutationShape){
		int rateToChange=int(rnd.uniform()*(numElements));
		*vals[rateToChange] *= rnd.gamma( mutationShape );
		if(*vals[rateToChange]>maxv) *vals[rateToChange]=maxv;		
		}
	};

class RateMultipliers:public BaseParameter{
public:
	RateMultipliers(FLOAT_TYPE **dv, int numE):BaseParameter("Rate mults", dv, RATEMULTS, numE, (FLOAT_TYPE)0.0, (FLOAT_TYPE)999.9){};
	void Mutator(FLOAT_TYPE mutationShape){
		int rateToChange=int(rnd.uniform()*(numElements));
		*vals[rateToChange] *= rnd.gamma( mutationShape );
		if(*vals[rateToChange]>maxv) *vals[rateToChange]=maxv;
		}
	};

class AlphaShape:public BaseParameter{
public:
	AlphaShape(const char *c, FLOAT_TYPE **dv):BaseParameter(c, dv, ALPHASHAPE, 1, (FLOAT_TYPE)0.0, (FLOAT_TYPE)999.9){};
	void Mutator(FLOAT_TYPE mutationShape){
		*vals[0] *=rnd.gamma( mutationShape );
		}
	};

class ProportionInvariant:public BaseParameter{
public:
	ProportionInvariant(const char *c, FLOAT_TYPE **dv):BaseParameter(c, dv, PROPORTIONINVARIANT, 1, (FLOAT_TYPE)0.0, (FLOAT_TYPE)999.9){};
	void Mutator(FLOAT_TYPE mutationShape){
		*vals[0] *=rnd.gamma( mutationShape );
		}
	};

class ModelSpecification{
	//this will hold the model specification as a global variable
	//so that any models allocated will immediately know what they are
public:
	int nstates;
	int nst;
	int numRateCats;

	bool equalStateFreqs;
	bool empiricalStateFreqs;
	bool fixStateFreqs;
	bool fixRelativeRates;

//	bool fixSubstitutionRates;
	bool flexRates;

	bool fixInvariantSites;
	bool fixAlpha;
	bool includeInvariantSites;
	
	bool gotRmatFromFile;
	bool gotStateFreqsFromFile;
	bool gotAlphaFromFile;
	bool gotFlexFromFile;
	bool gotPinvFromFile;

	ModelSpecification(){
		nstates=4;
		//this is the default model
		SetGTR();
		SetGammaRates();
		SetNumRateCats(4, false);
		SetInvariantSites();
		gotRmatFromFile = gotStateFreqsFromFile = gotAlphaFromFile = gotFlexFromFile = gotPinvFromFile = false;
		}

	//A number of canned model setups

	void SetJC(){
		nstates=4;
		SetNst(1);
		SetEqualStateFreqs();
		fixRelativeRates=true;
		}

	void K2P(){
		nstates=4;
		SetNst(2);
		SetEqualStateFreqs();
		fixRelativeRates=false;
		}

	void SetF81(){
		nstates=4;
		SetNst(1);
		SetEstimateStateFreqs();
		fixRelativeRates=true;	
		}

	void SetHKY(){
		nstates=4;
		SetNst(2);
		SetEstimateStateFreqs();
		fixRelativeRates=false;
		}

	void SetGTR(){
		nstates=4;
		SetNst(6);
		SetEstimateStateFreqs();
		fixRelativeRates=false;
		}

	void SetNst(int n){
		nst=n;
		}

	void SetGammaRates(){
		flexRates=false;
		fixAlpha=false;
		}


	void SetFlexRates(){
		if(includeInvariantSites==true) throw(ErrorException("Sorry, invariant sites models cannot be used with the \"flex\" model of rate heterogeneity"));
		flexRates=true;		
		}

	void SetNumRateCats(int nrates, bool test){//correct behavior here depends on the fact that the default 
		//model includes gamma with 4 rate cats
		if(test ==true){
			if(numRateCats == 1 && nrates > 1)
				throw(ErrorException("ratehetmodel set to \"none\", but numratecats is equal to %d!", nrates));
			if(numRateCats > 1 && nrates == 1){
				if(flexRates == false && fixAlpha == false)
					throw(ErrorException("ratehetmodel set to \"gamma\", but numratecats is equal to 1!"));
				else if(flexRates == false && fixAlpha == true)
					throw(ErrorException("ratehetmodel set to \"gammafixed\", but numratecats is equal to 1!"));
				else if(flexRates == true)
					throw(ErrorException("ratehetmodel set to \"flex\", but numratecats is equal to 1!"));
				}
			}
		
		if(nrates < 1) throw(ErrorException("1 is the minimum value for numratecats."));
		numRateCats=nrates;
		}

	void SetInvariantSites(){
		includeInvariantSites=true;
		fixInvariantSites=false;
		}

	void RemoveInvariantSites(){
		includeInvariantSites=false;
		fixInvariantSites=false;
		}

	void SetEmpiricalStateFreqs(){
		empiricalStateFreqs=fixStateFreqs=true;
		equalStateFreqs=false;
		}

	void SetFixedAlpha(){
		fixAlpha=true;
		}
	void SetFixedInvariantSites(){
		fixInvariantSites=true;
		includeInvariantSites=true;
		}
	void SetEqualStateFreqs(){
		equalStateFreqs=fixStateFreqs=true;
		empiricalStateFreqs=false;
		}
	void SetFixedStateFreqs(){
		fixStateFreqs=true;
		equalStateFreqs=empiricalStateFreqs=false;
		}
	void SetEstimateStateFreqs(){
		equalStateFreqs=fixStateFreqs=empiricalStateFreqs=false;
		}
	void SetFixedRateMatrix(){
		fixRelativeRates=true;
		}
	void SetStateFrequencies(const char *str){
		if(strcmp(str, "equal") == 0) SetEqualStateFreqs();
		else if(strcmp(str, "estimate") == 0) SetEstimateStateFreqs();
		else if(strcmp(str, "empirical") == 0) SetEmpiricalStateFreqs();
		else if(strcmp(str, "fixed") == 0) SetFixedStateFreqs();
		else throw(ErrorException("Unknown setting for statefrequencies: %s", str));
		}
	void SetRateMatrix(const char *str){
		if(strcmp(str, "6rate") == 0) SetNst(6);
		else if(strcmp(str, "2rate") == 0) SetNst(2);
		else if(strcmp(str, "1rate") == 0) SetNst(1);
		else if(strcmp(str, "fixed") == 0) SetFixedRateMatrix();
		else throw(ErrorException("Unknown setting for ratematrix: %s", str));
		}
	void SetProportionInvariant(const char *str){
		if(strcmp(str, "none") == 0) RemoveInvariantSites();
		else if(strcmp(str, "fixed") == 0) SetFixedInvariantSites();
		else if(strcmp(str, "estimate") == 0) SetInvariantSites();
		else throw(ErrorException("Unknown setting for proportioninvariant: %s", str));
		}
	void SetRateHetModel(const char *str){
		if(strcmp(str, "gamma") == 0) SetGammaRates();
		else if(strcmp(str, "gammafixed") == 0){
			SetGammaRates();
			SetFixedAlpha();		
			}
		else if(strcmp(str, "flex") == 0) SetFlexRates();
		else if(strcmp(str, "none") == 0) SetNumRateCats(1, false);
		else throw(ErrorException("Unknown setting for ratehetmodel: %s", str));
		}	

	};

class Model{
	int nst;
	int nstates;
//	FLOAT_TYPE pi[4];
	vector<FLOAT_TYPE*> stateFreqs;
	vector<FLOAT_TYPE*> relRates;

	bool eigenDirty;
	FLOAT_TYPE blen_multiplier;
	
	FLOAT_TYPE rateMults[20];
	FLOAT_TYPE rateProbs[20];
	
	FLOAT_TYPE *alpha;
	FLOAT_TYPE *propInvar;

	//variables used for the eigen process if nst=6
	int *iwork, *indx;
	FLOAT_TYPE *eigvals, *eigvalsimag, **eigvecs, **inveigvecs, **teigvecs, *work, *temp, *col, *c_ijk, *EigValexp;//, **p;
	FLOAT_TYPE **qmat, ***pmat;
	FLOAT_TYPE **tempqmat;
	
	//Newton Raphson crap
	FLOAT_TYPE ***deriv1, ***deriv2;

	public:
//	static bool noPinvInModel;
//	static bool useFlexRates;
//	static int nRateCats;
	static FLOAT_TYPE mutationShape;
	static FLOAT_TYPE maxPropInvar;

	vector<BaseParameter*> paramsToMutate;

	~Model();

	Model(){
		stateFreqs.reserve(4);
		relRates.reserve(6);
		paramsToMutate.reserve(5);
		CreateModelFromSpecification();
		}

	void CalcMutationProbsFromWeights();
	BaseParameter *SelectModelMutation();
	int PerformModelMutation();
	void CreateModelFromSpecification();

	private:
	void AllocateEigenVariables();
	void CalcEigenStuff();

	public:
	void CalcPmat(FLOAT_TYPE blen, FLOAT_TYPE *metaPmat, bool flip =false);
	void CalcDerivatives(FLOAT_TYPE, FLOAT_TYPE ***&, FLOAT_TYPE ***&, FLOAT_TYPE ***&);
	void UpdateQMat();
	void DiscreteGamma(FLOAT_TYPE *, FLOAT_TYPE *, FLOAT_TYPE);
	bool IsModelEqual(const Model *other) const ;	
	void CopyModel(const Model *from);
	void SetModel(FLOAT_TYPE *model_string);
	void OutputPaupBlockForModel(ofstream &outf, const char *treefname) const;
	void OutputGarliFormattedModel(ostream &outf) const;

	//model mutations
	void MutateRates();
	void MutatePis();
	void MutateAlpha();
	void MutatePropInvar();
	void MutateRateProbs();
	void MutateRateMults();
	
	//Accessor functions
	FLOAT_TYPE StateFreq(int p) const{ return *stateFreqs[p];}
	FLOAT_TYPE TRatio() const;
	int Nst() const {return nst;}
	FLOAT_TYPE Rates(int r) const { return *relRates[r];}
	int NRateCats() const {return modSpec.numRateCats;}
	FLOAT_TYPE *GetRateMults() {return rateMults;}
	FLOAT_TYPE Alpha() const {return *alpha;}
	FLOAT_TYPE PropInvar() const { return *propInvar;}
	bool NoPinvInModel() const { return ! (modSpec.includeInvariantSites);}
	FLOAT_TYPE MaxPinv() const{return maxPropInvar;}
	int NStates() const {return nstates;}
	int NumMutatableParams() const {return (int) paramsToMutate.size();}

	//Setting things
	void SetDefaultModelParameters(const HKYData *data);
	void SetRmat(FLOAT_TYPE *r, bool checkValidity){
		if(checkValidity == true){
			if(nst==1){
				if((r[0]==r[1] && r[1]==r[2] &&
					r[2]==r[3] && r[3]==r[4])==false)
					throw(ErrorException("Config file specifies ratematrix = 1rate, but starting model has nonequal rates!\n"));
				}
			if(nst==2){
				if(((r[0]==r[2]  && r[2]==r[3] && r[1]==r[4]))==false)
					throw(ErrorException("Config file specifies ratematrix = 2rate, but starting model does not match!\n"));
				}
			}
		for(int i=0;i<5;i++) *relRates[i]=r[i];
		*relRates[5]=1.0;
		eigenDirty=true;
		}
	void SetPis(FLOAT_TYPE *b, bool checkValidity){
		if(checkValidity == true){
			if(modSpec.equalStateFreqs==true && (b[0]==b[1] && b[1]==b[2]) == false) throw(ErrorException("Config file specifies equal statefrequencies,\nbut starting model has nonequal frequencies!\n"));
			if(modSpec.empiricalStateFreqs==true) throw(ErrorException("Config file specifies empirical statefrequencies,\nbut starting model specifies frequencies!\n"));
			}
		for(int i=0;i<3;i++) *stateFreqs[i]=b[i];
		*stateFreqs[3]=(FLOAT_TYPE)1.0 - *stateFreqs[0] - *stateFreqs[1] - *stateFreqs[2];
		eigenDirty=true;
		}

	void SetFlexRates(FLOAT_TYPE *rates, FLOAT_TYPE *probs){
		if(modSpec.flexRates == false) throw ErrorException("Error: Flex rate values specified in start file,\nbut ratehetmodel is = flex in conf file.");
		for(int r=0;r<NRateCats();r++){
			rateMults[r]=rates[r];
			rateProbs[r]=probs[r];
			}		
		}
	void SetAlpha(FLOAT_TYPE a, bool checkValidity){
		if(checkValidity == true)
			if(modSpec.numRateCats==1) throw(ErrorException("Config file specifies ratehetmodel = none, but starting model contains alpha!\n"));
		*alpha=a;
		DiscreteGamma(rateMults, rateProbs, *alpha);
		//This is odd, but we need to call normalize rates here if we are just using a gamma distrib to get starting rates for 
		//flex.  Flex expects that the rates will be normalized including pinv elsewhere
		if(modSpec.flexRates == true) NormalizeRates();
		}
	void SetPinv(FLOAT_TYPE p, bool checkValidity){
		if(checkValidity == true)
			if(modSpec.includeInvariantSites==false && p!=0.0) throw(ErrorException("Config file specifies invariantsites = none, but starting model contains it!\n"));
		*propInvar=p;
		//change the proportion of rates in each gamma cat
		for(int i=0;i<NRateCats();i++){
			rateProbs[i]=(FLOAT_TYPE)(1.0-*propInvar)/NRateCats();
			}
		}
	void SetMaxPinv(FLOAT_TYPE p){
		Model::maxPropInvar=p;
		}
	void SetDirty(bool tf){
		if(tf) eigenDirty=true;
		else eigenDirty=false;
		}

	void AdjustRateProportions(){
		//this will change the gamma class probs when pinv changes
		for(int i=0;i<NRateCats();i++) rateProbs[i]=(FLOAT_TYPE)(1.0-*propInvar)/NRateCats();
#ifndef NDEBUG
		FLOAT_TYPE sum=0.0;
		for(int i=0;i<NRateCats();i++){
			sum += rateProbs[i];
			}		
		sum += *propInvar;
		assert(fabs(sum - 1.0) < 0.0001);
#endif
		}

	void NormalizeRates(){
		assert(modSpec.flexRates == true);

		FLOAT_TYPE sum=0.0;

		for(int i=0;i<NRateCats();i++){
			sum += rateProbs[i];
			}

		//pinv is figured into the normalization here, but it won't be changed itself.
		if(NoPinvInModel()==false){
			sum = sum / (FLOAT_TYPE)(1.0-*propInvar);
			}	


		for(int i=0;i<NRateCats();i++)	{
			rateProbs[i] /= sum;
			}

		sum=0.0;
		for(int i=0;i<NRateCats();i++){
			sum += rateMults[i]*rateProbs[i];
			}
		for(int i=0;i<NRateCats();i++)	{
			rateMults[i] /= sum;
			}

#ifndef NDEBUG
		sum=0.0;
		for(int i=0;i<NRateCats();i++){
			sum += rateProbs[i];
			}		
		sum += *propInvar;
		assert(fabs(sum - 1.0) < 0.0001);
		sum=0.0;
		for(int i=0;i<NRateCats();i++){
			sum += rateMults[i]*rateProbs[i];
			}
		assert(fabs(sum - 1.0) < 0.0001);

#endif
		}

	const FLOAT_TYPE *GetRateProbs() {
		
/*		FLOAT_TYPE sum=0.0;
		for(int i=0;i<NRateCats();i++){
			sum += rateProbs[i];
			}
		sum+=*propInvar;
		assert(fabs(1.0-sum) < .001);
*/		
		return rateProbs;
		}
	};
	
#endif
