#ifndef DOBIESTATION_FAIRMUTEX_H
#define DOBIESTATION_FAIRMUTEX_H

#include <queue>
#include <condition_variable>
#include <mutex>
#include <cassert>


struct WaitingThread {
  std::condition_variable run_cv;
  bool can_run = false;

  bool* wakeup_done = nullptr;
  std::condition_variable* wakeup_cv = nullptr;
};


class FairMutex {
public:


  void lock() {

    // acquire mutex so we can run this section without other threads
    std::unique_lock<std::mutex> lock(mut);
    if(is_locked) {
      // we should wait our turn.
      // create our record on our stack...
      WaitingThread this_thread;
      // and add it to the back of the line
      queue.push(&this_thread);

      // wait our turn...
      while(!this_thread.can_run) {
        this_thread.run_cv.wait(lock);
      }

      // make sure we are in front of the line
      assert(queue.front() == &this_thread);

      // and remove us from the queue.
      // note that we remove ourselves from the queue so we don't
      // do this before we are woken up
      queue.pop();

      // and let the unlocker know we are done
      // we need him to hold the mutex until the pop is done
      *this_thread.wakeup_done = true;
      this_thread.wakeup_cv->notify_one();
    } else {
      // nobody locked it yet, so we get it!
      is_locked = true;
    }
  }

  void unlock() {
    std::unique_lock<std::mutex> lock(mut);

    if(queue.empty()) {
      // nobody is waiting, so it's free now
      is_locked = false;
    } else {
      // notify the first in line, who will wake up and kill themselves.
      bool done = false;
      std::condition_variable wakeup_cv;
      queue.front()->wakeup_done = &done;
      queue.front()->wakeup_cv = &wakeup_cv;
      queue.front()->can_run = true;
      queue.front()->run_cv.notify_one();
      while(!done) {
        wakeup_cv.wait(lock);
      }
    }
  }

private:
  std::queue<WaitingThread*> queue;
  std::mutex mut;
  bool is_locked = false;
};

class FairMutexLock {
public:
  FairMutexLock(FairMutex& fm) : _fm(fm) {
    _fm.lock();
  }

  ~FairMutexLock() {
    _fm.unlock();
  }

private:
  FairMutex& _fm;
};

#endif //DOBIESTATION_FAIRMUTEX_H
