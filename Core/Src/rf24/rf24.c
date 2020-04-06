#include "rf24_module/rf24_module.h"
#include "rf24_network/rf24_mac.h"
#include "rf24_network/rf24_csma_ca.h"
#include "rf24.h"
#include "rf24_network/rf24_worker.h"
#include "rf24_network/rf24_network.h"

void rf24_init()
{
	rf24_network_init();
}


