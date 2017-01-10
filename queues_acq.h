#ifndef QUEUES_H_
#define QUEUES_H_

#include <atomic>
#include <deque>
#include <mutex>
#include <iostream>
#include <sstream>

// Defines the interface for the Fixed Size Queue
template<typename T>
class FixedSizeQueueInterface {
 public:
  virtual ~FixedSizeQueueInterface() { }
  virtual bool Read(T* data) = 0;
  virtual bool Write(const T& data) = 0;
};

// Implements a fixed-size queue using a Mutex
/* Update this to use reader writer locks */
template<typename T>
class MutexFixedSizeQueue : public FixedSizeQueueInterface<T> {
 public:

  // Simple helper class to ensure a lock is unlocked once the scope is exited
  // original code
  class ScopedLock {
   public:
    ScopedLock(std::mutex* mutex) : mutex_(mutex) {
      mutex_->lock();
    }
    ~ScopedLock() {
      mutex_->unlock();
    }
   private:
    std::mutex* mutex_;
  };

  class ScopedWriteLock {
   public:
    ScopedWriteLock(std::mutex* wr_mutex) : mutex_(wr_mutex) {
      mutex_->lock();
    }
    ~ScopedWriteLock() {
      mutex_->unlock();
    }
   private:
    std::mutex* mutex_;
  };

  class ScopedReadLock {
   public:
    ScopedReadLock(std::mutex* rd_mutex) : mutex_(rd_mutex) {
      mutex_->lock();
    }
    ~ScopedReadLock() {
      mutex_->unlock();
    }
   private:
    std::mutex* mutex_;
  };


  MutexFixedSizeQueue(int max_size) : max_size_(max_size) { }

  // Reads the next data item into 'data', returns true
  // if successful or false if the queue was empty.
  bool Read(T* data) {
    ScopedReadLock Rlock(&rd_mutex);
    if(queue_.empty()) {
	return false;
    }
    while(queue_.size()==1){
     if(wr_mutex.try_lock()){
        *data = queue_.front();
        queue_.pop_front();
        wr_mutex.unlock();
        return true;
     }else{std::cout<<"\nrnayar size: "<<queue_.size();}}
   // if(queue_.size()>max_size_){
   //     queue_.pop_front();
   //     return false;
   // }else 
	if(queue_.size()>1){
    	*data = queue_.front();
        queue_.pop_front();
        return true;
    }
    
  }

  // Writes 'data' into the queue.  Returns true if successful
  // or false if the queue was full.
  bool Write(const T& data) {
    ScopedWriteLock Wlock(&wr_mutex);
    if (queue_.size() >=max_size_) {
      return false;
    }
    queue_.push_back(data);
    return true;
  }

  bool isEmpty() {
      return queue_.empty();
  }

 private:
  std::deque<T> queue_;
  int max_size_;
  //std::mutex mutex_;
  std::mutex wr_mutex;
  std::mutex rd_mutex;
};



// Implements a fixed-size queue using no lock, but limited to a single
// producer thread and a single consumer thread.
template<typename T>
class SingleProducerSingleConsumerFixedSizeQueue : public FixedSizeQueueInterface<T> {
 public:
  SingleProducerSingleConsumerFixedSizeQueue(int max_size)
    : max_size_(max_size),
      buffer_(new Entry[max_size]),
      head_(0),
      tail_(0) {
    /* Needs to be updated */      
    for (int i = 0; i < max_size; ++i) {
      buffer_[i].valid = false;
    }
  }

  ~SingleProducerSingleConsumerFixedSizeQueue() {
    delete[] buffer_;
  }

  virtual bool Read(T* w) {
    /* Needs to be updated */
      isLockFree();
      int current_head = head_;
      int next_head = ((head_+1)%max_size_);
      if(isEmpty()|| (buffer_[current_head].valid == false))
        return false;
      if(buffer_[current_head].valid){
      	*w=buffer_[current_head].data;
      	buffer_[current_head].valid = false;
        head_ = next_head;
      	return true;
      }
  }

  virtual bool Write(const T& w) {
    /* Needs to be updated */ 
      isLockFree();
      int current_tail = tail_;
      int next_tail = ((tail_+1)%max_size_);
      if(next_tail != head_)
      {
	buffer_[current_tail].data = w;
        buffer_[current_tail].valid = true;
        tail_=next_tail;
        return true;
      }   
      return false;
  }

  bool isEmpty() {
      if (head_ == tail_ && !buffer_[tail_].valid) 
          return true;
      else
          return false;
  }

  void isLockFree(){
  if(!head_.is_lock_free())
  	std::cout<<"\nhead_ atomic type is not lock free is SignleProducerSingleConsumer\n";
  
  if(!tail_.is_lock_free())
  	std::cout<<"\ntail_ atomic type is not lock free is SignleProducerSingleConsumer\n";
  }

 private:
  struct Entry {
    T data;
    bool valid; 
  };

  int max_size_;
  Entry* buffer_;
  std::atomic_int head_ __attribute__((aligned(64)));
  std::atomic_int tail_ __attribute__((aligned(64)));
};


// Implements a fixed-size queue using no lock, but limited to a single
// producer thread and multiple consumer threads.
template<typename T>
class SingleProducerMultipleConsumerFixedSizeQueue : public FixedSizeQueueInterface<T> {
 public:
  SingleProducerMultipleConsumerFixedSizeQueue(int max_size)
    : max_size_(1000),
      buffer_(new Entry[max_size]),
      head_(0),
      tail_(0) {
    /* Needs to be updated */      
    for (int i = 0; i < max_size; ++i) {
      buffer_[i].valid = false;
    }
  }

  ~SingleProducerMultipleConsumerFixedSizeQueue() {
    delete[] buffer_;
  }

  /*virtual bool Read(T* w) {
      int current_head; 
      int next_head;
    do{ isLockFree();     
      current_head = head_;
      std::cout<<"\n inside do while"<<head_<<" "<<tail_;
      next_head = ((head_+1)%max_size_);
      if(isEmpty()|| (buffer_[current_head].valid == false))
        return false;
       } while(!(head_.compare_exchange_strong(current_head,next_head))); 
      if(buffer_[current_head].valid){
      	*w=buffer_[current_head].data;
      	buffer_[current_head].valid = false;
      	return true;
       }
       else false;
  }*/
  virtual bool Read(T* w) {
      int current_head; 
      int next_head;
    do{ isLockFree();     
      current_head = head_;
      next_head = ((head_+1)%max_size_);
      if(isEmpty()|| (buffer_[current_head].valid == false))
        return false;
      std::stringstream msg;
      msg<<"\n inside do while head:"<<head_<<" tail: "<<tail_<<"current_head :"<<current_head<<"next_head :"<<next_head;
      //std::cout<< msg.str();
       } while(!(head_.compare_exchange_strong(current_head,next_head,std::memory_order_acq_rel))); 
      if(buffer_[current_head].valid){
      	*w=buffer_[current_head].data;
      	buffer_[current_head].valid = false;
      	return true;
       }
       else return false;
  }
/*
  virtual bool Read(T* w) {
      isLockFree();
      int current_head = head_;
      int next_head = ((head_+1)%max_size_);
      std::cout<<"\nin read head:"<<head_<<" and"<<tail_<<" next_head"<<next_head;
      if(isEmpty()|| (buffer_[current_head].valid == false))
        return false;
      if(buffer_[current_head].valid){
      	*w=buffer_[current_head].data;
      	buffer_[current_head].valid = false;
        head_ = next_head;
      	return true;
      }
  }
*/
  virtual bool Write(const T& w) {
     isLockFree();     
      int current_tail = tail_;
      int next_tail = ((tail_+1)%max_size_);
      std::stringstream msg;
	msg <<"\nin write head:"<<head_<<" and tail"<<tail_<<" next_tail"<<next_tail;
      //std::cout<<msg.str();
      if(next_tail != head_)
      {
	buffer_[current_tail].data = w;
        buffer_[current_tail].valid = true;
        tail_=next_tail;
        return true;
      }   
      return false;
  }

/*  virtual bool Write(const T& w) {
      int current_tail;
      int next_tail;  
    do{
      isLockFree();
      current_tail = tail_;
      next_tail = ((tail_+1)%max_size_);
      if(next_tail == head_){
        return false;}
      std::stringstream msg;
	msg <<"\nin write head:"<<head_<<" and tail"<<tail_<<" next_tail"<<next_tail;
      std::cout<<msg.str();
      } while(!(tail_.compare_exchange_strong(current_tail,next_tail)));
	buffer_[current_tail].data = w;
        buffer_[current_tail].valid = true;
      return true;   
  }
*/
  bool isEmpty() {
      if (head_ == tail_ && !buffer_[tail_].valid) 
          return true;
      else
          return false;
  }
  void isLockFree(){
  if(!head_.is_lock_free())
  	std::cout<<"\nhead_ atomic type is not lock free is SignleProducerMultipleConsumer\n";
  
  if(!tail_.is_lock_free())
  	std::cout<<"\ntail_ atomic type is not lock free is SignleProducerMultipleConsumer\n";
  }


 private:
  struct Entry {
    T data;
    bool valid; 
  };

  int max_size_;
  Entry* buffer_;
  std::atomic_int head_ __attribute__((aligned(64)));
  std::atomic_int tail_ __attribute__((aligned(64)));
};


// Implements a fixed-size queue using no lock, but limited to multiple 
// producer threads and a single consumer thread.
template<typename T>
class MultipleProducerSingleConsumerFixedSizeQueue : public FixedSizeQueueInterface<T> {
 public:
  MultipleProducerSingleConsumerFixedSizeQueue(int max_size)
    : max_size_(max_size),
      buffer_(new Entry[max_size]),
      head_(0),
      tail_(0) {
    /* Needs to be updated */      
    for (int i = 0; i < max_size; ++i) {
      buffer_[i].valid = false;
    }
  }

  ~MultipleProducerSingleConsumerFixedSizeQueue() {
    delete[] buffer_;
  }

  virtual bool Read(T* w) {
    /* Needs to be updated */      
      isLockFree();
      int current_head = head_;
      int next_head = ((head_+1)%max_size_);
      if(isEmpty()|| (buffer_[current_head].valid == false))
        return false;
      if(buffer_[current_head].valid){
      	*w=buffer_[current_head].data;
      	buffer_[current_head].valid = false;
        head_ = next_head;
      	return true;
      }
  }

  virtual bool Write(const T& w) {
    /* Needs to be updated */    
      int current_tail;
      int next_tail;  
    do{
      isLockFree();
      current_tail = tail_;
      next_tail = ((tail_+1)%max_size_);
      if(next_tail == head_){
        return false;}
      } while(!(tail_.compare_exchange_strong(current_tail,next_tail,std::memory_order_acq_rel)));
	buffer_[current_tail].data = w;
        buffer_[current_tail].valid = true;
      return true;   
  }

  bool isEmpty() {
      if (head_ == tail_ && !buffer_[tail_].valid) 
          return true;
      else
          return false;
  }

  void isLockFree(){
  if(!head_.is_lock_free())
  	std::cout<<"\nhead_ atomic type is not lock free is MultipleProducerSingleConsumer\n";
  
  if(!tail_.is_lock_free())
  	std::cout<<"\ntail_ atomic type is not lock free is MultipleProducerSingleConsumer\n";
  }

 private:
  struct Entry {
    T data;
    bool valid; 
  };

  int max_size_;
  Entry* buffer_;
  std::atomic_int head_ __attribute__((aligned(64)));
  std::atomic_int tail_ __attribute__((aligned(64)));
};


#endif  // QUEUES_H_
