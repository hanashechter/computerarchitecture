/* 046267 Computer Architecture - Spring 2020 - HW #1 */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h"
#include <stdlib.h>
#include <iostream> 


#define not_using_share 0
#define using_share_lsb 1
#define using_share_mid 2

#define NOT_in_BTB 1<<9

// ========================= Aux Functions ================================

/* Extracts */
uint32_t extract_entry_number (uint32_t pc, unsigned btbSize, unsigned tagSize)
{
	// extract the entry number in the BTB table
	int shift_left = 32 - 2 - log2(btbSize);
	uint32_t tmp_entry = pc<<shift_left; //cut the tag's bits
	int shift_right = 32 - log2(btbSize);
	tmp_entry = tmp_entry>>shift_right;
	return tmp_entry;
}

uint32_t extract_tag (uint32_t pc, unsigned btbSize, unsigned tagSize)
{
	// extract the tag
	int shift_left2 = 32 - 2 - log2(btbSize) - tagSize;
	uint32_t tmp_tag = pc<<shift_left2;
	int shift_right2 = 32 - tagSize;
	tmp_tag = tmp_tag>>shift_right2;
	return tmp_tag;
}

uint32_t manipulateHistory (uint32_t pc, uint32_t history ,unsigned historySize ,bool isGlobalTable, int isShare)
{
	if(isGlobalTable == false || isShare == not_using_share)
		return history;

	int shift_left; 
	int shift_right = 32 - historySize;
	if(isShare == using_share_lsb)
		shift_left = 32 - historySize - 2;
	else
		shift_left = 32 - historySize - 16;

	uint32_t tmp_pc = (pc<<shift_left)>>shift_right;

	return tmp_pc^history; //TO DO
}

// ================================ BTB =========================================
 
class BTB_line
{
	public:

	uint32_t tag;
	uint32_t target_pc;
	uint32_t history;

	BTB_line()
	{
		tag = 0;
		target_pc = 0;
		history = 0;
	}

};



// local entry
class BTB_local_entry
{

public:
	uint32_t tag;
	uint32_t target;
	uint32_t history; 

	BTB_local_entry();
	~BTB_local_entry();
};

BTB_local_entry::BTB_local_entry()
{
	tag = 0;
	target= 0;
	history = 0;
}

BTB_local_entry::~BTB_local_entry()
{
}


// global entry
class BTB_global_entry
{

public:
	uint32_t tag;
	uint32_t target;


	BTB_global_entry();
	~BTB_global_entry();
};

BTB_global_entry::BTB_global_entry()
{
	tag = 0;
	target = 0;
}

BTB_global_entry::~BTB_global_entry()
{
}


// ======================== BTB class ===========================
class BTB
{
public:
	unsigned btbSize;
	unsigned tagSize;
	unsigned historySize;

	virtual uint32_t readHistory(uint32_t pc) = 0;
	virtual uint32_t readTarget(uint32_t pc) = 0;
	virtual void addBranch (uint32_t pc, uint32_t target) = 0;
	virtual void updateHistory(uint32_t pc, bool taken) = 0;
	virtual void updateTarget(uint32_t pc, uint32_t target) = 0;


//	BTB(unsigned btbSize_, unsigned tagSize_, unsigned historySize_);
	BTB();
	virtual ~BTB();
};

BTB::BTB()
{
}

BTB::~BTB()
{
}

/*BTB::BTB(unsigned btbSize_, unsigned tagSize_,unsigned historySize_)
{
	btbSize = btbSize_;
	tagSize = tagSize_;
	historySize = historySize_;
}
*/

// =================== BTB local ==========================

class BTB_local : public BTB
{
public:
	BTB_local_entry * BTB_table;
	
	
	virtual uint32_t readHistory(uint32_t pc);
	virtual uint32_t readTarget(uint32_t pc);
	virtual void addBranch (uint32_t pc, uint32_t target);
	virtual void updateHistory(uint32_t pc, bool taken);
	virtual void updateTarget(uint32_t pc, uint32_t target);

	 
 	BTB_local(unsigned btbSize_, unsigned tagSize_,unsigned historySize_);
	~BTB_local();
};

 BTB_local:: BTB_local(unsigned btbSize_, unsigned tagSize_, unsigned historySize_)
{
	btbSize = btbSize_;
	tagSize = tagSize_;
	historySize = historySize_;
	BTB_table = new BTB_local_entry [btbSize];
}

 BTB_local::~BTB_local()
{
	delete[] BTB_table; // TO DO
}

uint32_t  BTB_local::readHistory(uint32_t pc)
{
	// extract the entry number in the BTB table
	uint32_t entry = extract_entry_number(pc,btbSize,tagSize);

	// extract the tag
    uint32_t tag = extract_tag(pc,btbSize,tagSize);

	if (BTB_table[entry].tag==tag) // the branch exists in the BTB already
	{
		return BTB_table[entry].history;
	}
	else // the branch doesn't exist in the BTB already
	{
		uint32_t ret = NOT_in_BTB; // bigger than the max history 
		return ret;
	}
}

uint32_t BTB_local::readTarget(uint32_t pc)
{
	uint32_t entry = extract_entry_number(pc,btbSize,tagSize);
	return BTB_table[entry].target;
}

void BTB_local::addBranch (uint32_t pc, uint32_t target)
{
	// extract the entry number in the BTB table
	uint32_t entry = extract_entry_number(pc,btbSize,tagSize);

	// extract the tag
    uint32_t tag = extract_tag(pc,btbSize,tagSize);

	BTB_table[entry].tag = tag;
	BTB_table[entry].target = target;
	BTB_table[entry].history = 0;
	
}

void BTB_local::updateHistory(uint32_t pc, bool taken)
{
	// extract the entry number in the BTB table
	uint32_t entry = extract_entry_number(pc,btbSize,tagSize);

	uint32_t tmp_hist = BTB_table[entry].history;
	tmp_hist = tmp_hist<<1;
    tmp_hist = (taken) ? tmp_hist+1 : tmp_hist;
	tmp_hist = tmp_hist%(1<<historySize);

	BTB_table[entry].history = tmp_hist;
}

void BTB_local::updateTarget(uint32_t pc, uint32_t target)
{
	uint32_t entry = extract_entry_number(pc,btbSize,tagSize);
	BTB_table[entry].target = target;
}



// =================== BTB global ==========================

class BTB_global : public BTB
{

public:
	BTB_global_entry * BTB_table;
	uint32_t GHR;

	virtual uint32_t readHistory(uint32_t pc);
	virtual uint32_t readTarget(uint32_t pc);
	virtual void addBranch (uint32_t pc, uint32_t target);
	virtual void updateHistory(uint32_t pc, bool taken);
	virtual void updateTarget(uint32_t pc, uint32_t target);	

	BTB_global(unsigned btbSize_, unsigned tagSize_, unsigned historySize_);
	~BTB_global();
};


BTB_global:: BTB_global(unsigned btbSize_, unsigned tagSize_, unsigned historySize_)
{
	btbSize = btbSize_;
	tagSize = tagSize_;
	historySize = historySize_;

	BTB_table = new BTB_global_entry [btbSize];
	GHR = 0;
}

BTB_global::~BTB_global()
{
	delete[] BTB_table;
}

uint32_t BTB_global::readHistory(uint32_t pc)
{
	// extract the entry number in the BTB table
	uint32_t entry = extract_entry_number(pc,btbSize,tagSize);

	// extract the tag
    uint32_t tag = extract_tag(pc,btbSize,tagSize);

	if (BTB_table[entry].tag==tag) // the branch exists in the BTB already
	{
		return GHR;
	}
	else // the branch doesn't exist in the BTB already
	{
		uint32_t ret = NOT_in_BTB; // bigger than the max history 
		return ret;
	}
}


uint32_t BTB_global::readTarget(uint32_t pc)
{
	uint32_t entry = extract_entry_number(pc,btbSize,tagSize);
	return BTB_table[entry].target;
}

void BTB_global::addBranch (uint32_t pc, uint32_t target)
{
	// extract the entry number in the BTB table
	uint32_t entry = extract_entry_number(pc,btbSize,tagSize);

	// extract the tag
    uint32_t tag = extract_tag(pc,btbSize,tagSize);

	BTB_table[entry].tag = tag;
	BTB_table[entry].target = target;
	
}

void BTB_global::updateHistory(uint32_t pc, bool taken)
{
	uint32_t tmp_hist = GHR;
	tmp_hist = tmp_hist<<1;
    tmp_hist = (taken) ? tmp_hist+1 : tmp_hist;
	tmp_hist = tmp_hist%(1<<historySize);

	GHR = tmp_hist;
}


void BTB_global::updateTarget(uint32_t pc, uint32_t target)
{
	uint32_t entry = extract_entry_number(pc,btbSize,tagSize);
	BTB_table[entry].target = target;
}





//============================ FSM =====================================
class FSM
{

public:
	unsigned btbSize;
	unsigned historySize;
	unsigned tagSize;

	unsigned fsmState;
	
	
	virtual bool readStatePredict(uint32_t pc, uint32_t history) = 0;
	virtual void updateState(uint32_t pc, uint32_t history, bool taken) = 0;
	virtual void restartState(uint32_t pc) = 0;

	FSM();
	virtual ~FSM();
};


FSM::FSM()
{
}

FSM::~FSM()
{
}


// =================== FSM local ==========================
class FSM_local : public FSM
{

public:
	unsigned ** FSM_table;

	virtual bool readStatePredict(uint32_t pc, uint32_t history);
	virtual void updateState(uint32_t pc, uint32_t history, bool taken);
	virtual void restartState(uint32_t pc);

	FSM_local (unsigned btbSize,unsigned historySize,unsigned tagSize,unsigned fsmState);
	~FSM_local ();
};

 FSM_local :: FSM_local (unsigned btbSize_,unsigned historySize_,unsigned tagSize_,unsigned fsmState_)
{
	btbSize = btbSize_;
	historySize = historySize_;
	tagSize = tagSize_;
	fsmState = fsmState_;

	FSM_table = new unsigned*[btbSize];
	for (unsigned i = 0; i < btbSize; i++)
	{
		FSM_table[i] = new unsigned [1<<historySize];
		for (int j = 0; j < (1<<historySize) ; j++)
		{
			FSM_table[i][j] = fsmState;
		}
		
	}
}

 FSM_local :: ~FSM_local ()
{
	for (unsigned i = 0; i < btbSize; i++)
	{
		delete[] FSM_table[i];
	}
	delete[] FSM_table;
	
}


bool FSM_local::readStatePredict(uint32_t pc, uint32_t history)
{
	// extract the entry number in the BTB table
	uint32_t entry = extract_entry_number(pc,btbSize,tagSize);

	unsigned tmp_state = FSM_table[entry][history];

	return (tmp_state>1) ? true : false ;
}

void FSM_local::updateState(uint32_t pc, uint32_t history, bool taken)
{
	// extract the entry number in the BTB table
	uint32_t entry = extract_entry_number(pc,btbSize,tagSize);

	unsigned tmp_state = FSM_table[entry][history];
	if(taken)
		tmp_state = (tmp_state>2) ? tmp_state : tmp_state+1;	
	else
		tmp_state = (tmp_state<1) ? tmp_state : tmp_state-1;
	
	FSM_table[entry][history] = tmp_state;

}


void FSM_local::restartState(uint32_t pc)
{
	// extract the entry number in the BTB table
	uint32_t entry = extract_entry_number(pc,btbSize,tagSize);
	for (int h = 0; h < 1<<historySize; h++)
	{
		FSM_table[entry][h] = fsmState;
	}
}

// =================== FSM global ==========================
class FSM_global : public FSM
{

public:
	unsigned * FSM_table;

	virtual bool readStatePredict(uint32_t pc, uint32_t history); // TO DO
	virtual void updateState(uint32_t pc, uint32_t history, bool taken);
	virtual void restartState(uint32_t pc);

	FSM_global (unsigned btbSize_,unsigned historySize_,unsigned tagSize_,unsigned fsmState_);
	~FSM_global ();
};

 FSM_global::FSM_global (unsigned btbSize_,unsigned historySize_,unsigned tagSize_,unsigned fsmState_)
{
	btbSize = btbSize_;
	historySize = historySize_;
	tagSize = tagSize_;
	fsmState = fsmState_;
	
	FSM_table = new unsigned[1<<historySize];
	for (int i = 0; i < (1<<historySize); i++)
	{
		FSM_table[i] = fsmState;
	}
}

FSM_global::~FSM_global() 
{
	delete[] FSM_table;
}

bool FSM_global::readStatePredict(uint32_t pc, uint32_t history)
{	
	unsigned tmp_state = FSM_table[history];
	return (tmp_state>1) ? true : false ;
}

void FSM_global::updateState(uint32_t pc, uint32_t history, bool taken)
{
	unsigned tmp_state = FSM_table[history];

	//std::cout << "the history is: " << std::hex << history <<
	//		", the state before is: " << tmp_state;

	if(taken)
		tmp_state = (tmp_state>2) ? tmp_state : tmp_state+1;	
	else
		tmp_state = (tmp_state<1) ? tmp_state : tmp_state-1;
	
	FSM_table[history] = tmp_state;

	//std::cout << ", the state after is: " << tmp_state << " " << std::endl;
}


void FSM_global::restartState(uint32_t pc)
{

}


// ============================= Pred ====================================

class Pred
{
	public:

	unsigned btbSize;
	unsigned historySize; 
	unsigned tagSize;
	unsigned fsmState;
	bool isGlobalHist;
	bool isGlobalTable;
	int isShared;


	BTB* pBTB; // TO DO
	FSM* pFSM;

	int counter_branch;
	int counter_flush;

	Pred(unsigned btbSize_, unsigned historySize_, unsigned tagSize_, unsigned fsmState_,
		bool isGlobalHist_, bool isGlobalTable_, int isShared_);
	~Pred(); // TO DO

};

Pred::Pred(unsigned btbSize_, unsigned historySize_, unsigned tagSize_, unsigned fsmState_,
		bool isGlobalHist_, bool isGlobalTable_, int isShared_)
{
	btbSize = btbSize_;
	historySize = historySize_; 
	tagSize = tagSize_;
	fsmState = fsmState_;
	isGlobalHist = isGlobalHist_;
	isGlobalTable = isGlobalTable_;
	isShared = isShared_;

	if ( isGlobalHist == true )  // GLOBAL
	{
		pBTB = new BTB_global(btbSize,tagSize,historySize); 
	}
	else  // LOCAL
	{
		pBTB = new BTB_local(btbSize,tagSize,historySize); 
	}
	if ( isGlobalTable == true )  // GLOBAL
		pFSM = new FSM_global(btbSize,historySize,tagSize,fsmState);

	else // LOCAL
		pFSM = new FSM_local (btbSize,historySize,tagSize,fsmState);

	counter_branch = 0;
	counter_flush = 0;

};

Pred::~Pred()
{
 	delete pBTB;
	delete pFSM;
}



/* Globals */
Pred* MyPred;

int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int isShare){

	MyPred = new Pred (btbSize, historySize, tagSize, fsmState,
		isGlobalHist, isGlobalTable, isShare);

	return 0;
}

bool BP_predict(uint32_t pc, uint32_t *dst){
	if(MyPred->pBTB->readHistory(pc)==NOT_in_BTB)
	{
		*dst = pc+4;
		return false;
	}
	
	// The branch is in the BTB
	uint32_t tmp_history = MyPred->pBTB->readHistory(pc);
	bool is_taken = MyPred->pFSM->readStatePredict(pc,tmp_history);

	if (is_taken == false)
	{
		*dst = pc+4;
		return false;	
	}

	//The branch is taken
	*dst = MyPred->pBTB->readTarget(pc);
	return true;
}

void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst){

	//update the branch counter
	MyPred->counter_branch++;

	//update the flush counter
	uint32_t tmp_target = MyPred->pBTB->readTarget(pc);
	uint32_t tmp_history = MyPred->pBTB->readHistory(pc);

	bool tmp_taken = MyPred->pFSM->readStatePredict(pc,tmp_history);


	if(tmp_taken != taken) // predict != reality
	{
		if(tmp_taken == true) // predict - T, reality - NT
		{
			if(pc+4!=tmp_target)
				MyPred->counter_flush++;
			
			//else - not flush
		}
		else // predict - NT, reality - T
		{
			if(targetPc!=pc+4)
				MyPred->counter_flush++;
		}
	}
	else // predict = reality
	{
		 if( taken == true) // predict = reality - T
		 {
			 if(targetPc != tmp_target)
			 	MyPred->counter_flush++;
		 }
	}
	//if the branch is not in the BTB
	if(MyPred->pBTB->readHistory(pc)==NOT_in_BTB)
	{
		MyPred->pBTB->addBranch(pc,targetPc); 	//add new branch
		MyPred->pFSM->restartState(pc); // only for local FSM tables
	}
	MyPred->pBTB->updateHistory(pc, taken);
	MyPred->pBTB->updateTarget(pc, targetPc);
	MyPred->pFSM->updateState(pc, tmp_history,taken);	

	return;
}

void BP_GetStats(SIM_stats *curStats){
	curStats->br_num = MyPred->counter_branch;
	curStats->flush_num = MyPred->counter_flush;

	unsigned BTB_size;
	unsigned FSM_size;

	unsigned valid = 0;
	unsigned targetSize = 30;
	unsigned FSM_num_bits = 2;

	if(MyPred->isGlobalHist==false)
		BTB_size = MyPred->btbSize*(valid+MyPred->tagSize+MyPred->historySize+targetSize); //local history
	else
		BTB_size = MyPred->btbSize*(valid+MyPred->tagSize+targetSize)+MyPred->historySize; // global history

	if(MyPred->isGlobalTable==false)
		FSM_size = MyPred->btbSize*(1<<MyPred->historySize)*FSM_num_bits; //local fsm tables
	else
		FSM_size = (1<<MyPred->historySize)*FSM_num_bits; //global fsm tables

	curStats->size = BTB_size + FSM_size;


	return;
}

