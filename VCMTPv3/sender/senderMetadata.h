#ifndef SENDERMETADATA_H_
#define SENDERMETADATA_H_

#include <set>
#include <map>
#include <pthread.h>

using namespace std;

struct RetxMetadata {
	uint32_t  prodindex;
    /** recording the whole product size (for timeout factor use) */
    uint32_t  prodLength;
    clock_t   mcastStartTime;	 /*!< multicasting start time */
    clock_t   mcastEndTime; 	 /*!< multicasting end time */
    float	  retxTimeoutRatio;	 /*!< ratio to scale timeout time */
    double	  retxTimeoutTime; 	 /*!< timeout time in seconds */
    void* 	  dataprod_p; 		 /*!< pointer to the data product */
    set<int>  unfinReceivers;	 /*!< unfinished receiver set indexed by socket id */


    RetxMetadata(): prodindex(0), prodLength(0), mcastStartTime(0.0),
    				mcastEndTime(0.0), retxTimeoutRatio(20.0),
					retxTimeoutTime(99999999999.0), dataprod_p(NULL) {}
    virtual ~RetxMetadata() {}
};


class senderMetadata {
public:
	senderMetadata();
	~senderMetadata();

	//void 	RemoveMessageMetadata(uint msg_id);
	//void 	ClearAllMetadata();
	void addRetxMetadata(RetxMetadata* ptrMeta);
	RetxMetadata* getMetadata(uint32_t prodindex);
	bool isRetxAllFinished(uint32_t prodindex);
	void removeFinishedReceiver(uint32_t prodindex, int retxsockfd);

private:
    /** first: prodindex; second: pointer to metadata of the specified prodindex */
	map<uint32_t, RetxMetadata*> indexMetaMap;
	pthread_rwlock_t 		     indexMetaMapLock;
	//pthread_rwlock_t 		     metadata_lock;
};

#endif /* SENDERMETADATA_H_ */
