#include "common.h"

#include "SmallVector.h"
#include <cstdint>

SC_NAMESPACE_BEGIN

// Note: Moving this function into the header may cause performance regression.
static SC_SIZE_TYPE getNewCapacity(SC_SIZE_TYPE MinSize, SC_SIZE_TYPE OldCapacity) {
    constexpr SC_SIZE_TYPE MaxSize = std::numeric_limits<SC_SIZE_TYPE>::max();

    SC_ASSERT(MaxSize >= MinSize);
    SC_ASSERT(OldCapacity != MaxSize);

    // Overflow
    SC_ASSERT(SC_SIZE_TYPE(double(MaxSize - 1) / SC_CAPACITY_STEP) >= OldCapacity);

    SC_SIZE_TYPE NewCapacity = SC_CAPACITY_STEP * OldCapacity + 1; // Always grow.
    return std::min(std::max(NewCapacity, MinSize), MaxSize);
}

// Note: Moving this function into the header may cause performance regression.
void SmallVectorBase::grow_pod(void* FirstEl, SC_SIZE_TYPE MinSize, SC_SIZE_TYPE TSize) 
{
    SC_SIZE_TYPE NewCapacity = getNewCapacity(MinSize, this->capacity());
    void* NewElts;
    if (BeginX == FirstEl) {
        NewElts = malloc(static_cast<size_t>(NewCapacity) * TSize);
        SC_ASSERT(NewElts);

        // Copy the elements over.  No need to run dtors on PODs.
        memcpy(NewElts, this->BeginX, static_cast<size_t>(size()) * TSize);
    }
    else {
        // If this wasn't grown from the inline copy, grow the allocated space.
        NewElts = realloc(this->BeginX, static_cast<size_t>(NewCapacity) * TSize);
    }

    this->BeginX = NewElts;
    this->Capacity = NewCapacity;
}

SC_NAMESPACE_END