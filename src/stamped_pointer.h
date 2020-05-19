void * getUnstampedPointer(void *p);
uint16_t getStamp(void *p);
void resetStamp(void ** p);
void setStamp(void ** p, uint16_t v);
void incrementStamp(void ** p);