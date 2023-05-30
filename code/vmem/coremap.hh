#ifndef NACHOS_THREADS_COREMAP__HH
#define NACHOS_THREADS_COREMAP__HH

#include "lib/bitmap.hh"
#include "threads/thread.hh"
#include "threads/system.hh"
#include "lib/list.hh"

class Entry
{
public:
   unsigned virtualPage;
   // unsigned physicalPage;
   Thread* process;
   // TranslationEntry(Thread* process_,unsigned virtualPage_, unsigned physicalPage_);
};

// TranslationEntry::TranslationEntry(Thread* process_,unsigned virtualPage_, unsigned physicalPage_){
//    virtualPage = virtualPage_;
//    physicalPage = physicalPage_;
//    process = process_;
// }


class Coremap
{
public:
   Coremap(unsigned numPages_);
   ~Coremap();
   unsigned Find(unsigned virtualPage);
   void Clear(unsigned virtualPage);
   unsigned CountClear();
   void Get(unsigned physicalPage);
private:
   Bitmap *physicals;
   Entry* entries;
   List<unsigned>* order;

   unsigned numPages;
};


#endif