#ifndef _PRES_DFKS_H_
#define _PRES_DFKS_H_
#include "../../modules/sl/sl.h"
#include "../presence/bind_presence.h"
#include "../../modules/tm/tm_load.h"
#include "../pua/pidf.h"

extern add_event_t pres_add_event;
extern sl_api_t slb;
extern struct tm_binds tmb;
extern presence_api_t pres;
extern libxml_api_t libxml_api;
extern str outbound_proxy;

#endif
