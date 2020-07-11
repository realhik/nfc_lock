#ifndef NFCLOCK_HELPERS_H
#define NFCLOCK_HELPERS_H

#include <stdbool.h>
#include <stdint.h>
#include <string.h> // memcpy
#include <freefare.h>

uint8_t nfclock_applicationsettings(uint8_t accesskey, bool frozen, bool req_auth_fileops, bool req_auth_dir, bool allow_master_key_chg);

// This basically only wraps the MDAR -macro
uint16_t nfclock_fileaccessrights(uint8_t readkey, uint8_t writekey, uint8_t rwkey, uint8_t aclkey);

int nfclock_write_uint32(FreefareTag tag, uint8_t fileid, uint32_t data);
int nfclock_read_uint32(FreefareTag tag, uint8_t fileid, uint32_t *data);


#endif
