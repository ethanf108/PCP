#include <Arduino.h>


#ifndef __PCP_TASK_LIST_H
#define __PCP_TASK_LIST_H

#define TASK_LIST_SIZE 16

typedef struct {
  union {
    void (*funca)(void*);
    void (*func)();
  };
  void* data;
  unsigned long timeout;
} task_entry;

class TaskList {
private:
  byte length;
  task_entry* list[TASK_LIST_SIZE];
  
  boolean remove(byte i);
  boolean add(task_entry*);
public:
  boolean queueTask(void (*funca)(void*), void* data, unsigned long timeout);
  boolean queueTask(void (*func)(), unsigned long timeout);
  byte size();
  void callTasks();
};

#endif
