/*
 * CGItable.cpp
 *
 *  Created on: Aug 5, 2024
 *      Author: dig
 */

#include "cgiScripts.h"
#include "sensorTask.h"
#include "CGIcommonScripts.h"
#include "CGItable.h"
#include "log.h"

#include <cstddef>

const tCGI CGIurls[] = {
	{"/cgi-bin/getLogMeasValues", (tCGIHandler_t) readCGIvalues, (CGIresponseFileHandler_t) getAllLogsScript},
	{"/cgi-bin/getRTMeasValues", (tCGIHandler_t)readCGIvalues, (CGIresponseFileHandler_t)getRTMeasValuesScript},
	{"/cgi-bin/getSensorInfo", (tCGIHandler_t)readCGIvalues, (CGIresponseFileHandler_t)getSensorInfoScript},
	{"/cgi-bin/getCommonInfo", (tCGIHandler_t)readCGIvalues, (CGIresponseFileHandler_t)getCommonInfoScript},
	{"/cgi-bin/getSettings", (tCGIHandler_t)readCGIvalues, (CGIresponseFileHandler_t)getSettingsScript},
	{"/cgi-bin/getFanInfo", (tCGIHandler_t)readCGIvalues, (CGIresponseFileHandler_t)getFanInfoScript},
	{"/cgi-bin/clearLog", (tCGIHandler_t)readCGIvalues, (CGIresponseFileHandler_t)clearLogScript},
	{NULL, NULL, NULL} // last
};
