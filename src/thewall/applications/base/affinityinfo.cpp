#include "affinityinfo.h"
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#if defined(Q_OS_LINUX)
#include <numa.h>
#include <utmpx.h>
#endif


#include <QDebug>

/* initialize static member variables */
int AffinityInfo::Num_Numa_Nodes = 0;
int AffinityInfo::Cpu_Per_Numa_Node = 0;
int AffinityInfo::HwThread_Per_Cpu = 0;
int AffinityInfo::SwThread_Per_Cpu = 0;
int AffinityInfo::Num_Cpus = 0;


AffinityInfo::AffinityInfo(SN_RailawareWidget *p) :
		_widgetPtr(p)
{
	flag = false;
	useBits = false; // by default, QString is used to store affinity info
	_widgetID = 0;

	_cpuOfMain = -1;
	_cpuOfMine = -1;

//	Num_Numa_Nodes = settings->value("system/numnumanodes", 1).toInt();
//	Num_Cpus =  settings->value("system/numcpus",1).toInt();

//	nodeMask_.resize( numNumaNodes );

	nodeMask = (char *)malloc(sizeof(char) *  AffinityInfo::Num_Numa_Nodes + 1);
	cpuMask = (char *)malloc(sizeof(char) * AffinityInfo::Num_Cpus+1);
	memMask = (char *)malloc(sizeof(char) * AffinityInfo::Num_Numa_Nodes +1);
	memset(nodeMask, '1', AffinityInfo::Num_Numa_Nodes);
	memset(cpuMask, '1', AffinityInfo::Num_Cpus);
	memset(memMask, '1', AffinityInfo::Num_Numa_Nodes);
	nodeMask[ AffinityInfo::Num_Numa_Nodes ] = '\0';
	cpuMask[ AffinityInfo::Num_Cpus ] = '\0';
	memMask[ AffinityInfo::Num_Numa_Nodes ] = '\0';

	readyNodeMask = (char *)malloc(sizeof(char) *  AffinityInfo::Num_Numa_Nodes+1);
	readyCpuMask = (char *)malloc(sizeof(char) * AffinityInfo::Num_Cpus+1);
	readyMemMask = (char *)malloc(sizeof(char) * AffinityInfo::Num_Numa_Nodes +1);
	memset(readyNodeMask, '0', AffinityInfo::Num_Numa_Nodes);
	memset(readyCpuMask, '0', AffinityInfo::Num_Cpus);
	memset(readyMemMask, '0', AffinityInfo::Num_Numa_Nodes);
	readyNodeMask[ AffinityInfo::Num_Numa_Nodes ] = '\0';
	readyCpuMask[ AffinityInfo::Num_Cpus ] = '\0';
	readyMemMask[ AffinityInfo::Num_Numa_Nodes ] = '\0';


	streamerCpuMask = (char *)malloc(sizeof(char) * AffinityInfo::Num_Cpus+1);
	memset(streamerCpuMask, '1', AffinityInfo::Num_Cpus);
	streamerCpuMask[ AffinityInfo::Num_Cpus ] = '\0';

//	qDebug("%s::%s() : Instantiated. %d, %d ", metaObject()->className(), __FUNCTION__, AffinityInfo::Num_Numa_Nodes, AffinityInfo::Num_Cpus);
}

AffinityInfo::~AffinityInfo() {
//	QObject::disconnect(this, 0, 0, 0);

	assert(nodeMask && cpuMask && memMask);
	free(nodeMask);
	free(cpuMask);
	free(memMask);
	free(readyNodeMask);
	free(readyCpuMask);
	free(readyMemMask);
	free(streamerCpuMask);
}

void AffinityInfo::setCpuOfMine(int c, bool applyToSender /*false*/) {
	if ( _cpuOfMine != c) {
		int oldvalue = _cpuOfMine;
		_cpuOfMine = c;

		// this signal will trigger calling ResourceMonitor::updateAffInfo()
//		emit affInfoChanged(this);

//		qDebug() << "AffinityInfo::setCpuOfMine() ------------- " << _cpuOfMine;
		emit cpuOfMineChanged(this->widgetPtr(), oldvalue, _cpuOfMine);


		/* send SAIL rail info */
		// find SMT sibling from _cpuOfMine
		if (applyToSender) {
			int smt_sibling;
			if ( _cpuOfMine < 8 ) smt_sibling = _cpuOfMine + 8; // because Intel's SMT core id-ing policy
			else smt_sibling = _cpuOfMine - 8;

			char cpus[AffinityInfo::Num_Cpus]; // 16 cores
			for (int i=0; i<AffinityInfo::Num_Cpus; i++) {
				cpus[i] = '0'; // reset
			}
			cpus[smt_sibling] = '1'; // set

			// emits signal streamerAffInfoChanged connected to fsManager::sendSailSetRail... signal
			setSageStreamerAffinity(cpus); // this function will memcpy 'cpus' to member variable
		}
	}
}

void AffinityInfo::applyNewParameters() {
	/* apply changes in readyXXXmask */
//	char *readyNodeMask;
//	char *readyCpuMask;
//	char *readyMemMask;

#if defined(Q_OS_LINUX)

	if ( useBits ) {
		bool bitChanged = false;
		struct bitmask *mask = numa_allocate_nodemask();
		//	numa_bitmask_clearall(mask);
		for ( int i=0; i<AffinityInfo::Num_Numa_Nodes; ++i ) {
			if ( readyNodeMask[i] == '1' ) {
				bitChanged = true;
				numa_bitmask_setbit(mask, i);
				readyNodeMask[i] = '0'; // reset
			}
		}
		// numa node
		// void numa_bind(struct bitmask *nodemask);
		// int numa_run_on_node(int node);
		// int numa_run_on_node_mask(struct bitmask *);
		if (bitChanged) {
			if ( numa_run_on_node_mask(mask) != 0 ) {
				perror("numa_run_on_node_mask");
				qDebug("AffinityInfo::%s() : numa_run_on_node_mask() failed", __FUNCTION__);
			}

			// mem bind
			// void numa_set_membind(struct bitmask *nodemask);
			// void numa_bind(struct bitmask *nodemask);
			numa_set_membind(mask);
			bitChanged = false; //reset
		}
		numa_free_nodemask(mask);



		/* streamer affinity */
//		setSageStreamerAffinity(readyCpuMask);


		mask = numa_allocate_cpumask();
		for ( int i=0; i<AffinityInfo::Num_Cpus; ++i) {
			if ( readyCpuMask[i] == '1' ) {
				bitChanged = true;
				numa_bitmask_setbit(mask, i);
				readyCpuMask[i] = '0'; // reset
			}
		}
		// cpu affinity
		// int numa_sched_setaffinity(pid_t pid, struct bitmask *);
		if (bitChanged) {
			if ( numa_sched_setaffinity(0, mask) != 0 ) {
				qDebug("AffinityInfo::%s() : numa_sched_setaffinity() failed", __FUNCTION__);
			}
		}
		numa_free_cpumask(mask);
	}

	// QString is used to set affinity
	else {
		struct bitmask *mask = 0;
		if ( !readyNodeMaskStr.isNull()  &&  !readyNodeMaskStr.isEmpty() ) {
			mask = numa_parse_nodestring(readyNodeMaskStr.toAscii().data());
			if ( mask ) {
				if ( mask == numa_no_nodes_ptr ) {
					qDebug() << "numa_no_nodes_ptr";
				}
				else {
					// NUMA node
					if (numa_run_on_node_mask(mask) != 0 ) {
						qDebug("AffinityInfo::%s() : numa_run_on_node_mask() failed", __FUNCTION__);
						perror("numa_run_on_node_mask");
					}
				}
				numa_free_nodemask(mask);
			}
			else {
				qCritical() << "numa_parse_nodestring(nodemask str) returned 0";
			}
			readyNodeMaskStr = ""; // reset
		}
		mask = 0;

		if ( !readyMemMaskStr.isNull() && !readyMemMaskStr.isEmpty() ) {
			mask = numa_parse_nodestring(readyMemMaskStr.toAscii().data());
			if ( mask ) {
				if ( mask == numa_no_nodes_ptr ) {
					qDebug() << "numa_no_nodes_ptr";
				}
				else {
					// MEMBIND
					numa_set_membind(mask);
				}
				numa_free_nodemask(mask);
			}
			else {
				qCritical() << "numa_parse_nodestring(memmask string) returned 0";
			}
			readyMemMaskStr = ""; // reset to empty string
		}
		mask = 0;


		if ( !readyCpuMaskStr.isNull() && !readyCpuMaskStr.isEmpty()) {
			mask = numa_parse_cpustring(readyCpuMaskStr.toAscii().data());
			if ( mask ) {
				if ( mask == numa_no_nodes_ptr ) {
					qDebug() << "numa_no_nodes_ptr";
				}
				else {
					// CPU node
					if (numa_sched_setaffinity(0, mask) != 0 ) {
						qDebug("AffinityInfo::%s() : numa_sched_setaffinity() failed", __FUNCTION__);
						perror("numa_sched_setaffinity");
					}
				}
				numa_free_cpumask(mask);
			}
			else {
				qCritical() << "numa_parse_cpustring(cpumask str) returned 0";
			}
			readyCpuMaskStr = ""; // reset to empty string
		}
		mask = 0;
	}






#endif

	// this signal will trigger calling ResourceMonitor::updateAffInfo()
//	emit affInfoChanged(this);
	emit cpuOfMineChanged(this->widgetPtr(), -1, -1);

	/* flag is set only when there's change that needs to be applied. After applying, reset the flag */
	flag = false;
	waitCond.wakeOne();
}


/**
  * finds out affinity settings of CALLING thread
  * ,and fill the data structure
  */
void AffinityInfo::figureOutCurrentAffinity() {
#if defined(Q_OS_LINUX)
	/* where the main() is running */
	// this shouldn't be called here
//	this->cpuOfMain = sched_getcpu(); // -1 means error

	/* where am I */
//	_cpuOfMine = sched_getcpu();

	/* find out on which node, memory is bound */
//	QByteArray bitmask(numNumaNodes, '0');
	struct bitmask *m1 = numa_get_membind();
	for (int i=0; i<AffinityInfo::Num_Numa_Nodes; ++i ) {
		if ( numa_bitmask_isbitset(m1, i) ) {
//			setBit(MEM_MASK, i); // set node i
			memMask[i] = '1';
		}
		else {
			memMask[i] = '0';
		}
	}

	/* find out current node affinity */
	struct bitmask *m2 = numa_get_run_node_mask();
	for (int i=0; i<AffinityInfo::Num_Numa_Nodes; ++i ) {
		if ( numa_bitmask_isbitset(m2, i) ) {
//			setBit(NODE_MASK, i);
			nodeMask[i] = '1';
		}
		else {
			nodeMask[i] = '0';
		}
	}

	/* find out current cpu affinity */
	struct bitmask *tmp = numa_bitmask_alloc(512);
	numa_bitmask_clearall(tmp);
	numa_sched_getaffinity(0, tmp);
	for (int i=0; i<AffinityInfo::Num_Cpus; ++i ) {
		if ( numa_bitmask_isbitset(tmp, i) ) {
//			setBit(CPU_MASK, i);
			cpuMask[i] = '1';
		}
		else {
			cpuMask[i] = '0';
		}
	}
	numa_bitmask_free(tmp);
#endif
}

//void AffinityInfo::setFlag() {
//	lock.lock();
//	while(flag == true) {
////		QThread::yieldCurrentThread();

//		/* wait until the thread finishes applying new settings
//		   before queueing another new setting
//		   */
//		waitCond.wait(&lock);
//	}
////	qDebug("AffinityInfo::%s() : setting flag true", __FUNCTION__);
//	flag = true;
//	lock.unlock();
//}


/*
void AffinityInfo::resetFlag() {
	flag = false; // only the thread resets this
	waitCond.wakeOne();
}
*/

/*
void AffinityInfo::setBit(int type, int which) {
	// 1000 0000 : node/cpu 0
	 //  1100 0000 : node or cpu 0 and 1
	switch (type) {
	case NODE_MASK:
		nodeMask[which] = '1';
		break;
	case CPU_MASK:
		cpuMask[which] = '1';
		break;
	case MEM_MASK:
		memMask[which] = '1';
		break;
	}
}
*/

void AffinityInfo::setReadyBit(int type, int which) {
	/* 1000 0000 : node or cpu 0
	   1100 0000 : node or cpu 0 and 1
	   */
	lock.lock();
	while(flag == true) {
		waitCond.wait(&lock);
	}

	// flag is now false

	switch (type) {
	case NODE_MASK:
		memset(readyNodeMask, '0', AffinityInfo::Num_Numa_Nodes);
		readyNodeMask[which] = '1';
		break;
	case CPU_MASK:
		memset(readyCpuMask, '0', AffinityInfo::Num_Cpus);
		readyCpuMask[which] = '1';
		break;
	case MEM_MASK:
		memset(readyMemMask, '0', AffinityInfo::Num_Numa_Nodes);
		readyMemMask[which] = '1';
		break;
	}
	useBits = true;
	flag = true;

	lock.unlock();
}

void AffinityInfo::setReadyBit(int type, const char * const str, int length) {
	/* 1000 0000 : node or cpu 0
	   1100 0000 : node or cpu 0 and 1
	   */
	char *mask = 0;
	switch (type) {
	case NODE_MASK:
		assert(length == AffinityInfo::Num_Numa_Nodes);
		mask = readyNodeMask;
		break;
	case CPU_MASK:
		assert(length == AffinityInfo::Num_Cpus);
		mask = readyCpuMask;
		break;
	case MEM_MASK:
		assert(length == AffinityInfo::Num_Numa_Nodes);
		mask = readyMemMask;
		break;
	}
	assert(mask);

	lock.lock();
	while(flag == true) {
		waitCond.wait(&lock);
	}
	// now flag is false (out of critical section) so we can make changes
	memcpy(mask, str, length);
	useBits = true;
	flag = true;
	lock.unlock();
}


void AffinityInfo::setAllReadyBit() {
	lock.lock();
	while(flag == true) {
		waitCond.wait(&lock);
	}
	memset(readyNodeMask, '1', AffinityInfo::Num_Numa_Nodes);
	memset(readyMemMask, '1', AffinityInfo::Num_Numa_Nodes);
	memset(readyCpuMask, '1', AffinityInfo::Num_Cpus);
	useBits = true;
	flag = true;
	lock.unlock();
}

void AffinityInfo::setReadyString(QString &node, QString &mem, QString &cpu) {
	lock.lock();
	while(flag == true) {
		waitCond.wait(&lock);
	}

	/* flag is now false */
	/* make changes */

	/*****************
	  There must be some mechanism to check if string is valid.
	  For example, node string 1 and cpu string 0 wouldn't make sense
	  ************/
	readyNodeMaskStr = node;
	readyCpuMaskStr = cpu;
	readyMemMaskStr = mem;

	useBits = false;

	/* change is done set flag true until this changes are applied */
	flag = true;

	lock.unlock();
//	qDebug("AffinityInfo::%s() : mask str [%s]", __FUNCTION__, qPrintable(maskStr));
}

void AffinityInfo::setSageStreamerAffinity(const char *str) {
	//memset(streamerCpuMask, '0', numCpus);
	memcpy(streamerCpuMask, str, AffinityInfo::Num_Cpus);

	emit streamerAffInfoChanged(this, _widgetID);
}

void AffinityInfo::clearBit(int type) {
	char *mask = 0;
	int size = 0;
	switch (type) {
	case NODE_MASK:
		mask = nodeMask;
		size = AffinityInfo::Num_Numa_Nodes;
		break;
	case CPU_MASK:
		mask = cpuMask;
		size = AffinityInfo::Num_Cpus;
		break;
	case MEM_MASK:
		mask = memMask;
		size = AffinityInfo::Num_Numa_Nodes;
		break;
	}
//	for (int i=0; i<size; ++i ) {
//		mask[i] = '0';
//	}

	memset(mask, '0', size);
}

void AffinityInfo::clearReadyBit(int type) {
	char *mask = 0;
	int size = 0;
	switch (type) {
	case NODE_MASK:
		mask = readyNodeMask;
		size = AffinityInfo::Num_Numa_Nodes;
		break;
	case CPU_MASK:
		mask = readyCpuMask;
		size = AffinityInfo::Num_Cpus;
		break;
	case MEM_MASK:
		mask = readyMemMask;
		size = AffinityInfo::Num_Numa_Nodes;
		break;
	}
	/* WAIT ON CONDITION (wait until flag == false) */
	lock.lock();
	while(flag == true) {
		waitCond.wait(&lock);
	}
	memset(mask, '0', size);
	lock.unlock();
}

//void AffinityInfo::setBits(int type, const char *array, int size) {
//	char *mask = 0;

//	switch (type) {
//	case NODE_MASK:
//		mask = nodeMask;
//		break;
//	case CPU_MASK:
//		mask = cpuMask;
//		break;
//	case MEM_MASK:
//		mask = memMask;
//		break;
//	}

//	int numNumaNodes = settings->value("system/numnumanodes").toInt();
//	int numCpus = settings->value("system/numcpus",1).toInt();

//	if ( type == NODE_MASK || MEM_MASK   &&  size <numNumaNodes  ) {
//		loopc = size;
//	}
//	else if ( type == CPU_MASK  &&  size < numCpus ) {
//		loopc = size;
//	}

//	for (int i=0; i<loopc; ++i ) {
//		mask[i] = array[i];
//	}
//}

