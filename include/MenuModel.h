#pragma once
#include "UiState.h"

struct MenuItem { const char* label; Page target; };

int  menuItemCount(Page p);
const MenuItem& menuItemAt(Page p, int i);
void menuActivate(Page p, int i);