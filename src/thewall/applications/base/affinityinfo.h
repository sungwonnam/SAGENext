#ifndef AFFINITYINFO_H
#define AFFINITYINFO_H

//#include <QBitArray>
//#include "common/commondefinitions.h"
#include <QMutex>
#include <QWaitCondition>
#include <QObject>

class SN_RailawareWidget;


/*!
  * There is only one object of this class per widget.
  * However, multiple threads can access this object simultaneously.
  * For instance, AffinityControlDialog reside in MainWindow thread and
  * SagePixelReceiver (reside in a different thread)
  *
  * The following member functions MUST be called in the actual thread that needs affinity info applied.
  * figureOutCurrentAffinity(), applyNewParameters()
  *
  * Any modification on readyXXXMask MUST wait on condition (wait until flag == false)
  * Because flag = true means the thread is applying new settings in readyXXXMask.
  */
class AffinityInfo : public QObject
{
        Q_OBJECT
        Q_PROPERTY(int cpuOfMine READ cpuOfMine WRITE setCpuOfMine)
        Q_ENUMS(MASK_TYPE)
public:
        AffinityInfo(SN_RailawareWidget *widget);
        ~AffinityInfo();

        /*!
          Hardware info. These variables are static because it's a single machine
          */
        static int Num_Numa_Nodes;
        static int Cpu_Per_Numa_Node;
        static int HwThread_Per_Cpu;
        static int SwThread_Per_Cpu;
        static int Num_Cpus;


        enum MASK_TYPE { NODE_MASK, CPU_MASK, MEM_MASK };

        inline SN_RailawareWidget * widgetPtr() {return _widgetPtr;}
        inline void setWidgetPtr(SN_RailawareWidget *w) { _widgetPtr = w;}

        /*!
          * These are called by BaseWidget to show info,and AffinityControlDialog to update text on QLabels.
          * Note that if a thread is in the middle of figureOutCurrentAffinity(), then this could report wrong values!!
          */
        inline const char * getNodeMask() const {return nodeMask;}
        inline const char * getCpuMask() const {return cpuMask;}
        inline const char * getMemMask() const {return memMask;}

        inline int getNumCpus() const {return AffinityInfo::Num_Cpus;}
        inline int cpuOfMain() const {return _cpuOfMain;}
        inline int cpuOfMine() const {return _cpuOfMine;}

        /*!
          This function is called by SagePixelReceiver thread. The thread uses sched_getcpu() system call to find out
          current cpu id to which the thread is attached.

          If applyToSender is true, fsmMsgThread will send railinfo to sail so that sail thread can execute cpu affinity.
          */
        void setCpuOfMine(int c, bool applyToSender = false);

//	void setBit(int type, int which);

        /**
          * This function is used to set single bit in the bitmask
          * type determines if it's for node or cpu
          * which determines node or cpu number in the mask
          */
        void setReadyBit(int type, int which);

        /*!
          This function is used to pass entire bitmask string
          */
        void setReadyBit(int type, const char * const str, int length);
//	void setBits(int type, const char *array, int size);

        /*!
          sets FFFFFFFFFFF
          */
        void setAllReadyBit();

        /*!
          NUMA api provides numa_parse_nodestring() and numa_parse_cpustring() functions
          This takes human readable bitmask string
          */
        void setReadyString(QString &node, QString &mem, QString &cpu);

        inline bool isChanged() const { return flag;}

        /*!
          * This must wait on condition (wait until flag == false)
          */
//	void setFlag();

        void clearBit(int type);

        /*!
          * This is called from AffinityControlDialog, and is read from the thread.
          * When a flag is set, this function should wait until flag resets, because flag means the thread hasn't applied new settings yet!
          */
        void clearReadyBit(int type);

        /*!
         * NUMA API <numa.h> is used in this funciton to set affinity of the CALLING thread.
         * This function must be called within the thread (who wants to apply new affinity settings)
         * It applied new value stored in readyXXXMask
         * Resets the flag before returning.
         */
        void applyNewParameters();

        /*!
          * NUMA API (numa.h> is used in this funciton to find out current affinity settings of the CALLING thread.
          * This function must be called within the thread (who wants to find out affinity settings) otherwise wrong value will be stored.
          */
        void figureOutCurrentAffinity();


        /*!
          * The AffinityControlDialog calls this to send affinity info to SAIL to set sender's affinity. This function support cpu mask only.
          This function emits streamerAffInfoChanged signal which is connected to fsManager's sailSendSetRailMsg signal.

          Note that this should be used only when sage app is running on local machine (which runs SAGE Next)
          */
        void setSageStreamerAffinity(const char *str);

        inline const char * getStreamerCpuMask() const {return streamerCpuMask;}

        inline void setWidgetID(quint64 i) {_widgetID = i;}
        inline quint64 widgetID() const {return _widgetID;}

private:
        /*!
          Pointer to the widget that has this object
          */
        SN_RailawareWidget *_widgetPtr;

        /*!
          * The cpu on which main() is running
          */
        int _cpuOfMain;

        /*!
          * The cpu on which the calling thread is running
          */
        int _cpuOfMine;


        /*!
          Current NUMA node mask, and is updaetd in figureOutCurrentAffinity()
          */
        char *nodeMask;
        char *cpuMask;
        char *memMask;
//	QBitArray nodeMask_; //isNull, isEmpty

        /*!
          Desired NUMA node mask chosen by user and is going to applied by applyNewParameters() when flag is set
          */
        char *readyNodeMask;
        char *readyCpuMask;
        char *readyMemMask;

        /*!
          Desired NUMA node mask chosen by user and is going to applied by applyNewParameters() when flag is set
          */
        QString readyNodeMaskStr;
        QString readyCpuMaskStr;
        QString readyMemMaskStr;

        /*!
          * Holding cpu mask of SAIL, and is updated in setSageStreamerAffinity()
          */
        char *streamerCpuMask;


        /*!
          * if set, thread should apply mask to change affinity settings. This, in fact, is Semaphore. So true means it's in critical section.

          This flag is set whenever readyXXXMask or readyXXXMaskStr have changed.
          This flag is unset in applyNewParameters() after desired affinity info has applied.
          */
        bool flag;

        /*!
          Mutex for waitCond
          */
        QMutex lock;

        /*!
          Wait condition on flag
          */
        QWaitCondition waitCond;


        /*!
          Whether to use char * or QString to store affinity data.
          true for char *
          false for QString (Default)
          */
        bool useBits;

        /*!
          To hold an ID of widget that has this AffinityInfo object.
          SageStreamWidget sets this with sageappid in its constructor.
          */
        quint64 _widgetID;

        /*!
          * Only the thread (affected by affinity) calls this
          * It must call waitCondition::wakeOne()
          */
//	void resetFlag();

signals:
        /*!
          This signal is to let resourceMonitor know widget's current CPU affinity

          * This signal is connected to resourceMonitor::updateAffinityInfo() by GraphicsViewMain::startSageApp()
          * And is emitted in setCpuOfMine(), applyNewParameter()
          */
        void cpuOfMineChanged(SN_RailawareWidget *, int oldvalue, int newvalue);

        /*!
          This signal is to let fsManager sends sender's CPU affinity info chosen by a user through AffinityControlDialog

          * This signal is connected to fsManager::sailSendSetRailMsg() signal by GraphicsViewMain::startSageApp()
          * And is emitted in setStreamerAffinity() which is invoked by AffinityControlDialog
          */
        void streamerAffInfoChanged(AffinityInfo *, quint64 sageappid);

public slots:

};

#endif // AFFINITYINFO_H
