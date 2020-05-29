void * getPointer(void *p);
bool getFlag(void *p);
void setFlag(void ** p);
void resetFlag(void ** p);
uint16_t getStamp(void *p);
void resetStamp(void ** p);
void setStamp(void ** p, uint16_t v);
void incrementStamp(void ** p);