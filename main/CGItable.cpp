/*
 * CGItable.cpp
 *
 *  Created on: Aug 5, 2024
 *      Author: dig
 */

#include "cgiScripts.h"
#include "sensorTask.h"
#include "CGItable.h"
#include "log.h"

#include <cstddef>

const tCGI CGIurls[] = {
	{"/cgi-bin/getLogMeasValues", (tCGIHandler_t) readCGIvalues, (CGIresponseFileHandler_t) getAllLogsScript},
	{"/cgi-bin/getRTMeasValues", (tCGIHandler_t)readCGIvalues, (CGIresponseFileHandler_t)getRTMeasValuesScript},
	{"/cgi-bin/getSensorStatus", (tCGIHandler_t)readCGIvalues, (CGIresponseFileHandler_t)getSensorStatusScript},
	{"/cgi-bin/clearLog", (tCGIHandler_t)readCGIvalues, (CGIresponseFileHandler_t)clearLogScript},
	{NULL, NULL, NULL} // last
};
