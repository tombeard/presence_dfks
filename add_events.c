#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libxml/parser.h>
#include "../../core/parser/parse_content.h"
#include "../presence/event_list.h"
#include "presence_dfks.h"
#include "add_events.h"

static str pu_415_rpl  = str_init("Unsupported media type");
static str unk_dev = str_init("<notKnown/>");

int dfks_add_events(void)
{
	pres_ev_t event;

	memset(&event, 0, sizeof(pres_ev_t));
	event.name.s = "as-feature-event";
	event.name.len = 16;

	event.content_type.s = "application/x-as-feature-event+xml";
	event.content_type.len = 34;
	event.default_expires= 3600;
	event.type = PUBL_TYPE;
	event.req_auth = 0;

	/* register event handlers */
	event.evs_publ_handl = dfks_publ_handler;
	event.evs_subs_handl = dfks_subs_handler;

	if (pres.add_event(&event) < 0) {
		LM_ERR("failed to add event \"as-feature-event\"\n");
		return -1;
	}

	return 0;
}

int dfks_publ_handler(struct sip_msg* msg) {
	str body= {0, 0};
	xmlDocPtr doc= NULL;

	LM_DBG("dfks_publ_handler start\n");
	if ( get_content_length(msg) == 0 )
		return 1;

	body.s=get_body(msg);
	if (body.s== NULL) {
		LM_ERR("cannot extract body from msg\n");
		goto error;
	}

	/* content-length (if present) must be already parsed */
	body.len = get_content_length( msg );
	doc= xmlParseMemory( body.s, body.len );
	if(doc== NULL)
	{
		LM_ERR("bad body format\n");
		if(slb.freply(msg, 415, &pu_415_rpl) < 0)
		{
			LM_ERR("while sending '415 Unsupported media type' reply\n");
		}
		goto error;
	}
	xmlFreeDoc(doc);
	xmlCleanupParser();
	xmlMemoryDump();
	return 1;

error:
	xmlFreeDoc(doc);
	xmlCleanupParser();
	xmlMemoryDump();
	return -1;
}

int dfks_subs_handler(struct sip_msg* msg, int *suppress_fast_notify) {
	str body= {0, 0};
	xmlDocPtr doc= NULL;
	xmlNodePtr top_elem= NULL;
	xmlNodePtr param = NULL;
	char *dndact=NULL,*fwdact=NULL,*fwdtype=NULL,*fwdDN=NULL,*device=NULL;

	LM_DBG("dfks_subs_handler start\n");

	if ( get_content_length(msg) == 0 ){
		LM_DBG("no body. (ok for initial subscribe)\n");
		return 1;
	}
	body.s=get_body(msg);
	if (body.s== NULL)
	{
		LM_ERR("cannot extract body from msg\n");
		goto error;
	}

	/* suppress fast notify to avoid sending conflicting replies when
	 	using an async method to process a change of state
		- this should be moved to a callback */
	LM_WARN("suppressing fast notify reply for subscribe with body\n");
	*suppress_fast_notify = 1;

	/* content-length (if present) must be already parsed */
	body.len = get_content_length( msg );
	doc=xmlParseMemory( body.s, body.len );
	if(doc== NULL)
	{
		LM_ERR("bad body format\n");
		if(slb.freply(msg, 415, &pu_415_rpl) < 0)
		{
			LM_ERR("while sending '415 Unsupported media type' reply\n");
		}
		goto error;
	}
	top_elem=libxml_api.xmlDocGetNodeByName(doc, "SetDoNotDisturb", NULL);
	if(top_elem != NULL) {
		LM_INFO(" got SetDoNotDisturb\n");
		param = libxml_api.xmlNodeGetNodeByName(top_elem, "doNotDisturbOn", NULL);
		if(param!= NULL) {
			dndact= (char*)xmlNodeGetContent(param);
			if(dndact== NULL)  {
				LM_ERR("while extracting value from 'doNotDisturbOn' in 'SetDoNotDisturb'\n");
				goto error;
			}
			LM_INFO("got 'doNotDisturbOn'=%s in 'SetDoNotDisturb'\n",dndact);
		}
		param = libxml_api.xmlNodeGetNodeByName(top_elem, "device", NULL);
		if(param!= NULL) {
			device= (char*)xmlNodeGetContent(param);
			if(device== NULL)  {
				LM_ERR("while extracting value from 'device' in 'SetDoNotDisturb'\n");
				goto error;
			}
			if (strlen(device)==0)
			    device=unk_dev.s;
			LM_INFO("got 'device'=%s in 'SetDoNotDisturb'\n",device);
		}
		pkg_free(body.s);
	}
	top_elem=libxml_api.xmlDocGetNodeByName(doc, "SetForwarding", NULL);
	if(top_elem != NULL) {
		LM_INFO(" got SetForwarding\n");
		param = libxml_api.xmlNodeGetNodeByName(top_elem, "forwardDN", NULL);
		if(param!= NULL) {
			fwdDN= (char*)xmlNodeGetContent(param);
			if(fwdDN== NULL) {
				LM_ERR("while extracting value from 'forwardDN' in 'SetForwarding'\n");
				goto error;
			}
			LM_INFO("got 'forwardDN'=%s in 'SetForwarding'\n",fwdDN);
		}
		param = libxml_api.xmlNodeGetNodeByName(top_elem, "forwardingType", NULL);
		if(param!= NULL) {
			fwdtype= (char*)xmlNodeGetContent(param);
			if(fwdtype== NULL) {
				LM_ERR("while extracting value from 'forwardingType' in 'SetForwarding'\n");
				goto error;
			}
			LM_INFO("got 'forwardingType'=%s in 'SetForwarding'\n",fwdtype);
		}
		param = libxml_api.xmlNodeGetNodeByName(top_elem, "activateForward", NULL);
		if(param!= NULL) {
			fwdact= (char*)xmlNodeGetContent(param);
			if(fwdact== NULL) {
				LM_ERR("while extracting value from 'activateForward' in 'SetForwarding'\n");
				goto error;
			}
			LM_INFO("got 'activateForward'=%s in 'SetForwarding'\n",fwdact);
		}
		param = libxml_api.xmlNodeGetNodeByName(top_elem, "device", NULL);
		if(param!= NULL) {
			device= (char*)xmlNodeGetContent(param);
			if(device== NULL)  {
				LM_ERR("while extracting value from 'device' in 'SetForwarding'\n");
				goto error;
			}
			LM_ERR("got 'device'=%s in 'SetDoNotDisturb'\n",device);
		}
		pkg_free(body.s);
	}

	xmlFreeDoc(doc);
	xmlCleanupParser();
	xmlMemoryDump();
	return 1;

error:
	xmlFreeDoc(doc);
	xmlCleanupParser();
	xmlMemoryDump();
	return -1;

}
