#include "ble_radio_policy.h"

namespace {

bool scan_active = false;

}  // namespace

void bleRadioPolicySetScanActive(bool active) { scan_active = active; }

bool bleRadioPolicyIsScanActive() { return scan_active; }
