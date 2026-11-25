#ifndef __NRF_SPECTRUM_H
#define __NRF_SPECTRUM_H

#include "modules/NRF24/nrf_common.h"
#include <RF24.h>

enum class NrfSpectrumView : uint8_t {
    ALL_CHANNELS = 0,
    ACTIVE_ONLY = 1
};

void nrf_spectrum(NrfSpectrumView view = NrfSpectrumView::ALL_CHANNELS);

String scanChannels(bool web = false);

#endif
