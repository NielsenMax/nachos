#ifndef NACHOS_LIB_LRU__HH
#define NACHOS_LIB_LRU__HH

#include "list.hh"
#include "table.hh"

template <class Item>
class LRU
{
public:
    LRU();
    ~LRU();

    // Add an item and return it index
    int Add(Item i);

    // Get an item by index and update it freshness
    Item Get(int i);

    // Check if an item is present
    bool HasKey(int i) const;

    // Check if the LRU is empty
    bool IsEmpty() const;

    // Remove the least fresh item
    Item Pop();

    // Remove an item by index
    Item Remove(int i);
    
private:
    List<int> *order;
    Table<Item> *items;
};

template <class Item>
LRU<Item>::LRU()
{
    order = new List<int>();
    items = new Table<Item>()
}

template <class Item>
LRU<Item>::~LRU()
{
    delete order;
    delete items;
}

template <class Item>
int LRU<Item>::Add(Item i) {
    int index = items->Add(i);
    if(index == -1){
        return index;
    }
    order->Append(index);
    return index;
}

template <class Item>
bool LRU<Item>::HasKey(int i) const {
    return items->HasKey(i);
}

template <class Item>
bool LRU<Item>::IsEmpty() const {
    return items->IsEmpty();
}

template <class Item>
Item LRU<Item>::Get(int i) {
    if(!HasKey(i)){
        return T();
    }
    order->Remove(i);
    order->Append(i);
    return items->Get(i);
}

template <class Item>
Item LRU<Item>::Pop() {
    if(items->IsEmpty()){
        return Item();
    }
    int i = order->Pop();
    return items->Remove(i);
}

template <class Item>
Item LRU<Item>::Remove(int i) {
    if(!items->HasKey()){
        return Item();
    }
    order->Remove(i);
    return items->Remove(i);
}


#endif