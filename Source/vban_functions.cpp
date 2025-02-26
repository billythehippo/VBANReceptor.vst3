#include "vban_functions.h"


int compare_vban_tx_context(vban_plugin_tx_context_t* context1, vban_plugin_tx_context_t* context2)
{
    for (int i=0; i<sizeof(vban_plugin_tx_context_t); i++) if (((uint8_t*)context1)[i]!=((uint8_t*)context1)[i]) return 1;
    return 0;
}
