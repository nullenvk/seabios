// Rudimentary support for LPC Interface Bridges
//
// Copyright (C) 2022 nullenvk 
//
// This file may be distributed under the terms of the GNU LGPLv3 license.

#include "byteorder.h" // cpu_to_*
#include "types.h"
#include "config.h" 
#include "output.h"
#include "pcidevice.h"
#include "pci_ids.h"
#include "pci.h"

#include "lpc.h"

#define PCH_BIOS_CNTL_OFFSET 0xDC

const u8 PCH_LPC_BIOS_BIOSWE = (1 << 0); // BIOS Write Enable
const u8 PCH_LPC_BIOS_BLE = (1 << 1); // BIOS Lock Enable
const u8 PCH_LPC_BIOS_SMM_BWP = (1 << 5); // SMM Bios Write Protect Enable

enum lpc_features {
    LPC_FEATURE_FLASH_WPROTECT = (1 << 0),
};

static u16 pch_lpc_device_ids[] = {
    PCI_DEVICE_ID_INTEL_LPT_QM87
};

static struct {
    u16 bdf;
} pch_lpc_state;

int pch_lpc_probe(void) {
    int found = 0;

    struct pci_device *pci;
    foreachpci(pci) {
        if(pci->vendor != PCI_VENDOR_ID_INTEL)
            continue;

        size_t idcount = sizeof(pch_lpc_device_ids)/sizeof(*pch_lpc_device_ids);
        for(size_t i = 0; i < idcount; i++) {
            if(pci->device == pch_lpc_device_ids[i]) {
                found = 1;
                break;
            }
        }

        if(found)
            break;
    }

    if(!found)
        return 0;

    pch_lpc_state.bdf = pci->bdf;

    return found;
}

enum lpc_features pch_lpc_get_features(void) {
    return LPC_FEATURE_FLASH_WPROTECT;
}

void pch_lpc_enable_wprotect(void) {
    u8 bios_cntl = pci_config_readb(pch_lpc_state.bdf, PCH_BIOS_CNTL_OFFSET);

    bios_cntl &= PCH_LPC_BIOS_BIOSWE ^ 0xFF;
    bios_cntl |= PCH_LPC_BIOS_SMM_BWP;
    bios_cntl |= PCH_LPC_BIOS_BLE;

    pci_config_writeb(pch_lpc_state.bdf, PCH_BIOS_CNTL_OFFSET, bios_cntl);
}

struct lpc_driver {
    int (*probe)(void);
    enum lpc_features (*get_features)(void);

    void (*enable_wprotect)(void);
};

enum lpc_driver_num {
    LPC_DRIVER_INTEL_PCH = 0,

    LPC_DRIVERS_NUM,
};

static struct lpc_driver lpc_drivers[LPC_DRIVERS_NUM] = {
    [LPC_DRIVER_INTEL_PCH] =
    {
        .probe = pch_lpc_probe,
        .get_features = pch_lpc_get_features,
        
        .enable_wprotect = pch_lpc_enable_wprotect,
    },
};

static struct lpc_driver *lpc_drv = NULL;

void lpc_setup(void) {
    for(size_t i = 0; i < LPC_DRIVERS_NUM; i++) {
        struct lpc_driver *drv = &lpc_drivers[i];
        if (drv->probe()) {
            lpc_drv = drv;
            break;
        }
    }
}

int lpc_is_detected(void) {
    return lpc_drv != NULL;
}

enum lpc_features lpc_get_features(void) {
    return lpc_is_detected() ? lpc_drv->get_features() : 0;
}

int lpc_can_wprotect(void) {
    return lpc_get_features() & LPC_FEATURE_FLASH_WPROTECT;
}

void lpc_wprotect(void) {
    if (! CONFIG_LPCPROTECT)
        return;

    if (! lpc_can_wprotect())
        return;

    lpc_drv->enable_wprotect();
}
