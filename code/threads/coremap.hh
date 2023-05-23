#ifndef NACHOS_THREADS_COREMAP__HH
#define NACHOS_THREADS_COREMAP__HH

#include "lib/bitmap.hh"
#include "thread.hh"
#include "system.hh"
#include "lib/list.hh"

class TranslationEntry
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
private:
   Bitmap *physicals;
   TranslationEntry* entries;
   List<unsigned>* order;

   unsigned numPages;
};

Coremap::Coremap(unsigned numPages_)
{
   numPages = numPages_;
   physicals = new Bitmap(numPages);
   order = new List<unsigned>();
   entries = new TranslationEntry[numPages];
}

Coremap::~Coremap()
{
   delete physicals;
   delete [] entries;
}

unsigned
Coremap::Find(unsigned virtualPage){
   int page = physicals->Find();
   if(page < 0) {
      page = order->Pop();
      ASSERT(entries[page].process->space != nullptr);
      entries[page].process->space->SwapPage(entries[page].virtualPage);
   }
   entries[page].process = currentThread;
   entries[page].virtualPage = virtualPage;
   order->Append(page);
   return page;
}

void
Coremap::Clear(unsigned physicalPage){
   if(!physicals->Test(physicalPage)){
      return;
   }
   order->Remove(physicalPage);
   physicals->Clear(physicalPage);
}


#endif