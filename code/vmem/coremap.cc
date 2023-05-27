#include "coremap.hh"

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